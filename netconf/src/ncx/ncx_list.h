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
#ifndef _H_ncx_list
#define _H_ncx_list

/*  FILE: ncx_list.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Module Library List Utility Functions

 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
17-feb-10    abb      Begun; split out from ncx.c
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
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
extern ncx_list_t *
    ncx_new_list (ncx_btype_t btyp);


/********************************************************************
* FUNCTION ncx_init_list
* 
* Initialize an allocated ncx_list_t
*
* INPUTS:
*    list == pointer to ncx_list_t memory
*    btyp == base type for the list
*********************************************************************/
extern void
    ncx_init_list (ncx_list_t *list,
		   ncx_btype_t btyp);


/********************************************************************
* FUNCTION ncx_clean_list
* 
* Scrub the memory of a ncx_list_t but do not delete it
*
* INPUTS:
*    list == ncx_list_t struct to clean
*********************************************************************/
extern void 
    ncx_clean_list (ncx_list_t *list);


/********************************************************************
* FUNCTION ncx_free_list
* 
* Clean and free an allocated ncx_list_t
*
* INPUTS:
*    list == pointer to ncx_list_t memory
*********************************************************************/
extern void
    ncx_free_list (ncx_list_t *list);


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
extern uint32
    ncx_list_cnt (const ncx_list_t *list);


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
extern boolean
    ncx_list_empty (const ncx_list_t *list);


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
extern boolean
    ncx_string_in_list (const xmlChar *str,
			const ncx_list_t *list);

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
extern int32
    ncx_compare_lists (const ncx_list_t *list1,
		       const ncx_list_t *list2);


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
extern status_t
    ncx_copy_list (const ncx_list_t *list1,
		   ncx_list_t *list2);


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
extern void
    ncx_merge_list (ncx_list_t *src,
		    ncx_list_t *dest,
		    ncx_merge_t mergetyp,
		    boolean allow_dups);


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
extern status_t
    ncx_set_strlist (const xmlChar *liststr,
		     ncx_list_t *list);

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
extern status_t 
    ncx_set_list (ncx_btype_t btyp,
		  const xmlChar *strval,
		  ncx_list_t  *list);


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
extern status_t
    ncx_finish_list (typ_def_t *typdef,
		     ncx_list_t *list);


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
extern ncx_lmem_t *
    ncx_new_lmem (void);


/********************************************************************
* FUNCTION ncx_clean_lmem
* 
* Scrub the memory of a ncx_lmem_t but do not delete it
*
* INPUTS:
*    lmem == ncx_lmem_t struct to clean
*    btyp == base type of list member (lmem)
*********************************************************************/
extern void
    ncx_clean_lmem (ncx_lmem_t *lmem,
		    ncx_btype_t btyp);


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
extern void
    ncx_free_lmem (ncx_lmem_t *lmem,
		   ncx_btype_t btyp);


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
extern ncx_lmem_t *
    ncx_find_lmem (ncx_list_t *list,
		   const ncx_lmem_t *memval);


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
extern void
    ncx_insert_lmem (ncx_list_t *list,
		     ncx_lmem_t *memval,
		     ncx_merge_t mergetyp);


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
extern ncx_lmem_t *
    ncx_first_lmem (ncx_list_t *list);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx_list */

