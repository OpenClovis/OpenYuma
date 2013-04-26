/* 
 * 
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * All Rights Reserved.
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *

    module ietf-netconf-partial-lock
    revision 2009-10-19
    file: agt_plock.c:

    namespace urn:ietf:params:xml:ns:netconf:partial-lock:1.0
    organization IETF Network Configuration (netconf) Working Group

 */

#include <xmlstring.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cb.h"
#include "agt_ncx.h"
#include "agt_plock.h"
#include "agt_rpc.h"
#include "agt_timer.h"
#include "agt_util.h"
#include "dlq.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "plock.h"
#include "plock_cb.h"
#include "rpc.h"
#include "ses.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "xml_util.h"
#include "xml_val.h"
#include "xpath.h"
#include "xpath1.h"


/* module static variables */
static ncx_module_t *ietf_netconf_partial_lock_mod;
static obj_template_t *partial_lock_obj;
static obj_template_t *partial_unlock_obj;

/* put your static variables here */
static boolean agt_plock_init_done = FALSE;


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_partial_lock_validate
* 
* RPC validation phase
* All YANG constraints have passed at this point.
* Add description-stmt checks in this function.
* 
* INPUTS:
*     see agt/agt_rpc.h for details
* 
* RETURNS:
*     error status
********************************************************************/
static status_t
    y_ietf_netconf_partial_lock_partial_lock_validate (
        ses_cb_t *scb,
        rpc_msg_t *msg,
        xml_node_t *methnode)
{
    plock_cb_t *plcb;
    cfg_template_t *running;
    val_value_t *select_val, *testval;
    xpath_result_t *result;
    xpath_resnode_t *resnode;
    status_t res, retres;
    ses_id_t lockowner;

    res = NO_ERR;
    retres = NO_ERR;
    plcb = NULL;
    result = NULL;

    running = cfg_get_config_id(NCX_CFGID_RUNNING);

    /* make sure the running config state is READY */
    res = cfg_ok_to_partial_lock(running);
    if (res != NO_ERR) {
        agt_record_error(scb,
                         &msg->mhdr,
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_NONE,
                         NULL);
        return res;
    }

    /* the stack has already made sure at least 1 of these
     * is present because min-elements=1
     */
    select_val = val_find_child
        (msg->rpc_input,
         y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
         y_ietf_netconf_partial_lock_N_select);
    if (select_val == NULL || select_val->res != NO_ERR) {
        /* should not happen */
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* allocate a new lock cb
     * the plcb pointer will be NULL if the result is not NO_ERR
     */
    res = NO_ERR;
    plcb = plock_cb_new(SES_MY_SID(scb), &res);
    if (res == ERR_NCX_RESOURCE_DENIED && !plcb &&
        !cfg_is_partial_locked(running)) {
        /* no partial locks so it is safe to reset the lock index */
        plock_cb_reset_id();
        res = NO_ERR;
        plcb = plock_cb_new(SES_MY_SID(scb), &res);
    }

    if (res != NO_ERR || !plcb ) {
        if ( res==NO_ERR && !plcb ) {
            res = ERR_INTERNAL_MEM;
        }
        agt_record_error(scb,
                         &msg->mhdr,
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_NONE,
                         NULL);
        if ( plcb ) {
            plock_cb_free(plcb);
        }
        return res;
    }

    /* get all the select parm instances and save them
     * in the partial lock cb
     */
    while (select_val != NULL) {
        result = xpath1_eval_xmlexpr(scb->reader,
                                     select_val->xpathpcb,
                                     running->root,
                                     running->root,
                                     FALSE,  /* logerrors */
                                     TRUE,   /* config-only */
                                     &res);

        if (result == NULL || res != NO_ERR) {
            /* should not get invalid XPath expression
             * but maybe something was valid in the
             * object tree but not in the value tree
             */
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_OPERATION,
                             res,
                             methnode,
                             NCX_NT_STRING,
                             VAL_STRING(select_val),
                             NCX_NT_VAL,
                             select_val);
            retres = res;
            xpath_free_result(result);
        } else if (result->restype != XP_RT_NODESET) {
            res = ERR_NCX_XPATH_NOT_NODESET;
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_OPERATION,
                             res,
                             methnode,
                             NCX_NT_STRING,
                             VAL_STRING(select_val),
                             NCX_NT_VAL,
                             select_val);
            retres = res;
            xpath_free_result(result);
        } else {
            /* save these pointers; the result will be
             * pruned for redundant nodes after all the
             * select expressions have been processed;
             * transfer the memory straight across
             */
            plock_add_select(plcb, 
                             select_val->xpathpcb,
                             result);
            select_val->xpathpcb = NULL;
            result = NULL;
        }

        /* set up the next select value even if errors so far */
        select_val = val_find_next_child
            (msg->rpc_input,
             y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
             y_ietf_netconf_partial_lock_N_select,
             select_val);
    }

    /* check the result for non-empty only if all
     * the select stmts were valid
     */
    if (retres == NO_ERR) {
        res = plock_make_final_result(plcb);
        if (res != NO_ERR) {
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_OPERATION,
                             res,
                             methnode,
                             NCX_NT_NONE,
                             NULL,
                             NCX_NT_VAL,
                             select_val);
            retres = res;
        }
    }

    /* make sure all the nodeptrs identify subtrees in the
     * running config that can really be locked right now
     * only if the final result is a non-empty nodeset
     */
    if (retres == NO_ERR) {
        result = plock_get_final_result(plcb);

        for (resnode = xpath_get_first_resnode(result);
             resnode != NULL;
             resnode = xpath_get_next_resnode(resnode)) {

            testval = xpath_get_resnode_valptr(resnode);

            /* !!! Update 2011-09-27
             * Just talked to the RFC author; Martin and I
             * agree RFC 5717 does not specify that write 
             * access be required to create a partial lock
             * In fact -- a user may want to lock a node
             * to do a stable read
             **** deleted agt_acm check ***/

            /* make sure there is a plock slot available
             * and no part of this subtree is already locked
             * do not check lock conflicts if this subtree
             * is unauthorized for writing
             */
            lockowner = 0;
            res = val_ok_to_partial_lock(testval, SES_MY_SID(scb),
                                         &lockowner);
            if (res != NO_ERR) {
                if (res == ERR_NCX_LOCK_DENIED) {
                    agt_record_error(scb,
                                     &msg->mhdr,
                                     NCX_LAYER_OPERATION,
                                     res,
                                     methnode,
                                     NCX_NT_UINT32_PTR,
                                     &lockowner,
                                     NCX_NT_VAL,
                                     testval);
                } else {
                    agt_record_error(scb,
                                     &msg->mhdr,
                                     NCX_LAYER_OPERATION,
                                     res,
                                     methnode,
                                     NCX_NT_NONE,
                                     NULL,
                                     NCX_NT_VAL,
                                     testval);
                }
                retres = res;
            }
        }  // for loop
    }  // if retres == NO_ERR

    /* make sure there is no partial commit in progress */
    if (agt_ncx_cc_active()) {
        res = ERR_NCX_IN_USE_COMMIT;
        agt_record_error(scb,
                         &msg->mhdr,
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_NONE,
                         NULL);
        retres = res;
    }

    /* save the plock CB only if there were no errors;
     * otherwise the invoke function will not get called
     */
    if (retres == NO_ERR) {
        /* the invoke function must free this pointer */
        msg->rpc_user1 = plcb;
    } else {
        plock_cb_free(plcb);
    }

    return retres;

} /* y_ietf_netconf_partial_lock_partial_lock_validate */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_partial_lock_invoke
* 
* RPC invocation phase
* All constraints have passed at this point.
* Call device instrumentation code in this function.
* 
* INPUTS:
*     see agt/agt_rpc.h for details
* 
* RETURNS:
*     error status
********************************************************************/
static status_t
    y_ietf_netconf_partial_lock_partial_lock_invoke (
        ses_cb_t *scb,
        rpc_msg_t *msg,
        xml_node_t *methnode)
{
    plock_cb_t *plcb;
    val_value_t *testval, *newval;
    xpath_result_t *result;
    xpath_resnode_t *resnode, *clearnode;
    xmlChar *pathbuff;
    cfg_template_t *running;
    status_t res;
    ncx_num_t num;

    res = NO_ERR;

    plcb = (plock_cb_t *)msg->rpc_user1;
    result = plock_get_final_result(plcb);
    running = cfg_get_config_id(NCX_CFGID_RUNNING);

    /* try to lock all the target nodes */
    for (resnode = xpath_get_first_resnode(result);
         resnode != NULL && res == NO_ERR;
         resnode = xpath_get_next_resnode(resnode)) {

        testval = xpath_get_resnode_valptr(resnode);

        res = val_set_partial_lock(testval, plcb);
        if (res != NO_ERR) {
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_OPERATION,
                             res,
                             methnode,
                             NCX_NT_NONE,
                             NULL,
                             NCX_NT_VAL,
                             testval);
        }
    }

    /* unlock any nodes already attempted if any fail */
    if (res != NO_ERR) {
        for (clearnode = xpath_get_first_resnode(result);
             clearnode != NULL;
             clearnode = xpath_get_next_resnode(clearnode)) {

            testval = xpath_get_resnode_valptr(clearnode);

            val_clear_partial_lock(testval, plcb);
            if (clearnode == resnode) {
                return res;
            }
        }
        return res;
    }

    /* add this partial lock to the running config */
    res = cfg_add_partial_lock(running, plcb);
    if (res != NO_ERR) {
        /* should not happen since config lock state could
         * not have changed since validate callback
         */
        agt_record_error(scb,
                         &msg->mhdr,
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_NONE,
                         NULL);
        for (clearnode = xpath_get_first_resnode(result);
             clearnode != NULL;
             clearnode = xpath_get_next_resnode(clearnode)) {

            testval = xpath_get_resnode_valptr(clearnode);
            val_clear_partial_lock(testval, plcb);
        }
        plock_cb_free(plcb);
        return res;
    }

    /* setup return data only if lock successful
     * cache the reply instead of stream the reply
     * in case there is any error; if so; the partial
     * lock will be backed out and the dataQ cleaned
     * for an error exit; add lock-id leaf first 
     */
    msg->rpc_data_type = RPC_DATA_YANG;
    
    ncx_init_num(&num);
    num.u = plock_get_id(plcb);
    newval = xml_val_new_number
        (y_ietf_netconf_partial_lock_N_lock_id,
         val_get_nsid(msg->rpc_input),
         &num,
         NCX_BT_UINT32);
    ncx_clean_num(NCX_BT_UINT32, &num);

    if (newval == NULL) {
        res = ERR_INTERNAL_MEM;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
    } else {
        dlq_enque(newval, &msg->rpc_dataQ);
    }

    /* add lock-node leaf-list instance for each resnode */
    for (resnode = xpath_get_first_resnode(result);
         resnode != NULL && res == NO_ERR;
         resnode = xpath_get_next_resnode(resnode)) {

        /* Q&D method: generate the i-i string as a plain
         * string and add any needed prefixes to the global
         * prefix map for the reply message (in mhdr)
         */
        pathbuff = NULL;
        testval = xpath_get_resnode_valptr(resnode);
        res = val_gen_instance_id(&msg->mhdr, 
                                  testval,
                                  NCX_IFMT_XPATH1, 
                                  &pathbuff);
        if (res == NO_ERR) {
            /* make leaf; pass off pathbuff malloced memory */
            newval = xml_val_new_string
                (y_ietf_netconf_partial_lock_N_locked_node,
                 val_get_nsid(msg->rpc_input),
                 pathbuff);
            if (newval == NULL) {
                res = ERR_INTERNAL_MEM;
                m__free(pathbuff);
                pathbuff = NULL;
            }
        }

        if (res == NO_ERR) {
            dlq_enque(newval, &msg->rpc_dataQ);
        } else {
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE, 
                             NULL, 
                             NCX_NT_NONE, 
                             NULL);
        }
    }

    if (res != NO_ERR) {
        /* back out everything, except waste the lock ID */
        for (clearnode = xpath_get_first_resnode(result);
             clearnode != NULL;
             clearnode = xpath_get_next_resnode(clearnode)) {

            testval = xpath_get_resnode_valptr(clearnode);
            val_clear_partial_lock(testval, plcb);
        }
        cfg_delete_partial_lock(running, plock_get_id(plcb));

        /* clear any data already queued */
        while (!dlq_empty(&msg->rpc_dataQ)) {
            testval = (val_value_t *)
                dlq_deque(&msg->rpc_dataQ);
            val_free_value(testval);
        }
        msg->rpc_data_type = RPC_DATA_NONE;
    }

    return res;

} /* y_ietf_netconf_partial_lock_partial_lock_invoke */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_partial_unlock_validate
* 
* RPC validation phase
* All YANG constraints have passed at this point.
* Add description-stmt checks in this function.
* 
* INPUTS:
*     see agt/agt_rpc.h for details
* 
* RETURNS:
*     error status
********************************************************************/
static status_t
    y_ietf_netconf_partial_lock_partial_unlock_validate (
        ses_cb_t *scb,
        rpc_msg_t *msg,
        xml_node_t *methnode)
{
    val_value_t *lock_id_val;
    cfg_template_t *running;
    plock_cb_t *plcb;
    uint32 lock_id;
    status_t res;

    res = NO_ERR;

    lock_id_val = val_find_child(
        msg->rpc_input,
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_N_lock_id);
    if (lock_id_val == NULL || lock_id_val->res != NO_ERR) {
        return ERR_NCX_INVALID_VALUE;
    }

    lock_id = VAL_UINT(lock_id_val);

    running = cfg_get_config_id(NCX_CFGID_RUNNING);

    plcb = cfg_find_partial_lock(running, lock_id);

    if (plcb == NULL || (plock_get_sid(plcb) != SES_MY_SID(scb))) {
        res = ERR_NCX_INVALID_VALUE;
        agt_record_error(
            scb,
            &msg->mhdr,
            NCX_LAYER_OPERATION,
            res,
            methnode,
            NCX_NT_NONE,
            NULL,
            NCX_NT_VAL,
            lock_id_val);
    } else {
        msg->rpc_user1 = plcb;
    }
    
    return res;

} /* y_ietf_netconf_partial_lock_partial_unlock_validate */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_partial_unlock_invoke
* 
* RPC invocation phase
* All constraints have passed at this point.
* Call device instrumentation code in this function.
* 
* INPUTS:
*     see agt/agt_rpc.h for details
* 
* RETURNS:
*     error status
********************************************************************/
static status_t
    y_ietf_netconf_partial_lock_partial_unlock_invoke (
        ses_cb_t *scb,
        rpc_msg_t *msg,
        xml_node_t *methnode)
{
    plock_cb_t  *plcb;
    cfg_template_t *running;

    (void)scb;
    (void)methnode;

    plcb = (plock_cb_t *)msg->rpc_user1;
    running = cfg_get_config_id(NCX_CFGID_RUNNING);

    cfg_delete_partial_lock(running,
                            plock_get_id(plcb));
    return NO_ERR;

} /* y_ietf_netconf_partial_lock_partial_unlock_invoke */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_init_static_vars
* 
* initialize module static variables
* 
********************************************************************/
static void
    y_ietf_netconf_partial_lock_init_static_vars (void)
{
    ietf_netconf_partial_lock_mod = NULL;
    partial_lock_obj = NULL;
    partial_unlock_obj = NULL;

    /* init your static variables here */

} /* y_ietf_netconf_partial_lock_init_static_vars */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_init
* 
* initialize the ietf-netconf-partial-lock server instrumentation library
* 
* INPUTS:
*    modname == requested module name
*    revision == requested version (NULL for any)
* 
* RETURNS:
*     error status
********************************************************************/
status_t
    y_ietf_netconf_partial_lock_init (
        const xmlChar *modname,
        const xmlChar *revision)
{
    agt_profile_t *agt_profile;
    status_t res;

    y_ietf_netconf_partial_lock_init_static_vars();

    /* change if custom handling done */
    if (xml_strcmp(modname, 
                   y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock)) {
        return ERR_NCX_UNKNOWN_MODULE;
    }

    if (revision && xml_strcmp
        (revision, y_ietf_netconf_partial_lock_R_ietf_netconf_partial_lock)) {
        return ERR_NCX_WRONG_VERSION;
    }

    agt_profile = agt_get_profile();

    res = ncxmod_load_module(
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_R_ietf_netconf_partial_lock,
        &agt_profile->agt_savedevQ,
        &ietf_netconf_partial_lock_mod);
    if (res != NO_ERR) {
        return res;
    }

    agt_plock_init_done = TRUE;

    partial_lock_obj = ncx_find_object(
        ietf_netconf_partial_lock_mod,
        y_ietf_netconf_partial_lock_N_partial_lock);
    if (ietf_netconf_partial_lock_mod == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }
    
    partial_unlock_obj = ncx_find_object(
        ietf_netconf_partial_lock_mod,
        y_ietf_netconf_partial_lock_N_partial_unlock);
    if (ietf_netconf_partial_lock_mod == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }
    
    res = agt_rpc_register_method(
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_N_partial_lock,
        AGT_RPC_PH_VALIDATE,
        y_ietf_netconf_partial_lock_partial_lock_validate);
    if (res != NO_ERR) {
        return res;
    }
    
    res = agt_rpc_register_method(
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_N_partial_lock,
        AGT_RPC_PH_INVOKE,
        y_ietf_netconf_partial_lock_partial_lock_invoke);
    if (res != NO_ERR) {
        return res;
    }
    
    res = agt_rpc_register_method(
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_N_partial_unlock,
        AGT_RPC_PH_VALIDATE,
        y_ietf_netconf_partial_lock_partial_unlock_validate);
    if (res != NO_ERR) {
        return res;
    }
    
    res = agt_rpc_register_method(
        y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
        y_ietf_netconf_partial_lock_N_partial_unlock,
        AGT_RPC_PH_INVOKE,
        y_ietf_netconf_partial_lock_partial_unlock_invoke);
    if (res != NO_ERR) {
        return res;
    }
    
    /* put your module initialization code here */
    
    return res;
} /* y_ietf_netconf_partial_lock_init */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_init2
* 
* SIL init phase 2: non-config data structures
* Called after running config is loaded
* 
* RETURNS:
*     error status
********************************************************************/
status_t
    y_ietf_netconf_partial_lock_init2 (void)
{
    status_t res;

    res = NO_ERR;
    /* put your init2 code here */
    
    return res;
} /* y_ietf_netconf_partial_lock_init2 */


/********************************************************************
* FUNCTION y_ietf_netconf_partial_lock_cleanup
*    cleanup the server instrumentation library
* 
********************************************************************/
void
    y_ietf_netconf_partial_lock_cleanup (void)
{
    if (agt_plock_init_done) {
        agt_rpc_unregister_method
            (y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
             y_ietf_netconf_partial_lock_N_partial_lock);

        agt_rpc_unregister_method
            (y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
             y_ietf_netconf_partial_lock_N_partial_unlock);

    }
    
} /* y_ietf_netconf_partial_lock_cleanup */

/* END agt_plock.c */
