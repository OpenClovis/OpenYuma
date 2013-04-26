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
#ifndef _H_bobhash
#define _H_bobhash

/*  FILE: bobhash.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    BOB hash function

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
17-oct-05    abb      Begun -- copied from PSAMP Sampling Techniques 

*/

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

#define hashsize(n) ((uint32)1<<(n))  

#define hashmask(n) (hashsize(n)-1)  


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION bobhash
*
* Calculate a 32-bit BOB hash value
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
extern uint32
    bobhash (register const uint8 *k,        /* the key */  
             register uint32  length,   /* the length of the key */  
             register uint32  initval);  /* an arbitrary value */  

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_bobhash */
