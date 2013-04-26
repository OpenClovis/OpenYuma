#ifndef __YUMA_NC_MESSAGE_BUILDER__H
#define __YUMA_NC_MESSAGE_BUILDER__H

#include <string>
#include <cstdint>
#include <boost/lexical_cast.hpp>

namespace YumaTest
{

// ---------------------------------------------------------------------------!
/**
 * Utility class for building Netconf XML Messages.
 */
class NCMessageBuilder
{
public:
    /** Constructor */
    explicit NCMessageBuilder();
 
    /** Destructor */
    virtual ~NCMessageBuilder();
 
    /**
     * Build aNetconf Message. This function simlpy adds the XML 
     * start tag and NC_SSH_END indication to the supplied message.
     *
     * \param message the message to build.
     * \return a valid Netconf Message.
     */
    std::string buildNCMessage( const std::string& message ) const;
 
    /**
     * Build a Netconf 'hello' message.
     *
     * \return a Netconf 'hello' Message.
     */
    std::string buildHelloMessage() const;
 
    /**
     * Build a Netconf 'rpc' message.
     *
     * \param queryStr the RPC body
     * \param messageId the id of the message
     * \return a Netconf 'rpc' Message.
     */
    std::string buildRPCMessage( const std::string& queryStr,
                                 uint16_t messageId ) const;

    /**
     * Build a commit message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'rpc-commit' Message.
     */
    std::string buildCommitMessage( const uint16_t messageId );

    /**
     * Build a confirmed-commit message.
     *
     * \param timeout the confirm-timeout of the message in seconds
     * \param messageId the id of the message
     * \return a Netconf 'rpc-commit' Message.
     */
    std::string buildConfirmedCommitMessage( const int timeout,
                                             const uint16_t messageId );

    /**
     * Build a Netconf 'load' message.
     *
     * \param moduleName the name of the module to load.
     * \param messageId the id of the message
     * \return a Netconf 'load' Message.
     */
    std::string buildLoadMessage( const std::string& moduleName,
                                  uint16_t messageId ) const;
 
    /** 
     * Build a Netconf 'get-config' message
     *
     * \param filter the filter to apply in the format:
     *      \<filter type="XXXX' select="YYYYY" /\>.
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get-config' Message.
     */
     std::string buildGetConfigMessage( const std::string& filter,
                                        const std::string& target,
                                        uint16_t messageId ) const;
 
    /**
     * Build a Netconf 'get-config' message with an XPath filter.
     *
     * \param xPathStr the XPath filter to apply
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get-config' Message.
     *
     */
     std::string buildGetConfigMessageXPath( const std::string& xPathStr,
                                             const std::string& target,
                                             uint16_t messageId ) const;

    /**
     * Build a Netconf 'get-config' message with a Subtree filter.
     *
     * \param subtreeStr the Subtree filter to apply
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get-config' Message.
     *
     */
     std::string buildGetConfigMessageSubtree( const std::string& subtreeStr,
                                               const std::string& target,
                                               uint16_t messageId ) const;

    /** 
     * Build a Netconf 'get' message
     *
     * \param filter the filter to apply in the format:
     *      \<filter type="XXXX' select="YYYYY" /\>.
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get' Message.
     */
     std::string buildGetMessage( const std::string& filter,
                                  const std::string& target,
                                  uint16_t messageId ) const;
 
    /**
     * Build a Netconf 'get' message with an XPath filter.
     *
     * \param xPathStr the XPath filter to apply
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get' Message.
     *
     */
     std::string buildGetMessageXPath( const std::string& xPathStr,
                                       const std::string& target,
                                       uint16_t messageId ) const;

    /**
     * Build a Netconf 'get' message with a Subtree filter.
     *
     * \param subtreeStr the Subtree filter to apply
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'get-config' Message.
     *
     */
     std::string buildGetMessageSubtree( const std::string& subtreeStr,
                                         const std::string& target,
                                         uint16_t messageId ) const;

    /**
     * Build a Netconf 'lock' message.
     *
     * \param messageId the id of the message
     * \param target the target database name
     * \return a Netconf 'lock' Message.
     */
    std::string buildLockMessage( uint16_t messageId, 
                                  const std::string& target ) const;

    /**
     * Build a Netconf 'unlock' message.
     *
     * \param messageId the id of the message
     * \param target the target database name
     * \return a Netconf 'unlock' Message.
     */
    std::string buildUnlockMessage( uint16_t messageId, 
                                    const std::string& target ) const;

    /**
     * Build a Netconf 'validate' message.
     *
     * \param messageId the id of the message
     * \param source the source database name
     * \return a Netconf 'validate' Message.
     */
    std::string buildValidateMessage( uint16_t messageId, 
                                      const std::string& source ) const;

    /**
     * Build a Netconf 'set-log-level' message.
     * The following Log Levels are supported:
     * <ul>
     *   <li>off</li>
     *   <li>error</li>
     *   <li>warn</li>
     *   <li>info</li>
     *   <li>debug</li>
     *   <li>debug2</li>
     *   <li>debug3</li>
     *   <li>debug4</li>
     * </ul>
     *
     * \param logLevelStr the log level to set to lock
     * \param messageId the id of the message
     * \return a Netconf 'set-log-level' Message.
     */
    std::string buildSetLogLevelMessage( const std::string& logLevelStr,
                                         uint16_t messageId ) const;

    /**
     * Build a Netconf 'edit-config' message.
     *
     * \param configChange the configuration change
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'edit-config' message.
     */
    std::string buildEditConfigMessage( const std::string& configChange,
                    const std::string& target,
                    uint16_t messageId ) const;

    /**
     * Build a Netconf 'copy-config' message.
     *
     * \param target the target database name
     * \param source the source database name
     * \param messageId the id of the message
     * \return a Netconf 'copy-config' message.
     */
    std::string buildCopyConfigMessage( const std::string& target,
                    const std::string& source,
                    uint16_t messageId ) const;

    /**
     * Build a Netconf 'delete-config' message.
     *
     * \param target the target database name
     * \param messageId the id of the message
     * \return a Netconf 'delete-config' message.
     */
    std::string buildDeleteConfigMessage( const std::string& target,
                    uint16_t messageId ) const;

    /**
     * Build a Netconf 'discard-changes' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'edit-config' message.
     */
    std::string buildDiscardChangesMessage( uint16_t messageId ) const;

    /**
     * Build a Netconf 'get-my-session' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'edit-config' message.
     */
    std::string buildGetMySessionMessage( uint16_t messageId ) const;

    /**
     * Build a Netconf 'set-my-session' message.
     *
     * \param indent the indent to be set.
     * \param linesize the linesize to be set.
     * \param withDefaults the with-defaults setting to be set.
     * \param messageId the id of the message
     * \return a Netconf 'edit-config' message.
     */
    std::string buildSetMySessionMessage( const std::string& indent,
                                          const std::string& linesize,
                                          const std::string& withDefaults,
                                          uint16_t messageId ) const;

    /**
     * Build a Netconf 'kill-session' message.
     *
     * \param sessionId the id of the session to be killed
     * \param messageId the id of the message
     * \return a Netconf 'kill-session' message.
     */
    std::string buildKillSessionMessage( uint16_t sessionId,
                                         uint16_t messageId ) const;

    /**
     * Build a Netconf 'close-session' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'close-session' message.
     */
    std::string buildCloseSessionMessage( uint16_t messageId ) const;

    /**
     * Build a Netconf 'shutdown' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'shutdown' message.
     */
    std::string buildShutdownMessage( uint16_t messageId ) const;

    /**
     * Build a Netconf 'restart' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'restart' message.
     */
    std::string buildRestartMessage( uint16_t messageId ) const;

    /**
     * Build a Netconf 'get-schema' message.
     *
     * \param messageId the id of the message
     * \return a Netconf 'get-schema' message.
     */
    std::string buildGetSchemaMessage( const std::string& schemaId,
                                       uint16_t messageId ) const;

    /**
     * Set the default edit operation.
     *
     * \param defaultOperation the new default operation
     */
    void setDefaultOperation( const std::string& defaultOperation );

    /**
     * Set the default edit operation.
     *
     * \param defaultOperation the new default operation
     */
    const std::string getDefaultOperation() const;

    /**
     * Utility function for generating text for 'edit-config' operations.
     *
     * \param moduleName the name of the module
     * \param moduleNs the namespace of the module
     * \param operation the operation to perform 
     * \return a std::string containing the create text
     */
    std::string genTopLevelContainerText( const std::string& moduleName, 
                                          const std::string& moduleNs, 
                                          const std::string& operation ) const;

    /**
     * Generating text for interface container 'edit-config' operations.
     *
     * \param moduleNs the namespace of the module
     * \param type the type of the interface (ethernet or atm)
     * \param MTU the MTU of the interface
     * \param operation the operation to perform 
     * \return a std::string containing the create text
     */
    std::string genInterfaceContainerText( const std::string& moduleNs, 
                                           const std::string& type,
                                           int mtu,
                                           const std::string& operation ) const;

    /**
     * Generate XML text for an operation on a node.
     * This function generates XML in the following form:
     *
     * \<nodeName xmlns="NS" nc_operation="operation"\>
     *   nodeVal
     * \</nodeName\>
     *
     * \param nodeName the name of the node 
     * \param nodeVal the new value of the node 
     * \param operation the operation to perform 
     * \return a string containing the xml formatted query
     */
    std::string genOperationText( const std::string& nodeName,
                                  const std::string& nodeVal,
                                  const std::string& operation ) const;

    /**
     * Utility template function for generating operation text.
     *
     * \param nodeName the name of the node 
     * \param nodeVal the new value of the node 
     * \param operation the operation to perform 
     * \return a string containing the xml formatted query
     */
    std::string genOperationText( const std::string& nodeName,
                                  bool nodeVal,
                                  const std::string& operation ) const
    {
        return genOperationText( nodeName, 
                ( nodeVal ? "true" : "false" ), operation );
    }

    /**
     * Utility template function for generating operation text.
     *
     * \param nodeName the name of the node 
     * \param nodeVal the new value of the node 
     * \param operation the operation to perform 
     * \return a string containing the xml formatted query
     */
    template<typename T>
    std::string genOperationText( const std::string& nodeName,
                                  const T& nodeVal,
                                  const std::string& operation ) const
    {
        return genOperationText( nodeName, 
                boost::lexical_cast<std::string>( nodeVal ), operation );
    }

    /**
     * Generate XML text for an operation on a node.
     * This function generates XML in the following form:
     *
     * \<nodeName xmlns="NS" nc_operation="operation"\>
     *   \<keyName\>keyVal\</keyName\>
     * \</nodeName\>
     *
     * \param nodeName the name of the node 
     * \param keyName the name of the key for the node 
     * \param keyVal the new value of the key 
     * \param operation the operation to perform 
     * \return a string containing the xml formatted query
     */
    std::string genKeyOperationText( const std::string& nodeName,
                                     const std::string& keyName,
                                     const std::string& keyVal,
                                     const std::string& operation ) const;

    /**
     * Utility function for formatting module operation text.
     * This formats the supplied text as follows:
     * \<moduleName xmlns="moduleNs" \> queryText \</moduleName\>
     *
     * \param moduleName the name of the module
     * \param moduleNs the namespace of the module
     * \param queryText the query to format
     * \return a string containing the xml formatted query
     */
    std::string genModuleOperationText( const std::string& moduleName,
                                        const std::string& moduleNs,
                                        const std::string& queryText ) const;
  
    
    /**
     * Utility function for formatting a 'path' around the supplied 
     * operation text.
     * This formats the supplied text as follows:
     * \<nodeName\>
     *   \<keyName\>keyVal\</keyName\>
     *   queryText 
     * \</nodeName\>
     *
     * \param nodeName the name of the parent node
     * \param keyName the name of the key for the node 
     * \param keyVal the new value of the key 
     * \param queryText the query to format
     * \param op the operation text. 
     * \return a string containing the xml formatted query
     */
    std::string genKeyParentPathText( const std::string& nodeName,
                                      const std::string& keyName,
                                      const std::string& keyVal,
                                      const std::string& queryText,
                                      const std::string& op="" ) const;

protected:
    /**
     * Utility function for generating node text.
     *
     * \param nodeName the name of the parent node
     * \param op the operation text. 
     * \return a string containing the xml formatted node 
     * ( note this does not include the closing '>' )
     */
    std::string genNodeText( const std::string& nodeName, 
                             const std::string& op ) const;
    /**
     * Utility function for generating xmlns text.
     *
     * \param xmlnsArg the argument string (e.g. xmlns or xmlns:nc )
     * \param ns the namespace string 
     */
    std::string genXmlNsText( const std::string& xmlnsArg, 
                              const std::string& ns ) const;

    /**
     * Utility function for generating xmlns text.
     *
     * \param ns the namespace string 
     */
    std::string genXmlNsText( const std::string& ns ) const;

    /**
     * Utility function for generating xmlns:nc text.
     *
     * \param ns the namespace string 
     */
    std::string genXmlNsNcText( const std::string& ns ) const;

protected:
    std::string defaultOperation_; ///< The default edit operation
};

} // namespace YumaTest

#endif // __YUMA_NC_MESSAGE_BUILDER__H
