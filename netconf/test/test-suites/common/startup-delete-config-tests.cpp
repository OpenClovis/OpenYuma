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

BOOST_FIXTURE_TEST_SUITE( startup_slt_tests, SimpleContainerModuleFixture )

// ---------------------------------------------------------------------------|
// Add some data and check that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_startup_delete_config )
{
    DisplayTestDescrption( 
            "Demonstrate delete-config operation on startup config...",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list with 3 key-value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Commit the operation\n"
            "\t     (performing a copy-config if startup capability is in use)\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Perform delete-config operation\n"
            "\t 9 - Check all values are still in the running\n"
            "\t10 - Check all values are still in the candidate\n"
            "\t11 - Perform restart operation\n"
            "\t12 - Check no values are loaded to the running\n"
            "\t13 - Check no values are loaded to the candidate\n"
            );

    {
        // RAII Vector of database locks 
        vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

        createMainContainer( primarySession_ );

        //Reset logged callbacks
        cbChecker_->resetModuleCallbacks("simple_list_test");
        cbChecker_->resetExpectedCallbacks();     

        // set some values
        populateDatabase( 3 );

        // check the entries exist
        checkEntries( primarySession_ );

        // commit the changes
        commitChanges( primarySession_ );

        // check the entries exist
        checkEntries( primarySession_ );

        // delete startup config
        runDeleteStartupConfig( primarySession_ );

        // check the entries exist
        checkEntries( primarySession_ );
    }

    // restart
    runRestart( primarySession_ );

    // get a secondary session
    std::shared_ptr<AbstractNCSession> secondarySession( sessionFactory_->createSession() );

    {
        // RAII Vector of database locks 
        vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( secondarySession );
        
        // simulate changes lost on restart
        rollbackChanges( secondarySession );

        // check the entries have not been loaded on startup
        checkEntries( secondarySession );
    }
    
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

