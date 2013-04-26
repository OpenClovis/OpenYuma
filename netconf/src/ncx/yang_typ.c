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
/*  FILE: yang_typ.c


   YANG module parser, typedef and type statement support

    Data type conversion

    Current NCX base type   YANG builtin type

    NCX_BT_ANY              anyxml
    NCX_BT_BITS             bits
    NCX_BT_ENUM             enumeration
    NCX_BT_EMPTY            empty
    NCX_BT_INT8             int8
    NCX_BT_INT32            int16
    NCX_BT_INT32            int32
    NCX_BT_INT32            int64
    NCX_BT_UINT8            uint8
    NCX_BT_UINT32           uint16
    NCX_BT_UINT32           uint32
    NCX_BT_UINT64           uint64
    NCX_BT_DECIMAL64        decimal64
    NCX_BT_FLOAT64          float64
    NCX_BT_STRING           string
    NCX_BT_USTRING          binary
    NCX_BT_UNION            union
    NCX_BT_SLIST            N/A
    NCX_BT_CONTAINER        container ** Not a type **
    NCX_BT_CHOICE           choice ** Not a type **
    NCX_BT_LIST             list ** Not a type **
    NCX_BT_LEAFREF          leafref
    NCX_BT_INSTANCE_ID      instance-identifier

    NCX_BT_CHOICE           meta-type for YANG choice
    NCX_BT_CASE             meta-type for YANG case

    sim-typ w/ '*' iqual    leaf-list


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
26oct07      abb      begun; start from ncx_parse.c
15nov07      abb      split out from yang_parse.c


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <ctype.h>

#include <xmlstring.h>

#include "procdefs.h"
#include "def_reg.h"
#include "dlq.h"
#include "log.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "ncx.h"
#include "ncx_appinfo.h"
#include "ncx_num.h"
#include "obj.h"
#include "status.h"
#include "typ.h"
#include "xml_util.h"
#include "xpath.h"
#include "xpath_yang.h"
#include "yangconst.h"
#include "yang.h"
#include "yang_typ.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
/* #define YANG_TYP_DEBUG 1 */
/* #define YANG_TYP_TK_DEBUG 1 */
#endif


static status_t 
    resolve_type (yang_pcb_t *pcb,
                  tk_chain_t *tkc,
                  ncx_module_t *mod,
                  typ_def_t *typdef,
                  const xmlChar *name,
                  const xmlChar *defval,
                  obj_template_t *obj,
                  grp_template_t *grp);


/********************************************************************
* FUNCTION loop_test
* 
* Check for named type dependency loops
* Called during phase 2 of module parsing
*
* INPUTS:
*   typdef == typdef in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    loop_test (typ_def_t *typdef)
{
    typ_def_t *testdef, *lastdef;
    status_t   res;

    res = NO_ERR;

    if (typdef->tclass == NCX_CL_NAMED) {
        testdef = typ_get_parent_typdef(typdef);
        lastdef = testdef;
        while (testdef && res==NO_ERR) {
            if (testdef == typdef) {
                res = ERR_NCX_DEF_LOOP;
                log_error("\nError: named type loops with "
                          "type '%s' on line %u",
                          testdef->typenamestr,
                          lastdef->tkerr.linenum);
            } else {
                lastdef = testdef;    
                testdef = typ_get_parent_typdef(testdef);
            }
        }
    }

    return res;

}   /* loop_test */


/********************************************************************
* FUNCTION one_restriction_test
* 
* Check 1 restriction Q
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   newdef == new typdef in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    one_restriction_test (tk_chain_t *tkc,
                      ncx_module_t *mod,
                      typ_def_t *newdef)
{
    dlq_hdr_t      *rangeQ;
    typ_pattern_t  *pat;
    status_t        retres;
    ncx_btype_t     btyp;
    boolean         doerr;
    ncx_strrest_t   strrest;

    retres = NO_ERR;
    doerr = FALSE;

    btyp = typ_get_basetype(newdef);
    if (btyp == NCX_BT_UNION) {
        return NO_ERR;
    }

    rangeQ = typ_get_rangeQ_con(newdef);
    strrest = typ_get_strrest(newdef);
    
    /* check if proper base type restrictions
     * are present. Range allowed for numbers
     * and strings only
     */
    if (rangeQ && !dlq_empty(rangeQ)) {
        if (!(typ_is_number(btyp) || 
              typ_is_string(btyp) || btyp==NCX_BT_BINARY)) {
            log_error("\nError: Range or length not "
                          "allowed for the %s builtin type",
                      tk_get_btype_sym(btyp));
            retres = ERR_NCX_RESTRICT_NOT_ALLOWED;
            tkc->curerr = &newdef->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }
    }

    /* Check that a pattern was actually entered
     * Check Pattern allowed for NCX_BT_STRING only
     */
    pat = typ_get_first_pattern(newdef);
    if (pat) {
        if (btyp != NCX_BT_STRING) {
            log_error("\nError: keyword 'pattern' "
                      "within a restriction for a %s type",
                      tk_get_btype_sym(btyp));
            doerr = TRUE;
        }
    } else {
        switch (strrest) {
        case NCX_SR_ENUM:
            if (btyp != NCX_BT_ENUM) {
                log_error("\nError: keyword 'enumeration' "
                          "within a restriction for a %s type",
                          tk_get_btype_sym(btyp));
                doerr = TRUE;
            }
            break;
        case NCX_SR_BIT:
            if (btyp != NCX_BT_BITS) {
                log_error("\nError: keyword 'bit' "
                          "within a restriction for a %s type",
                          tk_get_btype_sym(btyp));
                doerr = TRUE;
            }
            break;
        default:
            ;
        }
    }

    if (doerr) {
        retres = ERR_NCX_RESTRICT_NOT_ALLOWED;
        tkc->curerr = &newdef->tkerr;
        ncx_print_errormsg(tkc, mod, retres);
    }

    return retres;

}   /* one_restriction_test */


/********************************************************************
* FUNCTION restriction_test
* 
* Check for proper restrictions to a data type definition
* Called during phase 2 of module parsing
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typdef in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    restriction_test (tk_chain_t *tkc,
                      ncx_module_t *mod,
                      typ_def_t *typdef)
{
    typ_def_t    *testdef, *newdef;
    ncx_btype_t   btyp, testbtyp;
    status_t      res, retres;


    if (typdef->tclass == NCX_CL_SIMPLE) {
        return one_restriction_test(tkc, mod, typdef);
    } else if (typdef->tclass != NCX_CL_NAMED) {
        return NO_ERR;
    }

    res = NO_ERR;
    retres = NO_ERR;
    btyp = typ_get_basetype(typdef);


    /* if there is a 'newtyp', then restrictions were
     * added; If NULL, then just a type name was given
     */
    testdef = typdef;
    while (testdef && testdef->tclass==NCX_CL_NAMED) {
        newdef = typ_get_new_named(testdef);
        if (!newdef) {
            testdef = typ_get_parent_typdef(testdef);
            continue;
        }

        /* validate the 'newdef' typdef */
        testbtyp = typ_get_basetype(newdef);
        if (testbtyp==NCX_BT_NONE) {
            newdef->def.simple.btyp = btyp;
            res = one_restriction_test(tkc, mod, newdef);
            CHK_EXIT(res, retres);
        } else if (testbtyp != btyp) {
            log_error("\nError: Derived type '%s' does not match "
                      "the eventual builtin type (%s)",
                      tk_get_btype_sym(testbtyp),
                      tk_get_btype_sym(btyp));
            retres = ERR_NCX_WRONG_DATATYP;
            tkc->curerr = &typdef->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }

        /* setup the next (parent) typdef in the chain */
        testdef = typ_get_parent_typdef(testdef);
    }

    return retres;

}   /* restriction_test */


/********************************************************************
* FUNCTION consume_yang_rangedef
* 
* Current token is the start of range fragment
* Process the token chain and gather the range fragment
* in a pre-allocated typ_rangedef_t struct
*
* Normal exit with current token as the next one to
* be processed.
*
* Cloned from consume_rangedef in ncx_parse.c because
* the YANG range clause is different than NCX
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typdef in progress
*   btyp == base type of range vals
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    consume_yang_rangedef (tk_chain_t *tkc,
                           ncx_module_t *mod,
                           typ_def_t *typdef,
                           ncx_btype_t btyp)
{
    typ_rangedef_t  *rv;
    const char      *expstr;
    dlq_hdr_t       *rangeQ;
    status_t         res, retres;
    boolean          done;
    int32            cmpval;

    expstr = "number or min, max, -INF, INF keywords";
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    rangeQ = typ_get_rangeQ_con(typdef);
    if (!rangeQ) {
        res = SET_ERROR(ERR_NCX_INVALID_VALUE);
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* get a new range value struct */
    rv = typ_new_rangedef();
    if (!rv) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* save the range builtin type */
    rv->btyp = btyp;

    /* get the first range-boundary, check if min requested */
    if (TK_CUR_STR(tkc)) {
        if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_MIN)) {
            /* flag lower bound min for later eval */
            rv->flags |= TYP_FL_LBMIN;
        } else if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_MAX)) {
            /* flag lower bound max for later eval */
            rv->flags |= TYP_FL_LBMAX;
        } else if (btyp==NCX_BT_FLOAT64) {
            /* -INF and INF keywords allowed for real numbers */
            if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_NEGINF)) {
                /* flag lower bound -INF for later eval */
                rv->flags |= TYP_FL_LBINF;
            } else if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_POSINF)) {
                /* flag lower bound INF for later eval */
                rv->flags |= TYP_FL_LBINF2;
            } else {
                res = ERR_NCX_WRONG_TKVAL;
            }
        } else {
            res = ERR_NCX_WRONG_TKVAL;
        }
    } else if (TK_CUR_NUM(tkc)) {
        if (btyp != NCX_BT_NONE) {
            if (btyp == NCX_BT_DECIMAL64) {
                res = ncx_convert_tkc_dec64(tkc, 
                                            typ_get_fraction_digits(typdef), 
                                            &rv->lb);
            } else {
                res = ncx_convert_tkcnum(tkc, btyp, &rv->lb);
            }
        } else {
            rv->lbstr = xml_strdup(TK_CUR_VAL(tkc));
            if (!rv->lbstr) {
                res = ERR_INTERNAL_MEM;
            }
        }
    } else {
        res = ERR_NCX_WRONG_TKTYPE;
    }

    /* record any error so far */
    if (res != NO_ERR) {
        retres = res;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        if (NEED_EXIT(res)) {
            typ_free_rangedef(rv, btyp);
            return retres;
        }
    }

    /* move past lower range-boundary */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        typ_free_rangedef(rv, btyp);
        return res;
    }

    /* check if done with this range part */
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
    case TK_TT_LBRACE:
    case TK_TT_BAR:
        /* normal end of range reached 
         * lower bound is the upper bound also
         */
        if (rv->flags & TYP_FL_LBMIN) {
            rv->flags |= TYP_FL_UBMIN;
        } else if (rv->flags & TYP_FL_LBMAX) {
            rv->flags |= TYP_FL_UBMAX;
        } else if (rv->flags & TYP_FL_LBINF) {
            rv->flags |= TYP_FL_UBINF2;
        } else if (rv->flags & TYP_FL_LBINF2) {
            rv->flags |= TYP_FL_UBINF;
        } else {
            if (btyp != NCX_BT_NONE) {
                res = ncx_copy_num(&rv->lb, &rv->ub, btyp);
            } else {
                rv->ubstr = xml_strdup(rv->lbstr);
                if (!rv->ubstr) {
                    res = ERR_INTERNAL_MEM;
                }
            }
        }
        done = TRUE;
        break;
    case TK_TT_RANGESEP:
        /* continue on to upper bound */
        res = TK_ADV(tkc);
        break;
    default:
        res = ERR_NCX_WRONG_TKTYPE;
    }

    /* record any error in previous section */
    if (res != NO_ERR) {
        ncx_mod_exp_err(tkc, mod, res, expstr);
        retres = res;
        if (NEED_EXIT(res)) {
            typ_free_rangedef(rv, btyp);
            return res;
        }
    }

    /* get the last range-boundary, check if max requested */
    if (!done) {
        if (TK_CUR_STR(tkc)) {
            if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_MIN)) {
                /* flag upper bound min for later eval */
                rv->flags |= TYP_FL_UBMIN;
            } else if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_MAX)) {
                /* flag upper bound max for later eval */
                rv->flags |= TYP_FL_UBMAX;
            } else if (btyp==NCX_BT_FLOAT64) {
                if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_NEGINF)) {
                    /* flag upper bound -INF for later eval */
                    rv->flags |= TYP_FL_UBINF2;
                } else if (!xml_strcmp(TK_CUR_VAL(tkc), YANG_K_POSINF)) {
                    /* flag upper bound INF for later eval */
                    rv->flags |= TYP_FL_UBINF;
                } else {
                    res = ERR_NCX_WRONG_TKVAL;
                }
            } else {
                res = ERR_NCX_WRONG_TKVAL;
            }
        } else if (TK_CUR_NUM(tkc)) {
            if (btyp != NCX_BT_NONE) {
                if (btyp == NCX_BT_DECIMAL64) {
                    res = ncx_convert_tkc_dec64
                        (tkc, 
                         typ_get_fraction_digits(typdef), 
                         &rv->ub);
                } else {
                    res = ncx_convert_tkcnum(tkc, btyp, &rv->ub);
                }
            } else {
                rv->ubstr = xml_strdup(TK_CUR_VAL(tkc));
                if (!rv->ubstr) {
                    res = ERR_INTERNAL_MEM;
                }
            }
        } else {
            res = ERR_NCX_WRONG_TKTYPE;
        }

        /* record any error in previous section */
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            retres = res;
            if (NEED_EXIT(res)) {
                typ_free_rangedef(rv, btyp);
                return res;
            }
        }

        /* move past this keyword or number */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_mod_exp_err(tkc, mod, res, expstr);
            typ_free_rangedef(rv, btyp);
            return res;
        }
    }

    /* check basic overlap, still need to evaluate min max later */
    if (!(rv->flags & TYP_RANGE_FLAGS)) {
        /* just numbers entered, no min.max.-INF, INF */
        if (btyp != NCX_BT_NONE) {
            cmpval = ncx_compare_nums(&rv->lb, &rv->ub, btyp);
            if (cmpval > 0) {
                retres = ERR_NCX_INVALID_RANGE;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }
    } else {
        /* check corner cases with min, max, -INF, INF keywords
         * check this LB > the last UB (if any lastrv) 
         */
        if (rv->flags & (TYP_FL_LBMAX|TYP_FL_LBINF2)) {
            /* lower bound set to max or INF */
            if (!(rv->flags & (TYP_FL_UBMAX|TYP_FL_UBINF))) {
                /* upper bound not set to max or INF */
                retres = ERR_NCX_INVALID_RANGE;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }
        if (rv->flags & (TYP_FL_UBMIN|TYP_FL_UBINF2)) {
            /* upper bound set to min or -INF */
            if (!(rv->flags & (TYP_FL_LBMIN|TYP_FL_LBINF))) {
                /* lower bound not set to min or -INF */
                retres = ERR_NCX_INVALID_RANGE;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }
        if ((rv->flags & TYP_FL_LBINF) && !dlq_empty(rangeQ)) {
            /* LB set to -INF and the rangeQ is not empty */
            retres = ERR_NCX_OVERLAP_RANGE;
            ncx_print_errormsg(tkc, mod, retres);
        }
    }

    /* save and exit if rangedef is valid */
    if (retres == NO_ERR) {
        dlq_enque(rv, rangeQ);
    } else {
        typ_free_rangedef(rv, btyp);
    }
    return retres;

}  /* consume_yang_rangedef */


/********************************************************************
* FUNCTION consume_yang_range
* 
* Current token is the 'range' or 'length' keyword
* Process the token chain and gather the range definition in typdef
*
* Cloned from consume_range in ncx_parse.c because
* the YANG range clause is different than NCX
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typedef struct in progress (already setup as SIMPLE)
*            simple.btyp == enum for the specific builtin type
*   isrange == TRUE if called from range definition
*           == FALSE if called from a length definition
*
* RETURNS:
*  status
*********************************************************************/
static status_t 
    consume_yang_range (tk_chain_t *tkc,
                        ncx_module_t *mod,
                        typ_def_t *typdef,
                        boolean isrange)
{
    typ_range_t     *range;
    status_t         res = NO_ERR;
    boolean          done;
    ncx_btype_t      rbtyp;
    ncx_btype_t      btyp;

    range = typ_get_range_con(typdef);

    /* save the range token in case needed for error msg */
    ncx_set_error( &range->tkerr, mod, TK_CUR_LNUM(tkc), TK_CUR_LPOS(tkc)); 

    if ( !range ) {
        res = ERR_INTERNAL_PTR;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    btyp = typ_get_basetype(typdef);
    if (btyp == NCX_BT_NONE) {
        if (isrange) {
            /* signal that the range will be processed in phase 2 */
            rbtyp = NCX_BT_NONE; 
        } else {
            /* length range is always uint32 */
            rbtyp = NCX_BT_UINT32;   
        }
    } else {
        /* get the correct number type for the range specification */
        rbtyp = typ_get_range_type(btyp);
    }

    /* move past start-of-range keyword check token type, which may be in a 
     * quoted string * an identifier string, or a series of tokens */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    if (TK_CUR_STR(tkc)) {
        if (range->rangestr) {
            m__free(range->rangestr);
        }
        range->rangestr = xml_strdup(TK_CUR_VAL(tkc));
        if (!range->rangestr) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }

        /* undo quotes or identifier string into separate tokens. In YANG there
         * are very few tokens, so all these no-WSP strings are valid. If 
         * quoted, the entire range clause will be a single string.  If not, 
         * many combinations of valid range clause tokens might be 
         * mis-classified * E.g.:
         *  range min..max;  (TSTRING)
         *  range 1..100;    (DNUM, STRING, DNUM)
         *  range -INF..47.8 (STRING, RNUM) */
        res = tk_retokenize_cur_string(tkc, mod);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }           
    }

    /* get all the range-part sections */
    done = FALSE;
    while (!done) {
        /* get one range spec */
        res = consume_yang_rangedef(tkc, mod, typdef, rbtyp);
        if ( ( NO_ERR != res && res < ERR_LAST_SYS_ERR ) 
             || ERR_NCX_EOF == res ) { 
	        return res; 
	    } 

        /* Current token is either a BAR or SEMICOL/LBRACE. Move past it if 
         * BAR and keep going * Else exit loop, pointing at this token */
        switch (TK_CUR_TYP(tkc)) {
        case TK_TT_SEMICOL:
        case TK_TT_LBRACE:
            done = TRUE;
            break;
        case TK_TT_BAR:
            res = TK_ADV(tkc);
            if (res != NO_ERR) {
                ncx_print_errormsg(tkc, mod, res);
                return res;
            }
            break;
        default:
            res = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err( tkc, mod, res, 
                             "semi-colon, left brace, or vertical bar" );
            continue;
        }
    }

    /* check sub-section for error-app-tag and error-message */
    if (TK_CUR_TYP(tkc)==TK_TT_LBRACE) {
        res = yang_consume_error_stmts( tkc, mod, &range->range_errinfo,
                                        &typdef->appinfoQ );
    }

    return res;
}  /* consume_yang_range */


/********************************************************************
* FUNCTION consume_yang_pattern
* 
* Current token is the 'pattern' keyword
* Process the token chain and gather the pattern definition in typdef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typedef struct in progress (already setup as SIMPLE)
*            simple.btyp == enum for the specific builtin type
*
* RETURNS:
*  status
*********************************************************************/
static status_t 
    consume_yang_pattern ( tk_chain_t *tkc,
                           ncx_module_t *mod,
                           typ_def_t *typdef )
{
    typ_pattern_t   *pat = NULL;
    status_t         res;

    typ_set_strrest(typdef, NCX_SR_PATTERN);

    /* move past pattern keyword to pattern value, get 1 string */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* need to accept any token as the pattern string,
     * even if it was parsed as a non-string token
     * special case: check for module prefix present, which
     * is not present in the TK_CUR_VAL() string
     */
    if (TK_TT_MSTRING == TK_CUR_TYP(tkc) || 
        TK_TT_MSSTRING == TK_CUR_TYP(tkc)) {

        /* need to use the entire prefix:value string */
        xmlChar *p, *buff = m__getMem(TK_CUR_MODLEN(tkc) + 
                                      TK_CUR_LEN(tkc) + 2);

        if (NULL == buff) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }

        p = buff;
        p += xml_strncpy(p, TK_CUR_MOD(tkc), TK_CUR_MODLEN(tkc));
        *p++ = ':';
        p += xml_strncpy(p, TK_CUR_VAL(tkc), TK_CUR_LEN(tkc));
        *p = 0;

        pat = typ_new_pattern(buff);
        m__free(buff);
    } else {
        pat = typ_new_pattern(TK_CUR_VAL(tkc));
    }

    if (!pat) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* make sure the pattern is valid */
    res = typ_compile_pattern(pat);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        if (res < ERR_LAST_SYS_ERR || res==ERR_NCX_EOF) {
            typ_free_pattern( pat );
            return res;
        }
    }

    /* save the struct in the patternQ early, even if it didn't compile */
    dlq_enque(pat, &typdef->def.simple.patternQ);

    /* move to the next token, which must be ';' or '{' */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        break;
    case TK_TT_LBRACE:
        /* check sub-section for error-app-tag and error-message */
        res = yang_consume_error_stmts( tkc, mod, &pat->pat_errinfo,
                                        &typdef->appinfoQ);
        break;
    default:
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, "semicolon or left brace" );
    }

    return res;

}  /* consume_yang_pattern */


/********************************************************************
* FUNCTION finish_string_type
* 
* Parse the next N tokens as 1 string type definition
* sub-section, expecting 'length' or 'pattern' keywords
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*   btyp == the builtin type already figured out
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_string_type (tk_chain_t  *tkc,
                        ncx_module_t *mod,
                        typ_def_t *typdef,
                        ncx_btype_t btyp)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, lendone;

    expstr = "length or pattern keyword";
    done = FALSE;    
    lendone = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_LENGTH)) {
            if (lendone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: length clause already entered");
                ncx_print_errormsg(tkc, mod, retres);
            }

            lendone = TRUE;
            res = consume_yang_range(tkc, mod, typdef, FALSE);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_PATTERN)) {
            /* make sure this is not the 'binary' data type */
            if (btyp == NCX_BT_BINARY) {
                log_error("\nPattern restriction not allowed"
                          " for the binary type");
                retres = ERR_NCX_RESTRICT_NOT_ALLOWED;
                ncx_print_errormsg(tkc, mod, retres);
            } 

            res = consume_yang_pattern(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    return retres;

}  /* finish_string_type */


/********************************************************************
* FUNCTION finish_number_type
* 
* Parse the next N tokens as the sub-section for a number type
* Expecting range statement.
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   btyp == basetype of the number
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_number_type (tk_chain_t  *tkc,
                        ncx_module_t *mod,
                        ncx_btype_t btyp,
                        typ_def_t *typdef)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    uint32         digits;
    boolean        done, rangedone, digitsdone;

    expstr = "range keyword";
    rangedone = FALSE;
    digitsdone = FALSE;
    digits = 0;
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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_RANGE)) {
            if (rangedone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: range statement already entered");
                ncx_print_errormsg(tkc, mod, retres);
            }

            rangedone = TRUE;
            res = consume_yang_range(tkc, mod, typdef, TRUE);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_FRACTION_DIGITS)) {
            res = yang_consume_uint32(tkc,
                                      mod,
                                      &digits,
                                      &digitsdone,
                                      &typdef->appinfoQ);
            if (res == NO_ERR && btyp != NCX_BT_DECIMAL64) {
                retres = res = ERR_NCX_RESTRICT_NOT_ALLOWED;
                log_error("\nError: fraction-digits found for '%s'",
                          tk_get_btype_sym(btyp));
                ncx_print_errormsg(tkc, mod, retres);
            }

            if (res == NO_ERR) {
                /* check value is actually in proper range */
                if (digits < TYP_DEC64_MIN_DIGITS ||
                    digits > TYP_DEC64_MAX_DIGITS) {
                    retres = res = ERR_NCX_INVALID_VALUE;
                    log_error("\nError: fraction-digits '%u' out of range",
                              digits);
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }

            if (res == NO_ERR) {
                res = typ_set_fraction_digits(typdef, (uint8)digits);
                if (res != NO_ERR) {
                    retres = res;
                    log_error("\nError: set fraction-digits failed");
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    return retres;

}  /* finish_number_type */


/********************************************************************
* FUNCTION finish_unknown_type
* 
* Parse the next N tokens as the subsection of a
* local type that is being refined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_unknown_type (tk_chain_t  *tkc,
                         ncx_module_t *mod,
                         typ_def_t *typdef)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, rangedone, lendone;

    expstr = "range, length, or pattern keyword";
    rangedone = FALSE;
    lendone = FALSE;
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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
            CHK_EXIT(res, retres);
            continue;
        case TK_TT_RBRACE:
            done = TRUE;
            continue;
        case TK_TT_TSTRING:
            break;  /* YANG clause assumed */
        default:
            res = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_RANGE)) {
            if (lendone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: length clause already entered,"
                          "  range clause not allowed");
                ncx_print_errormsg(tkc, mod, retres);
            }
            if (rangedone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: range clause already entered");
                ncx_print_errormsg(tkc, mod, retres);
            }

            rangedone = TRUE;
            res = consume_yang_range(tkc, mod, typdef, TRUE);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_LENGTH)) {
            if (rangedone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: range clause already entered,"
                          "  length clause not allowed");
                ncx_print_errormsg(tkc, mod, retres);
            }
            if (lendone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: length clause already entered");
                ncx_print_errormsg(tkc, mod, retres);
            }
            lendone = TRUE;
            res = consume_yang_range(tkc, mod, typdef, FALSE);
            CHK_EXIT(res, retres);
        } else if (!xml_strcmp(val, YANG_K_PATTERN)) {
            res = consume_yang_pattern(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    return retres;

}  /* finish_unknown_type */


/********************************************************************
* FUNCTION insert_yang_enumbit
* 
* Insert an enum or bit in its proper order in the simple->valQ
*
*
* Error messages are printed by this function
*
* INPUTS:
*   tkc == token chain in progress
*   mod == module in progress
*   en == typ_enum_t struct to insert in the sim->valQ
*   typdef == simple typdef in progress
*   btyp == builtin type (NCX_BT_ENUM or NCX_BT_BITS)
*   valset == TRUE is value or position is set
*          == FALSE to use auto-numbering
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    insert_yang_enumbit (tk_chain_t *tkc,
                         ncx_module_t *mod,
                         typ_enum_t *en, 
                         typ_def_t *typdef,
                         ncx_btype_t btyp,
                         boolean valset)
{
    typ_simple_t *sim;
    typ_enum_t   *enl;
    boolean       done;
    status_t      res;

    sim = &typdef->def.simple;

    /* check if this is a duplicate entry */
    for (enl = (typ_enum_t *)dlq_firstEntry(&sim->valQ);
         enl != NULL;
         enl = (typ_enum_t *)dlq_nextEntry(enl)) {
        if (!xml_strcmp(en->name, enl->name)) {
            res = ERR_NCX_DUP_ENTRY;
            log_error("\nError: duplicate enum or bit name (%s)", en->name);
            ncx_print_errormsg(tkc, mod, res);
            return res;
        } else if (valset && btyp==NCX_BT_ENUM && en->val==enl->val) {
            res = ERR_NCX_DUP_ENTRY;
            log_error("\nError: duplicate enum value (%d)", en->val);
            ncx_print_errormsg(tkc, mod, res);
            return res;
        } else if (valset && btyp==NCX_BT_BITS && en->pos==enl->pos) {
            res = ERR_NCX_DUP_ENTRY;
            log_error("\nError: duplicate bit position (%u)", en->pos);
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }
    }

    if (valset) {
        /* check if this is an out-of-order insert */
        done = FALSE;
        for (enl = (typ_enum_t *)dlq_firstEntry(&sim->valQ);
             enl != NULL && !done;
             enl = (typ_enum_t *)dlq_nextEntry(enl)) {
            if (btyp==NCX_BT_ENUM) {
                if (en->val < enl->val) {
                    res = ERR_NCX_ENUM_VAL_ORDER;
                    if (ncx_warning_enabled(res)) {
                        log_warn("\nWarning: Out of order enum '%s' = %d",
                                 (char *)en->name, en->val);
                        ncx_print_errormsg(tkc, mod, res);
                    } else {
                        ncx_inc_warnings(mod);
                    }
                    done = TRUE;
                }
            } else {
                if (en->pos < enl->pos) {
                    res = ERR_NCX_BIT_POS_ORDER;
                    if (ncx_warning_enabled(res)) {
                        log_warn("\nWarning: Out of order bit '%s' = %u",
                                 (char *)en->name, en->pos);
                        ncx_print_errormsg(tkc, mod, res);
                    } else {
                        ncx_inc_warnings(mod);
                    }
                    done = TRUE;
                }
            }
        }
    } else {
        /* value not set, use highest value + 1 */
        if (dlq_empty(&sim->valQ)) {
            /* first enum set gets the value zero */
            if (btyp==NCX_BT_ENUM) {
                en->val = 0;
                sim->maxenum = 0;
            } else {
                en->pos = 0;
                sim->maxbit = 0;
            }
        } else {
            /* assign the highest value in use + 1 */
            if (btyp==NCX_BT_ENUM) {
                if (sim->maxenum < NCX_MAX_INT) {
                    en->val = ++(sim->maxenum);
                } else {
                    res = ERR_NCX_INVALID_VALUE;
                    log_error("\nError: Cannot auto-increment enum "
                              "value beyond MAXINT");
                    ncx_print_errormsg(tkc, mod, res);
                }
            } else {
                if (sim->maxbit < NCX_MAX_UINT) {
                    en->pos = ++(sim->maxbit);
                } else {
                    res = ERR_NCX_INVALID_VALUE;
                    log_error("\nError: Cannot auto-increment bit "
                              "position beyond MAXUINT");
                    ncx_print_errormsg(tkc, mod, res);
                }
            }
        }
    }

    /* always insert in the user-defined order == new last entry */
    dlq_enque(en, &sim->valQ);
    return NO_ERR;

}  /* insert_yang_enumbit */


/********************************************************************
* FUNCTION consume_yang_bit
* 
* Current token is the 'bit' keyword
* Process the token chain and gather the bit definition in typdef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typedef struct in progress (already setup as SIMPLE)
*            simple.btyp == enum for the specific builtin type
*
* RETURNS:
*  status
*********************************************************************/
static status_t 
    consume_yang_bit (tk_chain_t *tkc,
                      ncx_module_t *mod,
                      typ_def_t *typdef)
{
    typ_enum_t    *enu;
    xmlChar       *val, *str;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, desc, ref, stat, pos;

    enu = NULL;
    str = NULL;
    expstr = NULL;
    retres = NO_ERR;
    done = FALSE;
    desc = FALSE;
    ref = FALSE;
    stat = FALSE;
    pos = FALSE;

    /* move past bit keyword
     * check token type, which should be an identifier string
     */
    res = yang_consume_id_string(tkc, mod, &str);
    CHK_EXIT(res, retres);

    if (str) {
        enu = typ_new_enum2(str);
    } else {
        enu = typ_new_enum((const xmlChar *)"none");
    }
    if (!enu) {
        res = ERR_INTERNAL_MEM;
        if (str) {
            m__free(str);
        }
        ncx_print_errormsg(tkc, mod, res);
        return res;
    } else {
        enu->flags |= TYP_FL_ISBITS;
    }

    /* move past identifier-str to stmtsep */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        typ_free_enum(enu);
        return res;
    }

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        done = FALSE;
        break;
    default:
        expstr = "semi-colon or left brace";
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        typ_free_enum(enu);
        return res;
    }

    /* get all the bit sub-clauses */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            typ_free_enum(enu);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            typ_free_enum(enu);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &enu->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_enum(enu);
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
            expstr = "bit sub-statement";
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, &enu->descr,
                                     &desc, 
                                     &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &enu->ref,
                                     &ref, 
                                     &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &enu->status,
                                      &stat, 
                                      &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_POSITION)) {
            res = yang_consume_uint32(tkc, 
                                      mod, 
                                      &enu->pos,
                                      &pos, 
                                      &enu->appinfoQ);
            enu->flags |= TYP_FL_ESET;   /* mark explicit set val */
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                typ_free_enum(enu);
                return res;
            }
        }
    }

    /* save the bit definition */
    if (retres == NO_ERR) {
        res = insert_yang_enumbit(tkc, 
                                  mod, 
                                  enu, 
                                  typdef,
                                  NCX_BT_BITS, 
                                  pos);
        if (res != NO_ERR) {
            typ_free_enum(enu);
        }
    } else {
        typ_free_enum(enu);
    }

    return retres;

}  /* consume_yang_bit */


/********************************************************************
* FUNCTION finish_bits_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_BITS type that is being refined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_bits_type (tk_chain_t  *tkc,
                      ncx_module_t *mod,
                      typ_def_t *typdef)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, bitdone;

    expstr = "bit keyword";
    bitdone = FALSE;
    retres = NO_ERR;
    done = FALSE;

    typdef->def.simple.strrest = NCX_SR_BIT;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_BIT)) {
            bitdone = TRUE;
            res = consume_yang_bit(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    /* check all the mandatory clauses are present */
    if (!bitdone) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, 
                            mod, 
                            "bits", 
                            "bit");
    }

    return retres;

}  /* finish_bits_type */


/********************************************************************
* FUNCTION consume_yang_enum
* 
* Current token is the 'enum' keyword
* Process the token chain and gather the enum definition in typdef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typedef struct in progress (already setup as SIMPLE)
*            simple.btyp == enum for the specific builtin type
*
* RETURNS:
*  status
*********************************************************************/
static status_t 
    consume_yang_enum (tk_chain_t *tkc,
                       ncx_module_t *mod,
                       typ_def_t *typdef)
{
    typ_enum_t    *enu;
    xmlChar       *val, *str;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, desc, ref, stat, valdone;

    enu = NULL;
    str = NULL;
    res = NO_ERR;
    retres = NO_ERR;
    desc = FALSE;
    ref = FALSE;
    stat = FALSE;
    valdone = FALSE;
    expstr = "enum value string";

    /* move past enum keyword
     * check token type, which should be a trimmed string
     */
    res = yang_consume_string(tkc, mod, &str);
    CHK_EXIT(res, retres);

    /* validate the enum string format */
    if (str) {
        if (!*str) {
            res = ERR_NCX_WRONG_LEN;
            log_error("\nError: Zero length enum string");
            ncx_mod_exp_err(tkc, mod, res, expstr);
            m__free(str);
            str = NULL;
        } else if (xml_isspace(*str)) {
            res = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Leading whitespace in enum string '%s'",
                      str);
            ncx_mod_exp_err(tkc, mod, res, expstr);
            m__free(str);
            str = NULL;
        } else if (xml_isspace(str[xml_strlen(str)-1])) {
            res = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Trailing whitespace in enum string '%s'",
                      str);
            ncx_mod_exp_err(tkc, mod, res, expstr);
            m__free(str);
            str = NULL;
        } else {
            /* pass off the malloced string */
            enu = typ_new_enum2(str);
        }
    } else {
        /* copy the placeholder string during error processing */
        enu = typ_new_enum(NCX_EL_NONE);
    }

    /* check enum malloced OK */
    if (!enu) {
        if (res == NO_ERR) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
        }
        if (str) {
            m__free(str);
        }
        return res;
    }

    /* have malloced enum struct
     * move past identifier-str to stmtsep
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        typ_free_enum(enu);
        return res;
    }

    /* check statement exit or sub-statement start */
    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_SEMICOL:
        done = TRUE;
        break;
    case TK_TT_LBRACE:
        done = FALSE;
        break;
    default:
        expstr = "semi-colon or left brace";
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, res, expstr);
        typ_free_enum(enu);
        return res;
    }

    /* get all the enum sub-clauses */
    while (!done) {
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            typ_free_enum(enu);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            typ_free_enum(enu);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &enu->appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_enum(enu);
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
            expstr = "bit sub-statement";
            ncx_mod_exp_err(tkc, mod, retres, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod,  
                                     &enu->descr, 
                                     &desc,  
                                     &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod,  
                                     &enu->ref, 
                                     &ref,  
                                     &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod,  
                                      &enu->status, 
                                      &stat,
                                      &enu->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_VALUE)) {
            res = yang_consume_int32(tkc,
                                     mod,
                                     &enu->val, 
                                     &valdone,
                                     &enu->appinfoQ);
            enu->flags |= TYP_FL_ESET;   /* mark explicit set val */
        } else {
            res = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, res, expstr);
        }
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                typ_free_enum(enu);
                return res;
            }
        }
    }

    /* save the enum definition */
    if (retres == NO_ERR) {
        res = insert_yang_enumbit(tkc,
                                  mod,
                                  enu,
                                  typdef,
                                  NCX_BT_ENUM,
                                  valdone);
        if (res != NO_ERR) {
            typ_free_enum(enu);
        }
    } else {
        typ_free_enum(enu);
    }

    return retres;

}  /* consume_yang_enum */


/********************************************************************
* FUNCTION finish_enum_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_ENUM type that is being refined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_enum_type (tk_chain_t  *tkc,
                      ncx_module_t *mod,
                      typ_def_t *typdef)
{
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, enumdone;

    expstr = "enum keyword";
    enumdone = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    typdef->def.simple.strrest = NCX_SR_ENUM;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_ENUM)) {
            enumdone = TRUE;
            res = consume_yang_enum(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    /* check all the mandatory clauses are present */
    if (!enumdone) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "enumeration", "enum");
    }

    return retres;

}  /* finish_enum_type */


/********************************************************************
* FUNCTION finish_union_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_UNION type that is being defined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_union_type (yang_pcb_t *pcb,
                       tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       typ_def_t *typdef)
{
    const xmlChar *val;
    const char    *expstr;
    typ_unionnode_t  *un;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, typedone;

    expstr = "type keyword";
    typedone = FALSE;
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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_TYPE)) {
            typedone = TRUE;
            un = typ_new_unionnode(NULL);
            if (!un) {
                res = ERR_INTERNAL_MEM;
                ncx_print_errormsg(tkc, mod, res);
                return res;
            } else {
                un->typdef = typ_new_typdef();
                if (!un->typdef) {
                    res = ERR_INTERNAL_MEM;
                    ncx_print_errormsg(tkc, mod, res);
                    typ_free_unionnode(un);
                    return res;
                }
            }

            res = yang_typ_consume_type(pcb,
                                        tkc, 
                                        mod, 
                                        un->typdef);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_unionnode(un);
                    return res;
                }
            }

            if (res == NO_ERR) {
                dlq_enque(un, &typdef->def.simple.unionQ);
            } else {
                typ_free_unionnode(un);
            }
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    /* check all the mandatory clauses are present */
    if (!typedone) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, mod, "union", "type");
    }

    return retres;

}  /* finish_union_type */


/********************************************************************
* FUNCTION finish_leafref_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_LEAFREF type that is being defined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_leafref_type (tk_chain_t  *tkc,
                        ncx_module_t *mod,
                        typ_def_t *typdef)
{
    typ_simple_t  *sim;
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, pathdone, constrained;

    expstr = "path keyword";
    pathdone = FALSE;
    constrained = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    sim = &typdef->def.simple;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_PATH)) {
            if (pathdone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                log_error("\nError: path statement already entered");
                ncx_print_errormsg(tkc, mod, retres);
            }

            /* get the path string value and save it */
            res = TK_ADV(tkc);
            if (res != NO_ERR) {
                ncx_print_errormsg(tkc, mod, res);
                return res;
            }

            /* store, parse, and validate that the leafref
             * is well-formed. Do not check the objects
             * in the path string until later
             */
            if (TK_CUR_STR(tkc)) {
                /* create an XPath parser control block and
                 *  store the path string there
                 */
                if (!pathdone) {
                    /* very little validation is done now
                     * because the object using this leafref
                     * data type is needed to set the starting
                     * context node
                     */
                    sim->xleafref = xpath_new_pcb(TK_CUR_VAL(tkc), NULL);
                    if (!sim->xleafref) {
                        res = ERR_INTERNAL_MEM;
                        ncx_print_errormsg(tkc, mod, res);
                        return res;
                    } else {
                        res = tk_check_save_origstr
                            (tkc, 
                             TK_CUR(tkc),
                             (const void *)&sim->xleafref->exprstr);
                        if (res == NO_ERR) {
                            ncx_set_error(&sim->xleafref->tkerr,
                                          mod,
                                          TK_CUR_LNUM(tkc),
                                          TK_CUR_LPOS(tkc));
                            res = xpath_yang_parse_path(tkc, 
                                                        mod, 
                                                        XP_SRC_LEAFREF,
                                                        sim->xleafref);
                        }
                        if (res != NO_ERR) {
                            /* errors already reported */
                            retres = res;
                        }
                    }
                }

                pathdone = TRUE;
                
                res = yang_consume_semiapp(tkc, mod, &typdef->appinfoQ);
                CHK_EXIT(res, retres);
            }
            sim->constrained = TRUE;
        } else {
            retres = ERR_NCX_WRONG_TKTYPE;
            expstr = "path statement";
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    if (retres == NO_ERR && !pathdone) {
        retres = ERR_NCX_MISSING_REFTARGET;
        log_error("\nError: path missing in leafref type");
        ncx_print_errormsg(tkc, mod, retres);
    }

    if (retres == NO_ERR && !constrained) {
        /* set to default */
        sim->constrained = TRUE;
    }

    return retres;

}  /* finish_leafref_type */


/********************************************************************
* FUNCTION finish_instanceid_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_INSTANCE_ID type that is being defined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_instanceid_type (tk_chain_t  *tkc,
                            ncx_module_t *mod,
                            typ_def_t *typdef)
{
    typ_simple_t  *sim;
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, constrained;

    expstr = "require-instance keyword";
    constrained = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    sim = &typdef->def.simple;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_REQUIRE_INSTANCE)) {
            res = yang_consume_boolean(tkc, 
                                       mod, 
                                       &sim->constrained,
                                       &constrained,
                                       &typdef->appinfoQ);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKTYPE;
            expstr = "require-instance statement";
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    if (retres == NO_ERR && !constrained) {
        /* set to default */
        sim->constrained = TRUE;
    }

    return retres;

}  /* finish_instanceid_type */


/********************************************************************
* FUNCTION finish_idref_type
* 
* Parse the next N tokens as the subsection of a
* NCX_BT_IDREF type that is being defined
*
* Current token is the left brace starting the string
* sub-section. Continue until right brace or error.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_idref_type (tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       typ_def_t *typdef)
{
    typ_simple_t  *sim;
    const xmlChar *val;
    const char    *expstr;
    tk_type_t      tktyp;
    status_t       res, retres;
    boolean        done, basedone;

    expstr = "base keyword";
    basedone = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;

    sim = &typdef->def.simple;

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
            res = ncx_consume_appinfo(tkc, mod, &typdef->appinfoQ);
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
        if (!xml_strcmp(val, YANG_K_BASE)) {
            sim->idref.modname = ncx_get_modname(mod);
            res = yang_consume_pid(tkc, 
                                   mod,
                                   &sim->idref.baseprefix,
                                   &sim->idref.basename,
                                   &basedone,
                                   &typdef->appinfoQ);
            CHK_EXIT(res, retres);
        } else {
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    return retres;

}  /* finish_idref_type */


/********************************************************************
* FUNCTION resolve_minmax
* 
* Resolve the min and max keywords in the rangeQ if present
* 
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   rangeQ == rangeQ to process
*   simple == TRUE if this is a NCX_CL_SIMPLE typedef
*          == FALSE if this is a NCX_CL_NAMED typedef
*   rbtyp == range builtin type
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_minmax (tk_chain_t *tkc,
                    ncx_module_t *mod,
                    typ_def_t *typdef,
                    boolean simple,
                    ncx_btype_t  rbtyp)
{
    dlq_hdr_t      *rangeQ, *parentQ;
    typ_rangedef_t *rv, *pfirstrv, *plastrv;
    typ_def_t      *parentdef, *rangedef;
    status_t        res;

    rangeQ = typ_get_rangeQ_con(typdef);
    if (!rangeQ || dlq_empty(rangeQ)) {
        return ERR_NCX_SKIPPED;
    }

    res = NO_ERR;
    pfirstrv = NULL;
    plastrv = NULL;

    /* get the parent range bounds, if any */
    if (!simple) {
        parentdef = typ_get_parent_typdef(typdef);
        rangedef = typ_get_qual_typdef(parentdef, NCX_SQUAL_RANGE);
        if (rangedef) {
            parentQ = typ_get_rangeQ_con(rangedef);
            if (!parentQ || dlq_empty(parentQ)) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }
            pfirstrv = (typ_rangedef_t *)dlq_firstEntry(parentQ);
            plastrv = (typ_rangedef_t *)dlq_lastEntry(parentQ);
        }
    }

    for (rv = (typ_rangedef_t *)dlq_firstEntry(rangeQ);
         rv != NULL && res==NO_ERR;
         rv = (typ_rangedef_t *)dlq_nextEntry(rv)) {

        if (rv->btyp ==NCX_BT_NONE) {
            rv->btyp = rbtyp;
        } else if (rv->btyp != rbtyp) {
            res = ERR_NCX_WRONG_DATATYP;
        }

        /* set the lower bound if set to 'min' or 'max' */
        if (rv->flags & TYP_FL_LBMIN) {
            if (pfirstrv) {
                if (pfirstrv->flags & TYP_FL_LBINF) {
                    rv->flags |= TYP_FL_LBINF;
                } else if (pfirstrv->flags & TYP_FL_LBINF2) {
                    rv->flags |= TYP_FL_LBINF2;
                } else {
                    res = ncx_copy_num(&pfirstrv->lb, &rv->lb, rbtyp);
                }
            } else {
                if (rbtyp==NCX_BT_FLOAT64) {
                    rv->flags |= TYP_FL_LBINF;
                } else {
                    ncx_set_num_min(&rv->lb, rbtyp);
                }
            }
        } else if (rv->flags & TYP_FL_LBMAX) {
            if (plastrv) {
                if (plastrv->flags & TYP_FL_UBINF) {
                    rv->flags |= TYP_FL_LBINF2;
                } else if (plastrv->flags & TYP_FL_UBINF2) {
                    rv->flags |= TYP_FL_LBINF;
                } else {
                    res = ncx_copy_num(&plastrv->ub, &rv->lb, rbtyp);
                }
            } else {
                if (rbtyp==NCX_BT_FLOAT64) {
                    rv->flags |= TYP_FL_LBINF2;
                } else {
                    ncx_set_num_max(&rv->lb, rbtyp);
                }
            }
        } else if (rv->lbstr) {
            res = ncx_decode_num(rv->lbstr, rbtyp, &rv->lb);
        }

        if (res != NO_ERR) {
            continue;
        }

        /* set the upper bound if set to 'min' or 'max' */
        if (rv->flags & TYP_FL_UBMAX) {
            if (plastrv) {
                if (plastrv->flags & TYP_FL_UBINF) {
                    rv->flags |= TYP_FL_UBINF;
                } else if (plastrv->flags & TYP_FL_UBINF2) {
                    rv->flags |= TYP_FL_UBINF2;
                } else {
                    res = ncx_copy_num(&plastrv->ub, &rv->ub, rbtyp);
                }
            } else {
                if (rbtyp==NCX_BT_FLOAT64) {
                    rv->flags |= TYP_FL_UBINF;
                } else {
                    ncx_set_num_max(&rv->ub, rbtyp);
                }
            }
        } else if (rv->flags & TYP_FL_UBMIN) {
            if (pfirstrv) {
                if (pfirstrv->flags & TYP_FL_LBINF) {
                    rv->flags |= TYP_FL_UBINF2;
                } else if (pfirstrv->flags & TYP_FL_LBINF2) {
                    rv->flags |= TYP_FL_UBINF;
                } else {
                    res = ncx_copy_num(&pfirstrv->lb, &rv->ub, rbtyp);
                }
            } else {
                if (rbtyp==NCX_BT_FLOAT64) {
                    rv->flags |= TYP_FL_UBINF2;
                } else {
                    ncx_set_num_min(&rv->ub, rbtyp);
                }
            }
        } else if (rv->ubstr) {
            res = ncx_decode_num(rv->ubstr, rbtyp, &rv->ub);
        }
    }

    if (res != NO_ERR) {
        if (simple) {
            tkc->curerr = &typdef->def.simple.range.tkerr;
        } else {
            tkc->curerr = &typdef->tkerr;
        }
        ncx_print_errormsg(tkc, mod, res);
    } else {
        /* set flags even if boundary set explicitly so
         * the ncxdump program will suppress these clauses
         * unless really needed
         */
        rv = (typ_rangedef_t *)dlq_firstEntry(rangeQ);
        if (rv && ncx_is_min(&rv->lb, rbtyp)) {
            rv->flags |= TYP_FL_LBMIN;
        }
        rv = (typ_rangedef_t *)dlq_lastEntry(rangeQ);
        if (rv && ncx_is_max(&rv->ub, rbtyp)) {
            rv->flags |= TYP_FL_UBMAX;
        }
    }
            
    return res;

}  /* resolve_minmax */


/********************************************************************
* FUNCTION finish_yang_range
* 
* Validate the typdef chain range definitions
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   typdef == typ_def_t in progress
*   rbtyp == range builtin type
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    finish_yang_range (tk_chain_t  *tkc,
                       ncx_module_t *mod,
                       typ_def_t *typdef,
                       ncx_btype_t rbtyp)
{
    dlq_hdr_t    *rangeQ;
    typ_def_t    *parentdef;
    status_t      res;
    
    res = NO_ERR;

    switch (typdef->tclass) {
    case NCX_CL_NAMED:
        parentdef = typ_get_parent_typdef(typdef);
        if (parentdef) {
            res = finish_yang_range(tkc, 
                                    mod,
                                    parentdef,
                                    rbtyp);
        } else {
            res = ERR_NCX_MISSING_TYPE;
        }
        if (res == NO_ERR) {
            res = resolve_minmax(tkc, mod, typdef, FALSE, rbtyp);
            if (res == NO_ERR) {
                rangeQ = typ_get_rangeQ_con(typdef);
                typ_normalize_rangeQ(rangeQ, rbtyp);
            } else if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            }
        }
        break;
    case NCX_CL_SIMPLE:
        /* this is the final stop in the type chain
         * if this typdef has a range, then check it
         * and convert min/max to real numbers
         */
        res = resolve_minmax(tkc, mod, typdef, TRUE, rbtyp);
        if (res == NO_ERR) {
            rangeQ = typ_get_rangeQ_con(typdef);
            typ_normalize_rangeQ(rangeQ, rbtyp);
        } else if (res == ERR_NCX_SKIPPED) {
            res = NO_ERR;
        }
        break;
    default:
        break;
    }

    return res;

} /* finish_yang_range */


/********************************************************************
* FUNCTION rv_fits_in_rangedef
* 
* Check if the testrv fits within the checkrv
*
* INPUTS:
*    btyp == number base type
*    testrv == rangedef to test
*    checkrv == rangedef to test against
*
* RETURNS:
*    status, NO_ERR if rangedef is valid and passes the range
*            defined by the rangedefs in checkQ
*********************************************************************/
static status_t
    rv_fits_in_rangedef (ncx_btype_t  btyp,
                         const typ_rangedef_t *testrv,
                         const typ_rangedef_t *checkrv)
{
    int32            cmp;
    boolean          lbok, lbok2, ubok, ubok2;

    lbok = FALSE;
    lbok2 = FALSE;
    ubok = FALSE;
    ubok2 = FALSE;

    /* make sure any INF corner cases are met */
    if (testrv->flags & TYP_FL_LBINF && !(checkrv->flags & TYP_FL_LBINF)) {
        return ERR_NCX_NOT_IN_RANGE;
    }
    if (testrv->flags & TYP_FL_LBINF2 && !(checkrv->flags & TYP_FL_LBINF2)) {
        return ERR_NCX_NOT_IN_RANGE;
    }
    if (testrv->flags & TYP_FL_UBINF && !(checkrv->flags & TYP_FL_UBINF)) {
        return ERR_NCX_NOT_IN_RANGE;
    }
    if (testrv->flags & TYP_FL_UBINF2 && !(checkrv->flags & TYP_FL_UBINF2)) {
        return ERR_NCX_NOT_IN_RANGE;
    }

    /* check lower bound */
    if (!(checkrv->flags & TYP_FL_LBINF)) {
        cmp = ncx_compare_nums(&testrv->lb, &checkrv->lb, btyp);
        if (cmp >= 0) {
            lbok = TRUE;
        }
    } else {
        /* LB == -INF, always passes the test */
        lbok = TRUE;
    } 
    if (!(checkrv->flags & TYP_FL_UBINF)) {
        cmp = ncx_compare_nums(&testrv->lb, &checkrv->ub, btyp);
        if (cmp <= 0) {
            lbok2 = TRUE;
        }
    } else {
        /* UB == INF, always passes the test */
        lbok2 = TRUE;
    }

    /* check upper bound */
    if (!(checkrv->flags & TYP_FL_LBINF)) {
        cmp = ncx_compare_nums(&testrv->ub, &checkrv->lb, btyp);
        if (cmp >= 0) {
            ubok = TRUE;
        }
    } else {
        /* LB == -INF, always passes the test */
        ubok = TRUE;
    } 
    if (!(checkrv->flags & TYP_FL_UBINF)) {
        cmp = ncx_compare_nums(&testrv->ub, &checkrv->ub, btyp);
        if (cmp <= 0) {
            ubok2 = TRUE;
        }
    } else {
        /* UB == INF, always passes the test */
        ubok2 = TRUE;
    }

    return (lbok && lbok2 && ubok && ubok2) ? NO_ERR :
        ERR_NCX_NOT_IN_RANGE;

}  /* rv_fits_in_rangedef */


/********************************************************************
* FUNCTION validate_range_chain
* 
* Final validatation of all range definitions within 
* the type definition 
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   intypdef == typ_def_t in progress to validate
*   typname == name from typedef (may be NULL for unnamed types)
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    validate_range_chain (tk_chain_t  *tkc,
                          ncx_module_t *mod,
                          typ_def_t *intypdef,
                          const xmlChar *typname)
{
    dlq_hdr_t      *rangeQ, *prangeQ;
    typ_def_t      *typdef, *parentdef;
    typ_rangedef_t *rv, *checkrv;
    typ_range_t    *range;
    ncx_num_t      *curmax;
    status_t        res, res2;
    boolean         done, errdone, ubinf, first;
    ncx_btype_t     rbtyp;
    int32           retval;

    typdef = typ_get_qual_typdef(intypdef, NCX_SQUAL_RANGE);
    if (!typdef) {
        return NO_ERR;
    }

    res = NO_ERR;
    errdone = FALSE;
    rbtyp = typ_get_range_type(typ_get_basetype(typdef));
    if (!typname) {
        typname = (const xmlChar *)"(unnamed type)";
    }

    /* Test 1: Individual tests
     * validate each individual range definition in each
     * typdef in the chain for the specified typ_template_t
     * Check that each rangeQ is well-formed on its own
     */
    while (typdef && res==NO_ERR) {

        rangeQ = typ_get_rangeQ_con(typdef);
        if (!rangeQ || dlq_empty(rangeQ)) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        curmax = NULL;
        ubinf = FALSE;
        first = TRUE;

        for (rv = (typ_rangedef_t *)dlq_firstEntry(rangeQ);
             rv != NULL && res==NO_ERR;
             rv = (typ_rangedef_t *)dlq_nextEntry(rv)) {

            /* check range-part after INF */
            if (ubinf) {
                res = ERR_NCX_INVALID_RANGE;
                log_error("\nError: INF already used and no more range"
                          " part clauses are allowed in type '%s'",
                          typname);
                continue;
            }

            /* check upper-bound not >= lower-bound */
            retval = ncx_compare_nums(&rv->lb, &rv->ub, rbtyp);
            if (retval == 1) {
                res = ERR_NCX_INVALID_RANGE;
                log_error("\nError: lower bound is greater than "
                          "upper bound in range part clause in type '%s'",
                          typname);
                continue;
            }

            /* check -INF used in a range-part other than first */
            if (rv->flags & TYP_FL_LBINF) {
                if (!first) {
                    res = ERR_NCX_OVERLAP_RANGE;
                    log_error("\nError: -INF not allowed here in type '%s'",
                              typname);
                    continue;
                }
            }

            /* check this lower-bound > any previous upper bound */
            if (rv->flags & TYP_FL_UBINF) {
                ubinf = TRUE;
            } else if (!curmax) {
                curmax = &rv->ub;
            } else {
                retval = ncx_compare_nums(&rv->lb, curmax, rbtyp);
                if (retval != 1) {
                    res = ERR_NCX_OVERLAP_RANGE;
                } else {
                    curmax = &rv->ub;
                }
            }
            first = FALSE;
        }

        if (res == NO_ERR) {
            typdef = typ_get_parent_typdef(typdef);
            if (typdef) {
                typdef = typ_get_qual_typdef(typdef, NCX_SQUAL_RANGE);
            }
        } else {
            tkc->curerr = &typdef->tkerr;
        }
    }

    if (res != NO_ERR && !errdone) {
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* Test 2: Typdef Chain tests
     * validate each individual range definition in each
     * typdef in the chain against any previously defined
     * range definitions in the typdef chain
     */
    typdef = typ_get_qual_typdef(intypdef, NCX_SQUAL_RANGE);
    while (typdef && res==NO_ERR) {

        /* have a typdef with a range definition rangeQ contains the rangedef 
         * structs to test For each parent typdef with a range clause:
         *    check raneQ contents against prangeQ contents
         *
         * rv is the rangedef to test
         * checkrv is the rangedef to test against */
        rangeQ = typ_get_rangeQ_con(typdef);
        for (rv = (typ_rangedef_t *)dlq_firstEntry(rangeQ);
             rv != NULL && res==NO_ERR;
             rv = (typ_rangedef_t *)dlq_nextEntry(rv)) {

            parentdef = typ_get_parent_typdef(typdef);
            if (parentdef) {
                parentdef = typ_get_qual_typdef(parentdef, NCX_SQUAL_RANGE);

                while (parentdef && res==NO_ERR) {
                    prangeQ = typ_get_rangeQ_con(parentdef);

                    /* rv rangedef just has to fit within one of the checkrv 
                     * rangedefs to pass the test */
                    done = FALSE;
                    for (checkrv = (typ_rangedef_t *)dlq_firstEntry(prangeQ);
                         checkrv != NULL && !done;
                         checkrv = (typ_rangedef_t *)
                             dlq_nextEntry(checkrv)) {
                        res2 = rv_fits_in_rangedef(rbtyp, rv, checkrv);
                        if (res2 == NO_ERR) {
                            done = TRUE;
                        }
                    }

                    if (!done) {
                        res = ERR_NCX_NOT_IN_RANGE;
                        log_error("\nError: Range definition not a "
                                  "valid restriction of parent type '%s'",
                                  typname);
                    } else {
                        parentdef = typ_get_parent_typdef(parentdef);
                        if (parentdef) {
                            parentdef = 
                                typ_get_qual_typdef(parentdef,
                                                    NCX_SQUAL_RANGE);
                        }
                    }
                }
            }
        }

        if (res == NO_ERR) {
            /* setup next loop */
            typdef = typ_get_parent_typdef(typdef);
            if (typdef) {
                typdef = typ_get_qual_typdef(typdef, NCX_SQUAL_RANGE);
            }
        } else {
            /* set error token */
            range = typ_get_range_con(typdef);
            if (range) {
                tkc->curerr = &range->tkerr;
            } else {
                tkc->curerr = &typdef->tkerr;
            }
        }
    }

    if (res != NO_ERR && !errdone) {
        ncx_print_errormsg(tkc, mod, res);
    }

    return res;

} /* validate_range_chain */


#if 0
/********************************************************************
* FUNCTION check_defval
* 
* Check the defval agtainst the typdef
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typdef to check
*   obj == obj_template_t containing the union (may be NULL)
*   btyp == base type for the typdef
*   badvalok == TRUE if invalid value is OK (called from union)
*               FALSE if must be OK (called from plain simple type)
*   defval == default value, if any
*   hasdefval == address of has default value return value
*   name == typ_template_t name, if any
*
* OUTPUTS:
*   *hasdefval == default value return value
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    check_defval (tk_chain_t *tkc,
                  ncx_module_t *mod,
                  typ_def_t *typdef,
                  obj_template_t *obj,
                  ncx_btype_t  btyp,
                  boolean badvalok,
                  const xmlChar *defval,
                  boolean *hasdefval,
                  const xmlChar *name)
{
    status_t     res = NO_ERR;

    *hasdefval = FALSE;
    if (defval != NULL) {
        *hasdefval = TRUE;        
        if (btyp == NCX_BT_IDREF) {
            res = val_parse_idref(mod, defval, NULL, NULL, NULL);
        } else {
            res = val_simval_ok(typdef, defval);
        }
        if (res != NO_ERR) {
            if (!badvalok) {
                if (obj) {
                    log_error("\nError: %s '%s' has invalid "
                              "default value (%s)",
                              (name) ? "Type" : 
                              (const char *)obj_get_typestr(obj),
                              (name) ? name : obj_get_name(obj), 
                              defval);
                } else {
                    log_error("\nError: %s '%s' has invalid "
                              "default value (%s)",
                              (name) ? "Type" : "leaf or leaf-list",
                              (name) ? (const char *)name : "--", 
                              defval);
                }
                tkc->curerr = &typdef->tkerr;
                ncx_print_errormsg(tkc, mod, res);
            }
        }
    } else if (typdef->tclass == NCX_CL_NAMED) {
        const xmlChar *typdefval;
        typdefval = typ_get_defval(typdef->def.named.typ);
        if (typdefval) {
            *hasdefval = TRUE;
            if (btyp == NCX_BT_IDREF) {
                res = val_parse_idref(mod, 
                                      typdefval, 
                                      NULL, 
                                      NULL, 
                                      NULL);
            } else {
                res = val_simval_ok(typdef, typdefval);
            }
            if (res != NO_ERR) {
                if (!badvalok) {
                    if (obj) {
                        log_error("\nError: %s '%s' has invalid "
                                  "inherited default value (%s)",
                                  (name) ? "Type" : 
                                  (const char *)obj_get_typestr(obj),
                                  (name) ? name : obj_get_name(obj), 
                                  typdefval);
                    } else {
                        log_error("\nError: %s '%s' has invalid "
                                  "inherited default value (%s)",
                                  (name) ? "Type" : "leaf or leaf-list",
                                  (name) ? (const char *)name : "--", 
                                  typdefval);
                    }
                    tkc->curerr = &typdef->tkerr;
                    ncx_print_errormsg(tkc, mod, res);
                }
            }
        }
    }
    return res;

} /* check_defval */
#endif



/********************************************************************
* FUNCTION check_defval
* 
* Check the defval agtainst the typdef
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typdef == typdef to check
*   obj == obj_template_t containing the union (may be NULL)
*   btyp == base type for the typdef
*   badvalok == TRUE if invalid value is OK (called from union)
*               FALSE if must be OK (called from plain simple type)
*   defval == default value, if any
*   name == typ_template_t name, if any
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    check_defval (tk_chain_t *tkc,
                  ncx_module_t *mod,
                  typ_def_t *typdef,
                  obj_template_t *obj,
                  ncx_btype_t  btyp,
                  boolean badvalok,
                  const xmlChar *defval,
                  const xmlChar *name)
{
    status_t     res = ERR_NCX_SKIPPED;

    if (defval != NULL) {
        if (btyp == NCX_BT_IDREF) {
            res = val_parse_idref(mod, defval, NULL, NULL, NULL);
        } else {
            res = val_simval_ok(typdef, defval);
        }
        if (res != NO_ERR) {
            if (!badvalok) {
                if (obj) {
                    log_error("\nError: %s '%s' has invalid "
                              "default value (%s)",
                              (name) ? "Type" : 
                              (const char *)obj_get_typestr(obj),
                              (name) ? name : obj_get_name(obj), 
                              defval);
                } else {
                    log_error("\nError: %s '%s' has invalid "
                              "default value (%s)",
                              (name) ? "Type" : "leaf or leaf-list",
                              (name) ? (const char *)name : "--", 
                              defval);
                }
                tkc->curerr = &typdef->tkerr;
                ncx_print_errormsg(tkc, mod, res);
            }
        }
    }
    return res;

} /* check_defval */


/********************************************************************
* FUNCTION resolve_union_type
* 
* Check for named type dependency loops
* Called during phase 2 of module parsing
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod == module in progress
*   unionQ == Q of unfinished typ_unionnode_t structs
*   parent == obj_template_t containing the union (may be NULL)
*   grp == grp_template_t containing the union (may be NULL)
*   defval == default value, if any
*   name == typ_template_t name, if any
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    resolve_union_type (yang_pcb_t *pcb,
                        tk_chain_t *tkc,
                        ncx_module_t *mod,
                        dlq_hdr_t  *unionQ,
                        obj_template_t *parent,
                        grp_template_t *grp,
                        const xmlChar *defval,
                        const xmlChar *name)
{
    typ_unionnode_t *un = NULL;
    status_t         res = NO_ERR, retres = NO_ERR;
    ncx_btype_t      btyp = NCX_BT_NONE;
    boolean          defvaldone = FALSE;

    /* first resolve all the local type names */
    for (un = (typ_unionnode_t *)dlq_firstEntry(unionQ);
         un != NULL && retres == NO_ERR;
         un = (typ_unionnode_t *)dlq_nextEntry(un)) {

        res = resolve_type(pcb, tkc,  mod, un->typdef, NULL, 
                           NULL, parent, grp);
        CHK_EXIT(res, retres);

        btyp = typ_get_basetype(un->typdef);

        if (btyp != NCX_BT_NONE && !typ_ok_for_union(btyp)) {
            retres = ERR_NCX_WRONG_TYPE;
            log_error("\nError: builtin type '%s' not allowed"
                      " within a union", 
                      tk_get_btype_sym(btyp));
            tkc->curerr = &un->typdef->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        }


        /* check default value if any defined */
        if ((retres == NO_ERR) && 
            !defvaldone && defval &&
            (btyp != NCX_BT_NONE) &&
            typ_ok(un->typdef)) {

            res = check_defval(tkc, mod, un->typdef, parent, btyp, 
                               TRUE, defval, name);
            if (res == NO_ERR) {
                defvaldone = TRUE;
            }
        }

        if (un->typdef->tclass == NCX_CL_NAMED) {
            un->typ = un->typdef->def.named.typ;
            /* keep the typdef around for yangdump */
        }
    }

    /* only report error if defval passed caused the error
     * otherwise the resolve_type pass for member types in 
     * the union will print a typdef defualt error
     */
    if (retres == NO_ERR && defval != NULL && !defvaldone) {
        retres = ERR_NCX_INVALID_VALUE;
        if (parent != NULL) {
            log_error("\nError: Union %s '%s' has invalid "
                      "default value (%s)",
                      (name) ? "type" : 
                      (const char *)obj_get_typestr(parent),
                      (name) ? name : obj_get_name(parent), 
                      defval);
        } else {
            log_error("\nError: Union %s '%s' has invalid default value",
                      (name) ? "Type" : "leaf or leaf-list",
                      (name) ? (const char *)name : "--", 
                      defval);
        }
        if (parent) {
            tkc->curerr = &parent->tkerr;
        } else if (grp) {
            tkc->curerr = &grp->tkerr;
        }
        ncx_print_errormsg(tkc, mod, retres);
    }

    return retres;

}   /* resolve_union_type */


/********************************************************************
* FUNCTION resolve_union_type_final
* 
* Check for XPath defaults
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   unionQ == Q of unfinished typ_unionnode_t structs
*   parent == obj_template_t containing the union (may be NULL)
*   defval == default value, if any
*   name == typ_template_t name, if any
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    resolve_union_type_final (tk_chain_t *tkc,
                              ncx_module_t *mod,
                              dlq_hdr_t *unionQ,
                              obj_template_t *parent,
                              const xmlChar *defval,
                              const xmlChar *name)
{
    typ_unionnode_t *un;
    status_t         res = NO_ERR;
    ncx_btype_t      btyp = NCX_BT_NONE;
    boolean          needtest = FALSE, defvaldone = FALSE;

    if (!defval) {
        return NO_ERR;
    }

    /* check if this extra test is needed */
    for (un = (typ_unionnode_t *)dlq_firstEntry(unionQ);
         un != NULL && !needtest;
         un = (typ_unionnode_t *)dlq_nextEntry(un)) {

        if (!typ_ok(un->typdef)) {
            return NO_ERR;
        }

        btyp = typ_get_basetype(un->typdef);
        if (btyp == NCX_BT_LEAFREF || btyp == NCX_BT_UNION ||
            typ_is_xpath_string(un->typdef)) {
            needtest = TRUE;
        }
    }

    if (!needtest) {
        return NO_ERR;
    }

    /* run the extra test */
    for (un = (typ_unionnode_t *)dlq_firstEntry(unionQ);
         un != NULL;
         un = (typ_unionnode_t *)dlq_nextEntry(un)) {

        /* check default value if any defined */
        if (defval && !defvaldone && (btyp != NCX_BT_NONE)) {
            res = check_defval(tkc, mod, un->typdef, parent, btyp, TRUE, 
                               defval, name);
            if (res == NO_ERR) {
                defvaldone = TRUE;
            }
        }
    }

    /* only report error if defval passed caused the error
     * otherwise the resolve_type pass for member types in 
     * the union will print a typdef default error
     */
    if (defval && !defvaldone) {
        res = ERR_NCX_INVALID_VALUE;
        if (parent) {
            log_error("\nError: Union %s '%s' has invalid "
                      "default value (%s)",
                      (name) ? "type" : 
                      (const char *)obj_get_typestr(parent),
                      (name) ? name : obj_get_name(parent), 
                      defval);
        } else {
            log_error("\nError: Union %s '%s' has invalid default value",
                      (name) ? "Type" : "leaf or leaf-list",
                      (name) ? (const char *)name : "--", 
                      defval);
        }
        if (tkc) {
            if (parent) {
                tkc->curerr = &parent->tkerr;
            }
            ncx_print_errormsg(tkc, mod, res);
        }
    }

    return res;

}   /* resolve_union_type_final */



/*******************************************************************
* FUNCTION resolve_type
* 
* Analyze the typdef
* Finish if a named type, and/or range clauses present,
* which were left unfinished due to possible forward references
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typdef == typ_def_t struct to check
*   name == name of the typ_template that contains 'typdef'
*   defval == default value string to check (may be NULL)
*   obj == obj_template containing this typdef, NULL if top-level
*   grp == grp_template containing this typdef, otherwise NULL
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_type (yang_pcb_t *pcb,
                  tk_chain_t *tkc,
                  ncx_module_t  *mod,
                  typ_def_t *typdef,
                  const xmlChar *name,
                  const xmlChar *defval,
                  obj_template_t *obj,
                  grp_template_t *grp)
{
    typ_def_t        *testdef;
    typ_template_t   *errtyp;
    grp_template_t   *nextgrp;
    typ_enum_t       *enu;
    typ_idref_t      *idref;
    ncx_identity_t   *testidentity;
    const xmlChar    *errname = NULL;
    status_t         res = NO_ERR, retres = NO_ERR;
    boolean          errdone = FALSE;
    ncx_btype_t      btyp;

    if (typdef->tclass == NCX_CL_BASE) {
        return NO_ERR;
    }

    btyp = typ_get_basetype(typdef);

#ifdef YANG_TYP_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\nyang_typ: resolve type '%s' (name %s) on line %u",
                   (typdef->typenamestr) ? 
                   typdef->typenamestr : NCX_EL_NONE,
                   (name) ? name : NCX_EL_NONE,
                   typdef->tkerr.linenum);
    }
#endif

    /* check the appinfoQ */
    res = ncx_resolve_appinfoQ(pcb, tkc, mod, &typdef->appinfoQ);
    if (NEED_EXIT(res)) {
        return res;
    } else {
        res = NO_ERR;
    }

    /* first resolve all the local type names */
    if (res == NO_ERR && btyp != NCX_BT_UNION) {
        res = obj_set_named_type(tkc, mod, name, typdef, obj, grp);
    }

    /* type name loop check */
    if (res == NO_ERR && btyp != NCX_BT_UNION) {
        res = loop_test(typdef);
    }

    /* If no loops then make sure base type is correct */
    if (res == NO_ERR && btyp != NCX_BT_UNION) {
        res = restriction_test(tkc, mod, typdef);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    /* print any errors so far, and exit if the typdef may
     * not be stable enough to test further
     */
    if (res != NO_ERR) {
        if (!errdone) {
            tkc->curerr = &typdef->tkerr;
            ncx_print_errormsg(tkc, mod, res);
        }
        return res;
    }

    /* OK so far: safe to process the typdef further
     * go through again and resolve the range definitions
     * This is needed if min and max keywords used
     * Errors printed in the called fn
     */
    if (btyp != NCX_BT_UNION) {
        res = finish_yang_range(tkc, mod, typdef,
                                typ_get_range_type(typ_get_basetype(typdef)));
    }

    /* validate that the ranges are valid now that min/max is set 
     * and all parent ranges are also set.  A range must be valid
     * wrt/ its parent range definition(s)
     * Errors printed in the called fn
     */
    if (res == NO_ERR && btyp != NCX_BT_UNION) {
        if (!name && (typdef->tclass == NCX_CL_NAMED)) {
            /* name field just used for error messages */
            errname = typdef->typenamestr;
        } else {
            errname = name;
        }
        res = validate_range_chain(tkc, mod, typdef, errname);
    }

    /* check default value if any defined */
    if ((res == NO_ERR) &&
        (btyp != NCX_BT_NONE) && 
        (btyp != NCX_BT_UNION) && typ_ok(typdef)) {

        if (btyp == NCX_BT_LEAFREF || typ_is_xpath_string(typdef)) {
            if (LOGDEBUG4) {
                log_debug4("\nyang_typ: postponing default check "
                           "for type '%s'", 
                           (errname) ? errname : 
                           (typdef->typenamestr) ? 
                           typdef->typenamestr : NCX_EL_NONE);
            }
        } else {
            if (defval) {
                res = check_defval(tkc, mod, typdef, obj, btyp, FALSE, 
                                   defval, name);
            }
        }
    }

    /* check built-in type specific details */
    if (res == NO_ERR) {
        switch (btyp) {
        case NCX_BT_ENUM:
        case NCX_BT_BITS:
            /* check each enumeration appinfoQ */
            if (typdef->tclass == NCX_CL_SIMPLE) {
                for (enu = (typ_enum_t *)
                         dlq_firstEntry(&typdef->def.simple.valQ);
                     enu != NO_ERR;
                     enu = (typ_enum_t *)dlq_nextEntry(enu)) {

                    res = ncx_resolve_appinfoQ(pcb, tkc, mod, &enu->appinfoQ);
                    CHK_EXIT(res, retres);
                }
                res = NO_ERR;
            }
            break;
        case NCX_BT_IDREF:
            idref = typ_get_idref(typdef);
            if (idref) {
                testidentity = NULL;
                if (idref->baseprefix &&
                    xml_strcmp(idref->baseprefix, mod->prefix)) {

                    /* find the identity in another module */
                    res = yang_find_imp_identity(pcb, tkc, mod, 
                                                 idref->baseprefix,
                                                 idref->basename, 
                                                 &typdef->tkerr,
                                                 &testidentity);
                } else {
                    testidentity = ncx_find_identity(mod, idref->basename, 
                                                     FALSE);
                    if (!testidentity) {
                        res = ERR_NCX_DEF_NOT_FOUND;
                    } else {
                        idref->base = testidentity;
                    }
                }
                idref->base = testidentity;
                if (res != NO_ERR) {
                    log_error("\nError: identityref '%s' has invalid "
                              "base value (%s)",
                              (name) ? (const char *)name : "--", 
                              idref->basename);
                    tkc->curerr = &typdef->tkerr;
                    ncx_print_errormsg(tkc, mod, res);
                }
            } else {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
            break;
        default:
            ;
        }
    }

    /* special check for union typdefs, errors printed by called fn */
    if (res == NO_ERR &&
        typdef->tclass == NCX_CL_SIMPLE &&  
        btyp == NCX_BT_UNION) {
        testdef = typ_get_base_typdef(typdef);
        res = resolve_union_type(pcb, tkc, mod, &testdef->def.simple.unionQ,
                                 obj, grp, defval, name);
    }

    /*  check shadow typedef name error, even if other errors so far */
    if (name != NULL) {
        errtyp = NULL;

        /* if object directly within a grouping, then need
         * to make sure that parent groupings do not contain
         * this type definition
         */
        if (obj && obj->grp && obj->grp->parentgrp) {
            nextgrp = obj->grp->parentgrp;
            while (nextgrp) {
                errtyp = ncx_find_type_que(&nextgrp->typedefQ, name);
                if (errtyp) {
                    nextgrp = NULL;
                } else {
                    nextgrp = nextgrp->parentgrp;
                }
            }
        }

        /* check local typedef shadowed further up the chain */
        if (!errtyp && obj && obj->parent && !obj_is_root(obj->parent)) {
            errtyp = obj_find_type(obj->parent, name);
        }

        /* check module-global (exportable) typedef shadowed
         * only check for nested typedefs
         */
        if (!errtyp && obj ) {
            errtyp = ncx_find_type(mod, name, TRUE);
        }

        if (errtyp) {
            res = ERR_NCX_DUP_ENTRY;
            log_error("\nError: local typedef %s shadows "
                      "definition in %s on line %u", 
                      name,
                      errtyp->tkerr.mod->name,
                      errtyp->tkerr.linenum);
            tkc->curerr = &typdef->tkerr;
            ncx_print_errormsg(tkc, mod, res);
        }
    }

    return (retres != NO_ERR) ? retres : res;

}  /* resolve_type */

/*******************************************************************
* FUNCTION resolve_type_final
* 
* Finish checking default values
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   obj == object containing this type (may be NULL)
*   typdef == typ_def_t struct to check
*   defval == default value string to check (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_type_final (tk_chain_t *tkc,
                        ncx_module_t  *mod,
                        obj_template_t *obj,
                        typ_def_t *typdef,
                        const xmlChar *defval)
{
    const xmlChar *typname;
    xpath_pcb_t   *pcb;
    obj_template_t *leafobj;
    status_t       res = NO_ERR;
    ncx_btype_t    btyp;
    
    if (!defval || !typ_ok(typdef) || typdef->tclass == NCX_CL_BASE ) {
        /* skip this test if typdef malformed */
        return res;
    }

    btyp = typ_get_basetype(typdef);

    if ((btyp != NCX_BT_LEAFREF) && (btyp != NCX_BT_UNION) && 
        !typ_is_xpath_string(typdef) ) {
        return res;
    }

    typname = (typdef->typenamestr) ? typdef->typenamestr : NCX_EL_NONE;

    log_debug4("\nyang_typ: resolve type final '%s'", typname);

    /* check default value if any defined */
    if (btyp == NCX_BT_LEAFREF || btyp == NCX_BT_INSTANCE_ID) {
        pcb = typ_get_leafref_pcb(typdef);
        if (tkc) {
            tkc->curerr = &pcb->tkerr;
        }

        leafobj = NULL;
        if (!obj) {
            obj = ncx_get_gen_root();
        }
        
        res = xpath_yang_validate_path(mod, obj, pcb, FALSE, &leafobj);
        if (res == NO_ERR && leafobj != NULL) {
            typ_set_xref_typdef(typdef, obj_get_typdef(leafobj));
            if (obj->objtype == OBJ_TYP_LEAF) {
                obj->def.leaf->leafrefobj = leafobj;
            } else {
                obj->def.leaflist->leafrefobj = leafobj;
            }
        }
    }

    if (res != NO_ERR) {
        return res;
    }
    
    if (btyp == NCX_BT_UNION) {
        if (defval) {
            res = check_defval(tkc, mod, typdef, obj, btyp, FALSE, defval, 
                               typname);
        }
    } else if (typdef->tclass == NCX_CL_SIMPLE) {
        /* special check for union typdefs, errors printed by called fn */
        res = resolve_union_type_final(tkc, mod, &typdef->def.simple.unionQ,
                                       obj, defval,
                                       (obj) ? obj_get_name(obj) : NULL);
        
    }
    
    return res;

}  /* resolve_type_final */


/********************************************************************
* FUNCTION resolve_typedef
* 
* Analyze the typdef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typ == typ_template struct to check
*   obj == obj_template containing this typdef, NULL if top-level
*   grp == grp_template containing this typdef, NULL if none
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_typedef (yang_pcb_t *pcb,
                     tk_chain_t *tkc,
                     ncx_module_t  *mod,
                     typ_template_t *typ,
                     obj_template_t *obj,
                     grp_template_t *grp)
{
    status_t   res, retres = NO_ERR;

    /* check the appinfoQ */
    typ->res = res = ncx_resolve_appinfoQ(pcb, tkc, mod, &typ->appinfoQ);
    CHK_EXIT(res, retres);

    typ->res = res = resolve_type(pcb, tkc, mod, &typ->typdef, typ->name,
                                  typ->defval, obj, grp);
    CHK_EXIT(res, retres);

    return retres;

}  /* resolve_typedef */


/********************************************************************
* FUNCTION resolve_typedef_final
* 
* Analyze the typdef (check defaults for XPath types)
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typ == typ_template struct to check
*   obj == obj_template containing this typdef, NULL if top-level
*   grp == grp_template containing this typdef, NULL if none
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    resolve_typedef_final (tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           typ_template_t *typ)
{
    status_t   res;

    if (typ->res != NO_ERR) {
        return NO_ERR;
    }
    typ->res = res = resolve_type_final(tkc, mod, NULL,  &typ->typdef, 
                                        typ->defval);
    return res;

}  /* resolve_typedef_final */


/********************************************************************
* FUNCTION consume_type
* 
* Parse the next N tokens as a type clause
* Add to the typ_template_t struct in progress
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'type' keyword
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod   == module in progress
*   intypdef == struct that will get the type info
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_type (yang_pcb_t *pcb,
                  tk_chain_t *tkc,
                  ncx_module_t  *mod,
                  typ_def_t *intypdef,
                  boolean metamode)
{
    const char     *expstr;
    typ_template_t *imptyp;
    typ_def_t      *typdef;
    ncx_btype_t     btyp;
    boolean         extonly, derived;
    status_t        res, retres;

    expstr = "type name";
    imptyp = NULL;
    extonly = FALSE;
    derived = FALSE;
    res = NO_ERR;
    retres = NO_ERR;
    btyp = NCX_BT_NONE;

    ncx_set_error(&intypdef->tkerr, mod, TK_CUR_LNUM(tkc), TK_CUR_LPOS(tkc));

    /* Get the mandatory base type */
    res = yang_consume_pid_string(tkc, 
                                  mod, 
                                  &intypdef->prefix,
                                  &intypdef->typenamestr);
    CHK_EXIT(res, retres);

    /* got ID and prefix if there is one */
    if (intypdef->prefix && intypdef->typenamestr &&
        xml_strcmp(intypdef->prefix, mod->prefix)) {
        /* real import - not the same as this module's prefix */
        res = yang_find_imp_typedef(pcb,
                                    tkc, 
                                    mod,
                                    intypdef->prefix,
                                    intypdef->typenamestr,
                                    NULL,
                                    &imptyp);
        if (res != NO_ERR) {
            CHK_EXIT(res, retres);
            /* type not found but continue processing errors for now */
            typ_init_named(intypdef);
        } else {
            /* found named type OK, setup typdef */
            typ_init_named(intypdef);
            btyp = typ_get_basetype(&imptyp->typdef);
            typ_set_named_typdef(intypdef, imptyp);
        }
    } else if (intypdef->typenamestr) {
        /* there was no real prefix, so try to resolve this ID
         * as a builtin type, else save as a local type
         * btyp == NCX_BT_NONE indicates a local type
         */
        btyp = tk_get_yang_btype_id(intypdef->typenamestr,
                                    xml_strlen(intypdef->typenamestr));
        if (btyp != NCX_BT_NONE) {
            /* base type is builtin type */
            typ_init_simple(intypdef, btyp);
        } else {
            /* base type local named type */
            typ_init_named(intypdef);
        }
    } else {
        /* not even a typename to work with, error already set */
        typ_init_named(intypdef);
    }   
                
    /* check for ending semi-colon or starting left brace */
    if (!metamode) {
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }
    }

    if (TK_CUR_TYP(tkc) == TK_TT_SEMICOL || metamode) {
        /* got the type name and that's all.
         * Check if a named imported type is given
         * or a builtin type name, and gen an error
         * if any subclauses are expected
         */
        if (imptyp) {
            return NO_ERR;
        }

        /* check if builtin type and missing data */
        switch (btyp) {
        case NCX_BT_BITS:
            retres = ERR_NCX_MISSING_TYPE;
            expstr = "bit definitions";
            break;
        case NCX_BT_ENUM:
            retres = ERR_NCX_MISSING_TYPE;
            expstr = "enum definitions";
            break;
        case NCX_BT_UNION:
            retres = ERR_NCX_MISSING_TYPE;
            expstr = "union member definitions";
            break;
        case NCX_BT_LEAFREF:
            retres = ERR_NCX_MISSING_TYPE;
            expstr = "path specifier";
            break;
        case NCX_BT_NONE:
            if (metamode && typ_get_basetype(intypdef) == NCX_BT_NONE) {
                retres = obj_set_named_type(tkc, 
                                            mod, 
                                            NULL,
                                            intypdef, 
                                            NULL, 
                                            NULL);
            }
            return retres;
        default:
            if (btyp == NCX_BT_INSTANCE_ID) {
                intypdef->def.simple.constrained = TRUE;
            }
            return NO_ERR;
        }
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        return retres;
    } else if (TK_CUR_TYP(tkc) != TK_TT_LBRACE) {
        retres = ERR_NCX_WRONG_TKTYPE;
        ncx_mod_exp_err(tkc, mod, retres, expstr);
        return retres;
    }

    /* got left brace, get the type sub-section clauses
     * adding restrictions to an imported type or
     * to a local named type (btyp not set yet)
     */
    if (imptyp || btyp == NCX_BT_NONE) {
        res = typ_set_new_named(intypdef, btyp);
        if (res == NO_ERR) {
            typdef = typ_get_new_named(intypdef);
            derived = TRUE;
            typdef->def.simple.flags |= TYP_FL_REPLACE;
        } else {
            ncx_print_errormsg(tkc, mod, res);
            return res;
        }
    } else {
        typdef = intypdef;
    }


    /* check for hard-wired builtin type sub-sections */
    switch (btyp) {
    case NCX_BT_NONE:
        /* extending a local type that has not been resolved yet */
        res = finish_unknown_type(tkc, mod, typdef);
        CHK_EXIT(res, retres);
        break;
    case NCX_BT_ANY:
        extonly = TRUE;
        break;
    case NCX_BT_BITS:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_bits_type(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        }
        break;
    case NCX_BT_ENUM:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_enum_type(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        }
        break;
    case NCX_BT_EMPTY:
        extonly = TRUE;
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        res = finish_number_type(tkc, mod, btyp, typdef);
        CHK_EXIT(res, retres);
        break;
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
        res = finish_string_type(tkc, mod, typdef, btyp);
        CHK_EXIT(res, retres);
        break;
    case NCX_BT_UNION:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_union_type(pcb,
                                    tkc, 
                                    mod, 
                                    typdef);
            CHK_EXIT(res, retres);
        }
        break;
    case NCX_BT_LEAFREF:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_leafref_type(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        }
        break;
    case NCX_BT_IDREF:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_idref_type(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        }
        break;
    case NCX_BT_INSTANCE_ID:
        if (derived) {
            extonly = TRUE;
        } else {
            res = finish_instanceid_type(tkc, mod, typdef);
            CHK_EXIT(res, retres);
        }
        break;
    default:
        retres = SET_ERROR(ERR_INTERNAL_VAL);
        ncx_print_errormsg(tkc, mod, retres);
    }

    /* check if this sub-section is only allowed
     * to have extensions in it; no YANG statements
     */
    if (extonly) {
        /* give back left brace and get the appinfo */
        TK_BKUP(tkc);  
        res = yang_consume_semiapp(tkc, mod, &typdef->appinfoQ);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* consume_type */


/**************  E X T E R N A L   F U N C T I O N S  **************/


/********************************************************************
* FUNCTION yang_typ_consume_type
* 
* Parse the next N tokens as a type clause
* Add to the typ_template_t struct in progress
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'type' keyword
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod   == module in progress
*   intypdef == struct that will get the type info
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_consume_type (yang_pcb_t *pcb,
                           tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           typ_def_t *intypdef)
{
#ifdef DEBUG
    if (!tkc || !mod || !intypdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return consume_type(pcb,
                        tkc, 
                        mod, 
                        intypdef, 
                        FALSE);

}  /* yang_typ_consume_type */


/********************************************************************
* FUNCTION yang_typ_consume_metadata_type
* 
* Parse the next token as a type declaration
* for an ncx:metadata definition
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod   == module in progress
*   intypdef == struct that will get the type info
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t yang_typ_consume_metadata_type( yang_pcb_t *pcb,
                                         tk_chain_t *tkc,
                                         ncx_module_t  *mod,
                                         typ_def_t *intypdef )
{
    assert ( pcb && "pcb is NULL" );
    assert ( tkc && "tkc is NULL" );
    assert ( mod && "mod is NULL" );
    assert ( intypdef && "intypdef is NULL" );

    return consume_type(pcb, tkc, mod, intypdef, TRUE);
}  /* yang_typ_consume_metadata_type */


/********************************************************************
* FUNCTION yang_typ_consume_typedef
* 
* Parse the next N tokens as a typedef clause
* Create a typ_template_t struct and add it to the specified module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'typedef' keyword
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod == module in progress
*   que == queue will get the typ_template_t 
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_consume_typedef (yang_pcb_t *pcb,
                              tk_chain_t *tkc,
                              ncx_module_t  *mod,
                              dlq_hdr_t *que)
{
    typ_template_t  *typ, *testtyp;
    const xmlChar   *val;
    const char      *expstr;
    yang_stmt_t     *stmt;
    tk_type_t        tktyp;
    boolean          done, typdone, typeok, unit, def, stat, desc, ref;
    status_t         res, retres;

    typ = NULL;
    testtyp = NULL;
    val = NULL;
    expstr = "identifier string";
    tktyp = TK_TT_NONE;
    done = FALSE;
    typdone = FALSE;
    typeok = FALSE;
    unit = FALSE;
    def = FALSE;
    stat = FALSE;
    desc = FALSE;
    ref = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

    /* Get a new typ_template_t to fill in */
    typ = typ_new_template();
    if (!typ) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    } else {
        ncx_set_error(&typ->tkerr, mod, TK_CUR_LNUM(tkc), TK_CUR_LPOS(tkc));
    }

    /* Get the mandatory type name */
    res = yang_consume_id_string(tkc, mod, &typ->name);
    if ( ( NO_ERR != res && res < ERR_LAST_SYS_ERR ) || res == ERR_NCX_EOF ) {
        typ_free_template(typ);
        return res;
    }


    /* Get the starting left brace for the sub-clauses */
    res = ncx_consume_token(tkc, mod, TK_TT_LBRACE);
    if ( ( NO_ERR != res && res < ERR_LAST_SYS_ERR ) || res == ERR_NCX_EOF ) {
        typ_free_template(typ);
        return res;
    }

    /* get the prefix clause and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            typ_free_template(typ);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            typ_free_template(typ);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead
             * Note that typ appinfo is saved in typ.type appinfo
             * so leaf processing code can access it in the typdef
             * instead of needed a back pointer access to the 
             * typ_template_t
             */
            res = ncx_consume_appinfo(tkc, mod, &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
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
            expstr = "keyword";
            res = ERR_NCX_WRONG_TKTYPE;
            ncx_mod_exp_err(tkc, mod, res, expstr);
            continue;
        }

        /* Got a token string so check the value */
        if (!xml_strcmp(val, YANG_K_TYPE)) {
            if (typdone) {
                retres = ERR_NCX_ENTRY_EXISTS;
                typeok = FALSE;
                log_error("\nError: type-stmt already entered");
                ncx_print_errormsg(tkc, mod, res);
                typ_clean_typdef(&typ->typdef);
            }
            typdone = TRUE;
            res = yang_typ_consume_type(pcb,
                                        tkc, 
                                        mod, 
                                        &typ->typdef);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            } else {
                typeok = TRUE;
            }
        } else if (!xml_strcmp(val, YANG_K_UNITS)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &typ->units,
                                         &unit, 
                                         &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_DEFAULT)) {
            res = yang_consume_strclause(tkc, 
                                         mod, 
                                         &typ->defval,
                                         &def, 
                                         &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &typ->status,
                                      &stat, 
                                      &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &typ->descr,
                                     &desc, 
                                     &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            }
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &typ->ref,
                                     &ref, 
                                     &typ->typdef.appinfoQ);
            if (res != NO_ERR) {
                retres = res;
                if (NEED_EXIT(res)) {
                    typ_free_template(typ);
                    return res;
                }
            }
        } else {
            expstr = "keyword";
            retres = ERR_NCX_WRONG_TKVAL;
            ncx_mod_exp_err(tkc, mod, retres, expstr);
        }
    }

    /* check all the mandatory clauses are present */
    if (!typdone) {
        retres = ERR_NCX_DATA_MISSING;
        ncx_mod_missing_err(tkc, 
                            mod, 
                            "typedef", 
                            "type");
    }

    /* check if the type name is already used in this module */
    if (typ->name && ncx_valid_name2(typ->name)) {
        testtyp = ncx_find_type_que(que, typ->name);
        if (testtyp == NULL) {
            testtyp = ncx_find_type(mod, typ->name, TRUE);
        }
        if (testtyp) {
            /* fatal error for duplicate type w/ same name */
            retres = ERR_NCX_DUP_ENTRY;
            log_error("\nError: type '%s' is already defined in '%s' "
                      "on line %u",
                      testtyp->name, 
                      testtyp->tkerr.mod->name,
                      testtyp->tkerr.linenum);
            tkc->curerr = &typ->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
            typ_free_template(typ);
        } else if (typeok) {
#ifdef YANG_TYP_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nyang_typ: adding type (%s) to mod (%s)", 
                           typ->name, 
                           mod->name);
            }
#endif
            dlq_enque(typ, que);

            if (mod->stmtmode && que==&mod->typeQ) {
                /* save stmt for top-level typedefs only */
                stmt = yang_new_typ_stmt(typ);
                if (stmt) {
                    dlq_enque(stmt, &mod->stmtQ);
                } else {
                    log_error("\nError: malloc failure for typ_stmt");
                    retres = ERR_INTERNAL_MEM;
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        } else {
            typ_free_template(typ);
        }           
    } else {
        typ_free_template(typ);
    }

    return retres;

}  /* yang_typ_consume_typedef */


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs
* 
* Analyze the entire typeQ within the module struct
* Finish all the named types and range clauses,
* which were left unfinished due to possible forward references
*
* Check all the types in the Q
* If a type has validation errors, it will be removed
* from the typeQ
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_resolve_typedefs (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
                               ncx_module_t  *mod,
                               dlq_hdr_t *typeQ,
                               obj_template_t *parent)
{
    typ_template_t  *typ;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !typeQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;

    /* first resolve all the local type names */
    for (typ = (typ_template_t *)dlq_firstEntry(typeQ);
         typ != NULL;
         typ = (typ_template_t *)dlq_nextEntry(typ)) {

        res = resolve_typedef(pcb,
                              tkc, 
                              mod, 
                              typ, 
                              parent, 
                              NULL);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* yang_typ_resolve_typedefs */


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs_final
* 
* Analyze the entire typeQ within the module struct
* Finish all default value checking that was not done before
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_resolve_typedefs_final (tk_chain_t *tkc,
                                     ncx_module_t *mod,
                                     dlq_hdr_t *typeQ)
{
    typ_template_t  *typ;
    status_t         res, retres;

#ifdef DEBUG
    if (!tkc || !mod || !typeQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;

    /* first resolve all the local type names */
    for (typ = (typ_template_t *)dlq_firstEntry(typeQ);
         typ != NULL;
         typ = (typ_template_t *)dlq_nextEntry(typ)) {

        res = resolve_typedef_final(tkc, mod, typ);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* yang_typ_resolve_typedefs_final */


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs_grp
* 
* Analyze the entire typeQ within the module struct
* Finish all the named types and range clauses,
* which were left unfinished due to possible forward references
*
* Check all the types in the Q
* If a type has validation errors, it will be removed
* from the typeQ
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*   grp == grp_template containing this typedef
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_resolve_typedefs_grp (yang_pcb_t *pcb,
                                   tk_chain_t *tkc,
                                   ncx_module_t  *mod,
                                   dlq_hdr_t *typeQ,
                                   obj_template_t *parent,
                                   grp_template_t *grp)
{
    typ_template_t  *typ;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !typeQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;

    /* first resolve all the local type names */
    for (typ = (typ_template_t *)dlq_firstEntry(typeQ);
         typ != NULL;
         typ = (typ_template_t *)dlq_nextEntry(typ)) {

        res = resolve_typedef(pcb, tkc, mod, typ, parent, grp);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* yang_typ_resolve_typedefs_grp */


/********************************************************************
* FUNCTION yang_typ_resolve_type
* 
* Analyze the typdef within a single leaf or leaf-list statement
* Finish if a named type, and/or range clauses present,
* which were left unfinished due to possible forward references
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typdef == typdef struct from leaf or leaf-list to check
*   defval == default value string for this leaf (may be NULL)
*   obj == obj_template containing this typdef
*       == NULL if this is a top-level union typedef,
*          checking its nested unnamed type clauses
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_resolve_type (yang_pcb_t *pcb,
                           tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           typ_def_t *typdef,
                           const xmlChar *defval,
                           obj_template_t *obj)
{
    status_t         res;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = resolve_type(pcb, tkc, mod, typdef, NULL, defval, obj, NULL);
    return res;

}  /* yang_typ_resolve_type */


/********************************************************************
* FUNCTION yang_typ_resolve_type_final
* 
* Check deferred XPath resolution tests
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typdef == typdef struct from leaf or leaf-list to check
*   defval == default value string for this typedef or leaf
*   obj == obj_template containing this typdef
*       == NULL if this is a top-level union typedef,
*          checking its nested unnamed type clauses
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_resolve_type_final (tk_chain_t *tkc,
                                 ncx_module_t *mod,
                                 typ_def_t *typdef,
                                 const xmlChar *defval,
                                 obj_template_t *obj)
{
    status_t         res;

#ifdef DEBUG
    if (!tkc || !mod || !typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = resolve_type_final(tkc, mod, obj, typdef, defval);
    return res;

}  /* yang_typ_resolve_type_final */


/********************************************************************
* FUNCTION yang_typ_rangenum_ok
* 
* Check a typdef for range definitions
* and check if the specified number passes all
* the range checks (if any)
*
* INPUTS:
*   typdef == typdef to check
*   num == number to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_typ_rangenum_ok (typ_def_t *typdef,
                          const ncx_num_t *num)
{
    typ_def_t   *rdef;
    status_t     res;
    ncx_btype_t  rbtyp;

    res = NO_ERR;
    rbtyp = typ_get_range_type(typ_get_basetype(typdef));

    rdef = typ_get_qual_typdef(typdef, NCX_SQUAL_RANGE);
    while (rdef && res==NO_ERR) {
        res = val_range_ok(rdef, rbtyp, num);
        if (res != NO_ERR) {
            continue;
        }
        rdef = typ_get_parent_typdef(rdef);
        if (rdef) {
            rdef = typ_get_qual_typdef(rdef, NCX_SQUAL_RANGE);
        }
    }

    return res;
    
} /* yang_typ_rangenum_ok */

/* END file yang_typ.c */
