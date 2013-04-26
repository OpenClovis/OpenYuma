// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/system-fixture-helper-factory.h"
#include "test/support/fixtures/system-fixture-helper.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>

// ---------------------------------------------------------------------------|
// File wide namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
SystemFixtureHelperFactory::SystemFixtureHelperFactory() 
{
}

// ---------------------------------------------------------------------------|
SystemFixtureHelperFactory::~SystemFixtureHelperFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<AbstractFixtureHelper> SystemFixtureHelperFactory::createHelper()
{
    return shared_ptr<AbstractFixtureHelper> ( 
            new SystemFixtureHelper() );
}

} // namespace YumaTest

