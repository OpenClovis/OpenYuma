#ifndef __STATE_DATA_TEST_DATABASE_H
#define __STATE_DATA_TEST_DATABASE_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <map>
#include <string>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
struct Component;

// ---------------------------------------------------------------------------|
/* Convenience typedefs. */
typedef boost::optional<std::string> opt_string_t;

// ---------------------------------------------------------------------------|
struct ToasterContainer
{
    std::string toasterManufacturer_;
    std::string toasterModelNumber_;
    std::string toasterStatus_;

    /** Constructor */
    ToasterContainer();

    /** 
     * Constructor. Populate from a parsed xml property tree.
     *
     * \param ptree the parsed xml representing the object to construct. 
     */
    explicit ToasterContainer( const boost::property_tree::ptree& pt );

    /**
     * Unpack a property tree value.
     *
     * \param v the value to unpack
     * \return true if the item was unpacked.
     */
    bool unpackItem( const boost::property_tree::ptree::value_type& v );

    /** 
     * Clear the contents of the container.
     */
//    void clear();
};

// ---------------------------------------------------------------------------|
/**
 * Utility class to support test harness setting of ToasterContainers.
 * This class supplies optional parameters for each element in a
 * Toaster container. 
 *
 * Example to only set the toaster manufacturer details,
 * use this class as follows:
 *
 * ToasterContainerConfig connCfg{ string ( "Acme" ),
 *                                 boost::optional<std::string>(),
                                   boost::optional<uint32_t>() };
 */
struct ToasterContainerConfig
{
    opt_string_t toasterManufacturer_;  ///< optional value for toaster manufacturer
    opt_string_t toasterModelNumber_;   ///< optional value for toaster model number
    opt_string_t toasterStatus_;        ///< optional value for toaster status
};

/**
 * Check if two ToasterContainers are equal.
 *
 * \param lhs the lhs containing expected results for comparison.
 * \param rhs the rhs containing expected results for comparison.
 */
void checkEqual( const ToasterContainer& lhs, const ToasterContainer& rhs );

} // namespace YumaTest

#endif // __STATE_DATA_TEST_DATABASE_H

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
