#include "test/support/fixtures/test-context.h"

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
// initialise static data
std::shared_ptr<TestContext> TestContext::testContext_;

// ---------------------------------------------------------------------------|
TestContext::TestContext( 
        bool isIntegTest,
        TargetDbConfig targetDbConfig,
        bool usingStartup,
        int argc,
        const char** argv,
        std::shared_ptr<AbstractNCSessionFactory> sessionFactory,
        std::shared_ptr<AbstractCBCheckerFactory> cbCheckerFactory,
        std::shared_ptr<AbstractFixtureHelperFactory> fixtureHelperFactory )
    : isIntegTest_( isIntegTest )
    , targetDbConfig_( targetDbConfig )
    , usingStartup_( usingStartup )
    , writeableDbName_( 
            targetDbConfig == TestContext::CONFIG_WRITEABLE_RUNNNIG ?
            "running" : "candidate" )
    , numArgs_( argc )
    , argv_( argv ) 
    , sessionFactory_( sessionFactory )
    , cbCheckerFactory_( cbCheckerFactory )
    , fixtureHelperFactory_( fixtureHelperFactory )
{
}

} // namespace YumaTest
