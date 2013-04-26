#ifndef __DEVICE_TEST_DATABASE_DEBUG_H
#define __DEVICE_TEST_DATABASE_DEBUG_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
struct ResourceNode;
struct ConnectionItem;
struct StreamItem;
struct StreamConnectionItem;
struct Profile;
struct XPO3Container;

/** 
 * Display the contents of a StreamConnectionItem.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const StreamConnectionItem& it );

/** 
 * Display the contents of a StreamConnectionItem.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const ConnectionItem& it );

/** 
 * Display the contents of a ConnectionItem.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const ResourceNode& it );

/** 
 * Display the contents of a ResourceNode.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const StreamItem& it );

/** 
 * Display the contents of a StreamItem.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const Profile& it );

/** 
 * Display the contents of a XPO3Container.
 *
 * \param out the output stream.
 * \param it the item to display.
 */
std::ostream& operator<<( std::ostream& o, const XPO3Container& it );

} // namespace YumaTest

#endif // __DEVICE_TEST_DATABASE_DEBUG_H
