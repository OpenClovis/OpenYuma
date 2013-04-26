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
/*  FILE: netconf-subsystem.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
14-jan-07    abb      begun;
03-mar-11    abb      get rid of usleeps and replace with
                      design that checks for EAGAIN

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <stdarg.h>

#define _C_main 1

#include "procdefs.h"

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_agt
#include "agt.h"
#endif

#ifndef _H_agt_ncxserver
#include "agt_ncxserver.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_send_buff
#include "send_buff.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#define BUFFLEN  2000

#define MAX_READ_TRIES 1000

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static char *client_addr;
static struct sockaddr_un ncxname;
static int    ncxsock;
static char *user;
static char *port;

static int traceLevel = 0;
static FILE *errfile;

#define SUBSYS_TRACE1(fmt, ...) if (traceLevel && errfile) \
                                { \
                                    fprintf( errfile, fmt, ##__VA_ARGS__); \
                                    fflush( errfile ); \
                                }

#define SUBSYS_TRACE2(fmt, ...) if (traceLevel > 1 && errfile) \
                                { \
                                    fprintf( errfile, fmt, ##__VA_ARGS__); \
                                    fflush( errfile ); \
                                }

#define SUBSYS_TRACE3(fmt, ...) if (traceLevel > 2 && errfile) \
                                { \
                                    fprintf( errfile, fmt, ##__VA_ARGS__); \
                                    fflush( errfile ); \
                                }

static boolean ncxconnect;
static char msgbuff[BUFFLEN];

/******************************************************************
 * FUNCTION configure_logging
 *
 * Configure debug logging. This function evaluates command line
 * arguments to configure debug logging.
 *
 ******************************************************************/
static void configure_logging( int argc, char** argv )
{
    char* err_filename;
    int arg_idx = 1;
    char  defname[21];

    strncpy(defname, "/tmp/subsys-err.log", 20);
    err_filename = defname;
    while ( arg_idx < argc-1 ) {
        if ( !strcmp( argv[arg_idx], "-filename" ) ||
             !strcmp( argv[arg_idx], "-f" ) ) {
            err_filename = argv[++arg_idx];
            if ( !traceLevel )
            {
                traceLevel = 1;
            }
        }
        else if ( !strcmp( argv[arg_idx], "-trace" ) ||
                  !strcmp( argv[arg_idx], "-t" ) ) {
            traceLevel = atoi( argv[++arg_idx] );
        }
        ++arg_idx;
    }

    if ( traceLevel ) {
        errfile = fopen( err_filename, "a" );
        SUBSYS_TRACE1( "\n*** New netconf Session Started ***\n" );
    }

}

/********************************************************************
* FUNCTION init_subsys
*
* Initialize the subsystem, and get it ready to send and receive
* the first message of any kind
* 
* RETURNS:
*   status
*********************************************************************/
static status_t
    init_subsys (void)
{
    char *cp, *con;
    int   ret;

    client_addr = NULL;
    port = NULL;
    user = NULL;
    ncxsock = -1;
    ncxconnect = FALSE;

    /* get the client address */
    con = getenv("SSH_CONNECTION");
    if (!con) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): "
                       "Get SSH_CONNECTION variable failed\n" );
        return ERR_INTERNAL_VAL;
    }

    /* get the client addr */
    client_addr = strdup(con);
    if (!client_addr) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): strdup(client_addr) failed\n" );
        return ERR_INTERNAL_MEM;
    }
    cp = strchr(client_addr, ' ');
    if (!cp) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): "
                       "Malformed SSH_CONNECTION variable\n" );
        return ERR_INTERNAL_VAL;
    } else {
        *cp = 0;
    }

    /* get the server connect port */
    cp = strrchr(con, ' ');
    if (cp && cp[1]) {
        port = strdup(++cp);
    }
    if (!port) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): "
                       "Malformed SSH_CONNECTION variable\n" );
        return ERR_INTERNAL_VAL;
    }
        
    /* get the username */
    cp = getenv("USER");
    if (!cp) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): Get USER variable failed\n");
        return ERR_INTERNAL_VAL;
    }
    user = strdup(cp);
    if (!user) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): strdup(user) failed\n" );
        return ERR_INTERNAL_MEM;
    }

    /* make a socket to connect to the NCX server */
    ncxsock = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (ncxsock < 0) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): NCX Socket Creation failed\n" );
        return ERR_NCX_CONNECT_FAILED;
    } 
    ncxname.sun_family = AF_LOCAL;
    strncpy(ncxname.sun_path, 
            NCXSERVER_SOCKNAME, 
            sizeof(ncxname.sun_path));

    /* try to connect to the NCX server */
    ret = connect(ncxsock,
                  (const struct sockaddr *)&ncxname,
                  SUN_LEN(&ncxname));
    if (ret != 0) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): NCX Socket Connect failed\n" );
        return ERR_NCX_OPERATION_FAILED;
    } else {
        SUBSYS_TRACE2( "INFO:  init_subsys(): "
                       "NCX Socket Connected on FD: %d \n", ncxsock );
        ncxconnect = TRUE;
    }

#ifdef USE_NONBLOCKING_IO
    /* set non-blocking IO */
    if (fcntl(ncxsock, F_SETFD, O_NONBLOCK)) {
        SUBSYS_TRACE1( "ERROR: init_subsys(): fnctl() failed\n" );
    }
#endif

    /* connected to the ncxserver and setup the ENV vars ok */
    return NO_ERR;

} /* init_subsys */


/********************************************************************
* FUNCTION cleanup_subsys
*
* Cleanup the subsystem
* 
*********************************************************************/
static void
    cleanup_subsys (void)
{
    if (client_addr) {
        m__free(client_addr);
    }
    if (user) {
        m__free(user);
    }
    if (ncxconnect) {
        close(ncxsock);
    }

    if (errfile) {
        fclose(errfile);
    }
} /* cleanup_subsys */


/********************************************************************
* FUNCTION send_ncxconnect
*
* Send the <ncx-connect> message to the ncxserver
* 
* RETURNS:
*   status
*********************************************************************/
static status_t
    send_ncxconnect (void)
{
    status_t  res;
    char connectmsg[] = 
    "%s\n<ncx-connect xmlns=\"%s\" version=\"1\" user=\"%s\" "
    "address=\"%s\" magic=\"%s\" transport=\"ssh\" port=\"%s\" />\n%s";

    memset(msgbuff, 0x0, BUFFLEN);
    snprintf(msgbuff, BUFFLEN, connectmsg, (const char *)XML_START_MSG, 
             NCX_URN, user, client_addr, NCX_SERVER_MAGIC, port, NC_SSH_END);

    res = send_buff(ncxsock, msgbuff, strlen(msgbuff));
    return res;

} /* send_ncxconnect */


/********************************************************************
* FUNCTION do_read
*
* Read from a FD
* 
* INPUTS:
*              
* RETURNS:
*   return byte count
*********************************************************************/
static ssize_t
    do_read (int readfd, 
             char *readbuff, 
             size_t readcnt, 
             status_t *retres)
{
    boolean   readdone;
    ssize_t   retcnt;

    readdone = FALSE;
    retcnt = 0;
    *retres = NO_ERR;

    while (!readdone && *retres == NO_ERR) {
        retcnt = read(readfd, readbuff, readcnt);
        if (retcnt < 0) {
            if (errno != EAGAIN) {
                SUBSYS_TRACE1( "ERROR: do_read(): read of FD(%d): "
                               "failed with error: %s\n", 
                               readfd, strerror( errno  ) );
                *retres = ERR_NCX_READ_FAILED;
                continue;
            }
        } else if (retcnt == 0) {
            SUBSYS_TRACE1( "INFO: do_read(): closed connection\n");
            *retres = ERR_NCX_EOF;
            readdone = TRUE;
            continue;
        } else {
            /* retcnt is the number of bytes read */
            readdone = TRUE;
        }
    }  /*end readdone loop */

    return retcnt;

}  /* do_read */


/********************************************************************
* FUNCTION io_loop
*
* Handle the IO for the program
* 
* INPUTS:
*              
* RETURNS:
*   status
*********************************************************************/
static status_t
    io_loop (void)
{
    status_t  res;
    boolean   done;
    fd_set    fds;
    int       ret;
    ssize_t   retcnt;

    res = NO_ERR;
    done = FALSE;
    FD_ZERO(&fds);

    while (!done) {
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(ncxsock, &fds);

        ret = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            if ( errno != EINTR ) {
                SUBSYS_TRACE1( "ERROR: io_loop(): select() "
                               "failed with error: %s\n", strerror( errno ) );
                res = ERR_NCX_OPERATION_FAILED;
                done = TRUE;
            }
            else
            {
                SUBSYS_TRACE2( "INFO: io_loop(): select() "
                               "failed with error: %s\n", strerror( errno  ) );
            }
            continue;
        } else if (ret == 0) {
            SUBSYS_TRACE1( "ERROR: io_loop(): select() "
                           "returned 0, exiting...\n" );
            res = NO_ERR;
            done = TRUE;
            continue;
        } /* else some IO to process */

        /* check any input from client */
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            /* get buff from openssh */
            retcnt = do_read(STDIN_FILENO, 
                             msgbuff, 
                             (size_t)BUFFLEN,
                             &res);

            if (res == ERR_NCX_EOF) {
                res = NO_ERR;
                done = TRUE;
                continue;
            } else if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            } else if (res == NO_ERR && retcnt > 0) {
                /* send this buffer to the ncxserver */
                res = send_buff(ncxsock, msgbuff, (size_t)retcnt);
                if (res != NO_ERR) {
                    SUBSYS_TRACE1( "ERROR: io_loop(): send_buff() to ncxserver "
                                   "failed with %s\n", strerror( errno ) );
                    done = TRUE;
                    continue;
                }
            }
        }  /* if STDIN set */

        /* check any input from the ncxserver */
        if (FD_ISSET(ncxsock, &fds)) {
            res = NO_ERR;
            retcnt = do_read(ncxsock, 
                             msgbuff, 
                             (size_t)BUFFLEN,
                             &res);

            if (res == ERR_NCX_EOF) {
                res = NO_ERR;
                done = TRUE;
                continue;
            } else if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            } else if (res == NO_ERR && retcnt > 0) {
                /* send this buffer to STDOUT */
                res = send_buff(STDOUT_FILENO, msgbuff, (size_t)retcnt);
                if (res != NO_ERR) {
                    SUBSYS_TRACE1( "ERROR: io_loop(): send_buff() to client "
                                   "failed with %s\n", strerror( errno ) );
                    done = TRUE;
                    continue;
                }
            }
        }
    }

    return res;

} /* io_loop */


/********************************************************************
* FUNCTION main
*
* STDIN is input from the SSH client (sent to ncxserver)
* STDOUT is output to the SSH client (rcvd from ncxserver)
* 
* RETURNS:
*   0 if NO_ERR
*   1 if error connecting or logging into ncxserver
*********************************************************************/
int main (int argc, char **argv) 
{
    status_t  res;
    const char *msg;

    configure_logging( argc, argv );

    res = init_subsys();
    if (res != NO_ERR) {
        msg = "init failed";
    }

    if (res == NO_ERR) {
        res = send_ncxconnect();
        if (res != NO_ERR) {
            msg = "connect failed";
        }
    }

    if (res == NO_ERR) {
        res = io_loop();
        if (res != NO_ERR) {
            msg = "IO error";
        }
    }

    if (res != NO_ERR) {
        SUBSYS_TRACE1( "ERROR: io_loop(): exited with error %s \n", msg );
    }

    cleanup_subsys();

    if (res != NO_ERR) {
        return 1;
    } else {
        return 0;
    }

} /* main */

