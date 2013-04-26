#ifndef __YUMA_STRING_PRESENCE_CHECKERS_H
#define __YUMA_STRING_PRESENCE_CHECKERS_H

// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Check the results of a netconf query and ensure that all the
 * supplied strings are present.
 */
class StringPresenceChecker
{
public:
    /**
     * Constructor.
     *
     * \param expPresentText a list of strings that must be
     *                       present in the reply for the test to pass
     */
    explicit StringPresenceChecker( const std::vector< std::string >& expPresentText );

    /** 
     * Destructor
     */
    ~StringPresenceChecker();

    /**
     * Check the query results
     *
     * \param queryResult the query to check,
     */
    void operator()( const std::string& queryResult ) const;

private:
    /** list of strings that must be present in the output */
    const std::vector< std::string >& checkStrings_;  
};

/**
 * Check the results of a netconf query and ensure that all the
 * supplied strings are not present.
 */
class StringNonPresenceChecker
{
public:
     /**
      * Constructor.
      *
      * \param expPresentText a list of strings that must be
      *                       present in the reply for the test to pass
     */
    explicit StringNonPresenceChecker( const std::vector< std::string >& expNotPresentText );

    /** 
     * Destructor
     */
    ~StringNonPresenceChecker();

    /**
     * Check the query results
     *
     * \param queryResult the query to check,
     */
    void operator()( const std::string& queryResult ) const;

private:
    /** list of strings that must be present in the output */
    const std::vector< std::string >& checkStrings_;  
};

/**
 * Convenience Checker for checking for the presence and absence of
 * strings.
 */
class StringsPresentNotPresentChecker
{
public:
     /**
      * Constructor.
      *
      * \param expPresentReplyText a list of strings that must be
      *                            present in the reply for the test to pass
      * \param expAbsentReplyText  a list of strings that must NOT be
      *                            present in the reply for the test to pass
     */
    StringsPresentNotPresentChecker( 
            const std::vector< std::string >& expPresentReplyText, 
            const std::vector< std::string >& expAbsentReplyText );

    /** 
     * Destructor
     */
    ~StringsPresentNotPresentChecker();

    /**
     * Check the query results
     *
     * \param queryResult the query to check,
     */
    void operator()( const std::string& queryResult ) const;

private:
    /** checker for list of strings that must be present in the output */
    StringPresenceChecker expPresentChecker_;

    /** checker for list of strings that must not be present in the output */
    StringNonPresenceChecker expNotPresentChecker_;
};

} // namespace YumaTest

#endif // __YUMA_STRING_PRESENCE_CHECKER_H

