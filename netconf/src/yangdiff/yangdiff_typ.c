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
/*  FILE: yangdiff_typ.c

                
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

#ifndef _H_ncxtypes
#include  "ncxtypes.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_yangconst
#include  "yangconst.h"
#endif

#ifndef _H_yangdiff
#include  "yangdiff.h"
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
#define YANGDIFF_TYP_DEBUG   1


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
 * FUNCTION output_range_diff
 * 
 *  Output the differences report for one range or length
 *  clause and any sub-clauses,
 *  within a leaf, leaf-list, or typedef definition
 *
 * Only the top-level typedef is checked, instead of
 * following the type chain looking for the first rangedef.
 * This prevents chain mis-alignment false positives
 *
 * Only the top-level rangedef is relevant for validation,
 * since each refinement must be a valid subset of its parent
 * range definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    keyword == range or length keyword
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *
 *********************************************************************/
static void
    output_range_diff (yangdiff_diffparms_t *cp,
                       const xmlChar *keyword,
                       typ_def_t *oldtypdef,
                       typ_def_t *newtypdef)
{
    typ_range_t         *oldrange, *newrange;
    const xmlChar       *oldstr, *newstr;
    ncx_errinfo_t       *olderr, *newerr;
    yangdiff_cdb_t       cdb;
    boolean              isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    oldrange = typ_get_range_con(oldtypdef);
    oldstr = oldrange ? oldrange->rangestr : NULL;
    newrange = typ_get_range_con(newtypdef);
    newstr = newrange ? newrange->rangestr : NULL;
    olderr = typ_get_range_errinfo(oldtypdef);
    newerr = typ_get_range_errinfo(newtypdef);

    if (str_field_changed(keyword, 
                          oldstr, 
                          newstr,
                          isrev, 
                          &cdb)) {
        output_cdb_line(cp, &cdb);
        indent_in(cp);
        output_errinfo_diff(cp, olderr, newerr);
        indent_out(cp);
    }

} /* output_range_diff */


/********************************************************************
 * FUNCTION output_pattern_diff
 * 
 *  Output the differences report for one pattern clause and
 *  any of its sub-clauses, within a leaf, leaf-list, or 
 *  typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldpat == old internal pattern
 *    newpat == new internal pattern
 *    patnum == index number of pattern in the Q [1 .. N]
 *
 *********************************************************************/
static void
    output_pattern_diff (yangdiff_diffparms_t *cp,
                         const typ_pattern_t *oldpat,
                         const typ_pattern_t *newpat,
                         uint32 patnum)
{
    const ncx_errinfo_t *olderr, *newerr;
    const xmlChar       *oldstr, *newstr;
    xmlChar              buff[NCX_MAX_NUMLEN+16], *p;

    oldstr = (oldpat) ? oldpat->pat_str : NULL;
    newstr = (newpat) ? newpat->pat_str : NULL;

    olderr = (oldpat) ? &oldpat->pat_errinfo : NULL;
    newerr = (newpat) ? &newpat->pat_errinfo : NULL;

    p = buff;
    p += xml_strcpy(p, YANG_K_PATTERN);
    *p++ = ' ';
    *p++ = '[';
    p += (uint32)sprintf((char *)p, "%u", patnum);
    *p++ = ']';
    *p = 0;
    
    if (!oldstr && newstr) {
        /* pattern added in new revision */
        output_diff(cp, buff, oldstr, newstr, FALSE);
        indent_in(cp);
        output_errinfo_diff(cp, olderr, newerr);
        indent_out(cp);
    } else if (oldstr && !newstr) {
        /* pattern removed in new revision */
        output_diff(cp, buff, oldstr, newstr, FALSE);
        indent_in(cp);
        output_errinfo_diff(cp, olderr, newerr);
        indent_out(cp);
    } else if (oldstr && newstr) {
        /* check if pattern changed */
        if (xml_strcmp(oldstr, newstr)) {
            output_diff(cp, buff, oldstr, newstr, FALSE);
            indent_in(cp);
            output_errinfo_diff(cp, olderr, newerr);
            indent_out(cp);
        } else if (errinfo_changed(olderr, newerr)) {
            output_mstart_line(cp, buff, oldstr, FALSE);
            indent_in(cp);
            output_errinfo_diff(cp, olderr, newerr);
            indent_out(cp);
        }
    }

} /* output_pattern_diff */


/********************************************************************
 * FUNCTION output_patternQ_diff
 * 
 *  Output the differences report for the Q of pattern clauses
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *
 *********************************************************************/
static void
    output_patternQ_diff (yangdiff_diffparms_t *cp,
                         const typ_def_t *oldtypdef,
                         const typ_def_t *newtypdef)
{
    const typ_pattern_t *oldpat, *newpat;
    uint32               cnt;

    oldpat = typ_get_first_cpattern(oldtypdef);
    newpat = typ_get_first_cpattern(newtypdef);
    cnt = 1;

    while (oldpat || newpat) {

        output_pattern_diff(cp, oldpat, newpat, cnt);

        oldpat = (oldpat) ? typ_get_next_cpattern(oldpat) : NULL;
        newpat = (newpat) ? typ_get_next_cpattern(newpat) : NULL;
        cnt++;
    }

} /* output_patternQ_diff */


/********************************************************************
 * FUNCTION output_eb_type_diff
 * 
 *  Output the differences report for one enum or bits sub-clauses
 *  within a leaf, leaf-list, or typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    name == type name to use
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *    isbits == TRUE if NCX_BT_BITS
 *              FALSE if NCX_BT_ENUM
 *********************************************************************/
static void
    output_eb_type_diff (yangdiff_diffparms_t *cp,
                         typ_def_t *oldtypdef,
                         typ_def_t *newtypdef,
                         boolean isbits)
{
    typ_enum_t      *oldval, *newval;
    dlq_hdr_t       *oldQ, *newQ;
    typ_def_t       *oldbase, *newbase;
    const xmlChar   *kw;
    xmlChar          oldnum[NCX_MAX_NUMLEN];
    xmlChar          newnum[NCX_MAX_NUMLEN];
    yangdiff_cdb_t   cdb[4];
    boolean          isrev;
    uint32           chcount, i;

    oldbase = typ_get_base_typdef(oldtypdef);
    newbase = typ_get_base_typdef(newtypdef);

    oldQ = &oldbase->def.simple.valQ;
    newQ = &newbase->def.simple.valQ;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;
    kw = isbits ? YANG_K_BIT : YANG_K_ENUM;

    /* clear the seen flag to find new enums/bits */
    for (newval = (typ_enum_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_enum_t *)dlq_nextEntry(newval)) {
        newval->flags &= ~TYP_FL_SEEN;
    }

    /* check for matching entries */
    for (oldval = (typ_enum_t *)dlq_firstEntry(oldQ);
         oldval != NULL;
         oldval = (typ_enum_t *)dlq_nextEntry(oldval)) {

        chcount = 0;
        newval = typ_find_enumdef(newQ, oldval->name);
        if (newval) {
            if (isbits) {
                sprintf((char *)oldnum, "%u", oldval->pos);
                sprintf((char *)newnum, "%u", newval->pos);                     
                chcount += str_field_changed(YANG_K_POSITION,
                                             oldnum, newnum,
                                             isrev, &cdb[0]);
            } else {
                sprintf((char *)oldnum, "%d", oldval->val);
                sprintf((char *)newnum, "%d", newval->val);                     
                chcount += str_field_changed(YANG_K_VALUE,
                                             oldnum, newnum,
                                             isrev, &cdb[0]);
            }
            chcount += status_field_changed(YANG_K_STATUS,
                                            oldval->status, newval->status,
                                            isrev, &cdb[1]);
            chcount += str_field_changed(YANG_K_DESCRIPTION,
                                         oldval->descr, newval->descr,
                                         isrev, &cdb[2]);
            chcount += str_field_changed(YANG_K_REFERENCE,
                                         oldval->ref, newval->ref,
                                         isrev, &cdb[3]);
            newval->flags |= TYP_FL_SEEN;
            if (chcount) {
                output_mstart_line(cp, kw, oldval->name, isbits);
                if (cp->edifftype != YANGDIFF_DT_TERSE) {
                    indent_in(cp);
                    for (i=0; i<4; i++) {
                        output_cdb_line(cp, &cdb[i]);
                    }
                    indent_out(cp);
                }
            }
        } else {
            /* removed name in new version */
            output_diff(cp, kw, oldval->name, NULL, isbits);
        }
    }

    /* check for new entries */
    for (newval = (typ_enum_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_enum_t *)dlq_nextEntry(newval)) {
        if ((newval->flags & TYP_FL_SEEN) == 0) {
            output_diff(cp, kw, NULL, newval->name, isbits);
        }
    }

} /* output_eb_type_diff */


/********************************************************************
* FUNCTION unQ_changed
*
* Check if the union Q in the typdef has changed
*
* INPUTS:
*   cp == comparison parameter block to use
*   oldQ == Q of old typ_unionnode_t structs to check
*   newQ == Q of new typ_unionnode_t structs to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
static uint32
    unQ_changed (yangdiff_diffparms_t *cp,
                 dlq_hdr_t *oldQ,
                 dlq_hdr_t *newQ)
{
    typ_unionnode_t     *oldval, *newval;
    typ_def_t           *olddef, *newdef;
    boolean              done;


    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    /* clear the seen flag to be safe */
    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        newval->seen = FALSE;
    }

    oldval = (typ_unionnode_t *)dlq_firstEntry(oldQ);
    newval = (typ_unionnode_t *)dlq_firstEntry(newQ);


    /* check for matching entries */
    done = FALSE;
    while (!done) {
        if (!oldval && !newval) {
            return 0;
        } else if (!oldval && newval) {
            return 1;
        } else if (oldval && !newval) {
            return 1;
        } /* else both old val and new val exist */

        olddef = typ_get_unionnode_ptr(oldval);
        newdef = typ_get_unionnode_ptr(newval);

        if (type_changed(cp, olddef, newdef)) {
            return 1;
        }

        newval->seen = TRUE;

        oldval = (typ_unionnode_t *)dlq_nextEntry(oldval);
        newval = (typ_unionnode_t *)dlq_nextEntry(newval);
    }

    /* check for new entries */
    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        if (!newval->seen) {
            return 1;
        }
    }

    return 0;
    
}  /* unQ_changed */


/********************************************************************
* FUNCTION unQ_match
*
* Find the specified type in the unionnode Q
*
* INPUTS:
*   cp == comparison parameters to use
*   oldval == old typ_unionnode_t struct to check
*   newQ == Q of new typ_unionnode_t structs to check
*   idx == address of return index
*
* OUTPUTS:
*   *idx == index of the unionnode entry found [0..N-1 numbering]
*
* RETURNS:
*   pointer to the found entry
*   NULL if not found
*********************************************************************/
static typ_unionnode_t *
    unQ_match (yangdiff_diffparms_t *cp,
               typ_unionnode_t *oldval,
               dlq_hdr_t *newQ,
               uint32 *idx)
{
    typ_unionnode_t     *newval;
    typ_def_t           *olddef, *newdef;

    *idx = 0;

    olddef = typ_get_unionnode_ptr(oldval);

    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        if (!newval->seen) {
            newdef = typ_get_unionnode_ptr(newval);
            if (!type_changed(cp, olddef, newdef)) {
                return newval;
            }
        }
        (*idx)++;
    }

    return NULL;
    
}  /* unQ_match */


/********************************************************************
* FUNCTION unQ_match_id
*
* Find the specified type by position in the unionnode Q
*
* INPUTS:
*   oldid == index of typ_unionnode_t struct to match
*   newQ == Q of new typ_unionnode_t structs to check
*
* RETURNS:
*   pointer to the found entry
*   NULL if not found
*********************************************************************/
static typ_unionnode_t *
    unQ_match_id (uint32 oldid,
                  dlq_hdr_t *newQ)
{
    typ_unionnode_t     *newval;
    uint32               idx;

    idx = 0;

    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        if (idx==oldid) {
            return (newval->seen) ? NULL : newval;
        } else if (idx>oldid) {
            return NULL;
        }
        idx++;
    }

    return NULL;
    
}  /* unQ_match_id */


/********************************************************************
 * FUNCTION output_union_diff
 * 
 *  Output the differences report for one union sub-clauses
 *  within a leaf, leaf-list, or typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *********************************************************************/
static void
    output_union_diff (yangdiff_diffparms_t *cp,
                       typ_def_t *oldtypdef,
                       typ_def_t *newtypdef)
{
    typ_unionnode_t     *oldval, *newval, *curnew;
    dlq_hdr_t           *oldQ, *newQ;
    typ_def_t           *olddef, *newdef;
    uint32               oldid, newid;
    char                 oldnum[NCX_MAX_NUMLEN];
    char                 newnum[NCX_MAX_NUMLEN];

    olddef = typ_get_base_typdef(oldtypdef);
    newdef = typ_get_base_typdef(newtypdef);

    oldQ = &olddef->def.simple.unionQ,
    newQ = &newdef->def.simple.unionQ;

    if (!unQ_changed(cp, oldQ, newQ)) {
        return;
    }

    oldid = 0;
    newid = 0;

    /* clear the seen flag to be safe */
    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        newval->seen = FALSE;
    }

    /* start the diff output */
    output_mstart_line(cp, YANG_K_UNION, NULL, FALSE);
    if (cp->edifftype == YANGDIFF_DT_TERSE) {
        return;
    }

    indent_in(cp);

    /* check for matching unionnode entries */
    for (oldval = (typ_unionnode_t *)dlq_firstEntry(oldQ);
         oldval != NULL;
         oldval = (typ_unionnode_t *)dlq_nextEntry(oldval), oldid++) {

        curnew = NULL;
        sprintf(oldnum, "[%u]", oldid);
        olddef = typ_get_unionnode_ptr(oldval);

        /* first try the corresponded entry if available */
        newval = unQ_match_id(oldid, newQ);
        if (newval) {
            curnew = newval;
            newid = oldid;
            sprintf(newnum, "[%u]", newid);
            newdef = typ_get_unionnode_ptr(newval);

            /* if the corresponding entry did not change
             * then this is a match and continue to next type
             */
            if (!type_changed(cp, olddef, newdef)) {
                newval->seen = TRUE;
                continue;
            }
        }
            
        /* did not match the corresponding entry,
         * so see if the typdef moved in the new union
         */
        newval = unQ_match(cp, oldval, newQ, &newid);
        if (newval) {
            newval->seen = TRUE;
            if (oldid != newid) {
                sprintf(newnum, "[%u]", newid);
                /* old union node was moved in new version */
                output_diff(cp, YANG_K_TYPE, 
                            (const xmlChar *)oldnum, 
                            (const xmlChar *)newnum, TRUE);
            }
        } else if (curnew) {
            /* type node was changed in the new union */
            curnew->seen = TRUE;
            newdef = typ_get_unionnode_ptr(curnew);
            sprintf(newnum, "[%u]", oldid);
            output_one_type_diff(cp, olddef, newdef);
        } else {
            /* old union node was removed in new version */
            output_diff(cp, YANG_K_TYPE,
                        (const xmlChar *)oldnum, NULL, TRUE);
        }
    }

    indent_out(cp);

    /* check for new entries */
    newid = 0;
    for (newval = (typ_unionnode_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_unionnode_t *)dlq_nextEntry(newval)) {
        if (!newval->seen) {
            sprintf(newnum, "[%u]", newid);
            output_diff(cp, YANG_K_TYPE, NULL,
                        (const xmlChar *)newnum, TRUE);
        }
        newid++;
    }

} /* output_union_diff */


/********************************************************************
* FUNCTION pattern_changed
*
* Check if the pattern-stmt changed at all
*
* INPUTS:
*   oldpat == old pattern struct to check
*   newpat == new pattern struct to check
*
* RETURNS:
*   1 if struct changed
*   0 if struct not changed
*********************************************************************/
static uint32
    pattern_changed (const typ_pattern_t *oldpat,
                     const typ_pattern_t *newpat)
{
    const ncx_errinfo_t *olderr, *newerr;
    
    /* check pattern string is the same */
    if ((!oldpat && newpat) || (oldpat && !newpat)) {
        return 1;
    } else if (oldpat && newpat) {
        if (xml_strcmp(oldpat->pat_str, 
                       newpat->pat_str)) {
            return 1;
        }
    } else {
        return 0;       /* no pattern defined */
    }

    olderr = (oldpat) ? &oldpat->pat_errinfo : NULL;
    newerr = (newpat) ? &newpat->pat_errinfo : NULL;

    /* pattern string is the same, check error stuff */
    return errinfo_changed(olderr, newerr);
    
}  /* pattern_changed */


/********************************************************************
* FUNCTION patternQ_changed
*
* Check if the Q of pattern-stmts changed at all
*
* INPUTS:
*   oldtypdef == old type def struct to check
*   newtypdef == new type def struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
static uint32
    patternQ_changed (const typ_def_t *oldtypdef,
                      const typ_def_t *newtypdef)
{
    const typ_pattern_t  *oldpat, *newpat;
    uint32                oldpatcnt, newpatcnt;
    
    oldpatcnt = typ_get_pattern_count(oldtypdef);
    newpatcnt = typ_get_pattern_count(newtypdef);

    if (oldpatcnt != newpatcnt) {
        return 1;
    }

    if (!oldpatcnt) {
        return 0;
    }

    oldpat = typ_get_first_cpattern(oldtypdef);
    newpat = typ_get_first_cpattern(newtypdef);

    while (oldpat && newpat) {

        if (pattern_changed(oldpat, newpat)) {
            return 1;
        }

        oldpat = typ_get_next_cpattern(oldpat);
        newpat = typ_get_next_cpattern(newpat);
    }

    return 0;

}  /* patternQ_changed */


/********************************************************************
* FUNCTION range_changed
*
* Check if the range-stmt or length-stmt changed at all
*
* INPUTS:
*   oldtypdef == old type def struct to check
*   newtypdef == new type def struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
static uint32
    range_changed (typ_def_t *oldtypdef,
                   typ_def_t *newtypdef)
{
    typ_range_t     *oldr, *newr;
    const xmlChar   *oldstr, *newstr;
    ncx_errinfo_t   *olderr, *newerr;

    oldr = typ_get_range_con(oldtypdef);
    oldstr = (oldr) ? oldr->rangestr : NULL;
    newr = typ_get_range_con(newtypdef);
    newstr = (newr) ? newr->rangestr : NULL;

    /* check range string is the same */
    if (str_field_changed(YANG_K_RANGE, oldstr, newstr, FALSE, NULL)) {
        return 1;
    }

    /* range string is the same, check error stuff */
    olderr = typ_get_range_errinfo(oldtypdef);
    newerr = typ_get_range_errinfo(newtypdef);
    return errinfo_changed(olderr, newerr);
    
}  /* range_changed */


/********************************************************************
* FUNCTION ebQ_changed
*
* Check if the enum/bits Q in the typdef has changed, used for 
* enums and bits
*
* INPUTS:
*   oldQ == Q of old typ_enum_t structs to check
*   newQ == Q of new typ_enum_t structs to check
*   isbits == TRUE if checking for a bits datatype
*             FALSE if checking for an enum datatype
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
static uint32
    ebQ_changed (dlq_hdr_t *oldQ,
                 dlq_hdr_t *newQ,
                 boolean isbits)
{
    typ_enum_t     *oldval, *newval;

    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    /* clear the seen flag to be safe */
    for (newval = (typ_enum_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_enum_t *)dlq_nextEntry(newval)) {
        newval->flags &= ~TYP_FL_SEEN;
    }

    /* check for matching entries */
    for (oldval = (typ_enum_t *)dlq_firstEntry(oldQ);
         oldval != NULL;
         oldval = (typ_enum_t *)dlq_nextEntry(oldval)) {

        newval = typ_find_enumdef(newQ, oldval->name);
        if (newval) {
            if (isbits) {
                if (oldval->pos != newval->pos) {
                    return 1;
                }
            } else {
                if (oldval->val != newval->val) {
                    return 1;
                }
            }
            if (str_field_changed(YANG_K_DESCRIPTION,
                                  oldval->descr, newval->descr,
                                  FALSE, NULL)) {
                return 1;
            }
            if (str_field_changed(YANG_K_REFERENCE,
                                  oldval->ref, newval->ref,
                                  FALSE, NULL)) {
                return 1;
            }
            if (status_field_changed(YANG_K_STATUS,
                                     oldval->status, newval->status,
                                     FALSE, NULL)) {
                return 1;
            }
            /* mark new enum as used */
            newval->flags |= TYP_FL_SEEN;
        } else {
            /* removed name in new version */
            return 1;
        }
    }

    /* check for new entries */
    for (newval = (typ_enum_t *)dlq_firstEntry(newQ);
         newval != NULL;
         newval = (typ_enum_t *)dlq_nextEntry(newval)) {
        if ((newval->flags & TYP_FL_SEEN) == 0) {
            return 1;
        }
    }

    return 0;
    
}  /* ebQ_changed */


/********************************************************************
 * FUNCTION output_one_typedef_diff
 * 
 *  Output the differences report for one typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtyp == old typedef
 *    newtyp == new typedef
 *
 *********************************************************************/
static void
    output_one_typedef_diff (yangdiff_diffparms_t *cp,
                             typ_template_t *oldtyp,
                             typ_template_t *newtyp)
{
    yangdiff_cdb_t  typcdb[5];
    uint32          changecnt, i;
    boolean         isrev, tchanged;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;

    tchanged = FALSE;
    changecnt = 0;

    if (type_changed(cp, &oldtyp->typdef, &newtyp->typdef)) {
        tchanged = TRUE;
        changecnt++;
    }

    changecnt += str_field_changed(YANG_K_UNITS,
                                   oldtyp->units, newtyp->units, 
                                   isrev, &typcdb[0]);
    changecnt += str_field_changed(YANG_K_DEFAULT,
                                   oldtyp->defval, newtyp->defval, 
                                   isrev, &typcdb[1]);
    changecnt += status_field_changed(YANG_K_STATUS,
                                      oldtyp->status, newtyp->status, 
                                      isrev, &typcdb[2]);
    changecnt += str_field_changed(YANG_K_DESCRIPTION,
                                   oldtyp->descr, newtyp->descr, 
                                   isrev, &typcdb[3]);
    changecnt += str_field_changed(YANG_K_REFERENCE,
                                   oldtyp->ref, newtyp->ref, 
                                   isrev, &typcdb[4]);
    if (changecnt == 0) {
        return;
    }

    /* generate the diff output, based on the requested format */
    output_mstart_line(cp, YANG_K_TYPEDEF, oldtyp->name, TRUE);

    if (cp->edifftype == YANGDIFF_DT_TERSE) {
        return;
    }

    indent_in(cp);

    for (i=0; i<5; i++) {
        if (typcdb[i].changed) {
            output_cdb_line(cp, &typcdb[i]);
        }
    }

    if (tchanged) {
        output_one_type_diff(cp, &oldtyp->typdef, &newtyp->typdef);
    }

    indent_out(cp);

} /* output_one_typedef_diff */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION type_changed
*
* Check if the type clause and sub-clauses changed at all
*
* INPUTS:
*   cp == compare parameters to use
*   oldtypdef == old type def struct to check
*   newtypdef == new type def struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
uint32
    type_changed (yangdiff_diffparms_t *cp,
                  typ_def_t *oldtypdef,
                  typ_def_t *newtypdef)
{
    typ_def_t         *oldtest, *newtest;
    const xmlChar     *oldpath, *newpath;
    ncx_btype_t        oldbtyp, newbtyp;

#ifdef DEBUG
    if (!oldtypdef || !newtypdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 1;
    }
#endif

    if (oldtypdef->tclass != newtypdef->tclass) {
        return 1;
    }
    if (prefix_field_changed(cp->oldmod, 
                             cp->newmod,
                             oldtypdef->prefix, 
                             newtypdef->prefix)) {
        return 1;
    }
    if (str_field_changed(NULL, 
                          oldtypdef->typenamestr, 
                          newtypdef->typenamestr, 
                          FALSE, 
                          NULL)) {
        return 1;
    }

    switch (oldtypdef->tclass) {
    case NCX_CL_BASE:
        return (uint32)((oldtypdef->def.base == newtypdef->def.base) ? 0 : 1);
    case NCX_CL_SIMPLE:
        oldbtyp = typ_get_basetype(oldtypdef);
        newbtyp = typ_get_basetype(newtypdef);
        if (oldbtyp != newbtyp) {
            return 1;
        }
        if (oldbtyp == NCX_BT_LEAFREF) {
            oldpath = typ_get_leafref_path(oldtypdef);
            newpath = typ_get_leafref_path(newtypdef);
            return str_field_changed(YANG_K_PATH, 
                                     oldpath, 
                                     newpath, 
                                     FALSE, 
                                     NULL);
        } else if (typ_is_string(oldbtyp)) {
            if (patternQ_changed(oldtypdef, newtypdef)) {
                return 1;
            }
            if (range_changed(oldtypdef, newtypdef)) {
                return 1;
            }
        } else if (typ_is_number(oldbtyp)) {
            if (range_changed(oldtypdef, newtypdef)) {
                return 1;
            }
        } else {
            switch (oldbtyp) {
            case NCX_BT_BITS:
                return ebQ_changed(&oldtypdef->def.simple.valQ,
                                   &newtypdef->def.simple.valQ, 
                                   TRUE);
            case NCX_BT_ENUM:
                return ebQ_changed(&oldtypdef->def.simple.valQ,
                                   &newtypdef->def.simple.valQ,
                                   FALSE);
            case NCX_BT_UNION:
                return unQ_changed(cp, 
                                   &oldtypdef->def.simple.unionQ,
                                   &newtypdef->def.simple.unionQ);
            default:
                ;
            }
        }
        return 0;
    case NCX_CL_COMPLEX:
        /* not supported for NCX complex types!!! */
        return 0;     
    case NCX_CL_NAMED:
        oldtest = typ_get_new_named(oldtypdef);
        newtest = typ_get_new_named(newtypdef);
        if ((!oldtest && newtest) || (oldtest && !newtest)) {
            return 1;
        } else if (oldtest && newtest) {
            if (type_changed(cp, oldtest, newtest)) {
                return 1;
            }
        }
        return 0;
        /*** do not look deep into named typed
         *** add switch to check this
         *** type_changed(cp, &oldtypdef->def.named.typ->typdef,
         ***                &newtypdef->def.named.typ->typdef);
         ***/
    case NCX_CL_REF:
        return type_changed(cp, 
                            oldtypdef->def.ref.typdef,
                            newtypdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
    /*NOTREACHED*/

} /* type_changed */


/********************************************************************
 * FUNCTION typedef_changed
 * 
 *  Check if a (nested) typedef changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtyp == old typ_template_t to use
 *    newtyp == new typ_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
uint32
    typedef_changed (yangdiff_diffparms_t *cp,
                     typ_template_t *oldtyp,
                     typ_template_t *newtyp)
{
    if (type_changed(cp, &oldtyp->typdef, &newtyp->typdef)) {
        return 1;
    }

    if (str_field_changed(YANG_K_UNITS,
                          oldtyp->units, 
                          newtyp->units, 
                          FALSE, 
                          NULL)) {
        return 1;
    }
    if (str_field_changed(YANG_K_DEFAULT,
                          oldtyp->defval, 
                          newtyp->defval, 
                          FALSE, 
                          NULL)) {
        return 1;
    }
    if (status_field_changed(YANG_K_STATUS,
                             oldtyp->status, 
                             newtyp->status, 
                             FALSE,
                             NULL)) {
        return 1;
    }
    if (str_field_changed(YANG_K_DESCRIPTION,
                          oldtyp->descr,
                          newtyp->descr, 
                          FALSE,
                          NULL)) {
        return 1;
    }
    if (str_field_changed(YANG_K_REFERENCE,
                          oldtyp->ref,
                          newtyp->ref, 
                          FALSE,
                          NULL)) {
        return 1;
    }

    return 0;

} /* typedef_changed */


/********************************************************************
 * FUNCTION typedefQ_changed
 * 
 *  Check if a (nested) Q of typedefs changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old typ_template_t to use
 *    newQ == Q of new typ_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
uint32
    typedefQ_changed (yangdiff_diffparms_t *cp,
                      dlq_hdr_t *oldQ,
                      dlq_hdr_t *newQ)
{
    typ_template_t *oldtyp, *newtyp;

    if (dlq_count(oldQ) != dlq_count(newQ)) {
        return 1;
    }

    /* borrowing the 'used' flag for marking matched typedefs
     * first set all these flags to FALSE
     */
    for (newtyp = (typ_template_t *)dlq_firstEntry(oldQ);
         newtyp != NULL;
         newtyp = (typ_template_t *)dlq_nextEntry(newtyp)) {
        newtyp->used = FALSE;
    }

    /* look through the old type Q for matching types in the new type Q */
    for (oldtyp = (typ_template_t *)dlq_firstEntry(oldQ);
         oldtyp != NULL;
         oldtyp = (typ_template_t *)dlq_nextEntry(oldtyp)) {

        /* find this revision in the new module */
        newtyp = ncx_find_type_que(newQ, oldtyp->name);
        if (newtyp) {
            if (typedef_changed(cp, oldtyp, newtyp)) {
                return 1;
            }
        } else {
            return 1;
        }
    }

    /* look for typedefs that were added in the new module */
    for (newtyp = (typ_template_t *)dlq_firstEntry(newQ);
         newtyp != NULL;
         newtyp = (typ_template_t *)dlq_nextEntry(newtyp)) {
        if (!newtyp->used) {
            return 1;
        }
    }

    return 0;

}  /* typedefQ_changed */


/********************************************************************
 * FUNCTION output_typedefQ_diff
 * 
 *  Output the differences report for a Q of typedef definitions
 *  Not always called for top-level typedefs; Can be called
 *  for nested typedefs
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old typ_template_t to use
 *    newQ == Q of new typ_template_t to use
 *
 *********************************************************************/
void
    output_typedefQ_diff (yangdiff_diffparms_t *cp,
                          dlq_hdr_t *oldQ,
                          dlq_hdr_t *newQ)
{
    typ_template_t *oldtyp, *newtyp;

    /* borrowing the 'used' flag for marking matched typedefs
     * first set all these flags to FALSE
     */
    for (newtyp = (typ_template_t *)dlq_firstEntry(newQ);
         newtyp != NULL;
         newtyp = (typ_template_t *)dlq_nextEntry(newtyp)) {
        newtyp->used = FALSE;
    }

    /* look through the old type Q for matching types in the new type Q */
    for (oldtyp = (typ_template_t *)dlq_firstEntry(oldQ);
         oldtyp != NULL;
         oldtyp = (typ_template_t *)dlq_nextEntry(oldtyp)) {

        /* find this revision in the new module */
        newtyp = ncx_find_type_que(newQ, oldtyp->name);
        if (newtyp) {
            output_one_typedef_diff(cp, oldtyp, newtyp);
            newtyp->used = TRUE;
        } else {
            /* typedef was removed from the new module */
            output_diff(cp, YANG_K_TYPEDEF, oldtyp->name, NULL, TRUE);
        }
    }

    /* look for typedefs that were added in the new module */
    for (newtyp = (typ_template_t *)dlq_firstEntry(newQ);
         newtyp != NULL;
         newtyp = (typ_template_t *)dlq_nextEntry(newtyp)) {
        if (!newtyp->used) {
            /* this typedef was added in the new version */
            output_diff(cp, YANG_K_TYPEDEF, NULL, newtyp->name, TRUE);
        }
    }

} /* output_typedefQ_diff */


/********************************************************************
 * FUNCTION output_one_type_diff
 * 
 *  Output the differences report for one type section
 *  within a leaf, leaf-list, or typedef definition
 *
 * type_changed should be called first to determine
 * if the type actually changed.  Otherwise a 'M typedef foo'
 * output line will result and be a false positive
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *
 *********************************************************************/
void
    output_one_type_diff (yangdiff_diffparms_t *cp,
                          typ_def_t *oldtypdef,
                          typ_def_t *newtypdef)
{
    xmlChar            *p, *oldp, *newp;
    const xmlChar      *oldname, *newname;
    const xmlChar      *oldpath, *newpath;
    yangdiff_cdb_t      typcdb[5];
    ncx_btype_t         oldbtyp, newbtyp;
    ncx_tclass_t        oldclass, newclass;
    boolean             isrev;

    isrev = (cp->edifftype==YANGDIFF_DT_REVISION) ? TRUE : FALSE;

    oldclass = oldtypdef->tclass;
    newclass = newtypdef->tclass;

    oldname = typ_get_name(oldtypdef);
    newname = typ_get_name(newtypdef);

    oldbtyp = typ_get_basetype(oldtypdef);
    newbtyp = typ_get_basetype(newtypdef);

    if (oldbtyp == NCX_BT_LEAFREF) {
        oldpath = typ_get_leafref_path(oldtypdef);
    } else {
        oldpath = NULL;
    }
    if (newbtyp==NCX_BT_LEAFREF) {
        newpath = typ_get_leafref_path(newtypdef);
    } else {
        newpath = NULL;
    }

    /* check if there is a module prefix involved
     * in the change.  This may be a false positive
     * if the prefix simply changed
     * create YANG QNames for the type change record
     * use the scratch buffer cp->buff for both strings
     */
    p = cp->buff;
    if (oldtypdef->prefix) {
        oldp = p;
        p += xml_strcpy(p, oldtypdef->prefix);
        *p++ = ':';
        p += xml_strcpy(p, oldname);
        if (oldtypdef->tclass==NCX_CL_NAMED) {
            *p++ = ' ';
            *p++ = '(';
            p += xml_strcpy(p, (const xmlChar *)
                            tk_get_btype_sym(oldbtyp));
            *p++ = ')';
            *p = 0;         
        }
        p++;   /* leave last NULL char in place */
        oldname = oldp;
    }
    if (newtypdef->prefix) {
        newp = p;
        p += xml_strcpy(p, newtypdef->prefix);
        *p++ = ':';
        p += xml_strcpy(p, newname);
        if (newtypdef->tclass==NCX_CL_NAMED) {
            *p++ = ' ';
            *p++ = '(';
            p += xml_strcpy(p, (const xmlChar *)
                            tk_get_btype_sym(newbtyp));
            *p++ = ')';
            *p = 0;
        }
        newname = newp;
    }

    /* Print the type name change set only if it really
     * changed; otherwise force an M line to be printed
     */
    if (str_field_changed(YANG_K_TYPE, oldname, newname, 
                          isrev, &typcdb[0])) {
        output_cdb_line(cp, &typcdb[0]);
    } else {
        output_mstart_line(cp, YANG_K_TYPE, NULL, FALSE);
    }

    /* check invalid change of builtin type */
    if (oldbtyp != newbtyp) {
        /* need to figure out what type changes are really allowed */
        if (!((typ_is_number(oldbtyp) && typ_is_number(newbtyp)) ||
              (typ_is_string(oldbtyp) && typ_is_string(newbtyp)))) {
            /* ses_putstr(cp->scb, (const xmlChar *)" (invalid)"); */
            return;
        }
    }

    /* check if that is all the data requested */
    if (cp->edifftype == YANGDIFF_DT_TERSE) {
        return;
    }

    /* check corner-case, no sub-fields to compare */
    if (oldclass==NCX_CL_BASE && newclass==NCX_CL_BASE) {
        /* type field is a plain builtin like 'int32;' */
        return;
    }

    /* in all modes except 'normal', indent 1 level and
     * show the specific sub-clauses that have changed
     */
    indent_in(cp);

    /* special case -- check leafref here */
    if (oldpath || newpath) {
        output_diff(cp, YANG_K_PATH, oldpath, newpath, FALSE);
    }

    if (typ_is_number(oldbtyp)) {
        output_range_diff(cp, YANG_K_RANGE, oldtypdef, newtypdef);
    } else if (typ_is_string(oldbtyp)) {
        output_range_diff(cp, YANG_K_LENGTH, oldtypdef, newtypdef);
        output_patternQ_diff(cp, oldtypdef, newtypdef);
    } else {
        switch (oldbtyp) {
        case NCX_BT_ENUM:
            output_eb_type_diff(cp, oldtypdef, newtypdef, FALSE);
            break;
        case NCX_BT_BITS:
            output_eb_type_diff(cp, oldtypdef, newtypdef, TRUE);
            break;
        case NCX_BT_UNION:
            output_union_diff(cp, oldtypdef, newtypdef);
            break;
        default:
            ;
        }
    }

    indent_out(cp);

} /* output_one_type_diff */


/* END yangdiff_typ.c */



