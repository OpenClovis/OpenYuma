// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/integ-test-fixture.h"
#include "test/support/fixtures/test-context.h"
#include "test/support/fixtures/integ-fixture-helper-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-query-util/yuma-op-policies.h"
#include "test/support/nc-session/spoof-nc-session-factory.h"
#include "test/support/callbacks/integ-cb-checker-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cassert>
#include <iostream>
#include <algorithm>
#include <memory>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|
#include "agt.h"
#include "ncx.h"
#include "ncxmod.h"

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
// IntegrationTestFixture
// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
IntegrationTestFixture<SpoofedArgs, OpPolicy>::IntegrationTestFixture()  
    : AbstractGlobalTestFixture( 
            ( sizeof( Args::argv )/sizeof( Args::argv[0] ) ),
            Args::argv )
{
    DisplayTestBreak( '=', true );
    BOOST_TEST_MESSAGE( "Initialising..." );

    // TODO: Ensure that the Traits target and Spoofed Args Targets match

    configureTestContext();

    fixtureHelper_ = TestContext::getTestContext()->
                                     fixtureHelperFactory_->createHelper();
    fixtureHelper_->mimicStartup();
}

// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
IntegrationTestFixture<SpoofedArgs, OpPolicy>::~IntegrationTestFixture()
{
    fixtureHelper_->mimicShutdown();
}

// ---------------------------------------------------------------------------|
template<class SpoofedArgs, class OpPolicy>
void IntegrationTestFixture<SpoofedArgs, OpPolicy>::configureTestContext()
{
   // TODO: Make these strings configurable - via environment
   // TODO: variables / test config file / traits?
   using std::shared_ptr;

   shared_ptr< AbstractYumaOpLogPolicy > queryLogPolicy( 
           new OpPolicy( "./yuma-op" ) );
   assert( queryLogPolicy );

   shared_ptr< AbstractNCSessionFactory > sessionFactory(
           new SpoofNCSessionFactory( queryLogPolicy ) ) ;

   shared_ptr< AbstractCBCheckerFactory > cbCheckerFactory(
           new IntegCBCheckerFactory( getTargetDbConfig()==TestContext::CONFIG_USE_CANDIDATE) ) ;

   shared_ptr< AbstractFixtureHelperFactory > fixtureHelperFactory(
           new IntegFixtureHelperFactory() );

   shared_ptr< TestContext > testContext( 
           new TestContext( true, getTargetDbConfig(), usingStartupCapability(), 
                            numArgs_, argv_, sessionFactory, cbCheckerFactory, 
                            fixtureHelperFactory ) );

   assert( testContext );

   TestContext::setTestContext( testContext );
}

} // namespace YumaTest
