// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File scope namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
NCDbScopedLock::NCDbScopedLock( 
        shared_ptr< NCQueryTestEngine > engine, 
        shared_ptr< AbstractNCSession > session,
        const string& target )
    : engine_( engine )
    , session_( session )
    , target_( target )
{
    engine_->lock( session_, target_ );
}

// ---------------------------------------------------------------------------|
NCDbScopedLock::~NCDbScopedLock()
{
    engine_->unlock( session_, target_ );
}

// ---------------------------------------------------------------------------|
NCQueryTestEngine::NCQueryTestEngine( shared_ptr<NCMessageBuilder> builder ) 
    : NCBaseQueryTestEngine( builder )
{
}

// ---------------------------------------------------------------------------|
NCQueryTestEngine::~NCQueryTestEngine()
{
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::lock( shared_ptr<AbstractNCSession> session,
                              const string& target )
{
    vector<string> expPresent = { "ok" };
    vector<string> expNotPresent = { "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    tryLockDatabase( session, target, checker );
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::unlock( shared_ptr<AbstractNCSession> session,
                                const string& target )
{
    vector<string> expPresent = { "ok" };
    vector<string> expNotPresent = { "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    tryUnlockDatabase( session, target, checker );
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::commit( shared_ptr<AbstractNCSession> session )
{
    vector<string> expPresent = { "ok" };
    vector<string> expNotPresent = { "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    string queryStr = messageBuilder_->buildCommitMessage( 
            session->allocateMessageId() );
    runQuery( session, queryStr, checker );
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::commitFailure( shared_ptr<AbstractNCSession> session )
{
    vector<string> expPresent = { "error", "rpc-error" };
    vector<string> expNotPresent = { "ok" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    string queryStr = messageBuilder_->buildCommitMessage( 
            session->allocateMessageId() );
    runQuery( session, queryStr, checker );
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::confirmedCommit( shared_ptr<AbstractNCSession> session,
                                         const int timeout )
{
    vector<string> expPresent = { "ok" };
    vector<string> expNotPresent = { "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    string queryStr = messageBuilder_->buildConfirmedCommitMessage( 
            timeout, session->allocateMessageId() );
    runQuery( session, queryStr, checker );
}

// ---------------------------------------------------------------------------|
void NCQueryTestEngine::loadModule( shared_ptr<AbstractNCSession> session,
                                    const string& moduleName )
{
    vector<string> expPresent = { "mod-revision" };
    vector<string> expNotPresent = { "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );

    tryLoadModule( session, moduleName, checker );
}

} // namespace YumaTest
