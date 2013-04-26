// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/fixtures/state-get-module-fixture.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>
#include <boost/foreach.hpp>

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/nc-query-util/nc-query-test-engine.h"
#include "test/support/nc-session/abstract-nc-session-factory.h"
#include "test/support/misc-util/log-utils.h"
#include "test/support/checkers/xml-content-checker.h" 
#include "test/support/checkers/string-presence-checkers.h"

// ---------------------------------------------------------------------------|
// File wide namespace use
// ---------------------------------------------------------------------------|
using namespace std;

// ---------------------------------------------------------------------------|

namespace YumaTest 
{

// ---------------------------------------------------------------------------|
StateGetModuleFixture::StateGetModuleFixture() 
    : StateDataModuleCommonFixture(
          "http://netconfcentral.org/ns/toaster" ) 
    , filteredStateData_( new ToasterContainer )
{
    // ensure the module is loaded
    queryEngine_->loadModule( primarySession_, "toaster" );
}

// ---------------------------------------------------------------------------|
StateGetModuleFixture::~StateGetModuleFixture() 
{
}

// ---------------------------------------------------------------------------|
void StateGetModuleFixture::configureToasterInFilteredState( 
        const ToasterContainerConfig& config )
{
    if ( config.toasterManufacturer_ )
    {
        filteredStateData_->toasterManufacturer_ = *config.toasterManufacturer_;
    }
    if ( config.toasterModelNumber_ )
    {
        filteredStateData_->toasterModelNumber_ = *config.toasterModelNumber_;
    }
    if ( config.toasterStatus_ )
    {
        filteredStateData_->toasterStatus_ = *config.toasterStatus_;
    }
}

// ---------------------------------------------------------------------------|
void StateGetModuleFixture::clearFilteredState()
{
    filteredStateData_->toasterManufacturer_.clear();
    filteredStateData_->toasterModelNumber_.clear();
    filteredStateData_->toasterStatus_.clear();
}

// ---------------------------------------------------------------------------|
void StateGetModuleFixture::checkXpathFilteredState(const string& xPath) const
{
    XMLContentChecker<ToasterContainer> checker( filteredStateData_ );
    queryEngine_->tryGetXpath( primarySession_, xPath, 
            writeableDbName_, checker );
}

// ---------------------------------------------------------------------------|
void StateGetModuleFixture::checkSubtreeFilteredState(
        const string& subtree) const
{
    XMLContentChecker<ToasterContainer> checker( filteredStateData_ );
    queryEngine_->tryGetSubtree( primarySession_, subtree, 
            writeableDbName_, checker );
}

} // namespace StateDataNetconfIntegrationTest

//------------------------------------------------------------------------------
// End of file
//------------------------------------------------------------------------------
