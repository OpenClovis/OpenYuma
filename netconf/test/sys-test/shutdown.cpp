#define BOOST_TEST_MODULE SysTestShutdown

#include "configure-yuma-systest.h"

namespace YumaTest {

// ---------------------------------------------------------------------------|
// Initialise the spoofed command line arguments 
// ---------------------------------------------------------------------------|
const char* SpoofedArgs::argv[] = {
    ( "yuma-test" ),
    ( "--target=candidate" ),
};

#include "define-yuma-systest-global-fixture.h"

} // namespace YumaTest
