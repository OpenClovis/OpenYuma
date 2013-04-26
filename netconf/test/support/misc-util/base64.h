#ifndef _YUMA_TEST_BASE64_H_
#define _YUMA_TEST_BASE64_H_

#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

//------------------------------------------------------------------------------
//
std::string base64_encode( const std::string& str_to_encode );
//
/// @brief Encode a byte buffer as a Base64 std::string.
///
/// @param bytes_to_encode Buffer of binary data to encode.
/// @param len Length of binary data to encode.
/// @return the base64 encoded std::string.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
std::string base64_decode( const std::string& encoded_string );
//
/// @brief Decode a base64 encoded std::string.
///
/// @param encoded_std::string std::string to decode.
/// @return the binary data encapsulated in a std::string type! (why not std::vector<byte>)
//------------------------------------------------------------------------------

// Example
//#include "base64.h"
//#include <iostream>
//
//int main() {
//  const std::string s = "ADP GmbH\nAnalyse Design & Programmierung\nGesellschaft mit beschränkter Haftung" ;
//
//  std::string encoded = base64_encode(s);
//  std::string decoded = base64_decode(encoded);
//
//  std::cout << "encoded: " << encoded << std::endl;
//  std::cout << "decoded: " << decoded << std::endl;
//
//  return 0;
//}

} // YumaTest

#endif /* _YUMA_TEST_BASE64_H_ */
