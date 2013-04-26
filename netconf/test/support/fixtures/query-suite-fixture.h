#ifndef __YUMA_QUERY_SUITE_FIXTURE__H
#define __YUMA_QUERY_SUITE_FIXTURE__H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/test-context.h"
#include "test/support/fixtures/base-suite-fixture.h"
#include "test/support/fixtures/abstract-fixture-helper.h"
#include "test/support/msg-util/NCMessageBuilder.h"
#include "test/support/callbacks/callback-checker.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <memory>
#include <map>

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest 
{
class YumaQueryOpLogPolicy;
class NCQueryTestEngine;
class NCDbScopedLock;
class AbstractNCSession;
class AbstractNCSessionFactory;
class CallbackChecker;
class AbstractCBCheckerFactory;
class AbstractFixtureHelper;
class AbstractFixtureHelperFactory;

// ---------------------------------------------------------------------------|
/**
 * This class is used to perform simple test case initialisation.
 * It can be used on a per test case basis or on a per test suite
 * basis. This fixture provides basic functionality for querying Yuma.
 */
class QuerySuiteFixture : public BaseSuiteFixture
{
public:
    /** 
     * Constructor. 
     */
    QuerySuiteFixture();

    /** 
     * Constructor. 
     * \param builder an NCMessageBuilder
     */
    QuerySuiteFixture( std::shared_ptr< NCMessageBuilder> builder);
    
    /**
     * Destructor. Shutdown the test.
     */
    virtual ~QuerySuiteFixture();

    /** 
     * This function is used to obtain a full lock of the system under
     * test. 
     *
     * If the configuration is writeable running the 'startup' and
     * 'running' databases are locked.
     *
     * If the configuration is candidate the 'startup', 'running' and
     * 'candidate' databases are locked.
     *
     * \param session  the session requesting the locks
     * \param useStartup  true to get startup lock and false otherwise
     * \return a vector of RAII locks. 
     */
    std::vector< std::unique_ptr< NCDbScopedLock > >
    getFullLock( std::shared_ptr<AbstractNCSession> session );

    /**
     * Commit the changes.
     * If the configuration is CONFIG_USE_CANDIDATE a commit message
     * is sent to Yuma.
     *
     * \param session  the session requesting the locks
     */
    virtual void commitChanges( std::shared_ptr<AbstractNCSession> session );

    /**
     * Commit the changes expecting the commit to fail.
     * If the configuration is CONFIG_USE_CANDIDATE a commit message
     * is sent to Yuma.
     *
     * \param session  the session requesting the locks
     */
    virtual void commitChangesFailure( std::shared_ptr<AbstractNCSession> session );

    /**
     * Confirmed commit of the changes.
     * If the configuration is CONFIG_USE_CANDIDATE a confirmed-commit message
     * is sent to Yuma.
     *
     * \param session  the session requesting the locks
     * \param timeout  the confirm-timeout of the message in seconds 
     */
    virtual void confirmedCommitChanges( 
                      std::shared_ptr<AbstractNCSession> session,
                      const int timeout );

    /**
     * Run an edit query.
     *
     * \param session the session running the query
     * \param query the query to run
     */
    virtual void runEditQuery( std::shared_ptr<AbstractNCSession> session,
                               const std::string& query );

    /**
     * Run an edit query.
     *
     * \param session the session running the query.
     * \param query the query to run.
     * \param failReson the expected fail reason.
     */
    virtual void runFailedEditQuery( std::shared_ptr<AbstractNCSession> session,
                       const std::string& query,
                       const std::string& failReason );

    /**
     * Run a validate command.
     *
     * \param session the session running the query
     */
    void runValidateCommand( std::shared_ptr<AbstractNCSession> session );

    /**
     * Run a get-my-session query.
     *
     * \param session the session running the query.
     * \param expIndent the expected indent.
     * \param expLinesize the expected linesize.
     * \param expWithDefaults the expected with-defaults setting.
     */
    void runGetMySession( std::shared_ptr<AbstractNCSession> session,
                           const std::string& expIndent,
                           const std::string& expLinesize,
                           const std::string& expWithDefaults );

    /**
     * Run a set-my-session query.
     *
     * \param session the session running the query.
     * \param indent the indent to be set.
     * \param linesize the linesize to be set.
     * \param withDefaults the with-defaults setting to be set.
     */
    void runSetMySession( std::shared_ptr<AbstractNCSession> session,
                           const std::string& indent,
                           const std::string& linesize,
                           const std::string& withDefaults );

    /**
     * Run a shutdown query.
     *
     * \param session the session running the query.
     */
    void runShutdown( std::shared_ptr<AbstractNCSession> session );

    /**
     * Run a restart query.
     *
     * \param session the session running the query.
     * \param query the query to run.
     */
    void runRestart( std::shared_ptr<AbstractNCSession> session );

    /**
     * Run a query when there is expected to be no response.
     *
     * \param session the session running the query.
     */
    void runNoSession( std::shared_ptr<AbstractNCSession> session );

    /** the session factory. */
    std::shared_ptr<AbstractNCSessionFactory> sessionFactory_;

    /** the cb checker factory. */
    std::shared_ptr<AbstractCBCheckerFactory> cbCheckerFactory_;
    
    /** the fixture helper factory. */
    std::shared_ptr<AbstractFixtureHelperFactory> fixtureHelperFactory_;
    
    /** Each test always has one session */
    std::shared_ptr<AbstractNCSession> primarySession_;

    /** Each test always has one cb checker */
    std::shared_ptr<CallbackChecker> cbChecker_;

    /** Each test always has one fixture helper */
    std::shared_ptr<AbstractFixtureHelper> fixtureHelper_;

    /** Utility XML message builder */
    std::shared_ptr<NCMessageBuilder> messageBuilder_;

    /** The Query Engine */
    std::shared_ptr<NCQueryTestEngine> queryEngine_;
};

} // namespace YumaTest

// ---------------------------------------------------------------------------|

#endif // __YUMA_QUERY_SUITE_FIXTURE__H

