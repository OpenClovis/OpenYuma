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

BOOST_FIXTURE_TEST_SUITE( simple_get_test_suite_running, SimpleContainerModuleFixture )

// ---------------------------------------------------------------------------|
// Simple get of database data
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( get_slt_entries_using_xpath_filter )
{
    DisplayTestDescrption( 
            "Demonstrate Simple retrieval of a modules entries.",
            "Procedure: \n"
            "\t1 - Query the entries for the 'simple_list_test' module "
            "using an xpath filter" );
   
    vector<string> expPresent{ "data", "rpc-reply" };
    vector<string> expNotPresent{ "error", "rpc-error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryGetConfigXpath( primarySession_, "simple_list_test", 
            "running", checker );
}

// ---------------------------------------------------------------------------|
// Demonstrate poorly formed xml messages are ignored
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( edit_test5_entries_badxml )
{
    DisplayTestDescrption( 
            "Demonstrate that poorly formed XML create requests are rejected.",
            "Procedure: \n"
            "\t1 - Create a query containing poorly formed xml (missing "
            "termination /\n"
            "\t2 - Inject the message and check that it was ignored.\n\n"
            "Note: originally this caused Yuma to exit with SIG SEGV" );

    // build the query
    stringstream query;

    query << "<theList xmlns:nc=\"urn:ietf:params:xml:ns:netconf:base:1.0\"\n"
            "    nc:operation=\"create\"\n"
            "    xmlns=\"http://netconfcentral.org/ns/simple_list_test\">\n";
   
    vector<string> expPresent{};
    vector<string> expNotPresent{ "error" "ok", "rpc-reply", "rpc-error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryEditConfig( primarySession_, query.str(), "running",
                                 checker );
}

// ---------------------------------------------------------------------------|
// Add some data and check that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( edit_slt_entries_test_create )
{
    DisplayTestDescrption( 
            "Demonstrate modification of a simple container.",
            "Procedure: \n"
            "\t1 - Create the top level container\n"
            "\t2 - Create a keyed entry with key = entry1Key\n"
            "\t3 - Set the value coressponding to entry1Key\n"
            "\t4 - Create a keyed entry with key = entry2Key\n"
            "\t5 - Set the value coressponding to entry2Key\n"
            "\t6 - Get the container and verify that all entries were "
            "created in the writeable database\n"
            "\t7 - Delete all entries\n"
            "\t8 - Verfiy that all entries were removed.\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();     

    // create the top level container
    createMainContainer( primarySession_ );

    // Check callbacks
    vector<string> elements = {};
    cbChecker_->updateContainer("simple_list_test",
                                "simple_list",
                                elements,
                                "create");
    cbChecker_->checkCallbacks("simple_list_test");                             
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // Add an entry
    addEntryValuePair( primarySession_, "entryKey1", "entryVal1" );
    
    // Check callbacks
    elements.push_back("theList");
    cbChecker_->addKeyValuePair("simple_list_test",
                                "simple_list",
                                elements,
                                "theKey",
                                "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");

    // Add an entry
    addEntryValuePair( primarySession_, "entryKey2", "entryVal2" );
    
    // Check callbacks
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
 
    // Add an entry
    addEntryValuePair( primarySession_, "entryKey3", "entryVal3" );

    // Check callbacks
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();            

    checkEntries( primarySession_ );

    // remove all entries
    deleteMainContainer( primarySession_ );
    
    checkEntries( primarySession_ );

}

// ---------------------------------------------------------------------------|
// Delete existing list entries and container; attempt to delete non-existant
// data.
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( edit_slt_entries_test_delete )
{
    DisplayTestDescrption( 
            "Demonstrate the deletion of existing nodes and appropriate "
            "errors when attempting to delete non-existant nodes.",
            "Procedure: \n"
            "\t1  - Create the top level container\n"
            "\t2  - Create a keyed entry with key = entry1Key\n"
            "\t3  - Set the value corresponding to entry1Key\n"
            "\t4  - Create a keyed entry with key = entry2Key\n"
            "\t5  - Set the value corresponding to entry2Key\n"
            "\t6  - Delete the first key/value pair\n"
            "\t7  - Verify entry deleted\n"
            "\t8  - Attempt to delete non-existent key/value pair\n"
            "\t9  - Verify error message\n"
            "\t10 - Delete the second key/value pair\n"
            "\t11 - Delete the list\n"
            "\t12 - Verify list deleted\n"
            "\t13 - Delete the container\n"
            "\t14 - Verify container deleted\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // create the top level container
    createMainContainer( primarySession_ );

    // Add two entries
    addEntryValuePair( primarySession_, "entryKey1", "entryVal1" );
    addEntryValuePair( primarySession_, "entryKey2", "entryVal2" );
    
    //Reset logged callbacks
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    // Delete first entry
    deleteEntryValuePair( primarySession_, "entryKey1", "entryVal1" );

    // Check callbacks
    vector<string> elements = {"theList"};
    cbChecker_->deleteKeyValuePair("simple_list_test",
                                   "simple_list",
                                   elements,
                                   "theKey",
                                   "theVal");
    cbChecker_->checkCallbacks("simple_list_test");                               
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();            

    // Attempt to delete non-existent list entry
    deleteEntryValuePairFailed( primarySession_, "entryKey3", "entryVal3" );

    // Delete second entry
    deleteEntryValuePair( primarySession_, "entryKey2", "entryVal2" );

    checkEntries( primarySession_ );

    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();            

    // Delete the container
    deleteMainContainer( primarySession_ );
    
    // Check callbacks
    elements.clear();
    cbChecker_->updateContainer("simple_list_test",
                                "simple_list",
                                elements,
                                "delete");
    cbChecker_->checkCallbacks("simple_list_test");                             
    cbChecker_->resetModuleCallbacks("simple_list_test");
    cbChecker_->resetExpectedCallbacks();

    checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

