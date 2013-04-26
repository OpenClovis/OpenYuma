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
#include "test/support/fixtures/device-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-query-util/nc-default-operation-config.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/callbacks/sil-callback-log.h"

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

BOOST_FIXTURE_TEST_SUITE( DTDelete, DeviceModuleFixture )

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( Profile )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a Profile ",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, remove profile-2\n"
            "\t 3 - Commit the change and check the configuration content, "
            "profile-2 should not exist\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    xpoProfileQuery( primarySession_, 2, "delete" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( Stream )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a Stream",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, remove profile-2, stream-1\n"
            "\t 3 - Commit the change and check the configuration content, "
            "profile-2, stream-1 should not exist\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    profileStreamQuery( primarySession_, 2, 1, "delete" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( ResourceNode )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a ResourceNode ",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, remove profile-2, stream-1, res-2\n"
            "\t 3 - Commit the change and check the configuration content, "
            "profile-2, stream-1, res-2 should not exist\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    resourceDescriptionQuery( primarySession_, 2, 1, 2, "delete" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( ResourceConnection)
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a ResourceConnection",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, remove profile-2, stream-1, "
            "resCon-2 \n"
            "\t 3 - Commit the change and check the configuration content, "
            "profile-2, stream-1, res-2 should not exist\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    resourceConnectionQuery( primarySession_, 2, 1, 2, "delete" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( NonExistentProfile )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a non-existent Profile",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, try to remove profile-3\n"
            "\t 3 - Check that the operation failed with reason data-missing\n"
            "\t 4 - Commit the change and check the configuration content "
            "is unchanged.\n"
            "\t 5 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    initialise2Profiles();
    xpoProfileQuery( primarySession_, 4, "delete", "data-missing" );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( NonExistentResourceNode1 )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for a non-existent ResourceNode and "
            "default operation of <none> does not accidentally create the "
            "parent hierarchy.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Set the default operation to None\n"
            "\t 3 - Using a <delete> operation, try to remove profile-3, "
            "stream-1 res-1\n"
            "\t 4 - Commit the change and check the configuration content "
            "is unchanged.\n"
            "\t 5 - Check that the operation failed with reason data-missing\n"
            "\t 6 - Check that profile-3, stream-1 were not accidentally created\n"
            "\t 7 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    {
        DefaultOperationConfig defConfig( xpoBuilder_, "none" );
        resourceDescriptionQuery( primarySession_, 3, 1, 1, "delete", "data-missing" );
        commitChanges( primarySession_ );
        checkConfig();
    }

    deleteXpoContainer( primarySession_ );
}

// TODO: Re-enable me once expected behaviour is confirmed!
#if 0
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( NonExistentResourceNode2 )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for a non-existent ResourceNode and "
            "default operation of <merge> does create the parent hierarchy.",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Set the default operation to None\n"
            "\t 3 - Using a <delete> operation, try to remove profile-3, "
            "stream-1 res-1\n"
            "\t 4 - Commit the change and check the configuration content "
            "is unchanged.\n"
            "\t 5 - Check that the operation failed with reason data-missing\n"
            "\t 6 - Check that profile-3, stream-1 were not accidentally created\n"
            "\t 7 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    {
        DefaultOperationConfig defConfig( xpoBuilder_, "merge" );
        resourceDescriptionQuery( primarySession_, 3, 1, 1, "delete", "data-missing" );
        commitChanges( primarySession_ );
        checkConfig();
    }

    deleteXpoContainer( primarySession_ );
}
#endif

// @TODO: Re-enable once Yuma's delete replace functionality works
#if 0
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( ProfileDeleteAndReCreate )
{
    DisplayTestDescrption( 
            "Demonstrate <delete> operation for Deletion of a Profile, "
            "and re-create the same profile before calling <commit>",
            "Procedure: \n"
            "\t 1 - Add 2 full profiles, commit changes and check content\n"
            "\t 2 - Using a <delete> operation, remove profile-2\n"
            "\t 3 - Create a new profile-2 using a <create> operation\n"
            "\t 4 - Commit the change and check the configuration content, "
            "profile-2 should not exist\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );
    initialise2Profiles();

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    xpoProfileQuery( primarySession_, 2, "delete" );

    configureResourceDescrption( primarySession_, 2, 1, 1, 
                                 ResourceNodeConfig{ 106, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-17" ) } );
    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}
#endif

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

