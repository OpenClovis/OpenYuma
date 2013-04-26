#ifndef __ABSTRACT_FIXTURE_HELPER_H
#define __ABSTRACT_FIXTURE_HELPER_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Base class for Fixture Helpers.
 */
class AbstractFixtureHelper
{
public:
    /** 
     * Constructor. 
     */
    AbstractFixtureHelper()
    {
    }
    
    /**
     * Destructor.
     */
    virtual ~AbstractFixtureHelper()
    {
    }

    /**
     * Mimic a yuma startup if required by test harness.
     */
    virtual void mimicStartup() = 0; 

    /**
     * Mimic a yuma shutdown if required by test harness.
     */
    virtual void mimicShutdown() = 0; 

    /**
     * Mimic a yuma restart if required by test harness.
     */
    virtual void mimicRestart() = 0; 

};

} // namespace YumaTest

#endif // __ABSTRACT_FIXTURE_HELPER_H

