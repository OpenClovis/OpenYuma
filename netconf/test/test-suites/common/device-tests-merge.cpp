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

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( DTMerge, DeviceModuleFixture )

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( SimpleNewItems )
{
    DisplayTestDescrption( 
            "Demonstrate <merge> for creation of Simple New Items.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a merge operation, add profile-3\n"
            "\t 3 - Commit the change and check the configuration content\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");
    // TODO: Add callback checking....

    initialise2Profiles();
    xpoProfileQuery( primarySession_, 3, "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( ComplexNewItems )
{
    DisplayTestDescrption( 
            "Demonstrate <merge> for creation of Complex New Items.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a merge operation, add profile-3, stream-1, resDesc-2\n"
            "\t 3 - Commit the change and check the configuration content\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");
    // TODO: Add callback checking....

    initialise2Profiles();
    configureResourceDescrption( primarySession_, 3, 1, 1, 
              ResourceNodeConfig{ 100,  
                                  boost::optional<string>(),
                                  boost::optional<string>(),
                                  boost::optional<string>(),
                                  string( "/card[0]/sdiConnector[0]" ) },
              "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( SimpleExistingItem )
{
    DisplayTestDescrption( 
            "Demonstrate <merge> for creation / update of Simple Existing Items.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a merge operation, add profile-3\n"
            "\t 3 - Using a merge operation, add profile-3 again\n"
            "\t 4 - Commit the change and check the configuration content\n"
            "\t 5 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");
    // TODO: Add callback checking....

    initialise2Profiles();
    xpoProfileQuery( primarySession_, 3, "merge" );
    xpoProfileQuery( primarySession_, 3, "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( ComplexExistingItems )
{
    DisplayTestDescrption( 
            "Demonstrate <merge> for creation / update of Complex Existing Items.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a merge operation, add profile-3, stream-1, resDesc-2\n"
            "\t 3 - Commit the change and check the configuration content\n"
            "\t 4 - Using a merge operation, modify profile-3, stream-1, resDesc-2, "
            "with some values changed\n"
            "\t 5 - Commit the change and check the configuration content\n"
            "\t 6 - Using a merge operation, modify profile-3, stream-1, resDesc-2, "
            "with no values changed\n"
            "\t 7 - Commit the change and check the configuration content\n"
            "\t 8 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");
    // TODO: Add callback checking....

    initialise2Profiles();
    configureResourceDescrption( primarySession_, 3, 1, 1, 
              ResourceNodeConfig{ 100,  
                                  boost::optional<string>(),
                                  boost::optional<string>(),
                                  boost::optional<string>(),
                                  string( "/card[0]/sdiConnector[0]" ) },
              "merge" );
    configureResourceConnection( primarySession_, 3, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, boost::optional<uint32_t>(),
                                   boost::optional<uint32_t>() }, 
             "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    configureResourceConnection( primarySession_, 3, 1, 1, 
             ConnectionItemConfig{ boost::optional<uint32_t>(),
                                   boost::optional<uint32_t>(),
                                   311, 400, 500 }, 
             "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    configureResourceConnection( primarySession_, 3, 1, 1, 
             ConnectionItemConfig{ 100,
                                   200,
                                   311, 400, 500 }, 
             "merge" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

