// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-yang-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( simple_must, SimpleYangFixture )

BOOST_AUTO_TEST_CASE( add_must )
{
    DisplayTestDescrption( 
            "Demonstrate that attempts to validate entries disallowed by "
            "must statements cause an error.",
            "Procedure: \n"
            "\t1 - Create the top level container with a valid entry\n"
            "\t2 - Check that errors are returned for invalid entries\n"
            "\t3 - Delete the top level container\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // create the top level container
    createInterfaceContainer( primarySession_, "ethernet", 1500, "create" );
    createInterfaceContainer( primarySession_, "ethernet", 1501, "merge" );
    createInterfaceContainer( primarySession_, "atm", 64, "merge" );
    createInterfaceContainer( primarySession_, "atm", 17966, "merge" );
    createInterfaceContainer( primarySession_, "atm", 63, "merge" );
    createInterfaceContainer( primarySession_, "atm", 17967, "merge" );

    // remove container
    deleteContainer( primarySession_, "interface" );
    
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

