// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/base-suite-fixture.h"

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/misc-util/log-utils.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
BaseSuiteFixture::BaseSuiteFixture() 
    : testContext_( TestContext::getTestContext() )
    , writeableDbName_( testContext_->writeableDbName_ )
{
}

// ---------------------------------------------------------------------------|
BaseSuiteFixture::~BaseSuiteFixture() 
{
    DisplayCurrentTestResult();
}

} // namespace YumaTest
