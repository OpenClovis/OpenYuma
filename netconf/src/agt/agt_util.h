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
#ifndef _H_agt_util
#define _H_agt_util

/*  FILE: agt_util.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Utility Functions for NCX Server method routines

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-may-06    abb      Begun

*/

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_getcb
#include "getcb.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
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

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_get_cfg_from_parm
*
* Get the cfg_template_t for the config in the target param
*
* INPUTS:
*    parmname == parameter to get from (e.g., target)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    retcfg == address of return cfg pointer
* 
* OUTPUTS:
*   *retcfg is set to the address of the cfg_template_t struct 
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_get_cfg_from_parm (const xmlChar *parmname,
			   rpc_msg_t *msg,
			   xml_node_t *methnode,
			   cfg_template_t  **retcfg);


/********************************************************************
* FUNCTION agt_get_inline_cfg_from_parm
*
* Get the val_value_t node for the inline config element
*
* INPUTS:
*    parmname == parameter to get from (e.g., source)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    retval == address of return value node pointer
* 
* OUTPUTS:
*   *retval is set to the address of the val_value_t struct 
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_get_inline_cfg_from_parm (const xmlChar *parmname,
                                  rpc_msg_t *msg,
                                  xml_node_t *methnode,
                                  val_value_t  **retval);


/********************************************************************
* FUNCTION agt_get_url_from_parm
*
* Get the URL string for the config in the target param
*
*  1) an edit-content choice for edit-config
*  2) a source or target config-choice option for copy-config
*  3) a target of delete-config
*  4) config-source choice option for validate
*
* INPUTS:
*    parmname == parameter to get from (e.g., target)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    returl == address of return URL string pointer
*    retval == address of value node from input used
* OUTPUTS:
*   *returl is set to the address of the URL string
*   pointing to the memory inside the found parameter
*   *retval is set to parm node found, if return NO_ERR
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_get_url_from_parm (const xmlChar *parmname,
                           rpc_msg_t *msg,
                           xml_node_t *methnode,
                           const xmlChar **returl,
                           val_value_t **retval);


/********************************************************************
* FUNCTION agt_get_filespec_from_url
*
* Check the URL and get the filespec part out of it
*
* INPUTS:
*    urlstr == URL to check
*    res == address of return status
* 
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*    malloced URL string; must be freed by caller!!
*    NULL if some error
*********************************************************************/
extern xmlChar *
    agt_get_filespec_from_url (const xmlChar *urlstr,
                               status_t *res);


/********************************************************************
* FUNCTION agt_get_parmval
*
* Get the identified val_value_t for a given parameter
* Used for error purposes!
* INPUTS:
*    parmname == parameter to get
*    msg == incoming rpc_msg_t in progress
*
* RETURNS:
*    status
*********************************************************************/
extern const val_value_t *
    agt_get_parmval (const xmlChar *parmname,
		     rpc_msg_t *msg);


/********************************************************************
* FUNCTION agt_record_error
*
* Generate an rpc_err_rec_t and save it in the msg
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured               <error-type>
*    res == internal error code                      <error-app-tag>
*    xmlnode == XML node causing error  <bad-element>   <error-path> 
*            == NULL if not available 
*    parmtyp == type of node in 'error_info'
*    error_info == error data, specific to 'res'        <error-info>
*               == NULL if not available (then nodetyp ignored)
*    nodetyp == type of node in 'error_path'
*    error_path == internal data node with the error       <error-path>
*            == NULL if not available or not used  
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL
*
* RETURNS:
*    none
*********************************************************************/
extern void
    agt_record_error (ses_cb_t *scb,
		      xml_msg_hdr_t *msghdr,
		      ncx_layer_t layer,
		      status_t  res,
		      const xml_node_t *xmlnode,
		      ncx_node_t parmtyp,
		      const void *error_info,
		      ncx_node_t nodetyp,
		      void *error_path);


/********************************************************************
* FUNCTION agt_record_error_errinfo
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured               <error-type>
*    res == internal error code                      <error-app-tag>
*    xmlnode == XML node causing error  <bad-element>   <error-path> 
*            == NULL if not available 
*    parmtyp == type of node in 'error_info'
*    error_info == error data, specific to 'res'        <error-info>
*               == NULL if not available (then nodetyp ignored)
*    nodetyp == type of node in 'error_path'
*    error_path == internal data node with the error       <error-path>
*            == NULL if not available or not used  
*    errinfo == error info record to use
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
extern void
    agt_record_error_errinfo (ses_cb_t *scb,
			      xml_msg_hdr_t *msghdr,
			      ncx_layer_t layer,
			      status_t  res,
			      const xml_node_t *xmlnode,
			      ncx_node_t parmtyp,
			      const void *error_parm,
			      ncx_node_t nodetyp,
			      void *error_path,
			      const ncx_errinfo_t *errinfo);


/********************************************************************
* FUNCTION agt_record_attr_error
*
* Generate an rpc_err_rec_t and save it in the msg
*
* INPUTS:
*    scb == session control block
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured
*    res == internal error code
*    xmlattr == XML attribute node causing error
*               (NULL if not available)
*    xmlnode == XML node containing the attr
*    badns == bad namespace string value
*    nodetyp == type of node in 'errnode'
*    errnode == internal data node with the error
*            == NULL if not used
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*
* RETURNS:
*    none
*********************************************************************/
extern void
    agt_record_attr_error (ses_cb_t *scb,
			   xml_msg_hdr_t *msghdr,
			   ncx_layer_t layer,
			   status_t  res,
			   const xml_attr_t *xmlattr,
			   const xml_node_t *xmlnode,
			   const xmlChar *badns,
			   ncx_node_t nodetyp,
			   const void *errnode);


/********************************************************************
* FUNCTION agt_record_insert_error
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* For YANG 'missing-instance' error-app-tag
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    res == internal error code                      <error-app-tag>
*    errval == value node generating the insert error
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
extern void
    agt_record_insert_error (ses_cb_t *scb,
			     xml_msg_hdr_t *msghdr,
			     status_t  res,
			     val_value_t *errval);


/********************************************************************
* FUNCTION agt_record_unique_error
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* For YANG 'data-not-unique' error-app-tag
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    errval == list value node that contains the unique-stmt
*    valuniqueQ == Q of val_unique_t structs for error-info
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
extern void
    agt_record_unique_error (ses_cb_t *scb,
			     xml_msg_hdr_t *msghdr,
			     val_value_t *errval,
			     dlq_hdr_t  *valuniqueQ);


/********************************************************************
* FUNCTION agt_validate_filter
*
* Validate the <filter> parameter if present
*
* INPUTS:
*    scb == session control block
*    msg == rpc_msg_t in progress
*
* OUTPUTS:
*    msg->rpc_filter is filled in if NO_ERR; type could be NONE
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_validate_filter (ses_cb_t *scb,
			 rpc_msg_t *msg);


/********************************************************************
* FUNCTION agt_validate_filter_ex
*
* Validate the <filter> parameter if present
*
* INPUTS:
*    scb == session control block
*    msg == rpc_msg_t in progress
*    filter == filter element to use
* OUTPUTS:
*    msg->rpc_filter is filled in if NO_ERR; type could be NONE
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_validate_filter_ex (ses_cb_t *scb,
			    rpc_msg_t *msg,
			    val_value_t *filter);


/********************************************************************
* FUNCTION agt_check_config
*
* val_nodetest_fn_t callback
*
* Used by the <get-config> operation to return any type of 
* configuration data
*
* INPUTS:
*    see ncx/val_util.h   (val_nodetest_fn_t)
*
* RETURNS:
*    TRUE if config; FALSE if non-config
*********************************************************************/
extern boolean
    agt_check_config (ncx_withdefaults_t withdef,
                      boolean realtest,
		      val_value_t *node);


/********************************************************************
* FUNCTION agt_check_default
*
* val_nodetest_fn_t callback
*
* Used by the <get*> operation to return only values
* not set to the default
*
* INPUTS:
*    see ncx/val_util.h   (val_nodetest_fn_t)
*
* RETURNS:
*    status
*********************************************************************/
extern boolean
    agt_check_default (ncx_withdefaults_t withdef,
                       boolean realtest,
		       val_value_t *node);



/********************************************************************
* FUNCTION agt_check_save
*
* val_nodetest_fn_t callback
*
* Used by agt_ncx_cfg_save function to filter just what
* is supposed to be saved in the <startup> config file
*
* INPUTS:
*    see ncx/val_util.h   (val_nodetest_fn_t)
*
* RETURNS:
*    status
*********************************************************************/
extern boolean
    agt_check_save (ncx_withdefaults_t withdef,
                    boolean realtest,
		    val_value_t *node);


/********************************************************************
* FUNCTION agt_output_filter
*
* output the proper data for the get or get-config operation
* generate the data that matched the subtree or XPath filter
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_output_filter (ses_cb_t *scb,
		       rpc_msg_t *msg,
		       int32 indent);


/********************************************************************
* FUNCTION agt_output_schema
*
* generate the YANG file contents for the get-schema operation
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_output_schema (ses_cb_t *scb,
		       rpc_msg_t *msg,
		       int32 indent);


/********************************************************************
* FUNCTION agt_output_empty
*
* output no data for the get or get-config operation
* because the if-modified-since fileter did not pass
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_output_empty (ses_cb_t *scb,
                      rpc_msg_t *msg,
                      int32 indent);


/********************************************************************
* FUNCTION agt_check_max_access
* 
* Check if the max-access for a parameter is exceeded
*
* INPUTS:
*   newval == value node from PDU to check
*   cur_exists == TRUE if the corresponding node in the target exists
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_check_max_access (val_value_t *newval,
                          boolean cur_exists);


/********************************************************************
* FUNCTION agt_check_editop
* 
* Check if the edit operation is okay
*
* INPUTS:
*   pop == parent edit operation in affect
*          Starting at the data root, the parent edit-op
*          is derived from the default-operation parameter
*   cop == address of child operation (MAY BE ADJUSTED ON EXIT!!!)
*          This is the edit-op field in the new node corresponding
*          to the curnode position in the data model
*   newnode == pointer to new node in the edit-config PDU
*   curnode == pointer to the current node in the data model
*              being affected by this operation, if any.
*           == NULL if no current node exists
*   iqual == effective instance qualifier for this value
*   proto == protocol in use
*
* OUTPUTS:
*   *cop may be adjusted to simplify further processing,
*    based on the following reduction algorithm:
*
*    create, replace, and delete operations are 'sticky'.
*    Once set, any nested edit-ops must be valid
*    within the context of the fixed operation (create or delete)
*    and the child operation gets changed to the sticky parent edit-op
*
*   if the parent is create or delete, and the child
*   is merge or replace, then the child can be ignored
*   and will be changed to the parent, since the create
*   or delete must be carried through all the way to the leaves
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_check_editop (op_editop_t   pop,
                      op_editop_t  *cop,
                      val_value_t *newnode,
                      val_value_t *curnode,
                      ncx_iqual_t iqual,
                      ncx_protocol_t proto);


/********************************************************************
* FUNCTION agt_enable_feature
* 
* Enable a YANG feature in the server
* This will not be detected by any sessions in progress!!!
* It will take affect the next time a <hello> message
* is sent by the server
*
* INPUTS:
*   modname == module name containing the feature
*   featurename == feature name to enable
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_enable_feature (const xmlChar *modname,
			const xmlChar *featurename);


/********************************************************************
* FUNCTION agt_disable_feature
* 
* Disable a YANG feature in the server
* This will not be detected by any sessions in progress!!!
* It will take affect the next time a <hello> message
* is sent by the server
*
* INPUTS:
*   modname == module name containing the feature
*   featurename == feature name to disable
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_disable_feature (const xmlChar *modname,
			 const xmlChar *featurename);
extern status_t
    agt_check_feature (const xmlChar *modname,
		       const xmlChar *featurename,
                       int* enabled/*output*/);


/********************************************************************
* FUNCTION agt_make_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafstrval == string version of value to set for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_leaf (obj_template_t *parentobj,
		   const xmlChar *leafname,
		   const xmlChar *leafstrval,
		   status_t *res);


/********************************************************************
* FUNCTION agt_make_uint_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == number value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_uint_leaf (obj_template_t *parentobj,
                        const xmlChar *leafname,
                        uint32 leafval,
                        status_t *res);

/********************************************************************
* FUNCTION agt_make_int_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == integer number value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_int_leaf (obj_template_t *parentobj,
                       const xmlChar *leafname,
                       int32 leafval,
                       status_t *res);


/********************************************************************
* FUNCTION agt_make_uint64_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == number value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_uint64_leaf (obj_template_t *parentobj,
                          const xmlChar *leafname,
                          uint64 leafval,
                          status_t *res);


/********************************************************************
* FUNCTION agt_make_int64_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == integer number value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_int64_leaf (obj_template_t *parentobj,
                         const xmlChar *leafname,
                         int64 leafval,
                         status_t *res);


/********************************************************************
* FUNCTION agt_make_idref_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == identityref value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_idref_leaf (obj_template_t *parentobj,
                         const xmlChar *leafname,
                         const val_idref_t *leafval,
                         status_t *res);


/********************************************************************
* FUNCTION agt_make_list
*
* make a val_value_t struct for a specified list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   listname == name of list object to find (namespace hardwired)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct for the list or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_list (obj_template_t *parentobj,
                   const xmlChar *listname,
                   status_t *res);


/********************************************************************
* FUNCTION agt_make_object
*
* make a val_value_t struct for a specified node
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   objname == name of the object to find (namespace hardwired)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct for the list or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_object (obj_template_t *parentobj,
                     const xmlChar *objname,
                     status_t *res);


/********************************************************************
* FUNCTION agt_make_virtual_leaf
*
* make a val_value_t struct for a specified virtual 
* leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   callbackfn == get callback function to install
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
extern val_value_t *
    agt_make_virtual_leaf (obj_template_t *parentobj,
			   const xmlChar *leafname,
			   getcb_fn_t callbackfn,
			   status_t *res);


/********************************************************************
* FUNCTION agt_add_top_virtual
*
* make a val_value_t struct for a specified virtual 
* top-level data node
*
INPUTS:
*   obj == object node of the virtual data node to create
*   callbackfn == get callback function to install
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_add_top_virtual (obj_template_t *obj,
                         getcb_fn_t callbackfn);


/********************************************************************
* FUNCTION agt_add_top_container
*
* make a val_value_t struct for a specified 
* top-level container data node.  This is used
* by SIL functions to create a top-level container
* which may have virtual nodes within in it
*
* TBD: fine-control over SIL C code generation to
* allow mix of container virtual callback plus
* child node virtual node callbacks
*
INPUTS:
*   obj == object node of the container data node to create
*   val == address of return val pointer
*
* OUTPUTS:
*   *val == pointer to node created in the database
*           this is not live memory! It will be freed
*           by the database management code
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_add_top_container (obj_template_t *obj,
                           val_value_t **val);


/********************************************************************
* FUNCTION agt_add_container
*
* make a val_value_t struct for a specified 
* nested container data node.  This is used
* by SIL functions to create a nested container
* which may have virtual nodes within it
*
* TBD: fine-control over SIL C code generation to
* allow mix of container virtual callback plus
* child node virtual node callbacks
*
INPUTS:
*   modname == module name defining objname
*   objname == name of object node to create
*   parentval == parent value node to add container to
*   val == address of return val pointer
*
* OUTPUTS:
*   *val == pointer to node created in the database
*           this is not live memory! It will be freed
*           by the database management code
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_add_container (const xmlChar *modname,
                       const xmlChar *objname,
                       val_value_t *parentval,
                       val_value_t **val);



/********************************************************************
* FUNCTION agt_init_cache
*
* init a cache pointer during the init2 callback
*
* INPUTS:
*   modname == name of module defining the top-level object
*   objname == name of the top-level database object
*   res == address of return status
*
* OUTPUTS:
*   *res is set to the return status
*
* RETURNS:
*   pointer to object value node from running config,
*   or NULL if error or not found
*********************************************************************/
extern val_value_t *
    agt_init_cache (const xmlChar *modname,
                    const xmlChar *objname,
                    status_t *res);


/********************************************************************
* FUNCTION agt_check_cache
*
* check if a cache pointer needs to be changed or NULLed out
*
INPUTS:
*   cacheptr == address of pointer to cache value node
*   newval == newval from the callback function
*   curval == curval from the callback function
*   editop == editop from the callback function
*
* OUTPUTS:
*   *cacheptr may be changed, depending on the operation
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_check_cache (val_value_t **cacheptr,
                     val_value_t *newval,
                     val_value_t *curval,
                     op_editop_t editop);


/********************************************************************
* FUNCTION agt_new_xpath_pcb
*
* Get a new XPath parser control block and
* set up the server variable bindings
*
* INPUTS:
*   scb == session evaluating the XPath expression
*   expr == expression string to use (may be NULL)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced and initialied xpath_pcb_t structure
*   NULL if some error
*********************************************************************/
extern xpath_pcb_t *
    agt_new_xpath_pcb (ses_cb_t *scb,
                       const xmlChar *expr,
                       status_t *res);


/********************************************************************
* FUNCTION agt_get_startup_filespec
*
* Figure out where to store the startup file
*
* INPUTS:
*   res == address of return status
*
* OUTPUTS:
*   *res == return status

* RETURNS:
*   malloced and filled in filespec string; must be freed by caller
*   NULL if malloc error
*********************************************************************/
extern xmlChar *
    agt_get_startup_filespec (status_t *res);


/********************************************************************
* FUNCTION agt_get_target_filespec
*
* Figure out where to store the URL target file
*
* INPUTS:
*   target_url == target url spec to use; this is
*                 treated as a relative pathspec, and
*                 the appropriate data directory is used
*                 to create this file
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced and filled in filespec string; must be freed by caller
*   NULL if some error
*********************************************************************/
extern xmlChar *
    agt_get_target_filespec (const xmlChar *target_url,
                             status_t *res);


/********************************************************************
* FUNCTION agt_set_mod_defaults
*
* Check for any top-level config leafs that have a default
* value, and add them to the running configuration.
*
* INPUTS:
*   mod == module that was just added and should be used
*          to check for top-level database leafs with a default
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_set_mod_defaults (ncx_module_t *mod);


/********************************************************************
* FUNCTION agt_set_with_defaults
*
* Check if the <with-defaults> parameter is set
* in the request message, and if so, is it 
* one of the server's supported values
*
* If not, then record an error
* If so, then set the msg->mhdr.withdef enum
*
* INPUTS:
*   scb == session control block to use
*   msg == rpc message in progress
*   methnode == XML node for the method name
*
* OUTPUTS:
*   msg->mhdr.withdef set if NO_ERR
*   rpc-error recorded if any error detected
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_set_with_defaults (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode);


/********************************************************************
* FUNCTION agt_get_key_value
*
* Get the next expected key value in the ancestor chain
* Used in Yuma SIL code to invoke User SIL callbacks with key values
*
* INPUTS:
*   startval == value node to start from
*   lastkey == address of last key leaf found in ancestor chain
*              caller maintains this state-var as the anscestor
*              keys are traversed
*             *lastkey == NULL to get the first key
*
* OUTPUTS:
*   *lastkey updated with the return value if key found
*
* RETURNS:
*   value node to use (do not free!!!)
*   NULL if some bad error like no next key found
*********************************************************************/
extern val_value_t *
    agt_get_key_value (val_value_t *startval,
                       val_value_t **lastkey);


/********************************************************************
* FUNCTION agt_add_top_node_if_missing
* 
* SIL Phase 2 init:  Check the running config to see if
* the specified node is there; if not create one
* return the node either way
* INPUTS:
*   mod      == module containing object
*   objname == object name
*   added   == address of return added flag
*   res     == address of return status
*
* OUTPUTS:
*   *added == TRUE if new node was added
*   *res == return status, return NULL unless NO_ERR
*
* RETURNS:
*   if *res==NO_ERR,
*      pointer to new or existing node for modname:objname
*      this is added to running config and does not have to
*      be deleted.  
*      !!!! No MRO nodes or default nodes have been added !!!!
*********************************************************************/
extern val_value_t *
    agt_add_top_node_if_missing (ncx_module_t *mod,
                                 const xmlChar *objname,
                                 boolean *added,
                                 status_t *res);

/********************************************************************
* FUNCTION agt_any_operations_set
*
* Check the new node and all descendants
* for any operation attibutes prsent
*
* INPUTS:
*   val == value node to check
*
* RETURNS:
*   TRUE if any operation attributes found
*   FALSE if no operation attributes found
*/
extern boolean
    agt_any_operations_set (val_value_t *val);


/********************************************************************
* FUNCTION agt_apply_this_node
* 
* Check if the write operation applies to the current node
*
* INPUTS:
*    editop == edit operation value
*    newnode == pointer to new node (if any)
*    curnode == pointer to current value node (if any)
*                (just used to check if non-NULL or compare leaf)
*
* RETURNS:
*    TRUE if the current node needs the write operation applied
*    FALSE if this is a NO=OP node (either explicit or special merge)
*********************************************************************/
extern boolean
    agt_apply_this_node (op_editop_t editop,
                         const val_value_t *newnode,
                         const val_value_t *curnode);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_util */
