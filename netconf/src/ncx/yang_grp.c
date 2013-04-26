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
/*  FILE: yang_grp.c


    YANG module parser, grouping statement support


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
09dec07      abb      begun; start from yang_typ.c


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

#ifndef _H_obj
#include "obj.h"
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

#ifndef _H_yang_grp
#include "yang_grp.h"
#endif

#ifndef _H_yang_obj
#include "yang_obj.h"
#endif

#ifndef _H_yang_typ
#include "yang_typ.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define YANG_GRP_DEBUG 1
/* #define YANG_GRP_USES_DEBUG 1 */
#endif


/********************************************************************
*                                                                   *
*                          M A C R O S                              *
*                                                                   *
*********************************************************************/

/* used in parser routines to decide if processing can continue
 * will exit the function if critical error or continue if not
 */
#define CHK_GRP_EXIT(res, retres)                         \
    if (res != NO_ERR) {                                  \
        if (res < ERR_LAST_SYS_ERR || res==ERR_NCX_EOF) { \
            grp_free_template(grp);                       \
            return res;                                   \
        } else {                                          \
            retres = res;                                 \
        }                                                 \
    }


/********************************************************************
* FUNCTION follow_loop
* 
* Follow the uses chain, looking for a loop
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   grp == grp_template_t that this 'obj' is using
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    follow_loop (tk_chain_t *tkc,
                 ncx_module_t  *mod,
                 grp_template_t *grp,
                 grp_template_t *testgrp)
{
    obj_template_t  *testobj;
    status_t         res, retres;

    retres = NO_ERR;

    for (testobj = (obj_template_t *)dlq_firstEntry(&grp->datadefQ);
         testobj != NULL;
         testobj = (obj_template_t *)dlq_nextEntry(testobj)) {

        if (testobj->objtype != OBJ_TYP_USES) {
            continue;
        }

        if (testobj->def.uses->grp == testgrp) {
            log_error("\nError: grouping '%s'"
                      " on line %u loops in uses, defined "
                      "in module %s, line %u",
                      testgrp->name, 
                      testgrp->tkerr.linenum,
                      testobj->tkerr.mod->name,
                      testobj->tkerr.linenum);
            retres = ERR_NCX_DEF_LOOP;
            tkc->curerr = &testgrp->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
        } else if (testobj->def.uses->grp) {
            res = follow_loop(tkc, mod,
                              testobj->def.uses->grp, testgrp);
            CHK_EXIT(res, retres);
        }
    }

    return retres;

}  /* follow_loop */


/********************************************************************
* FUNCTION check_chain_loop
* 
* Check the 'group' object and determine if there is a chained loop
* where another group eventually uses the group that started
* the test.
*
*    grouping A {
*      uses B;
*    }
*
*    grouping B {
*      uses C;
*    }
*
*    grouping C {
*      uses A;
*   }
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   obj == 'uses' obj_template containing the ref to 'grp'
*   grp == grp_template_t that this 'obj' is using
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    check_chain_loop (tk_chain_t *tkc,
                      ncx_module_t  *mod,
                      grp_template_t *grp)
{
    status_t         res;

#ifdef DEBUG
    if (!tkc || !grp) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = follow_loop(tkc, mod, grp, grp);
    return res;

}  /* check_chain_loop */


/***************   E X T E R N A L   F U N C T I O N S   ***********/


/********************************************************************
* FUNCTION yang_grp_consume_grouping
* 
* 2nd pass parsing
* Parse the next N tokens as a grouping-stmt
* Create a grp_template_t struct and add it to the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'grouping' keyword
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == module in progress
*   que == queue will get the grp_template_t 
*   parent == parent object or NULL if top-level grouping-stmt
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_grp_consume_grouping (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
                               ncx_module_t  *mod,
                               dlq_hdr_t *que,
                               obj_template_t *parent)
{
    grp_template_t  *grp, *testgrp;
    typ_template_t  *testtyp;
    const xmlChar   *val;
    const char      *expstr;
    yang_stmt_t     *stmt;
    tk_type_t        tktyp;
    boolean          done, stat, desc, ref;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !que) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    grp = NULL;
    testgrp = NULL;
    val = NULL;
    expstr = "keyword";
    done = FALSE;
    stat = FALSE;
    desc = FALSE;
    ref = FALSE;
    res = NO_ERR;
    retres = NO_ERR;

    /* Get a new grp_template_t to fill in */
    grp = grp_new_template();
    if (!grp) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    ncx_set_error(&grp->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    grp->parent = parent;
    grp->istop = (que == &mod->groupingQ) ? TRUE : FALSE;

    /* Get the mandatory grouping name */
    res = yang_consume_id_string(tkc, mod, &grp->name);
    CHK_GRP_EXIT(res, retres);

    /* Get the starting left brace for the sub-clauses
     * or a semi-colon to end the grouping clause
     */
    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, mod, res);
        grp_free_template(grp);
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

    /* get the grouping statements and any appinfo extensions */
    while (!done) {
        /* get the next token */
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            ncx_print_errormsg(tkc, mod, res);
            grp_free_template(grp);
            return res;
        }

        tktyp = TK_CUR_TYP(tkc);
        val = TK_CUR_VAL(tkc);

        /* check the current token type */
        switch (tktyp) {
        case TK_TT_NONE:
            res = ERR_NCX_EOF;
            ncx_print_errormsg(tkc, mod, res);
            grp_free_template(grp);
            return res;
        case TK_TT_MSTRING:
            /* vendor-specific clause found instead */
            res = ncx_consume_appinfo(tkc, mod, &grp->appinfoQ);
            CHK_GRP_EXIT(res, retres);
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
        if (!xml_strcmp(val, YANG_K_TYPEDEF)) {
            res = yang_typ_consume_typedef(pcb,
                                           tkc, 
                                           mod,
                                           &grp->typedefQ);
        } else if (!xml_strcmp(val, YANG_K_GROUPING)) {
            res = yang_grp_consume_grouping(pcb,
                                            tkc, 
                                            mod,
                                            &grp->groupingQ, 
                                            parent);
        } else if (!xml_strcmp(val, YANG_K_STATUS)) {
            res = yang_consume_status(tkc, 
                                      mod, 
                                      &grp->status,
                                      &stat, 
                                      &grp->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_DESCRIPTION)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &grp->descr,
                                     &desc, 
                                     &grp->appinfoQ);
        } else if (!xml_strcmp(val, YANG_K_REFERENCE)) {
            res = yang_consume_descr(tkc, 
                                     mod, 
                                     &grp->ref,
                                     &ref, 
                                     &grp->appinfoQ);
        } else {
            res = yang_obj_consume_datadef_grp(pcb,
                                               tkc, 
                                               mod,
                                               &grp->datadefQ,
                                               parent, 
                                               grp);
        }
        CHK_GRP_EXIT(res, retres);
    }

    /* save or delete the grp_template_t struct */
    if (grp->name && ncx_valid_name2(grp->name)) {
        boolean errone;

        testgrp = ncx_find_grouping_que(que, grp->name);
        if (testgrp == NULL) {
            errone = FALSE;
            testgrp = ncx_find_grouping(mod, grp->name, TRUE);
        } else {
            errone = TRUE;
        }

        if (testgrp) {
            if (errone) {
                log_error("\nError: grouping '%s' already "
                          "defined at line %u",
                          testgrp->name, 
                          testgrp->tkerr.linenum);
            } else {
                log_error("\nError: grouping '%s' already "
                          "defined in '%s' at line %u",
                          testgrp->name,
                          testgrp->tkerr.mod->name, 
                          testgrp->tkerr.linenum);
            }
            retres = ERR_NCX_DUP_ENTRY;
            ncx_print_errormsg(tkc, mod, retres);
            grp_free_template(grp);
        } else {
            for (testgrp = (grp_template_t *)
                     dlq_firstEntry(&grp->groupingQ);
                 testgrp != NULL;
                 testgrp = (grp_template_t *)dlq_nextEntry(testgrp)) {
                testgrp->parentgrp = grp;
            }
            for (testtyp = (typ_template_t *)
                     dlq_firstEntry(&grp->typedefQ);
                 testtyp != NULL;
                 testtyp = (typ_template_t *)dlq_nextEntry(testtyp)) {
                testtyp->grp = grp;
            }
#ifdef YANG_GRP_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nyang_grp: adding grouping (%s) to mod (%s)", 
                           grp->name, 
                           mod->name);
            }
#endif
            dlq_enque(grp, que);  /* may have some errors */

            if (mod->stmtmode && que==&mod->groupingQ) {
                /* save stmt record for top-level groupings only */
                stmt = yang_new_grp_stmt(grp);
                if (stmt) {
                    dlq_enque(stmt, &mod->stmtQ);
                } else {
                    log_error("\nError: malloc failure for grp_stmt");
                    retres = ERR_INTERNAL_MEM;
                    ncx_print_errormsg(tkc, mod, retres);
                }
            }
        }
    } else {
        grp_free_template(grp);
    }

    return retres;

}  /* yang_grp_consume_grouping */


/********************************************************************
* FUNCTION yang_grp_resolve_groupings
* 
* 3rd pass parsing
* Analyze the entire 'groupingQ' within the module struct
* Finish all the clauses within this struct that
* may have been defered because of possible forward references
*
* Any uses or augment within the grouping is deferred
* until later passes because of forward references
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*   parent == obj_template containing this groupingQ
*          == NULL if this is a module-level groupingQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_grp_resolve_groupings (yang_pcb_t *pcb,
                                tk_chain_t *tkc,
                                ncx_module_t  *mod,
                                dlq_hdr_t *groupingQ,
                                obj_template_t *parent)
{
    grp_template_t  *grp, *nextgrp, *errgrp;
    obj_template_t  *testobj;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !groupingQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    retres = NO_ERR;

    /* go through the groupingQ and check the local types/groupings
     * and data-def statements
     */
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

#ifdef YANG_GRP_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nyang_grp: resolve grouping '%s'",
                       (grp->name) ? grp->name : EMPTY_STRING);
        }
#endif
        /* check the appinfoQ */
        res = ncx_resolve_appinfoQ(pcb,
                                   tkc, 
                                   mod, 
                                   &grp->appinfoQ);
        CHK_EXIT(res, retres);

        /* check any local typedefs */
        res = yang_typ_resolve_typedefs_grp(pcb,
                                            tkc, 
                                            mod,
                                            &grp->typedefQ,
                                            parent, 
                                            grp);
        CHK_EXIT(res, retres);

        /* check any local groupings */
        res = yang_grp_resolve_groupings(pcb,
                                         tkc, 
                                         mod,
                                         &grp->groupingQ, 
                                         parent);
        CHK_EXIT(res, retres);

        /* check any local objects */
        res = yang_obj_resolve_datadefs(pcb,
                                        tkc, 
                                        mod, 
                                        &grp->datadefQ);
        CHK_EXIT(res, retres);
    }

    /* go through and check grouping shadow error
     * this cannot be checked until the previous loop completes
     */
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

        res = NO_ERR;
        errgrp = NULL;
        nextgrp = grp->parentgrp;
        while (nextgrp && res==NO_ERR) {
            if (!xml_strcmp(nextgrp->name, grp->name)) {
                res = ERR_NCX_DUP_ENTRY;
                errgrp = nextgrp;
            } else if (&nextgrp->groupingQ != groupingQ) {
                /* check local Q if it is at least 2 layers up */
                errgrp = ncx_find_grouping_que(&nextgrp->groupingQ,
                                               grp->name);
                if (errgrp) {
                    res = ERR_NCX_DUP_ENTRY;
                }
            }
            nextgrp = nextgrp->parentgrp;
        }

        if (res != NO_ERR) {
            log_error("\nError: local grouping '%s' shadows"
                      " definition on line %u",
                      grp->name,
                      errgrp->tkerr.linenum);
            tkc->curerr = &grp->tkerr;
            ncx_print_errormsg(tkc, mod, res);
        } else if (parent) {
            testobj = parent->parent;
            if (testobj) {
                nextgrp = obj_find_grouping(testobj, grp->name);
                if (nextgrp) {
                    log_error("\nError: local grouping '%s' shadows"
                              " definition on line %u",
                              grp->name,
                              nextgrp->tkerr.linenum);
                    tkc->curerr = &grp->tkerr;
                    ncx_print_errormsg(tkc, mod, res);
                }
            }
        }

        /* check nested groupings for module-level conflict */
        if (grp->parent) {
            nextgrp = ncx_find_grouping(mod, grp->name, TRUE);
            if (nextgrp) {
                log_error("\nError: local grouping '%s' shadows"
                          " module definition in '%s' on line %u",
                          grp->name,
                          nextgrp->tkerr.mod->name,                          
                          nextgrp->tkerr.linenum);
                res = ERR_NCX_DUP_ENTRY;
                tkc->curerr = &grp->tkerr;
                ncx_print_errormsg(tkc, mod, res);
            }
            CHK_EXIT(res, retres);
        }
    }

    /* go through the groupingQ again and check for uses chain loops */
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = nextgrp) {

        nextgrp = (grp_template_t *)dlq_nextEntry(grp);

        /* check any group/uses loops */
        res = check_chain_loop(tkc, mod, grp);
        CHK_EXIT(res, retres);
        if (res != NO_ERR) {
            dlq_remove(grp);
            grp_free_template(grp);
        }
    }

    return retres;

}  /* yang_grp_resolve_groupings */


/********************************************************************
* FUNCTION yang_grp_resolve_complete
* 
* 4th pass parsing
* Analyze the entire 'groupingQ' within the module struct
* Expand any uses and augment statements within the group and 
* validate as much as possible
*
* Completes processing for all the groupings
* sust that it is safe to expand any uses clauses
* within objects, via the yang_obj_resolve_final fn
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*   parent == parent object contraining groupingQ (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    yang_grp_resolve_complete (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
                               ncx_module_t  *mod,
                               dlq_hdr_t *groupingQ,
                               obj_template_t *parent)
{
    grp_template_t  *grp;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !groupingQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    retres = NO_ERR;

    /* go through the groupingQ and check the local types/groupings
     * and data-def statements
     */
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

        /* check any local groupings */
        res = yang_grp_resolve_complete(pcb,
                                        tkc, 
                                        mod, 
                                        &grp->groupingQ,
                                        parent);
        CHK_EXIT(res, retres);
    }


    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

#ifdef YANG_GRP_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nyang_grp_resolve: %s", grp->name);
        }
#endif

        if (grp->expand_done) {
#ifdef YANG_GRP_USES_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\n   skip expanded group %s",
                           grp->name);
            }
#endif
            continue;
        }

        /* check any local objects for uses clauses */
        res = yang_obj_resolve_uses(pcb,
                                    tkc, 
                                    mod, 
                                    &grp->datadefQ);
        CHK_EXIT(res, retres);
        grp->expand_done = TRUE;

#ifdef YANG_GRP_USES_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nyang_grp: '%s' after expand",
                       grp->name);
            obj_dump_child_list(&grp->datadefQ,
                                NCX_DEF_INDENT,
                                NCX_DEF_INDENT);
        }
#endif
    }

    return retres;

}  /* yang_grp_resolve_complete */


/********************************************************************
* FUNCTION yang_grp_resolve_final
* 
* Analyze the entire 'groupingQ' within the module struct
* Check final warnings etc.
*
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_grp_resolve_final (yang_pcb_t *pcb,
                            tk_chain_t *tkc,
                            ncx_module_t  *mod,
                            dlq_hdr_t *groupingQ)
{
    grp_template_t  *grp;
    status_t         res, retres;

#ifdef DEBUG
    if (!pcb || !tkc || !mod || !groupingQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    retres = NO_ERR;

    /* go through the groupingQ and check the local types/groupings
     * and data-def statements
     */
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL && res==NO_ERR;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

        /* check any local groupings */
        res = yang_grp_resolve_final(pcb,
                                     tkc, 
                                     mod, 
                                     &grp->groupingQ);
        CHK_EXIT(res, retres);

        /* final check on all objects within groupings */
        res = yang_obj_resolve_final(pcb,
                                     tkc, 
                                     mod, 
                                     &grp->datadefQ,
                                     TRUE);
        CHK_EXIT(res, retres);

        yang_check_obj_used(tkc, 
                            mod, 
                            &grp->typedefQ,
                            &grp->groupingQ);

    }

    return retres;

}  /* yang_grp_resolve_final */


/********************************************************************
* FUNCTION yang_grp_check_nest_loop
* 
* Check the 'uses' object and determine if it is contained
* within the group being used.
*
*    grouping A {
*      uses A;
*    }
*
*    grouping B {
*      container C {
*        grouping BB {
*          uses B;
*        }
*      }
*    } 
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   obj == 'uses' obj_template containing the ref to 'grp'
*   grp == grp_template_t that this 'obj' is using
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    yang_grp_check_nest_loop (tk_chain_t *tkc,
                              ncx_module_t  *mod,
                              obj_template_t *obj,
                              grp_template_t *grp)
{
    obj_template_t  *testobj;
    grp_template_t  *testgrp;
    status_t         retres;

#ifdef DEBUG
    if (!tkc || !obj || !grp) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    testobj = obj;

    while (testobj) {
        if (testobj->grp == grp) {
            log_error("\nError: uses of '%s'"
                      " is contained within that grouping, "
                      "defined on line %u",
                      grp->name, 
                      grp->tkerr.linenum);
            retres = ERR_NCX_DEF_LOOP;
            tkc->curerr = &obj->tkerr;
            ncx_print_errormsg(tkc, mod, retres);
            return retres;
        } else if (testobj->grp) {
            testgrp = testobj->grp->parentgrp;
            while (testgrp) {
                if (testgrp == grp) {
                    log_error("\nError: uses of '%s'"
                              " is contained within "
                              "that grouping, defined on line %u",
                              grp->name, 
                              grp->tkerr.linenum);
                    retres = ERR_NCX_DEF_LOOP;
                    tkc->curerr = &obj->tkerr;
                    ncx_print_errormsg(tkc, mod, retres);
                    return retres;
                } else {
                    testgrp = testgrp->parentgrp;
                }
            }
        }
        
        testobj = testobj->parent;
    }

    return NO_ERR;

}  /* yang_grp_check_nest_loop */


/* END file yang_grp.c */
