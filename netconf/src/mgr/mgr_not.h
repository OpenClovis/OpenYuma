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
#ifndef _H_mgr_not
#define _H_mgr_not
/*  FILE: mgr_not.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol notification manager-side definitions

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
03-jun-09    abb      Begun;
*/
#include <time.h>

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

#ifndef _H_tstamp
#include "tstamp.h"
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


/********************************************************************
*                                                                   *
*                         T Y P E S                                 *
*                                                                   *
*********************************************************************/


/* struct to save and process an incoming notification */
typedef struct mgr_not_msg_t_ {
    dlq_hdr_t               qhdr;
    val_value_t            *notification;  /* parsed message */
    val_value_t            *eventTime;   /* ptr into notification */
    val_value_t            *eventType;   /* ptr into notification */
    status_t                res;         /* parse result */
} mgr_not_msg_t;


/* manager notification callback function
 *
 *  INPUTS:
 *   scb == session control block for session that got the reply
 *   msg == incoming notification msg
 *   consumed == address of return message consumed flag
 *
 *   OUTPUTS:
 *     *consumed == TRUE if msg has been consumed so
 *                  it will not be freed by mgr_not_dispatch
 *               == FALSE if msg has been not consumed so
 *                  it will be freed by mgr_not_dispatch
 */
typedef void (*mgr_not_cbfn_t) (ses_cb_t *scb,
				mgr_not_msg_t *msg,
                                boolean *consumed);


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


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
extern status_t 
    mgr_not_init (void);


/********************************************************************
* FUNCTION mgr_not_cleanup
*
* Cleanup the mgr_not module.
* call once to cleanup module
* Unregister the top-level NETCONF <rpc> element
*
*********************************************************************/
extern void 
    mgr_not_cleanup (void);


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
extern void
    mgr_not_free_msg (mgr_not_msg_t *msg);


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
extern void
    mgr_not_clean_msgQ (dlq_hdr_t *msgQ);


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
extern void
    mgr_not_dispatch (ses_cb_t  *scb,
		      xml_node_t *top);


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
extern void
    mgr_not_set_callback_fn (mgr_not_cbfn_t cbfn);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_mgr_not */
