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
/*  FILE: blob.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
22apr05      abb      begun, borrowed from gr8cxt2 code

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_blob
#include  "blob.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION c2i
* Convert an ASCII HEX char ('0'..'F') into a number (0..15)
* Only Works on CAPITAL hex letters: ABCDEF
* INPUTS:
*   c: char to convert
* RETURNS:
*   value converted to its numeric value
*********************************************************************/
static uint32 
    c2i (const char *c)
{
    switch (*c) {
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    default:
      return (uint32)(*c-'0');   /* assume a number 0..9 */
    }
    /*@notreached@*/
} /* c2i */


/********************************************************************
* FUNCTION i2c
* Convert a number to its ASCII HEX char value ('0'..'F')
* Only Works on CAPITAL hex letters: ABCDEF
* INPUTS:
*   i: number to convert (0..15)
* RETURNS:
*   value converted to its HEX character ('0'..'F')
*********************************************************************/
static char 
    i2c (uint32 i)
{
    if (i<10) {
        return (char)(i+(uint32)'0');
    } else /* if (i<16) */ {
        return (char)(i-10+(uint32)'A');
    } 
    /*@notreached@*/
} /* i2c */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION blob2bin
*
* Convert a mySQL BLOB to a binary string
*
* INPUTS:
*   pblob == pointer to BLOB to convert; must be of the
*            same type as the binary object; this blob will be
*            bsize * 2 + 1 bytes in length
*   pbuff == pointer to buffer to fill in; must be at least
*            bsize+1 bytes in length
*   bsize == binary object size
* OUTPUTS:
*   pbuff is filled in 
*********************************************************************/
void 
    blob2bin (const char *pblob, 
              unsigned char *pbuff,
              uint32 bsize)
{
    uint32  i, b1, b2;

    for (i=0;i<bsize;i++) {
        b1 = c2i(pblob++);
        b2 = c2i(pblob++);
        pbuff[i] = (unsigned char)((b1*16)+b2);
    }
} /* blob2bin */


/********************************************************************
* FUNCTION bin2blob
*
* Convert a binary string to to a mySQL BLOB
*
* INPUTS:
*   pbuff == pointer to buffer to convert; must be at least
*            bsize+1 bytes in length
*   pblob == pointer to BLOB to fill in; must be at least
*            bsize * 2 + 1 bytes in length
*   bsize == binary object size
* OUTPUTS:
*   pblob is filled in, and zero-terminated
*********************************************************************/
void 
    bin2blob (const unsigned char *pbuff,
              char *pblob,
              uint32 bsize)
{
    uint32  i, b1, b2;

    for (i=0;i<bsize;i++) {
        b1 = (uint32)(*pbuff >> 4);
        b2 = (uint32)(*pbuff++ & 0xf);
        *pblob++ = i2c(b1);
        *pblob++ = i2c(b2);
    }
    *pblob=0x0;
} /* bin2blob */

/* END file blob.c */
