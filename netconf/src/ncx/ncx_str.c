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
/*  FILE: ncx_num.c

                
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

#ifndef _H_ncx_str
#include "ncx_str.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_status
#include  "status.h"
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
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
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
int32
    ncx_compare_strs (const ncx_str_t *str1,
                      const ncx_str_t *str2,
                      ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!str1 || !str2) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (!typ_is_string(btyp)) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }   

    return xml_strcmp(*str1, *str2);

    /*NOTREACHED*/
}  /* ncx_compare_strs */


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
status_t
    ncx_copy_str (const ncx_str_t *str1,
                  ncx_str_t *str2,
                  ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!str1 || !str2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    if (!(typ_is_string(btyp) || btyp==NCX_BT_BITS)) {
        return ERR_NCX_INVALID_VALUE;
    }   

    if (*str1) {
        *str2 = xml_strdup(*str1);
        if (!*str2) {
            return ERR_INTERNAL_MEM;
        }
    } else {
        *str2 = NULL;
    }
    return NO_ERR;

}  /* ncx_copy_str */


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
void 
    ncx_clean_str (ncx_str_t *str)
{
#ifdef DEBUG
    if (!str) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;  
    }
#endif

    /* clean the num->union, depending on base type */
    if (*str) {
        m__free(*str);
        *str = NULL;
    }

}  /* ncx_clean_str */


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
uint32
    ncx_copy_c_safe_str (xmlChar *buffer,
                         const xmlChar *strval)
{
    const xmlChar *s;
    uint32         count;

    count = 0;
    s = strval;

    while (*s) {
        if (*s == '.' || *s == '-' || *s == NCXMOD_PSCHAR) {
            *buffer++ = '_';
        } else {
            *buffer++ = *s;
        }
        s++;
        count++;
    }
    *buffer = 0;

    return count;

}  /* ncx_copy_c_safe_str */


/* END file ncx.c */
