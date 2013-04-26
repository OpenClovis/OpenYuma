// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-yang-fixture.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
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
SimpleYangFixture::SimpleYangFixture() 
    : QuerySuiteFixture()
    , moduleNs_( "http://netconfcentral.org/ns/simple_yang_test" )
    , containerName_( "protocol" )
    , runningChoice_( "" )
    , candidateChoice_( "" )
{
    // ensure the module is loaded
    queryEngine_->loadModule( primarySession_, "simple_yang_test" );
}

// ---------------------------------------------------------------------------|
SimpleYangFixture::~SimpleYangFixture() 
{
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::createContainer(
    std::shared_ptr<AbstractNCSession> session,
    const string& container)
{
    assert( session );
    string query = messageBuilder_->genTopLevelContainerText( 
            container, moduleNs_, "create" );
    runEditQuery( session, query );
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::deleteContainer(
    std::shared_ptr<AbstractNCSession> session,
    const string& container)
{
    string query = messageBuilder_->genTopLevelContainerText( 
            container, moduleNs_, "delete" );
    runEditQuery( session, query );

    if (container == "protocol") 
    {
        candidateChoice_ = "";
        if ( !useCandidate() )
        {
            runningChoice_= candidateChoice_;
        }
    }
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::createInterfaceContainer(
    std::shared_ptr<AbstractNCSession> session,
    const string& type,
    int mtu,
    const string& operation)
{
    assert( session );
    string query = messageBuilder_->genInterfaceContainerText( 
            moduleNs_, type, mtu, operation );
    
    if ((type == "ethernet" && mtu == 1500) ||
        (type == "atm" && mtu >= 64 && mtu <= 17966))
    {
        runEditQuery( session, query );
    }
    else
    {
        runFailedEditQuery( session, query, "MTU must be");
    }
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::addChoice(
    std::shared_ptr<AbstractNCSession> session,
    const string& choiceStr,
    const string& operationStr )
{
    assert( session );
    
    string query = messageBuilder_->genModuleOperationText( containerName_, moduleNs_,
            "<" + choiceStr + " nc:operation=\"" + operationStr +"\"/>"  );
    runEditQuery( session, query );

    candidateChoice_ = choiceStr;
    if ( !useCandidate() )
    {
        runningChoice_= candidateChoice_;
    }
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::checkChoiceImpl(
    std::shared_ptr<AbstractNCSession> session,
    const string& targetDbName,
    const string& choice )
{
    vector<string> expPresent{ "data" };
    vector<string> expNotPresent{ "error", "rpc-error" };

    if (choice == "")
    {
        expNotPresent.push_back("udp");
        expNotPresent.push_back("tcp");
    }
    else
    {
        string notChoice = (choice == "udp") ? "tcp" : "udp";
        expPresent.push_back(choice);
        expNotPresent.push_back(notChoice);
    }

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigXpath( session, containerName_, targetDbName, 
                                     checker );
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::checkChoice(
    std::shared_ptr<AbstractNCSession> session)
{
    assert( session );

    if ( useCandidate() )
    {
        checkChoiceImpl( session, "candidate", candidateChoice_ );
    }
    
    checkChoiceImpl( session, "running", runningChoice_ );
}

// ---------------------------------------------------------------------------|
void SimpleYangFixture::commitChanges(
    std::shared_ptr<AbstractNCSession> session )
{
    QuerySuiteFixture::commitChanges( session );
    
    // copy the editted changes to the running changes
    if ( useCandidate() )
    {
        runningChoice_ = candidateChoice_;
    }

}

} // namespace YumaTest
