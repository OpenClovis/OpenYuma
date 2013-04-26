// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-base-query-test-engine.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File scope namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
NCBaseQueryTestEngine::NCBaseQueryTestEngine( 
    shared_ptr<NCMessageBuilder> builder ) : messageBuilder_( builder )
{}

// ---------------------------------------------------------------------------|
NCBaseQueryTestEngine::~NCBaseQueryTestEngine()
{}

// ---------------------------------------------------------------------------|
void NCBaseQueryTestEngine::setLogLevel(
        shared_ptr<AbstractNCSession> session,
        const string& logLevel )
{
    vector<string> expPresent = { "ok" };
    vector<string> expNotPresent = { "error", "rpc-error" };

    // build a load module message for test.yang
    string queryStr = messageBuilder_->buildSetLogLevelMessage( 
            logLevel,
            session->allocateMessageId() );

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    runQuery( session, queryStr, checker );
}

} // namespace YumaTest

