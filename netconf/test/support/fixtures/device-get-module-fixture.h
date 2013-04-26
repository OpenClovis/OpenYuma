#ifndef __DEVICE_GET_MODULE_FIXTURE_H
#define __DEVICE_GET_MODULE_FIXTURE_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/device-module-common-fixture.h"
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

//TODO Potential future refactoring - move common device module test code to
//TODO parent class. 

// ---------------------------------------------------------------------------|
//
/// @brief This class is used to perform test case initialisation for testing
/// with the Device Get Test module.
//
struct DeviceGetModuleFixture : public DeviceModuleCommonFixture
{
public:
    /// @brief Constructor. 
    DeviceGetModuleFixture();

    /// @brief Destructor. Shutdown the test.
    ~DeviceGetModuleFixture();

    /// @brief Check the content of the xpath filtered <get-config> response. 
    /// @desc Get the contents of the <get-config> response and verify 
    /// that all entries are as expected.
    virtual void checkXpathFilteredConfig(const string& xPath) const;

    /// @brief Check the content of the subtree filtered <get-config> response. 
    /// @desc Get the contents of the <get-config> response and verify 
    /// that all entries are as expected.
    virtual void checkSubtreeFilteredConfig(const string& subtree) const;

//protected:
    /// @brief Update activeProfile in expected <get-config> response.  
    /// Utility function to ensure that the activeProfile is set in the
    /// filteredConfig_ tree.
    ///
    /// @param profileId the id of the active profile.
    void ensureActiveProfileInFilteredConfig( uint16_t profileId );

    /// @brief Update profile in expected <get-config> response.  
    /// Utility function to ensure that the profile exists in the
    /// filteredConfig_ tree. If the item does not exist it will be created, 
    /// initialised and added.
    ///
    /// @param profileId the of of the connection.
    void ensureProfileInFilteredConfig( uint16_t profileId );

    /// @brief Update stream connection in expected <get-config> response.  
    /// @desc Utility function to ensure that the connection exists in the 
    /// filteredConfig_ tree. If the item does not exist it will be created, 
    /// initialised and added.
    ///
    /// @param profileId the of of the profile.
    /// @param connectionId the of of the connection.
    void ensureStreamConnectionInFilteredConfig( uint16_t profileId,
                                                 uint16_t connectionId );

    /// @brief Update stream in expected <get-config> response.
    /// @desc Utility function to ensure that the stream exists for the 
    /// profile in the filteredConfig_ tree. If the item does not exist
    /// it will be created, initialised and added.
    ///
    /// @param profileId the of of the profile.
    /// @param streamId the of of the connection.
    void ensureProfileStreamInFilteredConfig( uint16_t profileId, 
                                              uint16_t streamId );

    /// @brief Update resource descrip in expected <get-config> response.
    /// @desc Utility function to ensure that the resource exists for the
    /// stream of a profile in the filteredConfig_ tree. If the item does
    /// not exist it will be created, initialised and added.
    ///
    /// @param profileId the of of the profile.
    /// @param streamId the of of the connection.
    /// @param resourceId the of of the resource.
    void ensureResourceDescriptionInFilteredConfig( uint16_t profileId,
                                                    uint16_t streamId, 
                                                    uint16_t resourceId );
    
    /// @brief Update resource connection in expected <get-config> response.
    /// @desc Utility function to ensure that the connection exists for the
    /// stream of a profile in the filteredConfig_ tree. If the item does
    /// not exist it will be created, initialised and added.
    ///
    /// @param profileId the of of the connection.
    /// @param streamId the of of the connection.
    /// @param connectionId the of of the connection.
    void ensureResourceConnectionInFilteredConfig( uint16_t profileId,
                                                   uint16_t streamId, 
                                                   uint16_t connectionId );

    /// @brief Configure resource in expected <get-config> response.
    /// @desc This function generates the complete configuration for a 
    /// ResourceNode, items are configured depending on whether or 
    /// not they have been set in the supplied ResourceNodeConfig.
    ///
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param connectionId the id of the connection node to create.
    /// @param config the configuration to generate
    void configureResourceDescrptionInFilteredConfig( 
            uint16_t profileId,
            uint16_t streamId,
            uint16_t resourceId,
            const YumaTest::ResourceNodeConfig& config );

    /// @brief Configure resource connection in expected <get-config> response.
    /// @desc This function generates the complete configuration for a 
    /// ConnectionItem, items are configured depending on whether or 
    /// not they have been set in the supplied ConnectionItemConfig.
    ///
    /// @param profileId the of the owning profile.
    /// @param streamId the of the stream to create. 
    /// @param connectionId the id of the connection node to create.
    /// @param config the configuration to generate
    void configureResourceConnectionInFilteredConfig( 
             uint16_t profileId,
             uint16_t streamId,
             uint16_t connectionId,
             const YumaTest::ConnectionItemConfig& config );

    /// @brief Configure stream connection in expected <get-config> response.  
    /// @desc This function generates the complete configuration for a 
    /// ConnectionItem, items are configured depending on whether or not 
    /// they have been set in the supplied ConnectionItemConfig.
    ///
    /// @param profileId the of the owning profile.
    /// @param connectionId the id of the connection node to create.
    /// @param config the configuration to generate
    void configureStreamConnectionInFilteredConfig( 
             uint16_t profileId,
             uint16_t connectionId,
             const YumaTest::StreamConnectionItemConfig& config );

    /// @brief Clear container of expected <get-config> response.
    void clearFilteredConfig();

protected:
   std::shared_ptr<YumaTest::XPO3Container> filteredConfig_;  ///< <get-config> response payload
};

} // namespace YumaTest 

#endif // __DEVICE_GET_MODULE_FIXTURE_H
//------------------------------------------------------------------------------
