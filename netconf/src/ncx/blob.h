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
#ifndef _H_blob
#define _H_blob
/*  FILE: blob.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    binary to string conversion for database storage

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22-apr-05    abb      begun
*/

#ifdef __cplusplus
extern "C" {
#endif

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
extern void 
    blob2bin (const char *pblob, 
	      unsigned char *pbuff,
	      uint32 bsize);


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
extern void 
    bin2blob (const unsigned char *pbuff,
	      char  *pblob,
	      uint32 bsize);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_blob */
