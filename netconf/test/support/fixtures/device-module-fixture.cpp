// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-module-fixture.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>
#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/checkers/xml-content-checker.h" 
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|

// @TODO This suite currently only supports 'happy days' scenarios.
// @TODO There is no support for many failures sceanrios such as:
// @TODO    An attempt to  create a streamItem for a profile that does 
// @TODO    not exist.
// @TODO Support for these scenarios will be required!
// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
DeviceModuleFixture::DeviceModuleFixture() 
    : DeviceModuleCommonFixture( "http://netconfcentral.org/ns/device_test" ) 
{
    // ensure the module is loaded
    queryEngine_->loadModule( primarySession_, "device_test" );
}

// ---------------------------------------------------------------------------|
DeviceModuleFixture::~DeviceModuleFixture() 
{
}

} // namespace XPO3NetconfIntegrationTest

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
