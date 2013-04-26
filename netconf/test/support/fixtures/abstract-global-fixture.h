#ifndef __ABSTRACT_GLOBAL_TEST_FIXTURE__H
#define __ABSTRACT_GLOBAL_TEST_FIXTURE__H

#include "test/support/fixtures/test-context.h"

namespace YumaTest 
{

// ---------------------------------------------------------------------------|
/**
 * This class is a base class for the global etst fixtures which perform all 
 * global initialisation and teardown for the Yuma test harness. 
 * If any errors occur or are reported during initialisation
 * the test is terminated with an assertion.
 */
class AbstractGlobalTestFixture
{
public:
    /** 
     * Constructor. 
     *
     * \param argc the number of arguments
     * \param argv the arguments
     */
    AbstractGlobalTestFixture( int argc, const char** argv );

    /**
     * Destructor. Shutdown the Yuma system test environment.
     */
    virtual ~AbstractGlobalTestFixture();

protected:
    /**
     * Get the target DB configuration type
     *
     * \return the target DB configuration type
     */
    TestContext::TargetDbConfig getTargetDbConfig();

    /**
     * Determine whether the startup capability is in use
     *
     * \return true if the startup capability is in use and false otherwise
     */
    bool usingStartupCapability();

protected:
    int numArgs_;           ///< The number of spoofed command line arguments
    const char** argv_;     ///< the spoofed command line arguments
};

} // namespace YumaTest

#endif // __ABSTRACT_GLOBAL_TEST_FIXTURE__H

