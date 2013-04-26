/**
 * Define the Yuma System test Global Test Fixture.
 * This file simply defines the global test fixture for Yuma System
 * Testing.
 *
 * NOTE: Yuma System Testing. It purposely omits header include guards 
 *
 * Yuma System Test Files Should have the following format:
 *
 * #define BOOST_TEST_MODULE [Name of test module]
 * #include "configure-yuma-systest.h"
 *
 * namespace YumaTest {
 *
 * Custom Initialisation (e.g. definition of SpoofedArgs )
 *
 * #include "define-yuma-systest-global-fixture.h"
 *
 * } // namespace YumaTest.
 */


// typedef that allows the use of parameterised test fixtures with 
// BOOST_GLOBAL_FIXTURE
typedef SystemTestFixture<SpoofedArgs> MyFixtureType_T; 

// Set the global test fixture
BOOST_GLOBAL_FIXTURE( MyFixtureType_T );




