// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#include <stdint.h>

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|
#include "ncx.h"

// ---------------------------------------------------------------------------|
// Test cases for ncx_init() 
// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_1) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ),
        const_cast<char*>( "--modpath=../../../modules/netconfcentral:"
                           "../../../modules/ietf:../../../modules/yang" ) 
    };

    BOOST_TEST_MESSAGE( "ncx_init() - Test with minimal Command line "
                        "parameters!");
    BOOST_CHECK_EQUAL( NO_ERR, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 2, argv ) );
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_2) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ) 
    };

    BOOST_TEST_MESSAGE( "ncx_init() - Test failure to find modules" );
    BOOST_CHECK_EQUAL( ERR_NCX_MOD_NOT_FOUND, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 1, argv ) );
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_3) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ),
        const_cast<char*>( "--modpath=../../../modules/netconfcentral:"
                           "../../../modules/ietf:../../../modules/yang" ) ,
        const_cast<char*>( "--unknown-param=somethings_wrong" )

    };

    BOOST_TEST_MESSAGE( "ncx_init() - Check unexpected command line parameters "
                        "are ignored" );
    BOOST_CHECK_EQUAL( NO_ERR, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 3, argv ) );
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_4) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ),
        const_cast<char*>( "--yuma-home=~/MyYumaHomeDir" ),
        const_cast<char*>( "--modpath=../../../modules/netconfcentral" ),
    };

    BOOST_TEST_MESSAGE( "ncx_init() - Check setting of $YUMA_HOME via command line" );
    BOOST_CHECK_EQUAL( NO_ERR, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 3, argv ) );
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_5) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ),
        const_cast<char*>( "--modpath=../../../modules/netconfcentral" ),
        const_cast<char*>( "--modpath=../../../modules/ietf" ),
    };

    BOOST_TEST_MESSAGE( "ncx_init() - Check multiple --modpath entries are disallowed" );
    BOOST_CHECK_EQUAL( ERR_NCX_DUP_ENTRY, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 3, argv ) );
    ncx_cleanup();
}

// ---------------------------------------------------------------------------|
BOOST_AUTO_TEST_CASE(ncx_init_test_6) 
{
    char* argv[] = { 
        const_cast<char*>( "ncx_init_test" ),
        const_cast<char*>( "--modpath=../../../modules/netconfcentral" ),
        const_cast<char*>( "--yuma-home=~/MyYumaHomeDir" ),
        const_cast<char*>( "--yuma-home=~/AnotherMyYumaHomeDir" ),
    };

    BOOST_TEST_MESSAGE( "ncx_init() - Check multiple --yuma-home entries are disallowed" );
    BOOST_CHECK_EQUAL( ERR_NCX_DUP_ENTRY, 
                       ncx_init( FALSE, LOG_DEBUG_WARN, FALSE, 
                                 0, 4, argv ) );
    ncx_cleanup();
}
