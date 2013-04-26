// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

// ---------------------------------------------------------------------------|
// Boost Includes
// ---------------------------------------------------------------------------|
#include <boost/range/algorithm.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/fusion/at.hpp> 
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/operator.hpp> 

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/callbacks/sil-callback-controller.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

// ---------------------------------------------------------------------------|
// File wide namespace use and aliases
// ---------------------------------------------------------------------------|
namespace ph = boost::phoenix;
namespace ph_args = boost::phoenix::arg_names;
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( callback_failure_tests, SimpleContainerModuleFixture )

#if 0
//TODO Test needs updating when callback functionality is fixed 
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( slt_validate_failure, 49 )
BOOST_AUTO_TEST_CASE( slt_validate_failure )
{
    DisplayTestDescrption( 
            "Demonstrate correct behaviour following validate callback failure.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - TODO\n"
            );

    SILCallbackController& cbCon = SILCallbackController::getInstance();
    cbCon.reset();

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    cbCon.setValidateSuccess(false);

    // set some values
    populateDatabase( 3 );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    // commit the changes
    commitChanges( primarySession_ );

    // TODO Check callbacks
    //cbChecker_->checkCallbacks("simple_list_test");                               
    //cbChecker_->resetModuleCallbacks("simple_list_test");
    //cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    cbCon.reset();

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

#endif 

//TODO Test needs updating when callback functionality is fixed 
BOOST_AUTO_TEST_CASE( slt_apply_failure )
{
    DisplayTestDescrption( 
            "Demonstrate correct behaviour following apply callback failure.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - TODO\n"
            );

    SILCallbackController& cbCon = SILCallbackController::getInstance();
    cbCon.reset();

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     


    // set some values
    populateDatabase( 3 );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    cbCon.setApplySuccess(false);

    // commit the changes should fail
    commitChangesFailure( primarySession_ );

    // TODO Check callbacks
    //cbChecker_->checkCallbacks("simple_list_test");                               
    //cbChecker_->resetModuleCallbacks("simple_list_test");
    //cbChecker_->resetExpectedCallbacks();

    // TODO Candidate entries are removed when commit fails which is
    // TODO not the same behaviour as seen when a commit callback
    // TODO fails in the later tests.
    // TODO Behaviour should be made standard.
    // check the entries exist
    checkEntries( primarySession_ );

    cbCon.reset();

    // remove all entries
    //deleteMainContainer( primarySession_ );
    //checkEntries( primarySession_ );
    
    //commitChanges( primarySession_ );
    //checkEntries( primarySession_ );
    
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_CASE( slt_create_failure )
{
    DisplayTestDescrption( 
            "Demonstrate correct behaviour following create callback failure.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list\n"
            "\t 3 - Force the commit callback to fail\n"
            "\t 4 - Check that data is rolled back\n"
            );

    SILCallbackController& cbCon = SILCallbackController::getInstance();
    cbCon.reset();

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    // set some values
    populateDatabase( 3 );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    // cause first commit to fail
    cbCon.setCreateSuccess(false);

    // commit the changes should fail
    commitChangesFailure( primarySession_ );

    // TODO Check callbacks
    //cbChecker_->checkCallbacks("simple_list_test");                               
    //cbChecker_->resetModuleCallbacks("simple_list_test");
    //cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    cbCon.reset();

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

// ---------------------------------------------------------------------------|

BOOST_AUTO_TEST_CASE( slt_delete_failure )
{
    DisplayTestDescrption( 
            "Demonstrate correct behaviour following delete callback failure.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list\n"
            "\t 2 - Commit the list\n"
            "\t 3 - Delete the main container\n"
            "\t 4 - Force the commit callback to fail\n"
            "\t 5 - Check that data is rolled back\n"
            );

    SILCallbackController& cbCon = SILCallbackController::getInstance();
    cbCon.reset();

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    // set some values
    populateDatabase( 3 );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->addKeyValuePair("simple_list_test", "simple_list", elements, "theKey", "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    // commit the changes
    commitChanges( primarySession_ );

    // Check callbacks
    cbChecker_->commitKeyValuePairs("simple_list_test", "simple_list", elements, "theKey", "theVal", 3);
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // check the entries exist
    checkEntries( primarySession_ );

    cbCon.setDeleteSuccess(false);

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChangesFailure( primarySession_ );
    
    // TODO Check callbacks
    //cbChecker_->checkCallbacks("simple_list_test");                               
    //cbChecker_->resetModuleCallbacks("simple_list_test");
    //cbChecker_->resetExpectedCallbacks();
    
    checkEntries( primarySession_ );
    
    cbCon.reset();

    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

