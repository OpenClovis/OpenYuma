// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/abstract-nc-session.h"
#include "test/support/nc-query-util/nc-query-utils.h"
#include "test/support/nc-query-util/yuma-op-policies.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <fstream>
#include <sstream>
#include <boost/range/algorithm.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp> 
#include <boost/phoenix/fusion/at.hpp> 
#include <boost/fusion/include/std_pair.hpp>

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
    string lineBreak( 75, '-' );
    string testBreak( 75, '=' );
}

// ---------------------------------------------------------------------------!
namespace YumaTest
{

// ---------------------------------------------------------------------------!
AbstractNCSession::AbstractNCSession( 
        shared_ptr< AbstractYumaOpLogPolicy > policy, uint16_t sessionId )
    : sessionId_( sessionId ) 
    , messageCount_( 0 ) 
    , opLogFilenamePolicy_( policy )
    , messageIdToLogMap_()
{
}

// ---------------------------------------------------------------------------!
AbstractNCSession::~AbstractNCSession()
{
    concatenateLogFiles();
}

// ---------------------------------------------------------------------------!
void AbstractNCSession::concatenateLogFiles()
{
    namespace ph = boost::phoenix;
    using ph::arg_names::arg1;

    string logName = opLogFilenamePolicy_->generateSessionLog( sessionId_ );
    ofstream op( logName );
    BOOST_REQUIRE_MESSAGE( op, "Error opening output logfile: " << logName );

    op << testBreak << "\n\n";

    std::vector< uint16_t > keys;
    boost::transform( messageIdToLogMap_, back_inserter( keys ), 
                      ph::at_c<0>( arg1 ) );

    auto sorted = boost::sort( keys );

    boost::for_each( keys, 
            ph::bind( &AbstractNCSession::appendLog, this,  ph::ref( op ), arg1 ) );
}

// ---------------------------------------------------------------------------!
void AbstractNCSession::appendLog( ofstream& op, uint16_t key )
{
    ifstream in( messageIdToLogMap_[key] );
    BOOST_REQUIRE_MESSAGE( in, "Error opening input logfile: " 
                           << messageIdToLogMap_[key] );
    
    op << in.rdbuf() << "\n";
    op << lineBreak << "\n\n";
}

// ---------------------------------------------------------------------------!
string AbstractNCSession::newQueryLogFilename( const string& queryStr )
{
    uint16_t messageId = extractMessageId( queryStr );

    BOOST_REQUIRE_MESSAGE( 
            ( messageIdToLogMap_.find( messageId ) == messageIdToLogMap_.end() ), 
            "An entry already exists for message id: " << messageId );
    
    string logFilename ( opLogFilenamePolicy_->generate( queryStr, sessionId_ ) );
    messageIdToLogMap_[ messageId ] = logFilename;

    return logFilename;
}

// ---------------------------------------------------------------------------!
string AbstractNCSession::retrieveLogFilename( const uint16_t messageId ) const
{
    auto iter = messageIdToLogMap_.find( messageId );
    BOOST_REQUIRE_MESSAGE( iter != messageIdToLogMap_.end(),
           "Entry Not found for messageId: " << messageId );

    string logFilename = iter->second;
    if ( logFilename.empty() )
    {
        BOOST_FAIL( "Empty log filename for Message Id: " << messageId );
    }

    return logFilename;
}

// ---------------------------------------------------------------------------!
string AbstractNCSession::getSessionResult( const uint16_t messageId ) const
{
    // get the filename
    string logFilename( retrieveLogFilename( messageId ) );

    // open the file
    ifstream inFile( logFilename.c_str() );
    if ( !inFile )
    {
        BOOST_FAIL( "Error opening file for Yuma output" );
    }

    // read the result
    ostringstream oss;
    oss << inFile.rdbuf();

    return oss.str();
}

} // namespace YumaTest
