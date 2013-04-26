// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/sil-callback-controller.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void SILCallbackController::setValidateSuccess(bool success)
{
    validateSuccess_ = success;
}
    
// ---------------------------------------------------------------------------|
bool SILCallbackController::validateSuccess()
{
    return validateSuccess_;
}

// ---------------------------------------------------------------------------|
void SILCallbackController::setApplySuccess(bool success)
{
    applySuccess_ = success;
}
    
// ---------------------------------------------------------------------------|
bool SILCallbackController::applySuccess()
{
    return applySuccess_;
}

// ---------------------------------------------------------------------------|
void SILCallbackController::setCreateSuccess(bool success)
{
    createSuccess_ = success;
}
    
// ---------------------------------------------------------------------------|
bool SILCallbackController::createSuccess()
{
    return createSuccess_;
}

// ---------------------------------------------------------------------------|
void SILCallbackController::setDeleteSuccess(bool success)
{
    deleteSuccess_ = success;
}
    
// ---------------------------------------------------------------------------|
bool SILCallbackController::deleteSuccess()
{
    return deleteSuccess_;
}

// ---------------------------------------------------------------------------|
void SILCallbackController::reset()
{
    validateSuccess_ = true;
    applySuccess_ = true;
    createSuccess_ = true;
    deleteSuccess_ = true;
}

} // namespace YumaTest
