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

#ifdef HAS_FLOAT
#include <math.h>
#include <tgmath.h>
#endif

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

#ifndef _H_ncx_num
#include "ncx_num.h"
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


#ifdef HAS_FLOAT
/********************************************************************
* FUNCTION remove_trailing_zero_count
* 
* Get the number of trailing zeros and maybe the decimal point too
*
* INPUTS:
*   buff == number buffer to check
*
* RETURNS:
*   number of chars to remove from the end of string
*********************************************************************/
static int32
    remove_trailing_zero_count (const xmlChar *buff)
{
    int32  len, newlen;
    const xmlChar  *str;

    len = strlen((const char *)buff);
    if (!len) {
        return 0;
    }

    str = &buff[len-1];

    while (str >= buff && *str == '0') {
        str--;
    }

    if (*str == '.') {
        str--;
    }

    newlen = (int32)(str - buff) + 1;

    return len - newlen;

}  /* remove_trailing_zero_count */
#endif


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION ncx_init_num
* 
* Init a ncx_num_t struct
*
* INPUTS:
*     num == number to initialize
*********************************************************************/
void
    ncx_init_num (ncx_num_t *num)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(num, 0x0, sizeof(ncx_num_t));

}  /* ncx_init_num */


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
void 
    ncx_clean_num (ncx_btype_t btyp,
                   ncx_num_t *num)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;  
    }
#endif

    /* clean the num->union, depending on base type */
    switch (btyp) {
    case NCX_BT_NONE:
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
        memset(num, 0x0, sizeof(ncx_num_t));
        break;
    case NCX_BT_DECIMAL64:
        num->dec.val = 0;
        num->dec.digits = 0;
        num->dec.zeroes = 0;
        break;
    case NCX_BT_FLOAT64:
        num->d = 0;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* ncx_clean_num */


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
int32
    ncx_compare_nums (const ncx_num_t *num1,
                      const ncx_num_t *num2,
                      ncx_btype_t  btyp)
{
    int64    temp1, temp2;
    uint8    diffdigits;

#ifdef DEBUG
    if (!num1 || !num2) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        if (num1->i < num2->i) {
            return -1;
        } else if (num1->i == num2->i) {
            return 0;
        } else {
            return 1;
        }
    case NCX_BT_INT64:
        if (num1->l < num2->l) {
            return -1;
        } else if (num1->l == num2->l) {
            return 0;
        } else {
            return 1;
        }
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        if (num1->u < num2->u) {
            return -1;
        } else if (num1->u == num2->u) {
            return 0;
        } else {
            return 1;
        }
    case NCX_BT_UINT64:
        if (num1->ul < num2->ul) {
            return -1;
        } else if (num1->ul == num2->ul) {
            return 0;
        } else {
            return 1;
        }
    case NCX_BT_DECIMAL64:
        /* check the base parts first */
        temp1 = ncx_get_dec64_base(num1);
        temp2 = ncx_get_dec64_base(num2);

        if (temp1 < temp2) {
            return -1;
        } else if (temp1 == temp2) {
            /* check if any leading zeroes */
            if (temp1 == 0) {
                if (num1->dec.zeroes < num2->dec.zeroes) {
                    return 1;
                } else if (num1->dec.zeroes > num2->dec.zeroes) {
                    return -1;
                }  /* else need to compare fraction parts */
            }

            /* check fraction parts next */
            temp1 = ncx_get_dec64_fraction(num1);
            temp2 = ncx_get_dec64_fraction(num2);

            /* normalize these numbers to compare them */
            if (num1->dec.digits > num2->dec.digits) {
                diffdigits = num1->dec.digits - num2->dec.digits;
                temp2 *= (10 * diffdigits);
            } else if (num1->dec.digits < num2->dec.digits) {
                diffdigits = num2->dec.digits - num1->dec.digits;
                temp1 *= (10 * diffdigits);
            }

            if (temp1 < temp2) {
                return -1;
            } else if (temp1 == temp2) {
                return 0;
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    case NCX_BT_FLOAT64:
        if (num1->d < num2->d) {
            return -1;
        } else if (num1->d == num2->d) {
            return 0;
        } else {
            return 1;
        }
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
    /*NOTREACHED*/
}  /* ncx_compare_nums */


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
void
    ncx_set_num_min (ncx_num_t *num,
                     ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
        num->i = NCX_MIN_INT8;
        break;
    case NCX_BT_INT16:
        num->i = NCX_MIN_INT16;
        break;
    case NCX_BT_INT32:
        num->i = NCX_MIN_INT;
        break;
    case NCX_BT_INT64:
        num->l = NCX_MIN_LONG;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        num->u = NCX_MIN_UINT;
        break;
    case NCX_BT_UINT64:
        num->ul = NCX_MIN_ULONG;
        break;
    case NCX_BT_DECIMAL64:
        num->dec.val = NCX_MIN_LONG;
        num->dec.zeroes = 0;
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        num->d = -INFINITY;
#else
        num->d = NCX_MIN_LONG;
#endif
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* ncx_set_num_min */


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
void
    ncx_set_num_max (ncx_num_t *num,
                     ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
        num->i = NCX_MAX_INT8;
        break;
    case NCX_BT_INT16:
        num->i = NCX_MAX_INT16;
        break;
    case NCX_BT_INT32:
        num->i = NCX_MAX_INT;
        break;
    case NCX_BT_INT64:
        num->l = NCX_MAX_LONG;
        break;
    case NCX_BT_UINT8:
        num->u = NCX_MAX_UINT8;
        break;
    case NCX_BT_UINT16:
        num->u = NCX_MAX_UINT16;
        break;
    case NCX_BT_UINT32:
        num->u = NCX_MAX_UINT;
        break;
    case NCX_BT_UINT64:
        num->ul = NCX_MAX_ULONG;
        break;
    case NCX_BT_DECIMAL64:
        num->dec.val = NCX_MAX_LONG;
        num->dec.zeroes = 0;
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        num->d = INFINITY;
#else
        num->d = NCX_MAX_LONG-1;
#endif
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* ncx_set_num_max */


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
void
    ncx_set_num_one (ncx_num_t *num,
                     ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num->i = 1;
        break;
    case NCX_BT_INT64:
        num->l = 1;
        break;
    case NCX_BT_UINT8:
        num->u = 1;
        break;
    case NCX_BT_UINT64:
        num->ul = 1;
        break;
    case NCX_BT_DECIMAL64:
        num->dec.val = 10 * num->dec.digits;
        num->dec.zeroes = 0;
        break;
    case NCX_BT_FLOAT64:
        num->d = 1;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* ncx_set_num_one */


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
void
    ncx_set_num_zero (ncx_num_t *num,
                      ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num->i = 0;
        break;
    case NCX_BT_INT64:
        num->l = 0;
        break;
    case NCX_BT_UINT8:
        num->u = 0;
        break;
    case NCX_BT_UINT64:
        num->ul = 0;
        break;
    case NCX_BT_DECIMAL64:
        num->dec.val = 0;
        num->dec.zeroes = 1;
        break;
    case NCX_BT_FLOAT64:
        num->d = 0;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* ncx_set_num_zero */


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
void
    ncx_set_num_nan (ncx_num_t *num,
                     ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (btyp == NCX_BT_FLOAT64) {
#ifdef HAS_FLOAT
        num->d = NAN;
#else
        num->d = NCX_MAX_LONG;
#endif
    }

} /* ncx_set_num_nan */


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
boolean
    ncx_num_is_nan (ncx_num_t *num,
                    ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TRUE;
    }
#endif

    if (btyp == NCX_BT_FLOAT64) {
#ifdef HAS_FLOAT
        return (num->d == NAN) ? TRUE : FALSE;
#else
        return (num->d == NCX_MAX_LONG) ? TRUE : FALSE;
#endif
    }
    return FALSE;

} /* ncx_num_is_nan */


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
boolean
    ncx_num_zero (const ncx_num_t *num,
                  ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        return (num->i) ? FALSE : TRUE;
    case NCX_BT_INT64:
        return (num->l) ? FALSE : TRUE;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        return (num->u) ? FALSE : TRUE;
    case NCX_BT_UINT64:
        return (num->ul) ? FALSE : TRUE;
    case NCX_BT_DECIMAL64:
        return (num->dec.val == 0) ? TRUE : FALSE;
    case NCX_BT_FLOAT64:
        return (num->d == 0) ? TRUE : FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/
}  /* ncx_num_zero */


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
status_t
    ncx_convert_num (const xmlChar *numstr,
                     ncx_numfmt_t   numfmt,
                     ncx_btype_t  btyp,
                     ncx_num_t    *val)
{
    char  *err;
    long  l;
    long long  ll;
    unsigned long ul;
    unsigned long long ull;

#ifdef HAS_FLOAT
    double d;
#endif

#ifdef DEBUG
    if (!numstr || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (*numstr == '\0') {
        return ERR_NCX_INVALID_VALUE;
    }

    err = NULL;
    l = 0;
    ll = 0;
    ul = 0;
    ull = 0;

    /* check the number format set to don't know */
    if (numfmt==NCX_NF_NONE) {
        numfmt = ncx_get_numfmt(numstr);
    }


    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        switch (numfmt) {
        case NCX_NF_OCTAL:
            l = strtol((const char *)numstr, &err, 8);
            break;
        case NCX_NF_DEC:
            l = strtol((const char *)numstr, &err, 10);
            break;
        case NCX_NF_HEX:
            l = strtol((const char *)numstr, &err, 16);
            break;
        case NCX_NF_REAL:
            return ERR_NCX_WRONG_NUMTYP;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (err && *err) {
            return ERR_NCX_INVALID_NUM;
        }

        switch (btyp) {
        case NCX_BT_INT8:
            if (l < NCX_MIN_INT8 || l > NCX_MAX_INT8) {
                return ERR_NCX_NOT_IN_RANGE;
            }
            break;
        case NCX_BT_INT16:
            if (l < NCX_MIN_INT16 || l > NCX_MAX_INT16) {
                return ERR_NCX_NOT_IN_RANGE;
            }
            break;
        default:
            ;
        }
        val->i = (int32)l;
        break;
    case NCX_BT_INT64:
        switch (numfmt) {
        case NCX_NF_OCTAL:
            ll = strtoll((const char *)numstr, &err, 8);
            break;
        case NCX_NF_DEC:
            ll = strtoll((const char *)numstr, &err, 10);
            break;
        case NCX_NF_HEX:
            ll = strtoll((const char *)numstr, &err, 16);
            break;
        case NCX_NF_REAL:
            return ERR_NCX_WRONG_NUMTYP;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (err && *err) {
            return ERR_NCX_INVALID_NUM;
        }
        val->l = (int64)ll;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        switch (numfmt) {
        case NCX_NF_OCTAL:
            ul = strtoul((const char *)numstr, &err, 8);
            break;
        case NCX_NF_DEC:
            ul = strtoul((const char *)numstr, &err, 10);
            break;
        case NCX_NF_HEX:
            ul = strtoul((const char *)numstr, &err, 16);
            break;
        case NCX_NF_REAL:
            return ERR_NCX_WRONG_NUMTYP;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (err && *err) {
            return ERR_NCX_INVALID_NUM;
        }

        switch (btyp) {
        case NCX_BT_UINT8:
            if (ul > NCX_MAX_UINT8) {
                return ERR_NCX_NOT_IN_RANGE;
            }
            break;
        case NCX_BT_UINT16:
            if (ul > NCX_MAX_UINT16) {
                return ERR_NCX_NOT_IN_RANGE;
            }
            break;
        default:
            ;
        }

        if (*numstr == '-') {
            return ERR_NCX_NOT_IN_RANGE;
        }

        val->u = (uint32)ul;
        break;
    case NCX_BT_UINT64:
        switch (numfmt) {
        case NCX_NF_OCTAL:
            ull = strtoull((const char *)numstr, &err, 8);
            break;
        case NCX_NF_DEC:
            ull = strtoull((const char *)numstr, &err, 10);
            break;
        case NCX_NF_HEX:
            ull = strtoull((const char *)numstr, &err, 16);
            break;
        case NCX_NF_REAL:
            return ERR_NCX_WRONG_TKTYPE;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (err && *err) {
            return ERR_NCX_INVALID_NUM;
        }

        if (*numstr == '-') {
            return ERR_NCX_NOT_IN_RANGE;
        }

        val->ul = (uint64)ull;
        break;
    case NCX_BT_DECIMAL64:
        return SET_ERROR(ERR_INTERNAL_VAL);
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        switch (numfmt) {
        case NCX_NF_OCTAL:
        case NCX_NF_DEC:
        case NCX_NF_REAL:
            errno = 0;
            d = strtod((const char *)numstr, &err);
            if (errno) {
                return ERR_NCX_INVALID_NUM;
            }
            val->d = d;
            break;
        case NCX_NF_HEX:
            return ERR_NCX_WRONG_NUMTYP;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
#else
        switch (numfmt) {
        case NCX_NF_OCTAL:
            ll = strtoll((const char *)numstr, &err, 8);
            if (err && *err) {
                return ERR_NCX_INVALID_NUM;
            }
            val->d = (int64)ll;
            break;
        case NCX_NF_DEC:
            ll = strtoll((const char *)numstr, &err, 10);
            if (err && *err) {
                return ERR_NCX_INVALID_NUM;
            }
            val->d = (int64)ll;
            break;
        case NCX_NF_HEX:
            ll = strtoll((const char *)numstr, &err, 16);
            if (err && *err) {
                return ERR_NCX_INVALID_HEXNUM;
            }
            val->d = (int64)ll;
            break;
        case NCX_NF_REAL:
            return ERR_NCX_WRONG_NUMTYP;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
#endif
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    return NO_ERR;

}  /* ncx_convert_num */


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
status_t
    ncx_convert_dec64 (const xmlChar *numstr,
                       ncx_numfmt_t numfmt,
                       uint8 digits,
                       ncx_num_t *val)
{
    const xmlChar  *point, *str;
    char           *err;
    int64           basenum, fracnum, testnum;
    uint32          numdigits;
    boolean         isneg;
    uint8           i, lzeroes;
    xmlChar         numbuff[NCX_MAX_NUMLEN];

#ifdef DEBUG
    if (!numstr || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (*numstr == '\0') {
        val->dec.val = 0;
        val->dec.digits = digits;
        return NO_ERR;
    }

    err = NULL;
    point = NULL;
    basenum = 0;
    fracnum = 0;
    isneg = FALSE;
    lzeroes = 0;

    /* check the number format set to don't know */
    if (numfmt==NCX_NF_NONE) {
        numfmt = ncx_get_numfmt(numstr);
    }

    /* check the number string for plus or minus sign */
    str = numstr;
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        str++;
        isneg = TRUE;
    }
    while (isdigit((int)*str)) {
        str++;
    }

    /* check if stopped on a decimal point */
    if (*str == '.') {
        /* get just the base part now */
        point = str;
        xml_strncpy(numbuff, 
                    numstr, 
                    (uint32)(point - numstr));
        basenum = strtoll((const char *)numbuff, &err, 10);

        if (basenum == 0) {
            /* need to count all the zeroes in the number
             * since they will get lost in the conversion algorithm
             * 0.00034 --> 34    0.0034 --> 34
             */
            lzeroes = 1;   /* count the leading 0.xxx */
            while (point[lzeroes] == '0') {
                lzeroes++;
            }
        }
    } else {
        /* assume the entire string is just a base part
         * the token parser broke up the string
         * already so a string concat '123foo' should
         * not happen here
         */
        switch (numfmt) {
        case NCX_NF_OCTAL:
            basenum = strtoll((const char *)numstr, &err, 8);
            break;
        case NCX_NF_DEC:
        case NCX_NF_REAL:
            basenum = strtoll((const char *)numstr, &err, 10);
            break;
        case NCX_NF_HEX:
            basenum = strtoll((const char *)numstr, &err, 16);
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        /* check if strtoll accepted the number string */
        if (err && *err) {
            return ERR_NCX_INVALID_NUM;
        }
    }

    /* check that the number is actually in range */
    if (isneg) {
        testnum = NCX_MIN_LONG;
    } else {
        testnum = NCX_MAX_LONG;
    }

    /* adjust the test number to the maximum for
     * the specified number of fraction digits
     */
    for (i = 0; i < digits; i++) {
        testnum /= 10;
    }

    /* check if the base number is OK wrt/ testnum */
    if (isneg) {
        if (basenum < testnum) {
            return ERR_NCX_DEC64_BASEOVFL;
        }
    } else {
        if (basenum > testnum) {
            return ERR_NCX_DEC64_BASEOVFL;
        }
    }

    /* check if there is a fraction part entered */
    if (point) {
        fracnum = 0;
        str = point + 1;
        while (isdigit((int)*str)) {
            str++;
        }
        numdigits = (uint32)(str - point - 1);

        /* check if fraction part too big */
        if (numdigits > (uint32)digits) {
            return ERR_NCX_DEC64_FRACOVFL;
        }

        if (numdigits) {
            err = NULL;
            xml_strncpy(numbuff, point+1, numdigits);
            fracnum = strtoll((const char *)numbuff, &err, 10);

            /* check if strtoll accepted the number string */
            if (err && *err) {
                return ERR_NCX_INVALID_NUM;
            }

            /* adjust the fraction part will trailing zeros
             * if the user omitted them
             */
            for (i = numdigits; i < digits; i++) {
                fracnum *= 10;
            }

            if (isneg) {
                fracnum *= -1;
            }
        }
    }

    /* encode the base part shifted left 10 * fraction-digits */
    if (basenum) {
        for (i= 0; i < digits; i++) {
            basenum *= 10;
        }
    }

    /* save the number with the fraction-digits value added in */
    val->dec.val = basenum + fracnum;
    val->dec.digits = digits;
    val->dec.zeroes = lzeroes;

    return NO_ERR;

}  /* ncx_convert_dec64 */


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
status_t 
    ncx_decode_num (const xmlChar *numstr,
                    ncx_btype_t  btyp,
                    ncx_num_t  *retnum)
{
    const xmlChar *str;

#ifdef DEBUG
    if (!numstr || !retnum) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* check if this is a hex number */
    if (*numstr == '0' && NCX_IS_HEX_CH(*(numstr+1))) {
        return ncx_convert_num(numstr+2, 
                               NCX_NF_HEX, 
                               btyp, 
                               retnum);
    }

    /* check if this is a real number */
    str = numstr;
    while (*str && (*str != '.')) {
        str++;
    }
    if (*str) {
        return ncx_convert_num(numstr, NCX_NF_REAL, btyp, retnum);
    }

    /* check octal number */
    if (*numstr == '0' && numstr[1] != '.') {
        return ncx_convert_num(numstr, 
                               NCX_NF_OCTAL, 
                               btyp, 
                               retnum);
    }

    /* else assume this is a decimal number */
    return ncx_convert_num(numstr, 
                           NCX_NF_DEC, 
                           btyp, 
                           retnum);

}  /* ncx_decode_num */


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
status_t 
    ncx_decode_dec64 (const xmlChar *numstr,
                      uint8  digits,
                      ncx_num_t  *retnum)
{
    const xmlChar *str;

#ifdef DEBUG
    if (!numstr || !retnum) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* check if this is a hex number */
    if (*numstr == '0' && NCX_IS_HEX_CH(*(numstr+1))) {
        return ncx_convert_dec64(numstr+2, 
                                 NCX_NF_HEX, 
                                 digits, 
                                 retnum);
    }

    /* check if this is a real number */
    str = numstr;
    while (*str && (*str != '.')) {
        str++;
    }
    if (*str) {
        return ncx_convert_dec64(numstr, 
                                 NCX_NF_REAL, 
                                 digits, 
                                 retnum);
    }

    /* check octal number */
    if (*numstr == '0') {
        return ncx_convert_dec64(numstr, 
                                 NCX_NF_OCTAL, 
                                 digits, 
                                 retnum);
    }

    /* else assume this is a decimal number */
    return ncx_convert_dec64(numstr, 
                             NCX_NF_DEC, 
                             digits, 
                             retnum);

}  /* ncx_decode_dec64 */


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
status_t
    ncx_copy_num (const ncx_num_t *num1,
                  ncx_num_t *num2,
                  ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num1 || !num2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num2->i = num1->i;
        break;
    case NCX_BT_INT64:
        num2->l = num1->l;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        num2->u = num1->u;
        break;
    case NCX_BT_UINT64:
        num2->ul = num1->ul;
        break;
    case NCX_BT_DECIMAL64:
        num2->dec.val = num1->dec.val;
        num2->dec.digits = num1->dec.digits;
        num2->dec.zeroes = num1->dec.zeroes;
        break;
    case NCX_BT_FLOAT64:
        num2->d = num1->d;
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    return NO_ERR;
}  /* ncx_copy_num */


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
status_t
    ncx_cast_num (const ncx_num_t *num1,
                  ncx_btype_t  btyp1,
                  ncx_num_t *num2,
                  ncx_btype_t  btyp2)
{
    int64      testbase /*, testfrac */;
    status_t   res;

#ifdef DEBUG
    if (!num1 || !num2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    switch (btyp1) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
            num2->i = num1->i;
            break;
        case NCX_BT_INT64:
            num2->l = (int64)num1->i;
            break;
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            num2->u = (uint32)num1->i;
            break;
        case NCX_BT_UINT64:
            num2->ul = (uint64)num1->i;
            break;
        case NCX_BT_DECIMAL64:
            if (num2->dec.digits == 0) {
                /* hack: set a default if none set already */
                num2->dec.digits = NCX_DEF_FRACTION_DIGITS;
            }
            /* this may cause an overflow, but too bad !!! */
            num2->dec.val = 
                (int64)(num1->i * (10 * num2->dec.digits));
            break;
        case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
            num2->d = (double)num1->i;
#else
            num2->d = (int64)num1->i;
#endif
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_INT64:
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            res = ERR_NCX_INVALID_VALUE;
            break;
        case NCX_BT_INT64:
            num2->l = num1->l;
            break;
        case NCX_BT_UINT64:
            num2->ul = (uint64)num1->l;
            break;
        case NCX_BT_DECIMAL64:
            if (num2->dec.digits == 0) {
                /* hack: set a default if none set already */
                num2->dec.digits = NCX_DEF_FRACTION_DIGITS;
            }
            /* this may cause an overflow, but too bad !!! */
            num2->dec.val = 
                (int64)(num1->l * (10 * num2->dec.digits));
            break;
        case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
            num2->d = (double)num1->l;
#else
            num2->d = (int64)num1->l;
#endif
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
            num2->i = (int32)num1->u;
            break;
        case NCX_BT_INT64:
            num2->l = (int64)num1->u;
            break;
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            num2->u = num1->u;
            break;
        case NCX_BT_UINT64:
            num2->ul = (uint64)num1->u;
            break;
        case NCX_BT_DECIMAL64:
            if (num2->dec.digits == 0) {
                /* hack: set a default if none set already */
                num2->dec.digits = NCX_DEF_FRACTION_DIGITS;
            }
            /* this may cause an overflow, but too bad !!! */
            num2->dec.val = 
                (int64)(num1->u * (10 * num2->dec.digits));
            break;
        case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
            num2->d = (double)num1->u;
#else
            num2->d = (int64)num1->u;
#endif
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_UINT64:
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            res = ERR_NCX_INVALID_VALUE;
            break;
        case NCX_BT_INT64:
            num2->l = (int64)num1->ul;
            break;
        case NCX_BT_UINT64:
            num2->ul = num1->ul;
            break;
        case NCX_BT_DECIMAL64:
            if (num2->dec.digits == 0) {
                /* hack: set a default if none set already */
                num2->dec.digits = NCX_DEF_FRACTION_DIGITS;
            }
            /* this may cause an overflow, but too bad !!! */
            num2->dec.val = 
                (int64)(num1->ul * (10 * num2->dec.digits));
            break;
        case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
            num2->d = (double)num1->ul;
#else
            num2->d = (int64)num1->ul;
#endif
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_DECIMAL64:
        if (num1->dec.digits == 0) {
            res = ERR_NCX_INVALID_VALUE;
        } else {
            /* just use testbase for now;
             * not sure if this will ever be used
             */
            testbase = num1->dec.val / (10 * num1->dec.digits);
            /* testfrac = num1->dec.val % (10 * num1->dec.digits); */

            switch (btyp2) {
            case NCX_BT_INT8:
            case NCX_BT_INT16:
            case NCX_BT_INT32:
            case NCX_BT_UINT8:
            case NCX_BT_UINT16:
            case NCX_BT_UINT32:
                return ERR_NCX_INVALID_VALUE;
            case NCX_BT_INT64:
                /* just do a floor() function for now */
                num2->l = testbase;
                break;
            case NCX_BT_UINT64:
                num2->ul = (uint64)testbase;
                break;
            case NCX_BT_DECIMAL64:
                num2->dec.val = num1->dec.val;
                num2->dec.digits = num1->dec.digits;
                num2->dec.zeroes = num1->dec.zeroes;
                break;
            case NCX_BT_FLOAT64:
                num2->d = (double)testbase;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
        case NCX_BT_DECIMAL64:
            return ERR_NCX_INVALID_VALUE;
        case NCX_BT_INT64:
            {
                double d = num1->d;
                num2->l = (int64)lrint(d);
            }
            break;
        case NCX_BT_UINT64:
            {
                double d = num1->d;
                num2->ul = (uint64)lrint(d);
           }
            break;
        case NCX_BT_FLOAT64:
            num2->d = num1->d;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
#else
        switch (btyp2) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
        case NCX_BT_DECIMAL64:
            return ERR_NCX_INVALID_VALUE;
        case NCX_BT_INT64:
            num2->l = num1->d;
            break;
        case NCX_BT_UINT64:
            num2->ul = (uint64)num1->d;
            break;
        case NCX_BT_FLOAT64:
            num2->d = num1->d;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
#endif
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}  /* ncx_cast_num */


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
status_t
    ncx_num_floor (const ncx_num_t *num1,
                   ncx_num_t *num2,
                   ncx_btype_t  btyp)
{
    status_t   res;

#ifdef DEBUG
    if (!num1 || !num2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num2->i = num1->i;
        break;
    case NCX_BT_INT64:
        num2->l = num1->l;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        num2->u = num1->u;
        break;
    case NCX_BT_UINT64:
        num2->ul = num1->ul;
        break;
    case NCX_BT_DECIMAL64:
        num2->dec.digits = num1->dec.digits;
        num2->dec.val = num1->dec.val % (10 * num1->dec.digits);
        num2->dec.zeroes = num1->dec.zeroes;
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        {
            double d = num1->d;
            num2->d = floor(d);
        }
#else
        num2->d = num1->d;
#endif
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;
    
}  /* ncx_num_floor */


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
status_t
    ncx_num_ceiling (const ncx_num_t *num1,
                     ncx_num_t *num2,
                     ncx_btype_t  btyp)
{
    status_t   res;

#ifdef DEBUG
    if (!num1 || !num2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num2->i = num1->i;
        break;
    case NCX_BT_INT64:
        num2->l = num1->l;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        num2->u = num1->u;
        break;
    case NCX_BT_UINT64:
        num2->ul = num1->ul;
        break;
    case NCX_BT_DECIMAL64:
        num2->dec.digits = num1->dec.digits;
        /*** !!!!! this is not right !!!! ***/
        num2->dec.val = num1->dec.val % (10 * num1->dec.digits);
        num2->dec.zeroes = num1->dec.zeroes;
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        {
            double d = num1->d;
            num2->d = ceil(d);
        }
#else
        num2->d = num1->d;
#endif
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;
    
}  /* ncx_num_ceiling */


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
status_t
    ncx_round_num (const ncx_num_t *num1,
                   ncx_num_t *num2,
                   ncx_btype_t  btyp)
{
    status_t   res;

#ifdef DEBUG
    if (!num1 || !num2) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        num2->i = num1->i;
        break;
    case NCX_BT_INT64:
        num2->l = num1->l;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        num2->u = num1->u;
        break;
    case NCX_BT_UINT64:
        num2->ul = num1->ul;
        break;

    case NCX_BT_DECIMAL64:
        num2->dec.digits = num1->dec.digits;
        /*** this is not right !!!! ***/
        num2->dec.val = num1->dec.val % (10 * num1->dec.digits);
        num2->dec.zeroes = num1->dec.zeroes;
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        {
            double d = num1->d;
            num2->d = round(d);
        }
#else
        num2->d = num1->d;
#endif
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;
    
}  /* ncx_round_num */


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
boolean
    ncx_num_is_integral (const ncx_num_t *num,
                         ncx_btype_t  btyp)
{
#ifdef HAS_FLOAT
    double d;
#endif

#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
        return TRUE;
    case NCX_BT_DECIMAL64:
        if (num->dec.digits == 0) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return FALSE;
        }
        if (num->dec.val / (10 * num->dec.digits)) {
            return TRUE;
        } else {
            return FALSE;
        }
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT        
        d = num->d;
        d = round(d);
        return (d == num->d) ? TRUE : FALSE;
#else
        return TRUE;
#endif
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}  /* ncx_num_is_integral */


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
int64
    ncx_cvt_to_int64 (const ncx_num_t *num,
                      ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        return (int64)num->i;
    case NCX_BT_INT64:
        return num->l;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        return (int64)num->u;
    case NCX_BT_UINT64:
        return (int64)num->ul;
    case NCX_BT_DECIMAL64:
        if (num->dec.digits == 0) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return 0;
        }
        return (int64)(num->dec.val / (10 * num->dec.digits));
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT        
        {
            double d = num->d;
            return lrint(d);
        }
#else
        return num->d;
#endif
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
    /*NOTREACHED*/

}  /* ncx_cvt_to_int64 */


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
ncx_numfmt_t
    ncx_get_numfmt (const xmlChar *numstr)
{
#ifdef DEBUG
    if (!numstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_NF_NONE;
    }
#endif

    if (*numstr == '\0') {
        return NCX_NF_NONE;
    }

    /* check for a HEX string first */
    if (*numstr=='0' && (numstr[1]=='x' || numstr[1]=='X')) {
        return NCX_NF_HEX;
    }

    /* check real number next */
    while (*numstr && (*numstr != '.')) {
        numstr++;
    }
    if (*numstr) {
        return NCX_NF_REAL;
    }

    /* leading zero means octal, otherwise decimal */
    return (*numstr == '0') ? NCX_NF_OCTAL : NCX_NF_DEC;

}  /* ncx_get_numfmt */


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
void
    ncx_printf_num (const ncx_num_t *num,
                    ncx_btype_t  btyp)
{
    xmlChar   numbuff[VAL_MAX_NUMLEN];
    uint32    len;
    status_t  res;

#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    res = ncx_sprintf_num(numbuff, num, btyp, &len);
    if (res != NO_ERR) {
        log_write("invalid num '%s'", get_error_string(res));
    } else {
        log_write("%s", numbuff);
    }

} /* ncx_printf_num */


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
void
    ncx_alt_printf_num (const ncx_num_t *num,
                        ncx_btype_t  btyp)
{
    xmlChar   numbuff[VAL_MAX_NUMLEN];
    uint32    len;
    status_t  res;

#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    res = ncx_sprintf_num(numbuff, num, btyp, &len);
    if (res != NO_ERR) {
        log_alt_write("invalid num '%s'", get_error_string(res));
    } else {
        log_alt_write("%s", numbuff);
    }

} /* ncx_alt_printf_num */


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
status_t
    ncx_sprintf_num (xmlChar *buff,
                     const ncx_num_t *num,
                     ncx_btype_t  btyp,
                     uint32   *len)
{
    xmlChar  *point;
    int32     ilen, pos, i;
    uint32    ulen;
    xmlChar   dumbuff[VAL_MAX_NUMLEN];
    xmlChar   decbuff[VAL_MAX_NUMLEN];
#ifdef HAS_FLOAT
    int32     tzcount;
#endif

#ifdef DEBUG
    if (!num || !len) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (!buff) {
        buff = dumbuff;    
    }

    ilen = 0;
    *len = 0;
    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        ilen = sprintf((char *)buff, "%d", num->i);
        break;
    case NCX_BT_INT64:
        ilen = sprintf((char *)buff, "%lld", (long long)num->l);
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        ilen = sprintf((char *)buff, "%u", num->u);
        break;
    case NCX_BT_UINT64:
        ilen = sprintf((char *)buff, "%llu", (unsigned long long)num->ul);
        break;
    case NCX_BT_DECIMAL64:
        if (num->dec.val == 0) {
            ilen = xml_strcpy(buff, (const xmlChar *)"0.0");
        } else if (num->dec.zeroes > 0) {
            ilen = xml_strcpy(buff, (const xmlChar *)"0.");
            i = 1;
            while (i < num->dec.zeroes) {
                buff[ilen++] = '0';
                i++;
            }
            ilen += sprintf((char *)&buff[ilen], "%lld", 
                            (long long)num->dec.val);
        } else {
            if (num->dec.digits == 0) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            } else {
                /* get the encoded number in the temp buffer */
                pos = sprintf((char *)decbuff, 
                              "%lld", 
                              (long long)num->dec.val);

                if (pos <= num->dec.digits) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    /* find where the decimal point should go */
                    point = &decbuff[pos - num->dec.digits];

                    /* copy the base part to the real buffer */
                    ulen = xml_strncpy(buff, 
                                       decbuff, 
                                       (uint32)(point - decbuff));

                    buff[ulen] = '.';

                    xml_strcpy(&buff[ulen+1], point);

                    /* current length is pos+1
                     * need to check for trailing zeros
                     * and remove them
                     * (!!! need flag to override!!!)
                     * !!! TBD: WAITING FOR WG TO DECIDE
                     * !! RETURN WITH TRAILING ZEROS FOR NOW
                     */
                    ilen = pos + 1;
                }
            }
        }
        break;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        ilen = sprintf((char *)buff, "%.14f", num->d);
        tzcount = remove_trailing_zero_count(buff);
        if (tzcount) {
            ilen -= tzcount;
            if (buff != dumbuff) {
                buff[ilen] =  0;
            }
        }
#else
        ilen = sprintf((char *)buff, "%lld", (long long)num->d);
#endif
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* check the sprintf return value */
    if (ilen < 0) {
        return ERR_NCX_INVALID_NUM;
    } else {
        *len = (uint32)ilen;
    }
    return NO_ERR;

} /* ncx_sprintf_num */


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
boolean
    ncx_is_min (const ncx_num_t *num,
                ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
        return (num->i == NCX_MIN_INT8) ? TRUE : FALSE;
    case NCX_BT_INT16:
        return (num->i == NCX_MIN_INT16) ? TRUE : FALSE;
    case NCX_BT_INT32:
        return (num->i == NCX_MIN_INT) ? TRUE : FALSE;
    case NCX_BT_INT64:
        return (num->l == NCX_MIN_LONG) ? TRUE : FALSE;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        return (num->u == NCX_MIN_UINT) ? TRUE : FALSE;
    case NCX_BT_UINT64:
        return (num->ul == NCX_MIN_ULONG) ? TRUE : FALSE;

    case NCX_BT_DECIMAL64:
        return (num->dec.val == NCX_MIN_LONG) ? TRUE : FALSE;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        return (num->d == -INFINITY) ? TRUE : FALSE;
#else
        return (num->d == NCX_MIN_LONG) ? TRUE : FALSE;
#endif
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}  /* ncx_is_min */


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
boolean
    ncx_is_max (const ncx_num_t *num,
                ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (btyp) {
    case NCX_BT_INT8:
        return (num->i == NCX_MAX_INT8) ? TRUE : FALSE;
    case NCX_BT_INT16:
        return (num->i == NCX_MAX_INT16) ? TRUE : FALSE;
    case NCX_BT_INT32:
        return (num->i == NCX_MAX_INT) ? TRUE : FALSE;
    case NCX_BT_INT64:
        return (num->l == NCX_MAX_LONG) ? TRUE : FALSE;
    case NCX_BT_UINT8:
        return (num->u == NCX_MAX_UINT8) ? TRUE : FALSE;
    case NCX_BT_UINT16:
        return (num->u == NCX_MAX_UINT16) ? TRUE : FALSE;
    case NCX_BT_UINT32:
        return (num->u == NCX_MAX_UINT) ? TRUE : FALSE;
    case NCX_BT_UINT64:
        return (num->ul == NCX_MAX_ULONG) ? TRUE : FALSE;
    case NCX_BT_DECIMAL64:
        return (num->dec.val == NCX_MAX_LONG) ? TRUE : FALSE;
    case NCX_BT_FLOAT64:
#ifdef HAS_FLOAT
        return (num->d == INFINITY) ? TRUE : FALSE;
#else
        return (num->d == NCX_MAX_LONG-1) ? TRUE : FALSE;
#endif
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}  /* ncx_is_max */


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
status_t
    ncx_convert_tkcnum (tk_chain_t *tkc,
                        ncx_btype_t btyp,
                        ncx_num_t *val)
{
    const xmlChar *numstr;

    if (btyp == NCX_BT_DECIMAL64) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_DNUM:
        numstr = TK_CUR_VAL(tkc);
        if (numstr && *numstr=='0') {
            return ncx_convert_num(TK_CUR_VAL(tkc), 
                                   NCX_NF_OCTAL, 
                                   btyp, 
                                   val);
        } else {
            return ncx_convert_num(TK_CUR_VAL(tkc), 
                                   NCX_NF_DEC, 
                                   btyp, 
                                   val);
        }
    case TK_TT_HNUM:
        return ncx_convert_num(TK_CUR_VAL(tkc), 
                               NCX_NF_HEX, 
                               btyp, 
                               val);
    case TK_TT_RNUM:
        return ncx_convert_num(TK_CUR_VAL(tkc), 
                               NCX_NF_REAL, 
                               btyp, 
                               val);
    default:
        /* if this is a string, then this might work */
        return ncx_decode_num(TK_CUR_VAL(tkc), btyp, val);
    }
}  /* ncx_convert_tkcnum */


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
status_t
    ncx_convert_tkc_dec64 (tk_chain_t *tkc,
                           uint8 digits,
                           ncx_num_t *val)
{
    const xmlChar *numstr;

#ifdef DEBUG
    if (!tkc || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (TK_CUR_TYP(tkc)) {
    case TK_TT_DNUM:
        numstr = TK_CUR_VAL(tkc);
        if (numstr && *numstr=='0' && numstr[1] != '.') {
            return ncx_convert_dec64(TK_CUR_VAL(tkc), 
                                     NCX_NF_OCTAL, 
                                     digits, 
                                     val);
        } else {
            return ncx_convert_dec64(TK_CUR_VAL(tkc), 
                                     NCX_NF_DEC, 
                                     digits, 
                                     val);
        }
    case TK_TT_HNUM:
        return ncx_convert_dec64(TK_CUR_VAL(tkc), 
                                 NCX_NF_HEX, 
                                 digits, 
                                 val);
    case TK_TT_RNUM:
        return ncx_convert_dec64(TK_CUR_VAL(tkc), 
                                 NCX_NF_REAL, 
                                 digits, 
                                 val);
    default:
        /* if this is a string, then this might work */
        return ncx_decode_dec64(TK_CUR_VAL(tkc), 
                                digits, 
                                val);
    }
}  /* ncx_convert_tkc_dec64 */


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
int64
    ncx_get_dec64_base (const ncx_num_t *num)
{
    int64 temp1;

#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif
    temp1 = num->dec.val;
    if (num->dec.digits) {
         temp1 = temp1 / (10 * num->dec.digits);
    }
    return temp1;

}  /* ncx_get_dec64_base */


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
int64
    ncx_get_dec64_fraction (const ncx_num_t *num)
{
    int64 temp1;

#ifdef DEBUG
    if (!num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (num->dec.digits) {
        temp1 = num->dec.val % (10 * num->dec.digits);
    } else {
        temp1 = 0;
    }

    return temp1;

}  /* ncx_get_dec64_fraction */


/* END file ncx_num.c */

