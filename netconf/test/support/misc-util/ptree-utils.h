#ifndef __PTREE_UTILS_H
#define __PTREE_UTILS_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <string>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/property_tree/ptree.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
namespace PTreeUtils
{

/**
 * This file contains a number of utils to aid with accessing and
 * manipulating boost::property trees.
 */

/**
 * Display a property tree.
 * this isa  adebug utility thaht walks the entire property tree
 * printing out all entries.
 *
 * \param pt the property tree to walk.
 */
void Display( const boost::property_tree::ptree& pt );

/**
 * Populate from XML string.
 * Parse the xml formatted string into a property tree.
 *
 * \return A property tree;
 */
boost::property_tree::ptree ParseXMlString( const std::string& xmlString );

}} // namespace YumaTest::PTreeUtils

#endif // __PTREE_UTILS_H
