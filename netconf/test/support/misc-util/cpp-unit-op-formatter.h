#ifndef __CPP_UNIT_OP_FORMATTER_H
#define __CPP_UNIT_OP_FORMATTER_H

#include <boost/test/output/plain_report_formatter.hpp>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Boost test output formatter to output test results in a format that
 * mimics cpp unit.
 */
class CppUnitOpFormatter : public boost::unit_test::output::plain_report_formatter
{
public:
    /**
     * Overidden to provide output that is compatible with cpp unit.
     *
     * \param tu the top level test unit.
     * \param ostr the output stream
     */
    virtual void do_confirmation_report( boost::unit_test::test_unit const& tu, 
                                         std::ostream& ostr );
};

} // YumaTest

#endif // __CPP_UNIT_OP_FORMATTER_H
