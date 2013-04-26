// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/remote-nc-session-factory.h"
#include "test/support/nc-session/remote-nc-session.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>

// ---------------------------------------------------------------------------|
// File wide namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
RemoteNCSessionFactory::RemoteNCSessionFactory( 
        const std::string& ipAddress, 
        const uint16_t port,
        const std::string& user, 
        const std::string& password,
        shared_ptr< AbstractYumaOpLogPolicy > policy ) 
    : AbstractNCSessionFactory( policy )
    , serverIpAddress_( ipAddress )
    , port_( port )
    , user_( user )
    , password_( password )
{
}

// ---------------------------------------------------------------------------|
RemoteNCSessionFactory::~RemoteNCSessionFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<AbstractNCSession> RemoteNCSessionFactory::createSession()
{
    return shared_ptr<AbstractNCSession> ( 
            new RemoteNCSession( serverIpAddress_, port_, user_, password_,
                                 policy_, ++sessionId_ ) );
}

} // namespace YumaTest

