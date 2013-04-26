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
#include "test/support/fixtures/device-get-module-fixture.h"
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
namespace ph = boost::phoenix;
namespace ph_args = boost::phoenix::arg_names;
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( DTGet, DeviceGetModuleFixture )

// ---------------------------------------------------------------------------|
// Create content and check filtered config data can be retrieved correctly
// using xpath filter
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetConfigWithXpathFilterSuccess  )
{
    DisplayTestDescrption( 
            "Demonstrate successful <get-config> with xpath filter operations "
            "on XPO3 Container.",
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
            "\t13 - Query netconf for filtered config data with valid xpath and "
           "verify response\n"
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
                                  string( "a configuration"),
                                  string( "a status configuration"),
                                  string( "an alarm configuration"),
                                  string( "/card[0]/sdiConnector[0]") } );
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
    cbChecker_->addResourceNode("device_test", "xpo", elements, true, true);
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

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request the physical path of a resource node");
    configureResourceDescrptionInFilteredConfig(1, 1, 1, 
            ResourceNodeConfig{ boost::optional<uint32_t>(),
                                boost::optional<std::string>(),
                                boost::optional<std::string>(),
                                boost::optional<std::string>(),
                                string( "/card[0]/sdiConnector[0]" ) } );
    checkXpathFilteredConfig("xpo/profile[id='1']/stream[id='1']/resourceNode[id='1']/physicalPath");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request the resource type of a resource node");
    configureResourceDescrptionInFilteredConfig(1, 1, 1, 
            ResourceNodeConfig{ 100,
                                boost::optional<std::string>(),
                                boost::optional<std::string>(),
                                boost::optional<std::string>(),
                                boost::optional<std::string>() } );
    checkXpathFilteredConfig("xpo/profile[id='1']/stream[id='1']/resourceNode[id='1']/resourceType");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request the destination id of a resource connection");
    configureResourceConnectionInFilteredConfig( 1, 1, 1, 
             ConnectionItemConfig{ boost::optional<uint32_t>(),
                                   boost::optional<uint32_t>(),
                                   300,
                                   boost::optional<uint32_t>(),
                                   boost::optional<uint32_t>() } );
    checkXpathFilteredConfig("xpo/profile[id='1']/stream[id='1']/resourceConnection[id='1']/destinationId");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request a complete resource connection");
    configureResourceConnectionInFilteredConfig( 1, 1, 1, 
             ConnectionItemConfig{ 100,
                                   200,
                                   300,
                                   400,
                                   500 } );
    checkXpathFilteredConfig("xpo/profile[id='1']/stream[id='1']/resourceConnection[id='1']");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request the source stream id of a stream connection");
    configureStreamConnectionInFilteredConfig( 1, 1, 
             StreamConnectionItemConfig{ boost::optional<uint32_t>(),
                                         boost::optional<uint32_t>(),
                                         boost::optional<uint32_t>(),
                                         boost::optional<uint32_t>(),
                                         boost::optional<uint32_t>(),
                                         600,
                                         boost::optional<uint32_t>() } );
    checkXpathFilteredConfig("xpo/profile[id='1']/streamConnection[id='1']/sourceStreamId");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request a complete stream connection");
    configureStreamConnectionInFilteredConfig( 1, 1, 
             StreamConnectionItemConfig{ 100,
                                         200,
                                         300,
                                         400,
                                         500,
                                         600,
                                         700 } );
    checkXpathFilteredConfig("xpo/profile[id='1']/streamConnection[id='1']");
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request a complete stream");
    configureResourceDescrptionInFilteredConfig(1, 1, 1, 
            ResourceNodeConfig{ 100,
                                string( "a configuration"),
                                string( "a status configuration"),
                                string( "an alarm configuration"),
                                string( "/card[0]/sdiConnector[0]") } );
    configureResourceConnectionInFilteredConfig( 1, 1, 1, 
             ConnectionItemConfig{ 100,
                                   200,
                                   300,
                                   400,
                                   500 } );
    checkXpathFilteredConfig("xpo/profile[id='1']/stream[id='1']");
    clearFilteredConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check <get-config> with malformed xpath filter rejected
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetConfigWithXpathFilterFailure )
{
    DisplayTestDescrption( 
            "Demonstrate <get-config> with malformed xpath filter rejected.",
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
            "\t13 - Query netconf for filtered config data with invalid xpath "
           "and verify response\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
              ResourceNodeConfig{ 100,
                                  string( "a configuration"),
                                  string( "a status configuration"),
                                  string( "an alarm configuration"),
                                  string( "/card[0]/sdiConnector[0]") } );
    configureResourceConnection( primarySession_, 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );

    configureStreamConnection( primarySession_, 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );

    setActiveProfile( primarySession_, 1 );
    commitChanges( primarySession_ );
    checkConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with xpath filter: request includes invalid xpath expression");
    vector<string> expPresent{ "error", "rpc-error", "invalid XPath expression syntax" };
    vector<string> expNotPresent{ "ok" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigXpath( primarySession_, "%%>", 
            writeableDbName_, checker );

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check filtered config data can be retrieved correctly
// using subtree filter
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetConfigWithSubtreeFilterSuccess  )
{
    DisplayTestDescrption( 
            "Demonstrate successful <get-config> with subtree filter operations "
            "on XPO3 Container.",
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
            "\t13 - Query netconf for filtered config data with valid subtree and "
           "verify response\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
              ResourceNodeConfig{ 100,  string( "/card[0]/sdiConnector[0]" ) } );
    configureResourceConnection( primarySession_, 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );
    configureStreamConnection( primarySession_, 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );
    setActiveProfile( primarySession_, 1 );
    commitChanges( primarySession_ );
    checkConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with subtree filter: select the entire namespace");
    ensureActiveProfileInFilteredConfig(1);
    configureResourceDescrptionInFilteredConfig( 1, 1, 1, 
            ResourceNodeConfig{ 100,  string( "/card[0]/sdiConnector[0]" ) } );
    configureResourceConnectionInFilteredConfig( 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );
    configureStreamConnectionInFilteredConfig( 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );

    stringstream filter;
    filter << "      <xpo xmlns=\"http://netconfcentral.org/ns/device_test\"/>";
    checkSubtreeFilteredConfig( filter.str() );
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with subtree filter: empty filter");
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "xpo" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigSubtree( primarySession_, "", 
            writeableDbName_, checker );

    //TODO Test attribute matching expressions

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with subtree filter: complex content match");
    configureResourceDescrptionInFilteredConfig( 1, 1, 1, 
            ResourceNodeConfig{ 100,  string( "/card[0]/sdiConnector[0]" ) } );
    configureStreamConnectionInFilteredConfig( 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );

    filter.str("");
    filter << "      <xpo xmlns=\"http://netconfcentral.org/ns/device_test\">\n"
           << "        <profile>\n"
           << "          <id>1</id>\n"
           << "          <stream>\n"
           << "            <id>1</id>\n"
           << "            <resourceNode>\n"
           << "              <id>1</id>\n"
           << "            </resourceNode>\n"
           << "          </stream>\n"
           << "          <streamConnection>\n"
           << "            <id>1</id>\n"
           << "          </streamConnection>\n"
           << "        </profile>\n"
           << "      </xpo>";
    checkSubtreeFilteredConfig( filter.str() );
    clearFilteredConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-config> with subtree filter: request the resource type of a resource node");
    configureResourceDescrptionInFilteredConfig(1, 1, 1, 
            ResourceNodeConfig{ 100,  boost::optional<std::string>() } );


    filter.str("");
    filter << "      <xpo xmlns=\"http://netconfcentral.org/ns/device_test\">\n"
           << "        <profile>\n"
           << "          <id>1</id>\n"
           << "          <stream>\n"
           << "            <id>1</id>\n"
           << "            <resourceNode>\n"
           << "              <id>1</id>\n"
           << "              <resourceType/>\n"
           << "            </resourceNode>\n"
           << "          </stream>\n"
           << "        </profile>\n"
           << "      </xpo>";
    checkSubtreeFilteredConfig( filter.str() );
    clearFilteredConfig();

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check <get-config> with malformed subtree filter
// rejected
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetConfigWithSubtreeFilterFailure  )
{
    DisplayTestDescrption( 
            "Demonstrate <get-config> with malformed subtree filter rejected.",
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
            "\t13 - Query netconf for filtered config data with invalid subtree "
           "and verify response\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
              ResourceNodeConfig{ 100,  string( "/card[0]/sdiConnector[0]" ) } );
    configureResourceConnection( primarySession_, 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );
    configureStreamConnection( primarySession_, 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );
    setActiveProfile( primarySession_, 1 );
    commitChanges( primarySession_ );
    checkConfig();

    // Response to malformed subtree expression is the same as empty subtree,
    // not sure if this is correct
    BOOST_TEST_MESSAGE("Test: Checking <get-config> with subtree filter: malformed subtree expression");
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "xpo" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigSubtree( primarySession_, "%~", 
            writeableDbName_, checker );

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Create content and check <get-schema>
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( GetSchema )
{
    DisplayTestDescrption( 
            "Demonstrate <get-schema>.",
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
            "\t13 - Query netconf for the device_test schema\n"
            "\t14 - Flush the contents so the container is empty for the next test\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createXpoContainer( primarySession_ );
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
              ResourceNodeConfig{ 100,
                                  string( "a configuration"),
                                  string( "a status configuration"),
                                  string( "an alarm configuration"),
                                  string( "/card[0]/sdiConnector[0]") } );
    configureResourceConnection( primarySession_, 1, 1, 1, 
             ConnectionItemConfig{ 100, 200, 300, 400, 500 } );

    configureStreamConnection( primarySession_, 1, 1, 
             StreamConnectionItemConfig{ 100, 200, 300, 400, 500, 600, 700 } );

    setActiveProfile( primarySession_, 1 );
    commitChanges( primarySession_ );
    checkConfig();

    BOOST_TEST_MESSAGE("Test: Checking <get-schema>");
    vector<string> expPresent{ "namespace \"http://netconfcentral.org/ns/device_test\"",
                               "Start of main configuration block" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetSchema( primarySession_, "device_test", checker );

    deleteXpoContainer( primarySession_ );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

