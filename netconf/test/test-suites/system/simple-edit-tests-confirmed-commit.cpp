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
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/simple-container-module-fixture.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"

// ---------------------------------------------------------------------------|
namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( confirmed_commit_slt_tests, SimpleContainerModuleFixture )

BOOST_AUTO_TEST_CASE( slt_confirmed_commit_success )
{
    DisplayTestDescrption( 
            "Demonstrate population of simple container and successful "
            "confirmed-commit operation.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the database with 3 key/value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Confirmed-Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Commit the operation before the timeout\n"
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

    // set some values
    populateDatabase( 3 );

    // check the entries exist
    checkEntries( primarySession_ );

    // confirmed-commit
    confirmedCommitChanges (primarySession_, 1);

    // check the entries exist
    checkEntries( primarySession_ );

    // commit the changes
    commitChanges( primarySession_ );

    // sleep for 2 seconds
    sleep( 2 );

    // check the entries still exist
    checkEntries( primarySession_ );

    // remove all entries
    deleteMainContainer( primarySession_ );
    checkEntries( primarySession_ );
    
    commitChanges( primarySession_ );
    checkEntries( primarySession_ );
    
}

BOOST_AUTO_TEST_CASE( slt_confirmed_commit_timeout )
{
    DisplayTestDescrption( 
            "Demonstrate rollback of simple container following "
            "confirmed-commit operation timeout.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the database with 3 key/value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Confirmed-Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Allow the timeout to occur\n"
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

    // confirmed-commit
    confirmedCommitChanges (primarySession_, 1);

    // check the entries exist
    checkEntries( primarySession_ );

    // sleep for 2 seconds to cause rollback
    sleep( 2 );
    rollbackChanges( primarySession_);
    
    // check the entries no longer exist
    checkEntries( primarySession_ );
}

BOOST_AUTO_TEST_CASE( slt_confirmed_commit_extend_timeout )
{
    DisplayTestDescrption( 
            "Demonstrate rollback of simple container following "
            "extended confirmed-commit operation timeout.",
            "Procedure: \n"
            "\t 1 - Create the top level container for the module\n"
            "\t 2 - Populate the database with 3 key/value pairs\n"
            "\t 3 - Check all values are in the candidate\n"
            "\t 4 - Check all values are not in the running\n"
            "\t 5 - Confirmed-Commit the operation\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Confirmed-Commit to extend the timeout\n"
            "\t 8 - Allow initial timeout period to pass\n"
            "\t 6 - Check all values are in the running\n"
            "\t 7 - Check all values are in the candidate\n"
            "\t 8 - Allow the timeout to occur\n"
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

    // confirmed-commit
    confirmedCommitChanges (primarySession_, 2);

    // check the entries exist
    checkEntries( primarySession_ );

    sleep( 1 );

    // confirmed-commit to extend timeout
    confirmedCommitChanges (primarySession_, 4, true);

    // go beyond initial timeout
    sleep( 3 );

    // check the entries still exist
    checkEntries( primarySession_ );
    
    // sleep for 2 seconds to cause rollback
    sleep( 2 );
    rollbackChanges( primarySession_);
    
    // check the entries no longer exist
    checkEntries( primarySession_ );

    // FIXME: if all entries are not removed the next tests will fail! 
    // FIXME: this indicates that the rollback has  not worked
    // FIXME: correctly
    // deleteMainContainer( primarySession_ );
    // checkEntries( primarySession_ );
    // commitChanges( primarySession_ );
    // checkEntries( primarySession_ );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

