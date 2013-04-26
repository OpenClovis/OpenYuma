// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/system-cb-checker-factory.h"
#include "test/support/callbacks/system-cb-checker.h"

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
SystemCBCheckerFactory::SystemCBCheckerFactory() 
{
}

// ---------------------------------------------------------------------------|
SystemCBCheckerFactory::~SystemCBCheckerFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<CallbackChecker> SystemCBCheckerFactory::createChecker()
{
    return shared_ptr<CallbackChecker> ( 
            new SystemCBChecker() );
}

} // namespace YumaTest

