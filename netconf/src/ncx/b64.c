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

#include "b64.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "log.h"

/** translation Table used for encoding characters */
static const char encodeCharacterTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                           "abcdefghijklmnopqrstuvwxyz"
                                           "0123456789+/";

/** translation Table used for decoding characters */
static const char decodeCharacterTable[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

// ---------------------------------------------------------------------------|
/**
 * Determine if a character is a valid base64 value.
 *
 * \param c the character to test.
 * \return true if c is a base64 value.
 */
static bool is_base64(unsigned char c)
{
   return (isalnum(c) || (c == '+') || (c == '/'));
}

// ---------------------------------------------------------------------------|
/**
 * Count the number of valid base64 characters in the supplied input
 * buffer.
 * @desc this function counts the number of valid base64 characters,
 * skipping CR and LF characters. Invalid characters are treated as
 * end of buffer markers.
 *
 * \param inbuff the buffer to check.
 * \param inputlen the length of the input buffer.
 * \return the number of valid base64 characters.
 */
static uint32_t get_num_valid_characters( const uint8_t* inbuff, 
                                          size_t inputlen ) 
{
    uint32_t validCharCount=0;
    uint32_t idx=0;

    while ( idx < inputlen )
    {
        if ( is_base64( inbuff[idx] ) )
        {
            ++validCharCount;
            ++idx;
        }
        else if ( inbuff[idx] == '\r' || inbuff[idx] == '\n' ) 
        {
            ++idx;
        }
        else
        {
            break;
        }
    }
    return validCharCount;
}
// ---------------------------------------------------------------------------|
/**
 * Decode the 4 byte array pf base64 values into 3 bytes.
 *
 * \param inbuff the buffer to decode
 * \param outbuff the output buffer for decoded bytes.
 */
static void decode4Bytes( const uint8_t inbuff[4], uint8_t* outbuff )
{
    outbuff[0] = (decodeCharacterTable[ inbuff[0] ] << 2) + 
                 ((decodeCharacterTable[ inbuff[1] ] & 0x30) >> 4);
    outbuff[1] = ((decodeCharacterTable[ inbuff[1] ] & 0xf) << 4) + 
                 ((decodeCharacterTable[ inbuff[2] ] & 0x3c) >> 2);
    outbuff[2] = ((decodeCharacterTable[ inbuff[2] ] & 0x3) << 6) + 
                 decodeCharacterTable[ inbuff[3] ];
}

// ---------------------------------------------------------------------------|
/**
 * Extract up to 4 bytes of base64 data to decode.
 * this function skips CR anf LF characters and extracts up to 4 bytes
 * from the input buffer. Any invalid characters are treated as end of
 * buffer markers.
 *
 * \param iterPos the start of the data to decode. This value is updated.
 * \param endpos the end of the buffer marker.
 * \param arr4 the output buffer for extracted bytes (zero padded if
 * less than 4 bytes were extracted)
 * \param numExtracted the number of valid bytes that were extracted.
 * output buffer for extracted bytes.
 * \return true if the end of buffer was reached.
 */
static bool extract_4bytes( const uint8_t** iterPos, const uint8_t* endPos, 
                            uint8_t arr4[4], uint32_t* numExtracted )
{
    const uint8_t* iter=*iterPos;
    uint32_t validBytes = 0;
    bool endReached = false;

    while ( iter < endPos && validBytes<4 && !endReached )
    {
        if ( is_base64( *iter ) )
        {
            arr4[validBytes++] = *iter;
            ++iter;
        }
        else if ( *iter == '\r' || *iter == '\n' ) 
        {
            ++iter;
        }
        else 
        {
            // encountered a dodgy character or an =
            if ( *iter != '=' )
            {
               log_warn( "b64_decode() encountered invalid character(%c), "
                         "output string truncated!", *iter );
            }

            // pad the remaining characters to decode
            size_t pad = validBytes;
            for ( ; pad<4; ++pad )
            {
                arr4[pad] = 0;
            }
            endReached = true;
        }
    }

    *numExtracted = validBytes;
    *iterPos = iter;
    return endReached ? endReached : iter >= endPos;
}

// ---------------------------------------------------------------------------|
static bool extract_3bytes( const uint8_t** iterPos, const uint8_t* endPos, 
                            uint8_t arr3[3], uint32_t* numExtracted )
{
    const uint8_t* iter=*iterPos;
    uint32_t byteCount = 0;

    while ( iter < endPos && byteCount < 3 )
    {
        arr3[ byteCount++ ] = *iter;
        ++iter;
    }

    *numExtracted = byteCount;
    while ( byteCount < 3 )
    {
        arr3[byteCount++] = 0;
    }

    *iterPos = iter;
    return iter >= endPos;
}


// ---------------------------------------------------------------------------|
static bool encode_3bytes( const uint8_t** iterPos, const uint8_t* endPos, 
                                 uint8_t** outPos )
{
    uint8_t arr3[3];
    uint32_t numBytes;

    bool endReached = extract_3bytes( iterPos, endPos, arr3, &numBytes );

    if ( numBytes )
    {
        uint8_t* outArr = *outPos;
        outArr[0] = encodeCharacterTable[ ( arr3[0] & 0xfc) >> 2 ];
        outArr[1] = encodeCharacterTable[ ( ( arr3[0] & 0x03 ) << 4 ) + 
                                          ( ( arr3[1] & 0xf0 ) >> 4 ) ];
        outArr[2] = encodeCharacterTable[ ( ( arr3[1] & 0x0f ) << 2 ) + 
                                          ( ( arr3[2] & 0xc0 ) >> 6 ) ];
        outArr[3] = encodeCharacterTable[ arr3[2] & 0x3f ];

        // pad with '=' characters
        while ( numBytes < 3 )
        {
            outArr[numBytes+1] = '=';
            ++numBytes;
        }
        *outPos += 4;
    }

    return endReached;
}

/*************** E X T E R N A L    F U N C T I O N S  *************/

// ---------------------------------------------------------------------------|
uint32_t b64_get_encoded_str_len( uint32_t inbufflen, uint32_t linesize )
{
    uint32_t requiredBufLen = inbufflen%3 ? 4 * ( 1 + inbufflen/3 )  
                                          : 4 * ( inbufflen/3 );
    requiredBufLen += 2 * ( inbufflen/linesize );  // allow for line breaks
    requiredBufLen += 3; // allow for final CR-LF and null terminator

    return requiredBufLen;
}

// ---------------------------------------------------------------------------|
uint32_t b64_get_decoded_str_len( const uint8_t* inbuff, size_t inputlen )
{
    uint32_t validCharCount= get_num_valid_characters( inbuff, inputlen );

    uint32_t requiredBufLen = 3*(validCharCount/4);
    uint32_t rem=validCharCount%4;
    if ( rem )
    {
        requiredBufLen += rem-1;
    }

    return requiredBufLen;
}

// ---------------------------------------------------------------------------|
status_t b64_encode ( const uint8_t* inbuff, uint32_t inbufflen,
                            uint8_t* outbuff, uint32_t outbufflen,
                            uint32_t linesize, uint32_t* retlen)
{
    assert( inbuff && "b64_decode() inbuff is NULL!" );
    assert( outbuff && "b64_decode() outbuff is NULL!" );

    if ( b64_get_encoded_str_len( inbufflen, linesize ) > outbufflen )
    {
        return ERR_BUFF_OVFL;
    }

    bool endReached = false;
    const uint8_t* endPos = inbuff+inbufflen;
    const uint8_t* iter = inbuff;
    uint8_t* outIter = outbuff;
    uint32_t numBlocks=0;

    while( !endReached )
    {
        endReached = encode_3bytes( &iter, endPos, &outIter );
        ++numBlocks;
        if ( numBlocks*4 >= linesize )
        {
            *outIter++ ='\r';
            *outIter++ ='\n';
        }
    }

    *outIter++ ='\r';
    *outIter++ ='\n';

    *retlen = outIter - outbuff;
    *outIter++ ='\0';
    return NO_ERR;
}

// ---------------------------------------------------------------------------|
status_t b64_decode ( const uint8_t* inbuff, uint32_t inbufflen,
                            uint8_t* outbuff, uint32_t outbufflen,
                            uint32_t* retlen )
{
    uint8_t arr4[4];
    uint32_t numExtracted = 0;
    bool endReached = false;
    const uint8_t* endPos = inbuff+inbufflen;
    const uint8_t* iter = inbuff;

    assert( inbuff && "b64_decode() inbuff is NULL!" );
    assert( outbuff && "b64_decode() outbuff is NULL!" );

    *retlen=0;

    while ( !endReached )
    {
        endReached = extract_4bytes( &iter, endPos, arr4, &numExtracted ); 

        if ( numExtracted )
        {
            if ( *retlen+3 > outbufflen )
            {
                return ERR_BUFF_OVFL;
            }
            decode4Bytes( arr4, outbuff+*retlen );
            *retlen += numExtracted-1;
        }
    }

    return NO_ERR;
}

/* END b64.c */

