#ifndef __YUMA_SPOOF_NC_SESSION_FACTORY_H
#define __YUMA_SPOOF_NC_SESSION_FACTORY_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/abstract-nc-session-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractYumaOpLogPolicy;
class AbstractNCSession;

// ---------------------------------------------------------------------------|
/**
 * A session factory for creating Remote NC Sessions.
 */
class RemoteNCSessionFactory : public AbstractNCSessionFactory
{
public:
    /** 
     * Constructor.
     *
     * \param ipaddress the ipaddess of the remote host.
     * \param port the port to connect to
     * \param user the name of the user.
     * \param password the users password.
     * \param policy the output log filename generation policy.
     */
    RemoteNCSessionFactory( const std::string& ipAddress, 
            const uint16_t port,
            const std::string& user, 
            const std::string& password,
            std::shared_ptr< AbstractYumaOpLogPolicy > policy );

    /** Destructor */
    virtual ~RemoteNCSessionFactory();

    /** 
     * Create a new NCSession.
     *
     * \return a new NCSession.
     */
    std::shared_ptr<AbstractNCSession> createSession();

private:
    const std::string serverIpAddress_; ///< The Ip Address of the remote server
    uint16_t port_;                     ///< The port to connect to
    const std::string user_;            ///< The remote user 
    const std::string password_;        ///< The user's password
};

} // namespace YumaTest

#endif // __YUMA_SPOOF_NC_SESSION_FACTORY_H
