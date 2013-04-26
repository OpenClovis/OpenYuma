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
#ifndef _H_mgr_rpc
#define _H_mgr_rpc
/*  FILE: mgr_rpc.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol remote procedure call manager-side definitions

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
13-feb-07    abb      Begun; started from agt_rpc.h
*/
#include <sys/time.h>

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define MGR_RPC_NUM_PHASES   6

/********************************************************************
*                                                                   *
*                         T Y P E S                                 *
*                                                                   *
*********************************************************************/


/* NETCONF client <rpc> message header */
typedef struct mgr_rpc_req_t_ {
    dlq_hdr_t      qhdr;
    xml_msg_hdr_t  mhdr;
    uint32         group_id;
    obj_template_t *rpc;
    xmlChar       *msg_id;       /* malloced message ID */
    xml_attrs_t    attrs;          /* Extra <rpc> attrs */
    val_value_t   *data;      /* starts with the method */
    time_t         starttime;     /* tstamp for timeout */
    struct timeval perfstarttime; /* tstamp to perf meas */
    uint32         timeout;       /* timeout in seconds */
    void          *replycb;           /* mgr_rpc_cbfn_t */

} mgr_rpc_req_t;

/* NETCONF client <rpc-reply> message header */
typedef struct mgr_rpc_rpy_t_ {
    dlq_hdr_t      qhdr;
    /* xml_msg_hdr_t  mhdr; */
    uint32         group_id;
    xmlChar       *msg_id;
    status_t       res;
    val_value_t   *reply;
} mgr_rpc_rpy_t;


/* manager RPC reply callback function
 *
 *  INPUTS:
 *   scb == session control block for session that got the reply
 *   req == RPC request returned to the caller (for reuse or free)
 *   rpy == RPY reply header; this will be NULL if the timeout
 *          occurred so there is no reply
 */
typedef void (*mgr_rpc_cbfn_t) (ses_cb_t *scb,
				mgr_rpc_req_t *req,
				mgr_rpc_rpy_t *rpy);


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


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
extern status_t 
    mgr_rpc_init (void);


/********************************************************************
* FUNCTION mgr_rpc_cleanup
*
* Cleanup the mgr_rpc module.
* call once to cleanup RPC mgr module
* Unregister the top-level NETCONF <rpc> element
*
*********************************************************************/
extern void 
    mgr_rpc_cleanup (void);


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
extern mgr_rpc_req_t *
    mgr_rpc_new_request (ses_cb_t *scb);


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
extern void
    mgr_rpc_free_request (mgr_rpc_req_t *req);


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
extern void
    mgr_rpc_free_reply (mgr_rpc_rpy_t *rpy);


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
extern void
    mgr_rpc_clean_requestQ (dlq_hdr_t *reqQ);


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
extern uint32
    mgr_rpc_timeout_requestQ (dlq_hdr_t *reqQ);


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
extern status_t
    mgr_rpc_send_request (ses_cb_t *scb,
			  mgr_rpc_req_t *req,
			  mgr_rpc_cbfn_t rpyfn);


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
extern void
    mgr_rpc_dispatch (ses_cb_t  *scb,
		      xml_node_t *top);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_mgr_rpc */
