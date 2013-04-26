#ifndef __YUMA_SYSTEM_FIXTURE_HELPER_FACTORY_H
#define __YUMA_SYSTEM_FIXTURE_HELPER_FACTORY_H

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
 * A factory for creating System Fixture Helpers.
 */
class SystemFixtureHelperFactory : public AbstractFixtureHelperFactory
{
public:
    /** 
     * Constructor.
     */
    SystemFixtureHelperFactory();

    /** Destructor */
    virtual ~SystemFixtureHelperFactory();

    /** 
     * Create a new Fixture Helper.
     *
     * \return a new Fixture Helper.
     */
    std::shared_ptr<AbstractFixtureHelper> createHelper();

};

} // namespace YumaTest

#endif // __YUMA_SYSTEM_FIXTURE_HELPER_FACTORY_H
