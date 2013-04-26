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

BOOST_FIXTURE_TEST_SUITE( candidate_slt_tests, SimpleContainerModuleFixture )

// ---------------------------------------------------------------------------|
// Add some data and check that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_value_create )
{
    DisplayTestDescrption( 
            "Demonstrate population of simple container and candidate "
            "commit operation.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Create a keyed entry with key = entry1Key\n"
            "\t 3 - Set the value corresponding to entry1Key\n"
            "\t 4 - Create a keyed entry with key = entry2Key\n"
            "\t 5 - Set the value corresponding to entry2Key\n"
            "\t 6 - Check all values are in the candidate\n"
            "\t 7 - Check all values are not in the running\n"
            "\t 8 - Commit the operation\n"
            "\t 9 - Check all values are in the running\n"
            "\t10 - Check all values are in the candidate\n"
            "\t11 - Delete all entries from the candidate\n"
            "\t12 - Check all values are not in the candidate\n"
            "\t13 - Check all values are in the running\n"
            "\t14 - Commit the operation\n"
            "\t15 - Check all values are not in the candidate\n"
            "\t16 - Check all values are not in the running\n"
            );

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

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

// ---------------------------------------------------------------------------|
// Add some data then modify the data that it was added correctly
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_edit_values )
{
    DisplayTestDescrption( 
            "Demonstrate population and modification of database values.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Create some keyed entry values \n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Commit the operation\n"
            "\t 5 - Check all values are in the running\n"
            "\t 6 - Modify some of the values\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Commit the operation\n"
            "\t 9 - Check all values are in the running\n"
            "\t10 - Modify some of the values\n"
            "\t11 - Check all values are in the candidate\n"
            "\t12 - Commit the operation\n"
            "\t13 - Check all values are in the running\n"
            "\t14 - Clean out the database and check it was cleaned"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // create the top level container

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 20 );

    // commit & check entries exist
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // modify some values
    editEntryValue( primarySession_, "entryKey2", "newValue2" );
    editEntryValue( primarySession_, "entryKey5", "newValue5" );
    editEntryValue( primarySession_, "entryKey6", "newValue6" );

    // commit & check entries exist
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    editEntryValue( primarySession_, "entryKey0", "newValue0" );
    editEntryValue( primarySession_, "entryKey1", "newValue1" );
    // 
    // // commit & check entries exist
    checkEntries( primarySession_ );
    // commitChanges( primarySession_ );
    // checkEntries( primarySession_ );

    // // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_lock_released_no_comit )
{
    DisplayTestDescrption( 
            "Demonstrate that any changes made to the database are reverted "
            "if the lock is released without a commit.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Create some keyed entry values \n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Commit the operation\n"
            "\t 5 - Check all values are in the running\n"
            "\t 6 - Modify some of the values\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Release all locks\n"
            "\t 9 - Obtain new locks\n"
            "\t10 - Check that  all changes from step 6 were discarded\n"
            "\t11 - Clean out the database and check it was cleaned\n"
            );

    std::shared_ptr<AbstractNCSession> secondarySession( 
            sessionFactory_->createSession() );

    // RAII Vector of database locks 
    { 
        vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

        // create the top level container
        createMainContainer( primarySession_ );
        // set some values
        populateDatabase( 20 );

        // commit & check entries exist
        checkEntries( primarySession_ );
        commitChanges( primarySession_ );
        checkEntries( primarySession_ );
    }

    // session 1 modifications
    map<string, string> ses1Mods = { { "entryKey2", "newValue2" },
                                     { "entryKey5", "newValue5" },
                                     { "entryKey6", "newValue6" } };

    {
        vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

        // modify some values
        boost::for_each( ses1Mods,
                         ph::bind( &SimpleContainerModuleFixture::editEntryValue,
                                   this,
                                   primarySession_,
                                   ph::at_c<0>( ph_args::arg1 ),
                                   ph::at_c<1>( ph_args::arg1 ) ) );
        checkEntries( primarySession_ );
    }

    discardChanges(); // let tst harness know that changes should be discarded
    {
        vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

        checkEntries( primarySession_ );
        deleteMainContainer( primarySession_ );
        checkEntries( primarySession_ );
        commitChanges( primarySession_ );
        checkEntries( primarySession_ );
    }
}

// ---------------------------------------------------------------------------|
// Add some data then merge some data with a mix of modified values for 
// existing keys and new key/value pairs; verify that the new pairs are added
// and the existing values remain unchanged
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_merge_values )
{
    DisplayTestDescrption( 
            "Demonstrate population of database and merging of new and "
            "updated data.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Create some keyed entry values \n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Commit the operation\n"
            "\t 5 - Check all values are in the running\n"
            "\t 6 - Merge new pairs and modified values\n"
            "\t 7 - Check only new pairs added to candidate\n"
            "\t 8 - Commit the operation\n"
            "\t 9 - Check only new pairs added to running\n"
            "\t10 - Clean out the database and check it was cleaned"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // create the top level container

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 20 );

    // commit & check entries exist
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // merge some values
    BOOST_TEST_MESSAGE("MERGING VALUES");
    mergeEntryValuePair( primarySession_, "entryKey18", "newValue22" );
    mergeEntryValuePair( primarySession_, "entryKey19", "newValue23" );
    mergeEntryValuePair( primarySession_, "entryKey20", "newValue20" );
    mergeEntryValuePair( primarySession_, "entryKey21", "newValue21" );

    // commit & check entries exist
    BOOST_TEST_MESSAGE("CHECKING ENTRIES - PRE-COMMIT");
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("CHECKING ENTRIES - POST-COMMIT");
    checkEntries( primarySession_ );

    // remove all entries
    BOOST_TEST_MESSAGE("DELETING MAIN CONTAINER");
    deleteMainContainer( primarySession_ );
    BOOST_TEST_MESSAGE("CHECKING ENTRIES - PRE-COMMIT");
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    BOOST_TEST_MESSAGE("CHECKING ENTRIES - POST-COMMIT");
    checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
// Add some data then replace some data with a mix of modified values for 
// existing keys and new key/value pairs; verify that the new pairs are added
// and the existing values remain unchanged
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE( slt_replace_values )
{
    DisplayTestDescrption( 
            "Demonstrate population of database and replacing of new and "
            "updated data.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Create some keyed entry values \n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Commit the operation\n"
            "\t 5 - Check all values are in the running\n"
            "\t 6 - Replace new pairs and modified values\n"
            "\t 7 - Check new pairs added to candidate\n"
            "\t 8 - Check existing pairs updated with modified values in "
            "candidate\n"
            "\t 9 - Commit the operation\n"
            "\t10 - Check new pairs added to running\n"
            "\t11 - Check existing pairs updated with modified values in "
            "running\n"
            "\t12 - Clean out the database and check it was cleaned"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    // create the top level container

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 20 );

    // commit & check entries exist
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // replace some values
    replaceEntryValuePair( primarySession_, "entryKey18", "newValue22" );
    replaceEntryValuePair( primarySession_, "entryKey19", "newValue23" );
    replaceEntryValuePair( primarySession_, "entryKey20", "newValue20" );
    replaceEntryValuePair( primarySession_, "entryKey21", "newValue21" );

    // commit & check entries exist
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
}
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

