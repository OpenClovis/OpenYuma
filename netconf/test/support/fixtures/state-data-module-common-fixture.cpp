// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/state-data-module-common-fixture.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>
#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/checkers/xml-content-checker.h" 
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|

namespace YumaTest 
{

// ---------------------------------------------------------------------------|
StateDataModuleCommonFixture::StateDataModuleCommonFixture(
        const string& moduleNs ) 
    : QuerySuiteFixture( shared_ptr<NCMessageBuilder>( 
        new  StateDataQueryBuilder( moduleNs ) ) )
    , toasterBuilder_(
          static_pointer_cast< StateDataQueryBuilder >( messageBuilder_ ) )
    , runningEntries_( new ToasterContainer )
    , candidateEntries_( useCandidate() 
            ? shared_ptr<ToasterContainer>( new ToasterContainer )
            : runningEntries_ )
{
}

// ---------------------------------------------------------------------------|
StateDataModuleCommonFixture::~StateDataModuleCommonFixture() 
{
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::createToasterContainer(
    std::shared_ptr<AbstractNCSession> session,
    const std::string& op,
    const string& failReason )
{
    string query = toasterBuilder_->genToasterQuery( "create" );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::deleteToasterContainer(
    std::shared_ptr<AbstractNCSession> session,
    const bool commitChange,
    const string& failReason )
{
    string query = toasterBuilder_->genToasterQuery( "delete" );

    if ( failReason.empty() )
    {
        runEditQuery( session, query );

        if ( commitChange )
        {
            commitChanges( session );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}


// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::configureToaster( 
        const ToasterContainerConfig& config )
{
    if ( config.toasterManufacturer_ )
    {
        candidateEntries_->toasterManufacturer_ = *config.toasterManufacturer_;
    }
    if ( config.toasterModelNumber_ )
    {
        candidateEntries_->toasterModelNumber_ = *config.toasterModelNumber_;
    }
    if ( config.toasterStatus_ )
    {
        candidateEntries_->toasterStatus_ = *config.toasterStatus_;
    }
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::commitChanges(
    std::shared_ptr<AbstractNCSession> session )
{
    QuerySuiteFixture::commitChanges( session );

    if ( useCandidate() )
    {
        *runningEntries_ = *candidateEntries_;
    }
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::discardChanges()
{
    if ( useCandidate() )
    {
        *candidateEntries_ = *runningEntries_;
    }
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::queryState( const string& dbName ) const
{
    vector<string> expPresent{ "data" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetXpath( primarySession_, "toaster", 
            dbName, checker );
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::checkEntriesImpl( const string& dbName,
        const shared_ptr<ToasterContainer>& db ) const
{
    XMLContentChecker<ToasterContainer> checker( db );
    queryEngine_->tryGetXpath( primarySession_, "toaster", 
            dbName, checker );
}

// ---------------------------------------------------------------------------|
void StateDataModuleCommonFixture::checkState() const
{
    checkEntriesImpl( writeableDbName_, candidateEntries_ );
    checkEntriesImpl( "running", runningEntries_ );
}

} // namespace StateDataNetconfIntegrationTest

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
