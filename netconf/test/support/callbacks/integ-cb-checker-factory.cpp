// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/integ-cb-checker-factory.h"
#include "test/support/callbacks/running-cb-checker.h"
#include "test/support/callbacks/candidate-cb-checker.h"

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
IntegCBCheckerFactory::IntegCBCheckerFactory(bool usingCandidate) 
    : usingCandidate_(usingCandidate)
{
}

// ---------------------------------------------------------------------------|
IntegCBCheckerFactory::~IntegCBCheckerFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<CallbackChecker> IntegCBCheckerFactory::createChecker()
{
    if (usingCandidate_) {
        return shared_ptr<CallbackChecker> ( 
                new CandidateCBChecker() );
    }
    else {
        return shared_ptr<CallbackChecker> ( 
                new RunningCBChecker() );
    }
}

} // namespace YumaTest

