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
/*  FILE: yangdiff_grp.c

                
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
 * FUNCTION output_one_grouping_diff
 * 
 *  Output the differences report for one typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldgrp == old grouping
 *    newgrp == new grouping
 *
 *********************************************************************/
static void
    output_one_grouping_diff (yangdiff_diffparms_t *cp,
                              grp_template_t *oldgrp,
                              grp_template_t *newgrp)
{
    yangdiff_cdb_t  cdb[3];
    uint32          changecnt, i;
    boolean         isrev, tchanged, gchanged, dchanged;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;

    tchanged = FALSE;
    gchanged = FALSE;
    dchanged = FALSE;
    changecnt = 0;

    if (typedefQ_changed(cp, 
                         &oldgrp->typedefQ, 
                         &newgrp->typedefQ)) {
        tchanged = TRUE;
        changecnt++;
    }

    if (groupingQ_changed(cp, 
                          &oldgrp->groupingQ, 
                          &newgrp->groupingQ)) {
        gchanged = TRUE;
        changecnt++;
    }

    if (datadefQ_changed(cp, 
                         &oldgrp->datadefQ, 
                         &newgrp->datadefQ)) {
        dchanged = TRUE;
        changecnt++;
    }

    changecnt += status_field_changed(YANG_K_STATUS,
                                      oldgrp->status, 
                                      newgrp->status, 
                                      isrev, 
                                      &cdb[0]);
    changecnt += str_field_changed(YANG_K_DESCRIPTION,
                                   oldgrp->descr, 
                                   newgrp->descr, 
                                   isrev, 
                                   &cdb[1]);
    changecnt += str_field_changed(YANG_K_REFERENCE,
                                   oldgrp->ref, 
                                   newgrp->ref, 
                                   isrev, 
                                   &cdb[2]);
    if (changecnt == 0) {
        return;
    }

    /* generate the diff output, based on the requested format */
    output_mstart_line(cp, YANG_K_GROUPING, oldgrp->name, TRUE);

    if (cp->edifftype == YANGDIFF_DT_TERSE) {
        return;
    }

    indent_in(cp);

    for (i=0; i<3; i++) {
        if (cdb[i].changed) {
            output_cdb_line(cp, &cdb[i]);
        }
    }

    if (tchanged) {
        output_typedefQ_diff(cp, 
                             &oldgrp->typedefQ, 
                             &newgrp->typedefQ);
    }

    if (gchanged) {
        output_groupingQ_diff(cp, 
                              &oldgrp->groupingQ, 
                              &newgrp->groupingQ);
    }

    if (dchanged) {
        output_datadefQ_diff(cp, 
                             &oldgrp->datadefQ, 
                             &newgrp->datadefQ);
    }

    indent_out(cp);

} /* output_one_grouping_diff */


/********************************************************************
 * FUNCTION grouping_changed
 * 
 *  Check if a (nested) grouping changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldgrp == old grp_template_t to use
 *    newgrp == new grp_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
static uint32
    grouping_changed (yangdiff_diffparms_t *cp,
                      grp_template_t *oldgrp,
                      grp_template_t *newgrp)
{
    if (status_field_changed(YANG_K_STATUS,
                             oldgrp->status, 
                             newgrp->status, 
                             FALSE, 
                             NULL)) {
        return 1;
    }
    if (str_field_changed(YANG_K_DESCRIPTION,
                          oldgrp->descr, 
                          newgrp->descr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }
    if (str_field_changed(YANG_K_REFERENCE,
                          oldgrp->ref, 
                          newgrp->ref, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    if (typedefQ_changed(cp, 
                         &oldgrp->typedefQ, 
                         &newgrp->typedefQ)) {
        return 1;
    }

    if (groupingQ_changed(cp, 
                          &oldgrp->groupingQ, 
                          &newgrp->groupingQ)) {
        return 1;
    }

    if (datadefQ_changed(cp, 
                         &oldgrp->datadefQ, 
                         &newgrp->datadefQ)) {
        return 1;
    }

    return 0;

} /* grouping_changed */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
 * FUNCTION output_groupingQ_diff
 * 
 *  Output the differences report for a Q of grouping definitions
 *  Not always called for top-level groupings; Can be called
 *  for nested groupings
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old grp_template_t to use
 *    newQ == Q of new grp_template_t to use
 *
 *********************************************************************/
void
    output_groupingQ_diff (yangdiff_diffparms_t *cp,
                           dlq_hdr_t *oldQ,
                           dlq_hdr_t *newQ)
{
    grp_template_t *oldgrp, *newgrp;

    /* borrowing the 'used' flag for marking matched groupings
     * first set all these flags to FALSE
     */
    for (newgrp = (grp_template_t *)dlq_firstEntry(newQ);
         newgrp != NULL;
         newgrp = (grp_template_t *)dlq_nextEntry(newgrp)) {
        newgrp->used = FALSE;
    }

    /* look through the old Q for matching entries in the new Q */
    for (oldgrp = (grp_template_t *)dlq_firstEntry(oldQ);
         oldgrp != NULL;
         oldgrp = (grp_template_t *)dlq_nextEntry(oldgrp)) {

        /* find this grouping in the new Q */
        newgrp = ncx_find_grouping_que(newQ, oldgrp->name);
        if (newgrp) {
            output_one_grouping_diff(cp, oldgrp, newgrp);
            newgrp->used = TRUE;
        } else {
            /* grouping was removed from the new module */
            output_diff(cp, 
                        YANG_K_GROUPING, 
                        oldgrp->name, 
                        NULL, 
                        TRUE);
        }
    }

    /* look for groupings that were added in the new module */
    for (newgrp = (grp_template_t *)dlq_firstEntry(newQ);
         newgrp != NULL;
         newgrp = (grp_template_t *)dlq_nextEntry(newgrp)) {
        if (!newgrp->used) {
            /* this grouping was added in the new version */
            output_diff(cp, 
                        YANG_K_GROUPING, 
                        NULL, 
                        newgrp->name, 
                        TRUE);
        }
    }

} /* output_groupingQ_diff */


/********************************************************************
 * FUNCTION groupingQ_changed
 * 
 *  Check if a (nested) Q of groupings changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old grp_template_t to use
 *    newQ == Q of new grp_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
uint32
    groupingQ_changed (yangdiff_diffparms_t *cp,
                       dlq_hdr_t *oldQ,
                       dlq_hdr_t *newQ)
{
    grp_template_t *oldgrp, *newgrp;

    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    /* borrowing the 'used' flag for marking matched groupings
     * first set all these flags to FALSE
     */
    for (newgrp = (grp_template_t *)dlq_firstEntry(oldQ);
         newgrp != NULL;
         newgrp = (grp_template_t *)dlq_nextEntry(newgrp)) {
        newgrp->used = FALSE;
    }

    /* look through the old type Q for matching types in the new type Q */
    for (oldgrp = (grp_template_t *)dlq_firstEntry(oldQ);
         oldgrp != NULL;
         oldgrp = (grp_template_t *)dlq_nextEntry(oldgrp)) {

        newgrp = ncx_find_grouping_que(newQ, oldgrp->name);
        if (newgrp) {
            if (grouping_changed(cp, oldgrp, newgrp)) {
                return 1;
            }
        } else {
            return 1;
        }
    }

    /* look for groupings that were added in the new module */
    for (newgrp = (grp_template_t *)dlq_firstEntry(newQ);
         newgrp != NULL;
         newgrp = (grp_template_t *)dlq_nextEntry(newgrp)) {
        if (!newgrp->used) {
            return 1;
        }
    }

    return 0;

}  /* groupingQ_changed */

/* END yangdiff_grp.c */



