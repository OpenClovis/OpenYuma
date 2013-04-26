// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/misc-util/ptree-utils.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Filewide namespace usage
// ---------------------------------------------------------------------------|
using namespace std;
using boost::property_tree::ptree;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
namespace PTreeUtils
{

// ---------------------------------------------------------------------------|
void Display( const ptree& pt)
{
    BOOST_FOREACH( const ptree::value_type& v, pt )
    {
        if ( v.first == "xpo" )
        {
            cout << "Node is xpo\n";
        }
        cout << v.first << " : " << v.second.get_value<string>() << "\n";
        Display( v.second );
    }
}

// ---------------------------------------------------------------------------|
ptree ParseXMlString( const string& xmlString )
{
    using namespace boost::property_tree::xml_parser;
    // Create empty property tree object
    ptree pt;
    istringstream ss( xmlString ); 
    read_xml( ss, pt, trim_whitespace );
    return pt;
}

}} // namespace YumaTest::PTreeUtils
