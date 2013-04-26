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
#ifndef _H_ncx_str
#define _H_ncx_str

/*  FILE: ncx_str.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Module Library String Utility Functions

 
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

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
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
* FUNCTION ncx_compare_strs
* 
* Compare 2 ncx_str_t union contents
*
* INPUTS:
*     str1 == first string
*     str2 == second string
*     btyp == expected data type 
*             (NCX_BT_STRING, NCX_BT_INSTANCE_ID)
* RETURNS:
*     -1 if str1 is < str2
*      0 if str1 == str2   (also for error, after SET_ERROR called)
*      1 if str1 is > str2
*********************************************************************/
extern int32
    ncx_compare_strs (const ncx_str_t *str1,
		      const ncx_str_t *str2,
		      ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_copy_str
* 
* Copy the contents of str1 to str2
* Supports base types:
*     NCX_BT_STRING
*     NCX_BT_INSTANCE_ID
*     NCX_BT_LEAFREF
*
* INPUTS:
*     str1 == first string
*     str2 == second string
*     btyp == expected data type
*
* OUTPUTS:
*    str2 contains copy of str1
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_copy_str (const ncx_str_t *str1,
		  ncx_str_t *str2,
		  ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_clean_str
* 
* Scrub the memory in a ncx_str_t by freeing all
* the sub-fields. DOES NOT free the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    str == ncx_str_t data structure to clean
*********************************************************************/
extern void 
    ncx_clean_str (ncx_str_t *str);


/********************************************************************
* FUNCTION ncx_copy_c_safe_str
* 
* Copy the string to the buffer, changing legal YANG identifier
* chars that cannot be used in C function names to underscore
*
* !!! DOES NOT CHECK BUFFER OVERRUN !!!
* !!! LENGTH OF STRING IS NOT CHANGED WHEN COPIED TO THE BUFFER !!!
*
* INPUTS:
*   buffer == buffer to write into
*   strval == string value to copy
*
* RETURNS
*   number of chars copied
*********************************************************************/
extern uint32
    ncx_copy_c_safe_str (xmlChar *buffer,
                         const xmlChar *strval);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx_str */
