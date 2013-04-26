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
/*  FILE: val_util.c

   val_value_t struct utilities for object validateion support
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
19dec05      abb      begun
21jul08      abb      start obj-based rewrite

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include <xmlstring.h>

#include "procdefs.h"
#include "cfg.h"
#include "cli.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncx_feature.h"
#include "ncx_list.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "status.h"
#include "typ.h"
#include "val.h"
#include "val_util.h"
#include "xml_util.h"
#include "xpath.h"
#include "xpath1.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
/* #define VAL_UTIL_DEBUG_CANONICAL 1 */

/********************************************************************
* FUNCTION new_index
* 
* Malloc and initialize the fields in a val_index_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static val_index_t * 
    new_index (val_value_t *valnode)
{
    val_index_t  *in;

    in = m__getObj(val_index_t);
    if (!in) {
        return NULL;
    }
    in->val = valnode;
    return in;

}  /* new_index */


/********************************************************************
* FUNCTION choice_check
* 
* Check a val_value_t struct against its expected OBJ
* for instance validation:
*
*    - choice validation: 
*      only one case allowed if the data type is choice
*      Only issue errors based on the instance test qualifiers
*
* The input is checked against the specified obj_template_t.
*
* INPUTS:
*   val == parent val_value_t to check
*   choicobj == object template for the choice to check
*
* OUTPUTS:
*   log any errors encountered
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    choice_check (val_value_t *val,
                  obj_template_t *choicobj)
{
    val_value_t           *chval, *testval;
    status_t               res, retres;

    retres = NO_ERR;

    /* Go through all the child nodes for this object
     * and look for choices against the value set to see if each 
     * a choice case is present in the correct number of instances.
     *
     * The current value could never be a OBJ_TYP_CHOICE since
     * those nodes are not stored in the val_value_t tree
     * Instead, it is the parent of the choice object,
     * and the accessible case nodes will be child nodes
     * of that complex parent type
     */
    chval = val_get_choice_first_set(val, choicobj);
    if (!chval) {
        if (obj_is_mandatory(choicobj)) {
            /* error missing choice */
            retres = ERR_NCX_MISSING_CHOICE;
            log_error("\nError: Nothing selected for "
                      "mandatory choice '%s'",
                      obj_get_name(choicobj));
            ncx_print_errormsg(NULL, NULL, retres);
        }
        return retres;
    }

    /* else a choice was selected
     * first make sure all the mandatory case 
     * objects are present
     */
    res = val_instance_check(val, chval->casobj);
    if (res != NO_ERR) {
        retres = res;
    }

    /* check if any objects from other cases are present */
    testval = val_get_choice_next_set(choicobj, chval);
    while (testval) {
        if ((testval->casobj != chval->casobj) &&
            (testval->casobj->parent == chval->casobj->parent)) {
            /* error: extra case object in this choice */
            retres = ERR_NCX_EXTRA_CHOICE;
            log_error("\nError: Extra object '%s' "
                      "in choice '%s'; Case '%s' already selected", 
                      testval->name,
                      obj_get_name(choicobj),
                      obj_get_name(chval->casobj));
            ncx_print_errormsg(NULL, NULL, retres);
        }
        testval = val_get_choice_next_set(choicobj, testval);
    }
    return retres;

}  /* choice_check */


/********************************************************************
 * FUNCTION add_defaults
 * 
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
 *   addcas == obj_template for OBJ_TYP_CASE when adding defaults
 *             to a case
 *
 * OUTPUTS:
 *   *val and any sub-nodes are set to the default value as requested
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    add_defaults (val_value_t *val,
                  val_value_t *rootval,
                  val_value_t *cxtval,
                  boolean scriptmode,
                  obj_template_t *addcas)
{
#ifdef DEBUG
    /* test in static fn because it is so recursive */
    if (!val || !val->obj) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    obj_template_t *obj, *chobj;

    if (addcas) {
        obj = addcas;
    } else {
        obj = val->obj;
    }

    status_t res = NO_ERR;

    /* skip any uses or augment nodes */
    if (!obj_has_name(obj)) {
        return NO_ERR;
    }

    /* go through each child object node and determine
     * if a child value node exists for it or not
     *
     * If not, then determine if a default is needed
     * and available
     *
     * Objects without children will drop through the loop
     * All the uses and augment nodes will be skipped,
     */
    for (chobj = obj_first_child(obj);
         chobj != NULL && res == NO_ERR;
         chobj = obj_next_child(chobj)) {

        const xmlChar *defval = NULL;
        val_value_t *chval = NULL, *testval = NULL, *chcxtval = NULL;
        obj_template_t *casobj = NULL;
        uint32 whencount = 0;
        boolean condresult = FALSE;

        switch (chobj->objtype) {
        case OBJ_TYP_ANYXML:
            break;
        case OBJ_TYP_LEAF:
            /* first check if this leaf even has a default */
            defval = obj_get_default(chobj);
            if (!defval) {
                continue;
            }

            /* check if the child leaf is a config node */
            if (!obj_is_config(chobj)) {
                continue;
            }

            /* check if this leaf has a false when-stmt associated
             * with it; skip this leaf if when FALSE
             */
            if (rootval) {
                res = val_check_obj_when((cxtval) ? cxtval : val, 
                                         rootval, NULL, chobj,
                                         &condresult, &whencount);
                if (res != NO_ERR) {
                    return res;
                }
                if (whencount && !condresult) {
                    if (LOGDEBUG3) {
                        log_debug3("\nadd_default: skipping false when '%s:%s'",
                                   obj_get_mod_name(chobj),
                                   obj_get_name(chobj));
                    }
                    continue;
                }
            }

            /* node is not conditional or when=TRUE
             * check if the node exists alread
             */
            chval = val_find_child(val, obj_get_mod_name(chobj),
                                   obj_get_name(chobj));
            if (!chval) {
                res = cli_parse_parm(NULL, val, chobj, defval, scriptmode);
                if (res==NO_ERR) {
                    chval = val_find_child(val, obj_get_mod_name(chobj),
                                           obj_get_name(chobj));
                    if (!chval) {
                        SET_ERROR(ERR_INTERNAL_VAL);
                    } else {
                        if (LOGDEBUG4) {
                            log_debug4("\nadd default leaf '%s:%s'",
                                       val_get_mod_name(chval),
                                       chval->name);
                        }
                        chval->flags |= VAL_FL_DEFSET;
                    }
                }
            }
            break;
        case OBJ_TYP_CHOICE:
            /* if the choice is not set, and it has a default
             * case, then add that case to the val struct
             * If a partial case is present, then try to fill in 
             * any of the nodes with defaults
             */
            if (obj_is_mandatory(chobj)) {
                break;
            }

            /* get the default case for this choice (if any) */
            casobj = obj_get_default_case(chobj);

            /* check if the choice has been set at all */
            testval = val_get_choice_first_set(val, chobj);
            if (testval) {
                /* use the selected case instead of the default case */
                casobj = testval->casobj;
                if (!casobj) {
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                }
            }
            if (casobj) {
                /* add all the default nodes in the default case
                 * or selected case 
                 */
                res = add_defaults(val, rootval, cxtval, scriptmode, casobj);
            }
            break;
        case OBJ_TYP_LEAF_LIST:
            /* leaf list objects never get default entries */
            break;
        case OBJ_TYP_CONTAINER:
            if (obj_is_root(chobj)) {
                break;
            }
            /* else fall through */
        case OBJ_TYP_RPCIO:
        case OBJ_TYP_LIST:
        case OBJ_TYP_CASE:
            /* add defaults to the subtrees of existing
             * complex nodes, but do not add any new ones
             */
            chval = val_find_child(val, 
                                   obj_get_mod_name(chobj),
                                   obj_get_name(chobj));
            if (chval) {
                if (cxtval) {
                    chcxtval = val_first_child_match(cxtval, chval);
                }
                res = add_defaults(chval, rootval, chcxtval, scriptmode, NULL);
            }

            if (chobj->objtype == OBJ_TYP_LIST) {
                while (res == NO_ERR && chval) {
                    chval = val_find_next_child(val,
                                                obj_get_mod_name(chobj),
                                                obj_get_name(chobj),
                                                chval);
                    if (chval) {
                        if (chcxtval) {
                            chcxtval = val_next_child_match(cxtval, chval,
                                                            chcxtval);
                        }
                        res = add_defaults(chval, rootval, chcxtval, 
                                           scriptmode, NULL);
                    }
                }
            }
            break;
        case OBJ_TYP_RPC:
        case OBJ_TYP_NOTIF:
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    return res;

} /* add_defaults */


/********************************************************************
* FUNCTION get_index_comp
* 
* Get the index component string for the specified value
* 
* USAGE:
*  call 1st time with a NULL buffer to get the length
*  call the 2nd time with a buffer of the returned length
*
* !!!! DOES NOT CHECK BUFF OVERRUN IF buff is non-NULL !!!!
*
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   format == desired output format
*   val == val_value_t for table or container
*   buff = buffer to hold result; NULL == get length only
*   
* OUTPUTS:
*   mhdr.pmap may have entries added if prefixes used
*      in the instance identifier which are not already in the pmap
*   *len = number of bytes that were (or would have been) written 
*          to buff
* RETURNS:
*   status
*********************************************************************/
static status_t
    get_index_comp (xml_msg_hdr_t *mhdr,
                    ncx_instfmt_t format,
                    const val_value_t *val,
                    xmlChar *buff,
                    uint32  *len)
{
    const xmlChar      *prefix;
    uint32              cnt, total;
    status_t            res;
    boolean             quotes;

    total = 0;

    /* get the data type to determine if a quoted string is needed */
    if (format==NCX_IFMT_XPATH1 || format==NCX_IFMT_XPATH2) {
        quotes = TRUE;
    } else if (typ_is_string(val->btyp)) {
        quotes = (format==NCX_IFMT_CLI) ?
            val_need_quotes(VAL_STR(val)) : TRUE;
    } else if (typ_is_number(val->btyp)) {
        quotes = FALSE;
    } else {
        switch (val->btyp) {
        case NCX_BT_ENUM:
            quotes = val_need_quotes(VAL_ENUM_NAME(val));
            break;
        case NCX_BT_BOOLEAN:
            quotes = FALSE;
            break;
        case NCX_BT_BINARY:
        case NCX_BT_BITS:
            quotes = TRUE;
            break;
        case NCX_BT_UNION:
            quotes = TRUE;
            break;
        case NCX_BT_IDREF:
            quotes = FALSE;
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    /* check if foo:parmname='parmval' format is needed */
    if (format==NCX_IFMT_XPATH1 || format==NCX_IFMT_XPATH2) {
        if (mhdr) {
            prefix = xml_msg_get_prefix_xpath(mhdr, val->nsid);
        } else {
            prefix = xmlns_get_ns_prefix(val->nsid);
        }
        if (!prefix) {
            return ERR_INTERNAL_MEM;
        }

        if (buff) {
            buff += xml_strcpy(buff, prefix);
            *buff++ = ':';
        }
        total += xml_strlen(prefix);
        total++;

        if (buff) {
            buff += xml_strcpy(buff, val->name);
        }
        total += xml_strlen(val->name);

        if (buff) {
            *buff++ = VAL_EQUAL_CH;
        }
        total++;

    }

    if (quotes) {
        if (buff) {
            *buff++ = (xmlChar)((format==NCX_IFMT_XPATH1) ?
                                VAL_QUOTE_CH : VAL_DBLQUOTE_CH);
        }
        total++;  
    }

    res = val_sprintf_simval_nc(buff, val, &cnt);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }
    if (buff) {
        buff += cnt;
    }
    total += cnt;

    if (quotes) {
        if (buff) {
            *buff++ = (xmlChar)((format==NCX_IFMT_XPATH1) ?
                                VAL_QUOTE_CH : VAL_DBLQUOTE_CH);
        }
        total++;  
    }

    /* end the string */
    if (buff) {
        *buff = 0;
    }

    *len = total;
    return NO_ERR;

}  /* get_index_comp */


/********************************************************************
* FUNCTION get_instance_string
* 
* Get the instance ID string for the specified value
* 
* USAGE:
*  call 1st time with a NULL buffer to get the length
*  call the 2nd time with a buffer of the returned length
*
* !!!! DOES NOT CHECK BUFF OVERRUN IF buff is non-NULL !!!!
*
* INPUTS:
*   mhdr == message hdr w/ prefix map or NULL to just use
*           the internal prefix mappings
*   format == desired output format
*   val == value node to generate instance string for
*   stop_at_root == TRUE to stop if a 'root' node is encountered
*                == FALSE to keep recursing all the way to 
*   buff = buffer to hold result; NULL == get length only
*   len == address of return length
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
static status_t
    get_instance_string (xml_msg_hdr_t *mhdr,
                         ncx_instfmt_t format,
                         const val_value_t *val,
                         boolean stop_at_root,
                         xmlChar *buff,
                         uint32  *len)
{
    const xmlChar      *name = NULL, *prefix = NULL, *ncprefix = NULL;
    xmlChar             numbuff[NCX_MAX_NUMLEN+1];
    uint32              cnt = 0, childcnt = 0, total = 0;
    status_t            res = NO_ERR;
    boolean             root = FALSE, skiproot = FALSE;
    xmlns_id_t          rpcid = 0;

    /* process the specific node type 
     * Recurively find the top node and start there
     */
    if (stop_at_root && val->obj && obj_is_root(val->obj)) {
        root = TRUE;
        skiproot = TRUE;
    } else if (val->parent && val->parent->obj) {
        if (stop_at_root && obj_is_root(val->parent->obj)) {
            root = TRUE;
        }
    } else {
        root = TRUE;
    }
    if (!root) {
        res = get_instance_string(mhdr, format, val->parent,
                                   stop_at_root, buff, &cnt);
    }

    *len = 0;

    if (res != NO_ERR) {
        return res;
    }

    /* make sure not to generate an i-i that starts with /nc:config */
    if (val->obj && obj_is_root(val->obj) && val->parent == NULL) {
        return NO_ERR;
    }

    /* move the buffer pointer to the end to append */
    if (buff) {
        buff += cnt;
    }

    if (val->obj->objtype == OBJ_TYP_RPCIO) {
        /* get the prefix and name of the RPC method 
         * instead of this node named 'input'
         */
        rpcid = obj_get_nsid(val->obj->parent);
        if (rpcid) {
            if (mhdr) {
                prefix = xml_msg_get_prefix_xpath(mhdr, rpcid);
            } else {
                prefix = xmlns_get_ns_prefix(rpcid);
            }
        }
        name = obj_get_name(val->obj->parent);
    } else {
        /* make sure the prefix is in the message header so
         * an xmlns directive will be generated for this prefix
         */
        if (mhdr) {
            prefix = xml_msg_get_prefix_xpath(mhdr, val->nsid);
        } else {
            prefix = xmlns_get_ns_prefix(val->nsid);
        }
        name = val->name;
    }

#ifdef DEBUG
    if (!prefix || !*prefix || !name || !*name) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* check if a path sep char is needed */
    if (format == NCX_IFMT_C) {
        if (cnt) {
            if (buff) {
                *buff++ = VAL_INST_SEPCH;
            }
            cnt++;
        }
    } else {
        if (buff) {
            *buff++ = VAL_XPATH_SEPCH;   /*   starting '/' */
        }
        cnt++;
    }

    /* check if the 'rpc' root element needs to be added here */
    if (root && val->obj->objtype == OBJ_TYP_RPCIO) {
        /* copy prefix */
        if (mhdr) {
            ncprefix = xml_msg_get_prefix_xpath(mhdr, xmlns_nc_id());
            if (!ncprefix) {
                return ERR_INTERNAL_MEM;
            }
        } else {
            ncprefix = xmlns_get_ns_prefix(xmlns_nc_id());
            if (!ncprefix) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        if (buff) {
            buff += xml_strcpy(buff, ncprefix);
            *buff++ = ':';
        }
        cnt += xml_strlen(ncprefix);
        cnt++;

        /* copy name */
        if (buff) {
            buff += xml_strcpy(buff, NCX_EL_RPC);
        }
        cnt += xml_strlen(NCX_EL_RPC);

        /* add another path sep char */
        if (buff) {
            *buff++ = (xmlChar)((format==NCX_IFMT_C) ? 
                                VAL_INST_SEPCH : VAL_XPATH_SEPCH);
        }
        cnt++;
    }


    /* add prefix string for this component */
    if (buff && !skiproot) {
        buff += xml_strcpy(buff, prefix);
        *buff++ = ':';
    }
    if (!skiproot) {
        cnt += xml_strlen(prefix);
        cnt++;
    }

    /* add name string for this component */
    if (buff && !skiproot) {
        buff += xml_strcpy(buff, name);
    }
    if (!skiproot) {
        cnt += xml_strlen(name);
    }

    total = cnt;

    /* check if this is a value node with an index clause */
    if (!dlq_empty(&val->indexQ)) {
        cnt = 0;
        res = val_get_index_string(mhdr, format, val, buff, &cnt);
        if (res == NO_ERR) {
            if (buff) {
                buff[cnt] = 0;
            }
            total += cnt;
        }
    } else if (val->parent) {
        childcnt = val_child_inst_cnt(val->parent,
                                      val_get_mod_name(val),
                                      val->name);
        if (childcnt > 1) {     
            /* there are multiple unnamed instances, so force
             * an instance ID on any of them
             */
            cnt = val_get_child_inst_id(val->parent, val);
            if (cnt > 0) {
                /* add an instance ID like foo[3] */
                if (buff) {
                    *buff++ = '[';
                }
                total++;

                snprintf((char *)numbuff, sizeof(numbuff), "%u", cnt);

                if (buff) {
                    buff += xml_strcpy(buff, numbuff);
                }
                total += xml_strlen(numbuff);

                if (buff) {
                    *buff++ = ']';
                }
                total++;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }

    /* set the length even if index error, and exit */
    *len = total;
    return res;

}  /* get_instance_string */


/********************************************************************
* FUNCTION purge_errors
* 
* Remove any error nodes recursively
* Need to use raw Q access to find nodes marked deleted
* INPUTS:
*   val == node to purge
*
*********************************************************************/
static void
    purge_errors (val_value_t *val)
{

    if (!typ_has_children(val->btyp)) {
        return;
    }

    val_value_t *nextval;
    val_value_t *chval = (val_value_t *)dlq_firstEntry(&val->v.childQ);
    for (; chval != NULL; chval = nextval) {

        nextval = (val_value_t *)dlq_nextEntry(chval);

        if (chval->res != NO_ERR) {
            log_debug("\nDeleting error node '%s:%s' (%s)",
                      val_get_mod_name(chval), chval->name,
                      get_error_string(chval->res));
            if (obj_is_key(chval->obj)) {
                val_remove_key(chval);
            }
            val_remove_child(chval);
            val_free_value(chval);
        } else if (typ_has_children(chval->btyp)) {
            purge_errors(chval);
        }
    }

}  /* purge_errors */


/********************************************************************
* FUNCTION check_when_stmt
* 
* Check if the specified when-stmt for the specified node
*
* INPUTS:
*   val == parent value node of the object node to check
*   valroot == database root for XPath purposes
*   context == value node to use for context node
*   whenstmt == XPath PCB to use
*   condresult == address of conditional test result
*   
* OUTPUTS:
*   *condresult == TRUE if conditional is true or there are none
*                  FALSE if conditional test failed
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_when_stmt (val_value_t *val,
                     val_value_t *valroot,
                     val_value_t *context,
                     xpath_pcb_t *whenstmt,
                     boolean *condresult)
{
    status_t res = NO_ERR;
    xpath_result_t *result = 
        xpath1_eval_expr(whenstmt, context, valroot, FALSE /* logerrors */, 
                         TRUE /* configonly */, &res);
    if (result == NULL || res != NO_ERR) {
        *condresult = FALSE;
    } else {
        boolean whentest = xpath_cvt_boolean(result);
        *condresult = whentest;
        if (LOGDEBUG3) {
            if (whentest) {
                log_debug3("\nval: when test '%s' OK for node '%s' with "
                           "context '%s'", whenstmt->exprstr, 
                           val->name, context->name);
            } else {
                log_debug3("\nval: when test '%s' failed for node '%s' "
                           "with context '%s'", whenstmt->exprstr, 
                           val->name, context->name);
            }
        }
    }
    xpath_free_result(result);

    return res;

} /* check_when_stmt */


/*************** E X T E R N A L    F U N C T I O N S  *************/


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
void
    val_set_canonical_order (val_value_t *val)
{
    obj_template_t        *chobj;
    const obj_key_t       *key;
    val_value_t           *chval;
    dlq_hdr_t              tempQ;

    assert( val && "val is NULL!" );
    assert( val->obj && "val->obj is NULL!" );

    if (val_is_virtual(val)) {
        return;
    }

    boolean skip = FALSE;
    switch (val->obj->objtype) {
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        skip = TRUE;
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPC:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    case OBJ_TYP_LIST:
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_RPCIO:
    case OBJ_TYP_NOTIF:
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    if (skip) {
#ifdef VAL_UTIL_DEBUG_CANONICAL
        log_debug3("\nval_canonical skip '%s'", val->name);
#endif
        return;
    }

#ifdef VAL_UTIL_DEBUG_CANONICAL
    log_debug3("\nval_canonical start '%s'", val->name);
    if (LOGDEBUG4) {
        val_dump_value(val, 0);
    }
#endif

    /* transfer all the val->childQ nodes to the tempQ */
    dlq_createSQue(&tempQ);
    dlq_block_enque(&val->v.childQ, &tempQ);

    switch (val->obj->objtype) {
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        break;
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPC:
        SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case OBJ_TYP_LIST:
        /* for a list, put all the key leafs first */
        for (key = obj_first_ckey(val->obj);
             key != NULL;
             key = obj_next_ckey(key)) {

            chval = val_find_child_que(&tempQ, obj_get_mod_name(key->keyobj),
                                       obj_get_name(key->keyobj));
            if (chval) {
                dlq_remove(chval);
                val_add_child(chval, val);
            }
        }
        /* fall through to do the rest of the child nodes */
    case OBJ_TYP_CONTAINER:
        if (obj_is_root(val->obj)) {
            while (!dlq_empty(&tempQ)) {
                chval = (val_value_t *)dlq_deque(&tempQ);
                val_add_child_sorted(chval, val);
                val_set_canonical_order(chval);
            }
            break;
        }
        /* else fall through to normal container or list case */
    case OBJ_TYP_RPCIO:
    case OBJ_TYP_NOTIF:
        for (chobj = obj_first_child_deep(val->obj);
             chobj != NULL;
             chobj = obj_next_child_deep(chobj)) {

            if (obj_is_key(chobj)) {
                continue;
            }

            chval = val_find_child_que(&tempQ, obj_get_mod_name(chobj),
                                       obj_get_name(chobj));
            while (chval) {
                dlq_remove(chval);
                val_add_child_sorted(chval, val);

                switch (chval->obj->objtype) {
                case OBJ_TYP_LEAF:
                case OBJ_TYP_LEAF_LIST:
                    break;
                case OBJ_TYP_CONTAINER:
                    if (obj_is_root(chval->obj)) {
                        break;
                    } /* else fall through */
                case OBJ_TYP_LIST:
                    val_set_canonical_order(chval);
                    break;
                default:
                    ;
                }
                chval = val_find_child_que(&tempQ, obj_get_mod_name(chobj),
                                           obj_get_name(chobj));
            }
        }

        /* check left over nodes */
        if (dlq_count(&tempQ)) {
            /* could be some error nodes left over not
             * part of the schema definition
             */
            log_debug("\nset_canonical: %d leftover nodes added "
                      " to end of childQ for val %s",
                      dlq_count(&tempQ), val->name);
            dlq_block_enque(&tempQ, &val->v.childQ);
        }

        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

#ifdef VAL_UTIL_DEBUG_CANONICAL
    log_debug3("\nval_canonical end '%s'", val->name);
    //log_debug4("\n   tempQ count is %u", dlq_count(&tempQ));
    if (LOGDEBUG4) {
        val_dump_value(val, 0);
    }
#endif

}  /* val_set_canonical_order */


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
status_t 
    val_gen_index_comp  (const obj_key_t *in,
                         val_value_t *val)
{
    val_value_t       *chval;
    status_t           res;
    boolean            found;

#ifdef DEBUG
    if (!in || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* 1 or more index components expected */
    found = FALSE;
    res = NO_ERR;

    for (chval = (val_value_t *)dlq_firstEntry(&val->v.childQ);
         chval != NULL && !found && res==NO_ERR;
         chval = (val_value_t *)dlq_nextEntry(chval)) {

        if (chval->index) {
            continue;
        } else if (val_get_nsid(chval) != obj_get_nsid(in->keyobj)) {
            continue;
        } else if (!xml_strcmp(obj_get_name(in->keyobj), 
                               chval->name)) {
            res = val_gen_key_entry(chval);
            if (res == NO_ERR) {
                found = TRUE;
            }
        }
    }

    if (res == NO_ERR && !found) {
        res = ERR_NCX_MISSING_INDEX;
    }

    return res;

}  /* val_gen_index_comp */


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
status_t 
    val_gen_key_entry  (val_value_t *keyval)
{
    val_index_t       *valin;
    status_t           res;

#ifdef DEBUG
    if (!keyval || !keyval->parent) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    valin = new_index(keyval);
    if (!valin) {
        res = ERR_INTERNAL_MEM;
    } else {
        /* save the index marker record */
        keyval->index = valin;
        dlq_enque(valin, &keyval->parent->indexQ);
    }

    return res;

}  /* val_gen_key_entry */


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
status_t 
    val_gen_index_chain (const obj_template_t *obj,
                         val_value_t *val)
{
    const obj_key_t       *key;
    status_t               res;

#ifdef DEBUG
    if (!obj || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (obj->objtype != OBJ_TYP_LIST) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* 0 or more index components expected */
    for (key = obj_first_ckey(obj);
         key != NULL;
         key = obj_next_ckey(key)) {
        res = val_gen_index_comp(key, val);
        if (res != NO_ERR) {
            return res;
        }
    }

    return NO_ERR;

} /* val_gen_index_chain */


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
status_t 
    val_add_defaults (val_value_t *val,
                      val_value_t *rootval,
                      val_value_t *cxtval,
                      boolean scriptmode)
{
    return add_defaults(val, rootval, cxtval, scriptmode, NULL);

} /* val_add_defaults */


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
status_t
    val_instance_check (val_value_t  *val,
                        obj_template_t *obj)
{
    obj_template_t        *chobj;
    ncx_iqual_t            iqual;
    uint32                 cnt, minelems, maxelems;
    boolean                minset, maxset, minerr, maxerr;
    status_t               res, retres;

    retres = NO_ERR;

    /* check all the child nodes for correct number of instances */
    for (chobj = obj_first_child(obj);
         chobj != NULL;
         chobj = obj_next_child(chobj)) {

        iqual = obj_get_iqualval(chobj);
        minerr = FALSE;
        maxerr = FALSE;
        minelems = 0;
        maxelems = 0;

        switch (chobj->objtype) {
        case OBJ_TYP_LEAF_LIST:
            minset = chobj->def.leaflist->minset;
            minelems = chobj->def.leaflist->minelems;
            maxset = chobj->def.leaflist->maxset;
            maxelems = chobj->def.leaflist->maxelems;
            break;
        case OBJ_TYP_LIST:
            minset = chobj->def.list->minset;
            minelems = chobj->def.list->minelems;
            maxset = chobj->def.list->maxset;
            maxelems = chobj->def.list->maxelems;
            break;
        default:
            minset = FALSE;
            maxset = FALSE;
        }

        switch (chobj->objtype) {
        case OBJ_TYP_CHOICE:
            res = choice_check(val, chobj);
            if (res != NO_ERR) {
                retres = res;
            }
            continue;
        case OBJ_TYP_CASE:
            retres = SET_ERROR(ERR_INTERNAL_VAL);
            continue;
        default:
            cnt = val_instance_count(val, 
                                     obj_get_mod_name(chobj),
                                     obj_get_name(chobj));
        }

        if (minset) {
            if (cnt < minelems) {
                /* not enough instances error */
                minerr = TRUE;
                retres = ERR_NCX_MISSING_VAL_INST;
                log_error("\nError: Not enough instances of object '%s' "
                          "Got '%u', needed '%u'",
                          obj_get_name(chobj), 
                          cnt, 
                          minelems);
                ncx_print_errormsg(NULL, NULL, retres);
            }
        }

        if (maxset) {
            if (cnt > maxelems) {
                /* too many instances error */
                maxerr = TRUE;
                retres = ERR_NCX_EXTRA_VAL_INST;
                log_error("\nError: Too many instances of object '%s' entered "
                          "Got '%u', allowed '%u'",
                          obj_get_name(chobj), 
                          cnt, 
                          maxelems);
                ncx_print_errormsg(NULL, NULL, retres);
            }
        }

        switch (iqual) {
        case NCX_IQUAL_ONE:
            if (cnt < 1 && !minerr) {
                /* missing single parameter */
                retres = ERR_NCX_MISSING_VAL_INST;
                log_error("\nError: Mandatory object '%s' is missing",
                          obj_get_name(chobj));
                ncx_print_errormsg(NULL, NULL, retres);
            } else if (cnt > 1 && !maxerr) {
                /* too many parameters */
                retres = ERR_NCX_EXTRA_VAL_INST;
                log_error("\nError: Extra instances of object '%s' entered",
                          obj_get_name(chobj));
                ncx_print_errormsg(NULL, NULL, retres);
            }
            break;
        case NCX_IQUAL_OPT:
            if (cnt > 1 && !maxerr) {
                /* too many parameters */
                retres = ERR_NCX_EXTRA_VAL_INST;
                log_error("\nError: Extra instances of object '%s' entered",
                          obj_get_name(chobj));
                ncx_print_errormsg(NULL, NULL, retres);
            }
            break;
        case NCX_IQUAL_1MORE:
            if (cnt < 1 && !minerr) {
                /* missing parameter error */
                retres = ERR_NCX_MISSING_VAL_INST;
                log_error("\nError: Mandatory object '%s' is missing",
                          obj_get_name(chobj));
                ncx_print_errormsg(NULL, NULL, retres);
            }
            break;
        case NCX_IQUAL_ZMORE:
            break;
        default:
            retres = SET_ERROR(ERR_INTERNAL_VAL);
        }

    }

    return retres;
    
}  /* val_instance_check */


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
val_value_t *
    val_get_choice_first_set (val_value_t *val,
                              const obj_template_t *obj)
{
    val_value_t  *chval;

#ifdef DEBUG
    if (!val || !obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (chval = val_get_first_child(val);
         chval != NULL;
         chval = val_get_next_child(chval)) {

        if (chval->casobj != NULL) {
            boolean done2 = FALSE;
            const obj_template_t 
                *testobj = chval->casobj->parent;

            while (!done2) {
                if (testobj == obj) {
                    return chval;
                } else if (testobj != NULL &&
                           (testobj->objtype == OBJ_TYP_CHOICE ||
                            testobj->objtype == OBJ_TYP_CASE)) {
                    testobj = testobj->parent;
                } else {
                    done2 = TRUE;
                }
            }
        }
    }

    return NULL;

}  /* val_get_choice_first_set */


/********************************************************************
* FUNCTION val_get_choice_next_set
* 
* Check a val_value_t struct against its expected OBJ
* to determine if a specific choice has already been set
* Get the value struct for the next value set from the
* specified choice, afvter 'curval'
*
* INPUTS:
*   val == val_value_t to check
*   obj == choice object to check
*   curchild == current child selected from this choice (obj)
*
* RETURNS:
*   pointer to first value struct or NULL if choice not set
*********************************************************************/
val_value_t *
    val_get_choice_next_set (const obj_template_t *obj,
                             val_value_t *curchild)
{
    val_value_t  *chval;

#ifdef DEBUG
    if (!obj || !curchild) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    for (chval = val_get_next_child(curchild);
         chval != NULL;
         chval = val_get_next_child(chval)) {

        if (chval->casobj && chval->casobj->parent==obj) {
            return chval;
        }
    }
    return NULL;

}  /* val_get_choice_next_set */


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
boolean
    val_choice_is_set (val_value_t *val,
                       obj_template_t *obj)
{
    obj_template_t        *cas, *child;
    val_value_t           *testval, *chval;
    boolean                done;

#ifdef DEBUG
    if (!val || !obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    chval = NULL;
    done = FALSE;
    for (testval = val_get_first_child(val);
         testval != NULL && !done;
         testval = val_get_next_child(testval)) {

        if (testval->casobj != NULL) {
            boolean done2 = FALSE;
            const obj_template_t 
                *testobj = testval->casobj->parent;

            while (!done2) {
                if (testobj == obj) {
                    done2 = done = TRUE;
                } else if (testobj != NULL &&
                           (testobj->objtype == OBJ_TYP_CHOICE ||
                            testobj->objtype == OBJ_TYP_CASE)) {
                    testobj = testobj->parent;
                } else {
                    done2 = TRUE;
                }
            }

            if (done) {
                chval = testval;
            }
        }
    }

    if (!done) {
        return FALSE;
    }

    cas = chval->casobj;

    /* check if all the mandatory parms are present in this case */
    for (child = obj_first_child(cas);
         child != NULL;
         child = obj_next_child(child)) {

        if (!obj_is_config(child)) {
            continue;
        }
        if (!obj_is_mandatory(child)) {
            continue;
        }
        if (!val_find_child(val, obj_get_mod_name(child),
                           obj_get_name(child))) {
            return FALSE;
        }
    }
    return TRUE;

}  /* val_choice_is_set */


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
void
    val_purge_errors_from_root (val_value_t *val)
{

    assert( val && "val is NULL!" );

    purge_errors(val);
    val->res = NO_ERR;

}  /* val_purge_errors_from_root */


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
val_value_t *
    val_new_child_val (xmlns_id_t   nsid,
                       const xmlChar *name,
                       boolean copyname,
                       val_value_t *parent,
                       op_editop_t editop,
                       obj_template_t *obj)
{
    val_value_t *chval;

    chval = val_new_value();
    if (!chval) {
        return NULL;
    }

    /* save a const pointer to the name of this field */
    if (copyname) {
        chval->dname = xml_strdup(name);
        if (chval->dname) {
            chval->name = chval->dname;
        } else {
            val_free_value(chval);
            return NULL;
        }
    } else {
        chval->name = name;
    }

    chval->parent = parent;
    chval->editop = editop;
    chval->nsid = nsid;
    chval->obj = obj;

    return chval;

} /* val_new_child_val */


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
status_t
    val_gen_instance_id (xml_msg_hdr_t *mhdr,
                         const val_value_t  *val, 
                         ncx_instfmt_t format,
                         xmlChar  **buff)
{
    return val_gen_instance_id_ex(mhdr, val, format, TRUE, buff);

}  /* val_gen_instance_id */


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
status_t
    val_gen_instance_id_ex (xml_msg_hdr_t *mhdr,
                            const val_value_t  *val, 
                            ncx_instfmt_t format,
                            boolean stop_at_root,
                            xmlChar  **buff)
{
    assert( val && "val is NULL!" );
    assert( buff && "buff is NULL!" );

    uint32    len = 0, len2 = 0;
    status_t  res;

    /* figure out the length of the parmset instance ID */
    res = get_instance_string(mhdr, format, val, stop_at_root, NULL, &len);
    if (res != NO_ERR) {
        return res;
    }

    /* check no instance ID */
    if (len==0) {
        if (obj_is_root(val->obj)) {
            len = 1;
        } else {
            *buff = NULL;
            return ERR_NCX_NO_INSTANCE;
        }
    }

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    } else {
        memset(*buff, 0x0, len+1);
    }

    if (obj_is_root(val->obj) && len == 1) {
        xml_strcpy(*buff, (const xmlChar *)"/");
        len2 = 1;
    } else {
        /* get the instance ID string for real this time */
        res = get_instance_string(mhdr, format, val, stop_at_root, 
                                  *buff, &len2);
        if (res != NO_ERR) {
            m__free(*buff);
            *buff = NULL;
        }
    }

    if (res == NO_ERR && len != len2) {
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}  /* val_gen_instance_id_ex */


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
status_t
    val_gen_split_instance_id (xml_msg_hdr_t *mhdr,
                               const val_value_t  *val, 
                               ncx_instfmt_t format,
                               xmlns_id_t leaf_nsid,
                               const xmlChar *leaf_name,
                               boolean stop_at_root,
                               xmlChar  **buff)
{

    const xmlChar *leaf_pfix = NULL;
    uint32    len = 0, leaf_len = 0;
    status_t  res = NO_ERR;

#ifdef DEBUG 
    if (!val || !leaf_name || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (leaf_nsid) {
        leaf_pfix = xmlns_get_ns_prefix(leaf_nsid);
    }
    if (!leaf_pfix) {
        leaf_pfix = (const xmlChar *)"inv";
    }

    if (!(stop_at_root && val->obj && obj_is_root(val->obj))) {
        /* figure out the length of the parmset instance ID */
        res = get_instance_string(mhdr, format, val, stop_at_root, NULL, &len);
        if (res != NO_ERR) {
            return res;
        }

        /* check no instance ID */
        if (len==0) {
            *buff = NULL;
            return ERR_NCX_NO_INSTANCE;
        }
    }

    /* get the length of the extra leaf '/pfix:name' */
    leaf_len = 1 + xml_strlen(leaf_pfix) + 1 + 
        xml_strlen(leaf_name);

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+leaf_len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    } else {
        memset(*buff, 0x0, len+1);
    }

    /* get the instance ID string for real this time */
    if (!(stop_at_root && val->obj && obj_is_root(val->obj))) {
        res = get_instance_string(mhdr, format, val, stop_at_root, *buff, &len);
    }
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
    } else {
        xmlChar  *p = *buff;
        p += len;
        *p++ = '/';
        p += xml_strcpy(p, leaf_pfix);
        *p++ = ':';
        xml_strcpy(p, leaf_name);
    }

    return res;

}  /* val_gen_split_instance_id */


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
status_t
    val_get_index_string (xml_msg_hdr_t *mhdr,
                          ncx_instfmt_t format,
                          const val_value_t *val,
                          xmlChar *buff,
                          uint32  *len)
{
    const val_value_t  *ival, *nextival;
    const val_index_t  *valin;
    uint32              cnt, total;
    status_t            res;

#ifdef DEBUG
    if (!val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    cnt = 0;
    total = 0;
    *len = 0;

    /* get the first index node value struct */
    valin = (const val_index_t *)dlq_firstEntry(&val->indexQ);
    if (valin) {
        ival = valin->val;
    } else {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* check that there is at least one index component */
    if (format != NCX_IFMT_CLI) {
        /* start with the left bracket */
        if (buff) {
            *buff++ = (xmlChar)VAL_BINDEX_CH;
        }
        total++;
    }

    /* at least one real index component; generate entire index */
    while (ival) {
        /* generate one component, or N if it is a struct */
        res = get_index_comp(mhdr, format, ival, buff, &cnt);
        if (res != NO_ERR) {
            return res;
        } else {
            if (buff) {
                buff += cnt;
            }
            total += cnt;
        }

        /* setup next index component */
        nextival = NULL;
        valin = (const val_index_t *)dlq_nextEntry(valin);
        if (valin) {
            nextival = valin->val;
        } 

        /* check if an index separator string is needed */
        if (nextival) {
            switch (format) {
            case NCX_IFMT_C:
                /* add the index separator char ',' */
                if (buff) {
                    *buff++ = VAL_INDEX_SEPCH;
                }
                total++;
                break;
            case NCX_IFMT_XPATH1:
            case NCX_IFMT_XPATH2:
                /* format is one of the Xpath variants 
                 * add the index separator string ' and ' 
                 */
                if (buff) {
                    buff += xml_strcpy(buff, VAL_XPATH_INDEX_SEPSTR);
                }
                total += VAL_XPATH_INDEX_SEPLEN;
                break;
            case NCX_IFMT_CLI:
                /* add the CLI index separator char ' ' */
                if (buff) {
                    *buff++ = VAL_INDEX_CLI_SEPCH;
                }
                total++;
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        /* setup the next index component */
        ival = nextival;
    }

    /* add the closing right bracket */
    if (format != NCX_IFMT_CLI) {
        if (buff) {
            *buff++ = VAL_EINDEX_CH;
            *buff = 0;
        }
        total++;
    }

    /* return success */
    *len = total;
    return NO_ERR;

}  /* val_get_index_string */


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
status_t
    val_check_obj_when (val_value_t *val,
                        val_value_t *valroot,
                        val_value_t *objval,
                        obj_template_t *obj,
                        boolean *condresult,
                        uint32  *whencount)
{
    assert( val && "val is NULL!" );
    assert( valroot && "valroot is NULL!" );
    assert( obj && "obj is NULL!" );
    assert( condresult && "condresult is NULL!" );

    status_t res = NO_ERR;
    uint32   cnt = 0;

    /* check for any when statements attached to this object
     *  1) direct + parent choice/case
     *  2) inherited from uses or augment-when + parent choice/case
     */
    if (obj->when) {
        cnt++;

        val_value_t *dummychild = NULL, *usechild = objval;
        if (objval == NULL) {
            /* this code demopnstrates a bug in YANG
             * the dummy node alters tha value tree that the
             * test is being performed on. It adds a node
             * with no value, even if the type is not 'empty'
             *
             * The when-stmt MUST NOT use descendant-or-self
             * nodes in the test!  This should seem obvious
             * but it is still allowed in YANG
             */
            dummychild = val_new_value();
            if (!dummychild) {
                return ERR_INTERNAL_MEM;
            }
            val_init_from_template(dummychild, obj);
            val_add_child(dummychild, val);
            usechild = dummychild;
        }

        res = check_when_stmt(val, valroot, usechild, obj->when, condresult);
        if (dummychild) {
            val_remove_child(dummychild);
            val_free_value(dummychild);
        }
        if (res != NO_ERR || !*condresult) {
            if (whencount) {
                *whencount = cnt;
            }
            return res;
        }
    }

    if (val == valroot || val->parent == NULL) {
        if (whencount) {
            *whencount = cnt;
        }
        *condresult = FALSE;
        return NO_ERR;
    }

    /* !! the parent node is passed in so it is the context node
     * !! when xptrs are saved in the object
     */
    obj_xpath_ptr_t *xptr = obj_first_xpath_ptr(obj);
    for (; xptr; xptr = obj_next_xpath_ptr(xptr)) {
        cnt++;
        res = check_when_stmt(val, valroot, val, 
                              xptr->xpath, condresult);
        if (res != NO_ERR || !*condresult) {
            if (whencount) {
                *whencount = cnt;
            }
            return res;
        }
    }

    obj_template_t *testobj = obj->parent;
    boolean done = FALSE;
    while (!done) {
        if (testobj && 
            (testobj->objtype == OBJ_TYP_CHOICE ||
             testobj->objtype == OBJ_TYP_CASE)) {

            if (testobj->when) {
                cnt++;
                res = check_when_stmt(val, valroot, val->parent, 
                                      testobj->when, condresult);
                if (res != NO_ERR || !*condresult) {
                    if (whencount) {
                        *whencount = cnt;
                    }
                    return res;
                }
            }
            
            for (xptr = obj_first_xpath_ptr(testobj);
                 xptr; xptr = obj_next_xpath_ptr(xptr)) {
                cnt++;
                res = check_when_stmt(val, valroot, val->parent, 
                                      xptr->xpath, condresult);
                if (res != NO_ERR || !*condresult) {
                    if (whencount) {
                        *whencount = cnt;
                    }
                    return res;
                }
            }

            testobj = testobj->parent;
        } else {
            done = TRUE;
        }
    }

    if (whencount) {
        *whencount = cnt;
    }

    *condresult = TRUE;
    return NO_ERR;

} /* val_check_obj_when */


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
xpath_pcb_t *
    val_get_xpathpcb (val_value_t *val)
{

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return val->xpathpcb;

}  /* val_get_xpathpcb */


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
const xpath_pcb_t *
    val_get_const_xpathpcb (const val_value_t *val)
{

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return val->xpathpcb;

}  /* val_get_const_xpathpcb */


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
val_value_t *
    val_make_simval_obj (obj_template_t *obj,
                         const xmlChar *valstr,
                         status_t  *res)
{
    val_value_t  *newval;

#ifdef DEBUG
    if (!obj || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    newval = val_new_value();
    if (!newval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    val_init_from_template(newval, obj);

    *res = val_set_simval(newval,
                          obj_get_typdef(obj),
                          obj_get_nsid(obj),
                          obj_get_name(obj),
                          valstr);

    if (*res != NO_ERR) {
        val_free_value(newval);
        newval = NULL;
    }

    return newval;

} /* val_make_simval_obj */


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
status_t 
    val_set_simval_obj (val_value_t  *val,
                        obj_template_t *obj,
                        const xmlChar *valstr)
{
#ifdef DEBUG
    if (!val || !obj) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_set_simval(val,
                          obj_get_typdef(obj),
                          obj_get_nsid(obj),
                          obj_get_name(obj),
                          valstr);

}  /* val_set_simval_obj */


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
status_t
    val_set_warning_parms (val_value_t *parentval)
{
    val_value_t        *parmval;
    status_t            res = NO_ERR;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* warn-idlen parameter */
    parmval = val_find_child(parentval,
                             val_get_mod_name(parentval),
                             NCX_EL_WARN_IDLEN);
    if (parmval && parmval->res == NO_ERR) {
        ncx_set_warn_idlen(VAL_UINT(parmval));
    }

    /* warn-linelen parameter */
    parmval = val_find_child(parentval,
                             val_get_mod_name(parentval),
                             NCX_EL_WARN_LINELEN);
    if (parmval && parmval->res == NO_ERR) {
        ncx_set_warn_linelen(VAL_UINT(parmval));
    }

    /* warn-off parameter */
    for (parmval = val_find_child(parentval,
                                  val_get_mod_name(parentval),
                                  NCX_EL_WARN_OFF);
         parmval != NULL;
         parmval = val_find_next_child(parentval,
                                       val_get_mod_name(parentval),
                                       NCX_EL_WARN_OFF,
                                       parmval)) {
        if (parmval->res == NO_ERR) {
            res = ncx_turn_off_warning(VAL_UINT(parmval));
            if (res != NO_ERR) {
                log_error("\nError: disable warning failed (%s)",
                          get_error_string(res));
            }
        }
    }

    return res;

}  /* val_set_warning_parms */


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
status_t
    val_set_logging_parms (val_value_t *parentval)
{
    val_value_t        *val;
    char               *logfilename;
    status_t            res = NO_ERR;
    boolean             logappend;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    logappend = FALSE;

    /* get the log-level parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_LOGLEVEL);
    if (val && val->res == NO_ERR) {
        log_set_debug_level
            (log_get_debug_level_enum
             ((const char *)VAL_ENUM_NAME(val)));
        if (log_get_debug_level() == LOG_DEBUG_NONE) {
            log_error("\nError: invalid log-level value (%s)",
                      (const char *)VAL_ENUM_NAME(val));
            return ERR_NCX_INVALID_VALUE;
        }
    }

    val = val_find_child(parentval, val_get_mod_name(parentval),
                         NCX_EL_LOGAPPEND);
    if (val && val->res == NO_ERR) {
        logappend = TRUE;
    }

    /* get the log file name parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_LOG);
    if (val && val->res == NO_ERR && VAL_STR(val)) {
        if (!log_is_open()) {
            res = NO_ERR;
            logfilename = (char *)ncx_get_source(VAL_STR(val), &res);
            if (logfilename) {
                res = log_open(logfilename, logappend, TRUE);
                if (res != NO_ERR) {
                    log_error("\nError: open logfile '%s' failed (%s)",
                              logfilename,
                              get_error_string(res));
                }
                m__free(logfilename);
            }
        }
    }

    return res;

}  /* val_set_logging_parms */


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
status_t
    val_set_path_parms (val_value_t *parentval)
{
    val_value_t        *val;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* get the modpath parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_MODPATH);
    if (val && val->res == NO_ERR) {
        ncxmod_set_modpath(VAL_STR(val));
    }

    /* get the datapath parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_DATAPATH);
    if (val && val->res == NO_ERR) {
        ncxmod_set_datapath(VAL_STR(val));
    }

    /* get the runpath parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_RUNPATH);
    if (val && val->res == NO_ERR) {
        ncxmod_set_runpath(VAL_STR(val));
    }

    return NO_ERR;

}  /* val_set_path_parms */


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
status_t
    val_set_subdirs_parm (val_value_t *parentval)
{
    val_value_t        *val;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* get the subdirs parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_SUBDIRS);
    if (val && val->res == NO_ERR) {
        ncxmod_set_subdirs(VAL_BOOL(val));
    }

    return NO_ERR;

}  /* val_set_subdirs_parm */


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
status_t
    val_set_feature_parms (val_value_t *parentval)
{
    val_value_t        *val;
    status_t   res = NO_ERR;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* get the feature-code-default parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_CODE_DEFAULT);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val),
                        NCX_EL_DYNAMIC)) {
            ncx_set_feature_code_default(NCX_FEATURE_CODE_DYNAMIC);
        } else if (!xml_strcmp(VAL_ENUM_NAME(val),
                               NCX_EL_STATIC)) {
            ncx_set_feature_code_default(NCX_FEATURE_CODE_STATIC);
        } else {
            return ERR_NCX_INVALID_VALUE;
        }
    }

    /* get the feature-enable-default parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_ENABLE_DEFAULT);
    if (val && val->res == NO_ERR) {
        ncx_set_feature_enable_default(VAL_BOOL(val));
    }

    /* process the feature-static leaf-list */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_STATIC);
    while (val && val->res == NO_ERR) {
        res = ncx_set_feature_code_entry(VAL_STR(val),
                                         NCX_FEATURE_CODE_STATIC);
        if (res != NO_ERR) {
            return res;
        }

        val = val_find_next_child(parentval, 
                                  val_get_mod_name(parentval),
                                  NCX_EL_FEATURE_STATIC,
                                  val);
    }

    /* process the feature-dynamic leaf-list */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_DYNAMIC);
    while (val && val->res == NO_ERR) {
        res = ncx_set_feature_code_entry(VAL_STR(val),
                                         NCX_FEATURE_CODE_DYNAMIC);
        if (res != NO_ERR) {
            return res;
        }

        val = val_find_next_child(parentval, 
                                  val_get_mod_name(parentval),
                                  NCX_EL_FEATURE_DYNAMIC,
                                  val);
    }


    /* process the feature-enable leaf-list */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_ENABLE);
    while (val && val->res == NO_ERR) {
        res = ncx_set_feature_enable_entry(VAL_STR(val), TRUE);
        if (res != NO_ERR) {
            return res;
        }

        val = val_find_next_child(parentval, 
                                  val_get_mod_name(parentval),
                                  NCX_EL_FEATURE_ENABLE,
                                  val);
    }

    /* process the feature-disable leaf-list */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_FEATURE_DISABLE);
    while (val && val->res == NO_ERR) {
        res = ncx_set_feature_enable_entry(VAL_STR(val), FALSE);
        if (res != NO_ERR) {
            return res;
        }

        val = val_find_next_child(parentval, 
                                  val_get_mod_name(parentval),
                                  NCX_EL_FEATURE_DISABLE,
                                  val);
    }

    return res;

}  /* val_set_feature_parms */


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
status_t
    val_set_protocols_parm (val_value_t *parentval)
{
    val_value_t        *val;
    boolean             anyset = FALSE;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* get the protocols parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_PROTOCOLS);
    if (val && val->res == NO_ERR) {
        /* set to the values specified */
        if (ncx_string_in_list(NCX_EL_NETCONF10,
                               &(VAL_BITS(val)))) {
            anyset = TRUE;
            ncx_set_protocol_enabled(NCX_PROTO_NETCONF10);
        }
        if (ncx_string_in_list(NCX_EL_NETCONF11,
                               &(VAL_BITS(val)))) {
            anyset = TRUE;
            ncx_set_protocol_enabled(NCX_PROTO_NETCONF11);
        }
    } else {
        /* set to the default -- all versions enabled */
        anyset = TRUE;
        ncx_set_protocol_enabled(NCX_PROTO_NETCONF10);
        ncx_set_protocol_enabled(NCX_PROTO_NETCONF11);
    }
    
    return (anyset) ? NO_ERR : ERR_NCX_MISSING_PARM;

}  /* val_set_protocols_parm */


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
status_t
    val_set_ses_protocols_parm (ses_cb_t *scb,
                                val_value_t *parentval)
{
    val_value_t        *val;
    boolean             anyset = FALSE;

#ifdef DEBUG
    if (!parentval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!(parentval->btyp == NCX_BT_CONTAINER || 
          parentval->btyp == NCX_BT_LIST)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    /* get the protocols parameter */
    val = val_find_child(parentval, 
                         val_get_mod_name(parentval),
                         NCX_EL_PROTOCOLS);
    if (val && val->res == NO_ERR) {
        /* set to the values specified */
        if (ncx_string_in_list(NCX_EL_NETCONF10,
                               &(VAL_BITS(val)))) {
            anyset = TRUE;
            ses_set_protocols_requested(scb, NCX_PROTO_NETCONF10);
        }
        if (ncx_string_in_list(NCX_EL_NETCONF11,
                               &(VAL_BITS(val)))) {
            anyset = TRUE;
            ses_set_protocols_requested(scb, NCX_PROTO_NETCONF11);
        }
    } else {
        /* set to the default -- whatever was set globally */
        anyset = TRUE;
        if (ncx_protocol_enabled(NCX_PROTO_NETCONF10)) {
            ses_set_protocols_requested(scb, NCX_PROTO_NETCONF10);
        }
        if (ncx_protocol_enabled(NCX_PROTO_NETCONF11)) {
            ses_set_protocols_requested(scb, NCX_PROTO_NETCONF11);
        }
    }
    
    return (anyset) ? NO_ERR : ERR_NCX_MISSING_PARM;

}  /* val_set_ses_protocols_parm */


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
status_t
    val_ok_to_partial_lock (val_value_t *val,
                            ses_id_t sesid,
                            ses_id_t *lockowner)
{
    val_value_t   *childval;
    status_t       res;
    uint32         i;
    boolean        anyavail;
    ses_id_t       owner;

#ifdef DEBUG
    if (val == NULL || lockowner == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (sesid == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }        
#endif

    if (!val_is_config_data(val)) {
        return ERR_NCX_NOT_CONFIG;
    }

    res = NO_ERR;
    *lockowner = 0;

    /* check for an empty slot and locked-by-another session */
    anyavail = FALSE;
    for (i = 0; i < VAL_MAX_PLOCKS; i++) {
        if (val->plock[i] == NULL) {
            anyavail = TRUE;
        } else {
            owner = plock_get_sid(val->plock[i]);
            if (owner != sesid) {
                *lockowner = owner;
                return ERR_NCX_LOCK_DENIED;
            }
        }
    }

    if (!anyavail) {
        return ERR_NCX_RESOURCE_DENIED;
    }

    for (childval = val_get_first_child(val);
         childval != NULL;
         childval = val_get_next_child(childval)) {

        if (!val_is_config_data(childval)) {
            continue;
        }

        res = val_ok_to_partial_lock(childval, 
                                     sesid, 
                                     lockowner);
        if (res != NO_ERR) {
            return res;
        }
    }

    return NO_ERR;

    } /* val_ok_to_partial_lock */


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
status_t
    val_set_partial_lock (val_value_t *val,
                          plock_cb_t *plcb)
{
    uint32           i;
    boolean          anyavail, done;
    ses_id_t         newsid;

#ifdef DEBUG
    if (val == NULL || plcb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (!val_is_config_data(val)) {
        return ERR_NCX_NOT_CONFIG;
    }

    newsid = plock_get_sid(plcb);

    /* check for an empty slot and locked-by-another session */
    anyavail = FALSE;
    for (i = 0; i < VAL_MAX_PLOCKS; i++) {
        if (val->plock[i] == NULL) {
            anyavail = TRUE;
        } else if (plock_get_sid(val->plock[i]) != newsid) {
            return ERR_NCX_LOCK_DENIED;
        }
    }

    if (!anyavail) {
        return ERR_NCX_RESOURCE_DENIED;
    }

    done = FALSE;
    for (i = 0; i < VAL_MAX_PLOCKS && !done; i++) {
        if (val->plock[i] == NULL) {
            val->plock[i] = plcb;
            done = TRUE;
        }
    }

    return NO_ERR;

}  /* val_set_partial_lock */


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
void
    val_clear_partial_lock (val_value_t *val,
                            plock_cb_t *plcb)
{
    val_value_t   *childval;
    uint32         i;

#ifdef DEBUG
    if (val == NULL || plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!val_is_config_data(val)) {
        return;
    }

    /* check for the specified plcb */
    for (i = 0; i < VAL_MAX_PLOCKS; i++) {
        if (val->plock[i] == plcb) {
            val->plock[i] = NULL;
            return;
        }
    }

    for (childval = val_get_first_child(val);
         childval != NULL;
         childval = val_get_next_child(childval)) {

        if (val_is_config_data(childval)) {
            val_clear_partial_lock(childval, plcb);
        }
    }

}  /* val_clear_partial_lock */


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
status_t
    val_write_ok (val_value_t *val,
                  op_editop_t editop,
                  ses_id_t sesid,
                  boolean checkup,
                  uint32 *lockid)

{
    val_value_t   *childval, *upval;
    cfg_template_t *running;
    uint32         i;
    status_t       res;

#ifdef DEBUG
    if (val == NULL || lockid == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (editop == OP_EDITOP_NONE) {
        return NO_ERR;
    }

    if (!val_is_config_data(val)) {
        return ERR_NCX_NO_ACCESS_MAX;
    }

    /* quick exit check */
    running = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (cfg_first_partial_lock(running) == NULL) {
        return NO_ERR;
    }

    /* check for the specified session ID
     * this function should not be called for
     * the create operation, since there must not be
     * an existing instance that could be locked
     * in order for the createoperation to be valid
     */
    for (i = 0; i < VAL_MAX_PLOCKS; i++) {
        if (val->plock[i] == NULL) {
            continue;
        }
        if (plock_get_sid(val->plock[i]) != sesid) {
            /* this node locked by another session */
            *lockid = plock_get_id(val->plock[i]);
            return ERR_NCX_IN_USE_LOCKED;
        }
    }

    /* see if the path to root needs to be checked
     * because the agt_val_check_commit_edits waited until
     * applyhere to check the partial lock
     */
    if (checkup) {
        upval = val->parent;
        while (upval != NULL && !obj_is_root(upval->obj)) {
            for (i = 0; i < VAL_MAX_PLOCKS; i++) {
                if (upval->plock[i] == NULL) {
                    continue;
                }
                if (plock_get_sid(upval->plock[i]) != sesid) {
                    /* this node locked by another session */
                    *lockid = plock_get_id(upval->plock[i]);
                    return ERR_NCX_IN_USE_LOCKED;
                }
            }
            upval = upval->parent;
        }
    }

    /* only replace and delete need to dive into the subtree
     * because the config in the PDU does not need to
     * align with the target data tree; and the request
     * is all-or-nothing.  The merge operation will
     * dive into the subtree as indicated by the
     * config in the PDU input, and not all subtrees
     * may be affected by the merge, so any partial locks
     * in those subtrees need to be ignored now
     */
    if (editop == OP_EDITOP_REPLACE ||
        editop == OP_EDITOP_DELETE ||
        editop == OP_EDITOP_REMOVE) {

        for (childval = val_get_first_child(val);
             childval != NULL;
             childval = val_get_next_child(childval)) {

            if (val_is_config_data(childval)) {
                res = val_write_ok(childval, editop, sesid, FALSE, lockid);
                if (res != NO_ERR) {
                    return res;
                }
            }
        }
    }

    return NO_ERR;

}  /* val_write_ok */


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
void
    val_check_swap_resnode (val_value_t *curval,
                            val_value_t *newval)
{
    if (curval == NULL || newval == NULL) {
        return;
    }

    uint32 i = 0;
    for (; i < VAL_MAX_PLOCKS; i++) {
        newval->plock[i] = curval->plock[i];
        if (curval->plock[i] != NULL) {
            xpath_result_t *result = plock_get_final_result(curval->plock[i]);
            xpath_nodeset_swap_valptr(result, curval, newval);
        }
    }

}  /* val_check_swap_resnode */


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
void
    val_check_delete_resnode (val_value_t *curval)
{
    if (curval == NULL) {
        return;
    }

    uint32 i = 0;
    for (; i < VAL_MAX_PLOCKS; i++) {
        if (curval->plock[i] != NULL) {
            xpath_result_t *result = plock_get_final_result((curval->plock[i]));
            xpath_nodeset_delete_valptr(result, curval);
        }
    }

}  /* val_check_delete_resnode */


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
void
    val_write_extern (ses_cb_t *scb,
                      const val_value_t *val)
{
    FILE               *fil;
    boolean             done, inxml, xmldone, firstline;
    int                 ch, lastch;

    if (val->v.fname == NULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    fil = fopen((const char *)val->v.fname, "r");
    if (fil == NULL) {
        log_error("\nError: open extern var "
                  "file '%s' failed",
                  val->v.fname);
        return;
    }

    xmldone = FALSE;
    inxml = FALSE;
    done = FALSE;
    firstline = TRUE;
    lastch = 0;

    while (!done) {
        ch = fgetc(fil);

        if (ch == EOF) {
            if (lastch && !inxml) {
                ses_putchar(scb, (uint32)lastch);
            }
            fclose(fil);
            done = TRUE;
            continue;
        }

        if (firstline) {
            /* do not match the first char in the file */
            if (lastch && !inxml) {
                if (lastch == '<' && ch == '?') {
                    inxml = TRUE;
                } else {
                    /* done with xml checking */
                    xmldone = TRUE;
                    firstline = FALSE;
                }
            } else if (lastch && ch == '\n') {
                /* done with xml checking */
                firstline = FALSE;
                xmldone = TRUE;
            } else if (!xmldone && inxml) {
                /* look for xml declaration and remove it */
                if (lastch == '?' && ch == '>') {
                    xmldone = TRUE;
                }
            }

            /* first time xmldone is true skip this */
            if (xmldone && !inxml) {
                if (lastch) {
                    ses_putchar(scb, (uint32)lastch);
                }
            }

            /* setup 3rd loop to print char after '?>' */
            if (xmldone && inxml) {
                if (ch != '>') {
                    inxml = FALSE;
                }
            }
        } else {
            if (lastch) {
                ses_putchar(scb, (uint32)lastch);
            }
        }

        lastch = ch;
    }
}  /* val_write_extern */


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
void
    val_write_intern (ses_cb_t *scb,
                      const val_value_t *val)
{
    if (val->v.intbuff) {
        ses_putstr(scb, val->v.intbuff);
    }
    
}  /* val_write_intern */


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
val_value_t *
    val_get_value (ses_cb_t *scb,
                   xml_msg_hdr_t *msg,
                   val_value_t *val,
                   val_nodetest_fn_t testfn,
                   boolean acmcheck,
                   boolean *malloced,
                   status_t *res)
{
    val_value_t       *realval = NULL;
    val_value_t       *v_val = NULL;
    val_value_t       *useval;

#ifdef DEBUG
    if (!scb || !msg || !val || !malloced || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *malloced = FALSE;

    /* check the user filter callback function */
    if (testfn) {
        if (!(*testfn)(msg->withdef, TRUE, val)) {
            *res = ERR_NCX_SKIPPED;
            return NULL;
        }
    }

    if (acmcheck && msg->acm_cbfn) {
        xml_msg_authfn_t cbfn = (xml_msg_authfn_t)msg->acm_cbfn;
        boolean acmtest = (*cbfn)(msg, scb->username, val);
        if (!acmtest) {
            *res = ERR_NCX_SKIPPED;
            return NULL;
        }
    }
                                   
    if (val_is_virtual(val)) {
        v_val = val_get_virtual_value(scb, val, res);
        if (!v_val) {
            return NULL;
        }
    }

    useval = (v_val) ? v_val : val;

    if (useval->btyp == NCX_BT_LEAFREF) {
        typ_def_t *realtypdef = typ_get_xref_typdef(val->typdef);
        if (realtypdef) {
            switch (typ_get_basetype(realtypdef)) {
            case NCX_BT_STRING:
            case NCX_BT_BINARY:
            case NCX_BT_BOOLEAN:
            case NCX_BT_ENUM:
                break;
            default:
                realval = val_make_simval(realtypdef,
                                          val_get_nsid(useval),
                                          useval->name,
                                          VAL_STR(useval),
                                          res);
                if (realval) {
                    *malloced = TRUE;
                    val_move_fields_for_xml(val, 
                                            realval,
                                            msg->acm_cbfn == NULL);
                    return realval;
                } else {
                    return NULL;
                }
            }
        } else {
            *res = SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
    }

    return useval;

}  /* val_get_value */


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
void
    val_traverse_keys (val_value_t *val,
                       void *cookie1,
                       void *cookie2,
                       val_walker_fn_t walkerfn)
{
    val_index_t *valkey;

#ifdef DEBUG
    if (!val || !val->obj || !walkerfn) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (obj_is_root(val->obj)) {
        return;
    }

    if (val->parent != NULL) {
        val_traverse_keys(val->parent, cookie1, cookie2, walkerfn);
    }

    if (val->btyp != NCX_BT_LIST) {
        return;
    }

    for (valkey = val_get_first_key(val);
         valkey != NULL;
         valkey = val_get_next_key(valkey)) {

        if (valkey->val) {
            boolean ret = (*walkerfn)(valkey->val, cookie1, cookie2);
            if (!ret) {
                return;
            }
        } // else some error; skip this key!!!
    }
    
}  /* val_traverse_keys */


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
status_t
    val_build_index_chains (val_value_t *val)
{
    val_value_t *childval = NULL;
    status_t     res = NO_ERR;

#ifdef DEBUG
    if (!val || !val->obj) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (obj_is_leafy(val->obj)) {
        return NO_ERR;
    }

    for (childval = val_get_first_child(val);
         childval != NULL;
         childval = val_get_next_child(childval)) {
        if (!obj_is_leafy(childval->obj)) {
            res = val_build_index_chains(childval);
            if (res != NO_ERR) {
                return res;
            }
        }
    }

    if (val->btyp != NCX_BT_LIST) {
        /* container or maybe anyxml */
        return NO_ERR;
    }

    if (!dlq_empty(&val->indexQ)) {
        /* assume index chain already built */
        return NO_ERR;
    }

    /* 0 or more index components expected */
    res = val_gen_index_chain(val->obj, val);
    return res;
    
}  /* val_build_index_chains */


/* END file val_util.c */


