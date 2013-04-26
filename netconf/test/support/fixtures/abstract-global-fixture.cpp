// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/abstract-global-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/fixtures/test-context.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cassert>
#include <algorithm>
#include <iostream>

// ---------------------------------------------------------------------------|
// Boost Includes
// ---------------------------------------------------------------------------|
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp> 

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
namespace ph = boost::phoenix;
namespace ph_args = boost::phoenix::arg_names;
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{
// ---------------------------------------------------------------------------|
AbstractGlobalTestFixture::AbstractGlobalTestFixture( int argc, const char** argv )
    : numArgs_( argc )
    , argv_( argv )
{
}

// ---------------------------------------------------------------------------|
AbstractGlobalTestFixture::~AbstractGlobalTestFixture()
{
    DisplayTestBreak( '=', true );
}

// ---------------------------------------------------------------------------|
TestContext::TargetDbConfig 
AbstractGlobalTestFixture::getTargetDbConfig()
{
    const string running( "--target=running" );
    const char** endPos = argv_ + numArgs_; 
    const char** res = find_if( argv_, endPos, ph_args::arg1 == running );

    if ( res == endPos )
    {
        cout << "Using Candidate Config!\n";
        return TestContext::CONFIG_USE_CANDIDATE;
    }
    
    cout << "Using Running Config!\n";
    return TestContext::CONFIG_WRITEABLE_RUNNNIG;
}

// ---------------------------------------------------------------------------|
bool AbstractGlobalTestFixture::usingStartupCapability()
{
    const string startup( "--with-startup=true" );
    const char** endPos = argv_ + numArgs_; 
    const char** res = find_if( argv_, endPos, ph_args::arg1 == startup );

    if ( res == endPos )
    {
        cout << "Startup capability is NOT in use!\n";
        return false;
    }
    
    cout << "Startup capability is in use!\n";
    return true;
}

} // namespace YumaTest
