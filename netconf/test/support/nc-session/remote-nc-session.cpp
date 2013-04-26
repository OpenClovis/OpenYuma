// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/remote-nc-session.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>

// ---------------------------------------------------------------------------|
// Platform Includes 
// ---------------------------------------------------------------------------|
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------!
namespace YumaTest 
{
   
// ---------------------------------------------------------------------------!
RemoteNCSession::RemoteNCSession( 
        const string& ipAddress, 
        const uint16_t port,
        const string& user, 
        const string& password,
        shared_ptr< AbstractYumaOpLogPolicy > policy,
        uint16_t sessionId ) 
  : SysTestNCSessionBase( user, policy, sessionId )
  , ipAddress_( ipAddress )
  , port_( port )
  , password_( password )
  , channel_( 0 )
{
   ncxsock_ = socket( AF_INET, SOCK_STREAM, 0 );
   assert( ( ncxsock_ >= 0 ) && "Error: socket creation failed!" );

   connect();

   sendHelloResponse();
}

// ---------------------------------------------------------------------------!
RemoteNCSession::~RemoteNCSession()
{
    if( channel_ )
    {
        libssh2_channel_free( channel_ );
    }

    if( session_ )
    {
        libssh2_session_disconnect( session_, "Normal Shutdown" );
        libssh2_session_free( session_ );
    }
}

// ---------------------------------------------------------------------------!
void RemoteNCSession::connect()
{
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons( port_ );
    sin.sin_addr.s_addr = inet_addr( ipAddress_.c_str() );

    if ( 0 != ::connect( ncxsock_, 
                         reinterpret_cast<const sockaddr*>( &sin ), 
                         sizeof( sockaddr_in ) ) )
    {
       assert( false && "NCX Socket Connect Failed!" );
    }

    // Create a session instance and start it up. This will trade welcome
    // banners, exchange keys, and setup crypto, compression, and MAC layers
    session_ = libssh2_session_init();
    assert( session_ &&  "Fatal: Failure initialising SSH session" );

    if (libssh2_session_startup(session_, ncxsock_))
    {
        assert( false && "Fatal: Failure establishing SSH session" );
    }

    connected_ = true;
    // We could authenticate via password
    if ( libssh2_userauth_password( session_, user_.c_str(), password_.c_str()))
    {
        assert( false && "Fatal: Authentication by password failed" );
    }

    // Request a shell
    channel_ = libssh2_channel_open_session( session_ );
    if ( !channel_ )
    {
        assert( false && "Fatal: Unable to open a session" );
    }

    libssh2_channel_subsystem( channel_, "netconf" );

    // set the channel to non-blocking
    libssh2_session_set_blocking( session_, 0 );
}

// ---------------------------------------------------------------------------!
ssize_t RemoteNCSession::receive( char* buffer, const ssize_t length )
{
    ssize_t count = libssh2_channel_read( channel_, buffer, length );
    return count;
}

// ---------------------------------------------------------------------------!
ssize_t RemoteNCSession::send( const char* buffer, const ssize_t length )
{
    ssize_t count = libssh2_channel_write( channel_, buffer, length );
    return count;
}

} // namespace YumaTest

