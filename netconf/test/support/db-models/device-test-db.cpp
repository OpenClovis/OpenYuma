// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/db-models/device-test-db.h"
#include "test/support/misc-util/ptree-utils.h"
#include "test/support/misc-util/base64.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

#include "test/support/db-models/db-check-utils.h"


// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;
using boost::property_tree::ptree;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ===========================================================================|
ConnectionItem::ConnectionItem() : id_(0)
                                 , sourceId_(0)
                                 , sourcePinId_(0)
                                 , destinationId_(0)
                                 , destinationPinId_(0)
                                 , bitrate_(0)
{}

// ---------------------------------------------------------------------------|
bool ConnectionItem::unpackItem( const ptree::value_type& v )
{
    bool res = true;

    if ( v.first == "id" )
    {
        id_ = v.second.get_value<uint32_t>();
    }
    else if ( v.first == "sourceId" )
    {
        sourceId_ = v.second.get_value<uint32_t>();
    }
    else if ( v.first == "sourcePinId" )
    {
        sourcePinId_ = v.second.get_value<uint32_t>();
    }
    else if ( v.first == "destinationId" )
    {
        destinationId_ = v.second.get_value<uint32_t>();
    }
    else if ( v.first == "destinationPinId" )
    {
        destinationPinId_ = v.second.get_value<uint32_t>();
    }
    else if ( v.first == "bitrate" )
    {
        bitrate_ = v.second.get_value<uint32_t>();
    }
    else
    {
        res = false;
    }

    return res;
}
// ---------------------------------------------------------------------------|
ConnectionItem::ConnectionItem( const boost::property_tree::ptree& pt )
    : id_(0)
    , sourceId_(0)
    , sourcePinId_(0)
    , destinationId_(0)
    , destinationPinId_(0)
    , bitrate_(0)
{
    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( !unpackItem( v ) )
        {
            BOOST_FAIL( "Unsupported child for ConnectionItem: " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const ConnectionItem& lhs, const ConnectionItem& rhs )
{
    BOOST_CHECK_EQUAL( lhs.id_, rhs.id_ );
    BOOST_CHECK_EQUAL( lhs.sourceId_, rhs.sourceId_ );
    BOOST_CHECK_EQUAL( lhs.sourcePinId_, rhs.sourcePinId_ );
    BOOST_CHECK_EQUAL( lhs.destinationId_, rhs.destinationId_ );
    BOOST_CHECK_EQUAL( lhs.destinationPinId_, rhs.destinationPinId_ );
    BOOST_CHECK_EQUAL( lhs.bitrate_, rhs.bitrate_ );
}

// ===========================================================================|
StreamConnectionItem::StreamConnectionItem() : ConnectionItem()
                                 , sourceStreamId_(0)
                                 , destinationStreamId_(0)
{}

// ---------------------------------------------------------------------------|
StreamConnectionItem::StreamConnectionItem( const boost::property_tree::ptree& pt )
    : ConnectionItem()
    , sourceStreamId_(0)
    , destinationStreamId_(0)
{
    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( !unpackItem( v ) )
        {
            if ( v.first == "sourceStreamId" )
            {
                sourceStreamId_ = v.second.get_value<uint32_t>();
            }
            else if ( v.first == "destinationStreamId" )
            {
                destinationStreamId_ = v.second.get_value<uint32_t>();
            }
            else 
            {
                BOOST_FAIL( "Unsupported child for StreamConnectionItem: " << v.first );
            }
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const StreamConnectionItem& lhs, const StreamConnectionItem& rhs )
{
    checkEqual( static_cast<const ConnectionItem&>( lhs ), 
                static_cast<const ConnectionItem&>( rhs ) );

    BOOST_CHECK_EQUAL( lhs.sourceStreamId_, rhs.sourceStreamId_ );
    BOOST_CHECK_EQUAL( lhs.destinationStreamId_, rhs.destinationStreamId_ );
}

// ===========================================================================|
ResourceNode::ResourceNode() : id_(0)
                             , channelId_(0)
                             , resourceType_(0)
                             , configuration_()
                             , statusConfig_()
                             , alarmConfig_()
                             , physicalPath_()
{}

// ---------------------------------------------------------------------------|
ResourceNode::ResourceNode( const boost::property_tree::ptree& pt )
    : id_(0)
    , channelId_(0)
    , resourceType_()
    , configuration_()
    , statusConfig_()
    , alarmConfig_()
    , physicalPath_()
{
    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( v.first == "id" )
        {
            id_ = v.second.get_value<uint16_t>();
        }
        else if ( v.first == "channelId" )
        {
            channelId_ = v.second.get_value<uint32_t>();
        }
        else if ( v.first == "resourceType" )
        {
            resourceType_ = v.second.get_value<uint32_t>();
        }
        else if ( v.first == "configuration" )
        {
            configuration_ = base64_decode( v.second.get_value<string>() );
        }
        else if ( v.first == "statusConfig" )
        {
            statusConfig_ = base64_decode( v.second.get_value<string>() );
        }
        else if ( v.first == "alarmConfig" )
        {
            alarmConfig_ = base64_decode( v.second.get_value<string>() );
        }
        else if ( v.first == "physicalPath" )
        {
            physicalPath_ = v.second.get_value<string>();
        }
        else
        {
            BOOST_FAIL( "Unsupported child for ResourceNode: " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const ResourceNode& lhs, const ResourceNode& rhs )
{
    BOOST_CHECK_EQUAL( lhs.id_, rhs.id_ );
    BOOST_CHECK_EQUAL( lhs.channelId_, rhs.channelId_ );
    BOOST_CHECK_EQUAL( lhs.resourceType_, rhs.resourceType_ );
    BOOST_CHECK_EQUAL( lhs.configuration_, rhs.configuration_ );
    BOOST_CHECK_EQUAL( lhs.statusConfig_, rhs.statusConfig_ );
    BOOST_CHECK_EQUAL( lhs.alarmConfig_, rhs.alarmConfig_ );
    BOOST_CHECK_EQUAL( lhs.physicalPath_, rhs.physicalPath_ );
}

// ===========================================================================|
StreamItem::StreamItem() : id_(0)
                         , resourceDescription_()
                         , resourceConnections_()
{}

// ---------------------------------------------------------------------------|
StreamItem::StreamItem( const boost::property_tree::ptree& pt )
    : id_(0)
    , resourceDescription_()
    , resourceConnections_()
{

    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( v.first == "id" )
        {
            id_ = v.second.get_value<uint16_t>();
        }
        else if ( v.first == "resourceNode" )
        {
            ResourceNode res( v.second );

            BOOST_REQUIRE_MESSAGE( 
                    resourceDescription_.end() 
                    == resourceDescription_.find( res.id_ ),
                    "ERROR Duplicate Resource found! stream ( " << id_ 
                    << ") resource ( " << res.id_ << " )" );

            resourceDescription_.insert( make_pair( res.id_, res ) );
        }
        else if ( v.first == "resourceConnection" )
        {
            YumaTest::ConnectionItem conn( v.second );
    
            BOOST_REQUIRE_MESSAGE( 
                resourceConnections_.end() == resourceConnections_.find( conn.id_ ),
                "ERROR Duplicate connection item found! ID: " << conn.id_);
    
            resourceConnections_.insert( make_pair( conn.id_, conn ) );
        }
        else
        {
            BOOST_FAIL( "Unsupported child for stream: " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const StreamItem& lhs, const StreamItem& rhs )
{
    BOOST_TEST_MESSAGE( "Checking StreamItem match..." );

    BOOST_TEST_MESSAGE( "Comparing Stream Id ... " << lhs.id_ );
    BOOST_CHECK_EQUAL( lhs.id_,  rhs.id_ );

    BOOST_TEST_MESSAGE( "Comparing Stream Resource Descriptions ..." );
    CheckMaps( lhs.resourceDescription_, rhs.resourceDescription_ );

    BOOST_TEST_MESSAGE( "Comparing Stream Virtual Connection Resources ..." );
    CheckMaps( lhs.resourceConnections_, rhs.resourceConnections_ );
}

// ===========================================================================|
Profile::Profile() 
    : id_(0)
    , streams_()
    , streamConnections_()
{}


// ===========================================================================|
Profile::Profile( const boost::property_tree::ptree& pt)
    : id_(0)
    , streams_()
    , streamConnections_()
{
    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( v.first == "id" )
        {
            id_ = v.second.get_value<uint16_t>();
        }
        else if ( v.first == "stream" )
        {
            StreamItem str( v.second );

            BOOST_REQUIRE_MESSAGE( streams_.end() == streams_.find( str.id_ ),
                    "ERROR Duplicate Stream found! profile ( " << id_ 
                    << ") stream ( " << str.id_ << " )" );
            streams_.insert( make_pair( str.id_, str ) );
        }
        else if ( v.first == "streamConnection" )
        {
            YumaTest::StreamConnectionItem conn( v.second );
    
            BOOST_REQUIRE_MESSAGE( 
                streamConnections_.end() == streamConnections_.find( conn.id_ ),
                "ERROR Duplicate stream connection item found! ID: " << conn.id_);
    
            streamConnections_.insert( make_pair( conn.id_, conn ) );
        }
        else
        {
            BOOST_FAIL( "Unsupported child for profile: " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const Profile& lhs, const Profile& rhs )
{
    BOOST_TEST_MESSAGE( "Checking Profile match..." );

    BOOST_TEST_MESSAGE( "Comparing Profile Id ..." << lhs.id_ );
    BOOST_CHECK_EQUAL( lhs.id_,  rhs.id_ );

    BOOST_TEST_MESSAGE( "Comparing Streams List ..." );
    CheckMaps( lhs.streams_, rhs.streams_ );

    BOOST_TEST_MESSAGE( "Comparing Stream Connections List ..." );
    CheckMaps( lhs.streamConnections_, rhs.streamConnections_ );
}

// ===========================================================================|
XPO3Container::XPO3Container() : activeProfile_(0)
                               , profiles_()
{}

// ---------------------------------------------------------------------------|
XPO3Container::XPO3Container( const ptree& pt) : activeProfile_(0)
                                               , profiles_()
{
    const ptree& xpo = pt.get_child( "data.xpo", ptree() );

    BOOST_FOREACH( const ptree::value_type& v, xpo )
    {
        if ( v.first == "profile" )
        {
            Profile prof( v.second ); 

            BOOST_REQUIRE_MESSAGE( profiles_.end() == profiles_.find( prof.id_ ),
                    "ERROR Duplicate Profile with id( " << prof.id_ << " )\n" );
            profiles_.insert( std::make_pair( prof.id_, prof ) );
        }
        else if ( v.first == "activeProfile" )
        {
            activeProfile_ = v.second.get_value<uint16_t>();
        }
        else if ( !boost::starts_with( v.first, "<xmlattr" ) )
        {
            BOOST_FAIL( "Unsupported child for XPO3:  " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void XPO3Container::clear()
{
    activeProfile_ = 0;
    profiles_.clear();
}

// ---------------------------------------------------------------------------|
void checkEqual( const XPO3Container& lhs, const XPO3Container& rhs )
{
    BOOST_TEST_MESSAGE( "Checking XPO3Container match..." );
    BOOST_TEST_MESSAGE( "Comparing Active Profile Id ..." );
    BOOST_CHECK_EQUAL( lhs.activeProfile_,  rhs.activeProfile_ );

    BOOST_TEST_MESSAGE( "Comparing Profile List ..." );
    CheckMaps( lhs.profiles_, rhs.profiles_ );
}

} // namespace XPO3NetconfIntegrationTest
