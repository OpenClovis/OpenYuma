#ifndef __YUMA_SIMPLE_YANG_TEST_FIXTURE__H
#define __YUMA_SIMPLE_YANG_TEST_FIXTURE__H

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
struct SimpleYangFixture : public QuerySuiteFixture
{
public:
    /** 
     * Constructor. 
     */
    SimpleYangFixture();

    /**
     * Destructor. Shutdown the test.
     */
    ~SimpleYangFixture();

    /** 
     * Create the top level container.
     *
     * \param session the session running the query
     * \param container the container to create
     */
    void createContainer( std::shared_ptr<AbstractNCSession> session, 
                          const std::string& container );

    /** 
     * Delete the top level container.
     *
     * \param session the session running the query
     * \param container the container to delete
     */
    void deleteContainer( std::shared_ptr<AbstractNCSession> session,
                          const std::string& container );

    /** 
     * Create interface container.
     *
     * \param session the session running the query
     * \param type the type of the interface (ethernet or atm) 
     * \param mtu the MTU of the interface 
     * \param operation the operation to perform on the container 
     */
    void createInterfaceContainer( std::shared_ptr<AbstractNCSession> session,
                                   const std::string& type,
                                   int mtu,
                                   const std::string& operation );

    /** 
     * Add a choice leaf.
     *
     * \param session the session running the query
     * \param entryChoice the name of the choice leaf to add.
     * \param operationStr the type of addition - add, merge, replace.
     */
    void addChoice( std::shared_ptr<AbstractNCSession> session,
                    const std::string& choiceStr,
                    const std::string& operationStr );

    /**
     * Verify the choices in the specified database.
     * 
     * \param session the session running the query
     * \param targetDbName the name of the database to check
     * \param choice the expected choice in the database being checked.
     */
    void checkChoiceImpl(
        std::shared_ptr<AbstractNCSession> session,
        const std::string& targetDbName,
        const std::string& choice );

    /**
     * Check the status of both databases.
     *
     * \param session the session running the query
     */
    void checkChoice( 
        std::shared_ptr<AbstractNCSession> session );

    /**
     * Commit the changes.
     *
     * \param session  the session requesting the locks
     */
    virtual void commitChanges( std::shared_ptr<AbstractNCSession> session );

    const std::string moduleNs_;        ///< the module namespace
    const std::string containerName_;   ///< the container name 

    std::string runningChoice_; /// Running Db Choice 
    std::string candidateChoice_; /// Candidate Db Choice 
};

} // namespace YumaTest

#endif // __YUMA_SIMPLE_YANG_TEST_FIXTURE__H
