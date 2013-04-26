#ifndef __YUMA_CHECKER_GROUP_H
#define __YUMA_CHECKER_GROUP_H

// ---------------------------------------------------------------------------|
#include <string>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Utility class for grouping checkers together so that multiple
 * different checks can be performed on a test query output.
 */
class CheckerGroup
{
public:
    /** Convenience typedef. */
    typedef std::function< void ( const std::string& ) > CheckSignature_T;

public:
    /** Constructor */
    CheckerGroup();

    /** Destructor */
    ~CheckerGroup() ;

    /** 
     * Run the registered checkers.
     *
     * \param queryResult the query to check,
     */
    void operator()( const std::string& str );

    /**
     * Register a checker, that matches the CheckSignature_T.
     *
     * \param checker the checker to register.
     * \return a reference to this object.
     */
    CheckerGroup& registerChecker( CheckSignature_T& checker );

    /**
     * Convenience initialisation operator.
     *
     * \param checker the checker to register.
     * \return a reference to this object.
     */
    CheckerGroup& operator()( CheckSignature_T& checker );

private:
    std::vector< CheckSignature_T > checkers_;
};

} // namespace YumaTest

#endif // __YUMA_CHECKER_GROUP_H

