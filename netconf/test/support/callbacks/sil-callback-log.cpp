// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/sil-callback-log.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
bool SILCallbackLog::CallbackInfo::operator==(const SILCallbackLog::CallbackInfo& cbInfo) const
{
    return ((this->cbName == cbInfo.cbName) && (this->cbType == cbInfo.cbType) &&
            (this->cbPhase == cbInfo.cbPhase));
}

// ---------------------------------------------------------------------------|
void SILCallbackLog::addCallback(const string& modName, CallbackInfo cbInfo)
{
    callbackMap_[modName].push_back(cbInfo);
}
    
// ---------------------------------------------------------------------------|
SILCallbackLog::ModuleCallbackData SILCallbackLog::getModuleCallbacks(const std::string& modName)
{
    return callbackMap_[modName];
}

// ---------------------------------------------------------------------------|
void SILCallbackLog::resetModuleCallbacks(const std::string& modName)
{
    callbackMap_[modName].clear();
}

} // namespace YumaTest
