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
#include "test/support/nc-session/abstract-nc-session-factory.h"
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

BOOST_FIXTURE_TEST_SUITE( db_lock_test_suite_common, QuerySuiteFixture )

// ---------------------------------------------------------------------------|
// Simple lock and unlock of the target and running database
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( lock_unlock_running_db )
{
    DisplayTestDescrption( 
            "Demonstrate locking and unlocking of the running databases.",
            "Procedure: \n"
            "\t1 - Lock the running database\n"
            "\t2 - Unlock the running database" );
   
    vector<string> expPresent{ "ok" };
    vector<string> expNotPresent{ "lock-denied", "error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    queryEngine_->tryLockDatabase( primarySession_, "running", checker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", checker );
}

// ---------------------------------------------------------------------------|
// Multiple attempts to lock the target database - same session
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( lock_running_twice )
{
    DisplayTestDescrption( 
            "Demonstrate multiple attempts to lock the database by the "
            "same session are prevented.",
            "Procedure: \n"
            "\t1 - Lock the running database\n"
            "\t2 - Attempt to Lock the running database again ( should "
            "fail )\n"
            "\t3 - Unlock the running database" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );
    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );

    vector<string> failPresentText{ "lock-denied", "no-access", "error" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );
    queryEngine_->tryLockDatabase( primarySession_, "running", failChecker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", successChecker );
}

// ---------------------------------------------------------------------------|
// Multiple attempts to lock the target database - same session
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( unlock_running_twice )
{
    DisplayTestDescrption( 
            "Demonstrate multiple attempts to unlock the database by the "
            "same session are prevented.",
            "Procedure: \n"
            "\t1 - Lock the running database\n"
            "\t2 - Unlock the running database\n"
            "\t3 - Attempt to unlock the running database again ( should "
            "fail )\n" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );
    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", successChecker );

    vector<string> failPresentText{ "error", "operation-failed", 
                                    "no-access", "wrong config state" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", failChecker );
}

// ---------------------------------------------------------------------------|
// Multiple attempts to lock the target database - different session
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( lock_running_different_sessions )
{
    DisplayTestDescrption( 
            "Demonstrate multiple attempts to lock the database by the "
            "different sessions are prevented.",
            "Procedure: \n"
            "\t1 - Session 1 Locks the running database\n"
            "\t2 - Session 2 attempts to lock the running database "
            "( should fail )\n"
            "\t3 - Session 1 Unlocks the running database" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );
    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );

    std::shared_ptr<AbstractNCSession> secondarySession( 
            sessionFactory_->createSession() );

    vector<string> failPresentText{ "lock-denied", "error" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );

    queryEngine_->tryLockDatabase( secondarySession, "running", failChecker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", successChecker );
}

// ---------------------------------------------------------------------------|
// Try to unlock the target database from a session that did not lock it 
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( unlock_running_different_sessions )
{
    DisplayTestDescrption( 
            "Demonstrate that only the session that locked the database "
            "can unlock it.",
            "Procedure: \n"
            "\t1 - Session 1 Locks the running database\n"
            "\t2 - Session 2 attempts to unlock the running database "
            "( should fail )\n"
            "\t3 - Session 1 Unlocks the running database" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );
    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );

    vector<string> failPresentText{ "error", "in-use", 
                                    "no-access", "config locked" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );

    std::shared_ptr<AbstractNCSession> secondarySession( 
            sessionFactory_->createSession() );
    queryEngine_->tryUnlockDatabase( secondarySession, "running", failChecker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", successChecker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

