#ifndef __STATE_GET_MODULE_FIXTURE_H
#define __STATE_GET_MODULE_FIXTURE_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/state-data-module-common-fixture.h"
#include "test/support/msg-util/state-data-query-builder.h"
#include "test/support/db-models/state-data-test-db.h"

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
class ToasterContainer;

// ---------------------------------------------------------------------------|
//
/// @brief This class is used to perform test case initialisation for testing
/// with the State Get Test module.
//
struct StateGetModuleFixture : public StateDataModuleCommonFixture
{
public:
    /// @brief Constructor. 
    StateGetModuleFixture();

    /// @brief Destructor. Shutdown the test.
    ~StateGetModuleFixture();

    /// @brief Check the content of the <get> response. 
    /// @desc Get the contents of the <get> response and verify 
    /// that all entries are as expected.
    virtual void checkXpathFilteredState(const string& xPath) const;

    /// @brief Check the content of the subtree filtered <get> response. 
    /// @desc Get the contents of the <get> response and verify 
    /// that all entries are as expected.
    virtual void checkSubtreeFilteredState(const string& subtree) const;

//protected:
    /// @brief Configure toaster in expected <get> response.
    /// @desc This function generates the complete configuration for a 
    /// toaster, items are configured depending on whether or 
    /// not they have been set in the supplied ToasterContainerConfig.
    ///
    /// @param config the configuration to generate
    void configureToasterInFilteredState( 
            const YumaTest::ToasterContainerConfig& config );

    /// @brief Clear container of expected <get> response.
    void clearFilteredState();

protected:
   std::shared_ptr<YumaTest::ToasterContainer> filteredStateData_;  ///< <get> response payload
};

} // namespace YumaTest 

#endif // __STATE_GET_MODULE_FIXTURE_H
//------------------------------------------------------------------------------
