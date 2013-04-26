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
#ifndef _H_val_util
#define _H_val_util

/*  FILE: val_util.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Value Struct Utilities

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
19-dec-05    abb      Begun
21jul08      abb      start obj-based rewrite
29jul08      abb      split out from val.h

*/

#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_plock
#include "plock.h"
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

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/* user function callback template to test output 
 * of a specified node.
 *
 * val_nodetest_fn_t
 * 
 *  Run a user-defined test on the supplied node, and 
 *  determine if it should be output or not.
 *
 * INPUTS:
 *   withdef == with-defaults value in affect
 *   realtest == FALSE to just check object properties
 *               in the val->obj template
 *            == TRUE if OK to check the other fields
 *   node == pointer to the value struct to check
 *
 * RETURNS:
 *   TRUE if the node should be output
 *   FALSE if the node should be skipped
 */
typedef boolean 
    (*val_nodetest_fn_t) (ncx_withdefaults_t withdef,
                          boolean realtest,
			  val_value_t *node);


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
 * FUNCTION val_set_canonical_order
 * 
 * Change the child XML nodes throughout an entire subtree
 * to the canonical order defined in the object template
 *
 * >>> IT IS ASSUMED THAT ONLY VALID NODES ARE PRESENT
 * >>> AND ALL ERROR NODES HAVE BEEN PURGED ALREADY
 *
 * There is no canonical order defined for 
 * the contents of the following nodes:
 *    - anyxml leaf
 *    - ordered-by user leaf-list
 *
 * These nodes are not ordered, but their child nodes are ordered
 *    - ncx:root container
 *    - ordered-by user list
 *      
 * Leaf objects will not be processed, if val is OBJ_TYP_LEAF
 * Leaf-list objects will not be processed, if val is 
 * OBJ_TYP_LEAF_LIST.  These object types must be processed
 * within the context of the parent object.
 *
 * List child key nodes are ordered first among all
 * of the list's child nodes.
 *
 * List nodes with system keys are not kept in sorted order
 * This is not required by YANG.  Instead the user-given
 * order servers as the canonical order.  It is up to
 * the application setting the config to pick an
 * order for the list nodes.
 *
 * Also, leaf-list order is not changed, regardless of
 * the order. The default insert order is 'last'.
 *
 * INPUTS:
 *   val == value node to change to canonical order
 *
 * OUTPUTS:
 *   val->v.childQ may be reordered, for all complex types
 *   in the subtree
 *
 *********************************************************************/
extern void
    val_set_canonical_order (val_value_t *val);


/********************************************************************
 * FUNCTION val_gen_index_comp
 * 
 * Create an index component
 *
 * INPUTS:
 *   in == obj_key_t in the chain to process
 *   val == the just parsed table row with the childQ containing
 *          nodes to check as index nodes
 *
 * OUTPUTS:
 *   val->indexQ will get a val_index_t record added if return NO_ERR
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    val_gen_index_comp  (const obj_key_t *in,
			 val_value_t *val);


/********************************************************************
 * FUNCTION val_gen_key_entry
 * 
 * Create a key record within an index comp
 *
 * INPUTS:
 *   in == obj_key_t in the chain to process
 *   keyval == the just parsed table row with the childQ containing
 *          nodes to check as index nodes
 *
 * OUTPUTS:
 *   val->indexQ will get a val_index_t record added if return NO_ERR
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    val_gen_key_entry  (val_value_t *keyval);


/********************************************************************
 * FUNCTION val_gen_index_chain
 * 
 * Create an index chain for the just-parsed table or container struct
 *
 * INPUTS:
 *   obj == list object containing the keyQ
 *   val == the just parsed table row with the childQ containing
 *          nodes to check as index nodes
 *
 * OUTPUTS:
 *   *val->indexQ has entries added for each index component, if NO_ERR
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    val_gen_index_chain (const obj_template_t *obj,
			 val_value_t *val);


/********************************************************************
 * FUNCTION val_add_defaults
 * 
 * add defaults to an initialized complex value
 * Go through the specified value struct and add in any defaults
 * for missing leaf and choice nodes, that have defaults.
 *
 * !!! Only the child nodes will be checked for missing defaults
 * !!! The top-level value passed to this function is assumed to
 * !!! be already set
 *
 * This function does not handle top-level choice object subtrees.
 * This special case must be handled with the datadefQ
 * for the module.  If a top-level leaf value is passed in,
 * which is from a top-level choice case-arm, then the
 * rest of the case-arm objects will not get added by
 * this function.
 *
 * It is assumed that even top-level data-def-stmts will
 * be handled within a <config> container, so the top-level
 * object should always a container.
 *
 * INPUTS:
 *   val == the value struct to modify
 *   rootval == the root value for XPath purposes
 *           == NULL to skip when-stmt check
 *   cxtval == the context value for XPath purposes
 *           == NULL to use val instead
 *   scriptmode == TRUE if the value is a script object access
 *              == FALSE for normal val_get_simval access instead
 *
 * OUTPUTS:
 *   *val and any sub-nodes are set to the default value as requested
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    val_add_defaults (val_value_t *val,
                      val_value_t *valroot,
                      val_value_t *cxtval,
		      boolean scriptmode);


/********************************************************************
* FUNCTION val_instance_check
* 
* Check for the proper number of object instances for
* the specified value struct. Checks the direct accessible
* children of 'val' only!!!
* 
* The 'obj' parameter is usually the val->obj field
* except for choice/case processing
*
* Log errors as needed and mark val->res as needed
*
* INPUTS:
*   val == value to check
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_instance_check (val_value_t  *val,
			obj_template_t *obj);


/********************************************************************
* FUNCTION val_get_choice_first_set
* 
* Check a val_value_t struct against its expected OBJ
* to determine if a specific choice has already been set
* Get the value struct for the first value set for
* the specified choice
*
* INPUTS:
*   val == val_value_t to check
*   obj == choice object to check
*
* RETURNS:
*   pointer to first value struct or NULL if choice not set
*********************************************************************/
extern val_value_t *
    val_get_choice_first_set (val_value_t *val,
			      const obj_template_t *obj);


/********************************************************************
* FUNCTION val_get_choice_next_set
* 
* Check a val_value_t struct against its expected OBJ
* to determine if a specific choice has already been set
* Get the value struct for the next value set from the
* specified choice, afvter 'curval'
*
* INPUTS:
*   obj == choice object to check
*   curchild == current child selected from this choice (obj)
*
* RETURNS:
*   pointer to first value struct or NULL if choice not set
*********************************************************************/
extern val_value_t *
    val_get_choice_next_set (const obj_template_t *obj,
			     val_value_t *curchild);


/********************************************************************
* FUNCTION val_choice_is_set
* 
* Check a val_value_t struct against its expected OBJ
* to determine if a specific choice has already been set
* Check that all the mandatory config fields in the selected
* case are set
*
* INPUTS:
*   val == parent of the choice object to check
*   obj == choice object to check
*
* RETURNS:
*   pointer to first value struct or NULL if choice not set
*********************************************************************/
extern boolean
    val_choice_is_set (val_value_t *val,
		       obj_template_t *obj);


/********************************************************************
* FUNCTION val_purge_errors_from_root
* 
* Remove any error nodes under a root container
* that were saved for error recording purposes
*
* INPUTS:
*   val == root container to purge
*
*********************************************************************/
extern void
    val_purge_errors_from_root (val_value_t *val);


/********************************************************************
 * FUNCTION val_new_child_val
 * 
 * INPUTS:
 *   nsid == namespace ID of name
 *   name == name string (direct or strdup, based on copyname)
 *   copyname == TRUE is dname strdup should be used
 *   parent == parent node
 *   editop == requested edit operation
 *   obj == object template to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern val_value_t *
    val_new_child_val (xmlns_id_t   nsid,
                       const xmlChar *name,
                       boolean copyname,
                       val_value_t *parent,
                       op_editop_t editop,
                       obj_template_t *obj);


/********************************************************************
* FUNCTION val_gen_instance_id
* 
* Malloc and Generate the instance ID string for this value node, 
* 
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   val == node to generate the instance ID for
*   format == desired output format (NCX or Xpath)
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   mhdr.pmap may have entries added if prefixes used
*      in the instance identifier which are not already in the pmap
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_gen_instance_id (xml_msg_hdr_t *mhdr,
			 const val_value_t  *val, 
			 ncx_instfmt_t format,
			 xmlChar  **buff);


/********************************************************************
* FUNCTION val_gen_instance_id_ex
* 
* Malloc and Generate the instance ID string for this value node, 
* 
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   val == node to generate the instance ID for
*   format == desired output format (NCX or Xpath)
*   stop_at_root == TRUE to stop if a 'root' node is encountered
*                == FALSE to keep recursing all the way to 
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   mhdr.pmap may have entries added if prefixes used
*      in the instance identifier which are not already in the pmap
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_gen_instance_id_ex (xml_msg_hdr_t *mhdr,
                            const val_value_t  *val, 
                            ncx_instfmt_t format,
                            boolean stop_at_root,
                            xmlChar  **buff);


/********************************************************************
* FUNCTION val_gen_split_instance_id
* 
* Malloc and Generate the instance ID string for this value node, 
* Add the last node from the parameters, not the value node
*
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   val == node to generate the instance ID for
*   format == desired output format (NCX or Xpath)
*   leaf_pfix == namespace prefix string of the leaf to add
*   leaf_name ==  name string of the leaf to add
*   stop_at_root == TRUE to stop if a 'root' node is encountered
*                == FALSE to keep recursing all the way to 
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   mhdr.pmap may have entries added if prefixes used
*      in the instance identifier which are not already in the pmap
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_gen_split_instance_id (xml_msg_hdr_t *mhdr,
			       const val_value_t  *val, 
			       ncx_instfmt_t format,
			       xmlns_id_t leaf_nsid,
			       const xmlChar *leaf_name,
                               boolean stop_at_root,
			       xmlChar  **buff);


/********************************************************************
* FUNCTION val_get_index_string
* 
* Get the index string for the specified table or container entry
* 
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   format == desired output format
*   val == val_value_t for table or container
*   buff == buffer to hold result; 
         == NULL means get length only
*   
* OUTPUTS:
*   mhdr.pmap may have entries added if prefixes used
*      in the instance identifier which are not already in the pmap
*   *len = number of bytes that were (or would have been) written 
*          to buff
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_get_index_string (xml_msg_hdr_t *mhdr,
			  ncx_instfmt_t format,
			  const val_value_t *val,
			  xmlChar *buff,
			  uint32  *len);


/********************************************************************
* FUNCTION val_check_obj_when
* 
* checks when-stmt only
* Check if the specified object node is
* conditionally TRUE or FALSE, based on any
* when statements attached to the child node
*
* INPUTS:
*   val == parent value node of the object node to check
*   valroot == database root for XPath purposes
*   objval == database value node to check (may be NULL)
*   obj == object template of data node object to check
*   condresult == address of conditional test result
*   whencount == address of number of when-stmts tested
*                 (may be NULL if caller does not care)
*
* OUTPUTS:
*   *condresult == TRUE if conditional is true or there are none
*                  FALSE if conditional test failed
*   if non-NULL:
*     *whencount == number of when-stmts tested
*                   this can be 0 if *condresult == TRUE
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_check_obj_when (val_value_t *val,
                        val_value_t *valroot,
                        val_value_t *objval,
                        obj_template_t *obj,
                        boolean *condresult,
                        uint32 *whencount);


/********************************************************************
* FUNCTION val_get_xpathpcb
* 
* Get the XPath parser control block in the specified value struct
* 
* INPUTS:
*   val == value struct to check
*
* RETURNS:
*    pointer to xpath control block or NULL if none
*********************************************************************/
extern xpath_pcb_t *
    val_get_xpathpcb (val_value_t *val);


/********************************************************************
* FUNCTION val_get_const_xpathpcb
* 
* Get the XPath parser control block in the specified value struct
* 
* INPUTS:
*   val == value struct to check
*
* RETURNS:
*    pointer to xpath control block or NULL if none
*********************************************************************/
extern const xpath_pcb_t *
    val_get_const_xpathpcb (const val_value_t *val);


/********************************************************************
* FUNCTION val_make_simval_obj
* 
* Create and set a val_value_t as a simple type
* from an object template instead of individual fields
* Calls val_make_simval with the object settings
*
* INPUTS:
*    obj == object template to use
*    valstr == simple value encoded as a string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced and filled in val_value_t struct
*    NULL if some error
*********************************************************************/
extern val_value_t *
    val_make_simval_obj (obj_template_t *obj,
			 const xmlChar *valstr,
			 status_t  *res);


/********************************************************************
* FUNCTION val_set_simval_obj
* 
* Set an initialized val_value_t as a simple type

* Set a pre-initialized val_value_t as a simple type
* from an object template instead of individual fields
* Calls val_set_simval with the object settings
*
* INPUTS:
*    val == value struct to set
*    obj == object template to use
*    valstr == simple value encoded as a string to set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    val_set_simval_obj (val_value_t  *val,
			obj_template_t *obj,
			const xmlChar *valstr);


/********************************************************************
* FUNCTION val_set_warning_parms
* 
* Check the parent value struct (expected to be a container or list)
* for the common warning control parameters.
* invoke the warning parms that are present
*
*   --warn-idlen
*   --warn-linelen
*   --warn-off
*
* INPUTS:
*    parentval == parent value struct to check
*
* OUTPUTS:
*  prints an error message if a warn-off record cannot be added
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    val_set_warning_parms (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_logging_parms
* 
* Check the parent value struct (expected to be a container or list)
* for the common warning control parameters.
* invoke the warning parms that are present
*
*   --log=filename
*   --log-level=<debug-enum>
*   --log-append=<boolean>
*
* INPUTS:
*    parentval == parent value struct to check
*
* OUTPUTS:
*  prints an error message if any errors occur
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    val_set_logging_parms (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_path_parms
*   --datapath
*   --modpath
*   --runpath
*
* Check the specified value set for the 3 path CLI parms
* and override the environment variable setting, if any.
*
* Not all of these parameters are supported in all programs
* The object tree is not checked, just the value tree
*
* INPUTS:
*   parentval == CLI container to check for the runpath,
*                 modpath, and datapath variables
* RETURNS:
*  status
*********************************************************************/
extern status_t
    val_set_path_parms (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_subdirs_parm
*   --subdirs=<boolean>
*
* Handle the --subdirs parameter
*
* INPUTS:
*   parentval == CLI container to check for the subdirs parm
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    val_set_subdirs_parm (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_feature_parms
*   --feature-code-default
*   --feature-enable-default
*   --feature-static
*   --feature-dynamic
*   --feature-enable
*   --feature-disable
*
* Handle the feature-related CLI parms for the specified value set
*
* Not all of these parameters are supported in all programs
* The object tree is not checked, just the value tree
*
* INPUTS:
*   parentval == CLI container to check for the feature parms
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    val_set_feature_parms (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_protocols_parm
*
*   --protocols=bits [netconf1.0, netconf1.1]
*
* Handle the protocols parameter
*
* INPUTS:
*   parentval == CLI container to check for the protocols parm
*
* RETURNS:
*    status:  at least 1 protocol must be selected
*********************************************************************/
extern status_t
    val_set_protocols_parm (val_value_t *parentval);


/********************************************************************
* FUNCTION val_set_ses_protocols_parm
*
*   --protocols=bits [netconf1.0, netconf1.1]
*
* Handle the protocols parameter
*
* INPUTS:
*   scb == session control block to use
*   parentval == CLI container to check for the protocols parm
*
* RETURNS:
*    status:  at least 1 protocol must be selected
*********************************************************************/
extern status_t
    val_set_ses_protocols_parm (ses_cb_t *scb,
                                val_value_t *parentval);


/********************************************************************
* FUNCTION val_ok_to_partial_lock
*
* Check if the specified root val could be locked
* right now by the specified session
*
* INPUTS:
*   val == start value struct to use
*   sesid == session ID requesting the partial lock
*   lockowner == address of first lock owner violation
*
* OUTPUTS:
*   *lockowner == pointer to first lock owner violation
*
* RETURNS:
*   status:  if any error, then val_clear_partial_lock
*   MUST be called with the start root, to back out any
*   partial operations.  This can happen if the max number
*   of 
*********************************************************************/
extern status_t
    val_ok_to_partial_lock (val_value_t *val,
                            ses_id_t sesid,
                            ses_id_t *lockowner);


/********************************************************************
* FUNCTION val_set_partial_lock
*
* Set the partial lock throughout the value tree
*
* INPUTS:
*   val == start value struct to use
*   plcb == partial lock to set on entire subtree
*
* RETURNS:
*   status:  if any error, then val_clear_partial_lock
*   MUST be called with the start root, to back out any
*   partial operations.  This can happen if the max number
*   of locks reached or lock already help by another session
*********************************************************************/
extern status_t
    val_set_partial_lock (val_value_t *val,
                          plock_cb_t *plcb);


/********************************************************************
* FUNCTION val_clear_partial_lock
*
* Clear the partial lock throughout the value tree
*
* INPUTS:
*   val == start value struct to use
*   plcb == partial lock to clear
*
*********************************************************************/
extern void
    val_clear_partial_lock (val_value_t *val,
                            plock_cb_t *plcb);


/********************************************************************
* FUNCTION val_write_ok
*
* Check if there are any partial-locks owned by another
* session in the node that is going to be written
* If the operation is replace or delete, then the
* entire target subtree will be checked
*
* INPUTS:
*   val == start value struct to use
*   editop == requested write operation
*   sesid == session requesting this write operation
*   checkup == TRUE to check up the tree as well
*   lockid == address of return partial lock ID
*
* OUTPUTS:
*   *lockid == return lock ID if any portion is locked
*              Only the first lock violation detected will
*              be reported
*    error code will be NCX_ERR_IN_USE if *lockid is set
*
* RETURNS:
*    status, NO_ERR indicates no partial lock conflicts
*********************************************************************/
extern status_t
    val_write_ok (val_value_t *val,
                  op_editop_t editop,
                  ses_id_t sesid,
                  boolean checkup,
                  uint32 *lockid);


/********************************************************************
* FUNCTION val_check_swap_resnode
*
* Check if the curnode has any partial locks
* and if so, transfer them to the new node
* and change any resnodes as well
*
* INPUTS:
*   curval == current node to check
*   newval == new value taking its place
*
*********************************************************************/
extern void
    val_check_swap_resnode (val_value_t *curval,
                            val_value_t *newval);


/********************************************************************
* FUNCTION val_check_delete_resnode
*
* Check if the curnode has any partial locks
* and if so, remove them from the final result
*
* INPUTS:
*   curval == current node to check
*
*********************************************************************/
extern void
    val_check_delete_resnode (val_value_t *curval);


/********************************************************************
* FUNCTION val_write_extern
*
* Write an external file to the session
*
* INPUTS:
*   scb == session control block
*   val == value to write (NCX_BT_EXTERN)
*
* RETURNS:
*   none
*********************************************************************/
extern void
    val_write_extern (ses_cb_t *scb,
                      const val_value_t *val);


/********************************************************************
* FUNCTION val_write_intern
*
* Write an internal buffer to the session
*
* INPUTS:
*   scb == session control block
*   val == value to write (NCX_BT_INTERN)
*
* RETURNS:
*   none
*********************************************************************/
extern void
    val_write_intern (ses_cb_t *scb,
                      const val_value_t *val);


/********************************************************************
* FUNCTION val_get_value
* 
* Get the value node for output to a session
* Checks access control if enabled
* Checks filtering via testfn if non-NULL
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write (node from system)
*   acmcheck == TRUE if NACM should be checked; FALSE to skip
*   testcb == callback function to use, NULL if not used
*   malloced == address of return malloced flag
*   res == address of return status
*
* OUTPUTS:
*   *malloced == TRUE if the 
*   *res == return status
*
* RETURNS:
*   value node to use; this is malloced if *malloced is TRUE
*   NULL if some error; check *res; 
*   !!!! check for ERR_NCX_SKIPPED !!!
*********************************************************************/
extern val_value_t *
    val_get_value (ses_cb_t *scb,
                   xml_msg_hdr_t *msg,
                   val_value_t *val,
                   val_nodetest_fn_t testfn,
                   boolean acmtest,
                   boolean *malloced,
                   status_t *res);


/********************************************************************
* FUNCTION val_traverse_keys
* 
* Check ancestor-or-self nodes until root reached
* Find all lists; For each list, starting with the
* closest to root, invoke the callback function
* for each of the key objects in order
*
* INPUTS:
*   val == value node to start check from
*   cookie1 == cookie1 to pass to the callback function
*   cookie2 == cookie2 to pass to the callback function
*   walkerfn == walker callback function
*           returns FALSE to terminate traversal
*
*********************************************************************/
extern void
    val_traverse_keys (val_value_t *val,
                       void *cookie1,
                       void *cookie2,
                       val_walker_fn_t walkerfn);


/********************************************************************
* FUNCTION val_build_index_chains
* 
* Check descendant-or-self nodes for lists
* Check if they have index chains built already
* If not, then try to add one
* for each of the key objects in order
*
* INPUTS:
*   val == value node to start check from
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_build_index_chains (val_value_t *val);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_val_util */
