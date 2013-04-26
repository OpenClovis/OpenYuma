#ifndef __DB_CHECK_UTILS_H
#define __DB_CHECK_UTILS_H

// ---------------------------------------------------------------------------|
// Boost includes
// ---------------------------------------------------------------------------|
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp> 
#include <boost/phoenix/fusion/at.hpp> 
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/tuple.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
template <class T>
void MapZipContainerHelper( const boost::tuple< const T&, const T& >& zIter )
{
    using namespace std;

    BOOST_CHECK_EQUAL( zIter.get<0>().first, zIter.get<1>().first );
    checkEqual( zIter.get<0>().second, zIter.get<1>().second );
}

// ---------------------------------------------------------------------------|
template <class T>
void CheckMaps( const T& lhs, const T& rhs )
{
    using namespace std;
    BOOST_REQUIRE_EQUAL( lhs.size(), rhs.size() );

    auto begZip = boost::make_zip_iterator( boost::make_tuple( lhs.begin(), rhs.begin() ) );
    auto endZip = boost::make_zip_iterator( boost::make_tuple( lhs.end(), rhs.end() ) );

    typedef typename T::value_type value_type;
    std::for_each( begZip, endZip, 
                   boost::phoenix::bind( &MapZipContainerHelper<value_type>, 
                   boost::phoenix::arg_names::arg1 ) );
}

} // namespace YumaTest

#endif // __DB_CHECK_UTILS_H
