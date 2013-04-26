// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/system-fixture-helper.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void SystemFixtureHelper::mimicStartup() 
{
    // Do nothing as mimicking a startup is not necessary during system tests.
}

// ---------------------------------------------------------------------------|
void SystemFixtureHelper::mimicShutdown() 
{
    // Do nothing as mimicking a shutdown is not necessary during system tests.
}

// ---------------------------------------------------------------------------|
void SystemFixtureHelper::mimicRestart() 
{
    // Do nothing as mimicking a restart is not necessary during system tests.
}

} // namespace YumaTest

