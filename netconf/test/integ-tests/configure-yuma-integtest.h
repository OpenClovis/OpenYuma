/**
 * Configure the BOOST::TEST environment for Yuma integration Testing.
 * This file is contains common configuration of BOOST::TEST for 
 * Yuma Integration Testing.
 *
 * NOTE: It purposely omits header include guards 
 *
 * Yuma Integration Test Main Files Should have the following format:
 *
 * #define BOOST_TEST_MODULE [Name of test module]
 * #include "configure-yuma-integtest.h"
 *
 * namespace YumaTest {
 *
 * Custom Initialisation (e.g. definition of SpoofedArgs )
 *
 * #include "define-yuma-integtest-global-fixture.h"
 *
 * } // namespace YumaTest.
 */

/** Configure Dynamic linking with boost test libraries */
#define BOOST_TEST_DYN_LINK

// ---------------------------------------------------------------------------|
// Boost Test Framework
// ---------------------------------------------------------------------------|
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

// ---------------------------------------------------------------------------|
// Yuma Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/integ-test-fixture.inl"
#include "test/support/fixtures/spoofed-args.h"


