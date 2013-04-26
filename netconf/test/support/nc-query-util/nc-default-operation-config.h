#ifndef __YUMA_NC_DEFAULT_OPERATION_CONFIG_H
#define __YUMA_NC_DEFAULT_OPERATION_CONFIG_H

// ---------------------------------------------------------------------------|
// Standard Includes
// ---------------------------------------------------------------------------|
#include <string>
#include <memory>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class NCMessageBuilder;

// ---------------------------------------------------------------------------|
/**
 * RAII Style change of default operation.
 * This class should be used to change the default-operation that an
 * NCBaseQueryTestEngine uses for generating queries. The constructor
 * stores the current default-operation and sets the new one, the
 * destructor restores the original default-operation.
 */
class DefaultOperationConfig
{
public:
    /**
     * Constructor
     *
     * \param builder the message builder to change the default operation for.
     * \param newDefaultOperation the new default operation.
     */
    DefaultOperationConfig( std::shared_ptr< NCMessageBuilder > builder,
                            const std::string& newDefaultOperation );

    /**
     * Destructor.
     */
    ~DefaultOperationConfig();

private:
    std::shared_ptr< NCMessageBuilder > builder_; ///< the message buidler
    const std::string orignalOperation_ ; ///< the original default operation
};

} // namespace YumaTest

#endif // __YUMA_NC_DEFAULT_OPERATION_CONFIG_H


