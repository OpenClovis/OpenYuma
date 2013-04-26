#ifndef __YUMA_INTEG_CB_CHECKER_FACTORY_H
#define __YUMA_INTEG_CB_CHECKER_FACTORY_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/abstract-cb-checker-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class CallbackChecker;

// ---------------------------------------------------------------------------|
/**
 * A session factory for creating Integration CB Checkers.
 */
class IntegCBCheckerFactory : public AbstractCBCheckerFactory
{
public:
    /** 
     * Constructor.
     *
     * \param usingCandidate if true indicates that the candidate database is in use.
     */
    explicit IntegCBCheckerFactory(bool usingCandidate);

    /** Destructor */
    virtual ~IntegCBCheckerFactory();

    /** 
     * Create a new CallbackChecker.
     *
     * \return a new CallbackChecker.
     */
    std::shared_ptr<CallbackChecker> createChecker();

private:
    bool usingCandidate_;

};

} // namespace YumaTest

#endif // __YUMA_INTEG_CB_CHECKER_FACTORY_H
