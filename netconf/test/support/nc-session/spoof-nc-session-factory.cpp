// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-session/spoof-nc-session-factory.h"
#include "test/support/nc-session/spoof-nc-session.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <memory>

// ---------------------------------------------------------------------------|
// File wide namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
SpoofNCSessionFactory::SpoofNCSessionFactory( 
            shared_ptr< AbstractYumaOpLogPolicy > policy )
    : AbstractNCSessionFactory( policy )
{
}

// ---------------------------------------------------------------------------|
SpoofNCSessionFactory::~SpoofNCSessionFactory()
{
}

// ---------------------------------------------------------------------------|
shared_ptr<AbstractNCSession> SpoofNCSessionFactory::createSession()
{
    return shared_ptr<AbstractNCSession> ( 
            new SpoofNCSession( policy_, ++sessionId_ ) );
}

} // namespace YumaTest

