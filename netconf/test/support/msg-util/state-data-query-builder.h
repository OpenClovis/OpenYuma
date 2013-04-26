#ifndef __STATE_DATA_QUERY_BUILDER_H
#define __STATE_DATA_QUERY_BUILDER_H

// ---------------------------------------------------------------------------|
// Test Harness includes
// ---------------------------------------------------------------------------|
#include "test/support/msg-util/NCMessageBuilder.h"
#include "test/support/db-models/state-data-test-db.h"

// ---------------------------------------------------------------------------|
// Standard includes
// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Utility lass for build queries against the device test whole
 * container.
 */
class StateDataQueryBuilder : public NCMessageBuilder
{
public:
    /** 
     * Constructor.
     *
     * \param moduleNs the namespace of the module defining the XPO
     * container.
     */
    explicit StateDataQueryBuilder( const std::string& moduleNS );

    /** Destructor */
    virtual ~StateDataQueryBuilder();

    /**
     * Generate a query on the basic toaster container.
     *
     * \param op the operation to perform.
     * \return XML formatted query
     */
    std::string genToasterQuery( const std::string& op ) const;

private:
     const std::string moduleNs_; ///< the module's namespace
};

} // namespace YumaTest

#endif // __STATE_DATA_QUERY_BUILDER_H
