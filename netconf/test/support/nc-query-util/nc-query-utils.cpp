// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-utils.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
string extractMessageIdStr( const std::string& query )
{
    using namespace boost::xpressive;

    sregex re = sregex::compile( "message-id=\"(\\d+)\"" ); 
    smatch what;

    // find a whole word
    BOOST_REQUIRE_MESSAGE( regex_search( query, what, re ), 
                           "Invalid XML Query - No message-id!" );
    return what[1];
}

// ---------------------------------------------------------------------------|
uint16_t extractMessageId( const std::string& query )
{
    return boost::lexical_cast<uint16_t>( extractMessageIdStr( query ) );
}

}
