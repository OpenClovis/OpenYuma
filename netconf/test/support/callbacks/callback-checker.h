#ifndef __CALLBACK_CHECKER_H
#define __CALLBACK_CHECKER_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/sil-callback-log.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Base class for checking callback information.
 */
class CallbackChecker
{
public:
    /** 
     * Constructor. 
     */
    CallbackChecker()
    {
    }
    
    /**
     * Destructor.
     */
    virtual ~CallbackChecker()
    {
    }

    /**
     * Add an expected callback.
     *
     * \param modName the name of the module from which the callback is expected.
     * \param containerName the name of the top level container.
     * \param elementHierarchy a vector representing the hierarchy of elements 
     * leading to the one being operated on.
     * \param operation the operation being performed (e.g. get, edit, mro).
     * \param type the callback type.
     * \param phase the callback phase.
     */
    void addExpectedCallback(const std::string& modName, 
                             const std::string& containerName,
                             const std::vector<std::string>& elementHierarchy,
                             const std::string& operation,
                             const std::string& type,
                             const std::string& phase);

    /**
     * Add expected callbacks for adding main container.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     */
    virtual void addMainContainer(const std::string& modName, 
                                  const std::string& containerName) = 0;

    /**
     * Add expected callbacks for adding an element to a container.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param element a vector representing the hierarchy of elements 
     * leading to the element to be added.
     */
    virtual void addElement(const std::string& modName, 
                            const std::string& containerName,
                            const std::vector<std::string>& element) = 0;

    /**
     * Add expected callbacks for adding a key to a list.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the list that the pair will be added to.
     * \param key the key to be added.
     */
    virtual void addKey(const std::string& modName, 
                        const std::string& containerName,
                        const std::vector<std::string>& listElement,
                        const std::string& key) = 0;

    /**
     * Add expected callbacks for adding a choice.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param choiceElement a vector representing the hierarchy of elements 
     * leading to the choice.
     * \param removeElement a vector representing the hierarchy of elements 
     * leading to the previous choice which should be removed.
     */
    virtual void addChoice(const std::string& modName, 
                           const std::string& containerName,
                           const std::vector<std::string>& choiceElement,
                           const std::vector<std::string>& removeElement) = 0;

    /**
     * Add expected callbacks for adding a key value pair to a list.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the list that the pair will be added to.
     * \param key the key to be added.
     * \param value the value to be added.
     */
    virtual void addKeyValuePair(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& listElement,
                                 const std::string& key,
                                 const std::string& value) = 0;
    
    /**
     * Add expected callbacks for commiting a number of key value pairs to a list.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the list that the pair will be added to.
     * \param key the key to be added.
     * \param value the value to be added.
     * \param count the number of key value pairs to be added.
     */
    virtual void commitKeyValuePairs(const std::string& modName, 
                                     const std::string& containerName,
                                     const std::vector<std::string>& listElement,
                                     const std::string& key,
                                     const std::string& value,
                                     int count) = 0;

    /**
     * Add expected callbacks for deleting a key from a list.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the list that the key will be added to.
     * \param key the key to be added.
     */
    virtual void deleteKey(const std::string& modName, 
                           const std::string& containerName,
                           const std::vector<std::string>& listElement,
                           const std::string& key) = 0;

    /**
     * Add expected callbacks for deleting a key value pair from a list.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the list that the pair will be deleted from.
     * \param key the key to be deleted.
     * \param value the value to be deleted.
     */
    virtual void deleteKeyValuePair(const std::string& modName, 
                                    const std::string& containerName,
                                    const std::vector<std::string>& listElement,
                                    const std::string& key,
                                    const std::string& value) = 0;

    /**
     * Add expected callbacks for adding a resource node.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param elements a vector representing the hierarchy of elements 
     * leading to the resource node.
     * \param statusConfig true if the statusConfig leaf is added 
     * \param alarmConfig true if the alarmConfig leaf is added 
     */
    virtual void addResourceNode(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& elements,
                                 bool statusConfig,
                                 bool alarmConfig) = 0;

    /**
     * Add expected callbacks for adding a resource connection.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param elements a vector representing the hierarchy of elements 
     * leading to the resource connection.
     */
    virtual void addResourceCon(const std::string& modName, 
                                const std::string& containerName,
                                const std::vector<std::string>& elements) = 0;

    /**
     * Add expected callbacks for adding a stream connection.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param elements a vector representing the hierarchy of elements 
     * leading to the stream connection.
     */
    virtual void addStreamCon(const std::string& modName, 
                              const std::string& containerName,
                              const std::vector<std::string>& elements) = 0;

    /**
     * Add expected callbacks for adding a leaf to a container or updating a
     * leaf in a container.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the leaf.
     * \param phase the specific edit operation which has beem invoked.
     */
    virtual void updateLeaf(const std::string& modName, 
                            const std::string& containerName,
                            const std::vector<std::string>& listElement,
                            const std::string& phase = "merge") = 0;

    /**
     * Add expected callbacks for creating a container or updating a container.
     *
     * \param modName the name of the module from which the callbacks are expected.
     * \param containerName the name of the top level container.
     * \param listElement a vector representing the hierarchy of elements 
     * leading to the container; an empty vector means the top level container is
     * being checked.
     * \param phase the specific edit operation which has beem invoked.
     */
    virtual void updateContainer(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& listElement,
                                 const std::string& phase = "merge") = 0;

    /**
     * Check that the expected callbacks match those logged for a given module.
     *
     * \param modName the name of the module to check callbacks for.
     */
    virtual void checkCallbacks(const std::string& modName);

    /**
     * Clear the expected callbacks.
     */
    void resetExpectedCallbacks();
    
    /**
     * Clear the logged callbacks for a given module.
     *
     * \param modName the name of the module to clear callbacks for.
     */
    void resetModuleCallbacks(const std::string& modName);

private:
    SILCallbackLog::ModuleCallbackData expectedCallbacks_;
};

} // namespace YumaTest

#endif // __CALLBACK_CHECKER_H

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
