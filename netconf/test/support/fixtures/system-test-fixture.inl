// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/system-test-fixture.h"
#include "test/support/fixtures/test-context.h"
#include "test/support/fixtures/system-fixture-helper-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-query-util/yuma-op-policies.h"
#include "test/support/nc-session/remote-nc-session-factory.h"
#include "test/support/callbacks/system-cb-checker-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cassert>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cstdlib>

// ---------------------------------------------------------------------------|
// Boost Includes
// ---------------------------------------------------------------------------|
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace 
{
// ---------------------------------------------------------------------------|
/**
 * Get an environment variable. If the variable is mandatory and it is
 * not set this function calls assert.
 *
 * The system test harness uses the following environment variables to 
 * determine the configuration for testing:
 *
 * <ol>
 *   <li>$YUMA_AGENT_IPADDR - The ip address of the remote Yuma Agent.</li>
 *   <li>$YUMA_AGENT_USER - The user name to use. </li>
 *   <li>$YUMA_AGENT_PASSWORD - The user's password. <li>
 *   <li>$YUMA_AGENT_PORT - The port used by the Yuma Agent, (defaults: 830).</li>
 * </ol>
 *
 * \param key the name of the variable to retieve.
 * \param mandatory flag indicating if the variable is mandatory
 * \return the value for the environnment variable.
 */
char* GetEnvVar( const string& key, const bool mandatory = false )
{
    char* val = getenv( key.c_str() );
    if ( mandatory && !val )
    {
        assert( val && "Environment Not Configured!" );
    }
    return val;
}

}


// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
// SystemTestFixture
// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
SystemTestFixture<SpoofedArgs, OpPolicy>::SystemTestFixture()  
    : AbstractGlobalTestFixture( 
            ( sizeof( Args::argv )/sizeof( Args::argv[0] ) ),
            Args::argv )
{
    DisplayTestBreak( '=', true );
    BOOST_TEST_MESSAGE( "Initialising..." );

    configureTestContext();
}

// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
SystemTestFixture<SpoofedArgs, OpPolicy>::~SystemTestFixture()
{
    BOOST_TEST_MESSAGE( "SystemTestFixture() Cleaning Up..." );
}

// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
void SystemTestFixture<SpoofedArgs, OpPolicy>::configureTestContext()
{
    using std::shared_ptr;
    using boost::unit_test::framework::master_test_suite;

    cout << "command line arguments: " << master_test_suite().argc << "\n";

    int argc = master_test_suite().argc;
    char** argv = master_test_suite().argv;

    for( int idx = 0; idx < argc; ++idx )
    {
        cout << "\t" << argv[idx] << "\n";
    }

    shared_ptr< AbstractYumaOpLogPolicy > queryLogPolicy( 
            new OpPolicy( "./yuma-op" ) );
    assert( queryLogPolicy );

    string ipAddr( GetEnvVar( "YUMA_AGENT_IPADDR", true ) );
    string user( GetEnvVar( "YUMA_AGENT_USER", true ) );
    string password( GetEnvVar( "YUMA_AGENT_PASSWORD", true ) );
    char* val = GetEnvVar( "YUMA_AGENT_PORT", false );
    uint16_t ipPort = val ? boost::lexical_cast<uint16_t>( val ) : 830;

   shared_ptr< AbstractNCSessionFactory > sessionFactory(
           new RemoteNCSessionFactory( ipAddr, ipPort, user, password,
                                       queryLogPolicy ) ) ;

   shared_ptr< AbstractCBCheckerFactory > cbCheckerFactory(
           new SystemCBCheckerFactory() );

   shared_ptr< AbstractFixtureHelperFactory > fixtureHelperFactory(
           new SystemFixtureHelperFactory() );

   shared_ptr< TestContext > testContext( 
           new TestContext( false, getTargetDbConfig(), usingStartupCapability(), 
                            numArgs_, argv_,
                            sessionFactory, cbCheckerFactory, fixtureHelperFactory ) );

   assert( testContext );

   TestContext::setTestContext( testContext );
}

} // namespace YumaTest
