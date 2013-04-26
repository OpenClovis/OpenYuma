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
#ifndef _H_log
#define _H_log
/*  FILE: log.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Logging manager

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
08-jan-06    abb      begun
*/

#include <stdio.h>
#include <xmlstring.h>

#include "procdefs.h"
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* macros to check the debug level */
#define LOGERROR   (log_get_debug_level() >= LOG_DEBUG_ERROR)
#define LOGWARN    (log_get_debug_level() >= LOG_DEBUG_WARN)
#define LOGINFO    (log_get_debug_level() >= LOG_DEBUG_INFO)
#define LOGDEBUG   (log_get_debug_level() >= LOG_DEBUG_DEBUG)
#define LOGDEBUG2  (log_get_debug_level() >= LOG_DEBUG_DEBUG2)
#define LOGDEBUG3  (log_get_debug_level() >= LOG_DEBUG_DEBUG3)
#define LOGDEBUG4  (log_get_debug_level() >= LOG_DEBUG_DEBUG4)


#define LOG_DEBUG_STR_OFF     (const xmlChar *)"off"
#define LOG_DEBUG_STR_ERROR   (const xmlChar *)"error"
#define LOG_DEBUG_STR_WARN    (const xmlChar *)"warn"
#define LOG_DEBUG_STR_INFO    (const xmlChar *)"info"
#define LOG_DEBUG_STR_DEBUG   (const xmlChar *)"debug"
#define LOG_DEBUG_STR_DEBUG2  (const xmlChar *)"debug2"
#define LOG_DEBUG_STR_DEBUG3  (const xmlChar *)"debug3"
#define LOG_DEBUG_STR_DEBUG4  (const xmlChar *)"debug4"

/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/

/* The debug level enumerations used in util/log.c */
typedef enum log_debug_t_ {
    LOG_DEBUG_NONE,                 /* value not set or error */
    LOG_DEBUG_OFF,                      /* logging turned off */ 
    LOG_DEBUG_ERROR,          /* fatal + internal errors only */
    LOG_DEBUG_WARN,                  /* all errors + warnings */
    LOG_DEBUG_INFO,         /* all previous + user info trace */
    LOG_DEBUG_DEBUG,                        /* debug level 1 */
    LOG_DEBUG_DEBUG2,                       /* debug level 2 */
    LOG_DEBUG_DEBUG3,                       /* debug level 3 */
    LOG_DEBUG_DEBUG4                        /* debug level 3 */
}  log_debug_t;


/* logging function template to switch between
 * log_stdout and log_write
 */
typedef void (*logfn_t) (const char *fstr, ...);



/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION log_open
*
*   Open a logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for logfile
*   append == TRUE if the log should be appended
*          == FALSE if it should be rewriten
*   tstamps == TRUE if the datetime stamp should be generated
*             at log-open and log-close time
*          == FALSE if no open and close timestamps should be generated
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    log_open (const char *fname,
	      boolean append,
	      boolean tstamps);


/********************************************************************
* FUNCTION log_close
*
*   Close the logfile
*
* RETURNS:
*    none
*********************************************************************/
extern void
    log_close (void);


/********************************************************************
* FUNCTION log_audit_open
*
*   Open the audit logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the audit_logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for audit logfile
*   append == TRUE if the log should be appended
*          == FALSE if it should be rewriten
*   tstamps == TRUE if the datetime stamp should be generated
*             at log-open and log-close time
*          == FALSE if no open and close timestamps should be generated
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    log_audit_open (const char *fname,
                    boolean append,
                    boolean tstamps);


/********************************************************************
* FUNCTION log_audit_close
*
*   Close the audit_logfile
*
* RETURNS:
*    none
*********************************************************************/
extern void
    log_audit_close (void);


/********************************************************************
* FUNCTION log_audit_is_open
*
*   Check if the audit log is open
*
* RETURNS:
*   TRUE if audit log is open; FALSE if not
*********************************************************************/
extern boolean
    log_audit_is_open (void);


/********************************************************************
* FUNCTION log_alt_open
*
*   Open an alternate logfile for writing
*   DO NOT use this function to send log entries to STDOUT
*   Leave the logfile NULL instead.
*
* INPUTS:
*   fname == full filespec string for logfile
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    log_alt_open (const char *fname);


/********************************************************************
* FUNCTION log_alt_close
*
*   Close the alternate logfile
*
* RETURNS:
*    none
*********************************************************************/
extern void
    log_alt_close (void);


/********************************************************************
* FUNCTION log_stdout
*
*   Write lines of text to STDOUT, even if the logfile
*   is open, unless the debug mode is set to NONE
*   to indicate silent batch mode
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_stdout (const char *fstr, ...);


/********************************************************************
* FUNCTION log_write
*
*   Generate a log entry, regardless of log level
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_write (const char *fstr, ...);


/********************************************************************
* FUNCTION log_audit_write
*
*   Generate an audit log entry, regardless of log level
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_audit_write (const char *fstr, ...);


/********************************************************************
* FUNCTION log_alt_write
*
*   Write to the alternate log file
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for fprintf
*
*********************************************************************/
extern void 
    log_alt_write (const char *fstr, ...);


/********************************************************************
* FUNCTION log_alt_indent
* 
* Printf a newline to the alternate logfile,
* then the specified number of space chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
extern void
    log_alt_indent (int32 indentcnt);


/********************************************************************
* FUNCTION log_error
*
*   Generate a LOG_DEBUG_ERROR log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void log_error (const char *fstr, ...);

/********************************************************************
* FUNCTION log_error
*
*   Generate a LOG_DEBUG_ERROR log entry
*
* INPUTS:
*   fstr == format string in printf format
*   args == any additional arguments for printf
*
*********************************************************************/
extern void vlog_error (const char *fstr, va_list args );

/********************************************************************
* FUNCTION log_warn
*
*   Generate LOG_DEBUG_WARN log output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_warn (const char *fstr, ...);


/********************************************************************
* FUNCTION log_info
*
*   Generate a LOG_DEBUG_INFO log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_info (const char *fstr, ...);


/********************************************************************
* FUNCTION log_debug
*
*   Generate a LOG_DEBUG_DEBUG log entry
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_debug (const char *fstr, ...);


/********************************************************************
* FUNCTION log_debug2
*
*   Generate LOG_DEBUG_DEBUG2 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_debug2 (const char *fstr, ...);


/********************************************************************
* FUNCTION log_debug3
*
*   Generate LOG_DEBUG_DEBUG3 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_debug3 (const char *fstr, ...);


/********************************************************************
* FUNCTION log_debug4
*
*   Generate LOG_DEBUG_DEBUG4 log trace output
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_debug4 (const char *fstr, ...);


/********************************************************************
* FUNCTION log_noop
*
*  Do not generate any log message NO-OP
*  Used to set logfn_t to no-loggging option
*
* INPUTS:
*   fstr == format string in printf format
*   ... == any additional arguments for printf
*
*********************************************************************/
extern void 
    log_noop (const char *fstr, ...);


/********************************************************************
* FUNCTION log_set_debug_level
* 
* Set the global debug filter threshold level
* 
* INPUTS:
*   dlevel == desired debug level
*
*********************************************************************/
extern void
    log_set_debug_level (log_debug_t dlevel);


/********************************************************************
* FUNCTION log_get_debug_level
* 
* Get the global debug filter threshold level
* 
* RETURNS:
*   the global debug level
*********************************************************************/
extern log_debug_t
    log_get_debug_level (void);


/********************************************************************
* FUNCTION log_get_debug_level_enum
* 
* Get the corresponding debug enum for the specified string
* 
* INPUTS:
*   str == string value to convert
*
* RETURNS:
*   the corresponding enum for the specified debug level
*********************************************************************/
extern log_debug_t
    log_get_debug_level_enum (const char *str);


/********************************************************************
* FUNCTION log_get_debug_level_string
* 
* Get the corresponding string for the debug enum
* 
* INPUTS:
*   level ==  the enum for the specified debug level
*
* RETURNS:
*   the string value for this enum

*********************************************************************/
extern const xmlChar *
    log_get_debug_level_string (log_debug_t level);


/********************************************************************
* FUNCTION log_is_open
* 
* Check if the logfile is active
* 
* RETURNS:
*   TRUE if logfile open, FALSE otherwise
*********************************************************************/
extern boolean
    log_is_open (void);


/********************************************************************
* FUNCTION log_indent
* 
* Printf a newline, then the specified number of chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
extern void
    log_indent (int32 indentcnt);


/********************************************************************
* FUNCTION log_stdout_indent
* 
* Printf a newline to stdout, then the specified number of chars
*
* INPUTS:
*    indentcnt == number of indent chars, -1 == skip everything
*
*********************************************************************/
extern void
    log_stdout_indent (int32 indentcnt);


/********************************************************************
* FUNCTION log_get_logfile
* 
* Get the open logfile for direct output
* Needed by libtecla to write command line history
*
* RETURNS:
*   pointer to open FILE if any
*   NULL if no open logfile
*********************************************************************/
extern FILE *
    log_get_logfile (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_log */
