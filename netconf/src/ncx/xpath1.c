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
/*  FILE: xpath1.c

    Xpath 1.0 search support
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13nov08      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>
#include  <assert.h>

#include <xmlstring.h>

#include  "procdefs.h"
#include "def_reg.h"
#include "dlq.h"
#include "grp.h"
#include "ncxconst.h"
#include "ncx.h"
#include "ncx_feature.h"
#include "ncx_num.h"
#include "obj.h"
#include "tk.h"
#include "typ.h"
#include "xpath.h"
#include "xpath1.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define XPATH1_PARSE_DEBUG 1 */
/* #define EXTRA_DEBUG 1 */

#define TEMP_BUFFSIZE  1024

/********************************************************************
*                                                                   *
*           F O R W A R D   D E C L A R A T I O N S                 *
*                                                                   *
*********************************************************************/
static xpath_result_t* parse_expr( xpath_pcb_t *pcb, status_t  *res); 
static xpath_result_t* boolean_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                   status_t *res );
static xpath_result_t* ceiling_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ,
                                   status_t *res);
static xpath_result_t* concat_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                  status_t *res); 
static xpath_result_t* contains_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                    status_t *res); 
static xpath_result_t* count_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                 status_t *res); 
static xpath_result_t* current_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                   status_t *res); 
static xpath_result_t* false_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                 status_t *res); 
static xpath_result_t* floor_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                 status_t *res); 
static xpath_result_t* id_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                              status_t *res); 
static xpath_result_t* lang_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                status_t *res); 
static xpath_result_t* last_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                status_t *res); 
static xpath_result_t* local_name_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                      status_t *res); 
static xpath_result_t* namespace_uri_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                         status_t *res); 
static xpath_result_t* name_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                status_t *res); 
static xpath_result_t* normalize_space_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                           status_t *res); 
static xpath_result_t* not_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                               status_t *res); 
static xpath_result_t* number_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                  status_t *res); 
static xpath_result_t* position_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                    status_t *res); 
static xpath_result_t* round_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                 status_t *res); 
static xpath_result_t* starts_with_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                       status_t *res); 
static xpath_result_t* string_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                  status_t *res); 
static xpath_result_t* string_length_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                         status_t *res); 
static xpath_result_t* substring_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                     status_t *res); 
static xpath_result_t* substring_after_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                           status_t *res); 
static xpath_result_t* substring_before_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                            status_t *res); 
static xpath_result_t* sum_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                               status_t *res); 
static xpath_result_t* translate_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                     status_t *res); 
static xpath_result_t* true_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                status_t *res); 
static xpath_result_t* module_loaded_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                         status_t *res); 
static xpath_result_t* feature_enabled_fn( xpath_pcb_t *pcb, dlq_hdr_t *parmQ, 
                                           status_t *res); 

/********************************************************************
*                                  *
*                         V A R I A B L E S                         *
*                                                                   *
*********************************************************************/

static xpath_fncb_t functions [] = {
    { XP_FN_BOOLEAN, XP_RT_BOOLEAN, 1, boolean_fn },
    { XP_FN_CEILING, XP_RT_NUMBER, 1, ceiling_fn },
    { XP_FN_CONCAT, XP_RT_STRING, -1, concat_fn },
    { XP_FN_CONTAINS, XP_RT_BOOLEAN, 2, contains_fn },
    { XP_FN_COUNT, XP_RT_NUMBER, 1, count_fn },
    { XP_FN_CURRENT, XP_RT_NODESET, 0, current_fn },
    { XP_FN_FALSE, XP_RT_BOOLEAN, 0, false_fn },
    { XP_FN_FLOOR, XP_RT_NUMBER, 1, floor_fn },
    { XP_FN_ID, XP_RT_NODESET, 1, id_fn },
    { XP_FN_LANG, XP_RT_BOOLEAN, 1, lang_fn },
    { XP_FN_LAST, XP_RT_NUMBER, 0, last_fn },
    { XP_FN_LOCAL_NAME, XP_RT_STRING, -1, local_name_fn },
    { XP_FN_NAME, XP_RT_STRING, -1, name_fn },
    { XP_FN_NAMESPACE_URI, XP_RT_STRING, -1, namespace_uri_fn },
    { XP_FN_NORMALIZE_SPACE, XP_RT_STRING, -1, normalize_space_fn },
    { XP_FN_NOT, XP_RT_BOOLEAN, 1, not_fn },
    { XP_FN_NUMBER, XP_RT_NUMBER, -1, number_fn },
    { XP_FN_POSITION, XP_RT_NUMBER, 0, position_fn },
    { XP_FN_ROUND, XP_RT_NUMBER, 1, round_fn },
    { XP_FN_STARTS_WITH, XP_RT_BOOLEAN, 2, starts_with_fn },
    { XP_FN_STRING, XP_RT_STRING, -1, string_fn },
    { XP_FN_STRING_LENGTH, XP_RT_NUMBER, -1, string_length_fn },
    { XP_FN_SUBSTRING, XP_RT_STRING, -1, substring_fn },
    { XP_FN_SUBSTRING_AFTER, XP_RT_STRING, 2, substring_after_fn },
    { XP_FN_SUBSTRING_BEFORE, XP_RT_STRING, 2, substring_before_fn },
    { XP_FN_SUM, XP_RT_NUMBER, 1, sum_fn },
    { XP_FN_TRANSLATE, XP_RT_STRING, 3, translate_fn },
    { XP_FN_TRUE, XP_RT_BOOLEAN, 0, true_fn },
    { XP_FN_MODULE_LOADED, XP_RT_BOOLEAN, -1, module_loaded_fn },
    { XP_FN_FEATURE_ENABLED, XP_RT_BOOLEAN, 2, feature_enabled_fn },
    { NULL, XP_RT_NONE, 0, NULL }   /* last entry marker */
};


/********************************************************************
* FUNCTION set_uint32_num
* 
* Set an ncx_num_t with a uint32 number
*
* INPUTS:
*    innum == number value to use
*    outnum == address ofoutput number
*
* OUTPUTS:
*   *outnum is set with the converted value
*    
*********************************************************************/
static void
    set_uint32_num (uint32 innum,
                    ncx_num_t  *outnum)
{
#ifdef HAS_FLOAT
    outnum->d = (double)innum;
#else
    outnum->d = (int64)innum;
#endif

}  /* set_uint32_num */


/********************************************************************
* FUNCTION malloc_failed_error
* 
* Generate a malloc failed error if OK
*
* INPUTS:
*    pcb == parser control block to use
*********************************************************************/
static void
    malloc_failed_error (xpath_pcb_t *pcb)
{
    if (pcb->logerrors) {
        if (pcb->exprstr) {
            log_error("\nError: malloc failed in "
                      "Xpath expression '%s'.",
                      pcb->exprstr);
        } else {
            log_error("\nError: malloc failed in "
                      "Xpath expression");
        }
        ncx_print_errormsg(pcb->tkc, 
                           pcb->tkerr.mod, 
                           ERR_INTERNAL_MEM);
    }

}  /* malloc_failed_error */


/********************************************************************
* FUNCTION no_parent_warning
* 
* Generate a no parent available error if OK
*
* INPUTS:
*    pcb == parser control block to use
*********************************************************************/
static void
    no_parent_warning (xpath_pcb_t *pcb)
{
    if (pcb->logerrors && 
        ncx_warning_enabled(ERR_NCX_NO_XPATH_PARENT)) {
        log_warn("\nWarning: no parent found "
                  "in XPath expr '%s'", pcb->exprstr);
        ncx_print_errormsg(pcb->tkc, 
                           pcb->objmod, 
                           ERR_NCX_NO_XPATH_PARENT);
    } else if (pcb->objmod != NULL) {
        ncx_inc_warnings(pcb->objmod);
    }

}  /* no_parent_warning */


/********************************************************************
* FUNCTION unexpected_error
* 
* Generate an unexpected token error if OK
*
* INPUTS:
*    pcb == parser control block to use
*********************************************************************/
static void
    unexpected_error (xpath_pcb_t *pcb)
{
    if (pcb->logerrors) {
        if (TK_CUR(pcb->tkc)) {
            log_error("\nError: Unexpected token '%s' in "
                      "XPath expression '%s'",
                      tk_get_token_name(TK_CUR_TYP(pcb->tkc)),
                      pcb->exprstr);
        } else {
            log_error("\nError: End reached in "
                      "XPath expression '%s'",  pcb->exprstr);
        }
        ncx_print_errormsg(pcb->tkc, 
                           pcb->tkerr.mod, 
                           ERR_NCX_WRONG_TKTYPE);
    }

}  /* unexpected_error */


/********************************************************************
* FUNCTION wrong_parmcnt_error
* 
* Generate a wrong function parameter count error if OK
*
* INPUTS:
*    pcb == parser control block to use
*    parmcnt == the number of parameters received
*    res == error result to use
*********************************************************************/
static void
    wrong_parmcnt_error (xpath_pcb_t *pcb,
                         uint32 parmcnt,
                         status_t res)
{
    if (pcb->logerrors) {
        log_error("\nError: wrong function arg count '%u' in "
                  "Xpath expression '%s'",
                  parmcnt,
                  pcb->exprstr);
        ncx_print_errormsg(pcb->tkc, 
                           pcb->tkerr.mod, 
                           res);
    }

}  /* wrong_parmcnt_error */


/********************************************************************
* FUNCTION invalid_instanceid_error
* 
* Generate a invalid instance identifier error
*
* INPUTS:
*    pcb == parser control block to use
*********************************************************************/
static void
    invalid_instanceid_error (xpath_pcb_t *pcb)
{
    if (pcb->logerrors) {
        log_error("\nError: XPath found in instance-identifier '%s'",
                  pcb->exprstr);
        ncx_print_errormsg(pcb->tkc, 
                           pcb->tkerr.mod, 
                           ERR_NCX_INVALID_INSTANCEID);
    }

}  /* invalid_instanceid_error */


/********************************************************************
* FUNCTION check_instanceid_expr
* 
* Check the form of an instance-identifier expression
* The operation has been checked already and the
* expression is:
*
*       leftval = rightval
*
* Make sure the CLRs in the YANG spec are followed
* DOES NOT check the actual result value, just
* tries to keep track of the expression syntax
*
* Use check_instanceid_result to finish the 
* instance-identifier tests
*
* INPUTS:
*    pcb == parser control block to use
*    leftval == LHS result
*    rightval == RHS result
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_instanceid_expr (xpath_pcb_t *pcb,
                           xpath_result_t *leftval,
                           xpath_result_t *rightval)
{
    val_value_t           *contextval;
    const char            *msg;
    status_t               res;
    boolean                haserror;

    /* skip unless this is a real value tree eval */
    if (!pcb->val || !pcb->val_docroot) {
        return NO_ERR;
    }

    haserror = FALSE;
    res = ERR_NCX_INVALID_INSTANCEID;
    msg = NULL;

    /* check LHS which is supposed to be a nodeset.
     * it can only be an empty nodeset if the
     * instance-identifier is constrained to
     * existing values by 'require-instance true'
     */
    if (leftval) {
        if (leftval->restype != XP_RT_NODESET) {
            msg = "LHS has wrong data type";
            haserror = TRUE;
        } else {
            /* check out the nodeset contents */
            contextval = pcb->context.node.valptr;
            if (!contextval) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }

            if (contextval->obj->objtype == OBJ_TYP_LIST ||
                contextval->obj->objtype == OBJ_TYP_LEAF_LIST) {
                /* the '.=result' must be the format, and be
                 * the same as the context for a leaf-list
                 * For a list, the name='fred' expr is used,
                 *   'name' must be a key leaf
                 *    of the context node
                 * check the actual result later
                 */
                ;
            } else {
                msg = "wrong context node type";
                haserror = TRUE;
            }
        }
    } else {
        haserror = TRUE;
        msg = "missing LHS value in predicate";
    }

    if (!haserror) {
        if (rightval) {
            if (!(rightval->restype == XP_RT_STRING ||
                  rightval->restype == XP_RT_NUMBER)) {
                msg = "RHS has wrong data type";
                haserror = TRUE;
            }
        } else {
            haserror = TRUE;
            msg = "missing RHS value in predicate";
        }
    }

    if (haserror) {
        if (pcb->logerrors) {
            log_error("\nError: %s in instance-identifier '%s'",
                      msg, pcb->exprstr);
            ncx_print_errormsg(pcb->tkc, 
                               pcb->tkerr.mod, 
                               res);
        }
        return res;
    } else {
        return NO_ERR;
    }
    
}  /* check_instanceid_expr */


/********************************************************************
* FUNCTION check_instance_result
* 
* Check the results of a leafref or instance-identifier expression
*
* Make sure the require-instance condition is met
* if it is set to 'true'
*
* INPUTS:
*    pcb == parser control block to use
*    resultl == complete instance-identifier result to check
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_instance_result (xpath_pcb_t *pcb,
                           xpath_result_t *result)
{
    val_value_t           *contextval;
    const char            *msg;
    status_t               res;
    boolean                haserror, constrained;
    uint32                 nodecount;
    ncx_btype_t            btyp;

    /* skip unless this is a real value tree eval */
    if (!pcb->val || !pcb->val_docroot) {
        return NO_ERR;
    }

    haserror = FALSE;
    res = ERR_NCX_INVALID_INSTANCEID;
    msg = NULL;

    if (result->restype != XP_RT_NODESET) {
        msg = "wrong result type";
        haserror = TRUE;
    } else {
        /* check the node count */
        nodecount = dlq_count(&result->r.nodeQ);

        if (nodecount > 1) {
            if (pcb->val->btyp == NCX_BT_INSTANCE_ID) {
                msg = "too many instances";
                haserror = TRUE;
            }
        } else {
            /* check out the nodeset contents */
            contextval = pcb->val;
            if (!contextval) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }

            btyp = contextval->btyp;

            if (btyp == NCX_BT_INSTANCE_ID || 
                btyp == NCX_BT_LEAFREF) {
                constrained = 
                    typ_get_constrained
                    (obj_get_ctypdef(contextval->obj));
            } else {
                constrained = TRUE;
            }

            if (constrained && !nodecount) {
                /* should have matched exactly one instance */
                msg = "missing instance";
                haserror = TRUE;
                res = ERR_NCX_MISSING_INSTANCE;
            }
        }
    }

    if (haserror) {
        if (pcb->logerrors) {
            if (pcb->val->btyp == NCX_BT_LEAFREF) {
                log_error("\nError: %s in leafref path '%s'",
                          msg, pcb->exprstr);
            } else {
                log_error("\nError: %s in instance-identifier '%s'",
                          msg, pcb->exprstr);
            }
            ncx_print_errormsg(pcb->tkc, 
                               pcb->tkerr.mod, 
                               res);
        }
        return res;
    } else {
        return NO_ERR;
    }
    
}  /* check_instance_result */


/********************************************************************
* FUNCTION new_result
* 
* Get a new result from the cache or malloc if none available
*
* INPUTS:
*    pcb == parser control block to use
*    restype == desired result type
*
* RETURNS:
*    result from the cache or malloced; NULL if malloc fails
*********************************************************************/
static xpath_result_t *
    new_result (xpath_pcb_t *pcb,
                xpath_restype_t restype)
{
    xpath_result_t  *result;

    result = (xpath_result_t *)dlq_deque(&pcb->result_cacheQ);
    if (result) {
        pcb->result_count--;
        xpath_init_result(result, restype);
    } else {
        result = xpath_new_result(restype);
    }

    if (!result) {
        malloc_failed_error(pcb);
    }

    return result;

} /* new_result */


/********************************************************************
* FUNCTION free_result
* 
* Free a result struct: put in cache or free if cache maxed out
*
* INPUTS:
*    pcb == parser control block to use
*    result == result struct to free
*
*********************************************************************/
static void free_result (xpath_pcb_t *pcb, xpath_result_t *result)
{
    xpath_resnode_t *resnode;

    if (result->restype == XP_RT_NODESET) {
        while (!dlq_empty(&result->r.nodeQ) &&
               pcb->resnode_count < XPATH_RESNODE_CACHE_MAX) {

            resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);
            xpath_clean_resnode(resnode);
            dlq_enque(resnode, &pcb->resnode_cacheQ);
            pcb->resnode_count++;
        }
    }

    if (pcb->result_count < XPATH_RESULT_CACHE_MAX) {
        xpath_clean_result(result);
        dlq_enque(result, &pcb->result_cacheQ);
        pcb->result_count++;
    } else {
        xpath_free_result(result);
    }

} /* free_result */


/********************************************************************
* FUNCTION new_obj_resnode
* 
* Get a new result node from the cache or malloc if none available
*
* INPUTS:
*    pcb == parser control block to use
*           if pcb->val set then node.valptr will be used
*           else node.objptr will be used instead
*    position == position within the current search
*    dblslash == TRUE if // present on this result node
*                and has not been converted to separate
*                nodes for all descendants instead
*             == FALSE if '//' is not in effect for the 
*                current step
*    objptr == object pointer value to use
*
* RETURNS:
*    result from the cache or malloced; NULL if malloc fails
*********************************************************************/
static xpath_resnode_t *
    new_obj_resnode (xpath_pcb_t *pcb,
                     int64 position,
                     boolean dblslash,
                     obj_template_t *objptr)
{
    xpath_resnode_t  *resnode;

    resnode = (xpath_resnode_t *)dlq_deque(&pcb->resnode_cacheQ);
    if (resnode) {
        pcb->resnode_count--;
    } else {
        resnode = xpath_new_resnode();
    }

    if (!resnode) {
        malloc_failed_error(pcb);
    } else  {
        resnode->position = position;
        resnode->dblslash = dblslash;
        resnode->node.objptr = objptr;
    }

    return resnode;

} /* new_obj_resnode */


/********************************************************************
* FUNCTION new_val_resnode
* 
* Get a new result node from the cache or malloc if none available
*
* INPUTS:
*    pcb == parser control block to use
*           if pcb->val set then node.valptr will be used
*           else node.objptr will be used instead
*    position == position within the search context
*    dblslash == TRUE if // present on this result node
*                and has not been converted to separate
*                nodes for all descendants instead
*             == FALSE if '//' is not in effect for the 
*                current step
*    valptr == variable pointer value to use
*
* RETURNS:
*    result from the cache or malloced; NULL if malloc fails
*********************************************************************/
static xpath_resnode_t *
    new_val_resnode (xpath_pcb_t *pcb,
                     int64 position,
                     boolean dblslash,
                     val_value_t *valptr)
{
    xpath_resnode_t  *resnode;

    resnode = (xpath_resnode_t *)dlq_deque(&pcb->resnode_cacheQ);
    if (resnode) {
        pcb->resnode_count--;
    } else {
        resnode = xpath_new_resnode();
    }

    if (!resnode) {
        malloc_failed_error(pcb);
    } else {
        resnode->position = position;
        resnode->dblslash = dblslash;
        resnode->node.valptr = valptr;
    }

    return resnode;

} /* new_val_resnode */


/********************************************************************
* FUNCTION free_resnode
* 
* Free a result node struct: put in cache or free if cache maxed out
*
* INPUTS:
*    pcb == parser control block to use
*    resnode == result node struct to free
*
*********************************************************************/
static void
    free_resnode (xpath_pcb_t *pcb,
                  xpath_resnode_t *resnode)
{
    if (pcb->resnode_count < XPATH_RESNODE_CACHE_MAX) {
        xpath_clean_resnode(resnode);
        dlq_enque(resnode, &pcb->resnode_cacheQ);
        pcb->resnode_count++;
    } else {
        xpath_free_resnode(resnode);
    }

} /* free_resnode */


/********************************************************************
* FUNCTION new_nodeset
* 
* Start a nodeset result with a specified object or value ptr
* Only one of obj or val will really be stored,
* depending on the current parsing mode
*
* INPUTS:
*    pcb == parser control block to use
*    obj == object ptr to store as the first resnode
*    val == value ptr to store as the first resnode
*    dblslash == TRUE if unprocessed dblslash in effect
*                FALSE if not
*
* RETURNS:
*    malloced data structure or NULL if out-of-memory error
*********************************************************************/
static xpath_result_t *
    new_nodeset (xpath_pcb_t *pcb,
                 obj_template_t *obj,
                 val_value_t *val,
                 int64 position,
                 boolean dblslash)
{
    xpath_result_t  *result;
    xpath_resnode_t *resnode;
    
    result = new_result(pcb, XP_RT_NODESET);
    if (!result) {
        return NULL;
    }

    if (obj || val) {
        if (pcb->val) {
            resnode = new_val_resnode(pcb, 
                                      position, 
                                      dblslash, 
                                      val);
            result->isval = TRUE;
        } else {
            resnode = new_obj_resnode(pcb, 
                                      position, 
                                      dblslash, 
                                      obj);
            result->isval = FALSE;
        }
        if (!resnode) {
            xpath_free_result(result);
            return NULL;
        }
        dlq_enque(resnode, &result->r.nodeQ);
    }

    result->last = 1;

    return result;

} /* new_nodeset */


/********************************************************************
* FUNCTION convert_compare_result
* 
* Convert an int32 compare result to a boolean
* based on the XPath relation op
*
* INPUTS:
*    cmpresult == compare result
*    exop == XPath relational or equality expression OP
*
* RETURNS:
*    TRUE if relation is TRUE
*    FALSE if relation is FALSE
*********************************************************************/
static boolean
    convert_compare_result (int32 cmpresult,
                            xpath_exop_t exop)
{
    switch (exop) {
    case XP_EXOP_EQUAL:
        return (cmpresult) ? FALSE : TRUE;
    case XP_EXOP_NOTEQUAL:
        return (cmpresult) ? TRUE : FALSE;
    case XP_EXOP_LT:
        return (cmpresult < 0) ? TRUE : FALSE;
    case XP_EXOP_GT:
        return (cmpresult > 0) ? TRUE : FALSE;
    case XP_EXOP_LEQUAL:
        return (cmpresult <= 0) ? TRUE : FALSE;
    case XP_EXOP_GEQUAL:
        return (cmpresult >= 0) ? TRUE : FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return TRUE;
    }
    /*NOTREACHED*/

} /* convert_compare_result */


/********************************************************************
* FUNCTION compare_strings
* 
* Compare str1 to str2
* Use the specified operation
*
*    str1  <op>  str2
*
* INPUTS:
*    str1 == left hand side string
*    str2 == right hand side string
*    exop == XPath expression op to use
*
* RETURNS:
*    TRUE if relation is TRUE
*    FALSE if relation is FALSE
*********************************************************************/
static boolean
    compare_strings (const xmlChar *str1,
                     const xmlChar *str2,
                     xpath_exop_t  exop)
{
    int32  cmpresult;

    cmpresult = xml_strcmp(str1, str2);
    return convert_compare_result(cmpresult, exop);

} /* compare_strings */


/********************************************************************
* FUNCTION compare_numbers
* 
* Compare str1 to str2
* Use the specified operation
*
*    num1  <op>  num2
*
* INPUTS:
*    num1 == left hand side number
*    numstr2 == right hand side string to convert to num2
*               and then compare to num1
*           == NULL or empty string to compare to NAN
*    exop == XPath expression op to use
*
* RETURNS:
*    TRUE if relation is TRUE
*    FALSE if relation is FALSE
*********************************************************************/
static boolean
    compare_numbers (const ncx_num_t *num1,
                     const xmlChar *numstr2,
                     xpath_exop_t  exop)
{
    int32        cmpresult;
    ncx_num_t    num2;
    status_t     res;
    ncx_numfmt_t numfmt;

    cmpresult = 0;
    numfmt = NCX_NF_DEC;
    res = NO_ERR;

    if (numstr2 && *numstr2) {
        numfmt = ncx_get_numfmt(numstr2);
        if (numfmt == NCX_NF_OCTAL) {
            numfmt = NCX_NF_DEC;
        }
    }

    if (numfmt == NCX_NF_DEC || numfmt == NCX_NF_REAL) {
        ncx_init_num(&num2);

        if (numstr2 && *numstr2) {
            res = ncx_convert_num(numstr2, 
                                  numfmt, 
                                  NCX_BT_FLOAT64, 
                                  &num2);
        } else {
            ncx_set_num_nan(&num2, NCX_BT_FLOAT64);
        }
        if (res == NO_ERR) {
            cmpresult = ncx_compare_nums(num1, 
                                         &num2, 
                                         NCX_BT_FLOAT64);
        }
        ncx_clean_num(NCX_BT_FLOAT64, &num2);
    } else if (numfmt == NCX_NF_NONE) {
        res = ERR_NCX_INVALID_VALUE;
    } else {
        res = ERR_NCX_WRONG_NUMTYP;
    }

    if (res != NO_ERR) {
        return FALSE;
    }
    return convert_compare_result(cmpresult, exop);

} /* compare_numbers */


/********************************************************************
* FUNCTION compare_booleans
* 
* Compare bool1 to bool2
* Use the specified operation
*
*    bool1  <op>  bool2
*
* INPUTS:
*    num1 == left hand side number
*    str2 == right hand side string to convert to a number
*            and then compare to num1
*    exop == XPath expression op to use
*
* RETURNS:
*    TRUE if relation is TRUE
*    FALSE if relation is FALSE
*********************************************************************/
static boolean
    compare_booleans (boolean bool1,
                      boolean bool2,
                      xpath_exop_t  exop)
{

    int32  cmpresult;

    if ((bool1 && bool2) || (!bool1 && !bool2)) {
        cmpresult = 0;
    } else if (bool1) {
        cmpresult = 1;
    } else {
        cmpresult = -1;
    }

    return convert_compare_result(cmpresult, exop);

}  /* compare_booleans */


/********************************************************************
* FUNCTION compare_walker_fn
* 
* Compare the parm string to the current value
* Stop the walk on the first TRUE comparison
*
*    val1 <op>  val2
*
*  val1 == parms.cmpstring or parms.cmpnum
*  val2 == string value of node passed by val walker
*    op == parms.exop
*
* Matches val_walker_fn_t template in val.h
*
* INPUTS:
*    val == value node found in the search, used as 'val2'
*    cookie1 == xpath_pcb_t * : parser control block to use
*               currently not used!!!
*    cookie2 == xpath_compwalkerparms_t *: walker parms to use
*                cmpstring or cmpnum is used, not result2
* OUTPUTS:
*    *cookie2 contents adjusted  (parms.cmpresult and parms.res)
*
* RETURNS:
*    TRUE to keep walk going
*    FALSE to terminate walk
*********************************************************************/
static boolean
    compare_walker_fn (val_value_t *val,
                       void *cookie1,
                       void *cookie2)
{
    val_value_t              *useval, *v_val;
    xpath_compwalkerparms_t  *parms;
    xmlChar                  *buffer;
    status_t                  res;
    uint32                    cnt;

    (void)cookie1;
    parms = (xpath_compwalkerparms_t *)cookie2;

    /* skip all complex nodes */
    if (!typ_is_simple(val->btyp)) {
        return TRUE;
    }

    v_val = NULL;
    res = NO_ERR;

    if (val_is_virtual(val)) {
        v_val = val_get_virtual_value(NULL, val, &res);
        if (v_val == NULL) {
            parms->res = res;
            parms->cmpresult = FALSE;
            return FALSE;
        } else {
            useval = v_val;
        }
    } else {
        useval = val;
    }

    if (obj_is_password(val->obj)) {
        parms->cmpresult = FALSE;
    } else if (typ_is_string(val->btyp)) {
        if (parms->cmpstring) {
            parms->cmpresult = 
                compare_strings(parms->cmpstring, 
                                VAL_STR(useval),
                                parms->exop);
        } else {
            parms->cmpresult = 
                compare_numbers(parms->cmpnum, 
                                VAL_STR(useval),
                                parms->exop);
        }
    } else {
        /* get value sprintf size */
        res = val_sprintf_simval_nc(NULL, useval, &cnt);
        if (res != NO_ERR) {
            parms->res = res;
            return FALSE;
        }

        if (cnt < parms->buffsize) {
            /* use pre-allocated buffer */
            res = val_sprintf_simval_nc(parms->buffer, useval, &cnt);
            if (res == NO_ERR) {
                if (parms->cmpstring) {
                    parms->cmpresult = 
                        compare_strings(parms->cmpstring, 
                                        parms->buffer,
                                        parms->exop);
                } else {
                    parms->cmpresult = 
                        compare_numbers(parms->cmpnum, 
                                        parms->buffer,
                                        parms->exop);
                }
            } else {
                parms->res = res;
                return FALSE;
            }
        } else {
            /* use a temp buffer */
            buffer = m__getMem(cnt+1);
            if (!buffer) {
                parms->res = ERR_INTERNAL_MEM;
                return FALSE;
            }

            res = val_sprintf_simval_nc(buffer, useval, &cnt);
            if (res != NO_ERR) {
                m__free(buffer);
                parms->res = res;
                return FALSE;
            }

            if (parms->cmpstring) {
                parms->cmpresult = 
                    compare_strings(parms->cmpstring, 
                                    buffer,
                                    parms->exop);
            } else {
                parms->cmpresult = 
                    compare_numbers(parms->cmpnum, 
                                    buffer,
                                    parms->exop);
            }

            m__free(buffer);
        }
    }

    return !parms->cmpresult;

}  /* compare_walker_fn */


/********************************************************************
* FUNCTION top_compare_walker_fn
* 
* Compare the current string in the 1st nodeset
* to the entire 2nd node-set
*
* This callback should get called once for every
* node in the first parmset.  Each time a simple
* type is passed into the callback, the entire
* result2 is processed with new callbacks to
* compare the 'cmpstring' against each simple
* type in the 2nd node-set.
*
* Stop the walk on the first TRUE comparison
*
* Matches val_walker_fn_t template in val.h
*
* INPUTS:
*    val == value node found in the search
*    cookie1 == xpath_pcb_t * : parser control block to use
*               currently not used!!!
*    cookie2 == xpath_compwalkerparms_t *: walker parms to use
*               result2 is used, not cmpstring
* OUTPUTS:
*    *cookie2 contents adjusted  (parms.cmpresult and parms.res)
*
* RETURNS:
*    TRUE to keep walk going
*    FALSE to terminate walk
*********************************************************************/
static boolean
    top_compare_walker_fn (val_value_t *val,
                           void *cookie1,
                           void *cookie2)
{
    xpath_compwalkerparms_t  *parms, newparms;
    xmlChar                  *buffer, *comparestr;
    xpath_pcb_t              *pcb;
    xpath_resnode_t          *resnode;
    val_value_t              *testval, *useval, *newval;
    status_t                  res;
    uint32                    cnt;
    boolean                   fnresult, cfgonly;

    pcb = (xpath_pcb_t *)cookie1;
    parms = (xpath_compwalkerparms_t *)cookie2;

    /* skip all complex nodes */
    if (!typ_is_simple(val->btyp)) {
        return TRUE;
    }

    res = NO_ERR;
    buffer = NULL;
    newval = NULL;

    cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;

    if (val_is_virtual(val)) {
        newval = val_get_virtual_value(NULL, val, &res);
        if (newval == NULL) {
            parms->res = res;
            parms->cmpresult = FALSE;
            return FALSE;
        } else {
            useval = newval;
        }
    } else {
        useval = val;
    }
        
    if (typ_is_string(val->btyp)) {
        comparestr = VAL_STR(useval);
    } else {
        /* get value sprintf size */
        res = val_sprintf_simval_nc(NULL, useval, &cnt);
        if (res != NO_ERR) {
            parms->res = res;
            return FALSE;
        }

        if (cnt < parms->buffsize) {
            /* use pre-allocated buffer */
            res = val_sprintf_simval_nc(parms->buffer, useval, &cnt);
            if (res != NO_ERR) {
                parms->res = res;
                return FALSE;
            }
            comparestr = parms->buffer;
        } else {
            /* use a temp buffer */
            buffer = m__getMem(cnt+1);
            if (!buffer) {
                parms->res = ERR_INTERNAL_MEM;
                return FALSE;
            }

            res = val_sprintf_simval_nc(buffer, useval, &cnt);
            if (res != NO_ERR) {
                m__free(buffer);
                parms->res = res;
                return FALSE;
            }
            comparestr = buffer;
        }
    }

    /* setup 2nd walker parms */
    memset(&newparms, 0x0, sizeof(xpath_compwalkerparms_t));
    newparms.cmpstring = comparestr;
    newparms.buffer = m__getMem(TEMP_BUFFSIZE);
    if (!newparms.buffer) {
        if (buffer) {
            m__free(buffer);
        }
        parms->res = ERR_INTERNAL_MEM;
        return FALSE;
    }
    newparms.buffsize = TEMP_BUFFSIZE;
    newparms.exop = parms->exop;
    newparms.cmpresult = FALSE;
    newparms.res = NO_ERR;

    /* go through all the nodes in the first node-set
     * and compare each leaf against all the leafs
     * in the other node-set; stop when condition
     * is met or both node-sets completely searched
     */
    for (resnode = (xpath_resnode_t *)
             dlq_firstEntry(&parms->result2->r.nodeQ);
         resnode != NULL && res == NO_ERR;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        testval = resnode->node.valptr;
        
        fnresult = 
            val_find_all_descendants(compare_walker_fn,
                                     pcb, 
                                     &newparms,
                                     testval, 
                                     NULL,
                                     NULL,
                                     cfgonly,
                                     FALSE,
                                     TRUE,
                                     TRUE);
        if (newparms.res != NO_ERR) {
            res = newparms.res;
            parms->res = res;
        } else if (!fnresult) {
            /* condition was met if return FALSE and
             * walkerparms.res == NO_ERR
             */
            if (buffer) {
                m__free(buffer);
            }
            m__free(newparms.buffer);
            parms->res = NO_ERR;
            return FALSE;
        }
    }

    if (buffer != NULL) {
        m__free(buffer);
    }

    m__free(newparms.buffer);

    return TRUE;

}  /* top_compare_walker_fn */


/********************************************************************
* FUNCTION compare_nodeset_to_other
* 
* Compare 2 results, 1 of them is a node-set
*
* INPUTS:
*    pcb == parser control block to use
*    val1 == first result struct to compare
*    val2 == second result struct to compare
*    exop == XPath expression operator to use
*    res == address of resturn status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*    TRUE if relation is TRUE
*    FALSE if relation is FALSE or some error
*********************************************************************/
static boolean
    compare_nodeset_to_other (xpath_pcb_t *pcb,
                              xpath_result_t *val1,
                              xpath_result_t *val2,
                              xpath_exop_t exop,
                              status_t *res)
{
    xpath_resnode_t         *resnode;
    xpath_result_t          *tempval;
    val_value_t             *testval, *newval;
    xmlChar                 *cmpstring;
    ncx_num_t                cmpnum;
    int32                    cmpresult;
    boolean                  bool1, bool2, fnresult;
    status_t                 myres;

    *res = NO_ERR;
    myres = NO_ERR;

    /* only compare real results, not objects */
    if (!pcb->val) {
        return TRUE;
    }

    if (val2->restype == XP_RT_NODESET) {
        /* invert the exop; use val2 as val1 */
        switch (exop) {
        case XP_EXOP_EQUAL:
        case XP_EXOP_NOTEQUAL:
            break;
        case XP_EXOP_LT:
            exop = XP_EXOP_GT;
            break;
        case XP_EXOP_GT:
            exop = XP_EXOP_LT;
            break;
        case XP_EXOP_LEQUAL:
            exop = XP_EXOP_GEQUAL;
            break;
        case XP_EXOP_GEQUAL:
            exop = XP_EXOP_LEQUAL;
            break;
        default:
            *res = SET_ERROR(ERR_INTERNAL_VAL);
            return TRUE;
        }

        /* swap the parameters so the parmset is on the LHS */
        tempval = val1;
        val1 = val2;
        val2 = tempval;
    }

    if (dlq_empty(&val1->r.nodeQ)) {
        return FALSE;
    }

    if (val2->restype == XP_RT_BOOLEAN) {
        bool1 = xpath_cvt_boolean(val1);
        bool2 = val2->r.boo;
        return compare_booleans(bool1, bool2, exop);
    }

    /* compare the LHS node-set to the cmpstring or cmpnum
     * first match will end the loop
     */
    fnresult = FALSE;
    for (resnode = (xpath_resnode_t *)dlq_firstEntry(&val1->r.nodeQ);
         resnode != NULL && !fnresult && *res == NO_ERR;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        testval = resnode->node.valptr;
        newval = NULL;
        myres = NO_ERR;

        if (val_is_virtual(testval)) {
            newval = val_get_virtual_value(NULL, testval, &myres);
            if (newval == NULL) {
                *res = myres;
                continue;
            } else {
                testval = newval;
            }
        }

        switch (val2->restype) {
        case XP_RT_STRING:
            cmpstring = NULL;
            *res = xpath1_stringify_node(pcb,
                                         testval,
                                         &cmpstring);
            if (*res == NO_ERR) {
                fnresult = compare_strings(cmpstring,
                                           val2->r.str,
                                           exop);
            }
            if (cmpstring) {
                m__free(cmpstring);
            }
            break;
        case XP_RT_NUMBER:
            ncx_init_num(&cmpnum);
            if (typ_is_number(testval->btyp)) {
                myres = ncx_cast_num(&testval->v.num,
                                     testval->btyp,
                                     &cmpnum,
                                     NCX_BT_FLOAT64);
                if (myres != NO_ERR) {
                    ncx_set_num_nan(&cmpnum, NCX_BT_FLOAT64);
                }
            } else if (testval->btyp == NCX_BT_STRING) {
                myres = ncx_convert_num(VAL_STR(testval),
                                        NCX_NF_NONE,
                                        NCX_BT_FLOAT64,
                                        &cmpnum);
                if (myres != NO_ERR) {
                    ncx_set_num_nan(&cmpnum, NCX_BT_FLOAT64);
                }
            } else {
                ncx_set_num_nan(&cmpnum, NCX_BT_FLOAT64);
            }

            cmpresult = ncx_compare_nums(&cmpnum, 
                                         &val2->r.num, 
                                         NCX_BT_FLOAT64);

            fnresult = convert_compare_result(cmpresult, exop);

            ncx_clean_num(NCX_BT_FLOAT64, &cmpnum);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    return fnresult;

} /* compare_nodeset_to_other */


/********************************************************************
* FUNCTION compare_nodesets
* 
* Compare 2 nodeset results
*
* INPUTS:
*    pcb == parser control block to use
*    val1 == first result struct to compare
*    val2 == second result struct to compare
*    exop == XPath expression op to use
*    res == address of resturn status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*    TRUE if relation is TRUE
     FALSE if relation is FALSE or some error (check *res)
*********************************************************************/
static boolean
    compare_nodesets (xpath_pcb_t *pcb,
                      xpath_result_t *val1,
                      xpath_result_t *val2,
                      xpath_exop_t exop,
                      status_t *res)
{
    xpath_resnode_t         *resnode;
    val_value_t             *testval;
    boolean                  fnresult, cfgonly;
    xpath_compwalkerparms_t  walkerparms;

    *res = NO_ERR;
    if (!pcb->val) {
        return FALSE;
    }

    if ((val1->restype != val2->restype) ||
        (val1->restype != XP_RT_NODESET)) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    /* make sure both node sets are non-empty */
    if (dlq_empty(&val1->r.nodeQ) || dlq_empty(&val2->r.nodeQ)) {
        /* cannot be a matching node in both node-sets
         * if 1 or both node-sets are empty
         */
        return FALSE;
    }

    /* both node-sets have at least 1 node */
    cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;

    walkerparms.result2 = val2;
    walkerparms.cmpstring = NULL;
    walkerparms.cmpnum = NULL;
    walkerparms.buffer = m__getMem(TEMP_BUFFSIZE);
    if (!walkerparms.buffer) {
        *res = ERR_INTERNAL_MEM;
        return FALSE;
    }
    walkerparms.buffsize = TEMP_BUFFSIZE;
    walkerparms.exop = exop;
    walkerparms.cmpresult = FALSE;
    walkerparms.res = NO_ERR;

    /* go through all the nodes in the first node-set
     * and compare each leaf against all the leafs
     * in the other node-set; stop when condition
     * is met or both node-sets completely searched
     */
    for (resnode = (xpath_resnode_t *)dlq_firstEntry(&val1->r.nodeQ);
         resnode != NULL && *res == NO_ERR;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        testval = resnode->node.valptr;
        
        fnresult = 
            val_find_all_descendants(top_compare_walker_fn,
                                     pcb, 
                                     &walkerparms,
                                     testval, 
                                     NULL,
                                     NULL,
                                     cfgonly,
                                     FALSE,
                                     TRUE,
                                     TRUE);
        if (walkerparms.res != NO_ERR) {
            *res = walkerparms.res;
        } else if (!fnresult) {
            /* condition was met if return FALSE and
             * walkerparms.res == NO_ERR
             */
            m__free(walkerparms.buffer);
            return TRUE;
        }
    }

    m__free(walkerparms.buffer);

    return FALSE;

} /* compare_nodesets */


/********************************************************************
* FUNCTION compare_results
* 
* Compare 2 results, using the specified logic operator
*
* INPUTS:
*    pcb == parser control block to use
*    val1 == first result struct to compare
*    val2 == second result struct to compare
*    exop == XPath exression operator to use
*    res == address of resturn status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*     relation result (TRUE or FALSE)
*********************************************************************/
static boolean
    compare_results (xpath_pcb_t *pcb,
                     xpath_result_t *val1,
                     xpath_result_t *val2,
                     xpath_exop_t    exop,
                     status_t *res)
{
    xmlChar         *str1, *str2;
    ncx_num_t        num1, num2;
    boolean          retval, bool1, bool2;
    int32            cmpval;

    *res = NO_ERR;

    /* only compare real results, not objects */
    if (!pcb->val) {
        return TRUE;
    }

    cmpval = 0;

    /* compare directly if the vals are the same result type */
    if (val1->restype == val2->restype) {
        switch (val1->restype) {
        case XP_RT_NODESET:
            return compare_nodesets(pcb, val1, val2, 
                                    exop, res);
        case XP_RT_NUMBER:
            cmpval = ncx_compare_nums(&val1->r.num,
                                      &val2->r.num,
                                      NCX_BT_FLOAT64);
            return convert_compare_result(cmpval, exop);
        case XP_RT_STRING:
            return compare_strings(val1->r.str, 
                                   val2->r.str,
                                   exop);
        case XP_RT_BOOLEAN:
            return compare_booleans(val1->r.boo,
                                    val2->r.boo,
                                    exop);
            break;
        default:
            *res = SET_ERROR(ERR_INTERNAL_VAL);
            return TRUE;
        }
        /*NOTREACHED*/
    }

    /* if 1 nodeset is involved, then compare each node
     * to the other value until the relation is TRUE
     */
    if (val1->restype == XP_RT_NODESET ||
        val2->restype == XP_RT_NODESET) {
        return compare_nodeset_to_other(pcb, 
                                        val1, 
                                        val2, 
                                        exop, 
                                        res);
    }

    /* no nodesets involved, so the specific exop matters */
    if (exop == XP_EXOP_EQUAL || exop == XP_EXOP_NOTEQUAL) {
        /* no nodesets involved
         * not the same result types, so figure out the
         * correct comparision to make.  Priority defined
         * by XPath is
         *
         *  1) boolean
         *  2) number
         *  3) string
         */
        if (val1->restype == XP_RT_BOOLEAN) {
            bool2 = xpath_cvt_boolean(val2);
            return compare_booleans(val1->r.boo, bool2, exop);
        }  else if (val2->restype == XP_RT_BOOLEAN) {
            bool1 = xpath_cvt_boolean(val1);
            return compare_booleans(bool1, val2->r.boo, exop);
        } else if (val1->restype == XP_RT_NUMBER) {
            ncx_init_num(&num2);
            xpath_cvt_number(val2, &num2);
            cmpval = ncx_compare_nums(&val1->r.num, &num2,
                                      NCX_BT_FLOAT64);
            ncx_clean_num(NCX_BT_FLOAT64, &num2);
            return convert_compare_result(cmpval, exop);
        } else if (val2->restype == XP_RT_NUMBER) {
            ncx_init_num(&num1);
            xpath_cvt_number(val1, &num1);
            cmpval = ncx_compare_nums(&num1, &val2->r.num,
                                      NCX_BT_FLOAT64);
            ncx_clean_num(NCX_BT_FLOAT64, &num1);
            return convert_compare_result(cmpval, exop);
        } else if (val1->restype == XP_RT_STRING) {
            str2 = NULL;
            *res = xpath_cvt_string(pcb, val2, &str2);
            if (*res == NO_ERR) {
                cmpval = xml_strcmp(val1->r.str, str2);
            }
            if (str2) {
                m__free(str2);
            }
            if (*res == NO_ERR) {
                return convert_compare_result(cmpval, exop);
            } else {
                return TRUE;
            }
        } else if (val2->restype == XP_RT_STRING) {
            str1 = NULL;
            *res = xpath_cvt_string(pcb, val1, &str1);
            if (*res == NO_ERR) {
                cmpval = xml_strcmp(str1, val2->r.str);
            }
            if (str1) {
                m__free(str1);
            }
            if (*res == NO_ERR) {
                return convert_compare_result(cmpval, exop);
            } else {
                return TRUE;
            }
        } else {
            *res = ERR_NCX_INVALID_VALUE;
            return TRUE;
        }
    }

    /* no nodesets involved
     * expression op is a relational operator
     * not the same result types, so convert to numbers and compare
     */
    ncx_init_num(&num1);
    ncx_init_num(&num2);

    xpath_cvt_number(val1, &num1);
    xpath_cvt_number(val2, &num2);

    cmpval = ncx_compare_nums(&num1,&num2, NCX_BT_FLOAT64);
    retval = convert_compare_result(cmpval, exop);

    ncx_clean_num(NCX_BT_FLOAT64, &num1);
    ncx_clean_num(NCX_BT_FLOAT64, &num2);

    return retval;

} /* compare_results */


/********************************************************************
* FUNCTION dump_result
* 
* Generate log output displaying the contents of a result
*
* INPUTS:
*    pcb == parser control block to use
*    result == result to dump
*    banner == optional first banner string to use
*********************************************************************/
static void
    dump_result (xpath_pcb_t *pcb,
                 xpath_result_t *result,
                 const char *banner)
{
    xpath_resnode_t       *resnode;
    val_value_t           *val;
    const obj_template_t  *obj;

    if (banner) {
        log_write("\n%s", banner);
    }
    if (pcb->tkerr.mod) {
        log_write(" mod:%s, line:%u", 
                  ncx_get_modname(pcb->tkerr.mod),
                  pcb->tkerr.linenum);
    }
    log_write("\nxpath result for '%s'", pcb->exprstr);

    switch (result->restype) {
    case XP_RT_NONE:
        log_write("\n  typ: none");
        break;
    case XP_RT_NODESET:
        log_write("\n  typ: nodeset = ");
        for (resnode = (xpath_resnode_t *)
                 dlq_firstEntry(&result->r.nodeQ);
             resnode != NULL;
             resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

            log_write("\n   node ");
            if (result->isval) {
                val = resnode->node.valptr;
                if (val) {
                    log_write("%s:%s [L:%u]", 
                              xmlns_get_ns_prefix(val->nsid),
                              val->name,
                              val_get_nest_level(val));
                } else {
                    log_write("NULL");
                }
            } else {
                obj = resnode->node.objptr;
                if (obj) {
                    log_write("%s:%s [L:%u]", 
                              obj_get_mod_prefix(obj),
                              obj_get_name(obj),
                              obj_get_level(obj));
                } else {
                    log_write("NULL");
                }
            }
        }
        break;
    case XP_RT_NUMBER:
        if (result->isval) {
            log_write("\n  typ: number = ");
            ncx_printf_num(&result->r.num, NCX_BT_FLOAT64);
        } else {
            log_write("\n  typ: number");
        }
        break;
    case XP_RT_STRING:
        if (result->isval) {
            log_write("\n  typ: string = %s", result->r.str);
        } else {
            log_write("\n  typ: string", result->r.str);
        }
        break;
    case XP_RT_BOOLEAN:
        if (result->isval) {
            log_write("\n  typ: boolean = %s",
                      (result->r.boo) ? "true" : "false");
        } else {
            log_write("\n  typ: boolean");
        }
        break;
    default:
        log_write("\n  typ: INVALID (%d)", result->restype);
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    log_write("\n");

}  /* dump_result */


/********************************************************************
* FUNCTION get_axis_id
* 
* Check a string token tfor a match of an AxisName
*
* INPUTS:
*    name == name string to match
*
* RETURNS:
*   enum of axis name or XP_AX_NONE (0) if not an axis name
*********************************************************************/
static ncx_xpath_axis_t
    get_axis_id (const xmlChar *name)
{
    if (!name || !*name) {
        return XP_AX_NONE;
    }

    if (!xml_strcmp(name, XP_AXIS_ANCESTOR)) {
        return XP_AX_ANCESTOR;
    }
    if (!xml_strcmp(name, XP_AXIS_ANCESTOR_OR_SELF)) {
        return XP_AX_ANCESTOR_OR_SELF;
    }
    if (!xml_strcmp(name, XP_AXIS_ATTRIBUTE)) {
        return XP_AX_ATTRIBUTE;
    }
    if (!xml_strcmp(name, XP_AXIS_CHILD)) {
        return XP_AX_CHILD;
    }
    if (!xml_strcmp(name, XP_AXIS_DESCENDANT)) {
        return XP_AX_DESCENDANT;
    }
    if (!xml_strcmp(name, XP_AXIS_DESCENDANT_OR_SELF)) {
        return XP_AX_DESCENDANT_OR_SELF;
    }
    if (!xml_strcmp(name, XP_AXIS_FOLLOWING)) {
        return XP_AX_FOLLOWING;
    }
    if (!xml_strcmp(name, XP_AXIS_FOLLOWING_SIBLING)) {
        return XP_AX_FOLLOWING_SIBLING;
    }
    if (!xml_strcmp(name, XP_AXIS_NAMESPACE)) {
        return XP_AX_NAMESPACE;
    }
    if (!xml_strcmp(name, XP_AXIS_PARENT)) {
        return XP_AX_PARENT;
    }
    if (!xml_strcmp(name, XP_AXIS_PRECEDING)) {
        return XP_AX_PRECEDING;
    }
    if (!xml_strcmp(name, XP_AXIS_PRECEDING_SIBLING)) {
        return XP_AX_PRECEDING_SIBLING;
    }
    if (!xml_strcmp(name, XP_AXIS_SELF)) {
        return XP_AX_SELF;
    }
    return XP_AX_NONE;

} /* get_axis_id */


/********************************************************************
* FUNCTION get_nodetype_id
* 
* Check a string token tfor a match of a NodeType
*
* INPUTS:
*    name == name string to match
*
* RETURNS:
*   enum of node type or XP_EXNT_NONE (0) if not a node type name
*********************************************************************/
static xpath_nodetype_t
    get_nodetype_id (const xmlChar *name)
{
    if (!name || !*name) {
        return XP_EXNT_NONE;
    }

    if (!xml_strcmp(name, XP_NT_COMMENT)) {
        return XP_EXNT_COMMENT;
    }
    if (!xml_strcmp(name, XP_NT_TEXT)) {
        return XP_EXNT_TEXT;
    }
    if (!xml_strcmp(name, XP_NT_PROCESSING_INSTRUCTION)) {
        return XP_EXNT_PROC_INST;
    }
    if (!xml_strcmp(name, XP_NT_NODE)) {
        return XP_EXNT_NODE;
    }
    return XP_EXNT_NONE;

} /* get_nodetype_id */


/********************************************************************
* FUNCTION location_path_end
* 
* Check if the current location path is ending
* by checking if the next token is going to start some
* sub-expression (or end the expression)
*
* INPUTS:
*    pcb == parser control block to check
*
* RETURNS:
*   TRUE if the location path is ended
*   FALSE if the next token is part of the continuing
*   location path
*********************************************************************/
static boolean
    location_path_end (xpath_pcb_t *pcb)
{
    tk_type_t nexttyp;

    /* check corner-case path '/' */
    nexttyp = tk_next_typ(pcb->tkc);
    if (nexttyp == TK_TT_NONE ||
        nexttyp == TK_TT_EQUAL ||
        nexttyp == TK_TT_BAR ||
        nexttyp == TK_TT_PLUS ||
        nexttyp == TK_TT_MINUS ||
        nexttyp == TK_TT_LT ||
        nexttyp == TK_TT_GT ||
        nexttyp == TK_TT_NOTEQUAL ||
        nexttyp == TK_TT_LEQUAL ||
        nexttyp == TK_TT_GEQUAL) {

        return TRUE;
    } else {
        return FALSE;
    }

} /* location_path_end */


/********************************************************************
* FUNCTION stringify_walker_fn
* 
* Stringify the current node in a nodeset
*
* Matches val_walker_fn_t template in val.h
*
* INPUTS:
*    val == value node found to get the string value for
*    cookie1 == xpath_pcb_t * : parser control block to use
*               currently not used!!!
*    cookie2 == xpath_stringwalkerparms_t *: walker parms to use
*                buffer is filled unless NULL, then len is
*                calculated
* OUTPUTS:
*    *cookie2 contents adjusted 
*
* RETURNS:
*    TRUE to keep walk going
*    FALSE to terminate walk
*********************************************************************/
static boolean
    stringify_walker_fn (val_value_t *val,
                         void *cookie1,
                         void *cookie2)
{
    xpath_stringwalkerparms_t  *parms;
    val_value_t                *newval, *useval;
    status_t                    res;
    uint32                      cnt;

    (void)cookie1;
    parms = (xpath_stringwalkerparms_t *)cookie2;

    /* skip all complex nodes */
    if (!typ_is_simple(val->btyp)) {
        if (parms->buffer) {
            if (parms->buffpos+1 < parms->buffsize) {
                parms->buffer[parms->buffpos++] = '\n';
                parms->buffer[parms->buffpos] = '\0';           
            } else {
                parms->res = ERR_BUFF_OVFL;
                return FALSE;
            }
        } else {
            parms->buffpos++;
        }
        return TRUE;
    }

    newval = NULL;
    res = NO_ERR;

    if (val_is_virtual(val)) {
        newval = val_get_virtual_value(NULL, val, &res);
        if (newval == NULL) {
            parms->res = res;
            return FALSE;
        } else {
            useval = newval;
        }
    } else {
        useval = val;
    }
        
    if (typ_is_string(useval->btyp)) {
        cnt = xml_strlen(VAL_STR(useval));
        if (parms->buffer) {
            if (parms->buffpos+cnt+2 < parms->buffsize) {
                parms->buffer[parms->buffpos++] = '\n';
                xml_strcpy(&parms->buffer[parms->buffpos],
                           VAL_STR(useval));
            } else {
                parms->res = ERR_BUFF_OVFL;
                return FALSE;
            }
            parms->buffpos += cnt;
        } else {
            parms->buffpos += (cnt+1); 
        }
    } else {
        /* get value sprintf size */
        res = val_sprintf_simval_nc(NULL, useval, &cnt);
        if (res != NO_ERR) {
            parms->res = res;
            return FALSE;
        }
        if (parms->buffer) {
            if (parms->buffpos+cnt+2 < parms->buffsize) {
                parms->buffer[parms->buffpos++] = '\n';
                res = val_sprintf_simval_nc
                    (&parms->buffer[parms->buffpos], useval, &cnt);
                if (res != NO_ERR) {
                    parms->res = res;
                    return FALSE;
                }
            } else {
                parms->res = ERR_BUFF_OVFL;
                return FALSE;
            }
            parms->buffpos += cnt;
        } else {
            parms->buffpos += (cnt+1);
        }
    }

    return TRUE;

}  /* stringify_walker_fn */


/**********   X P A T H    F U N C T I O N S   ************/


/********************************************************************
* FUNCTION boolean_fn
* 
* boolean boolean(object) function [4.3]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 object to convert to boolean
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    boolean_fn (xpath_pcb_t *pcb,
                dlq_hdr_t *parmQ,
                status_t  *res)
{
    xpath_result_t  *parm, *result;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (!parm) {
        /* should already be reported in obj mode */
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    result->r.boo = xpath_cvt_boolean(parm);
    *res = NO_ERR;
    return result;

}  /* boolean_fn */


/********************************************************************
* FUNCTION ceiling_fn
* 
* number ceiling(number) function [4.4]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 number to convert to ceiling(number)
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    ceiling_fn (xpath_pcb_t *pcb,
                dlq_hdr_t *parmQ,
                status_t  *res)
{
    xpath_result_t  *parm, *result;
    ncx_num_t        num;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (!parm) {
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (parm->restype == XP_RT_NUMBER) {
        *res = ncx_num_ceiling(&parm->r.num,
                               &result->r.num,
                               NCX_BT_FLOAT64);
                           
    } else {
        ncx_init_num(&num);
        xpath_cvt_number(parm, &num);
        *res = ncx_num_ceiling(&num,
                               &result->r.num,
                               NCX_BT_FLOAT64);
        ncx_clean_num(NCX_BT_FLOAT64, &num);
    }

    if (*res != NO_ERR) {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* ceiling_fn */


/********************************************************************
* FUNCTION concat_fn
* 
* string concat(string, string, string*) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 or more strings to concatenate
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    concat_fn (xpath_pcb_t *pcb,
               dlq_hdr_t *parmQ,
               status_t  *res)
{
    xpath_result_t  *parm, *result;
    xmlChar         *str, *returnstr;
    uint32           parmcnt, len;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;
    len = 0;

    /* check at least 2 strings to concat */
    parmcnt = dlq_count(parmQ);
    if (parmcnt < 2) {
        *res = ERR_NCX_MISSING_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    /* check all parms are strings and get total len */
    for (parm = (xpath_result_t *)dlq_firstEntry(parmQ);
         parm != NULL && *res == NO_ERR;
         parm = (xpath_result_t *)dlq_nextEntry(parm)) {
        if (parm->restype != XP_RT_STRING) {
            *res = xpath_cvt_string(pcb, parm, &str);
            if (*res == NO_ERR) {
                len += xml_strlen(str);
                m__free(str);
            }
        } else if (parm->r.str) {
            len += xml_strlen(parm->r.str);
        }
    }

    if (*res != NO_ERR) {
        return NULL;
    }

    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    result->r.str = m__getMem(len+1);
    if (!result->r.str) {
        free_result(pcb, result);
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    returnstr = result->r.str;

    for (parm = (xpath_result_t *)dlq_firstEntry(parmQ);
         parm != NULL && *res == NO_ERR;
         parm = (xpath_result_t *)dlq_nextEntry(parm)) {

        if (parm->restype != XP_RT_STRING) {
            *res = xpath_cvt_string(pcb, parm, &str);
            if (*res == NO_ERR) {
                returnstr += xml_strcpy(returnstr, str);
                m__free(str);
            }
        } else if (parm->r.str) {
            returnstr += xml_strcpy(returnstr, parm->r.str);
        }
    }

    if (*res != NO_ERR) {
        xpath_free_result(result);
        result = NULL;
    }

    return result;

}  /* concat_fn */


/********************************************************************
* FUNCTION contains_fn
* 
* boolean contains(string, string) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 strings
*             returns true if the 1st string contains the 2nd string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    contains_fn (xpath_pcb_t *pcb,
                 dlq_hdr_t *parmQ,
                 status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *result;
    xmlChar         *str1, *str2;
    boolean          malloc1, malloc2;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;
    str1 = NULL;
    str2 = NULL;
    malloc1 = FALSE;
    malloc2 = FALSE;

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &str1);
        malloc1 = TRUE;
    } else {
        str1 = parm1->r.str;
    }
    if (*res != NO_ERR) {
        return NULL;
    }

    if (parm2->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm2, &str2);
        malloc2 = TRUE;
    } else {
        str2 = parm2->r.str;
    }
    if (*res != NO_ERR) {
        if (malloc1) {
            m__free(str1);
        }
        return NULL;
    }

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        if (strstr((const char *)str1, 
                   (const char *)str2)) {
            result->r.boo = TRUE;
        } else {
            result->r.boo = FALSE;
        }
        *res = NO_ERR;
    }

    if (malloc1) {
        m__free(str1);
    }

    if (malloc2) {
        m__free(str2);
    }

    return result;

}  /* contains_fn */


/********************************************************************
* FUNCTION count_fn
* 
* number count(nodeset) function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 parm (nodeset to get node count for)
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    count_fn (xpath_pcb_t *pcb,
              dlq_hdr_t *parmQ,
              status_t  *res)
{
    xpath_result_t *parm, *result;
    ncx_num_t       tempnum;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }
    parm = (xpath_result_t *)dlq_firstEntry(parmQ);

    *res = NO_ERR;
    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        if (parm->restype != XP_RT_NODESET) {
            ncx_set_num_zero(&result->r.num, NCX_BT_FLOAT64);
        } else {
            if (pcb->val) {
                ncx_init_num(&tempnum);
                tempnum.u = dlq_count(&parm->r.nodeQ);
                *res = ncx_cast_num(&tempnum, 
                                    NCX_BT_UINT32,
                                    &result->r.num,
                                    NCX_BT_FLOAT64);
                ncx_clean_num(NCX_BT_UINT32, &tempnum);
            } else {
                ncx_set_num_zero(&result->r.num, NCX_BT_FLOAT64);
            }

            if (*res != NO_ERR) {
                free_result(pcb, result);
                result = NULL;
            }
        }
    }
    return result;

}  /* count_fn */


/********************************************************************
* FUNCTION current_fn
* 
* number current() function [XPATH 2.0 used in YANG]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == empty parmQ
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    current_fn (xpath_pcb_t *pcb,
                dlq_hdr_t *parmQ,
                status_t  *res)
{
    xpath_result_t *result;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;

    result = new_nodeset(pcb, 
                         pcb->orig_context.node.objptr,
                         pcb->orig_context.node.valptr,
                         1, 
                         FALSE);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        *res = NO_ERR;
    }

    return result;

}  /* current_fn */


/********************************************************************
* FUNCTION false_fn
* 
* boolean false() function [4.3]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == empty parmQ
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    false_fn (xpath_pcb_t *pcb,
              dlq_hdr_t *parmQ,
              status_t  *res)
{
    xpath_result_t *result;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        result->r.boo = FALSE;
        *res = NO_ERR;
    }

    return result;

}  /* false_fn */


/********************************************************************
* FUNCTION floor_fn
* 
* number floor(number) function [4.4]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 number to convert to floor(number)
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    floor_fn (xpath_pcb_t *pcb,
              dlq_hdr_t *parmQ,
              status_t  *res)
{
    xpath_result_t  *parm, *result;
    ncx_num_t        num;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (!parm) {
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (parm->restype == XP_RT_NUMBER) {
        *res = ncx_num_floor(&parm->r.num,
                             &result->r.num,
                             NCX_BT_FLOAT64);
    } else {
        ncx_init_num(&num);
        xpath_cvt_number(parm, &num);
        *res = ncx_num_floor(&num,
                             &result->r.num,
                             NCX_BT_FLOAT64);
        ncx_clean_num(NCX_BT_FLOAT64, &num);
    }   
                           
    if (*res != NO_ERR) {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* floor_fn */


/********************************************************************
* FUNCTION id_fn
* 
* nodeset id(object) function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 parm, which is the object to match
*             against the current result in progress
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    id_fn (xpath_pcb_t *pcb,
           dlq_hdr_t *parmQ,
           status_t  *res)
{
    xpath_result_t  *result;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;  /*****/

    result = new_result(pcb, XP_RT_NODESET);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        *res = NO_ERR;
    }

    /**** get all nodes with the specified ID ****/
    /**** No IDs in YANG so return empty nodeset ****/
    /**** Change this later if there are standard IDs ****/

    return result;

}  /* id_fn */


/********************************************************************
* FUNCTION lang_fn
* 
* boolean lang(string) function [4.3]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 parm; lang string to match
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    lang_fn (xpath_pcb_t *pcb,
             dlq_hdr_t *parmQ,
             status_t  *res)
{
    xpath_result_t  *result;

    (void)parmQ;

    /* no attributes in YANG, so no lang attr either */
    result = new_result(pcb, XP_RT_NODESET);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        *res = NO_ERR;
    }
    return result;

}  /* lang_fn */


/********************************************************************
* FUNCTION last_fn
* 
* number last() function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == empty parmQ
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    last_fn (xpath_pcb_t *pcb,
             dlq_hdr_t *parmQ,
             status_t  *res)
{
    xpath_result_t *result;
    ncx_num_t       tempnum;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;
    *res = NO_ERR;
    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        ncx_init_num(&tempnum);
        tempnum.l = pcb->context.last;
        *res = ncx_cast_num(&tempnum, 
                            NCX_BT_INT64,
                            &result->r.num,
                            NCX_BT_FLOAT64);

        ncx_clean_num(NCX_BT_INT64, &tempnum);

        if (*res != NO_ERR) {
            free_result(pcb, result);
            result = NULL;
        }
    }

    return result;

}  /* last_fn */


/********************************************************************
* FUNCTION local_name_fn
* 
* string local-name(nodeset?) function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional node-set parm 
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    local_name_fn (xpath_pcb_t *pcb,
                   dlq_hdr_t *parmQ,
                   status_t  *res)
{
    xpath_result_t  *parm, *result;
    xpath_resnode_t *resnode;
    const xmlChar   *str;
    uint32           parmcnt;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    str = NULL;

    if (parm && parm->restype != XP_RT_NODESET) {
        ;
    } else if (parm) {
        resnode = (xpath_resnode_t *)
            dlq_firstEntry(&parm->r.nodeQ);
        if (resnode) {
            if (pcb->val) {
                str = resnode->node.valptr->name;
            } else {
                str = obj_get_name(resnode->node.objptr);
            }
        }
    } else {
        if (pcb->val) {
            str = pcb->context.node.valptr->name;
        } else {
            str = obj_get_name(pcb->context.node.objptr);
        }
    }
            
    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    
    if (str) {
        result->r.str = xml_strdup(str);
    } else {
        result->r.str = xml_strdup(EMPTY_STRING);
    }

    if (!result->r.str) {
        *res = ERR_INTERNAL_MEM;
        free_result(pcb, result);
        return NULL;
    }

    return result;

}  /* local_name_fn */


/********************************************************************
* FUNCTION namespace_uri_fn
* 
* string namespace-uri(nodeset?) function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional node-set parm 
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    namespace_uri_fn (xpath_pcb_t *pcb,
                      dlq_hdr_t *parmQ,
                      status_t  *res)
{
    xpath_result_t  *parm, *result;
    xpath_resnode_t *resnode;
    const xmlChar   *str;
    uint32           parmcnt;
    xmlns_id_t       nsid;


    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    nsid = 0;
    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm && parm->restype != XP_RT_NODESET) {
        ;
    } else if (parm) {
        resnode = (xpath_resnode_t *)
            dlq_firstEntry(&parm->r.nodeQ);
        if (resnode) {
            if (pcb->val) {
                nsid = obj_get_nsid(resnode->node.valptr->obj);
            } else {
                nsid = obj_get_nsid(resnode->node.objptr);
            }
        }
    } else {
        if (pcb->val) {
            nsid = obj_get_nsid(pcb->context.node.valptr->obj);
        } else {
            nsid = obj_get_nsid(pcb->context.node.objptr);
        }
    }
                
    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    
    if (nsid) {
        str = xmlns_get_ns_name(nsid);
        result->r.str = xml_strdup(str);
    } else {
        result->r.str = xml_strdup(EMPTY_STRING);
    }

    if (!result->r.str) {
        *res = ERR_INTERNAL_MEM;
        free_result(pcb, result);
        return NULL;
    }

    return result;

}  /* namespace_uri_fn */


/********************************************************************
* FUNCTION name_fn
* 
* string name(nodeset?) function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional node-set parm 
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    name_fn (xpath_pcb_t *pcb,
             dlq_hdr_t *parmQ,
             status_t  *res)
{
    xpath_result_t  *parm, *result;
    xpath_resnode_t *resnode;
    const xmlChar   *prefix, *name;
    xmlChar         *str;
    uint32           len, parmcnt;
    xmlns_id_t       nsid;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    nsid = 0;
    name = NULL;
    prefix = NULL;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm && parm->restype != XP_RT_NODESET) {
        ;
    } else if (parm) {
        resnode = (xpath_resnode_t *)
            dlq_firstEntry(&parm->r.nodeQ);
        if (resnode) {
            if (pcb->val) {
                nsid = obj_get_nsid(resnode->node.valptr->obj);
                name = resnode->node.valptr->name;
            } else {
                nsid = obj_get_nsid(resnode->node.objptr);
                name = obj_get_name(resnode->node.objptr);
            }
        }
    } else {
        if (pcb->val) {
            nsid = obj_get_nsid(pcb->context.node.valptr->obj);
            name = pcb->context.node.valptr->name;
        } else {
            nsid = obj_get_nsid(pcb->context.node.objptr);
            name = obj_get_name(pcb->context.node.objptr);
        }
    }
                
    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    
    if (nsid) {
        prefix = xmlns_get_ns_prefix(nsid);
    }

    len = 0;
    if (prefix) {
        len += xml_strlen(prefix);
        len++;
    }
    if (name) {
        len += xml_strlen(name);
    } else {
        name = EMPTY_STRING;
    }

    result->r.str = m__getMem(len+1);
    if (!result->r.str) {
        *res = ERR_INTERNAL_MEM;
        free_result(pcb, result);
        return NULL;
    }

    str = result->r.str;
    if (prefix) {
        str += xml_strcpy(str, prefix);
        *str++ = ':';
    }
    xml_strcpy(str, name);

    return result;

}  /* name_fn */


/********************************************************************
* FUNCTION normalize_space_fn
* 
* string normalize-space(string?) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional string to convert to normalized
*             string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    normalize_space_fn (xpath_pcb_t *pcb,
                        dlq_hdr_t *parmQ,
                        status_t  *res)
{
    xpath_result_t  *parm, *result, *tempresult;
    const xmlChar   *teststr;
    xmlChar         *mallocstr, *str;
    uint32           parmcnt;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    teststr = NULL;
    str = NULL;
    mallocstr = NULL;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm && parm->restype != XP_RT_STRING) {
        ;
    } else if (parm) {
        teststr = parm->r.str;
    } else {
        tempresult= new_nodeset(pcb,
                                pcb->context.node.objptr,
                                pcb->context.node.valptr,
                                1, 
                                FALSE);

        if (!tempresult) {
            *res = ERR_INTERNAL_MEM;
            return NULL;
        } else {
            *res = xpath_cvt_string(pcb, 
                                    tempresult, 
                                    &mallocstr);
            if (*res == NO_ERR) {
                teststr = mallocstr;
            }
            free_result(pcb, tempresult);
        }
    }
    
    if (*res != NO_ERR) {
        return NULL;
    }

    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        if (mallocstr) {
            m__free(mallocstr);
        }
        return NULL;
    }
    
    if (!teststr) {
        teststr = EMPTY_STRING;
    }

    result->r.str = m__getMem(xml_strlen(teststr)+1);
    if (!result->r.str) {
        *res = ERR_INTERNAL_MEM;
        free_result(pcb, result);
        if (mallocstr) {
            m__free(mallocstr);
        }
        return NULL;
    }

    str = result->r.str;

    while (*teststr && xml_isspace(*teststr)) {
        teststr++;
    }

    while (*teststr) {
        if (xml_isspace(*teststr)) {
            *str++ = ' ';
            teststr++;
            while (*teststr && xml_isspace(*teststr)) {
                teststr++;
            }

        } else {
            *str++ = *teststr++;
        }
    }

    if (str > result->r.str && *(str-1) == ' ') {
        str--;
    }

    *str = 0;

    if (mallocstr) {
        m__free(mallocstr);
    }

    return result;

}  /* normalize_space_fn */


/********************************************************************
* FUNCTION not_fn
* 
* boolean not(object) function [4.3]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 object to perform NOT boolean conversion
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    not_fn (xpath_pcb_t *pcb,
            dlq_hdr_t *parmQ,
            status_t  *res)
{
    xpath_result_t  *parm, *result;
    boolean          boolresult;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm->restype != XP_RT_BOOLEAN) {
        boolresult = xpath_cvt_boolean(parm);
    } else {
        boolresult = parm->r.boo;
    }

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        result->r.boo = !boolresult;
        *res = NO_ERR;
    }

    return result;

}  /* not_fn */


/********************************************************************
* FUNCTION number_fn
* 
* number number(object?) function [4.4]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 optional object to convert to a number
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    number_fn (xpath_pcb_t *pcb,
               dlq_hdr_t *parmQ,
               status_t  *res)
{
    xpath_result_t  *parm, *result, *tempresult;
    uint32           parmcnt;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    *res = NO_ERR;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm) {
        xpath_cvt_number(parm, &result->r.num);
    } else {
        /* convert the context node value */
        if (pcb->val) {
            /* get the value of the context node
             * and convert it to a string first
             */
            tempresult= new_nodeset(pcb,
                                    pcb->context.node.objptr,
                                    pcb->context.node.valptr,
                                    1, 
                                    FALSE);

            if (!tempresult) {
                *res = ERR_INTERNAL_MEM;
            } else {
                xpath_cvt_number(tempresult, &result->r.num);
                free_result(pcb, tempresult);
            }
        } else {
            /* nothing to check; conversion to NaN is
             * not an XPath error
             */
            ncx_set_num_zero(&result->r.num, NCX_BT_FLOAT64);
        }
    }

    if (*res != NO_ERR) {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* number_fn */


/********************************************************************
* FUNCTION position_fn
* 
* number position() function [4.1]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == empty parmQ
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    position_fn (xpath_pcb_t *pcb,
                 dlq_hdr_t *parmQ,
                 status_t  *res)
{
    xpath_result_t *result;
    ncx_num_t       tempnum;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;
    *res = NO_ERR;
    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        ncx_init_num(&tempnum);
        tempnum.l = pcb->context.position;
        *res = ncx_cast_num(&tempnum, 
                            NCX_BT_INT64,
                            &result->r.num,
                            NCX_BT_FLOAT64);

        ncx_clean_num(NCX_BT_INT64, &tempnum);

        if (*res != NO_ERR) {
            free_result(pcb, result);
            result = NULL;
        }
    }

    return result;

}  /* position_fn */


/********************************************************************
* FUNCTION round_fn
* 
* number round(number) function [4.4]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 number to convert to round(number)
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    round_fn (xpath_pcb_t *pcb,
              dlq_hdr_t *parmQ,
              status_t  *res)
{
    xpath_result_t  *parm, *result;
    ncx_num_t        num;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (!parm) {
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (parm->restype == XP_RT_NUMBER) {
        *res = ncx_round_num(&parm->r.num,
                             &result->r.num,
                             NCX_BT_FLOAT64);
    } else {
        ncx_init_num(&num);
        xpath_cvt_number(parm, &num);
        *res = ncx_round_num(&num,
                             &result->r.num,
                             NCX_BT_FLOAT64);
        ncx_clean_num(NCX_BT_FLOAT64, &num);
    }

    if (*res != NO_ERR) {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* round_fn */


/********************************************************************
* FUNCTION starts_with_fn
* 
* boolean starts-with(string, string) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 strings
*             returns true if the 1st string starts with the 2nd string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    starts_with_fn (xpath_pcb_t *pcb,
                    dlq_hdr_t *parmQ,
                    status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *result;
    xmlChar         *str1, *str2;
    uint32           len1, len2;
    boolean          malloc1, malloc2;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;
    str1 = NULL;
    str2 = NULL;
    malloc1 = FALSE;
    malloc2 = FALSE;
    len1 = 0;
    len2= 0;

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &str1);
        malloc1 = TRUE;
    } else {
        str1 = parm1->r.str;
    }
    if (*res != NO_ERR) {
        return NULL;
    }

    if (parm2->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm2, &str2);
        malloc2 = TRUE;
    } else {
        str2 = parm2->r.str;
    }
    if (*res != NO_ERR) {
        if (malloc1) {
            m__free(str1);
        }
        return NULL;
    }

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        len1 = xml_strlen(str1);
        len2 = xml_strlen(str2);

        if (len2 > len1) {
            result->r.boo = FALSE;
        } else if (!xml_strncmp(str1, str2, len2)) {
            result->r.boo = TRUE;
        } else {
            result->r.boo = FALSE;
        }
        *res = NO_ERR;
    }

    if (malloc1) {
        m__free(str1);
    }
    if (malloc2) {
        m__free(str2);
    }

    return result;

}  /* starts_with_fn */


/********************************************************************
* FUNCTION string_fn
* 
* string string(object?) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional object to convert to string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    string_fn (xpath_pcb_t *pcb,
               dlq_hdr_t *parmQ,
               status_t  *res)
{
    xpath_result_t  *parm, *result, *tempresult;
    uint32           parmcnt;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    *res = NO_ERR;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm) {
        xpath_cvt_string(pcb, parm, &result->r.str);
    } else {
        /* convert the context node value */
        if (pcb->val) {
            /* get the value of the context node
             * and convert it to a string first
             */
            tempresult= new_nodeset(pcb,
                                    pcb->context.node.objptr,
                                    pcb->context.node.valptr,
                                    1, 
                                    FALSE);

            if (!tempresult) {
                *res = ERR_INTERNAL_MEM;
            } else {
                *res = xpath_cvt_string(pcb, 
                                        tempresult, 
                                        &result->r.str);
                free_result(pcb, tempresult);
            }
        } else {
            result->r.str = xml_strdup(EMPTY_STRING);
            if (!result->r.str) {
                *res = ERR_INTERNAL_MEM;
            }
        }
    }

    if (*res != NO_ERR) {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* string_fn */


/********************************************************************
* FUNCTION string_length_fn
* 
* number string-length(string?) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with optional string to check length
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    string_length_fn (xpath_pcb_t *pcb,
                      dlq_hdr_t *parmQ,
                      status_t  *res)
{
    xpath_result_t  *parm, *result, *tempresult;
    xmlChar         *tempstr;
    uint32           parmcnt, len;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    parmcnt = dlq_count(parmQ);
    if (parmcnt > 1) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    *res = NO_ERR;
    len = 0;
    tempstr = NULL;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm && parm->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm, &tempstr);
        if (*res == NO_ERR) {
            len = xml_strlen(tempstr);
            m__free(tempstr);
        }
    } else if (parm) {
        len = xml_strlen(parm->r.str);
    } else {
        /* convert the context node value */
        if (pcb->val) {
            /* get the value of the context node
             * and convert it to a string first
             */
            tempresult= new_nodeset(pcb,
                                    pcb->context.node.objptr,
                                    pcb->context.node.valptr,
                                    1, 
                                    FALSE);

            if (!tempresult) {
                *res = ERR_INTERNAL_MEM;
            } else {
                tempstr = NULL;
                *res = xpath_cvt_string(pcb, 
                                        tempresult, 
                                        &tempstr);
                free_result(pcb, tempresult);
                if (*res == NO_ERR) {
                    len = xml_strlen(tempstr);
                    m__free(tempstr);
                }
            }
        }
    }

    if (*res == NO_ERR) {
        set_uint32_num(len, &result->r.num);
    } else {
        free_result(pcb, result);
        result = NULL;
    }

    return result;

}  /* string_length_fn */


/********************************************************************
* FUNCTION substring_fn
* 
* string substring(string, number, number?) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 or 3 parms
*             returns substring of 1st string starting at the
*             position indicated by the first number; copies
*             only N chars if the 2nd number is present
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    substring_fn (xpath_pcb_t *pcb,
                  dlq_hdr_t *parmQ,
                  status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *parm3, *result;
    xmlChar         *str1;
    ncx_num_t        tempnum, num2, num3;
    uint32           parmcnt, maxlen;
    int64            startpos, copylen, slen;
    status_t         myres;
    boolean          copylenset, malloc1;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;

    str1 = NULL;
    malloc1 = FALSE;
    ncx_init_num(&num2);
    ncx_init_num(&num3);

    /* check at least 2 strings to concat */
    parmcnt = dlq_count(parmQ);
    if (parmcnt < 2) {
        *res = ERR_NCX_MISSING_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    } else if (parmcnt > 3) {
        *res = ERR_NCX_EXTRA_PARM;
        wrong_parmcnt_error(pcb, parmcnt, *res);
        return NULL;
    }

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);
    parm3 = (xpath_result_t *)dlq_nextEntry(parm2);

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &str1);
        if (*res != NO_ERR) {
            return NULL;
        } else {
            malloc1 = TRUE;
        }
    } else {
        str1 = parm1->r.str;
    }

    xpath_cvt_number(parm2, &num2);
    if (parm3) {
        xpath_cvt_number(parm3, &num3);
    }

    startpos = 0;
    ncx_init_num(&tempnum);
    myres = ncx_round_num(&num2, &tempnum, NCX_BT_FLOAT64);
    if (myres == NO_ERR) {
        startpos = ncx_cvt_to_int64(&tempnum, NCX_BT_FLOAT64);
    }
    ncx_clean_num(NCX_BT_FLOAT64, &tempnum);

    copylenset = FALSE;
    copylen = 0;
    if (parm3) {
        copylenset = TRUE;
        myres = ncx_round_num(&num3, &tempnum, NCX_BT_FLOAT64);
        if (myres == NO_ERR) {
            copylen = ncx_cvt_to_int64(&tempnum, NCX_BT_FLOAT64);

        }
        ncx_clean_num(NCX_BT_FLOAT64, &tempnum);
    }

    slen = (int64)xml_strlen(str1);

    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (startpos > (slen+1) || (copylenset && copylen <= 0)) {
        /* starting after EOS */
        result->r.str = xml_strdup(EMPTY_STRING);
    } else if (copylenset) {
        if (startpos < 1) {
            /* adjust startpos, then copy N chars */
            copylen += (startpos-1);
            startpos = 1;
        }
        if (copylen >= 1) {
            if (copylen >= NCX_MAX_UINT) {
                /* internal max of strings way less than 4G
                 * so copy the whole string
                 */
                result->r.str = xml_strdup(&str1[startpos-1]);
            } else {
                /* copy at most N chars of whole string */
                maxlen = (uint32)copylen;
                result->r.str = xml_strndup(&str1[startpos-1],
                                            maxlen);
            }
        } else {
            result->r.str = xml_strdup(EMPTY_STRING);
        }
    } else if (startpos <= 1) {
        /* copying whole string */
        result->r.str = xml_strdup(str1);
    } else {
        /* copying rest of string */
        result->r.str = xml_strdup(&str1[startpos-1]);
    }

    if (!result->r.str) {
        *res = ERR_INTERNAL_MEM;
        free_result(pcb, result);
        result = NULL;
    }

    if (malloc1) {
        m__free(str1);
    }
    ncx_clean_num(NCX_BT_FLOAT64, &num2);
    ncx_clean_num(NCX_BT_FLOAT64, &num3);

    return result;

}  /* substring_fn */


/********************************************************************
* FUNCTION substring_after_fn
* 
* string substring-after(string, string) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 strings
*             returns substring of 1st string after
*             the occurance of the 2nd string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    substring_after_fn (xpath_pcb_t *pcb,
                        dlq_hdr_t *parmQ,
                        status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *result;
    const xmlChar   *str, *retstr;
    xmlChar         *str1, *str2;
    boolean          malloc1, malloc2;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    str1 = NULL;
    str2 = NULL;
    malloc1 = FALSE;
    malloc2 = FALSE;

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &str1);
        if (*res != NO_ERR) {
            return NULL;
        }
        malloc1 = TRUE;
    } else {
        str1 = parm1->r.str;
    }

    if (parm2->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm2, &str2);
        if (*res != NO_ERR) {
            if (malloc1) {
                m__free(str1);
            }
            return NULL;
        }
        malloc2 = TRUE;
    } else {
        str2 = parm2->r.str;
    }

    result = new_result(pcb, XP_RT_STRING);
    if (result) {
        str = (const xmlChar *)
            strstr((const char *)str1, 
                   (const char *)str2);
        if (str) {
            retstr = str + xml_strlen(str2);
            result->r.str = m__getMem(xml_strlen(retstr)+1);
            if (result->r.str) {
                xml_strcpy(result->r.str, retstr);
            }
        } else {
            result->r.str = xml_strdup(EMPTY_STRING);
        }

        if (!result->r.str) {
            *res = ERR_INTERNAL_MEM;
        }

        if (*res != NO_ERR) {
            free_result(pcb, result);
            result = NULL;
        }
    } else {
        *res = ERR_INTERNAL_MEM;
    }

    if (malloc1) {
        m__free(str1);
    }
    if (malloc2) {
        m__free(str2);
    }

    return result;

}  /* substring_after_fn */


/********************************************************************
* FUNCTION substring_before_fn
* 
* string substring-before(string, string) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 2 strings
*             returns substring of 1st string that precedes
*             the occurance of the 2nd string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    substring_before_fn (xpath_pcb_t *pcb,
                         dlq_hdr_t *parmQ,
                         status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *result;
    const xmlChar   *str;
    xmlChar         *str1, *str2;
    uint32           retlen;
    boolean          malloc1, malloc2;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    malloc1 = FALSE;
    malloc2 = FALSE;

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &str1);
        if (*res != NO_ERR) {
            return NULL;
        }
        malloc1 = TRUE;
    } else {
        str1 = parm1->r.str;
    }

    if (parm2->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm2, &str2);
        if (*res != NO_ERR) {
            if (malloc1) {
                m__free(str1);
            }
            return NULL;
        }
        malloc2 = TRUE;
    } else {
        str2 = parm2->r.str;
    }

    result = new_result(pcb, XP_RT_STRING);
    if (result) {
        str = (const xmlChar *)
            strstr((const char *)str1, 
                   (const char *)str2);
        if (str) {
            retlen = (uint32)(str - str1);
            result->r.str = m__getMem(retlen+1);
            if (result->r.str) {
                xml_strncpy(result->r.str,
                            str1,
                            retlen);
            }
        } else {
            result->r.str = xml_strdup(EMPTY_STRING);
        }

        if (!result->r.str) {
            *res = ERR_INTERNAL_MEM;
        }

        if (*res != NO_ERR) {
            free_result(pcb, result);
            result = NULL;
        }
    } else {
        *res = ERR_INTERNAL_MEM;
    }

    if (malloc1) {
        m__free(str1);
    }
    if (malloc2) {
        m__free(str2);
    }

    return result;

}  /* substring_before_fn */


/********************************************************************
* FUNCTION sum_fn
* 
* number sum(nodeset) function [4.4]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 nodeset to convert to numbers
*             and add together to resurn the total
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    sum_fn (xpath_pcb_t *pcb,
            dlq_hdr_t *parmQ,
            status_t  *res)
{
    xpath_result_t   *parm, *result;
    xpath_resnode_t  *resnode;
    val_value_t      *val;
    ncx_num_t         tempnum, mysum;
    status_t          myres;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    result = new_result(pcb, XP_RT_NUMBER);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm->restype != XP_RT_NODESET || !pcb->val) {
        ncx_set_num_zero(&result->r.num, NCX_BT_FLOAT64);
    } else {
        ncx_init_num(&tempnum);
        ncx_set_num_zero(&tempnum, NCX_BT_FLOAT64);
        ncx_init_num(&mysum);
        ncx_set_num_zero(&mysum, NCX_BT_FLOAT64);
        myres = NO_ERR;

        for (resnode = (xpath_resnode_t *)
                 dlq_firstEntry(&parm->r.nodeQ);
             resnode != NULL && myres == NO_ERR;
             resnode = (xpath_resnode_t *)
                 dlq_nextEntry(resnode)) {

            val = val_get_first_leaf(resnode->node.valptr);
            if (val && typ_is_number(val->btyp)) {
                myres = ncx_cast_num(&val->v.num,
                                     val->btyp,
                                     &tempnum,
                                     NCX_BT_FLOAT64);
                if (myres == NO_ERR) {
                    mysum.d += tempnum.d;
                }
            } else {
                myres = ERR_NCX_INVALID_VALUE;
            }
        }

        if (myres == NO_ERR) {
            result->r.num.d = mysum.d;
        } else {
            ncx_set_num_nan(&result->r.num, NCX_BT_FLOAT64);
        }
    }

    return result;

}  /* sum_fn */


/********************************************************************
* FUNCTION translate_fn
* 
* string translate(string, string, string) function [4.2]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 3 strings to translate
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    translate_fn (xpath_pcb_t *pcb,
                  dlq_hdr_t *parmQ,
                  status_t  *res)
{
    xpath_result_t  *parm1, *parm2, *parm3, *result;
    xmlChar         *p1str, *p2str, *p3str, *writestr;
    uint32           parmcnt, p1len, p2len, p3len, curpos;
    boolean          done, malloc1, malloc2, malloc3;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    malloc1 = FALSE;
    malloc2 = FALSE;
    malloc3 = FALSE;

    *res = NO_ERR;

    parm1 = (xpath_result_t *)dlq_firstEntry(parmQ);
    parm2 = (xpath_result_t *)dlq_nextEntry(parm1);
    parm3 = (xpath_result_t *)dlq_nextEntry(parm2);

    /* check at least 3 parameters */
    parmcnt = dlq_count(parmQ);
    if (parmcnt < 3)
    {
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    if (parm1->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm1, &p1str);
        if (*res != NO_ERR) {
            return NULL;
        }
        malloc1 = TRUE;
    } else {
        p1str = parm1->r.str;
    }
    p1len = xml_strlen(p1str);
        
    if (parm2->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm2, &p2str);
        if (*res != NO_ERR) {
            if (malloc1) {
                m__free(p1str);
            }
            return NULL;
        }
        malloc2 = TRUE;
    } else {
        p2str = parm2->r.str;
    }
    p2len = xml_strlen(p2str);

    if (parm3->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm3, &p3str);
        if (*res != NO_ERR) {
            if (malloc1) {
                m__free(p1str);
            }
            if (malloc2) {
                m__free(p2str);
            }
            return NULL;
        }
        malloc3 = TRUE;
    } else {
        p3str = parm3->r.str;
    }
    p3len = xml_strlen(p3str);

    result = new_result(pcb, XP_RT_STRING);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        result->r.str = m__getMem(p1len+1);
        if (!result->r.str) {
            *res = ERR_INTERNAL_MEM;
            free_result(pcb, result);
            result = NULL;
        } else {
            writestr = result->r.str;

            /* translate p1str into the result string */
            while (*p1str) {
                curpos = 0;
                done = FALSE;

                /* look for a match char in p2str */
                while (!done && curpos < p2len) {
                    if (p2str[curpos] == *p1str) {
                        done = TRUE;
                    } else {
                        curpos++;
                    }
                }

                if (done) {
                    if (curpos < p3len) {
                        /* replace p1char with p3str translation char */
                        *writestr++ = p3str[curpos];
                    } /* else drop p1char from result */
                } else {
                    /* copy p1char to result */
                    *writestr++ = *p1str;
                }

                p1str++;
            }

            *writestr = 0;
        }
    }

    if (malloc1) {
        m__free(p1str);
    }
    if (malloc2) {
        m__free(p2str);
    }
    if (malloc3) {
        m__free(p3str);
    }

    return result;

}  /* translate_fn */


/********************************************************************
* FUNCTION true_fn
* 
* boolean true() function [4.3]
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == empty parmQ
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    true_fn (xpath_pcb_t *pcb,
             dlq_hdr_t *parmQ,
             status_t  *res)
{
    xpath_result_t *result;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    (void)parmQ;
    result = new_result(pcb, XP_RT_BOOLEAN);
    if (!result) {
        *res = ERR_INTERNAL_MEM;
    } else {
        result->r.boo = TRUE;
        *res = NO_ERR;
    }
    return result;

}  /* true_fn */


/********************************************************************
* FUNCTION module_loaded_fn
* 
* boolean module-loaded(string [,string])
*   parm1 == module name
*   parm2 == optional revision-date
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 object to convert to boolean
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    module_loaded_fn (xpath_pcb_t *pcb,
                      dlq_hdr_t *parmQ,
                      status_t  *res)
{
    xpath_result_t  *parm, *parm2, *result;
    xmlChar         *modstr, *revstr, *str1, *str2;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;

    modstr = NULL;
    revstr = NULL;
    str1 = NULL;
    str2 = NULL;

    if (dlq_count(parmQ) > 2) {
        /* should already be reported in obj mode */
        *res = ERR_NCX_EXTRA_PARM;
        return NULL;
    }

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm == NULL) {
        /* should already be reported in obj mode */
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    parm2 = (xpath_result_t *)dlq_nextEntry(parm);

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (result == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    result->r.boo = FALSE;

    if (parm->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm, &str1);
        if (*res == NO_ERR) {
            modstr = str1;
        }
    } else {
        modstr = parm->r.str;
    }

    if (*res == NO_ERR && parm2 != NULL) {
        if (parm2->restype != XP_RT_STRING) {
            *res = xpath_cvt_string(pcb, parm2, &str2);
            if (*res == NO_ERR) {
                revstr = str2;
            }
        } else {
            revstr = parm2->r.str;
        }
    }

    if (*res == NO_ERR) {
        if (ncx_valid_name2(modstr)) {
            if (ncx_find_module(modstr, revstr)) {
                result->r.boo = TRUE;
            }
        }
    }

    if (str1 != NULL) {
        m__free(str1);
    }

    if (str2 != NULL) {
        m__free(str2);
    }

    return result;

}  /* module_loaded_fn */


/********************************************************************
* FUNCTION feature_enabled_fn
* 
* boolean feature-enabled(string ,string)
*   parm1 == module name
*   parm2 == feature name
*
* INPUTS:
*    pcb == parser control block to use
*    parmQ == parmQ with 1 object to convert to boolean
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    malloced xpath_result_t if no error and results being processed
*    NULL if error
*********************************************************************/
static xpath_result_t *
    feature_enabled_fn (xpath_pcb_t *pcb,
                        dlq_hdr_t *parmQ,
                        status_t  *res)
{
    xpath_result_t  *parm, *parm2, *result;
    xmlChar         *modstr, *featstr, *str1, *str2;
    ncx_module_t    *mod;
    ncx_feature_t   *feat;

    if (!pcb->val && !pcb->obj) {
        return NULL;
    }

    *res = NO_ERR;

    modstr = NULL;
    featstr = NULL;
    str1 = NULL;
    str2 = NULL;

    parm = (xpath_result_t *)dlq_firstEntry(parmQ);
    if (parm == NULL) {
        /* should already be reported in obj mode */
        *res = ERR_NCX_MISSING_PARM;
        return NULL;
    }

    parm2 = (xpath_result_t *)dlq_nextEntry(parm);

    result = new_result(pcb, XP_RT_BOOLEAN);
    if (result == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    result->r.boo = FALSE;

    if (parm->restype != XP_RT_STRING) {
        *res = xpath_cvt_string(pcb, parm, &str1);
        if (*res == NO_ERR) {
            modstr = str1;
        }
    } else {
        modstr = parm->r.str;
    }

    if (*res == NO_ERR) {
        if (parm2->restype != XP_RT_STRING) {
            *res = xpath_cvt_string(pcb, parm2, &str2);
            if (*res == NO_ERR) {
                featstr = str2;
            }
        } else {
            featstr = parm2->r.str;
        }
    }

    if (*res == NO_ERR) {
        if (ncx_valid_name2(modstr)) {
            mod = ncx_find_module(modstr, NULL);
            if (mod != NULL) {
                feat = ncx_find_feature(mod, featstr);
                if (feat != NULL && ncx_feature_enabled(feat)) {
                    result->r.boo = TRUE;
                }
            }
        }
    }

    if (str1 != NULL) {
        m__free(str1);
    }

    if (str2 != NULL) {
        m__free(str2);
    }

    return result;

}  /* feature_enabled_fn */


/****************   U T I L I T Y    F U N C T I O N S   ***********/


/********************************************************************
* FUNCTION find_fncb
* 
* Find an XPath function control block
*
* INPUTS:
*    pcb == parser control block to check
*    name == name string to check
*
* RETURNS:
*   pointer to found control block
*   NULL if not found
*********************************************************************/
static const xpath_fncb_t *
    find_fncb (xpath_pcb_t *pcb,
               const xmlChar *name)
{
    const xpath_fncb_t  *fncb;
    uint32               i;

    i = 0;
    fncb = &pcb->functions[i];
    while (fncb && fncb->name) {
        if (!xml_strcmp(name, fncb->name)) {
            return fncb;
        } else {
            fncb = &pcb->functions[++i];
        }
    }
    return NULL;

} /* find_fncb */


/********************************************************************
* FUNCTION get_varbind
* 
* Get the specified variable binding
*
* INPUTS:
*    pcb == parser control block in progress
*    prefix == prefix string of module with the varbind
*           == NULL for current module (pcb->tkerr.mod)
*    prefixlen == length of prefix string
*    name == variable name string
*     res == address of return status
*
* OUTPUTS:
*   *res == resturn status
*
* RETURNS:
*   pointer to found varbind, NULL if not found
*********************************************************************/
static ncx_var_t *
    get_varbind (xpath_pcb_t *pcb,
                 const xmlChar *prefix,
                 uint32 prefixlen,
                 const xmlChar *name,
                 status_t  *res)
{
    ncx_var_t    *var;

    var = NULL;
    *res = NO_ERR;

    /* 
     * varbinds with prefixes are not supported in NETCONF
     * there is no way to define a varbind in a YANG module
     * so they do not correlate to modules.
     *
     * They are name to value node bindings in yuma
     * and the pcb needs to be pre-configured with
     * a queue of varbinds or a callback function to get the
     * requested variable binding 
     */
    if (prefix && prefixlen) {
        /* no variables with prefixes allowed !!! */
        *res = ERR_NCX_DEF_NOT_FOUND;
    } else {
        /* try to find the variable */
        if (pcb->getvar_fn) {
            var = (*pcb->getvar_fn)(pcb, name, res);
        } else {
            var = var_get_que_raw(&pcb->varbindQ, 0, name);
            if (!var) {
                *res = ERR_NCX_DEF_NOT_FOUND;
            }
        }
    }

    return var;

}  /* get_varbind */


/********************************************************************
* FUNCTION match_next_token
* 
* Match the next token in the chain with a type and possibly value
*
* INPUTS:
*    pcb == parser control block in progress
*    tktyp == token type to match
*    tkval == string val to match (or NULL to just match tktyp)
*
* RETURNS:
*   TRUE if the type and value (if non-NULL) match
*   FALSE if next token is not a match
*********************************************************************/
static boolean
    match_next_token (xpath_pcb_t *pcb,
                      tk_type_t tktyp,
                      const xmlChar *tkval)
{
    const xmlChar  *nextval;
    tk_type_t       nexttyp;

    nexttyp = tk_next_typ(pcb->tkc);
    if (nexttyp == tktyp) {
        if (tkval) {
            nextval = tk_next_val(pcb->tkc);
            if (nextval && !xml_strcmp(tkval, nextval)) {
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            return TRUE;
        }
    } else {
        return FALSE;
    }
    /*NOTREACHED*/

}  /* match_next_token */


/********************************************************************
* FUNCTION check_qname_prefix
* 
* Check the prefix for the current node against the proper context
* and report any unresolved prefix errors
*
* Do not check the local-name part of the QName because
* the module is still being parsed and out of order
* nodes will not be found yet
*
* INPUTS:
*    pcb == parser control block in progress
*    prefix == prefix string to use (may be NULL)
*    prefixlen == length of prefix 
*    nsid == address of return NS ID (may be NULL)
*
* OUTPUTS:
*   if non-NULL:
*    *nsid == namespace ID for the prefix, if found
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_qname_prefix (xpath_pcb_t *pcb,
                        const xmlChar *prefix,
                        uint32 prefixlen,
                        xmlns_id_t *nsid)
{
    ncx_module_t   *targmod;
    status_t        res;

    res = NO_ERR;

    if (pcb->reader) {
        res = xml_get_namespace_id(pcb->reader,
                                   prefix,
                                   prefixlen,
                                   nsid);
    } else {
        res = xpath_get_curmod_from_prefix_str(prefix, 
                                               prefixlen,
                                               pcb->tkerr.mod, 
                                               &targmod);
        if (res == NO_ERR) {
            *nsid = targmod->nsid;
        }
    }

    if (res != NO_ERR) {
        if (pcb->logerrors) {
            log_error("\nError: Module for prefix '%s' not found",
                      (prefix) ? prefix : EMPTY_STRING);
            ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, res);
        }
    }

    return res;
    
}  /* check_qname_prefix */


/********************************************************************
* FUNCTION cvt_from_value
* 
* Convert a val_value_t into an XPath result
*
* INPUTS:
*    pcb == parser control block to use
*    val == value struct to convert
*
* RETURNS:
*   malloced and filled in xpath_result_t or NULL if malloc error
*********************************************************************/
static xpath_result_t *
    cvt_from_value (xpath_pcb_t *pcb,
                    val_value_t *val)
{
    xpath_result_t    *result;
    val_value_t       *newval;
    const val_value_t *useval;
    status_t           res;
    uint32             len;

    result = NULL;
    newval = NULL;
    res = NO_ERR;

    if (val_is_virtual(val)) {
        newval = val_get_virtual_value(NULL, val, &res);
        if (newval == NULL) {
            return NULL;
        } else {
            useval = newval;
        }
    } else {
        useval = val;
    }

    if (typ_is_string(useval->btyp)) {
        result = new_result(pcb, XP_RT_STRING);
        if (result) {
            if (VAL_STR(useval)) {
                result->r.str = xml_strdup(VAL_STR(useval));
            } else {
                result->r.str = xml_strdup(EMPTY_STRING);
            }
            if (!result->r.str) {
                free_result(pcb, result);
                result = NULL;
            }
        }
    } else if (typ_is_number(useval->btyp)) {
        result = new_result(pcb, XP_RT_NUMBER);
        if (result) {
            res = ncx_cast_num(&useval->v.num, 
                               useval->btyp,
                               &result->r.num,
                               NCX_BT_FLOAT64);
            if (res != NO_ERR) {
                free_result(pcb, result);
                result = NULL;
            }
        }
    } else if (typ_is_simple(useval->btyp)) {
        len = 0;
        res = val_sprintf_simval_nc(NULL, useval, &len);
        if (res == NO_ERR) {
            result = new_result(pcb, XP_RT_STRING);
            if (result) {
                result->r.str = m__getMem(len+1);
                if (!result->r.str) {
                    free_result(pcb, result);
                    result = NULL;
                } else {
                    res = val_sprintf_simval_nc(result->r.str, 
                                                useval, 
                                                &len);
                    if (res != NO_ERR) {
                        free_result(pcb, result);
                        result = NULL;
                    }
                }
            }
        }
    }

    return result;

}  /* cvt_from_value */


/********************************************************************
* FUNCTION find_resnode
* 
* Check if the specified resnode ptr is already in the Q
*
* INPUTS:
*    pcb == parser control block to use
*    resultQ == Q of xpath_resnode_t structs to check
*               DOES NOT HAVE TO BE WITHIN A RESULT NODE Q
*    ptr   == pointer value to find
*
* RETURNS:
*    found resnode or NULL if not found
*********************************************************************/
static xpath_resnode_t *
    find_resnode (xpath_pcb_t *pcb,
                  dlq_hdr_t *resultQ,
                  const void *ptr)
{
    xpath_resnode_t  *resnode;

    for (resnode = (xpath_resnode_t *)dlq_firstEntry(resultQ);
         resnode != NULL;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        if (pcb->val) {
            if (resnode->node.valptr == ptr) {
                return resnode;
            }
        } else {
            if (resnode->node.objptr == ptr) {
                return resnode;
            }
        }
    }
    return NULL;

}  /* find_resnode */


/********************************************************************
* FUNCTION find_resnode_slow
* 
* Check if the specified resnode ptr is already in the Q
*
* INPUTS:
*    pcb == parser control block to use
*    resultQ == Q of xpath_resnode_t structs to check
*               DOES NOT HAVE TO BE WITHIN A RESULT NODE Q
*    nsid == namespace ID of node to find; 0 to skip NS test
*    name == local-name of node to find
*
* RETURNS:
*    found resnode or NULL if not found
*********************************************************************/
static xpath_resnode_t *
    find_resnode_slow (xpath_pcb_t *pcb,
                       dlq_hdr_t *resultQ,
                       xmlns_id_t nsid,
                       const xmlChar *name)
{
    xpath_resnode_t  *resnode;

    for (resnode = (xpath_resnode_t *)dlq_firstEntry(resultQ);
         resnode != NULL;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        if (pcb->val) {
            if (nsid && (nsid != val_get_nsid(resnode->node.valptr))) {
                continue;
            }
            if (!xml_strcmp(name, resnode->node.valptr->name)) {
                return resnode;
            }
        } else {
            if (nsid && (nsid != obj_get_nsid(resnode->node.objptr))) {
                continue;
            }
            if (!xml_strcmp(name,
                            obj_get_name(resnode->node.objptr))) {
                return resnode;
            }
        }
    }
    return NULL;

}  /* find_resnode_slow */


/********************************************************************
* FUNCTION merge_nodeset
* 
* Add the nodes from val1 into val2
* that are not already there
*
* INPUTS:
*    pcb == parser control block to use
*    result == address of return XPath result nodeset
*
* OUTPUTS:
*    result->nodeQ contents adjusted
*
*********************************************************************/
static void
    merge_nodeset (xpath_pcb_t *pcb,
                   xpath_result_t *val1,
                   xpath_result_t *val2)
{
    xpath_resnode_t        *resnode, *findnode;

    if (!pcb->val && !pcb->obj) {
        return;
    }

    if (val1->restype != XP_RT_NODESET ||
        val2->restype != XP_RT_NODESET) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    while (!dlq_empty(&val1->r.nodeQ)) {
        resnode = (xpath_resnode_t *)
            dlq_deque(&val1->r.nodeQ);

        if (pcb->val) {
            findnode = find_resnode(pcb, 
                                    &val2->r.nodeQ,
                                    resnode->node.valptr);
        } else {
            findnode = find_resnode(pcb, 
                                    &val2->r.nodeQ,
                                    resnode->node.objptr);
        }
        if (findnode) {
            if (resnode->dblslash) {
                findnode->dblslash = TRUE;
            }
            findnode->position = resnode->position;
            free_resnode(pcb, resnode);
        } else {
            dlq_enque(resnode, &val2->r.nodeQ);
        }
    }

}  /* merge_nodeset */


/********************************************************************
* FUNCTION set_nodeset_dblslash
* 
* Set the current nodes dblslash flag
* to indicate that all unchecked descendants
* are also represented by this node, for
* further node-test or predicate testing
*
* INPUTS:
*    pcb == parser control block to use
*    result == address of return XPath result nodeset
*
* OUTPUTS:
*    result->nodeQ contents adjusted
*
*********************************************************************/
static void
    set_nodeset_dblslash (xpath_pcb_t *pcb,
                          xpath_result_t *result)
{
    xpath_resnode_t        *resnode;

    if (!pcb->val && !pcb->obj) {
        return;
    }

    for (resnode = (xpath_resnode_t *)
             dlq_firstEntry(&result->r.nodeQ);
         resnode != NULL;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        resnode->dblslash = TRUE;
    }

}  /* set_nodeset_dblslash */


/********************************************************************
* FUNCTION value_walker_fn
* 
* Check the current found value node, based on the
* criteria passed to the search function
*
* Matches val_walker_fn_t template in val.h
*
* INPUTS:
*    val == value node found in the search
*    cookie1 == xpath_pcb_t * : parser control block to use
*    cookie2 == xpath_walkerparms_t *: walker parms to use
*
* OUTPUTS:
*    result->nodeQ contents adjusted or replaced
*
* RETURNS:
*    TRUE to keep walk going
*    FALSE to terminate walk
*********************************************************************/
static boolean
    value_walker_fn (val_value_t *val,
                     void *cookie1,
                     void *cookie2)
{
    xpath_pcb_t          *pcb;
    xpath_walkerparms_t  *parms;
    xpath_resnode_t      *newresnode;
    val_value_t          *child;
    int64                 position;
    boolean               done;

    pcb = (xpath_pcb_t *)cookie1;
    parms = (xpath_walkerparms_t *)cookie2;

    /* check if this node is already in the result */
    if (find_resnode(pcb, parms->resnodeQ, val)) {
        return TRUE;
    }

    if (obj_is_root(val->obj) || val->parent==NULL) {
        position = 1;
    } else {
        position = 0;
        done = FALSE;
        for (child = val_get_first_child(val->parent);
             child != NULL && !done;
             child = val_get_next_child(child)) {
            position++;
            if (child == val) {
                done = TRUE;
            }
        }
    }

    ++parms->callcount;

    /* need to add this node */
    newresnode = new_val_resnode(pcb, 
                                 position,
                                 FALSE,
                                 val);
    if (!newresnode) {
        parms->res = ERR_INTERNAL_MEM;
        return FALSE;
    }

    dlq_enque(newresnode, parms->resnodeQ);
    return TRUE;

}  /* value_walker_fn */


/********************************************************************
* FUNCTION object_walker_fn
* 
* Check the current found object node, based on the
* criteria passed to the search function
*
* Matches obj_walker_fn_t template in obj.h
*
* INPUTS:
*    val == value node found in the search
*    cookie1 == xpath_pcb_t * : parser control block to use
*    cookie2 == xpath_walkerparms_t *: walker parms to use
*
* OUTPUTS:
*    result->nodeQ contents adjusted or replaced
*
* RETURNS:
*    TRUE to keep walk going
*    FALSE to terminate walk
*********************************************************************/
static boolean
    object_walker_fn (obj_template_t *obj,
                      void *cookie1,
                      void *cookie2)
{
    xpath_pcb_t          *pcb;
    xpath_walkerparms_t  *parms;
    xpath_resnode_t      *newresnode;

    pcb = (xpath_pcb_t *)cookie1;
    parms = (xpath_walkerparms_t *)cookie2;

    /* check if this node is already in the result */
    if (find_resnode(pcb, parms->resnodeQ, obj)) {
        return TRUE;
    }

    /* need to add this child node 
     * the position is wrong but it will not really matter
     */
    newresnode = new_obj_resnode(pcb, 
                                 ++parms->callcount, 
                                 FALSE,
                                 obj);
    if (!newresnode) {
        parms->res = ERR_INTERNAL_MEM;
        return FALSE;
    }

    dlq_enque(newresnode, parms->resnodeQ);
    return TRUE;

}  /* object_walker_fn */


/********************************************************************
* FUNCTION set_nodeset_self
* 
* Check the current result nodeset and keep each
* node, or remove it if filter tests fail
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of return XPath result nodeset
*    nsid == 0 if any namespace is OK
*              else only this namespace will be checked
*    name == name of parent to find
*         == NULL to find any parent name
*    textmode == TRUE if just selecting text() nodes
*                FALSE if ignored
*
* OUTPUTS:
*    result->nodeQ contents adjusted or removed
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_nodeset_self (xpath_pcb_t *pcb,
                      xpath_result_t *result,
                      xmlns_id_t nsid,
                      const xmlChar *name,
                      boolean textmode)
{
    xpath_resnode_t        *resnode, *findnode;
    obj_template_t         *testobj;
    const xmlChar           *modname;
    val_value_t            *testval;
    boolean                 keep, cfgonly, fnresult, fncalled, useroot;
    dlq_hdr_t               resnodeQ;
    status_t                res;
    xpath_walkerparms_t     walkerparms;

    if (!pcb->val && !pcb->obj) {
        return NO_ERR;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return NO_ERR;
    }

    dlq_createSQue(&resnodeQ);

    walkerparms.resnodeQ = &resnodeQ;
    walkerparms.res = NO_ERR;
    walkerparms.callcount = 0;

    modname = (nsid) ? xmlns_get_module(nsid) : NULL;
    useroot = (pcb->flags & XP_FL_USEROOT) ? TRUE : FALSE;

    res = NO_ERR;

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    while (!dlq_empty(&result->r.nodeQ) && res == NO_ERR) {

        resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);

        if (resnode->dblslash) {
            if (pcb->val) {
                testval = resnode->node.valptr;
                cfgonly = obj_is_config(testval->obj);

                fnresult = 
                    val_find_all_descendants(value_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testval, 
                                             modname,
                                             name,
                                             cfgonly,
                                             textmode,
                                             TRUE,
                                             FALSE);
            } else {
                testobj = resnode->node.objptr;
                cfgonly = obj_is_config(testobj);

                fnresult = 
                    obj_find_all_descendants(pcb->objmod,
                                             object_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testobj,
                                             modname,
                                             name,
                                             cfgonly,
                                             textmode,
                                             useroot,
                                             TRUE,
                                             &fncalled);
            }

            if (!fnresult || walkerparms.res != NO_ERR) {
                res = walkerparms.res;
            }
            free_resnode(pcb, resnode);
        } else {
            keep = FALSE;

            if (pcb->val) {
                testval = resnode->node.valptr;
            
                if (textmode) {
                    if (obj_has_text_content(testval->obj)) {
                        keep = TRUE;
                    }
                } else if (modname && name) {
                    if (!xml_strcmp(modname,
                                    val_get_mod_name(testval)) &&
                        !xml_strcmp(name, testval->name)) {
                        keep = TRUE;
                    }

                } else if (modname) {
                    if (!xml_strcmp(modname,
                                    val_get_mod_name(testval))) {
                        keep = TRUE;
                    }
                } else if (name) {
                    if (!xml_strcmp(name, testval->name)) {
                        keep = TRUE;
                    }
                } else {
                    keep = TRUE;
                }

                if (keep) {
                    findnode = find_resnode(pcb, 
                                            &resnodeQ, 
                                            testval);
                    if (findnode) {
                        if (resnode->dblslash) {
                            findnode->dblslash = TRUE;
                            findnode->position = 
                                ++walkerparms.callcount;
                        }
                        free_resnode(pcb, resnode);
                    } else {
                        /* set the resnode to its parent */
                        resnode->node.valptr = testval;
                        resnode->position = 
                            ++walkerparms.callcount;
                        dlq_enque(resnode, &resnodeQ);
                    }
                } else {
                    free_resnode(pcb, resnode);
                }
            } else {
                testobj = resnode->node.objptr;

                if (textmode) {
                    if (obj_has_text_content(testobj)) {
                        keep = TRUE;
                    }
                } else if (modname && name) {
                    if (!xml_strcmp(modname,
                                    obj_get_mod_name(testobj)) &&
                        !xml_strcmp(name, obj_get_name(testobj))) {
                        keep = TRUE;
                    }
                } else if (modname) {
                    if (!xml_strcmp(modname,
                                    obj_get_mod_name(testobj))) {
                        keep = TRUE;
                    }
                } else if (name) {
                    if (!xml_strcmp(name, obj_get_name(testobj))) {
                        keep = TRUE;
                    }
                } else {
                    keep = TRUE;
                }

                if (keep) {
                    findnode = find_resnode(pcb, 
                                            &resnodeQ, 
                                            testobj);
                    if (findnode) {
                        if (resnode->dblslash) {
                            findnode->dblslash = TRUE;
                            findnode->position = 
                                ++walkerparms.callcount;
                        }
                        free_resnode(pcb, resnode);
                    } else {
                        /* set the resnode to its parent */
                        resnode->node.objptr = testobj;
                        resnode->position =
                            ++walkerparms.callcount;
                        dlq_enque(resnode, &resnodeQ);
                    }
                } else {
                    if (pcb->logerrors && 
                        ncx_warning_enabled(ERR_NCX_NO_XPATH_NODES)) {
                        log_warn("\nWarning: no self node found "
                                 "in XPath expr '%s'", 
                                 pcb->exprstr);
                        ncx_print_errormsg(pcb->tkc, 
                                           pcb->objmod, 
                                           ERR_NCX_NO_XPATH_NODES);
                    } else if (pcb->objmod != NULL) {
                        ncx_inc_warnings(pcb->objmod);
                    }
                    free_resnode(pcb, resnode);
                }
            }
        }
    }

    /* put the resnode entries back where they belong */
    if (!dlq_empty(&resnodeQ)) {
        dlq_block_enque(&resnodeQ, &result->r.nodeQ);
    }

    result->last = walkerparms.callcount;

    return res;

}  /* set_nodeset_self */


/********************************************************************
* FUNCTION set_nodeset_parent
* 
* Check the current result nodeset and move each
* node to its parent, or remove it if no parent
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of return XPath result nodeset
*    nsid == 0 if any namespace is OK
*              else only this namespace will be checked
*    name == name of parent to find
*         == NULL to find any parent name
*
* OUTPUTS:
*    result->nodeQ contents adjusted or removed
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_nodeset_parent (xpath_pcb_t *pcb,
                        xpath_result_t *result,
                        xmlns_id_t nsid,
                        const xmlChar *name)
{
    xpath_resnode_t        *resnode, *findnode;
    obj_template_t         *testobj, *useobj;
    const xmlChar           *modname;
    val_value_t            *testval;
    boolean                 keep, cfgonly, fnresult, fncalled, useroot;
    dlq_hdr_t               resnodeQ;
    status_t                res;
    xpath_walkerparms_t     walkerparms;
    int64                   position;


    if (!pcb->val && !pcb->obj) {
        return NO_ERR;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return NO_ERR;
    }

    dlq_createSQue(&resnodeQ);

    walkerparms.resnodeQ = &resnodeQ;
    walkerparms.res = NO_ERR;
    walkerparms.callcount = 0;

    modname = (nsid) ? xmlns_get_module(nsid) : NULL;
    useroot = (pcb->flags & XP_FL_USEROOT) ? TRUE : FALSE;

    res = NO_ERR;
    position = 0;

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    while (!dlq_empty(&result->r.nodeQ) && res == NO_ERR) {

        resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);

        if (resnode->dblslash) {
            if (pcb->val) {
                testval = resnode->node.valptr;
                cfgonly = obj_is_config(testval->obj);

                fnresult = 
                    val_find_all_descendants(value_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testval, 
                                             modname,
                                             name,
                                             cfgonly,
                                             FALSE,
                                             TRUE,
                                             FALSE);
            } else {
                testobj = resnode->node.objptr;
                cfgonly = obj_is_config(testobj);

                fnresult = 
                    obj_find_all_descendants(pcb->objmod,
                                             object_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testobj,
                                             modname,
                                             name,
                                             cfgonly,
                                             FALSE,
                                             useroot,
                                             TRUE,
                                             &fncalled);
            }

            if (!fnresult || walkerparms.res != NO_ERR) {
                res = walkerparms.res;
            }
            free_resnode(pcb, resnode);
        } else {
            keep = FALSE;

            if (pcb->val) {
                testval = resnode->node.valptr;
            
                if (testval == pcb->val_docroot) {
                    if (resnode->dblslash) {
                        /* just move this node to the result */
                        resnode->position = ++position;
                        dlq_enque(resnode, &resnodeQ);
                    } else {
                        /* no parent available error
                         * remove node from result 
                         */
                        no_parent_warning(pcb);
                        free_resnode(pcb, resnode);
                    }
                } else {
                    testval = testval->parent;
                    
                    if (modname && name) {
                        if (!xml_strcmp(modname,
                                        val_get_mod_name(testval)) &&
                            !xml_strcmp(name, testval->name)) {
                            keep = TRUE;
                        }
                    } else if (modname) {
                        if (!xml_strcmp(modname,
                                        val_get_mod_name(testval))) {
                            keep = TRUE;
                        }
                    } else if (name) {
                        if (!xml_strcmp(name, testval->name)) {
                            keep = TRUE;
                        }
                    } else {
                        keep = TRUE;
                    }

                    if (keep) {
                        findnode = find_resnode(pcb, 
                                                &resnodeQ, 
                                                testval);
                        if (findnode) {
                            /* parent already in the Q
                             * remove node from result 
                             */
                            if (resnode->dblslash) {
                                findnode->position = ++position;
                                findnode->dblslash = TRUE;
                            }
                            free_resnode(pcb, resnode);
                        } else {
                            /* set the resnode to its parent */
                            resnode->position = ++position;
                            resnode->node.valptr = testval;
                            dlq_enque(resnode, &resnodeQ);
                        }
                    } else {
                        /* no parent available error
                         * remove node from result 
                         */
                        no_parent_warning(pcb);
                        free_resnode(pcb, resnode);
                    }
                }
            } else {
                testobj = resnode->node.objptr;
                if (testobj == pcb->docroot) {
                    resnode->position = ++position;
                    dlq_enque(resnode, &resnodeQ);
                } else if (!testobj->parent) {
                    if (!resnode->dblslash && (modname || name)) {
                        no_parent_warning(pcb);
                        free_resnode(pcb, resnode);
                    } else {
                        /* this is a databd node */
                        findnode = find_resnode(pcb, 
                                                &resnodeQ, 
                                                pcb->docroot);
                        if (findnode) {
                            if (resnode->dblslash) {
                                findnode->position = ++position;
                                findnode->dblslash = TRUE;
                            }
                            free_resnode(pcb, resnode);
                        } else {
                            resnode->position = ++position;
                            resnode->node.objptr = pcb->docroot;
                            dlq_enque(resnode, &resnodeQ);
                        }
                    }
                } else {
                    /* find a parent but not a case or choice */
                    testobj = testobj->parent;
                    while (testobj && testobj->parent && 
                           !obj_is_root(testobj->parent) &&
                           (testobj->objtype==OBJ_TYP_CHOICE ||
                            testobj->objtype==OBJ_TYP_CASE)) {
                        testobj = testobj->parent;
                    }

                    /* check if stopped on top-level choice
                     * a top-level case should not happen
                     * but check it anyway
                     */
                    if (testobj->objtype==OBJ_TYP_CHOICE ||
                        testobj->objtype==OBJ_TYP_CASE) {
                        useobj = pcb->docroot;
                    } else {
                        useobj = testobj;
                    }

                    if (modname && name) {
                        if (!xml_strcmp(modname,
                                        obj_get_mod_name(useobj)) &&
                            !xml_strcmp(name, obj_get_name(useobj))) {
                            keep = TRUE;
                        }
                    } else if (modname) {
                        if (!xml_strcmp(modname,
                                        obj_get_mod_name(useobj))) {
                            keep = TRUE;
                        }
                    } else if (name) {
                        if (!xml_strcmp(name, obj_get_name(useobj))) {
                            keep = TRUE;
                        }
                    } else {
                        keep = TRUE;
                    }

                    if (keep) {
                        /* replace this node with the useobj */
                        findnode = find_resnode(pcb, &resnodeQ, useobj);
                        if (findnode) {
                            if (resnode->dblslash) {
                                findnode->position = ++position;
                                findnode->dblslash = TRUE;
                            }
                            free_resnode(pcb, resnode);
                        } else {
                            resnode->node.objptr = useobj;
                            resnode->position = ++position;
                            dlq_enque(resnode, &resnodeQ);
                        }
                    } else {
                        no_parent_warning(pcb);
                        free_resnode(pcb, resnode);
                    }
                }
            }
        }
    }

    /* put the resnode entries back where they belong */
    if (!dlq_empty(&resnodeQ)) {
        dlq_block_enque(&resnodeQ, &result->r.nodeQ);
    }

    result->last = (position) ? position : walkerparms.callcount;

    return res;

}  /* set_nodeset_parent */


/********************************************************************
* FUNCTION set_nodeset_child
* 
* Check the current result nodeset and replace
* each node with a node for every child instead
*
* Handles child and descendant nodes
*
* If a child is specified then any node not
* containing this child will be removed
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of return XPath result nodeset
*    childnsid == 0 if any namespace is OK
*                 else only this namespace will be checked
*    childname == name of child to find
*              == NULL to find all child nodes
*                 In this mode, if the context object
*                 is config=true, then config=false
*                 children will be skipped
*    textmode == TRUE if just selecting text() nodes
*                FALSE if ignored
*     axis == actual axis used
*
* OUTPUTS:
*    result->nodeQ contents adjusted or replaced
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_nodeset_child (xpath_pcb_t *pcb,
                       xpath_result_t *result,
                       xmlns_id_t  childnsid,
                       const xmlChar *childname,
                       boolean textmode,
                       ncx_xpath_axis_t axis)
{
    xpath_resnode_t      *resnode;
    obj_template_t       *testobj;
    val_value_t          *testval;
    const xmlChar        *modname;
    status_t              res;
    boolean               fnresult, fncalled, cfgonly;
    boolean               orself, myorself, useroot;
    dlq_hdr_t             resnodeQ;
    xpath_walkerparms_t   walkerparms;

    if (!pcb->val && !pcb->obj) {
        return NO_ERR;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return NO_ERR;
    }

    dlq_createSQue(&resnodeQ);
    res = NO_ERR;
    cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;
    orself = (axis == XP_AX_DESCENDANT_OR_SELF) ? TRUE : FALSE;
    useroot = (pcb->flags & XP_FL_USEROOT) ? TRUE : FALSE;

    if (childnsid) {
        modname = xmlns_get_module(childnsid);
    } else {
        modname = NULL;
    }

    walkerparms.resnodeQ = &resnodeQ;
    walkerparms.res = NO_ERR;
    walkerparms.callcount = 0;

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    while (!dlq_empty(&result->r.nodeQ) && res == NO_ERR) {

        resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);
        myorself = (orself || resnode->dblslash) ? TRUE : FALSE;

        /* select 1 or all children of the resnode 
         * special YANG support; skip over nodes that
         * are not config nodes if they were selected
         * by the wildcard '*' operator
         */
        if (pcb->val) {

            testval = resnode->node.valptr;

            if (axis != XP_AX_CHILD || resnode->dblslash) {
                fnresult = 
                    val_find_all_descendants(value_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testval, 
                                             modname,
                                             childname,
                                             cfgonly,
                                             textmode,
                                             myorself,
                                             resnode->dblslash);
                if (!fnresult || walkerparms.res != NO_ERR) {
                    res = walkerparms.res;
                }
            } else {
                fnresult = 
                    val_find_all_children(value_walker_fn,
                                          pcb, 
                                          &walkerparms,
                                          testval, 
                                          modname,
                                          childname,
                                          cfgonly,
                                          textmode);
                if (!fnresult || walkerparms.res != NO_ERR) {
                    res = walkerparms.res;
                }
            }
        } else {
            testobj = resnode->node.objptr;

            if (axis != XP_AX_CHILD || resnode->dblslash) {
                fnresult = 
                    obj_find_all_descendants(pcb->objmod,
                                             object_walker_fn,
                                             pcb, 
                                             &walkerparms,
                                             testobj,
                                             modname,
                                             childname,
                                             cfgonly,
                                             textmode,
                                             useroot,
                                             myorself,
                                             &fncalled);
                if (!fnresult || walkerparms.res != NO_ERR) {
                    res = walkerparms.res;
                }
            } else {
                fnresult = 
                    obj_find_all_children(pcb->objmod,
                                          object_walker_fn,
                                          pcb,
                                          &walkerparms,
                                          testobj,
                                          modname,
                                          childname,
                                          cfgonly,
                                          textmode,
                                          useroot);
                if (!fnresult || walkerparms.res != NO_ERR) {
                    res = walkerparms.res;
                }
            }
        }

        free_resnode(pcb, resnode);
    }

    /* put the resnode entries back where they belong */
    if (!dlq_empty(&resnodeQ)) {
        dlq_block_enque(&resnodeQ, &result->r.nodeQ);
    } else if (!pcb->val && pcb->obj) {
        if (pcb->logerrors) {
            /* hack: the val_gen_instance_id code will generate
             * instance IDs that start with /nc:rpc QName.
             * The <rpc> node is currently unsupported in
             * node searches since all YANG content starts
             * from the RPC operation node;
             * check for the <nc:rpc> node and suppress the warning
             * if this is the node that is not found  */
            if ((childnsid == 0 || childnsid == xmlns_nc_id()) && 
                !xml_strcmp(childname, NCX_EL_RPC)) {
                ;  // assume this is an error-path XPath string
            } else if (axis != XP_AX_CHILD) {
                res = ERR_NCX_NO_XPATH_DESCENDANT;
                if (ncx_warning_enabled(res)) {
                    log_warn("\nWarning: no descendant nodes found "
                             "in XPath expr '%s'", 
                             pcb->exprstr);
                    ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                } else if (pcb->objmod) {
                    ncx_inc_warnings(pcb->objmod);
                }
            } else {
                res = ERR_NCX_NO_XPATH_CHILD;
                if (ncx_warning_enabled(res)) {
                    log_warn("\nWarning: no child nodes found "
                             "in XPath expr '%s'", 
                             pcb->exprstr);
                    ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                } else if (pcb->objmod) {
                    ncx_inc_warnings(pcb->objmod);
                }
            }
            res = NO_ERR;
        }
    }

    result->last = walkerparms.callcount;

    return res;

}  /* set_nodeset_child */


/********************************************************************
* FUNCTION set_nodeset_pfaxis
*
* Handles these axis node tests
*
*   preceding::*
*   preceding-sibling::*
*   following::*
*   following-sibling::*
*
* Combinations with '//' are handled as well
*
* Check the current result nodeset and replace
* each node with all the requested nodes instead,
* based on the axis used
*
* The result set is changed to the selected axis
* Since the current node is no longer going to
* be part of the result set
*
* After this initial step, the desired filtering
* is performed on each of the result nodes (if any)
*
* At this point, if a 'nsid' and/or 'name' is 
* specified then any selected node not
* containing this namespace and/or node name
* will be removed
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of return XPath result nodeset
*    nsid == 0 if any namespace is OK
*            else only this namespace will be checked
*    name == name of preceding node to find
*            == NULL to find all child nodes
*               In this mode, if the context object
*               is config=true, then config=false
*               preceding nodes will be skipped
*    textmode == TRUE if just selecting text() nodes
*                FALSE if ignored
*    axis == axis in use for this current step
*
* OUTPUTS:
*    result->nodeQ contents adjusted or replaced
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_nodeset_pfaxis (xpath_pcb_t *pcb,
                        xpath_result_t *result,
                        xmlns_id_t  nsid,
                        const xmlChar *name,
                        boolean textmode,
                        ncx_xpath_axis_t axis)
{
    xpath_resnode_t      *resnode;
    obj_template_t       *testobj;
    val_value_t          *testval;
    const xmlChar        *modname;
    status_t              res;
    boolean               fnresult, fncalled, cfgonly, useroot;
    dlq_hdr_t             resnodeQ;
    xpath_walkerparms_t   walkerparms;
    
    if (!pcb->val && !pcb->obj) {
        return NO_ERR;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return NO_ERR;
    }

    dlq_createSQue(&resnodeQ);
    res = NO_ERR;
    cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;

    modname = (nsid) ? xmlns_get_module(nsid) : NULL;
    useroot = (pcb->flags & XP_FL_USEROOT) ? TRUE : FALSE;

    walkerparms.resnodeQ = &resnodeQ;
    walkerparms.res = NO_ERR;
    walkerparms.callcount = 0;

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    while (!dlq_empty(&result->r.nodeQ) && res == NO_ERR) {

        resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);

        /* select 1 or all children of the resnode 
         * special YANG support; skip over nodes that
         * are not config nodes if they were selected
         * by the wildcard '*' operator
         */
        if (pcb->val) {
            testval = resnode->node.valptr;

            if (axis==XP_AX_PRECEDING || axis==XP_AX_FOLLOWING) {
                fnresult = 
                    val_find_all_pfaxis(value_walker_fn,
                                        pcb, 
                                        &walkerparms,
                                        testval, 
                                        modname,
                                        name, 
                                        cfgonly, 
                                        resnode->dblslash, 
                                        textmode,
                                        axis);
            } else {
                fnresult = 
                    val_find_all_pfsibling_axis(value_walker_fn,
                                                pcb, 
                                                &walkerparms,
                                                testval, 
                                                modname,
                                                name, 
                                                cfgonly, 
                                                resnode->dblslash, 
                                                textmode,
                                                axis);

            }
            if (!fnresult || walkerparms.res != NO_ERR) {
                res = walkerparms.res;
            }
        } else {
            testobj = resnode->node.objptr;
            fnresult = obj_find_all_pfaxis(pcb->objmod,
                                           object_walker_fn,
                                           pcb, 
                                           &walkerparms,
                                           testobj, 
                                           modname,
                                           name, 
                                           cfgonly,
                                           resnode->dblslash,
                                           textmode,
                                           useroot,
                                           axis, 
                                           &fncalled);
            if (!fnresult || walkerparms.res != NO_ERR) {
                res = walkerparms.res;
            }
        }
        free_resnode(pcb, resnode);
    }

    /* put the resnode entries back where they belong */
    if (!dlq_empty(&resnodeQ)) {
        dlq_block_enque(&resnodeQ, &result->r.nodeQ);
    } else if (!pcb->val && pcb->obj) {
        res = ERR_NCX_NO_XPATH_NODES;
        if (pcb->logerrors && ncx_warning_enabled(res)) {
            log_warn("\nWarning: no axis nodes found "
                     "in XPath expr '%s'", 
                     pcb->exprstr);
            ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
        } else if (pcb->objmod != NULL) {
            ncx_inc_warnings(pcb->objmod);
        }
        res = NO_ERR;
    }

    result->last = walkerparms.callcount;

    return res;

}  /* set_nodeset_pfaxis */


/********************************************************************
* FUNCTION set_nodeset_ancestor
* 
* Check the current result nodeset and move each
* node to the specified ancestor, or remove it 
* if none found that matches the filter criteria
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of return XPath result nodeset
*    nsid == 0 if any namespace is OK
*                 else only this namespace will be checked
*    name == name of ancestor to find
*              == NULL to find any ancestor nodes
*                 In this mode, if the context object
*                 is config=true, then config=false
*                 children will be skipped
*    textmode == TRUE if just selecting text() nodes
*                FALSE if ignored
*     axis == axis in use for this current step
*             XP_AX_ANCESTOR or XP_AX_ANCESTOR_OR_SELF
*
* OUTPUTS:
*    result->nodeQ contents adjusted or removed
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_nodeset_ancestor (xpath_pcb_t *pcb,
                          xpath_result_t *result,
                          xmlns_id_t  nsid,
                          const xmlChar *name,
                          boolean textmode,
                          ncx_xpath_axis_t axis)
{
    xpath_resnode_t        *resnode, *testnode;
    obj_template_t         *testobj;
    val_value_t            *testval;
    const xmlChar          *modname;
    xpath_result_t         *dummy;
    status_t                res;
    boolean                 cfgonly, fnresult, fncalled, orself, useroot;
    dlq_hdr_t               resnodeQ;
    xpath_walkerparms_t     walkerparms;

    if (!pcb->val && !pcb->obj) {
        return NO_ERR;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return NO_ERR;
    }

    dlq_createSQue(&resnodeQ);

    res = NO_ERR;
    testval = NULL;
    testobj = NULL;

    modname = (nsid) ? xmlns_get_module(nsid) : NULL;

    cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;
    useroot = (pcb->flags & XP_FL_USEROOT) ? TRUE : FALSE;
    orself = (axis == XP_AX_ANCESTOR_OR_SELF) ? TRUE : FALSE;

    walkerparms.resnodeQ = &resnodeQ;
    walkerparms.res = NO_ERR;

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    while (!dlq_empty(&result->r.nodeQ) && res == NO_ERR) {

        resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);

        walkerparms.callcount = 0;

        if (pcb->val) {
            testval = resnode->node.valptr;
            fnresult = val_find_all_ancestors(value_walker_fn,
                                              pcb,
                                              &walkerparms,
                                              testval,
                                              modname, 
                                              name, 
                                              cfgonly,
                                              orself,
                                              textmode);
        } else {
            testobj = resnode->node.objptr;
            fnresult = obj_find_all_ancestors(pcb->objmod,
                                              object_walker_fn,
                                              pcb,
                                              &walkerparms,
                                              testobj,
                                              modname, 
                                              name, 
                                              cfgonly,
                                              textmode,
                                              useroot,
                                              orself,
                                              &fncalled);

        }
        if (walkerparms.res != NO_ERR) {
            res = walkerparms.res;
        } else if (!fnresult) {
            res = ERR_NCX_OPERATION_FAILED;
        } else if (!walkerparms.callcount && resnode->dblslash) {
            dummy = new_result(pcb, XP_RT_NODESET);
            if (!dummy) {
                res = ERR_INTERNAL_MEM;
                continue;
            }

            walkerparms.resnodeQ = &dummy->r.nodeQ;
            if (pcb->val) {
                fnresult = val_find_all_descendants(value_walker_fn,
                                                    pcb,
                                                    &walkerparms,
                                                    testval,
                                                    modname, 
                                                    name, 
                                                    cfgonly,
                                                    textmode,
                                                    TRUE,
                                                    FALSE);

            } else {
                fnresult = obj_find_all_descendants(pcb->objmod,
                                                    object_walker_fn,
                                                    pcb,
                                                    &walkerparms,
                                                    testobj,
                                                    modname, 
                                                    name, 
                                                    cfgonly,
                                                    textmode,
                                                    useroot,
                                                    TRUE,
                                                    &fncalled);
            }
            walkerparms.resnodeQ = &resnodeQ;

            if (walkerparms.res != NO_ERR) {
                res = walkerparms.res;
            } else if (!fnresult) {
                res = ERR_NCX_OPERATION_FAILED;
            } else {
                res = set_nodeset_ancestor(pcb, 
                                           dummy,
                                           nsid, 
                                           name, 
                                           textmode,
                                           axis);
                while (!dlq_empty(&dummy->r.nodeQ)) {
                    testnode = (xpath_resnode_t *)
                        dlq_deque(&dummy->r.nodeQ);

                    /* It is assumed that testnode cannot NULL because the call 
                     * to dlq_empty returned false. */
                    if (find_resnode(pcb, &resnodeQ,
                                     (const void *)testnode->node.valptr)) {
                        free_resnode(pcb, testnode);
                    } else {
                        dlq_enque(testnode, &resnodeQ);
                    }
                }
                free_result(pcb, dummy);
            }
        }

        free_resnode(pcb, resnode);
    }

    /* put the resnode entries back where they belong */
    if (!dlq_empty(&resnodeQ)) {
        dlq_block_enque(&resnodeQ, &result->r.nodeQ);
    } else {
        res = ERR_NCX_NO_XPATH_ANCESTOR;
        if (pcb->logerrors && ncx_warning_enabled(res)) {
            if (orself) {
                log_warn("\nWarning: no ancestor-or-self nodes found "
                         "in XPath expr '%s'", pcb->exprstr);
            } else {
                log_warn("\nWarning: no ancestor nodes found "
                         "in XPath expr '%s'", pcb->exprstr);
            }
            ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
        } else if (pcb->objmod != NULL) {
            ncx_inc_warnings(pcb->objmod);
        }
        res = NO_ERR;
    }

    result->last = walkerparms.callcount;

    return res;

}  /* set_nodeset_ancestor */


/***********   B E G I N    E B N F    F U N C T I O N S *************/


/********************************************************************
* FUNCTION parse_node_test
* 
* Parse the XPath NodeTest sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [7] NodeTest ::= NameTest
*                  | NodeType '(' ')'
*                  | 'processing-instruction' '(' Literal ')'
*
* [37] NameTest ::= '*'
*                  | NCName ':' '*'
*                  | QName
*
* [38] NodeType ::= 'comment'
*                    | 'text'   
*                    | 'processing-instruction'
*                    | 'node'
*
* INPUTS:
*    pcb == parser control block in progress
*    axis  == current axis from first part of Step
*    result == address of pointer to result struct in progress
*
* OUTPUTS:
*   *result is modified
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_node_test (xpath_pcb_t *pcb,
                     ncx_xpath_axis_t axis,
                     xpath_result_t **result)
{
    const xmlChar     *name;
    tk_type_t          nexttyp;
    xpath_nodetype_t   nodetyp;
    status_t           res;
    xmlns_id_t         nsid;
    boolean            emptyresult, textmode;

    nsid = 0;
    name = NULL;
    textmode = FALSE;

    res = TK_ADV(pcb->tkc);
    if (res != NO_ERR) {
        res = ERR_NCX_INVALID_XPATH_EXPR;
        if (pcb->logerrors) {
            log_error("\nError: token expected in XPath "
                      "expression '%s'", pcb->exprstr);
            ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, res);
        } else {
            /*** handle agent error ***/
        }
        return res;
    }

    /* process the tokens but not the result yet */
    switch (TK_CUR_TYP(pcb->tkc)) {
    case TK_TT_STAR:
        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }
        break;
    case TK_TT_NCNAME_STAR:
        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }
        /* match all nodes in the namespace w/ specified prefix */
        if (!pcb->tkc->cur->nsid) {
            res = check_qname_prefix(pcb,
                                     TK_CUR_VAL(pcb->tkc),
                                     xml_strlen(TK_CUR_VAL(pcb->tkc)),
                                     &pcb->tkc->cur->nsid);
        }
        if (res == NO_ERR) {
            nsid = pcb->tkc->cur->nsid;
        }
        break;
    case TK_TT_MSTRING:
        /* match all nodes in the namespace w/ specified prefix */
        if (!pcb->tkc->cur->nsid) {
            res = check_qname_prefix(pcb, 
                                     TK_CUR_MOD(pcb->tkc),
                                     TK_CUR_MODLEN(pcb->tkc),
                                     &pcb->tkc->cur->nsid);
        }
        if (res == NO_ERR) {
            nsid = pcb->tkc->cur->nsid;
            name = TK_CUR_VAL(pcb->tkc);
        }
        break;
    case TK_TT_TSTRING:
        /* check the ID token for a NodeType name */
        nodetyp = get_nodetype_id(TK_CUR_VAL(pcb->tkc));
        if (nodetyp == XP_EXNT_NONE ||
            (tk_next_typ(pcb->tkc) != TK_TT_LPAREN)) {
            name = TK_CUR_VAL(pcb->tkc);
            break;
        }

        /* get the node test left paren */
        res = xpath_parse_token(pcb, TK_TT_LPAREN);
        if (res != NO_ERR) {
            return res;
        }

        /* check if a literal param can be present */
        if (nodetyp == XP_EXNT_PROC_INST) {
            /* check if a literal param is present */
            nexttyp = tk_next_typ(pcb->tkc);
            if (nexttyp==TK_TT_QSTRING ||
                nexttyp==TK_TT_SQSTRING) {
                /* temp save the literal string */
                res = xpath_parse_token(pcb, nexttyp);
                if (res != NO_ERR) {
                    return res;
                }
            }
        }

        /* get the node test right paren */
        res = xpath_parse_token(pcb, TK_TT_RPAREN);
        if (res != NO_ERR) {
            return res;
        }

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }

        /* process the result based on the node type test */
        switch (nodetyp) {
        case XP_EXNT_COMMENT:
            /* no comments to match */
            emptyresult = TRUE;
            if (pcb->obj && 
                pcb->logerrors && 
                ncx_warning_enabled(ERR_NCX_EMPTY_XPATH_RESULT)) {
                log_warn("\nWarning: no comment nodes available in "
                         "XPath expr '%s'", 
                         pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, 
                                   pcb->tkerr.mod,
                                   ERR_NCX_EMPTY_XPATH_RESULT);
            } else if (pcb->objmod != NULL) {
                ncx_inc_warnings(pcb->objmod);
            }

            break;
        case XP_EXNT_TEXT:
            /* match all leaf of leaf-list content */
            emptyresult = FALSE;
            textmode = TRUE;
            break;
        case XP_EXNT_PROC_INST:
            /* no processing instructions to match */
            emptyresult = TRUE;
            if (pcb->obj && 
                pcb->logerrors &&
                ncx_warning_enabled(ERR_NCX_EMPTY_XPATH_RESULT)) {
                log_warn("\nWarning: no processing instruction "
                         "nodes available in "
                         "XPath expr '%s'", 
                         pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, 
                                   pcb->tkerr.mod,
                                   ERR_NCX_EMPTY_XPATH_RESULT);
            } else if (pcb->objmod != NULL) {
                ncx_inc_warnings(pcb->objmod);
            }
            break;
        case XP_EXNT_NODE:
            /* match any node */
            emptyresult = FALSE;
            break;
        default:
            emptyresult = TRUE;
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (emptyresult) {
            if (*result) {
                free_result(pcb, *result);
            }
            *result = new_result(pcb, XP_RT_NODESET);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
            return res;
        }  /* else go on to the text() or node() test */
        break;
    default:
        /* wrong token type found */
        res = ERR_NCX_WRONG_TKTYPE;
        unexpected_error(pcb);
    }

    /* do not care about result if fatal error occurred */
    if (res != NO_ERR) {
        return res;
    } else if (!pcb->val && !pcb->obj) {
        /* nothing to do in first pass except create
         * dummy result to flag that a location step
         * has already started
         */
        if (!*result) {
            *result = new_result(pcb, XP_RT_NODESET);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }
        return res;
    }


    /* if we get here, then the axis and name fields are set
     * or a texttest is needed
     */
    switch (axis) {
    case XP_AX_ANCESTOR:
    case XP_AX_ANCESTOR_OR_SELF:
        if (!*result) {
            *result = new_nodeset(pcb,
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }
        if (res == NO_ERR) {
            res = set_nodeset_ancestor(pcb, 
                                       *result,
                                       nsid, 
                                       name,
                                       textmode,
                                       axis);
        } 
        break;
    case XP_AX_ATTRIBUTE:
        /* Attribute support in XPath is TBD
         * YANG does not define them and the ncx:metadata
         * extension is not fully supported within the 
         * the edit-config code yet anyway
         *
         * just set the result to the empty nodeset
         */
        if (pcb->obj && 
            pcb->logerrors &&
            ncx_warning_enabled(ERR_NCX_EMPTY_XPATH_RESULT)) {
            log_warn("\nWarning: attribute axis is empty in "
                     "XPath expr '%s'", pcb->exprstr);
            ncx_print_errormsg(pcb->tkc, 
                               pcb->tkerr.mod,
                               ERR_NCX_EMPTY_XPATH_RESULT);
        } else if (pcb->objmod != NULL) {
            ncx_inc_warnings(pcb->objmod);
        }

        if (*result) {
            free_result(pcb, *result);
        }
        *result = new_result(pcb, XP_RT_NODESET);
        if (!*result) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case XP_AX_DESCENDANT:
    case XP_AX_DESCENDANT_OR_SELF:
    case XP_AX_CHILD:
        /* select all the child nodes of each node in 
         * the result node set.
         * ALSO select all the descendant nodes of each node in 
         * the result node set. (they are the same in NETCONF)
         */
        if (!*result) {
            /* first step is child::* or descendant::*    */
            *result = new_nodeset(pcb, 
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }
        if (res == NO_ERR) {
            res = set_nodeset_child(pcb, 
                                    *result, 
                                    nsid, 
                                    name, 
                                    textmode,
                                    axis);
        }
        break;
    case XP_AX_FOLLOWING:
    case XP_AX_PRECEDING:
    case XP_AX_FOLLOWING_SIBLING:
    case XP_AX_PRECEDING_SIBLING:
        /* need to set the result to all the objects
         * or all instances of all value nodes
         * preceding or following the context node
         */
        if (!*result) {
            /* first step is following::* or preceding::* */
            *result = new_nodeset(pcb, 
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }

        if (res == NO_ERR) {
            res = set_nodeset_pfaxis(pcb,
                                     *result,
                                     nsid, 
                                     name,
                                     textmode,
                                     axis);
        }        
        break;
    case XP_AX_NAMESPACE:
        /* for NETCONF purposes, there is no need to
         * provide access to namespace xmlns declarations
         * within the object or value tree
         * This can be added later!
         *
         *
         * For now, just turn the result into the empty set
         */
        if (pcb->obj && 
            pcb->logerrors &&
            ncx_warning_enabled(ERR_NCX_EMPTY_XPATH_RESULT)) {
            log_warn("Warning: namespace axis is empty in "
                     "XPath expr '%s'", 
                     pcb->exprstr);
            ncx_print_errormsg(pcb->tkc, 
                               pcb->tkerr.mod,
                               ERR_NCX_EMPTY_XPATH_RESULT);
        } else if (pcb->objmod != NULL) {
            ncx_inc_warnings(pcb->objmod);
        }

        if (*result) {
            free_result(pcb, *result);
        }
        *result = new_result(pcb, XP_RT_NODESET);
        if (!*result) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case XP_AX_PARENT:
        /* step is parent::*  -- same as .. for nodes  */
        if (textmode) {
            if (pcb->obj && 
                pcb->logerrors &&
                ncx_warning_enabled(ERR_NCX_EMPTY_XPATH_RESULT)) {
                log_warn("Warning: parent axis contains no text nodes in "
                     "XPath expr '%s'", 
                         pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, 
                                   pcb->tkerr.mod,
                                   ERR_NCX_EMPTY_XPATH_RESULT);
            } else if (pcb->objmod != NULL) {
                ncx_inc_warnings(pcb->objmod);
            }

            if (*result) {
                free_result(pcb, *result);
            }
            *result = new_result(pcb, XP_RT_NODESET);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
            break;
        }
            
        if (!*result) {
            *result = new_nodeset(pcb, 
                                  pcb->context.node.objptr, 
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }

        if (res == NO_ERR) {
            res = set_nodeset_parent(pcb, 
                                     *result, 
                                     nsid,
                                     name);
        }
        break;
    case XP_AX_SELF:
        /* keep the same context node */
        if (!*result) {
            /* first step is self::*   */
            *result = new_nodeset(pcb, 
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }
        if (res == NO_ERR) {
            res = set_nodeset_self(pcb,
                                   *result,
                                   nsid,
                                   name,
                                   textmode);
        }                                  
        break;
    case XP_AX_NONE:
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}  /* parse_node_test */


/********************************************************************
* FUNCTION parse_predicate
* 
* Parse an XPath Predicate sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [8] Predicate ::= '[' PredicateExpr ']'
* [9] PredicateExpr ::= Expr
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of result in progress to filter
*
* OUTPUTS:
*   *result may be pruned based on filter matches
*            nodes may be updated if descendants are
*            checked and matches are found
*
* RETURNS:
*   malloced result of predicate expression
*********************************************************************/
static status_t
    parse_predicate (xpath_pcb_t *pcb,
                     xpath_result_t **result)
{
    xpath_result_t  *val1, *contextset;
    xpath_resnode_t  lastcontext, *resnode, *nextnode;
    tk_token_t      *leftbrack;
    boolean          boo;
    status_t         res;
    int64            position;

    res = xpath_parse_token(pcb, TK_TT_LBRACK);
    if (res != NO_ERR) {
        return res;
    }

    boo = FALSE;
    if (pcb->val || pcb->obj) {
        leftbrack = TK_CUR(pcb->tkc);
        contextset = *result;
        if (!contextset) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        } else if (contextset->restype == XP_RT_NODESET) {
            if (dlq_empty(&contextset->r.nodeQ)) {
                /* always one pass; do not care about result */
                val1 = parse_expr(pcb, &res);
                if (res == NO_ERR) {
                    res = xpath_parse_token(pcb, TK_TT_RBRACK);
                }
                if (val1) {
                    free_result(pcb, val1);
                }
                return res;
            }

            lastcontext.node.valptr = 
                pcb->context.node.valptr;
            lastcontext.position = pcb->context.position;
            lastcontext.last = pcb->context.last;
            lastcontext.dblslash = pcb->context.dblslash;

            for (resnode = (xpath_resnode_t *)
                     dlq_firstEntry(&contextset->r.nodeQ);
                 resnode != NULL;
                 resnode = nextnode) {

                /* go over and over the predicate expression
                 * with the resnode as the current context node
                 */
                TK_CUR(pcb->tkc) = leftbrack;
                boo = FALSE;

                nextnode = (xpath_resnode_t *)
                    dlq_nextEntry(resnode);

                pcb->context.node.valptr = resnode->node.valptr;
                pcb->context.position = resnode->position;
                pcb->context.last = contextset->last;
                pcb->context.dblslash = resnode->dblslash;

#ifdef EXTRA_DEBUG
                if (LOGDEBUG3 && pcb->val) {
                    log_debug3("\nXPath setting context node: %s",
                               pcb->context.node.valptr->name);
                }
#endif

                val1 = parse_expr(pcb, &res);
                if (res != NO_ERR) {
                    if (val1) {
                        free_result(pcb, val1);
                    }
                    return res;
                }
            
                res = xpath_parse_token(pcb, TK_TT_RBRACK);
                if (res != NO_ERR) {
                    if (val1) {
                        free_result(pcb, val1);
                    }
                    return res;
                }

                if (val1->restype == XP_RT_NUMBER) {
                    /* the predicate specifies a context
                     * position and this resnode is
                     * only selected if it is the Nth
                     * instance within the current context
                     */
                    if (ncx_num_is_integral(&val1->r.num,
                                            NCX_BT_FLOAT64)) {
                        position = 
                            ncx_cvt_to_int64(&val1->r.num,
                                             NCX_BT_FLOAT64);
                        /* check if the proximity position
                         * of this node matches the position
                         * value from this expression
                         */
                        boo = (position == resnode->position) ?
                            TRUE : FALSE;
                    } else {
                        boo = FALSE;
                    }
                } else {
                    boo = xpath_cvt_boolean(val1);
                }
                free_result(pcb, val1);

                if (!boo) {
                    /* predicate expression evaluated to false
                     * so delete this resnode from the result
                     */
                    dlq_remove(resnode);
                    free_resnode(pcb, resnode);
                }
            }

            pcb->context.node.valptr = 
                lastcontext.node.valptr;
            pcb->context.position = lastcontext.position;
            pcb->context.last = lastcontext.last;
            pcb->context.dblslash = lastcontext.dblslash;

#ifdef EXTRA_DEBUG
            if (LOGDEBUG3 && pcb->val) {
                log_debug3("\nXPath set context node to last: %s",
                           pcb->context.node.valptr->name);
            }
#endif

        } else {
            /* result is from a primary expression and
             * is not a nodeset.  It will get cleared
             * if the predicate evaluates to false
             */
            val1 = parse_expr(pcb, &res);
            if (res != NO_ERR) {
                if (val1) {
                    free_result(pcb, val1);
                }
                return res;
            }
            
            res = xpath_parse_token(pcb, TK_TT_RBRACK);
            if (res != NO_ERR) {
                if (val1) {
                    free_result(pcb, val1);
                }
                return res;
            }

            if (val1 && pcb->val) {
                boo = xpath_cvt_boolean(val1);
            }
            if (val1) {
                free_result(pcb, val1);
            }
            if (pcb->val && !boo && *result) {
                xpath_clean_result(*result);
                xpath_init_result(*result, XP_RT_NONE);
            }
        }
    } else {
        /* always one pass; do not care about result */
        val1 = parse_expr(pcb, &res);
        if (res == NO_ERR) {
            res = xpath_parse_token(pcb, TK_TT_RBRACK);
        }
        if (val1) {
            free_result(pcb, val1);
        }
    }

    return res;

} /* parse_predicate */


/********************************************************************
* FUNCTION parse_step
* 
* Parse the XPath Step sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [4] Step  ::=  AxisSpecifier NodeTest Predicate*        
*                | AbbreviatedStep        
*
* [12] AbbreviatedStep ::= '.'        | '..'
*
* [5] AxisSpecifier ::= AxisName '::'
*                | AbbreviatedAxisSpecifier
*
* [13] AbbreviatedAxisSpecifier ::= '@'?
*
* INPUTS:
*    pcb == parser control block in progress
*    result == address of erturn XPath result nodeset
*

* OUTPUTS:
*   *result pointer is set to malloced result struct
*    if it is NULL, or used if it is non-NULL;
*    MUST be a XP_RT_NODESET result

*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_step (xpath_pcb_t *pcb,
                xpath_result_t **result)
{
    const xmlChar    *nextval;
    tk_type_t         nexttyp, nexttyp2;
    ncx_xpath_axis_t  axis;
    status_t          res;

    nexttyp = tk_next_typ(pcb->tkc);
    axis = XP_AX_CHILD;

    /* check start token '/' or '//' */
    if (nexttyp == TK_TT_DBLFSLASH) {
        res = xpath_parse_token(pcb, TK_TT_DBLFSLASH);
        if (res != NO_ERR) {
            return res;
        }

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }

        if (!*result) {
            *result = new_nodeset(pcb,
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  TRUE);
            if (!*result) {
                return ERR_INTERNAL_MEM;
            }
        }
        set_nodeset_dblslash(pcb, *result);
    } else if (nexttyp == TK_TT_FSLASH) {
        res = xpath_parse_token(pcb, TK_TT_FSLASH);
        if (res != NO_ERR) {
            return res;
        }

        if (!*result) {
            /* this is the first call */
            *result = new_nodeset(pcb, 
                                  pcb->docroot, 
                                  pcb->val_docroot,
                                  1,
                                  FALSE);
            if (!*result) {
                return ERR_INTERNAL_MEM;
            }

            /* check corner-case path '/' */
            if (location_path_end(pcb)) {
                /* exprstr is simply docroot '/' */
                return NO_ERR;
            }
        }
    } else if (*result) {
        /* should not happen */
        SET_ERROR(ERR_INTERNAL_VAL);
        return ERR_NCX_INVALID_XPATH_EXPR;
    }

    /* handle an abbreviated step (. or ..) or
     * handle the axis-specifier for the full form step
     */
    nexttyp = tk_next_typ(pcb->tkc);
    switch (nexttyp) {
    case TK_TT_PERIOD:
        /* abbreviated step '.': 
         * current target node stays the same unless
         * this is the first token in the location path,
         * then the context result needs to be initialized
         *
         * first consume the period token
         */
        res = xpath_parse_token(pcb, TK_TT_PERIOD);
        if (res != NO_ERR) {
            return res;
        }

        /* first step is simply . */
        if (!*result) {
            *result = new_nodeset(pcb,
                                  pcb->context.node.objptr,
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                return ERR_INTERNAL_MEM;
            }
        } /* else leave current result alone */
        return NO_ERR;
    case TK_TT_RANGESEP:
        /* abbrev step '..': 
         * matches parent of current context */
        res = xpath_parse_token(pcb, TK_TT_RANGESEP);
        if (res != NO_ERR) {
            return res;
        }

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }

        /* step is .. or  //.. */
        if (!*result) {
            /* first step is .. or //..     */
            *result = new_nodeset(pcb, 
                                  pcb->context.node.objptr, 
                                  pcb->context.node.valptr,
                                  1, 
                                  FALSE);
            if (!*result) {
                res = ERR_INTERNAL_MEM;
            }
        }
        if (res == NO_ERR) {
            res = set_nodeset_parent(pcb, 
                                     *result,
                                     0,
                                     NULL);
        }
        return res;
    case TK_TT_ATSIGN:
        axis = XP_AX_ATTRIBUTE;
        res = xpath_parse_token(pcb, TK_TT_ATSIGN);
        if (res != NO_ERR) {
            return res;
        }

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            return ERR_NCX_INVALID_INSTANCEID;
        }
        break;
    case TK_TT_STAR:
    case TK_TT_NCNAME_STAR:
    case TK_TT_MSTRING:
        /* set the axis to default child, hit node test */
        axis = XP_AX_CHILD;
        break;
    case TK_TT_TSTRING:
        /* check the ID token for an axis name */
        nexttyp2 = tk_next_typ2(pcb->tkc);
        nextval = tk_next_val(pcb->tkc);
        axis = get_axis_id(nextval);
        if (axis != XP_AX_NONE && nexttyp2==TK_TT_DBLCOLON) {
            /* correct axis-name :: sequence */
            res = xpath_parse_token(pcb, TK_TT_TSTRING);
            if (res != NO_ERR) {
                return res;
            }
            res = xpath_parse_token(pcb, TK_TT_DBLCOLON);
            if (res != NO_ERR) {
                return res;
            }

            if (pcb->flags & XP_FL_INSTANCEID) {
                invalid_instanceid_error(pcb);
                return ERR_NCX_INVALID_INSTANCEID;
            }
        } else if (axis == XP_AX_NONE && nexttyp2==TK_TT_DBLCOLON) {
            /* incorrect axis-name :: sequence */
            (void)TK_ADV(pcb->tkc);
            res = ERR_NCX_INVALID_XPATH_EXPR;
            if (pcb->logerrors) {
                log_error("\nError: invalid axis name '%s' in "
                          "XPath expression '%s'",
                          (TK_CUR_VAL(pcb->tkc) != NULL) ?
                          TK_CUR_VAL(pcb->tkc) : EMPTY_STRING,
                          pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, res);
            } else {
                /*** log agent error ***/
            }
            (void)TK_ADV(pcb->tkc);
            return res;
        } else {
            axis = XP_AX_CHILD;
        }
        break;
    default:
        /* wrong token type found */
        (void)TK_ADV(pcb->tkc);
        res = ERR_NCX_WRONG_TKTYPE;
        unexpected_error(pcb);
        return res;
    }

    /* axis or default child parsed OK, get node test */
    res = parse_node_test(pcb, axis, result);
    if (res == NO_ERR) {
        nexttyp = tk_next_typ(pcb->tkc);
        while (nexttyp == TK_TT_LBRACK && res==NO_ERR) {
            res = parse_predicate(pcb, result);
            nexttyp = tk_next_typ(pcb->tkc);
        }
    }

    return res;

}  /* parse_step */


/********************************************************************
* FUNCTION parse_location_path
* 
* Parse the Location-Path sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [1] LocationPath  ::=  RelativeLocationPath        
*                       | AbsoluteLocationPath        
*
* [2] AbsoluteLocationPath ::= '/' RelativeLocationPath?
*                     | AbbreviatedAbsoluteLocationPath        
*
* [10] AbbreviatedAbsoluteLocationPath ::= '//' RelativeLocationPath
*
* [3] RelativeLocationPath ::= Step
*                       | RelativeLocationPath '/' Step
*                       | AbbreviatedRelativeLocationPath
*
* [11] AbbreviatedRelativeLocationPath ::= 
*               RelativeLocationPath '//' Step
*
* INPUTS:
*    pcb == parser control block in progress
*    result == result struct in progress or NULL if none
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_location_path (xpath_pcb_t *pcb,
                         xpath_result_t *result,
                         status_t *res)
{
    xpath_result_t     *val1;
    tk_type_t           nexttyp;
    boolean             done;

    val1 = result;

    done = FALSE;
    while (!done && *res == NO_ERR) {
        *res = parse_step(pcb, &val1);
        if (*res == NO_ERR) {
            nexttyp = tk_next_typ(pcb->tkc);
            if (!(nexttyp == TK_TT_FSLASH ||
                  nexttyp == TK_TT_DBLFSLASH)) {
                done = TRUE;
            }
        }
    }

    return val1;

}  /* parse_location_path */


/********************************************************************
* FUNCTION parse_function_call
* 
* Parse an XPath FunctionCall sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [16] FunctionCall ::= FunctionName 
*                        '(' ( Argument ( ',' Argument )* )? ')'
* [17] Argument ::= Expr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_function_call (xpath_pcb_t *pcb,
                         status_t *res)
{
    xpath_result_t         *val1, *val2;
    const xpath_fncb_t     *fncb;
    dlq_hdr_t               parmQ;
    tk_type_t               nexttyp;
    int32                   parmcnt;
    boolean                 done;

    val1 = NULL;
    parmcnt = 0;
    dlq_createSQue(&parmQ);

    /* get the function name */
    *res = xpath_parse_token(pcb, TK_TT_TSTRING);
    if (*res != NO_ERR) {
        return NULL;
    }

    if (pcb->flags & XP_FL_INSTANCEID) {
        invalid_instanceid_error(pcb);
        *res = ERR_NCX_INVALID_INSTANCEID;
        return NULL;
    }

    /* find the function in the library */
    fncb = NULL;
    if (TK_CUR_VAL(pcb->tkc) != NULL) {
        fncb = find_fncb(pcb, TK_CUR_VAL(pcb->tkc));
    }
    if (fncb) {
        /* get the mandatory left paren */
        *res = xpath_parse_token(pcb, TK_TT_LPAREN);
        if (*res != NO_ERR) {
            return NULL;
        }

        /* get parms until a matching right paren is reached */
        nexttyp = tk_next_typ(pcb->tkc);
        done = (nexttyp == TK_TT_RPAREN) ? TRUE : FALSE;
        while (!done && *res == NO_ERR) {
            val1 = parse_expr(pcb, res);
            if (*res == NO_ERR) {
                parmcnt++;
                if (val1) {
                    dlq_enque(val1, &parmQ);
                    val1 = NULL;
                }

                /* check for right paren or else should be comma */
                nexttyp = tk_next_typ(pcb->tkc);
                if (nexttyp == TK_TT_RPAREN) {
                    done = TRUE;
                } else {
                    *res = xpath_parse_token(pcb, TK_TT_COMMA);
                }
            }
        }

        /* get closing right paren */
        if (*res == NO_ERR) {
            *res = xpath_parse_token(pcb, TK_TT_RPAREN);
        }

        /* check parameter count */
        if (fncb->parmcnt >= 0 && fncb->parmcnt != parmcnt) {
            *res = (parmcnt > fncb->parmcnt) ?
                ERR_NCX_EXTRA_PARM : ERR_NCX_MISSING_PARM;

            if (pcb->logerrors) {       
                log_error("\nError: wrong number of "
                          "parameters got %d, need %d"
                          " for function '%s'",
                          parmcnt, fncb->parmcnt,
                          fncb->name);
                ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, *res);
            } else {
                /*** log agent error ***/
            }
        } else {
            /* make the function call */
            val1 = (*fncb->fn)(pcb, &parmQ, res);

            if (LOGDEBUG3) {
                if (val1) {
                    log_debug3("\nXPath fn %s result:",
                               fncb->name);
                    dump_result(pcb, val1, NULL);
                    if (pcb->val && pcb->context.node.valptr->name) {
                        log_debug3("\nXPath context val name: %s",
                                   pcb->context.node.valptr->name);
                    }
                }
            }
        }
    } else {
        *res = ERR_NCX_UNKNOWN_PARM;

        if (pcb->logerrors) {   
            log_error("\nError: Invalid XPath function name '%s'",
                      (TK_CUR_VAL(pcb->tkc) != NULL) ?
                      TK_CUR_VAL(pcb->tkc) : EMPTY_STRING);
            ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, *res);
        } else {
            /*** log agent error ***/
        }
    }

    /* clean up any function parameters */
    while (!dlq_empty(&parmQ)) {
        val2 = (xpath_result_t *)dlq_deque(&parmQ);
        free_result(pcb, val2);
    }

    return val1;

} /* parse_function_call */


/********************************************************************
* FUNCTION parse_primary_expr
* 
* Parse an XPath PrimaryExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [15] PrimaryExpr ::= VariableReference
*                       | '(' Expr ')'
*                       | Literal
*                       | Number
*                       | FunctionCall
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_primary_expr (xpath_pcb_t *pcb,
                        status_t *res)
{
    xpath_result_t         *val1;
    ncx_var_t              *varbind;
    const xmlChar          *errstr;
    tk_type_t               nexttyp;
    ncx_numfmt_t            numfmt;

    val1 = NULL;
    nexttyp = tk_next_typ(pcb->tkc);

    switch (nexttyp) {
    case TK_TT_VARBIND:
    case TK_TT_QVARBIND:
        *res = xpath_parse_token(pcb, nexttyp);

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            *res = ERR_NCX_INVALID_INSTANCEID;
            return NULL;
        }

        /* get QName or NCName variable reference
         * but only if this get is a real one
         */
        if (*res == NO_ERR && pcb->val) {
            if (TK_CUR_TYP(pcb->tkc) == TK_TT_VARBIND) {
                varbind = get_varbind(pcb, 
                                      NULL, 
                                      0, 
                                      TK_CUR_VAL(pcb->tkc), res);
                errstr = TK_CUR_VAL(pcb->tkc);
            } else {
                varbind = get_varbind(pcb, 
                                      TK_CUR_MOD(pcb->tkc),
                                      TK_CUR_MODLEN(pcb->tkc), 
                                      TK_CUR_VAL(pcb->tkc), res);
                errstr = TK_CUR_MOD(pcb->tkc);
            }
            if (!varbind || *res != NO_ERR) {
                if (pcb->logerrors) {
                    if (*res == ERR_NCX_DEF_NOT_FOUND) {
                        log_error("\nError: unknown variable binding '%s'",
                                  errstr);
                        ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, *res);
                    } else {
                        log_error("\nError: error in variable binding '%s'",
                                  errstr);
                        ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, *res);
                    }
                }
            } else {
                /* OK: found the variable binding */
                val1 = cvt_from_value(pcb, varbind->val);
                if (!val1) {
                    *res = ERR_INTERNAL_MEM;
                }
            }
        }
        break;
    case TK_TT_LPAREN:
        /* get ( expr ) */
        *res = xpath_parse_token(pcb, TK_TT_LPAREN);
        if (*res == NO_ERR) {

            if (pcb->flags & XP_FL_INSTANCEID) {
                invalid_instanceid_error(pcb);
                *res = ERR_NCX_INVALID_INSTANCEID;
                return NULL;
            }

            val1 = parse_expr(pcb, res);
            if (*res == NO_ERR) {
                *res = xpath_parse_token(pcb, TK_TT_RPAREN);
            }
        }
        break;
    case TK_TT_DNUM:
    case TK_TT_RNUM:
        *res = xpath_parse_token(pcb, nexttyp);

        if (pcb->flags & XP_FL_INSTANCEID) {
            invalid_instanceid_error(pcb);
            *res = ERR_NCX_INVALID_INSTANCEID;
            return NULL;
        }

        /* get the Number token */
        if (*res == NO_ERR) {
            val1 = new_result(pcb, XP_RT_NUMBER);
            if (!val1) {
                *res = ERR_INTERNAL_MEM;
                return NULL;
            }

            numfmt = ncx_get_numfmt(TK_CUR_VAL(pcb->tkc));
            if (numfmt == NCX_NF_OCTAL) {
                numfmt = NCX_NF_DEC;
            }
            if (numfmt == NCX_NF_DEC || numfmt == NCX_NF_REAL) {
                *res = ncx_convert_num(TK_CUR_VAL(pcb->tkc),
                                       numfmt,
                                       NCX_BT_FLOAT64,
                                       &val1->r.num);
            } else if (numfmt == NCX_NF_NONE) {
                *res = ERR_NCX_INVALID_VALUE;
            } else {
                *res = ERR_NCX_WRONG_NUMTYP;
            }
        }
        break;
    case TK_TT_QSTRING:             /* double quoted string */
    case TK_TT_SQSTRING:            /* single quoted string */
        /* get the literal token */
        *res = xpath_parse_token(pcb, nexttyp);

        if (*res == NO_ERR) {
            val1 = new_result(pcb, XP_RT_STRING);
            if (!val1) {
                *res = ERR_INTERNAL_MEM;
                return NULL;
            }

            if (TK_CUR_VAL(pcb->tkc) != NULL) {
                val1->r.str = xml_strdup(TK_CUR_VAL(pcb->tkc));
            } else {
                val1->r.str = xml_strdup(EMPTY_STRING);
            }
            if (!val1->r.str) {
                *res = ERR_INTERNAL_MEM;
                malloc_failed_error(pcb);
                xpath_free_result(val1);
                val1 = NULL;
            }
        }
        break;
    case TK_TT_TSTRING:                    /* NCName string */
        /* get the string ID token */
        nexttyp = tk_next_typ2(pcb->tkc);
        if (nexttyp == TK_TT_LPAREN) {
            val1 = parse_function_call(pcb, res);
        } else {
            *res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case TK_TT_NONE:
        /* unexpected end of token chain */
        (void)TK_ADV(pcb->tkc);
        *res = ERR_NCX_INVALID_XPATH_EXPR;
        if (pcb->logerrors) {
            log_error("\nError: token expected in XPath expression '%s'",
                      pcb->exprstr);
            /* hack to get correct error token to print */
            pcb->tkc->curerr = &pcb->tkerr;
            ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, *res);
        } else {
            ;  /**** log agent error ****/
        }
        break;
    default:
        /* unexpected token error */
        (void)TK_ADV(pcb->tkc);
        *res = ERR_NCX_WRONG_TKTYPE;
        unexpected_error(pcb);
    }

    return val1;

} /* parse_primary_expr */


/********************************************************************
* FUNCTION parse_filter_expr
* 
* Parse an XPath FilterExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [20] FilterExpr ::= PrimaryExpr       
*                     | FilterExpr Predicate    
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_filter_expr (xpath_pcb_t *pcb,
                       status_t *res)
{
    xpath_result_t  *val1, *dummy;
    tk_type_t        nexttyp;

    val1 = parse_primary_expr(pcb, res);
    if (*res != NO_ERR) {
        return val1;
    }

    /* peek ahead to check the possible next chars */
    nexttyp = tk_next_typ(pcb->tkc);

    if (val1) {
        while (nexttyp == TK_TT_LBRACK && *res==NO_ERR) {
            *res = parse_predicate(pcb, &val1);
            nexttyp = tk_next_typ(pcb->tkc);
        }
    } else if (nexttyp==TK_TT_LBRACK) {
        dummy = new_result(pcb, XP_RT_NODESET);
        if (dummy) {
            while (nexttyp == TK_TT_LBRACK && *res==NO_ERR) {
                *res = parse_predicate(pcb, &dummy);
                nexttyp = tk_next_typ(pcb->tkc);
            }
            free_result(pcb, dummy);
        } else {
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
    }

    return val1;

} /* parse_filter_expr */


/********************************************************************
* FUNCTION parse_path_expr
* 
* Parse an XPath PathExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [19] PathExpr ::= LocationPath
*                   | FilterExpr
*                   | FilterExpr '/' RelativeLocationPath
*                   | FilterExpr '//' RelativeLocationPath
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_path_expr (xpath_pcb_t *pcb,
                     status_t *res)
{
    xpath_result_t  *val1, *val2;
    const xmlChar   *nextval;
    tk_type_t        nexttyp, nexttyp2;
    xpath_exop_t     curop;

    val1 = NULL;
    val2 = NULL;

    /* peek ahead to check the possible next sequence */
    nexttyp = tk_next_typ(pcb->tkc);
    switch (nexttyp) {
    case TK_TT_FSLASH:                /* abs location path */
    case TK_TT_DBLFSLASH:     /* abbrev. abs location path */
    case TK_TT_PERIOD:                      /* abbrev step */
    case TK_TT_RANGESEP:                    /* abbrev step */
    case TK_TT_ATSIGN:                 /* abbrev axis name */
    case TK_TT_STAR:              /* rel, step, node, name */
    case TK_TT_NCNAME_STAR:       /* rel, step, node, name */
    case TK_TT_MSTRING:          /* rel, step, node, QName */
        return parse_location_path(pcb, NULL, res);
    case TK_TT_TSTRING:
        /* some sort of identifier string to check
         * get the value of the string and the following token type
         */
        nexttyp2 = tk_next_typ2(pcb->tkc);
        nextval = tk_next_val(pcb->tkc);

        /* check 'axis-name ::' sequence */
        if (nexttyp2==TK_TT_DBLCOLON && get_axis_id(nextval)) {
            /* this is an axis name */
            return parse_location_path(pcb, NULL, res);
        }               

        /* check 'NodeType (' sequence */
        if (nexttyp2==TK_TT_LPAREN && get_nodetype_id(nextval)) {
            /* this is an nodetype name */
            return parse_location_path(pcb, NULL, res);
        }

        /* check not a function call, so must be a QName */
        if (nexttyp2 != TK_TT_LPAREN) {
            /* this is an NameTest QName w/o a prefix */
            return parse_location_path(pcb, NULL, res);
        }
        break;
    default:
        ;
    }

    /* if we get here, then a filter expression is expected */
    val1 = parse_filter_expr(pcb, res);

    if (*res == NO_ERR) {
        nexttyp = tk_next_typ(pcb->tkc);
        switch (nexttyp) {
        case TK_TT_FSLASH:
            curop = XP_EXOP_FILTER1;
            break;
        case TK_TT_DBLFSLASH:
            curop = XP_EXOP_FILTER2;
            break;
        default:
            curop = XP_EXOP_NONE;
        }

        if (curop != XP_EXOP_NONE) {
            val2 = parse_location_path(pcb, val1, res);
        } else {
            val2 = val1;
        }
        val1 = NULL;
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

} /* parse_path_expr */


/********************************************************************
* FUNCTION parse_union_expr
* 
* Parse an XPath UnionExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [18] UnionExpr ::= PathExpr
*                    | UnionExpr '|' PathExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_union_expr (xpath_pcb_t *pcb,
                      status_t *res)
{
    xpath_result_t  *val1, *val2;
    boolean          done;
    tk_type_t        nexttyp;

    val1 = NULL;
    val2 = NULL;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_path_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* add all the nodes from val1 into val2
                     * that are not already present
                     */
                    merge_nodeset(pcb, val1, val2);
                    
                    if (val1) {
                        free_result(pcb, val1);
                        val1 = NULL;
                    }
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            nexttyp = tk_next_typ(pcb->tkc);
            if (nexttyp != TK_TT_BAR) {
                done = TRUE;
            } else {
                *res = xpath_parse_token(pcb, TK_TT_BAR);
                if (*res == NO_ERR) {
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        invalid_instanceid_error(pcb);
                        *res = ERR_NCX_INVALID_INSTANCEID;
                    }
                }
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

} /* parse_union_expr */


/********************************************************************
* FUNCTION parse_unary_expr
* 
* Parse an XPath UnaryExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [27] UnaryExpr ::= UnionExpr  
*                    | '-' UnaryExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_unary_expr (xpath_pcb_t *pcb,
                      status_t *res)
{
    xpath_result_t  *val1, *result;
    tk_type_t        nexttyp;
    uint32           minuscnt;

    val1 = NULL;
    minuscnt = 0;

    nexttyp = tk_next_typ(pcb->tkc);
    while (nexttyp == TK_TT_MINUS) {
        *res = xpath_parse_token(pcb, TK_TT_MINUS);
        if (*res != NO_ERR) {
            return NULL;
        } else {
            nexttyp = tk_next_typ(pcb->tkc);
            minuscnt++;
        }
    }

    val1 = parse_union_expr(pcb, res);

    if (*res == NO_ERR && (minuscnt & 1)) {
        if (pcb->val || pcb->obj) {
            /* odd number of negate ops requested */

            if (val1->restype == XP_RT_NUMBER) {
                val1->r.num.d *= -1;
                return val1;
            } else {
                result = new_result(pcb, XP_RT_NUMBER);
                if (!result) {
                    *res = ERR_INTERNAL_MEM;
                    return NULL;
                }
                xpath_cvt_number(val1, &result->r.num);
                result->r.num.d *= -1;
                free_result(pcb, val1);
                return result;
            }
        }
    }

    return val1;

} /* parse_unary_expr */


/********************************************************************
* FUNCTION parse_multiplicative_expr
* 
* Parse an XPath MultiplicativeExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [26] MultiplicativeExpr ::= UnaryExpr 
*              | MultiplicativeExpr MultiplyOperator UnaryExpr
*              | MultiplicativeExpr 'div' UnaryExpr
*              | MultiplicativeExpr 'mod' UnaryExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_multiplicative_expr (xpath_pcb_t *pcb,
                               status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    ncx_num_t        num1, num2;
    xpath_exop_t     curop;
    boolean          done;
    tk_type_t        nexttyp;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    curop = XP_EXOP_NONE;
    done = FALSE;

    ncx_init_num(&num1);
    ncx_init_num(&num2);

    while (!done && *res == NO_ERR) {
        val1 = parse_unary_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* val2 holds the 1st operand
                     * val1 holds the 2nd operand
                     */
                    if (val1->restype != XP_RT_NUMBER) {
                        xpath_cvt_number(val1, &num1);
                    } else {
                        *res = ncx_copy_num(&val1->r.num,
                                            &num1, 
                                            NCX_BT_FLOAT64);
                    }

                    if (val2->restype != XP_RT_NUMBER) {
                        xpath_cvt_number(val2, &num2);
                    } else {
                        *res = ncx_copy_num(&val2->r.num,
                                            &num2, 
                                            NCX_BT_FLOAT64);
                    }

                    if (*res == NO_ERR) {
                        result = new_result(pcb, XP_RT_NUMBER);
                        if (!result) {
                            *res = ERR_INTERNAL_MEM;
                        } else {
                            switch (curop) {
                            case XP_EXOP_MULTIPLY:
                                result->r.num.d = num2.d * num1.d;
                                break;
                            case XP_EXOP_DIV:
                                if (ncx_num_zero(&num2, 
                                                 NCX_BT_FLOAT64)) {
                                    ncx_set_num_max(&result->r.num,
                                                    NCX_BT_FLOAT64);
                                } else {
                                    result->r.num.d = num2.d / num1.d;
                                }
                                break;
                            case XP_EXOP_MOD:
                                result->r.num.d = num2.d / num1.d;
#ifdef HAS_FLOAT
                                result->r.num.d = trunc(result->r.num.d);
#endif
                                break;
                            default:
                                *res = SET_ERROR(ERR_INTERNAL_VAL);
                            }
                        }
                    }
                }

                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }
                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            nexttyp = tk_next_typ(pcb->tkc);
            switch (nexttyp) {
            case TK_TT_STAR:
                curop = XP_EXOP_MULTIPLY;
                break;
            case TK_TT_TSTRING:
                if (match_next_token(pcb, TK_TT_TSTRING,
                                     XP_OP_DIV)) {
                    curop = XP_EXOP_DIV;
                } else if (match_next_token(pcb, 
                                            TK_TT_TSTRING,
                                            XP_OP_MOD)) {
                    curop = XP_EXOP_MOD;
                } else {
                    done = TRUE;
                }
                break;
            default:
                done = TRUE;
            }
            if (!done) {
                *res = xpath_parse_token(pcb, nexttyp);
                if (*res == NO_ERR) {
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        invalid_instanceid_error(pcb);
                        *res = ERR_NCX_INVALID_INSTANCEID;
                    }
                }
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    ncx_clean_num(NCX_BT_FLOAT64, &num1);
    ncx_clean_num(NCX_BT_FLOAT64, &num2);

    return val2;

} /* parse_multiplicative_expr */


/********************************************************************
* FUNCTION parse_additive_expr
* 
* Parse an XPath AdditiveExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [25] AdditiveExpr ::= MultiplicativeExpr
*                | AdditiveExpr '+' MultiplicativeExpr
*                | AdditiveExpr '-' MultiplicativeExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_additive_expr (xpath_pcb_t *pcb,
                           status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    ncx_num_t        num1, num2;
    xpath_exop_t     curop;
    boolean          done;
    tk_type_t        nexttyp;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    curop = XP_EXOP_NONE;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_multiplicative_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                /* val2 holds the 1st operand
                 * val1 holds the 2nd operand
                 */

                if (pcb->val || pcb->val) {
                    if (val1->restype != XP_RT_NUMBER) {
                        xpath_cvt_number(val1, &num1);
                    } else {
                        *res = ncx_copy_num(&val1->r.num,
                                            &num1, 
                                            NCX_BT_FLOAT64);
                    }

                    if (val2->restype != XP_RT_NUMBER) {
                        xpath_cvt_number(val2, &num2);
                    } else {
                        *res = ncx_copy_num(&val2->r.num,
                                            &num2, 
                                            NCX_BT_FLOAT64);
                    }

                    if (*res == NO_ERR) {
                        result = new_result(pcb, XP_RT_NUMBER);
                        if (!result) {
                            *res = ERR_INTERNAL_MEM;
                        } else {
                            switch (curop) {
                            case XP_EXOP_ADD:
                                result->r.num.d = num2.d + num1.d;
                                break;
                            case XP_EXOP_SUBTRACT:
                                result->r.num.d = num2.d - num1.d;
                                break;
                            default:
                                *res = SET_ERROR(ERR_INTERNAL_VAL);
                            }
                        }
                    }
                }

                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }
                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            nexttyp = tk_next_typ(pcb->tkc);
            switch (nexttyp) {
            case TK_TT_PLUS:
                curop = XP_EXOP_ADD;
                break;
            case TK_TT_MINUS:
                curop = XP_EXOP_SUBTRACT;
                break;
            default:
                done = TRUE;
            }
            if (!done) {
                *res = xpath_parse_token(pcb, nexttyp);
                if (*res == NO_ERR) {
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        invalid_instanceid_error(pcb);
                        *res = ERR_NCX_INVALID_INSTANCEID;
                    }
                }
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    ncx_clean_num(NCX_BT_FLOAT64, &num1);
    ncx_clean_num(NCX_BT_FLOAT64, &num2);

    return val2;

} /* parse_additive_expr */


/********************************************************************
* FUNCTION parse_relational_expr
* 
* Parse an XPath RelationalExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [24]  RelationalExpr ::= AdditiveExpr
*             | RelationalExpr '<' AdditiveExpr
*             | RelationalExpr '>' AdditiveExpr
*             | RelationalExpr '<=' AdditiveExpr
*             | RelationalExpr '>=' AdditiveExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_relational_expr (xpath_pcb_t *pcb,
                           status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    xpath_exop_t     curop;
    boolean          done, cmpresult;
    tk_type_t        nexttyp;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    curop = XP_EXOP_NONE;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_additive_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* val2 holds the 1st operand
                     * val1 holds the 2nd operand
                     */
                    cmpresult = compare_results(pcb, 
                                                val2, 
                                                val1, 
                                                curop,
                                                res);

                    if (*res == NO_ERR) {
                        result = new_result(pcb, XP_RT_BOOLEAN);
                        if (!result) {
                            *res = ERR_INTERNAL_MEM;
                        } else {
                            result->r.boo = cmpresult;
                        }
                    }
                }

                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }

                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            nexttyp = tk_next_typ(pcb->tkc);
            switch (nexttyp) {
            case TK_TT_LT:
                curop = XP_EXOP_LT;
                break;
            case TK_TT_GT:
                curop = XP_EXOP_GT;
                break;
            case TK_TT_LEQUAL:
                curop = XP_EXOP_LEQUAL;
                break;
            case TK_TT_GEQUAL:
                curop = XP_EXOP_GEQUAL;
                break;
            default:
                done = TRUE;
            }
            if (!done) {
                *res = xpath_parse_token(pcb, nexttyp);
                if (*res == NO_ERR) {
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        invalid_instanceid_error(pcb);
                        *res = ERR_NCX_INVALID_INSTANCEID;
                    }
                }
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

}  /* parse_relational_expr */


/********************************************************************
* FUNCTION parse_equality_expr
* 
* Parse an XPath EqualityExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [23] EqualityExpr  ::=   RelationalExpr        
*                | EqualityExpr '=' RelationalExpr        
*                | EqualityExpr '!=' RelationalExpr        
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_equality_expr (xpath_pcb_t *pcb,
                         status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    xpath_exop_t     curop;
    boolean          done, cmpresult, equalsdone;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    curop = XP_EXOP_NONE;
    equalsdone = FALSE;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_relational_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* val2 holds the 1st operand
                     * val1 holds the 2nd operand
                     */
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        *res = check_instanceid_expr(pcb, 
                                                     val2, 
                                                     val1);
                    }

                    if (*res == NO_ERR) {
                        cmpresult = compare_results(pcb, 
                                                    val2, 
                                                    val1, 
                                                    curop,
                                                    res);

                        if (*res == NO_ERR) {
                            result = new_result(pcb, XP_RT_BOOLEAN);
                            if (!result) {
                                *res = ERR_INTERNAL_MEM;
                            } else {
                                result->r.boo = cmpresult;
                            }
                        }
                    }
                }

                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }

                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            if (match_next_token(pcb, TK_TT_EQUAL, NULL)) {
                *res = xpath_parse_token(pcb, TK_TT_EQUAL);
                if (*res == NO_ERR) {
                    if (equalsdone) {
                        if (pcb->flags & XP_FL_INSTANCEID) {
                            invalid_instanceid_error(pcb);
                            *res = ERR_NCX_INVALID_INSTANCEID;
                        }
                    } else {
                        equalsdone = TRUE;
                    }
                }
                curop = XP_EXOP_EQUAL;
            } else if (match_next_token(pcb, TK_TT_NOTEQUAL, NULL)) {
                *res = xpath_parse_token(pcb, TK_TT_NOTEQUAL);
                if (*res == NO_ERR) {
                    if (pcb->flags & XP_FL_INSTANCEID) {
                        invalid_instanceid_error(pcb);
                        *res = ERR_NCX_INVALID_INSTANCEID;
                    }
                }
                curop = XP_EXOP_NOTEQUAL;
            } else {
                done = TRUE;
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

}  /* parse_equality_expr */


/********************************************************************
* FUNCTION parse_and_expr
* 
* Parse an XPath AndExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [22] AndExpr  ::= EqualityExpr
*                   | AndExpr 'and' EqualityExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_and_expr (xpath_pcb_t *pcb,
                    status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    boolean          done, bool1, bool2;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_equality_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* val2 holds the 1st operand
                     * val1 holds the 2nd operand
                     */
                    bool1 = xpath_cvt_boolean(val1);
                    bool2 = xpath_cvt_boolean(val2);

                    result = new_result(pcb, XP_RT_BOOLEAN);
                    if (!result) {
                        *res = ERR_INTERNAL_MEM;
                    } else {
                        result->r.boo = (bool1 && bool2) 
                            ? TRUE : FALSE;
                    }
                }

                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }

                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            if (match_next_token(pcb, TK_TT_TSTRING, XP_OP_AND)) {
                *res = xpath_parse_token(pcb, TK_TT_TSTRING);
            } else {
                done = TRUE;
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

}  /* parse_and_expr */


/********************************************************************
* FUNCTION parse_or_expr
* 
* Parse an XPath OrExpr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [21] OrExpr ::= AndExpr        
*                 | OrExpr 'or' AndExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_or_expr (xpath_pcb_t *pcb,
                   status_t *res)
{
    xpath_result_t  *val1, *val2, *result;
    boolean          done, bool1, bool2;

    val1 = NULL;
    val2 = NULL;
    result = NULL;
    done = FALSE;

    while (!done && *res == NO_ERR) {
        val1 = parse_and_expr(pcb, res);

        if (*res == NO_ERR) {
            if (val2) {
                if (pcb->val || pcb->obj) {
                    /* val2 holds the 1st operand
                     * val1 holds the 2nd operand
                     */
                    bool1 = xpath_cvt_boolean(val1);
                    bool2 = xpath_cvt_boolean(val2);

                    result = new_result(pcb, XP_RT_BOOLEAN);
                    if (!result) {
                        *res = ERR_INTERNAL_MEM;
                    } else {
                        result->r.boo = (bool1 || bool2) 
                            ? TRUE : FALSE;
                    }
                }
                    
                if (val1) {
                    free_result(pcb, val1);
                    val1 = NULL;
                }

                if (val2) {
                    free_result(pcb, val2);
                    val2 = NULL;
                }

                if (result) {
                    val2 = result;
                    result = NULL;
                }
            } else {
                val2 = val1;
                val1 = NULL;
            }

            if (*res != NO_ERR) {
                continue;
            }

            if (match_next_token(pcb, TK_TT_TSTRING, XP_OP_OR)) {
                *res = xpath_parse_token(pcb, TK_TT_TSTRING);
            } else {
                done = TRUE;
            }
        }
    }

    if (val1) {
        free_result(pcb, val1);
    }

    return val2;

}  /* parse_or_expr */


/********************************************************************
* FUNCTION parse_expr
* 
* Parse an XPath Expr sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* [14] Expr ::= OrExpr
*
* INPUTS:
*    pcb == parser control block in progress
*    res == address of result status
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to malloced result struct or NULL if no
*   result processing in effect 
*********************************************************************/
static xpath_result_t *
    parse_expr (xpath_pcb_t *pcb,
                status_t  *res)
{

    return parse_or_expr(pcb, res);

}  /* parse_expr */


/********************************************************************
* FUNCTION get_context_objnode
* 
* Get the correct context node according to YANG rules
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   pointer to context object to use (probably obj)
*********************************************************************/
static obj_template_t *
    get_context_objnode (obj_template_t *obj)
{
    boolean done;

    /* get the correct context node to use */
    done = FALSE;
    while (!done) {
        if (obj->objtype == OBJ_TYP_CHOICE ||
            obj->objtype == OBJ_TYP_CASE ||
            obj->objtype == OBJ_TYP_USES) {
            if (obj->parent) {
                obj = obj->parent;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
                return obj;
            }
        } else {
            done = TRUE;
        }
    }
    return obj;

}  /* get_context_objnode */


/************    E X T E R N A L   F U N C T I O N S    ************/


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
status_t
    xpath1_parse_expr (tk_chain_t *tkc,
                       ncx_module_t *mod,
                       xpath_pcb_t *pcb,
                       xpath_source_t source)
{
    xpath_result_t  *result;
    status_t         res;
    uint32           linenum, linepos;

#ifdef DEBUG
    if (!pcb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (tkc && tkc->cur) {
        linenum = TK_CUR_LNUM(tkc);
        linepos = TK_CUR_LPOS(tkc);
    } else {
        linenum = 1;
        linepos = 1;
    }

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(mod, 
                                            pcb->exprstr, 
                                            linenum,
                                            linepos,
                                            &res);
        if (!pcb->tkc || res != NO_ERR) {
            log_error("\nError: Invalid XPath string '%s'",
                      pcb->exprstr);
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }
    }

    /* the module that contains the XPath expr is the one
     * that will always be used to resolve prefixes
     */
    pcb->tkerr.mod = mod;
    pcb->source = source;
    pcb->logerrors = TRUE;
    pcb->obj = NULL;
    pcb->objmod = NULL;
    pcb->docroot = NULL;
    pcb->doctype = XP_DOC_NONE;
    pcb->val = NULL;
    pcb->val_docroot = NULL;
    pcb->context.node.objptr = NULL;
    pcb->orig_context.node.objptr = NULL;
    pcb->parseres = NO_ERR;

    if (pcb->source == XP_SRC_INSTANCEID) {
        pcb->flags |= XP_FL_INSTANCEID;
        result = parse_location_path(pcb, NULL, &pcb->parseres);
    } else {
        result = parse_expr(pcb, &pcb->parseres);
    }

    /* since the pcb->obj is not set, this validation
     * phase will skip identifier tests, predicate tests
     * and completeness tests
     */
    if (result) {
        free_result(pcb, result);
    }

    if (pcb->parseres == NO_ERR && pcb->tkc->cur) {
        res = TK_ADV(pcb->tkc);
        if (res == NO_ERR) {
            /* should not get more tokens at this point
             * have something like '7 + 4 4 + 7'
             * and the parser stopped after the first complete
             * expression
             */
            if (pcb->source == XP_SRC_INSTANCEID) {         
                pcb->parseres = ERR_NCX_INVALID_INSTANCEID;
                if (pcb->logerrors) {
                    log_error("\nError: extra tokens in "
                              "instance-identifier '%s'",
                              pcb->exprstr);
                    ncx_print_errormsg(pcb->tkc, 
                                       pcb->tkerr.mod, 
                                       pcb->parseres);
                }
            } else {
                pcb->parseres = ERR_NCX_INVALID_XPATH_EXPR;
                if (pcb->logerrors) {
                    log_error("\nError: extra tokens in "
                              "XPath expression '%s'",
                              pcb->exprstr);
                    ncx_print_errormsg(pcb->tkc, 
                                       pcb->tkerr.mod, 
                                       pcb->parseres);
                }
            }
        }
    }

#ifdef XPATH1_PARSE_DEBUG
    if (LOGDEBUG3 && pcb->tkc) {
        log_debug3("\n\nParse chain for XPath '%s':\n",
                   pcb->exprstr);
        tk_dump_chain(pcb->tkc);
        log_debug3("\n");
    }
#endif

    /* the expression will not be processed further if the
     * parseres is other than NO_ERR
     */
    return pcb->parseres;

}  /* xpath1_parse_expr */


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
status_t
    xpath1_validate_expr_ex (ncx_module_t *mod,
                             obj_template_t *obj,
                             xpath_pcb_t *pcb,
                             boolean missing_is_error)
{
    xpath_result_t       *result;
    obj_template_t       *rootobj;
    boolean               rootdone;
 
#ifdef DEBUG
    if (!mod || !obj || !pcb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!pcb->tkc) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    pcb->objmod = mod;
    pcb->obj = obj;
    pcb->logerrors = TRUE;
    pcb->val = NULL;
    pcb->val_docroot = NULL;

    /* this is not used yet; a missing child node is always
     * a warning at this time
     */
    pcb->missing_errors = missing_is_error;

    if (pcb->source == XP_SRC_YANG && obj_is_config(obj)) {
        pcb->flags |= XP_FL_CONFIGONLY;
    }

    if (pcb->parseres != NO_ERR) {
        /* errors already reported, skip this one */
        return NO_ERR;
    }

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    pcb->context.node.objptr = get_context_objnode(obj);
    pcb->orig_context.node.objptr = pcb->context.node.objptr;

    rootdone = FALSE;
    if (obj_is_root(obj) || 
        obj_is_data_db(obj) ||
        obj_is_cli(obj)) {
        rootdone = TRUE;
        pcb->doctype = XP_DOC_DATABASE;
        pcb->docroot = ncx_get_gen_root();
        if (!pcb->docroot) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    } else if (obj_in_notif(obj)) {
        pcb->doctype = XP_DOC_NOTIFICATION;
    } else if (obj_in_rpc(obj)) {
        pcb->doctype = XP_DOC_RPC;
    } else if (obj_in_rpc_reply(obj)) {
        pcb->doctype = XP_DOC_RPC_REPLY;
    } else {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (!rootdone) {
        /* get the rpc/input, rpc/output, or /notif node */
        rootobj = obj;
        while (rootobj->parent && !obj_is_root(rootobj->parent) &&
               rootobj->objtype != OBJ_TYP_RPCIO) {
            rootobj = rootobj->parent;
        }
        pcb->docroot = rootobj;
    }

    /* validate the XPath expression against the 
     * full cooked object tree
     */
    if (pcb->source == XP_SRC_INSTANCEID) {
        result = parse_location_path(pcb, NULL, &pcb->validateres);
    } else {
        result = parse_expr(pcb, &pcb->validateres);
    }

    if (result) {
        if (LOGDEBUG3) {
            dump_result(pcb, result, "validate_expr");
        }

        free_result(pcb, result);
    }

    return pcb->validateres;

}  /* xpath1_validate_expr_ex */


/********************************************************************
* FUNCTION xpath1_validate_expr
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
*
* OUTPUTS:
*   pcb->obj and pcb->objmod are set
*   pcb->validateres is set
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath1_validate_expr (ncx_module_t *mod,
                          obj_template_t *obj,
                          xpath_pcb_t *pcb)
{
    return xpath1_validate_expr_ex(mod, obj, pcb, TRUE);
}


/********************************************************************
* FUNCTION xpath1_eval_expr
* 
* use if the prefixes are YANG: must/when
* Evaluate the expression and get the expression nodeset result
*
* INPUTS:
*    pcb == XPath parser control block to use
*    val == start context node for value of current()
*    docroot == ptr to cfg->root or top of rpc/rpc-reply/notif tree
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
xpath_result_t *
    xpath1_eval_expr (xpath_pcb_t *pcb,
                      val_value_t *val,
                      val_value_t *docroot,
                      boolean logerrors,
                      boolean configonly,
                      status_t *res)
{
    xpath_result_t *result;

    assert( pcb && "pcb is NULL" );
    assert( val && "val is NULL" );
    assert( docroot && "docroot is NULL" );
    assert( res && "res is NULL" );

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(NULL, pcb->exprstr, 1, 1, res);
        if (!pcb->tkc || *res != NO_ERR) {
            if (logerrors) {
                log_error("\nError: Invalid XPath string '%s'", pcb->exprstr);
            }
            return NULL;
        }
    }

    if (pcb->parseres != NO_ERR) {
        *res = pcb->parseres;
        return NULL;
    }

    if (pcb->validateres != NO_ERR) {
        *res = pcb->validateres;
        return NULL;
    }

    pcb->val = val;
    pcb->val_docroot = docroot;
    pcb->logerrors = logerrors;
    pcb->context.node.valptr = val;
    pcb->orig_context.node.valptr = val;

#ifdef EXTRA_DEBUG
    log_debug3("\nXPath setting context node to val: %s",
        pcb->context.node.valptr->name);
#endif

    if (configonly ||
        (pcb->source == XP_SRC_YANG && obj_is_config(val->obj))) {
        pcb->flags |= XP_FL_CONFIGONLY;
    }

    pcb->flags |= XP_FL_USEROOT;

    if (pcb->source == XP_SRC_INSTANCEID) {
        result = parse_location_path(pcb, NULL, &pcb->valueres);
    } else {
        result = parse_expr(pcb, &pcb->valueres);
    }

    if (pcb->valueres != NO_ERR) {
        *res = pcb->valueres;
    }

    if (LOGDEBUG3 && result) {
        dump_result(pcb, result, "eval_expr");
    }

    return result;

}  /* xpath1_eval_expr */


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
xpath_result_t *
    xpath1_eval_xmlexpr (xmlTextReaderPtr reader,
                         xpath_pcb_t *pcb,
                         val_value_t *val,
                         val_value_t *docroot,
                         boolean logerrors,
                         boolean configonly,
                         status_t *res)
{
    xpath_result_t *result;
    status_t        myres;

#ifdef DEBUG
    if (!pcb || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;
    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(NULL, 
                                            pcb->exprstr, 
                                            1, 
                                            1, 
                                            res);
        if (!pcb->tkc || *res != NO_ERR) {
            if (logerrors) {
                log_error("\nError: Invalid XPath string '%s'",
                          pcb->exprstr);
            }
            return NULL;
        }
    }

    pcb->obj = NULL;
    pcb->tkerr.mod = NULL;
    pcb->val = val;
    pcb->val_docroot = docroot;
    pcb->logerrors = logerrors;
    pcb->reader = reader;

    if (val) {
        pcb->context.node.valptr = val;
        pcb->orig_context.node.valptr = val;

#ifdef EXTRA_DEBUG
        if (LOGDEBUG3 && pcb->val) {
            log_debug3("\nXPath setting context node to val: %s",
                       pcb->context.node.valptr->name);
        }
#endif
    } else {

        pcb->context.node.valptr = docroot;
        pcb->orig_context.node.valptr = docroot;

#ifdef EXTRA_DEBUG
        if (LOGDEBUG3 && pcb->val) {
            log_debug3("\nXPath setting context node to docroot: %s",
                       pcb->context.node.valptr->name);
        }
#endif
    }

    if (configonly ||
        (pcb->source == XP_SRC_YANG && val && obj_is_config(val->obj))) {
        pcb->flags |= XP_FL_CONFIGONLY;
    }

    pcb->flags |= XP_FL_USEROOT;

    result = parse_expr(pcb, &pcb->valueres);

    if (pcb->valueres == NO_ERR && pcb->tkc->cur) {
        myres = TK_ADV(pcb->tkc);
        if (myres == NO_ERR) {
            pcb->valueres = ERR_NCX_INVALID_XPATH_EXPR;     
            if (pcb->logerrors) {
                log_error("\nError: extra tokens in XPath expression '%s'",
                          pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, pcb->valueres);
            }
        }
    }

    if (val && pcb->valueres == NO_ERR && result &&
        val->btyp == NCX_BT_LEAFREF) {
        pcb->valueres = check_instance_result(pcb, result);
    }

    *res = pcb->valueres;

    if (LOGDEBUG3 && result) {
        dump_result(pcb, result, "eval_xmlexpr");
    }

    return result;

}  /* xpath1_eval_xmlexpr */


/********************************************************************
* FUNCTION xpath1_get_functions_ptr
* 
* Get the start of the function array for XPath 1.0 plus
* the current() function
*
* RETURNS:
*   pointer to functions array
*********************************************************************/
const xpath_fncb_t *
    xpath1_get_functions_ptr (void)
{
    return &functions[0];

}  /* xpath1_get_functions_ptr */


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
void
    xpath1_prune_nodeset (xpath_pcb_t *pcb,
                          xpath_result_t *result)
{
    xpath_resnode_t        *resnode, *nextnode;

#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (result->restype != XP_RT_NODESET) {
        return;
    }

    if (!result->isval || !pcb->val_docroot) {
        return;
    }

    if (dlq_empty(&result->r.nodeQ)) {
        return;
    }

    /* the resnodes need to be deleted or moved to a tempQ
     * to correctly track duplicates and remove them
     */
    for (resnode = (xpath_resnode_t *)
             dlq_firstEntry(&result->r.nodeQ);
         resnode != NULL;
         resnode = nextnode) {

        nextnode = (xpath_resnode_t *)dlq_nextEntry(resnode);

        dlq_remove(resnode);

        if (xpath1_check_node_exists(pcb, 
                                     &result->r.nodeQ,
                                     resnode->node.valptr)) {
            log_debug2("\nxpath1: prune node '%s:%s'",
                       val_get_mod_name(resnode->node.valptr),
                       resnode->node.valptr->name);
            free_resnode(pcb, resnode);
        } else {
            if (nextnode) {
                dlq_insertAhead(resnode, nextnode);
            } else {
                dlq_enque(resnode, &result->r.nodeQ);
            }
        }
    }

}  /* xpath1_prune_nodeset */


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
boolean
    xpath1_check_node_exists (xpath_pcb_t *pcb,
                              dlq_hdr_t *resultQ,
                              const val_value_t *val)
{
#ifdef DEBUG
    if (!pcb || !resultQ || !val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* quick test -- see if docroot is already in the Q
     * which means nothing else is needed
     */
    if (find_resnode(pcb, 
                     resultQ,
                     pcb->val_docroot)) {
        return TRUE;
    }

    /* no docroot in the Q so check the node itself */
    if (val == pcb->val_docroot) {
        return FALSE;
    }
        
    while (val) {
        if (find_resnode(pcb, resultQ, val)) {
            return TRUE;
        }

        if (val->parent && !obj_is_root(val->parent->obj)) {
            val = val->parent;
        } else {
            return FALSE;
        }
    }
    return FALSE;

}  /* xpath1_check_node_exists */


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
boolean
    xpath1_check_node_exists_slow (xpath_pcb_t *pcb,
                                   dlq_hdr_t *resultQ,
                                   const val_value_t *val)
{
#ifdef DEBUG
    if (!pcb || !resultQ || !val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* quick test -- see if docroot is already in the Q
     * which means nothing else is needed
     */
    if (find_resnode_slow(pcb, 
                          resultQ,
                          0,
                          pcb->val_docroot->name)) {
        return TRUE;
    }

    /* no docroot in the Q so check the node itself */
    if (val == pcb->val_docroot) {
        return FALSE;
    }
        
    while (val) {
        if (find_resnode_slow(pcb, 
                              resultQ, 
                              val_get_nsid(val),
                              val->name)) {
            return TRUE;
        }

        if (val->parent && !obj_is_root(val->parent->obj)) {
            val = val->parent;
        } else {
            return FALSE;
        }
    }
    return FALSE;

}  /* xpath1_check_node_exists_slow */


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
status_t
    xpath1_stringify_nodeset (xpath_pcb_t *pcb,
                              const xpath_result_t *result,
                              xmlChar **str)
{
    xpath_resnode_t *resnode, *bestnode;
    val_value_t     *bestval;
    uint32           reslevel, bestlevel;

#ifdef DEBUG
    if (!pcb || !result || !str) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (result->restype != XP_RT_NODESET) {
        return SET_ERROR(ERR_INTERNAL_VAL); 
    }

    if (!pcb->val_docroot || !result->isval) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
        
    /* find the best node to use in the result
     * which is the first node at the lowest value nest level
     */
    reslevel = 0;
    bestlevel = NCX_MAX_UINT;
    bestnode = NULL;

    for (resnode = (xpath_resnode_t *)dlq_firstEntry(&result->r.nodeQ);
         resnode != NULL;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {

        if (resnode->node.valptr == pcb->val_docroot) {
            bestlevel = 0;
            bestnode = resnode;
        } else {
            reslevel = val_get_nest_level(resnode->node.valptr);
            if (reslevel < bestlevel) {
                bestlevel = reslevel;
                bestnode = resnode;
            }
        }
    }

    if (!bestnode) {
        *str = xml_strdup(EMPTY_STRING);
        if (!*str) {
            return ERR_INTERNAL_MEM;
        } else {
            return NO_ERR;
        }
    }

    /* If the node == val_docroot
     * DO NOT use the entire contents of the 
     * database in the string!!!
     * Since multiple documents are supported
     * under one root in NETCONF, just grab the
     * first child no matter what node it is
     */
    bestval = bestnode->node.valptr;

    return xpath1_stringify_node(pcb, bestval, str);

}  /* xpath1_stringify_nodeset */


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
status_t
    xpath1_stringify_node (xpath_pcb_t *pcb,
                           val_value_t *val,
                           xmlChar **str)
{
    status_t                   res;
    uint32                     cnt;
    boolean                    cfgonly;
    xpath_stringwalkerparms_t  walkerparms;

#ifdef DEBUG
    if (!pcb || !val || !str) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val == pcb->val_docroot) {
        val = val_get_first_child(val);
    }

    if (!val) {
        *str = xml_strdup(EMPTY_STRING);
        if (!*str) {
            return ERR_INTERNAL_MEM;
        } else {
            return NO_ERR;
        }
    }

    if (typ_is_simple(val->btyp)) {
        res = val_sprintf_simval_nc(NULL, val, &cnt);
        if (res != NO_ERR) {
            return res;
        }
        *str = m__getMem(cnt+1);
        if (!*str) {
            return ERR_INTERNAL_MEM;
        }
        res = val_sprintf_simval_nc(*str, val, &cnt);
        if (res != NO_ERR) {
            m__free(*str);
            *str = NULL;
            return res;
        }
    } else {
        cfgonly = (pcb->flags & XP_FL_CONFIGONLY) ? TRUE : FALSE;
        walkerparms.buffer = NULL;
        walkerparms.buffsize = 0;
        walkerparms.buffpos = 0;
        walkerparms.res = NO_ERR;

        /* first walk to get the buffer size
         * don't care about result; just need to check
         * walkerparms.res
         */
        (void)val_find_all_descendants(stringify_walker_fn,
                                       pcb,
                                       &walkerparms,
                                       val,
                                       NULL,  /* modname */
                                       NULL,  /* name */
                                       cfgonly,
                                       FALSE,  /* textmode */
                                       TRUE,   /* orself */
                                       TRUE);  /* forceall */
        if (walkerparms.res != NO_ERR) {
            return walkerparms.res;
        }

        walkerparms.buffer = m__getMem(walkerparms.buffpos+2);
        if (!walkerparms.buffer) {
            return ERR_INTERNAL_MEM;
        }
        walkerparms.buffsize = walkerparms.buffpos+2;
        walkerparms.buffpos = 0;

        /* second walk to fill in the buffer
         * don't care about result; just need to check
         * walkerparms.res
         */
        (void)val_find_all_descendants(stringify_walker_fn,
                                       pcb,
                                       &walkerparms,
                                       val,
                                       NULL,  /* modname */
                                       NULL,  /* name */
                                       cfgonly,
                                       FALSE,  /* textmode */
                                       TRUE,   /* orself */
                                       TRUE);  /* forceall */
        if (walkerparms.res != NO_ERR) {
            m__free(walkerparms.buffer);
            return walkerparms.res;
        }

        *str = walkerparms.buffer;
    }
    return NO_ERR;

}  /* xpath1_stringify_node */


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
boolean
    xpath1_compare_result_to_string (xpath_pcb_t *pcb,
                                     xpath_result_t *result,
                                     xmlChar *strval,
                                     status_t *res)
{
    xpath_result_t   sresult;
    boolean          retval;

    /* only compare real results, not objects */
    if (!pcb->val) {
        return TRUE;
    }

    *res = NO_ERR;
    xpath_init_result(&sresult, XP_RT_STRING);
    sresult.r.str = strval;

    retval = compare_results(pcb, 
                             result, 
                             &sresult,
                             XP_EXOP_EQUAL, 
                             res);
    
    sresult.r.str = NULL;
    xpath_clean_result(&sresult);

    return retval;

}  /* xpath1_compare_result_to_string */


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
boolean
    xpath1_compare_result_to_number (xpath_pcb_t *pcb,
                                     xpath_result_t *result,
                                     ncx_num_t *numval,
                                     status_t *res)
{
    xpath_result_t   sresult;
    boolean          retval;

    /* only compare real results, not objects */
    if (!pcb->val) {
        return TRUE;
    }

    *res = NO_ERR;
    xpath_init_result(&sresult, XP_RT_NUMBER);
    ncx_copy_num(numval, &sresult.r.num, NCX_BT_FLOAT64);

    retval = compare_results(pcb, 
                             result, 
                             &sresult,
                             XP_EXOP_EQUAL, 
                             res);
    
    xpath_clean_result(&sresult);

    return retval;

}  /* xpath1_compare_result_to_number */


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
boolean
    xpath1_compare_nodeset_results (xpath_pcb_t *pcb,
                                    xpath_result_t *result1,
                                    xpath_result_t *result2,
                                    status_t *res)
{
    /* only compare real results, not objects */
    if (!pcb->val) {
        return TRUE;
    }

    *res = NO_ERR;
    boolean retval = compare_results(pcb, result1, result2,
                                     XP_EXOP_EQUAL, res);
    return retval;

}  /* xpath1_compare_nodeset_results */


/* END xpath1.c */
