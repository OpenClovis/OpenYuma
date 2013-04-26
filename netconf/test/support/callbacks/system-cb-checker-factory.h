#ifndef __YUMA_SYSTEM_CB_CHECKER_FACTORY_H
#define __YUMA_SYSTEM_CB_CHECKER_FACTORY_H

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
 * A session factory for creating System CB Checkers.
 */
class SystemCBCheckerFactory : public AbstractCBCheckerFactory
{
public:
    /** 
     * Constructor.
     */
    explicit SystemCBCheckerFactory();

    /** Destructor */
    virtual ~SystemCBCheckerFactory();

    /** 
     * Create a new CallbackChecker.
     *
     * \return a new CallbackChecker.
     */
    std::shared_ptr<CallbackChecker> createChecker();

};

} // namespace YumaTest

#endif // __YUMA_SYSTEM_CB_CHECKER_FACTORY_H
