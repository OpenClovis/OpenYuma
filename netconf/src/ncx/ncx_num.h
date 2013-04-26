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
#ifndef _H_ncx_num
#define _H_ncx_num

/*  FILE: ncx_num.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Module Library Number Utility Functions

 
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
* FUNCTION ncx_init_num
* 
* Init a ncx_num_t struct
*
* INPUTS:
*     num == number to initialize
*********************************************************************/
extern void
    ncx_init_num (ncx_num_t *num);


/********************************************************************
* FUNCTION ncx_clean_num
* 
* Scrub the memory in a ncx_num_t by freeing all
* the sub-fields. DOES NOT free the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    btyp == base type of number
*    num == ncx_num_t data structure to clean
*********************************************************************/
extern void 
    ncx_clean_num (ncx_btype_t btyp,
		   ncx_num_t *num);


/********************************************************************
* FUNCTION ncx_compare_nums
* 
* Compare 2 ncx_num_t union contents
*
* INPUTS:
*     num1 == first number
*     num2 == second number
*     btyp == expected data type (NCX_BT_INT, UINT, REAL)
* RETURNS:
*     -1 if num1 is < num2
*      0 if num1 == num2
*      1 if num1 is > num2
*********************************************************************/
extern int32
    ncx_compare_nums (const ncx_num_t *num1,
		      const ncx_num_t *num2,
		      ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_set_num_min
* 
* Set a number to the minimum value for its type
*
* INPUTS:
*     num == number to set
*     btyp == expected data type
*
*********************************************************************/
extern void
    ncx_set_num_min (ncx_num_t *num,
		     ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_set_num_max
* 
* Set a number to the maximum value for its type
*
* INPUTS:
*     num == number to set
*     btyp == expected data type
*
*********************************************************************/
extern void
    ncx_set_num_max (ncx_num_t *num,
		     ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_set_num_one
* 
* Set a number to one
*
* INPUTS:
*     num == number to set
*     btyp == expected data type
*
*********************************************************************/
extern void
    ncx_set_num_one (ncx_num_t *num,
		     ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_set_num_zero
* 
* Set a number to zero
*
* INPUTS:
*     num == number to set
*     btyp == expected data type
*
*********************************************************************/
extern void
    ncx_set_num_zero (ncx_num_t *num,
		      ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_set_num_nan
* 
* Set a FP number to the Not a Number value
*
* INPUTS:
*     num == number to set
*     btyp == expected data type
*
*********************************************************************/
extern void
    ncx_set_num_nan (ncx_num_t *num,
		     ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_num_is_nan
* 
* Check if a FP number is set to the Not a Number value
*
* INPUTS:
*     num == number to check
*     btyp == expected data type
*
* RETURNS:
*    TRUE if number is not-a-number (NaN)
*    FALSE otherwise
*********************************************************************/
extern boolean
    ncx_num_is_nan (ncx_num_t *num,
		    ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_num_zero
* 
* Compare a ncx_num_t to zero
*
* INPUTS:
*     num == number to check
*     btyp == expected data type (e.g., NCX_BT_INT32, NCX_BT_UINT64)
*
* RETURNS:
*     TRUE if value is equal to zero
*     FALSE if value is not equal to zero
*********************************************************************/
extern boolean
    ncx_num_zero (const ncx_num_t *num,
		  ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_convert_num
* 
* Convert a number string to a numeric type
*
* INPUTS:
*     numstr == number string
*     numfmt == NCX_NF_OCTAL, NCX_NF_DEC, NCX_NF_HEX, or NCX_NF_REAL
*     btyp == desired number type 
*             (e.g., NCX_BT_INT32, NCX_BT_UINT32, NCX_BT_FLOAT64)
*     val == pointer to ncx_num_t to hold result
*
* OUTPUTS:
*     *val == converted number value
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_convert_num (const xmlChar *numstr,
		     ncx_numfmt_t numfmt,
		     ncx_btype_t  btyp,
		     ncx_num_t    *val);


/********************************************************************
* FUNCTION ncx_convert_dec64
* 
* Convert a number string to a decimal64 number
*
* INPUTS:
*     numstr == number string
*     numfmt == number format used
*     digits == number of fixed-point digits expected
*     val == pointer to ncx_num_t to hold result
*
* OUTPUTS:
*     *val == converted number value
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_convert_dec64 (const xmlChar *numstr,
		       ncx_numfmt_t numfmt,
		       uint8 digits,
		       ncx_num_t *val);


/********************************************************************
* FUNCTION ncx_decode_num
* 
* Handle some sort of number string
*
* INPUTS:
*   numstr == number string
*    btyp == desired number type
*   retnum == pointer to initialized ncx_num_t to hold result
*
* OUTPUTS:
*   *retnum == converted number
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_decode_num (const xmlChar *numstr,
		    ncx_btype_t  btyp,
		    ncx_num_t  *retnum);


/********************************************************************
* FUNCTION ncx_decode_dec64
* 
* Handle some sort of decimal64 number string (NCX_BT_DECIMAL64)
*
* INPUTS:
*   numstr == number string
*    digits == number of expected digits for this decimal64
*   retnum == pointer to initialized ncx_num_t to hold result
*
* OUTPUTS:
*   *retnum == converted number
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_decode_dec64 (const xmlChar *numstr,
		      uint8 digits,
		      ncx_num_t  *retnum);


/********************************************************************
* FUNCTION ncx_copy_num
* 
* Copy the contents of num1 to num2
*
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_DECIMAL64
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num1 == first number
*     num2 == second number
*     btyp == expected data type (NCX_BT_INT, UINT, REAL)
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_copy_num (const ncx_num_t *num1,
		  ncx_num_t *num2,
		  ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_cast_num
* 
* Cast a number as another number type
*
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num1 == source number
*     btyp1 == expected data type of num1
*     num2 == target number
*     btyp2 == desired data type of num2
*
* OUTPUTS:
*   *num2 set to cast value of num1
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_cast_num (const ncx_num_t *num1,
		  ncx_btype_t  btyp1,
		  ncx_num_t *num2,
		  ncx_btype_t  btyp2);


/********************************************************************
* FUNCTION ncx_num_floor
* 
* Get the floor value of a number
*
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_DECIMAL64
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num1 == source number
*     num2 == address of target number
*     btyp == expected data type of numbers
*
* OUTPUTS:
*   *num2 set to floor of num1
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_num_floor (const ncx_num_t *num1,
		   ncx_num_t *num2,
		   ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_num_ceiling
* 
* Get the ceiling value of a number
*
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_DECIMAL64
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num1 == source number
*     num2 == target number
*     btyp == expected data type of numbers
*
* OUTPUTS:
*   *num2 set to ceiling of num1
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_num_ceiling (const ncx_num_t *num1,
		     ncx_num_t *num2,
		     ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_round_num
* 
* Get the rounded value of a number
*
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_DECIMAL64
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num1 == source number
*     num2 == target number
*     btyp == expected data type of numbers
*
* OUTPUTS:
*   *num2 set to round of num1
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_round_num (const ncx_num_t *num1,
		   ncx_num_t *num2,
		   ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_num_is_integral
* 
* Check if the number is integral or if it has a fractional part
* Supports all NCX numeric types:
*    NCX_BT_INT*
*    NCX_BT_UINT*
*    NCX_BT_DECIMAL64
*    NCX_BT_FLOAT64
*
* INPUTS:
*     num == number to check
*     btyp == expected data type
*
* RETURNS:
*     TRUE if integral, FALSE if not
*********************************************************************/
extern boolean
    ncx_num_is_integral (const ncx_num_t *num,
			 ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_cvt_to_int64
* 
* Convert a number to an integer64;
* Use rounding for float64
*
* INPUTS:
*     num == number to convert
*     btyp == data type of num
*
* RETURNS:
*     int64 representation
*********************************************************************/
extern int64
    ncx_cvt_to_int64 (const ncx_num_t *num,
		      ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_get_numfmt
* 
* Get the number format of the specified string
* Does not check for valid format
* Just figures out which type it must be if it were valid
*
* INPUTS:
*     numstr == number string
*
* RETURNS:
*    NCX_NF_NONE, NCX_NF_DEC, NCX_NF_HEX, or NCX_NF_REAL
*********************************************************************/
extern ncx_numfmt_t
    ncx_get_numfmt (const xmlChar *numstr);


/********************************************************************
* FUNCTION ncx_printf_num
* 
* Printf a ncx_num_t contents
*
* INPUTS:
*    num == number to printf
*    btyp == number base type
*
*********************************************************************/
extern void
    ncx_printf_num (const ncx_num_t *num,
		    ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_alt_printf_num
* 
* Printf a ncx_num_t contents to the alternate log file
*
* INPUTS:
*    num == number to printf
*    btyp == number base type
*
*********************************************************************/
extern void
    ncx_alt_printf_num (const ncx_num_t *num,
			ncx_btype_t  btyp);


/********************************************************************
* FUNCTION ncx_sprintf_num
* 
* Sprintf a ncx_num_t contents
*
* INPUTS:
*    buff == buffer to write; NULL means just get length
*    num == number to printf
*    btyp == number base type
*    len == address of return length
*
* OUTPUTS::
*    *len == number of bytes written (or would have been) to buff
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    ncx_sprintf_num (xmlChar *buff,
		     const ncx_num_t *num,
		     ncx_btype_t  btyp,
		     uint32  *len);


/********************************************************************
* FUNCTION ncx_is_min
* 
* Return TRUE if the specified number is the min value
* for its type
*
* INPUTS:
*     num == number to check
*     btyp == data type of num
* RETURNS:
*     TRUE if this is the minimum value
*     FALSE otherwise
*********************************************************************/
extern boolean
    ncx_is_min (const ncx_num_t *num,
		ncx_btype_t btyp);


/********************************************************************
* FUNCTION ncx_is_max
* 
* Return TRUE if the specified number is the max value
* for its type
*
* INPUTS:
*     num == number to check
*     btyp == data type of num
* RETURNS:
*     TRUE if this is the maximum value
*     FALSE otherwise
*********************************************************************/
extern boolean
    ncx_is_max (const ncx_num_t *num,
		ncx_btype_t btyp);


/********************************************************************
* FUNCTION ncx_convert_tkcnum
* 
* Convert the current token in a token chain to
* a ncx_num_t struct
*
* INPUTS:
*     tkc == token chain; current token will be converted
*            tkc->typ == TK_TT_DNUM, TK_TT_HNUM, TK_TT_RNUM
*     btyp == desired number type 
*             (e.g., NCX_BT_INT32, NCX_BT_UINT64, NCX_BT_FLOAT64)
* OUTPUTS:
*     *val == converted number value (0..2G-1), if NO_ERR
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_convert_tkcnum (tk_chain_t *tkc,
			ncx_btype_t  btyp,
			ncx_num_t *val);


/********************************************************************
* FUNCTION ncx_convert_tkc_dec64
* 
* Convert the current token in a token chain to
* a ncx_num_t struct, expecting NCX_BT_DECIMAL64
*
* INPUTS:
*     tkc == token chain; current token will be converted
*            tkc->typ == TK_TT_DNUM, TK_TT_HNUM, TK_TT_RNUM
*     digits == number of expected digits
*
* OUTPUTS:
*     *val == converted number value, if NO_ERR
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    ncx_convert_tkc_dec64 (tk_chain_t *tkc,
			   uint8 digits,
			   ncx_num_t *val);


/********************************************************************
* FUNCTION ncx_get_dec64_base
* 
* Get the base part of a decimal64 number
* 
* INPUT:
*   num == number to check (expected to be NCX_BT_DECIMAL64)
*
* RETURNS:
*   base part of the number
*********************************************************************/
extern int64
    ncx_get_dec64_base (const ncx_num_t *num);


/********************************************************************
* FUNCTION ncx_get_dec64_fraction
* 
* Get the fraction part of a decimal64 number
* 
* INPUT:
*   num == number to check (expected to be NCX_BT_DECIMAL64)
*
* RETURNS:
*   fraction part of the number
*********************************************************************/
extern int64
    ncx_get_dec64_fraction (const ncx_num_t *num);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx_num */

