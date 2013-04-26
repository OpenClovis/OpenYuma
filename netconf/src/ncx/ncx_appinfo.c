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
/*  FILE: ncx_appinfo.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
17feb10      abb      begun; split out from ncx.c

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
#include <unistd.h>
#include <errno.h>

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
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

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/********************************************************************
* FUNCTION consume_appinfo_entry
* 
* Check if an appinfo sub-clause is present
*
*   foovar "fred";
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Consume the appinfo clause tokens, and save it in
* appinfoQ, if that var is non-NULL
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress
*   appinfoQ == address of Queue to hold this entry (may be NULL)
*   bkup == TRUE if not looking for a right brace
*           FALSE if right brace should be checked
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_appinfo_entry (tk_chain_t *tkc,
                           ncx_module_t  *mod,
                           dlq_hdr_t     *appinfoQ,
                           boolean bkup)
{
    ncx_appinfo_t   *appinfo;
    status_t         res, retres;

    /* right brace means appinfo is done */
    if (tkc->source == TK_SOURCE_YANG && !bkup) {
        if (tk_next_typ(tkc)==TK_TT_RBRACE) {
            return ERR_NCX_SKIPPED;
        }
    }

    res = NO_ERR;
    retres = NO_ERR;
    appinfo = NULL;

    appinfo = ncx_new_appinfo(FALSE);
    if (!appinfo) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, mod, res);
        return res;
    }

    /* get the appinfo prefix value and variable name
     *
     * Get the first token; should be an unquoted string
     * if OK then malloc a new appinfo struct and make
     * a copy of the token value.
     */
    res = yang_consume_pid_string(tkc, 
                                  mod,
                                  &appinfo->prefix,
                                  &appinfo->name);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_appinfo(appinfo);
            return retres;
        }
    }

    ncx_set_error(&appinfo->tkerr,
                  mod,
                  TK_CUR_LNUM(tkc),
                  TK_CUR_LPOS(tkc));

    /* at this point, if appinfoQ non-NULL:
     *    appinfo is malloced initialized
     *    appinfo name is set, prefix may be set
     *
     * Now get the optional appinfo value string
     *
     * move to the 2nd token, either a string or a semicolon
     * if the value is missing
     */
    switch (tk_next_typ(tkc)) {
    case TK_TT_SEMICOL:
    case TK_TT_LBRACE:
        break;
    default:
        res = yang_consume_string(tkc, mod, &appinfo->value);
        if (res != NO_ERR) {
            retres = res;
            if (NEED_EXIT(res)) {
                ncx_free_appinfo(appinfo);
                return retres;
            }
        }
    }

    /* go around and get nested extension statements or semi-colon */
    res = yang_consume_semiapp(tkc, mod, appinfo->appinfoQ);
    if (res != NO_ERR) {
        retres = res;
        if (NEED_EXIT(res)) {
            ncx_free_appinfo(appinfo);
            return retres;
        }
    }

    if (retres != NO_ERR || !appinfoQ) {
        ncx_free_appinfo(appinfo);
    } else {
        dlq_enque(appinfo, appinfoQ);
    }

    return retres;

}  /* consume_appinfo_entry */


/********************************************************************
* FUNCTION consume_appinfo
* 
* Check if an appinfo clause is present
*
* Save in appinfoQ if non-NULL
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress (NULL if none)
*   appinfoQ  == queue to use for any found entries (may be NULL)
*   bkup == TRUE if token should be backed up first
*           FALSE if not
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    consume_appinfo (tk_chain_t *tkc,
                     ncx_module_t  *mod,
                     dlq_hdr_t *appinfoQ,
                     boolean bkup)
{
    status_t       res;
    boolean        done;

    if (tkc->source == TK_SOURCE_YANG && bkup) {
        /* hack: all the YANG fns that call this function
         * already parsed the MSTRING since extensions
         * can be spread out throughout the file
         */
        TK_BKUP(tkc);
    }

    res = NO_ERR;
    done = FALSE;
    while (!done) {
        res = consume_appinfo_entry(tkc, 
                                    mod, 
                                    appinfoQ, 
                                    bkup);
        if (res != NO_ERR || tkc->source == TK_SOURCE_YANG) {
            done = TRUE;
        }
    }

    return res;

}  /* consume_appinfo */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION ncx_new_appinfo
* 
* Create an appinfo entry
*
* INPOUTS:
*   isclone == TRUE if this is for a cloned object
*
* RETURNS:
*    malloced appinfo entry or NULL if malloc error
*********************************************************************/
ncx_appinfo_t *
    ncx_new_appinfo (boolean isclone)
{
    ncx_appinfo_t *appinfo;

    appinfo = m__getObj(ncx_appinfo_t);
    if (!appinfo) {
        return NULL;
    }
    memset(appinfo, 0x0, sizeof(ncx_appinfo_t));
    appinfo->isclone = isclone;

    if (!isclone) {
        appinfo->appinfoQ = dlq_createQue();
        if (!appinfo->appinfoQ) {
            m__free(appinfo);
            appinfo = NULL;
        }
    }

    return appinfo;

}  /* ncx_new_appinfo */


/********************************************************************
* FUNCTION ncx_free_appinfo
* 
* Free an appinfo entry
*
* INPUTS:
*    appinfo == ncx_appinfo_t data structure to free
*********************************************************************/
void 
    ncx_free_appinfo (ncx_appinfo_t *appinfo)
{
#ifdef DEBUG
    if (!appinfo) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!appinfo->isclone) {
        if (appinfo->prefix) {
            m__free(appinfo->prefix);
        }
        if (appinfo->name) {
            m__free(appinfo->name);
        }
        if (appinfo->value) {
            m__free(appinfo->value);
        }
        if (appinfo->appinfoQ) {
            ncx_clean_appinfoQ(appinfo->appinfoQ);
            dlq_destroyQue(appinfo->appinfoQ);
        }
    }
    m__free(appinfo);

}  /* ncx_free_appinfo */


/********************************************************************
* FUNCTION ncx_find_appinfo
* 
* Find an appinfo entry by name (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    appinfoQ == pointer to Q of ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
ncx_appinfo_t *
    ncx_find_appinfo (dlq_hdr_t *appinfoQ,
                      const xmlChar *prefix,
                      const xmlChar *varname)
{
    ncx_appinfo_t *appinfo;

#ifdef DEBUG
    if (!appinfoQ || !varname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (appinfo = (ncx_appinfo_t *)dlq_firstEntry(appinfoQ);
         appinfo != NULL;
         appinfo = (ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (prefix && appinfo->prefix &&
            xml_strcmp(prefix, appinfo->prefix)) {
            continue;
        }

        if (!xml_strcmp(varname, appinfo->name)) {
            return appinfo;
        }
    }
    return NULL;

}  /* ncx_find_appinfo */


/********************************************************************
* FUNCTION ncx_find_const_appinfo
* 
* Find an appinfo entry by name (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    appinfoQ == pointer to Q of ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
const ncx_appinfo_t *
    ncx_find_const_appinfo (const dlq_hdr_t *appinfoQ,
                            const xmlChar *prefix,
                            const xmlChar *varname)
{
    const ncx_appinfo_t *appinfo;

#ifdef DEBUG
    if (!appinfoQ || !varname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (appinfo = (const ncx_appinfo_t *)dlq_firstEntry(appinfoQ);
         appinfo != NULL;
         appinfo = (const ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (prefix && appinfo->prefix &&
            xml_strcmp(prefix, appinfo->prefix)) {
            continue;
        }

        if (!xml_strcmp(varname, appinfo->name)) {
            return appinfo;
        }
    }
    return NULL;

}  /* ncx_find_const_appinfo */


/********************************************************************
* FUNCTION ncx_find_next_appinfo
* 
* Find the next instance of an appinfo entry by name
* (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    current == pointer to current ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
const ncx_appinfo_t *
    ncx_find_next_appinfo (const ncx_appinfo_t *current,
                           const xmlChar *prefix,
                           const xmlChar *varname)
{
    ncx_appinfo_t *appinfo;

#ifdef DEBUG
    if (!current || !varname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (appinfo = (ncx_appinfo_t *)dlq_nextEntry(current);
         appinfo != NULL;
         appinfo = (ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (prefix && appinfo->prefix &&
            xml_strcmp(prefix, appinfo->prefix)) {
            continue;
        }

        if (!xml_strcmp(varname, appinfo->name)) {
            return appinfo;
        }
    }
    return NULL;

}  /* ncx_find_next_appinfo */


/********************************************************************
* FUNCTION ncx_find_next_appinfo2
* 
* Find the next instance of an appinfo entry by name
* (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    current == pointer to current ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
ncx_appinfo_t *
    ncx_find_next_appinfo2 (ncx_appinfo_t *current,
                           const xmlChar *prefix,
                           const xmlChar *varname)
{
    ncx_appinfo_t *appinfo;

#ifdef DEBUG
    if (!current || !varname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (appinfo = (ncx_appinfo_t *)dlq_nextEntry(current);
         appinfo != NULL;
         appinfo = (ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (prefix && appinfo->prefix &&
            xml_strcmp(prefix, appinfo->prefix)) {
            continue;
        }

        if (!xml_strcmp(varname, appinfo->name)) {
            return appinfo;
        }
    }
    return NULL;

}  /* ncx_find_next_appinfo2 */


/********************************************************************
* FUNCTION ncx_clone_appinfo
* 
* Clone an appinfo value
*
* INPUTS:
*    appinfo ==  ncx_appinfo_t data structure to clone
*
* RETURNS:
*    pointer to the malloced ncx_appinfo_t struct clone of appinfo
*    NULL if a malloc error
*********************************************************************/
ncx_appinfo_t *
    ncx_clone_appinfo (ncx_appinfo_t *appinfo)
{
    ncx_appinfo_t *newapp;

#ifdef DEBUG
    if (!appinfo) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    newapp = ncx_new_appinfo(TRUE);
    if (!newapp) {
        return NULL;
    }
    newapp->prefix = appinfo->prefix;
    newapp->name = appinfo->name;
    newapp->value = appinfo->value;
    newapp->appinfoQ = appinfo->appinfoQ;

    return newapp;

}  /* ncx_clone_appinfo */


/********************************************************************
* FUNCTION ncx_clean_appinfoQ
* 
* Check an initialized appinfoQ for any entries
* Remove them from the queue and delete them
*
* INPUTS:
*    appinfoQ == Q of ncx_appinfo_t data structures to free
*********************************************************************/
void 
    ncx_clean_appinfoQ (dlq_hdr_t *appinfoQ)
{
    ncx_appinfo_t *appinfo;

#ifdef DEBUG
    if (!appinfoQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(appinfoQ)) {
        appinfo = (ncx_appinfo_t *)dlq_deque(appinfoQ);
        ncx_free_appinfo(appinfo);
    }
} /* ncx_clean_appinfoQ */


/********************************************************************
* FUNCTION ncx_consume_appinfo
* 
* Check if an appinfo clause is present
*
* Save in appinfoQ if non-NULL
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress (NULL if none)
*   appinfoQ  == queue to use for any found entries (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    ncx_consume_appinfo (tk_chain_t *tkc,
                         ncx_module_t  *mod,
                         dlq_hdr_t *appinfoQ)
{

#ifdef DEBUG
    if (!tkc || !appinfoQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return consume_appinfo(tkc, mod, appinfoQ, TRUE);

}  /* ncx_consume_appinfo */


/********************************************************************
* FUNCTION ncx_consume_appinfo2
* 
* Check if an appinfo clause is present
* Do not backup the current token
* The TK_TT_MSTRING token has not been seen yet
* Called from yang_consume_semiapp
*
* Save in appinfoQ if non-NULL
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress (NULL if none)
*   appinfoQ  == queue to use for any found entries (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    ncx_consume_appinfo2 (tk_chain_t *tkc,
                          ncx_module_t  *mod,
                          dlq_hdr_t *appinfoQ)
{
#ifdef DEBUG
    if (!tkc || !appinfoQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return consume_appinfo(tkc, mod, appinfoQ, FALSE);

}  /* ncx_consume_appinfo2 */


/********************************************************************
* FUNCTION ncx_resolve_appinfoQ
* 
* Validate all the appinfo clauses present in the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == ncx_module_t in progress
*   appinfoQ == queue to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    ncx_resolve_appinfoQ (yang_pcb_t *pcb,
                          tk_chain_t *tkc,
                          ncx_module_t  *mod,
                          dlq_hdr_t *appinfoQ)
{
    ncx_appinfo_t  *appinfo;
    ext_template_t  *ext = NULL;
    status_t         res, retres;

#ifdef DEBUG
    if (!tkc || !mod || !appinfoQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    retres = NO_ERR;

    for (appinfo = (ncx_appinfo_t *)dlq_firstEntry(appinfoQ);
         appinfo != NULL;
         appinfo = (ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (appinfo->isclone) {
            continue;
        }

        if (appinfo->ext) {
            /* this is a redo validation */
            continue;
        }

        res = NO_ERR;
        if (appinfo->prefix &&
            xml_strcmp(appinfo->prefix, mod->prefix)) {

            res = yang_find_imp_extension(pcb,
                                          tkc, 
                                          mod, 
                                          appinfo->prefix,
                                          appinfo->name, 
                                          &appinfo->tkerr,
                                          &ext);
            CHK_EXIT(res, retres);
        } else if (appinfo->prefix != NULL) {
            ext = ext_find_extension(mod, appinfo->name);
            if (!ext) {
                log_error("\nError: Local module extension '%s' not found",
                          appinfo->name);
                res = retres = ERR_NCX_DEF_NOT_FOUND;
                tkc->curerr = &appinfo->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            } else {
                res = NO_ERR;
            }
        }  /* else skipping stmt assumed to be YANG inside an ext-stmt */

        if (res == NO_ERR && appinfo->prefix != NULL) {
            appinfo->ext = ext;
            if (ext->arg && !appinfo->value) {
                retres = ERR_NCX_MISSING_PARM;
                log_error("\nError: argument missing for extension '%s:%s' ",
                          appinfo->prefix, ext->name);
                tkc->curerr = &appinfo->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            } else if (!ext->arg && appinfo->value) {
                retres = ERR_NCX_EXTRA_PARM;
                log_error("\nError: argument '%s' provided for"
                          " extension '%s:%s' is not allowed",
                          appinfo->value, 
                          appinfo->prefix, 
                          ext->name);
                tkc->curerr = &appinfo->tkerr;
                ncx_print_errormsg(tkc, mod, retres);
            }
        }

        /* recurse through any nested appinfo statements */
        res = ncx_resolve_appinfoQ(pcb, tkc, mod, appinfo->appinfoQ);
        CHK_EXIT(res, retres);
    }

    return retres;

}  /* ncx_resolve_appinfoQ */


/********************************************************************
* FUNCTION ncx_get_appinfo_value
* 
* Get the value string from an appinfo struct
*
* INPUTS:
*    appinfo ==  ncx_appinfo_t data structure to use
*
* RETURNS:
*    pointer to the string value if name
*    NULL if no value
*********************************************************************/
const xmlChar *
    ncx_get_appinfo_value (const ncx_appinfo_t *appinfo)
{
#ifdef DEBUG
    if (appinfo == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return appinfo->value;

}  /* ncx_get_appinfo_value */


/* END file ncx_appinfo.c */

