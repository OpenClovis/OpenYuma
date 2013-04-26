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
#ifndef _H_b64
#define _H_b64

/*  FILE: b64.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    RFC 4648 base64 support, from b64.c

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
25-oct-08    abb      Begun

*/

#include "procdefs.h"
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/

/**
 * base64 encode a stream adding padding and line breaks as per spec.
 *
 * \param inbuff pointer to buffer of binary chars
 * \param inbufflen number of binary chars in inbuff
 * \param outbuff pointer to the output buffer to use.
 * \param outbufflen max number of chars to write to outbuff
 * \param linesize the output line length to use
 * \param retlen address of return length
 * \returns NO_ERR if all OK, ERR_BUFF_OVFL if outbuff not big enough
 */
status_t b64_encode ( const unsigned char *inbuff,
		              unsigned int inbufflen,
		              unsigned char *outbuff, 
		              unsigned int outbufflen,
		              unsigned int linesize,
		              unsigned int *retlen );

/**
 * Decode a base64 string.
 * This function decodes the supplied base 64 string. It has the
 * following constraints:
 * 1 - The length of the supplied base64 string must be divisible by 4
 *     (any other length strings are NOT valid base 64)
 * 2 - If any non base64 characters are encountered the length of
 *     decoded string will be truncated.
 * 3 - CR and LF characters in the encoded string will be skipped.
 *
 * \param inbuff pointer to buffer of base64 chars
 * \param inbufflen number of chars in inbuff
 * \param outbuff pointer to the output buffer to use
 * \param outbufflen the length of outbuff.
 * \param retlen the number of decoded bytes
 * \return NO_ERR if all OK
 *         ERR_BUFF_OVFL if outbuff not big enough
 */
status_t b64_decode ( const uint8_t* inbuff, uint32_t inbufflen,
                            uint8_t* outbuff, uint32_t outbufflen,
                            uint32_t* retlen );
/**
 * Calculate the length of the buffer required to decode the 
 * base64 string.
 *
 * @param inbuff the base64 string to decode.
 * @param inputlen the length of inbuff.
 * @return the length of the required buffer.
 */
uint32_t b64_get_decoded_str_len( const uint8_t* inbuff, size_t inputLen );

/**
 * Get the output buffer size required for encoding the string. 
 *
 * \param inbufflen the size of the input buffer.
 * \param linesize the length of each line 
 * \return the size required for encoding the string. 
 */
uint32_t b64_get_encoded_str_len( uint32_t inbufflen, uint32_t linesize );

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_b64 */
