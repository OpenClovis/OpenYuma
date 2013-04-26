#ifndef __XPO3_SUITE_FIXTURE_H
#define __XPO3_SUITE_FIXTURE_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-module-common-fixture.h"
#include "test/support/msg-util/xpo-query-builder.h"
#include "test/support/db-models/device-test-db.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <map>
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractNCSession;
class XPO3Container;

// ---------------------------------------------------------------------------|
//
/// @brief This class is used to perform test case initialisation for testing
/// with the Device Test module.
//
struct DeviceModuleFixture : public DeviceModuleCommonFixture
{
public:
    /// @brief Constructor. 
    DeviceModuleFixture();

    /// @brief Destructor. Shutdown the test.
    ~DeviceModuleFixture();
};

} // namespace YumaTest 

#endif // __XPO3_SUITE_FIXTURE_H
//------------------------------------------------------------------------------
