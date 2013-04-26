#define BOOST_TEST_MODULE SysTestSimpleEditRunning

#include "configure-yuma-systest.h"

namespace YumaTest {

// ---------------------------------------------------------------------------|
// Initialise the spoofed command line arguments 
// ---------------------------------------------------------------------------|
const char* SpoofedArgs::argv[] = {
    ( "yuma-test" ),
    ( "--target=running" ),
};

#include "define-yuma-systest-global-fixture.h"

} // namespace YumaTest
