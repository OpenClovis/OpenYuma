#ifndef __DEVICE_MODULE_COMMON_FIXTURE_H
#define __DEVICE_MODULE_COMMON_FIXTURE_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
#include "test/support/msg-util/xpo-query-builder.h"
#include "test/support/db-models/device-test-db.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <map>
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractNCSession;
class XPO3Container;

// ---------------------------------------------------------------------------|
//
/// @brief This class is used to perform test case initialisation for testing
/// with the Device Test module.
/// @desc This class should be used as a base class for test suites
/// fuixtures that need to use common DeviceModule access code.
//
struct DeviceModuleCommonFixture : public YumaTest::QuerySuiteFixture
{
public:
    /// @brief Constructor. 
    explicit DeviceModuleCommonFixture( const std::string& moduleNs );

    /// @brief Destructor. Shutdown the test.
    ~DeviceModuleCommonFixture();

    /// @brief Create the xpo level container.
    ///
    /// @param session the session running the query
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void createXpoContainer( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            const std::string& failReason = std::string() );

    /// @brief Delete the xpo level container.
    ///
    /// @param session the session running the query
    /// @param commitChange flag indicating if the change should be committed.
    /// @param failReason optional expected failure reason.
    void deleteXpoContainer( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            bool commitChange = true,
            const std::string& failReason = std::string() );

    /// @brief Set the active profile
    ///
    /// @param session the session running the query
    /// @param profileId the id of the active profile
    /// @param failReason optional expected failure reason.
    void setActiveProfile(
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            const std::string& failReason = std::string() );

    /// @brief Perform an xpo profile query operation.
    ///
    /// @param session the session running the query.
    /// @param profileId the is of the profile.
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void xpoProfileQuery( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Create a StreamItem for a profile.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void profileStreamQuery( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            uint16_t streamId,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Create a resource node.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param resourceNodeId the id of the resource node to create.
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void resourceDescriptionQuery( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            uint16_t streamId,
            uint16_t resourceNodeId,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Create a VR connection node .
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param connectionId the id of the connection node to create.
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void resourceConnectionQuery( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            uint16_t streamId,
            uint16_t connectionId,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Configure a Virtual resource item.
    /// @desc This function generates the complete configuration for a 
    /// ResourceNode, items are configured depending on whether or 
    /// not they have been set in the supplied ResourceNodeConfig.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param resourceId the id of the resource node to create.
    /// @param config the configuration to generate
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void configureResourceDescrption( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            uint16_t streamId,
            uint16_t resourceId,
            const YumaTest::ResourceNodeConfig& config,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Configure a virtual resource connection item.
    /// @desc This function generates the complete configuration for a 
    /// ConnectionItem, items are configured depending on whether or 
    /// not they have been set in the supplied ConnectionItemConfig.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param connectionId the id of the connection node to create.
    /// @param config the configuration to generate
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void configureResourceConnection( 
             std::shared_ptr<YumaTest::AbstractNCSession> session,
             uint16_t profileId,
             uint16_t streamId,
             uint16_t connectionId,
             const YumaTest::ConnectionItemConfig& config,
             const std::string& op = std::string( "create" ),
             const std::string& failReason = std::string() );

    /// @brief Configure a stream connection item.  
    /// @desc This function generates the complete configuration for a 
    /// ConnectionItem, items are configured depending on whether or not 
    /// they have been set in the supplied ConnectionItemConfig.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param connectionId the id of the connection node to create.
    /// @param config the configuration to generate
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void configureStreamConnection( 
             std::shared_ptr<YumaTest::AbstractNCSession> session,
             uint16_t profileId,
             uint16_t connectionId,
             const YumaTest::StreamConnectionItemConfig& config,
             const std::string& op = std::string( "create" ),
             const std::string& failReason = std::string() );

    /// @brief Create a new stream connection.
    ///
    /// @param session the session running the query.
    /// @param profileId the of the owning profile.
    /// @param id the id of the stream
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void streamConnectionQuery(
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            uint16_t profileId,
            uint16_t id,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Commit the changes.
    ///
    /// @param session  the session requesting the locks
    virtual void commitChanges( 
            std::shared_ptr<YumaTest::AbstractNCSession> session );

    /// @brief Discard all changes locally.
    /// @desc Let the test harness know that changes should be discarded 
    /// (e.g. due to unlocking the database without a commit.
    void discardChanges();

    /// @brief Check the configration. 
    /// @desc Get the contents of the candidate database and verify 
    /// that all entries are as expected.
    virtual void checkConfig();

    /// @brief Initialise 2 profiles for use in most test scenarios. 
    /// @desc This function creates 2 profiles with the following
    /// format:
    ///
    /// profile<N>
    ///   stream<1>
    ///     virtual-resource<1>
    ///     virtual-resource<2>
    ///     virtual-resource<3>
    ///     virtual-connection<1>
    ///     virtual-connection<2>
    ///   stream<2>
    ///     virtual-resource<1>
    ///     virtual-resource<2>
    ///     virtual-resource<3>
    ///     virtual-connection<1>
    ///     virtual-connection<2>
    ///   stream-connection<1>
    /// 
    void initialise2Profiles();

protected:
    /// @brief Store connection configuration data in a connection item.
    ///
    /// @tparam ConnectionItemType the connection item type
    /// @tparam ConfigType the config item type
    /// @param connectionItem the connection item being updated
    /// @param config the new configuration.
    template< class ConnectionItemType, class ConfigType >
    void storeConnectionItemConfig( 
        ConnectionItemType& connectionItem, 
        const ConfigType& config );

    /// @brief Update the stream connection entry in the expected data.  
    /// @desc Utility function to ensure that the connection exists in the 
    /// candidateEntries_ tree. If the item does not exist it will be created, 
    /// initialised and added.
    ///
    /// @param profileId the of the profile.
    /// @param connectionId the of the connection.
    void ensureStreamConnectionExists( uint16_t profileId,
                                       uint16_t connectionId );

    /// @brief Update the profile entry in the expected data.  
    /// Utility function to ensure that the profile exists in the
    /// candidateEntries_ tree. If the item does not exist it will be created, 
    /// initialised and added.
    ///
    /// @param profileId the of the connection.
    void ensureProfileExists( uint16_t profileId );

    /// @brief Update the stream entry in the expected data.
    /// @desc Utility function to ensure that the stream exists for the 
    /// profile in the candidateEntries_ tree. If the item does not exist
    /// it will be created, initialised and added.
    ///
    /// @param profileId the of the profile.
    /// @param streamId the of the connection.
    void ensureProfileStreamExists( uint16_t profileId, 
                                    uint16_t streamId );

    /// @brief Ensure a resource description node exists.
    /// @param resourceMap the map to create teh resouce for.
    /// @param resourceId the of the resource.
    void ensureResourceExists( ResourceDescriptionMap& resourceMap,
                               uint32_t resourceId );

    /// @brief Update the resource description entry in the expected data.
    /// @desc Utility function to ensure that the resource exists for the
    /// stream of a profile in the candidateEntries_ tree. If the item does
    /// not exist it will be created, initialised and added.
    ///
    /// @param profileId the of the profile.
    /// @param streamId the of the connection.
    /// @param resourceId the of the resource.
    void ensureResourceDescriptionExists( uint16_t profileId,
                                          uint16_t streamId, 
                                          uint16_t resourceId );
    
    /// @brief Update the resource connection entry in the expected data.
    /// @desc Utility function to ensure that the connection exists for the
    /// stream of a profile in the candidateEntries_ tree. If the item does
    /// not exist it will be created, initialised and added.
    ///
    /// @param profileId the of the connection.
    /// @param streamId the of the connection.
    /// @param connectionId the of the connection.
    void ensureResourceConnectionExists( uint16_t profileId,
                                         uint16_t streamId, 
                                         uint16_t connectionId );

    /// @brief Utility fuction for checking the contents of a configuartion
    ///
    /// @param dbName the name of the netconf database to check
    /// @param db the expected contents.
    virtual void checkEntriesImpl( const std::string& dbName,
        const std::shared_ptr<YumaTest::XPO3Container>& db );

    /// @brief Utility function to query a configuration.
    /// @desc this function can be used for debugging purposes. It
    /// queries the specified config and as a result the configuration
    /// xml will be dumped into the test output file SesXXXX.
    ///
    /// @param dbName the name of the netconf database to check
    void queryConfig( const string& dbName ) const;

    /// @param resourceNode the resource node to update.
    /// @param config the configuration to generate
    /// @param op the operation type
    void updateResourceDescriptionEntry( 
             YumaTest::ResourceNode& resourceNode,
             const YumaTest::ResourceNodeConfig& config );

protected:
    ///< The builder for the generating netconf messages in xml format. 
    std::shared_ptr< YumaTest::XPOQueryBuilder > xpoBuilder_;

    std::shared_ptr<YumaTest::XPO3Container> runningEntries_;    ///< the running data
    std::shared_ptr<YumaTest::XPO3Container> candidateEntries_;  ///< the candidate data
};

} // namespace YumaTest 

#endif // __DEVICE_MODULE_COMMON_FIXTURE_H
//------------------------------------------------------------------------------
