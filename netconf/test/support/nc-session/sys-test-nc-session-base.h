#ifndef __YUMA_SYSTEM_TEST_SESSION_BASE_H
#define __YUMA_SYSTEM_TEST_SESSION_BASE_H

// ---------------------------------------------------------------------------|
// test Harness Includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/abstract-nc-session.h"
#include <string>
#include <fstream>
#include <memory>

// ---------------------------------------------------------------------------|
namespace YumaTest 
{

/**
 * Base class for System Test Net Conf Sessions.
 *
 * This class simply provides some utility functions for sending&
 * receiving messages to/from the netconf server.
 */
class SysTestNCSessionBase : public AbstractNCSession
{
public:
    /** 
     * Constructor 
     *
     * \param user the name of the user.
     * \param policy the log filename generation policy
     * \param sessionId the session id.
     */
    SysTestNCSessionBase( const std::string& user,
        std::shared_ptr< AbstractYumaOpLogPolicy > policy,
        uint16_t sessionId );

    /** Destructor */
    virtual ~SysTestNCSessionBase();

    /** 
     * Inject the query into netconf. This test function sends the
     * supplied query into the netconf server via a local or network
     * connection. It reads back any response from the netconf server
     * and writes it to the logfile.
     *
     * \param queryStr a string containing the XML message to inject.
     * \return the id of the message used to inject the query.
     */
    virtual uint16_t injectMessage( const std::string& queryStr );

    /**
     * Get the remote user.
     *
     * \return the user.
     */
    const std::string& getUser() { return user_; }
    
protected:
    /**
     * Send a hello response message to the server.
     * This message is called to establis a session after a successful
     * connection to the netconf server. The order of events is 
     *
     * <ol>
     *   <li>a connection to the netconf server is established.</li>
     *   <li>the netconf server sends it's capabilities.</li>
     *   <li>send the hello message to complete session *   establishment.</li>
     * </ol>
     */
    void sendHelloResponse();

private:
    /** 
     * Send a  query into netconf. 
     *
     * \param queryStr a string containing the XML message to inject.
     */
    void sendMessage( const std::string& queryStr );

    /**
     * Read the reply from the netconf server.
     *
     * \return a string containing the reply.
     */
    std::string receiveMessage();

    /**
     * open output logfile for a query.
     *
     * \param queryStr a string containing the XML message to inject.
     * \return a file opened for output asa  stream.
     */
    std::shared_ptr<std::ofstream> openLogFileForQuery( 
            const std::string& queryStr );

    /**
     * open an output logfile.
     *
     * \param logFilename the name of the file to open.
     * \return a file opened for output asa  stream.
     */
    std::shared_ptr<std::ofstream> openLogFile( 
            const std::string& logFilename );

    /**
     * Connect to the netconf server 
     */
    virtual void connect() = 0;
 
    /**
     * read the connection
     *
     * \param buffer the buffer to read into.
     * \param length the number of bytes to read.
     * \return the number of bytes read.
     */
    virtual ssize_t receive( char* buffer, ssize_t length ) = 0;
 
    /**
     * write to the connection
     *
     * \param buffer the buffer to write.
     * \param length the number of bytes to sent.
     * \return the number of bytes read.
     */
    virtual ssize_t send( const char* buffer, ssize_t length ) = 0;
 
protected:
    int ncxsock_;        ///< The file descriptor for the socket
    bool connected_;     ///< Flag indicating connection status
    std::string user_;   ///< The remote user
};

} // namespace YumaTest

#endif // __YUMA_SYSTEM_TEST_SESSION_BASE_H
