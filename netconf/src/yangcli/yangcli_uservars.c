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
/*  FILE: yangcli_uservars.c

   NETCONF YANG-based CLI Tool

   uservars command

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
01-oct-11    abb      begun;

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "procdefs.h"
#include "log.h"
#include "mgr_load.h"
#include "ncx.h"
#include "status.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "yangcli.h"
#include "yangcli_cmd.h"
#include "yangcli_uservars.h"
#include "yangcli_util.h"


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
 * FUNCTION do_uservars (local RPC)
 * 
 * uservars clear
 * uservars load[=filespec]
 * uservars save[=filespec]
 *
 * Handle the uservars command
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the uservars command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_uservars (server_cb_t *server_cb,
                 obj_template_t *rpc,
                 const xmlChar *line,
                 uint32  len)
{
    val_value_t   *valset;
    status_t       res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (res == NO_ERR && valset) {
        /* get the 1 of N 'alias-action' choice */
        val_value_t *parm;
        const xmlChar *parmval = NULL;
        boolean done = FALSE;
        
        /* uservars clear */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_CLEAR);
            if (parm) {
                dlq_hdr_t *que = 
                    runstack_get_que(server_cb->runstack_context, ISGLOBAL);
                if (que == NULL) {
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    if (!dlq_empty(que)) {
                        var_clean_type_from_varQ(que, VAR_TYP_GLOBAL);
                        log_info("\nDeleted all global user variables "
                                 "from memory\n");
                    }else {
                        log_info("\nNo global user variables found\n");
                    }
                }
                done = TRUE;
            }
        }

        /* uservars load */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_LOAD);
            if (parm) {
                if (xml_strlen(VAL_STR(parm))) {
                    parmval = VAL_STR(parm);
                } else {
                    parmval = get_uservars_file();
                }
                res = load_uservars(server_cb, parmval);
                if (res == NO_ERR) {
                    log_info("\nLoaded global user variables OK from '%s'\n",
                             parmval);
                } else {
                    log_error("\nLoad global user variables from '%s' "
                              "failed (%s)\n", parmval, get_error_string(res));
                }
                done = TRUE;
            }
        }

        /* uservars save */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_SAVE);
            if (parm) {
                if (xml_strlen(VAL_STR(parm))) {
                    parmval = VAL_STR(parm);
                } else {
                    parmval = get_uservars_file();
                }
                res = save_uservars(server_cb, parmval);
                if (res == NO_ERR) {
                    log_info("\nSaved global user variables OK to '%s'\n",
                             parmval);
                } else {
                    log_error("\nSave global user variables to '%s' "
                              "failed (%s)\n", parmval, get_error_string(res));
                }
                done = TRUE;
            }
        }

        if (!done) {
            ;// missing mandatory choice error already reported
        }
    }

    if (valset) {
        val_free_value(valset);
    }

    return res;

}  /* do_uservars */


/********************************************************************
 * FUNCTION load_uservars
 * 
 * Load the user variables from the specified filespec
 *
 * INPUT:
 *   server_cb == server control block to use
 *   fspec == input filespec to use (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    load_uservars (server_cb_t *server_cb,
                   const xmlChar *fspec)
{
    xmlChar        *fullspec;
    ncx_module_t   *mod;
    dlq_hdr_t      *que;
    obj_template_t *varsobj;
    status_t        res = NO_ERR;

    if (fspec == NULL) {
        fspec = get_uservars_file();
    }

    mod = ncx_find_module(YANGCLI_MOD, NULL);
    if (mod == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    varsobj = obj_find_template_top(mod, YANGCLI_MOD, YANGCLI_VARS);
    if (varsobj == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    que = runstack_get_que(server_cb->runstack_context, ISGLOBAL);
    if (que == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }        

    fullspec = ncx_get_source(fspec, &res);
    if (res == NO_ERR && fullspec) {
        val_value_t *varsval = 
            mgr_load_extern_file(fullspec, varsobj, &res);
        if (varsval && res == NO_ERR) {
            val_value_t *varval;
            for (varval = val_get_first_child(varsval);
                 varval != NULL;
                 varval = val_get_next_child(varval)) {
                val_value_t *nameval, *childval;

                if (xml_strcmp(varval->name, YANGCLI_VAR)) {
                    log_error("\nError: user variable missing 'var' element, "
                              "from file '%s'\n", fullspec);
                    res = ERR_NCX_INVALID_VALUE;
                    continue;
                }

                /* get var/var/name and save */
                nameval = val_find_child(varval, YANGCLI_MOD, NCX_EL_NAME);
                if (nameval == NULL) {
                    log_error("\nError: user variable missing 'name' element, "
                              "from file '%s'\n", fullspec);
                    res = ERR_NCX_MISSING_PARM;
                    continue;
                }

                /* check /var/var/vartype = 'global' */
                childval = val_find_child(varval, YANGCLI_MOD, YANGCLI_VARTYPE);
                if (childval && xml_strcmp(VAL_ENUM_NAME(childval), 
                                           YANGCLI_GLOBAL)) {
                    log_error("\nError: wrong user variable type '%s' "
                              "from file '%s'\n", 
                              VAL_ENUM_NAME(childval), fullspec);
                    res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                    continue;
                }

                /* get /vars/var/value, rename and transfer it */
                childval = val_find_child(varval, YANGCLI_MOD, YANGCLI_VALUE);
                if (childval == NULL) {
                    log_error("\nError: user variable '%s' missing 'value' "
                              "element, from file '%s'\n", 
                              VAL_STR(nameval), fullspec);
                    res = ERR_NCX_MISSING_PARM;
                    continue;
                }
                val_remove_child(childval);

                /***!!! have wrong name?
                    !!! ignoring target object for now;
                    !!! <value> was parsed as an anyxml
                ***/

                val_set_name(childval, VAL_STR(nameval),
                             xml_strlen(VAL_STR(nameval)));
                res = var_set_move(server_cb->runstack_context,
                                   VAL_STR(nameval), 
                                   xml_strlen(VAL_STR(nameval)),
                                   VAR_TYP_GLOBAL, childval);
                if (res != NO_ERR) {
                    log_error("\nError: could not create user "
                              "variable '%s' (%s)",
                              VAL_STR(nameval), get_error_string(res));
                    val_free_value(childval);
                } else if (LOGDEBUG2) {
                    log_debug2("\nAdded user variable '%s' OK from file '%s'",
                               VAL_STR(nameval), fullspec);
                }
            }
        }
        if (varsval) {
            val_free_value(varsval);
        }
        if (res == ERR_XML_READER_START_FAILED) {
            log_debug("\nUser variables file '%s' not found", fullspec);
            res = NO_ERR;
        }
    }

    if (fullspec) {
        m__free(fullspec);
    }

    
    return res;

}  /* load_uservars */


/********************************************************************
 * FUNCTION save_uservars
 * 
 * Save the uservares to the specified filespec
 *
 * INPUT:
 *   server_cb == server control block to use
 *   fspec == output filespec to use  (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    save_uservars (server_cb_t *server_cb,
                   const xmlChar *fspec)
{
    xmlChar        *fullspec;
    ncx_module_t   *mod;
    dlq_hdr_t      *que;
    obj_template_t *varsobj, *varobj, *childobj;
    val_value_t    *varsval, *varval, *childval;
    status_t        res = NO_ERR;

    if (fspec == NULL) {
        fspec = get_uservars_file();
    }

    mod = ncx_find_module(YANGCLI_MOD, NULL);
    if (mod == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    varsobj = obj_find_template_top(mod, YANGCLI_MOD, YANGCLI_VARS);
    if (varsobj == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    varobj = obj_find_child(varsobj, YANGCLI_MOD, YANGCLI_VAR);
    if (varobj == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    que = runstack_get_que(server_cb->runstack_context, ISGLOBAL);
    if (que == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }        

    /* make vars container */
    varsval = val_new_value();
    if (varsval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(varsval, varsobj);

    fullspec = ncx_get_source(fspec, &res);
    if (res == NO_ERR && fullspec) {
        ncx_var_t *var;

        for (var = (ncx_var_t *)dlq_firstEntry(que);
             var != NULL && res == NO_ERR;
             var = (ncx_var_t *)dlq_nextEntry(var)) {

            if (var->vartype != VAR_TYP_GLOBAL || var->val == NULL) {
                continue;
            }

            /* make a <var> element for this global uservar */
            varval = val_new_value();
            if (varval == NULL) {
                res = ERR_INTERNAL_MEM;
                continue;
            }
            val_init_from_template(varval, varobj);
            /* pass off memory to parent here */
            val_add_child(varval, varsval);

            /* add var/name */
            childobj = obj_find_child(varobj, YANGCLI_MOD, NCX_EL_NAME);
            if (childobj == NULL) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
                continue;
            }
            childval = val_make_simval_obj(childobj, var->name, &res);
            if (childval == NULL) {
                continue;
            }
            /* pass off memory to parent here */
            val_add_child(childval, varval);

            /* leave out var/vartype because it is default (global) */

#if 0
            /* add var/target if needed
             * FIXME: the var->val->obj pointers are stale
             * if they were from session data node objects
             * and the memory has already been freed for these
             * objects.  No generic replacement was done!
             */
            if (var->val->obj && obj_is_data_db(var->val->obj)) {

                xmlChar *objbuff = NULL;
                childobj = obj_find_child(varobj, YANGCLI_MOD, NCX_EL_TARGET);
                if (childobj == NULL) {
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                    continue;
                }

                res = obj_gen_object_id(var->val->obj, &objbuff);
                if (res != NO_ERR) {
                    continue;
                }

                childval = val_make_simval_obj(childobj, objbuff, &res);
                m__free(objbuff);
                if (childval == NULL) {
                    continue;
                }
                /* pass off memory to parent here */
                val_add_child(childval, varval);
            }
#endif

            /* for now just clone the value and put it in the
             * tree instead of manipulating the var structs
             * and temporarily adding the var->val node to
             * the <var> node and removing it later to avoid
             * a double-free
             */
            childval = val_clone2(var->val);
            if (childval == NULL) {
                res = ERR_INTERNAL_MEM;
                continue;
            }
            childval->nsid = varval->nsid;
            val_set_name(childval, NCX_EL_VALUE, xml_strlen(NCX_EL_VALUE));
            val_add_child(childval, varval);
        }

        /* got a vars tree. now output it to the file */
        if (res == NO_ERR) {
            xml_attrs_t  attrs;
            xml_init_attrs(&attrs);
            res = xml_wr_file(fullspec, varsval, &attrs, XMLMODE, 
                              TRUE,  /* xmlhdr */
                              TRUE, /* withns */
                              0, /* startindent */
                              NCX_DEF_INDENT);
            xml_clean_attrs(&attrs);
        }
    }

    if (fullspec) {
        m__free(fullspec);
    }

    val_free_value(varsval);

    return res;

}  /* save_uservars */



/* END yangcli_uservars.c */
