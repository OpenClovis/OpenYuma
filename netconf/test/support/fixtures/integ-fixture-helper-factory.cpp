// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/integ-fixture-helper-factory.h"
#include "test/support/fixtures/integ-fixture-helper.h"

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
IntegFixtureHelperFactory::IntegFixtureHelperFactory() 
{
}

// ---------------------------------------------------------------------------|
IntegFixtureHelperFactory::~IntegFixtureHelperFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<AbstractFixtureHelper> IntegFixtureHelperFactory::createHelper()
{
    return shared_ptr<AbstractFixtureHelper> ( 
            new IntegFixtureHelper() );
}

} // namespace YumaTest

