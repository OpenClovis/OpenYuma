#ifndef __SIL_CALLBACK_LOG_H
#define __SIL_CALLBACK_LOG_H

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
 * Support class for logging callback information for multiple modules.
 * Implements the Singleton pattern.
 */
class SILCallbackLog
{
public:
    /** Simple structure to hold callback information */
    struct CallbackInfo
    {
        /** Constructor */
        CallbackInfo(std::string name="", std::string type="", std::string phase="") :
                                cbName(name),
                                cbType(type),
                                cbPhase(phase)
        {
        }

        /** Comparison of CallbackInfo */
        bool operator==(const CallbackInfo& cbInfo) const;

        std::string cbName;
        std::string cbType;
        std::string cbPhase;
    };
    
    /** Convenience typedef */
    typedef std::vector<CallbackInfo> ModuleCallbackData;

    /** 
     * Returns the single instance of SILCallbackLog 
     *
     * \return the SILCallbackLog instance.
     */
    static SILCallbackLog& getInstance()
    {
        static SILCallbackLog instance;
        return instance;
    }

    /**
     * Add a callback to the log.
     *
     * \param modName the name of the module from which the callback originated.
     * \param cbInfo the callback information.
     */
    void addCallback(const std::string& modName, CallbackInfo cbInfo);

    /**
     * Return a vector containing all callbacks logged for a given module.
     *
     * \param modName the name of the module to return callbacks for.
     * \return a vector of callback information.
     */
    ModuleCallbackData getModuleCallbacks(const std::string& modName);

    /**
     * Clear the callbacks for a given module.
     *
     * \param modName the name of the module to clear callbacks for.
     */
    void resetModuleCallbacks(const std::string& modName);

private:
    // private as implementing Singleton pattern
    SILCallbackLog() {}
    ~SILCallbackLog() {}

    // not generated as implementing Singleton pattern
    SILCallbackLog(const SILCallbackLog &) = delete;
    SILCallbackLog& operator=(SILCallbackLog) = delete;

    typedef std::map<std::string, ModuleCallbackData> CallbackMap;
    
    CallbackMap callbackMap_;
};

} // namespace YumaTest

#endif // __SIL_CALLBACK_LOG_H

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
