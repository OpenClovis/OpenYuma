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
/*  FILE: mgr_rpc.c

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
#include <sys/time.h>

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

#ifndef _H_mgr_rpc
#include  "mgr_rpc.h"
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

#ifndef _H_ncx_num
#include  "ncx_num.h"
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
#define MGR_RPC_DEBUG 1
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
static boolean mgr_rpc_init_done = FALSE;


/********************************************************************
* FUNCTION new_reply
*
* Malloc and initialize a new mgr_rpc_rpy_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
static mgr_rpc_rpy_t *
    new_reply (void)
{
    mgr_rpc_rpy_t *rpy;

    rpy = m__getObj(mgr_rpc_rpy_t);
    if (!rpy) {
        return NULL;
    }
    memset(rpy, 0x0, sizeof(mgr_rpc_rpy_t));

    rpy->reply = val_new_value();
    if (!rpy->reply) {
        m__free(rpy);
        return NULL;
    }
    /* xml_msg_init_hdr(&rpy->mhdr); */
    return rpy;

} /* new_reply */


/********************************************************************
* FUNCTION add_request
*
* Add an RPC request to the Q
* 
* INPUTS:
*   scb == session control block
*   req == mgr_rpc_req_t to add
*
* RETURNS:
*   none
*********************************************************************/
static void
    add_request (ses_cb_t *scb,
                 mgr_rpc_req_t  *req)
{
    mgr_scb_t *mscb;

    mscb = mgr_ses_get_mscb(scb);

    (void)time(&req->starttime);

    dlq_enque(req, &mscb->reqQ);

}  /* add_request */


/********************************************************************
* FUNCTION find_request
*
* Find an RPC request but do not remove it
* 
*
* The NETCONF protocol actually allows duplicates in
* the message-id attribute, but if a message is cancelled
* then only the first match is deleted
*
* INPUTS:
*   scb == session control block
*   msg_id == ID for the mgr_rpc_req_t to find
*
* RETURNS:
*   pointer to the request or NULL if not found
*********************************************************************/
static mgr_rpc_req_t *
    find_request (ses_cb_t *scb,
                  const xmlChar *msg_id)
{
    mgr_scb_t *mscb;
    mgr_rpc_req_t *req;

    mscb = mgr_ses_get_mscb(scb);

    for (req = (mgr_rpc_req_t *)dlq_firstEntry(&mscb->reqQ);
         req != NULL;
         req = (mgr_rpc_req_t *)dlq_nextEntry(req)) {
        if (!xml_strcmp(msg_id, req->msg_id)) {
            return req;
        }
    }
    return NULL;

}  /* find_request */


/************** E X T E R N A L   F U N C T I O N S  ***************/


/********************************************************************
* FUNCTION mgr_rpc_init
*
* Initialize the mgr_rpc module
* call once to init RPC mgr module
* Adds the mgr_rpc_dispatch function as the handler
* for the NETCONF <rpc> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
status_t 
    mgr_rpc_init (void)
{
    status_t  res;

    if (!mgr_rpc_init_done) {
        res = top_register_node(NC_MODULE,
                                NCX_EL_RPC_REPLY, 
                                mgr_rpc_dispatch);
        if (res != NO_ERR) {
            return res;
        }
        mgr_rpc_init_done = TRUE;
    }
    return NO_ERR;

} /* mgr_rpc_init */


/********************************************************************
* FUNCTION mgr_rpc_cleanup
*
* Cleanup the mgr_rpc module.
* call once to cleanup RPC mgr module
* Unregister the top-level NETCONF <rpc> element
*
*********************************************************************/
void 
    mgr_rpc_cleanup (void)
{
    if (mgr_rpc_init_done) {
        top_unregister_node(NC_MODULE, NCX_EL_RPC_REPLY);
        mgr_rpc_init_done = FALSE;
    }

} /* mgr_rpc_cleanup */


/********************************************************************
* FUNCTION mgr_rpc_new_request
*
* Malloc and initialize a new mgr_rpc_req_t struct
*
* INPUTS:
*   scb == session control block
*
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
mgr_rpc_req_t *
    mgr_rpc_new_request (ses_cb_t *scb)
{
    mgr_scb_t       *mscb;
    mgr_rpc_req_t   *req;
    char             numbuff[NCX_MAX_NUMLEN];

    req = m__getObj(mgr_rpc_req_t);
    if (!req) {
        return NULL;
    }
    memset(req, 0x0, sizeof(mgr_rpc_req_t));

    mscb = mgr_ses_get_mscb(scb);
    sprintf(numbuff, "%u", mscb->next_id);
    if (mscb->next_id >= MGR_MAX_REQUEST_ID) {
        mscb->next_id = 0;
    } else {
        mscb->next_id++;
    }

    req->msg_id = xml_strdup((const xmlChar *)numbuff);
    if (req->msg_id) {
        xml_msg_init_hdr(&req->mhdr);
        xml_init_attrs(&req->attrs);
    } else {
        m__free(req);
        req = NULL;
    }
    return req;

} /* mgr_rpc_new_request */


/********************************************************************
* FUNCTION mgr_rpc_free_request
*
* Free a mgr_rpc_req_t struct
*
* INPUTS:
*   req == struct to free
*
* RETURNS:
*   none
*********************************************************************/
void
    mgr_rpc_free_request (mgr_rpc_req_t *req)
{
#ifdef DEBUG
    if (!req) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (req->msg_id) {
        m__free(req->msg_id);
    }
    xml_clean_attrs(&req->attrs);
    if (req->data) {
        val_free_value(req->data);
    }
    m__free(req);

} /* mgr_rpc_free_request */


/********************************************************************
* FUNCTION mgr_rpc_free_reply
*
* Free a mgr_rpc_rpy_t struct
*
* INPUTS:
*   rpy == struct to free
*
* RETURNS:
*   none
*********************************************************************/
void
    mgr_rpc_free_reply (mgr_rpc_rpy_t *rpy)
{
#ifdef DEBUG
    if (!rpy) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (rpy->msg_id) {
        m__free(rpy->msg_id);
    }
    if (rpy->reply) {
        val_free_value(rpy->reply);
    }
    m__free(rpy);

} /* mgr_rpc_free_reply */


/********************************************************************
* FUNCTION mgr_rpc_clean_requestQ
*
* Clean the request Q of mgr_rpc_req_t entries
*
* INPUTS:
*   reqQ == Q of entries to free; the Q itself is not freed
*
* RETURNS:
*   none
*********************************************************************/
void
    mgr_rpc_clean_requestQ (dlq_hdr_t *reqQ)
{
    mgr_rpc_req_t *req;

#ifdef DEBUG
    if (!reqQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    req = (mgr_rpc_req_t *)dlq_deque(reqQ);
    while (req) {
        mgr_rpc_free_request(req);
        req = (mgr_rpc_req_t *)dlq_deque(reqQ);
    }

} /* mgr_rpc_clean_requestQ */


/********************************************************************
* FUNCTION mgr_rpc_timeout_requestQ
*
* Clean the request Q of mgr_rpc_req_t entries
* Only remove the entries that have timed out
*
* returning number of msgs timed out
* need a callback-based cleanup later on
* to support N concurrent requests per agent
*
* INPUTS:
*   reqQ == Q of entries to check
*
* RETURNS:
*   number of request timed out
*********************************************************************/
uint32
    mgr_rpc_timeout_requestQ (dlq_hdr_t *reqQ)
{
    mgr_rpc_req_t *req, *nextreq;
    time_t         timenow;
    double         timediff;
    uint32         deletecount;

#ifdef DEBUG
    if (!reqQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    deletecount = 0;
    (void)time(&timenow);

    for (req = (mgr_rpc_req_t *)dlq_firstEntry(reqQ);
         req != NULL;
         req = nextreq) {

        nextreq = (mgr_rpc_req_t *)dlq_nextEntry(req);

        if (!req->timeout) {
            continue;
        }

        timediff = difftime(timenow, req->starttime);
        if (timediff >= (double)req->timeout) {
            log_info("\nmgr_rpc: deleting timed out request '%s'",
                     req->msg_id);
            deletecount++;
            dlq_remove(req);
            mgr_rpc_free_request(req);
        }
    }

    return deletecount;

} /* mgr_rpc_timeout_requestQ */


/********************************************************************
* FUNCTION mgr_rpc_send_request
*
* Send an <rpc> request to the agent on the specified session
* non-blocking send, reply function will be called when
* one is received or a timeout occurs
*
* INPUTS:
*   scb == session control block
*   req == request to send
*   rpyfn == reply callback function
*
* RETURNS:
*   status
*********************************************************************/
status_t
    mgr_rpc_send_request (ses_cb_t *scb,
                          mgr_rpc_req_t *req,
                          mgr_rpc_cbfn_t rpyfn)
{
    xml_msg_hdr_t msg;
    xml_attr_t   *attr;
    status_t      res;
    boolean       anyout;
    xmlns_id_t    nc_id;

#ifdef DEBUG
    if (!scb || !req || !rpyfn) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef MGR_HELLO_DEBUG
    log_debug2("\nmgr sending RPC request %s on session %d", 
               req->msg_id, scb->sid);
#endif

    anyout = FALSE;
    xml_msg_init_hdr(&msg);
    nc_id = xmlns_nc_id();

    /* make sure the message-id attribute is not already present */
    attr = xml_find_attr_q(&req->attrs, 0, NCX_EL_MESSAGE_ID);
    if (attr) {
        dlq_remove(attr);
        xml_free_attr(attr);
    }

    /* setup the prefix map with the NETCONF (and maybe NCX) namespace */
    res = xml_msg_build_prefix_map(&msg, 
                                   &req->attrs, 
                                   FALSE, 
                                   (req->data->nsid == xmlns_ncx_id()));

    /* add the message-id attribute */
    if (res == NO_ERR) {
        res = xml_add_attr(&req->attrs, 
                           0, 
                           NCX_EL_MESSAGE_ID,
                           req->msg_id);
    }

    /* set perf timestamp in case response timing active */
    gettimeofday(&req->perfstarttime, NULL);     

    /* send the <?xml?> directive */
    if (res == NO_ERR) {
        res = ses_start_msg(scb);
    }

    /* start the <rpc> element */
    if (res == NO_ERR) {
        anyout = TRUE;
        xml_wr_begin_elem_ex(scb, 
                             &msg, 
                             0, 
                             nc_id, 
                             NCX_EL_RPC, 
                             &req->attrs, 
                             ATTRQ, 
                             0, 
                             START);
    }
    
    /* send the method and parameters */
    if (res == NO_ERR) {
        xml_wr_full_val(scb, &msg, req->data, NCX_DEF_INDENT);
    }

    /* finish the <rpc> element */
    if (res == NO_ERR) {
        xml_wr_end_elem(scb, &msg, nc_id, NCX_EL_RPC, 0);
    }

    /* finish the message */
    if (anyout) {
        ses_finish_msg(scb);
    }

    if (res == NO_ERR) {
        req->replycb = rpyfn;
        add_request(scb, req);
    }

    xml_msg_clean_hdr(&msg);
    return res;

}  /* mgr_rpc_send_request */


/********************************************************************
* FUNCTION mgr_rpc_dispatch
*
* Dispatch an incoming <rpc-reply> response
* handle the <rpc-reply> element
* called by mgr_top.c: 
* This function is registered with top_register_node
* for the module 'netconf', top-node 'rpc-reply'
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
void 
    mgr_rpc_dispatch (ses_cb_t *scb,
                      xml_node_t *top)
{
    obj_template_t          *rpyobj;
    mgr_rpc_rpy_t           *rpy;
    mgr_rpc_req_t           *req;
    xml_attr_t              *attr;
    xmlChar                 *msg_id;
    ncx_module_t            *mod;
    mgr_rpc_cbfn_t           handler;
    ncx_num_t                num;
    status_t                 res;

#ifdef DEBUG
    if (!scb || !top) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* init local vars */
    res = NO_ERR;
    msg_id = NULL;
    req = NULL;

    /* make sure any real session has been properly established */
    if (scb->type != SES_TYP_DUMMY && scb->state != SES_ST_IDLE) {
        log_error("\nError: mgr_rpc: skipping incoming message '%s'",
                  top->qname);
        mgr_xml_skip_subtree(scb->reader, top);
        return;
    }

    /* check if the reply template is already cached */
    rpyobj = NULL;
    mod = ncx_find_module(NC_MODULE, NULL);
    if (mod != NULL) {
        rpyobj = ncx_find_object(mod, NC_RPC_REPLY_TYPE);
    }
    if (rpyobj == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        mgr_xml_skip_subtree(scb->reader, top);
        return;
    }

    /* get the NC RPC message-id attribute; should be present
     * because the send-rpc function put a message-id in <rpc>
     */
    attr = xml_find_attr(top, 0, NCX_EL_MESSAGE_ID);
    if (attr && attr->attr_val) {
        msg_id = xml_strdup(attr->attr_val);
    }
    if (msg_id == NULL) {
        mgr_xml_skip_subtree(scb->reader, top);
        log_info("\nmgr_rpc: incoming message with no message-id");
        return;
    }       

    /* the current node is 'rpc-reply' in the netconf namespace
     * First get a new RPC reply struct
     */
    rpy = new_reply();
    if (rpy == NULL) {
        m__free(msg_id);
        log_error("\nError: mgr_rpc: skipping incoming message");
        mgr_xml_skip_subtree(scb->reader, top);
        return;
    } else {
        rpy->msg_id = msg_id;
    }
    
    /* get the NCX RPC group-id attribute if present */
    attr = xml_find_attr(top, xmlns_ncx_id(), NCX_EL_GROUP_ID);
    if (attr && attr->attr_val) {
        res = ncx_decode_num(attr->attr_val, NCX_BT_UINT32, &num);
        if (res == NO_ERR) {
            rpy->group_id = num.u;
        }
    }

    /* find the request that goes with this reply */
    if (rpy->msg_id != NULL) {
        req = find_request(scb, rpy->msg_id);
        if (req == NULL) {
#ifdef MGR_RPC_DEBUG
            log_debug("\nmgr_rpc: got request found for msg (%s) "
                      "on session %d", 
                      rpy->msg_id, 
                      scb->sid);
#endif
            mgr_xml_skip_subtree(scb->reader, top);
            mgr_rpc_free_reply(rpy);
            return;
        } else {
            dlq_remove(req);
        }
    }

    /* have a request/reply pair, so parse the reply 
     * as a val_value_t tree, stored in rpy->reply
     */
    rpy->res = mgr_val_parse_reply(scb, 
                                   rpyobj, 
                                   (req != NULL) ?
                                   req->rpc : ncx_get_gen_anyxml(),
                                   top, 
                                   rpy->reply);
    if (rpy->res != NO_ERR && LOGINFO) {
        log_info("\nmgr_rpc: got invalid reply on session %d (%s)",
                 scb->sid, get_error_string(rpy->res));
    }

    /* check that there is nothing after the <rpc-reply> element */
    if (rpy->res==NO_ERR && 
        !xml_docdone(scb->reader) && LOGINFO) {
        log_info("\nmgr_rpc: got extra nodes in reply on session %d",
                 scb->sid);
    }

    /* invoke the reply handler */
    if (req != NULL) { 
        handler = (mgr_rpc_cbfn_t)req->replycb;
        (*handler)(scb, req, rpy);
    }

    /* only reset the session state to idle if was not changed
     * to SES_ST_SHUTDOWN_REQ during this RPC call
     */
    if (scb->state == SES_ST_IN_MSG) {
        scb->state = SES_ST_IDLE;
    }

#ifdef MGR_RPC_DEBUG
    print_errors();
    clear_errors();
#endif

} /* mgr_rpc_dispatch */


/* END file mgr_rpc.c */
