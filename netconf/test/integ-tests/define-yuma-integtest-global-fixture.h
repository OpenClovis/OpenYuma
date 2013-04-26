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


// typedef that allows the use of parameterised test fixtures with 
// BOOST_GLOBAL_FIXTURE
typedef IntegrationTestFixture<SpoofedArgs> MyFixtureType_T; 

// Set the global test fixture
BOOST_GLOBAL_FIXTURE( MyFixtureType_T );




