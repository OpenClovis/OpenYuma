#ifndef __YUMA_SPOOF_NC_SESSION_H
#define __YUMA_SPOOF_NC_SESSION_H

// ---------------------------------------------------------------------------|
// test Harness Includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/abstract-nc-session.h"

// ---------------------------------------------------------------------------|
// Standard Includes
// ---------------------------------------------------------------------------|
#include <string>
#include <cstdint>
#include <cstdio>
#include <memory>

// ---------------------------------------------------------------------------|
// Yuma Includes
// ---------------------------------------------------------------------------|
#include "src/ncx/ses.h"

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * A 'Spoofed' Netconf session. This class is used by integration test
 * harnesses to inject queries directly into Yuma by calling
 * agt_top_dispatch_msg directly. It is responsible for spoofing all
 * configuration required by Yuma for the operation to succeed.
 */
class SpoofNCSession : public AbstractNCSession
{
public:
    /** utility typedef of shared pointer to ses_cb_t */
    typedef std::shared_ptr<ses_cb_t> SharedSCB_T;

    /** Constructor. 
     * 
     * \param policy the log filename generation policy
     * \param sessionId the id of the session
     */
    SpoofNCSession( std::shared_ptr< AbstractYumaOpLogPolicy > policy,
                    uint16_t sessionId );

    /** Destructor */
    virtual ~SpoofNCSession();

    /** 
     * Inject the query into netconf. This test function spoofs up the
     * structures required for calling agt_top_dispatch_msg(), and
     * makes the call, redirecting all output to an appropriate
     * logfile.
     *
     * \param queryStr a string containing the XML message to inject.
     * \return the id of the message used to inject the query.
     */
    virtual uint16_t injectMessage( const std::string& queryStr );

private:
    /** 
     * Create and configure the dummy SCB structure.
     *
     * \param query a string containing the XML message to inject.
     * \return a 'bald' pointer to an ses_cb_t.
     */
    ses_cb_t* configureDummySCB( const std::string& queryStr );

    /**
     * open output logfile.
     *
     * \param queryStr a string containing the XML message to inject.
     * \return a FILE* opened with fopen.
     */
    FILE* openLogFileForQuery( const std::string& queryStr );

};

} // namespace YumaTest

#endif // __YUMA_NC_SESSION_H
