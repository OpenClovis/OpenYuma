// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/db-models/device-test-db-debug.h"
#include "test/support/db-models/device-test-db.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iomanip>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace 
{

static uint8_t indent;

} // anonymous namespace

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const StreamConnectionItem& it )
{
    o << setw(indent) << setfill( ' ' ) <<
        "StreamConnectionItem = { id_ = " << it.id_ << 
                              ", sourceId=" << it.sourceId_ <<
                              ", sourcePinId_=" << it.sourcePinId_ <<
                              ", destinationId_=" << it.destinationId_ <<
                              ", destinationPinId_=" << it.destinationPinId_ <<
                              ", bitrate_=" << it.bitrate_ << 
                              ", sourceStreamId_=" << it.sourceStreamId_ << 
                              ", destinationStreamId_=" << it.destinationStreamId_ << 
                              " }\n";
    return o;
}

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const ConnectionItem& it )
{
    o << setw(indent) << setfill( ' ' ) <<
            "ConnectionItem = { id_ = " << it.id_ << 
                              ", sourceId=" << it.sourceId_ <<
                              ", sourcePinId_=" << it.sourcePinId_ <<
                              ", destinationId_=" << it.destinationId_ <<
                              ", destinationPinId_=" << it.destinationPinId_ <<
                              ", bitrate_=" << it.bitrate_ << 
                              " }\n";
    return o;
}

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const ResourceNode& it )
{
    o << setw(indent) << setfill( ' ' ) <<
            "ResourceNode = { id_ = " << it.id_ << 
                              ", channelId_=" << it.channelId_ <<
                              ", resourceType=" << it.resourceType_ <<
                              ", configuration=" << it.configuration_ <<
                              ", alarmConfig=" << it.alarmConfig_ <<
                              ", statusConfig=" << it.statusConfig_ <<
                              ", physicalPath_=" << it.physicalPath_ <<
                              " }\n";
    return o;
}

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const StreamItem& it )
{
    o << setw(indent) << setfill( ' ' ) <<
            "StreamItem = { id_ = " << it.id_ << "\n";
    indent +=2;
    BOOST_FOREACH( const ResourceDescriptionMap::value_type& val,
            it.resourceDescription_ )
    { 
        o << val.second;
    }

    BOOST_FOREACH( const StreamItem::ResourceConnections_type::value_type& val,
            it.resourceConnections_ )
    { 
        o << val.second;
    }
    indent -=2;

    o << setw(indent) << setfill( ' ' ) << " }\n";
    return o;
}

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const Profile& it )
{
    o << setw(indent) << setfill( ' ' ) <<
            "Profile = { id_ = " << it.id_ << "\n";
    indent +=2;
    BOOST_FOREACH( const Profile::StreamsMap_type::value_type& val,
            it.streams_ )
    { 
        o << val.second;
    }

    BOOST_FOREACH( const Profile::StreamConnectionMap_type::value_type& val,
            it.streamConnections_ )
    { 
        o << val.second;
    }
    indent -=2;

    o << setw(indent) << setfill( ' ' ) << " }\n";
    return o;
}

// ---------------------------------------------------------------------------|
ostream& operator<<( ostream& o,  const XPO3Container& it )
{
    o << setw(indent) << setfill( ' ' ) <<
            "XPO3Container = { activeProfile=" << it.activeProfile_ << "\n";
    indent +=2;
    BOOST_FOREACH( const XPO3Container::ProfilesMap_type::value_type& val,
            it.profiles_ )
    { 
        o << val.second;
    }

    indent -=2;

    o << setw(indent) << setfill( ' ' ) << " }\n";
    return o;
}

} // namespace XPO3NetconfIntegrationTest
