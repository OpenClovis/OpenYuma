#ifndef __YUMA_ABSTRACT_FIXTURE_HELPER_FACTORY_H
#define __YUMA_ABSTRACT_FIXTURE_HELPER_FACTORY_H

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
 * An abstract Fixture Helper Factory.
 */
class AbstractFixtureHelperFactory
{
public:
    /** 
     * Constructor.
     */
    AbstractFixtureHelperFactory()
    {}

    /** Destructor */
    virtual ~AbstractFixtureHelperFactory()
    {}

    /** 
     * Create a new Fixture Helper.
     *
     * \return a new Fixture Helper.
     */
    virtual std::shared_ptr<AbstractFixtureHelper> createHelper() = 0;
};

} // namespace YumaTest

#endif // __YUMA_ABSTRACT_FIXTURE_HELPER_FACTORY_H
