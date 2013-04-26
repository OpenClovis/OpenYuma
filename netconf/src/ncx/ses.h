/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
#ifndef _H_ses
#define _H_ses
/*  FILE: ses.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF Session Common definitions module

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
30-dec-05    abb      Begun.
*/

/* used by applications to generate FILE output */
#include <stdio.h>

/* used for timestamps and time deltas */
#include <time.h>

/* used by the agent for the xmlTextReader interface */
#include <xmlreader.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tstamp
#include "tstamp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define SES_MY_SID(S)   ((S)->sid)

#define SES_MY_USERNAME(S)   ((S)->username)

#define SES_KILLREQ_SET(S) ((S)->state >= SES_ST_SHUTDOWN_REQ)

#define SES_ACK_KILLREQ(S) ((S)->state = SES_ST_SHUTDOWN)

#define SES_OUT_BYTES(S) (S)->stats.out_bytes

#define SES_LINELEN(S) (S)->stats.out_line

#define SES_LINESIZE(S) (S)->linesize

#define SES_NULL_SID  0

/* controls the size of each buffer chuck */
#define SES_MSG_BUFFSIZE  2000   // 1024

/* max number of buffer chunks a session can have allocated at once  */
#define SES_MAX_BUFFERS  4096

/* max number of buffers a session is allowed to cache in its freeQ */
#define SES_MAX_FREE_BUFFERS  32

/* max number of buffers to try to send in one call to the write fn */
#define SES_MAX_BUFFSEND   32

/* max number of bytes to try to send in one call to the write_fn */
#define SES_MAX_BYTESEND   0xffff

/* max desired lines size; not a hard limit */
#define SES_DEF_LINESIZE   72

/* max size of a valid base:1.1 chunk header start tag */
#define SES_MAX_STARTCHUNK_SIZE 13

/* max size of the chunk size number in the chunk start tag */
#define SES_MAX_CHUNKNUM_SIZE 10

/* padding at start of buffer for chunk tagging
 * Max: \n#xxxxxxx\n --> 7 digit chunk size
 */
#define SES_STARTCHUNK_PAD  10

/* leave enough room at the end for EOChunks */
#define SES_ENDCHUNK_PAD  4

/* default read buffer size */
#define SES_READBUFF_SIZE  1000

/* port number for NETCONF over TCP */
#define SES_DEF_TCP_PORT    2023
    
/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/* Session ID */
typedef uint32 ses_id_t;

/* Session Types */
typedef enum ses_type_t_ {
    SES_TYP_NONE,
    SES_TYP_NETCONF,
    SES_TYP_NCX,
    SES_TYP_DUMMY
} ses_type_t;


/* NETCONF Transport Types */
typedef enum ses_transport_t_ {
    SES_TRANSPORT_NONE,
    SES_TRANSPORT_SSH,   /* only enum supported */
    SES_TRANSPORT_BEEP,
    SES_TRANSPORT_SOAP,
    SES_TRANSPORT_SOAPBEEP,
    SES_TRANSPORT_TLS,
    SES_TRANSPORT_TCP    /* tail-f NETCONF over TCP */
} ses_transport_t;


/* Session States */
typedef enum ses_state_t_ {
    SES_ST_NONE,
    SES_ST_INIT,
    SES_ST_HELLO_WAIT,
    SES_ST_IDLE,
    SES_ST_IN_MSG,
    SES_ST_SHUTDOWN_REQ,
    SES_ST_SHUTDOWN
} ses_state_t;


/* Session Input Handler States */
typedef enum ses_instate_t_ {
    SES_INST_NONE,
    SES_INST_IDLE,
    SES_INST_INMSG,
    SES_INST_INSTART,
    SES_INST_INBETWEEN,
    SES_INST_INEND
} ses_instate_t;


/* Session Output Mode */
typedef enum ses_mode_t_ {
    SES_MODE_NONE,
    SES_MODE_XML,
    SES_MODE_XMLDOC,
    SES_MODE_HTML,
    SES_MODE_TEXT
} ses_mode_t;


/* Session Termination reason */
typedef enum ses_term_reason_t_ {
    SES_TR_NONE,
    SES_TR_CLOSED,
    SES_TR_KILLED,
    SES_TR_DROPPED,
    SES_TR_TIMEOUT,
    SES_TR_OTHER,
    SES_TR_BAD_START,
    SES_TR_BAD_HELLO
} ses_term_reason_t;


/* prolog parsing state */
typedef enum ses_prolog_state_t_ {
    SES_PRST_NONE,
    SES_PRST_WAITING,
    SES_PRST_DONE
} ses_prolog_state_t;


/*** using uint32 instead of uint64 because the netconf-state
 *** data model is specified that way
 ***/

/* Per Session Statistics */
typedef struct ses_stats_t_ {
    /* extra original internal byte counters */
    uint32            in_bytes;
    uint32            out_bytes;

    /* hack: bytes since '\n', pretty-print */
    uint32            out_line;    

    /* netconf-state counters */
    uint32            inRpcs;
    uint32            inBadRpcs;
    uint32            outRpcErrors;
    uint32            outNotifications;
} ses_stats_t;


/* Session Total Statistics */
typedef struct ses_total_stats_t_ {
    uint32            active_sessions;
    uint32            closed_sessions;
    uint32            failed_sessions;
    uint32            inBadHellos;
    uint32            inSessions;
    uint32            droppedSessions;
    ses_stats_t       stats;
    xmlChar           startTime[TSTAMP_MIN_SIZE];
} ses_total_stats_t;


/* Session Message Buffer */
typedef struct ses_msg_buff_t_ {
    dlq_hdr_t        qhdr;
    size_t           buffstart;        /* buff start pos */
    size_t           bufflen;        /* buff actual size */
    size_t           buffpos;       /* buff cur position */
    boolean          islast;      /* T: last buff in msg */
    xmlChar          buff[SES_MSG_BUFFSIZE];   
} ses_msg_buff_t;


/* embedded Q header for the message ready Q */
typedef struct ses_ready_t_ {
    dlq_hdr_t hdr;
    ses_id_t  sid;
    boolean   inq;
} ses_ready_t;


/* Session Message */
typedef struct ses_msg_t_ {
    dlq_hdr_t        qhdr;        /* Q header for buffcb->msgQ */
    boolean          ready;               /* ready for parsing */
    ses_msg_buff_t  *curbuff;         /* cur position in buffQ */
    dlq_hdr_t        buffQ;             /* Q of ses_msg_buff_t */
    ses_prolog_state_t prolog_state;      /* for insert prolog */
    size_t           curchunksize;           /* cur chunk rcvd */
    size_t           expchunksize;      /* expected chunk size */
} ses_msg_t;

/* optional read function for the session */
typedef ssize_t (*ses_read_fn_t) (void *s,
				  char *buff,
				  size_t bufflen,
                                  boolean *erragain);

/* optional write function for the session */
typedef status_t (*ses_write_fn_t) (void *s);

/* Session Control Block */
typedef struct ses_cb_t_ {
    dlq_hdr_t        qhdr;           /* queued by manager only */
    ses_type_t       type;                      /* session type */
    uint32           protocols_requested;            /* bitmask */
    ncx_protocol_t   protocol;       /* protocol version in use */
    ses_transport_t  transport;               /* transport type */
    ses_state_t      state;                    /* session state */
    ses_mode_t       mode;                      /* session mode */
    ses_id_t         sid;                         /* session ID */
    ses_id_t         killedbysid;       /* killed-by session ID */
    ses_id_t         rollback_sid;   /* session ID for rollback */
    ses_term_reason_t termreason;
    time_t           hello_time;      /* used for hello timeout */
    time_t           last_rpc_time;    /* used for idle timeout */
    xmlChar         *start_time;         /* dateTime start time */
    xmlChar         *username;                       /* user ID */
    xmlChar         *peeraddr;           /* Inet address string */
    boolean          active;            /* <hello> completed ok */
    boolean          notif_active;       /* subscription active */
    boolean          stream_output;        /* buffer/stream svr */
    boolean          noxmlns;          /* xml-nons display-mode */
    boolean          framing11;     /* T: base:1.1, F: base:1.0 */
    xmlTextReaderPtr reader;             /* input stream reader */
    FILE            *fp;             /* set if output to a file */
    int              fd;           /* set if output to a socket */
    ses_read_fn_t    rdfn;          /* set if external write fn */
    ses_write_fn_t   wrfn;           /* set if external read fn */
    uint32           inendpos;      /* inside framing directive */
    ses_instate_t    instate;               /* input state enum */
    uint32           buffcnt;           /* current buffer count */
    uint32           freecnt;            /* current freeQ count */
    dlq_hdr_t        msgQ;              /* Q of ses_msg_t input */
    dlq_hdr_t        freeQ;              /* Q of ses_msg_buff_t */
    dlq_hdr_t        outQ;               /* Q of ses_msg_buff_t */
    ses_msg_buff_t  *outbuff;          /* current output buffer */
    ses_ready_t      inready;            /* header for inreadyQ */
    ses_ready_t      outready;          /* header for outreadyQ */
    ses_stats_t      stats;           /* per-session statistics */
    void            *mgrcb;    /* if manager session, mgr_scb_t */

    /* base:1.1 chunk state handling;
     * need to store number part of incoming chunk markers
     * in the scb in case they are split across buffers
     */
    xmlChar          startchunk[SES_MAX_STARTCHUNK_SIZE+1];

    /* input buffer for session */
    xmlChar         *readbuff;
    uint32           readbuffsize;

    /*** user preferences ***/
    int32            indent;          /* indent N spaces (0..9) */
    uint32           linesize;              /* TERM line length */
    ncx_withdefaults_t  withdef;       /* with-defaults default */
    uint32           cache_timeout;  /* vir-val cache tmr in sec */

    /* agent access control for database reads and writes;
     * for incoming agent <rpc> requests, the access control
     * cache is used to minimize data structure processing
     * during authorization procedures in agt/agt_acm.c
     * there is a back-ptr embedded in the XML header so it can
     * be easily passed to the agt_val and xml_wr functions
     */
    struct agt_acm_cache_t_ *acm_cache;

} ses_cb_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION ses_new_scb
*
* Create a new session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized SCB, or NULL if malloc error
*********************************************************************/
extern ses_cb_t *
    ses_new_scb (void);


/********************************************************************
* FUNCTION ses_new_dummy_scb
*
* Create a new dummy session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized SCB, or NULL if malloc error
*********************************************************************/
extern ses_cb_t *
    ses_new_dummy_scb (void);


/********************************************************************
* FUNCTION ses_free_scb
*
* Free a session control block
*
* INPUTS:
*   scb == session control block to free
* RETURNS:
*   none
*********************************************************************/
extern void
    ses_free_scb (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_putchar
*
* Write one char to the session, without any translation
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* NO CHARS ARE ACTUALLY WRITTEN TO A REAL SESSION!!!
* The 'output ready' indicator will be set and the session
* queued in the outreadyQ.  Non-blocking IO functions
* will send the data when the connection allows.
*
* INPUTS:
*   scb == session control block to start msg 
*   ch = xmlChar to write, cast as uint32 to avoid compiler warnings
*
*********************************************************************/
extern void
    ses_putchar (ses_cb_t *scb,
		 uint32    ch);


/********************************************************************
* FUNCTION ses_putstr
*
* Write a zero-terminated string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*
*********************************************************************/
extern void
    ses_putstr (ses_cb_t *scb,
		const xmlChar *str);


/********************************************************************
* FUNCTION ses_putstr_indent
*
* Write a zero-terminated content string to the session
* with indentation
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
extern void
    ses_putstr_indent (ses_cb_t *scb,
		       const xmlChar *str,
		       int32 indent);


/********************************************************************
* FUNCTION ses_putcstr
*
* write XML element safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
extern void
    ses_putcstr (ses_cb_t *scb,
		 const xmlChar *str,
		 int32 indent);


/********************************************************************
* FUNCTION ses_puthstr
*
* write HTML element safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*
*********************************************************************/
extern void
    ses_puthstr (ses_cb_t *scb,
                 const xmlChar *str);


/********************************************************************
* FUNCTION ses_putcchar
*
* Write one content char to the session, with translation as needed
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* NO CHARS ARE ACTUALLY WRITTEN TO A REAL SESSION!!!
* The 'output ready' indicator will be set and the session
* queued in the outreadyQ.  Non-blocking IO functions
* will send the data when the connection allows.
*
* INPUTS:
*   scb == session control block to write 
*   ch = xmlChar to write, cast as uint32 to avoid compiler warnings
*
*********************************************************************/
extern void
    ses_putcchar (ses_cb_t *scb,
                  uint32    ch);


/********************************************************************
* FUNCTION ses_putastr
*
* write XML attribute safe content string
* Write a zero-terminated attribute content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
extern void
    ses_putastr (ses_cb_t *scb,
                 const xmlChar *str,
                 int32 indent);


/********************************************************************
* FUNCTION ses_putjstr
*
* write JSON safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL JSON CHARS ARE CONVERTED TO ESCAPED CHARS
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
extern void
    ses_putjstr (ses_cb_t *scb,
                 const xmlChar *str,
                 int32 indent);


/********************************************************************
* FUNCTION ses_indent
*
* Write the proper newline + indentation to the specified session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to start msg 
*   indent == number of chars to indent after a newline
*             will be ignored if indent is turned off
*             in the agent profile
*          == -1 means no newline or indent
*          == 0 means just newline
*
*********************************************************************/
extern void
    ses_indent (ses_cb_t *scb,
		int32 indent);


/********************************************************************
* FUNCTION ses_indent_count
*
* Get the indent count for this session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   indent value for the session
*********************************************************************/
extern int32
    ses_indent_count (const ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_set_indent
*
* Set the indent count for this session
*
* INPUTS:
*   scb == session control block to check
*   indent == value to use (may get adjusted)
*
*********************************************************************/
extern void
    ses_set_indent (ses_cb_t *scb,
		    int32 indent);


/********************************************************************
* FUNCTION ses_set_mode
*
* Set the output mode for the specified session
*
* INPUTS:
*   scb == session control block to set
*   mode == new mode value
* RETURNS:
*   none
*********************************************************************/
extern void
    ses_set_mode (ses_cb_t *scb,
		  ses_mode_t mode);


/********************************************************************
* FUNCTION ses_get_mode
*
* Get the output mode for the specified session
*
* INPUTS:
*   scb == session control block to get
*
* RETURNS:
*   session mode value
*********************************************************************/
extern ses_mode_t
    ses_get_mode (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_start_msg
*
* Start a new outbound message on the specified session
*
* INPUTS:
*   scb == session control block to start msg 
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ses_start_msg (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_finish_msg
*
* Finish an outbound message on the specified session
*
* INPUTS:
*   scb == session control block to finish msg 
* RETURNS:
*   none
*********************************************************************/
extern void
    ses_finish_msg (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_read_cb
*
* The IO input front-end for the xmlTextReader parser read fn
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.  For SSH, also need to detect the EOM flag
* and remove it + control input to the reader.
*
* Uses a complex state machine which does not assume that the
* input from the network is going to arrive in well-formed chunks.
* It has to be treated as a byte stream (SOCK_STREAM).
*
* Does not remove char entities or any XML, just the SSH EOM directive
*
* INPUTS:
*   context == scb pointer for the session to read
*   buffer == char buffer to fill
*   len == length of the buffer
*
* RETURNS:
*   number of bytes read into the buffer
*   -1     indicates error and EOF
*********************************************************************/
extern int
    ses_read_cb (void *context,
		 char *buffer,
		 int len);


/********************************************************************
* FUNCTION ses_accept_input
*
* The IO input handler for the ncxserver loop
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.  For SSH, also need to detect the EOM flag
* and remove it + control input to the reader.
*
* This function breaks the byte stream into ses_msg_t structs
* that get queued on the session's msgQ
*
* INPUTS:
*   scb == session control block to accept input for
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ses_accept_input (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_state_name
*
* Get the name of a session state from the enum value
*
* INPUTS:
*   state == session state enum value
*
* RETURNS:
*   staing corresponding to the state name
*********************************************************************/
extern const xmlChar *
    ses_state_name (ses_state_t state);


/********************************************************************
* FUNCTION ses_withdef
*
* Get the with-defaults value for this session
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   with-defaults value for the session
*********************************************************************/
extern ncx_withdefaults_t
    ses_withdef (const ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_line_left
*
* Get the number of bytes that can be added to the current line
* before the session linesize limit is reached
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   number of bytes left, or zero if limit already reached
*********************************************************************/
extern uint32
    ses_line_left (const ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_put_extern
* 
*  write the contents of a file to the session
*
* INPUTS:
*    scb == session to write
*    fspec == filespec to write
*
*********************************************************************/
extern void
    ses_put_extern (ses_cb_t *scb,
		    const xmlChar *fname);


/********************************************************************
* FUNCTION ses_get_total_stats
* 
*  Get a r/w pointer to the the session totals stats
*
* RETURNS:
*  pointer to the global session stats struct 
*********************************************************************/
extern ses_total_stats_t *
    ses_get_total_stats (void);


/********************************************************************
* FUNCTION ses_get_transport_name
* 
*  Get the name of the transport for a given enum value
*
* INPUTS:
*   transport == ses_transport_t enum value
*
* RETURNS:
*   pointer to the string value for the specified enum
*********************************************************************/
extern const xmlChar *
    ses_get_transport_name (ses_transport_t transport);


/********************************************************************
* FUNCTION ses_set_xml_nons
* 
*  force xmlns attributes to be skipped in XML mode
*
* INPUTS:
*    scb == session to set
*
*********************************************************************/
extern void
    ses_set_xml_nons (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_get_xml_nons
* 
*  force xmlns attributes to be skipped in XML mode
*
* INPUTS:
*    scb == session to get
*
* RETURNS:
*   TRUE if no xmlns attributes set
*   FALSE if OK to use xmlns attributes
*********************************************************************/
extern boolean
    ses_get_xml_nons (const ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_set_protocol
* 
*  set the NETCONF protocol version in use
*
* INPUTS:
*    scb == session to set
*    proto == protocol to set
* RETURNS:
*    status
*********************************************************************/
extern status_t
    ses_set_protocol (ses_cb_t *scb,
                      ncx_protocol_t proto);


/********************************************************************
* FUNCTION ses_get_protocol
* 
*  Get the NETCONF protocol set (or unset) for this session
*
* INPUTS:
*    scb == session to get
*
* RETURNS:
*   protocol enumeration in use
*********************************************************************/
extern ncx_protocol_t
    ses_get_protocol (const ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_set_protocols_requested
* 
*  set the NETCONF protocol versions requested
*
* INPUTS:
*    scb == session to set
*    proto == protocol to set
* RETURNS:
*    status
*********************************************************************/
extern void
    ses_set_protocols_requested (ses_cb_t *scb,
                                 ncx_protocol_t proto);


/********************************************************************
* FUNCTION ses_protocol_requested
* 
*  check if the NETCONF protocol version was requested
*
* INPUTS:
*    scb == session to check
*    proto == protocol to check
* RETURNS:
*    TRUE is requested; FALSE otherwise
*********************************************************************/
extern boolean
    ses_protocol_requested (ses_cb_t *scb,
                            ncx_protocol_t proto);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_ses */
