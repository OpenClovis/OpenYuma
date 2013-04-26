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
/*  FILE: yangcli_eval.c

   NETCONF YANG-based CLI Tool

   XPath evaluation support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13-aug-09    abb      begun; started from yangcli_cmd.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "libtecla.h"

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_num
#include "ncx_num.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_var
#include "var.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xml_val
#include "xml_val.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xpath1
#include "xpath1.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifndef _H_yangcli_eval
#include "yangcli_eval.h"
#endif

#ifndef _H_yangcli_cmd
#include "yangcli_cmd.h"
#endif

#ifndef _H_yangcli_util
#include "yangcli_util.h"
#endif


/********************************************************************
 * FUNCTION convert_result_to_val
 * 
 * Convert an Xpath result to a val_value_t struct
 * representing the rpc output in xpath-result-group
 * in yuma-xpath.yang
 *
 * INPUTS:
 *    result == XPath result to convert
 *
 * RETURNS:
 *    malloced and filled out value node
 *********************************************************************/
static val_value_t *
    convert_result_to_val (xpath_result_t *result)
{
    val_value_t      *resultval, *childval;
    xpath_resnode_t  *resnode;
    xmlns_id_t        ncid;

    resultval = NULL;
    ncid = xmlns_nc_id();

    switch (result->restype) {
    case XP_RT_NONE:
        /* this should be an error, but treat as an empty result */
        resultval = xml_val_new_flag(NCX_EL_DATA, ncid);
        break;
    case XP_RT_NODESET:
        if (dlq_empty(&result->r.nodeQ)) {
            resultval = xml_val_new_flag(NCX_EL_DATA, ncid);
        } else {
            resultval = xml_val_new_struct(NCX_EL_DATA, ncid);
            for (resnode = (xpath_resnode_t *)
                     dlq_firstEntry(&result->r.nodeQ);
                 resnode != NULL;
                 resnode = (xpath_resnode_t *)
                     dlq_nextEntry(resnode)) {

                childval = val_clone(resnode->node.valptr);
                if (childval == NULL) {
                    val_free_value(resultval);
                    return NULL;
                }
                val_add_child(childval, resultval);
            }
        }
        break;
    case XP_RT_NUMBER:
        resultval = xml_val_new_number(NCX_EL_DATA,
                                       ncid,
                                       &result->r.num,
                                       NCX_BT_FLOAT64);
        break;
    case XP_RT_STRING:
        resultval = xml_val_new_cstring(NCX_EL_DATA, ncid, result->r.str);
        break;
    case XP_RT_BOOLEAN:
        /* !!! this is wrong but not sure if any XML conversion used it !!!
         * contains empty string if FALSE; <data/> if TUR
         *resultval = xml_val_new_boolean(NCX_EL_DATA, ncid, result->r.boo);
         */
        resultval = xml_val_new_cstring(NCX_EL_DATA, 
                                        ncid,
                                        (result->r.boo) ? 
                                        NCX_EL_TRUE : NCX_EL_FALSE);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    return resultval;

}  /* convert_result_to_val */


/********************************************************************
 * FUNCTION do_eval (local RPC)
 * 
 * eval select='expr' docroot=$myvar
 *
 * Evaluate the XPath expression 
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the exec command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *    status
 *********************************************************************/
status_t
    do_eval (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len)
{
    val_value_t        *valset, *docroot, *expr;
    val_value_t        *resultval, *dummydoc, *childval;
    xpath_pcb_t        *pcb;
    xpath_result_t     *result;
    xmlChar            *resultstr;
    status_t            res;
    uint32              childcnt;

    docroot = NULL;
    expr = NULL;
    pcb = NULL;
    result = NULL;
    resultval = NULL;
    resultstr = NULL;
    dummydoc = NULL;

    res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);
    if (valset == NULL) {
        return res;
    }
    if (res != NO_ERR) {
        val_free_value(valset);
        return res;
    }
    if (valset->res != NO_ERR) {
        res = valset->res;
        val_free_value(valset);
        return res;
    }

    /* get the expr parameter */
    expr = val_find_child(valset, 
                          YANGCLI_MOD, 
                          YANGCLI_EXPR);
    if (expr == NULL) {
        res = ERR_NCX_MISSING_PARM;
    } else if (expr->res != NO_ERR) {
        res = expr->res;
    }

    if (res == NO_ERR) {
        /* get the optional docroot parameter */
        docroot = val_find_child(valset, 
                                 YANGCLI_MOD, 
                                 YANGCLI_DOCROOT);
        if (docroot && docroot->res != NO_ERR) {
            res = docroot->res;
        }
    }

    if (res == NO_ERR && docroot == NULL) {
        dummydoc = xml_val_new_struct(NCX_EL_DATA, xmlns_nc_id());
        if (dummydoc == NULL) {
            res = ERR_INTERNAL_MEM;
        } else {
            docroot = dummydoc;
        }
    }

    if (res == NO_ERR) {
        /* got all the parameters, and setup the XPath control block */
        pcb = xpath_new_pcb_ex(VAL_STR(expr), 
                               xpath_getvar_fn,
                               server_cb->runstack_context);
        if (pcb == NULL) {
            res = ERR_INTERNAL_MEM;
        } else {
            /* try to parse the XPath expression */
            result = xpath1_eval_expr(pcb, 
                                      docroot, /* context */
                                      docroot,
                                      TRUE,
                                      FALSE,
                                      &res);

            if (result != NULL && res == NO_ERR) {
                /* create a result val out of the XPath result */
                resultval = convert_result_to_val(result);
                if (resultval == NULL) {
                    res = ERR_INTERNAL_MEM;
                }
            }
        }
    }

    /* check save result or clear it */
    if (res == NO_ERR) {
        if (server_cb->result_name || 
            server_cb->result_filename) {

            /* decide if the saved variable should be
             * a string or a <data> element
             *
             * Convert to string:
             *   <data> is a simple type
             *   <data> contains 1 node that is a simple type
             *
             * Remove the <data> container:
             *   <data> contains a single complex element
             *
             * Retain the <data> container:
             *   <data> contains multiple elements
             */
            if (typ_is_simple(resultval->btyp)) {
                resultstr = val_make_sprintf_string(resultval);
                if (resultstr == NULL) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    val_free_value(resultval);
                    resultval = NULL;
                }
            } else {
                childcnt = val_child_cnt(resultval);
                if (childcnt == 1) {
                    /* check if the child is a simple 
                     * type or complex type
                     */
                    childval = val_get_first_child(resultval);
                    if (childval == NULL) {
                        res = SET_ERROR(ERR_INTERNAL_VAL);
                    } else if (typ_is_simple(childval->btyp)) {
                        /* convert the simple val to a string */
                        resultstr = val_make_sprintf_string(childval);
                        if (resultstr == NULL) {
                            res = ERR_INTERNAL_MEM;
                        } else {
                            val_free_value(resultval);
                            resultval = NULL;
                        }
                    } else {
                        /* remove the complex node from the
                         * data container
                         */
                        val_remove_child(childval);
                        val_free_value(resultval);
                        resultval = childval;
                    }
                } else {
                    /* either 0 or 2+ entries so leave the
                     * data container in place
                     */
                    ;
                }
            }
                
            if (res == NO_ERR) {
                /* save the filled in value */
                res = finish_result_assign(server_cb, 
                                           resultval, 
                                           resultstr);
                /* resultvar is freed no matter what */
                resultval = NULL;
            }
        }
    } else {
        clear_result(server_cb);
    }
    
    /* cleanup and exit */
    val_free_value(valset);
    xpath_free_pcb(pcb);
    xpath_free_result(result);
    val_free_value(resultval);
    if (resultstr) {
        m__free(resultstr);
    }
    val_free_value(dummydoc);

    if (res != NO_ERR) {
        log_error("\nError: XPath evaluation failed (%s)", 
                  get_error_string(res));
    }

    return res;
}  /* do_eval */


/* END yangcli_eval.c */
