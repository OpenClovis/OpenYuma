#ifndef __YUMA_ABSTRACT_NC_SESSION_H
#define __YUMA_ABSTRACT_NC_SESSION_H

// ---------------------------------------------------------------------------|
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <fstream>

// ---------------------------------------------------------------------------|
namespace YumaTest
{
class AbstractYumaOpLogPolicy;

// ---------------------------------------------------------------------------|
/**
 * Abstract base class for Netconf sessions.
 */
class AbstractNCSession
{
public:
    /** 
     * Constructor.
     *
     * \param policy the log filename generation policy
     * \param sessionId the session id.
     */
    AbstractNCSession( std::shared_ptr< AbstractYumaOpLogPolicy > policy,
                       uint16_t sessionId );

    /** Desstructor */
    virtual ~AbstractNCSession();

    /** 
     * Inject the query into netconf. This function is used to inject
     * the supplied XML NC query. It returns the id of the message 
     * that was used to inject the query.
     *
     * \param queryStr a string containing the XML message to inject.
     * \return the id of the message used to inject the query.
     */
    virtual uint16_t injectMessage( const std::string& queryStr ) = 0;

    /**
     * Get the output of the query.
     *
     * \param messageId the id of the message containing the initial
     *                  query.
     * \return the query output returned by Yuma.
     */
    virtual std::string getSessionResult( uint16_t messageId ) const;

    /**
     * Allocate a messageId.
     *
     * \return the next message id;
     */
    uint16_t allocateMessageId() 
    { return ++messageCount_; }

    /**
     * Return sessionId.
     *
     * \return the session id;
     */
    uint16_t getId() 
    { return sessionId_; }

protected:
    /**
     * Genearte a new log filename and add it to the internal map
     * of messages to coressponding lig filename.
     *
     * \param queryStr the query to generate a new log file for.
     */
    std::string newQueryLogFilename( const std::string& queryStr );

    /**
     * Get the log filename associated with the message id.
     *
     * \param messageId the message id.
     * \return the associated log filename.
     */
    std::string retrieveLogFilename( const uint16_t messageId ) const;


    /**
     * Concatenate all of thye logfiles genearted by this session into
     * a single large logfile.
     */
    void concatenateLogFiles();

    /**
     * Concatenate the associated logfile top op.
     *
     * \param op the output stream
     * \param key the key associated with the input filename;
     */
    void appendLog( std::ofstream& op, uint16_t key );
   
protected:
    /** The session id */
    uint16_t sessionId_;

    /** next message id */
    uint16_t messageCount_;

    /** Output log filename generation policy */
    std::shared_ptr< AbstractYumaOpLogPolicy > opLogFilenamePolicy_;

    /** A map of messageIds to output log filenames */
    std::map< uint16_t, std::string > messageIdToLogMap_; 
};

} // namespace YumaTest

#endif // __YUMA_ABSTRACT_NC_SESSION_H
