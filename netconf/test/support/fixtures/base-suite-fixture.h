#ifndef __YUMA_BASE_SUITE_FIXTURE__H
#define __YUMA_BASE_SUITE_FIXTURE__H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/test-context.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
/**
 * This class is used to perform simple test case initialisation.
 * It can be used on a per test case basis or on a per test suite
 * basis. This fixture provides functionality common to all tests.
 */
class BaseSuiteFixture
{
public:
    /** 
     * Constructor. 
     */
    BaseSuiteFixture();
    
    /**
     * Destructor. Shutdown the test.
     */
    virtual ~BaseSuiteFixture();

    /** 
     * Utility to check if the candidate configuration is in use.
     *
     * \return true if the candidate configuration is in use.
     */
    bool useCandidate() const
    {
        return ( TestContext::CONFIG_USE_CANDIDATE == 
                 testContext_->targetDbConfig_ );
    }

    /** 
     * Utility to check if the startup capability is in use.
     *
     * \return true if the startup capability is in use.
     */
    bool useStartup() const
    {

        return ( testContext_->usingStartup_ );
    }

    /** the test context */
    std::shared_ptr<TestContext> testContext_;

    /** The writable database name */
    std::string writeableDbName_;
};

} // namespace YumaTest

// ---------------------------------------------------------------------------|

#endif // __YUMA_BASE_SUITE_FIXTURE__H
