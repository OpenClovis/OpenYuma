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
#ifndef _H_xpath_yang
#define _H_xpath_yang

/*  FILE: xpath_yang.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG-specific Xpath support

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
13-nov-08    abb      Begun

*/

#include <xmlstring.h>
#include <xmlreader.h>
#include <xmlregexp.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_val
#include "val.h"
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
* FUNCTION xpath_yang_parse_path
* 
* Parse the leafref path as a leafref path
*
* DOES NOT VALIDATE PATH NODES USED IN THIS PHASE
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used on all objects, even in groupings
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*    tkc == parent token chain (may be NULL)
*    mod == module in progress
*    source == context for this expression
*              XP_SRC_LEAFREF or XP_SRC_INSTANCEID
*    pcb == initialized xpath parser control block
*           for the leafref path; use xpath_new_pcb
*           to initialize before calling this fn.
*           The pcb->exprstr field must be set
*
* OUTPUTS:
*   pcb->tkc is filled and then validated for well-formed
*   leafref or instance-identifier syntax
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_parse_path (tk_chain_t *tkc,
			   ncx_module_t *mod,
			   xpath_source_t source,
			   xpath_pcb_t *pcb);


/********************************************************************
* FUNCTION xpath_yang_parse_path_ex
* 
* Parse the leafref path as a leafref path
*
* DOES NOT VALIDATE PATH NODES USED IN THIS PHASE
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used on all objects, even in groupings
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*    tkc == parent token chain (may be NULL)
*    mod == module in progress
*    obj == context node
*    source == context for this expression
*              XP_SRC_LEAFREF or XP_SRC_INSTANCEID
*    pcb == initialized xpath parser control block
*           for the leafref path; use xpath_new_pcb
*           to initialize before calling this fn.
*           The pcb->exprstr field must be set
*    logerrors == TRUE to log errors; 
*              == FALSE to suppress error messages
* OUTPUTS:
*   pcb->tkc is filled and then validated for well-formed
*   leafref or instance-identifier syntax
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_parse_path_ex (tk_chain_t *tkc,
                              ncx_module_t *mod,
                              xpath_source_t source,
                              xpath_pcb_t *pcb,
                              boolean logerrors);


/********************************************************************
* FUNCTION xpath_yang_validate_path
* 
* Validate the previously parsed leafref path
*   - QNames are valid
*   - object structure referenced is valid
*   - objects are all 'config true'
*   - target object is a leaf
*   - leafref represents a single instance
*
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used only on cooked (real) objects
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*        == NULL if no object in progress
*    obj == object using the leafref data type
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    schemainst == TRUE if ncx:schema-instance string
*               == FALSE to use the pcb->source field 
*                  to determine the exact parse mode
*    leafobj == address of the return target object
*
* OUTPUTS:
*   *leafobj == the target leaf found by parsing the path (NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_validate_path (ncx_module_t *mod,
			      obj_template_t *obj,
			      xpath_pcb_t *pcb,
			      boolean schemainst,
			      obj_template_t **leafobj);


/********************************************************************
* FUNCTION xpath_yang_validate_path_ex
* 
* Validate the previously parsed leafref path
*   - QNames are valid
*   - object structure referenced is valid
*   - objects are all 'config true'
*   - target object is a leaf
*   - leafref represents a single instance
* 
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used only on cooked (real) objects
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*        == NULL if no object in progress
*    obj == object using the leafref data type
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    schemainst == TRUE if ncx:schema-instance string
*               == FALSE to use the pcb->source field 
*                  to determine the exact parse mode
*    leafobj == address of the return target object
*    logerrors == TRUE to log errors, FALSE to suppress errors
*              val_parse uses FALSE if basetype == NCX_BT_UNION
*
* OUTPUTS:
*   *leafobj == the target leaf found by parsing the path (NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_validate_path_ex (ncx_module_t *mod,
                                 obj_template_t *obj,
                                 xpath_pcb_t *pcb,
                                 boolean schemainst,
                                 obj_template_t **leafobj,
                                 boolean logerrors);


/********************************************************************
* FUNCTION xpath_yang_validate_xmlpath
* 
* Validate an instance-identifier expression
* within an XML PDU context
*
* INPUTS:
*    reader == XML reader to use
*    pcb == initialized XPath parser control block
*           with a possibly unchecked pcb->exprstr.
*           This function will  call tk_tokenize_xpath_string
*           if it has not already been called.
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*                !!! use FALSE unless DEBUG mode !!!
*    targobj == address of return target object
*     
* OUTPUTS:
*   *targobj is set to the object that this instance-identifier
*    references, if NO_ERR
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_validate_xmlpath (xmlTextReaderPtr reader,
				 xpath_pcb_t *pcb,
				 obj_template_t *pathobj,
				 boolean logerrors,
				 obj_template_t **targobj);


/********************************************************************
* FUNCTION xpath_yang_validate_xmlkey
* 
* Validate a key XML attribute value given in
* an <edit-config> operation with an 'insert' attribute
* Check that a complete set of predicates is present
* for the specified list of leaf-list
*
* INPUTS:
*    reader == XML reader to use
*    pcb == initialized XPath parser control block
*           with a possibly unchecked pcb->exprstr.
*           This function will  call tk_tokenize_xpath_string
*           if it has not already been called.
*    obj == list or leaf-list object associated with
*           the pcb->exprstr predicate expression
*           (MAY be NULL if first-pass parsing and
*           object is not known yet -- parsed in XML attribute)
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*                !!! use FALSE unless DEBUG mode !!!
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_validate_xmlkey (xmlTextReaderPtr reader,
				xpath_pcb_t *pcb,
				obj_template_t *obj,
				boolean logerrors);


/********************************************************************
* FUNCTION xpath_yang_make_instanceid_val
* 
* Make a value subtree out of an instance-identifier
* Used by yangcli to send PDUs from CLI target parameters
*
* The XPath pcb must be previously parsed and found valid
* It must be an instance-identifier value, 
* not a leafref path
*
* INPUTS:
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    retres == address of return status (may be NULL)
*    deepest == address of return deepest node created (may be NULL)
*
* OUTPUTS:
*   if (non-NULL)
*       *retres == return status
*       *deepest == pointer to end of instance-id chain node
* RETURNS:
*   malloced value subtree representing the instance-identifier
*   in internal val_value_t data structures
*********************************************************************/
extern val_value_t *
    xpath_yang_make_instanceid_val (xpath_pcb_t *pcb,
				    status_t *retres,
				    val_value_t **deepest);


/********************************************************************
* FUNCTION xpath_yang_get_namespaces
* 
* Get the namespace URI IDs used in the specified
* XPath expression;
* 
* usually an instance-identifier or schema-instance node
* but this function simply reports all the TK_TT_MSTRING
* tokens that have an nsid set
*
* The XPath pcb must be previously parsed and found valid
*
* INPUTS:
*    pcb == the XPath parser control block to use
*    nsid_array == address of return array of xmlns_id_t
*    max_nsids == number of NSIDs that can be held
*    num_nsids == address of return number of NSIDs
*                 written to the buffer. No duplicates
*                 will be present in the buffer
*
* OUTPUTS:
*    nsid_array[0..*num_nsids] == returned NSIDs used
*       in the XPath expression
*    *num_nsids == number of NSIDs written to the array
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_yang_get_namespaces (const xpath_pcb_t *pcb,
			       xmlns_id_t *nsid_array,
			       uint32 max_nsids,
			       uint32 *num_nsids);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xpath_yang */
