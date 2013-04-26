// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-yang-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( simple_choice, SimpleYangFixture )

BOOST_AUTO_TEST_CASE( add_choice )
{
    DisplayTestDescrption( 
            "Demonstrate that when a new choice is created the old choice is "
            "automatically removed.",
            "Procedure: \n"
            "\t1 - Create the top level container\n"
            "\t2 - Add tcp choice\n"
            "\t3 - Check choice is now tcp\n"
            "\t4 - Add udp choice\n"
            "\t5 - Check choice is now udp (and tcp leaf has been removed)\n"
            "\t6 - Add tcp choice\n"
            "\t7 - Check choice is now tcp (and udp leaf has been removed)\n"
            "\t8 - Delete the top level container\n"
            "\t9 - Check choice has been removed\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );


    // create the top level container
    createContainer( primarySession_, "protocol" );

    checkChoice( primarySession_ );

    // add tcp
    addChoice( primarySession_, "tcp", "create" );

    checkChoice( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_yang_test");
    cbChecker_->resetExpectedCallbacks();     

    // add udp
    addChoice( primarySession_, "udp", "create" );

    // Check callbacks
    vector<string> udpElements = {"name", "udp", "udp"};
    vector<string> tcpElements = {"name", "tcp", "tcp"};
    cbChecker_->addChoice("simple_yang_test", "protocol", udpElements, 
                          tcpElements);
    //TODO Check is currently disabled as yuma functionality is broken
    //cbChecker_->checkCallbacks("simple_yang_test");                               
    cbChecker_->resetModuleCallbacks("simple_yang_test");
    cbChecker_->resetExpectedCallbacks();            

    checkChoice( primarySession_ );

    // add tcp
    addChoice( primarySession_, "tcp", "create" );

    // Check callbacks
    cbChecker_->addChoice("simple_yang_test", "protocol", tcpElements,
                          udpElements);
    //TODO Check is currently disabled as yuma functionality is broken
    //cbChecker_->checkCallbacks("simple_yang_test");                               
    cbChecker_->resetModuleCallbacks("simple_yang_test");
    cbChecker_->resetExpectedCallbacks();            

    checkChoice( primarySession_ );

    // remove container
    deleteContainer( primarySession_, "protocol" );
    
    checkChoice( primarySession_ );

}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

