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

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/state-get-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-query-util/nc-default-operation-config.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/callbacks/sil-callback-log.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( SDGet, StateGetModuleFixture )

// ---------------------------------------------------------------------------|
// Create content and check filtered state data can be retrieved correctly
// using xpath filter
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetStateDataWithXpathFilterSuccess  )
{
    DisplayTestDescrption( 
            "Demonstrate successful <get> with xpath filter operations "
            "on toaster Container.",
            "Procedure: \n"
            "\t 1 - Create the  Toaster container\n"
            "\t 2 - Commit the changes \n"
            "\t 3 - Query netconf for filtered state data with valid xpath and "
           "verify response\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("state_data_test");

    // TODO: Add callback checking....

    createToasterContainer( primarySession_ );
    configureToaster( 
            ToasterContainerConfig{ string( "Acme, Inc." ), string( "Super Toastamatic 2000" ), string( "up" ) } );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("Test: Checking state data in database");
    checkState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with filter: request toasterManufacturer");
    configureToasterInFilteredState( 
            ToasterContainerConfig{ string( "Acme, Inc." ), boost::optional<std::string>(), boost::optional<std::string>() } );
    checkXpathFilteredState("/toaster/toasterManufacturer");
    clearFilteredState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with filter: request toasterModelNumber");
    configureToasterInFilteredState( 
            ToasterContainerConfig{ boost::optional<std::string>(), string( "Super Toastamatic 2000" ), boost::optional<std::string>() } );
    checkXpathFilteredState("/toaster/toasterModelNumber");
    clearFilteredState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with filter: request toasterStatus");
    configureToasterInFilteredState( 
            ToasterContainerConfig{ boost::optional<std::string>(), boost::optional<std::string>(), string( "up" ) } );
    checkXpathFilteredState("/toaster/toasterStatus");
    clearFilteredState();

    deleteToasterContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check <get> with malformed xpath filter rejected
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetStateDataWithXpathFilterFailure )
{
    DisplayTestDescrption( 
            "Demonstrate <get> with malformed xpath filter rejected.",
            "Procedure: \n"
            "\t 1 - Create the  Toaster container\n"
            "\t 2 - Commit the changes \n"
            "\t 3 - Query netconf for filtered state data with invalid xpath "
           "and verify response\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("state_data_test");

    // TODO: Add callback checking....

    createToasterContainer( primarySession_ );
    configureToaster( 
            ToasterContainerConfig{ string( "Acme, Inc." ), string( "Super Toastamatic 2000" ), string( "up" ) } );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("Test: Checking state data in database");
    checkState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with xpath filter: request includes invalid xpath expression");
    vector<string> expPresent{ "error", "rpc-error", "invalid XPath expression syntax" };
    vector<string> expNotPresent{ "ok" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetXpath( primarySession_, "%%>", 
            writeableDbName_, checker );

    deleteToasterContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check filtered state data can be retrieved correctly
// using subtree filter
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetStateDataWithSubtreeFilterSuccess  )
{
    DisplayTestDescrption( 
            "Demonstrate successful <get> with subtree filter operations "
            "on toaster Container.",
            "Procedure: \n"
            "\t 1 - Create the  Toaster container\n"
            "\t 2 - Commit the changes \n"
            "\t 3 - Query netconf for filtered state data with valid subtree and "
           "verify response\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("state_data_test");

    // TODO: Add callback checking....

    createToasterContainer( primarySession_ );
    configureToaster( 
            ToasterContainerConfig{ string( "Acme, Inc." ), string( "Super Toastamatic 2000" ), string( "up" ) } );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("Test: Checking state data in database");
    checkState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with subtree filter: select the entire namespace");
    configureToasterInFilteredState( 
            ToasterContainerConfig{ string( "Acme, Inc." ), string( "Super Toastamatic 2000" ), string( "up" ) } );

    stringstream filter;
    filter << "      <toaster xmlns=\"http://netconfcentral.org/ns/toaster\"/>";
    checkSubtreeFilteredState( filter.str() );
    clearFilteredState();

    BOOST_TEST_MESSAGE("Test: Checking <get> with subtree filter: empty filter");
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "toaster" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetSubtree( primarySession_, "", 
            writeableDbName_, checker );

    //TODO Test attribute matching expressions

    BOOST_TEST_MESSAGE("Test: Checking <get> with subtree filter: complex content match");
    configureToasterInFilteredState( 
            ToasterContainerConfig{ string( "Acme, Inc." ), boost::optional<std::string>(), string( "up" ) } );

    filter.str("");
    filter << "      <toaster xmlns=\"http://netconfcentral.org/ns/toaster\">\n"
           << "        <toasterManufacturer/>\n"
           << "        <toasterStatus/>\n"
           << "      </toaster>";
    checkSubtreeFilteredState( filter.str() );
    clearFilteredState();

    deleteToasterContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check <get> with malformed subtree filter
// rejected
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetStateDataWithSubtreeFilterFailure  )
{
    DisplayTestDescrption( 
            "Demonstrate <get> with malformed subtree filter rejected.",
            "Procedure: \n"
            "\t 1 - Create the  Toaster container\n"
            "\t 2 - Commit the changes \n"
            "\t 3 - Query netconf for filtered state data with invalid subtree "
           "and verify response\n"
            "\t 4 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // Reset logged callbacks
    cbChecker_->resetExpectedCallbacks();
    cbChecker_->resetModuleCallbacks("state_data_test");

    // TODO: Add callback checking....

    createToasterContainer( primarySession_ );
    configureToaster( 
            ToasterContainerConfig{ string( "Acme, Inc." ), string( "Super Toastamatic 2000" ), string( "up" ) } );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("Test: Checking state data in database");
    checkState();

    // Response to malformed subtree expression is the same as empty subtree,
    // not sure if this is correct
    BOOST_TEST_MESSAGE("Test: Checking <get> with subtree filter: malformed subtree expression");
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "toaster" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetSubtree( primarySession_, "%~", 
            writeableDbName_, checker );

    deleteToasterContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

