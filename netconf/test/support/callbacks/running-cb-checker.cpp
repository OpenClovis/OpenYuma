// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/running-cb-checker.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------!
RunningCBChecker::RunningCBChecker() 
   : CallbackChecker()
{
}

// ---------------------------------------------------------------------------!
RunningCBChecker::~RunningCBChecker()
{
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addMainContainer(const std::string& modName, 
                                        const std::string& containerName)
{
    vector<string> empty;
    addExpectedCallback(modName, containerName, empty, "edit", "validate", "");
    addExpectedCallback(modName, containerName, empty, "edit", "apply", "");
    addExpectedCallback(modName, containerName, empty, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addElement(const std::string& modName, 
                                  const std::string& containerName,
                                  const std::vector<std::string>& element)
{
    addExpectedCallback(modName, containerName, element, "edit", "validate", "");
    addExpectedCallback(modName, containerName, element, "edit", "apply", "");
    addExpectedCallback(modName, containerName, element, "edit", "commit", "replace");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addKey(const std::string& modName, 
                              const std::string& containerName,
                              const std::vector<std::string>& listElement,
                              const std::string& key)
{
    vector<string> elements(listElement);
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addChoice(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& choiceElement,
                                 const std::vector<std::string>& removeElement)
{
    vector<string> deleteElements(removeElement);
    addExpectedCallback(modName, containerName, deleteElements, "edit", "validate", "");
    addExpectedCallback(modName, containerName, deleteElements, "edit", "apply", "");
    addExpectedCallback(modName, containerName, deleteElements, "edit", "commit", "delete");
    vector<string> addElements(choiceElement);
    addExpectedCallback(modName, containerName, addElements, "edit", "validate", "");
    addExpectedCallback(modName, containerName, addElements, "edit", "apply", "");
    addExpectedCallback(modName, containerName, addElements, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addKeyValuePair(const std::string& modName, 
                                       const std::string& containerName,
                                       const std::vector<std::string>& listElement,
                                       const std::string& key,
                                       const std::string& value)
{
    vector<string> elements(listElement);
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
    elements.pop_back(); 
    elements.push_back(value); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::commitKeyValuePairs(const std::string& modName, 
                                           const std::string& containerName,
                                           const std::vector<std::string>& listElement,
                                           const std::string& key,
                                           const std::string& value,
                                           int count)
{
    // Do nothing as commits are not performed separately when using writeable running.
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::deleteKey(const std::string& modName, 
                                 const std::string& containerName,
                                 const std::vector<std::string>& listElement,
                                 const std::string& key)
{
    vector<string> elements(listElement);
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");

    //TODO - Add commit callbacks
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::deleteKeyValuePair(const std::string& modName, 
                                          const std::string& containerName,
                                          const std::vector<std::string>& listElement,
                                          const std::string& key,
                                          const std::string& value)
{
    vector<string> elements(listElement);
    elements.push_back(value); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "delete");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "delete");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addResourceNode(const std::string& modName, 
                                       const std::string& containerName,
                                       const std::vector<std::string>& elements,
                                       bool statusConfig,
                                       bool alarmConfig)
{
    vector<string> hierarchy(elements);
    hierarchy.push_back("resourceNode"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("resourceType"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("configuration"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    if (statusConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("statusConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    }
    if (alarmConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("alarmConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    }
    hierarchy.pop_back(); 
    hierarchy.push_back("physicalPath"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("resourceType"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("configuration"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    if (statusConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("statusConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    }
    if (alarmConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("alarmConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    }
    hierarchy.pop_back(); 
    hierarchy.push_back("physicalPath"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("resourceType"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("configuration"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    if (statusConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("statusConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    }
    if (alarmConfig)
    {
        hierarchy.pop_back(); 
        hierarchy.push_back("alarmConfig"); 
        addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    }
    hierarchy.pop_back(); 
    hierarchy.push_back("physicalPath"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addResourceCon(const std::string& modName, 
                                      const std::string& containerName,
                                      const std::vector<std::string>& elements)
{
    vector<string> hierarchy(elements);
    hierarchy.push_back("resourceConnection"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::addStreamCon(const std::string& modName, 
                                    const std::string& containerName,
                                    const std::vector<std::string>& elements)
{
    vector<string> hierarchy(elements);
    hierarchy.push_back("streamConnection"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "validate", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "apply", "");
    hierarchy.pop_back(); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.push_back("id"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationStreamId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourceId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("sourcePinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("destinationPinId"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
    hierarchy.pop_back(); 
    hierarchy.push_back("bitrate"); 
    addExpectedCallback(modName, containerName, hierarchy, "edit", "commit", "create");
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::updateLeaf(const std::string& modName, 
                const std::string& containerName,
                const std::vector<std::string>& listElement,
                const std::string& phase)
{
    addExpectedCallback(modName, containerName, listElement, "edit", "validate", "");
    addExpectedCallback(modName, containerName, listElement, "edit", "apply", "");
    addExpectedCallback(modName, containerName, listElement, "edit", "commit", phase);
}

// ---------------------------------------------------------------------------|
void RunningCBChecker::updateContainer(const std::string& modName, 
                                         const std::string& containerName,
                                         const std::vector<std::string>& listElement,
                                         const std::string& phase)
{
    addExpectedCallback(modName, containerName, listElement, "edit", "validate", "");
    addExpectedCallback(modName, containerName, listElement, "edit", "apply", "");
    addExpectedCallback(modName, containerName, listElement, "edit", "commit", phase);
}

} // namespace YumaTest
