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
/*  FILE: agt_ses.c

   NETCONF Session Manager: Agent Side Support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
06jun06      abb      begun; cloned from ses_mgr.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cb.h"
#include "agt_connect.h"
#include "agt_ncx.h"
#include "agt_ncxserver.h"
#include "agt_rpc.h"
#include "agt_ses.h"
#include "agt_state.h"
#include "agt_sys.h"
#include "agt_top.h"
#include "agt_util.h"
#include "cfg.h"
#include "def_reg.h"
#include "getcb.h"
#include "log.h"
#include "ncxmod.h"
#include "rpc.h"
#include "ses.h"
#include "ses_msg.h"
#include "status.h"
#include "tstamp.h"
#include "val.h"
#include "xmlns.h"
#include "xml_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* maximum number of concurrent inbound sessions 
 * Must be >= 2 since session 0 is used for the dummy scb
 */
#define AGT_SES_MAX_SESSIONS     1024

#define AGT_SES_MODULE      (const xmlChar *)"yuma-mysession"

#define AGT_SES_GET_MY_SESSION   (const xmlChar *)"get-my-session"

#define AGT_SES_SET_MY_SESSION   (const xmlChar *)"set-my-session"


/* number of seconds to wait between session timeout checks */
#define AGT_SES_TIMEOUT_INTERVAL  1

/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean    agt_ses_init_done = FALSE;

static uint32     next_sesid;

static ses_cb_t  *agtses[AGT_SES_MAX_SESSIONS];

static ses_total_stats_t *agttotals;

static ncx_module_t *mysesmod;

static time_t     last_timeout_check;

/********************************************************************
* FUNCTION get_session_idval
*
* Get the session ID number for the virtual
* counter entry that was called from a <get> operation
*
* INPUTS:
*    virval == virtual value
*    retsid == address of return session ID number
*
* OUTPUTS:
*   *retsid == session ID for this <session> entry

* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_session_key (const val_value_t *virval,
                     ses_id_t *retsid)
{
    const val_value_t  *parentval, *sidval;

    parentval = virval->parent;
    if (parentval == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    sidval = val_find_child(parentval,
                            obj_get_mod_name(parentval->obj),
                            (const xmlChar *)"session-id");

    if (sidval == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;        
    }

    *retsid = VAL_UINT(sidval);
    return NO_ERR;

} /* get_session_key */


/********************************************************************
* FUNCTION get_my_session_invoke
*
* get-my-session : invoke params callback
*
* INPUTS:
*    see rpc/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_my_session_invoke (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode)
{
    val_value_t     *indentval, *linesizeval, *withdefval;
    xmlChar          numbuff[NCX_MAX_NUMLEN];

    (void)methnode;

    snprintf((char *)numbuff, sizeof(numbuff), "%u", scb->indent);
    indentval = val_make_string(mysesmod->nsid,
                                NCX_EL_INDENT,
                                numbuff);
    if (indentval == NULL) {
        return ERR_INTERNAL_MEM;
    }

    snprintf((char *)numbuff, sizeof(numbuff), "%u", scb->linesize);
    linesizeval = val_make_string(mysesmod->nsid,
                                  NCX_EL_LINESIZE,
                                  numbuff);
    if (linesizeval == NULL) {
        val_free_value(indentval);
        return ERR_INTERNAL_MEM;
    }

    withdefval = 
        val_make_string(mysesmod->nsid,
                        NCX_EL_WITH_DEFAULTS,
                        ncx_get_withdefaults_string(scb->withdef));
    if (withdefval == NULL) {
        val_free_value(indentval);
        val_free_value(linesizeval);
        return ERR_INTERNAL_MEM;
    }

    dlq_enque(indentval, &msg->rpc_dataQ);
    dlq_enque(linesizeval, &msg->rpc_dataQ);
    dlq_enque(withdefval, &msg->rpc_dataQ);
    msg->rpc_data_type = RPC_DATA_YANG;
    return NO_ERR;

} /* get_my_session_invoke */


/********************************************************************
* FUNCTION set_my_session_invoke
*
* set-my-session : invoke params callback
*
* INPUTS:
*    see rpc/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    set_my_session_invoke (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode)
{
    val_value_t     *indentval, *linesizeval, *withdefval;

    (void)methnode;

    /* get the indent amount parameter */
    indentval = val_find_child(msg->rpc_input, 
                               AGT_SES_MODULE,
                               NCX_EL_INDENT);
    if (indentval && indentval->res == NO_ERR) {
        scb->indent = VAL_UINT(indentval);
    }

    /* get the line sizer parameter */
    linesizeval = val_find_child(msg->rpc_input, 
                                 AGT_SES_MODULE,
                                 NCX_EL_LINESIZE);
    if (linesizeval && linesizeval->res == NO_ERR) {
        scb->linesize = VAL_UINT(linesizeval);
    }

    /* get the with-defaults parameter */
    withdefval = val_find_child(msg->rpc_input, 
                                AGT_SES_MODULE,
                                NCX_EL_WITH_DEFAULTS);
    if (withdefval && withdefval->res == NO_ERR) {
        scb->withdef = 
            ncx_get_withdefaults_enum(VAL_ENUM_NAME(withdefval));
    }

    return NO_ERR;

} /* set_my_session_invoke */


/************* E X T E R N A L    F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_ses_init
*
* INIT 1:
*   Initialize the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_ses_init (void)
{

    agt_profile_t   *agt_profile;
    status_t         res;
    uint32           i;

    if (agt_ses_init_done) {
        return ERR_INTERNAL_INIT_SEQ;
    }

#ifdef AGT_STATE_DEBUG
    log_debug2("\nagt: Loading netconf-state module");
#endif

    agt_profile = agt_get_profile();

    for (i=0; i<AGT_SES_MAX_SESSIONS; i++) {
        agtses[i] = NULL;
    }
    next_sesid = 1;
    mysesmod = NULL;

    agttotals = ses_get_total_stats();
    memset(agttotals, 0x0, sizeof(ses_total_stats_t));
    tstamp_datetime(agttotals->startTime);
    (void)time(&last_timeout_check);
    agt_ses_init_done = TRUE;

    /* load the netconf-state module */
    res = ncxmod_load_module(AGT_SES_MODULE, 
                             NULL, 
                             &agt_profile->agt_savedevQ,
                             &mysesmod);
    if (res != NO_ERR) {
        return res;
    }

    /* set up get-my-session RPC operation */
    res = agt_rpc_register_method(AGT_SES_MODULE,
                                  AGT_SES_GET_MY_SESSION,
                                  AGT_RPC_PH_INVOKE,
                                  get_my_session_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* set up set-my-session RPC operation */
    res = agt_rpc_register_method(AGT_SES_MODULE,
                                  AGT_SES_SET_MY_SESSION,
                                  AGT_RPC_PH_INVOKE,
                                  set_my_session_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    return res;

}  /* agt_ses_init */


/********************************************************************
* FUNCTION agt_ses_cleanup
*
* Cleanup the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
void 
    agt_ses_cleanup (void)
{
    uint32 i;

    if (agt_ses_init_done) {
        for (i=0; i<AGT_SES_MAX_SESSIONS; i++) {
            if (agtses[i]) {
                agt_ses_free_session(agtses[i]);
            }
        }

        next_sesid = 0;

        agt_rpc_unregister_method(AGT_SES_MODULE,
                                  AGT_SES_GET_MY_SESSION);

        agt_rpc_unregister_method(AGT_SES_MODULE,
                                  AGT_SES_SET_MY_SESSION);

        agt_ses_init_done = FALSE;
    }

}  /* agt_ses_cleanup */


/********************************************************************
* FUNCTION agt_ses_new_dummy_session
*
* Create a dummy session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized dummy SCB, or NULL if malloc error
*********************************************************************/
ses_cb_t *
    agt_ses_new_dummy_session (void)
{
    ses_cb_t  *scb;

    if (!agt_ses_init_done) {
        agt_ses_init();
    }

    /* check if dummy session already cached */
    if (agtses[0]) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return NULL;
    }

    /* no, so create it */
    scb = ses_new_dummy_scb();
    if (!scb) {
        return NULL;
    }

    agtses[0] = scb;
    
    return scb;

}  /* agt_ses_new_dummy_session */


/********************************************************************
* FUNCTION agt_ses_set_dummy_session_acm
*
* Set the session ID and username of the user that
* will be responsible for the rollback if needed
*
* INPUTS:
*   dummy_session == session control block to change
*   use_sid == Session ID to use for the rollback
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_ses_set_dummy_session_acm (ses_cb_t *dummy_session,
                                   ses_id_t  use_sid)
{
    ses_cb_t  *scb;

    assert( dummy_session && "dummy_session is NULL!" );

    if (!agt_ses_init_done) {
        agt_ses_init();
    }

    scb = agtses[use_sid];
    if (scb == NULL) {
        return ERR_NCX_INVALID_VALUE;
    }

    dummy_session->rollback_sid = use_sid;

    /*TODO Quick fix for confirmed-commit/rollback bug. */
    /*     Needs further investigation. */
    dummy_session->sid = use_sid;

    if (scb == dummy_session) {
        return NO_ERR;  /* skip -- nothing to do */
    }

    if (dummy_session->username && scb->username) {
        m__free(dummy_session->username);
        dummy_session->username = NULL;
    }

    if (scb->username) {
        dummy_session->username = xml_strdup(scb->username);
        if (dummy_session->username == NULL) {
            return ERR_INTERNAL_MEM;
        }
    }

    if (dummy_session->peeraddr && scb->peeraddr) {
        m__free(dummy_session->peeraddr);
        dummy_session->peeraddr = NULL;
    }

    if (scb->peeraddr) {
        dummy_session->peeraddr = xml_strdup(scb->peeraddr);
        if (dummy_session->peeraddr == NULL) {
            return ERR_INTERNAL_MEM;
        }
    }
    
    return NO_ERR;

}  /* agt_ses_set_dummy_session_acm */


/********************************************************************
* FUNCTION agt_ses_free_dummy_session
*
* Free a dummy session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
void
    agt_ses_free_dummy_session (ses_cb_t *scb)
{
    assert( scb && "scb is NULL!" );
    assert( agt_ses_init_done && "agt_ses_init_done is false!" );
    assert( agtses[0] && "agtses[0] is null" );

    agt_acm_clear_session_cache(scb);
    ses_free_scb(scb);
    agtses[0] = NULL;

}  /* agt_ses_free_dummy_session */


/********************************************************************
* FUNCTION agt_ses_new_session
*
* Create a real agent session control block
*
* INPUTS:
*   transport == the transport type
*   fd == file descriptor number to use for IO 
* RETURNS:
*   pointer to initialized SCB, or NULL if some error
*   This pointer is stored in the session table, so it does
*   not have to be saved by the caller
*********************************************************************/
ses_cb_t *
    agt_ses_new_session (ses_transport_t transport,
                         int fd)
{
    ses_cb_t       *scb;
    agt_profile_t  *profile;
    uint32          i, slot;
    status_t        res;

    if (!agt_ses_init_done) {
        agt_ses_init();
    }

    res = NO_ERR;
    slot = 0;
    scb = NULL;

    /* check if any sessions are available */
    if (next_sesid == 0) {
        /* end has already been reached, so now in brute force
         * session reclaim mode
         */
        slot = 0;
        for (i=1; i<AGT_SES_MAX_SESSIONS && !slot; i++) {
            if (!agtses[i]) {
                slot = i;
            }
        }
    } else {
        slot = next_sesid;
    }

    if (slot) {
        /* make sure there is memory for a session control block */
        scb = ses_new_scb();
        if (scb) {
            /* initialize the profile vars */
            profile = agt_get_profile();
            scb->linesize = profile->agt_linesize;
            scb->withdef = profile->agt_defaultStyleEnum;
            scb->indent = profile->agt_indent;

            if (ncx_protocol_enabled(NCX_PROTO_NETCONF10)) {
                scb->protocols_requested |= NCX_FL_PROTO_NETCONF10;
            }
            if (ncx_protocol_enabled(NCX_PROTO_NETCONF11)) {
                scb->protocols_requested |= NCX_FL_PROTO_NETCONF11;
            }

            /* initialize the static vars */
            scb->type = SES_TYP_NETCONF;
            scb->transport = transport;
            scb->state = SES_ST_INIT;
            scb->mode = SES_MODE_XML;
            scb->sid = slot;
            scb->inready.sid = slot;
            scb->outready.sid = slot;
            scb->state = SES_ST_INIT;
            scb->fd = fd;
            scb->instate = SES_INST_IDLE;
            scb->stream_output = TRUE;
            res = ses_msg_new_buff(scb, TRUE, &scb->outbuff);
        } else {
            res = ERR_INTERNAL_MEM;
        }
    } else {
        res = ERR_NCX_RESOURCE_DENIED;
    }

    /* add the FD to SCB mapping in the definition registry */
    if (res == NO_ERR) {
        res = def_reg_add_scb(scb->fd, scb);
    }

    /* check result and add to session array if NO_ERR */
    if (res == NO_ERR) {
        agtses[slot] = scb;

        /* update the next slot now */
        if (next_sesid) {
            if (++next_sesid==AGT_SES_MAX_SESSIONS) {
                /* reached the end */
                next_sesid = 0;
            }
        }
        if (LOGINFO) {
            log_info("\nNew session %d created OK", slot);
        }
        agttotals->inSessions++;
        agttotals->active_sessions++;
    } else {
        if (scb) {
            agt_ses_free_session(scb);
            scb = NULL;
        }
        if (LOGINFO) {
            log_info("\nNew session request failed (%s)",
                     get_error_string(res));
        }
    }

    return scb;

}  /* agt_ses_new_session */


/********************************************************************
* FUNCTION agt_ses_free_session
*
* Free a real session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
void
    agt_ses_free_session (ses_cb_t *scb)
{
    ses_id_t  slot;

    assert( scb && "scb is NULL!" );
    assert( agt_ses_init_done && "agt_ses_init_done is false!" );

    if (scb->type==SES_TYP_DUMMY) {
        agt_ses_free_dummy_session( scb );
        return;
    }

    slot = scb->sid;

    if (scb->fd) {
        def_reg_del_scb(scb->fd);
    }

    cfg_release_locks(slot);

    agt_state_remove_session(slot);
    agt_not_remove_subscription(slot);

    /* add this session to ses stats */
    agttotals->active_sessions--;
    if (scb->active) {
        agttotals->closed_sessions++;
    } else {
        agttotals->failed_sessions++;
    }

    /* make sure the ncxserver loop does not try
     * to read from this file descriptor again
     */
    agt_ncxserver_clear_fd(scb->fd);

    /* clear any NACM cache that this session was using */
    agt_acm_clear_session_cache(scb);

    /* this will close the socket if it is still open */
    ses_free_scb(scb);

    agtses[slot] = NULL;

    if (LOGINFO) {
        log_info("\nSession %d closed", slot);
    }

}  /* agt_ses_free_session */


/********************************************************************
* FUNCTION agt_ses_session_id_valid
*
* Check if a session-id is for an active session
*
* INPUTS:
*   sid == session ID to check
* RETURNS:
*   TRUE if active session; FALSE otherwise
*********************************************************************/
boolean
    agt_ses_session_id_valid (ses_id_t  sid)
{
    if (sid < AGT_SES_MAX_SESSIONS && agtses[sid]) {
        return TRUE;
    } else {
        return FALSE;
    }
} /* agt_ses_session_id_valid */


/********************************************************************
* FUNCTION agt_ses_request_close
*
* Start the close of the specified session
*
* INPUTS:
*   sid == session ID to close
*   killedby == session ID executing the kill-session or close-session
*   termreason == termination reason code
*
* RETURNS:
*   none
*********************************************************************/
void
    agt_ses_request_close (ses_cb_t *scb,
                           ses_id_t killedby,
                           ses_term_reason_t termreason)
{
    if (!scb) {
        return;
    }

    /* N/A for dummy sessions */
    if (scb->type==SES_TYP_DUMMY) {
        return;
    }

    scb->killedbysid = killedby;
    scb->termreason = termreason;

    /* change the session control block state */
    switch (scb->state) {
    case SES_ST_IDLE:
    case SES_ST_SHUTDOWN:
    case SES_ST_SHUTDOWN_REQ:
        agt_ses_kill_session(scb, 
                             killedby,
                             termreason);
        break;
    case SES_ST_IN_MSG:
        scb->state = SES_ST_SHUTDOWN_REQ;
        break;
    default:
        if (dlq_empty(&scb->outQ)) {
            agt_ses_kill_session(scb, 
                                 killedby,
                                 termreason);
        } else {
            scb->state = SES_ST_SHUTDOWN_REQ;
        }
    }

}  /* agt_ses_request_close */


/********************************************************************
* FUNCTION agt_ses_kill_session
*
* Kill the specified session
*
* INPUTS:
*   sid == session ID to kill
* RETURNS:
*   none
*********************************************************************/
void
    agt_ses_kill_session (ses_cb_t *scb,
                          ses_id_t killedby,
                          ses_term_reason_t termreason)
{
    if (!scb) {
        return;    /* no session found for this ID */
    }

    /* N/A for dummy sessions */
    if (scb->type==SES_TYP_DUMMY) {
        return;
    }

    /* handle confirmed commit started by this session */
    if (agt_ncx_cc_active() && agt_ncx_cc_ses_id() == scb->sid) {
        if (agt_ncx_cc_persist_id() == NULL) {
            agt_ncx_cancel_confirmed_commit(scb, NCX_CC_EVENT_CANCEL);
        } else {
            agt_ncx_clear_cc_ses_id();
        }
    }

    agt_sys_send_sysSessionEnd(scb,
                               termreason,
                               killedby);

    /* clear all resources and delete the session control block */
    agt_ses_free_session(scb);

} /* agt_ses_kill_session */


/********************************************************************
* FUNCTION agt_ses_process_first_ready
*
* Check the readyQ and process the first message, if any
*
* RETURNS:
*     TRUE if a message was processed
*     FALSE if the readyQ was empty
*********************************************************************/
boolean
    agt_ses_process_first_ready (void)
{
    ses_cb_t     *scb;
    ses_ready_t  *rdy;
    ses_msg_t    *msg;
    status_t      res;
    uint32        cnt;
    xmlChar       buff[32];

    rdy = ses_msg_get_first_inready();
    if (!rdy) {
        return FALSE;
    }

    /* get the session control block that rdy is embedded into */
    scb = agtses[rdy->sid];

    if (scb == NULL) {
        log_debug("\nagt_ses: session %d gone", rdy->sid);
        return FALSE;
    }

    log_debug2("\nagt_ses msg ready for session %d", scb->sid);

    /* check the session control block state */
    if (scb->state >= SES_ST_SHUTDOWN_REQ) {
        /* don't process the message or even it mark it
         * It will be cleaned up when the session is freed
         */
        log_debug("\nagt_ses drop input, session %d shutting down", 
                  scb->sid);
        return TRUE;
    }

    /* make sure a message is really there */
    msg = (ses_msg_t *)dlq_firstEntry(&scb->msgQ);
    if (!msg || !msg->ready) {
        SET_ERROR(ERR_INTERNAL_PTR);
        log_error("\nagt_ses ready Q message not correct");
        if (msg && scb->state != SES_ST_INIT) {
            /* do not echo the ncx-connect message */
            cnt = xml_strcpy(buff, 
                             (const xmlChar *)"Incoming msg for session ");
            snprintf((char *)(&buff[cnt]), sizeof(buff) - cnt, "%u", scb->sid);
            ses_msg_dump(msg, buff);
        }
            
        return FALSE;
    } else if (LOGDEBUG2 && scb->state != SES_ST_INIT) {
        cnt = xml_strcpy(buff, 
                         (const xmlChar *)"Incoming msg for session ");
        snprintf((char *)(&buff[cnt]), sizeof(buff) - cnt, "%u", scb->sid);
        ses_msg_dump(msg, buff);
    }

    /* setup the XML parser */
    if (scb->reader) {
            /* reset the xmlreader */
        res = xml_reset_reader_for_session(ses_read_cb,
                                           NULL, 
                                           scb, 
                                           scb->reader);
    } else {
        res = xml_get_reader_for_session(ses_read_cb,
                                         NULL, 
                                         scb, 
                                         &scb->reader);
    }

    /* process the message */
    if (res == NO_ERR) {
        /* process the message 
         * the scb pointer may get deleted !!!
         */
        agt_top_dispatch_msg(&scb);
    } else {
        if (LOGINFO) {
            log_info("\nReset xmlreader failed for session %d (%s)",
                     scb->sid, 
                     get_error_string(res));
        }
        agt_ses_kill_session(scb, 0, SES_TR_OTHER);
        scb = NULL;
    }

    if (scb) {
        /* free the message that was just processed */
        dlq_remove(msg);
        ses_msg_free_msg(scb, msg);


        /* check if any messages left for this session */
        msg = (ses_msg_t *)dlq_firstEntry(&scb->msgQ);
        if (msg && msg->ready) {
            ses_msg_make_inready(scb);
        }
    }

    return TRUE;
    
}  /* agt_ses_process_first_ready */


/********************************************************************
* FUNCTION agt_ses_check_timeouts
*
* Check if any sessions need to be dropped because they
* have been idle too long.
*
*********************************************************************/
void
    agt_ses_check_timeouts (void)
{
    ses_cb_t         *scb;
    agt_profile_t    *agt_profile;
    uint32            i, last;
    time_t            timenow;
    double            timediff;

    agt_profile = agt_get_profile();

    /* check if both timeouts are disabled and the
     * confirmed-commit is not active -- quick exit 
     */
    if (agt_profile->agt_idle_timeout == 0 &&
        agt_profile->agt_hello_timeout == 0 &&
        !agt_ncx_cc_active()) {
        return;
    }

    /* at least one timeout enabled, so check if enough
     * time has elapsed since the last time through
     * the process loop; the AGT_SES_TIMEOUT_INTERVAL
     * is used for this purpose
     */
    (void)time(&timenow);
    timediff = difftime(timenow, last_timeout_check);
    if (timediff < (double)AGT_SES_TIMEOUT_INTERVAL) {
        return;
    }

    /* reset the timeout interval for next time */
    last_timeout_check = timenow;

    /* check all the sessions only if idle or hello
     * timeout checking is enabled
     */
    if (agt_profile->agt_idle_timeout == 0 &&
        agt_profile->agt_hello_timeout == 0) {
        last = 0;
    } else if (next_sesid == 0) {
        last = AGT_SES_MAX_SESSIONS;
    } else {
        last = next_sesid;
    }

    for (i=1; i < last; i++) {

        scb = agtses[i];
        if (scb == NULL) {
            continue;
        }

        /* check if the the hello timer needs to be tested */
        if (agt_profile->agt_hello_timeout > 0 &&
            scb->state == SES_ST_HELLO_WAIT) {

            timediff = difftime(timenow, scb->hello_time);
            if (timediff >= (double)agt_profile->agt_hello_timeout) {
                if (LOGDEBUG) {
                    log_debug("\nHello timeout for session %u", i);
                }
                agt_ses_kill_session(scb, 0, SES_TR_TIMEOUT);
                continue;
            }
        }

        /* check if the the idle timer needs to be tested 
         * check only active sessions
         * skip if notifications are active 
         */
        if (agt_profile->agt_idle_timeout > 0 &&
            scb->active && 
            !scb->notif_active) {

            timediff = difftime(timenow, scb->last_rpc_time);
            if (timediff >= (double)agt_profile->agt_idle_timeout) {
                if (LOGDEBUG) {
                    log_debug("\nIdle timeout for session %u", i);
                }
                agt_ses_kill_session(scb, 0, SES_TR_TIMEOUT);
                continue;
            }
        }
    }

    /* check the confirmed-commit timeout */
    agt_ncx_check_cc_timeout();

}  /* agt_ses_check_timeouts */


/********************************************************************
* FUNCTION agt_ses_ssh_port_allowed
*
* Check if the port number used for SSH connect is okay
*
* RETURNS:
*     TRUE if port allowed
*     FALSE if port not allowed
*********************************************************************/
boolean
    agt_ses_ssh_port_allowed (uint16 port)
{
    const agt_profile_t *profile;
    uint32         i;

    if (port == 0) {
        return FALSE;
    }

    profile = agt_get_profile();
    if (!profile) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    if (profile->agt_ports[0]) {
        /* something configured, so use only that list */
        for (i = 0; i < AGT_MAX_PORTS; i++) {
            if (port == profile->agt_ports[i]) {
                return TRUE;
            }
        }
        return FALSE;
    } else {
        /* no ports configured so allow 830 */
        if (port==NCX_NCSSH_PORT) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    /*NOTREACHED*/

}  /* agt_ses_ssh_port_allowed */


/********************************************************************
* FUNCTION agt_ses_fill_writeset
*
* Drain the ses_msg outreadyQ and set the specified fdset
* Used by agt_ncxserver write_fd_set
*
* INPUTS:
*    fdset == pointer to fd_set to fill
*    maxfdnum == pointer to max fd int to fill in
*
* OUTPUTS:
*    *fdset is updated in
*    *maxfdnum may be updated
*********************************************************************/
void
    agt_ses_fill_writeset (fd_set *fdset,
                           int *maxfdnum)
{
    ses_ready_t *rdy;
    ses_cb_t *scb;
    boolean done;

    FD_ZERO(fdset);
    done = FALSE;
    while (!done) {
        rdy = ses_msg_get_first_outready();
        if (!rdy) {
            done = TRUE;
        } else {
            scb = agtses[rdy->sid];
            if (scb && scb->state <= SES_ST_SHUTDOWN_REQ) {
                FD_SET(scb->fd, fdset);
                if (scb->fd > *maxfdnum) {
                    *maxfdnum = scb->fd;
                }
            }
        }
    }

}  /* agt_ses_fill_writeset */


/********************************************************************
* FUNCTION agt_ses_get_inSessions
*
* <get> operation handler for the inSessions counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_inSessions (ses_cb_t *scb,
                            getcb_mode_t cbmode,
                            const val_value_t *virval,
                            val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->inSessions;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_inSessions */


/********************************************************************
* FUNCTION agt_ses_get_inBadHellos
*
* <get> operation handler for the inBadHellos counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_inBadHellos (ses_cb_t *scb,
                             getcb_mode_t cbmode,
                             const val_value_t *virval,
                             val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->inBadHellos;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_inBadHellos */


/********************************************************************
* FUNCTION agt_ses_get_inRpcs
*
* <get> operation handler for the inRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_inRpcs (ses_cb_t *scb,
                        getcb_mode_t cbmode,
                        const val_value_t *virval,
                        val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->stats.inRpcs;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_inRpcs */


/********************************************************************
* FUNCTION agt_ses_get_inBadRpcs
*
* <get> operation handler for the inBadRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_inBadRpcs (ses_cb_t *scb,
                           getcb_mode_t cbmode,
                           const val_value_t *virval,
                           val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->stats.inBadRpcs;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_inBadRpcs */


/********************************************************************
* FUNCTION agt_ses_get_outRpcErrors
*
* <get> operation handler for the outRpcErrors counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_outRpcErrors (ses_cb_t *scb,
                              getcb_mode_t cbmode,
                              const val_value_t *virval,
                              val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->stats.outRpcErrors;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_outRpcErrors */


/********************************************************************
* FUNCTION agt_ses_get_outNotifications
*
* <get> operation handler for the outNotifications counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_outNotifications (ses_cb_t *scb,
                                  getcb_mode_t cbmode,
                                  const val_value_t *virval,
                                  val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->stats.outNotifications;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_outNotifications */


/********************************************************************
* FUNCTION agt_ses_get_droppedSessions
*
* <get> operation handler for the droppedSessions counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_droppedSessions (ses_cb_t *scb,
                                 getcb_mode_t cbmode,
                                 const val_value_t *virval,
                                 val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        VAL_UINT(dstval) = agttotals->droppedSessions;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* agt_ses_get_droppedSessions */


/********************************************************************
* FUNCTION agt_ses_get_session_inRpcs
*
* <get> operation handler for the session/inRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_session_inRpcs (ses_cb_t *scb,
                                getcb_mode_t cbmode,
                                const val_value_t *virval,
                                val_value_t  *dstval)
{
    ses_cb_t    *testscb;
    ses_id_t     sid;
    status_t     res;

    (void)scb;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    sid = 0;
    res = get_session_key(virval, &sid);
    if (res != NO_ERR) {
        return res;
    }

    testscb = agtses[sid];
    VAL_UINT(dstval) = testscb->stats.inRpcs;
    return NO_ERR;

} /* agt_ses_get_session_inRpcs */


/********************************************************************
* FUNCTION agt_ses_get_session_inBadRpcs
*
* <get> operation handler for the inBadRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_session_inBadRpcs (ses_cb_t *scb,
                                   getcb_mode_t cbmode,
                                   const val_value_t *virval,
                                   val_value_t  *dstval)
{
    ses_cb_t    *testscb;
    ses_id_t     sid;
    status_t     res;

    (void)scb;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    sid = 0;
    res = get_session_key(virval, &sid);
    if (res != NO_ERR) {
        return res;
    }

    testscb = agtses[sid];
    VAL_UINT(dstval) = testscb->stats.inBadRpcs;
    return NO_ERR;

} /* agt_ses_get_session_inBadRpcs */


/********************************************************************
* FUNCTION agt_ses_get_session_outRpcErrors
*
* <get> operation handler for the outRpcErrors counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_session_outRpcErrors (ses_cb_t *scb,
                                      getcb_mode_t cbmode,
                                      const val_value_t *virval,
                                      val_value_t  *dstval)
{
    ses_cb_t    *testscb;
    ses_id_t     sid;
    status_t     res;

    (void)scb;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    sid = 0;
    res = get_session_key(virval, &sid);
    if (res != NO_ERR) {
        return res;
    }

    testscb = agtses[sid];
    VAL_UINT(dstval) = testscb->stats.outRpcErrors;
    return NO_ERR;

} /* agt_ses_get_session_outRpcErrors */


/********************************************************************
* FUNCTION agt_ses_get_session_outNotifications
*
* <get> operation handler for the outNotifications counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_ses_get_session_outNotifications (ses_cb_t *scb,
                                          getcb_mode_t cbmode,
                                          const val_value_t *virval,
                                          val_value_t  *dstval)
{
    ses_cb_t    *testscb;
    ses_id_t     sid;
    status_t     res;

    (void)scb;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    sid = 0;
    res = get_session_key(virval, &sid);
    if (res != NO_ERR) {
        return res;
    }

    testscb = agtses[sid];
    VAL_UINT(dstval) = testscb->stats.outNotifications;
    return NO_ERR;

} /* agt_ses_get_session_outNotifications */


/********************************************************************
* FUNCTION agt_ses_invalidate_session_acm_caches
*
* Invalidate all session ACM caches so they will be rebuilt
* TBD:: optimize and figure out exactly what needs to change
*
*********************************************************************/
void
    agt_ses_invalidate_session_acm_caches (void)
{
    uint32          i;

    for (i=0; i<AGT_SES_MAX_SESSIONS; i++) {
        if (agtses[i] != NULL) {
            agt_acm_invalidate_session_cache(agtses[i]);
        }
    }
}  /* acm_ses_invalidate_session_acm_caches */

/********************************************************************
* FUNCTION agt_ses_get_session_for_id
*
* get the session for the supplied sid
*
* INPUTS:
*    ses_id_t the id of the session to get
*
* RETURNS:
*    ses_cb_t*
*    
*********************************************************************/
ses_cb_t* 
    agt_ses_get_session_for_id(ses_id_t sid)
{
    ses_cb_t *scb = NULL;

    if ( sid >0 && sid < AGT_SES_MAX_SESSIONS )
    {
        scb = agtses[ sid ];
    }
    return scb;
}



/* END file agt_ses.c */
