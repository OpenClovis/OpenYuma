#define BOOST_TEST_MODULE SysTestDeviceEditCandidate

#include "configure-yuma-systest.h"

// ---------------------------------------------------------------------------|
// Yuma includes for files under test
// ---------------------------------------------------------------------------|

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
