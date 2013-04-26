#ifndef __INTEG_FIXTURE_HELPER_H
#define __INTEG_FIXTURE_HELPER_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/abstract-fixture-helper.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <string>
#include <vector>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Helper class for integration test fixtures.
 */
class IntegFixtureHelper : public AbstractFixtureHelper
{
public:
    /**
     * Mimic a yuma startup if required by test harness.
     */
    virtual void mimicStartup(); 

    /**
     * Mimic a yuma shutdown if required by test harness.
     */
    virtual void mimicShutdown(); 

    /**
     * Mimic a yuma restart if required by test harness.
     */
    virtual void mimicRestart(); 

private:
    /** 
     * Initialise the NCX engine. 
     * This function this simply calls ncx_init() and checks that it 
     * returned NO_ERR.  
     */
    void initialiseNCXEngine(int numArgs, const char** argv);

    /**
     * Load the base schemas.
     * This function loads the following base schemas:
     * <ul>
     *   <li>NCXMOD_YUMA_NETCONF - NETCONF data types and RPC methods.</li>
     *   <li>NCXMOD_NETCONFD - The netconf server boot parameter definition 
     *                        file.</li>
     * </ul>
     */
    void loadBaseSchemas();

    /**
     * Load the core schemas.
     * This function loads the following base schemas:
     * <ul>
     *   <li>NCXMOD_YUMA_NETCONF - NETCONF data types and RPC methods.</li>
     *   <li>NCXMOD_NETCONFD - The netconf server boot parameter definition 
     *                        file.</li>
     * </ul>
     */
    void loadCoreSchema();
    
    /**
     * Perform stage 1 Agt initialisation.
     * This function calls agt_init1 to perform stage 1 Agt initialisation.
     */
    void stage1AgtInitialisation(int numArgs, const char** argv);

    /**
     * Perform stage 2 Agt initialisation.
     * This function calls agt_init2 to perform stage 2 Agt initialisation.
     */
    void stage2AgtInitialisation();
};

} // namespace YumaTest

#endif // __INTEG_FIXTURE_HELPER_H

