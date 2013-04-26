#ifndef __YUMA_LOG_ENTRY_PRESENCE_CHECKERS_H
#define __YUMA_LOG_ENTRY_PRESENCE_CHECKERS_H

// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Check the netconf log includes all the supplied strings.
 */
class LogEntryPresenceChecker
{
public:
    /**
     * Constructor.
     *
     * \param expPresentText a list of strings that must be
     *                       present in the log for the test to pass
     */
    explicit LogEntryPresenceChecker( const std::vector< std::string >& expPresentText );

    /** 
     * Destructor
     */
    ~LogEntryPresenceChecker();

    /**
     * Check the log results
     *
     * \param logFilePath the path/name of the log file to check
     */
    void operator()( const std::string& logFilePath ) const;

private:
    /** list of strings that must be present in the log */
    const std::vector< std::string >& checkStrings_;  
};

/**
 * Check the netconf log excludes all the supplied strings.
 */
class LogEntryNonPresenceChecker
{
public:
     /**
      * Constructor.
      *
      * \param expNotPresentText a list of strings that must not be
      *                       present in the log for the test to pass
     */
    explicit LogEntryNonPresenceChecker( const std::vector< std::string >& expNotPresentText );

    /** 
     * Destructor
     */
    ~LogEntryNonPresenceChecker();

    /**
     * Check the query results
     *
     * \param logFilePath the path/name of the log file to check
     */
    void operator()( const std::string& logFilePath ) const;

private:
    /** list of strings that must not be present in the output */
    const std::vector< std::string >& checkStrings_;  
};

/**
 * Convenience Checker for checking for the presence and absence of
 * log entries.
 */
class LogEntriesPresentNotPresentChecker
{
public:
     /**
      * Constructor.
      *
      * \param expPresentReplyText a list of strings that must be
      *                            present in the netconf log for the test to pass
      * \param expAbsentReplyText  a list of strings that must NOT be
      *                            present in the netconf log for the test to pass
     */
    LogEntriesPresentNotPresentChecker( 
            const std::vector< std::string >& expPresentLogText, 
            const std::vector< std::string >& expAbsentLogText );

    /** 
     * Destructor
     */
    ~LogEntriesPresentNotPresentChecker();

    /**
     * Check the query results
     *
     * \param logFilePath the path/name of the log file to check
     */
    void operator()( const std::string& logFilePath ) const;

private:
    /** checker for list of entries that must be present in the log */
    LogEntryPresenceChecker expPresentChecker_;

    /** checker for list of entries that must not be present in the log */
    LogEntryNonPresenceChecker expNotPresentChecker_;
};

} // namespace YumaTest

#endif // __YUMA_LOG_ENTRY_PRESENCE_CHECKERS_H

