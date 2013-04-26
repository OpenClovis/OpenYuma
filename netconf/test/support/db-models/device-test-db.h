#ifndef __DEVICE_TEST_DATABASE_H
#define __DEVICE_TEST_DATABASE_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <map>
#include <string>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
struct ResourceNode;
struct ConnectionItem;
struct StreamItem;

// ---------------------------------------------------------------------------|
/* Convenience typedefs. */
typedef boost::optional<uint32_t> opt_uint32_t;
typedef boost::optional<std::string> opt_string_t;

// ---------------------------------------------------------------------------|
struct ConnectionItem
{
    uint32_t id_;                   ///< The id 
    uint32_t sourceId_;             ///< the id of the source 
    uint32_t sourcePinId_;          ///< additional source data
    uint32_t destinationId_;        ///< the id of the destination 
    uint32_t destinationPinId_;     ///< additional destination data
    uint32_t bitrate_;              ///< the bitrate of the connection

    /** Constructor */
    ConnectionItem();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit ConnectionItem( const boost::property_tree::ptree& pt );

    /**
     * Unpack a property tree value.
     *
     * \param v the value to unpack
     * \return true if the item was unpacked.
     */
    bool unpackItem( const boost::property_tree::ptree::value_type& v );
};

// ---------------------------------------------------------------------------|
/**
 * Utility class to support test harness setting of Connection Items.
 * This class supplies optional parameters for each element in a
 * connectionItem. 
 *
 * Example to only set the destination details and the bitrate,
 * use this class as follows:
 *
 * ConnectionItemConfig connCfg{ boost::optional<uint32_t>(),
 *                               boost::optional<uint32_t>(),
 *                               10,
 *                               100,
 *                               1000 };
 */
struct ConnectionItemConfig
{
    opt_uint32_t sourceId_;             ///< optional value for sourceId
    opt_uint32_t sourcePinId_;          ///< optional value for sourcePinId
    opt_uint32_t destinationId_;        ///< optional value for destinationId_
    opt_uint32_t destinationPinId_;     ///< optional value for destinationPinId_
    opt_uint32_t bitrate_;              ///< optional value for bitrate
};

/**
 * Check if two ConnectionItems are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const ConnectionItem& lhs, const ConnectionItem& rhs );

// ---------------------------------------------------------------------------|
struct StreamConnectionItem : public ConnectionItem
{
    uint32_t sourceStreamId_;       ///< the id of the source endpoint
    uint32_t destinationStreamId_;  ///< the is of the destination endpoint

    /** Constructor */
    StreamConnectionItem();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit StreamConnectionItem( const boost::property_tree::ptree& pt );
};

// ---------------------------------------------------------------------------|
/**
 * Utility class to support test harness setting of Stream Connection Items.
 * This class supplies optional parameters for each element in a
 * connectionItem. 
 *
 * note: this class connot be derived from ConnectionItemConfig
 * because the use of inheritance prevents easy initialisation, which
 * defeats the purpose of the class!
 */
struct StreamConnectionItemConfig 
{
    opt_uint32_t sourceId_;             ///< optional value for sourceId
    opt_uint32_t sourcePinId_;          ///< optional value for sourcePinId
    opt_uint32_t destinationId_;        ///< optional value for destinationId_
    opt_uint32_t destinationPinId_;     ///< optional value for destinationPinId_
    opt_uint32_t bitrate_;              ///< optional value for bitrate
    opt_uint32_t sourceStreamId_;       ///< optional value for source stream Id
    opt_uint32_t destinationStreamId_;  ///< optional value for destination stream id
};

/**
 * Check if two ConnectionItems are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const StreamConnectionItem& lhs, const StreamConnectionItem& rhs );

// ---------------------------------------------------------------------------|
struct ResourceNode
{
    uint32_t id_;                    ///< The id of the resource node
    uint32_t channelId_;             ///< The id of the channel 
    uint32_t resourceType_;          ///< the type of resource
    std::string configuration_;      ///< binary configuration data
    std::string statusConfig_;       ///< binary status configuration data
    std::string alarmConfig_;        ///< binary alarm configuration data
    std::string physicalPath_;       ///< that name of the path

    /** Constructor */
    ResourceNode();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit ResourceNode( const boost::property_tree::ptree& pt );
};

// ---------------------------------------------------------------------------|
/** convenience typedef. */
typedef std::map<uint32_t, ResourceNode> ResourceDescriptionMap;

// ---------------------------------------------------------------------------|
/**
 * Utility class to support test harness setting of Resource Nodes.
 * This class supplies optional parameters for each element in a
 * connectionItem. 
 *
 * Example to only set the destination details and the bitrate,
 * use this class as follows:
 *
 * ResourceNodeConfig cfg{ boost::optional<uint32_t>(),
 *                         "physical path" };
 */
struct ResourceNodeConfig
{
    opt_uint32_t resourceType_;     ///< optional value for resourceType_
    opt_string_t configuration_;    ///< optional value for configuration_
    opt_string_t statusConfig_;     ///< optional value for statusConfig_
    opt_string_t alarmConfig_;      ///< optional value for alarmConfig_
    opt_string_t physicalPath_;     ///< optional value for physicalPath_
};

/**
 * Check if two ResourceNodes are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const ResourceNode& lhs, const ResourceNode& rhs );

// ---------------------------------------------------------------------------|
struct StreamItem
{
    typedef std::map<uint32_t, ConnectionItem> ResourceConnections_type;

    uint32_t id_;         ///< the id of the profile.
    ResourceDescriptionMap resourceDescription_;    ///< resource description details
    ResourceConnections_type resourceConnections_;    /// resource connections

    /** Constructor */
    StreamItem();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit StreamItem( const boost::property_tree::ptree& pt );
};

/**
 * Check if two Profiles are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const StreamItem& lhs, const StreamItem& rhs );

// ---------------------------------------------------------------------------|
struct Profile
{
    typedef std::map< uint32_t, StreamItem >              StreamsMap_type; 
    typedef std::map< uint32_t, StreamConnectionItem >    StreamConnectionMap_type;

    uint32_t id_;                ///< the id of the profile.
    StreamsMap_type streams_;    ///< the profiles streams
    StreamConnectionMap_type streamConnections_; ///< the list of stream connections.

    /** Constructor. */
    Profile();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit Profile( const boost::property_tree::ptree& pt );
};

/**
 * Check if two Profiles are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const Profile& lhs, const Profile& rhs );

// ---------------------------------------------------------------------------|
struct XPO3Container
{
    typedef std::map<uint32_t, Profile> ProfilesMap_type;

    uint32_t activeProfile_;                     ///< the id of the active profile. 
    ProfilesMap_type profiles_;                  ///< the list of profiles 

    /** Constructor */
    XPO3Container();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     * construct. 
     */
    explicit XPO3Container( const boost::property_tree::ptree& pt);

    /** 
     * Clear the contents of the container.
     */
    void clear();
};

/**
 * Check if two XPO3Containers are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const XPO3Container& lhs, const XPO3Container& rhs );

} // namespace YumaTest

#endif // __DEVICE_TEST_DATABASE_H

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
