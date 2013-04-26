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
/*  FILE: c.c

    Generate C file output from YANG module

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
27oct09      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>
#include <xmlreader.h>

#include "procdefs.h"
#include "c.h"
#include "c_util.h"
#include "dlq.h"
#include "ext.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "rpc.h"
#include "ses.h"
#include "status.h"
#include "typ.h"
#include "yang.h"
#include "yangconst.h"
#include "yangdump.h"
#include "yangdump_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION write_cb_logging_init
* 
* Generate the initialisation of callback logging
*
* INPUTS:
*   scb == session control block to use for writing
*   idstr == id of the call to be logged
*   suffix == suffix of the call to be logged
*   indent == level of indentation for generated code
*
*********************************************************************/
static void
    write_cb_logging_init (ses_cb_t *scb,
                           const xmlChar * idstr,
                           const xmlChar * suffix,
                           int32 indent)
{
    ses_putstr_indent(scb,
                      (const xmlChar *)"YumaTest::SILCallbackLog& cbLog "
                      "= YumaTest::SILCallbackLog::getInstance();",
                      indent);
    ses_putstr_indent(scb,
                      (const xmlChar *)"YumaTest::SILCallbackLog::Callback"
                      "Info cbData;",
                      indent);
    ses_putstr_indent(scb,
                      (const xmlChar *)"cbData.cbName = \"",
                      indent);
    ses_putstr(scb, idstr);
    ses_putstr(scb, suffix);
    ses_putstr(scb, (const xmlChar *)"\";");
} /* write_cb_logging_init */


/********************************************************************
* FUNCTION write_cb_logging_add_cb
* 
* Generate the adding of a callback to the callback log
*
* INPUTS:
*   scb == session control block to use for writing
*   module_name == session control block to use for writing
*   is_string == TRUE if the module name given should be generated
*                as a string (in quotation marks)
*                FALSE if the module name given should be generated as 
*                a variable name
*   indent == level of indentation for generated code
*
*********************************************************************/
static void
    write_cb_logging_add_cb (ses_cb_t *scb,
                             const xmlChar *mod_name,
                             boolean is_string,
                             int32 indent)
{
    ses_putstr_indent(scb,
                      (const xmlChar *)"cbLog.addCallback(",
                      indent);
    if (is_string) {
        ses_putstr(scb, (const xmlChar *)"\"");
    }
    ses_putstr(scb, mod_name);
    if (is_string) {
        ses_putstr(scb, (const xmlChar *)"\"");
    }
    ses_putstr(scb, (const xmlChar *)", cbData);");
} /* write_cb_logging_add_cb */


/********************************************************************
* FUNCTION write_if_res
* 
* Write if (res != NO_ERR) ... statements
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   startindent == start indent amount
*
*********************************************************************/
static void
    write_if_res (ses_cb_t *scb,
                  const yangdump_cvtparms_t *cp,
                  int32 startindent)
{
    ses_putstr_indent(scb, 
                      (const xmlChar *)"if (res != NO_ERR) {",
                      startindent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"return res;",
                      startindent + cp->indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"}",
                      startindent);

} /* write_if_res */


/********************************************************************
* FUNCTION write_if_record_error
* 
* Write if (...) { agt_record_error } stmts
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   startindent == start indent amount
*   oper_layer == TRUE if operation layer
*                 FALSE if application layer
*   with_return == TRUE if return res; added
*                  FALSE if not added
*   with_methnode == TRUE if methnode should be in error call
*                    FALSE to use NULL instead of methnode
*
*   
*********************************************************************/
static void
    write_if_record_error (ses_cb_t *scb,
                           const yangdump_cvtparms_t *cp,
                           int32 startindent,
                           boolean oper_layer,
                           boolean with_return,
                           boolean with_methnode)
{
    int32 indent;

    indent = cp->indent;

    ses_putchar(scb, '\n');
    ses_putstr_indent(scb, 
                      (const xmlChar *)"if (res != NO_ERR) {",
                      startindent);

    ses_putstr_indent(scb, 
                      (const xmlChar *)"agt_record_error(",
                      startindent + indent);

    indent += cp->indent;

    ses_putstr_indent(scb, 
                      (const xmlChar *)"scb,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"&msg->mhdr,",
                      startindent + indent);
    if (oper_layer) {
        ses_putstr_indent(scb, 
                          (const xmlChar *)"NCX_LAYER_OPERATION,",
                          startindent + indent);
    } else {
        ses_putstr_indent(scb, 
                          (const xmlChar *)"NCX_LAYER_CONTENT,",
                          startindent + indent);
    }
    ses_putstr_indent(scb, 
                      (const xmlChar *)"res,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (with_methnode) ?
                      (const xmlChar *)"methnode," :
                      (const xmlChar *)"NULL,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"(errorval) ? NCX_NT_VAL : NCX_NT_NONE,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"errorval,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"(errorval) ? NCX_NT_VAL : NCX_NT_NONE,",
                      startindent + indent);
    ses_putstr_indent(scb, 
                      (const xmlChar *)"errorval);",
                      startindent + indent);

    if (with_return) {
        indent -= cp->indent;

        ses_putstr_indent(scb, 
                          (const xmlChar *)"return res;",
                          startindent + indent);
    }

    ses_indent(scb, startindent);
    ses_putchar(scb, '}');

} /* write_if_record_error */


/********************************************************************
* FUNCTION write_c_iffeature_start
* 
* Generate the start C for 1 dynamic if (ncx_feature_enabled()
*
* INPUTS:
*   scb == session control block to use for writing
*   iffeatureQ == Q of ncx_feature_t to use
*   cp == conversion parameters to use
*   startindent == starting indent amount to use
* RETURNS:
*   TRUE if conditional was printed; FALSE if nothing done
*********************************************************************/
static boolean
    write_c_iffeature_start (ses_cb_t *scb,
                             const dlq_hdr_t *iffeatureQ,
                             const yangdump_cvtparms_t *cp,
                             int32 startindent)
{
    /* check if conditional wrapper needed */
    ncx_iffeature_t *iffeature = (ncx_iffeature_t *)dlq_firstEntry(iffeatureQ);
    if (iffeature == NULL) {
        return FALSE;
    }

    const xmlChar *modname = ncx_get_modname(iffeature->feature->tkerr.mod);
    if (modname == NULL) {
        return FALSE;
    }

    const xmlChar *modversion = 
        ncx_get_modversion(iffeature->feature->tkerr.mod);
    if (modversion == NULL) {

    }

    ses_indent(scb, startindent);
    ses_putstr(scb, (const xmlChar *)"if (ncx_feature_enabled_str(");
    ses_indent(scb, startindent + cp->indent);
    write_identifier(scb, modname, BAR_MOD, modname, FALSE);
    ses_putstr(scb, (const xmlChar *)", ");
    ses_indent(scb, startindent + cp->indent);
    if (modversion) {
        write_identifier(scb, modname, BAR_REV, modname, FALSE);
    } else {
        ses_putstr(scb, (const xmlChar *)"NULL");
    }
    ses_putchar(scb, ',');
    ses_putstr_indent(scb, (const xmlChar *)"(const xmlChar *)\"",
                      startindent + cp->indent);
    ses_putstr(scb, iffeature->feature->name);
    ses_putstr(scb, (const xmlChar *)"\")");

    iffeature = (ncx_iffeature_t *)dlq_nextEntry(iffeature);
    while (iffeature) {
        ses_indent(scb, startindent + cp->indent);
        ses_putstr(scb, (const xmlChar *)"&& ncx_feature_enabled_str(");
        ses_indent(scb, startindent + cp->indent + cp->indent);
        write_identifier(scb, modname, BAR_MOD, modname, FALSE);
        ses_putstr(scb, (const xmlChar *)", ");
        ses_indent(scb, startindent + cp->indent + cp->indent);
        if (modversion) {
            write_identifier(scb, modname, BAR_REV, modname, FALSE);
        } else {
            ses_putstr(scb, (const xmlChar *)"NULL");
        }
        ses_putchar(scb, ',');
        ses_putstr_indent(scb, (const xmlChar *)"(const xmlChar *)\"",
                          startindent + cp->indent + cp->indent);
        ses_putstr(scb, iffeature->feature->name);
        ses_putstr(scb, (const xmlChar *)"\")");

        iffeature = (ncx_iffeature_t *)dlq_nextEntry(iffeature);
    }

    ses_putstr(scb, (const xmlChar *)") {");
    return TRUE;

}  /* write_c_iffeature_start */


/********************************************************************
* FUNCTION write_c_iffeature_end
* 
* Generate the end C for 1 dynamic if-feature conditiona;
*
* INPUTS:
*   scb == session control block to use for writing
*   iffeatureQ == Q of ncx_feature_t to use
*
*********************************************************************/
static void
    write_c_iffeature_end (ses_cb_t *scb,
                           const dlq_hdr_t *iffeatureQ,
                           int32 startindent)
{
    if (!dlq_empty(iffeatureQ)) {
        ses_indent(scb, startindent);
        ses_putstr(scb, (const xmlChar *)"} /* ");

        ncx_iffeature_t *iffeature = 
            (ncx_iffeature_t *)dlq_firstEntry(iffeatureQ);
        ncx_iffeature_t *nexif = NULL;

        for (; iffeature != NULL; iffeature = nexif) {
            write_identifier(scb, iffeature->feature->tkerr.mod->name,
                             BAR_FEAT, iffeature->feature->name, TRUE);
            nexif = (ncx_iffeature_t *)dlq_nextEntry(iffeature);
            if (nexif) {
                ses_putstr(scb, (const xmlChar *)", ");
            }
        }

        ses_putstr(scb, (const xmlChar *)" */");
    }

}  /* write_c_iffeature_end */


/********************************************************************
* FUNCTION write_c_includes
* 
* Write the C file #include statements
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
static void
    write_c_includes (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp)
{
#ifdef LEAVE_OUT_FOR_NOW
    const ncx_include_t      *inc;
#endif
    boolean                   needrpc, neednotif;

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* allow correct compiling with cpp flags */
        ses_putchar(scb, '\n');
        ses_putstr(scb, (const xmlChar *)"extern \"C\" {");
    }

    needrpc = need_rpc_includes(mod, cp);
    neednotif = need_notif_includes(mod, cp);

    /* add xmlChar include */
    write_ext_include(scb, (const xmlChar *)"xmlstring.h");

    /* add procdefs include first NCX include */
    write_ncx_include(scb, (const xmlChar *)"procdefs");

    /* add includes even if they may not get used
     * TBD: prune all unused include files
     */
    write_ncx_include(scb, (const xmlChar *)"agt");

    write_ncx_include(scb, (const xmlChar *)"agt_cb");

    if (neednotif) {
        write_ncx_include(scb, (const xmlChar *)"agt_not");
    }
        
    if (needrpc) {
        write_ncx_include(scb, (const xmlChar *)"agt_rpc");
    }

    write_ncx_include(scb, (const xmlChar *)"agt_timer");
    write_ncx_include(scb, (const xmlChar *)"agt_util");
    write_ncx_include(scb, (const xmlChar *)"dlq");
    write_ncx_include(scb, (const xmlChar *)"ncx");
    write_ncx_include(scb, (const xmlChar *)"ncx_feature");
    write_ncx_include(scb, (const xmlChar *)"ncxmod");
    write_ncx_include(scb, (const xmlChar *)"ncxtypes");

    if (needrpc) {
        write_ncx_include(scb, (const xmlChar *)"rpc");
    }

    if (needrpc || neednotif) {
        write_ncx_include(scb, (const xmlChar *)"ses");
    }

    write_ncx_include(scb, (const xmlChar *)"status");

    if (needrpc || neednotif) {
        write_ncx_include(scb, (const xmlChar *)"val");
        write_ncx_include(scb, (const xmlChar *)"val_util");
        write_ncx_include(scb, (const xmlChar *)"xml_util");
    }

    /* include for the module H file */
    switch (cp->format) {
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
        write_ncx_include(scb, ncx_get_modname(mod));
        break;
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_YC:
        /* write includes for paired yuma USER and then SIL file */
        write_cvt_include(scb, ncx_get_modname(mod), NCX_CVTTYP_UC);
        write_cvt_include(scb, ncx_get_modname(mod), NCX_CVTTYP_YC);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

#ifdef LEAVE_OUT_FOR_NOW
    /* includes for submodules */
    if (!cp->unified) {
        for (inc = (const ncx_include_t *)
                 dlq_firstEntry(&mod->includeQ);
             inc != NULL;
             inc = (const ncx_include_t *)dlq_nextEntry(inc)) {

            write_ncx_include(scb, inc->submodule);
        }
    }
#endif

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* close extern */
        ses_putstr(scb, (const xmlChar *)"\n}\n");
        /* add include for callback logging */
        write_ncx_include(scb, (const xmlChar *)
                          "test/support/callbacks/sil-callback-log");
    }

    ses_putchar(scb, '\n');
} /* write_c_includes */


/********************************************************************
* FUNCTION write_c_object_var
* 
* Write the C file object variable name
*
* INPUTS:
*   scb == session control block to use for writing
*   objname == object name to use
*
*********************************************************************/
static void
    write_c_object_var (ses_cb_t *scb,
                        const xmlChar *objname)
{
    write_c_safe_str(scb, objname);
    ses_putstr(scb, (const xmlChar *)"_obj");

} /* write_c_object_var */


/********************************************************************
* FUNCTION write_c_value_var
* 
* Write the C file value node variable name
*
* INPUTS:
*   scb == session control block to use for writing
*   valname == value name to use
*
*********************************************************************/
static void
    write_c_value_var (ses_cb_t *scb,
                       const xmlChar *valname)
{
    write_c_safe_str(scb, valname);
    ses_putstr(scb, (const xmlChar *)"_val");

} /* write_c_value_var */


/********************************************************************
* FUNCTION write_c_static_vars
* 
* Write the C file static variable declarations
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*********************************************************************/
static void
    write_c_static_vars (ses_cb_t *scb,
                         ncx_module_t *mod,
                         const yangdump_cvtparms_t *cp)
{
    obj_template_t           *obj;

#ifdef LEAVE_OUT_FOR_NOW
    const ncx_include_t      *inc;
#endif


    /* create a static object template pointer for
     * each top-level real object, RPC, or notification
     */
    if (cp->format != NCX_CVTTYP_UC) {
        if (mod->ismod) {
            /* print a banner */
            ses_putstr(scb, (const xmlChar *)"\n/* module static variables */");
            /* create a static module back-pointer */
            ses_putstr(scb, (const xmlChar *)"\nstatic ncx_module_t *");
            write_c_safe_str(scb, ncx_get_modname(mod));
            ses_putstr(scb, (const xmlChar *)"_mod;");
        }
    }

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) ||
            obj_is_cli(obj) || 
            obj_is_abstract(obj)) {
            continue;
        }

        if ((cp->format == NCX_CVTTYP_UC && obj_is_notif(obj)) ||
            (cp->format == NCX_CVTTYP_YC && !obj_is_notif(obj)) ||
            !(cp->format == NCX_CVTTYP_UC || cp->format == NCX_CVTTYP_YC)) {

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            ses_putstr(scb, (const xmlChar *)"\nstatic obj_template_t *");
            write_c_object_var(scb, obj_get_name(obj));
            ses_putchar(scb, ';');
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }
    }
    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (skip_c_top_object(obj)) {
            continue;
        }
        if (cp->format != NCX_CVTTYP_UC) {
            write_h_iffeature_start(scb, &obj->iffeatureQ);
            ses_putstr(scb, (const xmlChar *)"\nstatic val_value_t *");
            write_c_value_var(scb, obj_get_name(obj));
            ses_putchar(scb, ';');
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }
    }
    ses_putchar(scb, '\n');

#ifdef LEAVE_OUT_FOR_NOW
        /* includes for submodules */
        if (!cp->unified) {
            for (inc = (const ncx_include_t *)
                     dlq_firstEntry(&mod->includeQ);
                 inc != NULL;
                 inc = (const ncx_include_t *)dlq_nextEntry(inc)) {
                write_c_static_vars(scb, inc->submodule);
            }
        }
#endif

    if (cp->format != NCX_CVTTYP_YC) {
        /* user can put static var init inline */
        ses_putstr(scb, (const xmlChar *)"\n/* put your static "
                   "variables here */\n");
    }

} /* write_c_static_vars */


/********************************************************************
* FUNCTION write_c_init_static_vars_fn
* 
* Write the C file init static variable function
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*********************************************************************/
static void
    write_c_init_static_vars_fn (ses_cb_t *scb,
                                 ncx_module_t *mod,
                                 const yangdump_cvtparms_t *cp)
{
    if (cp->format == NCX_CVTTYP_UC) {
        return;
    }

    const xmlChar            *modname = mod->name;
    obj_template_t           *obj = NULL;
    int32                     indent = cp->indent;

    /* generate function banner comment */
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL, FN_INIT_STATIC_VARS, cp->isuser);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"initialize module static variables");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_END);

    ses_putstr(scb, (const xmlChar *)"\nstatic void ");
    write_identifier(scb, modname, NULL, FN_INIT_STATIC_VARS, cp->isuser);
    ses_putstr(scb, (const xmlChar *)" (void)\n{");

    if (mod->ismod) {
        ses_indent(scb, indent);
        write_c_safe_str(scb, ncx_get_modname(mod));
        ses_putstr(scb, (const xmlChar *)"_mod = NULL;");
    }
        
    /* create an init line for each top-level 
     * real object, RPC, or notification
     */
    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) ||
            obj_is_cli(obj) || 
            obj_is_abstract(obj)) {
            continue;
        }

        if ((cp->format == NCX_CVTTYP_YC && !obj_is_notif(obj)) ||
            cp->format != NCX_CVTTYP_YC) {

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            ses_indent(scb, indent);
            write_c_object_var(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" = NULL;");
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }
    }

    /* create an init line for each top-level 
     * real value cache pointer for a database object
     */
    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (skip_c_top_object(obj)) {
            continue;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        ses_indent(scb, indent);
        write_c_value_var(scb, obj_get_name(obj));
        ses_putstr(scb, (const xmlChar *)" = NULL;");
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

    if (cp->format != NCX_CVTTYP_YC) {
        /* put inline user marker comment */
        ses_putchar(scb, '\n');
        ses_putstr_indent(scb, (const xmlChar *)"/* init your "
                          "static variables here */", indent);
    }
    ses_putchar(scb, '\n');

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n} /* ");
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init_static_vars", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" */\n");

} /* write_c_init_static_vars_fn */


/********************************************************************
* FUNCTION write_c_edit_cbfn
* 
* Generate the C code for the foo_object_edit function
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   obj == object struct for the database object
*   objnameQ == Q of c_define_t structs to search for this object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_edit_cbfn (ses_cb_t *scb,
                       const yangdump_cvtparms_t *cp,
                       obj_template_t *obj,
                       dlq_hdr_t *objnameQ,
                       boolean uprotomode)
{
    c_define_t      *cdef;
    int32            indent = cp->indent, i = 0;
    uint32           keycount;

    cdef = find_path_cdefine(objnameQ, obj);
    if (cdef == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    keycount = obj_key_count_to_root(obj);

    /* generate function banner comment */
    if (dlq_empty(&obj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }
    ses_putstr(scb, FN_BANNER_START);
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    } /* else static function so no prefix */

    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, EDIT_SUFFIX);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Edit database object callback");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Path: ");
    ses_putstr(scb, cdef->valstr);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, 
               (const xmlChar *)"Add object instrumentation "
               "in COMMIT phase.");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_INPUT);
    ses_putstr(scb, (const xmlChar *)"    see agt/agt_cb.h for details");
    if (cp->isuser && keycount) {
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"    k_ parameters are ancestor "
                   "list key values.");
    }
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern status_t ");
        ses_putstr(scb, U_PREFIX);
    } else if (cp->isuser) {
        ses_putstr(scb, (const xmlChar *)"\nstatus_t ");
        ses_putstr(scb, U_PREFIX);
    } else {
        ses_putstr(scb, (const xmlChar *)"\nstatic status_t ");
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, EDIT_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" (");

    ses_putstr_indent(scb, (const xmlChar *)"ses_cb_t *scb,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"rpc_msg_t *msg,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"agt_cbtyp_t cbtyp,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"op_editop_t editop,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"val_value_t *newval,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"val_value_t *curval", indent);

    /* add the key parameters, if any */
    if (cp->isuser && keycount) {
        ses_putchar(scb, ',');
        write_c_key_params(scb, obj, objnameQ, keycount, indent);
    }
    ses_putchar(scb, ')');

    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* start function body */
    ses_putstr(scb, (const xmlChar *)"\n{");
    ses_putstr_indent(scb, 
                      (const xmlChar *)"status_t res = NO_ERR;",
                      indent);
    if (!cp->isuser) {
        /* generate static vars */
        ses_putstr_indent(scb, 
                          (const xmlChar *)"val_value_t *errorval = "
                          "(curval) ? curval : newval;", indent);
        if (cp->format == NCX_CVTTYP_YC && keycount) {
            ses_putstr_indent(scb, 
                              (const xmlChar *)"val_value_t *lastkey = NULL;",
                              indent);
            /* add the key local variables */
            write_c_key_vars(scb, obj, objnameQ, PARM_ERRORVAL, 
                             keycount, indent);
        }
    }

    /* print an enter fn DEBUG trace */
    ses_putchar(scb, '\n');
    ses_putstr_indent(scb, (const xmlChar *)"if (LOGDEBUG) {", indent);
    ses_putstr_indent(scb, (const xmlChar *)"log_debug(\"\\nEnter ", 
                      indent+indent);
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, EDIT_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" callback for %s phase\",");
    ses_putstr_indent(scb, (const xmlChar *)"agt_cbtype_name(cbtyp));",
                      indent*3);
    ses_putstr_indent(scb, (const xmlChar *)"}", indent);

    /* generate call to user edit function if format is yuma C
     * first need to get all the local variables
     */
    if (cp->format == NCX_CVTTYP_YC) {
        ses_putchar(scb, '\n');

        ses_putstr_indent(scb, (const xmlChar *)"res = u_", indent);
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, EDIT_SUFFIX);
        ses_putstr(scb, 
                   (const xmlChar *)"(scb, msg, cbtyp, editop, newval, curval");
        if (keycount) {
            ses_putchar(scb, ',');
            write_c_key_values(scb, obj, objnameQ, keycount, indent+indent);
        }
        ses_putchar(scb, ')');
        ses_putchar(scb, ';');
    }

    /* generate switch on callback type */
    if (cp->format != NCX_CVTTYP_YC) {

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putchar(scb, '\n');
            write_cb_logging_init(scb, cdef->idstr, EDIT_SUFFIX, indent);
            ses_putstr_indent(scb,
                              (const xmlChar *)"val_value_t* value = "
                              "(newval) ? newval : curval;",
                              indent);
            ses_putstr_indent(scb,
                              (const xmlChar *)"std::string module_name = "
                              "(char*) val_get_mod_name(value);",
                              indent);
        }

        ses_putchar(scb, '\n');
        ses_putstr_indent(scb, (const xmlChar *)"switch (cbtyp) {", indent);

        ses_putstr_indent(scb, (const xmlChar *)"case AGT_CB_VALIDATE:",
                          indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbType = \"validate\";",
                              indent+indent);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent+indent);
        }

        ses_putstr_indent(scb, (const xmlChar *)"/* description-stmt "
                          "validation here */",
                          indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"break;", indent+indent);

        ses_putstr_indent(scb, (const xmlChar *)"case AGT_CB_APPLY:", indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbType = \"apply\";",
                              indent+indent);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent+indent);
        }

        ses_putstr_indent(scb, (const xmlChar *)"/* database manipulation "
                          "done here */", indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"break;", indent+indent);

        /* commit case arm */
        ses_putstr_indent(scb, (const xmlChar *)"case AGT_CB_COMMIT:", indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbType = \"commit\";",
                              indent+indent);
        }

        ses_putstr_indent(scb,  (const xmlChar *)"/* device instrumentation "
                          "done here */", indent+indent);

        /* generate switch on editop */
        ses_putstr_indent(scb, (const xmlChar *)"switch (editop) {",
                          indent+indent);

        ses_putstr_indent(scb, (const xmlChar *)"case OP_EDITOP_LOAD:",
                          indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"load\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb, (const xmlChar *)"break;", indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"case OP_EDITOP_MERGE:",
                          indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"merge\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb, (const xmlChar *)"break;", indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"case OP_EDITOP_REPLACE:",
                          indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"replace\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb, (const xmlChar *)"break;", indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"case OP_EDITOP_CREATE:",
                          indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"create\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb, (const xmlChar *)"break;", indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"case OP_EDITOP_DELETE:",
                          indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"delete\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb, (const xmlChar *)"break;", indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"default:", indent+indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbPhase = \"default\";",
                              indent*3);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent*3);
        }

        ses_putstr_indent(scb,
                          (const xmlChar *)"res = SET_ERROR(ERR_INTERNAL_VAL);",
                          indent*3);
        ses_putstr_indent(scb, (const xmlChar *)"}", indent+indent);
    }

    /* generate cache object check */
    if (!cp->isuser && obj_is_top(obj)) {
        ses_putchar(scb, '\n');
        if (cp->format == NCX_CVTTYP_C || cp->format == NCX_CVTTYP_CPP_TEST) {
            ses_putstr_indent(scb, (const xmlChar *)"if (res == NO_ERR) {",
                              indent+indent);
            i = 3;
        } else {
            ses_putstr_indent(scb, (const xmlChar *)"if (res == NO_ERR "
                              "&& cbtyp == AGT_CB_COMMIT) {", indent);
            i = 2;
        }
        ses_putstr_indent(scb, (const xmlChar *)"res = agt_check_cache(&",
                          indent*i);
        write_c_value_var(scb, obj_get_name(obj));
        ses_putchar(scb, ',');
        ses_putstr(scb, (const xmlChar *)" newval, curval, editop);");
        if (cp->format == NCX_CVTTYP_C || cp->format == NCX_CVTTYP_CPP_TEST) {
            ses_putstr_indent(scb, (const xmlChar *)"}\n", indent+indent);
        } else {
            ses_putstr_indent(scb, (const xmlChar *)"}\n", indent);
        }
    }

    /* generate check on read-only child nodes */
    if (!cp->isuser && obj_has_ro_children(obj)) {
        if (cp->format == NCX_CVTTYP_C || cp->format == NCX_CVTTYP_CPP_TEST) {
            i = 2;
            ses_putstr_indent(scb, (const xmlChar *)
                              "if (res == NO_ERR && curval == NULL) {",
                              indent*i);

        } else {
            i = 1;
            ses_putstr_indent(scb, (const xmlChar *)
                              "if (res == NO_ERR && cbtyp == AGT_CB_COMMIT "
                              "&& curval == NULL) {",
                              indent);
        }
        ses_putstr_indent(scb, (const xmlChar *)"res = ", indent*(i+1));
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, MRO_SUFFIX);
        ses_putstr(scb, (const xmlChar *)"(newval);");
        ses_indent(scb, indent*i);
        ses_putchar(scb, '}');
    }

    if (cp->format != NCX_CVTTYP_YC) {
        ses_putstr_indent(scb,
                          (const xmlChar *)"break;",
                          indent+indent);

        /* rollback case arm */
        ses_putstr_indent(scb, (const xmlChar *)"case AGT_CB_ROLLBACK:",
                          indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbType = \"rollback\";",
                              indent+indent);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent+indent);
        }

        ses_putstr_indent(scb,
                          (const xmlChar *)"/* undo device instrumentation "
                          "here */", indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"break;", indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"default:", indent);

        if (cp->format == NCX_CVTTYP_CPP_TEST) {
            /* generate callback logging */
            ses_putstr_indent(scb,
                              (const xmlChar *)"cbData.cbType = \"default\";",
                              indent+indent);
            write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                    FALSE, indent+indent);
        }

        ses_putstr_indent(scb,
                          (const xmlChar *)"res = SET_ERROR(ERR_INTERNAL_VAL);",
                          indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"}", indent);
    }

    if (!cp->isuser) {
        write_if_record_error(scb, cp, indent, FALSE, FALSE, FALSE);
    }

    /* return result */
    ses_indent(scb, indent);
    ses_putstr(scb, (const xmlChar *)"return res;");

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n\n} /* ");
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, EDIT_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" */");
    if (dlq_empty(&obj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }

} /* write_c_edit_cbfn */


/********************************************************************
* FUNCTION write_c_get_cbfn
* 
* Generate the C code for the foo_object_get function
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   obj == object struct for the database object
*   objnameQ == Q of c_define_t structs to search for this object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_get_cbfn (ses_cb_t *scb,
                      const yangdump_cvtparms_t *cp,
                      obj_template_t *obj,
                      dlq_hdr_t *objnameQ,
                      boolean uprotomode)
{
    const xmlChar   *defval;
    c_define_t      *cdef;
    typ_enum_t      *typ_enum;
    int32            indent;
    uint32           keycount;
    ncx_btype_t      btyp;

    indent = cp->indent;

    cdef = find_path_cdefine(objnameQ, obj);
    if (cdef == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    keycount = obj_key_count_to_root(obj);

    /* generate function banner comment */
    ses_putstr(scb, FN_BANNER_START);
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, GET_SUFFIX);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Get database object callback");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Path: ");
    ses_putstr(scb, cdef->valstr);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Fill in 'dstval' contents");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_INPUT);
    ses_putstr(scb, (const xmlChar *)"    see ncx/getcb.h for details");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern status_t ");
        ses_putstr(scb, U_PREFIX);
    } else if (cp->isuser) {
        ses_putstr(scb, (const xmlChar *)"\nstatus_t ");
        ses_putstr(scb, U_PREFIX);
    } else {
        ses_putstr(scb, (const xmlChar *)"\nstatic status_t ");
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, GET_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" (");
    if (cp->isuser) {
        ses_putstr_indent(scb, (const xmlChar *)"val_value_t *dstval", indent);
        if (keycount) {
            ses_putchar(scb, ',');
            write_c_key_params(scb, obj, objnameQ, keycount, indent);
        }
        ses_putchar(scb, ')');
    } else {
        ses_putstr_indent(scb, (const xmlChar *)"ses_cb_t *scb,", indent);
        ses_putstr_indent(scb, (const xmlChar *)"getcb_mode_t cbmode,", 
                          indent);
        ses_putstr_indent(scb, (const xmlChar *)"const val_value_t *virval,",
                          indent);
        ses_putstr_indent(scb, (const xmlChar *)"val_value_t *dstval)", 
                          indent);
    }

    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* start function body */
    ses_putstr(scb, (const xmlChar *)"\n{");

    /* declare static vars */
    ses_putstr_indent(scb, (const xmlChar *)"status_t res = NO_ERR;", indent);
    switch (cp->format) {
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
    case NCX_CVTTYP_UC:
        ses_indent(scb, indent);
        write_c_objtype_ex(scb, obj, objnameQ, ';', TRUE, FALSE);
        break;
    case NCX_CVTTYP_YC:
        if (keycount) {
            ses_putstr_indent(scb, 
                              (const xmlChar *)"val_value_t *lastkey = NULL;",
                              indent);
            /* add the key local variables */
            write_c_key_vars(scb, obj, objnameQ, PARM_DSTVAL, keycount, indent);
        }
        break;
    default:
        ;
    }

    /* print an enter fn DEBUG trace */
    ses_putchar(scb, '\n');
    ses_putstr_indent(scb,
                      (const xmlChar *)"if (LOGDEBUG) {",
                      indent);
    ses_putstr_indent(scb,
                      (const xmlChar *)"log_debug(\"\\nEnter ",
                      indent+indent);
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, GET_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" callback\");");
    ses_putstr_indent(scb, (const xmlChar *)"}", indent);

    ses_putchar(scb, '\n');

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* generate callback logging */
        write_cb_logging_init(scb, cdef->idstr, GET_SUFFIX, indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)"std::string module_name = "
                          "(char*) val_get_mod_name(dstval);",
                          indent);
        write_cb_logging_add_cb(scb, (const xmlChar *)"module_name", 
                                FALSE, indent);
    }

    switch (cp->format) {
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
    case NCX_CVTTYP_YC:
        /* print message about removing (void)foo; line */
        ses_putchar(scb, '\n');
        ses_putstr_indent(scb,
                          (const xmlChar *)"/* remove the next line "
                          "if scb is used */",
                          indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)"(void)scb;",
                          indent);

        ses_putchar(scb, '\n');
        ses_putstr_indent(scb,
                          (const xmlChar *)"/* remove the next line "
                          "if virval is used */",
                          indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)"(void)virval;",
                          indent);

        /* generate if-stmt to screen out any other mode except 'get' */
        ses_putchar(scb, '\n');
        ses_putstr_indent(scb,
                          (const xmlChar *)"if (cbmode != GETCB_GET_VALUE) {",
                          indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)"return ERR_NCX_OPERATION"
                          "_NOT_SUPPORTED;",
                          indent+indent);
        ses_putstr_indent(scb, (const xmlChar *)"}", indent);
        break;
    default:
        ;
    }

    ses_putchar(scb, '\n');
    switch (cp->format) {
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
    case NCX_CVTTYP_UC:
        ses_putstr_indent(scb, 
                          (const xmlChar *)"/* set the ", 
                          indent);
        write_c_safe_str(scb, obj_get_name(obj));
        ses_putstr(scb, (const xmlChar *)" var here, ");

        btyp = obj_get_basetype(obj);
        defval = obj_get_default(obj);

        if (defval) {
            ses_putstr(scb, 
                       (const xmlChar *)"replace (void) or use "
                       "default value */");
            ses_putstr_indent(scb, 
                              (const xmlChar *)"(void)",
                              indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putchar(scb, ';');
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = val_set_simval_obj(",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval,",
                              indent+indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval->obj,",
                              indent+indent);
            ses_indent(scb, indent+indent);
            ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
            ses_putstr(scb, defval);
            ses_putstr(scb, (const xmlChar *)"\");");
        } else if (typ_is_string(btyp) || 
                   btyp == NCX_BT_IDREF ||
                   btyp == NCX_BT_INSTANCE_ID ||
                   btyp == NCX_BT_UNION ||
                   btyp == NCX_BT_LEAFREF ||
                   btyp == NCX_BT_BINARY) {

            ses_putstr(scb, (const xmlChar *)"change EMPTY_STRING */");
            ses_indent(scb, indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" = EMPTY_STRING;");
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = val_set_simval_obj(",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval,",
                              indent+indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval->obj,",
                              indent+indent);
            ses_indent(scb, indent+indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)");");
        } else if (btyp == NCX_BT_DECIMAL64) {
            ses_putstr(scb, (const xmlChar *)"change zero */");
            ses_indent(scb, indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" = (const xmlChar *)\"0.0\";");
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = val_set_simval_obj(",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval,",
                              indent+indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"dstval->obj,",
                              indent+indent);
            ses_indent(scb, indent+indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)");");
        } else if (typ_is_number(btyp)) {
            ses_putstr(scb, (const xmlChar *)"change zero */");
            ses_indent(scb, indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" = 0;");

            ses_indent(scb, indent);
            write_c_val_macro_type(scb, obj);
            ses_putstr(scb, (const xmlChar *)"(dstval) = ");
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putchar(scb, ';');
        } else {
            switch (btyp) {
            case NCX_BT_ENUM:
            case NCX_BT_BITS:
                ses_putstr(scb, (const xmlChar *)"change enum */");
                ses_indent(scb, indent);
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putstr(scb, (const xmlChar *)" = ");

                typ_enum = typ_first_enumdef(obj_get_typdef(obj));
                if (typ_enum != NULL) {
                    ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
                    ses_putstr(scb, typ_enum->name);
                    ses_putstr(scb, (const xmlChar *)"\";");
                } else {
                    ses_putstr(scb, (const xmlChar *)"EMPTY_STRING;");
                }
                ses_putstr_indent(scb, 
                                  (const xmlChar *)"res = val_set_simval_obj(",
                                  indent);
                ses_putstr_indent(scb, 
                                  (const xmlChar *)"dstval,",
                                  indent+indent);
                ses_putstr_indent(scb, 
                                  (const xmlChar *)"dstval->obj,",
                                  indent+indent);
                ses_indent(scb, indent+indent);
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putstr(scb, (const xmlChar *)");");
                break;
            case NCX_BT_BOOLEAN:
            case NCX_BT_EMPTY:
                ses_putstr(scb, (const xmlChar *)"change TRUE if needed */");
                ses_indent(scb, indent);
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putstr(scb, (const xmlChar *)" = TRUE;");
                write_c_val_macro_type(scb, obj);
                ses_putstr(scb, (const xmlChar *)"(dstval) = ");
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putchar(scb, ';');
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case NCX_CVTTYP_YC:
        /* call the user get function */
        ses_putstr_indent(scb, (const xmlChar *)"res = u_", indent);
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, GET_SUFFIX);
        ses_putstr(scb, (const xmlChar *)"(dstval");
        if (keycount) {
            ses_putchar(scb, ',');
            write_c_key_values(scb, obj, objnameQ, keycount, indent+indent);
        }
        ses_putchar(scb, ')');
        ses_putchar(scb, ';');
        break;
    default:
        ;
    }

    /* return result */
    ses_putchar(scb, '\n');
    ses_indent(scb, indent);
    ses_putstr(scb, (const xmlChar *)"return res;");

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n\n} /* ");
    if (cp->isuser) {
        ses_putstr(scb, U_PREFIX);
    }
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, GET_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" */\n");

} /* write_c_get_cbfn */


/********************************************************************
* FUNCTION write_c_mro_fn
* 
* Generate the C code for the foo_object_mro function
* Make read-only children
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   fullmode == TRUE for full function
*               FALSE for just C statement guts
*   obj == object struct for the database object
*   objnameQ == Q of c_define_t structs to search for this object
*********************************************************************/
static void
    write_c_mro_fn (ses_cb_t *scb,
                    ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    obj_template_t *obj,
                    boolean fullmode,
                    dlq_hdr_t *objnameQ)
{
    const xmlChar   *modname;
    c_define_t      *cdef, *childcdef;
    obj_template_t  *childobj;
    int32            indent;
    boolean          child_done;

    modname = ncx_get_modname(mod);
    indent = cp->indent;

    cdef = find_path_cdefine(objnameQ, obj);
    if (cdef == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    /* generate function banner comment */
    if (fullmode) {
        ses_putstr(scb, FN_BANNER_START);
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, MRO_SUFFIX);
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"Make read-only child nodes");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"Path: ");
        ses_putstr(scb, cdef->valstr);
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, FN_BANNER_INPUT);
        ses_putstr(scb, 
                   (const xmlChar *)"    parentval == the parent struct to "
                   "use for new child nodes");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, FN_BANNER_RETURN_STATUS);
        ses_putstr(scb, FN_BANNER_END);

        /* generate the function prototype lines */
        ses_putstr(scb, (const xmlChar *)"\nstatic status_t");
        ses_putstr_indent(scb, cdef->idstr, indent);
        ses_putstr(scb, MRO_SUFFIX);
        ses_putstr(scb, (const xmlChar *)" (val_value_t *parentval)");
        ses_putstr(scb, (const xmlChar *)"\n{");

        /* generate static vars */
        ses_putstr_indent(scb, 
                          (const xmlChar *)"status_t res = NO_ERR;",
                          indent);
        ses_putstr_indent(scb, 
                          (const xmlChar *)"val_value_t *childval = NULL;\n",
                          indent);
    }

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* generate callback logging */
        write_cb_logging_init(scb, cdef->idstr, MRO_SUFFIX, indent);
        write_cb_logging_add_cb(scb, ncx_get_modname(mod), TRUE, indent);
        ses_putchar(scb, '\n');
    }

    /* create read-only child nodes as needed */
    for (childobj = obj_first_child(obj);
         childobj != NULL;
         childobj = obj_next_child(childobj)) {

        if (!obj_has_name(childobj) ||
            obj_is_cli(childobj) ||
            obj_is_abstract(childobj) ||
            obj_get_config_flag(childobj)) {
            continue;
        }

        /* this is a real data node config = false */
        childcdef = find_path_cdefine(objnameQ, childobj);
        if (childcdef == NULL) {
            SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
            return;
        }

        child_done = FALSE;

        switch (childobj->objtype) {
        case OBJ_TYP_LEAF:
            ses_putchar(scb, '\n');
            ses_putstr_indent(scb, 
                              (const xmlChar *)"/* add ",
                              indent);
            ses_putstr(scb, childcdef->valstr);
            ses_putstr(scb, (const xmlChar *)" */");
            ses_putstr_indent(scb, 
                              (const xmlChar *)"childval = agt_make_"
                              "virtual_leaf(",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"parentval->obj,",
                              indent+indent);
            ses_indent(scb, indent+indent);
            write_identifier(scb, modname, BAR_NODE,
                             obj_get_name(childobj), cp->isuser);
            ses_putchar(scb, ',');
            ses_indent(scb, indent+indent);
            ses_putstr(scb, childcdef->idstr);
            ses_putstr(scb, GET_SUFFIX);
            ses_putchar(scb, ',');            
            ses_putstr_indent(scb, (const xmlChar *)"&res);", indent+indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"if (childval != NULL) {",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"val_add_child(childval, "
                              "parentval);",
                              indent+indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"} else {",
                              indent);
            ses_putstr_indent(scb, 
                              (const xmlChar *)"return res;",
                              indent+indent);

            ses_indent(scb, indent);
            ses_putchar(scb, '}');
            child_done = TRUE;
            break;
        case OBJ_TYP_LEAF_LIST:

            break;
        case OBJ_TYP_LIST:

            break;
        case OBJ_TYP_CONTAINER:
            /* make the container */
            ses_putstr_indent(scb, (const xmlChar *)"res = ", indent);
            ses_putstr(scb, (const xmlChar *)"agt_add_container(");
            ses_indent(scb, indent+indent);
            write_identifier(scb, modname, BAR_MOD, modname, cp->isuser);
            ses_putchar(scb, ',');
            ses_indent(scb, indent+indent);
            write_identifier(scb, modname, BAR_NODE, 
                             obj_get_name(childobj), cp->isuser);
            ses_putchar(scb, ',');
            ses_putstr_indent(scb, (const xmlChar *)"parentval,",
                              indent+indent);
            ses_putstr_indent(scb, (const xmlChar *)"&childval);",
                              indent+indent);
            write_if_res(scb, cp, indent);
            ses_putchar(scb, '\n');

            /* any config=false container gets an MRO function */
            ses_putstr_indent(scb, (const xmlChar *)"res = ", indent);
            ses_putstr(scb, childcdef->idstr);
            ses_putstr(scb, MRO_SUFFIX);
            ses_putstr(scb, (const xmlChar *)"(childval);");
            write_if_res(scb, cp, indent);
            ses_putchar(scb, '\n');
            child_done = TRUE;
            break;
        case OBJ_TYP_ANYXML:

            break;
        case OBJ_TYP_CHOICE:
            child_done = TRUE;  // nothing to do
            break;
        case OBJ_TYP_CASE:
            child_done = TRUE;  // nothing to do
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (!child_done) {
            /* treat as not-handled flag!!! */
            ses_putstr_indent(scb, (const xmlChar *)"/* ", indent);
            ses_putstr(scb, obj_get_typestr(obj));
            ses_putchar(scb, ' ');
            ses_putstr(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" not handled!!! */");
        }
    }

    if (fullmode) {
        /* return result */
        ses_putchar(scb, '\n');
        ses_indent(scb, indent);
        ses_putstr(scb, (const xmlChar *)"return res;");

        /* end the function */
        ses_putstr(scb, (const xmlChar *)"\n\n} /* ");
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, MRO_SUFFIX);
        ses_putstr(scb, (const xmlChar *)" */");
        if (dlq_empty(&obj->iffeatureQ)) {
            ses_putchar(scb, '\n');
        }
    }
} /* write_c_mro_fn */


/********************************************************************
* FUNCTION write_c_top_mro_fn
* 
* Generate the C code for the foo_object_mro function
* Make virtual read-only top-level object
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   obj == object struct for the database object
*   objnameQ == Q of c_define_t structs to search for this object
*********************************************************************/
static void
    write_c_top_mro_fn (ses_cb_t *scb,
                        ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        obj_template_t *obj,
                        dlq_hdr_t *objnameQ)
{
    c_define_t      *cdef;
    int32            indent;

    mod = mod; // Warning suppression 

    switch (obj->objtype) {
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_ANYXML:
        return;
    default:
        ;
    }

    indent = cp->indent;

    cdef = find_path_cdefine(objnameQ, obj);
    if (cdef == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    /* generate function banner comment */
    if (dlq_empty(&obj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }
    ses_putstr(scb, FN_BANNER_START);
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, MRO_SUFFIX);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Make read-only top-level node");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Path: ");
    ses_putstr(scb, cdef->valstr);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    ses_putstr(scb, (const xmlChar *)"\nstatic status_t");
    ses_putstr_indent(scb, cdef->idstr, indent);
    ses_putstr(scb, MRO_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" (void)");
    ses_putstr(scb, (const xmlChar *)"\n{");

    /* generate static vars */
    if (obj_is_np_container(obj)) {
        ses_putstr_indent(scb, 
                          (const xmlChar *)
                          "val_value_t *parentval = NULL, "
                          "*childval = NULL;",
                          indent);
    }
    ses_putstr_indent(scb, 
                      (const xmlChar *)"status_t res = NO_ERR;",
                      indent);
    ses_putchar(scb, '\n');

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* generate callback logging */
        write_cb_logging_init(scb, cdef->idstr, MRO_SUFFIX, indent);
        write_cb_logging_add_cb(scb, ncx_get_modname(mod), TRUE, indent);
    }

    /* generate 'add <path>' comment */
    ses_putchar(scb, '\n');
    ses_putstr_indent(scb, 
                      (const xmlChar *)"/* add ",
                      indent);
    ses_putstr(scb, cdef->valstr);
    ses_putstr(scb, (const xmlChar *)" */");

    /* generate 'add <path>' code */    
    if (obj_is_np_container(obj)) {
        /* call agt_add_top_container(...)  */
        ses_putstr_indent(scb, 
                          (const xmlChar *)
                          "res = agt_add_top_container(",
                          indent);
        write_c_safe_str(scb, obj_get_name(obj));
        ses_putstr(scb, (const xmlChar *)"_obj, &parentval);");
        write_if_res(scb, cp, indent);
        write_c_mro_fn(scb, mod, cp, obj, FALSE, objnameQ);
    } else if (obj_is_leafy(obj)) {
        /* call agt_add_top_virtual(...)  */
        ses_putstr_indent(scb, 
                          (const xmlChar *)
                          "res = agt_add_top_virtual(",
                          indent);
        ses_indent(scb, indent+indent);
        write_c_safe_str(scb, obj_get_name(obj));
        ses_putstr(scb, (const xmlChar *)"_obj,");
        ses_indent(scb, indent+indent);
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, GET_SUFFIX);
        ses_putstr(scb, (const xmlChar *)");");
    } else {
        ses_putstr_indent(scb, 
                          (const xmlChar *)
                          "/* add code to generate top-level "
                          "node here */",
                          indent);
        ses_putstr_indent(scb, 
                          (const xmlChar *)"res = NO_ERR;",
                          indent);
    }
    /* return result */
    ses_putchar(scb, '\n');
    ses_indent(scb, indent);
    ses_putstr(scb, (const xmlChar *)"return res;");

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n\n} /* ");
    ses_putstr(scb, cdef->idstr);
    ses_putstr(scb, MRO_SUFFIX);
    ses_putstr(scb, (const xmlChar *)" */");
    if (dlq_empty(&obj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }

} /* write_c_top_mro_fn */


/********************************************************************
* FUNCTION write_c_objects
* 
* Generate the callback functions for each object found
*
* INPUTS:
*   scb == session to use
*   mod == module in progress
*   cp == conversion parameters to use
*   datadefQ == que of obj_template_t to use
*   objnameQ == Q of c_define_t structs to use
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*
*********************************************************************/
static void
    write_c_objects (ses_cb_t *scb,
                     ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     dlq_hdr_t *datadefQ,
                     dlq_hdr_t *objnameQ,
                     boolean uprotomode)
{
    obj_template_t    *obj;
    dlq_hdr_t         *childdatadefQ;

    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) ||
            !obj_is_data_db(obj) ||
            obj_is_cli(obj) ||
            obj_is_abstract(obj)) {
            continue;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);

        /* generate functions in reverse order so parent functions
         * will be able to access decendant functions without
         * any forward function declarations
         */
        childdatadefQ = obj_get_datadefQ(obj);
        if (childdatadefQ) {
            write_c_objects(scb, mod, cp, childdatadefQ, objnameQ, uprotomode);
        }

        /* generate the callback function: edit or get */
        if (obj_get_config_flag(obj)) {
            if (!uprotomode) {
                /* check if this node has any non-config children */
                if (!cp->isuser && obj_has_ro_children(obj)) {
                    write_c_mro_fn(scb, mod, cp, obj, TRUE, objnameQ);
                }
            }

            /* generate the foo_edit function, except for
             * choice nodes and case nodes
             */
            if (!(obj->objtype == OBJ_TYP_CHOICE ||
                  obj->objtype == OBJ_TYP_CASE)) {
                write_c_edit_cbfn(scb, cp, obj, objnameQ, uprotomode);
            }
        } else {
            if (obj_is_leafy(obj)) {
                /* generate the foo_get function */
                write_c_get_cbfn(scb, cp, obj, objnameQ, uprotomode);
            } else if (cp->isuser) {
                ; // do nothing
            } else if (obj_is_np_container(obj) && !obj_is_top(obj)) {
                write_c_mro_fn(scb, mod, cp, obj, TRUE, objnameQ);
            } else {
                log_warn("\nWarning: no get-CB generated "
                         "for top-level operational "
                         "%s '%s'",
                         obj_get_typestr(obj),
                         obj_get_name(obj));
            }
            if (!cp->isuser && obj_is_top(obj)) {
                /* handles top-level leafs and NP containers only ! */
                write_c_top_mro_fn(scb, mod, cp, obj, objnameQ);
            }
        }

        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

}  /* write_c_objects */


/********************************************************************
* FUNCTION write_c_rpc_fn
* 
* Generate the C code for the foo_rpc_fn_validate function
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   rpcobj == object struct for the RPC method
*   is_validate == TRUE for _validate, FALSE for _invoke
*   objnameQ == Q of c_define_t structs to use to find
*               the mapped C identifier for each object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_rpc_fn (ses_cb_t *scb,
                    ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    obj_template_t *rpcobj,
                    boolean is_validate,
                    dlq_hdr_t *objnameQ,
                    boolean uprotomode)
{
    const xmlChar *modname = ncx_get_modname(mod);
    int32 indent = cp->indent;
    obj_template_t *inputobj = obj_find_child(rpcobj, NULL, YANG_K_INPUT);
    boolean hasinput = (inputobj && obj_has_children(inputobj));

    /* generate function banner comment */
    if (dlq_empty(&rpcobj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL, obj_get_name(rpcobj), cp->isuser);
    if (is_validate) {
        ses_putstr(scb, (const xmlChar *)"_validate");
    } else {
        ses_putstr(scb, (const xmlChar *)"_invoke");
    }
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    if (is_validate) {
        ses_putstr(scb, (const xmlChar *)"RPC validation phase");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"All YANG constraints have "
                   "passed at this point.");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"Add description-stmt checks "
                   "in this function.");
    } else {
        ses_putstr(scb, (const xmlChar *)"RPC invocation phase");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"All constraints have "
                   "passed at this point.");
        ses_putstr(scb, FN_BANNER_LN);
        ses_putstr(scb, (const xmlChar *)"Call "
                   "device instrumentation code "
                   "in this function.");
    }
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_INPUT);
    ses_putstr(scb, (const xmlChar *)"    see agt/agt_rpc.h for details");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern status_t ");
    } else if (cp->isuser) {
        ses_putstr(scb, (const xmlChar *)"\nstatus_t ");
    } else {
        ses_putstr(scb, (const xmlChar *)"\nstatic status_t ");
    }
    write_identifier(scb, modname, NULL, obj_get_name(rpcobj), cp->isuser);
    if (is_validate) {
        ses_putstr(scb, (const xmlChar *)"_validate (");
    } else {
        ses_putstr(scb, (const xmlChar *)"_invoke (");
    }
    ses_putstr_indent(scb, (const xmlChar *)"ses_cb_t *scb,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"rpc_msg_t *msg,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"xml_node_t *methnode)", indent);

    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* begin function body */
    ses_putstr(scb, (const xmlChar *)"\n{");
    ses_putstr_indent(scb, (const xmlChar *)"status_t res = NO_ERR;", indent);

    if (is_validate) {
        ses_putstr_indent(scb, (const xmlChar *)"val_value_t *errorval = NULL;",
                          indent);
    }
    ses_putchar(scb, '\n');


    if (hasinput) {
        /* declare value pointer node variables for 
         * the RPC input parameters 
         */
        obj_template_t *obj;
        for (obj = obj_first_child(inputobj);
             obj != NULL;
             obj = obj_next_child(obj)) {

            if (obj_is_abstract(obj) || !obj_has_name(obj)) {
                continue;
            }

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            ses_putstr_indent(scb, (const xmlChar *)"val_value_t *", indent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)"_val;");
            ses_indent(scb, indent);
            write_c_objtype_ex(scb, obj, objnameQ, ';', TRUE, FALSE);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }

        /* retrieve the parameter from the input */
        for (obj = obj_first_child(inputobj);
             obj != NULL;
             obj = obj_next_child(obj)) {

            if (obj_is_abstract(obj) || !obj_has_name(obj)) {
                continue;
            }

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            boolean iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ, 
                                                      cp, indent);
            int32 extraindent = (iffdone) ? indent : 0;

            ses_putchar(scb, '\n');
            ses_indent(scb, indent+extraindent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)"_val = val_find_child(");

            ses_putstr_indent(scb, (const xmlChar *)"msg->rpc_input,",
                              indent+indent+extraindent);
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE, obj_get_name(obj), FALSE);
            ses_putstr(scb, (const xmlChar *)");");
            ses_putstr_indent(scb, (const xmlChar *)"if (", indent+extraindent);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)"_val != NULL && ");
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)"_val->res == NO_ERR) {");

            if (obj_is_leafy(obj)) {
                ses_indent(scb, indent+indent+extraindent);
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putstr(scb, (const xmlChar *)" = ");
                write_c_val_macro_type(scb, obj);
                ses_putchar(scb, '(');
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putstr(scb, (const xmlChar *)"_val);");
            } else {
                ses_putstr_indent(scb, (const xmlChar *)"/* replace the "
                                  "following line with real code to "
                                  "fill in structure */", 
                                  indent+indent+extraindent);
                ses_putstr_indent(scb, (const xmlChar *)"(void)",
                                  indent+indent+extraindent);
                write_c_safe_str(scb, obj_get_name(obj));
                ses_putchar(scb, ';');
            }
            ses_indent(scb, indent+extraindent);
            ses_putchar(scb, '}');

            write_c_iffeature_end(scb, &obj->iffeatureQ, indent);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }
    }

    if (is_validate) {
        /* write generic record error code */
        write_if_record_error(scb, cp, indent, TRUE, FALSE, TRUE);
    } else {
        ses_putchar(scb, '\n');
        ses_putstr_indent(scb, (const xmlChar *)"/* remove the next line "
                          "if scb is used */", indent);
        ses_putstr_indent(scb, (const xmlChar *)"(void)scb;", indent);

        if (!hasinput) {
            ses_putchar(scb, '\n');
            ses_putstr_indent(scb, (const xmlChar *)"/* remove the next line "
                              "if msg is used */", indent);
            ses_putstr_indent(scb, (const xmlChar *)"(void)msg;", indent);
        }

        ses_putchar(scb, '\n');
        ses_putstr_indent(scb, (const xmlChar *)"/* remove the next line "
                          "if methnode is used */", indent);
        ses_putstr_indent(scb, (const xmlChar *)"(void)methnode;", indent);

        ses_putchar(scb, '\n');
        ses_putstr_indent(scb, (const xmlChar *)"/* invoke your device "
                          "instrumentation code here */\n", indent);
    }

    if (cp->format == NCX_CVTTYP_CPP_TEST) {
        /* generate callback logging */
        ses_putchar(scb, '\n');
        ses_putstr_indent(scb,
                          (const xmlChar *)"YumaTest::SILCallbackLog& "
                          "cbLog = YumaTest::SILCallbackLog::getInstance();",
                          indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)
                          "YumaTest::SILCallbackLog::CallbackInfo cbData;",
                          indent);
        ses_putstr_indent(scb,
                          (const xmlChar *)"cbData.cbName = \"",
                          indent);
        write_identifier(scb, modname, NULL, obj_get_name(rpcobj), cp->isuser);
        if (is_validate) {
            ses_putstr(scb, (const xmlChar *)"_validate");
        } else {
            ses_putstr(scb, (const xmlChar *)"_invoke");
        }
        ses_putstr(scb, (const xmlChar *)"\";");
        write_cb_logging_add_cb(scb, ncx_get_modname(mod), TRUE, indent);
        ses_putchar(scb, '\n');
    }

    /* return NO_ERR */
    ses_indent(scb, indent);
    ses_putstr(scb, (const xmlChar *)"return res;");

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n\n} /* ");
    write_identifier(scb, modname, NULL, obj_get_name(rpcobj), cp->isuser);
    if (is_validate) {
        ses_putstr(scb, (const xmlChar *)"_validate */");
    } else {
        ses_putstr(scb, (const xmlChar *)"_invoke */");
    }
    if (dlq_empty(&rpcobj->iffeatureQ) || is_validate) {
        ses_putchar(scb, '\n');
    }

} /* write_c_rpc_fn */


/********************************************************************
* FUNCTION write_c_rpcs
* 
* Generate the C file decls for the RPC methods in the 
* specified module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == que of obj_template_t to use
*   cp == conversion parameters to use
*   objnameQ == Q of c_define_t structs to use to find
*               the mapped C identifier for each object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_rpcs (ses_cb_t *scb,
                  ncx_module_t *mod,
                  const yangdump_cvtparms_t *cp,
                  dlq_hdr_t *objnameQ,
                  boolean uprotomode)
{
    obj_template_t    *obj;

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_is_rpc(obj) || 
            obj_is_abstract(obj)) {
            continue;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        write_c_rpc_fn(scb, mod, cp, obj, TRUE, objnameQ, uprotomode);
        write_c_rpc_fn(scb, mod, cp, obj, FALSE, objnameQ, uprotomode);
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

}  /* write_c_rpcs */


/********************************************************************
* FUNCTION write_c_notif
* 
* Generate the C file decls for 1 notification
*
* INPUTS:
*   scb == session control block to use for writing
*   datadefQ == que of obj_template_t to use
*   cp == conversion parms to use
*   objnameQ == Q of c_define_t structs to use to find
*               the mapped C identifier for each object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_notif (ses_cb_t *scb,
                   obj_template_t *notifobj,
                   const yangdump_cvtparms_t *cp,
                   dlq_hdr_t *objnameQ,
                   boolean uprotomode)
{
    const xmlChar *modname = obj_get_mod_name(notifobj);
    int32 indent = cp->indent;
    boolean haspayload = obj_has_children(notifobj);

    /* generate function banner comment */
    if (dlq_empty(&notifobj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL,
                     obj_get_name(notifobj), cp->isuser);
    ses_putstr(scb, (const xmlChar *)"_send");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Send a ");
    write_identifier(scb, obj_get_mod_name(notifobj), NULL,
                     obj_get_name(notifobj), cp->isuser);
    ses_putstr(scb, (const xmlChar *)" notification");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"Called by your code when "
               "notification event occurs");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern void ");
    } else {
        ses_putstr(scb, (const xmlChar *)"\nvoid ");
    }
    write_identifier(scb, obj_get_mod_name(notifobj), NULL,
                     obj_get_name(notifobj), cp->isuser);
    ses_putstr(scb, (const xmlChar *)"_send (");

    boolean anydone = FALSE;
    obj_template_t  *obj, *nextobj;
    for (obj = obj_first_child(notifobj); obj != NULL; obj = nextobj) {
        nextobj = obj_next_child(obj);
        if (!obj_has_name(obj)) {
            continue;
        }
        anydone = TRUE;
        ses_indent(scb, indent);
        write_c_objtype_ex(scb, obj, objnameQ,
                           (nextobj == NULL) ? ')' : ',', TRUE, TRUE);
    }
    if (!anydone) {
        ses_putstr(scb, (const xmlChar *)"void)");
    }
    if (uprotomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }
    
    /* start function body */
    ses_putchar(scb, '\n');
    ses_putchar(scb, '{');

    /* declare local variables */
    ses_putstr_indent(scb, (const xmlChar *)"agt_not_msg_t *notif;",
                      indent);

    if (haspayload) {
        ses_putstr_indent(scb, (const xmlChar *)"val_value_t *parmval;",
                          indent);
        ses_putstr_indent(scb, (const xmlChar *)"status_t res = NO_ERR;",
                          indent);
    }

    /* make the entire contents conditional */
    ses_putchar(scb, '\n');
    boolean iffdone = write_c_iffeature_start(scb, &notifobj->iffeatureQ, 
                                              cp, indent);
    int32 extraindent = (iffdone) ? indent : 0;

    /* print a debug message */
    ses_putchar(scb, '\n');
    ses_putstr_indent(scb, (const xmlChar *)"if (LOGDEBUG) {",
                      indent+extraindent);
    ses_putstr_indent(scb,
                      (const xmlChar *)"log_debug(\"\\nGenerating <",
                      indent+indent+extraindent);
    ses_putstr(scb, obj_get_name(notifobj));
    ses_putstr(scb, (const xmlChar *)"> notification\");");
    ses_putstr_indent(scb, (const xmlChar *)"}\n", indent+extraindent);

    /* allocate a new notification */
    ses_putstr_indent(scb,
                      (const xmlChar *)"notif = agt_not_new_notification(",
                      indent+extraindent);
    write_c_safe_str(scb, obj_get_name(notifobj));
    ses_putstr(scb, (const xmlChar *)"_obj);");
    ses_putstr_indent(scb, (const xmlChar *)"if (notif == NULL) {",
                      indent+extraindent);
    ses_putstr_indent(scb, (const xmlChar *)"log_error(\"\\nError: "
                      "malloc failed, cannot send \"",
                      indent+indent+extraindent);
    ses_putstr_indent(scb, (const xmlChar *)"\"<", indent+indent+extraindent);
    ses_putstr(scb, obj_get_name(notifobj));
    ses_putstr(scb, (const xmlChar *)"> notification\");");
    ses_putstr_indent(scb, (const xmlChar *)"return;", 
                      indent+indent+extraindent);
    ses_putstr_indent(scb, (const xmlChar *)"}\n", indent+extraindent);

    /* generate code to construct the notification payload */
    for (obj = obj_first_child(notifobj);
         obj != NULL;
         obj = obj_next_child(obj)) {

        if (!obj_has_name(obj)) {
            continue;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        boolean objiffdone = 
            write_c_iffeature_start(scb, &obj->iffeatureQ, cp, 
                                    indent+extraindent);
        int32 extra2 = (objiffdone) ? indent : 0;

        if (obj->objtype != OBJ_TYP_LEAF) {
            /* TBD: need to handle non-leafs automatically */
            ses_putstr_indent(scb, (const xmlChar *)"/* add ", 
                              indent+extraindent+extra2);
            ses_putstr(scb, obj_get_typestr(obj));
            ses_putchar(scb, ' ');
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" to payload");
            ses_putstr_indent(scb, (const xmlChar *)" * replace following line"
                              " with real code", indent+extraindent+extra2);
            ses_putstr_indent(scb, (const xmlChar *)" */", 
                              indent+extraindent+extra2);
            ses_putstr_indent(scb, (const xmlChar *)"(void)", 
                              indent+extraindent+extra2);
            write_c_safe_str(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)";\n");
            write_c_iffeature_end(scb, &obj->iffeatureQ, indent+extraindent);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
            continue;
        }

        /* this is a leaf parameter -- add starting comment */
        ses_putstr_indent(scb, (const xmlChar *)"/* add ", 
                          indent+extraindent+extra2);
        write_c_safe_str(scb, obj_get_name(obj));
        ses_putstr(scb, (const xmlChar *)" to payload */");

        ncx_btype_t btyp = obj_get_basetype(obj);
        const xmlChar *fname;
        switch (btyp) {
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            fname = (const xmlChar *)"agt_make_uint_leaf";
            break;
        case NCX_BT_UINT64:
            fname = (const xmlChar *)"agt_make_uint64_leaf";
            break;
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
            fname = (const xmlChar *)"agt_make_int_leaf";
            break;
        case NCX_BT_INT64:
            fname = (const xmlChar *)"agt_make_int64_leaf";
            break;
        case NCX_BT_IDREF:
            fname = (const xmlChar *)"agt_make_idref_leaf";
            break;
        default:
            /* not all types are covered yet!!! such as:
             * uint64, int64, decimal64, binary, boolean
             * instance-identifier, leafref
             */
            fname = (const xmlChar *)"agt_make_leaf";
        }

        /* agt_make_leaf stmt */
        ses_putstr_indent(scb, (const xmlChar *)"parmval = ", 
                          indent+extraindent+extra2);
        ses_putstr(scb, fname);
        ses_putchar(scb, '(');

        ses_indent(scb, indent+indent+extraindent+extra2);
        write_c_safe_str(scb, obj_get_name(notifobj));
        ses_putstr(scb, (const xmlChar *)"_obj,");
        ses_indent(scb, indent+indent+extraindent+extra2);
        write_identifier(scb, modname, BAR_NODE, obj_get_name(obj), FALSE);
        ses_putchar(scb, ',');
        ses_indent(scb, indent+indent+extraindent+extra2);
        write_c_safe_str(scb, obj_get_name(obj));
        ses_putchar(scb, ',');
        ses_putstr_indent(scb, (const xmlChar *)"&res);", 
                          indent+indent+extraindent);

        /* check result stmt, Q leaf if non-NULL */
        ses_putstr_indent(scb, (const xmlChar *)"if (parmval == NULL) {",
                          indent+extraindent);
        ses_putstr_indent(scb, (const xmlChar *)"log_error(", 
                          indent+indent+extraindent+extra2);
        ses_putstr_indent(scb, (const xmlChar *)"\"\\nError: "
                          "make leaf failed (%s), cannot send \"",
                          (indent*3)+extraindent+extra2);
        ses_putstr_indent(scb, (const xmlChar *)"\"<",
                          (indent*3)+extraindent+extra2);
        ses_putstr(scb, obj_get_name(notifobj));
        ses_putstr(scb, (const xmlChar *)"> notification\",");
        ses_putstr_indent(scb, (const xmlChar *)"get_error_string(res));",
                          (indent*3)+extraindent+extra2);
        ses_putstr_indent(scb, (const xmlChar *)"} else {", 
                          indent+extraindent);
        ses_putstr_indent(scb, (const xmlChar *)"agt_not_add_to_payload"
                          "(notif, parmval);", 
                          indent+indent+extraindent+extra2);
        ses_putstr_indent(scb, (const xmlChar *)"}\n", 
                          indent+extraindent+extra2);
        write_c_iffeature_end(scb, &obj->iffeatureQ, indent+extraindent);
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

    /* save the malloced notification struct */
    ses_putstr_indent(scb,
                      (const xmlChar *)"agt_not_queue_notification(notif);\n",
                      indent+extraindent);

    write_c_iffeature_end(scb, &notifobj->iffeatureQ, indent);

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n} /* ");
    write_identifier(scb, modname, NULL, obj_get_name(notifobj), cp->isuser);
    ses_putstr(scb, (const xmlChar *)"_send */");
    if (dlq_empty(&notifobj->iffeatureQ)) {
        ses_putchar(scb, '\n');
    }

}  /* write_c_notif */


/********************************************************************
* FUNCTION write_c_notifs
* 
* Generate the C file decls for the notifications in the 
* specified datadefQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parms to use
*   objnameQ == Q of c_define_t structs to use to find
*               the mapped C identifier for each object
*   uprotomode == TRUE: just generate a format==NCX_CVTTYP_UH prototype
*                 FALSE: normal mode
*********************************************************************/
static void
    write_c_notifs (ses_cb_t *scb,
                    ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    dlq_hdr_t *objnameQ,
                    boolean uprotomode)
{
    obj_template_t    *obj;

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_is_notif(obj) ||
            obj_is_abstract(obj)) {
            continue;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        write_c_notif(scb, obj, cp, objnameQ, uprotomode);
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

}  /* write_c_notifs */


/********************************************************************
* FUNCTION write_c_register_datacb
* 
* Generate the C code for registering a data node callback fn
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   obj == object to generate init function calls for
*   cdefQ == Q of ncx_define_t structs to search
*   startindent == starting indent amount
*********************************************************************/
static void
    write_c_register_datacb (ses_cb_t *scb,
                             ncx_module_t *mod,
                             const yangdump_cvtparms_t *cp,
                             obj_template_t *obj,
                             dlq_hdr_t *cdefQ,
                             int32 startindent)
{
    if (!obj_get_config_flag(obj)) {
        /* nothing to do now for read-only objects */
        return;
    }

    int32 indent = startindent;
    if (obj->objtype != OBJ_TYP_CHOICE && obj->objtype != OBJ_TYP_CASE) {

        const c_define_t *cdef = find_path_cdefine(cdefQ, obj);
        if (cdef == NULL) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        boolean iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ,
                                                  cp, startindent);
        if (iffdone) {
            indent += cp->indent;
        }

        /* register the edit callback */
        ses_putstr_indent(scb, (const xmlChar *)"res = agt_cb_"
                          "register_callback(", indent);

        /* modname parameter */
        const xmlChar *modname = ncx_get_modname(mod);
        ses_indent(scb, indent + cp->indent);
        write_identifier(scb, modname, BAR_MOD, modname, FALSE);
        ses_putchar(scb, ',');

        /* defpath parameter */
        ses_indent(scb, indent + cp->indent);
        ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
        ses_putstr(scb, cdef->valstr);
        ses_putchar(scb, '"');
        ses_putchar(scb, ',');

        /* version parameter */
        if (mod->version) {
            ses_indent(scb, indent + cp->indent);
            write_identifier(scb, modname, BAR_REV, modname, cp->isuser);
            ses_putchar(scb, ',');
        } else {
            ses_putstr_indent(scb, (const xmlChar *)"NULL,", 
                              indent + cp->indent);
        }

        /* cbfn parameter: 
         * use the static function in the YC or C file
         */
        ses_indent(scb, indent + cp->indent);
        ses_putstr(scb, cdef->idstr);
        ses_putstr(scb, (const xmlChar *)"_edit);");
        write_if_res(scb, cp, indent);
        ses_putchar(scb, '\n');
    }

    obj_template_t *childobj = obj_first_child(obj);
    for (; childobj != NULL; childobj = obj_next_child(childobj)) {
        if (obj_has_name(childobj) && !obj_is_cli(childobj) &&
            !obj_is_abstract(childobj) && obj_is_data_db(childobj)) {

            write_c_register_datacb(scb, mod, cp, childobj, cdefQ, indent);
        }
    }

    if (obj->objtype != OBJ_TYP_CHOICE && obj->objtype != OBJ_TYP_CASE) {
        write_c_iffeature_end(scb, &obj->iffeatureQ, startindent);
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

}  /* write_c_register_datacb */


/********************************************************************
* FUNCTION write_c_feature
* 
* Generate the enable or disable code for 1 feature statement
*
* #ifdef <feature>
*   ncx_set_feature_enable_entry(<feature>, TRUE);
* #else
*   ncx_set_feature_enable_entry(<feature>, FALSE);
* #endif
*
* INPUTS:
*   scb == session control block to use for writing
*   feature == ncx_feature_t to use
*   cp == conversion parameters to use
*********************************************************************/
static void
    write_c_feature (ses_cb_t *scb,
                     const ncx_feature_t *feature,
                     const yangdump_cvtparms_t *cp)
{
    int32 indent = cp->indent;
    const xmlChar *modname = feature->tkerr.mod->name;

    ses_putstr(scb, POUND_IFDEF);
    write_identifier(scb, modname, BAR_FEAT, feature->name, TRUE);
    ses_putstr_indent(scb, (const xmlChar *)"ncx_set_feature_enable(", indent);
    ses_indent(scb, indent+indent);
    write_identifier(scb, modname, BAR_MOD, modname, FALSE);
    ses_putchar(scb, ',');
    ses_indent(scb, indent+indent);
    ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
    ses_putstr(scb, feature->name);
    ses_putstr(scb, (const xmlChar *)"\",");
    ses_indent(scb, indent+indent);
    ses_putstr(scb, (const xmlChar *)"TRUE);");

    ses_putstr(scb, POUND_ELSE);

    ses_putstr_indent(scb, (const xmlChar *)"ncx_set_feature_enable(", indent);
    ses_indent(scb, indent+indent);
    write_identifier(scb, modname, BAR_MOD, modname, FALSE);
    ses_putchar(scb, ',');
    ses_indent(scb, indent+indent);
    ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
    ses_putstr(scb, feature->name);
    ses_putstr(scb, (const xmlChar *)"\",");
    ses_indent(scb, indent+indent);
    ses_putstr(scb, (const xmlChar *)"FALSE);");

    ses_putstr(scb, POUND_ENDIF);
    ses_putchar(scb, '\n');

}  /* write_c_feature */


/********************************************************************
* FUNCTION write_c_features
* 
* Generate the C code to enable or disable the module YANG features
*
* INPUTS:
*   scb == session control block to use for writing
*   featureQ == que of ncx_feature_t to use
*   cp == conversion parameters to use
*
*********************************************************************/
static void
    write_c_features (ses_cb_t *scb,
                      const dlq_hdr_t *featureQ,
                      const yangdump_cvtparms_t *cp)
{
    const ncx_feature_t *feature;

    if (dlq_empty(featureQ)) {
        return;
    }

    ses_putchar(scb, '\n');
    for (feature = (const ncx_feature_t *)dlq_firstEntry(featureQ);
         feature != NULL;
         feature = (const ncx_feature_t *)dlq_nextEntry(feature)) {

        write_c_feature(scb, feature, cp);
    }

}  /* write_c_features */


/********************************************************************
* FUNCTION write_c_init_fn
* 
* Generate the C code for the foo_init function
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   objnameQ == Q of c_define_t structs to use to find
*               the mapped C identifier for each object
*   protomode == TRUE: just generate any H format prototype
*                FALSE: normal mode
*********************************************************************/
static void
    write_c_init_fn (ses_cb_t *scb,
                     ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     dlq_hdr_t *objnameQ,
                     boolean protomode)
{
    const xmlChar   *modname = ncx_get_modname(mod);
    int32            indent = cp->indent;

    /* generate function banner comment */
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init", cp->isuser);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"initialize the ");
    ses_putstr(scb, mod->name);
    ses_putstr(scb, (const xmlChar *)" server instrumentation library");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_INPUT);
    ses_putstr(scb, (const xmlChar *)"   modname == requested module name");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, (const xmlChar *)"   revision == requested "
               "version (NULL for any)");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (protomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern status_t ");
    } else {
        ses_putstr(scb, (const xmlChar *)"\nstatus_t ");
    }
    write_identifier(scb, modname, NULL, (const xmlChar *)"init", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" (");
    ses_putstr_indent(scb, (const xmlChar *)"const xmlChar *modname,", indent);
    ses_putstr_indent(scb, (const xmlChar *)"const xmlChar *revision)",
                      indent);

    if (protomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* start function body */
    ses_putstr(scb, (const xmlChar *)"\n{");
    ses_putstr_indent(scb, (const xmlChar *)"status_t res = NO_ERR;", indent);
    
    if (cp->isuser) {
        /* --format=uc so this init section not needed; just init
         * a local module var to get the module loaded by the y_init fn */
        ses_putstr_indent(scb, (const xmlChar *)"ncx_module_t *", indent);
        write_c_safe_str(scb, modname);
        ses_putstr(scb, (const xmlChar *)"_mod = NULL;");
    } else {
        /* declare the function variables */
        ses_putstr_indent(scb, (const xmlChar *)
                          "agt_profile_t *agt_profile = agt_get_profile();",
                          indent);

        /* call the init_static_vars function */
        ses_putchar(scb, '\n');
        ses_indent(scb, indent);
        write_identifier(scb, modname, NULL,
                         (const xmlChar *)"init_static_vars", cp->isuser);
        ses_putstr(scb, (const xmlChar *)"();\n");

        /* check the input variables */
        ses_putstr_indent(scb, (const xmlChar *)"/* change if custom handling "
                          "done */",
                          indent);
        ses_putstr_indent(scb, (const xmlChar *)"if (xml_strcmp(modname, ",
                          indent);
        write_identifier(scb, modname, BAR_MOD, modname, cp->isuser);
        ses_putstr(scb, (const xmlChar *)")) {");
        ses_putstr_indent(scb, 
                          (const xmlChar *)"return ERR_NCX_UNKNOWN_MODULE;",
                          indent+indent);
        ses_indent(scb, indent);
        ses_putstr(scb, (const xmlChar *)"}\n");

        ses_putstr_indent(scb, (const xmlChar *)"if (revision && "
                          "xml_strcmp(revision, ", indent);
        write_identifier(scb, modname, BAR_REV, modname, cp->isuser);
        ses_putstr(scb, (const xmlChar *)")) {");
        ses_putstr_indent(scb, 
                          (const xmlChar *)"return ERR_NCX_WRONG_VERSION;",
                          indent+indent);
        ses_indent(scb, indent);
        ses_putstr(scb, (const xmlChar *)"}");
    
    }

    /* load-time: enable or disable the module features set at 
     * compile time; the user can override this action at run-time */
    write_c_features(scb, &mod->featureQ, cp);

    /* load the module */
    if (mod->ismod) {
        /* user C so find the module already loaded */
        if (cp->format == NCX_CVTTYP_UC) {
            ses_indent(scb, indent);
            write_c_safe_str(scb, modname);
            ses_putstr(scb, (const xmlChar *)"_mod = ncx_find_module(");
            ses_indent(scb, indent+indent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');
            if (mod->version) {
                ses_indent(scb, indent+indent);
                write_identifier(scb, modname, BAR_REV, modname, FALSE);
            } else {
                ses_putstr_indent(scb, (const xmlChar *)" NULL", indent+indent);
            }
            ses_putstr(scb, (const xmlChar *)");");
            ses_putstr_indent(scb, (const xmlChar *)"if (", indent);
            write_c_safe_str(scb, modname);
            ses_putstr(scb, (const xmlChar *)"_mod == NULL) {");
            ses_putstr_indent(scb, (const xmlChar *)
                              "return ERR_NCX_OPERATION_FAILED;",
                              indent+indent);
            ses_putstr_indent(scb, (const xmlChar *)"}", indent);
        } else {
            /* main C or y_c so load the module */
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = ncxmod_load_module(",
                              indent);
            ses_indent(scb, indent+indent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');
            if (mod->version) {
                ses_indent(scb, indent+indent);
                write_identifier(scb, modname, BAR_REV, modname, FALSE);
                ses_putchar(scb, ',');
            } else {
                ses_putstr_indent(scb, (const xmlChar *)" NULL,", 
                                  indent+indent);
            }
            ses_putstr_indent(scb, 
                              (const xmlChar *)"&agt_profile->agt_savedevQ,",
                              indent+indent);
            ses_indent(scb, indent+indent);
            ses_putchar(scb, '&');
            write_c_safe_str(scb, modname);
            ses_putstr(scb, (const xmlChar *)"_mod);");
            write_if_res(scb, cp, indent);
            ses_putchar(scb, '\n');
        }
    }

    /* load the static object variable pointers */
    boolean iffdone = FALSE;
    int32 extraindent = 0;
    obj_template_t *obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
    for (; obj != NULL; obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) || obj_is_cli(obj) || obj_is_abstract(obj)) {
            continue;
        }

        if ((cp->format == NCX_CVTTYP_UC && obj_is_notif(obj)) ||
            (cp->format == NCX_CVTTYP_YC && !obj_is_notif(obj)) ||
            !(cp->format == NCX_CVTTYP_UC || cp->format == NCX_CVTTYP_YC)) {
        
            write_h_iffeature_start(scb, &obj->iffeatureQ);
            iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ,
                                              cp, indent);
            extraindent = (iffdone) ? indent : 0;
                
            ses_indent(scb, indent+extraindent);
            write_c_object_var(scb, obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)" = ncx_find_object(");
            ses_indent(scb, indent+indent+extraindent);
            write_c_safe_str(scb, modname);
            ses_putstr(scb, (const xmlChar *)"_mod,");
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE,
                             obj_get_name(obj), FALSE);
            ses_putstr(scb, (const xmlChar *)");");
            ses_putstr_indent(scb, (const xmlChar *)"if (", 
                              indent+extraindent);
            write_c_safe_str(scb, modname);
            ses_putstr(scb, (const xmlChar *)"_mod == NULL) {");
            ses_putstr_indent(scb, (const xmlChar *)"return SET_ERROR("
                              "ERR_NCX_DEF_NOT_FOUND);", 
                              indent+indent+extraindent);
            ses_putstr_indent(scb, (const xmlChar *)"}",
                              indent+extraindent);

            write_c_iffeature_end(scb, &obj->iffeatureQ, indent);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        
        }
    }

    if (!cp->isuser) {
        /* initialize any RPC methods */
        for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
             obj != NULL;
             obj = (obj_template_t *)dlq_nextEntry(obj)) {

            if (!obj_is_rpc(obj) || obj_is_abstract(obj)) {
                continue;
            }

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ,
                                              cp, indent);
            extraindent = (iffdone) ? indent : 0;

            /* register validate function */
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = agt_rpc_register_method(",
                              indent+extraindent);

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE, obj_get_name(obj), FALSE);
            ses_putchar(scb, ',');

            ses_putstr_indent(scb, (const xmlChar *)"AGT_RPC_PH_VALIDATE,",
                              indent+indent+extraindent);
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, NULL, obj_get_name(obj), 
                             cp->format == NCX_CVTTYP_YC);
            ses_putstr(scb, (const xmlChar *)"_validate);");

            write_if_res(scb, cp, indent+extraindent);
            ses_putchar(scb, '\n');

            /* register invoke function */
            ses_putstr_indent(scb, 
                              (const xmlChar *)"res = agt_rpc_register_method(",
                              indent+extraindent);

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE, obj_get_name(obj), FALSE);
            ses_putchar(scb, ',');

            ses_putstr_indent(scb, (const xmlChar *)"AGT_RPC_PH_INVOKE,",
                              indent+indent+extraindent);
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, NULL, obj_get_name(obj), 
                             cp->format == NCX_CVTTYP_YC);
            ses_putstr(scb, (const xmlChar *)"_invoke);");

            write_if_res(scb, cp, indent+extraindent);
            ses_putchar(scb, '\n');

            write_c_iffeature_end(scb, &obj->iffeatureQ, indent);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }

        /* initialize any object callbacks here */
        for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
             obj != NULL;
             obj = (obj_template_t *)dlq_nextEntry(obj)) {

            if (obj_has_name(obj) && obj_is_data_db(obj) && 
                !obj_is_cli(obj) && !obj_is_abstract(obj)) {
                write_c_register_datacb(scb, mod, cp, obj, objnameQ, indent);
            }
        }
    }

    if (cp->format == NCX_CVTTYP_YC) {
        /* call the user init1 function */
        ses_putstr_indent(scb, (const xmlChar *)"res = ", indent);
        write_identifier(scb, modname, NULL, (const xmlChar *)"init", TRUE);
        ses_putstr(scb, (const xmlChar *)"(modname, revision);");
    } else {
        ses_putstr_indent(scb,(const xmlChar *)"/* put your module "
                          "initialization code here */\n", indent);
    }

    /* return res; */
    ses_indent(scb, indent);
    ses_putstr(scb, (const xmlChar *)"return res;");

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n} /* ");
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" */\n");

} /* write_c_init_fn */


/********************************************************************
* FUNCTION write_c_init2_fn
* 
* Generate the C code for the foo_init2 function
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   objnameQ == Q of object ID bindings to use
*   protomode == TRUE: just generate any H format prototype
*                FALSE: normal mode
*********************************************************************/
static void
    write_c_init2_fn (ses_cb_t *scb,
                      ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      dlq_hdr_t *objnameQ,
                      boolean protomode)
{
    c_define_t      *cdef;
    const xmlChar   *modname;
    int32            indent;

    modname = ncx_get_modname(mod);
    indent = cp->indent;

    /* generate function banner comment */
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init2", cp->isuser);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, 
               (const xmlChar *)"SIL init phase 2: "
               "non-config data structures");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, 
               (const xmlChar *)"Called after running config is loaded");
    ses_putstr(scb, FN_BANNER_LN);

    ses_putstr(scb, FN_BANNER_RETURN_STATUS);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (protomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern status_t ");
    } else {
        ses_putstr(scb, (const xmlChar *)"\nstatus_t ");
    }
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init2", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" (void)");

    if (protomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* start function body */
    ses_putstr(scb, (const xmlChar *)"\n{");

    ses_putstr_indent(scb, (const xmlChar *)"status_t res = NO_ERR;", indent);

    if (!cp->isuser) {
        /* generate any cache inits here; top-level data-db objects only! */
        for (cdef = (c_define_t *)dlq_firstEntry(objnameQ);
             cdef != NULL;
             cdef = (c_define_t *)dlq_nextEntry(cdef)) {

            if (!obj_is_data_db(cdef->obj) ||
                !obj_is_top(cdef->obj) ||
                obj_is_cli(cdef->obj) ||
                obj_is_abstract(cdef->obj)) {
                continue;
            }

            write_h_iffeature_start(scb, &cdef->obj->iffeatureQ);
            boolean iffdone = 
                write_c_iffeature_start(scb, &cdef->obj->iffeatureQ, 
                                        cp, indent);
            int32 extraindent = (iffdone) ? indent : 0;

            /* check if this is a top-level config=false node */
            if (!obj_get_config_flag(cdef->obj)) {
                if (obj_is_leafy(cdef->obj) ||
                    obj_is_np_container(cdef->obj)) {
                    /* OBJ_TYP_CONTAINER(NP) or OBJ_TYP_LEAF
                     * or OBJ_TYP_LEAFLIST so create a
                     * top-level virtual leaf for this node
                     * TBD: support _mro functions for more 
                     * complex types
                     */
                    ses_putchar(scb, '\n');
                    ses_putstr_indent(scb, (const xmlChar *)"res = ",
                                      indent+extraindent);
                    ses_putstr(scb, cdef->idstr);
                    ses_putstr(scb, MRO_SUFFIX);
                    ses_putstr(scb, (const xmlChar *)"();");
                    write_if_res(scb, cp, indent+extraindent);
                }
                write_c_iffeature_end(scb, &cdef->obj->iffeatureQ, indent);
                write_h_iffeature_end(scb, &cdef->obj->iffeatureQ);
                continue;
            }

            /* init the cache value pointer for config=true top nodes */
            ses_putchar(scb, '\n');
            ses_indent(scb, indent+extraindent);
            write_c_value_var(scb, obj_get_name(cdef->obj));
            ses_putstr(scb, (const xmlChar *)" = agt_init_cache(");
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_MOD, modname, cp->isuser);
            ses_putchar(scb, ',');
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE,
                             obj_get_name(cdef->obj), cp->isuser);
            ses_putchar(scb, ',');
            ses_putstr_indent(scb, (const xmlChar *)"&res);",
                              indent+indent+extraindent);
            write_if_res(scb, cp, indent+extraindent);

            write_c_iffeature_end(scb, &cdef->obj->iffeatureQ, indent);
            write_h_iffeature_end(scb, &cdef->obj->iffeatureQ);
        }
    }

    ses_putchar(scb, '\n');
    if (cp->format == NCX_CVTTYP_YC) {
        /* call the user init2 function */
        ses_putstr_indent(scb, (const xmlChar *)"res = ", indent);
        write_identifier(scb, modname, NULL, (const xmlChar *)"init2", TRUE);
        ses_putstr(scb, (const xmlChar *)"();");
    } else {
        ses_putstr_indent(scb, 
                          (const xmlChar *)"/* put your init2 code here */",
                          indent);
    }

    ses_putchar(scb, '\n');
    ses_putstr_indent(scb, (const xmlChar *)"return res;", indent);

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n} /* ");
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"init2", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" */\n");

} /* write_c_init2_fn */


/********************************************************************
* FUNCTION write_c_register_datacb
* 
* Generate the C code for registering a data node callback fn
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   obj == object to generate init function calls for
*   cdefQ == Q of ncx_define_t structs to checl
*   startindent == starting indent amount
*********************************************************************/
static void
    write_c_unregister_datacb (ses_cb_t *scb,
                               ncx_module_t *mod,
                               const yangdump_cvtparms_t *cp,
                               obj_template_t *obj,
                               dlq_hdr_t *cdefQ,
                               int32 startindent)
{
    if (!obj_get_config_flag(obj)) {
        /* nothing to do now for read-only objects */
        return;
    }

    int32 indent = startindent;
    if (obj->objtype != OBJ_TYP_CHOICE && obj->objtype != OBJ_TYP_CASE) {

        const c_define_t *cdef = find_path_cdefine(cdefQ, obj);
        if (cdef == NULL) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }

        write_h_iffeature_start(scb, &obj->iffeatureQ);
        boolean iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ,
                                                  cp, startindent);

        /* unregister the edit callback */
        if (iffdone) {
            indent += cp->indent;
        }
        ses_putstr_indent(scb, (const xmlChar *)"agt_cb_unregister_callbacks(",
                          indent);
        ses_indent(scb, indent + cp->indent);
        const xmlChar *modname = ncx_get_modname(mod);
        write_identifier(scb, modname, BAR_MOD, modname, FALSE);
        ses_putchar(scb, ',');
        ses_indent(scb, indent + cp->indent);
        ses_putstr(scb, (const xmlChar *)"(const xmlChar *)\"");
        ses_putstr(scb, cdef->valstr);
        ses_putstr(scb, (const xmlChar *)"\");\n");
    }

    obj_template_t *childobj = obj_first_child(obj);
    for (; childobj != NULL; childobj = obj_next_child(childobj)) {
        if (obj_has_name(childobj) && !obj_is_cli(childobj) &&
            !obj_is_abstract(childobj) && obj_is_data_db(childobj)) {

            write_c_unregister_datacb(scb, mod, cp, childobj, cdefQ, indent);
        }
    }

    if (obj->objtype != OBJ_TYP_CHOICE && obj->objtype != OBJ_TYP_CASE) {
        write_c_iffeature_end(scb, &obj->iffeatureQ, startindent);
        write_h_iffeature_end(scb, &obj->iffeatureQ);
    }

}  /* write_c_unregister_datadb */


/********************************************************************
* FUNCTION write_c_cleanup_fn
* 
* Generate the C code for the foo_cleanup function
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   objnameQ == Q of object ID bindings to use
*   protomode == TRUE: just generate any H format prototype
*                FALSE: normal mode
*********************************************************************/
static void
    write_c_cleanup_fn (ses_cb_t *scb,
                        ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        dlq_hdr_t *objnameQ,
                        boolean protomode)
{
    const xmlChar   *modname = ncx_get_modname(mod);
    int32            indent = cp->indent;

    /* generate function banner comment */
    ses_putstr(scb, FN_BANNER_START);
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"cleanup", cp->isuser);
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, 
               (const xmlChar *)"   cleanup the server "
               "instrumentation library");
    ses_putstr(scb, FN_BANNER_LN);
    ses_putstr(scb, FN_BANNER_END);

    /* generate the function prototype lines */
    if (protomode) {
        ses_putstr(scb, (const xmlChar *)"\nextern void ");
    } else {
        ses_putstr(scb, (const xmlChar *)"\nvoid ");
    }
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"cleanup", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" (void)");

    if (protomode) {
        ses_putstr(scb, (const xmlChar *)";\n");
        return;
    }

    /* start function body */
    ses_putstr(scb, (const xmlChar *)"\n{");

    /* function contents */
    if (!cp->isuser) {
        /* cleanup any RPC methods */
        obj_template_t  *obj;
        for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
             obj != NULL;
             obj = (obj_template_t *)dlq_nextEntry(obj)) {

            if (!obj_is_rpc(obj) ||
                obj_is_abstract(obj)) {
                continue;
            }

            write_h_iffeature_start(scb, &obj->iffeatureQ);
            boolean iffdone = write_c_iffeature_start(scb, &obj->iffeatureQ,
                                                      cp, indent);
            int32 extraindent = (iffdone) ? indent : 0;

            /* unregister all callback fns for this RPC function */
            ses_putstr_indent(scb, 
                              (const xmlChar *)"\nagt_rpc_unregister_method(",
                              indent+extraindent);

            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_MOD, modname, FALSE);
            ses_putchar(scb, ',');
            ses_indent(scb, indent+indent+extraindent);
            write_identifier(scb, modname, BAR_NODE, obj_get_name(obj), FALSE);
            ses_putstr(scb, (const xmlChar *)");");

            write_c_iffeature_end(scb, &obj->iffeatureQ, indent);
            write_h_iffeature_end(scb, &obj->iffeatureQ);
        }

        /* cleanup any registered callbacks */
        for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
             obj != NULL;
             obj = (obj_template_t *)dlq_nextEntry(obj)) {

            if (obj_has_name(obj) && obj_is_data_db(obj) && 
                !obj_is_cli(obj) && !obj_is_abstract(obj)) {
                write_c_unregister_datacb(scb, mod, cp, obj, objnameQ, indent);
            }
        }
    }

    if (cp->format == NCX_CVTTYP_YC) {
        /* call the user cleanup function */
        ses_indent(scb, indent);
        write_identifier(scb, modname, NULL, (const xmlChar *)"cleanup", TRUE);
        ses_putstr(scb, (const xmlChar *)"();\n");
    } else {
        ses_putstr_indent(scb, (const xmlChar *)"/* put your cleanup code "
                          "here */\n", indent);
    }

    /* end the function */
    ses_putstr(scb, (const xmlChar *)"\n} /* ");
    write_identifier(scb, modname, NULL,
                     (const xmlChar *)"cleanup", cp->isuser);
    ses_putstr(scb, (const xmlChar *)" */\n");

} /* write_c_cleanup_fn */


/********************************************************************
* FUNCTION write_c_file
* 
* Generate the C file for the module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    write_c_file (ses_cb_t *scb,
                  ncx_module_t *mod,
                  const yangdump_cvtparms_t *cp)
{
    yang_node_t *node;
    dlq_hdr_t    objnameQ;
    status_t     res;

    dlq_createSQue(&objnameQ);
    res = NO_ERR;

    /* name strings for callback fns for objects */
    res = save_all_c_objects(mod, cp, &objnameQ, C_MODE_CALLBACK);
    if (res == NO_ERR) {
        /* file header, meta-data, static data decls */
        write_c_header(scb, mod, cp);
        write_c_includes(scb, mod, cp);
        write_c_static_vars(scb, mod, cp);
        write_c_init_static_vars_fn(scb, mod, cp);

        /* static functions */
        write_c_objects(scb, mod, cp, &mod->datadefQ, &objnameQ, FALSE);
        if (cp->unified && mod->ismod) {
            for (node = (yang_node_t *)
                     dlq_firstEntry(&mod->allincQ);
                 node != NULL && res == NO_ERR;
                 node = (yang_node_t *)dlq_nextEntry(node)) {
                if (node->submod) {
                    write_c_objects(scb, node->submod, cp,
                                    &node->submod->datadefQ,
                                    &objnameQ, FALSE);
                }
            }
        }

        if (cp->format != NCX_CVTTYP_YC) {
            write_c_rpcs(scb, mod, cp, &objnameQ, FALSE);
        }
        /* external functions */
        if (cp->format != NCX_CVTTYP_YC) {
            write_c_notifs(scb, mod, cp, &objnameQ, FALSE);
        }
        write_c_init_fn(scb, mod, cp, &objnameQ, FALSE);
        write_c_init2_fn(scb, mod, cp, &objnameQ, FALSE);
        write_c_cleanup_fn(scb, mod, cp, &objnameQ, FALSE);

        /* end comment */
        write_c_footer(scb, mod, cp);
    }

    clean_cdefineQ(&objnameQ);

    return res;

} /* write_c_file */


/*********     E X P O R T E D   F U N C T I O N S    **************/


/********************************************************************
* FUNCTION c_convert_module
* 
* Generate the SIL C code for the specified module(s)
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*
* RETURNS:
*   status
*********************************************************************/
status_t
    c_convert_module (yang_pcb_t *pcb,
                      const yangdump_cvtparms_t *cp,
                      ses_cb_t *scb)
{
    ncx_module_t  *mod = pcb->top;

#ifdef DEBUG
    if (!pcb || !cp || !scb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* the module should already be parsed and loaded */
    if (!mod) {
        return SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
    }

    /* do not generate C code for obsolete objects */
    ncx_delete_all_obsolete_objects();

    return write_c_file(scb, mod, cp);

}   /* c_convert_module */


/********************************************************************
* FUNCTION c_write_fn_prototypes
* 
* Generate the SIL H code for the external function definitions
* in the specified module, already parsed and H file generation
* is in progress
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*   objnameQ == Q of c_define_t mapping structs to use
*
*********************************************************************/
void
    c_write_fn_prototypes (ncx_module_t *mod,
                           const yangdump_cvtparms_t *cp,
                           ses_cb_t *scb,
                           dlq_hdr_t *objnameQ)
{
    yang_node_t *node;

#ifdef DEBUG
    if (!mod || !cp || !scb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (cp->isuser) {
        write_c_objects(scb, mod, cp, &mod->datadefQ, objnameQ, TRUE);
        write_c_rpcs(scb, mod, cp, objnameQ, TRUE);
    }
    if (cp->format == NCX_CVTTYP_H || cp->format == NCX_CVTTYP_UH) {
        write_c_notifs(scb, mod, cp, objnameQ, TRUE);
    }

    if (cp->unified && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                if (cp->isuser) {
                    write_c_objects(scb, node->submod, cp, 
                                    &node->submod->datadefQ, objnameQ, TRUE);
                    write_c_rpcs(scb, node->submod, cp, objnameQ, TRUE);
                } else {
                    write_c_notifs(scb, node->submod, cp, objnameQ, TRUE);
                }
            }
        }
    }

    if (mod->ismod) {
        write_c_init_fn(scb, mod, cp, objnameQ, TRUE);
        write_c_init2_fn(scb, mod, cp, objnameQ, TRUE);
        write_c_cleanup_fn(scb, mod, cp, objnameQ, TRUE);
    }

}   /* c_write_fn_prototypes */


/* END file c.c */
