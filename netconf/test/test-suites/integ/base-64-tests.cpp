// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Standard Includes 
// ---------------------------------------------------------------------------|
#include <string>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/base-suite-fixture.h"
#include "test/support/misc-util/base64.h"
#include "test/support/misc-util/log-utils.h"
#include "b64.h"
#include "status.h"

// ---------------------------------------------------------------------------|
using namespace std;
using namespace YumaTest;

// ---------------------------------------------------------------------------|
namespace 
{

void test_yuma_b64_decode( const string& rawText )
{
    string rawEncoded = base64_encode( rawText );

    char decodedChars[ 200 ];
    unsigned int decodedLen = 0;

    BOOST_CHECK( rawText.length() <= b64_get_decoded_str_len( 
                reinterpret_cast<const uint8_t*>( rawEncoded.c_str() ), rawEncoded.length() ) );

    BOOST_REQUIRE_EQUAL( NO_ERR, 
            b64_decode( reinterpret_cast< const uint8_t* >( rawEncoded.c_str() ),
                rawEncoded.length(),
                reinterpret_cast< uint8_t* >( decodedChars ),
                200,
                &decodedLen ) );

    BOOST_CHECK_EQUAL( rawText.length(), decodedLen );
    BOOST_CHECK_EQUAL( rawText, string( decodedChars, decodedLen ) );
}

void test_yuma_b64_decode_Ins( const string& rawText, 
                        const string& insText, 
                        uint16_t insPos, 
                        bool valid=true )
{
    string rawEncoded = base64_encode( rawText );
    string modified( rawEncoded );
    modified.insert(insPos, insText );
    modified += "\r\n";

    char decodedChars[ 200 ];
    unsigned int decodedLen = 0;

    BOOST_REQUIRE_EQUAL( NO_ERR, 
            b64_decode( reinterpret_cast< const uint8_t* >( modified.c_str() ),
                modified.length(),
                reinterpret_cast< uint8_t* >( decodedChars ),
                200,
                &decodedLen ) );

    BOOST_CHECK_EQUAL( decodedLen, b64_get_decoded_str_len(
                reinterpret_cast<const uint8_t*>( modified.c_str() ), modified.length() ) );

    if ( valid )
    {
       BOOST_CHECK_EQUAL( rawText.length(), decodedLen );
       BOOST_CHECK_EQUAL( rawText, string( decodedChars, decodedLen ) );
    }
    else
    {
       // check the decoded string was truncated to insPos length
       uint16_t expLen = 0;

       switch ( insPos%4 )
       {
           case 0: expLen = (insPos/4)*3; break;
           case 1: expLen = (insPos/4)*3; break;
           case 2: expLen = (insPos%4)-1+(insPos/4)*3; break;
           case 3: expLen = (insPos%4)-1+(insPos/4)*3; break;
       }

       BOOST_CHECK_EQUAL( expLen, decodedLen );
    }
}

void test_yuma_b64_encode( const string& rawText )
{
    // TODO: test embedding of CR-LF characters
    string th_encoded = base64_encode( rawText );
    string th_encoded_appended = th_encoded += "\r\n\0";

    // encode using yumas b64_encode
    char encodedChars[ 200 ];
    unsigned int encodedLen = 0;
    BOOST_REQUIRE_EQUAL( NO_ERR, 
            b64_encode( reinterpret_cast< const unsigned char*>( rawText.c_str() ),
                rawText.length(),
                reinterpret_cast< unsigned char* >( encodedChars ),
                200,
                200,
                &encodedLen ) );
    string yumaEncoded( encodedChars, encodedLen );

    // check that the lengths match
    BOOST_CHECK_EQUAL( th_encoded_appended.length(), encodedLen );

    // check that the encoded string is as expected 
    BOOST_CHECK_EQUAL( th_encoded_appended, yumaEncoded );

    // check that the string decodes correctly
    BOOST_CHECK_EQUAL( rawText, base64_decode( yumaEncoded ) );
}

} // anonymous namespace

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( Base64Tests, BaseSuiteFixture )

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_large )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 1)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );
    string rawText = "ADP GmbH\nAnalyse Design & Programmierung\n"
                     "Gesellschaft mit beschränkter Haftung";

    test_yuma_b64_decode( rawText );
    test_yuma_b64_encode( rawText );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_1 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 1)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0" );
    test_yuma_b64_decode( "1" );
    test_yuma_b64_encode( "1" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_2 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 2)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "01" );
    test_yuma_b64_decode( "11" );
    test_yuma_b64_encode( "11" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_3 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 3)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "012" );
    test_yuma_b64_decode( "111" );
    test_yuma_b64_encode( "111" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_4 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 4)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123" );
    test_yuma_b64_decode( "1111" );
    test_yuma_b64_encode( "1111" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_5 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 5)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "01234" );
    test_yuma_b64_decode( "11110" );
    test_yuma_b64_encode( "11110" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_6 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 6)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "012345" );
    test_yuma_b64_decode( "111100" );
    test_yuma_b64_encode( "111100" );
}

BOOST_AUTO_TEST_CASE( encode_decode_7 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 7)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456" );
    test_yuma_b64_decode( "1111000" );
    test_yuma_b64_encode( "1111000" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_8 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 8)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "01234567" );
    test_yuma_b64_decode( "11110000" );
    test_yuma_b64_encode( "11110000" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_9 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 9)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "012345678" );
    test_yuma_b64_decode( "111100001" );
    test_yuma_b64_encode( "111100001" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_10 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 10)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789" );
    test_yuma_b64_decode( "1111000011" );
    test_yuma_b64_encode( "1111000011" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_11 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 11)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789A" );
    test_yuma_b64_decode( "11110000111" );
    test_yuma_b64_encode( "11110000111" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_12 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 12)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789AB" );
    test_yuma_b64_decode( "111100001111" );
    test_yuma_b64_encode( "111100001111" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_13 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 13)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789ABC" );
    test_yuma_b64_decode( "1111000011110" );
    test_yuma_b64_encode( "1111000011110" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_14 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 14)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789ABCD" );
    test_yuma_b64_decode( "11110000111100" );
    test_yuma_b64_encode( "11110000111100" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_15 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 15)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789ABCDE" );
    test_yuma_b64_decode( "111100001111000" );
    test_yuma_b64_encode( "111100001111000" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_16 )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string (length = 16)",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode( "0123456789ABCDEF" );
    test_yuma_b64_decode( "1111000011110000" );
    test_yuma_b64_encode( "1111000011110000" );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_crlf )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string "
            "containing CR-LF characters",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "\r\n", 8 );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_cr )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string "
            "containing CR characters",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "\r", 1 );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "\r", 2 );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "\r", 3 );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_nl )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string "
            "containing LF characters",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "\n", 11 );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( encode_decode_invalid )
{
    DisplayTestDescrption( 
            "Demonstrate Encode/Decode of Base64 format of string "
            "containing invalid characters",
            "Procedure: \n"
            "\t 1 - Encode and Decode the string\n"
            );

    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "!", 0, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "!", 1, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "£", 2, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "$", 3, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "%", 4, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "^", 5, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "*", 6, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", "(", 7, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", ")", 8, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", ")", 9, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", ")", 10, false );
    test_yuma_b64_decode_Ins( "0123456789ABCDEF", ")", 11, false );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

