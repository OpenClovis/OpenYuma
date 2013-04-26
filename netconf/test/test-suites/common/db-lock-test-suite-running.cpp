// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( db_lock_test_suite_running, QuerySuiteFixture )

// ---------------------------------------------------------------------------|
// Simple lock and unlock of the target and running database
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( lock_candidate_running_db )
{
    DisplayTestDescrption( 
            "Demonstrate that the candidate database cannot be locked with "
            "the writeable running configuration.",
            "Procedure: \n"
            "\t1 - Attempt to Lock the candidate database\n" );
   
    vector<string> failLockPresentText{ "error", "invalid-value", 
                                        "config not found" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failLockPresentText, failAbsentText );

    queryEngine_->tryLockDatabase( primarySession_, "candidate", failChecker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

