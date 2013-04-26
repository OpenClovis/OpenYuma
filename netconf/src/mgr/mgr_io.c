/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: mgr_io.c

     1) call mgr_io_init

     2) call mgr_io_set_stdin_handler

     3a) When a session socket has been created,
        call mgr_io_activate_session.
     3b) When a session socket is being closed,
        call mgr_io_deactivate_session.

     4) call mgr_io_run to loop until program exits

     Step 2 or 3 can occur N times, and be changed while
     mgr_io_run is active (i.e., via stdin_handler)

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
18-feb-07    abb      begun; start from agt_ncxserver.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

     
#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_def_reg
#include  "def_reg.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_mgr
#include  "mgr.h"
#endif

#ifndef _H_mgr_io
#include "mgr_io.h"
#endif

#ifndef _H_mgr_ses
#include  "mgr_ses.h"
#endif

#ifndef _H_ncx
#include  "ncx.h"
#endif

#ifndef _H_ncxconst
#include  "ncxconst.h"
#endif

#ifndef _H_ses
#include  "ses.h"
#endif

#ifndef _H_ses_msg
#include  "ses_msg.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xmlns
#include  "xmlns.h"
#endif

/********************************************************************
 *                                                                   *
 *                       C O N S T A N T S                           *
 *                                                                   *
 *********************************************************************/
#ifdef DEBUG
/* #define MGR_IO_DEBUG 1 */
#endif


/********************************************************************
 *                                                                  *
 *                       V A R I A B L E S                          *
 *                                                                  *
 ********************************************************************/

static mgr_io_stdin_fn_t  stdin_handler;

static fd_set active_fd_set, read_fd_set, write_fd_set;

static int maxwrnum;
static int maxrdnum;


/********************************************************************
 * FUNCTION any_fd_set
 * 
 * Check if any bits are set in the fd_set
 * INPUTS:
 *   fs == fs_set to check
 *   maxfd == max FD number possibly in use
 *
 * RETURNS:
 *   TRUE if any bits set
 *   FALSE if no bits set
 *********************************************************************/
static boolean
    any_fd_set (fd_set *fd,
                int maxfd)
{
    int  i;

    for (i=0; i<=maxfd; i++) {
        if (FD_ISSET(i, fd)) {
            return TRUE;
        }
    }
    return FALSE;

} /* any_fd_set */


/********************************************************************
 * FUNCTION write_sessions
 * 
 * Go through any sessions ready to write  and send
 * the buffers ready to send
 *
 *********************************************************************/
static void
    write_sessions (void)
{
    ses_cb_t      *scb;
    mgr_scb_t     *mscb;
    status_t       res;

    scb = mgr_ses_get_first_outready();
    while (scb) {
        res = ses_msg_send_buffs(scb);
        if (res != NO_ERR) {
            if (LOGINFO) {
                mscb = mgr_ses_get_mscb(scb);
                log_info("\nmgr_io write failed; "
                         "closing session %u (a:%u)", 
                         scb->sid,
                         mscb->agtsid);
            }
            mgr_ses_free_session(scb->sid);
            scb = NULL;
        }
        /* check if any buffers left over for next loop */
        if (scb && !dlq_empty(&scb->outQ)) {
            ses_msg_make_outready(scb);
        }

        scb = mgr_ses_get_first_outready();
    }

}  /* write_sessions */


/********************************************************************
 * FUNCTION read_session
 * 
 *
 *
 *********************************************************************/
static boolean
    read_session (int fd,
                  ses_id_t cursid)
{
    ses_cb_t      *scb;
    mgr_scb_t     *mscb;
    status_t       res;
    boolean        retval;

    retval = TRUE;

    if (LOGDEBUG3) {
        log_debug3("\nmgr_io: read FD %d for session %u start",
                   fd,
                   cursid);
    }

    /* Data arriving on an already-connected socket.
     * Need to have the xmlreader for this session
     * unless it is input from STDIO
     */
    scb = def_reg_find_scb(fd);
    if (scb) {
        mscb = mgr_ses_get_mscb(scb);
        if (!mscb) {
            SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            res = ses_accept_input(scb);
            if (res == NO_ERR) {
                if (mscb->returncode == LIBSSH2_ERROR_EAGAIN) {
                    /* this is assumed to be the openssh
                     * keepalive message, requesting
                     * some data be sent on the channel
                     */
                }
            } else {
                /* accept input failed because session died */
                if (scb->sid == cursid) {
                    retval = FALSE;
                }

                if (res != ERR_NCX_SESSION_CLOSED) {
                    if (LOGDEBUG) {
                        log_debug("\nmgr_io: input failed"
                                 " for session %u (a:%u) tr-err:%d (%s)",
                                  scb->sid, 
                                  mscb->agtsid,
                                  mscb->returncode,
                                  get_error_string(res));
                    }
                }

                mgr_ses_free_session(scb->sid);
                FD_CLR(fd, &active_fd_set);
                if (fd >= maxrdnum) {
                    maxrdnum = fd-1;
                }
            }
        }
    }

    return retval;

}  /* read_session */


/**************   E X T E R N A L    F U N C T I O N S  *************/


/********************************************************************
 * FUNCTION mgr_io_init
 * 
 * Init the IO server loop vars for the ncx manager
 * Must call this function before any other function can
 * be used from this module
 *********************************************************************/
void
    mgr_io_init (void)
{
    stdin_handler = NULL;
    FD_ZERO(&write_fd_set);
    FD_ZERO(&active_fd_set);
    maxwrnum = 0;
    maxrdnum = 0;

} /* mgr_io_init */


/********************************************************************
 * FUNCTION mgr_io_set_stdin_handler
 * 
 * Set the callback function for STDIN processing
 * 
 * INPUTS:
 *   handler == address of the STDIN handler function
 *********************************************************************/
void
    mgr_io_set_stdin_handler (mgr_io_stdin_fn_t handler)
{
    stdin_handler = handler;

} /* mgr_io_set_stdin_handler */


/********************************************************************
 * FUNCTION mgr_io_activate_session
 * 
 * Tell the IO manager to start listening on the specified socket
 * 
 * INPUTS:
 *   fd == file descriptor number of the socket
 *********************************************************************/
void
    mgr_io_activate_session (int fd)
{
    FD_SET(fd, &active_fd_set);

    /*
    if (fd+1 > maxwrnum) {
        maxwrnum = fd+1;
    }
    */

    if (fd+1 > maxrdnum) {
        maxrdnum = fd+1;
    }

} /* mgr_io_activate_session */


/********************************************************************
 * FUNCTION mgr_io_deactivate_session
 * 
 * Tell the IO manager to stop listening on the specified socket
 * 
 * INPUTS:
 *   fd == file descriptor number of the socket
 *********************************************************************/
void
    mgr_io_deactivate_session (int fd)
{
    FD_CLR(fd, &active_fd_set);

    /*
    if (fd+1 == maxwrnum) {
        maxwrnum--;
    }
    */

    if (fd+1 == maxrdnum) {
        maxrdnum--;
    }

} /* mgr_io_deactivate_session */


/********************************************************************
 * FUNCTION mgr_io_run
 * 
 * IO server loop for the ncx manager
 * 
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    mgr_io_run (void)
{
    struct timeval timeout;
    int            i, ret;
    boolean        done, done2;
    mgr_io_state_t state;

    state = MGR_IO_ST_INIT;

    /* first loop, handle user IO and get some data to read or write */
    done = FALSE;
    while (!done) {

        /* check exit program */
        if (mgr_shutdown_requested()) {
            done = TRUE;
            continue;
        }

        done2 = FALSE;
        ret = 0;

        while (!done2) {
            /* will block in idle states waiting for user KBD input
             * while no command is active
             */
            if (stdin_handler != NULL) {
                state = (*stdin_handler)();
            }

            /* check exit program or session */
            if (mgr_shutdown_requested()) {
                done = done2 = TRUE;
                continue;
            } else if (state == MGR_IO_ST_CONN_SHUT) {
                done2 = TRUE;
                continue;
            } else if (state == MGR_IO_ST_SHUT) {
                done = done2 = TRUE;
                continue;
            }

            write_sessions();

            switch (state) {
            case MGR_IO_ST_INIT:
            case MGR_IO_ST_IDLE:
                /* user didn't do anything at the KBD
                 * skip the IO loop and try again
                 */
                continue;
            case MGR_IO_ST_CONNECT:
            case MGR_IO_ST_CONN_START:
            case MGR_IO_ST_CONN_IDLE:
            case MGR_IO_ST_CONN_RPYWAIT:
            case MGR_IO_ST_CONN_CANCELWAIT:
            case MGR_IO_ST_CONN_CLOSEWAIT:
                /* input could be expected */
                break;
            case MGR_IO_ST_CONN_SHUT:
                /* session shutdown in progress */
                done2 = TRUE;
                continue;
            case MGR_IO_ST_SHUT:
                /* shutdown in progress */
                done = done2 = TRUE;
                continue;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
                done = done2 = TRUE;
            }

            /* check error exit from this loop */
            if (done2) {
                continue;
            }

            /* setup select parameters */
            read_fd_set = active_fd_set;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100;

            /* check if there are no sessions active to wait for,
             * so just go back to the STDIN handler
             */
            if (!any_fd_set(&read_fd_set, maxrdnum)) {
                continue;
            }

#ifdef MGR_IO_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nmgr_io: enter select (%u:%u)",
                           timeout.tv_sec,
                           timeout.tv_usec);
            }
#endif

            /* Block until input arrives on one or more active sockets. 
             * or the timer expires
             */
            ret = select(maxrdnum+1, 
                         &read_fd_set, 
                         &write_fd_set, 
                         NULL, 
                         &timeout);

#ifdef MGR_IO_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nmgr_io: exit select (%u:%u)",
                           timeout.tv_sec,
                           timeout.tv_usec);
            }
#endif

            if (ret > 0) {
                /* normal return with some bytes */
                done2 = TRUE;
            } else if (ret < 0) {
                if (!(errno == EINTR || errno==EAGAIN)) {
                    done2 = TRUE;
                }  /* else go again in the inner loop */
            } else {
                /* should only happen if a timeout occurred */
                if (mgr_shutdown_requested()) {
                    done2 = TRUE; 
                }
            }
        }  /* end inner loop */

        /* check exit program */
        if (mgr_shutdown_requested()) {
            done = TRUE;
            continue;
        }

        /* check select return status for non-recoverable error */
        if (ret < 0) {
            log_error("\nmgr_io select failed (%s)", 
                      strerror(errno));
            mgr_request_shutdown();
            done = TRUE;
            continue;
        }
     
        /* 2nd loop: go through the file descriptor numbers and
         * service all the sockets with input pending 
         */
        for (i = 0; i <= maxrdnum; i++) {
            /* check read input from agent */
            if (FD_ISSET(i, &read_fd_set)) {
                (void)read_session(i, 0);
            }
        }

        /* drain the ready queue before accepting new input */
        if (!done) {
            done2 = FALSE;
            while (!done2) {
                if (!mgr_ses_process_first_ready()) {
                    done2 = TRUE;
                } else if (mgr_shutdown_requested()) {
                    done = done2 = TRUE;
                }
            }
            write_sessions();
        }
    }  /* end outer loop */

    /* all open client sockets will be closed as the sessions are
     * torn down, but the original ncxserver socket needs to be closed now
     */
    return NO_ERR;

}  /* mgr_io_run */


/********************************************************************
 * FUNCTION mgr_io_process_timeout
 * 
 * mini server loop while waiting for KBD input
 * 
 * INPUTS:
 *    cursid == current session ID to check
 *    wantdata == address of return wantdata flag
 *    anystdout == address of return anystdout flag
 *
 * OUTPUTS:
 *   *wantdata == TRUE if the agent has sent a keepalive
 *                and is expecting a request
 *             == FALSE if no keepalive received this time
 *  *anystdout  == TRUE if maybe STDOUT was written
 *                 FALSE if definately no STDOUT written
 *
 * RETURNS:
 *   TRUE if session alive or not confirmed
 *   FALSE if cursid confirmed dropped
 *********************************************************************/
boolean
    mgr_io_process_timeout (ses_id_t  cursid,
                            boolean *wantdata,
                            boolean *anystdout)
{
    struct timeval  timeout;
    int             i, ret;
    boolean         done, retval, anyread;

#ifdef DEBUG
    if (wantdata == NULL || anystdout == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TRUE;
    }
#endif

    /* !!! the client keepalive polling is not implemented !!!! */
    *wantdata = FALSE;
    *anystdout = FALSE;

    write_sessions();

    /* setup select parameters */
    anyread = FALSE;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    read_fd_set = active_fd_set;

    if (!any_fd_set(&read_fd_set, maxrdnum)) {
        return TRUE;
    }

    /* Block until input arrives on one or more active sockets. 
     * or the short timer expires
     */
    ret = select(maxrdnum+1, 
                 &read_fd_set, 
                 &write_fd_set, 
                 NULL, 
                 &timeout);
    if (ret > 0) {
        /* normal return with some bytes */
        anyread = TRUE;
    } else if (ret < 0) {
        /* some error, don't care about EAGAIN here */
        return TRUE;
    } else {
        /* == 0: timeout */
        return TRUE;
    }

    retval = TRUE;

    /* loop: go through the file descriptor numbers and
     * service all the sockets with input pending 
     */
    if (anyread) {
        for (i = 0; i <= maxrdnum; i++) {
            
            /* check read input from agent */
            if (FD_ISSET(i, &read_fd_set)) {
                retval = read_session(i, cursid);
            }
        }
    }

    /* drain the ready queue before accepting new input */
    if (anyread) {
        done = FALSE;
        while (!done) {
            if (!mgr_ses_process_first_ready()) {
                /* did not write to any session */
                anyread = FALSE;  
                done = TRUE;
            } else if (mgr_shutdown_requested()) {
                done = TRUE;
            }
        }
        write_sessions();
    }

    *anystdout = anyread;
    return retval;

}  /* mgr_io_process_timeout */


/* END mgr_io.c */



