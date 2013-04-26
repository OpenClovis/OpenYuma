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
/*  FILE: mgr_not.c

   NETCONF Protocol Operations: RPC Manager Side Support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
17may05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_mgr
#include  "mgr.h"
#endif

#ifndef _H_mgr_not
#include  "mgr_not.h"
#endif

#ifndef _H_mgr_ses
#include  "mgr_ses.h"
#endif

#ifndef _H_mgr_val_parse
#include  "mgr_val_parse.h"
#endif

#ifndef _H_mgr_xml
#include  "mgr_xml.h"
#endif

#ifndef _H_ncx
#include  "ncx.h"
#endif

#ifndef _H_ncxconst
#include  "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include  "ncxtypes.h"
#endif

#ifndef _H_obj
#include  "obj.h"
#endif

#ifndef _H_rpc
#include  "rpc.h"
#endif

#ifndef _H_rpc_err
#include  "rpc_err.h"
#endif

#ifndef _H_ses
#include  "ses.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_top
#include  "top.h"
#endif

#ifndef _H_typ
#include  "typ.h"
#endif

#ifndef _H_val
#include  "val.h"
#endif

#ifndef _H_xmlns
#include  "xmlns.h"
#endif

#ifndef _H_xml_msg
#include  "xml_msg.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_xml_wr
#include  "xml_wr.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define MGR_NOT_DEBUG 1
#endif

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
static boolean               mgr_not_init_done = FALSE;

static obj_template_t       *notification_obj;

mgr_not_cbfn_t               callbackfn;


/********************************************************************
* FUNCTION new_msg
*
* Malloc and initialize a new mgr_not_msg_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
static mgr_not_msg_t *
    new_msg (void)
{
    mgr_not_msg_t *msg;

    msg = m__getObj(mgr_not_msg_t);
    if (!msg) {
        return NULL;
    }
    memset(msg, 0x0, sizeof(mgr_not_msg_t));

    msg->notification = val_new_value();
    if (!msg->notification) {
        m__free(msg);
        return NULL;
    }
    /* xml_msg_init_hdr(&msg->mhdr); */
    return msg;

} /* new_msg */


/************** E X T E R N A L   F U N C T I O N S  ***************/


/********************************************************************
* FUNCTION mgr_not_init
*
* Initialize the mgr_not module
* call once to init module
* Adds the mgr_not_dispatch function as the handler
* for the NETCONF <rpc> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
status_t 
    mgr_not_init (void)
{
    status_t  res;

    if (!mgr_not_init_done) {
        res = top_register_node(NCN_MODULE,
                                NCX_EL_NOTIFICATION, 
                                mgr_not_dispatch);
        if (res != NO_ERR) {
            return res;
        }
        notification_obj = NULL;
        callbackfn = NULL;
        mgr_not_init_done = TRUE;
    }
    return NO_ERR;

} /* mgr_not_init */


/********************************************************************
* FUNCTION mgr_not_cleanup
*
* Cleanup the mgr_not module.
* call once to cleanup module
* Unregister the top-level NETCONF <rpc> element
*
*********************************************************************/
void 
    mgr_not_cleanup (void)
{
    if (mgr_not_init_done) {
        top_unregister_node(NCN_MODULE, NCX_EL_NOTIFICATION);
        notification_obj = NULL;
        callbackfn = NULL;
        mgr_not_init_done = FALSE;
    }

} /* mgr_not_cleanup */


/********************************************************************
* FUNCTION mgr_not_free_msg
*
* Free a mgr_not_msg_t struct
*
* INPUTS:
*   msg == struct to free
*
* RETURNS:
*   none
*********************************************************************/
void
    mgr_not_free_msg (mgr_not_msg_t *msg)
{
#ifdef DEBUG
    if (!msg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (msg->notification) {
        val_free_value(msg->notification);
    }

    m__free(msg);

} /* mgr_not_free_msg */


/********************************************************************
* FUNCTION mgr_not_clean_msgQ
*
* Clean the msg Q of mgr_not_msg_t entries
*
* INPUTS:
*   msgQ == Q of entries to free; the Q itself is not freed
*
* RETURNS:
*   none
*********************************************************************/
void
    mgr_not_clean_msgQ (dlq_hdr_t *msgQ)
{
    mgr_not_msg_t *msg;

#ifdef DEBUG
    if (!msgQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    msg = (mgr_not_msg_t *)dlq_deque(msgQ);
    while (msg) {
        mgr_not_free_msg(msg);
        msg = (mgr_not_msg_t *)dlq_deque(msgQ);
    }

} /* mgr_not_clean_msgQ */


/********************************************************************
* FUNCTION mgr_not_dispatch
*
* Dispatch an incoming <rpc-reply> response
* handle the <notification> element
* called by mgr_top.c: 
* This function is registered with top_register_node
* for the module 'notification', top-node 'notification'
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
void 
    mgr_not_dispatch (ses_cb_t *scb,
                      xml_node_t *top)
{
    obj_template_t          *notobj;
    mgr_not_msg_t           *msg;
    ncx_module_t            *mod;
    val_value_t             *child;
    boolean                  consumed;

#ifdef DEBUG
    if (!scb || !top) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* check if the notification template is already cached */
    if (notification_obj) {
        notobj = notification_obj;
    } else {
        /* no: get the notification template */
        notobj = NULL;
        mod = ncx_find_module(NCN_MODULE, NULL);
        if (mod) {
            notobj = ncx_find_object(mod, NCX_EL_NOTIFICATION);
        }
        if (notobj) {
            notification_obj = notobj;
        } else {
            SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
            mgr_xml_skip_subtree(scb->reader, top);
            return;
        }
    }

    /* the current node is 'notification' in the notifications namespace
     * First get a new notification msg struct
     */
    msg = new_msg();
    if (!msg) {
        log_error("\nError: mgr_not: skipping incoming message");
        mgr_xml_skip_subtree(scb->reader, top);
        return;
    }
    
    /* parse the notification as a val_value_t tree,
     * stored in msg->notification
     */
    msg->res = mgr_val_parse_notification(scb, 
                                          notobj,
                                          top, 
                                          msg->notification);
    if (msg->res != NO_ERR && LOGINFO) {        
        log_info("\nmgr_not: got invalid notification on session %d (%s)",
                 scb->sid, 
                 get_error_string(msg->res));
    } 

    /* check that there is nothing after the <rpc-reply> element */
    if (msg->res==NO_ERR && 
        !xml_docdone(scb->reader) && LOGINFO) {
        log_info("\nmgr_not: got extra nodes in notification on session %d",
                 scb->sid);
    }

    consumed = FALSE;

    if (msg->res == NO_ERR && msg->notification) {
        child = val_get_first_child(msg->notification);
        if (child) {
            if (!xml_strcmp(child->name, 
                            (const xmlChar *)"eventTime")) {
                msg->eventTime = child;
            } else {
                log_error("\nError: expected 'eventTime' in "
                          "notification, got '%s'",
                          child->name);
            }

            child = val_get_next_child(child);
            if (child) {
                /* eventType is expected to be next!! */
                msg->eventType = child;
            }
        } else {
            log_error("\nError: expected 'eventTime' in "
                      "notification, got nothing");
        }
        
        /* invoke the notification handler */
        if (callbackfn) {
            (*callbackfn)(scb, msg, &consumed);
        }
    }

    if (!consumed) {
        mgr_not_free_msg(msg);
    }

} /* mgr_not_dispatch */


/********************************************************************
* FUNCTION mgr_not_set_callback_fn
*
* Set the application callback function to handle
* notifications when they arrive
*
* INPUTS:
*   cbfn == callback function to use
*        == NULL to clear the callback function
*********************************************************************/
void 
    mgr_not_set_callback_fn (mgr_not_cbfn_t cbfn)
{
    callbackfn = cbfn;
}  /* mgr_not_set_callback_fn */


/* END file mgr_not.c */
