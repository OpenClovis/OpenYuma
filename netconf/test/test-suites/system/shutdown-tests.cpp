// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( shutdown_tests, QuerySuiteFixture )

BOOST_AUTO_TEST_CASE( shutdown )
{
    DisplayTestDescrption( 
            "Demonstrate shutdown operation (WARNING: netconf server must be "
            "manualy restarted following this test).",
            "Procedure: \n"
            "\t 1 - Shutdown\n"
            "\t 2 - Try another query\n"
            "\t 3 - Check that no response is received\n"
            );

    runShutdown( primarySession_ );
    sleep(1);
    runNoSession( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

