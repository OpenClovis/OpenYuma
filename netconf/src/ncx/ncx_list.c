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
/*  FILE: ncx_list.c

                
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

#ifndef _H_ncx_list
#include "ncx_list.h"
#endif

#ifndef _H_ncx_num
#include "ncx_num.h"
#endif

#ifndef _H_ncx_str
#include "ncx_str.h"
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
* FUNCTION ncx_new_list
* 
* Malloc Initialize an allocated ncx_list_t
*
* INPUTS:
*    btyp == type of list desired
*
* RETURNS:
*   pointer to new entry, or NULL if memory error
*********************************************************************/
ncx_list_t *
    ncx_new_list (ncx_btype_t btyp)
{
    ncx_list_t *list;

    list = m__getObj(ncx_list_t);
    if (list) {
        ncx_init_list(list, btyp);
    }
    return list;

} /* ncx_new_list */


/********************************************************************
* FUNCTION ncx_init_list
* 
* Initialize an allocated ncx_list_t
*
* INPUTS:
*    list == pointer to ncx_list_t memory
*    btyp == base type for the list
*********************************************************************/
void
    ncx_init_list (ncx_list_t *list,
                   ncx_btype_t btyp)
{
    
#ifdef DEBUG
    if (!list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    list->btyp = btyp;
    dlq_createSQue(&list->memQ);

} /* ncx_init_list */


/********************************************************************
* FUNCTION ncx_clean_list
* 
* Scrub the memory of a ncx_list_t but do not delete it
*
* INPUTS:
*    list == ncx_list_t struct to clean
*********************************************************************/
void
    ncx_clean_list (ncx_list_t *list)
{
    ncx_lmem_t  *lmem;

    if (!list || list->btyp == NCX_BT_NONE) {
        return;
    }

    while (!dlq_empty(&list->memQ)) {
        lmem = (ncx_lmem_t *)dlq_deque(&list->memQ);
        ncx_clean_lmem(lmem, list->btyp);
        m__free(lmem);
    }

    list->btyp = NCX_BT_NONE;
    /* leave the list->memQ ready to use again */

} /* ncx_clean_list */


/********************************************************************
* FUNCTION ncx_free_list
* 
* Clean and free an allocated ncx_list_t
*
* INPUTS:
*    list == pointer to ncx_list_t memory
*********************************************************************/
void
    ncx_free_list (ncx_list_t *list)
{
#ifdef DEBUG
    if (!list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    ncx_clean_list(list);
    m__free(list);

} /* ncx_free_list */


/********************************************************************
* FUNCTION ncx_list_cnt
* 
* Get the number of entries in the list
*
* INPUTS:
*    list == pointer to ncx_list_t memory
* RETURNS:
*    number of entries counted
*********************************************************************/
uint32
    ncx_list_cnt (const ncx_list_t *list)
{
    const ncx_lmem_t *lmem;
    uint32      cnt;

#ifdef DEBUG
    if (!list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    cnt = 0;
    for (lmem = (const ncx_lmem_t *)dlq_firstEntry(&list->memQ);
         lmem != NULL;
         lmem = (const ncx_lmem_t *)dlq_nextEntry(lmem)) {
        cnt++;
    }
    return cnt;

} /* ncx_list_cnt */


/********************************************************************
* FUNCTION ncx_list_empty
* 
* Check if the list is empty or not
*
* INPUTS:
*    list == pointer to ncx_list_t memory
* RETURNS:
*    TRUE if list is empty
*    FALSE otherwise
*********************************************************************/
boolean
    ncx_list_empty (const ncx_list_t *list)
{
#ifdef DEBUG
    if (!list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TRUE;
    }
#endif
    
    return dlq_empty(&list->memQ);

} /* ncx_list_empty */


/********************************************************************
* FUNCTION ncx_string_in_list
* 
* Check if the string value is in the list
* List type must be string based, or an enum
*
* INPUTS:
*     str == string to find in the list
*     list == slist to check
*
* RETURNS:
*     TRUE if string is found; FALSE otherwise
*********************************************************************/
boolean
    ncx_string_in_list (const xmlChar *str,
                        const ncx_list_t *list)
{
    const ncx_lmem_t *lmem;

#ifdef DEBUG
    if (!str || !list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* screen the list base type */
    switch (list->btyp) {
    case NCX_BT_STRING:
    case NCX_BT_ENUM:
    case NCX_BT_BITS:
        break;
    default:
        SET_ERROR(ERR_NCX_WRONG_TYPE);
        return FALSE;
    }

    /* search the list for a match */
    for (lmem = (const ncx_lmem_t *)dlq_firstEntry(&list->memQ);
         lmem != NULL;
         lmem = (const ncx_lmem_t *)dlq_nextEntry(lmem)) {

        switch (list->btyp) {
        case NCX_BT_ENUM:
            if (!xml_strcmp(str, lmem->val.enu.name)) {
                return TRUE;
            }
            break;
        case NCX_BT_BITS:
            if (!xml_strcmp(str, lmem->val.bit.name)) {
                return TRUE;
            }
            break;
        default:
            if (!xml_strcmp(str, lmem->val.str)) {
                return TRUE;
            }
        }
    }

    return FALSE;

}  /* ncx_string_in_list */


/********************************************************************
* FUNCTION ncx_compare_lists
* 
* Compare 2 ncx_list_t struct contents
*
* Expected data type (NCX_BT_SLIST)
*
* INPUTS:
*     list1 == first number
*     list2 == second number
* RETURNS:
*     -1 if list1 is < list2
*      0 if list1 == list2   (also for error, after SET_ERROR called)
*      1 if list1 is > list2
*********************************************************************/
int32
    ncx_compare_lists (const ncx_list_t *list1,
                       const ncx_list_t *list2)
{
    const ncx_lmem_t  *s1, *s2;
    int                retval;

#ifdef DEBUG
    if (!list1 || !list2) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return -1;
    }
    if (list1->btyp != list2->btyp) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return -1;
    }   
#endif

    /* get start strings */
    s1 = (const ncx_lmem_t *)dlq_firstEntry(&list1->memQ);
    s2 = (const ncx_lmem_t *)dlq_firstEntry(&list2->memQ);
        
    /* have 2 start structs to compare */
    for (;;) {
        if (!s1 && !s2) {
            return 0;
        } else if (!s1) {
            return -1;
        } else if (!s2) {
            return 1;
        }

        if (typ_is_string(list1->btyp)) {
            retval = ncx_compare_strs(&s1->val.str, 
                                      &s2->val.str, 
                                      NCX_BT_STRING);
        } else if (typ_is_number(list1->btyp)) {
            retval = ncx_compare_nums(&s1->val.num, 
                                      &s2->val.num,
                                      list1->btyp);
        } else {
            switch (list1->btyp) {
            case NCX_BT_BITS:
                retval = ncx_compare_bits(&s1->val.bit, 
                                          &s2->val.bit);
                break;
            case NCX_BT_ENUM:
                retval = ncx_compare_enums(&s1->val.enu, 
                                           &s2->val.enu);
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
                return 0;
            }
        }

        switch (retval) {
        case -1:
            return -1;
        case 0:
            break;
        case 1:
            return 1;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return 0;
        }

        s1 = (const ncx_lmem_t *)dlq_nextEntry(s1);
        s2 = (const ncx_lmem_t *)dlq_nextEntry(s2);
    }
    /*NOTREACHED*/

}  /* ncx_compare_lists */


/********************************************************************
* FUNCTION ncx_copy_list
* 
* Copy the contents of list1 to list2
* Supports base type NCX_BT_SLIST
*
* A partial copy may occur, and list2 should be properly cleaned
* and freed, even if an error is returned
*
* INPUTS:
*     list1 == first list
*     list2 == second list
*
* RETURNS:
*     status
*********************************************************************/
status_t
    ncx_copy_list (const ncx_list_t *list1,
                   ncx_list_t *list2)
{
    const ncx_lmem_t *lmem;
    ncx_lmem_t       *lcopy;
    status_t                 res;

#ifdef DEBUG
    if (!list1 || !list2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    list2->btyp = list1->btyp;
    dlq_createSQue(&list2->memQ);

    /* go through all the list members and copy each one */
    for (lmem = (const ncx_lmem_t *)dlq_firstEntry(&list1->memQ);
         lmem != NULL;
         lmem = (const ncx_lmem_t *)dlq_nextEntry(lmem)) {
        lcopy = ncx_new_lmem();
        if (!lcopy) {
            return ERR_INTERNAL_MEM;
        }

        /* copy the string or number from lmem to lcopy */
        switch (list1->btyp) {
        case NCX_BT_STRING:
            res = ncx_copy_str(&lmem->val.str, &lcopy->val.str, list1->btyp);
            break;
        case NCX_BT_BITS:
            lcopy->val.bit.pos = lmem->val.bit.pos;
            lcopy->val.bit.dname = xml_strdup(lmem->val.bit.name);
            if (!lcopy->val.bit.dname) {
                res = ERR_INTERNAL_MEM;
            } else {
                lcopy->val.bit.name = lcopy->val.bit.dname;
            }
            break;
        case NCX_BT_ENUM:
            lcopy->val.enu.val = lmem->val.enu.val;
            lcopy->val.enu.dname = xml_strdup(lmem->val.enu.name);
            if (!lcopy->val.enu.dname) {
                res = ERR_INTERNAL_MEM;
            } else {
                lcopy->val.enu.name = lcopy->val.enu.dname;
            }
            break;
        case NCX_BT_BOOLEAN:
            lcopy->val.boo = lmem->val.boo;
            break;
        default:
            if (typ_is_number(list1->btyp)) {
                res = ncx_copy_num(&lmem->val.num, 
                                   &lcopy->val.num, list1->btyp);
            } else {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        if (res != NO_ERR) {
            ncx_free_lmem(lcopy, list1->btyp);
            return res;
        }

        /* save lcopy in list2 */
        dlq_enque(lcopy, &list2->memQ);
    }
    return NO_ERR;

}  /* ncx_copy_list */


/********************************************************************
* FUNCTION ncx_merge_list
* 
* The merge function is handled specially for lists.
* The contents are not completely replaced like a string.
* Instead, only new entries from src are added to the dest list.
*
* NCX merge algorithm for lists:
*
* If list types not the same, then error exit;
*
* If allow_dups == FALSE:
*    check if entry exists; if so, exit;
*
*   Merge src list member into dest, based on mergetyp enum
* }
*
* INPUTS:
*    src == ncx_list_t struct to merge from
*    dest == ncx_list_t struct to merge into
*    mergetyp == type of merge used for this list
*    allow_dups == TRUE if this list allows duplicate values
*    
* OUTPUTS:
*    ncx_lmem_t structs will be moved from the src to dest as needed
*
* RETURNS:
*   none
*********************************************************************/
void
    ncx_merge_list (ncx_list_t *src,
                    ncx_list_t *dest,
                    ncx_merge_t mergetyp,
                    boolean allow_dups)
{
    ncx_lmem_t      *lmem, *dest_lmem;

    if (!src || !dest) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (src->btyp != dest->btyp) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    /* get rid of dups in the src list if duplicates not allowed */
    if (!allow_dups) {
        for (dest_lmem = (ncx_lmem_t *)dlq_firstEntry(&dest->memQ);
             dest_lmem != NULL;
             dest_lmem = (ncx_lmem_t *)dlq_nextEntry(dest_lmem)) {

            lmem = ncx_find_lmem(src, dest_lmem);
            if (lmem) {
                dlq_remove(lmem);
                ncx_free_lmem(lmem, dest->btyp);
            }
        }
    }

    /* transfer the source members to the dest list */
    while (!dlq_empty(&src->memQ)) {

        /* pick an entry to merge, reverse of the merge type
         * to preserve the source order in the dest list
         */
        switch (mergetyp) {
        case NCX_MERGE_FIRST:
            lmem = (ncx_lmem_t *)dlq_lastEntry(&src->memQ);
            break;
        case NCX_MERGE_LAST:
        case NCX_MERGE_SORT:
            lmem = (ncx_lmem_t *)dlq_firstEntry(&src->memQ);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }
        if (lmem) {
            dlq_remove(lmem);

            /* merge lmem into the dest list */
            ncx_insert_lmem(dest, lmem, mergetyp);
        } /* else should not happen since dlq_empty is false */
    }

}  /* ncx_merge_list */


/********************************************************************
* FUNCTION ncx_set_strlist
* 
* consume a generic string list with no type checking
* Convert a text line into an ncx_list_t using NCX_BT_STRING
* as the list type. Must call ncx_init_list first !!!
*
* INPUTS:
*     liststr == list value in string form
*     list == ncx_list_t that should be initialized and
*             filled with the values from the string
*
* RETURNS:
*     status
*********************************************************************/
status_t
    ncx_set_strlist (const xmlChar *liststr,
                  ncx_list_t *list)
{
#ifdef DEBUG
    if (!liststr || !list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    ncx_init_list(list, NCX_BT_STRING);
    return ncx_set_list(NCX_BT_STRING, liststr, list);

}  /* ncx_set_strlist */


/********************************************************************
* FUNCTION ncx_set_list
* 
* consume a generic string list with base type checking
* Parse the XML input as an NCX_BT_SLIST
* Do not check the individual strings against any restrictions
* Just check that the strings parse as the expected type.
* Mark list members with errors as needed
*
* Must call ncx_init_list first!!!
*
* INPUTS:
*     btyp == expected basetype for the list member type
*     strval == cleaned XML string to parse into ncx_str_t or 
*             ncx_num_t values
*     list == ncx_list_t in progress that will get the ncx_lmem_t
*             structs added to it, as they are parsed
*
* OUTPUTS:
*     list->memQ has 1 or more ncx_lmem_t structs appended to it
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    ncx_set_list (ncx_btype_t btyp,
                  const xmlChar *strval,
                  ncx_list_t  *list)
{
    const xmlChar     *str1 = strval;
    const xmlChar     *str2 = NULL;
    ncx_lmem_t        *lmem;
    uint32             len;
    boolean            done = FALSE;
    boolean            checkexists;

    if (!strval || !list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!*strval) {
        return NO_ERR;
    }

    /* probably already set but make sure */
    list->btyp = btyp;

    checkexists = !dlq_empty(&list->memQ);
    while (!done) {
        /* skip any leading whitespace */
        while( xml_isspace( *str1 ) ) {
            ++str1;
        }
        if (!*str1) {
            done = TRUE;
            continue;
        }

        /* if str1 starts with a double quote, parse the string as 
         * a whitespace-allowed string */
        if (NCX_STR_START == *str1) {
            ++str1;
            str2 = str1;
            while (*str2 && (NCX_STR_END != *str2 )) {
                str2++;
            }
            len = (uint32)( str2 - str1 );
            if (*str2) {
                str2++;
            } else {
                log_info("\nncx_set_list: missing EOS marker\n  (%s)", str1);
            }
        } else {
            /* consume string until a WS, str-start, or EOS seen */
            str2 = str1+1;
            while (*str2 && !xml_isspace(*str2) && 
                   (NCX_STR_START != *str2 )) {
                str2++;
            }
            len = (uint32)(str2-str1);
        }

        /* set up a new list string struct */
        lmem = ncx_new_lmem();
        if (!lmem) {
            return ERR_INTERNAL_MEM;
        }

        /* copy the string just parsed for now just separate into strings and 
         * do not validate or parse into enums or numbers */
        lmem->val.str = xml_strndup(str1, len);
        if (!lmem->val.str) {
            ncx_free_lmem(lmem, NCX_BT_STRING);
            return ERR_INTERNAL_MEM;
        }

        if (checkexists &&
            ncx_string_in_list(lmem->val.str, list)) {
            /* The entry is already present, discard it */
            ncx_free_lmem(lmem, NCX_BT_STRING);
        } else {            
            /* save the list member in the Q */
            dlq_enque(lmem, &list->memQ);
        }

        /* reset the string pointer and loop */
        str1 = str2;
    }

    return NO_ERR;

}  /* ncx_set_list */


/********************************************************************
* FUNCTION ncx_finish_list
* 
* 2nd pass of parsing a ncx_list_t
* Finish converting the list members to the proper format
*
* INPUTS:
*    typdef == typ_def_t for the designated list member type
*    list == list struct with ncx_lmem_t structs to check
*
* OUTPUTS:
*   If return other than NO_ERR:
*     each list->lmem.flags field may contain bits set
*     for errors:
*        NCX_FL_RANGE_ERR: size out of range
*        NCX_FL_VALUE_ERR  value not permitted by value set, 
*                          or pattern
* RETURNS:
*    status
*********************************************************************/
status_t
    ncx_finish_list (typ_def_t *typdef,
                     ncx_list_t *list)
{
    ncx_lmem_t      *lmem;
    xmlChar         *str;
    ncx_btype_t      btyp;
    status_t         res, retres;
    dlq_hdr_t        tempQ;

#ifdef DEBUG
    if (!typdef || !list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    btyp = typ_get_basetype(typdef);
    res = NO_ERR;
    retres = NO_ERR;

    /* check if any work to do */
    switch (btyp) {
    case NCX_BT_STRING:
    case NCX_BT_BOOLEAN:
        return NO_ERR;
    default:
        ;
    }

    /* go through all the list members and check them */
    for (lmem = (ncx_lmem_t *)dlq_firstEntry(&list->memQ);
         lmem != NULL;
         lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {

        str = lmem->val.str;
        if (btyp == NCX_BT_ENUM) {
            res = val_enum_ok(typdef, 
                              str,
                              &lmem->val.enu.val,
                              &lmem->val.enu.name);
        } else if (btyp == NCX_BT_BITS) {
            /* transfer the malloced string from 
             * val.str to val.bit.dname
             */
            lmem->val.bit.dname = str;
            lmem->val.bit.name = lmem->val.bit.dname;
            res = val_bit_ok(typdef, str, 
                             &lmem->val.bit.pos);
        } else if (typ_is_number(btyp)){
            res = ncx_decode_num(str, btyp, &lmem->val.num);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (btyp != NCX_BT_BITS) {
            m__free(str);
        }

        if (res != NO_ERR) {
            /* the string did not match this pattern */
            CHK_EXIT(res, retres);
            lmem->flags |= NCX_FL_VALUE_ERR;
        } 
    }

    if (retres == NO_ERR && btyp == NCX_BT_BITS) {
        /* put bits in their canonical order */
        dlq_createSQue(&tempQ);
        dlq_block_enque(&list->memQ, &tempQ);

        while (!dlq_empty(&tempQ)) {
            lmem = (ncx_lmem_t *)dlq_deque(&tempQ);
            ncx_insert_lmem(list, lmem, NCX_MERGE_SORT);
        }
    }
        
    return retres;

} /* ncx_finish_list */


/********************************************************************
* FUNCTION ncx_new_lmem
*
* Malloc and fill in a new ncx_lmem_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to malloced and initialized ncx_lmem_t struct
*   NULL if malloc error
*********************************************************************/
ncx_lmem_t *
    ncx_new_lmem (void)
{
    ncx_lmem_t  *lmem;

    lmem = m__getObj(ncx_lmem_t);
    if (!lmem) {
        return NULL;
    }
    memset(lmem, 0x0, sizeof(ncx_lmem_t));
    return lmem;

}  /* ncx_new_lmem */


/********************************************************************
* FUNCTION ncx_clean_lmem
* 
* Scrub the memory of a ncx_lmem_t but do not delete it
*
* INPUTS:
*    lmem == ncx_lmem_t struct to clean
*    btyp == base type of list member (lmem)
*********************************************************************/
void
    ncx_clean_lmem (ncx_lmem_t *lmem,
                    ncx_btype_t btyp)
{

#ifdef DEBUG
    if (!lmem) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (typ_is_string(btyp)) {
        ncx_clean_str(&lmem->val.str);
    } else if (typ_is_number(btyp)) {
        ncx_clean_num(btyp, &lmem->val.num);
    } else {
        switch (btyp) {
        case NCX_BT_ENUM:
            ncx_clean_enum(&lmem->val.enu);
            break;
        case NCX_BT_BITS:
            ncx_clean_bit(&lmem->val.bit);
            break;
        case NCX_BT_BOOLEAN:
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

} /* ncx_clean_lmem */


/********************************************************************
* FUNCTION ncx_free_lmem
*
* Free all the memory in a  ncx_lmem_t struct
*
* INPUTS:
*   lmem == struct to clean and free
*   btyp == base type of the list member
*
*********************************************************************/
void
    ncx_free_lmem (ncx_lmem_t *lmem,
                   ncx_btype_t btyp)
{
#ifdef DEBUG
    if (!lmem) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    ncx_clean_lmem(lmem, btyp);
    m__free(lmem);

}  /* ncx_free_lmem */


/********************************************************************
* FUNCTION ncx_find_lmem
*
* Find a the first matching list member with the specified value
*
* INPUTS:
*   list == list to check
*   memval == value to find, based on list->btyp
*
* RETURNS:
*  pointer to the first instance of this value, or NULL if none
*********************************************************************/
ncx_lmem_t *
    ncx_find_lmem (ncx_list_t *list,
                   const ncx_lmem_t *memval)
{
    ncx_lmem_t        *lmem;
    const ncx_num_t   *num;
    const ncx_str_t   *str;
    const ncx_enum_t  *enu;
    const ncx_bit_t   *bit;
    int32              cmpval;
    boolean            boo;

#ifdef DEBUG
    if (!list || !memval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    num = NULL;
    str = NULL;
    enu = NULL;
    bit = NULL;
    boo = FALSE;

    if (typ_is_number(list->btyp)) {
        num = &memval->val.num;
    } else if (typ_is_string(list->btyp)) {
        str = &memval->val.str;
    } else if (list->btyp == NCX_BT_ENUM) {
        enu = &memval->val.enu;
    } else if (list->btyp == NCX_BT_BITS) {
        bit = &memval->val.bit;
    } else if (list->btyp == NCX_BT_BOOLEAN) {
        boo = memval->val.boo;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    for (lmem = (ncx_lmem_t *)dlq_firstEntry(&list->memQ);
         lmem != NULL;
         lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {
        if (num) {
            cmpval = ncx_compare_nums(&lmem->val.num, num, list->btyp);
        } else if (str) {
            cmpval = ncx_compare_strs(&lmem->val.str, str, list->btyp);
        } else if (enu) {
            cmpval = ncx_compare_enums(&lmem->val.enu, enu);
        } else if (bit) {
                cmpval = ncx_compare_bits(&lmem->val.bit, bit);
        } else {
            cmpval = (lmem->val.boo && boo) ? 0 : 1;
        }

        if (!cmpval) {
            return lmem;
        }
    }
    return NULL;

}  /* ncx_find_lmem */


/********************************************************************
* FUNCTION ncx_insert_lmem
*
* Insert a list entry into the specified list
*
* INPUTS:
*   list == list to insert into
*   memval == value to insert, based on list->btyp
*   mergetyp == requested merge type for the insertion
*
* RETURNS:
*   none
*********************************************************************/
void
    ncx_insert_lmem (ncx_list_t *list,
                     ncx_lmem_t *memval,
                     ncx_merge_t mergetyp)
{
    ncx_lmem_t        *lmem;
    const ncx_num_t   *num;
    const ncx_str_t   *str;
    const ncx_enum_t  *enu;
    const ncx_bit_t   *bit;
    int32              cmpval;
    boolean            boo;

#ifdef DEBUG
    if (!list || !memval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (mergetyp) {
    case NCX_MERGE_FIRST:
        lmem = (ncx_lmem_t *)dlq_firstEntry(&list->memQ);
        if (lmem) {
            dlq_insertAhead(memval, lmem);
        } else {
            dlq_enque(memval, &list->memQ);
        }
        break;
    case NCX_MERGE_LAST:
        dlq_enque(memval, &list->memQ);
        break;
    case NCX_MERGE_SORT:
        num = NULL;
        str = NULL;
        enu = NULL;
        bit = NULL;
        boo = FALSE;

        if (typ_is_number(list->btyp)) {
            num = &memval->val.num;
        } else if (typ_is_string(list->btyp)) {
            str = &memval->val.str;
        } else if (list->btyp == NCX_BT_ENUM) {
            enu = &memval->val.enu;
        } else if (list->btyp == NCX_BT_BITS) {
            bit = &memval->val.bit;
        } else if (list->btyp == NCX_BT_BOOLEAN) {
            boo = memval->val.boo;
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }

        for (lmem = (ncx_lmem_t *)dlq_firstEntry(&list->memQ);
             lmem != NULL;
             lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {
            if (num) {
                cmpval = ncx_compare_nums(&lmem->val.num, num, list->btyp);
            } else if (str) {
                cmpval = ncx_compare_strs(&lmem->val.str, str, list->btyp);
            } else if (enu) {
                cmpval = ncx_compare_enums(&lmem->val.enu, enu);
            } else if (bit) {
                cmpval = ncx_compare_bits(&lmem->val.bit, bit);
            } else {
                if (lmem->val.boo) {
                    cmpval = (boo) ? 0 : 1;
                } else {
                    cmpval = (boo) ? -1 : 0;
                }
            }

            if (cmpval >= 0) {
                dlq_insertAhead(memval, lmem);
                return;
            }
        }

        /* make new last entry */
        dlq_enque(memval, &list->memQ);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }   

}  /* ncx_insert_lmem */


/********************************************************************
* FUNCTION ncx_first_lmem
*
* Return the first list member
*
* INPUTS:
*   list == list to check
*
* RETURNS:
*  pointer to the first list member or NULL if none
*********************************************************************/
ncx_lmem_t *
    ncx_first_lmem (ncx_list_t *list)
{
#ifdef DEBUG
    if (!list) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    return (ncx_lmem_t *)dlq_firstEntry(&list->memQ);

}  /* ncx_first_lmem */


/* END file ncx_list.c */

