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
#include "test/support/fixtures/device-module-fixture.h"
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

BOOST_FIXTURE_TEST_SUITE( DTMisc, DeviceModuleFixture )

// ---------------------------------------------------------------------------|
// Simple get of database data
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( get_dt_entries_using_xpath_filter )
{
    DisplayTestDescrption( 
            "Demonstrate retrieval of a module's entries.",
            "Procedure: \n"
            "\t1 - Query the entries for the 'device_test' module "
            "using an xpath filter\n"
            "\t1 - Check that the query returned without error." 
            );
   
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "error", "rpc-error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigXpath( primarySession_, "device_test", 
            "running", checker );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest


