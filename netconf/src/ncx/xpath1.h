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
#ifndef _H_xpath1
#define _H_xpath1

/*  FILE: xpath1.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    XPath 1.0 expression support

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
13-nov-08    abb      Begun

*/

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xpath1_parse_expr
* 
* Parse the XPATH 1.0 expression string.
*
* parse initial expr with YANG prefixes: must/when
* the object is left out in case it is in a grouping
*
* This is just a first pass done when the
* XPath string is consumed.  If this is a
* YANG file source then the prefixes will be
* checked against the 'mod' import Q
*
* The expression is parsed into XPath tokens
* and checked for well-formed syntax and function
* invocations. Any variable 
*
* If the source is XP_SRC_INSTANCEID, then YANG
* instance-identifier syntax is followed, not XPath 1.0
* This is only used by instance-identifiers in
* default-stmt, conf file, CLI, etc.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*    tkc == parent token chain
*    mod == module in progress
*    pcb == initialized xpath parser control block
*           for the expression; use xpath_new_pcb
*           to initialize before calling this fn
*           The pcb->exprstr MUST BE SET BEFORE THIS CALL
*    source == enum indicating source of this expression
*
* OUTPUTS:
*   pcb->tkc is filled and then partially validated
*   pcb->parseres is set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath1_parse_expr (tk_chain_t *tkc,
		       ncx_module_t *mod,
		       xpath_pcb_t *pcb,
		       xpath_source_t source);


/********************************************************************
* FUNCTION xpath1_validate_expr
* 
* Validate the previously parsed expression string
*   - QName prefixes are valid
*   - function calls are well-formed and exist in
*     the pcb->functions array
*   - variable references exist in the pcb->varbindQ
*
* parse expr with YANG prefixes: must/when
* called from final OBJ xpath check after all
* cooked objects are in place
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*    obj == object containing the XPath clause
*    pcb == the XPath parser control block to process
*
* OUTPUTS:
*   pcb->obj and pcb->objmod are set
*   pcb->validateres is set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath1_validate_expr (ncx_module_t *mod,
			  obj_template_t *obj,
			  xpath_pcb_t *pcb);


/********************************************************************
* FUNCTION xpath1_validate_expr_ex
* 
* Validate the previously parsed expression string
*   - QName prefixes are valid
*   - function calls are well-formed and exist in
*     the pcb->functions array
*   - variable references exist in the &pcb->varbindQ
*
* parse expr with YANG prefixes: must/when
* called from final OBJ xpath check after all
* cooked objects are in place
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*    obj == object containing the XPath clause
*    pcb == the XPath parser control block to process
*    missing_is_error == TRUE if a missing node is an error
*                     == FALSE if a warning
* OUTPUTS:
*   pcb->obj and pcb->objmod are set
*   pcb->validateres is set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath1_validate_expr_ex (ncx_module_t *mod,
                             obj_template_t *obj,
                             xpath_pcb_t *pcb,
                             boolean missing_is_error);


/********************************************************************
* FUNCTION xpath1_eval_expr
* 
* use if the prefixes are YANG: must/when
* Evaluate the expression and get the expression nodeset result
*
* INPUTS:
*    pcb == XPath parser control block to use
*    val == start context node for value of current()
*    docroot == ptr to cfg->root or top of rpc/rpc-replay/notif tree
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*    configonly == 
*           XP_SRC_XML:
*                 TRUE if this is a <get-config> call
*                 and all config=false nodes should be skipped
*                 FALSE if <get> call and non-config nodes
*                 will not be skipped
*           XP_SRC_YANG and XP_SRC_LEAFREF:
*                 should be set to false
*     res == address of return status
*
* OUTPUTS:
*   *res is set to the return status
*
*
* RETURNS:
*   malloced result struct with expr result
*   NULL if no result produced (see *res for reason)
*********************************************************************/
extern xpath_result_t *
    xpath1_eval_expr (xpath_pcb_t *pcb,
		      val_value_t *val,
		      val_value_t *docroot,
		      boolean logerrors,
		      boolean configonly,
		      status_t *res);


/********************************************************************
* FUNCTION xpath1_eval_xmlexpr
* 
* use if the prefixes are XML: select
* Evaluate the expression and get the expression nodeset result
* Called from inside the XML parser, so the XML reader
* must be used to get the XML namespace to prefix mappings
*
* INPUTS:
*    reader == XML reader to use
*    pcb == initialized XPath parser control block
*           the xpath_new_pcb(exprstr) function is
*           all that is needed.  This function will
*           call xpath1_parse_expr if it has not
*           already been called.
*    val == start context node for value of current()
*    docroot == ptr to cfg->root or top of rpc/rpc-replay/notif tree
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*                !!! use FALSE unless DEBUG mode !!!
*    configonly == 
*           XP_SRC_XML:
*                 TRUE if this is a <get-config> call
*                 and all config=false nodes should be skipped
*                 FALSE if <get> call and non-config nodes
*                 will not be skipped
*           XP_SRC_YANG and XP_SRC_LEAFREF:
*                 should be set to false
*     res == address of return status
*
* OUTPUTS:
*   *res is set to the return status
*
* RETURNS:
*   malloced result struct with expr result
*   NULL if no result produced (see *res for reason)
*********************************************************************/
extern xpath_result_t *
    xpath1_eval_xmlexpr (xmlTextReaderPtr reader,
			 xpath_pcb_t *pcb,
			 val_value_t *val,
			 val_value_t *docroot,
			 boolean logerrors,
			 boolean configonly,
			 status_t *res);


/********************************************************************
* FUNCTION xpath1_get_functions_ptr
* 
* Get the start of the function array for XPath 1.0 plus
* the current() function
*
* RETURNS:
*   pointer to functions array
*********************************************************************/
extern const xpath_fncb_t *
    xpath1_get_functions_ptr (void);


/********************************************************************
* FUNCTION xpath1_prune_nodeset
* 
* Check the current result nodeset and remove
* any redundant nodes from a NETCONF POV
* Any node that has an ancestor already in
* the result will be deleted
*
* INPUTS:
*    pcb == XPath parser control block to use
*    result == XPath result nodeset to prune
*
* OUTPUTS:
*    some result->nodeQ contents adjusted or removed
*
*********************************************************************/
extern void
    xpath1_prune_nodeset (xpath_pcb_t *pcb,
			  xpath_result_t *result);


/********************************************************************
* FUNCTION xpath1_check_node_exists
* 
* Check if any ancestor-ot-self node is already in the specified Q
* ONLY FOR VALUE NODES IN THE RESULT
*
* This is only done after all the nodes have been processed
* and the nodeset is complete.  For NETCONF purposes,
* the entire path to root is added for the context node,
* and the entire context node contexts are always returned
*
* INPUTS:
*    pcb == parser control block to use
*    resultQ == Q of xpath_resnode_t structs to check
*               DOES NOT HAVE TO BE WITHIN A RESULT NODE Q
*    val   == value node pointer value to find
*
* RETURNS:
*    TRUE if found, FALSE otherwise
*********************************************************************/
extern boolean
    xpath1_check_node_exists (xpath_pcb_t *pcb,
			      dlq_hdr_t *resultQ,
			      const val_value_t *val);


/********************************************************************
* FUNCTION xpath1_check_node_exists_slow
* 
* Check if any ancestor-ot-self node is already in the specified Q
* ONLY FOR VALUE NODES IN THE RESULT
*
* This is only done after all the nodes have been processed
* and the nodeset is complete.  For NETCONF purposes,
* the entire path to root is added for the context node,
* and the entire context node contexts are always returned
*
* INPUTS:
*    pcb == parser control block to use
*    resultQ == Q of xpath_resnode_t structs to check
*               DOES NOT HAVE TO BE WITHIN A RESULT NODE Q
*    val   == value node pointer value to find
*
* RETURNS:
*    TRUE if found, FALSE otherwise
*********************************************************************/
extern boolean
    xpath1_check_node_exists_slow (xpath_pcb_t *pcb,
                                   dlq_hdr_t *resultQ,
                                   const val_value_t *val);


/********************************************************************
* FUNCTION xpath1_stringify_nodeset
* 
* Convert a value node pointer to a string node
* ONLY FOR VALUE NODES IN THE RESULT
*
*   string(nodeset)
*
* INPUTS:
*    pcb == parser control block to use
*    result == result to stringify
*    str == address of return string
*
* OUTPUTS:
*    *str == malloced and filled in string (if NO_ERR)
*
* RETURNS:
*    status, NO_ER or ERR_INTERNAL_MEM, etc.
*********************************************************************/
extern status_t
    xpath1_stringify_nodeset (xpath_pcb_t *pcb,
			      const xpath_result_t *result,
			      xmlChar **str);


/*******************************************************************
* FUNCTION xpath1_stringify_node
* 
* Convert a value node to a string node
*
*   string(node)
*
* INPUTS:
*    pcb == parser control block to use
*    val == value node to stringify
*    str == address of return string
*
* OUTPUTS:
*    *str == malloced and filled in string (if NO_ERR)
*
* RETURNS:
*    status, NO_ER or ERR_INTERNAL_MEM, etc.
*********************************************************************/
extern status_t
    xpath1_stringify_node (xpath_pcb_t *pcb,
			   val_value_t *val,
			   xmlChar **str);


/********************************************************************
* FUNCTION xpath1_compare_result_to_string
* 
* Compare an XPath result to the specified string
*
*    result = 'string'
*
* INPUTS:
*    pcb == parser control block to use
*    result == result struct to compare
*    strval == string value to compare to result
*    res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*     equality relation result (TRUE or FALSE)
*********************************************************************/
extern boolean
    xpath1_compare_result_to_string (xpath_pcb_t *pcb,
				     xpath_result_t *result,
				     xmlChar *strval,
				     status_t *res);


/********************************************************************
* FUNCTION xpath1_compare_result_to_number
* 
* Compare an XPath result to the specified number
*
*    result = number
*
* INPUTS:
*    pcb == parser control block to use
*    result == result struct to compare
*    numval == number struct to compare to result
*              MUST BE TYPE NCX_BT_FLOAT64
*    res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*     equality relation result (TRUE or FALSE)
*********************************************************************/
extern boolean
    xpath1_compare_result_to_number (xpath_pcb_t *pcb,
				     xpath_result_t *result,
				     ncx_num_t *numval,
				     status_t *res);


/********************************************************************
* FUNCTION xpath1_compare_nodeset_results
* 
* Compare an XPath result to another result
*
*    result1 = node-set
*    result2 = node-set
*
* INPUTS:
*    pcb == parser control block to use
*    result1 == result struct to compare
*    result2 == result struct to compare
*    res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*     equality relation result (TRUE or FALSE)
*********************************************************************/
extern boolean
    xpath1_compare_nodeset_results (xpath_pcb_t *pcb,
                                    xpath_result_t *result1,
                                    xpath_result_t *result2,
                                    status_t *res);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif


#endif	    /* _H_xpath1 */
