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
#ifndef _H_agt_rpc
#define _H_agt_rpc
/*  FILE: agt_rpc.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol remote procedure call server-side definitions

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
30-apr-05    abb      Begun.
*/

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
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

/* this constant is for the number of callback slots
 * allocated in a 'cbset', and only includes the
 * RPC phases that allow callback functions
 */
#define AGT_RPC_NUM_PHASES   3

/********************************************************************
*                                                                   *
*                         T Y P E S                                 *
*                                                                   *
*********************************************************************/

/* There are 3 different callbacks possible in the
 * server processing chain. 
 *
 * Only AGT_RPC_PH_INVOKE is needed to do any work
 * Validate is needed if parameter checking beyond the
 * YANG constraints, such as checking if a needed
 * lock is available
 *
 * The engine will check for optional callbacks during 
 * RPC processing.
 *
 */
typedef enum agt_rpc_phase_t_ {
    AGT_RPC_PH_VALIDATE,         /* (2) cb after the input is parsed */
    AGT_RPC_PH_INVOKE,      /* (3) cb to invoke the requested method */
    AGT_RPC_PH_POST_REPLY,    /* (5) cb after the reply is generated */ 
    AGT_RPC_PH_PARSE,                    /* (1) NO CB FOR THIS STATE */ 
    AGT_RPC_PH_REPLY                     /* (4) NO CB FOR THIS STATE */ 
} agt_rpc_phase_t;


/* Template for RPC server callbacks
 * The same template is used for all RPC callback phases
 */
typedef status_t 
    (*agt_rpc_method_t) (ses_cb_t *scb,
			 rpc_msg_t *msg,
			 xml_node_t *methnode);


typedef struct agt_rpc_cbset_t_ {
    agt_rpc_method_t  acb[AGT_RPC_NUM_PHASES];
} agt_rpc_cbset_t;



/* Callback template for RPCs that use an inline callback
 * function instead of generating a malloced val_value_t tree
 *
 * INPUTS:
 *   scb == session control block
 *   msg == RPC request in progress
 *   indent == start indent amount; ignored if the server
 *             is configured not to use PDU indentation
 * RETURNS:
 *   status of the output operation
 */
typedef status_t 
    (*agt_rpc_data_cb_t) (ses_cb_t *scb, 
			  rpc_msg_t *msg,
			  uint32 indent);

				      
				      
/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION agt_rpc_init
*
* Initialize the agt_rpc module
* Adds the agt_rpc_dispatch function as the handler
* for the NETCONF <rpc> top-level element.
* should call once to init RPC server module
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern status_t 
    agt_rpc_init (void);


/********************************************************************
* FUNCTION agt_rpc_cleanup
*
* Cleanup the agt_rpc module.
* Unregister the top-level NETCONF <rpc> element
* should call once to cleanup RPC server module
*
*********************************************************************/
extern void 
    agt_rpc_cleanup (void);


/********************************************************************
* FUNCTION agt_rpc_register_method
*
* add callback for 1 phase of RPC processing 
*
* INPUTS:
*    module == module name or RPC method
*    method_name == RPC method name
*    phase == RPC server callback phase for this callback
*    method == pointer to callback function
*
* RETURNS:
*    status of the operation
*********************************************************************/
extern status_t 
    agt_rpc_register_method (const xmlChar *module,
			     const xmlChar *method_name,
			     agt_rpc_phase_t  phase,
			     agt_rpc_method_t method);


/********************************************************************
* FUNCTION agt_rpc_support_method
*
* mark an RPC method as supported within the server
* this is needed for operations dependent on capabilities
*
* INPUTS:
*    module == module name of RPC method (really module name)
*    method_name == RPC method name
*********************************************************************/
extern void 
    agt_rpc_support_method (const xmlChar *module,
                            const xmlChar *method_name);


/********************************************************************
* FUNCTION agt_rpc_unsupport_method
*
* mark an RPC method as unsupported within the server
* this is needed for operations dependent on capabilities
*
* INPUTS:
*    module == module name of RPC method (really module name)
*    method_name == RPC method name
*********************************************************************/
extern void
    agt_rpc_unsupport_method (const xmlChar *module,
			      const xmlChar *method_name);


/********************************************************************
* FUNCTION agt_rpc_unregister_method
*
* remove the callback functions for all phases of RPC processing 
* for the specified RPC method
*
* INPUTS:
*    module == module name of RPC method (really module name)
*    method_name == RPC method name
*********************************************************************/
extern void
    agt_rpc_unregister_method (const xmlChar *module,
			       const xmlChar *method_name);


/********************************************************************
* FUNCTION agt_rpc_dispatch
*
* Dispatch an incoming <rpc> request
* called by top.c: 
* This function is registered with top_register_node
* for the module 'yuma-netconf', top-node 'rpc'
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
extern void
    agt_rpc_dispatch (ses_cb_t  *scb,
		      xml_node_t *top);

/********************************************************************
* FUNCTION agt_rpc_load_config_file
*
* Dispatch an internal <load-config> request
* used for OP_EDITOP_LOAD to load the running from startup
* and OP_EDITOP_REPLACE to restore running from backup
*
*    - Create a dummy session and RPC message
*    - Call a special agt_ps_parse function to parse the config file
*    - Call the special agt_ncx function to invoke the proper 
*      parmset and application 'validate' callback functions, and 
*      record all the error/warning messages
*    - Call the special ncx_agt function to invoke all the 'apply'
*      callbacks as needed
*    - transfer any error messages to the cfg->load_errQ
*
* INPUTS:
*   filespec == XML config filespec to load
*   cfg == cfg_template_t to fill in
*   isload == TRUE for normal load-config
*             FALSE for restore backup load-config
*   use_sid == session ID to use for the access control
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    agt_rpc_load_config_file (const xmlChar *filespec,
			      cfg_template_t  *cfg,
                              boolean isload,
                              ses_id_t  use_sid);


/********************************************************************
* FUNCTION agt_rpc_get_config_file
*
* Dispatch an internal <load-config> request
* except skip the INVOKE phase and just remove
* the 'config' node from the input and return it
*
*    - Create a dummy session and RPC message
*    - Call a special agt_ps_parse function to parse the config file
*    - Call the special agt_ncx function to invoke the proper 
*      parmset and application 'validate' callback functions, and 
*      record all the error/warning messages
*    - return the <config> element if no errors
*    - otherwise return all the error messages in a Q
*
* INPUTS:
*   filespec == XML config filespec to get
*   targetcfg == target database to validate against
*   use_sid == session ID to use for the access control
*   errorQ == address of return queue of rpc_err_rec_t structs
*   res == address of return status
*
* OUTPUTS:
*   if any errors, the error structs are transferred to
*   the errorQ (if it is non-NULL).  In this case, the caller
*   must free these data structures with ncx/rpc_err_clean_errQ
* 
*   *res == return status
*
* RETURNS:
*   malloced and filled in struct representing a <config> element
*   NULL if some error, check errorQ and *res
*********************************************************************/
extern val_value_t *
    agt_rpc_get_config_file (const xmlChar *filespec,
                             cfg_template_t *targetcfg,
                             ses_id_t  use_sid,
                             dlq_hdr_t *errorQ,
                             status_t *res);


/********************************************************************
* FUNCTION agt_rpc_fill_rpc_error
*
* Fill one <rpc-error> like element using the specified
* namespace and name, which may be different than NETCONF
*
* INPUTS:
*   err == error record to use to fill 'rpcerror'
*   rpcerror == NCX_BT_CONTAINER value struct already
*               initialized.  The val_add_child function
*               will use this parm as the parent.
*               This namespace will be used for all
*               child nodes
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_rpc_fill_rpc_error (const rpc_err_rec_t *err,
			    val_value_t *rpcerror);


/********************************************************************
* FUNCTION agt_rpc_send_error_reply
*
* Operation failed or was never attempted
* Return an <rpc-reply> with an <rpc-error>
* 
* INPUTS:
*   scb == session control block
*   retres == error number for termination  reason
*
*********************************************************************/
extern void
    agt_rpc_send_error_reply (ses_cb_t *scb,
                              status_t retres);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_agt_rpc */
