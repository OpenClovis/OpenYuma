// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/msg-util/state-data-query-builder.h"

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/lexical_cast.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;
using namespace YumaTest;

// ---------------------------------------------------------------------------|
StateDataQueryBuilder::StateDataQueryBuilder( const string& moduleNs ) 
    : NCMessageBuilder()
    , moduleNs_( moduleNs )
{
}

// ---------------------------------------------------------------------------|
StateDataQueryBuilder::~StateDataQueryBuilder()
{
}

// ---------------------------------------------------------------------------|
string StateDataQueryBuilder::genToasterQuery( const string& op ) const
{
    return genTopLevelContainerText( "toaster", moduleNs_, op );
}

} // namespace YumaTest
