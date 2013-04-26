#ifndef __YUMA_BOOST_TEST_LOGGING_UTIL_H
#define __YUMA_BOOST_TEST_LOGGING_UTIL_H

#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
// This file contains a number of simple wrapper functions around
// BOOST_TEST_MESSAGE that provide formatted test output.
//
namespace YumaTest
{

// ---------------------------------------------------------------------------|
#define USE_COLORED_PRINTING

#ifdef USE_COLORED_PRINTING
#define GREY_BOLD    "\033[30;1m"
#define RED_BOLD     "\033[31;1m"
#define GREEN_BOLD   "\033[32;1m"
#define YELLOW_BOLD  "\033[33;1m"
#define BLUE_BOLD    "\033[34;1m"
#define PURPLE_BOLD  "\033[35;1m"
#define CYAN_BOLD    "\033[36;1m"
#define WHITE_BOLD   "\033[37;1m"
#define BLACK_BOLD   "\033[39;1m"
#define BLACK_NORMAL "\033[39;0m"
#else
#define GREY_BOLD    ""
#define RED_BOLD     ""
#define GREEN_BOLD   ""
#define YELLOW_BOLD  ""
#define BLUE_BOLD    ""
#define PURPLE_BOLD  ""
#define CYAN_BOLD    ""
#define WHITE_BOLD   ""
#define BLACK_BOLD   ""
#define BLACK_NORMAL ""
#endif

#define HIGHLIGHT_ON BLACK_BOLD
#define DEFAULT_TEXT BLACK_NORMAL

// ---------------------------------------------------------------------------|
/** 
 * Utility function for simply printing a line between tests. 
 *
 * \param chr the character to use in the line break
 * \param force true if the break should sent to std::cout, false if 
 *              BOOST_LOG_MESSAGE should be used
 */
void DisplayTestBreak( char chr='=', bool force=false );

// ---------------------------------------------------------------------------|
/**
 * Utility function for displaying a test description.
 *
 * \param title the title of the test.
 */
void DisplayTestTitle( const std::string& title );

// ---------------------------------------------------------------------------|
/**
 * Utility function for displaying a test description.
 *
 * \param brief the brief description for the test.
 * \param desc the detailed description.
 * \param notes some additional text
 */
void DisplayTestDescrption( const std::string& brief, 
                            const std::string& desc = std::string(),
                            const std::string& notes = std::string() );

// ---------------------------------------------------------------------------|
/**
 * Utility function to display the current test summary.
 */
void DisplayCurrentTestSummary( const char* highlight = HIGHLIGHT_ON );

// ---------------------------------------------------------------------------|
/**
 * Utility function to display the current test summary.
 */
void DisplayCurrentTestId( const char* highlight = HIGHLIGHT_ON );

// ---------------------------------------------------------------------------|
/**
 * Utility function to display the result of the current test.
 */
void DisplayCurrentTestResult();

} // namespace YumaTest

#endif // __YUMA_BOOST_TEST_LOGGING_UTIL_H
