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
/*  FILE: bobhash.c
 *

  From Packet Sampling Techniques RFC 5475
  implemented from <draft-ietf-psamp-sample-tech-07.txt>

  BOB Hash Function  
     
    The BOB Hash Function is a Hash Function designed for having 
    each bit of the input affecting every bit of the return value 
    and using both 1-bit and 2-bit deltas to achieve the so called 
    avalanche effect [Jenk97]. This function was originally built 
    for hash table lookup with fast software implementation.  
           
    Input Parameters:  
    The input parameters of such a function are:  
    - the length of the input string (key) to be hashed, in    
    bytes. The elementary input blocks of Bob hash are the single 
    bytes, therefore no padding is needed.  
    - an init value (an arbitrary 32-bit number).  
     
    Built in parameters:  
    The Bob Hash uses the following built-in parameter:        
    - the golden ratio (an arbitrary 32-bit number used in the hash  
    function computation: its purpose is to avoid mapping all zeros 
    to all zeros);  
     
    Note: the mix sub-function (see mix (a,b,c) macro in the 
    reference code in 3.2.4) has a number of parameters governing 
    the shifts in the registers. The one presented is not the only 
    possible choice.  
     
    It is an open point whether these may be considered additional  
    built-in parameters to specify at function configuration.  
     
    Output.  
    The output of the BOB function is a 32-bit number. It should be 
    specified:  
    - A 32 bit mask to apply to the output  
    - The selection range as a list of non overlapping intervals 
    [start value, end value] where value is in [0,2^32]  

    Functioning:  
    The hash value is obtained computing first an initialization of 
    an internal state (composed of 3 32-bit numbers, called a, b, c 
    in the reference code below), then, for each input byte of the 
    key the internal state is combined by addition and mixed using 
    the mix sub-function. Finally, the internal state mixed one last 
    time and the third number of the state (c) is chosen as the 
    return value.  

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
17-oct-05    abb      begin -- adapted from PSAMP Sampling Techniques

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_bobhash
#include  "bobhash.h"
#endif


/********************************************************************
* FUNCTION mix
*
*   mix -- mix 3 32-bit values reversibly.  
*
*   For every delta with one or two bits set, and the deltas of 
*   all three high bits or all three low bits, whether the original 
*   value of a,b,c is almost all zero or is uniformly distributed,  
*     * If mix() is run forward or backward, at least 32 bits in 
*   a,b,c have at least 1/4 probability of changing.  
*     * If mix() is run forward, every bit of c will change between 
*   1/3 and 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-
*   bit deltas.) mix() was built out of 36 single-cycle latency 
*   instructions in a structure that could supported 2x parallelism, 
*   like so:  
*           a -= b;  
*           a -= c; x = (c>>13);  
*           b -= c; a ^= x;  
*           b -= a; x = (a<<8);  
*           c -= a; b ^= x;  
*           c -= b; x = (b>>13);  
*           ...  
*   Unfortunately, superscalar Pentiums and Sparcs can't take 
*   advantage of that parallelism.  They've also turned some of 
*   those single-cycle latency instructions into multi-cycle latency 
*   instructions  
*
* INPUTS: 
*   a, b, c == 3 unsigned long numbers to mix
* RETURNS
*   none
*********************************************************************/
#define mix(a,b,c)  \
{ \
    a -= b; a -= c; a ^= (c>>13); \
    b -= c; b -= a; b ^= (a<<8); \
    c -= a; c -= b; c ^= (b>>13); \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16); \
    c -= a; c -= b; c ^= (b>>5); \
    a -= b; a -= c; a ^= (c>>3);  \
    b -= c; b -= a; b ^= (a<<10); \
    c -= a; c -= b; c ^= (b>>15); \
}  /* MACRO mix */



/********************************************************************
* FUNCTION bobhash
*
* Calculate a 32-bit BOB hash value
*
*
* [Jenk97]   B. Jenkins, "Algorithm Alley", Dr. Dobb's Journal,
*              September 1997.
*              http://burtleburtle.net/bob/hash/doobs.html.
*
* Copied From:
*   Sampling and Filtering Techniques for IP Packet Selection
*   RFC 5475
*
*   Returns a 32-bit value.  Every bit of the key affects every bit 
*   of the return value.  Every 1-bit and 2-bit delta achieves 
*   avalanche. About 6*len+35 instructions.  
*
*   The best hash table sizes are powers of 2.  There is no need to 
*   do mod a prime (mod is sooo slow!).  If you need less than 32 
*   bits, use a bitmask.  For example, if you need only 10 bits, do  
*   h = (h & hashmask(10));  
*   In which case, the hash table should have hashsize(10) elements.  
*    
*   If you are hashing n strings (uint8 **)k, do it like this:  
*   for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);  
*     
*   By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may 
*   use this code any way you wish, private, educational, or 
*   commercial.  It's free.  
        
*   See http://burtleburtle.net/bob/hash/evahash.html  
*   Use for hash table lookup, or anything where one collision in 
*   2^^32 is acceptable.  Do NOT use for cryptographic purposes.  
*
* INPUTS: 
*   k       : the key (the unaligned variable-length array of bytes)  
*   len     : the length of the key, counting by bytes  
*   initval : can be any 4-byte value  
*
* RETURNS
*   unsigned long has value
*********************************************************************/
uint32 bobhash (register const uint8 *k,        /* the key */  
                register uint32  length,   /* the length of the key */  
                register uint32  initval)  /* an arbitrary value */  
{  
    register uint32 a,b,c,len;  

    /* Set up the internal state */  
    len = length;  
    a = b = 0x9e3779b9; /*the golden ratio; an arbitrary value */ 
    c = initval;         /* another arbitrary value */  
        
    /*------------------------------------ handle most of the key */  
    while (len >= 12) {  
        a += (k[0] +((uint32)k[1]<<8) +((uint32)k[2]<<16) 
              +((uint32)k[3]<<24));  
        b += (k[4] +((uint32)k[5]<<8) +((uint32)k[6]<<16) 
              +((uint32)k[7]<<24));  
        c += (k[8] +((uint32)k[9]<<8) +((uint32)k[10]<<16)
              +((uint32)k[11]<<24));  
        mix(a,b,c);  
        k += 12; len -= 12;  
    }  
        
    /*---------------------------- handle the last 11 bytes */  
    c += length;  
    switch (len) {    /* all the case statements fall through*/  
    case 11: c+=((uint32)k[10]<<24);  
    case 10: c+=((uint32)k[9]<<16);  
    case 9 : c+=((uint32)k[8]<<8);  
        /* the first byte of c is reserved for the length */  
    case 8 : b+=((uint32)k[7]<<24);  
    case 7 : b+=((uint32)k[6]<<16);  
    case 6 : b+=((uint32)k[5]<<8);  
    case 5 : b+=k[4];  
    case 4 : a+=((uint32)k[3]<<24);  
    case 3 : a+=((uint32)k[2]<<16);  
    case 2 : a+=((uint32)k[1]<<8);  
    case 1 : a+=k[0];  
        /* case 0: nothing left to add */  
    default:
        break;
    }  
    mix(a,b,c);  

    /*-------------------------------- report the result */  
    return c;  
} /* END bobhash */

/* END file bobhash.c */
