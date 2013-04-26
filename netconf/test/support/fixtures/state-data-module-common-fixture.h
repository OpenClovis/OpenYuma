#ifndef __STATE_MODULE_COMMON_FIXTURE_H
#define __STATE_MODULE_COMMON_FIXTURE_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
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
/// with the State Data Test module.
/// @desc This class should be used as a base class for test suites
/// fuixtures that need to use common StateDataModule access code.
//
struct StateDataModuleCommonFixture : public YumaTest::QuerySuiteFixture
{
public:
    /// @brief Constructor. 
    explicit StateDataModuleCommonFixture( const std::string& moduleNs );

    /// @brief Destructor. Shutdown the test.
    ~StateDataModuleCommonFixture();

    /// @brief Create the toaster level container.
    ///
    /// @param session the session running the query
    /// @param op the operation type
    /// @param failReason optional expected failure reason.
    void createToasterContainer( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            const std::string& op = std::string( "create" ),
            const std::string& failReason = std::string() );

    /// @brief Delete the toaster level container.
    ///
    /// @param session the session running the query
    /// @param commitChange flag indicating if the change should be committed.
    /// @param failReason optional expected failure reason.
    void deleteToasterContainer( 
            std::shared_ptr<YumaTest::AbstractNCSession> session,
            bool commitChange = true,
            const std::string& failReason = std::string() );

    /// @brief Configure toaster content in db.
    /// @desc This function generates the complete configuration for a 
    /// toaster, items are configured depending on whether or 
    /// not they have been set in the supplied ToasterContainerConfig.
    ///
    /// @param config the configuration to generate
    void configureToaster( 
            const YumaTest::ToasterContainerConfig& config );

    /// @brief Commit the changes.
    ///
    /// @param session  the session requesting the locks
    virtual void commitChanges( 
            std::shared_ptr<YumaTest::AbstractNCSession> session );

    /// @brief Discard all changes locally.
    /// @desc Let the test harness know that changes should be discarded 
    /// (e.g. due to unlocking the database without a commit.
    void discardChanges();

    /// @brief Check the state data. 
    /// @desc Get the contents of the candidate database and verify 
    /// that all entries are as expected.
    virtual void checkState() const;

protected:
    /// @brief Utility fuction for checking the db state content
    ///
    /// @param dbName the name of the netconf database to check
    /// @param db the expected contents.
    virtual void checkEntriesImpl( const std::string& dbName,
        const std::shared_ptr<YumaTest::ToasterContainer>& db ) const;

    /// @brief Utility function to query state data.
    /// @desc this function can be used for debugging purposes. It
    /// queries the specified data and as a result the state data
    /// xml will be dumped into the test output file SesXXXX.
    ///
    /// @param dbName the name of the netconf database to check
    void queryState( const string& dbName ) const;

protected:
    ///< The builder for the generating netconf messages in xml format. 
    std::shared_ptr< YumaTest::StateDataQueryBuilder > toasterBuilder_;

    std::shared_ptr<YumaTest::ToasterContainer> runningEntries_;    ///< the running data
    std::shared_ptr<YumaTest::ToasterContainer> candidateEntries_;  ///< the candidate data
};

} // namespace YumaTest 

#endif // __STATE_MODULE_COMMON_FIXTURE_H
//------------------------------------------------------------------------------
