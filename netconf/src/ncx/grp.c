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
/*  FILE: grp.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
09dec07      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
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

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                         V A R I A B L E S                         *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION grp_new_template
* 
* Malloc and initialize the fields in a grp_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
grp_template_t * 
    grp_new_template (void)
{
    grp_template_t  *grp;

    grp = m__getObj(grp_template_t);
    if (!grp) {
        return NULL;
    }
    (void)memset(grp, 0x0, sizeof(grp_template_t));
    dlq_createSQue(&grp->typedefQ);
    dlq_createSQue(&grp->groupingQ);
    dlq_createSQue(&grp->datadefQ);
    dlq_createSQue(&grp->appinfoQ);
    grp->status = NCX_STATUS_CURRENT;   /* default */
    return grp;

}  /* grp_new_template */


/********************************************************************
* FUNCTION grp_free_template
* 
* Scrub the memory in a grp_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    grp == grp_template_t data structure to free
*********************************************************************/
void 
    grp_free_template (grp_template_t *grp)
{
#ifdef DEBUG
    if (!grp) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (grp->name) {
        m__free(grp->name);
    }
    if (grp->descr) {
        m__free(grp->descr);
    }
    if (grp->ref) {
        m__free(grp->ref);
    }

    typ_clean_typeQ(&grp->typedefQ);
    grp_clean_groupingQ(&grp->groupingQ);
    obj_clean_datadefQ(&grp->datadefQ);
    ncx_clean_appinfoQ(&grp->appinfoQ);
    m__free(grp);

}  /* grp_free_template */


/********************************************************************
 * Clean a queue of grp_template_t structs
 *
 * \param que Q of grp_template_t data structures to free
 *********************************************************************/
void grp_clean_groupingQ (dlq_hdr_t *que)
{
    if (!que) {
        return;
    }

    while (!dlq_empty(que)) {
        grp_template_t *grp = (grp_template_t *)dlq_deque(que);
        grp_free_template(grp);
    }
}  /* grp_clean_groupingQ */


/********************************************************************
* FUNCTION grp_has_typedefs
* 
* Check if the grouping contains any typedefs
*
* INPUTS:
*    grp == grp_template_t struct to check
*
* RETURNS:
*    TRUE if any embedded typedefs
*    FALSE if no embedded typedefs
*********************************************************************/
boolean
    grp_has_typedefs (const grp_template_t *grp)
{
    const grp_template_t *chgrp;

#ifdef DEBUG
    if (!grp) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!dlq_empty(&grp->typedefQ)) {
        return TRUE;
    }

    for (chgrp = (const grp_template_t *)
             dlq_firstEntry(&grp->groupingQ);
         chgrp != NULL;
         chgrp = (const grp_template_t *)dlq_nextEntry(chgrp)) {
        if (grp_has_typedefs(chgrp)) {
            return TRUE;
        }
    }

    return FALSE;

}  /* grp_has_typedefs */


/********************************************************************
* FUNCTION grp_get_mod_name
* 
* Get the module name for a grouping
*
* INPUTS:
*    grp == grp_template_t struct to check
*
* RETURNS:
*    const pointer to module name
*********************************************************************/
const xmlChar *
    grp_get_mod_name (const grp_template_t *grp)
{
#ifdef DEBUG
    if (!grp || !grp->tkerr.mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (grp->tkerr.mod->ismod) {
        return grp->tkerr.mod->name;
    } else {
        return grp->tkerr.mod->belongs;
    }
    /*NOTREACHED*/

} /* grp_get_mod_name */

/* END grp.c */
