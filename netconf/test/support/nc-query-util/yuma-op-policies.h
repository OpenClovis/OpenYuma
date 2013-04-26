#ifndef __YUMA_TEST_OP_POLICIES_H
#define __YUMA_TEST_OP_POLICIES_H

// ---------------------------------------------------------------------------|
#include <string>

// ---------------------------------------------------------------------------|
namespace YumaTest
{

/**
 * Abstract base class for Yuma OP Policies.
 */
class AbstractYumaOpLogPolicy 
{
public:
   /** 
    * constructor.
    *
    * \param baseDir the base directory for logfiles.
    */
   explicit AbstractYumaOpLogPolicy( const std::string& baseDir );

   /** Destructor */
   virtual ~AbstractYumaOpLogPolicy();

   /**
    * Generate a logfile name.
    *
    * \param queryStr the query string.
    * \param sessionId the session id.
    * \return a filename for logging the query to.
    */
   virtual std::string generate( const std::string& queryStr, 
                                 uint16_t sessionId ) = 0;

   /**
    * Generate a logfile name for the 'hello' message exchange, 
    * that is just based on the sessionId.
    *
    * \param sessionId the session id.
    * \return a filename for logging the hello exchange to.
    */
   virtual std::string generateHelloLog( uint16_t sessionId ) = 0;

   /**
    * Generate a logfile name for writing all of the sessions messages
    * to. This function is used to generate the name of the file that 
    * contains all of the messages from the session. This is used by
    * the Session's destructor, which writes the messages from
    * the session to the generated file.
    *
    * \param sessionId the session id.
    * \return a filename for logging the sessions messages.
    */
   virtual std::string generateSessionLog( uint16_t sessionId );

protected:
   std::string baseDir_;      ///< The base directory for logfiles
};

/**
 * Log filename generation policy that generates the name of the
 * Yuma output logfile based in the injected query's mnessage id. 
 * The Yuma test harness redirects output into a log
 * generates awhich is generated when a message is injected
 * into Yuma by the test harness.based on
 */
class MessageIdLogFilenamePolicy : public AbstractYumaOpLogPolicy
{
public:
   /** 
    * constructor.
    */
   explicit MessageIdLogFilenamePolicy( const std::string& baseDir );

   /** Destructor */
   virtual ~MessageIdLogFilenamePolicy();

   /**
    * Generate a logfile name based on the message id in the supplied
    * query. The logfilename will have the following format:
    * \<baseDir\>/\<baseName\>-ses[id]-[messageId].xml.
    *
    * \param queryStr the query string
    * \param sessionId the session id
    */
   virtual std::string generate( const std::string& queryStr,
                                 uint16_t sessionId );

   /**
    * Generate a logfile name for the 'hello' message exchange, 
    * that is just based on the sessionId.
    *
    * \param sessionId the session id
    */
   virtual std::string generateHelloLog( uint16_t sessionId );
};

} // namespace YumaTest

#endif // __YUMA_TEST_OP_POLICIES_H
