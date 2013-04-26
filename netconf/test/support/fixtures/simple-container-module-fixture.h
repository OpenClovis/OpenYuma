#ifndef __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H
#define __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
#include "test/support/msg-util/NCMessageBuilder.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <map>
#include <string>
#include <memory>

// ---------------------------------------------------------------------------|
namespace YumaTest 
{
class AbstractNCSession;

// ---------------------------------------------------------------------------|
/**
 * This class is used to perform simple test case initialisation.
 * It can be used on a per test case basis or on a per test suite
 * basis.
 */
struct SimpleContainerModuleFixture : public QuerySuiteFixture
{
public:
    /** Convenience typedef */
    typedef std::map< std::string, std::string > EntryMap_T;

    /** Convenience typedef */
    typedef std::shared_ptr< EntryMap_T > SharedPtrEntryMap_T;

public:
    /** 
     * Constructor. 
     */
    SimpleContainerModuleFixture();

    /**
     * Destructor. Shutdown the test.
     */
    ~SimpleContainerModuleFixture();

    /** 
     * Create the top level container.
     *
     * \param session the session running the query
     */
    void createMainContainer( std::shared_ptr<AbstractNCSession> session );

    /** 
     * Delete the top level container.
     *
     * \param session the session running the query
     */
    void deleteMainContainer( std::shared_ptr<AbstractNCSession> session );

    /** 
     * Perform specified operation on the top level container.
     *
     * \param session the session running the query
     * \param op the operation to perform
     */
    void mainContainerOp( std::shared_ptr<AbstractNCSession> session,
                           const std::string& op);

    /** 
     * Add an entry.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key to add.
     * \param operationStr the type of addition - add, merge, replace.
     */
    void addEntry( std::shared_ptr<AbstractNCSession> session,
                   const std::string& entryKeyStr,
                   const std::string& operationStr );

    /** 
     * Add an entry value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     * \param operationStr the type of addition - add, merge, replace.
     */
    void addEntryValue( std::shared_ptr<AbstractNCSession> session,
                        const std::string& entryKeyStr,
                        const std::string& entryValStr,
                        const std::string& operationStr );

    /** 
     * Add an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void addEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                            const std::string& entryKeyStr,
                            const std::string& entryValStr );

    /** 
     * Merge an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void mergeEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                              const std::string& entryKeyStr,
                              const std::string& entryValStr );

    /** 
     * Replace an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void replaceEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                                const std::string& entryKeyStr,
                                const std::string& entryValStr );

    /** 
     * No operation on an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void noOpEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                             const std::string& entryKeyStr,
                             const std::string& entryValStr );

    /** 
     * Delete an entry.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key to delete.
     */
    void deleteEntry( std::shared_ptr<AbstractNCSession> session,
                      const std::string& entryKeyStr );

    /** 
     * Fail to delete an entry.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key to delete.
     */
    void deleteEntryFailed( std::shared_ptr<AbstractNCSession> session,
                            const std::string& entryKeyStr );

    /** 
     * Delete an entry value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void deleteEntryValue( std::shared_ptr<AbstractNCSession> session,
                        const std::string& entryKeyStr,
                        const std::string& entryValStr );

    /** 
     * Fail to delete an entry value.
     *
     * \param session the session running the query
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void deleteEntryValueFailed( std::shared_ptr<AbstractNCSession> session,
                                 const std::string& entryKeyStr,
                                 const std::string& entryValStr );

    /** 
     * Delete an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void deleteEntryValuePair( std::shared_ptr<AbstractNCSession> session,
                            const std::string& entryKeyStr,
                            const std::string& entryValStr );

    /** 
     * Fail to delete an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void deleteEntryValuePairFailed( std::shared_ptr<AbstractNCSession> session,
                                     const std::string& entryKeyStr,
                                     const std::string& entryValStr );

    /**
     * Populate the database with the entries. This function creates
     * numEntries elements in the database, each having a key matching
     * the following format "entryKey#". This function uses the
     * primarySession_ for all operations.
     *
     * \param numEntries the number of entries to add.
     */
    void populateDatabase( const uint16_t numEntries );

    /** 
     * Edit an entry key and value.
     *
     * \param session the session running the query.
     * \param entryKeyStr the name of the entry key.
     * \param entryValStr the value of the entry.
     */
    void editEntryValue( std::shared_ptr<AbstractNCSession> session,
                         const std::string& entryKeyStr,
                         const std::string& entryValStr );

    /**
     * Remove edits from the candidate configuration by performing a 
     * discard-changes operation.
     *
     * \param session the session running the query.
     */
    void discardChangesOperation( std::shared_ptr<AbstractNCSession> session );

    /**
     * Verify the entries in the specified database.
     * This function checks that all expected entries are present in
     * the specified database. It also checks that both databases
     * differ as expected. 
     *
     * e.g.:
     * If the candidate database is being checked it makes sure that
     * any values in the candidate database that should be different
     * are not present in the running database.
     *
     * TODO: This checking is still very crude - it currently does not
     *       support databases with multiple values that are the same.
     * 
     * \param session the session running the query
     * \param targetDbName the name of the database to check
     * \param refMap the map that corresponds to the expected values
     *               in the database being checked.
     * \param otherMap the map that corresponds to the entries in the
     *                 database not being checked.
     */
    void checkEntriesImpl(
        std::shared_ptr<AbstractNCSession> session,
        const std::string& targetDbName,
        const SharedPtrEntryMap_T refMap,
        const SharedPtrEntryMap_T otherMap );

    /**
     * Check the status of both databases.
     *
     * \param session the session running the query
     */
    void checkEntries( 
        std::shared_ptr<AbstractNCSession> session );

    /**
     * Commit the changes.
     *
     * \param session  the session
     */
    virtual void commitChanges( std::shared_ptr<AbstractNCSession> session );

    /**
     * Fail to commit the changes.
     *
     * \param session  the session
     */
    virtual void commitChangesFailure( std::shared_ptr<AbstractNCSession> session );

    /**
     * Confirmed commit of the changes.
     *
     * \param session  the session requesting the locks
     * \param timeout  the confirm-timeout of the message in seconds 
     * \param extend   true if the operation should extend an existing timeout 
     */
    virtual void confirmedCommitChanges( 
                      std::shared_ptr<AbstractNCSession> session,
                      const int timeout,
                      const bool extend = false );

    /**
     * Run a delete-config with startup as the target .
     *
     * \param session the session running the query.
     */
    void runDeleteStartupConfig( std::shared_ptr<AbstractNCSession> session );

    /**
     * Let the test harness know that changes should be discarded
     * (e.g. due to unlocking the database without a commit).
     */
    void discardChanges();

    /**
     * Check the running database to confirm a rollback has taken place
     * and update the test harness internal lists to reflect this.
     */
    void rollbackChanges( std::shared_ptr<AbstractNCSession> session );

    const std::string moduleNs_;        ///< the module namespace
    const std::string containerName_;   ///< the container name 

    SharedPtrEntryMap_T runningEntries_; /// Running Db Entries 
    SharedPtrEntryMap_T candidateEntries_; /// CandidateTarget Db Entries 
    SharedPtrEntryMap_T rollbackEntries_; /// Db Entries reloaded by rollback 
};

} // namespace YumaTest

#endif // __YUMA_SIMPLE_CONTAINER_MODULE_TEST_FIXTURE__H
