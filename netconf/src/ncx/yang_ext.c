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
/*  FILE: yang_ext.c


    YANG module parser, extension statement support


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
05jan08      abb      begun; start from yang_grp.c


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

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_ext
#include  "ext.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_appinfo
#include "ncx_appinfo.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_typ
#include  "typ.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yang_ext
#include "yang_ext.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define YANG_EXT_DEBUG 1
#endif


/********************************************************************
*                                                                   *
*                          M A C R O S                              *
*                                                                   *
*********************************************************************/

/* used in parser routines to decide if processing can continue
 * will exit the function if critical error or continue if not
 */
#define CHK_EXT_EXIT                                      \
    if (res != NO_ERR) {                                  \
        if (res < ERR_LAST_SYS_ERR || res==ERR_NCX_EOF) { \
            ext_free_template(ext);                       \
            return res;                                   \
        } else {                                          \
            retres = res;                                 \
        }                                                 \
    }


/********************************************************************
* FUNCTION consume_yang_arg
* 
* Parse the next N tokens as an argument-stmt
* Fill in the arg anf argel fields in the ext_template_t struct
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'argument' keyword
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   ext == extension template in progress
*   argdone == address of duplicate entry error flag
*
* OUTPUTS:
*   *argdone == TRUE upon exit
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_yang_arg (tk_chain_t *tkc,
                      ncx_module_t  *mod,
                      ext_template_t *ext,
                      boolean *argdone)
{
    const xmlChar   *val;
    const char      *expstr;
    xmlChar         *errstr;
    dlq_hdr_t        errQ;
    tk_type_t        tktyp;
    boolean          done, yinel, save, errsave;
    status_t         res, retres;

#ifdef DEBUG
    if (!tkc || !mod) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    val = NULL;
    expstr = NULL;
    done = FALSE;
    yinel = FALSE;
    save = TRUE;
    res = NO_ERR;
    retres = NO_ERR;
    dlq_createSQue(&errQ);

    /* check duplicate entry error first */
    if (*argdone) {
        retres = ERR_NCX_ENTRY_EXISTS;
        ncx_print_errormsg(tkc, mod, retres);
        save = FALSE;
    } else {
        *argdone = TRUE;
    }

    /* Get the mandatory argument name */
    if (save) {
        res = yang_consume_id_string(tkc, mod, &ext->arg);
    } else {
        errstr = NULL;
        res = yang_consume_id_string(tkc, mod, &errstr);
        if (errstr) {
            m__free(errstr);
        }
    }
    CHK_EXIT(res, retres);

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the extension-stmt
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

    /* get the extension statements and any appinfo extensions */
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
            if (save) {
                res = ncx_consume_appinfo(tkc, mod, &ext->appinfoQ);
            } else {
                res = ncx_consume_appinfo(tkc, mod, &errQ);
                ncx_clean_appinfoQ(&errQ);
            }
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_YIN_ELEMENT)) {
            if (save) {
                res = yang_consume_boolean(tkc, 
                                           mod, 
                                           &ext->argel,
                                           &yinel, 
                                           &ext->appinfoQ);
            } else {
                res = yang_consume_boolean(tkc, 
                                           mod, 
                                           &errsave,
                                           NULL, 
                                           NULL);
            }
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* consume_yang_arg */


/********************************************************************
* FUNCTION yang_ext_consume_extension
* 
* Parse the next N tokens as an extension-stmt
* Create an ext_template_t struct and add it to the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'extension' keyword
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_ext_consume_extension (tk_chain_t *tkc,
                                ncx_module_t  *mod)
{
    ext_template_t  *ext, *testext;
    const xmlChar   *val;
    const char      *expstr;
    yang_stmt_t     *stmt;
    tk_type_t        tktyp;
    boolean          done, arg, stat, desc, ref;
    status_t         res, retres;

#ifdef DEBUG
    if (!tkc || !mod) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    val = NULL;
    expstr = "keyword";
    done = FALSE;
    arg = FALSE;
    stat = FALSE;
    desc = FALSE;
    ref = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

    /* Get a new ext_template_t to fill in */
    ext = ext_new_template();
    if (!ext) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    ncx_set_error(&ext->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    /* Get the mandatory extension name */
    res = yang_consume_id_string(tkc, mod, &ext->name);
    CHK_EXT_EXIT;

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the extension-stmt
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        ext_free_template(ext);
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

    /* get the extension statements and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            ext_free_template(ext);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            ext_free_template(ext);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &ext->appinfoQ);
            CHK_EXT_EXIT;
            continue;
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_ARGUMENT)) {
            res = consume_yang_arg(tkc, mod, ext, &arg);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &ext->status,
                                      &stat, 
                                      &ext->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &ext->descr,
                                     &desc, 
                                     &ext->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &ext->ref,
                                     &ref, 
                                     &ext->appinfoQ);
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        CHK_EXT_EXIT;
    }

    /* save or delete the ext_template_t struct */
    if (ext->name && ncx_valid_name2(ext->name)) {
        testext = ext_find_extension_all(mod, ext->name);
        if (testext) {
            log_error("\nError: extension '%s' already defined "
                      "in '%s' at line %u",
                      ext->name, 
                      testext->tkerr.mod->name,
                      testext->tkerr.linenum);
            retres = ERR_NCX_DUP_ENTRY;
            ncx_print_errormsg(tkc, mod, retres);
            ext_free_template(ext);
        } else {
            dlq_enque(ext, &mod->extensionQ);  /* may have some errors */
            if (mod->stmtmode) {
                stmt = yang_new_ext_stmt(ext);
                if (stmt) {
                    dlq_enque(stmt, &mod->stmtQ);
                } else {
                    log_error("\nError: malloc failure for ext_stmt");
                    retres = ERR_INTERNAL_MEM;
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        }
    } else {
        ext_free_template(ext);
    }

    return retres;

}  /* yang_ext_consume_extension */


/* END file yang_ext.c */
