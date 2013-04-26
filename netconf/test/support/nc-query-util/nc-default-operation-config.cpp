// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-default-operation-config.h"
#include "test/support/msg-util/NCMessageBuilder.h"

// ---------------------------------------------------------------------------|
// File scope namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
DefaultOperationConfig::DefaultOperationConfig( 
        std::shared_ptr< NCMessageBuilder > builder,
        const std::string& newDefaultOperation )
    : builder_( builder )
    , orignalOperation_( builder_->getDefaultOperation() )
{
    builder_->setDefaultOperation( newDefaultOperation );
}

// ---------------------------------------------------------------------------|
DefaultOperationConfig::~DefaultOperationConfig()
{
    builder_->setDefaultOperation( orignalOperation_ );
}

} // namespace YumaTest
