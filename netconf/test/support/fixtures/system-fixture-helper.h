#ifndef __SYSTEM_FIXTURE_HELPER_H
#define __SYSTEM_FIXTURE_HELPER_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/abstract-fixture-helper.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Helper class for system fixtures.
 */
class SystemFixtureHelper : public AbstractFixtureHelper
{
public:
    /**
     * Mimic a yuma startup if required by test harness.
     */
    virtual void mimicStartup(); 

    /**
     * Mimic a yuma shutdown if required by test harness.
     */
    virtual void mimicShutdown(); 

    /**
     * Mimic a yuma restart if required by test harness.
     */
    virtual void mimicRestart(); 

};

} // namespace YumaTest

#endif // __SYSTEM_FIXTURE_HELPER_H

