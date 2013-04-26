#ifndef __YUMA_ABSTRACT_CB_CHECKER_FACTORY_H
#define __YUMA_ABSTRACT_CB_CHECKER_FACTORY_H

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
 * An abstract CB Checker Factory.
 */
class AbstractCBCheckerFactory
{
public:
    /** 
     * Constructor.
     */
    explicit AbstractCBCheckerFactory()
    {}

    /** Destructor */
    virtual ~AbstractCBCheckerFactory()
    {}

    /** 
     * Create a new CallbackChecker.
     *
     * \return a new CallbackChecker.
     */
    virtual std::shared_ptr<CallbackChecker> createChecker() = 0;
};

} // namespace YumaTest

#endif // __YUMA_ABSTRACT_CB_CHECKER_FACTORY_H
