// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/test-context.h"
#include "test/support/fixtures/integ-fixture-helper.h"
#include "test/support/callbacks/sil-callback-controller.h"

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
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::mimicStartup() 
{
    int numArgs = TestContext::getTestContext()->numArgs_;
    const char** argv = TestContext::getTestContext()->argv_;

    initialiseNCXEngine(numArgs, argv);
    loadBaseSchemas();
    stage1AgtInitialisation(numArgs, argv);
    loadCoreSchema();
    stage2AgtInitialisation();
    // Make sure callback controller still exists following restart
    YumaTest::SILCallbackController& cbCon = 
                               YumaTest::SILCallbackController::getInstance();
    cbCon.reset();
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::mimicShutdown() 
{
    BOOST_TEST_MESSAGE( "QuerySuiteFixture() Cleaning Up..." );
    agt_cleanup();
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::mimicRestart() 
{
    mimicShutdown();
    mimicStartup();
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::initialiseNCXEngine(int numArgs, const char** argv)
{
    BOOST_TEST_MESSAGE( "Initialising the NCX Engine..." );
    assert( "NCX Engine Failed to initialise" &&
            NO_ERR == ncx_init( FALSE, LOG_DEBUG_INFO, FALSE, 0, numArgs, 
                                const_cast<char**>( argv ) ) ); 
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::loadBaseSchemas()
{
    BOOST_TEST_MESSAGE( "Loading base schema..." );
    assert( "ncxmod_load_module( NCXMOD_YUMA_NETCONF ) failed!" &&
            NO_ERR == ncxmod_load_module( NCXMOD_YUMA_NETCONF, NULL, NULL, NULL ) );

    assert( "ncx_modload_module( NCXMOD_NETCONFD ) failed!" &&
            NO_ERR == ncxmod_load_module( NCXMOD_NETCONFD, NULL, NULL, NULL ) );
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::loadCoreSchema()
{
    BOOST_TEST_MESSAGE( "Loading core schemas..." );
    agt_profile_t* profile = agt_get_profile();
    assert ( "agt_get_profile() returned a null profile" && profile );
    assert( "ncxmod_load_module( NCXMOD_WITH_DEFAULTS ) failed!" &&
            NO_ERR == ncxmod_load_module( NCXMOD_YUMA_NETCONF, NULL, 
                                          &profile->agt_savedevQ, NULL ) );

    assert( "ncx_modload_module( NCXMOD_NETCONFD failed!" &&
            NO_ERR == ncxmod_load_module( NCXMOD_NETCONFD, NULL, NULL, NULL ) );
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::stage1AgtInitialisation(int numArgs, const char** argv)
{
    BOOST_TEST_MESSAGE( "AGT Initialisation stage 1..." );
    boolean showver = FALSE;
    help_mode_t showhelpmode = HELP_MODE_NONE;

    assert( "agt_init1() failed!" &&
            NO_ERR == agt_init1( numArgs, const_cast<char**>( argv ),
                                 &showver, &showhelpmode ) );  
}

// ---------------------------------------------------------------------------|
void IntegFixtureHelper::stage2AgtInitialisation()
{
    BOOST_TEST_MESSAGE( "AGT Initialisation stage 2..." );

    assert( "agt_init2() failed!" && NO_ERR == agt_init2() );
}

} // namespace YumaTest

