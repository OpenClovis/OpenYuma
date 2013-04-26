// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( discard_changes_tests, SimpleContainerModuleFixture )

BOOST_AUTO_TEST_CASE( slt_discard_new_container )
{
    DisplayTestDescrption( 
            "Demonstrate discard-changes operation on uncomitted "
            "container creation.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list with 3 key-value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Discard the changes\n"
            "\t 6 - Check all values are not in the candidate\n"
            "\t 7 - Check all values are not in the running\n"
            "\t 8 - Commit the operation\n"
            "\t 9 - Check all values are not in the candidate\n"
            "\t10 - Check all values are not in the running\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 3 );
    // check the entries exist
    checkEntries( primarySession_ );

    // discard entries
    discardChangesOperation( primarySession_ );
    // check the entries no longer exist
    checkEntries( primarySession_ );

    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

BOOST_AUTO_TEST_CASE( slt_discard_list_additions )
{
    DisplayTestDescrption( 
            "Demonstrate discard-changes operation on uncomitted "
            "list addition.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list with 3 key-value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Populate the list with 2 additional key-value pairs\n"
            "\t 9 - Check all values are in the candidate\n"
            "\t10 - Check additional values are not in the running\n"
            "\t11 - Discard the changes\n"
            "\t12 - Check additional values are not in the candidate\n"
            "\t13 - Check additional values are not in the running\n"
            "\t14 - Delete all entries from the candidate\n"
            "\t15 - Check all values are not in the candidate\n"
            "\t16 - Check all values are in the running\n"
            "\t17 - Commit the operation\n"
            "\t18 - Check all values are not in the candidate\n"
            "\t19 - Check all values are not in the running\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 3 );
    // check the entries exist
    checkEntries( primarySession_ );

    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // set some more values
    addEntryValuePair( primarySession_, "entryKey4", "entryVal4" );
    addEntryValuePair( primarySession_, "entryKey5", "entryVal5" );
    // check the entries exist
    checkEntries( primarySession_ );

    // discard new entries
    discardChangesOperation( primarySession_ );
    // check the new entries were discarded
    checkEntries( primarySession_ );

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

BOOST_AUTO_TEST_CASE( slt_discard_list_edits )
{
    DisplayTestDescrption( 
            "Demonstrate discard-changes operation on uncomitted "
            "list edits.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the list with 3 key-value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Modify 2 of the key-value pairs\n"
            "\t 9 - Check modified values are in the candidate\n"
            "\t10 - Check modified values are not in the running\n"
            "\t11 - Discard the changes\n"
            "\t12 - Check edited values are not in the candidate\n"
            "\t13 - Check edited values are not in the running\n"
            "\t14 - Delete all entries from the candidate\n"
            "\t15 - Check all values are not in the candidate\n"
            "\t16 - Check all values are in the running\n"
            "\t17 - Commit the operation\n"
            "\t18 - Check all values are not in the candidate\n"
            "\t19 - Check all values are not in the running\n"
            );

    // RAII Vector of database locks 
    vector< unique_ptr< NCDbScopedLock > > locks = getFullLock( primarySession_ );

    createMainContainer( primarySession_ );

    // set some values
    populateDatabase( 3 );
    // check the entries exist
    checkEntries( primarySession_ );

    commitChanges( primarySession_ );
    checkEntries( primarySession_ );

    // modify some values
    editEntryValue( primarySession_, "entryKey1", "newVal1" );
    editEntryValue( primarySession_, "entryKey3", "newVal3" );

    // discard modifications
    discardChangesOperation( primarySession_ );
    // check the edits were discarded
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

