// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/db-models/state-data-test-db.h"
#include "test/support/misc-util/ptree-utils.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/foreach.hpp>


// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;
using boost::property_tree::ptree;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ===========================================================================|
ToasterContainer::ToasterContainer() : toasterManufacturer_("")
                                     , toasterModelNumber_("")
                                     , toasterStatus_("")
{}

// ---------------------------------------------------------------------------|
bool ToasterContainer::unpackItem( const ptree::value_type& v )
{
    bool res = true;

    if ( v.first == "toasterManufacturer" )
    {
        toasterManufacturer_ = v.second.get_value<std::string>();
    }
    else if ( v.first == "toasterModelNumber" )
    {
        toasterModelNumber_ = v.second.get_value<std::string>();
    }
    else if ( v.first == "toasterStatus" )
    {
        toasterStatus_ = v.second.get_value<std::string>();
    }
    else
    {
        res = false;
    }

    return res;
}
// ---------------------------------------------------------------------------|
ToasterContainer::ToasterContainer( const boost::property_tree::ptree& pt )
    : toasterManufacturer_("")
    , toasterModelNumber_("")
    , toasterStatus_("")
{
    const ptree& toaster = pt.get_child( "data.toaster", ptree() );

    BOOST_FOREACH( const ptree::value_type& v, toaster )
    {
        if ( !unpackItem( v ) &&  v.first != "<xmlattr>")
        {
            BOOST_FAIL( "Unsupported child for Component: " << v.first );
        }
    }
}

// ---------------------------------------------------------------------------|
void checkEqual( const ToasterContainer& lhs, const ToasterContainer& rhs )
{
    BOOST_CHECK_EQUAL( lhs.toasterManufacturer_, rhs.toasterManufacturer_ );
    BOOST_CHECK_EQUAL( lhs.toasterModelNumber_, rhs.toasterModelNumber_ );
    BOOST_CHECK_EQUAL( lhs.toasterStatus_, rhs.toasterStatus_ );
}

} // namespace ToasterNetconfIntegrationTest
