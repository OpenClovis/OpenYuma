 #ifndef __XPO_QUERY_BUILDER_H
#define __XPO_QUERY_BUILDER_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/msg-util/NCMessageBuilder.h"
#include "test/support/db-models/device-test-db.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Utility lass for build queries against the device test XPO
 * container.
 */
class XPOQueryBuilder : public NCMessageBuilder
{
public:
    /** 
     * Constructor.
     *
     * \param moduleNs the namespace of the module defining the XPO
     * container.
     */
    explicit XPOQueryBuilder( const std::string& moduleNS );

    /** Destructor */
    virtual ~XPOQueryBuilder();

    /**
     * Add the profile node path and the xpo module to the XML query.
     *
     * \param profileId the id of the profile
     * \param queryText the query to add the path to.
     * \param op option operation type.
     * \return the formatted xml query,
     */
    std::string addProfileNodePath( 
        uint16_t profileId,
        const std::string& queryText,
        const std::string& op="" ) const;

    /** 
     * Add the path to the resource node (profile.stream.resource)
     *
     * \param profileId the of the owning profile.
     * \param streamId the of the stream to create. 
     * \param resourceId the id of the connection node to create.
     * \param queryText the resource query to add the path to.
     * \param op option operation type.
     * \return XML formatted query
     */
    std::string addProfileStreamNodePath( 
        uint16_t profileId,
        uint16_t streamId,
        const std::string& queryText,
        const std::string& op="" ) const;

    /** 
     * Add the path to the stream connection (streamConnection)
     *
     * \param profileId the of the owning profile.
     * \param streamId the of the stream to create. 
     * \param queryText the resource query to add the path to.
     * \param op option operation type.
     * \return XML formatted query
     */
    std::string addStreamConnectionPath( 
        uint16_t profileId,
        uint16_t connectionId,
        const std::string& queryText,
        const std::string& op="" ) const;

    /** 
     * Add the path to the resource node (profile.stream.resource).
     *
     * \param profileId the id of the owning profile.
     * \param streamId the id of the owning stream.
     * \param resourceId the id of the resource node.
     * \param queryText the resource query to add the path to.
     * \param op option operation type.
     * \return XML formatted query
     */
    std::string addResourceNodePath( 
        uint16_t profileId,
        uint16_t streamId,
        uint16_t resourceId,
        const std::string& queryText,
        const std::string& op="" ) const;

    /** 
     * Add the path to the VR connection node
     * (profile.stream.connection).
     *
     * \param profileId the id of the owning profile.
     * \param streamId the id of the owning stream.
     * \param connectionId the id of the connection node.
     * \param queryText the resource query to add the path to.
     * \param op option operation type.
     * \return XML formatted query
     */
    std::string addVRConnectionNodePath( 
        uint16_t profileId,
        uint16_t streamId,
        uint16_t connectionId,
        const std::string& queryText,
        const std::string& op="" ) const;

    /**
     * Generate a query on the basic XPO container.
     *
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genXPOQuery( const std::string& op ) const;

    /**
     * Generate a query to set the active profile id.
     *
     * \param profileId the id of the new active profile
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genSetActiveProfileIdQuery( uint16_t profileId,
           const std::string& op ) const;

    /**
     * Generate a query for a specific profile.
     *
     * \param profileId the id of the profile
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genProfileQuery( uint16_t profileId, const std::string& op ) const;

    /**
     * Generate a query for a specific stream item in a profile.
     *
     * \param profileId the id of the profile
     * \param streamId the id of the stream
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genProfileChildQuery( uint16_t profileId, 
                                      uint16_t streamId, 
                                      const std::string& op ) const;
    /** 
     * Generate a query for a child node of profile.
     *
     * \param profileId the of the owning profile.
     * \param streamId the of the stream to create. 
     * \param nodeName the name of the child node to create. 
     * \param nodeId the id of the child to create. 
     * \param resourceNodeId the id of the resource node to create.
     * \return XML formatted query
     */
    std::string genProfileChildQuery( uint16_t profileId,
                                      uint16_t streamId,
                                      const std::string&  nodeName,
                                      uint16_t nodeId,
                                      const std::string& op ) const;
    
    /**
     * Configure a resource connection item.
     * This function generates the complete configuration for a
     * ConnectionItem, items are configured depending on whether or
     * not they have been set in the supplied ConnectionItemConfig.
     *
     * \param config the configuration to generate
     * \param op the operation being performed
     * \return XML representation of the data being configured.
     */
     std::string configureResourceConnection( 
        const YumaTest::ConnectionItemConfig& config,
        const std::string& op ) const;

    /**
     * Configure a stream connection item.
     * This function generates the complete configuration for a
     * ConnectionItem, items are configured depending on whether or
     * not they have been set in the supplied StreamConnectionItemConfig.
     *
     * \param config the configuration to generate
     * \param op the operation being performed
     * \return XML representation of the data being configured.
     */
     std::string configureStreamConnection( 
        const YumaTest::StreamConnectionItemConfig& config,
        const std::string& op ) const;
   /**
     * Configure a Virtual resource item.
     * This function generates the complete configuration for a
     * ResourceNode, items are configured depending on whether or
     * not they have been set in the supplied ResourceNodeConfig.
     *
     * \param config the configuration to generate
     * \param op the operation being performed
     * \return XML representation of the data being configured.
     */
     std::string configureVResourceNode( 
         const YumaTest::ResourceNodeConfig& config, 
         const std::string& op ) const;

    /**
     * Generate a query for a specific stream.
     *
     * \param profileId the id of the stream
     * \param streamId the id of the stream
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genStreamConnectionQuery( 
           uint16_t profileId,
           uint16_t streamId,
           const std::string& op ) const;

    /**
     * Generate a query for a specific stream in a profile.
     *
     * \param profileId the id of the stream
     * \param streamId the id of the stream
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genProfileStreamItemQuery( uint16_t profileId,
                                            uint16_t streamId,
                                            const std::string& op ) const;

    /**
     * Generate a custom RPC query
     *
     * \param rpcName the name of the RPC
     * \param params a string contaiing the xml formatted parameters
     *               for the RPC
     * \return XML formatted query
     */
    std::string buildCustomRPC( const std::string& rpcName, 
                                const std::string& params ) const;

private:
    /**
     *
     * Generate the ConnectionItemConfig part of the supplied config.
     * \tparam T a class that mus supply the member variables:
     *           sourceId_
     *           sourcePinId_
     *           destinationId_
     *           destinationPinId_
     *           bitrate_
     * \param config the config
     * \param op the operation being performed
     * \return XML representation of the data being configured.
     */
    template <class T>
    std::string configureConnectionItem( const T& config, 
                                         const std::string& op ) const;

protected:
     const std::string moduleNs_; ///< the module's namespace
};

} // namespace YumaTest

#endif // __XPO_QUERY_BUILDER_H
