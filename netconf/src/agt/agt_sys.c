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
/*  FILE: agt_sys.c

   NETCONF System Data Model implementation: Server Side Support

identifiers:
container /system
leaf /system/sysName
leaf /system/sysCurrentDateTime
leaf /system/sysBootDateTime
leaf /system/sysLogLevel
leaf /system/sysNetconfServerId
notification /sysStartup
leaf /sysStartup/startupSource
list /sysStartup/bootError
leaf /sysStartup/bootError/error-type
leaf /sysStartup/bootError/error-tag
leaf /sysStartup/bootError/error-severity
leaf /sysStartup/bootError/error-app-tag
leaf /sysStartup/bootError/error-path
leaf /sysStartup/bootError/error-message
leaf /sysStartup/bootError/error-info
notification /sysConfigChange
leaf /sysConfigChange/userName
leaf /sysConfigChange/sessionId
list /sysConfigChange/edit
leaf /sysConfigChange/edit/target
leaf /sysConfigChange/edit/operation
notification /sysCapabilityChange
container /sysCapabilityChange/changed-by
choice /sysCapabilityChange/changed-by/server-or-user
case /sysCapabilityChange/changed-by/server-or-user/server
leaf /sysCapabilityChange/changed-by/server-or-user/server/server
case /sysCapabilityChange/changed-by/server-or-user/by-user
leaf /sysCapabilityChange/changed-by/server-or-user/by-user/userName
leaf /sysCapabilityChange/changed-by/server-or-user/by-user/sessionId
leaf /sysCapabilityChange/changed-by/server-or-user/by-user/remoteHost
leaf-list /sysCapabilityChange/added-capability
leaf-list /sysCapabilityChange/deleted-capability
notification /sysSessionStart
leaf /sysSessionStart/userName
leaf /sysSessionStart/sessionId
leaf /sysSessionStart/remoteHost
notification /sysSessionEnd
leaf /sysSessionEnd/userName
leaf /sysSessionEnd/sessionId
leaf /sysSessionEnd/remoteHost
leaf /sysSessionEnd/terminationReason
notification /sysConfirmedCommit
leaf /sysConfirmedCommit/userName
leaf /sysConfirmedCommit/sessionId
leaf /sysConfirmedCommit/remoteHost
leaf /sysConfirmedCommit/confirmEvent
rpc /set-log-level
leaf rpc/input/log-level

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24feb09      abb      begun

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
#include <sys/utsname.h>
#include <assert.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_cfg.h"
#include "agt_cli.h"
#include "agt_not.h"
#include "agt_rpc.h"
#include "agt_ses.h"
#include "agt_sys.h"
#include "agt_util.h"
#include "cfg.h"
#include "getcb.h"
#include "log.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "rpc.h"
#include "rpc_err.h"
#include "ses.h"
#include "ses_msg.h"
#include "status.h"
#include "tstamp.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define system_N_system (const xmlChar *)"system"
#define system_N_sysName (const xmlChar *)"sysName"
#define system_N_sysCurrentDateTime (const xmlChar *)"sysCurrentDateTime"
#define system_N_sysBootDateTime (const xmlChar *)"sysBootDateTime"
#define system_N_sysLogLevel (const xmlChar *)"sysLogLevel"
#define system_N_sysNetconfServerId (const xmlChar *)"sysNetconfServerId"
#define system_N_sysNetconfServerCLI (const xmlChar *)"sysNetconfServerCLI"

#define system_N_sysStartup (const xmlChar *)"sysStartup"

#define system_N_sysConfigChange (const xmlChar *)"sysConfigChange"
#define system_N_edit (const xmlChar *)"edit"

#define system_N_sysCapabilityChange (const xmlChar *)"sysCapabilityChange"
#define system_N_sysSessionStart (const xmlChar *)"sysSessionStart"
#define system_N_sysSessionEnd (const xmlChar *)"sysSessionEnd"
#define system_N_sysConfirmedCommit (const xmlChar *)"sysConfirmedCommit"



#define system_N_userName (const xmlChar *)"userName"
#define system_N_sessionId (const xmlChar *)"sessionId"
#define system_N_remoteHost (const xmlChar *)"remoteHost"
#define system_N_killedBy (const xmlChar *)"killedBy"
#define system_N_terminationReason (const xmlChar *)"terminationReason"

#define system_N_confirmEvent (const xmlChar *)"confirmEvent"

#define system_N_target (const xmlChar *)"target"
#define system_N_operation (const xmlChar *)"operation"

#define system_N_changed_by (const xmlChar *)"changed-by"
#define system_N_added_capability (const xmlChar *)"added-capability"
#define system_N_deleted_capability (const xmlChar *)"deleted-capability"

#define system_N_uname (const xmlChar *)"uname"
#define system_N_sysname (const xmlChar *)"sysname"
#define system_N_release (const xmlChar *)"release"
#define system_N_version (const xmlChar *)"version"
#define system_N_machine (const xmlChar *)"machine"
#define system_N_nodename (const xmlChar *)"nodename"

#define system_N_set_log_level (const xmlChar *)"set-log-level"
#define system_N_log_level (const xmlChar *)"log-level"


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

static boolean              agt_sys_init_done = FALSE;

/* system.yang */
static ncx_module_t         *sysmod;

/* cached pointer to the <system> element template */
static obj_template_t *systemobj;

/* cached pointers to the eventType nodes for this module */
static obj_template_t *sysStartupobj;
static obj_template_t *sysConfigChangeobj;
static obj_template_t *sysCapabilityChangeobj;
static obj_template_t *sysSessionStartobj;
static obj_template_t *sysSessionEndobj;
static obj_template_t *sysConfirmedCommitobj;



/********************************************************************
* FUNCTION payload_error
*
* Generate an error for a payload leaf that failed
*
* INPUTS:
*    name == leaf name that failed
*    res == error status
*********************************************************************/
static void
    payload_error (const xmlChar *name,
                        status_t res)
{
    log_error( "\nError: cannot make payload leaf '%s' (%s)", 
               name, get_error_string(res) );

}  /* payload_error */


/********************************************************************
* FUNCTION get_currentDateTime
*
* <get> operation handler for the sysCurrentDateTime leaf
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_currentDateTime (ses_cb_t *scb,
                         getcb_mode_t cbmode,
                         const val_value_t *virval,
                         val_value_t  *dstval)
{
    xmlChar      *buff;

    (void)scb;
    (void)virval;

    if (cbmode == GETCB_GET_VALUE) {
        buff = (xmlChar *)m__getMem(TSTAMP_MIN_SIZE);
        if (!buff) {
            return ERR_INTERNAL_MEM;
        }
        tstamp_datetime(buff);
        VAL_STR(dstval) = buff;
        return NO_ERR;
    } else {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

} /* get_currentDateTime */


/********************************************************************
* FUNCTION get_currentLogLevel
*
* <get> operation handler for the sysCurrentDateTime leaf
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_currentLogLevel (ses_cb_t *scb,
                         getcb_mode_t cbmode,
                         const val_value_t *virval,
                         val_value_t  *dstval)
{
    const xmlChar  *loglevelstr;
    log_debug_t     loglevel;
    status_t        res;

    (void)scb;
    (void)virval;
    res = NO_ERR;

    if (cbmode == GETCB_GET_VALUE) {
        loglevel = log_get_debug_level();
        loglevelstr = log_get_debug_level_string(loglevel);
        if (loglevelstr == NULL) {
            res = ERR_NCX_OPERATION_FAILED;
        } else {
            res = ncx_set_enum(loglevelstr, VAL_ENU(dstval));
        }
    } else {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    return res;

} /* get_currentLogLevel */


/********************************************************************
* FUNCTION set_log_level_invoke
*
* set-log-level : invoke params callback
*
* INPUTS:
*    see rpc/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    set_log_level_invoke (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    val_value_t     *levelval;
    log_debug_t      levelenum;
    status_t         res;

    (void)scb;
    (void)methnode;

    res = NO_ERR;

    /* get the level parameter */
    levelval = val_find_child(msg->rpc_input, 
                              AGT_SYS_MODULE,
                              NCX_EL_LOGLEVEL);
    if (levelval) {
        if (levelval->res == NO_ERR) {
            levelenum = 
                log_get_debug_level_enum((const char *)
                                         VAL_ENUM_NAME(levelval));
            if (levelenum != LOG_DEBUG_NONE) {
                log_set_debug_level(levelenum);
            } else {
                res = ERR_NCX_OPERATION_FAILED;
            }
        } else {
            res = levelval->res;
        }
    }

    return res;

} /* set_log_level_invoke */


/********************************************************************
* FUNCTION send_sysStartup
*
* Queue the <sysStartup> notification
*
*********************************************************************/
static void
    send_sysStartup (void)
{
    agt_not_msg_t         *not;
    cfg_template_t        *cfg;
    val_value_t           *leafval, *bootErrorval;
    rpc_err_rec_t         *rpcerror;
    obj_template_t        *bootErrorobj;
    status_t               res;

    log_debug("\nagt_sys: generating <sysStartup> notification");

    not = agt_not_new_notification(sysStartupobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot send <sysStartup>");
        return;
    }

    /* add sysStartup/startupSource */
    cfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (cfg) {
        if (cfg->src_url) {
            leafval = agt_make_leaf(sysStartupobj, 
                    (const xmlChar *)"startupSource", cfg->src_url, &res);
            if (leafval) {
                agt_not_add_to_payload(not, leafval);
            } else {
                payload_error((const xmlChar *)"startupSource", res);
            }
        }
    
        /* add sysStartup/bootError for each error recorded */
        if (!dlq_empty(&cfg->load_errQ)) {
            /* get the bootError object */
            bootErrorobj = obj_find_child( sysStartupobj, AGT_SYS_MODULE, 
                                           (const xmlChar *)"bootError");
            if (bootErrorobj) {
                rpcerror = (rpc_err_rec_t *)dlq_firstEntry(&cfg->load_errQ);
                for ( ; rpcerror;
                     rpcerror = (rpc_err_rec_t *)dlq_nextEntry(rpcerror)) {
                    /* make the bootError value struct */
                    bootErrorval = val_new_value();
                    if (!bootErrorval) {
                        payload_error((const xmlChar *)"bootError", 
                                      ERR_INTERNAL_MEM);
                    } else {
                        val_init_from_template(bootErrorval, bootErrorobj);
                        res = agt_rpc_fill_rpc_error(rpcerror, bootErrorval);
                        if (res != NO_ERR) {
                            log_error("\nError: problems making <bootError> "
                                      "(%s)", get_error_string(res));
                        }
                        /* add even if there are some missing leafs */
                        agt_not_add_to_payload(not, bootErrorval);
                    }
                }
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    agt_not_queue_notification(not);
} /* send_sysStartup */


/********************************************************************
* FUNCTION add_common_session_parms
*
* Add the leafs from the SysCommonSessionParms grouping
*
* INPUTS:
*   scb == session control block to use for payload values
*   not == notification msg to use to add parms into
*
* OUTPUTS:
*   'not' payloadQ has malloced entries added to it
*********************************************************************/
static void
    add_common_session_parms (const ses_cb_t *scb,
                              agt_not_msg_t *not)
{
    val_value_t     *leafval;
    status_t         res;
    ses_id_t         use_sid;

    /* add userName */
    if (scb->username) {
        leafval = agt_make_leaf(not->notobj,
                                system_N_userName,
                                scb->username,
                                &res);
        if (leafval) {
            agt_not_add_to_payload(not, leafval);
        } else {
            payload_error(system_N_userName, res);
        }
    }

    /* add sessionId */
    if (scb->sid) {
        use_sid = scb->sid;
    } else if (scb->rollback_sid) {
        use_sid = scb->rollback_sid;
    } else {
        res = ERR_NCX_NOT_IN_RANGE;
        use_sid = 0;
    }

    leafval = NULL;
    if (use_sid) {
        leafval = agt_make_uint_leaf(not->notobj,
                                     system_N_sessionId,
                                     use_sid,
                                     &res);
    }
    if (leafval) {
        agt_not_add_to_payload(not, leafval);
    } else {
        payload_error(system_N_sessionId, res);
    }

    /* add remoteHost */
    if (scb->peeraddr) {
        leafval = agt_make_leaf(not->notobj,
                                system_N_remoteHost,
                                scb->peeraddr,
                                &res);
        if (leafval) {
            agt_not_add_to_payload(not, leafval);
        } else {
            payload_error(system_N_remoteHost, res);
        }
    }

} /* add_common_session_parms */


/********************************************************************
* FUNCTION init_static_vars
*
* Init the static vars
*
*********************************************************************/
static void
    init_static_sys_vars (void)
{
    sysmod = NULL;
    systemobj = NULL;
    sysStartupobj = NULL;
    sysConfigChangeobj = NULL;
    sysCapabilityChangeobj = NULL;
    sysSessionStartobj = NULL;
    sysSessionEndobj = NULL;
    sysConfirmedCommitobj = NULL;

} /* init_static_sys_vars */


/************* E X T E R N A L    F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_sys_init
*
* INIT 1:
*   Initialize the server notification module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_sys_init (void)
{
    agt_profile_t  *agt_profile;
    status_t        res;

    if (agt_sys_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    if (LOGDEBUG2) {
        log_debug2("\nagt_sys: Loading notifications module");
    }

    agt_profile = agt_get_profile();
    init_static_sys_vars();
    agt_sys_init_done = TRUE;

    /* load the system module */
    res = ncxmod_load_module(AGT_SYS_MODULE, 
                             NULL, 
                             &agt_profile->agt_savedevQ,
                             &sysmod);
    if (res != NO_ERR) {
        return res;
    }

    /* find the object definition for the system element */
    systemobj = ncx_find_object(sysmod,
                                system_N_system);

    if (!systemobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysStartupobj = 
        ncx_find_object(sysmod,
                        system_N_sysStartup);
    if (!sysStartupobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysConfigChangeobj = 
        ncx_find_object(sysmod,
                        system_N_sysConfigChange);
    if (!sysConfigChangeobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysCapabilityChangeobj = 
        ncx_find_object(sysmod,
                        system_N_sysCapabilityChange);
    if (!sysCapabilityChangeobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysSessionStartobj = 
        ncx_find_object(sysmod,
                        system_N_sysSessionStart);
    if (!sysSessionStartobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysSessionEndobj = 
        ncx_find_object(sysmod,
                        system_N_sysSessionEnd);
    if (!sysSessionEndobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sysConfirmedCommitobj = 
        ncx_find_object(sysmod,
                        system_N_sysConfirmedCommit);
    if (!sysConfirmedCommitobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* set up set-log-level RPC operation */
    res = agt_rpc_register_method(AGT_SYS_MODULE,
                                  system_N_set_log_level,
                                  AGT_RPC_PH_INVOKE,
                                  set_log_level_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    return NO_ERR;

}  /* agt_sys_init */


/********************************************************************
* FUNCTION agt_sys_init2
*
* INIT 2:
*   Initialize the monitoring data structures
*   This must be done after the <running> config is loaded
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_sys_init2 (void)
{
    val_value_t           *topval, *unameval, *childval, *tempval;
    cfg_template_t        *runningcfg;
    const xmlChar         *myhostname;
    obj_template_t        *unameobj;
    status_t               res;
    xmlChar               *buffer, *p, tstampbuff[TSTAMP_MIN_SIZE];
    struct utsname         utsbuff;
    int                    retval;

    if (!agt_sys_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    /* get the running config to add some static data into */
    runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* add /system */
    topval = val_new_value();
    if (!topval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(topval, systemobj);

    /* handing off the malloced memory here */
    val_add_child_sorted(topval, runningcfg->root);

    /* add /system/sysName */
    myhostname = (const xmlChar *)getenv("HOSTNAME");
    if (!myhostname) {
        myhostname = (const xmlChar *)"localhost";
    }
    childval = agt_make_leaf(systemobj, system_N_sysName, myhostname, &res);
    if (childval) {
        val_add_child(childval, topval);
    } else {
        return res;
    }

    /* add /system/sysCurrentDateTime */
    childval = agt_make_virtual_leaf(systemobj, system_N_sysCurrentDateTime,
                                     get_currentDateTime, &res);
    if (childval) {
        val_add_child(childval, topval);
    } else {
        return res;
    }

    /* add /system/sysBootDateTime */
    tstamp_datetime(tstampbuff);
    childval = agt_make_leaf(systemobj, system_N_sysBootDateTime,
                             tstampbuff, &res);
    if (childval) {
        val_add_child(childval, topval);
    } else {
        return res;
    }

    /* add /system/sysLogLevel */
    childval = agt_make_virtual_leaf(systemobj, system_N_sysLogLevel,
                                     get_currentLogLevel, &res);
    if (childval) {
        val_add_child(childval, topval);
    } else {
        return res;
    }

    /* add /system/sysNetconfServerId */
    buffer = m__getMem(256);
    if (buffer == NULL) {
        return ERR_INTERNAL_MEM;
    }
    p = buffer;
    p += xml_strcpy(p, (const xmlChar *)"netconfd ");

    res = ncx_get_version(p, 247);
    if (res == NO_ERR) {
        childval = agt_make_leaf(systemobj, system_N_sysNetconfServerId,
                                 buffer, &res);
        m__free(buffer);
        if (childval) {
            val_add_child(childval, topval);
        } else {
            return res;
        }
    } else {
        m__free(buffer);
        log_error("\nError: could not get netconfd version");
    }
    buffer = NULL;

    /* add /system/sysNetconfServerCLI */
    tempval = val_clone(agt_cli_get_valset());
    if (tempval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    val_change_nsid(tempval, topval->nsid);

    childval = agt_make_object(systemobj, system_N_sysNetconfServerCLI, &res);
    if (childval == NULL) {
        val_free_value(tempval);
        return ERR_INTERNAL_MEM;
    }
    val_move_children(tempval, childval);
    val_free_value(tempval);
    val_add_child(childval, topval);

    /* get the system information */
    memset(&utsbuff, 0x0, sizeof(utsbuff));
    retval = uname(&utsbuff);
    if (retval) {
        log_warn("\nWarning: <uname> data not available");
    } else {
        unameobj = obj_find_child(systemobj, AGT_SYS_MODULE, system_N_uname);
        if (!unameobj) {
            return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        } 

        /* add /system/uname */
        unameval = val_new_value();
        if (!unameval) {
            return ERR_INTERNAL_MEM;
        }
        val_init_from_template(unameval, unameobj);
        val_add_child(unameval, topval);

        /* add /system/uname/sysname */
        childval = agt_make_leaf(unameobj, system_N_sysname,
                                 (const xmlChar *)utsbuff.sysname, &res);
        if (childval) {
            val_add_child(childval, unameval);
        } else {
            return res;
        }

        /* add /system/uname/release */
        childval = agt_make_leaf(unameobj, system_N_release,
                                 (const xmlChar *)utsbuff.release, &res);
        if (childval) {
            val_add_child(childval, unameval);
        } else {
            return res;
        }

        /* add /system/uname/version */
        childval = agt_make_leaf(unameobj, system_N_version,
                                 (const xmlChar *)utsbuff.version, &res);
        if (childval) {
            val_add_child(childval, unameval);
        } else {
            return res;
        }
        
        /* add /system/uname/machine */
        childval = agt_make_leaf(unameobj, system_N_machine,
                                 (const xmlChar *)utsbuff.machine, &res);
        if (childval) {
            val_add_child(childval, unameval);
        } else {
            return res;
        }

        /* add /system/uname/nodename */
        childval = agt_make_leaf(unameobj, system_N_nodename,
                                 (const xmlChar *)utsbuff.nodename, &res);
        if (childval) {
            val_add_child(childval, unameval);
        } else {
            return res;
        }
    }

    /* add sysStartup to notificationQ */
    send_sysStartup();

    return NO_ERR;

}  /* agt_sys_init2 */


/********************************************************************
* FUNCTION agt_sys_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   
* RETURNS:
*   none
*********************************************************************/
void 
    agt_sys_cleanup (void)
{
    if (agt_sys_init_done) {
        init_static_sys_vars();
        agt_sys_init_done = FALSE;
        agt_rpc_unregister_method(AGT_SYS_MODULE,
                                  system_N_set_log_level);
    }
}  /* agt_sys_cleanup */


/********************************************************************
* FUNCTION agt_sys_send_sysSessionStart
*
* Queue the <sysSessionStart> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
void
    agt_sys_send_sysSessionStart (const ses_cb_t *scb)
{
    agt_not_msg_t         *not;

    if (LOGDEBUG) {
        log_debug("\nagt_sys: generating <sysSessionStart> "
                  "notification");
    }

    not = agt_not_new_notification(sysSessionStartobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <sysSessionStartup>");
        return;
    }

    add_common_session_parms(scb, not);

    agt_not_queue_notification(not);

} /* agt_sys_send_sysSessionStart */


/********************************************************************
* FUNCTION getTermReasonStr
*
* Convert the termination reason enum to a string
*
* INPUTS:
*   termreason == enum for the terminationReason leaf
*
* OUTPUTS:
*   the termination reason string
*
*********************************************************************/
static const xmlChar*
    getTermReasonStr ( ses_term_reason_t termreason)
{
    const xmlChar         *termreasonstr;

    switch (termreason) {
    case SES_TR_NONE:
        SET_ERROR(ERR_INTERNAL_VAL);
        termreasonstr = (const xmlChar *)"other";
        break;
    case SES_TR_CLOSED:
        termreasonstr = (const xmlChar *)"closed";
        break;
    case SES_TR_KILLED:
        termreasonstr = (const xmlChar *)"killed";
        break;
    case SES_TR_DROPPED:
        termreasonstr = (const xmlChar *)"dropped";
        break;
    case SES_TR_TIMEOUT:
        termreasonstr = (const xmlChar *)"timeout";
        break;
    case SES_TR_OTHER:
        termreasonstr = (const xmlChar *)"other";
        break;
    case SES_TR_BAD_START:
        termreasonstr = (const xmlChar *)"bad-start";
        break;
    case SES_TR_BAD_HELLO:
        termreasonstr = (const xmlChar *)"bad-hello";
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        termreasonstr = (const xmlChar *)"other";
    }

    return termreasonstr;
}


/********************************************************************
* FUNCTION agt_sys_send_sysSessionEnd
*
* Queue the <sysSessionEnd> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   termreason == enum for the terminationReason leaf
*   killedby == session-id for killedBy leaf if termreason == "killed"
*               ignored otherwise
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
void
    agt_sys_send_sysSessionEnd (const ses_cb_t *scb,
                                ses_term_reason_t termreason,
                                ses_id_t killedby)
{
    agt_not_msg_t         *not;
    val_value_t           *leafval;
    const xmlChar         *termreasonstr;
    status_t               res;

    assert(scb && "agt_sys_send_sysSessionEnd() - param scb is NULL");

    log_debug("\nagt_sys: generating <sysSessionEnd> notification");

    not = agt_not_new_notification(sysSessionEndobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot send <sysSessionEnd>");
        return;
    }

    /* session started;  not just being killed
     * in the <ncxconnect> message handler */
    if (termreason != SES_TR_BAD_START) {
        add_common_session_parms(scb, not);
    }

    /* add sysSessionEnd/killedBy */
    if (termreason == SES_TR_KILLED) {
        leafval = agt_make_uint_leaf( sysSessionEndobj, system_N_killedBy, 
                                      killedby, &res );
        if (leafval) {
            agt_not_add_to_payload(not, leafval);
        } else {
            payload_error(system_N_killedBy, res);
        }
    }

    /* add sysSessionEnd/terminationReason */
    termreasonstr = getTermReasonStr(termreason);
    leafval = agt_make_leaf( sysSessionEndobj, system_N_terminationReason, 
                             termreasonstr, &res );
    if (leafval) {
        agt_not_add_to_payload(not, leafval);
    } else {
        payload_error(system_N_terminationReason, res);
    }

    agt_not_queue_notification(not);
} /* agt_sys_send_sysSessionEnd */



/********************************************************************
* FUNCTION agt_sys_send_sysConfigChange
*
* Queue the <sysConfigChange> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   auditrecQ == Q of agt_cfg_audit_rec_t structs to use
*                for the notification payload contents
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
void
    agt_sys_send_sysConfigChange (const ses_cb_t *scb,
                                  dlq_hdr_t *auditrecQ)
{
    agt_not_msg_t         *not;
    agt_cfg_audit_rec_t   *auditrec;
    val_value_t           *leafval, *listval;
    obj_template_t        *listobj;
    status_t               res;

#ifdef DEBUG
    if (!scb || !auditrecQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (LOGDEBUG) {
        log_debug("\nagt_sys: generating <sysConfigChange> "
                  "notification");
    }

    not = agt_not_new_notification(sysConfigChangeobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <sysConfigChange>");
        return;
    }

    add_common_session_parms(scb, not);

    listobj = obj_find_child(sysConfigChangeobj,
                             AGT_SYS_MODULE,
                             system_N_edit);
    if (listobj == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    } else {
        for (auditrec = (agt_cfg_audit_rec_t *)dlq_firstEntry(auditrecQ);
             auditrec != NULL;
             auditrec = (agt_cfg_audit_rec_t *)dlq_nextEntry(auditrec)) {

            /* add sysConfigChange/edit */
            listval = val_new_value();
            if (listval == NULL) {
                payload_error(system_N_edit, ERR_INTERNAL_MEM);
            } else {
                val_init_from_template(listval, listobj);

                /* pass off listval malloc here */
                agt_not_add_to_payload(not, listval);            

                /* add sysConfigChange/edit/target */
                leafval = agt_make_leaf(listobj,
                                        system_N_target,
                                        auditrec->target,
                                        &res);
                if (leafval) {
                    val_add_child(leafval, listval);
                } else {
                    payload_error(system_N_target, res);
                }

                /* add sysConfigChange/edit/operation */
                leafval = agt_make_leaf(listobj,
                                        system_N_operation,
                                        op_editop_name(auditrec->editop),
                                        &res);
                if (leafval) {
                    val_add_child(leafval, listval);
                } else {
                    payload_error(system_N_operation, res);
                }
            }
        }
    }

    agt_not_queue_notification(not);

} /* agt_sys_send_sysConfigChange */


/********************************************************************
* FUNCTION agt_sys_send_sysCapabilityChange
*
* Send a <sysCapabilityChange> event for a module
* being added
*
* Queue the <sysCapabilityChange> notification
*
* INPUTS:
*   changed_by == session control block that made the
*                 change to add this module
*             == NULL if the server made the change
*   is_add    == TRUE if the capability is being added
*                FALSE if the capability is being deleted
*   capstr == capability string that was added or deleted
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
void
    agt_sys_send_sysCapabilityChange (ses_cb_t *changed_by,
                                      boolean is_add,
                                      const xmlChar *capstr)
{
    agt_not_msg_t         *not;
    val_value_t           *changedbyval, *leafval;
    obj_template_t        *changedbyobj;
    status_t               res;

#ifdef DEBUG
    if (!capstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (LOGDEBUG) {
        log_debug("\nagt_sys: generating <sysCapabilityChange> "
                  "notification");
    }

    changedbyobj = obj_find_child(sysCapabilityChangeobj,
                                  AGT_SYS_MODULE,
                                  system_N_changed_by);
    if (!changedbyobj) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    not = agt_not_new_notification(sysCapabilityChangeobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <sysCapabilityChange>");
        return;
    }

    changedbyval = val_new_value();
    if (!changedbyval) {
        log_error("\nError: malloc failed; cannot "
                  "send <sysCapabilityChange>");
        agt_not_free_notification(not);
        return;
    }
    val_init_from_template(changedbyval, changedbyobj);
    agt_not_add_to_payload(not, changedbyval);

    if (changed_by) {
        /* add userName */
        if (changed_by->username) {
            leafval = agt_make_leaf(changedbyobj,
                                    system_N_userName,
                                    changed_by->username,
                                    &res);
            if (leafval) {
                val_add_child(leafval, changedbyval);
            } else {
                payload_error(system_N_userName, res);
            }
        }

        /* add sessionId */
        leafval = agt_make_uint_leaf(changedbyobj,
                                     system_N_sessionId,
                                     changed_by->sid,
                                     &res);
        if (leafval) {
            val_add_child(leafval, changedbyval);
        } else {
            payload_error(system_N_sessionId, res);
        }

        /* add remoteHost */
        if (changed_by->peeraddr) {
            leafval = agt_make_leaf(changedbyobj,
                                    system_N_remoteHost,
                                    changed_by->peeraddr,
                                    &res);
            if (leafval) {
                val_add_child(leafval, changedbyval);
            } else {
                payload_error(system_N_remoteHost, res);
            }
        }
    } else {
        leafval = agt_make_leaf(changedbyobj,
                                NCX_EL_SERVER,
                                NULL,
                                &res);
        if (leafval) {
            val_add_child(leafval, changedbyval);
        } else {
            payload_error(NCX_EL_SERVER, res);
        }
    }

    if (is_add) {
        /* add sysCapoabilityChange/added-capability */
        leafval = agt_make_leaf(sysCapabilityChangeobj,
                                system_N_added_capability,
                                capstr,
                                &res);
    } else {
        /* add sysCapoabilityChange/deleted-capability */
        leafval = agt_make_leaf(sysCapabilityChangeobj,
                                system_N_deleted_capability,
                                capstr,
                                &res);
    }
    if (leafval) {
        agt_not_add_to_payload(not, leafval);
    } else {
        payload_error(is_add ? system_N_added_capability :
                      system_N_deleted_capability, res);
    }

    agt_not_queue_notification(not);

} /* agt_sys_send_sysCapabilityChange */


/********************************************************************
* FUNCTION agt_sys_send_sysConfirmedCommit
*
* Queue the <sysConfirmedCommit> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   event == enum for the confirmEvent leaf
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
void
    agt_sys_send_sysConfirmedCommit (const ses_cb_t *scb,
                                     ncx_confirm_event_t event)
{
    agt_not_msg_t         *not;
    val_value_t           *leafval;
    const xmlChar         *eventstr;
    status_t               res;

    res = NO_ERR;

    eventstr = ncx_get_confirm_event_str(event);

    if (!eventstr) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    if (LOGDEBUG) {
        log_debug("\nagt_sys: generating <sysConfirmedCommit> "
                  "notification (%s)",
                  eventstr);
    }

    not = agt_not_new_notification(sysConfirmedCommitobj);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <sysConfirmedCommit>");
        return;
    }

    if (scb) {
        add_common_session_parms(scb, not);
    }

    /* add sysConfirmedCommit/confirmEvent */
    leafval = agt_make_leaf(sysConfirmedCommitobj,
                            system_N_confirmEvent,
                            eventstr,
                            &res);
    if (leafval) {
        agt_not_add_to_payload(not, leafval);
    } else {
        payload_error(system_N_confirmEvent, res);
    }

    agt_not_queue_notification(not);

} /* agt_sys_send_sysConfirmedCommit */



/* END file agt_sys.c */
