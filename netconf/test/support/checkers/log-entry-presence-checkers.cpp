// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/checkers/log-entry-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <sstream>
#include <iostream>
#include <fstream>

#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
LogEntryPresenceChecker::LogEntryPresenceChecker( 
        const vector<string>& expPresentText )
    : checkStrings_( expPresentText )
{
}

// ---------------------------------------------------------------------------|
LogEntryPresenceChecker::~LogEntryPresenceChecker()
{
}

// ---------------------------------------------------------------------------|
void LogEntryPresenceChecker::operator()( const string& logFilePath ) const
{
    ifstream logFile;
    string line;
    bool entryFound;

    BOOST_FOREACH ( const string& val, checkStrings_ )
    {
        entryFound = false;
        BOOST_TEST_MESSAGE( "\tChecking " << val << " is present" );
        logFile.open( logFilePath );
        if (logFile.is_open()) {
             while ( logFile.good() && entryFound == false ) {
                getline(logFile, line);
                if ( line.find( val ) != string::npos ) {
                    entryFound = true;
                }
            }
            logFile.close();
        }
        BOOST_CHECK_EQUAL( true, entryFound );
    }
}

// ---------------------------------------------------------------------------|
LogEntryNonPresenceChecker::LogEntryNonPresenceChecker( 
        const vector<string>& expNotPresentText )
    : checkStrings_( expNotPresentText )
{
}

// ---------------------------------------------------------------------------|
LogEntryNonPresenceChecker::~LogEntryNonPresenceChecker()
{
}

// ---------------------------------------------------------------------------|
void LogEntryNonPresenceChecker::operator()( const string& logFilePath ) const
{
    ifstream logFile;
    string line;
    bool entryFound;

    BOOST_FOREACH ( const string& val, checkStrings_ )
    {
        entryFound = false;
        BOOST_TEST_MESSAGE( "\tChecking " << val << " is NOT present" );
        logFile.open( logFilePath );
        if (logFile.is_open()) {
            while ( logFile.good() && entryFound == false ) {
                getline(logFile, line);
                if ( line.find( val ) != string::npos ) {
                    entryFound = true;
                }
            }
            logFile.close();
        }
        BOOST_CHECK_NE( true, entryFound );
    }
}

// ---------------------------------------------------------------------------|
LogEntriesPresentNotPresentChecker::LogEntriesPresentNotPresentChecker( 
        const vector< string >& expPresentLogText, 
        const vector< string >& expAbsentLogText )
   : expPresentChecker_( expPresentLogText )
   , expNotPresentChecker_( expAbsentLogText )
{
}

// ---------------------------------------------------------------------------|
LogEntriesPresentNotPresentChecker::~LogEntriesPresentNotPresentChecker() 
{
}

// ---------------------------------------------------------------------------|
void LogEntriesPresentNotPresentChecker::operator()( const string& logFilePath ) const
{
    expPresentChecker_( logFilePath ); 
    expNotPresentChecker_( logFilePath );
}

} // namespace YumaTest
