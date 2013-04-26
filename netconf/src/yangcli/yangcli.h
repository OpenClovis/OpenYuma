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
#ifndef _H_yangcli
#define _H_yangcli

/*  FILE: yangcli.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
27-mar-09    abb      Begun; moved from yangcli.c

*/

#include <time.h>
//#include <sys/time.h>
#include <xmlstring.h>
#include "libtecla.h"

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_mgr_io
#include "mgr_io.h"
#endif

#ifndef _H_mgr_rpc
#include "mgr_rpc.h"
#endif

#ifndef _H_runstack
#include "runstack.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/
#define PROGNAME            "yangcli"

#define MAX_PROMPT_LEN 56

#define YANGCLI_MAX_NEST  16

#define YANGCLI_MAX_RUNPARMS 9

#define YANGCLI_LINELEN   4095

/* 8K CLI buffer per server session */
#define YANGCLI_BUFFLEN  8192

#define YANGCLI_HISTLEN  4095

#define YANGCLI_MOD  (const xmlChar *)"yangcli"

#define YANGCLI_NUM_TIMERS    16

/* look in yangcli.c:yangcli_init for defaults not listed here */
#define YANGCLI_DEF_HISTORY_FILE  (const xmlChar *)"~/.yuma/.yangcli_history"

#define YANGCLI_DEF_ALIASES_FILE  (const xmlChar *)"~/.yuma/.yangcli_aliases"

#define YANGCLI_DEF_USERVARS_FILE \
    (const xmlChar *)"~/.yuma/yangcli_uservars.xml"

#define YANGCLI_DEF_TIMEOUT   30

#define YANGCLI_DEF_SERVER (const xmlChar *)"default"

#define YANGCLI_DEF_DISPLAY_MODE   NCX_DISPLAY_MODE_PLAIN

#define YANGCLI_DEF_FIXORDER   TRUE

#define YANGCLI_DEF_CONF_FILE (const xmlChar *)"/etc/yuma/yangcli.conf"

#define YANGCLI_DEF_SERVER (const xmlChar *)"default"

#define YANGCLI_DEF_TEST_OPTION OP_TESTOP_SET

#define YANGCLI_DEF_ERROR_OPTION OP_ERROP_NONE

#define YANGCLI_DEF_DEFAULT_OPERATION OP_DEFOP_MERGE

#define YANGCLI_DEF_WITH_DEFAULTS  NCX_WITHDEF_NONE

#define YANGCLI_DEF_INDENT    2

#define YANGCLI_DEF_BAD_DATA NCX_BAD_DATA_CHECK

#define YANGCLI_DEF_MAXLOOPS  65535

#define YANGCLI_DEF_HISTORY_LINES  25

#define YANGCLI_RECALL_CHAR  '!'

#ifdef MACOSX
#define ENV_HOST        (const char *)"HOST"
#else
#define ENV_HOST        (const char *)"HOSTNAME"
#endif

#define ENV_SHELL       (const char *)"SHELL"
#define ENV_USER        (const char *)"USER"
#define ENV_LANG        (const char *)"LANG"

/* CLI parmset for the ncxcli application */
#define YANGCLI_BOOT YANGCLI_MOD

/* core modules auto-loaded at startup */
#define NCXDTMOD       (const xmlChar *)"yuma-types"
#define XSDMOD         (const xmlChar *)"yuma-xsd"

#define DEF_PROMPT     (const xmlChar *)"yangcli> "
#define DEF_FALSE_PROMPT  (const xmlChar *)"yangcli[F]> "
#define DEF_FN_PROMPT  (const xmlChar *)"yangcli:"
#define MORE_PROMPT    (const xmlChar *)"   more> "
#define FALSE_PROMPT   (const xmlChar *)"[F]"

#define YESNO_NODEF  0
#define YESNO_CANCEL 0
#define YESNO_YES    1
#define YESNO_NO     2

/* YANGCLI boot and operation parameter names 
 * matches parm clauses in yangcli container in yangcli.yang
 */

#define YANGCLI_ALIASES_FILE  (const xmlChar *)"aliases-file"
#define YANGCLI_ALT_NAMES   (const xmlChar *)"alt-names"
#define YANGCLI_AUTOALIASES (const xmlChar *)"autoaliases"
#define YANGCLI_AUTOCOMP    (const xmlChar *)"autocomp"
#define YANGCLI_AUTOHISTORY (const xmlChar *)"autohistory"
#define YANGCLI_AUTOLOAD    (const xmlChar *)"autoload"
#define YANGCLI_AUTOUSERVARS (const xmlChar *)"autouservars"
#define YANGCLI_BADDATA     (const xmlChar *)"bad-data"
#define YANGCLI_BATCHMODE   (const xmlChar *)"batch-mode"
#define YANGCLI_BRIEF       (const xmlChar *)"brief"
#define YANGCLI_CLEANUP     (const xmlChar *)"cleanup"
#define YANGCLI_CLEAR       (const xmlChar *)"clear"
#define YANGCLI_CLI         (const xmlChar *)"cli"
#define YANGCLI_COMMAND     (const xmlChar *)"command"
#define YANGCLI_COMMANDS    (const xmlChar *)"commands"
#define YANGCLI_CONFIG      (const xmlChar *)"config"
#define YANGCLI_DEF_MODULE  (const xmlChar *)"default-module"
#define YANGCLI_DELTA       (const xmlChar *)"delta"
#define YANGCLI_DIR         (const xmlChar *)"dir"
#define YANGCLI_DISPLAY_MODE  (const xmlChar *)"display-mode"
#define YANGCLI_ECHO        (const xmlChar *)"echo"
#define YANGCLI_ECHO_REPLIES (const xmlChar *)"echo-replies"
#define YANGCLI_EDIT_TARGET (const xmlChar *)"edit-target"
#define YANGCLI_ERROR_OPTION (const xmlChar *)"error-option"
#define YANGCLI_FILES       (const xmlChar *)"files"
#define YANGCLI_FIXORDER    (const xmlChar *)"fixorder"
#define YANGCLI_FROM_CLI    (const xmlChar *)"from-cli"
#define YANGCLI_FORCE_TARGET (const xmlChar *)"force-target"
#define YANGCLI_FULL        (const xmlChar *)"full"
#define YANGCLI_GLOBAL      (const xmlChar *)"global"
#define YANGCLI_GLOBALS     (const xmlChar *)"globals"
#define YANGCLI_ID          (const xmlChar *)"id"
#define YANGCLI_INDEX       (const xmlChar *)"index"
#define YANGCLI_LOAD        (const xmlChar *)"load"
#define YANGCLI_LOCK_TIMEOUT (const xmlChar *)"lock-timeout"
#define YANGCLI_LOCAL       (const xmlChar *)"local"
#define YANGCLI_LOCALS      (const xmlChar *)"locals"
#define YANGCLI_MATCH_NAMES (const xmlChar *)"match-names"
#define YANGCLI_MODULE      (const xmlChar *)"module"
#define YANGCLI_MODULES     (const xmlChar *)"modules"
#define YANGCLI_NCPORT      (const xmlChar *)"ncport"
#define YANGCLI_NOFILL      (const xmlChar *)"nofill"
#define YANGCLI_OBJECTS     (const xmlChar *)"objects"
#define YANGCLI_OIDS        (const xmlChar *)"oids"
#define YANGCLI_OPERATION   (const xmlChar *)"operation"
#define YANGCLI_OPTIONAL    (const xmlChar *)"optional"
#define YANGCLI_ORDER       (const xmlChar *)"order"
#define YANGCLI_PASSWORD    (const xmlChar *)"password"
#define YANGCLI_PROTOCOLS   (const xmlChar *)"protocols"
#define YANGCLI_PRIVATE_KEY (const xmlChar *)"private-key"
#define YANGCLI_PUBLIC_KEY  (const xmlChar *)"public-key"
#define YANGCLI_RESTART_OK  (const xmlChar *)"restart-ok"
#define YANGCLI_RETRY_INTERVAL (const xmlChar *)"retry-interval"
#define YANGCLI_RUN_COMMAND (const xmlChar *)"run-command"
#define YANGCLI_RUN_SCRIPT  (const xmlChar *)"run-script"
#define YANGCLI_START       (const xmlChar *)"start"
#define YANGCLI_SCRIPTS     (const xmlChar *)"scripts"
#define YANGCLI_SERVER      (const xmlChar *)"server"
#define YANGCLI_SESSION     (const xmlChar *)"session"
#define YANGCLI_SYSTEM      (const xmlChar *)"system"
#define YANGCLI_TEST_OPTION (const xmlChar *)"test-option"
#define YANGCLI_TIMEOUT     (const xmlChar *)"timeout"
#define YANGCLI_TIME_RPCS   (const xmlChar *)"time-rpcs"
#define YANGCLI_TRANSPORT   (const xmlChar *)"transport"
#define YANGCLI_USE_XMLHEADER (const xmlChar *)"use-xmlheader"
#define YANGCLI_USER        (const xmlChar *)"user"
#define YANGCLI_USERVARS_FILE  (const xmlChar *)"uservars-file"
#define YANGCLI_VALUE       (const xmlChar *)"value"
#define YANGCLI_VAR         (const xmlChar *)"var"
#define YANGCLI_VARREF      (const xmlChar *)"varref"
#define YANGCLI_VARS        (const xmlChar *)"vars"
#define YANGCLI_VARTYPE     (const xmlChar *)"vartype"
#define YANGCLI_WITH_DEFAULTS  (const xmlChar *)"with-defaults"

/* YANGCLI local RPC commands */
#define YANGCLI_ALIAS   (const xmlChar *)"alias"
#define YANGCLI_ALIASES (const xmlChar *)"aliases"
#define YANGCLI_CD      (const xmlChar *)"cd"
#define YANGCLI_CONNECT (const xmlChar *)"connect"
#define YANGCLI_CREATE  (const xmlChar *)"create"
#define YANGCLI_DELETE  (const xmlChar *)"delete"
#define YANGCLI_ELSE    (const xmlChar *)"else"
#define YANGCLI_ELIF    (const xmlChar *)"elif"
#define YANGCLI_END     (const xmlChar *)"end"
#define YANGCLI_EVAL    (const xmlChar *)"eval"
#define YANGCLI_EVENTLOG (const xmlChar *)"eventlog"
#define YANGCLI_FILL    (const xmlChar *)"fill"
#define YANGCLI_GET_LOCKS (const xmlChar *)"get-locks"
#define YANGCLI_HELP    (const xmlChar *)"help"
#define YANGCLI_HISTORY (const xmlChar *)"history"
#define YANGCLI_INSERT  (const xmlChar *)"insert"
#define YANGCLI_IF      (const xmlChar *)"if"
#define YANGCLI_LIST    (const xmlChar *)"list"
#define YANGCLI_LOG_ERROR (const xmlChar *)"log-error"
#define YANGCLI_LOG_WARN (const xmlChar *)"log-warn"
#define YANGCLI_LOG_INFO (const xmlChar *)"log-info"
#define YANGCLI_LOG_DEBUG (const xmlChar *)"log-debug"
#define YANGCLI_MERGE   (const xmlChar *)"merge"
#define YANGCLI_MGRLOAD (const xmlChar *)"mgrload"
#define YANGCLI_PWD     (const xmlChar *)"pwd"
#define YANGCLI_QUIT    (const xmlChar *)"quit"
#define YANGCLI_RECALL  (const xmlChar *)"recall"
#define YANGCLI_RELEASE_LOCKS (const xmlChar *)"release-locks"
#define YANGCLI_REMOVE  (const xmlChar *)"remove"
#define YANGCLI_REPLACE (const xmlChar *)"replace"
#define YANGCLI_RUN     (const xmlChar *)"run"
#define YANGCLI_SAVE    (const xmlChar *)"save"
#define YANGCLI_SET     (const xmlChar *)"set"
#define YANGCLI_SGET    (const xmlChar *)"sget"
#define YANGCLI_SGET_CONFIG   (const xmlChar *)"sget-config"
#define YANGCLI_SHOW    (const xmlChar *)"show"
#define YANGCLI_START_TIMER  (const xmlChar *)"start-timer"
#define YANGCLI_STOP_TIMER  (const xmlChar *)"stop-timer"
#define YANGCLI_WHILE   (const xmlChar *)"while"
#define YANGCLI_XGET    (const xmlChar *)"xget"
#define YANGCLI_XGET_CONFIG   (const xmlChar *)"xget-config"
#define YANGCLI_UNSET   (const xmlChar *)"unset"
#define YANGCLI_USERVARS  (const xmlChar *)"uservars"



/* specialized prompts for the fill command */
#define YANGCLI_PR_LLIST (const xmlChar *)"Add another leaf-list?"
#define YANGCLI_PR_LIST (const xmlChar *)"Add another list?"

/* retry for a lock request once per second */
#define YANGCLI_RETRY_INTERNVAL   1

#define YANGCLI_EXPR    (const xmlChar *)"expr"
#define YANGCLI_DOCROOT (const xmlChar *)"docroot"
#define YANGCLI_CONTEXT (const xmlChar *)"context"

#define YANGCLI_MSG      (const xmlChar *)"msg"
#define YANGCLI_MAXLOOPS (const xmlChar *)"maxloops"

#define YANGCLI_MAX_INDENT    9

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* cache the module pointers known by a particular server,
 * as reported in the session <hello> message
 */
typedef struct modptr_t_ {
    dlq_hdr_t            qhdr;
    ncx_module_t         *mod;               /* back-ptr, not live */
    ncx_list_t           *feature_list;      /* back-ptr, not live */
    ncx_list_t           *deviation_list;    /* back-ptr, not live */
} modptr_t;


/* save the requested result format type */
typedef enum result_format_t {
    RF_NONE,
    RF_TEXT,
    RF_XML,
    RF_JSON
} result_format_t;


/* command state enumerations for each situation
 * where the tecla get_line function is called
 */
typedef enum command_state_t {
    CMD_STATE_NONE,
    CMD_STATE_FULL,
    CMD_STATE_GETVAL,
    CMD_STATE_YESNO,
    CMD_STATE_MORE
} command_state_t;


/* command mode enumerations */
typedef enum command_mode_t {
    CMD_MODE_NONE,
    CMD_MODE_NORMAL,
    CMD_MODE_AUTOLOAD,
    CMD_MODE_AUTOLOCK,
    CMD_MODE_AUTOUNLOCK,
    CMD_MODE_AUTODISCARD,
    CMD_MODE_SAVE
} command_mode_t;


/* saved state for libtecla command line completion */
typedef struct completion_state_t_ {
    obj_template_t        *cmdobj;
    obj_template_t        *cmdinput;
    obj_template_t        *cmdcurparm;
    struct server_cb_t_   *server_cb;
    ncx_module_t          *cmdmodule;
    command_state_t        cmdstate;
    boolean                assignstmt;
} completion_state_t;


/* save the server lock state for get-locks and release-locks */
typedef enum lock_state_t {
    LOCK_STATE_NONE,
    LOCK_STATE_IDLE,
    LOCK_STATE_REQUEST_SENT,
    LOCK_STATE_TEMP_ERROR,
    LOCK_STATE_FATAL_ERROR,
    LOCK_STATE_ACTIVE,
    LOCK_STATE_RELEASE_SENT,
    LOCK_STATE_RELEASED
} lock_state_t;


/* autolock control block, used by get-locks, release-locks */
typedef struct lock_cb_t_ {
    ncx_cfg_t             config_id;
    const xmlChar        *config_name;
    lock_state_t          lock_state;
    boolean               lock_used;
    time_t                start_time;
    time_t                last_msg_time;
} lock_cb_t;


/* auto-get-schema control block for a module */
typedef struct autoload_modcb_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *module;
    xmlChar            *source;
    xmlChar            *revision;
    const ncx_list_t   *features;
    const ncx_list_t   *deviations;
    status_t            res;
    boolean             retrieved;
} autoload_modcb_t;


/* auto-get-schema control block for a deviation */
typedef struct autoload_devcb_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *module;
    xmlChar            *source;
    status_t            res;
    boolean             retrieved;
} autoload_devcb_t;


/* yangcli command alias control block */
typedef struct alias_cb_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *name;
    xmlChar            *value;
    uint8               quotes;
}  alias_cb_t;


/* NETCONF server control block */
typedef struct server_cb_t_ {
    dlq_hdr_t            qhdr;
    xmlChar             *name;
    xmlChar             *address;
    xmlChar             *password;
    xmlChar             *publickey;
    xmlChar             *privatekey;
    const xmlChar       *default_target;
    val_value_t         *connect_valset; 

    /* assignment statement support */
    xmlChar             *result_name;
    var_type_t           result_vartype;
    xmlChar             *result_filename;
    result_format_t      result_format;
    val_value_t         *local_result; 

    /* per-server shadows of global config vars */
    boolean              get_optional;
    ncx_display_mode_t   display_mode;
    uint32               timeout;
    uint32               lock_timeout;
    ncx_bad_data_t       baddata;
    log_debug_t          log_level;
    boolean              autoload;
    boolean              fixorder;
    op_testop_t          testoption;
    op_errop_t           erroption;
    op_defop_t           defop;
    ncx_withdefaults_t   withdefaults;
    int32                defindent;
    boolean              echo_replies;
    boolean              time_rpcs;
    ncx_name_match_t     match_names;
    boolean              alt_names;
    boolean              overwrite_filevars;
    boolean              use_xmlheader;

    /* session support */
    mgr_io_state_t       state;
    ses_id_t             mysid;
    mgr_io_returncode_t  returncode;
    int32                errnocode;
    command_mode_t       command_mode;

    /* get-locks and release-locks support
     * there is one entry for each database
     * indexed by the ncx_cfg_t enum
     */
    boolean              locks_active;
    boolean              locks_waiting;
    ncx_cfg_t            locks_cur_cfg;
    uint32               locks_timeout;
    uint32               locks_retry_interval;
    boolean              locks_cleanup;
    time_t               locks_start_time;
    lock_cb_t            lock_cb[NCX_NUM_CFGS];

    /* TBD: session-specific user variables */
    dlq_hdr_t            varbindQ;   /* Q of ncx_var_t */

    /* before any server modules are loaded, all the
     * modules are checked out, and the results are stored in
     * this Q of ncxmod_search_result_t 
     */
    dlq_hdr_t            searchresultQ; /* Q of ncxmod_search_result_t */
    ncxmod_search_result_t  *cursearchresult;

    /* contains only the modules that the server is using
     * plus the 'netconf.yang' module
     */
    dlq_hdr_t            modptrQ;     /* Q of modptr_t */

    /* contains received notifications */
    dlq_hdr_t            notificationQ;   /* Q of mgr_not_msg_t */

    /* support fot auto-get-schema feature */
    dlq_hdr_t            autoload_modcbQ;  /* Q of autoload_modcb_t */
    dlq_hdr_t            autoload_devcbQ;  /* Q of autoload_devcb_t */
    autoload_modcb_t    *autoload_curmod;
    autoload_devcb_t    *autoload_curdev;

    /* support for temp directory for downloaded modules */
    ncxmod_temp_progcb_t *temp_progcb;
    ncxmod_temp_sescb_t  *temp_sescb;

    /* runstack context for script processing */
    runstack_context_t  *runstack_context;

    /* per session timer support */
    struct timeval       timers[YANGCLI_NUM_TIMERS];

    /* per-session CLI support */
    const xmlChar       *cli_fn;
    GetLine             *cli_gl;
    xmlChar             *history_filename;
    xmlChar             *history_line;
    boolean              history_line_active;
    boolean              history_auto;
    uint32               history_size;
    completion_state_t   completion_state;
    boolean              climore;
    xmlChar              clibuff[YANGCLI_BUFFLEN];
} server_cb_t;


/********************************************************************
* FUNCTION get_autocomp
* 
*  Get the autocomp parameter value
* 
* RETURNS:
*    autocomp boolean value
*********************************************************************/
extern boolean
    get_autocomp (void);


/********************************************************************
* FUNCTION get_autoload
* 
*  Get the autoload parameter value
* 
* RETURNS:
*    autoload boolean value
*********************************************************************/
extern boolean
    get_autoload (void);


/********************************************************************
* FUNCTION get_batchmode
* 
*  Get the batchmode parameter value
* 
* RETURNS:
*    batchmode boolean value
*********************************************************************/
extern boolean
    get_batchmode (void);


/********************************************************************
* FUNCTION get_default_module
* 
*  Get the default module
* 
* RETURNS:
*    default module value
*********************************************************************/
extern const xmlChar *
    get_default_module (void);


/********************************************************************
* FUNCTION get_runscript
* 
*  Get the runscript variable
* 
* RETURNS:
*    runscript value
*********************************************************************/
extern const xmlChar *
    get_runscript (void);


/********************************************************************
* FUNCTION get_baddata
* 
*  Get the baddata parameter
* 
* RETURNS:
*    baddata enum value
*********************************************************************/
extern ncx_bad_data_t
    get_baddata (void);



/********************************************************************
* FUNCTION get_yangcli_mod
* 
*  Get the yangcli module
* 
* RETURNS:
*    yangcli module
*********************************************************************/
extern ncx_module_t *
    get_yangcli_mod (void);


/********************************************************************
* FUNCTION get_mgr_cli_valset
* 
*  Get the CLI value set
* 
* RETURNS:
*    mgr_cli_valset variable
*********************************************************************/
extern val_value_t *
    get_mgr_cli_valset (void);


/********************************************************************
* FUNCTION get_connect_valset
* 
*  Get the connect value set
* 
* RETURNS:
*    connect_valset variable
*********************************************************************/
extern val_value_t *
    get_connect_valset (void);


/********************************************************************
* FUNCTION get_aliases_file
* 
*  Get the aliases-file value
* 
* RETURNS:
*    aliases_file variable
*********************************************************************/
extern const xmlChar *
    get_aliases_file (void);


/********************************************************************
* FUNCTION get_uservars_file
* 
*  Get the uservars-file value
* 
* RETURNS:
*    aliases_file variable
*********************************************************************/
extern const xmlChar *
    get_uservars_file (void);


/********************************************************************
* FUNCTION replace_connect_valset
* 
*  Replace the current connect value set with a clone
* of the specified connect valset
* 
* INPUTS:
*    valset == value node to clone that matches the object type
*              of the input section of the connect operation
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    replace_connect_valset (const val_value_t *valset);


/********************************************************************
* FUNCTION get_mgrloadQ
* 
*  Get the mgrloadQ value pointer
* 
* RETURNS:
*    mgrloadQ variable
*********************************************************************/
extern dlq_hdr_t *
    get_mgrloadQ (void);


/********************************************************************
* FUNCTION get_aliasQ
* 
*  Get the aliasQ value pointer
* 
* RETURNS:
*    aliasQ variable
*********************************************************************/
extern dlq_hdr_t *
    get_aliasQ (void);


/********************************************************************
 * FUNCTION yangcli_reply_handler
 * 
 *  handle incoming <rpc-reply> messages
 * 
 * INPUTS:
 *   scb == session receiving RPC reply
 *   req == original request returned for freeing or reusing
 *   rpy == reply received from the server (for checking then freeing)
 *
 * RETURNS:
 *   none
 *********************************************************************/
extern void
    yangcli_reply_handler (ses_cb_t *scb,
			   mgr_rpc_req_t *req,
			   mgr_rpc_rpy_t *rpy);


/********************************************************************
 * FUNCTION finish_result_assign
 * 
 * finish the assignment to result_name or result_filename
 * use 1 of these 2 parms:
 *    resultval == result to output to file
 *    resultstr == result to output as string
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    finish_result_assign (server_cb_t *server_cb,
			  val_value_t *resultvar,
			  const xmlChar *resultstr);


/********************************************************************
* FUNCTION report_capabilities
* 
* Generate a start session report, listing the capabilities
* of the NETCONF server
* 
* INPUTS:
*  server_cb == server control block to use
*  scb == session control block
*  isfirst == TRUE if first call when session established
*             FALSE if this is from show session command
*  mode == help mode; ignored unless first == FALSE
*********************************************************************/
extern void
    report_capabilities (server_cb_t *server_cb,
                         const ses_cb_t *scb,
                         boolean isfirst,
                         help_mode_t mode);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli */
