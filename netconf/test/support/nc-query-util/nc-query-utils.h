#ifndef __NC_QUERY_UTILS_H
#define __NC_QUERY_UTILS_H

// ---------------------------------------------------------------------------|
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------!
/**
 * Extract the message id from the supplied XML query.
 *
 * \param queryStr an XML query.
 * \return the message id as a string
 *
 */
std::string extractMessageIdStr( const std::string& queryStr );

/**
 * Extract the message id from the supplied XML query.
 *
 * \param queryStr an XML query.
 * \return the message id as a unit16_t
 *
 */
uint16_t extractMessageId( const std::string& queryStr );



} // namespace YumaTest

#endif // __NC_QUERY_UTILS_H
