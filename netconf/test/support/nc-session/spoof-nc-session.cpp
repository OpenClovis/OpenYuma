// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/spoof-nc-session.h"
#include "test/support/nc-query-util/nc-query-utils.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <cstdio>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// libxml2
// ---------------------------------------------------------------------------|
#include <libxml/xmlreader.h> 

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|
#include "src/agt/agt_ses.h"
#include "src/agt/agt_top.h"
#include "src/ncx/xml_util.h"

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------!
namespace YumaTest
{

// ---------------------------------------------------------------------------!
SpoofNCSession::SpoofNCSession(
        std::shared_ptr< AbstractYumaOpLogPolicy > policy,
        uint16_t sessionId ) 
   : AbstractNCSession( policy, sessionId )
{
}

// ---------------------------------------------------------------------------!
SpoofNCSession::~SpoofNCSession()
{
}

// ---------------------------------------------------------------------------!
uint16_t SpoofNCSession::injectMessage( const std::string& queryStr )
{
    BOOST_TEST_MESSAGE( "Injecting Test Message..." );

    ses_cb_t* baldScbPtr = configureDummySCB( queryStr );

    // dispatch the message - note this might free baldScbPtr if an
    // error occurred!
    agt_top_dispatch_msg( &baldScbPtr );

    if ( baldScbPtr )
    {
        agt_ses_free_dummy_session( baldScbPtr );
    }

    return extractMessageId( queryStr );
}

// ---------------------------------------------------------------------------!
ses_cb_t* SpoofNCSession::configureDummySCB( const string& queryStr )
{
    ses_cb_t* baldScbPtr = agt_ses_new_dummy_session();
    BOOST_REQUIRE_MESSAGE ( baldScbPtr, 
                            "Failed to generate dummy SCB" );

    // set the session id
    baldScbPtr->sid = sessionId_;

    // get the logfilename
    baldScbPtr->fp = openLogFileForQuery( queryStr );

    // Assign a memory xml reader, note this will be deallocated when
    // baldScbPtr is released.
    baldScbPtr->reader =  xmlReaderForMemory( 
            queryStr.c_str(), queryStr.size(), "", 0, XML_READER_OPTIONS );
    BOOST_REQUIRE_MESSAGE( baldScbPtr->reader,
                           "Error getting xmlTextReaderForMmemory!" );

    return baldScbPtr;
}

// ---------------------------------------------------------------------------!
FILE* SpoofNCSession::openLogFileForQuery( const string& queryStr )
{
    const string logFilename = newQueryLogFilename( queryStr );

    // set the file descriptor for output from yuma
    FILE* fp = fopen( logFilename.c_str(), "w" );
    BOOST_REQUIRE_MESSAGE( fp, "Failed to open file " << logFilename.c_str() );

    // Write out the initial query. to the logfile
    fwrite( queryStr.c_str(), sizeof(char), queryStr.size(), fp );
    fwrite( "\n", sizeof(char), sizeof(char), fp );

    return fp;
}


} // namespace YumaTest
