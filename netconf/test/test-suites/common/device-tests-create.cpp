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

BOOST_FIXTURE_TEST_SUITE( DTCreate, DeviceModuleFixture )

// ---------------------------------------------------------------------------|
// Add some data and check that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( FullProfile )
{
    DisplayTestDescrption( 
            "Demonstrate <create> operation for Creation of XPO3 Container "
            "Profile, Stream & Resource entries.",
            "Procedure: \n"
            "\t 1 - Create the  XPO3 container\n"
            "\t 2 - Add profile-1 to the XPO3 container\n"
            "\t 3 - Add Stream-1 to the profile-1\n"
            "\t 4 - Add resourceNode-1 to stream-1\n"
            "\t 5 - Add resourceConnection-1 to stream-1\n"
            "\t 7 - Configure resourceNode-1\n"
            "\t 8 - Configure resourceConnection-1\n"
            "\t 9 - Add StreamConnection-1\n"
            "\t10 - Configure StreamConnection-1\n"
            "\t11 - Set the active profile to profile-1\n"
            "\t12 - Commit the changes \n"
            "\t13 - Query netconf and check that all elements exist in the "
           "netconf database\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
              ResourceNodeConfig{ 100, 
                                  string( "111100001111" ),
                                  boost::optional<string>(),
                                  boost::optional<string>(),
                                  string( "/card[0]/sdiConnector[0]" ) } );
    
    configureResourceConnection( primarySession_, 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );

    configureStreamConnection( primarySession_, 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );

    setActiveProfile( primarySession_, 1 );
    
    // Check callbacks
    vector<string> elements = {};
    cbChecker_->updateContainer("device_test", "xpo", elements, "create");
    elements.push_back("profile");
    cbChecker_->addKey("device_test", "xpo", elements, "id");
    elements.push_back("stream");
    cbChecker_->addKey("device_test", "xpo", elements, "id");
    cbChecker_->addResourceNode("device_test", "xpo", elements, false, false);
    cbChecker_->addResourceCon("device_test", "xpo", elements);
    elements.pop_back();
    cbChecker_->addStreamCon("device_test", "xpo", elements);
    elements.pop_back();
    elements.push_back("activeProfile");
    cbChecker_->addElement("device_test", "xpo", elements);
    cbChecker_->checkCallbacks("device_test");                             
    cbChecker_->resetModuleCallbacks("device_test");
    cbChecker_->resetExpectedCallbacks();

    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

BOOST_AUTO_TEST_CASE( CreateExistingProfile )
{
    DisplayTestDescrption( 
            "Demonstrate <create> operations on an existing node are rejected",
            "Procedure: \n"
            "\t 1 - Create Profile1\n"
            "\t 2 - Create Profile2\n"
            "\t 3 - Create Profile3\n"
            "\t 4 - Commit the changes\n"
            "\t 5 - Check the configuration content\n"
            "\t 6 - Set the default operation to 'none'\n"
            "\t 7 - Attempt to Create Profile2 again\n"
            "\t 8 - Check the operation was rejected with 'data-exists'\n"
            "\t 9 - Set the default operation to 'merge'\n"
            "\t10 - Attempt to Create Profile2 again\n"
            "\t11 - Check the operation was rejected with 'data-exists'\n"
            "\t12 - Commit the changes\n"
            "\t13 - Check the configuration content\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("device_test");

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    xpoProfileQuery( primarySession_, 2 );
    xpoProfileQuery( primarySession_, 3 );
    setActiveProfile( primarySession_, 1 );
    commitChanges( primarySession_ );
    checkConfig();

    {
        DefaultOperationConfig opConfig( xpoBuilder_, "none" );
        xpoProfileQuery( primarySession_, 2, "create", "data-exists" );
    }

    {
        DefaultOperationConfig opConfig( xpoBuilder_, "merge" );
        xpoProfileQuery( primarySession_, 2, "create", "data-exists" );
    }

    commitChanges( primarySession_ );
    checkConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

