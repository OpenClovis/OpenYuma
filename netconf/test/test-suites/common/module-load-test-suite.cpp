// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/query-suite-fixture.h"
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

namespace YumaTest {

BOOST_FIXTURE_TEST_SUITE( mod_load_suite, QuerySuiteFixture )

// ---------------------------------------------------------------------------|
// Test cases for loading yang modules
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(load_yang_module_from_modpath)
{
    DisplayTestDescrption( 
        "Demonstrate loading of a 'yang' module from the configured --modpath.",
        "The test1 module should be found in one of the directories "
        "specified using the --modpath command line parameter.\n\n"
        "Note 1: There is no associated SIL for this module so it will be "
        "loaded as a '.yang' file\n" ); 
    
    vector<string> expPresent{ "mod-revision" };
    vector<string> expNotPresent{ "rpc-error", "error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "test1", checker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(load_bad_yang_module_from_modpath)
{
    DisplayTestDescrption( 
        "Demonstrate loading of a corrupted 'yang' module (missing namespace)"
        "from the configured --modpath.",
        "The test1badns module should be found in one of the directories "
        "specified using the --modpath command line parameter.\n\n"
        "Note 1: There is no associated SIL for this module so it will be "
        "loaded as a '.yang' file\n" ); 
    
    vector<string> expPresent{ "rpc-error", "error" };
    vector<string> expNotPresent{ "mod-revision" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "test1badns", checker );
}

// ---------------------------------------------------------------------------|
// Test cases for loading yang modules
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(load_sil_module_from_modpath)
{
    DisplayTestDescrption( 
        "Demonstrate loading of a 'sil' module.",
        "The libsimple_list_test.so shared object containing 'sil' callbacks "
        "for the simple_list_test yang module should be found in one of the "
        "directories specified using the --modpath command line parameter." );
    
    vector<string> expPresent{ "mod-revision" };
    vector<string> expNotPresent{ "rpc-error", "error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "simple_list_test", 
                                 checker );
}

#if 0
// THIS TEST IS DISABLED DUE TO YUMA BUGS!
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( load_yang_module_from_local_dir, 3 )
BOOST_AUTO_TEST_CASE(load_yang_module_from_local_dir)
{
    DisplayTestDescrption( 
        "Demonstrate loading of a 'yang' module from a local directory.",
        "The extension '.yang' is specified in the load command, ergo this "
        "module will be loaded from the local directory" );

    vector<string> expPresent{ "mod-revision" };
    vector<string> expNotPresent{ "rpc-error", "error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "test2.yang",
                                 checker );
}
#endif

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(load_missing_yang_module)
{
    DisplayTestDescrption( 
        "Demonstrate that an error is reported if a module is not found." );

    // build a load module message for test.yang
    vector<string> expNotPresent{ "mod-revision" };
    vector<string> expPresent{ "rpc-error", "error-message", "module not found" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "non-existent", checker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(load_module_twice)
{
    DisplayTestDescrption( "Attempt to load a module twice.",
        "Demonstrate that an attempt is made to load a module that has "
        "already been loaded has no affect" );

    // build a load module message for test.yang
    vector<string> expPresent{ "mod-revision" };
    vector<string> expNotPresent{ "rpc-error", "error" };

    StringsPresentNotPresentChecker checker( expPresent, expNotPresent );
    queryEngine_->tryLoadModule( primarySession_, "test3", checker ); 
    queryEngine_->tryLoadModule( primarySession_, "test3", checker );
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_SUITE_END()

} // namespace YumaTest

