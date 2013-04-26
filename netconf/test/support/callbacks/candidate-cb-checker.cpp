// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/callbacks/candidate-cb-checker.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|
namespace YumaTest
{

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addMainContainer(const std::string& modName, 
                                          const std::string& containerName)
{
    vector<string> empty;
    addExpectedCallback(modName, containerName, empty, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addElement(const std::string& modName, 
                                    const std::string& containerName,
                                    const std::vector<std::string>& element)
{
    addExpectedCallback(modName, containerName, element, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addKey(const std::string& modName, 
                                const std::string& containerName,
                                const std::vector<std::string>& listElement,
                                const std::string& key)
{
    vector<string> elements(listElement);
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addChoice(const std::string& modName, 
                                   const std::string& containerName,
                                   const std::vector<std::string>& choiceElement,
                                   const std::vector<std::string>& removeElement)
{
    vector<string> deleteElements(removeElement);
    addExpectedCallback(modName, containerName, deleteElements, "edit", "validate", "");
    vector<string> addElements(choiceElement);
    addExpectedCallback(modName, containerName, addElements, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addKeyValuePair(const std::string& modName, 
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
    elements.push_back(value); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::commitKeyValuePairs(const std::string& modName, 
                                             const std::string& containerName,
                                             const std::vector<std::string>& listElement,
                                             const std::string& key,
                                             const std::string& value,
                                             int count)
{
    vector<string> elements;
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    for (int i = 0; i < count; i++)
    {
        elements = listElement;
        addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
        elements.push_back(key);
        addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
        elements.pop_back();
        elements.push_back(value);
        addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    }
    elements.clear(); 
    addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    for (int i = 0; i < count; i++)
    {
        elements = listElement;
        addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
        elements.push_back(key);
        addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
        elements.pop_back();
        elements.push_back(value);
        addExpectedCallback(modName, containerName, elements, "edit", "apply", "");
    }
    elements.clear(); 
    addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
    for (int i = 0; i < count; i++)
    {
        elements = listElement;
        addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
        elements.push_back(key);
        addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
        elements.pop_back();
        elements.push_back(value);
        addExpectedCallback(modName, containerName, elements, "edit", "commit", "create");
    }
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::deleteKey(const std::string& modName, 
                                   const std::string& containerName,
                                   const std::vector<std::string>& listElement,
                                   const std::string& key)
{
    vector<string> elements(listElement);
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::deleteKeyValuePair(const std::string& modName, 
                                            const std::string& containerName,
                                            const std::vector<std::string>& listElement,
                                            const std::string& key,
                                            const std::string& value)
{
    vector<string> elements(listElement);
    elements.push_back(value); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.pop_back(); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
    elements.push_back(key); 
    addExpectedCallback(modName, containerName, elements, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addResourceNode(const std::string& modName, 
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
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addResourceCon(const std::string& modName, 
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
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::addStreamCon(const std::string& modName, 
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
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::updateLeaf(const std::string& modName, 
                const std::string& containerName,
                const std::vector<std::string>& listElement,
                const std::string& phase)
{
    addExpectedCallback(modName, containerName, listElement, "edit", "validate", "");
}

// ---------------------------------------------------------------------------|
void CandidateCBChecker::updateContainer(const std::string& modName, 
                                         const std::string& containerName,
                                         const std::vector<std::string>& listElement,
                                         const std::string& phase)
{
    addExpectedCallback(modName, containerName, listElement, "edit", "validate", "");
}

} // namespace YumaTest
