// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/misc-util/cpp-unit-op-formatter.h"

// ---------------------------------------------------------------------------|
// Boost test include files.
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test_suite_impl.hpp>
#include <boost/test/results_collector.hpp>

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;
using namespace boost::unit_test;

// ---------------------------------------------------------------------------|
// Anonymous namespace 
// ---------------------------------------------------------------------------|
namespace {

class CppUnitSuiteVisitor : public test_tree_visitor
{
public:
    explicit CppUnitSuiteVisitor( const string& name ) : name_( name )
    {}

    virtual void visit( const test_case& tu )
    {
        const test_results& tr = results_collector.results( tu.p_id );
        cout << name_ << "::" << tu.p_name << " : " << ( tr.passed() ? "OK\n" : "FAIL\n" );
    }
private:
    string name_;
};

} // anonymous namesapce

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void CppUnitOpFormatter::do_confirmation_report( 
        test_unit const& tu, std::ostream& ostr )
{
    using boost::unit_test::output::plain_report_formatter;

    CppUnitSuiteVisitor visitor( tu.p_name );
    traverse_test_tree( tu, visitor );
    
    const test_results& tr = results_collector.results( tu.p_id );
    if( tr.passed() ) 
    {
        ostr << "Test Passed\n";
    }
    else
    {
        plain_report_formatter::do_confirmation_report( tu, ostr );
    }
}

} // namespace YumaTest
