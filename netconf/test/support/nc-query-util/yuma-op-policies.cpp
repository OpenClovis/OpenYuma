// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/yuma-op-policies.h"
#include "test/support/nc-query-util/nc-query-utils.h"

// ---------------------------------------------------------------------------|
// STD Includes 
// ---------------------------------------------------------------------------|
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Global namespace usage
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
AbstractYumaOpLogPolicy:: AbstractYumaOpLogPolicy( const string& baseDir )
   : baseDir_( baseDir )
{
}

// ---------------------------------------------------------------------------|
AbstractYumaOpLogPolicy::~AbstractYumaOpLogPolicy()
{
}

// ---------------------------------------------------------------------------|
std::string AbstractYumaOpLogPolicy::generateSessionLog( uint16_t sessionId )
{
    ostringstream oss;
    using boost::unit_test::framework::current_test_case;

    oss << baseDir_ << "/" 
        << "Ses" << setw(4) << setfill('0') << sessionId << "-"
        << current_test_case().p_name 
        << ".xml";
    return oss.str();
}

// ---------------------------------------------------------------------------|
MessageIdLogFilenamePolicy::MessageIdLogFilenamePolicy( 
      const string& baseDir )
   : AbstractYumaOpLogPolicy( baseDir )
{
}

// ---------------------------------------------------------------------------|
MessageIdLogFilenamePolicy::~MessageIdLogFilenamePolicy()
{
}

// ---------------------------------------------------------------------------|
string MessageIdLogFilenamePolicy::generate( const string& queryStr,
       uint16_t sessionId ) 
{
    ostringstream oss;
    using boost::unit_test::framework::current_test_case;

    oss << baseDir_ << "/" << current_test_case().p_name 
        << "-Ses" << setw(4) << setfill('0') << sessionId << "-" 
        << setw(4) << setfill('0') << extractMessageIdStr( queryStr) << ".xml";
    
    return oss.str();
}

// ---------------------------------------------------------------------------|
string MessageIdLogFilenamePolicy::generateHelloLog( const uint16_t sessionId ) 
{
    ostringstream oss;
    using boost::unit_test::framework::current_test_case;

    oss << baseDir_ << "/" << current_test_case().p_name 
        << "-Ses" << setw(4) << setfill('0') << sessionId << "-hello.xml";
    
    return oss.str();
}

} // namespace YumaTest
