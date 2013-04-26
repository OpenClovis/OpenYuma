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
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-query-util/nc-default-operation-config.h"
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

BOOST_FIXTURE_TEST_SUITE( default_op_none, SimpleContainerModuleFixture )

BOOST_AUTO_TEST_CASE(default_none_test_create )
{
    DisplayTestDescrption( 
            "Demonstrate no modification of a simple container with default operation none.",
            "Procedure: \n"
            "\t1 - Verify container not created with no nc operation\n"
            "\t2 - Create the top level container\n"
            "\t3 - Verify container not modified with no nc operation\n"
            "\t4 - Delete container\n"
            "\t5 - Verfiy that all entries were removed.\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    // Set default operation to none
    messageBuilder_->setDefaultOperation("none");
    
    // no operation on the top level container
    mainContainerOp( primarySession_, "" );

    checkEntries( primarySession_ );
    
    // create the top level container
    createMainContainer( primarySession_ );

    checkEntries( primarySession_ );

    // No operation on an entry
    noOpEntryValuePair( primarySession_, "entryKey1", "entryVal1" );
    
    checkEntries( primarySession_ );
    
    // remove all entries
    deleteMainContainer( primarySession_ );
    
    checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

