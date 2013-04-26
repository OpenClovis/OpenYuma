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
/*  FILE: yangcli_show.c

   NETCONF YANG-based CLI Tool

   show command

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13-aug-09    abb      begun; moved from yangcli_cmd.c

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

#include "procdefs.h"
#include "log.h"
#include "mgr.h"
#include "mgr_ses.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "obj_help.h"
#include "runstack.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "var.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_val.h"
#include "xml_wr.h"
#include "yangconst.h"
#include "yangcli.h"
#include "yangcli_cmd.h"
#include "yangcli_show.h"
#include "yangcli_util.h"


/********************************************************************
 * FUNCTION show_user_var
 * 
 * generate the output for a global or local variable
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   varname == variable name to show
 *   vartype == type of user variable
 *   val == value associated with this variable
 *   mode == help mode in use
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    show_user_var (server_cb_t *server_cb,
                   const xmlChar *varname,
                   var_type_t vartype,
                   val_value_t *val,
                   help_mode_t mode)
{
    xmlChar      *objbuff;
    logfn_t       logfn;
    boolean       imode;
    int32         doubleindent;
    status_t      res;

    res = NO_ERR;
    doubleindent = 1;

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    switch (vartype) {
    case VAR_TYP_GLOBAL:
    case VAR_TYP_LOCAL:
    case VAR_TYP_SESSION:
    case VAR_TYP_SYSTEM:
    case VAR_TYP_CONFIG:
        if (xml_strcmp(varname, val->name)) {
            doubleindent = 2;

            (*logfn)("\n   %s ", varname);

            if (val->obj && obj_is_data_db(val->obj)) {
                res = obj_gen_object_id(val->obj, &objbuff);
                if (res != NO_ERR) {
                    (*logfn)("[no object id]\n   ");
                } else {
                    (*logfn)("[%s]\n   ", objbuff);
                    m__free(objbuff);
                }
            }
        } else if (server_cb->display_mode == NCX_DISPLAY_MODE_JSON) {
            (*logfn)("\n   %s: ", varname);
            if (!typ_is_simple(val->btyp)) {
                (*logfn)("\n");
            }
        }
        break;
    default:
        ;
    }

    if (!typ_is_simple(val->btyp) && mode == HELP_MODE_BRIEF) {
        if (doubleindent == 1) {
            (*logfn)("\n   %s (%s)",
                     varname,
                     tk_get_btype_sym(val->btyp));
        } else {
            (*logfn)("\n      (%s)", 
                     tk_get_btype_sym(val->btyp));
        }
    } else {
        val_dump_value_max(val, 
                           server_cb->defindent * doubleindent,
                           server_cb->defindent,
                           (imode) ? DUMP_VAL_STDOUT : DUMP_VAL_LOG,
                           server_cb->display_mode,
                           FALSE,
                           FALSE);
    }

    return res;

}  /* show_user_var */


/********************************************************************
 * FUNCTION do_show_cli (sub-mode of local RPC)
 * 
 * show CLI parms
 *
 * INPUTS:
 *  server_cb == server control block to use
 *********************************************************************/
static void
    do_show_cli (server_cb_t *server_cb)
{
    val_value_t  *mgrset;
    logfn_t       logfn;
    boolean       imode;

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    mgrset = get_mgr_cli_valset();

    /* CLI Parameters */
    if (mgrset && val_child_cnt(mgrset)) {
        (*logfn)("\nCLI Variables\n");
        val_dump_value_max(mgrset, 
                           0,
                           server_cb->defindent,
                           (imode) ? DUMP_VAL_STDOUT : DUMP_VAL_LOG,
                           server_cb->display_mode,
                           FALSE,
                           FALSE);
        (*logfn)("\n");
    } else {
        (*logfn)("\nNo CLI variables\n");
    }

}  /* do_show_cli */


/********************************************************************
 * FUNCTION do_show_session (sub-mode of local RPC)
 * 
 * show session startup screen
 *
 * INPUTS:
 *  server_cb == server control block to use
 *  mode == help mode
 *********************************************************************/
static void
    do_show_session (server_cb_t *server_cb,
                     help_mode_t mode)
{
    ses_cb_t     *scb;

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (scb == NULL) {
        /* session was dropped */
        log_write("\nError: No session available."
                  " Not connected to any server.\n");
        return;
    }

    report_capabilities(server_cb, scb, FALSE, mode);
    log_write("\n");

}  /* do_show_session */


/********************************************************************
 * FUNCTION do_show_system (sub-mode of local RPC)
 * 
 * show system parms
 *
 * INPUTS:
 *  server_cb == server control block to use
 *  mode == help mode
 *********************************************************************/
static void
    do_show_system (server_cb_t *server_cb,
                    help_mode_t mode)
{
    ncx_var_t    *var;
    dlq_hdr_t    *que;
    logfn_t       logfn;
    boolean       imode, first;

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    /* System Script Variables */
    que = runstack_get_que(server_cb->runstack_context, ISGLOBAL);
    first = TRUE;
    for (var = (ncx_var_t *)dlq_firstEntry(que);
         var != NULL;
         var = (ncx_var_t *)dlq_nextEntry(var)) {
        
        if (var->vartype != VAR_TYP_SYSTEM) {
            continue;
        }

        if (first) {
            (*logfn)("\nRead-only environment variables\n");
            first = FALSE;
        }
        show_user_var(server_cb,
                      var->name, 
                      var->vartype,
                      var->val,
                      mode);
    }
    if (first) {
        (*logfn)("\nNo read-only environment variables\n");
    }
    (*logfn)("\n");

    /* System Config Variables */
    que = runstack_get_que(server_cb->runstack_context, ISGLOBAL);
    first = TRUE;
    for (var = (ncx_var_t *)dlq_firstEntry(que);
         var != NULL;
         var = (ncx_var_t *)dlq_nextEntry(var)) {

        if (var->vartype != VAR_TYP_CONFIG) {
            continue;
        }

        if (first) {
            (*logfn)("\nRead-write system variables\n");
            first = FALSE;
        }
        show_user_var(server_cb,
                      var->name,
                      var->vartype,
                      var->val,
                      mode);
    }
    if (first) {
        (*logfn)("\nNo system config variables\n");
    }
    (*logfn)("\n");

}  /* do_show_system */


/********************************************************************
 * FUNCTION do_show_vars (sub-mode of local RPC)
 * 
 * show brief info for all user variables
 *
 * INPUTS:
 *  server_cb == server control block to use
 *  mode == help mode requested
 *  shortmode == TRUE if printing just global or local variables
 *               FALSE to print everything
 *  isglobal == TRUE if print just globals
 *              FALSE to print just locals
 *              Ignored unless shortmode==TRUE
 * isany == TRUE to choose global or local
 *          FALSE to use 'isglobal' valuse only
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_vars (server_cb_t *server_cb,
                  help_mode_t mode,
                  boolean shortmode,
                  boolean isglobal,
                  boolean isany)

{
    ncx_var_t    *var;
    dlq_hdr_t    *que;
    logfn_t       logfn;
    boolean       first, imode;

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    if (mode > HELP_MODE_BRIEF && !shortmode) {
        /* CLI Parameters */
        do_show_cli(server_cb);
    }

    /* System Script Variables */
    if (!shortmode) {
        do_show_system(server_cb, mode);
    }

    /* Global Script Variables */
    if (!shortmode || isglobal) {
        que = runstack_get_que(server_cb->runstack_context, ISGLOBAL);
        first = TRUE;
        for (var = (ncx_var_t *)dlq_firstEntry(que);
             var != NULL;
             var = (ncx_var_t *)dlq_nextEntry(var)) {

            if (var->vartype != VAR_TYP_GLOBAL) {
                continue;
            }

            if (first) {
                (*logfn)("\nGlobal variables\n");
                first = FALSE;
            }
            show_user_var(server_cb, var->name,  var->vartype,
                          var->val, mode);
        }
        if (first) {
            (*logfn)("\nNo global variables\n");
        }
        (*logfn)("\n");
    }

    /* Local Script Variables */
    if (!shortmode || !isglobal || isany) {
        que = runstack_get_que(server_cb->runstack_context, ISLOCAL);
        first = TRUE;
        for (var = (ncx_var_t *)dlq_firstEntry(que);
             var != NULL;
             var = (ncx_var_t *)dlq_nextEntry(var)) {
            if (first) {
                (*logfn)("\nLocal variables\n");
                first = FALSE;
            }
            show_user_var(server_cb, var->name, var->vartype,
                          var->val, mode);
        }
        if (first) {
            (*logfn)("\nNo local variables\n");
        }
        (*logfn)("\n");
    }

    return NO_ERR;

} /* do_show_vars */


/********************************************************************
 * FUNCTION do_show_var (sub-mode of local RPC)
 * 
 * show full info for one user var
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   name == variable name to find 
 *   isglobal == TRUE if global var, FALSE if local var
 *   isany == TRUE if don't care (global or local)
 *         == FALSE to force local or global with 'isglobal'
 *   mode == help mode requested
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_var (server_cb_t *server_cb,
                 const xmlChar *name,
                 var_type_t vartype,
                 boolean isany,
                 help_mode_t mode)
{
    val_value_t       *val;
    logfn_t            logfn;
    boolean            imode;

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    if (isany) {
        /* skipping VAR_TYP_SESSION for now */
        val = var_get_local(server_cb->runstack_context,
                            name);
        if (val) {
            vartype = VAR_TYP_LOCAL;
        } else {
            val = var_get(server_cb->runstack_context,
                          name, VAR_TYP_GLOBAL);
            if (val) {
                vartype = VAR_TYP_GLOBAL;
            } else {
                val = var_get(server_cb->runstack_context,
                              name, VAR_TYP_CONFIG);
                if (val) {
                    vartype = VAR_TYP_CONFIG;
                } else {
                    val = var_get(server_cb->runstack_context,
                                  name, VAR_TYP_SYSTEM);
                    if (val) {
                        vartype = VAR_TYP_SYSTEM;
                    }
                }
            }
        }
    } else {
        val = var_get(server_cb->runstack_context, name, vartype);
    }

    if (val) {
        show_user_var(server_cb, name, vartype, val, mode);
        (*logfn)("\n");
    } else {
        (*logfn)("\nVariable '%s' not found", name);
        return ERR_NCX_DEF_NOT_FOUND;
    }

    return NO_ERR;

} /* do_show_var */


/********************************************************************
 * FUNCTION do_show_module (sub-mode of local RPC)
 * 
 * show module=mod-name
 *
 * INPUTS:
 *    mod == module to show
 *    mode == requested help mode
 * 
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_module (const ncx_module_t *mod,
                    help_mode_t mode)
{
    help_data_module(mod, mode);
    return NO_ERR;

} /* do_show_module */


/********************************************************************
 * FUNCTION do_show_one_module (sub-mode of show modules RPC)
 * 
 * for 1 of N: show modules
 *
 * INPUTS:
 *    mod == module to show
 *    mode == requested help mode
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_one_module (ncx_module_t *mod,
                        help_mode_t mode)
{
    boolean        imode;

    imode = interactive_mode();

    if (mode == HELP_MODE_BRIEF) {
        if (imode) {
            log_stdout("\n  %s", mod->name); 
        } else {
            log_write("\n  %s", mod->name); 
        }
    } else if (mode == HELP_MODE_NORMAL) {
        if (imode) {
            if (mod->version) {
                log_stdout("\n  %s:%s@%s", 
                           ncx_get_mod_xmlprefix(mod), 
                           mod->name, 
                           mod->version);
            } else {
                log_stdout("\n  %s:%s", 
                           ncx_get_mod_xmlprefix(mod), 
                           mod->name);
            }
        } else {
            if (mod->version) {
                log_write("\n  %s@%s", mod->name, mod->version);
            } else {
                log_write("\n  %s", mod->name);
            }
        }
    } else {
        help_data_module(mod, HELP_MODE_BRIEF);
    }

    return NO_ERR;

}  /* do_show_one_module */


/********************************************************************
 * FUNCTION do_show_modules (sub-mode of local RPC)
 * 
 * show modules
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    mode == requested help mode
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_modules (server_cb_t *server_cb,
                     help_mode_t mode)
{
    ncx_module_t  *mod;
    modptr_t      *modptr;
    boolean        anyout, imode;
    status_t       res;

    imode = interactive_mode();
    anyout = FALSE;
    res = NO_ERR;

    if (use_servercb(server_cb)) {
        for (modptr = (modptr_t *)dlq_firstEntry(&server_cb->modptrQ);
             modptr != NULL && res == NO_ERR;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            res = do_show_one_module(modptr->mod, mode);
            anyout = TRUE;
        }
        for (modptr = (modptr_t *)dlq_firstEntry(get_mgrloadQ());
             modptr != NULL && res == NO_ERR;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            res = do_show_one_module(modptr->mod, mode);
            anyout = TRUE;
        }
    } else {
        mod = ncx_get_first_module();
        while (mod && res == NO_ERR) {
            res = do_show_one_module(mod, mode);
            anyout = TRUE;
            mod = ncx_get_next_module(mod);
        }
    }

    if (anyout) {
        if (imode) {
            log_stdout("\n");
        } else {
            log_write("\n");
        }
    } else {
        if (imode) {
            log_stdout("\nyangcli: no modules loaded\n");
        } else {
            log_error("\nyangcli: no modules loaded\n");
        }
    }

    return res;

} /* do_show_modules */


/********************************************************************
 * FUNCTION do_show_one_object (sub-mode of show objects local RPC)
 * 
 * show objects: 1 of N
 *
 * INPUTS:
 *    obj == object to show
 *    mode == requested help mode
 *    anyout == address of return anyout status
 *
 * OUTPUTS:
 *    *anyout set to TRUE only if any suitable objects found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_show_one_object (obj_template_t *obj,
                        help_mode_t mode,
                        boolean *anyout)
{
    boolean               imode;

    imode = interactive_mode();

    if (obj_is_data_db(obj) && 
        obj_has_name(obj) &&
        !obj_is_hidden(obj) && !obj_is_abstract(obj)) {

        if (mode == HELP_MODE_BRIEF) {
            if (imode) {
                log_stdout("\n%s:%s",
                           obj_get_mod_name(obj),
                           obj_get_name(obj));
            } else {
                log_write("\n%s:%s",
                          obj_get_mod_name(obj),
                          obj_get_name(obj));
            }
        } else {
            obj_dump_template(obj, mode-1, 0, 0); 
        }
        *anyout = TRUE;
    }

    return NO_ERR;

} /* do_show_one_object */


/********************************************************************
 * FUNCTION do_show_objects (sub-mode of local RPC)
 * 
 * show objects
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    mode == requested help mode
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    do_show_objects (server_cb_t *server_cb,
                     help_mode_t mode)
{
    ncx_module_t         *mod;
    obj_template_t       *obj;
    modptr_t             *modptr;
    boolean               anyout, imode;
    status_t              res;

    imode = interactive_mode();
    anyout = FALSE;
    res = NO_ERR;

    if (use_servercb(server_cb)) {
        for (modptr = (modptr_t *)
                 dlq_firstEntry(&server_cb->modptrQ);
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            for (obj = ncx_get_first_object(modptr->mod);
                 obj != NULL && res == NO_ERR;
                 obj = ncx_get_next_object(modptr->mod, obj)) {

                res = do_show_one_object(obj, mode, &anyout);
            }
        }

        for (modptr = (modptr_t *)
                 dlq_firstEntry(get_mgrloadQ());
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            for (obj = ncx_get_first_object(modptr->mod);
                 obj != NULL && res == NO_ERR;
                 obj = ncx_get_next_object(modptr->mod, obj)) {

                res = do_show_one_object(obj, mode, &anyout);
            }
        }
    } else {
        mod = ncx_get_first_module();
        while (mod) {
            for (obj = ncx_get_first_object(mod);
                 obj != NULL && res == NO_ERR;
                 obj = ncx_get_next_object(mod, obj)) {

                res = do_show_one_object(obj, mode, &anyout);
            }
            mod = (ncx_module_t *)ncx_get_next_module(mod);
        }
    }
    if (anyout) {
        if (imode) {
            log_stdout("\n");
        } else {
            log_write("\n");
        }
    }

    return res;

} /* do_show_objects */


/********************************************************************
 * FUNCTION do_show (local RPC)
 * 
 * show module=mod-name
 *      modules
 *      def=def-nmae
 *
 * Get the specified parameter and show the internal info,
 * based on the parameter
 *
 * INPUTS:
 * server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_show (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len)
{
    val_value_t        *valset, *parm;
    ncx_module_t       *mod;
    status_t            res;
    boolean             imode, done;
    help_mode_t         mode;
    xmlChar             versionbuffer[NCX_VERSION_BUFFSIZE];

    res = NO_ERR;
    imode = interactive_mode();
    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (valset && res == NO_ERR) {
        mode = HELP_MODE_NORMAL;

        /* check if the 'brief' flag is set first */
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_BRIEF);
        if (parm && parm->res == NO_ERR) {
            mode = HELP_MODE_BRIEF;
        } else {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_FULL);
            if (parm && parm->res == NO_ERR) {
                mode = HELP_MODE_FULL;
            }
        }
            
        /* get the 1 of N 'showtype' choice */
        done = FALSE;

        /* show cli */
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_CLI);
        if (parm) {
            do_show_cli(server_cb);
            done = TRUE;
        }

        /* show local <foo> */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_LOCAL);
            if (parm) {
                res = do_show_var(server_cb, VAL_STR(parm),  VAR_TYP_LOCAL, 
                                  FALSE, mode);
                done = TRUE;
            }
        }

        /* show locals */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_LOCALS);
            if (parm) {
                res = do_show_vars(server_cb, mode, TRUE, FALSE, FALSE);
                done = TRUE;
            }
        }

        /* show objects */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_OBJECTS);
            if (parm) {
                res = do_show_objects(server_cb, mode);
                done = TRUE;
            }
        }

        /* show global */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_GLOBAL);
            if (parm) {
                res = do_show_var(server_cb, VAL_STR(parm), VAR_TYP_GLOBAL, 
                                  FALSE, mode);
                done = TRUE;
            }
        }

        /* show globals */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_GLOBALS);
            if (parm) {
                res = do_show_vars(server_cb, mode, TRUE, TRUE, FALSE);
                done = TRUE;
            }
        }

        /* show session */
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_SESSION);
        if (parm) {
            do_show_session(server_cb, mode);
            done = TRUE;
        }

        /* show system */
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_SYSTEM);
        if (parm) {
            do_show_system(server_cb, mode);
            done = TRUE;
        }

        /* show var <foo> */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_VAR);
            if (parm) {
                res = do_show_var(server_cb, VAL_STR(parm), VAR_TYP_NONE, 
                                  TRUE, mode);
                done = TRUE;
            }
        }

        /* show vars */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_VARS);
            if (parm) {
                res = do_show_vars(server_cb, mode, FALSE, FALSE, TRUE);
                done = TRUE;
            }
        }

        /* show module <foo> */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_MODULE);
            if (parm) {
                mod = find_module(server_cb, VAL_STR(parm));
                if (mod) {
                    res = do_show_module(mod, mode);
                } else {
                    if (imode) {
                        log_stdout("\nyangcli: module (%s) not loaded",
                                   VAL_STR(parm));
                    } else {
                        log_error("\nyangcli: module (%s) not loaded",
                                  VAL_STR(parm));
                    }
                }
                done = TRUE;
            }
        }

        /* show modules */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_MODULES);
            if (parm) {
                res = do_show_modules(server_cb, mode);
                done = TRUE;
            }
        }

        /* show version */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, NCX_EL_VERSION);
            if (parm) {
                res = ncx_get_version(versionbuffer, NCX_VERSION_BUFFSIZE);
                if (res == NO_ERR) {
                    if (imode) {
                        log_stdout("\nyangcli version %s\n", versionbuffer);
                    } else {
                        log_write("\nyangcli version %s\n", versionbuffer);
                    }
                }
                done = TRUE;
            }
        }
    }

    if (valset) {
        val_free_value(valset);
    }

    return res;

}  /* do_show */


/* END yangcli_show.c */
