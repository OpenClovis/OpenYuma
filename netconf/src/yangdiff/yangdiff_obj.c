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
/*  FILE: yangdiff_obj.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
10-jul-08    abb      split out from yangdiff.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_ncx
#include  "ncx.h"
#endif

#ifndef _H_ncxconst
#include  "ncxconst.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_xpath
#include  "xpath.h"
#endif

#ifndef _H_yangconst
#include  "yangconst.h"
#endif

#ifndef _H_yangdiff
#include  "yangdiff.h"
#endif

#ifndef _H_yangdiff_grp
#include  "yangdiff_grp.h"
#endif

#ifndef _H_yangdiff_obj
#include  "yangdiff_obj.h"
#endif

#ifndef _H_yangdiff_typ
#include  "yangdiff_typ.h"
#endif

#ifndef _H_yangdiff_util
#include  "yangdiff_util.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#define YANGDIFF_GRP_DEBUG   1


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
 * FUNCTION mustQ_changed
 * 
 *  Check if a Q of must-stmt definitions changed
 *
 * INPUTS:
 *    oldQ == Q of old xpath_pcb_t to use
 *    newQ == Q of new xpath_pcb_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    mustQ_changed (dlq_hdr_t *oldQ,
                   dlq_hdr_t *newQ)
{
    xpath_pcb_t *oldm, *newm;
    uint32       oldcnt, newcnt;

    oldcnt = dlq_count(oldQ);
    newcnt = dlq_count(newQ);

    if (oldcnt != newcnt) {
        return 1;
    }

    if (oldcnt == 0) {
        return 0;
    }

    for (newm = (xpath_pcb_t *)dlq_firstEntry(newQ);
         newm != NULL;
         newm = (xpath_pcb_t *)dlq_nextEntry(newm)) {
        newm->seen = FALSE;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldm = (xpath_pcb_t *)dlq_firstEntry(oldQ);
         oldm != NULL;
         oldm = (xpath_pcb_t *)dlq_nextEntry(oldm)) {

        newm = xpath_find_pcb(newQ, oldm->exprstr);
        if (newm) {
            if (errinfo_changed(&oldm->errinfo, 
                                &newm->errinfo)) {
                return 1;
            } else {
                newm->seen = TRUE;
            }
        } else {
            return 1;
        }
    }

    /* look for must-stmts that were added in the new module */
    for (newm = (xpath_pcb_t *)dlq_firstEntry(newQ);
         newm != NULL;
         newm = (xpath_pcb_t *)dlq_nextEntry(newm)) {
        if (!newm->seen) {
            return 1;
        }
    }

    return 0;

}  /* mustQ_changed */


/********************************************************************
 * FUNCTION output_mustQ_diff
 * 
 *  Output any changes in a Q of must-stmt definitions
 *
 * INPUTS:
 *    cp == comparison paraeters to use
 *    oldQ == Q of old xpath_pcb_t to use
 *    newQ == Q of new xpath_pcb_t to use
 *
 *********************************************************************/
static void
    output_mustQ_diff (yangdiff_diffparms_t *cp,
                       dlq_hdr_t *oldQ,
                       dlq_hdr_t *newQ)
{
    xpath_pcb_t *oldm, *newm;

    for (newm = (xpath_pcb_t *)dlq_firstEntry(newQ);
         newm != NULL;
         newm = (xpath_pcb_t *)dlq_nextEntry(newm)) {
        newm->seen = FALSE;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldm = (xpath_pcb_t *)dlq_firstEntry(oldQ);
         oldm != NULL;
         oldm = (xpath_pcb_t *)dlq_nextEntry(oldm)) {

        newm = xpath_find_pcb(newQ, oldm->exprstr);
        if (newm) {
            newm->seen = TRUE;
            if (errinfo_changed(&oldm->errinfo, 
                                &newm->errinfo)) {
                output_mstart_line(cp, 
                                   YANG_K_MUST, 
                                   oldm->exprstr, 
                                   FALSE);
                indent_in(cp);
                output_errinfo_diff(cp, 
                                    &oldm->errinfo, 
                                    &newm->errinfo);
                indent_out(cp);
            }
        } else {
            /* must-stmt was removed from the new module */
            output_diff(cp, 
                        YANG_K_MUST,  
                        oldm->exprstr, 
                        NULL, 
                        FALSE);
        }
    }

    /* look for must-stmts that were added in the new module */
    for (newm = (xpath_pcb_t *)dlq_firstEntry(newQ);
         newm != NULL;
         newm = (xpath_pcb_t *)dlq_nextEntry(newm)) {
        if (!newm->seen) {
            /* must-stmt was added in the new module */
            output_diff(cp, 
                        YANG_K_MUST,  
                        NULL, 
                        newm->exprstr, 
                        FALSE);
        }
    }

}  /* output_mustQ_diff */


/********************************************************************
 * FUNCTION uniqueQ_changed
 * 
 *  Check if a Q of unique-stmt definitions changed
 *
 * INPUTS:
 *    oldQ == Q of old obj_unique_t to use
 *    newQ == Q of new obj_unique_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    uniqueQ_changed (dlq_hdr_t *oldQ,
                     dlq_hdr_t *newQ)
{

    obj_unique_t *oldun, *newun;

    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    for (newun = (obj_unique_t *)dlq_firstEntry(newQ);
         newun != NULL;
         newun = (obj_unique_t *)dlq_nextEntry(newun)) {
        newun->seen = FALSE;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldun = (obj_unique_t *)dlq_firstEntry(oldQ);
         oldun != NULL;
         oldun = (obj_unique_t *)dlq_nextEntry(oldun)) {

        newun = obj_find_unique(newQ, oldun->xpath);
        if (newun) {
            newun->seen = TRUE;
        } else {
            return 1;
        }
    }

    /* look for unique-stmts that were added in the new module */
    for (newun = (obj_unique_t *)dlq_firstEntry(newQ);
         newun != NULL;
         newun = (obj_unique_t *)dlq_nextEntry(newun)) {
        if (!newun->seen) {
            return 1;
        }
    }

    return 0;

}  /* uniqueQ_changed */


/********************************************************************
 * FUNCTION output_uniqueQ_diff
 * 
 *  Output any changes in a Q of unique-stmt definitions
 *
 * INPUTS:
 *    cp == comparison paraeters to use
 *    oldQ == Q of old obj_unique_t to use
 *    newQ == Q of new obj_unique_t to use
 *
 *********************************************************************/
static void
    output_uniqueQ_diff (yangdiff_diffparms_t *cp,
                         dlq_hdr_t *oldQ,
                         dlq_hdr_t *newQ)
{
    obj_unique_t *oldu, *newu;

    for (newu = (obj_unique_t *)dlq_firstEntry(newQ);
         newu != NULL;
         newu = (obj_unique_t *)dlq_nextEntry(newu)) {
        newu->seen = FALSE;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldu = (obj_unique_t *)dlq_firstEntry(oldQ);
         oldu != NULL;
         oldu = (obj_unique_t *)dlq_nextEntry(oldu)) {

        newu = obj_find_unique(newQ, oldu->xpath);
        if (newu) {
            newu->seen = TRUE;
        } else {
            /* must-stmt was removed from the new module */
            output_diff(cp, 
                        YANG_K_UNIQUE, 
                        oldu->xpath,
                        NULL, 
                        FALSE);
        }
    }

    /* look for unique-stmts that were added in the new module */
    for (newu = (obj_unique_t *)dlq_firstEntry(newQ);
         newu != NULL;
         newu = (obj_unique_t *)dlq_nextEntry(newu)) {
        if (!newu->seen) {
            /* must-stmt was added in the new module */
            output_diff(cp,
                        YANG_K_UNIQUE,
                        NULL,
                        newu->xpath,
                        FALSE);
        }
    }

}  /* output_uniqueQ_diff */


/********************************************************************
 * FUNCTION container_changed
 * 
 *  Check if a container definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    container_changed (yangdiff_diffparms_t *cp,
                       obj_template_t *oldobj,
                       obj_template_t *newobj)
{
    obj_container_t *old, *new;

    old = oldobj->def.container;
    new = newobj->def.container;


    if (mustQ_changed(&old->mustQ, &new->mustQ)) {
        return 1;
    }

    if (str_field_changed(YANG_K_PRESENCE,
                          old->presence,
                          new->presence, 
                          FALSE,
                          NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           FALSE,
                           NULL)) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE,
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr, 
                          FALSE,
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref, 
                          FALSE,
                          NULL)) {
        return 1;
    }

    if (typedefQ_changed(cp, old->typedefQ, new->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, old->groupingQ, new->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, old->datadefQ, new->datadefQ)) {
        return 1;
    }

    return 0;

} /* container_changed */


/********************************************************************
 * FUNCTION output_container_diff
 * 
 *  Output the differences for a container definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_container_diff (yangdiff_diffparms_t *cp,
                           obj_template_t *oldobj,
                           obj_template_t *newobj)
{
    obj_container_t *old, *new;
    yangdiff_cdb_t   cdb;
    boolean          isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.container;
    new = newobj->def.container;

    output_mustQ_diff(cp, &old->mustQ, &new->mustQ);

    if (str_field_changed(YANG_K_PRESENCE,
                          old->presence, 
                          new->presence, 
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (bool_field_changed(YANG_K_CONFIG, 
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status, 
                             isrev,
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_typedefQ_diff(cp, old->typedefQ, new->typedefQ);

    output_groupingQ_diff(cp, old->groupingQ, new->groupingQ);

    output_datadefQ_diff(cp, old->datadefQ, new->datadefQ);

} /* output_container_diff */


/********************************************************************
 * FUNCTION leaf_changed
 * 
 *  Check if a leaf definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    leaf_changed (yangdiff_diffparms_t *cp,
                  obj_template_t *oldobj,
                  obj_template_t *newobj)
{
    obj_leaf_t *old, *new;

    old = oldobj->def.leaf;
    new = newobj->def.leaf;

    if (type_changed(cp, old->typdef, new->typdef)) {
        return 1;
    }

    if (str_field_changed(YANG_K_UNITS, 
                          old->units, 
                          new->units, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (mustQ_changed(&old->mustQ, &new->mustQ)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DEFAULT,
                          old->defval, 
                          new->defval, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    return 0;

} /* leaf_changed */



/********************************************************************
 * FUNCTION anyxml_changed
 * 
 *  Check if an anyxml definition changed
 *
 * INPUTS:
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    anyxml_changed (obj_template_t *oldobj,
                    obj_template_t *newobj)
{
    obj_leaf_t *old, *new;

    old = oldobj->def.leaf;
    new = newobj->def.leaf;

    if (mustQ_changed(&old->mustQ, &new->mustQ)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    return 0;

} /* anyxml_changed */


/********************************************************************
 * FUNCTION output_leaf_diff
 * 
 *  Output the differences for a leaf definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_leaf_diff (yangdiff_diffparms_t *cp,
                      obj_template_t *oldobj,
                      obj_template_t *newobj)
{
    obj_leaf_t *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.leaf;
    new = newobj->def.leaf;

    if (type_changed(cp, old->typdef, new->typdef)) {
        output_one_type_diff(cp, old->typdef, new->typdef);
    }

    if (str_field_changed(YANG_K_UNITS, 
                          old->units, 
                          new->units, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_mustQ_diff(cp, &old->mustQ, &new->mustQ);

    if (str_field_changed(YANG_K_DEFAULT,
                          old->defval, 
                          new->defval, 
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status, 
                             isrev,
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

} /* output_leaf_diff */


/********************************************************************
 * FUNCTION output_anyxml_diff
 * 
 *  Output the differences for an anyxml definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_anyxml_diff (yangdiff_diffparms_t *cp,
                        obj_template_t *oldobj,
                        obj_template_t *newobj)
{
    obj_leaf_t        *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.leaf;
    new = newobj->def.leaf;

    output_mustQ_diff(cp, &old->mustQ, &new->mustQ);

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status, 
                             isrev,
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

} /* output_anyxml_diff */


/********************************************************************
 * FUNCTION leaf_list_changed
 * 
 *  Check if a leaf-list definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    leaf_list_changed (yangdiff_diffparms_t *cp,
                       obj_template_t *oldobj,
                       obj_template_t *newobj)
{
    obj_leaflist_t *old, *new;

    old = oldobj->def.leaflist;
    new = newobj->def.leaflist;

    if (type_changed(cp, old->typdef, new->typdef)) {
        return 1;
    }

    if (str_field_changed(YANG_K_UNITS, 
                          old->units, 
                          new->units, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (mustQ_changed(&old->mustQ, &new->mustQ)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MIN_ELEMENTS, 
                           old->minset,
                           new->minset, 
                           FALSE, 
                           NULL)) {
        return 1;
    }
    if (new->minelems != old->minelems) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MAX_ELEMENTS, 
                           old->maxset,
                           new->maxset, 
                           FALSE, 
                           NULL)) {
        return 1;
    }
    if (new->maxelems != old->maxelems) {
        return 1;
    }

    if (old->ordersys != new->ordersys) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    return 0;

} /* leaf_list_changed */


/********************************************************************
 * FUNCTION output_leaf_list_diff
 * 
 *  Output the differences for a leaf-list definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_leaf_list_diff (yangdiff_diffparms_t *cp,
                           obj_template_t *oldobj,
                           obj_template_t *newobj)
{
    obj_leaflist_t   *old, *new;
    yangdiff_cdb_t    cdb;
    char              oldnum[NCX_MAX_NUMLEN];
    char              newnum[NCX_MAX_NUMLEN];
    const xmlChar    *oldn, *newn;
    const xmlChar    *oldorder, *neworder;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.leaflist;
    new = newobj->def.leaflist;

    if (type_changed(cp, old->typdef, new->typdef)) {
        output_one_type_diff(cp, old->typdef, new->typdef);
    }

    if (str_field_changed(YANG_K_UNITS, old->units, new->units, 
                          isrev, &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_mustQ_diff(cp, &old->mustQ, &new->mustQ);

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           isrev, 
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (old->minset) {
        sprintf(oldnum, "%u", old->minelems);
        oldn = (const xmlChar *)oldnum;
    } else {
        oldn = NULL;
    }
    if (new->minset) {
        sprintf(newnum, "%u", new->minelems);
        newn = (const xmlChar *)newnum;
    } else {
        newn = NULL;
    }

    if (str_field_changed(YANG_K_MIN_ELEMENTS, 
                          oldn, 
                          newn,
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (old->maxset) {
        sprintf(oldnum, "%u", old->maxelems);
        oldn = (const xmlChar *)oldnum;
    } else {
        oldn = NULL;
    }
    if (new->maxset) {
        sprintf(newnum, "%u", new->maxelems);
        newn = (const xmlChar *)newnum;
    } else {
        newn = NULL;
    }

    if (str_field_changed(YANG_K_MAX_ELEMENTS, 
                          oldn, 
                          newn,
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    oldorder = (old->ordersys) ? YANG_K_SYSTEM : YANG_K_USER;
    neworder = (new->ordersys) ? YANG_K_SYSTEM : YANG_K_USER;
    if (str_field_changed(YANG_K_ORDERED_BY, 
                          oldorder, 
                          neworder,
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS, 
                             old->status,
                             new->status, 
                             isrev, 
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE, 
                          old->ref,
                          new->ref, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

} /* output_leaf_list_diff */


/********************************************************************
 * FUNCTION list_changed
 * 
 *  Check if a list definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    list_changed (yangdiff_diffparms_t *cp,
                  obj_template_t *oldobj,
                  obj_template_t *newobj)
{
    obj_list_t *old, *new;

    old = oldobj->def.list;
    new = newobj->def.list;

    if (mustQ_changed(&old->mustQ, &new->mustQ)) {
        return 1;
    }

    if (str_field_changed(YANG_K_KEY, 
                          obj_get_keystr(oldobj), 
                          obj_get_keystr(newobj), 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (uniqueQ_changed(&old->uniqueQ, &new->uniqueQ)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_CONFIG, 
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MIN_ELEMENTS, 
                           old->minset,
                           new->minset, 
                           FALSE, 
                           NULL)) {
        return 1;
    }
    if (new->minelems != old->minelems) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MAX_ELEMENTS, 
                           old->maxset,
                           new->maxset,
                           FALSE, 
                           NULL)) {
        return 1;
    }
    if (new->maxelems != old->maxelems) {
        return 1;
    }

    if (old->ordersys != new->ordersys) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status, 
                             FALSE,
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref, 
                          FALSE,
                          NULL)) {
        return 1;
    }

    if (typedefQ_changed(cp, old->typedefQ, new->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, old->groupingQ, new->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, old->datadefQ, new->datadefQ)) {
        return 1;
    }

    return 0;

} /* list_changed */


/********************************************************************
 * FUNCTION output_list_diff
 * 
 *  Output the differences for a list definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_list_diff (yangdiff_diffparms_t *cp,
                      obj_template_t *oldobj,
                      obj_template_t *newobj)
{
    obj_list_t       *old, *new;
    yangdiff_cdb_t    cdb;
    char              oldnum[NCX_MAX_NUMLEN];
    char              newnum[NCX_MAX_NUMLEN];
    const xmlChar    *oldn, *newn;
    const xmlChar    *oldorder, *neworder;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.list;
    new = newobj->def.list;

    output_mustQ_diff(cp, &old->mustQ, &new->mustQ);

    if (str_field_changed(YANG_K_KEY, 
                          obj_get_keystr(oldobj),
                          obj_get_keystr(newobj),
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_uniqueQ_diff(cp, &old->uniqueQ, &new->uniqueQ);

    if (bool_field_changed(YANG_K_CONFIG,
                           (oldobj->flags & OBJ_FL_CONFIG),
                           (newobj->flags & OBJ_FL_CONFIG), 
                           isrev,
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (old->minset) {
        sprintf(oldnum, "%u", old->minelems);
        oldn = (const xmlChar *)oldnum;
    } else {
        oldn = NULL;
    }
    if (new->minset) {
        sprintf(newnum, "%u", new->minelems);
        newn = (const xmlChar *)newnum;
    } else {
        newn = NULL;
    }

    if (str_field_changed(YANG_K_MIN_ELEMENTS, 
                          oldn,
                          newn,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (old->maxset) {
        sprintf(oldnum, "%u", old->maxelems);
        oldn = (const xmlChar *)oldnum;
    } else {
        oldn = NULL;
    }
    if (new->maxset) {
        sprintf(newnum, "%u", new->maxelems);
        newn = (const xmlChar *)newnum;
    } else {
        newn = NULL;
    }

    if (str_field_changed(YANG_K_MAX_ELEMENTS, 
                          oldn, 
                          newn,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    oldorder = (old->ordersys) ? YANG_K_SYSTEM : YANG_K_USER;
    neworder = (new->ordersys) ? YANG_K_SYSTEM : YANG_K_USER;
    if (str_field_changed(YANG_K_ORDERED_BY, 
                          oldorder, 
                          neworder,
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status,
                             isrev,
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE, 
                          old->ref,
                          new->ref, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_typedefQ_diff(cp, old->typedefQ, new->typedefQ);

    output_groupingQ_diff(cp, old->groupingQ, new->groupingQ);

    output_datadefQ_diff(cp, old->datadefQ, new->datadefQ);

} /* output_list_diff */


/********************************************************************
 * FUNCTION choice_changed
 * 
 *  Check if a choice definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    choice_changed (yangdiff_diffparms_t *cp,
                    obj_template_t *oldobj,
                    obj_template_t *newobj)
{
    obj_choice_t *old, *new;

    old = oldobj->def.choic;
    new = newobj->def.choic;


    if (str_field_changed(YANG_K_DEFAULT, 
                          old->defval, 
                          new->defval, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY),
                           FALSE, 
                           NULL)) {
        return 1;
    }

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (datadefQ_changed(cp, old->caseQ, new->caseQ)) {
        return 1;
    }

    return 0;


} /* choice_changed */


/********************************************************************
 * FUNCTION output_choice_diff
 * 
 *  Output the differences for a choice definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_choice_diff (yangdiff_diffparms_t *cp,
                        obj_template_t *oldobj,
                        obj_template_t *newobj)
{
    obj_choice_t     *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.choic;
    new = newobj->def.choic;

    if (str_field_changed(YANG_K_DEFAULT,
                          old->defval, 
                          new->defval, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (bool_field_changed(YANG_K_MANDATORY,
                           (oldobj->flags & OBJ_FL_MANDATORY),
                           (newobj->flags & OBJ_FL_MANDATORY),
                           isrev, 
                           &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (status_field_changed(YANG_K_STATUS, 
                             old->status,
                             new->status, 
                             isrev, 
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE, 
                          old->ref,
                          new->ref, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_datadefQ_diff(cp, old->caseQ, new->caseQ);

} /* output_choice_diff */


/********************************************************************
 * FUNCTION case_changed
 * 
 *  Check if a case definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    case_changed (yangdiff_diffparms_t *cp,
                  obj_template_t *oldobj,
                  obj_template_t *newobj)
{
    obj_case_t *old, *new;

    old = oldobj->def.cas;
    new = newobj->def.cas;

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (datadefQ_changed(cp, old->datadefQ, new->datadefQ)) {
        return 1;
    }

    return 0;

} /* case_changed */


/********************************************************************
 * FUNCTION output_case_diff
 * 
 *  Output the differences for a case definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_case_diff (yangdiff_diffparms_t *cp,
                      obj_template_t *oldobj,
                      obj_template_t *newobj)
{
    obj_case_t       *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.cas;
    new = newobj->def.cas;

    if (status_field_changed(YANG_K_STATUS, 
                             old->status,
                             new->status, 
                             isrev, 
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_datadefQ_diff(cp, old->datadefQ, new->datadefQ);

} /* output_case_diff */


/********************************************************************
 * FUNCTION rpc_changed
 * 
 *  Check if a rpc definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    rpc_changed (yangdiff_diffparms_t *cp,
                 obj_template_t *oldobj,
                 obj_template_t *newobj)
{
    obj_rpc_t *old, *new;

    old = oldobj->def.rpc;
    new = newobj->def.rpc;

    if (status_field_changed(YANG_K_STATUS,
                             old->status,
                             new->status, 
                             FALSE,
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (typedefQ_changed(cp, &old->typedefQ, &new->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, &old->groupingQ, &new->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, &old->datadefQ, &new->datadefQ)) {
        return 1;
    }

    return 0;

}  /* rpc_changed */


/********************************************************************
 * FUNCTION output_rpc_diff
 * 
 *  Output the differences for an RPC definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_rpc_diff (yangdiff_diffparms_t *cp,
                     obj_template_t *oldobj,
                     obj_template_t *newobj)
{
    obj_rpc_t        *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.rpc;
    new = newobj->def.rpc;

    if (status_field_changed(YANG_K_STATUS, 
                             old->status,
                             new->status, 
                             isrev, 
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION, 
                          old->descr,
                          new->descr, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE, 
                          old->ref,
                          new->ref, 
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_typedefQ_diff(cp, &old->typedefQ, &new->typedefQ);

    output_groupingQ_diff(cp, &old->groupingQ, &new->groupingQ);

    output_datadefQ_diff(cp, &old->datadefQ, &new->datadefQ);

} /* output_rpc_diff */


/********************************************************************
 * FUNCTION rpcio_changed
 * 
 *  Check if a rpc IO definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    rpcio_changed (yangdiff_diffparms_t *cp,
                   obj_template_t *oldobj,
                   obj_template_t *newobj)
{
    obj_rpcio_t *old, *new;

    old = oldobj->def.rpcio;
    new = newobj->def.rpcio;

    if (typedefQ_changed(cp, &old->typedefQ, &new->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, &old->groupingQ, &new->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, &old->datadefQ, &new->datadefQ)) {
        return 1;
    }

    return 0;

}  /* rpcio_changed */


/********************************************************************
 * FUNCTION output_rpcio_diff
 * 
 *  Output the differences for an RPC IO definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_rpcio_diff (yangdiff_diffparms_t *cp,
                       obj_template_t *oldobj,
                       obj_template_t *newobj)
{
    obj_rpcio_t      *old, *new;

    old = oldobj->def.rpcio;
    new = newobj->def.rpcio;

    output_typedefQ_diff(cp, &old->typedefQ, &new->typedefQ);

    output_groupingQ_diff(cp, &old->groupingQ, &new->groupingQ);

    output_datadefQ_diff(cp, &old->datadefQ, &new->datadefQ);

} /* output_rpcio_diff */


/********************************************************************
 * FUNCTION notif_changed
 * 
 *  Check if a notification definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    notif_changed (yangdiff_diffparms_t *cp,
                   obj_template_t *oldobj,
                   obj_template_t *newobj)
{
    obj_notif_t *old, *new;

    old = oldobj->def.notif;
    new = newobj->def.notif;

    if (status_field_changed(YANG_K_STATUS,
                             old->status, 
                             new->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr, 
                          new->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref, 
                          new->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (typedefQ_changed(cp, &old->typedefQ, &new->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, &old->groupingQ, &new->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, &old->datadefQ, &new->datadefQ)) {
        return 1;
    }

    return 0;

}  /* notif_changed */


/********************************************************************
 * FUNCTION output_notif_diff
 * 
 *  Output the differences for a notification definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 *********************************************************************/
static void
    output_notif_diff (yangdiff_diffparms_t *cp,
                       obj_template_t *oldobj,
                       obj_template_t *newobj)
{
    obj_notif_t      *old, *new;
    yangdiff_cdb_t    cdb;
    boolean           isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    old = oldobj->def.notif;
    new = newobj->def.notif;

    if (status_field_changed(YANG_K_STATUS, 
                             old->status,
                             new->status, 
                             isrev, 
                             &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_DESCRIPTION,
                          old->descr,
                          new->descr,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    if (str_field_changed(YANG_K_REFERENCE,
                          old->ref,
                          new->ref,
                          isrev,
                          &cdb)) {
        output_cdb_line(cp, &cdb);
    }

    output_typedefQ_diff(cp, &old->typedefQ, &new->typedefQ);

    output_groupingQ_diff(cp, &old->groupingQ, &new->groupingQ);

    output_datadefQ_diff(cp, &old->datadefQ, &new->datadefQ);

} /* output_notif_diff */


/********************************************************************
 * FUNCTION object_changed
 * 
 *  Check if a object definition changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old obj_template_t to use
 *    newobj == new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    object_changed (yangdiff_diffparms_t *cp,
                    obj_template_t *oldobj,
                    obj_template_t *newobj)
{
    if (oldobj->objtype != newobj->objtype) {
        return 1;
    }

    if (iffeatureQ_changed(obj_get_mod_prefix(oldobj),
                           &oldobj->iffeatureQ,
                           &newobj->iffeatureQ)) {
        return 1;
    }

    switch (oldobj->objtype) {
    case OBJ_TYP_CONTAINER:
        return container_changed(cp, oldobj, newobj);
    case OBJ_TYP_ANYXML:
        return anyxml_changed(oldobj, newobj);
    case OBJ_TYP_LEAF:
        return leaf_changed(cp, oldobj, newobj);
    case OBJ_TYP_LEAF_LIST:
        return leaf_list_changed(cp, oldobj, newobj);
    case OBJ_TYP_LIST:
        return list_changed(cp, oldobj, newobj);
    case OBJ_TYP_CHOICE:
        return choice_changed(cp, oldobj, newobj);
    case OBJ_TYP_CASE:
        return case_changed(cp, oldobj, newobj);
    case OBJ_TYP_RPC:
        return rpc_changed(cp, oldobj, newobj);
    case OBJ_TYP_RPCIO:
        return rpcio_changed(cp, oldobj, newobj);
    case OBJ_TYP_NOTIF:
        return notif_changed(cp, oldobj, newobj);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
    /*NOTREACHED*/

} /* object_changed */


/********************************************************************
 * FUNCTION output_one_object_diff
 * 
 *  Output the differences report for one object definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldobj == old object
 *    newobj == new object
 *
 *********************************************************************/
static void
    output_one_object_diff (yangdiff_diffparms_t *cp,
                            obj_template_t *oldobj,
                            obj_template_t *newobj)
{
    output_mstart_line(cp, 
                       obj_get_typestr(oldobj),
                       obj_get_name(oldobj), 
                       TRUE);

    if (cp->edifftype == YANGDIFF_DT_TERSE) {
        return;
    }

    indent_in(cp);

    if (oldobj->objtype != newobj->objtype) {
        output_diff(cp, 
                    (const xmlChar *)"object-type",
                    obj_get_typestr(oldobj),
                    obj_get_typestr(newobj), 
                    TRUE);
    } else {
        output_iffeatureQ_diff(cp,
                               obj_get_mod_prefix(oldobj),
                               &oldobj->iffeatureQ,
                               &newobj->iffeatureQ);

        switch (oldobj->objtype) {
        case OBJ_TYP_CONTAINER:
            output_container_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_LEAF:
            output_leaf_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_ANYXML:
            output_anyxml_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_LEAF_LIST:
            output_leaf_list_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_LIST:
            output_list_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_CHOICE:
            output_choice_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_CASE:
            output_case_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_RPC:
            output_rpc_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_RPCIO:
            output_rpcio_diff(cp, oldobj, newobj);
            break;
        case OBJ_TYP_NOTIF:
            output_notif_diff(cp, oldobj, newobj);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    indent_out(cp);

} /* output_one_object_diff */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
 * FUNCTION output_datadefQ_diff
 * 
 *  Output the differences report for a Q of data definitions
 *  Not always called for top-level objects; Can be called
 *  for nested objects
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old obj_template_t to use
 *    newQ == Q of new obj_template_t to use
 *
 *********************************************************************/
void
    output_datadefQ_diff (yangdiff_diffparms_t *cp,
                          dlq_hdr_t *oldQ,
                          dlq_hdr_t *newQ)
{
    obj_template_t *oldobj, *newobj;
    boolean         anyout;

    anyout = FALSE;

    for (newobj = (obj_template_t *)dlq_firstEntry(newQ);
         newobj != NULL;
         newobj = (obj_template_t *)dlq_nextEntry(newobj)) {
        newobj->flags &= ~OBJ_FL_SEEN;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldobj = (obj_template_t *)dlq_firstEntry(oldQ);
         oldobj != NULL;
         oldobj = (obj_template_t *)dlq_nextEntry(oldobj)) {

        if (obj_has_name(oldobj)) {
            newobj = obj_find_template(newQ, 
                                       cp->newmod->name, 
                                       obj_get_name(oldobj));
        } else {
            continue;
        }

        if (newobj) {
            newobj->flags |= OBJ_FL_SEEN;
            if (oldobj->flags & OBJ_FL_SEEN) {
                if (oldobj->flags & OBJ_FL_DIFF) {
                    output_one_object_diff(cp, oldobj, newobj);
                    anyout = TRUE;
                }
            } else if (object_changed(cp, oldobj, newobj)) {
                oldobj->flags |= (OBJ_FL_SEEN | OBJ_FL_DIFF);
                output_one_object_diff(cp, oldobj, newobj);
                anyout = TRUE;
            } else {
                oldobj->flags |= OBJ_FL_SEEN;
            }
        } else {
            /* object was removed from the new module */
            oldobj->flags |= OBJ_FL_SEEN;
            output_diff(cp, 
                        obj_get_typestr(oldobj), 
                        obj_get_name(oldobj), 
                        NULL, 
                        TRUE);
            anyout = TRUE;
        }
    }

    /* look for objects that were added in the new module */
    for (newobj = (obj_template_t *)dlq_firstEntry(newQ);
         newobj != NULL;
         newobj = (obj_template_t *)dlq_nextEntry(newobj)) {
        if (!obj_has_name(newobj)) {
            continue;
        }
        if (!(newobj->flags & OBJ_FL_SEEN)) {
            /* this object was added in the new version */
            output_diff(cp, 
                        obj_get_typestr(newobj),
                        NULL, 
                        obj_get_name(newobj),
                        TRUE);
            anyout = TRUE;          
        }
    }

    /* check if the object order changed at all, but only if this
     * is not a top-level object.  Use a little hack to test here
     */
    if (!anyout && (oldQ != &cp->oldmod->datadefQ)) {
        oldobj = (obj_template_t *)dlq_firstEntry(oldQ);
        newobj = (obj_template_t *)dlq_firstEntry(newQ);
        while (oldobj && newobj) {
            if (obj_has_name(oldobj)) {
                if (!obj_is_match(oldobj, newobj)) {
                    output_mstart_line(cp, 
                                       (const xmlChar *)
                                       "child node order", 
                                       NULL, 
                                       FALSE);
                    return;
                }
            }
            oldobj = (obj_template_t *)dlq_nextEntry(oldobj);
            newobj = (obj_template_t *)dlq_nextEntry(newobj);
        }
    }


} /* output_datadefQ_diff */


/********************************************************************
 * FUNCTION datadefQ_changed
 * 
 *  Check if a Q of data definitions changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old obj_template_t to use
 *    newQ == Q of new obj_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
uint32
    datadefQ_changed (yangdiff_diffparms_t *cp,
                      dlq_hdr_t *oldQ,
                      dlq_hdr_t *newQ)
{

    obj_template_t *oldobj, *newobj;

    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    for (newobj = (obj_template_t *)dlq_firstEntry(newQ);
         newobj != NULL;
         newobj = (obj_template_t *)dlq_nextEntry(newobj)) {
        newobj->flags &= ~OBJ_FL_SEEN;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldobj = (obj_template_t *)dlq_firstEntry(oldQ);
         oldobj != NULL;
         oldobj = (obj_template_t *)dlq_nextEntry(oldobj)) {

        if (obj_is_hidden(oldobj)) {
            continue;
        }

        if (obj_has_name(oldobj)) {
            newobj = obj_find_template(newQ, 
                                       cp->newmod->name, 
                                       obj_get_name(oldobj));
        } else {
            continue;
        }

        if (newobj) {
            if (!obj_is_hidden(newobj)) {
                if (object_changed(cp, oldobj, newobj)) {
                    /* use the seen flag in the old tree to indicate
                     * that a node has been visited and the 'diff' flag 
                     * to indicate a node has changed
                     */
                    oldobj->flags |= (OBJ_FL_SEEN | OBJ_FL_DIFF);
                    return 1;
                } else {
                    newobj->flags |= OBJ_FL_SEEN;
                }
            }
        } else {
            return 1;
        }
    }

    /* look for objects that were added in the new module */
    for (newobj = (obj_template_t *)dlq_firstEntry(newQ);
         newobj != NULL;
         newobj = (obj_template_t *)dlq_nextEntry(newobj)) {
        if (!obj_has_name(newobj) || obj_is_hidden(newobj)) {
            continue;
        }
        if (!(newobj->flags & OBJ_FL_SEEN)) {
            return 1;
        }
    }

    /* check if the object order changed at all, but only if this
     * is not a top-level object.  Use a little hack to test here
     */
    if (oldQ != &cp->oldmod->datadefQ) {
        oldobj = (obj_template_t *)dlq_firstEntry(oldQ);
        newobj = (obj_template_t *)dlq_firstEntry(newQ);
        while (oldobj && newobj) {
            if (obj_has_name(oldobj) && 
                obj_has_name(newobj) &&
                !(obj_is_hidden(oldobj) || 
                  obj_is_hidden(newobj))) {
                if (!obj_is_match(oldobj, newobj)) {
                    return 1;
                }
            }
            oldobj = (obj_template_t *)dlq_nextEntry(oldobj);
            newobj = (obj_template_t *)dlq_nextEntry(newobj);
        }
    }
                               
    return 0;

}  /* datadefQ_changed */

/* END yangdiff_obj.c */



