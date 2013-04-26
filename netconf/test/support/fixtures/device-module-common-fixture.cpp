// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-module-common-fixture.h"

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

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

// ---------------------------------------------------------------------------|
DeviceModuleCommonFixture::DeviceModuleCommonFixture( const string& moduleNs ) 
    : QuerySuiteFixture( shared_ptr<NCMessageBuilder>( 
        new  XPOQueryBuilder( moduleNs ) ) )
    , xpoBuilder_( static_pointer_cast< XPOQueryBuilder >( messageBuilder_ ) )
    , runningEntries_( new XPO3Container )
    , candidateEntries_( useCandidate() 
            ? shared_ptr<XPO3Container>( new XPO3Container )
            : runningEntries_ )
{
}

// ---------------------------------------------------------------------------|
DeviceModuleCommonFixture::~DeviceModuleCommonFixture() 
{
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::ensureProfileExists( const uint16_t profileId )
{
    if ( candidateEntries_->profiles_.end() == 
         candidateEntries_->profiles_.find( profileId ) )
    {
        Profile newProf;
        newProf.id_ = profileId;
        candidateEntries_->profiles_.insert( make_pair( profileId, newProf ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::ensureStreamConnectionExists( 
        const uint16_t profileId,
        const uint16_t connectionId )
{
    ensureProfileExists( profileId );

    if ( candidateEntries_->profiles_[ profileId ].streamConnections_.end() == 
         candidateEntries_->profiles_[ profileId ].streamConnections_.find( connectionId ) )
    {
        StreamConnectionItem conn;
        conn.id_ = connectionId;
        candidateEntries_->profiles_[ profileId ].streamConnections_.insert(
                make_pair( connectionId, conn ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::ensureProfileStreamExists( 
        const uint16_t profileId,
        const uint16_t streamId )
{
    ensureProfileExists( profileId );

    if ( candidateEntries_->profiles_[ profileId ].streams_.end() == 
         candidateEntries_->profiles_[ profileId ].streams_.find( streamId ) )
    {
        StreamItem newStream;
        newStream.id_ = streamId;

        candidateEntries_->profiles_[ profileId ].streams_.insert( 
                make_pair( streamId, newStream ) );
    }
}

//------------------------------------------------------------------------------
void DeviceModuleCommonFixture::ensureResourceExists( 
        ResourceDescriptionMap& resourceMap,
        const uint32_t resourceId )
{
    if ( resourceMap.find( resourceId ) == resourceMap.end() )
    {
        ResourceNode res;
        res.id_ = resourceId;
        resourceMap.insert( make_pair( resourceId, res ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::ensureResourceDescriptionExists( 
        const uint16_t profileId,
        const uint16_t streamId, 
        const uint16_t resourceId )
{
    ensureProfileStreamExists( profileId, streamId );
    ensureResourceExists( candidateEntries_->profiles_[ profileId ].
                streams_[ streamId ].resourceDescription_, resourceId );
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::ensureResourceConnectionExists( 
        const uint16_t profileId,
        const uint16_t streamId, 
        const uint16_t connectionId )
{
    ensureProfileStreamExists( profileId, streamId );

    if ( candidateEntries_->profiles_[ profileId ]
         .streams_[streamId].resourceConnections_.end() == 
         candidateEntries_->profiles_[ profileId ]
         .streams_[streamId].resourceConnections_.find( connectionId ) )
    {
        ConnectionItem conn;
        conn.id_ = connectionId;
        candidateEntries_->profiles_[ profileId ].streams_[ streamId ]
             .resourceConnections_.insert( make_pair( connectionId, conn ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::createXpoContainer(
    std::shared_ptr<AbstractNCSession> session,
    const string& failReason )
{
    string query = xpoBuilder_->genXPOQuery( "create" );
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
void DeviceModuleCommonFixture::deleteXpoContainer(
    std::shared_ptr<AbstractNCSession> session,
    const bool commitChange,
    const string& failReason )
{
    string query = xpoBuilder_->genXPOQuery( "delete" );

    if ( failReason.empty() )
    {
        runEditQuery( session, query );

        candidateEntries_->clear();
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
void DeviceModuleCommonFixture::xpoProfileQuery(
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const std::string& op,
    const string& failReason )
{
    string query = xpoBuilder_->genProfileQuery( profileId, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" )
        {
            candidateEntries_->profiles_.erase( profileId );
        }
        else if ( op == "replace" )
        {
            candidateEntries_->profiles_.erase( profileId );
            ensureProfileExists( profileId );
        }
        else
        {
            ensureProfileExists( profileId );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::setActiveProfile(
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const string& failReason )
{
    string query = xpoBuilder_->genSetActiveProfileIdQuery( profileId, "replace" );

    if ( failReason.empty() )
    {
        runEditQuery( session, query );

        // @TODO: add check for failure if the selected profile does not
        // @TODO: exist, when this is supported by the real database.
        candidateEntries_->activeProfile_ = profileId;
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::profileStreamQuery(
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamId,
    const std::string& op,
    const string& failReason )
{
    string query = xpoBuilder_->genProfileStreamItemQuery( profileId, streamId, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" )
        {
            // remove the stream. 
            candidateEntries_->profiles_[ profileId ].streams_.erase( streamId );
        }
        else if ( op == "replace" )
        {
            candidateEntries_->profiles_[ profileId ].streams_.erase( streamId );
            ensureProfileStreamExists( profileId, streamId );
        }
        else
        {
            ensureProfileStreamExists( profileId, streamId );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::resourceDescriptionQuery( 
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t resourceNodeId,
    const std::string& op,
    const string& failReason )
{
    string query = xpoBuilder_->genProfileChildQuery( profileId, streamId,
            "resourceNode", resourceNodeId, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" )
        {
            candidateEntries_->profiles_[ profileId ].streams_[ streamId ]
                .resourceDescription_.erase( resourceNodeId );
        }
        else
        {
            ensureResourceDescriptionExists( profileId, streamId, resourceNodeId );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::resourceConnectionQuery( 
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t connectionId,
    const std::string& op,
    const string& failReason )
{
    string query = xpoBuilder_->genProfileChildQuery( profileId, streamId,
            "resourceConnection", connectionId, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" )
        {
            candidateEntries_->profiles_[ profileId ].streams_[ streamId ]
                .resourceConnections_.erase( connectionId );
        }
        else
        {
            ensureResourceConnectionExists( profileId, streamId, connectionId );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }

}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::updateResourceDescriptionEntry( 
    ResourceNode& resourceNode, const ResourceNodeConfig& config )
{
    if ( config.resourceType_ )
    {
        resourceNode.resourceType_ = *config.resourceType_;
    }

    if ( config.configuration_ )
    {
        resourceNode.configuration_ = *config.configuration_;
    }

    if ( config.statusConfig_ )
    {
        resourceNode.statusConfig_ = *config.statusConfig_;
    }

    if ( config.alarmConfig_ )
    {
        resourceNode.alarmConfig_ = *config.alarmConfig_;
    }

    if ( config.physicalPath_ )
    {
        resourceNode.physicalPath_ = *config.physicalPath_;
    }
}


// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::configureResourceDescrption( 
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t resourceId,
    const ResourceNodeConfig& config,
    const std::string& op,
    const string& failReason )
{
    string query =  xpoBuilder_->configureVResourceNode( config, "" );
    query = xpoBuilder_->addResourceNodePath( profileId, streamId, resourceId, 
            query, op );

    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" || op == "replace" )
        {
            if ( candidateEntries_->profiles_.find( profileId ) != 
                 candidateEntries_->profiles_.end() &&
                 candidateEntries_->profiles_[ profileId ].streams_.find( streamId ) != 
                 candidateEntries_->profiles_[ profileId ].streams_.end() )
            {
                candidateEntries_->profiles_[ profileId ].streams_[ streamId ]
                    .resourceDescription_.erase( resourceId );
            }

            if ( op == "delete" )
            {
                return;
            }
        }

        ensureResourceDescriptionExists( profileId, streamId, resourceId );
        updateResourceDescriptionEntry( 
                candidateEntries_->profiles_[profileId].streams_[streamId].resourceDescription_[resourceId], 
            config );
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
template< class ConnectionItemType, class ConfigType >
void DeviceModuleCommonFixture::storeConnectionItemConfig( 
    ConnectionItemType& connectionItem, 
    const ConfigType& config )
{
    if ( config.sourceId_ )
    {
        connectionItem.sourceId_ = *config.sourceId_;
    }

    if ( config.sourcePinId_ )
    {
        connectionItem.sourcePinId_ = *config.sourcePinId_;
    }

    if ( config.destinationId_ )
    {
        connectionItem.destinationId_ = *config.destinationId_;
    }

    if ( config.destinationPinId_ )
    {
        connectionItem.destinationPinId_ = *config.destinationPinId_;
    }

    if ( config.bitrate_ )
    {
        connectionItem.bitrate_ = *config.bitrate_;
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::configureResourceConnection( 
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t connectionId,
    const ConnectionItemConfig& config,
    const std::string& op,
    const string& failReason )
{
    string query =  xpoBuilder_->configureResourceConnection( config, "" );
    query = xpoBuilder_->addVRConnectionNodePath( profileId, streamId, 
            connectionId, query, op );

    if ( failReason.empty() )
    {
        runEditQuery( session, query );

        if ( op == "delete" || op == "replace" )
        {
            if ( candidateEntries_->profiles_.find( profileId ) != 
                 candidateEntries_->profiles_.end() &&
                 candidateEntries_->profiles_[ profileId ].streams_.find( streamId ) != 
                 candidateEntries_->profiles_[ profileId ].streams_.end() )
            {
                candidateEntries_->profiles_[ profileId ].streams_[ streamId ]
                    .resourceConnections_.erase( connectionId );
            }

            if ( op == "delete" )
            {
                return;
            }
        }

        ensureResourceConnectionExists( profileId, streamId, connectionId );
        storeConnectionItemConfig( candidateEntries_->profiles_[ profileId ]
                .streams_[ streamId ].resourceConnections_[connectionId]
                , config );
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::configureStreamConnection( 
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t connectionId,
    const StreamConnectionItemConfig& config,
    const std::string& op,
    const string& failReason )
{
    string query =  xpoBuilder_->configureStreamConnection( config, "" );
    query = xpoBuilder_->addStreamConnectionPath( profileId, connectionId, query, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );

        ensureStreamConnectionExists( profileId, connectionId );

        StreamConnectionItem& connItem = 
            candidateEntries_->profiles_[ profileId ].streamConnections_[connectionId];

        storeConnectionItemConfig( connItem, config );

        if ( config.sourceStreamId_ )
        {
            connItem.sourceStreamId_ = *config.sourceStreamId_;
        }

        if ( config.destinationStreamId_ )
        {
            connItem.destinationStreamId_ = *config.destinationStreamId_;
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::streamConnectionQuery(
    std::shared_ptr<AbstractNCSession> session,
    const uint16_t profileId,
    const uint16_t streamConnId,
    const std::string& op,
    const string& failReason )
{
    string query = xpoBuilder_->genStreamConnectionQuery( profileId, 
            streamConnId, op );
    if ( failReason.empty() )
    {
        runEditQuery( session, query );
        if ( op == "delete" )
        {
            candidateEntries_->profiles_[ profileId ]
                .streamConnections_.erase( streamConnId );
        }
        else
        {
            ensureStreamConnectionExists( profileId, streamConnId );
        }
    }
    else
    {
        runFailedEditQuery( session, query, failReason );
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::commitChanges(
    std::shared_ptr<AbstractNCSession> session )
{
    QuerySuiteFixture::commitChanges( session );

    if ( useCandidate() )
    {
        *runningEntries_ = *candidateEntries_;
    }
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::discardChanges()
{
    if ( useCandidate() )
    {
        *candidateEntries_ = *runningEntries_;
    }
}


// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::queryConfig( const string& dbName ) const
{
    vector<string> expPresent{ "data" };
    vector<string> expNotPresent{ "error", "rpc-error" };
    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigXpath( primarySession_, "xpo", 
            dbName, checker );
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::checkEntriesImpl( const string& dbName,
        const shared_ptr<XPO3Container>& db )
{
    XMLContentChecker<XPO3Container> checker( db );
    queryEngine_->tryGetConfigXpath( primarySession_, "xpo", 
            dbName, checker );
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::checkConfig() 
{
    checkEntriesImpl( writeableDbName_, candidateEntries_ );
    checkEntriesImpl( "running", runningEntries_ );
}

// ---------------------------------------------------------------------------|
void DeviceModuleCommonFixture::initialise2Profiles() 
{
    createXpoContainer( primarySession_ );
    // create Profile 1
    xpoProfileQuery( primarySession_, 1 );
    profileStreamQuery( primarySession_, 1, 1 );
    configureResourceDescrption( primarySession_, 1, 1, 1, 
                                 ResourceNodeConfig{ 100, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-01" ) } );
    configureResourceDescrption( primarySession_, 1, 1, 2, 
                                 ResourceNodeConfig{ 101, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-02" ) } );
    configureResourceDescrption( primarySession_, 1, 1, 3, 
                                 ResourceNodeConfig{ 102, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-03" ) } );
    configureResourceConnection( primarySession_, 1, 1, 1, ConnectionItemConfig{ 1, 10, 2, 11, 100 } );
    configureResourceConnection( primarySession_, 1, 1, 2, ConnectionItemConfig{ 2, 20, 3, 21, 200 } );

    profileStreamQuery( primarySession_, 1, 2 );
    configureResourceDescrption( primarySession_, 1, 2, 1, 
                                 ResourceNodeConfig{ 103, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-04" ) } );
    configureResourceDescrption( primarySession_, 1, 2, 2, 
                                 ResourceNodeConfig{ 104, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-05" ) } );
    configureResourceDescrption( primarySession_, 1, 2, 3, 
                                 ResourceNodeConfig{ 105, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-06" ) } );
    configureResourceConnection( primarySession_, 1, 2, 1, ConnectionItemConfig{ 1, 1, 1, 1, 100 } );
    configureResourceConnection( primarySession_, 1, 2, 2, ConnectionItemConfig{ 1, 1, 1, 1, 100 } );

    configureStreamConnection( primarySession_, 1, 1, StreamConnectionItemConfig{ 1, 1, 3, 1, 100, 1, 2 } );
    configureStreamConnection( primarySession_, 1, 2, StreamConnectionItemConfig{ 2, 1, 2, 1, 100, 2, 1 } );

    // create Profile 2
    xpoProfileQuery( primarySession_, 2 );
    profileStreamQuery( primarySession_, 2, 1 );
    configureResourceDescrption( primarySession_, 2, 1, 1, 
                                 ResourceNodeConfig{ 100, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-11" ) } );
    configureResourceDescrption( primarySession_, 2, 1, 2, 
                                 ResourceNodeConfig{ 101, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-12" ) } );
    configureResourceDescrption( primarySession_, 2, 1, 3, 
                                 ResourceNodeConfig{ 102, 
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-13" ) } );
    configureResourceConnection( primarySession_, 2, 1, 1, ConnectionItemConfig{ 1, 10, 2, 11, 110 } );
    configureResourceConnection( primarySession_, 2, 1, 2, ConnectionItemConfig{ 2, 20, 3, 21, 210 } );

    profileStreamQuery( primarySession_, 2, 2 );
    configureResourceDescrption( primarySession_, 2, 2, 1, 
                                 ResourceNodeConfig{ 103,  
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-14" ) } );
    configureResourceDescrption( primarySession_, 2, 2, 2, 
                                 ResourceNodeConfig{ 104,  
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-15" ) } );
    configureResourceDescrption( primarySession_, 2, 2, 3, 
                                 ResourceNodeConfig{ 105,  
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     boost::optional<string>(),
                                                     string( "VR-16" ) } );
    configureResourceConnection( primarySession_, 2, 2, 1, ConnectionItemConfig{ 1, 1, 1, 1, 250 } );
    configureResourceConnection( primarySession_, 2, 2, 2, ConnectionItemConfig{ 1, 1, 1, 1, 300 } );

    configureStreamConnection( primarySession_, 2, 1, StreamConnectionItemConfig{ 1, 1, 3, 1, 100, 2, 1 } );
    configureStreamConnection( primarySession_, 2, 2, StreamConnectionItemConfig{ 2, 1, 2, 1, 100, 1, 2 } );

    // set the active profile
    setActiveProfile( primarySession_, 1 );

    // commit and check
    commitChanges( primarySession_ );
    checkConfig();
}

} // namespace XPO3NetconfIntegrationTest

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
