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
/*  FILE: yangcli_cond.c

   NETCONF YANG-based CLI Tool

   conditional command
    - if
    - elif
    - else
    - end
    - while

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
25-apr-10    abb      begun;

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

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_ses
#include "mgr_ses.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
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

#ifndef _H_yangcli_cond
#include "yangcli_cond.h"
#endif

#ifndef _H_yangcli_cmd
#include "yangcli_cmd.h"
#endif

#ifndef _H_yangcli_util
#include "yangcli_util.h"
#endif


/********************************************************************
 * FUNCTION do_elif (local RPC)
 * 
 * Handle the if command; start a new ifcb context
 *
 * elif expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *    isif == TRUE for if, FALSE for elif
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_if_elif (server_cb_t *server_cb,
                obj_template_t *rpc,
                const xmlChar *line,
                uint32  len,
                boolean isif)
{
    val_value_t        *valset, *docroot, *expr, *dummydoc;
    xpath_pcb_t        *pcb;
    xpath_result_t     *result;
    status_t            res;
    boolean             cond;

    docroot = NULL;
    expr = NULL;
    pcb = NULL;
    dummydoc = NULL;
    cond = FALSE;
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
        } else if ((isif && 
                    runstack_get_cond_state(server_cb->runstack_context)) ||
                   (!isif && 
                    !runstack_get_if_used(server_cb->runstack_context))) {

            /* figure out if this if or elif block is enabled or not */
            result = xpath1_eval_expr(pcb, 
                                      docroot, /* context */
                                      docroot,
                                      TRUE,
                                      FALSE,
                                      &res);
            if (result != NULL && res == NO_ERR) {
                /* get new condition state for this loop */
                cond = xpath_cvt_boolean(result);
            } 
            xpath_free_result(result);
        }

        if (res == NO_ERR) {
            if (isif) {
                res = runstack_handle_if(server_cb->runstack_context, cond);
            } else {
                res = runstack_handle_elif(server_cb->runstack_context, cond);
            }
        }
    }

    /* cleanup and exit */
    if (valset) {
        val_free_value(valset);
    }
    if (pcb) {
        xpath_free_pcb(pcb);
    }
    if (dummydoc) {
        val_free_value(dummydoc);
    }
    return res;

}  /* do_if_elif */


/**************    E X P O R T E D    F U N C T I O N S    *************/


/********************************************************************
 * FUNCTION do_while (local RPC)
 * 
 * Handle the while command; start a new loopcb context
 *
 * while expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_while (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len)
{
    val_value_t        *valset, *docroot, *expr, *dummydoc, *maxloops;
    xpath_pcb_t        *pcb;
    status_t            res;
    uint32              maxloopsval;

    docroot = NULL;
    expr = NULL;
    pcb = NULL;
    dummydoc = NULL;
    res = NO_ERR;
    maxloopsval = YANGCLI_DEF_MAXLOOPS;

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
        if (docroot != NULL) {
            if (docroot->res != NO_ERR) {
                res = docroot->res;
            } else {
                val_remove_child(docroot);
            }
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
        /* get the optional maxloops parameter */
        maxloops = val_find_child(valset, 
                                  YANGCLI_MOD, 
                                  YANGCLI_MAXLOOPS);
        if (maxloops != NULL) {
            if (maxloops->res != NO_ERR) {
                res = maxloops->res;
            } else {
                maxloopsval = VAL_UINT(maxloops);
            }
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
            /* save these parameter and start a new while context block */
            res = runstack_handle_while(server_cb->runstack_context,
                                        maxloopsval,
                                        pcb,  /* hand off memory here */
                                        docroot);  /* hand off memory here */
            if (res == NO_ERR) {
                /* hand off the pcb and docroot memory above */
                pcb = NULL;
                docroot = NULL;
            }
        }
    }

    /* cleanup and exit */
    if (valset) {
        val_free_value(valset);
    }
    if (pcb) {
        xpath_free_pcb(pcb);
    }
    if (docroot) {
        val_free_value(docroot);
    }
    return res;

}  /* do_while */


/********************************************************************
 * FUNCTION do_if (local RPC)
 * 
 * Handle the if command; start a new ifcb context
 *
 * if expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_if (server_cb_t *server_cb,
           obj_template_t *rpc,
           const xmlChar *line,
           uint32  len)
{
    return do_if_elif(server_cb, rpc, line, len, TRUE);

}  /* do_if */


/********************************************************************
 * FUNCTION do_elif (local RPC)
 * 
 * Handle the if command; start a new ifcb context
 *
 * elif expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_elif (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len)
{
    return do_if_elif(server_cb, rpc, line, len, FALSE);

}  /* do_elif */


/********************************************************************
 * FUNCTION do_else (local RPC)
 * 
 * Handle the else command; start another conditional sub-block
 * to the current if-block
 *
 *   else
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_else (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len)
{
    val_value_t        *valset;
    status_t            res;

    res = NO_ERR;
    valset = get_valset(server_cb, rpc, &line[len], &res);
    if (valset == NULL) {
        if (res != ERR_NCX_SKIPPED) {
            return res;
        }
    } else {
        val_free_value(valset);
        return ERR_NCX_INVALID_VALUE;
    }

    res = runstack_handle_else(server_cb->runstack_context);

    return res;

}  /* do_else */


/********************************************************************
 * FUNCTION do_end (local RPC)
 * 
 * Handle the end command; end an if or while block
 *
 *   end
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_end (server_cb_t *server_cb,
            obj_template_t *rpc,
            const xmlChar *line,
            uint32  len)
{
    val_value_t        *valset;
    status_t            res;

    res = NO_ERR;
    valset = get_valset(server_cb, rpc, &line[len], &res);
    if (valset == NULL) {
        if (res != ERR_NCX_SKIPPED) {
            return res;
        }
    } else {
        val_free_value(valset);
        return ERR_NCX_INVALID_VALUE;
    }

    res = runstack_handle_end(server_cb->runstack_context);

    return res;

}  /* do_end */


/* END yangcli_cond.c */
