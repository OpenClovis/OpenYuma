// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-get-module-fixture.h"
#include "test/support/misc-util/base64.h"

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
DeviceGetModuleFixture::DeviceGetModuleFixture() 
    : DeviceModuleCommonFixture( "http://netconfcentral.org/ns/device_test" ) 
    , filteredConfig_( new XPO3Container )
{
    // ensure the module is loaded
    queryEngine_->loadModule( primarySession_, "device_test" );
}

// ---------------------------------------------------------------------------|
DeviceGetModuleFixture::~DeviceGetModuleFixture() 
{
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureActiveProfileInFilteredConfig(
        const uint16_t profileId )
{
    filteredConfig_->activeProfile_ = profileId;
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureProfileInFilteredConfig(
        const uint16_t profileId )
{
    if ( filteredConfig_->profiles_.end() == 
         filteredConfig_->profiles_.find( profileId ) )
    {
        Profile newProf;
        newProf.id_ = profileId;
        filteredConfig_->profiles_.insert( make_pair( profileId, newProf ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureStreamConnectionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t connectionId )
{
    ensureProfileInFilteredConfig( profileId );

    if ( filteredConfig_->profiles_[ profileId ].streamConnections_.end() == 
         filteredConfig_->profiles_[ profileId ].streamConnections_.find( connectionId ) )
    {
        StreamConnectionItem conn;
        conn.id_ = connectionId;
        filteredConfig_->profiles_[ profileId ].streamConnections_.insert(
                make_pair( connectionId, conn ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureProfileStreamInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t streamId )
{
    ensureProfileInFilteredConfig( profileId );

    if ( filteredConfig_->profiles_[ profileId ].streams_.end() == 
         filteredConfig_->profiles_[ profileId ].streams_.find( streamId ) )
    {
        StreamItem newStream;
        newStream.id_ = streamId;

        filteredConfig_->profiles_[ profileId ].streams_.insert( 
                make_pair( streamId, newStream ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureResourceDescriptionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t streamId, 
        const uint16_t resourceId )
{
    ensureProfileStreamInFilteredConfig( profileId, streamId );

    if ( filteredConfig_->profiles_[ profileId ]
         .streams_[streamId].resourceDescription_.end() == 
         filteredConfig_->profiles_[ profileId ]
         .streams_[streamId].resourceDescription_.find( resourceId ) )
    {
        ResourceNode res;
        res.id_ = resourceId;

        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_.insert( make_pair( resourceId, res ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::ensureResourceConnectionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t streamId, 
        const uint16_t connectionId )
{
    ensureProfileStreamInFilteredConfig( profileId, streamId );

    if ( filteredConfig_->profiles_[ profileId ]
         .streams_[streamId].resourceConnections_.end() == 
         filteredConfig_->profiles_[ profileId ]
         .streams_[streamId].resourceConnections_.find( connectionId ) )
    {
        ConnectionItem conn;
        conn.id_ = connectionId;
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
             .resourceConnections_.insert( make_pair( connectionId, conn ) );
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::configureResourceDescrptionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t streamId,
        const uint16_t resourceId,
        const ResourceNodeConfig& config )
{
    ensureResourceDescriptionInFilteredConfig( profileId, streamId, resourceId );
    if ( config.resourceType_ )
    {
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_[ resourceId ].resourceType_ = *config.resourceType_;
    }
    if ( config.configuration_ )
    {
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_[ resourceId ].configuration_ = *config.configuration_;
    }
    if ( config.statusConfig_ )
    {
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_[ resourceId ].statusConfig_ = *config.statusConfig_;
    }
    if ( config.alarmConfig_ )
    {
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_[ resourceId ].alarmConfig_ = *config.alarmConfig_;
    }

    if ( config.physicalPath_ )
    {
        filteredConfig_->profiles_[ profileId ].streams_[ streamId ]
            .resourceDescription_[ resourceId ].physicalPath_ = *config.physicalPath_;
    }
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::configureResourceConnectionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t streamId,
        const uint16_t connectionId,
        const ConnectionItemConfig& config )
{

    ensureResourceConnectionInFilteredConfig( profileId, streamId, connectionId );
    storeConnectionItemConfig( filteredConfig_->profiles_[ profileId ]
            .streams_[ streamId ].resourceConnections_[connectionId]
            , config );
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::configureStreamConnectionInFilteredConfig( 
        const uint16_t profileId,
        const uint16_t connectionId,
        const StreamConnectionItemConfig& config )
{
    ensureStreamConnectionInFilteredConfig( profileId, connectionId );

    StreamConnectionItem& connItem = 
        filteredConfig_->profiles_[ profileId ].streamConnections_[connectionId];

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

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::clearFilteredConfig()
{
    filteredConfig_->clear();
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::checkXpathFilteredConfig(
        const string& xPath) const
{
    XMLContentChecker<XPO3Container> checker( filteredConfig_ );
    queryEngine_->tryGetConfigXpath( primarySession_, xPath, 
            writeableDbName_, checker );
}

// ---------------------------------------------------------------------------|
void DeviceGetModuleFixture::checkSubtreeFilteredConfig(
        const string& subtree) const
{
    XMLContentChecker<XPO3Container> checker( filteredConfig_ );
    queryEngine_->tryGetConfigSubtree( primarySession_, subtree, 
            writeableDbName_, checker );
}

} // namespace XPO3NetconfIntegrationTest

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
