#ifndef __SIL_CALLBACK_CONTROLLER_H
#define __SIL_CALLBACK_CONTROLLER_H

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <map>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Support class for controlling whether callbacks return success or failure.
 * Implements the Singleton pattern.
 */
class SILCallbackController
{
public:
    
    /** 
     * Returns the single instance of SILCallbackController 
     *
     * \return the SILCallbackController instance.
     */
    static SILCallbackController& getInstance()
    {
        static SILCallbackController instance;
        return instance;
    }

    /**
     * Set success/failure of validate callbacks.
     *
     * \param success true if validate callbacks should succeed and false 
     *                otherwise
     */
    void setValidateSuccess(bool success);
    
    /**
     * Should validate callbacks succeed?
     *
     * \return true if validate callbacks should succeed and false otherwise.
     */
    bool validateSuccess();

    /**
     * Set success/failure of apply callbacks.
     *
     * \param success true if apply callbacks should succeed and false 
     *                otherwise
     */
    void setApplySuccess(bool success);
    
    /**
     * Should apply callbacks succeed?
     *
     * \return true if apply callbacks should succeed and false otherwise.
     */
    bool applySuccess();

    /**
     * Set success/failure of create callback.
     *
     * \param success true if create callbacks should succeed and false 
     *                otherwise
     */
    void setCreateSuccess(bool success);
    
    /**
     * Should create callbacks succeed?
     *
     * \return true if create callbacks should succeed and false otherwise.
     */
    bool createSuccess();

    /**
     * Set success/failure of delete callbacks.
     *
     * \param success true if delete callbacks should succeed and false 
     *                otherwise
     */
    void setDeleteSuccess(bool success);
    
    /**
     * Should delete callbacks succeed?
     *
     * \return true if delete callbacks should succeed and false otherwise.
     */
    bool deleteSuccess();

    /**
     * Set all callbacks to succeed.
     */
    void reset();

private:
    // private as implementing Singleton pattern
    SILCallbackController() 
        : validateSuccess_(true),
          applySuccess_(true),
          createSuccess_(true),
          deleteSuccess_(true)
    {
    }
    
    ~SILCallbackController() 
    {
    }

    // not generated as implementing Singleton pattern
    SILCallbackController(const SILCallbackController &) = delete;
    SILCallbackController& operator=(SILCallbackController) = delete;

    bool validateSuccess_, applySuccess_;
    bool createSuccess_, deleteSuccess_;
};

} // namespace YumaTest

#endif // __SIL_CALLBACK_CONTROLLER_H

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
