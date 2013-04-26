#include "test/support/msg-util/NCMessageBuilder.h"

//#include <iostream>
#include <sstream>
#include <string>

#include "src/ncx/xml_util.h"

// ---------------------------------------------------------------------------!
using namespace std;

// ---------------------------------------------------------------------------!
// Anonymous Namespace
// ---------------------------------------------------------------------------!
namespace
{

// ---------------------------------------------------------------------------!
const char* IETF_BASE = "urn:ietf:params:netconf:base:1.0";
const char* IETF_NS = "urn:ietf:params:xml:ns:netconf:base:1.0";
const char* YUMA_NS = "http://netconfcentral.org/ns/yuma-system";

} // Anonymous namespace

namespace YumaTest
{

// ---------------------------------------------------------------------------!
NCMessageBuilder::NCMessageBuilder() : defaultOperation_( "merge" )
{
}

// ---------------------------------------------------------------------------!
NCMessageBuilder::~NCMessageBuilder()
{
}

// ---------------------------------------------------------------------------!
void NCMessageBuilder::setDefaultOperation( const string& defaultOperation )
{
    defaultOperation_ = defaultOperation;
}

// ---------------------------------------------------------------------------!
const std::string NCMessageBuilder::getDefaultOperation() const
{
    return defaultOperation_;
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::genXmlNsText( const string& xmlnsArg, 
                                       const string& ns ) const
{
    stringstream query;

    query << xmlnsArg << "=\"" << ns << "\"";

    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::genXmlNsText( const string& ns ) const
{
    return genXmlNsText( "xmlns", ns );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::genXmlNsNcText( const string& ns ) const
{
    return genXmlNsText( "xmlns:nc", ns );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildNCMessage( const string& message ) const
{
    stringstream query;

    query << XML_START_MSG << "\n" << message << "\n"; // << NC_SSH_END;
   
    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildHelloMessage() const
{
    stringstream query;

    query << "<hello " << genXmlNsNcText( IETF_NS )
          << "       " << genXmlNsText( IETF_NS ) << ">"
          << "  <capabilities>"
          << "    <capability>" << IETF_BASE << "</capability>"
          << "  </capabilities>\n"
          << "</hello>";

   return buildNCMessage( query.str() );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildRPCMessage( const std::string& queryStr,
                                          const uint16_t messageId ) const
{
    stringstream query;
    
    query << "<rpc message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsNcText( IETF_NS ) << "\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << queryStr << "\n"
          << "</rpc>";

    return buildNCMessage( query.str() );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildLoadMessage( const std::string& moduleName,
                                           const uint16_t messageId ) const
{
    stringstream query;
    query << "  <load " << genXmlNsText( YUMA_NS ) << ">\n"
          << "    <module> " << moduleName << " </module>\n"
          << "  </load>";

    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildGetConfigMessage( const string& filter,
                                                const string& target,
                                                const uint16_t messageId )  const
{
    stringstream query;
    query << "  <get-config " << genXmlNsText( IETF_NS ) << ">\n"
          << "    <source> " << "<" << target << "/> " << "</source>\n"
          << "    " << filter << "\n"
          << "  </get-config>";
 
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder:: buildGetConfigMessageXPath( 
        const string& xPathStr,
        const string& target,
        const uint16_t messageId )  const
{
    stringstream query;
    query << "<filter type=\"xpath\" " 
          << "select=\"" << xPathStr << "\"/>" ;
 
    return buildGetConfigMessage( query.str(), target, messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder:: buildGetConfigMessageSubtree( 
        const string& subtreeStr,
        const string& target,
        const uint16_t messageId )  const
{
    stringstream query;
    query << "<filter type=\"subtree\">\n" 
          << subtreeStr << "\n"
          << "    </filter>";
 
    return buildGetConfigMessage( query.str(), target, messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildGetMessage( const string& filter,
                                          const string& target,
                                          const uint16_t messageId )  const
{
    stringstream query;
    query << "  <get " << genXmlNsText( IETF_NS ) << ">\n"
          << "    " << filter << "\n"
          << "  </get>";
 
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder:: buildGetMessageXPath( 
        const string& xPathStr,
        const string& target,
        const uint16_t messageId )  const
{
    stringstream query;
    query << "<filter type=\"xpath\" " 
          << "select=\"" << xPathStr << "\"/>" ;
 
    return buildGetMessage( query.str(), target, messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder:: buildGetMessageSubtree( 
        const string& subtreeStr,
        const string& target,
        const uint16_t messageId )  const
{
    stringstream query;
    query << "<filter type=\"subtree\">\n" 
          << subtreeStr << "\n"
          << "    </filter>";
 
    return buildGetMessage( query.str(), target, messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildLockMessage( const uint16_t messageId,
                                           const string& target ) const
{
    stringstream query;
    
    query << "  <lock><target><" << target << "/></target></lock>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildUnlockMessage( const uint16_t messageId,
                                             const string& target ) const
{
    stringstream query;
    
    query << "  <unlock><target><" << target << "/></target></unlock>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildValidateMessage( const uint16_t messageId,
                                               const string& source ) const
{
    stringstream query;
    
    query << "  <validate><source><" << source << "/></source></validate>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildSetLogLevelMessage( 
      const std::string& logLevelStr,
      const uint16_t messageId ) const
{
    stringstream query;
    
    query << "  <set-log-level " << genXmlNsText( YUMA_NS ) << ">\n"
          << "     <log-level>" << logLevelStr << "</log-level>\n"
          << "  </set-log-level>\n";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildEditConfigMessage( const string& configChange, 
        const string& target,
        uint16_t messageId ) const
{
    stringstream query;
    query << "  <edit-config " << genXmlNsText( IETF_NS ) << ">\n"
          << "    <target> " << "<" << target << "/> " << "</target>\n"
          << "    <default-operation>" << defaultOperation_ 
              << "</default-operation>\n"
          << "    <config>\n"
          << "      " << configChange << "\n"
          << "    </config>\n"
          << "  </edit-config>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildCopyConfigMessage( const string& target, 
        const string& source,
        uint16_t messageId ) const
{
    stringstream query;
    query << "  <copy-config " << genXmlNsText( IETF_NS ) << ">\n"
          << "    <target> " << "<" << target << "/> " << "</target>\n"
          << "    <source> " << "<" << source << "/> " << "</source>\n"
          << "  </copy-config>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildDeleteConfigMessage( const string& target, 
        uint16_t messageId ) const
{
    stringstream query;
    query << "  <delete-config " << genXmlNsText( IETF_NS ) << ">\n"
          << "    <target> " << "<" << target << "/> " << "</target>\n"
          << "  </delete-config>";
    return buildRPCMessage( query.str(), messageId );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildDiscardChangesMessage( 
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << genXmlNsNcText( IETF_NS ) << "\n"
          << "     " << "message-id=\"" << messageId << "\">\n"
          << "     " << "<discard-changes/>\n"
          << "</rpc>\n";

    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildGetMySessionMessage( 
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << "message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << "     " << "<get-my-session xmlns="
          << "\"http://netconfcentral.org/ns/yuma-mysession\"/>\n"
          << "</rpc>\n";

    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildSetMySessionMessage( 
                           const string& indent,
                           const string& linesize,
                           const string& withDefaults,
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << "message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << "     " << "<set-my-session xmlns="
          << "\"http://netconfcentral.org/ns/yuma-mysession\">\n"
          << "       " << "<indent>" << indent << "</indent>\n"
          << "       " << "<linesize>" << linesize << "</linesize>\n"
          << "       " << "<with-defaults>" << withDefaults << "</with-defaults>\n"
          << "     " << "</set-my-session>\n"
          << "</rpc>\n";

    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildKillSessionMessage( uint16_t sessionId, 
        uint16_t messageId ) const
{
    stringstream query;
    query << "  <kill-session " << genXmlNsText( IETF_NS ) << ">\n"
          << "    <session-id>" << sessionId << "</session-id>\n"
          << "  </kill-session>";
    return buildRPCMessage( query.str(), messageId );
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildCloseSessionMessage( uint16_t messageId ) const
{
    stringstream query;
    query << "  <close-session " << genXmlNsText( IETF_NS ) << "/>\n";
    return buildRPCMessage( query.str(), messageId );
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildShutdownMessage( 
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << "message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << "     " << "<shutdown xmlns="
          << "\"http://netconfcentral.org/ns/yuma-system\"/>\n"
          << "</rpc>\n";

    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildRestartMessage( 
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << "message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << "     " << "<restart xmlns="
          << "\"http://netconfcentral.org/ns/yuma-system\"/>\n"
          << "</rpc>\n";

    return query.str();
}

// ---------------------------------------------------------------------------!
string NCMessageBuilder::buildGetSchemaMessage( 
                           const string& schemaId,
                           const uint16_t messageId ) const
{
    stringstream query;
    
    query << XML_START_MSG << "\n"
          << "<rpc " << "message-id=\"" << messageId << "\"\n"
          << "     " << genXmlNsText( IETF_NS ) << ">\n"
          << "     " << "<get-schema xmlns="
          << "\"urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring\">\n"
          << "       " << "<identifier>" << schemaId << "</identifier>\n"
          << "     " << "</get-schema>\n"
         << "</rpc>\n";

    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genNodeText( 
        const string& nodeName,
        const string& op ) const
{
    stringstream query;
 
    query << "<" << nodeName;
    if ( !op.empty() )
    {
        query << " " << genXmlNsNcText( IETF_NS );
        query << " nc:operation=\"" << op << "\"";
    }

    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genTopLevelContainerText( 
        const string& moduleName,
        const string& moduleNs,
        const string& operation ) const
{
    stringstream query;
 
    query << genNodeText( moduleName, operation );
    query << " " << genXmlNsText( moduleNs ) << "/>";
        
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genInterfaceContainerText( 
        const string& moduleNs,
        const string& type,
        int mtu,
        const string& operation ) const
{
    stringstream mtuStream, query;
    mtuStream << mtu;
 
    query << genNodeText( "interface", operation );
    query << "  " << genXmlNsText( moduleNs ) << ">\n"
          << "    " << genOperationText( "ifType", type, operation)
          << "    " << genOperationText( "ifMTU", mtuStream.str(), operation)
          << "</interface>\n";
        
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genOperationText( 
        const string& nodeName,
        const string& nodeVal,
        const string& operation ) const
{
    stringstream query;
 
    query << genNodeText( nodeName, operation );
    query << ">" 
          << nodeVal 
          << "</" << nodeName << ">\n";
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genKeyOperationText( 
        const string& nodeName,
        const string& keyName,
        const string& keyVal,
        const string& operation ) const
{
    stringstream query;
 
    query << genNodeText( nodeName, operation );
    query << ">"
          << "<" << keyName << ">" << keyVal << "</" << keyName << ">"
          << "</" << nodeName << ">";
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genKeyParentPathText( 
        const string& nodeName,
        const string& keyName,
        const string& keyVal,
        const string& queryText,
        const string& op ) const
{
    stringstream query;

    query << genNodeText( nodeName, op );
    query << ">"
          << "<" << keyName << ">" << keyVal << "</" << keyName << ">\n"
          << queryText << "\n"
         << "</" << nodeName << ">"; 
 
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::genModuleOperationText( 
        const string& moduleName,
        const string& moduleNs,
        const string& queryText ) const
{
    stringstream query;
 
    query << "<" << moduleName << " "
          << genXmlNsText( moduleNs ) << ">\n"
          << queryText
          << "</" << moduleName << ">";
        
    return query.str();
}

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildCommitMessage( const uint16_t messageId )
{
    return buildRPCMessage( "<commit/>", messageId );
} 

// ------------------------------------------------------------------------|
string NCMessageBuilder::buildConfirmedCommitMessage( const int timeout,
                                                      const uint16_t messageId )
{
    stringstream query;
    query << "<commit " << genXmlNsText( IETF_NS ) << ">\n" 
          << "    " << "<confirmed/>\n"
          << "    " << "<confirm-timeout>" << timeout << "</confirm-timeout>\n"
          << "</commit>";
    return buildRPCMessage( query.str(), messageId );
} 

} // namespace YumaTest
