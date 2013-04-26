// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/sys-test-nc-session-base.h"
#include "test/support/nc-query-util/nc-query-utils.h"
#include "test/support/nc-query-util/nc-strings.h"
#include "test/support/misc-util/array-deleter.h"
#include "test/support/msg-util/NCMessageBuilder.h"
#include "test/support/nc-query-util/yuma-op-policies.h"
#include "test/support/fixtures/test-context.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <stdexcept>
#include <cstring>
#include <libssh2.h>

// ---------------------------------------------------------------------------|
// Boost Includes 
// ---------------------------------------------------------------------------|
#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
// Anonymous namespace
// ---------------------------------------------------------------------------|
namespace
{
    /// The maximum time to wait for blocking IO
    const uint16_t MaxReadWaitSeconds = 20;      
}

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------!
SysTestNCSessionBase::SysTestNCSessionBase( 
        const string& user, 
        shared_ptr< AbstractYumaOpLogPolicy > policy,
        uint16_t sessionId ) 
   : AbstractNCSession( policy, sessionId )
   , ncxsock_( -1 )
   , connected_( false )
   , user_( user )
{
}

// ---------------------------------------------------------------------------!
SysTestNCSessionBase::~SysTestNCSessionBase() 
{
   if ( connected_ )
   {
       close( ncxsock_ );
   }  
}

// ---------------------------------------------------------------------------!
uint16_t SysTestNCSessionBase::injectMessage( const string& queryStr )
{
    // open an output log file 
    shared_ptr<ofstream> out = openLogFileForQuery( queryStr );

    // write out the initial query to the logfile
    *out<< queryStr << "\n";

    sendMessage( queryStr );

    // read the reply and log it
    const string replyStr = receiveMessage();
    *out << replyStr << "\n";

    return extractMessageId( queryStr );
}

// ---------------------------------------------------------------------------!
void SysTestNCSessionBase::sendMessage( const string& queryStr )
{
    // TODO: Commonalise with receive ... 
    const string fullQuery = queryStr + NC_SSH_END;
    BOOST_REQUIRE_MESSAGE( connected_, "ERROR Not connected to server!" );

    ssize_t remainingLength = fullQuery.length();
    const char* currentPosition = fullQuery.c_str();
    ssize_t sentLength;
    uint16_t numRemainingRetries = MaxReadWaitSeconds * 1000;

    while( remainingLength && numRemainingRetries )
    { 
        sentLength = send(currentPosition, remainingLength);

        if ( sentLength ==0 || sentLength == LIBSSH2_ERROR_EAGAIN )
        {
            --numRemainingRetries;
            usleep( 1000 );
        }
        else if ( sentLength < 0 )
        {
                BOOST_FAIL( "Yuma IO Opeartion Failed!" );
        }
        else if ( sentLength > 0 )
        {
            currentPosition += sentLength;
            remainingLength -= sentLength;
        }
    }

    if ( remainingLength )
    {
        BOOST_FAIL( "Failed to send the full message, sent( " 
                    << fullQuery.length() - remainingLength 
                    << " ) bytes, attempted( " << fullQuery.length() << " )" );
    }
}

// ---------------------------------------------------------------------------!
void SysTestNCSessionBase::sendHelloResponse()
{
    using namespace boost::xpressive;

    // write out the initial query to the logfile
    const string capabilities = receiveMessage();

    // extract the session id as assigned by the remote agent
    sregex sesIdRegex_( sregex::compile( "<session-id>(\\d+)<" ) ); 
    smatch what;
    BOOST_REQUIRE_MESSAGE( regex_search( capabilities, what, sesIdRegex_ ),
                           "Session ID not found in capabilities message!" );
    sessionId_ = boost::lexical_cast<uint16_t>( what[1] );

    // open an output log file 
    const string helloLogFile( 
            opLogFilenamePolicy_->generateHelloLog( sessionId_ ) );
    shared_ptr<ofstream> out = openLogFile( helloLogFile );

    // the hello message is a special case and is always added as id 0.
    messageIdToLogMap_[ 0 ] = helloLogFile;

    *out<< capabilities << "\n";

    // verify that the agent is running in the same mode as the test
    string dbName( TestContext::getTestContext()->writeableDbName_ );
    if  ( string::npos == capabilities.find( dbName ) )
    {
        BOOST_FAIL( "Agent is not in '" << dbName << "' mode!" );
    }

    // verify that the agent has the startup capability enabled if required
    bool usingStartup =  TestContext::getTestContext()->usingStartup_;
    if  ( usingStartup && (string::npos == capabilities.find( "startup" )) )
    {
        BOOST_FAIL( "Agent has not enabled the startup capability!" );
    }

    const string helloMsg = NCMessageBuilder().buildHelloMessage();
    sendMessage( helloMsg );
    *out<< helloMsg << "\n";
}

// ---------------------------------------------------------------------------!
string SysTestNCSessionBase::receiveMessage()
{
    BOOST_REQUIRE_MESSAGE( connected_, "ERROR Not connected to server!" );

    const ssize_t maxBufLen = 16*1024;
    shared_ptr<char> buffer( new char[maxBufLen], ArrayDeleter<char>() );
    ssize_t remainingLength = maxBufLen;
    char* currentPosition = buffer.get();
    ssize_t receivedLength;
    uint16_t numRemainingRetries = MaxReadWaitSeconds * 1000;
  
    // Note: the use case is send a message to the netconf server
    //       then wait for a reply, ergo we will never received more
    //       than one full message at a time.
    while( remainingLength && numRemainingRetries )
    {
        receivedLength = receive(currentPosition, remainingLength);

        if ( receivedLength == 0  || receivedLength == LIBSSH2_ERROR_EAGAIN )
        {
            --numRemainingRetries;
            usleep( 1000 );
        }
        else if ( receivedLength < 0 )
        {
            BOOST_FAIL( "Yuma IO Opeartion Failed!" );
        }
        else if ( receivedLength > 0 )
        {
            *(currentPosition+receivedLength)='\0';
            if ( strstr( currentPosition, NC_SSH_END ) )
            {
                string message( buffer.get() );
                return message;
            }
            currentPosition += receivedLength;
            remainingLength -= receivedLength;
        }
    }

    return string();
}

// ---------------------------------------------------------------------------!
shared_ptr<ofstream> SysTestNCSessionBase::openLogFileForQuery( const string& queryStr )
{
    const string logFilename = newQueryLogFilename( queryStr );

    return openLogFile( logFilename );
}

// ---------------------------------------------------------------------------!
shared_ptr<ofstream> SysTestNCSessionBase::openLogFile( const string& logFilename )
{
    shared_ptr<ofstream> outFile( new ofstream( logFilename.c_str() ) );
    BOOST_REQUIRE_MESSAGE( *outFile, "Failed to open file " << logFilename.c_str() );

    return outFile;
}

} // namespace YumaTest
