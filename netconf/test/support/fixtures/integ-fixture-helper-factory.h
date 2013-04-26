#ifndef __YUMA_INTEG_FIXTURE_HELPER_FACTORY_H
#define __YUMA_INTEG_FIXTURE_HELPER_FACTORY_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/abstract-fixture-helper-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractFixtureHelper;

// ---------------------------------------------------------------------------|
/**
 * A factory for creating Integration test Fixture Helpers.
 */
class IntegFixtureHelperFactory : public AbstractFixtureHelperFactory
{
public:
    /** 
     * Constructor.
     */
    IntegFixtureHelperFactory();

    /** Destructor */
    virtual ~IntegFixtureHelperFactory();

    /** 
     * Create a new Fixture Helper.
     *
     * \return a new Fixture Helper.
     */
    std::shared_ptr<AbstractFixtureHelper> createHelper();

};

} // namespace YumaTest

#endif // __YUMA_INTEG_FIXTURE_HELPER_FACTORY_H
