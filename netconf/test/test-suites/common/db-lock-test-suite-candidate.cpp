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

BOOST_FIXTURE_TEST_SUITE( db_lock_test_suite_candidate, QuerySuiteFixture )

// ---------------------------------------------------------------------------|
// Simple lock and unlock of the target and running database
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( lock_unlock_candidate_running_db )
{
    DisplayTestDescrption( 
            "Demonstrate locking and unlocking of the running " 
            "and candidate databases.",
            "Procedure: \n"
            "\t1 - Lock the running database\n"
            "\t2 - Unlock the running database" );
   
    vector<string> expPresent{ "ok" };
    vector<string> expNotPresent{ "lock-denied", "error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    
    queryEngine_->tryLockDatabase( primarySession_, "running", checker );
    queryEngine_->tryLockDatabase( primarySession_, "candidate", checker );
    queryEngine_->tryUnlockDatabase( primarySession_, "candidate", checker );
    queryEngine_->tryUnlockDatabase( primarySession_, "running", checker );
}

// ---------------------------------------------------------------------------|
// Multiple attempts to lock the target database - different session
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( deadlock_running_candidate_dbs )
{
    DisplayTestDescrption( 
            "Demonstrate deadlocking is handled whereby when different "
            "sessions lock the running and candidate databases.",
            "Procedure: \n"
            "\t1 - Session 1 Locks the running database\n"
            "\t2 - Session 2 Locks the candidate database\n"
            "\t3 - Session 1 attempts to Lock the candidate database"
            " (should fail)\n"
            "\t4 - Session 2 attempts to Lock the running database"
            " (should fail)\n"
            "\t5 - Session 1 attempts to Unlock the candidate database"
            " (should fail)\n"
            "\t6 - Session 2 attempts to Unlock the running database"
            " (should fail)\n"
            "\t7 - Session 2 Unlocks the candidate database\n"
            "\t8 - Session 1 Locks the candidate database\n"
            "\t9 - Session 1 Unlocks the candidate database\n"
            "\t10 - Session 1 Unlocks the running database\n"
            "\t11 - Session 2 Locks the running database\n"
            "\t12 - Session 2 Locks the candidate database\n"
            "\t13 - Session 1 Unlocks the candidate database\n"
            "\t14 - Session 1 Unlocks the running database\n" );
   
    // setup the expected results
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );

    vector<string> failAbsentText{ "ok" };
    vector<string> failLockPresentText{ "error", "no-access", "lock-denied" };
    vector<string> failUnlockPresentText{ "error", "no-access", "in-use", 
                                          "config locked" };
    StringsPresentNotPresentChecker lockFailChecker( failLockPresentText, failAbsentText );
    StringsPresentNotPresentChecker unlockFailChecker( failUnlockPresentText, failAbsentText );

    // get a secondary session
    std::shared_ptr<AbstractNCSession> secondarySession( sessionFactory_->createSession() );

    // 1 - Ses 1 Locks running 
    queryEngine_->tryLockDatabase( primarySession_,    "running",   successChecker ); 

    // 2 - Ses 2 Locks candidate 
    queryEngine_->tryLockDatabase( secondarySession,   "candidate", successChecker ); 

    // 3 - Ses 1 attempts Lock candidate
    queryEngine_->tryLockDatabase( primarySession_,    "candidate", lockFailChecker );
    
    // 4 - Ses 2 attempts Lock running
    queryEngine_->tryLockDatabase( secondarySession,   "running",   lockFailChecker );

    // 5 - Ses 1 attempts Unlock candidate
    queryEngine_->tryUnlockDatabase( primarySession_,  "candidate", unlockFailChecker );

    // 6 - Ses 2 attempts Unlock running 
    queryEngine_->tryUnlockDatabase( secondarySession, "running",   unlockFailChecker );

    // 7 - Ses 2 Unlocks candidate 
    queryEngine_->tryUnlockDatabase( secondarySession, "candidate", successChecker );   

    // 8 - Ses 1 Locks candidate 
    queryEngine_->tryLockDatabase( primarySession_,    "candidate", successChecker ); 

    // 9 - Ses 1 Unlocks candidate 
    queryEngine_->tryUnlockDatabase( primarySession_,  "candidate", successChecker );  

    // 10 - Ses 1 Unlocks running 
    queryEngine_->tryUnlockDatabase( primarySession_,  "running",   successChecker );    

    // 11 - Ses 2 Locks running 
    queryEngine_->tryLockDatabase( secondarySession,   "running",   successChecker );     

    // 12 - Ses 2 Locks candidate 
    queryEngine_->tryLockDatabase( secondarySession,   "candidate", successChecker );   

    // 13 - Ses 2 Unlocks candidate 
    queryEngine_->tryUnlockDatabase( secondarySession, "candidate", successChecker ); 

    // 14 - Ses 2 Unlocks running 
    queryEngine_->tryUnlockDatabase( secondarySession, "running",   successChecker );   
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

