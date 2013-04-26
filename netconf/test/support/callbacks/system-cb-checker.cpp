// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/system-cb-checker.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void SystemCBChecker::addMainContainer(const std::string& modName, 
                                       const std::string& containerName)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addElement(const std::string& /* modName */, 
                                 const std::string& /* containerName */,
                                 const std::vector<std::string>& /* element */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addKey(const std::string& /* modName */, 
                             const std::string& /* containerName */,
                             const std::vector<std::string>& /* listElement */,
                             const std::string& /* key */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addChoice(const std::string& /* modName */, 
                                const std::string& /* containerName */,
                                const std::vector<std::string>& /* choiceElement */,
                                const std::vector<std::string>& /* removeElement */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addKeyValuePair(const std::string& /* modName */, 
                                      const std::string& /* containerName */,
                                      const std::vector<std::string>& /* listElement */,
                                      const std::string& /* key */,
                                      const std::string& /* value */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::deleteKey( const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& listElement,
                                 const std::string& key )
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::deleteKeyValuePair( const std::string& modName, 
                                          const std::string& containerName,
                                          const std::vector<std::string>& listElement,
                                          const std::string& key,
                                          const std::string& value )
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::commitKeyValuePairs(const std::string& modName, 
                                          const std::string& containerName,
                                          const std::vector<std::string>& listElement,
                                          const std::string& key,
                                          const std::string& value,
                                          int count)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addResourceNode(const std::string& /* modName */, 
                                      const std::string& /* containerName */,
                                      const std::vector<std::string>& /* elements */,
                                      bool /* statusConfig */,
                                      bool /* alarmConfig */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addResourceCon(const std::string& /* modName */, 
                                     const std::string& /* containerName */,
                                     const std::vector<std::string>& /* elements */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::addStreamCon(const std::string& /* modName */, 
                                   const std::string& /* containerName */,
                                   const std::vector<std::string>& /* elements */)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::checkCallbacks(const std::string& modName)
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::updateLeaf(const std::string& modName, 
                            const std::string& containerName,
                            const std::vector<std::string>& listElement,
                            const std::string& phase )
{
    // Do nothing as callbacks are not logged during system tests.
}

// ---------------------------------------------------------------------------|
void SystemCBChecker::updateContainer(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& listElement,
                                 const std::string& phase )
{
    // Do nothing as callbacks are not logged during system tests.
}

} // namespace YumaTest
