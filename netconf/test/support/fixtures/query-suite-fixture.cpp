// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>
#include <memory>
#include <cassert>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/callbacks/abstract-cb-checker-factory.h"
#include "test/support/fixtures/abstract-fixture-helper-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
QuerySuiteFixture::QuerySuiteFixture() 
    : BaseSuiteFixture()
    , sessionFactory_( testContext_->sessionFactory_ )
    , cbCheckerFactory_( testContext_->cbCheckerFactory_ )
    , fixtureHelperFactory_( testContext_->fixtureHelperFactory_ )
    , primarySession_( sessionFactory_->createSession() )
    , cbChecker_( cbCheckerFactory_->createChecker() )
    , fixtureHelper_( fixtureHelperFactory_->createHelper() )
    , messageBuilder_( new NCMessageBuilder() )
    , queryEngine_( new NCQueryTestEngine( messageBuilder_ ) )
{
    assert( queryEngine_ );
}

// ---------------------------------------------------------------------------|
QuerySuiteFixture::QuerySuiteFixture( shared_ptr<NCMessageBuilder> builder ) 
    : BaseSuiteFixture()
    , sessionFactory_( testContext_->sessionFactory_ )
    , cbCheckerFactory_( testContext_->cbCheckerFactory_ )
    , primarySession_( sessionFactory_->createSession() )
    , cbChecker_( cbCheckerFactory_->createChecker() )
    , messageBuilder_( builder )
    , queryEngine_( new NCQueryTestEngine( messageBuilder_ ) )
{
    assert( queryEngine_ );
}

// ---------------------------------------------------------------------------|
QuerySuiteFixture::~QuerySuiteFixture() 
{
}

// ---------------------------------------------------------------------------|
std::vector< std::unique_ptr< NCDbScopedLock > >
QuerySuiteFixture::getFullLock( std::shared_ptr<AbstractNCSession> session )
{
    vector< std::unique_ptr< NCDbScopedLock > >locks;

    locks.push_back( std::unique_ptr< NCDbScopedLock >( new  
            NCDbScopedLock( queryEngine_, session, "running" ) ) );

    if( useCandidate() )
    {
        locks.push_back( std::unique_ptr< NCDbScopedLock >( new  
            NCDbScopedLock( queryEngine_, session, "candidate" ) ) );
    }

    if( useStartup() )
    {
        locks.push_back( std::unique_ptr< NCDbScopedLock >( new  
            NCDbScopedLock( queryEngine_, session, "startup" ) ) );
    }

    return locks;
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::commitChanges( 
        std::shared_ptr<AbstractNCSession> session )
{
    if( useCandidate() )
    {
        // send a commit
        queryEngine_->commit( session );
    }

    if( useStartup() )
    {
        assert( session );
        vector<string> expPresent{ "ok" };
        vector<string> expNotPresent{ "error", "rpc-error" };
        StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
        // send a copy-config
        queryEngine_->tryCopyConfig( session, "startup", "running", checker );
    }
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::commitChangesFailure( 
        std::shared_ptr<AbstractNCSession> session )
{
    if( useCandidate() )
    {
        // send a commit
        queryEngine_->commitFailure( session );
    }
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::confirmedCommitChanges( 
        std::shared_ptr<AbstractNCSession> session,
        const int timeout )
{
    if( useCandidate() )
    {
        // send a confirmed-commit
        queryEngine_->confirmedCommit( session, timeout );
    }
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runEditQuery( 
        shared_ptr<AbstractNCSession> session,
        const string& query )
{
    assert( session );
    vector<string> expPresent{ "ok" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryEditConfig( session, query, writeableDbName_, 
                                 checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runFailedEditQuery( 
        shared_ptr<AbstractNCSession> session,
        const string& query,
        const string& failReason )
{
    assert( session );
    vector<string> expNotPresent{ "ok" };
    vector<string> expPresent{ "error", "rpc-error", failReason };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryEditConfig( session, query, writeableDbName_, 
                                 checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runValidateCommand( 
        shared_ptr<AbstractNCSession> session )
{
    assert( session );
    vector<string> expPresent{ "ok" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryValidateDatabase( session, writeableDbName_, checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runGetMySession( 
         shared_ptr<AbstractNCSession> session,
         const string& expIndent,
         const string& expLinesize,
         const string& expWithDefaults )
{
    assert(session);

    vector<string> expPresent{ "rpc-reply", expIndent, expLinesize, expWithDefaults };
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetMySession( session, checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runSetMySession( 
         shared_ptr<AbstractNCSession> session,
         const string& indent,
         const string& linesize,
         const string& withDefaults )
{
    assert(session);

    vector<string> expPresent{ "rpc-reply", "ok"};
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->trySetMySession( session, indent, linesize, withDefaults, checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runShutdown( shared_ptr<AbstractNCSession> session )
{
    assert(session);

    vector<string> expPresent{ "rpc-reply", "ok" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryShutdown( session, checker );
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runRestart( shared_ptr<AbstractNCSession> session )
{
    assert(session);

    vector<string> expPresent{ "rpc-reply", "ok" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryRestart( session, checker );

    fixtureHelper_->mimicRestart();
}

// ---------------------------------------------------------------------------|
void QuerySuiteFixture::runNoSession( 
         shared_ptr<AbstractNCSession> session )
{
    assert(session);

    //If no response expect to check against sent message"
    vector<string> expPresent{ "rpc", "get-my-session" };
    vector<string> expNotPresent{ "rpc-reply", "rpc-error", "ok" };
    
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetMySession( session, checker );
}

} // namespace YumaTest

