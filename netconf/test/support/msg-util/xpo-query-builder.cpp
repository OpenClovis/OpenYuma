// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/msg-util/xpo-query-builder.h"
#include "test/support/misc-util/base64.h"

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;
using namespace YumaTest;

// ---------------------------------------------------------------------------|
XPOQueryBuilder::XPOQueryBuilder( const string& moduleNs ) 
    : NCMessageBuilder()
    , moduleNs_( moduleNs )
{
}

// ---------------------------------------------------------------------------|
XPOQueryBuilder::~XPOQueryBuilder()
{
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::addProfileNodePath( 
    const uint16_t profileId,
    const string& queryText,
    const string& op ) const
{
    // add the profile path
    string query = genKeyParentPathText( "profile", "id",
            boost::lexical_cast<string>( profileId ) , queryText, op );

    return genModuleOperationText( "xpo", moduleNs_, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::addStreamConnectionPath( 
    const uint16_t profileId,
    const uint16_t connectionId,
    const string& queryText,
    const string& op ) const
{
    // add the profile path
    string query = genKeyParentPathText( "streamConnection", "id",
            boost::lexical_cast<string>( connectionId ) , queryText, op );

    return addProfileNodePath( profileId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::addProfileStreamNodePath( 
    const uint16_t profileId,
    const uint16_t streamId,
    const string& queryText,
    const string& op ) const
{
    // add the stream path
    string query = genKeyParentPathText( "stream", "id",
            boost::lexical_cast<string>( streamId ) , queryText, op );

    // add the profile path
    return addProfileNodePath( profileId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::addResourceNodePath( 
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t resourceId,
    const string& queryText,
    const string& op ) const
{
    // add the resource path
    string query = genKeyParentPathText( "resourceNode", "id",
            boost::lexical_cast<string>( resourceId ) , queryText, op );

    // add the profile and stream path
    return addProfileStreamNodePath( profileId, streamId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::addVRConnectionNodePath( 
    const uint16_t profileId,
    const uint16_t streamId,
    const uint16_t connectionId,
    const string& queryText,
    const string& op ) const
{
    // add the resource path
    string query = genKeyParentPathText( "resourceConnection", "id",
            boost::lexical_cast<string>( connectionId ) , queryText, op );

    // add the profile and stream path
    return addProfileStreamNodePath( profileId, streamId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genXPOQuery( const string& op ) const
{
    return genTopLevelContainerText( "xpo", moduleNs_, op );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genSetActiveProfileIdQuery( const uint16_t profileId,
       const string& op ) const
{
    string query = genOperationText( "activeProfile", profileId, op );

    return genModuleOperationText( "xpo", moduleNs_, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genProfileQuery( const uint16_t profileId,
                                         const string& op ) const
{
    // build the operation
    string query = genKeyOperationText( "profile", "id", 
                boost::lexical_cast<string>( profileId ), op );

    // add the module info
    return genModuleOperationText( "xpo", moduleNs_, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genStreamConnectionQuery( 
        const uint16_t profileId,
        const uint16_t streamId,
        const string& op ) const
{
    // build the operation
    string query = genKeyOperationText( "streamConnection", "id", 
                boost::lexical_cast<string>( streamId ), op );

    // add the module info
    return addProfileNodePath( profileId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genProfileStreamItemQuery( const uint16_t profileId,
                                        const uint16_t streamId,
                                        const string& op ) const
{
    // build the operation
    string query = genKeyOperationText( "stream", "id", 
            boost::lexical_cast<string>( streamId ), op );

    // add the path
    return addProfileNodePath( profileId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::genProfileChildQuery( const uint16_t profileId,
                                              const uint16_t streamId,
                                              const string&  nodeName,
                                              const uint16_t nodeId,
                                              const string& op ) const
{
    // build the core operation
    string query = genKeyOperationText( nodeName, "id", 
            boost::lexical_cast<string>( nodeId ), op );

    // add the stream path
    return addProfileStreamNodePath( profileId, streamId, query );
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::configureVResourceNode( 
    const ResourceNodeConfig& config, 
    const string& op ) const
{
    ostringstream oss;

    if ( config.resourceType_ )
    {
        oss << genOperationText( "resourceType", *config.resourceType_, op );
    }

    if ( config.physicalPath_ )
    {
        oss << genOperationText( "physicalPath", 
                *config.physicalPath_, op );
    }

    if ( config.configuration_ )
    {
        oss << genOperationText( "configuration", 
                base64_encode( *config.configuration_ ), op );
    }

    if ( config.statusConfig_ )
    {
        oss << genOperationText( "statusConfig", 
                base64_encode( *config.statusConfig_ ), op );
    }

    if ( config.alarmConfig_ )
    {
        oss << genOperationText( "alarmConfig", 
                base64_encode( *config.alarmConfig_ ), op );
    }

    return oss.str();
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::configureResourceConnection( 
    const ConnectionItemConfig& config, 
    const string& op ) const
{
    return configureConnectionItem( config, op );
}

// ---------------------------------------------------------------------------|
template <class T>
string XPOQueryBuilder::configureConnectionItem(
    const T& config, 
    const string& op ) const
{
    ostringstream oss;

    if ( config.sourceId_ )
    {
        oss << genOperationText( "sourceId", *config.sourceId_, op );
    }

    if ( config.sourcePinId_ )
    {
        oss << genOperationText( "sourcePinId", *config.sourcePinId_, op );
    }

    if ( config.destinationId_ )
    {
        oss << genOperationText( "destinationId", *config.destinationId_, op );
    }

    if ( config.destinationPinId_ )
    {
        oss << genOperationText( "destinationPinId", *config.destinationPinId_, op );
    }

    if ( config.bitrate_ )
    {
        oss << genOperationText( "bitrate", *config.bitrate_, op );
    }

    return oss.str();
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::configureStreamConnection( 
    const StreamConnectionItemConfig& config, 
    const string& op ) const
{
    ostringstream oss;

    oss << configureConnectionItem( config, op );
    if ( config.sourceStreamId_ )
    {
        oss << genOperationText( "sourceStreamId", *config.sourceStreamId_, op );
    }

    if ( config.destinationStreamId_ )
    {
        oss << genOperationText( "destinationStreamId", *config.destinationStreamId_, op );
    }

    return oss.str();
}

// ---------------------------------------------------------------------------|
string XPOQueryBuilder::buildCustomRPC( const string& rpcName,
                                        const string& params ) const
{
    ostringstream oss;

    oss << "<" << rpcName << " " << genXmlNsText( moduleNs_ );
    if ( params.empty() )
    {
        oss << " />" ;
    }
    else
    {
        oss << ">\n" << params;
        oss << "</" << rpcName << ">";
    }
    return oss.str();
}

} // namespace YumaTest
