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
/*  FILE: yangcli_autolock.c

   NETCONF YANG-based CLI Tool

   autolock support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13-aug-09    abb      begun; started from yangcli.c

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

#ifndef _H_yangcli_autolock
#include "yangcli_autolock.h"
#endif

#ifndef _H_yangcli_cmd
#include "yangcli_cmd.h"
#endif

#ifndef _H_yangcli_util
#include "yangcli_util.h"
#endif


/********************************************************************
* FUNCTION setup_lock_cbs
* 
* Setup the lock state info in all the lock control blocks
* in the specified server_cb; call when a new sesion is started
* 
* INPUTS:
*  server_cb == server control block to use
*********************************************************************/
static void
    setup_lock_cbs (server_cb_t *server_cb)
{
    ses_cb_t     *scb;
    mgr_scb_t    *mscb;
    ncx_cfg_t     cfg_id;

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (scb == NULL) {
        log_error("\nError: active session dropped, cannot lock");
        return;
    }

    mscb = (mgr_scb_t *)scb->mgrcb;
    server_cb->locks_active = TRUE;
    server_cb->locks_waiting = FALSE;
    server_cb->locks_cur_cfg = NCX_CFGID_RUNNING;

    for (cfg_id = NCX_CFGID_RUNNING;
         cfg_id <= NCX_CFGID_STARTUP;
         cfg_id++) {

        server_cb->lock_cb[cfg_id].lock_state = LOCK_STATE_IDLE;
        server_cb->lock_cb[cfg_id].lock_used = FALSE;
        server_cb->lock_cb[cfg_id].start_time = (time_t)0;
        server_cb->lock_cb[cfg_id].last_msg_time = (time_t)0;
    }

    /* always request the lock on running */
    server_cb->lock_cb[NCX_CFGID_RUNNING].lock_used = TRUE;

    server_cb->lock_cb[NCX_CFGID_CANDIDATE].lock_used = 
        (cap_std_set(&mscb->caplist, CAP_STDID_CANDIDATE))
        ? TRUE : FALSE;

    server_cb->lock_cb[NCX_CFGID_STARTUP].lock_used =
        (cap_std_set(&mscb->caplist, CAP_STDID_STARTUP))
        ? TRUE : FALSE;

}  /* setup_lock_cbs */


/********************************************************************
* FUNCTION setup_unlock_cbs
* 
* Setup the lock state info in all the lock control blocks
* in the specified server_cb; call when release-locks or cleanup
* is releasing all the locks gained so far
* 
* INPUTS:
*     server_cb == server control block to use
*
* RETURNS:
*   TRUE if sending unlocks needed
*   FALSE if sending unlocks not needed
*********************************************************************/
static boolean
    setup_unlock_cbs (server_cb_t *server_cb)
{
    boolean       needed;
    ncx_cfg_t     cfg_id;

    if (!server_cb->locks_active) {
        return FALSE;
    }

    needed = FALSE;

    for (cfg_id = NCX_CFGID_RUNNING;
         cfg_id <= NCX_CFGID_STARTUP;
         cfg_id++) {

        server_cb->lock_cb[cfg_id].start_time = (time_t)0;
        server_cb->lock_cb[cfg_id].last_msg_time = (time_t)0;
        if (server_cb->lock_cb[cfg_id].lock_used && 
            server_cb->lock_cb[cfg_id].lock_state == 
            LOCK_STATE_ACTIVE) {
            needed = TRUE;
        }
    }

    return needed;

}  /* setup_unlock_cbs */


/********************************************************************
* FUNCTION send_lock_pdu_to_server
* 
* Send a <lock> or <unlock> operation to the server
*
* INPUTS:
*   server_cb == server control block to use
*   lockcb == lock control block to use within server_cb
*   islock == TRUE for lock; FALSE for unlock
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    send_lock_pdu_to_server (server_cb_t *server_cb,
                            lock_cb_t *lockcb,
                            boolean islock)
{
    obj_template_t        *rpc, *input;
    mgr_rpc_req_t         *req;
    val_value_t           *reqdata, *targetval, *parmval;
    ses_cb_t              *scb;
    status_t               res;
    xmlns_id_t             obj_nsid;

    req = NULL;
    reqdata = NULL;
    res = NO_ERR;

    if (LOGDEBUG) {
        log_debug("\nSending <%s> request",
                  (islock) ? NCX_EL_LOCK : NCX_EL_UNLOCK);
    }

    if (islock) {
        rpc = ncx_find_object(get_netconf_mod(server_cb), 
                              NCX_EL_LOCK);
    } else {
        rpc = ncx_find_object(get_netconf_mod(server_cb), 
                              NCX_EL_UNLOCK);
    }
    if (!rpc) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_nsid = obj_get_nsid(rpc);

    /* get the 'input' section container */
    input = obj_find_child(rpc, NULL, YANG_K_INPUT);
    if (!input) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* construct a method + parameter tree */
    reqdata = xml_val_new_struct(obj_get_name(rpc), obj_nsid);
    if (!reqdata) {
        log_error("\nError allocating a new RPC request");
        return ERR_INTERNAL_MEM;
    }

    /* set the [un]lock/input/target node XML namespace */
    targetval = xml_val_new_struct(NCX_EL_TARGET, obj_nsid);
    if (!targetval) {
        log_error("\nError allocating a new RPC request");
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(targetval, reqdata);
    }

    parmval = xml_val_new_flag(lockcb->config_name,
                               obj_nsid);
    if (!parmval) {
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(parmval, targetval);
    }

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (!scb) {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    } else {
        req = mgr_rpc_new_request(scb);
        if (!req) {
            res = ERR_INTERNAL_MEM;
            log_error("\nError allocating a new RPC request");
        } else {
            req->data = reqdata;
            req->rpc = rpc;
            req->timeout = server_cb->timeout;
        }
    }
        
    /* if all OK, send the RPC request */
    if (res == NO_ERR) {
        if (LOGDEBUG2) {
            log_debug2("\nabout to send RPC request with reqdata:");
            val_dump_value_max(reqdata, 
                               0,
                               server_cb->defindent,
                               DUMP_VAL_LOG,
                               server_cb->display_mode,
                               FALSE,
                               FALSE);
        }

        /* the request will be stored if this returns NO_ERR */
        res = mgr_rpc_send_request(scb, req, yangcli_reply_handler);
        if (res == NO_ERR) {
            if (islock) {
                lockcb->lock_state = LOCK_STATE_REQUEST_SENT;
            } else {
                lockcb->lock_state = LOCK_STATE_RELEASE_SENT;
            }
            (void)time(&lockcb->last_msg_time);
            server_cb->locks_cur_cfg = lockcb->config_id;
        }
    }

    /* cleanup and set next state */
    if (res != NO_ERR) {
        if (req) {
            mgr_rpc_free_request(req);
        } else if (reqdata) {
            val_free_value(reqdata);
        }
    } else {
        server_cb->state = MGR_IO_ST_CONN_RPYWAIT;
    }

    return res;

} /* send_lock_pdu_to_server */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
 * FUNCTION do_get_locks (local RPC)
 * 
 * get all the locks on the server
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the history command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_get_locks (server_cb_t *server_cb,
                  obj_template_t *rpc,
                  const xmlChar *line,
                  uint32  len)
{
    ses_cb_t      *scb;
    val_value_t   *valset, *parm;
    uint32         locks_timeout, retry_interval;
    boolean        cleanup, done;
    status_t       res;

    if (server_cb->locks_active) {
        log_error("\nError: locks are already active");
        return ERR_NCX_OPERATION_FAILED;
    }
    if (server_cb->state != MGR_IO_ST_CONN_IDLE) {
        log_error("\nError: no active session to lock");
        return ERR_NCX_OPERATION_FAILED;
    }

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (scb == NULL) {
        log_error("\nError: active session dropped, cannot lock");
        return ERR_NCX_OPERATION_FAILED;
    }

    locks_timeout = server_cb->locks_timeout;
    retry_interval = server_cb->locks_retry_interval;
    cleanup = TRUE;

    res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);
    if (valset && res == NO_ERR) {
        /* get the overall lock timeout */
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_LOCK_TIMEOUT);
        if (parm && parm->res == NO_ERR) {
            locks_timeout = VAL_UINT(parm);
        }

        /* get the retry interval between failed locks */
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_RETRY_INTERVAL);
        if (parm && parm->res == NO_ERR) {
            retry_interval = VAL_UINT(parm);
        }

        /* get the auto-cleanup flag */
        parm = val_find_child(valset, 
                              YANGCLI_MOD, 
                              YANGCLI_CLEANUP);
        if (parm && parm->res == NO_ERR) {
            cleanup = VAL_BOOL(parm);
        }
    }

    /* start the auto-lock procedure */
    setup_lock_cbs(server_cb);
    server_cb->locks_timeout = locks_timeout;
    server_cb->locks_retry_interval = retry_interval;
    server_cb->locks_cleanup = cleanup;

    done = FALSE;
    if (LOGINFO) {
        log_info("\nSending <lock> operations for get-locks...\n");
    }
    res = handle_get_locks_request_to_server(server_cb,
                                            TRUE,
                                            &done);
    if (res != NO_ERR && done) {
        /* need to undo the whole thing; whatever got done */

    }

    if (valset != NULL) {
        val_free_value(valset);
    }

    return res;

}  /* do_get_locks */


/********************************************************************
 * FUNCTION do_release_locks (local RPC)
 * 
 * release all the locks on the server
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the history command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_release_locks (server_cb_t *server_cb,
                      obj_template_t *rpc,
                      const xmlChar *line,
                      uint32  len)
{
    ses_cb_t      *scb;
    val_value_t   *valset;
    uint32         locks_timeout, retry_interval;
    boolean        cleanup, done, needed;
    status_t       res;

    if (!server_cb->locks_active) {
        log_error("\nError: locks are not active");
        return ERR_NCX_OPERATION_FAILED;
    }
    scb = mgr_ses_get_scb(server_cb->mysid);
    if (scb == NULL) {
        log_error("\nError: active session dropped, cannot lock");
        return ERR_NCX_OPERATION_FAILED;
    }

    locks_timeout = server_cb->locks_timeout;
    retry_interval = server_cb->locks_retry_interval;
    cleanup = TRUE;

    res = NO_ERR;
    valset = get_valset(server_cb, rpc, &line[len], &res);
    if (res == NO_ERR || res == ERR_NCX_SKIPPED) {

        /* start the auto-unlock procedure */
        server_cb->locks_timeout = locks_timeout;
        server_cb->locks_retry_interval = retry_interval;
        server_cb->locks_cleanup = cleanup;
        needed = setup_unlock_cbs(server_cb);

        if (LOGINFO && needed) {
            log_info("\nSending <unlock> operations for release-locks...\n");
        }

        if (needed) {
            done = FALSE;
            res = handle_release_locks_request_to_server(server_cb,
                                                        TRUE,
                                                        &done);
            if (done) {
                /* need to close the session or report fatal error */
                clear_lock_cbs(server_cb);
            }
        }
    }

    if (valset != NULL) {
        val_free_value(valset);
    }

    return res;

}  /* do_release_locks */


/********************************************************************
* FUNCTION handle_get_locks_request_to_server
* 
* Send the first <lock> operation to the server
* in a get-locks command
*
* INPUTS:
*   server_cb == server control block to use
*   first == TRUE if this is the first call; FALSE otherwise
*   done == address of return final done flag
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*    *done == TRUE when the return code is NO_ERR and all
*                 the locks are granted
*             FALSE otherwise
* RETURNS:
*    status; if NO_ERR then check *done flag
*            otherwise done is true on any error
*********************************************************************/
status_t
    handle_get_locks_request_to_server (server_cb_t *server_cb,
                                       boolean first,
                                       boolean *done)
{
    lock_cb_t      *lockcb;
    ncx_cfg_t       cfg_id;
    time_t          timenow;
    double          timediff;
    status_t        res;
    boolean         finddone, stillwaiting;

#ifdef DEBUG
    if (!server_cb || !done) {
        return  SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *done = FALSE;
    finddone = FALSE;
    stillwaiting = FALSE;
    lockcb = NULL;
    (void)time(&timenow);

    if (first) {
        /* this is the first try, start the overall timer */
        (void)time(&server_cb->locks_start_time);
    } else if (check_locks_timeout(server_cb)) {
        log_error("\nError: get-locks timeout");
        handle_locks_cleanup(server_cb);
        return ERR_NCX_TIMEOUT;
    }

    /* find a config that needs a lock PDU sent
     * first check for a config that has not sent
     * any request yet
     */
    for (cfg_id = NCX_CFGID_RUNNING;
         cfg_id <= NCX_CFGID_STARTUP && !finddone;
         cfg_id++) {

        lockcb = &server_cb->lock_cb[cfg_id];

        if (lockcb->lock_used) {
            if (lockcb->lock_state == LOCK_STATE_IDLE) {
                finddone = TRUE;
            } else if (lockcb->lock_state == LOCK_STATE_FATAL_ERROR) {
                log_error("\nError: fatal error getting lock"
                          " on the %s config",
                          lockcb->config_name);
                return ERR_NCX_OPERATION_FAILED;
            }
        }
    }

    if (!finddone) {
        /* all entries that need to send a lock
         * request have tried at least once to do that
         * now go through all the states and see if any
         * retries are needed; exit if a fatal error is found
         */
        stillwaiting = FALSE;
        for (cfg_id = NCX_CFGID_RUNNING;
             cfg_id <= NCX_CFGID_STARTUP && !finddone;
             cfg_id++) {

            lockcb = &server_cb->lock_cb[cfg_id];
            if (lockcb->lock_used) {
                if (lockcb->lock_state == LOCK_STATE_TEMP_ERROR) {
                    timediff = difftime(timenow, lockcb->last_msg_time);
                    if (timediff >= 
                        (double)server_cb->locks_retry_interval) {
                        finddone = TRUE;
                    } else {
                        server_cb->locks_waiting = TRUE;
                        stillwaiting = TRUE;
                    }
                }
            }
        }

        /* check if there is more work to do after this 
         * function exits
         */
        if (!finddone && !stillwaiting) {
            *done = TRUE;
        }
    }

    /* check if a <lock> request needs to be sent */
    if (finddone && lockcb) {
        server_cb->command_mode = CMD_MODE_AUTOLOCK;
        res = send_lock_pdu_to_server(server_cb,
                                     lockcb,
                                     TRUE);
    }

    return res;
    
}  /* handle_get_locks_request_to_server */


/********************************************************************
* FUNCTION handle_release_locks_request_to_server
* 
* Send an <unlock> operation to the server
* in a get-locks command teardown or a release-locks
* operation
*
* INPUTS:
*   server_cb == server control block to use
*   first == TRUE if this is the first call; FALSE otherwise
*   done == address of return final done flag
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*    *done == TRUE when the return code is NO_ERR and all
*                 the locks are granted
*             FALSE otherwise
* RETURNS:
*    status; if NO_ERR then check *done flag
*            otherwise done is true on any error
*********************************************************************/
status_t
    handle_release_locks_request_to_server (server_cb_t *server_cb,
                                           boolean first,
                                           boolean *done)
{
    lock_cb_t      *lockcb;
    ncx_cfg_t       cfg_id;
    status_t        res;
    boolean         finddone;

#ifdef DEBUG
    if (!server_cb || !done) {
        return  SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    lockcb = NULL;
    res = NO_ERR;
    finddone = FALSE;
    *done = FALSE;

    if (first) {
        server_cb->command_mode = CMD_MODE_AUTOUNLOCK;
        (void)time(&server_cb->locks_start_time);
    } else if (check_locks_timeout(server_cb)) {
        log_error("\nError: release-locks timeout");
        clear_lock_cbs(server_cb);
        return ERR_NCX_TIMEOUT;
    }

    /* do these in order because there are no
     * temporary errors for unlock
     */
    for (cfg_id = NCX_CFGID_RUNNING;
         cfg_id <= NCX_CFGID_STARTUP && !finddone;
         cfg_id++) {

        lockcb = &server_cb->lock_cb[cfg_id];
        if (lockcb->lock_used &&
            lockcb->lock_state == LOCK_STATE_ACTIVE) {
            finddone = TRUE;
        }
    }

    if (!finddone) {
        /* nothing to do */
        if (first) {
            log_info("\nNo locks to release");
        }
        server_cb->state = MGR_IO_ST_CONN_IDLE;
        clear_lock_cbs(server_cb);
        *done = TRUE;
    } else {
        res = send_lock_pdu_to_server(server_cb,
                                     lockcb,
                                     FALSE);
    }

    return res;
    
}  /* handle_release_locks_request_to_server */


/********************************************************************
* FUNCTION handle_locks_cleanup
* 
* Deal with the cleanup for the get-locks or release-locks
*
* INPUTS:
*   server_cb == server control block to use
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*
*********************************************************************/
void
    handle_locks_cleanup (server_cb_t *server_cb)
{
    status_t        res;
    boolean         done;

#ifdef DEBUG
    if (!server_cb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!use_servercb(server_cb)) {
        log_error("\nError: connection lost, canceling release-locks");
        clear_lock_cbs(server_cb);
        return;
    }

    if (server_cb->locks_cleanup) {
        server_cb->command_mode = CMD_MODE_AUTOUNLOCK;
        done = FALSE;
        res = handle_release_locks_request_to_server(server_cb,
                                                    TRUE, &done);
        if (res != NO_ERR) {
            log_error("\nError: handle lock request failed (%)",
                      get_error_string(res));
        }
        if (done) {
            clear_lock_cbs(server_cb);
        }
    } else {
        clear_lock_cbs(server_cb);
    }

}  /* handle_locks_cleanup */


/********************************************************************
* FUNCTION check_locks_timeout
* 
* Check if the locks_timeout is active and if it expired yet
*
* INPUTS:
*   server_cb == server control block to use
*
* RETURNS:
*   TRUE if locks_timeout expired
*   FALSE if no timeout has occurred
*********************************************************************/
boolean
    check_locks_timeout (server_cb_t *server_cb)
{
    time_t          timenow;
    double          timediff;

#ifdef DEBUG
    if (!server_cb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (server_cb->locks_timeout) {
        /* check if there is an overall timeout yet */
        (void)time(&timenow);
        timediff = difftime(timenow,
                            server_cb->locks_start_time);
        if (timediff >= (double)server_cb->locks_timeout) {
            log_debug("\nlock timeout");
            return TRUE;
        }
    }
    return FALSE;

}  /* check_locks_timeout */


/********************************************************************
* FUNCTION send_discard_changes_pdu_to_server
* 
* Send a <discard-changes> operation to the server
*
* INPUTS:
*   server_cb == server control block to use
*
* RETURNS:
*    status
*********************************************************************/
status_t
    send_discard_changes_pdu_to_server (server_cb_t *server_cb)
{
    obj_template_t        *rpc;
    mgr_rpc_req_t         *req;
    val_value_t           *reqdata;
    ses_cb_t              *scb;
    status_t               res;
    xmlns_id_t             obj_nsid;

    req = NULL;
    reqdata = NULL;
    res = NO_ERR;

    if (LOGDEBUG) {
        log_debug("\nSending <discard-changes> request");
    }
    
    rpc = ncx_find_object(get_netconf_mod(server_cb), 
                          NCX_EL_DISCARD_CHANGES);
    if (!rpc) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    obj_nsid = obj_get_nsid(rpc);

    /* construct a method node */
    reqdata = xml_val_new_flag(obj_get_name(rpc), 
                               obj_nsid);
    if (!reqdata) {
        log_error("\nError allocating a new RPC request");
        return ERR_INTERNAL_MEM;
    }

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (!scb) {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    } else {
        req = mgr_rpc_new_request(scb);
        if (!req) {
            res = ERR_INTERNAL_MEM;
            log_error("\nError allocating a new RPC request");
        } else {
            req->data = reqdata;
            req->rpc = rpc;
            req->timeout = server_cb->timeout;
        }
    }
        
    /* if all OK, send the RPC request */
    if (res == NO_ERR) {
        if (LOGDEBUG2) {
            log_debug2("\nabout to send RPC request with reqdata:");
            val_dump_value_max(reqdata, 
                               0,
                               server_cb->defindent,
                               DUMP_VAL_LOG,
                               server_cb->display_mode,
                               FALSE,
                               FALSE);
        }

        /* the request will be stored if this returns NO_ERR */
        res = mgr_rpc_send_request(scb, req, yangcli_reply_handler);
        if (res == NO_ERR) {
            server_cb->command_mode = CMD_MODE_AUTODISCARD;
        }
    }

    /* cleanup and set next state */
    if (res != NO_ERR) {
        if (req) {
            mgr_rpc_free_request(req);
        } else if (reqdata) {
            val_free_value(reqdata);
        }
    } else {
        server_cb->state = MGR_IO_ST_CONN_RPYWAIT;
    }

    return res;

} /* send_discard_changes_pdu_to_server */


/********************************************************************
* FUNCTION clear_lock_cbs
* 
* Clear the lock state info in all the lock control blocks
* in the specified server_cb
* 
* INPUTS:
*  server_cb == server control block to use
*
*********************************************************************/
void
    clear_lock_cbs (server_cb_t *server_cb)
{
    ncx_cfg_t  cfg_id;

    /* set up lock control blocks for get-locks */
    server_cb->locks_active = FALSE;
    server_cb->locks_waiting = FALSE;
    server_cb->locks_cur_cfg = NCX_CFGID_RUNNING;
    server_cb->command_mode = CMD_MODE_NORMAL;

    for (cfg_id = NCX_CFGID_RUNNING;
         cfg_id <= NCX_CFGID_STARTUP;
         cfg_id++) {

        server_cb->lock_cb[cfg_id].lock_state = LOCK_STATE_IDLE;
        server_cb->lock_cb[cfg_id].lock_used = FALSE;
        server_cb->lock_cb[cfg_id].start_time = (time_t)0;
        server_cb->lock_cb[cfg_id].last_msg_time = (time_t)0;
    }

}  /* clear_lock_cbs */


/* END yangcli_autolock.c */
