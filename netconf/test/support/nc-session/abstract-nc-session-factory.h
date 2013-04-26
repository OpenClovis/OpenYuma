#ifndef __YUMA_ABSTRACT_NC_SESSION_FACTORY_H
#define __YUMA_ABSTRACT_NC_SESSION_FACTORY_H

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
 * An abstract NC Session Factory.
 */
class AbstractNCSessionFactory
{
public:
    /** 
     * Constructor.
     */
    explicit AbstractNCSessionFactory(
            std::shared_ptr< AbstractYumaOpLogPolicy > policy )
        : policy_( policy )
        , sessionId_( 0 )
    {}

    /** Destructor */
    virtual ~AbstractNCSessionFactory()
    {}

    /** 
     * Create a new NCSession.
     *
     * \return a new NCSession.
     */
    virtual std::shared_ptr<AbstractNCSession> createSession() = 0;

protected:
    /** The policy for generating log filenames. */
    std::shared_ptr< AbstractYumaOpLogPolicy > policy_; 

    /** The current sessionId */
    uint16_t sessionId_;
};

} // namespace YumaTest

#endif // __YUMA_ABSTRACT_NC_SESSION_FACTORY_H
