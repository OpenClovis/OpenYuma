// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <sstream>
#include <boost/foreach.hpp>

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
StringPresenceChecker::StringPresenceChecker( 
        const vector<string>& expPresentReplyText )
    : checkStrings_( expPresentReplyText )
{
}

// ---------------------------------------------------------------------------|
StringPresenceChecker::~StringPresenceChecker()
{
}

// ---------------------------------------------------------------------------|
void StringPresenceChecker::operator()( const string& checkStr ) const
{
    BOOST_FOREACH ( const string& val, checkStrings_ )
    {
        BOOST_TEST_MESSAGE( "\tChecking " << val << " is present" );
        BOOST_CHECK_NE( string::npos, checkStr.find( val ) );
    }

}

// ---------------------------------------------------------------------------|
StringNonPresenceChecker::StringNonPresenceChecker( 
        const vector<string>& expPresentReplyText )
    : checkStrings_( expPresentReplyText )
{
}

// ---------------------------------------------------------------------------|
StringNonPresenceChecker::~StringNonPresenceChecker()
{
}

// ---------------------------------------------------------------------------|
void StringNonPresenceChecker::operator()( const string& checkStr ) const
{
    BOOST_FOREACH ( const string& val, checkStrings_ )
    {
        BOOST_TEST_MESSAGE( "\tChecking " << val << " is NOT present" );
        BOOST_CHECK_EQUAL( string::npos, checkStr.find( val ) );
    }
}

// ---------------------------------------------------------------------------|
StringsPresentNotPresentChecker::StringsPresentNotPresentChecker( 
        const vector< string >& expPresentReplyText, 
        const vector< string >& expAbsentReplyText )
   : expPresentChecker_( expPresentReplyText )
   , expNotPresentChecker_( expAbsentReplyText )
{
}

// ---------------------------------------------------------------------------|
StringsPresentNotPresentChecker::~StringsPresentNotPresentChecker() 
{
}

// ---------------------------------------------------------------------------|
void StringsPresentNotPresentChecker::operator()( const string& queryResult ) const
{
    expPresentChecker_( queryResult ); 
    expNotPresentChecker_( queryResult );
}

} // namespace YumaTest
