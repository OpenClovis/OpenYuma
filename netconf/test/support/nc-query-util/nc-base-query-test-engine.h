#ifndef __YUMA_NC_BASE_QUERY_TEST_ENGINE_H
#define __YUMA_NC_BASE_QUERY_TEST_ENGINE_H

// ---------------------------------------------------------------------------|
// Standard Includes
// ---------------------------------------------------------------------------|
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/msg-util/NCMessageBuilder.h"
#include "test/support/nc-session/abstract-nc-session.h"

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractNCSession;

// ---------------------------------------------------------------------------|
/**
 * Yuma-Test utility base class for sending Netconf Messages to Yuma.
 */
class NCBaseQueryTestEngine
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
    explicit NCBaseQueryTestEngine( std::shared_ptr<NCMessageBuilder> builder );

    /** Destructor */
    virtual ~NCBaseQueryTestEngine();

    /**
     * Utility function for setting the yuma log level.
     * The following Log Levels are supported:
     * <ul>
     *   <li>off</li>
     *   <li>error</li>
     *   <li>warn</li>
     *   <li>info</li>
     *   <li>debug</li>
     *   <li>debug2</li>
     *   <li>debug3</li>
     *   <li>debug4</li>
     * </ul>
     *
     * \param session the id of the session associated with this query
     * \param logLevel the log level to set
     */
    void setLogLevel( std::shared_ptr<AbstractNCSession> session,
                      const std::string& logLevel );

protected:
    /**
     * Run the supplied query and check the results.
     * This method builds a NC-Query containing the supplied query
     * string, injects the message into Yuma and parses the result to
     * determine if the query was successful.
     *
     * \tparam the type of results checker
     * \param session the id of the session associated with this query
     * \param queryStr the query to try.
     * \param checker the results checker.
     */
    template< class Checker >
    void runQuery( std::shared_ptr<AbstractNCSession> session,
                   const std::string& queryStr, 
                   Checker& checker )
    {
        // Inject the message
        uint16_t messageId = session->injectMessage( queryStr );

        BOOST_TEST_MESSAGE( "Injecting Message:\n " << queryStr );

        // Check the Result 
        const std::string queryResult = session->getSessionResult( messageId );

        BOOST_TEST_MESSAGE( "Response:\n " << queryResult );

        checker( queryResult );
    }

protected:
    /** Utility XML message builder */
    std::shared_ptr<NCMessageBuilder> messageBuilder_;
};    

} // namespace YumaTest

#endif // __YUMA_NC_BASE_QUERY_TEST_ENGINE_H

