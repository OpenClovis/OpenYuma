#ifndef __YUMA_REMOTE_NC_SESSION_H
#define __YUMA_REMOTE_NC_SESSION_H

// ---------------------------------------------------------------------------|
// test Harness Includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/sys-test-nc-session-base.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>
#include <memory>
#include <libssh2.h>

// ---------------------------------------------------------------------------!
namespace YumaTest 
{

/**
 * Utility class for creating a local connection to the netconf
 * server, bypassing SSH and netconf-subsytem.c.
 *
 * This class spoofs the work performed by netconf-subsystem.c
 * to allow a direct connection to the netconf server.
 */

// ---------------------------------------------------------------------------!
class RemoteNCSession : public SysTestNCSessionBase
{
public:
    /**
     * Constructor. 
     *
     * \param ipaddress the ipaddess of the remote host.
     * \param port the port to connect to
     * \param user the name of the user.
     * \param password the users password.
     * \param policy the log filename generation policy
     * \param sessionId the session id.
     */
    RemoteNCSession( const std::string& ipAddress, 
                     const uint16_t port,
                     const std::string& user, 
                     const std::string& password,
                     std::shared_ptr< AbstractYumaOpLogPolicy > policy,
                     uint16_t sessionId );

    /** Destructor.  */
    virtual ~RemoteNCSession();

private:
    /** Connect to the netconf server */
    virtual void connect();

    /**
     * read the connection
     *
     * \param buffer the buffer to read into.
     * \param length the number of bytes to read.
     * \return the number of bytes read.
     */
    virtual ssize_t receive( char* buffer, ssize_t length );

    /**
     * write to the connection
     *
     * \param buffer the buffer to write.
     * \param length the number of bytes to sent.
     * \return the number of bytes read.
     */
    virtual ssize_t send( const char* buffer, ssize_t length );

protected:
    std::string ipAddress_;      ///< ip address of netconf server
    uint16_t port_;              ///< netconf port
    std::string password_;       ///< password for remote user
    LIBSSH2_SESSION* session_;    ///< the ssh session
    LIBSSH2_CHANNEL* channel_;    ///< the ssh channel
};

} // YumaTest

#endif // __YUMA_REMOTE_NC_SESSION_H
