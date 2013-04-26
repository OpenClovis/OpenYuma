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
/*  FILE: yang.c


   YANG module parser utilities

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
15novt07      abb      begun; split out from yang_parse.c


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

#ifndef _H_grp
#include  "grp.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_appinfo
#include "ncx_appinfo.h"
#endif

#ifndef _H_ncx_feature
#include "ncx_feature.h"
#endif

#ifndef _H_ncx_num
#include "ncx_num.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tstamp
#include  "tstamp.h"
#endif

#ifndef _H_typ
#include  "typ.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xpath1
#include "xpath1.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
/* #define YANG_DEBUG 1 */
#endif

/********************************************************************
*                                                                   *
*                       S T A T I C   D A T A                       *
*                                                                   *
*********************************************************************/

static const xmlChar *top_keywords[] = 
{
    YANG_K_ANYXML,
    YANG_K_AUGMENT,
    YANG_K_BELONGS_TO,
    YANG_K_CHOICE,
    YANG_K_CONTACT,
    YANG_K_CONTAINER,
    YANG_K_DESCRIPTION,
    YANG_K_DEVIATION,
    YANG_K_EXTENSION,
    YANG_K_FEATURE,
    YANG_K_GROUPING,
    YANG_K_IDENTITY,
    YANG_K_IMPORT,
    YANG_K_INCLUDE,
    YANG_K_LEAF,
    YANG_K_LEAF_LIST,
    YANG_K_LIST,
    YANG_K_NAMESPACE,
    YANG_K_NOTIFICATION,
    YANG_K_ORGANIZATION,
    YANG_K_PREFIX,
    YANG_K_REFERENCE,
    YANG_K_REVISION,
    YANG_K_RPC,
    YANG_K_TYPEDEF,
    YANG_K_USES,
    YANG_K_YANG_VERSION,
    NULL
};

#ifdef YANG_DEBUG
static uint32 new_node_cnt = 0;
static uint32 free_node_cnt = 0;
#endif


/********************************************************************
* FUNCTION yang_consume_semiapp
* 
* consume a stmtsep clause
* Consume a semi-colon to end a simple clause, or
* consume a vendor extension
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   appinfoQ == queue to receive any vendor extension found
*
* OUTPUTS:
*   *appinfoQ has the extesnion added if any
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_semiapp (tk_chain_t *tkc,
                          ncx_module_t *mod,
                          dlq_hdr_t *appinfoQ)
{
    const char   *expstr;
    status_t      res, retres;
    boolean       done;

#ifdef DEBUG
    if (!tkc) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    expstr = "semi-colon or left brace";
    retres = NO_ERR;

    /* get semicolon or left brace */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_mod_exp_err(tkc, mod, res, expstr);
        return res;
    }
        
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        return NO_ERR;
    case TK_TT_LBRACE:
        break;
    case TK_TT_NONE:
    default:
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        switch (TK_CUR_TYP(tkc)) {
        case TK_TT_TSTRING:
        case TK_TT_MSTRING:
        case TK_TT_RBRACE:
            /* try to recover and keep parsing */
            TK_BKUP(tkc);
            break;
        default:
            ;
        }
        return res;
    }

    /* got a left brace '{' 
     * the start of a vendor extension section
     */
    done = FALSE;
    while (!done) {
        res = ncx_consume_appinfo2(tkc, mod, appinfoQ);
        if (res == ERR_NCX_SKIPPED) {
            res = NO_ERR;
            done = TRUE;
        }  else {
            CHK_EXIT(res, retres);
        }
    }

    res = ncx_consume_token(tkc, mod, TK_TT_RBRACE);
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_semiapp */


/********************************************************************
* FUNCTION yang_consume_string
* 
* consume 1 string token
* Consume a YANG string token in any of the 3 forms
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*   if field not NULL,
*        *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_string (tk_chain_t *tkc,
                         ncx_module_t *mod,
                         xmlChar **field)
{
    const char *expstr;
    xmlChar    *str;
    status_t    res;

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (TK_CUR_STR(tkc)) {
        if (field) {
            if (TK_CUR_MOD(tkc)) {
                *field = m__getMem(TK_CUR_MODLEN(tkc)+TK_CUR_LEN(tkc)+2);
                if (*field) {
                    str = *field;
                    str += xml_strncpy(str, TK_CUR_MOD(tkc),
                                       TK_CUR_MODLEN(tkc));
                    *str++ = ':';
                    if (TK_CUR_VAL(tkc)) {
                        xml_strncpy(str, TK_CUR_VAL(tkc),
                                    TK_CUR_LEN(tkc));
                    } else {
                        *str = 0;
                    }
                }
            } else {
                if (TK_CUR_VAL(tkc)) {
                    *field = xml_strdup(TK_CUR_VAL(tkc));
                } else {
                    *field = xml_strdup(EMPTY_STRING);
                }
            }
            if (!*field) {
                res = ERR_INTERNAL_MEM;
                ncx_print_errormsg(tkc, mod, res);
            }
            if (res == NO_ERR) {
                res = tk_check_save_origstr(tkc, 
                                            TK_CUR(tkc),
                                            (const void *)field);
                if (res != NO_ERR) {
                    ncx_print_errormsg(tkc, mod, res);
                }                    
            }
        } /* else server does not save descr/ref */
    } else {
        switch (TK_CUR_TYP(tkc)) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            break;
        case TK_TT_LBRACE:
        case TK_TT_RBRACE:
        case TK_TT_SEMICOL:
            res = ERR_NCX_WRONG_TKTYPE;
            expstr = "string";
            ncx_mod_exp_err(tkc, mod, res, expstr);
            break;
        default:
            if (field) {
                if (TK_CUR_VAL(tkc)) {
                    *field = xml_strdup(TK_CUR_VAL(tkc));
                } else {
                    *field = 
                        xml_strdup((const xmlChar *)
                                   tk_get_token_name(TK_CUR_TYP(tkc)));
                }
                if (!*field) {
                    res = ERR_INTERNAL_MEM;
                    ncx_print_errormsg(tkc, mod, res);
                }
            }
        }
    }

    return res;

}  /* yang_consume_string */


/********************************************************************
* FUNCTION yang_consume_keyword
* 
* consume 1 YANG keyword or vendor extension keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   prefix == address of field to get the prefix string (may be NULL)
*   field == address of field to get the name string (may be NULL)
*
* OUTPUTS:
*   if prefix not NULL,
*        *prefix is set with a malloced string in NO_ERR
*   if field not NULL,
*        *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_keyword (tk_chain_t *tkc,
                          ncx_module_t *mod,
                          xmlChar **prefix,
                          xmlChar **field)
{
    tk_type_t      tktyp;
    status_t       res, retres;


    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    retres = NO_ERR;
    tktyp = TK_CUR_TYP(tkc);

    if (tktyp==TK_TT_QSTRING || tktyp==TK_TT_SQSTRING) {
        res = ERR_NCX_INVALID_VALUE;
        log_error("\nError: quoted strings not allowed for keywords");
    } else if (TK_CUR_ID(tkc)) {
        if (TK_CUR_VAL(tkc)) {

            /* right kind of tokens, validate id name string */
            if (ncx_valid_name(TK_CUR_VAL(tkc), TK_CUR_LEN(tkc))) {
                if (field) {
                    *field = xml_strdup(TK_CUR_VAL(tkc));
                    if (!*field) {
                        res = ERR_INTERNAL_MEM;
                    }
                }
            } else {
                res = ERR_NCX_INVALID_NAME;
            }
            
            if (res != NO_ERR) {
                ncx_mod_exp_err(tkc, mod, res, "identifier-ref string");
                retres = res;
                res = NO_ERR;
            }

            /* validate prefix name string if any */
            if (TK_CUR_MOD(tkc)) {
                if (ncx_valid_name(TK_CUR_MOD(tkc),
                                   TK_CUR_MODLEN(tkc))) {
                    if (prefix) {
                        *prefix = xml_strdup(TK_CUR_MOD(tkc));
                        if (!*prefix) {
                            res = ERR_INTERNAL_MEM;
                        }
                    }
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
            }
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
    } else {
        res = ERR_NCX_WRONG_TKTYPE;
    }

    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        retres = res;
    }
    return retres;

}  /* yang_consume_keyword */


/********************************************************************
* FUNCTION yang_consume_nowsp_string
* 
* consume 1 string without any whitespace
* Consume a YANG string token in any of the 3 forms
* Check No whitespace allowed!
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*   if field not NULL,
*      *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_nowsp_string (tk_chain_t *tkc,
                               ncx_module_t *mod,
                               xmlChar **field)
{
    xmlChar      *str;
    status_t      res;

    res = TK_ADV(tkc);
    if (res == NO_ERR) {
        if (TK_CUR_STR(tkc)) {
            if (TK_CUR_MOD(tkc)) {
                if (field) {
                    *field = m__getMem(TK_CUR_MODLEN(tkc)+TK_CUR_LEN(tkc)+2);
                    if (*field) {
                        str = *field;
                        str += xml_strncpy(str, TK_CUR_MOD(tkc),
                                           TK_CUR_MODLEN(tkc));
                        *str++ = ':';
                        if (TK_CUR_VAL(tkc)) {
                            xml_strncpy(str, TK_CUR_VAL(tkc),
                                        TK_CUR_LEN(tkc));
                        } else {
                            *str = 0;
                        }
                    }
                }
            } else if (TK_CUR_VAL(tkc)) {
                if (!tk_is_wsp_string(TK_CUR(tkc))) {
                    if (field) {
                        *field = xml_strdup(TK_CUR_VAL(tkc));
                        if (!*field) {
                            res = ERR_INTERNAL_MEM;
                        }
                    }
                } else {
                    res = ERR_NCX_INVALID_VALUE;
                }
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            res = ERR_NCX_WRONG_TKTYPE;
        }
    }

    if (res != NO_ERR) {
        ncx_mod_exp_err(tkc, mod, res, "string w/o whitespace");
    }
    return res;
    
}  /* yang_consume_nowsp_string */


/********************************************************************
* FUNCTION yang_consume_id_string
* 
* consume an identifier-str token
* Consume a YANG string token in any of the 3 forms
* Check that it is a valid YANG identifier string

* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*  if field not NULL:
*       *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_id_string (tk_chain_t *tkc,
                            ncx_module_t *mod,
                            xmlChar **field)
{
    status_t      res;

    res = TK_ADV(tkc);
    if (res == NO_ERR) {
        if (TK_CUR_ID(tkc) ||
            (TK_CUR_STR(tkc) && !tk_is_wsp_string(TK_CUR(tkc)))) {
            if (TK_CUR_MOD(tkc)) {
                log_error("\nError: Prefix '%s' not allowed",
                          TK_CUR_MOD(tkc));
                res = ERR_NCX_INVALID_NAME;
            } else if (TK_CUR_VAL(tkc)) {
                if (ncx_valid_name(TK_CUR_VAL(tkc), TK_CUR_LEN(tkc))) {
                    if (field) {
                        *field = xml_strdup(TK_CUR_VAL(tkc));
                        if (!*field) {
                            res = ERR_INTERNAL_MEM;
                        }
                    }
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
            } else {
                res = ERR_NCX_INVALID_NAME;
            }
        } else {
            res = ERR_NCX_WRONG_TKTYPE;
        }
    }

    if (res != NO_ERR) {
        ncx_mod_exp_err(tkc, mod, res, "identifier string");
    } else {
        ncx_check_warn_idlen(tkc, 
                             mod,
                             TK_CUR_VAL(tkc));
    }
    return res;
    
}  /* yang_consume_id_string */


/********************************************************************
* FUNCTION yang_consume_pid_string
* 
* consume an identifier-ref-str token
* Consume a YANG string token in any of the 3 forms
* Check that it is a valid identifier-ref-string
* Get the prefix if any is set
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   prefix == address of prefix string if any is used (may be NULL)
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*  if field not NULL:
*       *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_pid_string (tk_chain_t *tkc,
                             ncx_module_t *mod,
                             xmlChar **prefix,
                             xmlChar **field)
{
    const xmlChar *p;
    tk_type_t      tktyp;
    status_t       res;
    uint32         plen, nlen;

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (prefix) {
        *prefix = NULL;
    }
    if (field) {
        *field = NULL;
    }

    tktyp = TK_CUR_TYP(tkc);

    if (tktyp == TK_TT_QSTRING || 
        tktyp == TK_TT_SQSTRING ||
        tktyp == TK_TT_STRING) {
        p = TK_CUR_VAL(tkc);
        while (*p && *p != ':') {
            p++;
        }
        if (*p) {
            /* found the prefix separator char,
             * try to parse this quoted string
             * as a prefix:identifier
             */
            plen = (uint32)(p - TK_CUR_VAL(tkc));
            nlen = TK_CUR_LEN(tkc) - (plen+1);

            if (plen && plen+1 < TK_CUR_LEN(tkc)) {
                /* have a possible string structure to try */
                if (ncx_valid_name(TK_CUR_VAL(tkc), plen) &&
                    ncx_valid_name(p+1, nlen)) {

                    /* have valid syntax for prefix:identifier */
                    if (prefix) {
                        *prefix = xml_strndup(TK_CUR_VAL(tkc), plen);
                        if (*prefix == NULL) {
                            res = ERR_INTERNAL_MEM;
                        }
                    }
                    if (res == NO_ERR && field) {
                        *field = xml_strndup(p+1, nlen);
                        if (*field == NULL) {
                            res = ERR_INTERNAL_MEM;
                            if (prefix) {
                                m__free(*prefix);
                                *prefix = NULL;
                            }
                        }
                    }
                    if (res != NO_ERR) {
                        ncx_print_errormsg(tkc, mod, res);
                    } else {
                        if (prefix && *prefix) {
                            ncx_check_warn_idlen(tkc, mod, *prefix);
                        }
                        if (field && *field) {
                            ncx_check_warn_idlen(tkc, mod, *field);
                        }
                    }
                    return res;
                }
            }
        } /* else drop through and try again */
    }

    if (TK_CUR_ID(tkc) ||
        (TK_CUR_STR(tkc) && !tk_is_wsp_string(TK_CUR(tkc)))) {
        if (TK_CUR_VAL(tkc)) {
            /* right kind of tokens, validate id name string */
            if (ncx_valid_name(TK_CUR_VAL(tkc), TK_CUR_LEN(tkc))) {
                if (field) {
                    *field = xml_strdup(TK_CUR_VAL(tkc));
                    if (*field == NULL) {
                        res = ERR_INTERNAL_MEM;
                    } else {
                        ncx_check_warn_idlen(tkc, mod, *field);
                    }
                }
            } else {
                res = ERR_NCX_INVALID_NAME;
            }
            
            /* validate prefix name string if any */
            if (res == NO_ERR && TK_CUR_MOD(tkc)) {
                if (ncx_valid_name(TK_CUR_MOD(tkc),
                                   TK_CUR_MODLEN(tkc))) {
                    if (prefix) {
                        *prefix = xml_strdup(TK_CUR_MOD(tkc));
                        if (*prefix == NULL) {
                            res = ERR_INTERNAL_MEM;
                        }
                    }
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
            }
        } else {
            res = ERR_NCX_INVALID_NAME;
        }

        if (res == NO_ERR) {
            if (prefix && *prefix) {
                ncx_check_warn_idlen(tkc, mod, *prefix);
            }
            if (field && *field) {
                ncx_check_warn_idlen(tkc, mod, *field);
            }
        }
    } else {
        res = ERR_NCX_WRONG_TKTYPE;
    }

    if (res != NO_ERR) {
        ncx_mod_exp_err(tkc, mod, res, "identifier-ref string");
        if (prefix && *prefix) {
            m__free(*prefix);
            *prefix = NULL;
        }
        if (field && *field) {
            m__free(*field);
            *field = NULL;
        }
    }
    return res;
    
}  /* yang_consume_pid_string */


/********************************************************************
* FUNCTION yang_consume_error_stmts
* 
* consume the range. length. pattern. must error info extensions
* Parse the sub-section as a sub-section for error-app-tag
* and error-message clauses
*
* Current token is the starting left brace for the sub-section
* that is extending a range, length, pattern, or must statement
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   errinfo == pointer to valid ncx_errinfo_t struct
*              The struct will be filled in by this fn
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *errinfo filled in with any clauses found
*   *appinfoQ filled in with any extensions found
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_error_stmts (tk_chain_t  *tkc,
                              ncx_module_t *mod,
                              ncx_errinfo_t  *errinfo,
                              dlq_hdr_t *appinfoQ)
{
    const xmlChar *val;
    const char    *expstr = 
        "description, reference, error-app-tag, or error-message keyword";
    ncx_errinfo_t *err = errinfo;
    tk_type_t      tktyp;
    status_t       res = NO_ERR;
    boolean        done = FALSE;
    boolean        desc = FALSE;
    boolean        ref = FALSE;
    boolean        etag = FALSE;
    boolean        emsg = FALSE;

#ifdef DEBUG
    if (!tkc || !errinfo) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

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
            res = ncx_consume_appinfo(tkc, mod, appinfoQ);
            if ( ( NO_ERR != res && res < ERR_LAST_SYS_ERR ) || 
                 res == ERR_NCX_EOF ) {
                return res;
            }
            break;

        case TK_TT_RBRACE:
            done = TRUE;
            break;

        case TK_TT_TSTRING:
            /* Got a token string so check the value */
            if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
                /* Optional 'description' field is present */
                res = yang_consume_descr( tkc, mod, &err->descr, 
                                          &desc, appinfoQ );
            } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
                /* Optional 'description' field is present */
                res = yang_consume_descr( tkc, mod, &err->ref, 
                                          &ref, appinfoQ );
            } else if (!xml_strcmp(val, YANG_K_ERROR_APP_TAG)) {
                /* Optional 'error-app-tag' field is present */
                res = yang_consume_strclause( tkc, mod, &err->error_app_tag, 
                                              &etag, appinfoQ );
            } else if (!xml_strcmp(val, YANG_K_ERROR_MESSAGE)) {
                /* Optional 'error-app-tag' field is present */
                res = yang_consume_strclause( tkc, mod, &err->error_message, 
                                              &emsg, appinfoQ );
            } else {
                res = ERR_NCX_WRONG_TKVAL;
                ncx_mod_exp_err(tkc, mod, res, expstr);         
            }
            if ( ( NO_ERR != res && res < ERR_LAST_SYS_ERR ) || 
                 res == ERR_NCX_EOF ) {
                return res;
            }
            break;  /* YANG clause assumed */

        default:
            res = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            break;
        }
    }

    return res;

}  /* yang_consume_error_stmts */


/********************************************************************
* FUNCTION yang_consume_descr
* 
* consume one descriptive string clause
* Parse the description or reference statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_descr (tk_chain_t  *tkc,
                        ncx_module_t *mod,
                        xmlChar **str,
                        boolean *dupflag,
                        dlq_hdr_t *appinfoQ)
{
    status_t    res, retres;
    boolean     save;

#ifdef DEBUG
    if (!tkc) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    save = TRUE;
    res = NO_ERR;
    retres = NO_ERR;

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the string value */
    if (ncx_save_descr() && str && save) {
        res = yang_consume_string(tkc, mod, str);
    } else {
        res = yang_consume_string(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

} /* yang_consume_descr */


/********************************************************************
* FUNCTION yang_consume_pid
* 
* consume one [prefix:]name clause
* Parse the rest of the statement (2 forms):
*
*        keyword [prefix:]name;
*
*        keyword [prefix:]name { appinfo }
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   prefixstr == prefix string to set  (may be NULL)
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_pid (tk_chain_t  *tkc,
                      ncx_module_t *mod,
                      xmlChar **prefixstr,
                      xmlChar **str,
                      boolean *dupflag,
                      dlq_hdr_t *appinfoQ)
{
    status_t    res, retres;
    boolean     save;

#ifdef DEBUG
    if (!tkc) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    save = TRUE;
    res = NO_ERR;
    retres = NO_ERR;

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the PID string value */
    res = yang_consume_pid_string(tkc, mod, prefixstr, str);
    CHK_EXIT(res, retres);

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

} /* yang_consume_pid */


/********************************************************************
* FUNCTION yang_consume_strclause
* 
* consume one normative string clause
* Parse the string-parameter-based statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_strclause (tk_chain_t  *tkc,
                            ncx_module_t *mod,
                            xmlChar **str,
                            boolean *dupflag,
                            dlq_hdr_t *appinfoQ)
{
    status_t    res, retres;
    boolean     save;

    save = TRUE;
    res = NO_ERR;
    retres = NO_ERR;

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the string value */
    if (str && save) {
        res = yang_consume_string(tkc, mod, str);
    } else {
        res = yang_consume_string(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_strclause */


/********************************************************************
* FUNCTION yang_consume_status
* 
* consume one status clause
* Parse the status statement
*
* Current token is the 'status' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   stat == status object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *stat set to the value if stat not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_status (tk_chain_t  *tkc,
                         ncx_module_t *mod,
                         ncx_status_t *status,
                         boolean *dupflag,
                         dlq_hdr_t *appinfoQ)
{
    xmlChar       *str;
    const char    *expstr;
    ncx_status_t   stat2;
    status_t       res, retres;
    boolean        save;

    expstr = "status enumeration string";
    retres = NO_ERR;
    save = TRUE;

    if (dupflag) {
        if (*dupflag) {
            res = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, res);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the string value */
    res = yang_consume_string(tkc, mod, &str);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            if (str) {
                m__free(str);
            }
            return res;
        }
    }

    if (str) {
        if (status && save) {
            *status = ncx_get_status_enum(str);
            stat2 = *status;
        } else {
            stat2 = ncx_get_status_enum(str);
        }

        if (save && stat2 == NCX_STATUS_NONE) {
            retres = ERR_NCX_INVALID_VALUE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
        m__free(str);
    }

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_status */


/********************************************************************
* FUNCTION yang_consume_ordered_by
* 
* consume one ordered-by clause
* Parse the ordered-by statement
*
* Current token is the 'ordered-by' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   ordsys == ordered-by object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *ordsys set to the value if not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_ordered_by (tk_chain_t  *tkc,
                             ncx_module_t *mod,
                             boolean *ordsys,
                             boolean *dupflag,
                             dlq_hdr_t *appinfoQ)
{
    xmlChar       *str;
    const char    *expstr;
    boolean        ordsys2;
    status_t       res, retres;
    boolean        save;

    expstr = "system or user keyword";
    retres = NO_ERR;
    save = TRUE;
    str = NULL;
    ordsys2 = FALSE;

    if (dupflag) {
        if (*dupflag) {
            res = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, res);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the string value */
    res = yang_consume_string(tkc, mod, &str);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            if (str) {
                m__free(str);
            }
            return res;
        }
    }

    if (str) {
        if (!xml_strcmp(str, YANG_K_USER)) {
            ordsys2 = FALSE;
        } else if (!xml_strcmp(str, YANG_K_SYSTEM)) {
            ordsys2 = TRUE;
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }

        if (ordsys && save) {
            *ordsys = ordsys2;
        }

        m__free(str);
    }

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_ordered_by */


/********************************************************************
* FUNCTION yang_consume_max_elements
* 
* consume one max-elements clause
* Parse the max-elements statement
*
* Current token is the 'max-elements' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   maxelems == max-elements object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *maxelems set to the value if not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_max_elements (tk_chain_t  *tkc,
                               ncx_module_t *mod,
                               uint32 *maxelems,
                               boolean *dupflag,
                               dlq_hdr_t *appinfoQ)
{
    xmlChar       *str;
    const xmlChar *nextval;
    tk_type_t      nexttk;
    status_t       res;

    res = NO_ERR;
    nexttk = tk_next_typ(tkc);
    nextval = tk_next_val(tkc);

    if (nexttk==TK_TT_DNUM) {
        res = yang_consume_uint32(tkc, mod, maxelems,
                                  dupflag, appinfoQ);
    } else if (TK_TYP_STR(nexttk)) {
        str = NULL;
        if (!xml_strcmp(nextval, YANG_K_UNBOUNDED)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &str,
                                         dupflag, 
                                         appinfoQ);
            if (str) {
                m__free(str);
                str = NULL;
            }
            if (maxelems) {
                *maxelems = 0;
            }
        } else {
            /* may be a quoted number or an error */
            res = yang_consume_uint32(tkc, 
                                      mod, 
                                      maxelems,
                                      dupflag, 
                                      appinfoQ);
        }
    }               

    return res;

}  /* yang_consume_max_elements */


/********************************************************************
* FUNCTION yang_consume_must
* 
* consume one must-stmt into mustQ
* Parse the must statement
*
* Current token is the 'must' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   mustQ  == address of Q of xpath_pcb_t structs to store
*             a new malloced xpath_pcb_t 
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   must entry malloced and  added to mustQ (if NO_ERR)
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    yang_consume_must (tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       dlq_hdr_t *mustQ,
                       dlq_hdr_t *appinfoQ)
{
    const xmlChar *val;
    const char    *expstr;
    xpath_pcb_t   *must;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, etag, emsg, desc, ref;
    
#ifdef DEBUG
    if (!tkc || !mustQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    must = xpath_new_pcb(NULL, NULL);
    if (!must) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    ncx_set_error(&must->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    retres = NO_ERR;
    expstr = "Xpath expression string";
    done = FALSE;
    etag = FALSE;
    emsg = FALSE;
    desc = FALSE;
    ref = FALSE;

    /* get the Xpath string for the must expression */
    res = yang_consume_string(tkc, mod, &must->exprstr);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            xpath_free_pcb(must);
            return retres;
        }
    }

    /* parse the must expression for well-formed XPath */
    res = xpath1_parse_expr(tkc, mod, must, XP_SRC_YANG);
    if (res != NO_ERR) {
        /* errors already reported */
        retres = res;
        if (NEED_EXIT(res)) {
            xpath_free_pcb(must);
            return retres;
        }
    }

    /* move on to the must sub-clauses, if any */
    expstr = "error-message, error-app-tag, "
        "description, or reference keywords";

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        xpath_free_pcb(must);
        return res;
    }

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        dlq_enque(must, mustQ);
        return retres;
    case TK_TT_LBRACE:
        break;
    default:
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        xpath_free_pcb(must);
        return res;
    }

    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            xpath_free_pcb(must);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            xpath_free_pcb(must);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    xpath_free_pcb(must);
                    return retres;
                }
            }
            continue;
        case TK_TT_RBRACE:
            dlq_enque(must, mustQ);
            return retres;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            xpath_free_pcb(must);
            done = TRUE;
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_ERROR_APP_TAG)) {
            /* Optional 'error-app-tag' field is present */
            res = yang_consume_strclause(tkc, 
                                         mod,
                                         &must->errinfo.error_app_tag,
                                         &etag, 
                                         appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_ERROR_MESSAGE)) {
            /* Optional 'error-message' field is present */
            res = yang_consume_strclause(tkc, 
                                         mod,
                                         &must->errinfo.error_message,
                                         &emsg, 
                                         appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            /* Optional 'description' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &must->errinfo.descr,
                                     &desc, 
                                     appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            /* Optional 'reference' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &must->errinfo.ref,
                                     &ref, 
                                     appinfoQ);
        } else {
            expstr = "must sub-statement";
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                xpath_free_pcb(must);
                done = TRUE;
            }
        }
    }

    return retres;

}  /* yang_consume_must */


/********************************************************************
* FUNCTION yang_consume_when
* 
* consume one when-stmt into obj->when
* Parse the when statement
*
* Current token is the 'when' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   obj    == obj_template_t of the parent object of this 'when'
*   whenflag == address of boolean set-once flag for the when-stmt
*               an error will be generated if the value passed
*               in is TRUE.  Set the initial value to FALSE
*               before first call, and do not change it after that
* OUTPUTS:
*   obj->when malloced and dilled in
*   *whenflag set if not set already
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    yang_consume_when (tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       obj_template_t *obj,
                       boolean        *whenflag)
{
    const xmlChar *val;
    const char    *expstr;
    xpath_pcb_t   *when;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, desc, ref;

#ifdef DEBUG
    if (!tkc || !mod || !obj) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;

    if (whenflag) {
        if (*whenflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
        } else {
            *whenflag = TRUE;
        }
    }

    when = xpath_new_pcb(NULL, NULL);
    if (!when) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    ncx_set_error(&when->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    expstr = "Xpath expression string";
    done = FALSE;
    desc = FALSE;
    ref = FALSE;

    /* pass off 'when' memory here */
    obj->when = when;

    /* get the Xpath string for the when expression */
    res = yang_consume_string(tkc, mod, &when->exprstr);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            return retres;
        }
    }

    /* parse the must expression for well-formed XPath */
    res = xpath1_parse_expr(tkc, mod, when, XP_SRC_YANG);
    if (res != NO_ERR) {
        /* errors already reported */
        retres = res;
        if (NEED_EXIT(res)) {
            return retres;
        }
    }

    /* move on to the must sub-clauses, if any */
    expstr = "description or reference keywords";

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        return retres;
    case TK_TT_LBRACE:
        break;
    default:
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        return res;
    }

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
            res = ncx_consume_appinfo(tkc, 
                                      mod, 
                                      &obj->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    return retres;
                }
            }
            continue;
        case TK_TT_RBRACE:
            return retres;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            retres = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            done = TRUE;
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            /* Optional 'description' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &when->errinfo.descr,
                                     &desc, 
                                     &obj->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            /* Optional 'reference' field is present */
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &when->errinfo.ref,
                                     &ref, 
                                     &obj->appinfoQ);
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                done = TRUE;
            }
        }
    }

    return retres;

}  /* yang_consume_when */


/********************************************************************
* FUNCTION yang_consume_iffeature
* 
* consume one if-feature-stmt into iffeatureQ
* Parse the if-feature statement
*
* Current token is the 'if-feature' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   iffeatureQ  == Q of ncx_iffeature_t to hold the new if-feature
*   appinfoQ  == queue to store appinfo (extension usage)
*
* OUTPUTS:
*   ncx_iffeature_t malloced and added to iffeatureQ
*   maybe ncx_appinfo_t structs added to appinfoQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    yang_consume_iffeature (tk_chain_t *tkc,
                            ncx_module_t *mod,
                            dlq_hdr_t *iffeatureQ,
                            dlq_hdr_t *appinfoQ)
{
    ncx_iffeature_t  *iff;
    xmlChar          *prefix, *name;
    status_t          res, res2;
    
#ifdef DEBUG
    if (!tkc || !mod || !iffeatureQ || !appinfoQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    prefix = NULL;
    name = NULL;

    res = yang_consume_pid_string(tkc, mod, &prefix, &name);
    if (res == NO_ERR) {
        /* check if this if if-feature already entered warning */
        iff = ncx_find_iffeature(iffeatureQ,
                                 prefix, 
                                 name,
                                 mod->prefix);
        if (iff) {
            if (ncx_warning_enabled(ERR_NCX_DUP_IF_FEATURE)) {
                log_warn("\nWarning: if-feature '%s%s%s' "
                         "already specified on line %u",
                         (prefix) ? prefix : EMPTY_STRING,
                         (prefix) ? ":" :  "", 
                         name,
                         iff->tkerr.linenum);
                ncx_print_errormsg(tkc, mod, ERR_NCX_DUP_IF_FEATURE);
            } else {
                ncx_inc_warnings(mod);
            }
            if (prefix) {
                m__free(prefix);
            }
            m__free(name);
        } else {
            /* not found so add it to the if-feature Q */
            iff = ncx_new_iffeature();
            if (!iff) {
                res = ERR_INTERNAL_MEM;
                ncx_print_errormsg(tkc, mod, res);
                if (prefix) {
                    m__free(prefix);
                }
                m__free(name);
            } else {
                /* transfer malloced fields */
                iff->prefix = prefix;
                iff->name = name;
                ncx_set_error(&iff->tkerr,
                              mod,
                              TK_CUR_LNUM(tkc),
                              TK_CUR_LPOS(tkc));
                dlq_enque(iff, iffeatureQ);
            }
        }
    }

    /* get the closing semi-colon even if previous error */
    res2 = yang_consume_semiapp(tkc, mod, appinfoQ);
    if (res == NO_ERR) {
        res = res2;
    }

    return res;

} /* yang_consume_iffeature */


/********************************************************************
* FUNCTION yang_consume_boolean
* 
* consume one boolean clause
* Parse the boolean parameter based statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   boolval == boolean value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_boolean (tk_chain_t  *tkc,
                          ncx_module_t *mod,
                          boolean *boolval,
                          boolean *dupflag,
                          dlq_hdr_t *appinfoQ)
{
    xmlChar    *str;
    const char *expstr;
    status_t    res, retres;
    boolean     save;

    retres = NO_ERR;
    expstr = "true or false keyword";
    str = NULL;
    save = TRUE;

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    /* get the string value */
    res = yang_consume_string(tkc, mod, &str);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            if (str) {
                m__free(str);
            }
            return res;
        }
    }

    if (str) {
        if (!xml_strcmp(str, NCX_EL_TRUE)) {
            if (save) {
                *boolval = TRUE;
            }
        } else if (!xml_strcmp(str, NCX_EL_FALSE)) {
            if (save) {
                *boolval = FALSE;
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
        m__free(str);
    }

    /* finish the clause */
    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_boolean */


/********************************************************************
* FUNCTION yang_consume_int32
* 
* consume one int32 clause
* Parse the int32 based parameter statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   num    == int32 value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *num set to the value if num not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_int32 (tk_chain_t  *tkc,
                        ncx_module_t *mod,
                        int32 *num,
                        boolean *dupflag,
                        dlq_hdr_t *appinfoQ)
{
    const char    *expstr;
    ncx_num_t      numstruct;
    status_t       res, retres;
    boolean        save;

    save = TRUE;
    retres = NO_ERR;
    expstr = "signed integer number";

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (TK_CUR_NUM(tkc) || TK_CUR_STR(tkc)) {
        res = ncx_convert_tkcnum(tkc, NCX_BT_INT32, &numstruct);
        if (res == NO_ERR) {
            if (num && save) {
                *num = numstruct.i;
            }
        } else {
            retres = res;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    } else {
        retres = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, retres, expstr);
    }

    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_int32 */


/********************************************************************
* FUNCTION yang_consume_uint32
* 
* consume one uint32 clause
* Parse the uint32 based parameter statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   num    == uint32 value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *num set to the value if num not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_consume_uint32 (tk_chain_t  *tkc,
                         ncx_module_t *mod,
                         uint32 *num,
                         boolean *dupflag,
                         dlq_hdr_t *appinfoQ)
{
    const char    *expstr;
    ncx_num_t      numstruct;
    status_t       res, retres;
    boolean        save;

    save = TRUE;
    retres = NO_ERR;
    expstr = "non-negative number";

    if (dupflag) {
        if (*dupflag) {
            retres = ERR_NCX_ENTRY_EXISTS;
            ncx_print_errormsg(tkc, mod, retres);
            save = FALSE;
        } else {
            *dupflag = TRUE;
        }
    }

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (TK_CUR_NUM(tkc) || TK_CUR_STR(tkc)) {
        res = ncx_convert_tkcnum(tkc, NCX_BT_UINT32, &numstruct);
        if (res == NO_ERR) {
            if (num && save) {
                *num = numstruct.u;
            }
        } else {
            retres = res;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    } else {
        retres = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, retres, expstr);
    }   

    if (save) {
        res = yang_consume_semiapp(tkc, mod, appinfoQ);
    } else {
        res = yang_consume_semiapp(tkc, mod, NULL);
    }
    CHK_EXIT(res, retres);

    return retres;

}  /* yang_consume_uint32 */


/********************************************************************
* FUNCTION yang_find_imp_typedef
* 
* Find the specified imported typedef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == type name to use
*   tkerr == error record to use in error messages (may be NULL)
*   typ  == address of return typ_template_t pointer
*
* OUTPUTS:
*   *typ set to the found typ_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_find_imp_typedef (yang_pcb_t *pcb,
                           tk_chain_t  *tkc,
                           ncx_module_t *mod,
                           const xmlChar *prefix,
                           const xmlChar *name,
                           ncx_error_t *tkerr,
                           typ_template_t **typ)
{
    ncx_import_t     *imp;
    ncx_node_t        dtyp;
    status_t          res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !prefix || !name || !typ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *typ = NULL;

    imp = ncx_find_pre_import(mod, prefix);
    if (!imp) {
        res = ERR_NCX_PREFIX_NOT_FOUND;
        log_error("\nError: import for prefix '%s' not found",
                  prefix);
    } else {
        /* found import OK, look up imported type definition */
        dtyp = NCX_NT_TYP;
        *typ = (typ_template_t *)
            ncx_locate_modqual_import(pcb, imp, name, &dtyp);
        if (!*typ) {
            res = ERR_NCX_DEF_NOT_FOUND;
            log_error("\nError: typedef definition for '%s:%s' not found"
                      " in module %s", 
                      prefix, 
                      name, 
                      imp->module);

        } else {
            return NO_ERR;
        }
    }

    tkc->curerr = tkerr;
    ncx_print_errormsg(tkc, mod, res);

    return res;

}  /* yang_find_imp_typedef */


/********************************************************************
* FUNCTION yang_find_imp_grouping
* 
* Find the specified imported grouping
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == grouping name to use
*   tkerr == error record to use in error messages (may be NULL)
*   grp  == address of return grp_template_t pointer
*
* OUTPUTS:
*   *grp set to the found grp_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_find_imp_grouping (yang_pcb_t *pcb,
                            tk_chain_t  *tkc,
                            ncx_module_t *mod,
                            const xmlChar *prefix,
                            const xmlChar *name,
                            ncx_error_t *tkerr,
                            grp_template_t **grp)
{
    ncx_import_t   *imp;
    ncx_node_t      dtyp;
    status_t        res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !prefix || !name || !grp) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *grp = NULL;

    imp = ncx_find_pre_import(mod, prefix);
    if (!imp) {
        res = ERR_NCX_PREFIX_NOT_FOUND;
        log_error("\nError: import for prefix '%s' not found",
                  prefix);
    } else {
        /* found import OK, look up imported type definition */
        dtyp = NCX_NT_GRP;
        *grp = (grp_template_t *)
            ncx_locate_modqual_import(pcb, imp, name, &dtyp);
        if (!*grp) {
            res = ERR_NCX_DEF_NOT_FOUND;
            log_error("\nError: grouping definition for '%s:%s' not found"
                      " in module %s", 
                      prefix, 
                      name, 
                      imp->module);
        } else {
            return NO_ERR;
        }
    }

    tkc->curerr = tkerr;
    ncx_print_errormsg(tkc, mod, res);

    return res;

}  /* yang_find_imp_grouping */


/********************************************************************
* FUNCTION yang_find_imp_extension
* 
* Find the specified imported extension
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == extension name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   ext  == address of return ext_template_t pointer
*
* OUTPUTS:
*   *ext set to the found ext_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_find_imp_extension (yang_pcb_t *pcb,
                             tk_chain_t  *tkc,
                             ncx_module_t *mod,
                             const xmlChar *prefix,
                             const xmlChar *name,
                             ncx_error_t *tkerr,
                             ext_template_t **ext)
{
    ncx_import_t   *imp;
    status_t        res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !prefix || !name || !ext) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *ext = NULL;

    imp = ncx_find_pre_import(mod, prefix);
    if (imp == NULL) {
        res = ERR_NCX_PREFIX_NOT_FOUND;
        log_error("\nError: import for prefix '%s' not found", prefix);
    } else if (imp->mod == NULL) {
        res = ncxmod_load_module(imp->module, 
                                 imp->revision,
                                 pcb->savedevQ,
                                 &imp->mod);
        if (res != NO_ERR) {
            log_error("\nError: failure importing module '%s'",
                      imp->module);
        }
    }
        
    /* found import OK, look up imported extension definition */
    if (imp != NULL && imp->mod != NULL) {
        *ext = ext_find_extension(imp->mod, name);
        if (*ext == NULL) {
            res = ERR_NCX_DEF_NOT_FOUND;
            log_error("\nError: extension definition for '%s:%s' not found"
                      " in module %s", 
                      prefix, 
                      name, 
                      imp->module);
        } else {
            return NO_ERR;
        }
    }

    tkc->curerr = tkerr;
    ncx_print_errormsg(tkc, mod, res);

    return res;

}  /* yang_find_imp_extension */


/********************************************************************
* FUNCTION yang_find_imp_feature
* 
* Find the specified imported feature
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == feature name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   feature  == address of return ncx_feature_t pointer
*
* OUTPUTS:
*   *feature set to the found ncx_feature_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_find_imp_feature (yang_pcb_t *pcb,
                           tk_chain_t  *tkc,
                           ncx_module_t *mod,
                           const xmlChar *prefix,
                           const xmlChar *name,
                           ncx_error_t *tkerr,
                           ncx_feature_t **feature)
{
    ncx_import_t   *imp;
    status_t        res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !prefix || !name || !feature) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *feature = NULL;

    imp = ncx_find_pre_import(mod, prefix);
    if (imp == NULL) {
        res = ERR_NCX_PREFIX_NOT_FOUND;
        log_error("\nError: import for prefix '%s' not found", prefix);
    } else if (imp->mod == NULL) {
        res = ncxmod_load_module(imp->module,
                                 imp->revision,
                                 pcb->savedevQ,
                                 &imp->mod);
        if (imp->mod == NULL) {
            log_error("\nError: failure importing module '%s'",
                      imp->module);
            res = ERR_NCX_DEF_NOT_FOUND;
        }
    }

    /* found import OK, look up imported extension definition */
    if (imp != NULL && imp->mod != NULL) {
        *feature = ncx_find_feature(imp->mod, name);
        if (*feature == NULL) {
            res = ERR_NCX_DEF_NOT_FOUND;
            log_error("\nError: feature definition for '%s:%s' not found"
                      " in module %s", 
                      prefix, 
                      name, 
                      imp->module);
        } else {
            return NO_ERR;
        }
    }

    tkc->curerr = tkerr;
    ncx_print_errormsg(tkc, mod, res);

    return res;

}  /* yang_find_imp_feature */


/********************************************************************
* FUNCTION yang_find_imp_identity
* 
* Find the specified imported identity
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == feature name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   identity  == address of return ncx_identity_t pointer
*
* OUTPUTS:
*   *identity set to the found ncx_identity_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_find_imp_identity (yang_pcb_t *pcb,
                            tk_chain_t  *tkc,
                            ncx_module_t *mod,
                            const xmlChar *prefix,
                            const xmlChar *name,
                            ncx_error_t *tkerr,
                            ncx_identity_t **identity)
{
    ncx_import_t   *imp;
    status_t        res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !prefix || !name || !identity) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *identity = NULL;

    imp = ncx_find_pre_import(mod, prefix);
    if (imp == NULL) {
        res = ERR_NCX_PREFIX_NOT_FOUND;
        log_error("\nError: import for prefix '%s' not found", prefix);
    } else if (imp->mod == NULL) {
        res = ncxmod_load_module(imp->module,
                                 imp->revision,
                                 pcb->savedevQ,
                                 &imp->mod);
        if (imp->mod == NULL) {
            log_error("\nError: failure importing module '%s'",
                      imp->module);
            res = ERR_NCX_DEF_NOT_FOUND;
        }
    } 

    if (imp != NULL && imp->mod != NULL) {
        /* found import OK, look up imported extension definition */
        *identity = ncx_find_identity(imp->mod, name, FALSE);
        if (*identity == NULL) {
            res = ERR_NCX_DEF_NOT_FOUND;
            log_error("\nError: identity definition for '%s:%s' not found"
                      " in module %s", 
                      prefix, 
                      name, 
                      imp->module);
        } else {
            return NO_ERR;
        }
    }

    tkc->curerr = tkerr;
    ncx_print_errormsg(tkc, mod, res);

    return res;

}  /* yang_find_imp_identity */


/********************************************************************
* FUNCTION yang_check_obj_used
* 
* generate warnings if local typedefs/groupings not used
* Check if the local typedefs and groupings are actually used
* Generate warnings if not used
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain in progress
*   mod == module in progress
*   typeQ == typedef Q to check
*   grpQ == pgrouping Q to check
*
*********************************************************************/
void
    yang_check_obj_used (tk_chain_t *tkc,
                         ncx_module_t *mod,
                         dlq_hdr_t *typeQ,
                         dlq_hdr_t *grpQ)
{
    typ_template_t *testtyp;
    grp_template_t *testgrp;

#ifdef DEBUG
    if (!tkc || !mod || !typeQ || !grpQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    for (testtyp = (typ_template_t *)dlq_firstEntry(typeQ);
         testtyp != NULL;
         testtyp = (typ_template_t *)dlq_nextEntry(testtyp)) {
        if (!testtyp->used) {
            if (ncx_warning_enabled(ERR_NCX_TYPDEF_NOT_USED)) {
                log_warn("\nWarning: Local typedef '%s' not used",
                         testtyp->name);
                tkc->curerr = &testtyp->tkerr;
                ncx_print_errormsg(tkc, mod, ERR_NCX_TYPDEF_NOT_USED);
            } else {
                ncx_inc_warnings(mod);
            }
        }
    }
    for (testgrp = (grp_template_t *)dlq_firstEntry(grpQ);
         testgrp != NULL;
         testgrp = (grp_template_t *)dlq_nextEntry(testgrp)) {
        if (!testgrp->used) {
            if (ncx_warning_enabled(ERR_NCX_GRPDEF_NOT_USED)) {
                log_warn("\nWarning: Local grouping '%s' not used",
                         testgrp->name);
                tkc->curerr = &testgrp->tkerr;
                ncx_print_errormsg(tkc, mod, ERR_NCX_GRPDEF_NOT_USED);
            } else {
                ncx_inc_warnings(mod);
            }
        }
    }
} /* yang_check_obj_used */


/********************************************************************
* FUNCTION yang_check_imports_used
* 
* generate warnings if imports not used
* Check if the imports statements are actually used
* Check if the  import is newer than the importing module
* Generate warnings if so
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain in progress
*   mod == module in progress
*
*********************************************************************/
void
    yang_check_imports_used (tk_chain_t *tkc,
                             ncx_module_t *mod)
{
    ncx_import_t  *testimp;
    ncx_module_t  *impmod;
    int            ret;

#ifdef DEBUG
    if (!tkc || !mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    impmod = NULL;

    for (testimp = (ncx_import_t *)dlq_firstEntry(&mod->importQ);
         testimp != NULL;
         testimp = (ncx_import_t *)dlq_nextEntry(testimp)) {

        if (!testimp->used) {
            if (ncx_warning_enabled(ERR_NCX_IMPORT_NOT_USED)) {
                log_warn("\nWarning: Module '%s' not used", 
                         testimp->module);
                tkc->curerr = &testimp->tkerr;
                ncx_print_errormsg(tkc, mod, ERR_NCX_IMPORT_NOT_USED);
            } else {
                ncx_inc_warnings(mod);
            }

        }

        /* find the imported module */
        if (testimp->mod == NULL) {
            impmod = ncx_find_module(testimp->module,
                                     testimp->revision);

            /* save the import back-ptr since it is not already set */
            testimp->mod = impmod;
        }

        /* check if the import is newer than this file,
         * skip yuma-netconf hack 
         */
        if (!testimp->force_yuma_nc && 
            impmod && 
            impmod->version && 
            mod->version) {

            ret = yang_compare_revision_dates(impmod->version,
                                              mod->version);
            if (ret > 0 && LOGDEBUG2) {
                log_debug2("\nNote: imported module '%s' (%s)"
                           " is newer than '%s' (%s)",
                           impmod->name, 
                           (impmod->version) 
                           ? impmod->version : EMPTY_STRING,
                           mod->name,
                           (mod->version) 
                           ? mod->version : EMPTY_STRING);
            }
        }
    }
} /* yang_check_imports_used */


/********************************************************************
* FUNCTION yang_new_node
* 
* Create a new YANG parser node
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_node_t *
    yang_new_node (void)
{
    yang_node_t *node;

    node = m__getObj(yang_node_t);
    if (!node) {
        return NULL;
    }

#ifdef YANG_DEBUG
    new_node_cnt++;
    if (LOGDEBUG4) {
        log_debug4("\nyang_new_node: %p (%u)", node, new_node_cnt);
    }
#endif

    memset(node, 0x0, sizeof(yang_node_t));
    return node;

} /* yang_new_node */


/********************************************************************
* FUNCTION yang_free_node
* 
* Delete a YANG parser node
*
* INPUTS:
*  node == parser node to delete
*
*********************************************************************/
void
    yang_free_node (yang_node_t *node)
{
#ifdef DEBUG
    if (!node) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

#ifdef YANG_DEBUG
    free_node_cnt++;
    if (LOGDEBUG4) {
        log_debug4("\nyang_free_node: %p (%u)", node, free_node_cnt);
        if (node->submod && node->submod->name) {
            log_debug4(" %s", node->submod->name);
        }
    }
#endif

    if (node->submod) {
        ncx_free_module(node->submod);
    }
    if (node->failed) {
        m__free(node->failed);
    }
    if (node->failedrev) {
        m__free(node->failedrev);
    }
    m__free(node);

}  /* yang_free_node */


/********************************************************************
* FUNCTION yang_clean_nodeQ
* 
* Delete all the node entries in a YANG parser node Q
*
* INPUTS:
*    que == Q of yang_node_t to clean
*
*********************************************************************/
void
    yang_clean_nodeQ (dlq_hdr_t *que)
{
    yang_node_t *node;

#ifdef DEBUG
    if (!que) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(que)) {
        node = (yang_node_t *)dlq_deque(que);
        yang_free_node(node);
    }

}  /* yang_clean_nodeQ */


/********************************************************************
* FUNCTION yang_find_node
* 
* Find a YANG parser node in the specified Q
*
* INPUTS:
*    que == Q of yang_node_t to check
*    name == module name to find
*    revision == module revision date (may be NULL)
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
yang_node_t *
    yang_find_node (const dlq_hdr_t *que,
                    const xmlChar *name,
                    const xmlChar *revision)
{
    yang_node_t *node;

#ifdef DEBUG
    if (!que || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (node = (yang_node_t *)dlq_firstEntry(que);
         node != NULL;
         node = (yang_node_t *)dlq_nextEntry(node)) {

        if (!xml_strcmp(node->name, name)) {
            if (!yang_compare_revision_dates(node->revision,
                                             revision)) {
                return node;
            }
        }
    }
    return NULL;

}  /* yang_find_node */


/********************************************************************
* FUNCTION yang_dump_nodeQ
* 
* log_debug output for contents of the specified nodeQ
*
* INPUTS:
*    que == Q of yang_node_t to check
*    name == Q name (may be NULL)
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
void
    yang_dump_nodeQ (dlq_hdr_t *que,
                     const char *name)
{
    yang_node_t *node;
    boolean      anyout;

#ifdef DEBUG
    if (!que) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!LOGDEBUG3) {
        return;
    }

    anyout = FALSE;
    if (name) {
        anyout = TRUE;
        log_debug3("\n%s Q:", name);
    }

    for (node = (yang_node_t *)dlq_firstEntry(que);
         node != NULL;
         node = (yang_node_t *)dlq_nextEntry(node)) {

        anyout = TRUE;
        log_debug3("\nNode %s ", node->name);

        if (node->res != NO_ERR) {
            log_debug3("res: %s ", get_error_string(node->res));
        }

        if (node->mod) {
            log_debug3("%smod:%s",
                      node->mod->ismod ? "" : "sub", node->mod->name);
        }
    }

    if (anyout) {
        log_debug3("\n");
    }

}  /* yang_dump_nodeQ */


/********************************************************************
* FUNCTION yang_new_pcb
* 
* Create a new YANG parser control block
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_pcb_t *
    yang_new_pcb (void)
{
    yang_pcb_t *pcb;

    pcb = m__getObj(yang_pcb_t);
    if (!pcb) {
        return NULL;
    }

    memset(pcb, 0x0, sizeof(yang_pcb_t));
    dlq_createSQue(&pcb->impchainQ);
    dlq_createSQue(&pcb->allimpQ);
    dlq_createSQue(&pcb->failedQ);
    /* needed for init load and imports for yangdump
     * do not do this for the agent, which will set 'save descr' 
     * mode to FALSE to indicate do not care about module display
     */
    pcb->stmtmode = ncx_save_descr(); 

    return pcb;

} /* yang_new_pcb */


/********************************************************************
* FUNCTION yang_free_pcb
* 
* Delete a YANG parser control block
*
* INPUTS:
*    pcb == parser control block to delete
*
*********************************************************************/
void
    yang_free_pcb (yang_pcb_t *pcb)
{
#ifdef DEBUG
    if (!pcb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (pcb->top && !pcb->topfound) {
        if (pcb->top->ismod) {
            if (!pcb->topadded) {
                if (pcb->searchmode || pcb->keepmode) {
                    /* need to get rid of the module, it is not wanted */
                    ncx_free_module(pcb->top);
                }
            }
        } else {
            ncx_free_module(pcb->top);
        }
    }

    yang_clean_import_ptrQ(&pcb->allimpQ);

    if (pcb->tkc) {
        tk_free_chain(pcb->tkc);
    }

    yang_clean_nodeQ(&pcb->impchainQ);
    yang_clean_nodeQ(&pcb->failedQ);
    m__free(pcb);

}  /* yang_free_pcb */


/********************************************************************
* FUNCTION yang_new_typ_stmt
* 
* Create a new YANG stmt node for a typedef
*
* INPUTS:
*   typ == type template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_typ_stmt (typ_template_t *typ)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_TYPEDEF;
    stmt->s.typ = typ;
    return stmt;

} /* yang_new_typ_stmt */


/********************************************************************
* FUNCTION yang_new_grp_stmt
* 
* Create a new YANG stmt node for a grouping
*
* INPUTS:
*   grp == grouping template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_grp_stmt (grp_template_t *grp)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!grp) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_GROUPING;
    stmt->s.grp = grp;
    return stmt;

} /* yang_new_grp_stmt */


/********************************************************************
* FUNCTION yang_new_ext_stmt
* 
* Create a new YANG stmt node for an extension
*
* INPUTS:
*   ext == extension template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_ext_stmt (ext_template_t *ext)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!ext) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_EXTENSION;
    stmt->s.ext = ext;
    return stmt;

} /* yang_new_ext_stmt */


/********************************************************************
* FUNCTION yang_new_obj_stmt
* 
* Create a new YANG stmt node for an object
*
* INPUTS:
*   obj == object template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_obj_stmt (obj_template_t *obj)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_OBJECT;
    stmt->s.obj = obj;
    return stmt;

} /* yang_new_obj_stmt */


/********************************************************************
* FUNCTION yang_new_id_stmt
* 
* Create a new YANG stmt node for an identity
*
* INPUTS:
*   identity == identity template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_id_stmt (ncx_identity_t *identity)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!identity) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_IDENTITY;
    stmt->s.identity = identity;
    return stmt;

} /* yang_new_id_stmt */


/********************************************************************
* FUNCTION yang_new_feature_stmt
* 
* Create a new YANG stmt node for a feature definition
*
* INPUTS:
*   feature == feature template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_feature_stmt (ncx_feature_t *feature)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!feature) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_FEATURE;
    stmt->s.feature = feature;
    return stmt;

} /* yang_new_feature_stmt */


/********************************************************************
* FUNCTION yang_new_deviation_stmt
* 
* Create a new YANG stmt node for a deviation definition
*
* INPUTS:
*   deviation == deviation template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_stmt_t *
    yang_new_deviation_stmt (obj_deviation_t *deviation)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!deviation) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    stmt = m__getObj(yang_stmt_t);
    if (!stmt) {
        return NULL;
    }
    memset(stmt, 0x0, sizeof(yang_stmt_t));
    stmt->stmttype = YANG_ST_DEVIATION;
    stmt->s.deviation = deviation;
    return stmt;

} /* yang_new_deviation_stmt */


/********************************************************************
* FUNCTION yang_free_stmt
* 
* Delete a YANG statement node
*
* INPUTS:
*    stmt == yang_stmt_t node to delete
*
*********************************************************************/
void
    yang_free_stmt (yang_stmt_t *stmt)
{
#ifdef DEBUG
    if (!stmt) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    m__free(stmt);

}  /* yang_free_stmt */


/********************************************************************
* FUNCTION yang_clean_stmtQ
* 
* Delete a Q of YANG statement node
*
* INPUTS:
*    que == Q of yang_stmt_t node to delete
*
*********************************************************************/
void
    yang_clean_stmtQ (dlq_hdr_t *que)
{
    yang_stmt_t *stmt;

#ifdef DEBUG
    if (!que) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    while (!dlq_empty(que)) {
        stmt = (yang_stmt_t *)dlq_deque(que);
        yang_free_stmt(stmt);
    }

}  /* yang_clean_stmtQ */


/********************************************************************
* FUNCTION yang_validate_date_string
* 
* Validate a YANG date string for a revision entry
*
* Expected format is:
*
*    YYYY-MM-DD
*
* Error messages are printed by this function
*
* INPUTS:
*    tkc == token chain in progress
*    mod == module in progress
*    tkerr == error struct to use (may be NULL to use tkc->cur)
*    datestr == string to validate
*
* RETURNS:
*   status
*********************************************************************/
status_t
    yang_validate_date_string (tk_chain_t *tkc,
                              ncx_module_t *mod,
                              ncx_error_t *tkerr,
                              const xmlChar *datestr)
{

#define DATE_STR_LEN 10

    status_t        res, retres;
    uint32          len, i;
    int             ret;
    ncx_num_t       num;
    xmlChar         numbuff[NCX_MAX_NUMLEN];
    xmlChar         curdate[16];

#ifdef DEBUG
    if (!tkc || !mod || !datestr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;
    len = xml_strlen(datestr);
    tstamp_date(curdate);

    /* validate the length */
    if (len != DATE_STR_LEN) {
        retres = ERR_NCX_INVALID_VALUE;
        log_error("\nError: Invalid date string length (%u)", len);
        tkc->curerr = tkerr;
        ncx_print_errormsg(tkc, mod, retres);
    }

    /* validate the year field */
    if (retres == NO_ERR) {
        for (i=0; i<4; i++) {
            numbuff[i] = datestr[i];
        }
        numbuff[4] = 0;
        res = ncx_decode_num(numbuff, NCX_BT_UINT32, &num);
        if (res != NO_ERR) {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid year string (%s)", numbuff);
            ncx_print_errormsg(tkc, mod, retres);
        } else if (num.u < 1970) {
            if (ncx_warning_enabled(ERR_NCX_DATE_PAST)) {
                log_warn("\nWarning: Invalid revision year (%s)", 
                         numbuff);
                tkc->curerr = tkerr;
                ncx_print_errormsg(tkc, mod, ERR_NCX_DATE_PAST);
            } else {
                ncx_inc_warnings(mod);
            }

        } 
    }

    /* validate the first separator */
    if (retres == NO_ERR) {
        if (datestr[4] != '-') {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid date string separator (%c)",
                      datestr[4]);
            tkc->curerr = tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        } 
    }

    /* validate the month field */
    if (retres == NO_ERR) {
        numbuff[0] = datestr[5];
        numbuff[1] = datestr[6];
        numbuff[2] = 0;
        res = ncx_convert_num(numbuff, 
                              NCX_NF_DEC, 
                              NCX_BT_UINT32, 
                              &num);
        if (res != NO_ERR) {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid month string (%s)", numbuff);
            ncx_print_errormsg(tkc, mod, retres);
        } else if (num.u < 1 || num.u > 12) {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid month string (%s)", numbuff);
            tkc->curerr = tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        } 
    }

    /* validate the last separator */
    if (retres == NO_ERR) {
        if (datestr[7] != '-') {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid date string separator (%c)",
                      datestr[7]);
            tkc->curerr = tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        } 
    }

    /* validate the day field */
    if (retres == NO_ERR) {
        numbuff[0] = datestr[8];
        numbuff[1] = datestr[9];
        numbuff[2] = 0;
        res = ncx_convert_num(numbuff, 
                              NCX_NF_DEC, 
                              NCX_BT_UINT32, 
                              &num);
        if (res != NO_ERR) {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid day string (%s)", numbuff);
            tkc->curerr = tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        } else if (num.u < 1 || num.u > 31) {
            retres = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Invalid day string (%s)", numbuff);
            tkc->curerr = tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }
    }

    /* check the date against the future */
    if (retres == NO_ERR) {
        ret = xml_strcmp(curdate, datestr);
        if (ret < 0) {
            if (ncx_warning_enabled(ERR_NCX_DATE_FUTURE)) {
                log_warn("\nWarning: Revision date in the future (%s)",
                         datestr);
                tkc->curerr = tkerr;
                ncx_print_errormsg(tkc, mod, ERR_NCX_DATE_FUTURE);
            } else {
                ncx_inc_warnings(mod);
            }

        }
    }

    return retres;

}  /* yang_validate_date_string */


/********************************************************************
* FUNCTION yang_skip_statement
* 
* Skip past the current invalid statement, starting at
* an invalid keyword
*
* INPUTS:
*    tkc == token chain in progress
*    mod == module in progress
*********************************************************************/
void
    yang_skip_statement (tk_chain_t *tkc,
                         ncx_module_t *mod)
{
    uint32    bracecnt;
    status_t  res;

#ifdef DEBUG
    if (!tkc || !tkc->cur) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    res = NO_ERR;
    bracecnt = 0;

    while (res==NO_ERR) {

        /* current should be value string or stmtend */
        switch (TK_CUR_TYP(tkc)) {
        case TK_TT_NONE:
            return;
        case TK_TT_SEMICOL:
            if (!bracecnt) {
                return;
            }
            break;
        case TK_TT_LBRACE:
            bracecnt++;
            break;
        case TK_TT_RBRACE:
            if (bracecnt) {
                bracecnt--;
                if (!bracecnt) {
                    return;
                }
            } else {
                return;
            }
        default:
            ;
        }

        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
        }
    }
    
}  /* yang_skip_statement */


/********************************************************************
* FUNCTION yang_top_keyword
* 
* Check if the string is a top-level YANG keyword
*
* INPUTS:
*    keyword == string to check
*
* RETURNS:
*   TRUE if a top-level YANG keyword, FALSE otherwise
*********************************************************************/
boolean
    yang_top_keyword (const xmlChar *keyword)
{
    const xmlChar *topkw;
    uint32         i;
    int            ret;

#ifdef DEBUG
    if (!keyword) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    for (i=0;;i++) {
        topkw = top_keywords[i];
        if (!topkw) {
            return FALSE;
        }
        ret = xml_strcmp(topkw, keyword);
        if (ret == 0) {
            return TRUE;
        } else if (ret > 0) {
            return FALSE;
        }
    }
    /*NOTREACHED*/

}  /* yang_top_keyword */


/********************************************************************
* FUNCTION yang_new_import_ptr
* 
* Create a new YANG import pointer node
*
* INPUTS:
*   modname = module name to set
*   modprefix = module prefix to set
*   revision = revision date to set
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
yang_import_ptr_t *
    yang_new_import_ptr (const xmlChar *modname,
                         const xmlChar *modprefix,
                         const xmlChar *revision)
{
    yang_import_ptr_t *impptr;

    impptr = m__getObj(yang_import_ptr_t);
    if (!impptr) {
        return NULL;
    }
    memset(impptr, 0x0, sizeof(yang_import_ptr_t));

    if (modname) {
        impptr->modname = xml_strdup(modname);
        if (!impptr->modname) {
            yang_free_import_ptr(impptr);
            return NULL;
        }
    }

    if (modprefix) {
        impptr->modprefix = xml_strdup(modprefix);
        if (!impptr->modprefix) {
            yang_free_import_ptr(impptr);
            return NULL;
        }
    }

    if (revision) {
        impptr->revision = xml_strdup(revision);
        if (!impptr->revision) {
            yang_free_import_ptr(impptr);
            return NULL;
        }
    }

    return impptr;

} /* yang_new_import_ptr */


/********************************************************************
* FUNCTION yang_free_impptr_ptr
* 
* Delete a YANG import pointer node
*
* INPUTS:
*  impptr == import pointer node to delete
*
*********************************************************************/
void
    yang_free_import_ptr (yang_import_ptr_t *impptr)
{
#ifdef DEBUG
    if (!impptr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (impptr->modname) {
        m__free(impptr->modname);
    }
    if (impptr->modprefix) {
        m__free(impptr->modprefix);
    }
    if (impptr->revision) {
        m__free(impptr->revision);
    }

    m__free(impptr);

}  /* yang_free_import_ptr */


/********************************************************************
* FUNCTION yang_clean_import_ptrQ
* 
* Delete all the node entries in a YANG import pointer node Q
*
* INPUTS:
*    que == Q of yang_import_ptr_t to clean
*
*********************************************************************/
void
    yang_clean_import_ptrQ (dlq_hdr_t *que)
{
    yang_import_ptr_t *impptr;

#ifdef DEBUG
    if (!que) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(que)) {
        impptr = (yang_import_ptr_t *)dlq_deque(que);
        yang_free_import_ptr(impptr);
    }

}  /* yang_clean_import_ptrQ */


/********************************************************************
* FUNCTION yang_find_import_ptr
* 
* Find a YANG import pointer node in the specified Q
*
* INPUTS:
*    que == Q of yang_import_ptr_t to check
*    name == module name to find
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
yang_import_ptr_t *
    yang_find_import_ptr (dlq_hdr_t *que,
                          const xmlChar *name)
{
    yang_import_ptr_t *impptr;

#ifdef DEBUG
    if (!que || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (impptr = (yang_import_ptr_t *)dlq_firstEntry(que);
         impptr != NULL;
         impptr = (yang_import_ptr_t *)dlq_nextEntry(impptr)) {

        if (!xml_strcmp(impptr->modname, name)) {
            return impptr;
        }
    }
    return NULL;

}  /* yang_find_import_ptr */


/********************************************************************
* FUNCTION yang_compare_revision_dates
* 
* Compare 2 revision strings, which either may be NULL
*
* INPUTS:
*   revstring1 == revision string 1 (may be NULL)
*   revstring2 == revision string 2 (may be NULL)
*
* RETURNS:
*    -1 if revision 1 < revision 2
*    0 of they are the same
*    +1 if revision1 > revision 2   
*********************************************************************/
int32
    yang_compare_revision_dates (const xmlChar *revstring1,
                                 const xmlChar *revstring2)
{

    /* if either revision is NULL then call it a match
     * else actually compare the revision date strings 
     */
    if (!revstring1 || !revstring2) {
        return 0;
    } else {
        return xml_strcmp(revstring1, revstring2);
    }

}  /* yang_compare_revision_dates */


/********************************************************************
* FUNCTION yang_make_filename
* 
* Malloc and construct a YANG filename 
*
* INPUTS:
*   modname == [sub]module name
*   revision == [sub]module revision date (may be NULL)
*   isyang == TRUE for YANG extension
*             FALSE for YIN extension
*
* RETURNS:
*    malloced and filled in string buffer with filename
*    NULL if any error
*********************************************************************/
xmlChar *
    yang_make_filename (const xmlChar *modname,
                        const xmlChar *revision,
                        boolean isyang)
{
    xmlChar    *buff, *p;
    uint32      mlen, rlen, slen;

#ifdef DEBUG
    if (!modname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    mlen = xml_strlen(modname);
    rlen = (revision) ? xml_strlen(revision) : 0;
    if (rlen) {
        rlen++;   /* file sep char */
    }
    slen = (isyang) ? xml_strlen(YANG_SUFFIX) :
        xml_strlen(YIN_SUFFIX);

    buff = m__getMem(mlen + rlen + slen + 2);
    if (!buff) {
        return NULL;
    }

    p = buff;

    p += xml_strcpy(p, modname);
    if (revision && *revision) {
        *p++ = YANG_FILE_SEPCHAR;
        p += xml_strcpy(p, revision);
    }
    *p++ = '.';
    if (isyang) {
        xml_strcpy(p, YANG_SUFFIX);
    } else {
        xml_strcpy(p, YIN_SUFFIX);
    }

    return buff;

}  /* yang_make_filename */


/********************************************************************
* FUNCTION yang_copy_filename
* 
* Construct a YANG filename into a provided buffer
*
* INPUTS:
*   modname == [sub]module name
*   revision == [sub]module revision date (may be NULL)
*   buffer == buffer to copy filename into
*   bufflen == number of bytes available in buffer
*   isyang == TRUE for YANG extension
*             FALSE for YIN extension
*
* RETURNS:
*    malloced and filled in string buffer with filename
*    NULL if any error
*********************************************************************/
status_t
    yang_copy_filename (const xmlChar *modname,
                        const xmlChar *revision,
                        xmlChar *buffer,
                        uint32 bufflen,
                        boolean isyang)
{
    xmlChar    *p;
    uint32      mlen, rlen, slen;

#ifdef DEBUG
    if (!modname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    mlen = xml_strlen(modname);
    rlen = (revision) ? xml_strlen(revision) : 0;
    if (rlen) {
        rlen++;   /* file sep char */
    }
    slen = (isyang) ? xml_strlen(YANG_SUFFIX) :
        xml_strlen(YIN_SUFFIX);

    if ((mlen + rlen + slen + 2) < bufflen) {
        p = buffer;
        p += xml_strcpy(p, modname);
        if (revision && *revision) {
            *p++ = YANG_FILE_SEPCHAR;
            p += xml_strcpy(p, revision);
        }
        *p++ = '.';
        if (isyang) {
            xml_strcpy(p, YANG_SUFFIX);
        } else {
            xml_strcpy(p, YIN_SUFFIX);
        }
        return NO_ERR;
    } else {
        return ERR_BUFF_OVFL;
    }

}  /* yang_copy_filename */


/********************************************************************
* FUNCTION yang_split_filename
* 
* Split a module parameter into its filename components
*
* INPUTS:
*   filename == filename string
*   modnamelen == address of return modname length
*
* OUTPUTS:
*   *modnamelen == module name length
*
* RETURNS:
*    TRUE if module@revision form was found
*    FALSE if not, and ignore the output because a
*      different form of the module parameter was used
*********************************************************************/
boolean
    yang_split_filename (const xmlChar *filename,
                         uint32 *modnamelen)
{
    const xmlChar *str, *sepchar;
    uint32         len, yangslen, yinslen;

#ifdef DEBUG
    if (filename == NULL || modnamelen == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *modnamelen = 0;

    str = filename;
    sepchar = NULL;

    /* check if special filespec first chars are present */
    if (*str == '$' || *str == '~') {
        return FALSE;
    }

    /* check if the revision separator is present
     * or if any path sep chars are present
     */
    while (*str) {
        if (*str == YANG_FILE_SEPCHAR) {
            sepchar = str;
        } else if (*str == NCXMOD_PSCHAR) {
            return FALSE;
        }
        str++;
    }
    if (sepchar == NULL) {
        return FALSE;
    }

    /* check if a file extension is present */
    len = (uint32)(str - filename);
    yangslen = xml_strlen(YANG_SUFFIX);
    yinslen = xml_strlen(YIN_SUFFIX);
    if (len > yangslen + 1) {
        if ((*(str - yangslen - 1) == '.') &&
            (!xml_strcmp(str - yangslen, YANG_SUFFIX))) {
            return FALSE;
        }
    } else if (len > yinslen + 1) {
        if ((*(str - yinslen - 1) == '.') &&
            (!xml_strcmp(str - yinslen, YIN_SUFFIX))) {
            return FALSE;
        }
    }

    *modnamelen = (uint32)(sepchar - filename);
    return TRUE;

}  /* yang_split_filename */


/********************************************************************
* FUNCTION yang_fileext_is_yang
* 
* Check if the filespec ends with the .yang extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .yang file extension found
*    FALSE if not
*********************************************************************/
boolean
    yang_fileext_is_yang (const xmlChar *filename)
{
    uint32   len;

#ifdef DEBUG
    if (filename == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    len = xml_strlen(filename);
    if (len < 6) {
        return FALSE;
    }

    if (filename[len - 5] != '.') {
        return FALSE;
    }

    return !xml_strcmp(&filename[len - 4], YANG_SUFFIX);

}  /* yang_fileext_is_yang */


/********************************************************************
* FUNCTION yang_fileext_is_yin
* 
* Check if the filespec ends with the .yin extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .yin file extension found
*    FALSE if not
*********************************************************************/
boolean
    yang_fileext_is_yin (const xmlChar *filename)
{
    uint32   len;

#ifdef DEBUG
    if (filename == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    len = xml_strlen(filename);
    if (len < 5) {
        return FALSE;
    }

    if (filename[len - 4] != '.') {
        return FALSE;
    }

    return !xml_strcmp(&filename[len - 3], YIN_SUFFIX);

}  /* yang_fileext_is_yin */


/********************************************************************
* FUNCTION yang_fileext_is_xml
* 
* Check if the filespec ends with the .xml extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .xml file extension found
*    FALSE if not
*********************************************************************/
boolean
    yang_fileext_is_xml (const xmlChar *filename)
{
    uint32   len;

#ifdef DEBUG
    if (filename == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    len = xml_strlen(filename);
    if (len < 5) {
        return FALSE;
    }

    if (filename[len - 4] != '.') {
        return FALSE;
    }

    return !xml_strcmp(&filename[len - 3], (const xmlChar *)"xml");

}  /* yang_fileext_is_xml */


/********************************************************************
* FUNCTION yang_final_memcheck
* 
* Check the node malloc and free counts
*********************************************************************/
void
    yang_final_memcheck (void)
{
#ifdef YANG_DEBUG
    if (new_node_cnt != free_node_cnt) {
        log_error("\nError: YANG node count: new(%u) free(%u)",
                  new_node_cnt,
                  free_node_cnt);
    }
#endif
}  /* yang_final_memcheck */



/* END file yang.c */
