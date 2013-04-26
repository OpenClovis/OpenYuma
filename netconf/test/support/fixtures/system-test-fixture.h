#ifndef __YUMA_SYSTEM_TEST_FIXTURE__H
#define __YUMA_SYSTEM_TEST_FIXTURE__H

#include "test/support/fixtures/abstract-global-fixture.h"
#include "test/support/nc-query-util/yuma-op-policies.h"
#include "test/support/fixtures/test-context.h"

namespace YumaTest 
{

// ---------------------------------------------------------------------------|
/**
 * This class is used to perform all global initialisation and teardown 
 * of Yuma. If any errors occur or are reported during initialisation
 * the test is terminated with an assertion.
 *
 * BOOST::TEST Fixtures do not have a way of configuring any internal
 * data at runtime. To allow configuration of the test fixture class 
 * all configuration data is passed compile time as a template
 * parameter.
 *
 * \tparam ArgTraits a structure containing any command line arguments the 
 *                   test harness should use to configure neconf.
 * \tparam YumaQueryOpLogPolicy the policy for generating Yuma OP log
 *                   filenames, which are written whenever the test harness 
 *                   injects a message into yuma.
 */
template < class SpoofedArgs, 
           class OpPolicy = MessageIdLogFilenamePolicy >
class SystemTestFixture : public AbstractGlobalTestFixture
{
   typedef SpoofedArgs Args;

public:
    /** 
     * Constructor. Initialise Yuma for system testing. 
     */
    SystemTestFixture();

    /**
     * Destructor. Shutdown the Yuma system test environment.
     */
    ~SystemTestFixture();

private:
    /**
     * Configure the test context
     */
    void configureTestContext();
};

} // namespace YumaTest

#endif // __YUMA_SYSTEM_TEST_FIXTURE__H
