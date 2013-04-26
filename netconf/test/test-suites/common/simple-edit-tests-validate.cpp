// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

// ---------------------------------------------------------------------------|
// Boost Includes
// ---------------------------------------------------------------------------|
#include <boost/range/algorithm.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/fusion/at.hpp> 
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp> 

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
namespace ph = boost::phoenix;
namespace ph_args = boost::phoenix::arg_names;
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( validate_slt_tests, SimpleContainerModuleFixture )

// ---------------------------------------------------------------------------|
// Add some data and check that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_validate )
{
    DisplayTestDescrption( 
            "Demonstrate validation of a simple container.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the database with 2 key value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Populate the database with another 2 key value pairs\n"
            "\t 9 - Check new values are in the candidate\n"
            "\t10 - Check new values are not in the running\n"
            "\t11 - Run a validate command\n"
            "\t12 - Check that validate callbacks are logged for each element\n"
            "\t13 - Delete all entries from the candidate\n"
            "\t14 - Commit the operation\n"
            "\t15 - Check all values are not in the candidate\n"
            "\t16 - Check all values are not in the running\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 2 );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->addMainContainer("simple_list_test", "simple_list");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    // commit the changes
    commitChanges( primarySession_ );

    // Check callbacks
    cbChecker_->commitKeyValuePairs("simple_list_test", "simple_list", elements, "theKey", "theVal", 2);
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    // Set some more values
    addEntryValuePair( primarySession_, "entryKey2", "entryVal2" );
    addEntryValuePair( primarySession_, "entryKey3", "entryVal3" );

    // Check callbacks
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();
    
    // check the entries exist
    checkEntries( primarySession_ );

    // Validate
    runValidateCommand( primarySession_ );

    // Check callbacks
    cbChecker_->addMainContainer("simple_list_test", "simple_list");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

