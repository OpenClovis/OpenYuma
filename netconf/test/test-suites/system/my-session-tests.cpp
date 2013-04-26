// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( my_session_tests, QuerySuiteFixture )

BOOST_AUTO_TEST_CASE( set_and_get_my_session )
{
    DisplayTestDescrption( 
            "Demonstrate set-my-session and get-my-session operations.",
            "Procedure: \n"
            "\t 1 - Set the session indent, linesize and with-defaults setting\n"
            "\t 2 - Check that the session has been updated correctly\n"
            "\t 3 - Set the session indent, linesize and with-defaults setting to new values\n"
            "\t 4 - Check that the session has been updated correctly\n"
            );

    runSetMySession( primarySession_, "1", "80", "report-all" );
    runGetMySession( primarySession_, "1", "80", "report-all" );
    runSetMySession( primarySession_, "5", "54", "trim" );
    runGetMySession( primarySession_, "5", "54", "trim" );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_CASE( kill_session )
{
    DisplayTestDescrption( 
            "Demonstrate that following a kill-session a locked database "
            "can be accessed.",
            "Procedure: \n"
            "\t1 - Session 1 Locks the running database\n"
            "\t2 - Session 2 attempts to lock the running database "
            "( should fail )\n"
            "\t3 - Session 2 kills session 1"
            "\t4 - Session 2 Locks the running database"
            "\t5 - Session 2 Unlocks the running database" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );

    vector<string> failPresentText{ "error", "no-access", "lock denied" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );

    std::shared_ptr<AbstractNCSession> secondarySession( 
            sessionFactory_->createSession() );

    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );

    queryEngine_->tryLockDatabase( secondarySession, "running", failChecker );

    queryEngine_->tryKillSession( secondarySession, primarySession_->getId(), successChecker);

    queryEngine_->tryLockDatabase( secondarySession, "running", successChecker );
    queryEngine_->tryUnlockDatabase( secondarySession, "running", successChecker );
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_CASE( close_session )
{
    DisplayTestDescrption( 
            "Demonstrate that following a close-session a locked database "
            "can be accessed.",
            "Procedure: \n"
            "\t1 - Session 1 Locks the running database\n"
            "\t2 - Session 2 attempts to lock the running database "
            "( should fail )\n"
            "\t3 - Session 1 performs a close-session"
            "\t4 - Session 2 Locks the running database"
            "\t5 - Session 2 Unlocks the running database" );
    
    vector<string> successPresentText{ "ok" };
    vector<string> successAbsentText{ "lock-denied", "error" };
    StringsPresentNotPresentChecker successChecker( successPresentText, successAbsentText );

    vector<string> failPresentText{ "error", "no-access", "lock denied" };
    vector<string> failAbsentText{ "ok" };
    StringsPresentNotPresentChecker failChecker( failPresentText, failAbsentText );

    std::shared_ptr<AbstractNCSession> secondarySession( 
            sessionFactory_->createSession() );

    queryEngine_->tryLockDatabase( primarySession_, "running", successChecker );

    queryEngine_->tryLockDatabase( secondarySession, "running", failChecker );

    queryEngine_->tryCloseSession( primarySession_, successChecker);

    queryEngine_->tryLockDatabase( secondarySession, "running", successChecker );
    queryEngine_->tryUnlockDatabase( secondarySession, "running", successChecker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

