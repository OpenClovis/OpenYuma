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
/*  FILE: yang_parse.c


   YANG module parser

    YANG modules are parsed in the following steps:

    1) basic tokenization and string processing (tk.c)
    2) first pass module processing, reject bad syntax
    3) resolve ranges, forward references, etc in typedefs
    4) resolve data model definitions (objects, rpcs, notifications)
    5) convert/copy obj_template_t data specifications to 
       typ_def_t representation


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
26oct07      abb      begun; start from ncx_parse.c


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>

#include "procdefs.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncx_appinfo.h"
#include "ncx_feature.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "obj.h"
#include "status.h"
#include "tstamp.h"
#include "typ.h"
#include "xml_util.h"
#include "yang.h"
#include "yangconst.h"
#include "yang_ext.h"
#include "yang_grp.h"
#include "yang_obj.h"
#include "yang_parse.h"
#include "yang_typ.h"
#include "yinyang.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define YANG_PARSE_DEBUG 1
/* #define YANG_PARSE_TK_DEBUG 1 */
/* #define YANG_PARSE_RDLN_DEBUG 1 */
/* #define YANG_PARSE_DEBUG_TRACE 1 */
/* #define YANG_PARSE_DEBUG_MEMORY 1 */
#endif

static status_t 
    consume_revision_date (tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           xmlChar **revstring);


/********************************************************************
* FUNCTION resolve_mod_appinfo
* 
* Validate the ncx_appinfo_t (extension usage) within
* the include, import, and feature clauses for this module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_mod_appinfo (yang_pcb_t *pcb,
                         tk_chain_t  *tkc,
                         ncx_module_t *mod)
{
    ncx_import_t    *imp;
    ncx_include_t   *inc;
    ncx_feature_t   *feature;
    status_t         res, retres;

    retres = NO_ERR;

    for (imp = (ncx_import_t *)dlq_firstEntry(&mod->importQ);
         imp != NULL;
         imp = (ncx_import_t *)dlq_nextEntry(imp)) {

        res = ncx_resolve_appinfoQ(pcb, tkc, mod, &imp->appinfoQ);
        CHK_EXIT(res, retres);
    }

    for (inc = (ncx_include_t *)dlq_firstEntry(&mod->includeQ);
         inc != NULL;
         inc = (ncx_include_t *)dlq_nextEntry(inc)) {

        res = ncx_resolve_appinfoQ(pcb, tkc, mod, &inc->appinfoQ);
        CHK_EXIT(res, retres);
    }

    for (feature = (ncx_feature_t *)dlq_firstEntry(&mod->featureQ);
         feature != NULL;
         feature = (ncx_feature_t *)dlq_nextEntry(feature)) {

        res = ncx_resolve_appinfoQ(pcb, tkc, mod, &feature->appinfoQ);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* resolve_mod_appinfo */


/********************************************************************
* FUNCTION consume_mod_hdr
* 
* Parse the module header statements
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_mod_hdr (tk_chain_t  *tkc,
                     ncx_module_t *mod)
{
    const xmlChar *val;
    xmlChar       *str;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, ver, ns, pfix;


    expstr = "module header statement";
    ver = FALSE;
    ns = FALSE;
    pfix = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    while (!done) {

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                res = NO_ERR;
            }
            continue;
        case TK_TT_RBRACE:
            TK_BKUP(tkc);
            done = TRUE;
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_YANG_VERSION)) {
            /* Optional 'yang-version' field is present */
            if (ver) {
                retres = ERR_NCX_ENTRY_EXISTS;
                ncx_print_errormsg(tkc, mod, retres);
            }
            ver = TRUE;

            /* get the version number */
            res = yang_consume_string(tkc, mod, &str);
            if (res != NO_ERR) {
                retres = res;
            } else {
                if (xml_strcmp(TK_CUR_VAL(tkc), YANG_VERSION_STR)) {
                    retres = ERR_NCX_WRONG_VERSION;
                    ncx_print_errormsg(tkc, mod, retres);
                } else if (!mod->langver) {
                    mod->langver = YANG_VERSION_NUM;
                }
            }
            if (str) {
                m__free(str);
            }

            res = yang_consume_semiapp(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
            }
        } else if (!xml_strcmp(val, YANG_K_NAMESPACE)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &mod->ns,
                                         &ns, 
                                         &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
            } else {
                /*** TBD: check valid URI ***/
            }
        } else if (!xml_strcmp(val, YANG_K_PREFIX)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &mod->prefix,
                                         &pfix, 
                                         &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
            } else if (!ncx_valid_name2(mod->prefix)) {
                retres = ERR_NCX_INVALID_NAME;
                log_error("\nError: invalid prefix value '%s'",
                          mod->prefix);
                ncx_print_errormsg(tkc, mod, retres);
            } else {
                ncx_check_warn_idlen(tkc, mod, mod->prefix);
            }
        } else if (!yang_top_keyword(val)) {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        } else {
            /* assume we reached the end of this sub-section
             * if there are simply clauses out of order, then
             * the compiler will not detect that.
             */
            TK_BKUP(tkc);
            done = TRUE;
        }
    }

    /* check missing mandatory sub-clauses */
    if (!mod->ns) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "mod-hdr", "namespace");
    }
    if (!mod->prefix) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "mod-hdr", "prefix");
    }

    return retres;

}  /* consume_mod_hdr */


/********************************************************************
* FUNCTION consume_belongs_to
* 
* Parse the next N tokens as a belongs-to clause
* Set the submodule prefix and belongs string
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'belongs-to' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get updated
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_belongs_to (tk_chain_t *tkc,
                        ncx_module_t  *mod)
{
    const xmlChar      *val;
    const char         *expstr;
    tk_type_t           tktyp;
    boolean             done, pfixdone;
    status_t            res, retres;

    val = NULL;
    expstr = "module name";
    done = FALSE;
    pfixdone = FALSE;
    retres = NO_ERR;

    /* Get the mandatory module name */
    res = yang_consume_id_string(tkc, mod, &mod->belongs);
    if (res != NO_ERR) {
        return res;
    } 

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the belongs-to statement
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        break;
    default:
        retres = ERR_NCX_WRONG_TKTYPE;
        expstr = "semi-colon or left brace";
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        done = TRUE;
    }

    /* get the prefix clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value, should be 'prefix' */
        if (!xml_strcmp(val, YANG_K_PREFIX)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &mod->prefix,
                                         &pfixdone, 
                                         &mod->appinfoQ);
            if (res == NO_ERR &&
                !ncx_valid_name2(mod->prefix)) {
                res = ERR_NCX_INVALID_NAME;
                log_error("\nError: invalid prefix value '%s'",
                          mod->prefix);
                ncx_print_errormsg(tkc, mod, res);
            } else {
                ncx_check_warn_idlen(tkc, mod, mod->prefix);
            }
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        }
    }

    if (!mod->prefix) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "belongs-to", "prefix");
    }

    return retres;

}  /* consume_belongs_to */


/********************************************************************
* FUNCTION consume_feature
* 
* Parse the next N tokens as a feature statement
* Create an ncx_feature_t struct and add it to the
* module or submodule featureQ
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'feature' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get updated
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_feature (tk_chain_t *tkc,
                     ncx_module_t  *mod)
{
    const xmlChar      *val;
    const char         *expstr;
    ncx_feature_t      *feature, *testfeature;
    yang_stmt_t        *stmt;
    tk_type_t           tktyp;
    boolean             done, stat, desc, ref, keep;
    status_t            res, retres;

    val = NULL;
    expstr = "feature name";
    done = FALSE;
    stat = FALSE;
    desc = FALSE;
    ref = FALSE;
    keep = TRUE;
    retres = NO_ERR;

    feature = ncx_new_feature();
    if (!feature) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }
    ncx_set_error(&feature->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    /* Get the mandatory feature name */
    res = yang_consume_id_string(tkc, mod, &feature->name);
    if (res != NO_ERR) {
        /* do not keep -- must have a name field */
        retres = res;
        keep = FALSE;
    }
        
    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the feature statement
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        ncx_free_feature(feature);
        return res;
    }
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        break;
    default:
        retres = ERR_NCX_WRONG_TKTYPE;
        expstr = "semi-colon or left brace";
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        done = TRUE;
    }

    expstr = "feature sub-statement";

    /* get the prefix clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            retres = res;
            done = TRUE;
            continue;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            retres = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            done = TRUE;
            continue;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    done = TRUE;
                }
            }
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value, should be 'prefix' */
        if (!xml_strcmp(val, YANG_K_IF_FEATURE)) {
            res = yang_consume_iffeature(tkc, 
                                         mod, 
                                         &feature->iffeatureQ,
                                         &feature->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &feature->status,
                                      &stat, 
                                      &feature->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &feature->descr,
                                     &desc, 
                                     &feature->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &feature->ref,
                                     &ref, 
                                     &feature->appinfoQ);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                done = TRUE;
            }
        }
    }

    /* check if feature already exists in this module */
    if (keep) {
        testfeature = ncx_find_feature_all(mod, feature->name);
        if (testfeature) {
            retres = ERR_NCX_DUP_ENTRY;
            log_error("\nError: feature '%s' already defined "
                      "in '%s' at line %u", 
                      feature->name,
                      testfeature->tkerr.mod->name,
                      testfeature->tkerr.linenum);
            ncx_print_errormsg(tkc, mod, retres);
        }

        feature->res = retres;
        dlq_enque(feature, &mod->featureQ);

        if (mod->stmtmode) {
            stmt = yang_new_feature_stmt(feature);
            if (stmt) {
                dlq_enque(stmt, &mod->stmtQ);
            } else {
                log_error("\nError: malloc failure for feature_stmt");
                retres = ERR_INTERNAL_MEM;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }

    } else {
        ncx_free_feature(feature);
    }

    return retres;

}  /* consume_feature */


/********************************************************************
* FUNCTION resolve_feature
* 
* Validate all the if-feature clauses present in 
* the specified feature
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == ncx_module_t in progress
*   feature == ncx_feature_t to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_feature (yang_pcb_t *pcb,
                     tk_chain_t *tkc,
                     ncx_module_t  *mod,
                     ncx_feature_t *feature)
{
    ncx_feature_t    *testfeature;
    ncx_iffeature_t  *iff;
    status_t          res, retres;
    boolean           errdone;

    retres = NO_ERR;

    /* check if there are any feature parameters set */
    ncx_set_feature_parms(feature);
     
    /* check if there are any if-feature statements inside
     * this feature that need to be resolved
     */
    for (iff = (ncx_iffeature_t *)
             dlq_firstEntry(&feature->iffeatureQ);
         iff != NULL;
         iff = (ncx_iffeature_t *)dlq_nextEntry(iff)) {

        testfeature = NULL;
        errdone = FALSE;
        res = NO_ERR;

        if (iff->prefix &&
            xml_strcmp(iff->prefix, mod->prefix)) {
            /* find the feature in another module */
            res = yang_find_imp_feature(pcb,
                                        tkc, 
                                        mod, 
                                        iff->prefix,
                                        iff->name, 
                                        &iff->tkerr,
                                        &testfeature);
            if (res != NO_ERR) {
                retres = res;
                errdone = TRUE;
            }
        } else if (!xml_strcmp(iff->name, feature->name)) {
            /* error: if-feature foo inside feature foo */
            res = retres = ERR_NCX_DEF_LOOP;
            log_error("\nError: 'if-feature %s' inside feature '%s'",
                      iff->name, iff->name);
            tkc->curerr = &iff->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
            errdone = TRUE;
        } else {
            testfeature = ncx_find_feature(mod, iff->name);
        }

        if (!testfeature && !errdone) {
            log_error("\nError: Feature '%s' not found "
                      "for if-feature statement",
                      iff->name);
            res = retres = ERR_NCX_DEF_NOT_FOUND;
            tkc->curerr = &iff->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }

        if (testfeature) {
            iff->feature = testfeature;
        }
    }

    return retres;

}  /* resolve_feature */


/********************************************************************
* FUNCTION check_feature_loop
* 
* Validate all the if-feature clauses present in 
* the specified feature, after all if-features have
* been resolved (or at least attempted)
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == ncx_module_t in progress
*   feature == ncx_feature_t to check now
*   startfeature == feature that started this off, so if this
*                   is reached again, it will trigger an error
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    check_feature_loop (tk_chain_t *tkc,
                        ncx_module_t  *mod,
                        ncx_feature_t *feature,
                        ncx_feature_t *startfeature)
{
    ncx_iffeature_t  *iff;
    status_t          res, retres;

    retres = NO_ERR;

    /* check if there are any if-feature statements inside
     * this feature that need to be resolved
     */
    for (iff = (ncx_iffeature_t *)
             dlq_firstEntry(&feature->iffeatureQ);
         iff != NULL;
         iff = (ncx_iffeature_t *)dlq_nextEntry(iff)) {

        if (!iff->feature) {
            continue;
        }

        if (iff->feature == startfeature) {
            retres = res = ERR_NCX_DEF_LOOP;
            startfeature->res = res;
            log_error("\nError: if-feature loop detected for '%s' "
                      "in feature '%s'",
                      startfeature->name,
                      feature->name);
            tkc->curerr = &startfeature->tkerr;
            ncx_print_errormsg(tkc, mod, res);
        } else if (iff->feature->res != ERR_NCX_DEF_LOOP) {
            res = check_feature_loop(tkc, mod, iff->feature,
                                     startfeature);
            if (res != NO_ERR) {
                retres = res;
            }
        }
    }

    return retres;

}  /* check_feature_loop */


/********************************************************************
* FUNCTION consume_identity
* 
* Parse the next N tokens as an identity statement
* Create an ncx_identity_t struct and add it to the
* module or submodule identityQ
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'identity' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get updated
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_identity (tk_chain_t *tkc,
                      ncx_module_t  *mod)
{
    const xmlChar      *val;
    const char         *expstr;
    ncx_identity_t     *identity, *testidentity;
    yang_stmt_t        *stmt;
    tk_type_t           tktyp;
    boolean             done, base, stat, desc, ref, keep;
    status_t            res, retres;

    val = NULL;
    expstr = "identity name";
    done = FALSE;
    base = FALSE;
    stat = FALSE;
    desc = FALSE;
    ref = FALSE;
    keep = TRUE;
    retres = NO_ERR;

    identity = ncx_new_identity();
    if (!identity) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }
    ncx_set_error(&identity->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));
    identity->isroot = TRUE;

    /* Get the mandatory identity name */
    res = yang_consume_id_string(tkc, mod, &identity->name);
    if (res != NO_ERR) {
        /* do not keep -- must have a name field */
        retres = res;
        keep = FALSE;
    }
        
    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the identity statement
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        ncx_free_identity(identity);
        return res;
    }
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        break;
    default:
        retres = ERR_NCX_WRONG_TKTYPE;
        expstr = "semi-colon or left brace";
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        done = TRUE;
    }

    expstr = "identity sub-statement";

    /* get the prefix clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            retres = res;
            done = TRUE;
            continue;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            retres = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            done = TRUE;
            continue;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    done = TRUE;
                }
            }
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value, should be 'prefix' */
        if (!xml_strcmp(val, YANG_K_BASE)) {
            identity->isroot = FALSE;
            res = yang_consume_pid(tkc, 
                                   mod, 
                                   &identity->baseprefix,
                                   &identity->basename,
                                   &base, 
                                   &identity->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &identity->status,
                                      &stat, 
                                      &identity->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &identity->descr,
                                     &desc, 
                                     &identity->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &identity->ref,
                                     &ref, 
                                     &identity->appinfoQ);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                done = TRUE;
            }
        }
    }

    /* check if feature already exists in this module */
    if (keep) {
        testidentity = ncx_find_identity(mod, identity->name, TRUE);
        if (testidentity) {
            retres = ERR_NCX_DUP_ENTRY;
            log_error("\nError: identity '%s' already defined "
                      "in %s at line %u", 
                      identity->name,
                      testidentity->tkerr.mod->name,
                      testidentity->tkerr.linenum);
            ncx_print_errormsg(tkc, mod, retres);
        }
        identity->res = retres;
        dlq_enque(identity, &mod->identityQ);

        if (mod->stmtmode) {
            stmt = yang_new_id_stmt(identity);
            if (stmt) {
                dlq_enque(stmt, &mod->stmtQ);
            } else {
                log_error("\nError: malloc failure for id_stmt");
                retres = ERR_INTERNAL_MEM;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }
    } else {
        ncx_free_identity(identity);
    }

    return retres;

}  /* consume_identity */


/********************************************************************
* FUNCTION resolve_identity
* 
* Validate the identity statement base clause, if any
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == ncx_module_t in progress
*   identity == ncx_identity_t to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_identity (yang_pcb_t *pcb,
                      tk_chain_t *tkc,
                      ncx_module_t  *mod,
                      ncx_identity_t *identity)
{
    if (identity->isroot) {
        return NO_ERR;
    }

    ncx_identity_t *testidentity = NULL;
    status_t res = NO_ERR;
    boolean errdone = FALSE;

    if (identity->baseprefix && mod->prefix &&
        xml_strcmp(identity->baseprefix, mod->prefix)) {

        /* find the identity in another module */
        res = yang_find_imp_identity(pcb,
                                     tkc, 
                                     mod, 
                                     identity->baseprefix,
                                     identity->basename, 
                                     &identity->tkerr,
                                     &testidentity);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    } else if (identity->name && identity->basename && 
               !xml_strcmp(identity->name, identity->basename)) {
        /* error: 'base foo' inside 'identity foo' */
        res = ERR_NCX_DEF_LOOP;
        log_error("\nError: 'base %s' inside identity '%s'",
                  identity->basename, identity->name);
        tkc->curerr = &identity->tkerr;
        ncx_print_errormsg(tkc, mod, res);
        errdone = TRUE;
    } else if (identity->basename) {
        testidentity = ncx_find_identity(mod, identity->basename, FALSE);
    }
    if (!testidentity && !errdone) {
        if (identity->baseprefix || identity->basename) {
            log_error("\nError: Base '%s%s%s' not found "
                      "for identity statement '%s'",
                      (identity->baseprefix) ? 
                      identity->baseprefix : EMPTY_STRING,
                      (identity->baseprefix) ? ":" : "",
                      (identity->basename) ? identity->basename : EMPTY_STRING,
                      (identity->name) ? identity->name : NCX_EL_NONE);
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            log_error("\nError: Invalid base name for identity statement '%s'",
                      (identity->name) ? identity->name : NCX_EL_NONE);
            res = ERR_NCX_INVALID_NAME;
        }

        tkc->curerr = &identity->tkerr;
        ncx_print_errormsg(tkc, mod, res);
    }

    if (testidentity) {
        identity->base = testidentity;
    }

    return res;

}  /* resolve_identity */


/********************************************************************
* FUNCTION check_identity_loop
* 
* Validate the base identity chain for loops
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == ncx_module_t in progress
*   identity == ncx_identity_t to check now
*   startidentity == identity that started this off, so if this
*                    is reached again, it will trigger an error
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    check_identity_loop (tk_chain_t *tkc,
                         ncx_module_t  *mod,
                         ncx_identity_t *identity,
                         ncx_identity_t *startidentity)
{
    status_t          res;

    res = NO_ERR;

    /* check if there is a base statement, and if it leads
     * back to startidentity or not
     */
    if (!identity->base) {
        res = NO_ERR;
    } else if (identity->base == startidentity) {
        res = ERR_NCX_DEF_LOOP;
        startidentity->res = res;
        log_error("\nError: identity base loop detected for '%s' "
                  "in identity '%s'",
                  startidentity->name,
                  identity->name);
        tkc->curerr = &startidentity->tkerr;
        ncx_print_errormsg(tkc, mod, res);
    } else if (identity->base->res != ERR_NCX_DEF_LOOP) {
        res = check_identity_loop(tkc, mod, 
                                  identity->base,
                                  startidentity);
        if (res == NO_ERR) {
            /* thread the idlink into the base identifier */
            dlq_enque(&identity->idlink, &identity->base->childQ);
            identity->idlink.inq = TRUE;
        }
    }

    return res;

}  /* check_identity_loop */


/********************************************************************
* FUNCTION consume_submod_hdr
* 
* Parse the sub-module header statements
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_submod_hdr (tk_chain_t  *tkc,
                        ncx_module_t *mod)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, ver;

    expstr = "submodule header statement";
    ver = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    while (!done) {

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            TK_BKUP(tkc);
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_YANG_VERSION)) {
            /* Optional 'yang-version' field is present */
            if (ver) {
                retres = ERR_NCX_ENTRY_EXISTS;
                ncx_print_errormsg(tkc, mod, retres);
            }
            ver = TRUE;

            /* get the version number */
            res = ncx_consume_token(tkc, mod, TK_TT_STRING);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    return res;
                }
            } else {
                if (xml_strcmp(TK_CUR_VAL(tkc), YANG_VERSION_STR)) {
                    retres = ERR_NCX_WRONG_VERSION;
                    ncx_print_errormsg(tkc, mod, retres);
                } else {
                    mod->langver = YANG_VERSION_NUM;
                }
            }

            res = yang_consume_semiapp(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_BELONGS_TO)) {
            res = consume_belongs_to(tkc, mod);
            CHK_EXIT(res, retres);
        } else if (!yang_top_keyword(val)) {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        } else {
            /* assume we reached the end of this sub-section
             * if there are simply clauses out of order, then
             * the compiler will not detect that.
             */
            TK_BKUP(tkc);
            done = TRUE;
        }
    }

    /* check missing mandatory sub-clause */
    if (!mod->belongs) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "submod-hdr", "belongs-to");
    }

    return retres;

}  /* consume_submod_hdr */


/********************************************************************
* FUNCTION consume_import
* 
* Parse the next N tokens as an import clause
* Create a ncx_import struct and add it to the specified module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'import' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get the ncx_import_t 
*   pcb == parser control block
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_import (tk_chain_t *tkc,
                    ncx_module_t  *mod,
                    yang_pcb_t *pcb)
{
    ncx_import_t       *imp, *testimp;
    const xmlChar      *val;
    const char         *expstr;
    yang_node_t        *node;
    yang_import_ptr_t  *impptr;
    ncx_module_t       *testmod;
    tk_type_t           tktyp;
    boolean             done, pfixdone, revdone;
    status_t            res, retres;
    ncx_error_t         tkerr;

    val = NULL;
    expstr = "module name";
    done = FALSE;
    pfixdone = FALSE;
    revdone = FALSE;
    retres = NO_ERR;
    memset(&tkerr, 0x0, sizeof(tkerr));

    /* Get a new ncx_import_t to fill in */
    imp = ncx_new_import();
    if (!imp) {
        retres = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, retres);
        return retres;
    } else {
        ncx_set_error(&imp->tkerr,
                      mod,
                      TK_CUR_LNUM(tkc),
                      TK_CUR_LPOS(tkc));
        imp->usexsd = TRUE;
    }

    /* Get the mandatory module name */
    res = yang_consume_id_string(tkc, mod, &imp->module);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_import(imp);
            return res;
        }
    }

    /* HACK: need to replace when ietf-netconf.yang is supported */
    if (res == NO_ERR &&
        !xml_strcmp(imp->module, NCXMOD_IETF_NETCONF)) {

        /* force the parser to skip loading this import and just
         * use the yuma-netconf module instead
         */
        testmod = ncx_find_module(NCXMOD_YUMA_NETCONF, NULL);
        if (testmod != NULL) {
            imp->force_yuma_nc = TRUE;
            imp->mod = testmod;
        }
    }
        
    /* Get the starting left brace for the sub-clauses */
    res = ncx_consume_token(tkc, mod, TK_TT_LBRACE);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_import(imp);
            return res;
        }
    }

    /* get the prefix clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            ncx_free_import(imp);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            ncx_free_import(imp);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &imp->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_import(imp);
                    return res;
                }
            }
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value, should be 'prefix' */
        if (!xml_strcmp(val, YANG_K_PREFIX)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &imp->prefix,
                                         &pfixdone, 
                                         &imp->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_import(imp);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_REVISION_DATE)) {
            ncx_set_error(&tkerr,
                          mod,
                          TK_CUR_LNUM(tkc),
                          TK_CUR_LPOS(tkc));
            if (revdone) {
                res = retres = ERR_NCX_ENTRY_EXISTS;
                ncx_print_errormsg(tkc, mod, retres);
                if (imp->revision) {
                    m__free(imp->revision);
                    imp->revision = NULL;
                }
            } else {
                res = NO_ERR;
                revdone = TRUE;
            }

            res = consume_revision_date(tkc, mod, &imp->revision);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_import(imp);
                    return res;
                }
            }

            res = yang_consume_semiapp(tkc, mod, &imp->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_import(imp);
                    return res;
                }
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        }
    }

    if (imp->revision) {
        /* validate the revision date */
        res = yang_validate_date_string(tkc, 
                                        mod, 
                                        &tkerr, 
                                        imp->revision);
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                ncx_free_import(imp);
                return res;
            }
        }
    }

    /* check special deviation or search processing mode */
    if (pcb->deviationmode || pcb->searchmode) {
        /* save the import for prefix translation later
         * do not need to actually import any of the symbols
         * now; may be a waste of time unless there are
         * any deviation statements found
         * don't really care how valid it is right now
         */
        dlq_enque(imp, &mod->importQ);
        return NO_ERR;
    }

    /* check all the mandatory clauses are present */
    if (!imp->prefix) {
        retres = ERR_NCX_DATA_MISSING;
        tkc->curerr = &imp->tkerr;
        ncx_mod_missing_err(tkc, 
                            mod, 
                            "import", 
                            "prefix");
    }

    /* check if the import is already present */
    if (imp->module && imp->prefix) {

        /* check if module already present */
        testimp = ncx_find_import_test(mod, imp->module);
        if (testimp) {
            if (((testimp->revision && imp->revision) ||
                 (!testimp->revision && !imp->revision)) &&
                yang_compare_revision_dates(testimp->revision, 
                                            imp->revision)) {
                
                log_error("\nError: invalid duplicate import found on line %u",
                          testimp->tkerr.linenum);
                retres = res = ERR_NCX_INVALID_DUP_IMPORT;
                tkc->curerr = &imp->tkerr;
                ncx_print_errormsg(tkc, mod, res);
            } else {
                imp->usexsd = FALSE;
                if (!xml_strcmp(testimp->prefix, imp->prefix)) {
                    /* warning for exact duplicate import */
                    if (ncx_warning_enabled(ERR_NCX_DUP_IMPORT)) {
                        log_warn("\nWarning: duplicate import "
                                 "found on line %u",
                                 testimp->tkerr.linenum);
                        res = ERR_NCX_DUP_IMPORT;
                        tkc->curerr = &imp->tkerr;
                        ncx_print_errormsg(tkc, mod, res);
                    } else {
                        ncx_inc_warnings(mod);
                    }
                } else if (ncx_warning_enabled(ERR_NCX_PREFIX_DUP_IMPORT)) {
                    /* warning for dup. import w/ different prefix */
                    log_warn("\nWarning: same import with different prefix"
                             " found on line %u", testimp->tkerr.linenum);
                    res = ERR_NCX_PREFIX_DUP_IMPORT;
                    tkc->curerr = &imp->tkerr;
                    ncx_print_errormsg(tkc, mod, res);
                } else {
                    ncx_inc_warnings(mod);
                }
            }
        }

        /* check simple module loop with itself */
        if (imp->usexsd && !xml_strcmp(imp->module, mod->name)) {
            log_error("\nError: import '%s' for current module",
                      mod->name);
            retres = ERR_NCX_IMPORT_LOOP;
            tkc->curerr = &imp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }

        /* check simple module loop with the top-level file */
        if (imp->usexsd && xml_strcmp(mod->name, pcb->top->name) &&
            !xml_strcmp(imp->module, pcb->top->name)) {
            log_error("\nError: import loop for top-level %smodule '%s'",
                      (pcb->top->ismod) ? "" : "sub", 
                      imp->module);
            retres = ERR_NCX_IMPORT_LOOP;
            tkc->curerr = &imp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }

        /* check simple submodule importing its parent module */
        if (imp->usexsd &&
            mod->belongs && !xml_strcmp(mod->belongs, imp->module)) {
            log_error("\nError: submodule '%s' cannot import its"
                      " parent module '%s'", 
                      mod->name, 
                      imp->module);
            retres = ERR_NCX_IMPORT_LOOP;
            tkc->curerr = &imp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }

        /* check prefix for this module corner-case */
        if (mod->ismod && !xml_strcmp(imp->prefix, mod->prefix)) {
            log_error("\nError: import '%s' using "
                      "prefix for current module (%s)",
                      imp->module, 
                      imp->prefix);
            retres = ERR_NCX_IN_USE;
            tkc->curerr = &imp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }
            
        /* check if prefix already used in other imports */
        testimp = ncx_find_pre_import_test(mod, imp->prefix);
        if (testimp) {
            if (xml_strcmp(testimp->module, imp->module)) {
                retres = ERR_NCX_IN_USE;
                log_error("\nImport %s on line %u already using prefix %s",
                          testimp->module, 
                          testimp->tkerr.linenum,
                          testimp->prefix);
                tkc->curerr = &imp->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }
        
        /* check for import loop */
        node = yang_find_node(&pcb->impchainQ, 
                              imp->module,
                              imp->revision);
        if (node) {
            log_error("\nError: loop created by import '%s'"
                      " from module '%s', line %u",
                      imp->module, 
                      node->mod->name,
                      node->tkerr.linenum);
            retres = ERR_NCX_IMPORT_LOOP;
            tkc->curerr = &imp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }
    }

    /* save or delete the import struct, except in search mode */
    if (retres == NO_ERR && imp->usexsd && !pcb->searchmode) {
        node = yang_new_node();
        if (!node) {
            retres = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, retres);
            ncx_free_import(imp);
        } else {
            impptr = yang_new_import_ptr(imp->module, 
                                         imp->prefix,
                                         imp->revision);
            if (!impptr) {
                retres = ERR_INTERNAL_MEM;
                tkc->curerr = &imp->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
                ncx_free_import(imp);
                yang_free_node(node);
            } else {
                /* save the import used record and the import */
                node->name = imp->module;
                node->revision = imp->revision;
                ncx_set_error(&node->tkerr,
                              imp->tkerr.mod,
                              imp->tkerr.linenum,
                              imp->tkerr.linepos);

                /* save the import on the impchain stack */
                dlq_enque(node, &pcb->impchainQ);

                /* save the import for prefix translation */
                dlq_enque(imp, &mod->importQ);

                /* save the import marker to keep a list
                 * of all the imports with no duplicates
                 * regardless of recursion or submodules
                 */
                dlq_enque(impptr, &pcb->allimpQ);

                /* load the module now instead of later for validation
                 * it may not get used, but assume it will 
                 * skip if forcing yuma-netconf override of ietf-netconf
                 * or if searchmode and just finding modname, revision
                 */
                if (!(imp->force_yuma_nc || pcb->searchmode)) {
                    ncx_module_t *impmod = NULL;

                    /* in all cases, even diff-mode, it should be OK
                     * to reuse the imported module if it has already
                     * been parsed and loaded into the registry.
                     * All modes that need the token chain are for the
                     * top module, never an imported module
                     */
                    impmod = ncx_find_module(imp->module, imp->revision);
                    if (impmod != NULL) {
                        res = NO_ERR;
                    } else {
                        res = ncxmod_load_imodule(imp->module, 
                                                  imp->revision,
                                                  pcb, 
                                                  YANG_PT_IMPORT,
                                                  NULL,
                                                  &impmod);
                    }

                    /* save the status to prevent retrying this module */
                    imp->res = res;
                    imp->mod = impmod;
                } else if (LOGDEBUG) {
                    log_debug("\nSkipping import of ietf-netconf, "
                              "using yuma-netconf instead");
                }

                if (res != NO_ERR) {
                    /* skip error if module has just warnings */
                    if (get_errtyp(res) < ERR_TYP_WARN) {
                        retres = ERR_NCX_IMPORT_ERRORS;
                        if (imp->revision) {
                            log_error("\nError: '%s' import of "
                                      "module '%s' revision '%s' failed",
                                      mod->sourcefn, 
                                      imp->module,
                                      imp->revision);
                        } else {
                            log_error("\nError: '%s' import of "
                                      "module '%s' failed",
                                      mod->sourcefn, 
                                      imp->module);
                        }
                        tkc->curerr = &imp->tkerr;
                        ncx_print_errormsg(tkc, mod, res);
                    }
                } /* else ignore the warnings */

                /* remove the node in the import chain that 
                 * was added before the module was loaded
                 */
                node = (yang_node_t *)dlq_lastEntry(&pcb->impchainQ);
                if (node) {
                    dlq_remove(node);
                    yang_free_node(node);
                } else {
                    retres = SET_ERROR(ERR_INTERNAL_VAL);
                }

            }
        }
    } else {
        ncx_free_import(imp);
    }

    return retres;

}  /* consume_import */


/********************************************************************
* FUNCTION consume_include
* 
* Parse the next N tokens as an include clause
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'include' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get the ncx_import_t 
*   pcb == parser control block
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_include (tk_chain_t *tkc,
                     ncx_module_t  *mod,
                     yang_pcb_t *pcb)
{

    ncx_include_t  *inc, *testinc;
    const char     *expstr;
    const xmlChar  *val;
    yang_node_t    *node, *testnode;
    dlq_hdr_t      *allQ, *chainQ;
    ncx_module_t   *foundmod, *realmod;
    tk_type_t       tktyp;
    status_t        res, retres;
    boolean         done, revdone;
    ncx_error_t     tkerr;

    done = FALSE;
    expstr = "submodule name";
    retres = NO_ERR;
    revdone = FALSE;
    memset(&tkerr, 0x0, sizeof(tkerr));

    /* Get a new ncx_include_t to fill in */
    inc = ncx_new_include();
    if (!inc) {
        retres = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, retres);
        return retres;
    } else {
        ncx_set_error(&inc->tkerr,
                      mod,
                      TK_CUR_LNUM(tkc),
                      TK_CUR_LPOS(tkc));
        inc->usexsd = TRUE;
    }

    /* Get the mandatory submodule name */
    res = yang_consume_id_string(tkc, mod, &inc->submodule);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_include(inc);
            return res;
        }
    }

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the include statement
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_free_include(inc);
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        break;
    default:
        retres = ERR_NCX_WRONG_TKTYPE;
        expstr = "semi-colon or left brace";
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        done = TRUE;
    }

    /* get the revision clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            ncx_free_include(inc);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            ncx_free_include(inc);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_include(inc);
                    return res;
                }
            }
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value, should be 'prefix' */
        if (!xml_strcmp(val, YANG_K_REVISION_DATE)) {
            ncx_set_error(&tkerr,
                          mod,
                          TK_CUR_LNUM(tkc),
                          TK_CUR_LPOS(tkc));

            if (revdone) {
                res = retres = ERR_NCX_ENTRY_EXISTS;
                ncx_print_errormsg(tkc, mod, retres);
                if (inc->revision) {
                    m__free(inc->revision);
                    inc->revision = NULL;
                }
            } else {
                res = NO_ERR;
                revdone = TRUE;
            }

            res = consume_revision_date(tkc, mod, &inc->revision);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_include(inc);
                    return res;
                }
            }

            res = yang_consume_semiapp(tkc, mod, &inc->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_include(inc);
                    return res;
                }
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        }
    }

    if (inc->revision) {
        /* validate the revision date */
        res = yang_validate_date_string(tkc, 
                                        mod, 
                                        &tkerr,
                                        inc->revision);
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                ncx_free_include(inc);
                return res;
            }
        }
    }

    /* get the include Qs to use
     * keep checking parents until the real module is reached
     */
    realmod = mod;
    while (realmod->parent != NULL) {
        realmod = realmod->parent;
    }

    allQ = &realmod->allincQ;
    chainQ = &realmod->incchainQ;

    /* check special scan mode looking for mod info */
    if (pcb->searchmode) {
        /* hand off malloced 'inc' struct here
         * do not load the sub-module; not being used
         */
        dlq_enque(inc, &mod->includeQ);
        return NO_ERR;
    }

    /* check if the mandatory submodule name is valid
     * and if the include is already present 
     */
    if (!inc->submodule) {
        retres = ERR_NCX_DATA_MISSING;
        tkc->curerr = &inc->tkerr;
        ncx_mod_exp_err(tkc, mod, retres, expstr);
    } else if (!ncx_valid_name2(inc->submodule)) {
        retres = ERR_NCX_INVALID_NAME;
        tkc->curerr = &inc->tkerr;
        ncx_mod_exp_err(tkc, mod, retres, expstr);
    } else {
        /* check if submodule already present */
        testinc = ncx_find_include(mod, inc->submodule);
        if (testinc) {
            /* check if there is a revision conflict */
            if (((testinc->revision && inc->revision) ||
                 (!testinc->revision && !inc->revision)) &&
                yang_compare_revision_dates(testinc->revision, 
                                            inc->revision)) {
                log_error("\nError: invalid duplicate "
                          "include found on line %u",
                          testinc->tkerr.linenum);
                retres = res = ERR_NCX_INVALID_DUP_INCLUDE;
                tkc->curerr = &inc->tkerr;
                ncx_print_errormsg(tkc, mod, res);
            } else {
                /* warning for same duplicate already in the
                 * include statements
                 */
                inc->usexsd = FALSE;
                if (ncx_warning_enabled(ERR_NCX_DUP_INCLUDE)) {
                    log_warn("\nWarning: duplicate include found "
                             "on line %u",
                             testinc->tkerr.linenum);
                    res = ERR_NCX_DUP_INCLUDE;
                    tkc->curerr = &inc->tkerr;
                    ncx_print_errormsg(tkc, mod, res);
                }  else {
                    ncx_inc_warnings(mod);
                }
            }
        } else {
            /* check simple submodule loop with itself */
            if (!xml_strcmp(inc->submodule, mod->name)) {
                log_error("\nError: include '%s' for current submodule",
                          mod->name);
                retres = ERR_NCX_INCLUDE_LOOP;
                tkc->curerr = &inc->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            }

            /* check simple submodule loop with the top-level file */
            if (retres == NO_ERR &&
                xml_strcmp(mod->name, pcb->top->name) &&
                !xml_strcmp(inc->submodule, pcb->top->name)) {
                log_error("\nError: include loop for top-level %smodule '%s'",
                          (pcb->top->ismod) ? "" : "sub", inc->submodule);
                retres = ERR_NCX_INCLUDE_LOOP;
                tkc->curerr = &inc->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            }

            /* check simple submodule including its parent module */
            if (retres == NO_ERR &&
                mod->belongs && !xml_strcmp(mod->belongs, inc->submodule)) {
                log_error("\nError: submodule '%s' cannot include its"
                          " parent module '%s'", mod->name, inc->submodule);
                retres = ERR_NCX_INCLUDE_LOOP;
                tkc->curerr = &inc->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            }

            /* check for include loop */
            if (retres == NO_ERR) {
                node = yang_find_node(chainQ, 
                                      inc->submodule,
                                      inc->revision);
                if (node) {
                    log_error("\nError: loop created by include '%s'"
                              " from %smodule '%s', line %u",
                              inc->submodule, (node->mod->ismod) ? "" : "sub",
                              node->mod->name, 
                              node->tkerr.linenum);
                    retres = ERR_NCX_INCLUDE_LOOP;
                    tkc->curerr = &inc->tkerr;
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        }
    }

    /* save or delete the include struct */
    if (retres == NO_ERR && inc->usexsd) {
        node = yang_new_node();
        if (!node) {
            retres = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, retres);
            ncx_free_include(inc);
        } else {
            /* save the include */
            node->name = inc->submodule;
            node->revision = inc->revision;
            node->mod = mod;
            ncx_set_error(&node->tkerr,
                          inc->tkerr.mod,
                          inc->tkerr.linenum,
                          inc->tkerr.linepos);

            /* hand off malloced 'inc' struct here */
            dlq_enque(inc, &mod->includeQ);

            /* check if already parsed, and in the allincQ */
            testnode = yang_find_node(allQ,
                                      inc->submodule,
                                      inc->revision);
            if (testnode == NULL) {
                dlq_enque(node, chainQ);

                foundmod = NULL;

                /* load the module now instead of later for validation */
                retres = 
                    ncxmod_load_imodule(inc->submodule, 
                                        inc->revision,
                                        pcb, 
                                        YANG_PT_INCLUDE,
                                        realmod,
                                        &foundmod);

                if (retres != NO_ERR) {
                    if (retres == ERR_NCX_MOD_NOT_FOUND ||
                        get_errtyp(res) < ERR_TYP_WARN) {
                        if (inc->revision) {
                            log_error("\nError: include of "
                                      "submodule '%s' revision '%s' failed",
                                      inc->submodule, 
                                      inc->revision);
                        } else {
                            log_error("\nError: include of "
                                      "submodule '%s' failed",
                                      inc->submodule); 
                        }
                        tkc->curerr = &inc->tkerr;
                        ncx_print_errormsg(tkc, mod, retres);
                    } else {
                        /* ignore warnings */
                        retres = NO_ERR;
                    }
                }

                /* remove the node in the include chain that 
                 * was added before the submodule was loaded
                 */
                node = (yang_node_t *)dlq_lastEntry(chainQ);
                if (node) {
                    dlq_remove(node);

                    if (foundmod == NULL) {
                        yang_free_node(node);
                    } else {
                        /* save this node in the mod->allincQ now
                         * because it may be needed while body-stmts
                         * are being processed and nodes are searched
                         */
                        node->submod = foundmod;
                        node->res = retres;
                        dlq_enque(node, allQ);

                        /* save a back-ptr in the include directive as well */
                        inc->submod = foundmod;

                        if (LOGDEBUG3) {
                            if (node->mod && node->submod) {
                                log_debug3("\nAdd node %p with submod "
                                           "%p (%s) to mod (%s) "
                                           "in Q %p",
                                           node,
                                           node->submod,
                                           node->submod->name,
                                           node->mod->name,
                                           allQ);
                            } else if (node->submod) {
                                log_debug3("\nAdd node %p with submod %p (%s) "
                                           "in Q %p",
                                           node,
                                           node->submod,
                                           node->submod->name,
                                           allQ);
                            } else if (node->mod) {
                                log_debug3("\nAdd node %p with mod %p (%s) "
                                           "in Q %p",
                                           node,
                                           node->mod,
                                           node->mod->name,
                                           allQ);
                            }
                        }
                    }
                } else {
                    SET_ERROR(ERR_INTERNAL_VAL);
                }
            } else {
                inc->submod = testnode->submod;
                yang_free_node(node);
            }
        }
    } else {
        ncx_free_include(inc);
    }   

    return retres;
    
}  /* consume_include */


/********************************************************************
* FUNCTION consume_linkage_stmts
* 
* Parse the next N tokens as N import or include clauses
* Create ncx_import structs and add them to the specified module
*
* Recusively parse any include submodule statement
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get the ncx_import_t 
*   pcb == parser control block
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_linkage_stmts (tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           yang_pcb_t *pcb)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done;

    expstr = "import or include keyword";
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    while (!done) {

        if (!pcb->keepmode && retres != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            TK_BKUP(tkc);
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_IMPORT)) {
            res = consume_import(tkc, mod, pcb);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_INCLUDE)) {
            res = consume_include(tkc, mod, pcb);
            CHK_EXIT(res, retres);
        } else if (!yang_top_keyword(val)) {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        } else {
            /* assume we reached the end of this sub-section
             * if there are simply clauses out of order, then
             * the compiler will not detect that.
             */
            TK_BKUP(tkc);
            done = TRUE;
        }
    }

    return retres;

}  /* consume_linkage_stmts */


/********************************************************************
* FUNCTION consume_meta_stmts
* 
* Parse the meta-stmts and submodule-meta-stmts constructs
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_meta_stmts (tk_chain_t  *tkc,
                        ncx_module_t *mod)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, org, contact, descr, ref;

    expstr = "meta-statement";
    done = FALSE;
    org = FALSE;
    contact = FALSE;
    descr = FALSE;
    ref = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

    while (!done) {

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            TK_BKUP(tkc);
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_ORGANIZATION)) {
            /* 'organization' field is present */
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &mod->organization,
                                         &org, 
                                         &mod->appinfoQ);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_CONTACT)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &mod->contact_info,
                                     &contact, 
                                     &mod->appinfoQ);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &mod->descr,
                                     &descr, 
                                     &mod->appinfoQ);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &mod->ref,
                                     &ref, 
                                     &mod->appinfoQ);
            CHK_EXIT(res, retres);
        } else if (!yang_top_keyword(val)) {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        } else {
            /* assume we reached the end of this sub-section
             * if there are simply clauses out of order, then
             * the compiler will not detect that.
             */
            TK_BKUP(tkc);
            done = TRUE;
        }
    }

    return retres;

}  /* consume_meta_stmts */


/********************************************************************
* FUNCTION consume_revision_date
* 
* Parse the next N tokens as a date-arg-str clause
* Adds the string form to the specified string object
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* NEXT token is the start of the 'date-arg-str' clause
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get the ncx_import_t 
*   revstring == address of return revision date string
*
* OUTPUTS:
*   *revstring malloced and set to date-arg-str value
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_revision_date (tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           xmlChar **revstring)
{
#define REVBUFF_SIZE   128

    status_t       res = NO_ERR;

    /* get the mandatory version identifier date string */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (TK_CUR_STR(tkc)) {
        *revstring = xml_strdup(TK_CUR_VAL(tkc));
        if (!*revstring) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
        }
    } else if (TK_CUR_TYP(tkc)==TK_TT_DNUM) {
        xmlChar       *str = NULL;
        xmlChar       *p;

        /* assume this is an unquoted date string this code does not detect 
         * corner-cases like 2007 -11 -20 because an unquoted dateTime string 
         * is the same as 3 integers and if there are spaces between them
         * this will be parsed ok by tk.c */
        str = m__getMem(REVBUFF_SIZE);
        if (!str) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
            return res;
        } 
        
        p = str;
        p += xml_strcpy(p, TK_CUR_VAL(tkc));
        res = ncx_consume_token(tkc, mod, TK_TT_DNUM);
        if ( NO_ERR == res ) {
            p += xml_strcpy(p, TK_CUR_VAL(tkc));
            res = ncx_consume_token(tkc, mod, TK_TT_DNUM);
            if ( NO_ERR == res ) {
                xml_strcpy(p, TK_CUR_VAL(tkc));
                *revstring = str;
            }
        }

        if ( NO_ERR != res ) {
            m__free(str);
        }
    } else {
        const char    *expstr = "date-arg-str";
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
    }

    return res;
}  /* consume_revision_date */


/********************************************************************
* FUNCTION consume_revision
* 
* Parse the next N tokens as a revision clause
* Create a ncx_revhist entry and add it to the specified module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'revision' keyword
*
* INPUTS:
*   tkc == token chain
*   mod   == module struct that will get the ncx_import_t 
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_revision (tk_chain_t *tkc,
                      ncx_module_t  *mod)
{
    ncx_revhist_t *rev, *testrev;
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    boolean        done, descrdone, refdone;
    status_t       res, retres;

    val = NULL;
    expstr = "description or reference";
    done = FALSE;
    descrdone = FALSE;
    refdone = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

    rev = ncx_new_revhist();
    if (!rev) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    } else {
        ncx_set_error(&rev->tkerr,
                      mod,
                      TK_CUR_LNUM(tkc),
                      TK_CUR_LPOS(tkc));
    }

    /* get the mandatory version identifier date string */
    res = consume_revision_date(tkc, mod, &rev->version);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_revhist(rev);
            return res;
        }
    }

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the revision statement
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        ncx_free_revhist(rev);
        return res;
    }
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        break;
    default:
        retres = ERR_NCX_WRONG_TKTYPE;
        expstr = "semi-colon or left brace";
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        done = TRUE;
    }

    /* get the description clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            ncx_free_revhist(rev);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            ncx_free_revhist(rev);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_revhist(rev);
                    return res;
                }
            }
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token str so check the value, should be 'description' */
        if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            /* Mandatory 'description' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &rev->descr,
                                     &descrdone, 
                                     &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_revhist(rev);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            /* Optional 'reference' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &rev->ref,
                                     &refdone, 
                                     &mod->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    ncx_free_revhist(rev);
                    return res;
                }
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            log_error("\nError: invalid reference-stmt");
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }
    }

    if (rev->version) {
        /* check if the version string is valid */
        res = yang_validate_date_string(tkc, 
                                        mod, 
                                        &rev->tkerr, 
                                        rev->version);
        CHK_EXIT(res, retres);
            
        /* check if the revision is already present */
        testrev = ncx_find_revhist(mod, rev->version);
        if (testrev) {
            /* error for dup. revision with same version */
            retres = ERR_NCX_DUP_ENTRY;
            log_error("\nError: revision with same date on line %u",
                      testrev->tkerr.linenum);
            tkc->curerr = &rev->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }
    }

    /* save the revision struct */
    rev->res = retres;
    dlq_enque(rev, &mod->revhistQ);
    return retres;

}  /* consume_revision */


/********************************************************************
* FUNCTION consume_revision_stmts
* 
* Parse the next N tokens as N revision statements
* Create a ncx_revhist struct and add it to the specified module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module struct that will get the ncx_revhist_t entries
*   pcb == parser control block to use to check rev-date
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_revision_stmts (tk_chain_t *tkc,
                            ncx_module_t  *mod,
                            yang_pcb_t *pcb)
{
    const xmlChar *val;
    const char    *expstr;
    ncx_revhist_t *rev;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done;
    int            ret;

    expstr = "description keyword";
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    while (!done) {

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        case TK_TT_RBRACE:
            TK_BKUP(tkc);
            done = TRUE;
            continue;
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_REVISION)) {
            res = consume_revision(tkc, mod);
            CHK_EXIT(res, retres);
        } else if (!yang_top_keyword(val)) {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
        } else {
            /* assume we reached the end of this sub-section
             * if there are simply clauses out of order, then
             * the compiler will not detect that.
             */
            TK_BKUP(tkc);
            done = TRUE;
        }
    }

    /* search the revision date strings, find the
     * highest value date string
     */
    val = NULL;
    for (rev = (ncx_revhist_t *)dlq_firstEntry(&mod->revhistQ);
         rev != NULL;
         rev = (ncx_revhist_t *)dlq_nextEntry(rev)) {

        if (rev->res == NO_ERR) {
            if (!val) {
                /* first pass through loop */
                val = rev->version;
            } else {
                ret = yang_compare_revision_dates(rev->version, val);
                if (ret > 0) {
                    if (ncx_warning_enabled(ERR_NCX_BAD_REV_ORDER)) {
                        log_warn("\nWarning: revision dates not in "
                                 "descending order");
                        tkc->curerr = &rev->tkerr;
                        ncx_print_errormsg(tkc, 
                                           mod, 
                                           ERR_NCX_BAD_REV_ORDER);
                    } else {
                        ncx_inc_warnings(mod);
                    }

                    val = rev->version;
                }
            }
        }
    }

    /* assign the module version string */
    if (val) {
        /* valid date string found */
        mod->version = xml_strdup(val);
    }

    /* check if revision warnings should be skipped */
    if (pcb->searchmode) {
        return retres;
    }

    /* leave the version NULL if no good revision dates found */
    if (!val) {
        if (ncx_warning_enabled(ERR_NCX_NO_REVISION)) {
            log_warn("\nWarning: no revision statements "
                     "for %smodule '%s'",
                     (mod->ismod) ? "" : "sub", mod->name);
            mod->warnings++;
        } else {
            ncx_inc_warnings(mod);
        }
    } else if (pcb->revision && 
               yang_compare_revision_dates(val, pcb->revision)) {

        log_error("\nError: found version '%s' instead of "
                  "requested version '%s'",
                  (val) ? val : EMPTY_STRING,
                  pcb->revision);
        retres = ERR_NCX_WRONG_VERSION;
        ncx_print_errormsg(tkc, mod, retres);
    }

    return retres;

}  /* consume_revision_stmts */


/********************************************************************
* FUNCTION consume_body_stmts
* 
* Parse the next N tokens as N body statements
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == module struct in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_body_stmts (yang_pcb_t *pcb,
                        tk_chain_t *tkc,
                        ncx_module_t  *mod)
{
    const char    *expstr = "body statement";
    status_t       res = NO_ERR, retres = NO_ERR;
    boolean        done = FALSE;

    while (!done) {

        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            if (res == ERR_NCX_EOF) {
                res = ERR_NCX_MISSING_RBRACE;
            }
            return res;
        }

        tk_type_t tktyp = TK_CUR_TYP(tkc);
        const xmlChar *val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            return res;
        case TK_TT_RBRACE:
            /* found end of module */
            TK_BKUP(tkc);
            return retres;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &mod->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            yang_skip_statement(tkc, mod);
            continue;
        }

        /* Got a token string so check the keyword value
         * separate the top-level only statements from
         * the data-def-stmts, which can appear within
         * groupings and nested objects
         */
        if (!xml_strcmp(val, YANG_K_EXTENSION)) {
            res = yang_ext_consume_extension(tkc, mod);
        } else if (!xml_strcmp(val, YANG_K_FEATURE)) {
            res = consume_feature(tkc, mod);
        } else if (!xml_strcmp(val, YANG_K_IDENTITY)) {
            res = consume_identity(tkc, mod);
        } else if (!xml_strcmp(val, YANG_K_TYPEDEF)) {
            res = yang_typ_consume_typedef( pcb, tkc, mod, &mod->typeQ );
        } else if (!xml_strcmp(val, YANG_K_GROUPING)) {
            res = yang_grp_consume_grouping( pcb, tkc, mod, &mod->groupingQ, 
                                             NULL );
        } else if (!xml_strcmp(val, YANG_K_RPC)) {
            res = yang_obj_consume_rpc( pcb, tkc, mod );
        } else if (!xml_strcmp(val, YANG_K_NOTIFICATION)) {
            res = yang_obj_consume_notification( pcb, tkc, mod );
        } else if (!xml_strcmp(val, YANG_K_AUGMENT)) {
            res = yang_obj_consume_augment( pcb, tkc, mod );
        } else if (!xml_strcmp(val, YANG_K_DEVIATION)) {
            res = yang_obj_consume_deviation( pcb, tkc, mod );
        } else {
            res = yang_obj_consume_datadef( pcb, tkc, mod, &mod->datadefQ, 
                                            NULL );
        }
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* consume_body_stmts */


/********************************************************************
* FUNCTION check_module_name
* 
*  Check if the module name part of the sourcefn is the
*  same as the mod name
*
* INPUTS:
*   mod == ncx_module in progress
*
* RETURNS:
*   TRUE if name check OK
*   FALSE if name check failed
*********************************************************************/
static boolean
    check_module_name (ncx_module_t *mod)
{
    const xmlChar *str;
    boolean  ret = TRUE;
    uint32   len1, len2;

    if (mod->sourcefn != NULL && mod->name != NULL) {
        str = mod->sourcefn;
        while (*str != '\0' && *str != '@') {
            str++;
        }
        if (*str == '@') {
            len1 = (uint32)(str - mod->sourcefn);
        } else {
            len1 = xml_strlen(mod->sourcefn);
            if (len1 > 2) {
                str = &mod->sourcefn[len1-1];
                while (str > mod->sourcefn && *str != '.') {
                    str--;
                }
                if (str > mod->sourcefn) {
                    len1 = (uint32)(str - mod->sourcefn);
                } else {
                    return FALSE;
                }
            } else {
                return FALSE;
            }
        }

        len2 = xml_strlen(mod->name);

        if (len1 != len2) {
            ret = FALSE;
        } else if (xml_strncmp(mod->sourcefn,
                               mod->name,
                               len1)) {
            ret = FALSE;
        }
    }

    return ret;

}  /* check_module_name */


/********************************************************************
* FUNCTION parse_yang_module
* 
* Parse, generate and register one ncx_module_t struct
* from an NCX instance document stream containing one NCX module,
* which conforms to the NcxModule ABNF.
*
* This is just the first pass parser.
* Many constructs are not validated or internal data structures
* completed yet, after this pass is done.
* This allows support for syntax error checking first,
* and support for forward references i the DML
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == ncx_module in progress
*   pcb == YANG parser control block
*   ptyp == yang parse type
*   wasadded == pointer to return registry-added flag
*
* OUTPUTS:
*   *wasadded == TRUE if the module was addeed to the NCX moduleQ
*             == FALSE if error-exit, not added
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    parse_yang_module (tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       yang_pcb_t *pcb,
                       yang_parsetype_t ptyp,
                       boolean *wasadded)
{
    yang_node_t    *node;
    ncx_feature_t  *feature;
    ncx_identity_t *identity;
    boolean         ismain, loaded, ietfnetconf, done;
    status_t        res, retres;

#ifdef YANG_PARSE_DEBUG_TRACE
    if (LOGDEBUG3) {
        log_debug3("\nEnter parse_yang_module");
        if (mod->sourcefn) {
            log_debug3(" %s", mod->sourcefn);
        }
        yang_dump_nodeQ(&pcb->impchainQ, "impchainQ");
        yang_dump_nodeQ(&pcb->incchainQ, "incchainQ");
        if (mod->ismod) {
            yang_dump_nodeQ(&mod->allincQ, "allincQ");
        }
    }
#endif

    loaded = FALSE;
    ietfnetconf = FALSE;
    ismain = TRUE;
    retres = NO_ERR;
    *wasadded = FALSE;

    /* could be module or submodule -- get the first keyword */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
    } else if (TK_CUR_ID(tkc)) {
        /* got an ID token, make sure it is correct to continue */
        res = NO_ERR;
        if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_MODULE)) {
            ismain = TRUE;
            TK_BKUP(tkc);
            res = ncx_consume_name(tkc, 
                                   mod, 
                                   YANG_K_MODULE, 
                                   &mod->name, 
                                   NCX_REQ, 
                                   TK_TT_LBRACE);
        } else if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_SUBMODULE)) {
            ismain = FALSE;
            TK_BKUP(tkc);
            res = ncx_consume_name(tkc, 
                                   mod, 
                                   YANG_K_SUBMODULE, 
                                   &mod->name, 
                                   NCX_REQ, 
                                   TK_TT_LBRACE);
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_print_errormsg(tkc, mod, res);
        }
#ifdef YANG_PARSE_DEBUG_MEMORY
        if (res == NO_ERR && LOGDEBUG3) {
            log_debug3(" malloced %p %smodule '%s'",
                       mod,
                       ismain ? "" : "sub",
                       mod->name);
        }
#endif
    } else {
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_print_errormsg(tkc, mod, res);
    }
    CHK_EXIT(res, retres);

    /* exit on all errors, since this is probably not a YANG file */
    if (retres != NO_ERR) {
        return res;
    }

    /* make sure the deviation or annotation parameter is pointing at
     * a module and not a submodule, if this is deviation mode
     */
    if (pcb->deviationmode && !ismain) {
        res = ERR_NCX_EXP_MODULE;
        log_error("\nError: deviation or annotation "
                  "cannot be a submodule");
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* got a start of [sub]module OK, check the parse type */
    switch (ptyp) {
    case YANG_PT_TOP:
        if (pcb->with_submods && !ismain) {
            return ERR_NCX_SKIPPED;
        } else {
            pcb->top = mod;
        }
        break;
    case YANG_PT_INCLUDE:
        if (ismain) {
            log_error("\nError: including module '%s', "
                      "should be submodule", mod->name);
            retres = ERR_NCX_EXP_SUBMODULE;
            ncx_print_errormsg(tkc, mod, retres);
            return retres;
        }
        break;
    case YANG_PT_IMPORT:
        if (!ismain) {
            log_error("\nError: importing submodule '%s', "
                      "should be module", mod->name);
            retres = ERR_NCX_EXP_MODULE;
            ncx_print_errormsg(tkc, mod, retres);
            return retres;
        }
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }


#ifdef YANG_PARSE_DEBUG
    if (res == NO_ERR && LOGDEBUG2) {
        log_debug2("\nyang_parse: Start %smodule '%s' OK",
                   ismain ? "" : "sub",
                   mod->name);
    }
#endif

    mod->ismod = ismain;

    /* check if the file name matches the [sub]module name given */
    if (check_module_name(mod) == FALSE) {
        if (ncx_warning_enabled(ERR_NCX_FILE_MOD_MISMATCH)) {
            log_warn("\nWarning: base file name in '%s' should match "
                     "the %smodule name '%s'",
                     mod->sourcefn,
                     ismain ? "" : "sub",
                     mod->name);
            ncx_print_errormsg(tkc, 
                               mod, 
                               ERR_NCX_FILE_MOD_MISMATCH);
        } else {
            ncx_inc_warnings(mod);
        }
    }

    if (ismain) {
        /* consume module-header-stmts */
        res = consume_mod_hdr(tkc, mod);
        CHK_EXIT(res, retres);
    } else {
        /* consume submodule-header-stmts */
        res = consume_submod_hdr(tkc, mod);
        CHK_EXIT(res, retres);
    }

    if (mod->prefix == NULL) {
        if (retres == NO_ERR) {
            SET_ERROR(ERR_INTERNAL_VAL);
            retres = ERR_NCX_INVALID_VALUE;
        }
        log_error("\nError: cannot continue without module prefix set");
        return retres;
    }

    if (mod->ismod && mod->ns == NULL) {
        if (retres == NO_ERR) {
            SET_ERROR(ERR_INTERNAL_VAL);
            retres = ERR_NCX_INVALID_VALUE;
        }
        log_error("\nError: cannot continue without module namespace set");
        return retres;
    }

    /* set the namespace and the XML prefix now,
     * so all namespace assignments in XPath
     * expressions will be valid when first parsed
     */
    if (!(pcb->deviationmode || pcb->diffmode || pcb->searchmode)) {
        /* add real or temp module NS to the registry */
        res = ncx_add_namespace_to_registry(mod, pcb->parsemode);
        CHK_EXIT(res, retres);
    }

    /* Get the linkage statements (imports, include) */
    res = consume_linkage_stmts(tkc, mod, pcb);
    CHK_EXIT(res, retres);

    if (!pcb->keepmode && retres != NO_ERR) {
        if (LOGDEBUG) {
            log_debug("\nStop parsing '%s' due to linkage errors",
                      mod->name);
        }
        return retres;
    }

    /* Get the meta statements (organization, etc.) */
    res = consume_meta_stmts(tkc, mod);
    CHK_EXIT(res, retres);

    /* Get the revision statements */
    res = consume_revision_stmts(tkc, mod, pcb);
    CHK_EXIT(res, retres);

    /* make sure there is at least name and prefix to continue
     * do not continue if requested version does not match
     */
    if (retres == ERR_NCX_WRONG_VERSION ||
        !mod->name || (mod->ismod && !mod->prefix) || 
        !*mod->name || (mod->ismod && !*mod->prefix)) {
        return retres;
    }

    /* check for early exit if just searching for modules */
    if (pcb->searchmode) {
        return retres;
    }

    /* special hack -- check if this is the ietf-netconf
     * YANG module, in which case do not load it because
     * the internal netconf.yang needs to be used instead
     *
     * Only do this hack in the client and server, 
     * if yuma-netconf.yang is already loaded.
     */
    if (mod->ismod && 
        !xml_strcmp(mod->name, NCXMOD_IETF_NETCONF)) {

        if (ncx_find_module(NCXMOD_NETCONF, NULL) != NULL) {
            mod->nsid = xmlns_nc_id();
            ietfnetconf = TRUE;
        }
    }

    /* check if this module is already loaded, except in diff mode 
     * this cannot be done until the module revision is known, which
     * unfortunately is not at the start of the module, but rather
     * after all the other headers
     */
    if (mod->ismod && 
        !pcb->diffmode &&
        ncx_find_module(mod->name, mod->version)) {
        switch (ptyp) {
        case YANG_PT_TOP:
            loaded = TRUE;
            break;
        case YANG_PT_IMPORT:
            return NO_ERR;
        default:
            ;
        }
    }

    /* Get the definition statements */
    res = consume_body_stmts(pcb, tkc, mod);
    CHK_EXIT(res, retres);
    if (res != ERR_NCX_EOF) {
        if (res != ERR_NCX_MISSING_RBRACE) {
            /* the next node should be the '(sub)module' end node */
            res = ncx_consume_token(tkc, mod, TK_TT_RBRACE);
            if (res != ERR_NCX_EOF) {
                CHK_EXIT(res, retres);

                /* check extra tokens left over */
                res = TK_ADV(tkc);
                if (res == NO_ERR) {
                    retres = ERR_NCX_EXTRA_NODE;
                    log_error("\nError: Extra input after end of module"
                              " starting on line %u", TK_CUR_LNUM(tkc));
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        }
    }  /* else errors already reported; inside body-stmts */

    /**************** Module Validation *************************/

    if (pcb->deviationmode) {
        /* Check any deviations, record the module name
         * and save the deviation in the global
         * deviationQ within the parser control block
         */
        if (LOGDEBUG4) {
            log_debug4("\nyang_parse: resolve deviations");
        }
        res = yang_obj_resolve_deviations(pcb, tkc, mod);
        return res;
    }

    /* check all the module level extension usage */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve appinfoQ");
    }
    res = ncx_resolve_appinfoQ(pcb, tkc, mod, &mod->appinfoQ);
    CHK_EXIT(res, retres);

    /* check all the module level extension usage
     * within the include, import, and feature statements
     */
    res = resolve_mod_appinfo(pcb, tkc, mod);
    CHK_EXIT(res, retres);

    /* resolve any if-feature statements within the featureQ */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve features");
    }
    for (feature = (ncx_feature_t *)dlq_firstEntry(&mod->featureQ);
         feature != NULL;
         feature = (ncx_feature_t *)dlq_nextEntry(feature)) {

        res = resolve_feature(pcb, tkc, mod, feature);
        CHK_EXIT(res, retres);
    }

    /* check for any if-feature loops caused by this module */
    for (feature = (ncx_feature_t *)dlq_firstEntry(&mod->featureQ);
         feature != NULL;
         feature = (ncx_feature_t *)dlq_nextEntry(feature)) {

        res = check_feature_loop(tkc, mod, feature, feature);
        CHK_EXIT(res, retres);
    }

    /* resolve the base-stmt within any identity statements 
     * within the identityQ 
     */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve identities");
    }
    for (identity = (ncx_identity_t *)dlq_firstEntry(&mod->identityQ);
         identity != NULL;
         identity = (ncx_identity_t *)dlq_nextEntry(identity)) {

        res = resolve_identity(pcb, tkc, mod, identity);
        CHK_EXIT(res, retres);
    }

    /* resolve any identity base loops within the identityQ */
    for (identity = (ncx_identity_t *)dlq_firstEntry(&mod->identityQ);
         identity != NULL;
         identity = (ncx_identity_t *)dlq_nextEntry(identity)) {

        res = check_identity_loop(tkc, mod, identity, identity);
        CHK_EXIT(res, retres);
    }

    /* Validate any module-level typedefs */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve typedefs");
    }
    res = yang_typ_resolve_typedefs(pcb, tkc, mod, &mod->typeQ, NULL);
    CHK_EXIT(res, retres);

    /* Validate any module-level groupings */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve groupings");
    }
    res = yang_grp_resolve_groupings(pcb, tkc, mod, &mod->groupingQ, NULL);
    CHK_EXIT(res, retres);

    /* Validate any module-level data-def-stmts */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve datadefs");
    }
    res = yang_obj_resolve_datadefs(pcb, tkc, mod, &mod->datadefQ);
    CHK_EXIT(res, retres);

    /* Expand and validate any uses-stmts within module-level groupings */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve groupings complete");
    }
    res = yang_grp_resolve_complete(pcb, tkc, mod, &mod->groupingQ, NULL);
    CHK_EXIT(res, retres);

    /* Expand and validate any uses-stmts within module-level datadefs */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve uses");
    }
    res = yang_obj_resolve_uses(pcb, tkc, mod, &mod->datadefQ);
    CHK_EXIT(res, retres);

    /* Expand and validate any augment-stmts within module-level datadefs */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve augments");
    }
    res = yang_obj_resolve_augments(pcb, tkc, mod, &mod->datadefQ);
    CHK_EXIT(res, retres);

    /* Expand and validate any deviation-stmts within the module
     * !!! This must be done before any object pointers such
     * !!! as leafref paths and must/when XPath expressions
     * !!! are cached.  The xpath_find_schema_target_int
     * !!! function should be used instead of caching leafref
     * !!! target object pointers
     */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve deviations");
    }
    res = yang_obj_resolve_deviations(pcb, tkc, mod);
    CHK_EXIT(res, retres);

    /* remove any nodes that were marked as deleted by deviations */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: remove deleted nodes");
    }
    res = yang_obj_remove_deleted_nodes(pcb, tkc, mod, &mod->datadefQ);

    /* One final check for grouping integrity */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve grp final");
    }
    res = yang_grp_resolve_final(pcb, tkc, mod, &mod->groupingQ);
    CHK_EXIT(res, retres);

    /* Final check for object integrity */
    if (LOGDEBUG4) {
        log_debug4("\nyang_parse: resolve obj final");
    }
    res = yang_obj_resolve_final(pcb, tkc, mod, &mod->datadefQ, FALSE);
    CHK_EXIT(res, retres);

    /* Validate all the XPath expressions within all cooked objects */
    if (mod->ismod || pcb->top == mod) {
        /* this will resolve all the XPath usage within
         * this module or submodule; 
         * need to wait until all the includes are processed for
         * the imported module, top module, or top submodule
         */
        if (LOGDEBUG4) {
            log_debug4("\nyang_parse: resolve xpath");
        }

        res = yang_obj_resolve_xpath(tkc, mod, &mod->datadefQ);
        CHK_EXIT(res, retres);

        /* fill in all the list keys in cross-submodule augments */
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {

            if (node->submod) {
                res = yang_obj_top_resolve_final(pcb, tkc, node->submod,
                                                 &node->submod->datadefQ);
                CHK_EXIT(res, retres);

                /* resolve XPath in submodules */
                res = yang_obj_resolve_xpath(tkc, node->submod,
                                             &node->submod->datadefQ);
                CHK_EXIT(res, retres);
            }
        }

        /* Final augment expand
         * List keys that were not cloned in yang_obj_resolve_augment
         * so another check for cloned lists is needed here
         */
        if (LOGDEBUG4) {
            log_debug4("\nyang_parse: resolve augments final");
        }
        res = yang_obj_resolve_augments_final(pcb, tkc, mod, &mod->datadefQ);
        CHK_EXIT(res, retres);

        /* fill in all the list keys in cross-submodule augments */
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {

            if (node->submod == NULL) {
                continue;
            }

            /* check submod augmenting external modules */
            res = yang_obj_resolve_augments_final(pcb, tkc, node->submod, 
                                                  &node->submod->datadefQ);
            CHK_EXIT(res, retres);
        }

        /* Final XPath check
         * defvals for XPath leafs (& leafref) were not checked
         * in yang_obj_resolve_xpath so another check for 
         * these leafs with defaults is needed
         */
        if (LOGDEBUG4) {
            log_debug4("\nyang_parse: resolve XPath final");
        }
        res = yang_obj_resolve_xpath_final(pcb, tkc, mod, &mod->datadefQ);
        CHK_EXIT(res, retres);

        /* final XPath check for all sub-modules */
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {

            if (node->submod == NULL) {
                continue;
            }

            /* check submod augmenting external modules */
            res = yang_obj_resolve_xpath_final(pcb, tkc, node->submod, 
                                               &node->submod->datadefQ);
            CHK_EXIT(res, retres);
        }

        /* final check of defvals in typedefs */
        if (LOGDEBUG4) {
            log_debug4("\nyang_parse: resolve XPath final");
        }
        res = yang_typ_resolve_typedefs_final(tkc, mod, &mod->typeQ);
        CHK_EXIT(res, retres);

        /* final XPath check in typedefs for all sub-modules */
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {

            if (node->submod == NULL) {
                continue;
            }

            /* check submod augmenting external modules */
            res = yang_typ_resolve_typedefs_final(tkc, node->submod, 
                                                  &node->submod->typeQ);
            CHK_EXIT(res, retres);
        }

        /* !!! TBD!!!
         * !!! Need to add final XPath check for grouping 
         * !!! only objects expanded from uses will have
         * !!! this final check; unused groupings will not
         * !!! be checked
         */        
    }

    /* check for loops in any leafref XPath targets */
    res = yang_obj_check_leafref_loops(tkc, mod, &mod->datadefQ);
    CHK_EXIT(res, retres);

    /* Check for imports not used warnings */
    yang_check_imports_used(tkc, mod);

    /* save the module parse status */
    mod->status = retres;

    /* make sure there is at least name and prefix to continue */
    if (!mod->name || !ncx_valid_name2(mod->name)) {
        return retres;
    }
    if (mod->ismod) {
        if (!mod->prefix || !ncx_valid_name2(mod->prefix)) {
            return retres;
        }
    } else if (mod->prefix) {
        if (mod->ismod && !ncx_valid_name2(mod->prefix)) {
            return retres;
        }
    } else {
        mod->prefix = xml_strdup(EMPTY_STRING);
        if (!mod->prefix) {
            return ERR_INTERNAL_MEM;
        }
    }

    /* add the NS definitions to the registry;
     * check the parse type first
     */
    switch (ptyp) {
    case YANG_PT_TOP:
    case YANG_PT_IMPORT:
        /* finish up the import or top module */
        if (mod->ismod || pcb->top == mod) {
            /* update the result, errors, and warnings */
            if (mod->ismod) {
                res = NO_ERR;
                done = FALSE;
                for (node = (yang_node_t *)
                         dlq_firstEntry(&mod->allincQ);
                     node != NULL && !done;
                     node = (yang_node_t *)dlq_nextEntry(node)) {

                    if (node->submod) {
                        if (node->submod->status != NO_ERR) {
                            mod->status = retres = node->submod->status;
                            if (NEED_EXIT(node->submod->status)) {
                                done = TRUE;
                            }
                        }
                        mod->errors += node->submod->errors;
                        mod->warnings += node->submod->warnings;
                    } /* else include not found */
                }
            }
            
            /* add this regular module to the registry */
            if (!loaded && retres == NO_ERR) {
                if (ietfnetconf) {
                    res = NO_ERR;
                } else if (pcb->diffmode || pcb->parsemode) {
                    res = ncx_add_to_modQ(mod);
                } else {
                    res = ncx_add_to_registry(mod);
                }
                if (res != NO_ERR) {
                    retres = res;
                } else {
                    /* if mod==top, and top is a submodule, then 
                     * yang_free_pcb will delete the submodule later
                     */
                    if (!ietfnetconf && mod->ismod) {
                        *wasadded = TRUE;
                    }
                }
            }
        } else if (mod->ns) {
            mod->nsid = xmlns_find_ns_by_name(mod->ns);
        }
        break;
    case YANG_PT_INCLUDE:
        /* set to TRUE because module is live in the realmod allincQ */
        *wasadded = TRUE;
        break;
    default:
        retres = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return retres;

}  /* parse_yang_module */


/********************************************************************
* FUNCTION set_source
* 
* Set the [sub]module source
*
* INPUTS:
*   mod == ncx_module_t struct in progress
*   str == source string to use
*********************************************************************/
static void
    set_source (ncx_module_t *mod,
                xmlChar *str)
{
    /* save the source of this ncx-module for monitor / debug 
     * hand off malloced src string here
     */
    mod->source = str;

    /* find the start of the file name */
    mod->sourcefn = &str[xml_strlen(str)];
    while (mod->sourcefn > str &&
           *mod->sourcefn != NCX_PATHSEP_CH) {
        mod->sourcefn--;
    }
    if (*mod->sourcefn == NCX_PATHSEP_CH) {
        mod->sourcefn++;
    }
    
}  /* set_source */

/********************************************************************
* FUNCTION load_yang_module
* 
* Load a Yang Module
*
* INPUTS:
*
* OUTPUTS:
*   res the result of the operation.
*********************************************************************/
static status_t
    load_yang_module ( const xmlChar  *filespec,
                       const xmlChar  *filename,
                       tk_chain_t     **tkc )
{
    FILE           *fp = NULL;

    log_debug( "\nLoading YANG module from file:\n  %s", filespec );
    /* open the YANG source file for reading */
    fp = fopen( (const char *)filename, "r" );
    if (!fp) {
        return ERR_NCX_MISSING_FILE;
    }

    /* setup the token chain to parse this YANG file */
    /* get a new token chain */
    *tkc = tk_new_chain();
    if ( !*tkc ) {
        log_error("\nyang_parse malloc error");
        fclose(fp);
        return ERR_INTERNAL_MEM;
    }
    else {
        // hand off management of fp
        tk_setup_chain_yang( *tkc, fp, filename );
    }

    return NO_ERR;
}

/********************************************************************
* FUNCTION load_yin_module
* 
* Load a Yin Module
*
* INPUTS:
*
* OUTPUTS:
*   res the result of the operation.
*********************************************************************/
static status_t
    load_yin_module ( const xmlChar  *filespec,
                      const xmlChar  *filename,
                      tk_chain_t     **tkc )
{
    status_t res = NO_ERR;

    log_debug( "\nLoading YIN module from file:\n  %s", filespec );
    /* just make sure the file exists for now */
    if ( !ncxmod_test_filespec( filename ) ) {
        return ERR_NCX_MISSING_FILE;
    }

    /* setup the token chain to parse this YANG file */
    /* get a new token chain */
    *tkc = yinyang_convert_token_chain( filename, &res );
    if ( !*tkc ) {
        log_error( "\nyang_parse: Invalid YIN file (%s)", 
                   get_error_string(res) );
        if ( NO_ERR == res ) {
            res = ERR_INTERNAL_MEM;
        }
    }

    return res;
}
/********************************************************************
* FUNCTION configure_module
* 
* Allocate and configre an NCX_Module.
*
* INPUTS:
*     
*
* OUTPUTS:
*   res the result of the operation.
*********************************************************************/
static ncx_module_t*
   configure_module( xmlChar        *filename,
                     yang_pcb_t     *pcb )
{
   ncx_module_t *mod = NULL;

    mod = ncx_new_module();
    if (mod) {
        set_source(mod, filename); /* hand off 'str' malloced memory here */

        /* set the back-ptr to parent of this submodule 
         * or NULL if this is not a submodule
         */
        mod->parent = pcb->parentparm;

        /* set the stmt-track mode flag to the master flag in the PCB */
        mod->stmtmode = pcb->stmtmode;
    } 

   return mod;
}

/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION yang_parse_from_filespec
* 
* Parse a file as a YANG module
*
* Error messages are printed by this function!!
*
* INPUTS:
*   filespec == absolute path or relative path
*               This string is used as-is without adjustment.
*   pcb == parser control block used as very top-level struct
*   ptyp == parser call type
*            YANG_PT_TOP == called from top-level file
*            YANG_PT_INCLUDE == called from an include-stmt in a file
*            YANG_PT_IMPORT == called from an import-stmt in a file
*   isyang == TRUE if a YANG file is expected
*             FALSE if a YIN file is expected
*
* OUTPUTS:
*   an ncx_module is filled out and validated as the file
*   is parsed.  If no errors:
*     TOP, IMPORT:
*        the module is loaded into the definition registry with 
*        the ncx_add_to_registry function
*     INCLUDE:
*        the submodule is loaded into the top-level module,
*        specified in the pcb
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_parse_from_filespec (const xmlChar *filespec,
                              yang_pcb_t *pcb,
                              yang_parsetype_t ptyp,
                              boolean isyang)
{
    tk_chain_t     *tkc = NULL;
    ncx_module_t   *mod = NULL;
    xmlChar        *str;
    status_t        res = NO_ERR ;
    boolean         wasadd = FALSE;
    boolean         keepmod = FALSE;

#ifdef DEBUG
    if (!filespec || !pcb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* expand and copy the filespec */
    str = ncx_get_source(filespec, &res);
    if ( !str || NO_ERR != res ) {
        if ( str ) {
            m__free( str );
        }
        else {
            res = ERR_INTERNAL_MEM;
        }
        return res;
    }

    res = (isyang) ? load_yang_module( filespec, str, &tkc )
                   : load_yin_module( filespec, str, &tkc );

    if ( !tkc ) {
        m__free( str );
        return ( NO_ERR == res ) ? ERR_INTERNAL_MEM : res;
    }
    else if ( NO_ERR != res ) {
        tk_free_chain(tkc);
        m__free( str );
        return res;
    }
    
    if ( pcb->docmode && (ptyp == YANG_PT_TOP || ptyp == YANG_PT_INCLUDE)) {
        tk_setup_chain_docmode(tkc);
    }

    mod = configure_module( str, pcb );
    if ( !mod ) {
        m__free( str );
        if ( tkc->fp ) {
            fclose( tkc->fp );
        }
        tk_free_chain(tkc);
        return  ERR_INTERNAL_MEM;
    }

    if (isyang) {
        /* serialize the file into language tokens
         * !!! need to change this later because it may use too
         * !!! much memory in embedded parsers */
        res = tk_tokenize_input(tkc, mod);
        if ( NO_ERR != res ) {
            ncx_free_module(mod);

            if ( tkc->fp ) {
                fclose( tkc->fp );
            }
            tk_free_chain(tkc);
            return res;
        }
    }

#ifdef YANG_PARSE_TK_DEBUG
     tk_dump_chain(tkc);
#endif

    /* parse the module and validate it only if a token chain
     * was properly parsed; set this only once for the top module
     */
    if ( pcb->savetkc && !pcb->docmode && pcb->tkc == NULL) {
        pcb->tkc = tk_clone_chain(tkc);
        if (pcb->tkc == NULL) {
            ncx_free_module(mod);
            if ( tkc->fp ) {
                fclose( tkc->fp );
            }
            tk_free_chain(tkc);
            return  ERR_INTERNAL_MEM;
        }
    }

    res = parse_yang_module( tkc, mod, pcb, ptyp, &wasadd );

    if (pcb->top == mod) {
        pcb->topadded = wasadd;
        pcb->retmod = NULL;
    } else if (pcb->top) {
        /* just parsed an import or include */
        pcb->retmod = mod;
    } else {
        /* the module did not start correctly */
        pcb->retmod = NULL;
    }

    if (res != NO_ERR) {
        /* cleanup in all modes if there was an error */
        if (pcb->retmod != NULL) {
            /* got a submodule or import module that is not top make sure 
             * this does not end up in the wrong Q and get freed 
             * incorrectly if import/include mismatch */
            if ((res == ERR_NCX_SKIPPED) ||
                (ptyp == YANG_PT_IMPORT && !mod->ismod) ||
                (ptyp == YANG_PT_INCLUDE && mod->ismod)) {
                pcb->retmod = NULL;
                ncx_free_module(mod);
                mod = NULL;
            }
        } 

        if (mod != NULL && (pcb->top == NULL || (!wasadd && !pcb->keepmode))) {
            /* module was not added to registry so it is live this will be 
             * skipped for an include file because wasadd is always set to 
             * TRUE for submods; this is an invalid module if top not set */
            if (pcb->top == mod) {
                pcb->top = NULL;
            }
            pcb->retmod = NULL;
            ncx_free_module(mod);
            mod = NULL;
        } /* else the module will be freed in ncx_cleanup or ncx_free_module
           * for a submodule */
    } else if (pcb->deviationmode) {
        /* toss the module even if NO_ERR, not needed since the savedevQ 
         * contains the deviations and annotations */
        if ( pcb->top == mod) {
            pcb->top = NULL;
        }
        pcb->retmod = NULL;
        ncx_free_module(mod);
        mod = NULL;
    } else if (!wasadd) {
        /* module was not added to the registry which means it was already 
         * there or it is a submodule; decide if the caller really wants 
         * the return module in this case */
        if (pcb->parsemode) {
            /* parse mode for yangcli_autoload do not need to keep the 
             * duplicate */
            if ( pcb->top == mod) {
                pcb->top = NULL;
            }
            pcb->retmod = NULL;
            ncx_free_module(mod);
            mod = NULL;
        } else if (!(pcb->diffmode || pcb->searchmode)) {
            /* do not swap out diffmode or searchmode for all other modes, 
             * check if the new module should be returned or a different 
             * module for the yuma-netconf hack */
            if (mod->ismod) {
                if (pcb->top == mod) {
                    /* hack: make sure netconf-ietf does not get swapped 
                     * out for netconf.yang; the internal version is used 
                     * instead of the standard one to fill in the missing 
                     * pieces */
                    if (pcb->keepmode) {
                        keepmod = TRUE;
                    } else if (xml_strcmp(mod->name, NCXMOD_IETF_NETCONF)) {
                        /* swap with the real module already done */
                        pcb->top = ncx_find_module(mod->name, mod->version);
                        pcb->retmod = NULL;
                    } else {
                        keepmod = TRUE;
                    }
                }
            } else {
                /* this is a submodule, make sure it was included */
                if (ptyp != YANG_PT_IMPORT) {
                    if (pcb->keepmode) {
                        keepmod = TRUE;
                    }
                }

                if (!pcb->with_submods) {
                    /* this is a submodule and they are being processed 
                     * instead of skipped; sub tree parsing mode can cause 
                     * top-level to already be loaded into the registry, 
                     * swap out the new dummy module with the real one */
                    if (!pcb->keepmode && pcb->top == mod) {

                        /*  !!!! leave this out now !!!
                        pcb->top = ncx_find_module(mod->belongs,
                                                   mod->version);
                        pcb->retmod = mod;
                        */
                        pcb->retmod = NULL;
                    }
                } else if (pcb->top == mod) {
                    /* don't care about submods in this mode so clear
                     * the top pointer so it won't be used */
                    pcb->retmod = NULL;
                    pcb->top = NULL;
                }
            }
            if (!pcb->top && !pcb->with_submods) {
                if (mod->ismod) {
                    res = ERR_NCX_MOD_NOT_FOUND;
                } else if (!keepmod) {
                    res = ERR_NCX_SUBMOD_NOT_LOADED;
                } else {
                    res = NO_ERR;
                }
            }

            if (!keepmod) {
                ncx_free_module(mod);
                mod = NULL;
            }
        }
    } else {
        /* was added */
        keepmod = (pcb->keepmode && pcb->top == mod) ? TRUE : FALSE;
    }

    /* final cleanup */
    if ( tkc->fp ) {
        fclose( tkc->fp );
        tkc->fp = NULL;
    }
    if (keepmod && pcb->tkc == NULL) {
        /* this is still NULL because pcb->docmode is TRUE and the altered 
         * token chain is desired, not * the clone like YIN parsing */
        pcb->tkc = tkc;
    } else {
        tk_free_chain(tkc);
    }

    return res;

}  /* yang_parse_from_filespec */


/* END file yang_parse.c */
