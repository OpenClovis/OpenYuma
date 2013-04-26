#ifndef __SPOOFED_ARGS_H
#define __SPOOFED_ARGS_H

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * This simple structure is used for spoofing command line arguments
 * which are passed to Text Fixtures as the ArgTraits template parameter.
 *
 * Usage: include this file and then simply initialise the internal
 * argv parameters to required 'spoofed' command line
 * arguments, e.g.:
 *
 *     const char* SpoofedArgs::argv[] = {
 *       ( "yuma-test" ),
 *       ( "--modpath=../../../../modules/netconfcentral"
 *                  ":../../../../modules/ietf"
 *                  ":../../../../modules/yang"
 *                  ":../../../../modules/test/pass"
 *       ),
 *       ( "--access-control=off" ),
 *       ( "--target=running" ),
 *    };
 */
struct SpoofedArgs
{
    static const char* argv[];      ///< Spoofed command line arguments.
};

}//  namespace YumaTest

#endif // __SPOOFED_ARGS_H
