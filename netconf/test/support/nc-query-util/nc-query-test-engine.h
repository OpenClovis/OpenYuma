#ifndef __YUMA_TEST_TRANSFORM_ENGINE_H
#define __YUMA_TEST_TRANSFORM_ENGINE_H

// ---------------------------------------------------------------------------|
// Standard Includes
// ---------------------------------------------------------------------------|
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-base-query-test-engine.h"

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractYumaOpLogPolicy;
class AbstractNCQueryFactory;

// ---------------------------------------------------------------------------|
/**
 * Yuma-Test utility class for sending Netconf Messages to Yuma that
 * modify the current configuration.
 */
class NCQueryTestEngine : public NCBaseQueryTestEngine
{
public:
    /** 
     * Constructor.
     * The target DB name must be either 'running' or 'candidate'. Running
     * is only valid if Yuma is configured to allow modifications to the
     * running config.
     *
     * \param builder the message builder to use.
     */
    explicit NCQueryTestEngine( std::shared_ptr<NCMessageBuilder> builder );

    /** Destructor */
    virtual ~NCQueryTestEngine();

    /**
     * Load the specified module.
     * This method builds a NC-Query containing a load request,
     * injects the message into Yuma and parses the result to
     * determine if the module was loaded successfully.
     *
     * \tparam the type of results checker
     * \param moduleName the name of the module to load
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryLoadModule( std::shared_ptr<AbstractNCSession> session,
                        const std::string& moduleName, 
                        Checker& checker )
    {
        // build a load module message for test.yang
        const std::string queryStr = messageBuilder_->buildLoadMessage( 
                moduleName, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Try to lock the database associated with this engine.
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryLockDatabase( std::shared_ptr<AbstractNCSession> session,
                          const std::string& target, 
                          Checker& checker )
    {
        // build a lock module message for test.yang
        const std::string queryStr = messageBuilder_->buildLockMessage( 
                session->allocateMessageId(), target );
        runQuery( session, queryStr, checker );
    }

    /**
     * Try to unlock the database associated with this engine.
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryUnlockDatabase( std::shared_ptr<AbstractNCSession> session,
                            const std::string& target, 
                            Checker& checker )
    {
        // build an unlock module message for test.yang
        const std::string queryStr = messageBuilder_->buildUnlockMessage( 
                session->allocateMessageId(), target );
        runQuery( session, queryStr, checker );
    }

    /**
     * Try to validate the database associated with this engine.
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryValidateDatabase( std::shared_ptr<AbstractNCSession> session,
                              const std::string& target, 
                              Checker& checker )
    {
        // build a validate message for test.yang
        const std::string queryStr = messageBuilder_->buildValidateMessage( 
                session->allocateMessageId(), target );
        runQuery( session, queryStr, checker );
    }

    /**
     * Get the configuration filtered using the supplied xpath
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param xPathFilterStr the XPath filter to apply
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetConfigXpath( std::shared_ptr<AbstractNCSession> session,
                            const std::string& xPathFilterStr,
                            const std::string& target, 
                            Checker& checker )
    {
        // build a load module message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetConfigMessageXPath( 
                xPathFilterStr, target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Get the configuration filtered using the supplied subtree
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param subtreeFilterStr the subtree filter to apply
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetConfigSubtree( std::shared_ptr<AbstractNCSession> session,
                              const std::string& subtreeFilterStr,
                              const std::string& target, 
                              Checker& checker )
    {
        // build a load module message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetConfigMessageSubtree( 
                subtreeFilterStr, target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Get the state data filtered using the supplied xpath
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param xPathFilterStr the XPath filter to apply
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetXpath( std::shared_ptr<AbstractNCSession> session,
                      const std::string& xPathFilterStr,
                      const std::string& target, 
                      Checker& checker )
    {
        // build a load module message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetMessageXPath( 
                xPathFilterStr, target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Get the state data filtered using the supplied subtree
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param subtreeFilterStr the subtree filter to apply
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetSubtree( std::shared_ptr<AbstractNCSession> session,
                        const std::string& subtreeFilterStr,
                        const std::string& target, 
                        Checker& checker )
    {
        // build a load module message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetMessageSubtree( 
                subtreeFilterStr, target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Edit the configuration using the supplied query
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param query the edit config operation 
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryEditConfig( std::shared_ptr<AbstractNCSession> session,
                        const std::string& query,
                        const std::string& target, 
                        Checker& checker )
    {
        const std::string queryStr = 
            messageBuilder_->buildEditConfigMessage( 
                query, target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Try a custom RPC Query.
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param query the edit config operation 
     * \param checker the results checker.
     */
    template< class Checker >
    void tryCustomRPC( std::shared_ptr<AbstractNCSession> session,
                        const std::string& query,
                        Checker& checker )
    {
        const std::string queryStr = 
            messageBuilder_->buildRPCMessage( 
                query, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Copy the configuration
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param target the target database name
     * \param source the source database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryCopyConfig( std::shared_ptr<AbstractNCSession> session,
                        const std::string& target, 
                        const std::string& source, 
                        Checker& checker )
    {
        const std::string queryStr = 
            messageBuilder_->buildCopyConfigMessage( 
                target, source, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Delete the configuration
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryDeleteConfig( std::shared_ptr<AbstractNCSession> session,
                          const std::string& target, 
                          Checker& checker )
    {
        const std::string queryStr = 
            messageBuilder_->buildDeleteConfigMessage( 
                target, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Set the log verbosity to the supplied level
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param logLevelStr the log level to apply
     * \param checker the results checker.
     */
    template< class Checker >
    void trySetLogLevel( std::shared_ptr<AbstractNCSession> session,
                            const std::string& logLevelStr,
                            Checker& checker )
    {
        // build a set log level message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildSetLogLevelMessage( 
                logLevelStr, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Discard changes
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryDiscardChanges( std::shared_ptr<AbstractNCSession> session,
                            Checker& checker )
    {
        // build a discard-changes message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildDiscardChangesMessage( 
                           session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Get my session
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetMySession( std::shared_ptr<AbstractNCSession> session,
                                Checker& checker )
    {
        // build a get my session message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetMySessionMessage( 
                           session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Set my session
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param indent the indent to be set.
     * \param linesize the linesize to be set.
     * \param withDefaults the with-defaults setting to be set.
     * \param checker the results checker.
     */
    template< class Checker >
    void trySetMySession( std::shared_ptr<AbstractNCSession> session,
                          const std::string& indent,
                          const std::string& linesize,
                          const std::string& withDefaults,
                          Checker& checker )
    {
        // build a set my session message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildSetMySessionMessage(
                indent, linesize, withDefaults, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Kill session
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param sessionId the id of the session to be killed.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryKillSession( std::shared_ptr<AbstractNCSession> session,
                                    uint16_t sessionId,
                                    Checker& checker )
    {
        // build a kill-session message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildKillSessionMessage( 
                           sessionId, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Close session
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryCloseSession( std::shared_ptr<AbstractNCSession> session,
                                    Checker& checker )
    {
        // build a close-session message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildCloseSessionMessage( 
                           session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Shutdown
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryShutdown( std::shared_ptr<AbstractNCSession> session,
                                    Checker& checker )
    {
        // build a shutdown message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildShutdownMessage( 
                           session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Restart
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param checker the results checker.
     */
    template< class Checker >
    void tryRestart( std::shared_ptr<AbstractNCSession> session,
                                    Checker& checker )
    {
        // build a restart message for test.yang
        const std::string queryStr = 
            messageBuilder_->buildRestartMessage( 
                           session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }

    /**
     * Get the data model definition files from the server
     *
     * \tparam the type of results checker
     * \param session the session to use for injecting the message.
     * \param xPathFilterStr the XPath filter to apply
     * \param target the target database name
     * \param checker the results checker.
     */
    template< class Checker >
    void tryGetSchema( std::shared_ptr<AbstractNCSession> session,
                       const std::string& schemaIdStr,
                       Checker& checker )
    {
        // build a get-schema message for device_test.yang
        const std::string queryStr = 
            messageBuilder_->buildGetSchemaMessage(
                schemaIdStr, session->allocateMessageId() );
        runQuery( session, queryStr, checker );
    }
    
    /**
     * Utility function for locking the database.
     *
     * \param session the session to use for injecting the message.
     * \param target the target database name
     */
    void unlock( std::shared_ptr<AbstractNCSession> session,
                 const std::string& target );

    /**
     * Utility function for unlocking the database.
     *
     * \param session the session to use for injecting the message.
     * \param target the target database name
     */
    void lock( std::shared_ptr<AbstractNCSession> session,
               const std::string& target );

    /**
     * Utility function for loading a module.
     *
     * \param session the session to use for injecting the message.
     * \param moduleName the name of the module to load 
     */
    void loadModule( std::shared_ptr<AbstractNCSession> session,
                     const std::string& moduleName );
    /**
     * Send a commit message.
     *
     * \param session the session to use for injecting the message.
     */
    void commit( std::shared_ptr<AbstractNCSession> session );
    
    /**
     * Send a commit message that is expected to fail.
     *
     * \param session the session to use for injecting the message.
     */
    void commitFailure( std::shared_ptr<AbstractNCSession> session );

    /**
     * Send a confirmed-commit message.
     *
     * \param session the session to use for injecting the message.
     * \param timeout the confirm-timeout in seconds.
     */
    void confirmedCommit( std::shared_ptr<AbstractNCSession> session, 
                          const int timeout );
};    

// ---------------------------------------------------------------------------|
/**
 * RAII Lock guard for locking and unlocking of a database during
 * testing. Use this lock guard to ensure that the database is left
 * unlocked when the test exits even on test failure.
 */
class NCDbScopedLock
{
public:
    /** Constructor. This issues a lock command to the database. 
     *
     * \param engine shared_ptr to the query-test-engine.
     * \param session shared_ptr to the session.
     * \param target the target database name
     */
    NCDbScopedLock( std::shared_ptr< NCQueryTestEngine > engine, 
                    std::shared_ptr< AbstractNCSession > session,
                    const std::string& target );

    /**
     * Destructor.
     */
    ~NCDbScopedLock();

private:
    /** The engine that issues lock / unlock requests */
    std::shared_ptr< NCQueryTestEngine > engine_; 

    /** The session locking / unlocking the database */
    std::shared_ptr< AbstractNCSession > session_;

    /** The name of the target database */
    std::string target_;
};


} // namespace YumaTest

#endif // __YUMA_TEST_TRANSFORM_ENGINE_H
