#ifndef __YUMA_SPOOF_NC_SESSION_FACTORY_H
#define __YUMA_SPOOF_NC_SESSION_FACTORY_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/abstract-nc-session-factory.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractYumaOpLogPolicy;
class AbstractNCSession;

// ---------------------------------------------------------------------------|
/**
 * A session factory for creating Spoofed NC Sessions.
 */
class SpoofNCSessionFactory : public AbstractNCSessionFactory
{
public:
    /** 
     * Constructor.
     *
     * \param policy the output log filename generation policy.
     */
    explicit SpoofNCSessionFactory( 
            std::shared_ptr< AbstractYumaOpLogPolicy > policy );

    /** Destructor */
    virtual ~SpoofNCSessionFactory();

    /** 
     * Create a new NCSession.
     *
     * \return a new NCSession.
     */
    std::shared_ptr<AbstractNCSession> createSession();

};

} // namespace YumaTest

#endif // __YUMA_SPOOF_NC_SESSION_FACTORY_H
