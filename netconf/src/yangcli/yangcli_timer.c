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
/*  FILE: yangcli_timer.c

   NETCONF YANG-based CLI Tool

   timers support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
26-may-11    abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "libtecla.h"

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_ses
#include "mgr_ses.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_feature
#include "ncx_feature.h"
#endif

#ifndef _H_ncx_list
#include "ncx_list.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_var
#include "var.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xml_val
#include "xml_val.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifndef _H_yangcli_autoload
#include "yangcli_autoload.h"
#endif

#ifndef _H_yangcli_cmd
#include "yangcli_cmd.h"
#endif

#ifndef _H_yangcli_timer
#include "yangcli_timer.h"
#endif

#ifndef _H_yangcli_util
#include "yangcli_util.h"
#endif

#ifdef DEBUG
#define YANGCLI_TIMER_DEBUG 1
#endif


/* macros from http://linux.die.net/man/2/gettimeofday */
#define timerisset(tvp)\
        ((tvp)->tv_sec || (tvp)->tv_usec)
#define timer_clear(tvp)\
        ((tvp)->tv_sec = (tvp)->tv_usec = 0)


/********************************************************************
 * FUNCTION timer_running
 * 
 * Check if a timer is running
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    timer_id == timer to use
 *
 * RETURNS:
 *   TRUE if timer running; FALSE otherwise
 *********************************************************************/
static boolean
    timer_running (server_cb_t *server_cb,
                   uint8 timer_id)
{
    return (timerisset(&server_cb->timers[timer_id])) ? TRUE : FALSE;

}  /* timer_running */


/********************************************************************
 * FUNCTION set_timer
 * 
 * Set timer to now
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    timer_id == timer to use
 *********************************************************************/
static void
    set_timer (server_cb_t *server_cb,
               uint8 timer_id)
{
    gettimeofday(&server_cb->timers[timer_id], NULL);

}  /* set_timer */


/********************************************************************
 * FUNCTION clear_timer
 * 
 * Clear a timer
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    timer_id == timer to use
 *
 *********************************************************************/
static void
    clear_timer (server_cb_t *server_cb,
                 uint8 timer_id)
{
    timer_clear(&server_cb->timers[timer_id]);

}  /* clear_timer */


/********************************************************************
 * FUNCTION clear_timer
 * 
 * Clear a timer
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    timer_id == timer to use
 *    result == address of return timer value
 * OUTPUTS:
 *    *result filled in with timer value
 *********************************************************************/
static void
    get_timer (server_cb_t *server_cb,
               uint8 timer_id,
               struct timeval *result)
{
    *result = server_cb->timers[timer_id];

}  /* get_timer */


/********************************************************************
 * FUNCTION yangcli_timer_start (local RPC)
 * 
 * timer-start [timernum] [restart-ok=false]
 *
 * Start the specified performance timer
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    yangcli_timer_start (server_cb_t *server_cb,
                         obj_template_t *rpc,
                         const xmlChar *line,
                         uint32  len)
{
    val_value_t        *valset, *parm;
    status_t            res;
    boolean             restart_ok;
    uint8               timer_id;

    res = NO_ERR;
    timer_id = 0;
    restart_ok = FALSE;

    valset = get_valset(server_cb, rpc, &line[len], &res);

    /* check if the 'id' field is set */
    if (res == NO_ERR) {
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_ID);
        if (parm && parm->res == NO_ERR) {
            timer_id = VAL_UINT8(parm);
        } else {
            log_error("\nError: missing 'id' parameter");
            res = ERR_NCX_MISSING_PARM;
        }
    }

    /* check if the 'restart-ok' field is set */
    if (res == NO_ERR) {
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_RESTART_OK);
        if (parm && parm->res == NO_ERR) {
            restart_ok = VAL_BOOL(parm);
        } else {
            restart_ok = TRUE;
        }
    }

    if (res == NO_ERR) {
        if (!restart_ok && timer_running(server_cb, timer_id)) {
            log_error("\nError: timer '%u' is already running", timer_id);
            res = ERR_NCX_IN_USE;
        }
    }

    if (res == NO_ERR) {
        set_timer(server_cb, timer_id);
        log_info("\nOK\n");
    }

    if (valset != NULL) {
        val_free_value(valset);
    }

    return res;

} /* yangcli_timer_start */


/********************************************************************
 * FUNCTION yangcli_timer_stop (local RPC)
 * 
 * delta = timer-stop [timernum] [echo=false]
 *
 * Start the specified performance timer
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    yangcli_timer_stop (server_cb_t *server_cb,
                        obj_template_t *rpc,
                        const xmlChar *line,
                        uint32  len)
{
    val_value_t        *valset, *parm;
    obj_template_t     *obj;
    struct timeval      now, timerval;
    status_t            res;
    long int            sec, usec;
    boolean             imode, echo;
    xmlChar             numbuff[NCX_MAX_NUMLEN];
    uint8               timer_id;

    gettimeofday(&now, NULL);

    timer_id = 0;
    res = NO_ERR;
    imode = interactive_mode();
    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (res == NO_ERR) {
        obj = obj_find_child(rpc, obj_get_mod_name(rpc), YANG_K_OUTPUT);
        if (obj == NULL) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            obj = obj_find_child(obj, 
                                 obj_get_mod_name(obj), 
                                 YANGCLI_DELTA);
            if (obj == NULL) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }

    /* check if the 'id' field is set */
    if (res == NO_ERR) {
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_ID);
        if (parm && parm->res == NO_ERR) {
            timer_id = VAL_UINT8(parm);
        } else {
            log_error("\nError: missing 'id' parameter");
            res = ERR_NCX_MISSING_PARM;
        }
    }

    /* make sure timer is running */
    if (res == NO_ERR) {
        if (!timer_running(server_cb, timer_id)) {
            log_error("\nError: timer '%u' is not running",
                      timer_id);
            res = ERR_NCX_OPERATION_FAILED;
        }
    }

    if (res == NO_ERR) {
        get_timer(server_cb, timer_id, &timerval);

        /* check if the 'echo' field is set */
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_ECHO);
        if (parm && parm->res == NO_ERR) {
            echo = VAL_BOOL(parm);
        } else {
            echo = TRUE;
        }

        /* get the resulting delta */
        if (now.tv_usec < timerval.tv_usec) {
            now.tv_usec += 1000000;
            now.tv_sec--;
        }
        sec = now.tv_sec - timerval.tv_sec;
        usec = now.tv_usec - timerval.tv_usec;

        sprintf((char *)numbuff, "%ld.%06ld", sec, usec);

        if (echo) {
            if (imode) {
                log_stdout("\nTimer %u value: %s seconds\n", 
                           timer_id,
                           numbuff);
                if (log_get_logfile() != NULL) {
                    log_write("\nTimer %u value: %s seconds\n", 
                              timer_id,
                              numbuff);
                }
            } else {
                log_write("\nTimer %u value: %s seconds\n", 
                          timer_id,
                          numbuff);
            }
        }

        if (server_cb->local_result != NULL) {
            log_debug3("\nDeleting old local result");
            val_free_value(server_cb->local_result);
        }

        server_cb->local_result = 
            val_make_simval_obj(obj, numbuff, &res);
        if (res != NO_ERR) {
            log_error("\nError: set value failed (%s)",
                      get_error_string(res));
        }
    }

    if (valset != NULL) {
        val_free_value(valset);
    }

    clear_timer(server_cb, timer_id);
    return res;

} /* yangcli_timer_stop */


/* END yangcli_timer.c */
