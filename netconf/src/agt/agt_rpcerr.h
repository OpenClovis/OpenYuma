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
#ifndef _H_agt_rpcerr
#define _H_agt_rpcerr
/*  FILE: agt_rpcerr.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol <rpc-error> server-side handler

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
07-mar-06    abb      Begun.
13-jan-07    abb      moved from rpc dir to agt; rename rpc_agterr
                      to agt_rpcerr
*/

#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION agt_rpc_gen_error
*
* Generate an internal <rpc-error> record for an element
* (or non-attribute) related error for any layer. 
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_error (ncx_layer_t layer,
			  status_t   interr,
			  const xml_node_t *errnode,
			  ncx_node_t parmtyp,
			  const void *error_parm,
			  xmlChar *error_path);


/********************************************************************
* FUNCTION agt_rpc_gen_error_errinfo
*
* Generate an internal <rpc-error> record for an element
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*  errinfo == error info struct to use for whatever fields are set
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_error_errinfo (ncx_layer_t layer,
				  status_t   interr,
				  const xml_node_t *errnode,
				  ncx_node_t  parmtyp,
				  const void *error_parm,
				  xmlChar *error_path,
				  const ncx_errinfo_t *errinfo);


/********************************************************************
* FUNCTION agt_rpc_gen_error_ex
*
* Generate an internal <rpc-error> record for an element
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*  errinfo == error info struct to use for whatever fields are set
*  nodetyp == type of node contained in error_path_raw
*  error_path_raw == pointer to the extra parameter expected for
*                this type of error.  
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_error_ex (ncx_layer_t layer,
                             status_t   interr,
                             const xml_node_t *errnode,
                             ncx_node_t  parmtyp,
                             const void *error_parm,
                             xmlChar *error_path,
                             const ncx_errinfo_t *errinfo,
                             ncx_node_t  nodetyp,
                             const void *error_path_raw);


/********************************************************************
* FUNCTION agt_rpc_gen_insert_error
*
* Generate an internal <rpc-error> record for an element
* for an insert operation failed error
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errval == pointer to the node with the insert error
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_insert_error (ncx_layer_t layer,
				 status_t   interr,
				 const val_value_t *errval,
				 xmlChar *error_path);


/********************************************************************
* FUNCTION agt_rpc_gen_unique_error
*
* Generate an internal <rpc-error> record for an element
* for a unique-stmt failed error (data-not-unique)
*
* INPUTS:
*   msghdr == message header to use for prefix storage
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errval == pointer to the node with the insert error
*   valuniqueQ == Q of val_unique_t structs to
*                   use for <non-unique> elements
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_unique_error (xml_msg_hdr_t *msghdr,
				 ncx_layer_t layer,
				 status_t   interr,
				 const dlq_hdr_t *valuniqueQ,
				 xmlChar *error_path);


/********************************************************************
* FUNCTION agt_rpc_gen_attr_error
*
* Generate an internal <rpc-error> record for an attribute
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   attr   == attribute that had the error
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   errnodeval == valuse struct for the error node id errnode NULL
*           == NULL if not used
*   badns == URI string of the namespace that is bad (or NULL)
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ (or add more error-info)
*   NULL if a record could not be allocated or not enough
*     valid info in the parameters to generate an error report
*********************************************************************/
extern rpc_err_rec_t *
    agt_rpcerr_gen_attr_error (ncx_layer_t layer,
                               status_t   interr,
                               const xml_attr_t *attr,
                               const xml_node_t *errnode,
                               const val_value_t *errnodeval,
                               const xmlChar *badns,
                               xmlChar *error_path);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_agt_rpcerr */
