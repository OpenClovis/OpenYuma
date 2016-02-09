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
/*  FILE: yangcli.c

   NETCONF YANG-based CLI Tool

   See ./README for details

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
01-jun-08    abb      begun; started from ncxcli.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

/* #define MEMORY_DEBUG 1 */

#ifdef MEMORY_DEBUG
#include <mcheck.h>
#endif

#include "libtecla.h"

#define _C_main 1

#include "procdefs.h"
#include "cli.h"
#include "conf.h"
#include "help.h"
#include "json_wr.h"
#include "log.h"
#include "ncxmod.h"
#include "mgr.h"
#include "mgr_hello.h"
#include "mgr_io.h"
#include "mgr_not.h"
#include "mgr_rpc.h"
#include "mgr_ses.h"
#include "ncx.h"
#include "ncx_list.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "op.h"
#include "rpc.h"
#include "runstack.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "var.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "yangconst.h"
#include "yangcli.h"
#include "yangcli_cmd.h"
#include "yangcli_alias.h"
#include "yangcli_autoload.h"
#include "yangcli_autolock.h"
#include "yangcli_save.h"
#include "yangcli_tab.h"
#include "yangcli_uservars.h"
#include "yangcli_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#ifdef DEBUG
#define YANGCLI_DEBUG   1
#endif


/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*             F O R W A R D   D E C L A R A T I O N S               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/*****************  I N T E R N A L   V A R S  ****************/

/* TBD: use multiple server control blocks, stored in this Q */
static dlq_hdr_t      server_cbQ;

/* hack for now instead of lookup functions to get correct
 * server processing context; later search by session ID
 */
static server_cb_t    *cur_server_cb;

/* yangcli.yang file used for quicker lookups */
static ncx_module_t  *yangcli_mod;

/* netconf.yang file used for quicker lookups */
static ncx_module_t  *netconf_mod;

/* need to save CLI parameters: other vars are back-pointers */
static val_value_t   *mgr_cli_valset;

/* true if running a script from the invocation and exiting */
static boolean         batchmode;

/* true if printing program help and exiting */
static boolean         helpmode;
static help_mode_t     helpsubmode;

/* true if printing program version and exiting */
static boolean         versionmode;

/* name of script passed at invocation to auto-run */
static xmlChar        *runscript;

/* TRUE if runscript has been completed */
static boolean         runscriptdone;

/* command string passed at invocation to auto-run */
static xmlChar        *runcommand;

/* TRUE if runscript has been completed */
static boolean         runcommanddone;

/* controls automaic command line history buffer load/save */
static boolean autohistory;

/* Q of modtrs that have been loaded with 'mgrload' */
static dlq_hdr_t       mgrloadQ;

/* temporary file control block for the program instance */
static ncxmod_temp_progcb_t  *temp_progcb;

/* Q of ncxmod_search_result_t structs representing all modules
 * and submodules found in the module library path at boot-time
 */
static dlq_hdr_t      modlibQ;

/* Q of alias_cb_t structs representing all command aliases */
static dlq_hdr_t      aliasQ;

/* flag to indicate init never completed OK; used during cleanup */
static boolean       init_done;

/*****************  C O N F I G   V A R S  ****************/

/* TRUE if OK to load aliases automatically
 * FALSE if --autoaliases=false set by user
 * when yangcli starts, this var controls
 * whether the ~/.yuma/.yangcli_aliases file will be loaded
 * into this application automatically
 */
static boolean         autoaliases;

/* set if the --aliases-file parameter is present */
static const xmlChar *aliases_file;

/* TRUE if OK to load modules automatically
 * FALSE if --autoload=false set by user
 * when server connection is made, and module discovery is done
 * then this var controls whether the matching modules
 * will be loaded into this application automatically
 */
static boolean         autoload;

/* TRUE if OK to check for partial command names and parameter
 * names by the user.  First match (TBD: longest match!!)
 * will be used if no exact match found
 * FALSE if only exact match should be used
 */
static boolean         autocomp;

/* TRUE if OK to load user vars automatically
 * FALSE if --autouservars=false set by user
 * when yangcli starts, this var controls
 * whether the ~/.yuma/.yangcli_uservars file will be loaded
 * into this application automatically
 */
static boolean         autouservars;

/* set if the --uservars=filespec parameter is set */
static const xmlChar  *uservars_file;

/* NCX_BAD_DATA_IGNORE to silently accept invalid input values
 * NCX_BAD_DATA_WARN to warn and accept invalid input values
 * NCX_BAD_DATA_CHECK to prompt user to keep or re-enter value
 * NCX_BAD_DATA_ERROR to prompt user to re-enter value
 */
static ncx_bad_data_t  baddata;

/* global connect param set, copied to server connect parmsets */
static val_value_t   *connect_valset;

/* name of external CLI config file used on invocation */
static xmlChar        *confname;

/* the module to check first when no prefix is given and there
 * is no parent node to check;
 * usually set to module 'netconf'
 */
static xmlChar        *default_module;  

/* 0 for no timeout; N for N seconds message timeout */
static uint32          default_timeout;

/* default value for val_dump_value display mode */
static ncx_display_mode_t   display_mode;

/* FALSE to send PDUs in manager-specified order
 * TRUE to always send in correct canonical order
 */
static boolean         fixorder;

/* FALSE to skip optional nodes in do_fill
 * TRUE to check optional nodes in do_fill
 */
static boolean         optional;

/* default NETCONF test-option value */
static op_testop_t     testoption;

/* default NETCONF error-option value */
static op_errop_t      erroption;

/* default NETCONF default-operation value */
static op_defop_t      defop;

/* default NETCONF with-defaults value */
static ncx_withdefaults_t  withdefaults;

/* default indent amount */
static int32            defindent;

/* default echo-replies */
static boolean echo_replies;

/* default time-rpcs */
static boolean time_rpcs;

/* default match-names */
static ncx_name_match_t match_names;

/* default alt-names */
static boolean alt_names;

/* default force-target */
static const xmlChar *force_target;

/* default use-xmlheader */
static boolean use_xmlheader;


/********************************************************************
* FUNCTION get_line_timeout
* 
*  Callback function for libtecla when the inactivity
*  timeout occurs.  
*
* This function checks to see:
*   1) if the session is still active
*   2) if any notifications are pending
*
* INPUTS:
*    gl == line in progress
*    data == server control block passed as cookie
* 
* OUTPUTS:
*    prints/logs notifications pending
*    may generate log output and/or change session state
*
* RETURNS:
*    if session state changed (session lost)
*    then GLTO_ABORT will be returned
*
*    if any text written to STDOUT, then GLTO_REFRESH 
*    will be returned
*
*    if nothing done, then GLTO_CONTINUE will be returned
*********************************************************************/
static GlAfterTimeout
    get_line_timeout (GetLine *gl, 
                      void *data)
{
    server_cb_t  *server_cb;
    ses_cb_t    *scb;
    boolean      retval, wantdata, anystdout;

    (void)gl;
    server_cb = (server_cb_t *)data;
    server_cb->returncode = MGR_IO_RC_NONE;

    if (server_cb->state != MGR_IO_ST_CONN_IDLE) {
        server_cb->returncode = MGR_IO_RC_IDLE;
        return GLTO_CONTINUE;
    }

    scb = mgr_ses_get_scb(server_cb->mysid);
    if (scb == NULL) {
        /* session was dropped */
        server_cb->returncode = MGR_IO_RC_DROPPED;
        server_cb->state = MGR_IO_ST_IDLE;
        return GLTO_ABORT;
    }

    wantdata = FALSE;
    anystdout = FALSE;
    retval = mgr_io_process_timeout(scb->sid, &wantdata, &anystdout);
    if (retval) {
        /* this session is probably still alive */
        if (wantdata) {
            server_cb->returncode = MGR_IO_RC_WANTDATA;
        } else {
            server_cb->returncode = MGR_IO_RC_PROCESSED;
        }
        return (anystdout) ? GLTO_REFRESH : GLTO_CONTINUE;
    } else {
        /* this session was dropped just now */
        server_cb->returncode = MGR_IO_RC_DROPPED_NOW;
        server_cb->state = MGR_IO_ST_IDLE;
        return GLTO_ABORT;
    }

} /* get_line_timeout */


/********************************************************************
* FUNCTION do_startup_screen
* 
*  Print the startup messages to the log and stdout output
* 
*********************************************************************/
static void
    do_startup_screen (void)
{
    logfn_t             logfn;
    boolean             imode;
    status_t            res;
    xmlChar             versionbuffer[NCX_VERSION_BUFFSIZE];

    imode = interactive_mode();
    if (imode) {
        logfn = log_stdout;
    } else {
        logfn = log_write;
    }

    res = ncx_get_version(versionbuffer, NCX_VERSION_BUFFSIZE);
    if (res == NO_ERR) {
        (*logfn)("\n  yangcli version %s",  versionbuffer);
        if (LOGINFO) {
            (*logfn)("\n  ");
            mgr_print_libssh2_version(!imode);
        }
    } else {
        SET_ERROR(res);
    }

    (*logfn)("\n\n  ");
    (*logfn)(COPYRIGHT_STRING_LINE0);
    (*logfn)("  ");
    (*logfn)(COPYRIGHT_STRING_LINE1);
    (*logfn)("  ");
    (*logfn)(COPYRIGHT_STRING_LINE2);

    if (!imode) {
        return;
    }

    (*logfn)("\n  Type 'help' or 'help <command-name>' to get started");
    (*logfn)("\n  Use the <tab> key for command and value completion");
    (*logfn)("\n  Use the <enter> key to accept the default value ");
    (*logfn)("in brackets");

    (*logfn)("\n\n  These escape sequences are available ");
    (*logfn)("when filling parameter values:");
    (*logfn)("\n\n\t?\thelp");
    (*logfn)("\n\t??\tfull help");
    (*logfn)("\n\t?s\tskip current parameter");
    (*logfn)("\n\t?c\tcancel current command");

    (*logfn)("\n\n  These assignment statements are available ");
    (*logfn)("when entering commands:");
    (*logfn)("\n\n\t$<varname> = <expr>\tLocal user variable assignment");
    (*logfn)("\n\t$$<varname> = <expr>\tGlobal user variable assignment");
    (*logfn)("\n\t@<filespec> = <expr>\tFile assignment\n");


}  /* do_startup_screen */


/********************************************************************
* FUNCTION free_server_cb
* 
*  Clean and free an server control block
* 
* INPUTS:
*    server_cb == control block to free
*                MUST BE REMOVED FROM ANY Q FIRST
*
*********************************************************************/
static void
    free_server_cb (server_cb_t *server_cb)
{

    modptr_t                *modptr;
    mgr_not_msg_t           *notif;
    int                      retval;

    /* save the history buffer if needed */
    if (server_cb->cli_gl != NULL && server_cb->history_auto) {
        retval = 
            gl_save_history(server_cb->cli_gl,
                            (const char *)server_cb->history_filename,
                            "#",   /* comment prefix */
                            -1);    /* save all entries */
        if (retval) {
            log_error("\nError: could not save command line "
                      "history file '%s'",
                      server_cb->history_filename);
        }
    }

    if (server_cb->name) {
        m__free(server_cb->name);
    }
    if (server_cb->address) {
        m__free(server_cb->address);
    }
    if (server_cb->password) {
        m__free(server_cb->password);
    }
    if (server_cb->local_result) {
        val_free_value(server_cb->local_result);
    }
    if (server_cb->result_name) {
        m__free(server_cb->result_name);
    }
    if (server_cb->result_filename) {
        m__free(server_cb->result_filename);
    }
    if (server_cb->history_filename) {
        m__free(server_cb->history_filename);
    }
    if (server_cb->history_line) {
        m__free(server_cb->history_line);
    }

    if (server_cb->connect_valset) {
        val_free_value(server_cb->connect_valset);
    }


    /* cleanup the user edit buffer */
    if (server_cb->cli_gl) {
        (void)del_GetLine(server_cb->cli_gl);
    }

    var_clean_varQ(&server_cb->varbindQ);

    ncxmod_clean_search_result_queue(&server_cb->searchresultQ);

    while (!dlq_empty(&server_cb->modptrQ)) {
        modptr = (modptr_t *)dlq_deque(&server_cb->modptrQ);
        free_modptr(modptr);
    }

    while (!dlq_empty(&server_cb->notificationQ)) {
        notif = (mgr_not_msg_t *)dlq_deque(&server_cb->notificationQ);
        mgr_not_free_msg(notif);
    }

    if (server_cb->runstack_context) {
        runstack_free_context(server_cb->runstack_context);
    }

    m__free(server_cb);

}  /* free_server_cb */


/********************************************************************
* FUNCTION new_server_cb
* 
*  Malloc and init a new server control block
* 
* INPUTS:
*    name == name of server record
*
* RETURNS:
*   malloced server_cb struct or NULL of malloc failed
*********************************************************************/
static server_cb_t *
    new_server_cb (const xmlChar *name)
{
    server_cb_t  *server_cb;
    int          retval;

    server_cb = m__getObj(server_cb_t);
    if (server_cb == NULL) {
        return NULL;
    }
    memset(server_cb, 0x0, sizeof(server_cb_t));

    server_cb->runstack_context = runstack_new_context();
    if (server_cb->runstack_context == NULL) {
        m__free(server_cb);
        return NULL;
    }

    dlq_createSQue(&server_cb->varbindQ);
    dlq_createSQue(&server_cb->searchresultQ);
    dlq_createSQue(&server_cb->modptrQ);
    dlq_createSQue(&server_cb->notificationQ);
    dlq_createSQue(&server_cb->autoload_modcbQ);
    dlq_createSQue(&server_cb->autoload_devcbQ);

    /* set the default CLI history file (may not get used) */
    server_cb->history_filename = xml_strdup(YANGCLI_DEF_HISTORY_FILE);
    if (server_cb->history_filename == NULL) {
        free_server_cb(server_cb);
        return NULL;
    }
    server_cb->history_auto = autohistory;

    /* store per-session temp files */
    server_cb->temp_progcb = temp_progcb;

    /* the name is not used yet; needed when multiple
     * server profiles are needed at once instead
     * of 1 session at a time
     */
    server_cb->name = xml_strdup(name);
    if (server_cb->name == NULL) {
        free_server_cb(server_cb);
        return NULL;
    }

    /* get a tecla CLI control block */
    server_cb->cli_gl = new_GetLine(YANGCLI_LINELEN, YANGCLI_HISTLEN);
    if (server_cb->cli_gl == NULL) {
        log_error("\nError: cannot allocate a new GL");
        free_server_cb(server_cb);
        return NULL;
    }

    /* setup CLI tab line completion */
    retval = gl_customize_completion(server_cb->cli_gl,
                                     &server_cb->completion_state,
                                     yangcli_tab_callback);
    if (retval != 0) {
        log_error("\nError: cannot set GL tab completion");
        free_server_cb(server_cb);
        return NULL;
    }

    /* setup the inactivity timeout callback function */
    retval = gl_inactivity_timeout(server_cb->cli_gl,
                                   get_line_timeout,
                                   server_cb,
                                   1,
                                   0);
    if (retval != 0) {
        log_error("\nError: cannot set GL inactivity timeout");
        free_server_cb(server_cb);
        return NULL;
    }

    /* setup the history buffer if needed */
    if (server_cb->history_auto) {
        retval = gl_load_history(server_cb->cli_gl,
                                 (const char *)server_cb->history_filename,
                                 "#");   /* comment prefix */
        if (retval) {
            log_error("\nError: cannot load command line history buffer");
            free_server_cb(server_cb);
            return NULL;
        }
    }

    /* set up lock control blocks for get-locks */
    server_cb->locks_active = FALSE;
    server_cb->locks_waiting = FALSE;
    server_cb->locks_cur_cfg = NCX_CFGID_RUNNING;
    server_cb->locks_timeout = 120;
    server_cb->locks_retry_interval = 1;
    server_cb->locks_cleanup = FALSE;
    server_cb->locks_start_time = (time_t)0;
    server_cb->lock_cb[NCX_CFGID_RUNNING].config_id = 
        NCX_CFGID_RUNNING;
    server_cb->lock_cb[NCX_CFGID_RUNNING].config_name = 
        NCX_CFG_RUNNING;
    server_cb->lock_cb[NCX_CFGID_RUNNING].lock_state = 
        LOCK_STATE_IDLE;

    server_cb->lock_cb[NCX_CFGID_CANDIDATE].config_id = 
        NCX_CFGID_CANDIDATE;
    server_cb->lock_cb[NCX_CFGID_CANDIDATE].config_name = 
        NCX_CFG_CANDIDATE;
    server_cb->lock_cb[NCX_CFGID_CANDIDATE].lock_state = 
        LOCK_STATE_IDLE;

    server_cb->lock_cb[NCX_CFGID_STARTUP].config_id = 
        NCX_CFGID_STARTUP;
    server_cb->lock_cb[NCX_CFGID_STARTUP].config_name = 
        NCX_CFG_STARTUP;
    server_cb->lock_cb[NCX_CFGID_STARTUP].lock_state = 
        LOCK_STATE_IDLE;

    /* set default server flags to current settings */
    server_cb->state = MGR_IO_ST_INIT;
    server_cb->baddata = baddata;
    server_cb->log_level = log_get_debug_level();
    server_cb->autoload = autoload;
    server_cb->fixorder = fixorder;
    server_cb->get_optional = optional;
    server_cb->testoption = testoption;
    server_cb->erroption = erroption;
    server_cb->defop = defop;
    server_cb->timeout = default_timeout;
    server_cb->display_mode = display_mode;
    server_cb->withdefaults = withdefaults;
    server_cb->history_size = YANGCLI_HISTLEN;
    server_cb->command_mode = CMD_MODE_NORMAL;
    server_cb->defindent = defindent;
    server_cb->echo_replies = echo_replies;
    server_cb->time_rpcs = time_rpcs;
    server_cb->match_names = match_names;
    server_cb->alt_names = alt_names;

    /* TBD: add user config for this knob */
    server_cb->overwrite_filevars = TRUE;

    server_cb->use_xmlheader = use_xmlheader;

    return server_cb;

}  /* new_server_cb */


/********************************************************************
* FUNCTION update_server_cb_vars
* 
*  Update the 
* 
* INPUTS:
*    server_cb == server_cb to update
*
* OUTPUTS: 
*     server_cb->foo is updated if it is a shadow of a global var
*
*********************************************************************/
static void
    update_server_cb_vars (server_cb_t *server_cb)
{
    server_cb->baddata = baddata;
    server_cb->log_level = log_get_debug_level();
    server_cb->autoload = autoload;
    server_cb->fixorder = fixorder;
    server_cb->get_optional = optional;
    server_cb->testoption = testoption;
    server_cb->erroption = erroption;
    server_cb->defop = defop;
    server_cb->timeout = default_timeout;
    server_cb->display_mode = display_mode;
    server_cb->withdefaults = withdefaults;
    server_cb->defindent = defindent;
    server_cb->echo_replies = echo_replies;
    server_cb->time_rpcs = time_rpcs;
    server_cb->match_names = match_names;
    server_cb->alt_names = alt_names;
    server_cb->use_xmlheader = use_xmlheader;

}  /* update_server_cb_vars */


/********************************************************************
 * FUNCTION handle_config_assign
 * 
 * handle a user assignment of a config variable
 * consume memory 'newval' if it is non-NULL
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   configval == value to set
 *  use 1 of:
 *   newval == value to use for changing 'configval' 
 *   newvalstr == value to use as string form 
 * 
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    handle_config_assign (server_cb_t *server_cb,
                          val_value_t *configval,
                          val_value_t *newval,
                          const xmlChar *newvalstr)
{
    const xmlChar         *usestr;
    xmlChar               *dupval;
    val_value_t           *testval;
    obj_template_t        *testobj;
    status_t               res;
    log_debug_t            testloglevel;
    ncx_bad_data_t         testbaddata;
    op_testop_t            testop;
    op_errop_t             errop;
    op_defop_t             mydefop;
    ncx_num_t              testnum;
    ncx_display_mode_t     dmode;

    res = NO_ERR;
    if (newval) {
        if (!typ_is_string(newval->btyp)) {
            val_free_value( newval );
            return ERR_NCX_WRONG_TYPE;
        }
        if (VAL_STR(newval) == NULL) {
            val_free_value( newval );
            return ERR_NCX_INVALID_VALUE;
        }
        usestr = VAL_STR(newval);
    } else if (newvalstr) {
        usestr = newvalstr;
    } else {
        log_error("\nError: NULL value in config assignment");
        return ERR_NCX_INVALID_VALUE;
    }

    if (!xml_strcmp(configval->name, YANGCLI_SERVER)) {
        /* should check for valid IP address!!! */
        if (val_need_quotes(usestr)) {
            /* using this dumb test as a placeholder */
            log_error("\nError: invalid hostname");
        } else {
            /* save or update the connnect_valset */
            testval = val_find_child(connect_valset, NULL, YANGCLI_SERVER);
            if (testval) {
                res = val_set_simval(testval,
                                     testval->typdef,
                                     testval->nsid,
                                     testval->name,
                                     usestr);
                if (res != NO_ERR) {
                    log_error("\nError: changing 'server' failed");
                }
            } else {
                testobj = obj_find_child(connect_valset->obj,
                                         NULL, 
                                         YANGCLI_SERVER);
                if (testobj) {
                    testval = val_make_simval_obj(testobj, usestr, &res);
                    if (testval) {
                        val_add_child(testval, connect_valset);
                    }
                }
            }
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_AUTOALIASES)) {
        if (ncx_is_true(usestr)) {
            autoaliases = TRUE;
        } else if (ncx_is_false(usestr)) {
            autoaliases = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_ALIASES_FILE)) {
        aliases_file = usestr;
    } else if (!xml_strcmp(configval->name, YANGCLI_AUTOCOMP)) {
        if (ncx_is_true(usestr)) {
            autocomp = TRUE;
        } else if (ncx_is_false(usestr)) {
            autocomp = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_AUTOHISTORY)) {
        if (ncx_is_true(usestr)) {
            autohistory = TRUE;
        } else if (ncx_is_false(usestr)) {
            autohistory = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_AUTOLOAD)) {
        if (ncx_is_true(usestr)) {
            server_cb->autoload = TRUE;
            autoload = TRUE;
        } else if (ncx_is_false(usestr)) {
            server_cb->autoload = FALSE;
            autoload = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_AUTOUSERVARS)) {
        if (ncx_is_true(usestr)) {
            autouservars = TRUE;
        } else if (ncx_is_false(usestr)) {
            autouservars = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_USERVARS_FILE)) {
        uservars_file = usestr;
    } else if (!xml_strcmp(configval->name, YANGCLI_BADDATA)) {
        testbaddata = ncx_get_baddata_enum(usestr);
        if (testbaddata != NCX_BAD_DATA_NONE) {
            server_cb->baddata = testbaddata;
            baddata = testbaddata;
        } else {
            log_error("\nError: value must be 'ignore', 'warn', "
                      "'check', or 'error'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_DEF_MODULE)) {
        if (!ncx_valid_name2(usestr)) {
            log_error("\nError: must be a valid module name");
            res = ERR_NCX_INVALID_VALUE;
        } else {
            /* save a copy of the string value */
            dupval = xml_strdup(usestr);
            if (dupval == NULL) {
                log_error("\nError: malloc failed");
                res = ERR_INTERNAL_MEM;
            } else {
                if (default_module) {
                    m__free(default_module);
                }
                default_module = dupval;
            }
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_DISPLAY_MODE)) {
        dmode = ncx_get_display_mode_enum(usestr);
        if (dmode != NCX_DISPLAY_MODE_NONE) {
            display_mode = dmode;
            server_cb->display_mode = dmode;
        } else {
            log_error("\nError: value must be 'plain', 'prefix', "
                      "'json, 'xml', or 'xml-nons'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_ECHO_REPLIES)) {
        if (ncx_is_true(usestr)) {
            server_cb->echo_replies = TRUE;
            echo_replies = TRUE;
        } else if (ncx_is_false(usestr)) {
            server_cb->echo_replies = FALSE;
            echo_replies = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_TIME_RPCS)) {
        if (ncx_is_true(usestr)) {
            server_cb->time_rpcs = TRUE;
            time_rpcs = TRUE;
        } else if (ncx_is_false(usestr)) {
            server_cb->time_rpcs = FALSE;
            time_rpcs = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_MATCH_NAMES)) {
        ncx_name_match_t match = ncx_get_name_match_enum(usestr);

        if (match == NCX_MATCH_NONE) {
            log_error("\nError: value must be 'exact', "
                      "'exact-nocase', 'one', 'one-nocase', "
                      "'first', or 'first-nocase'");
            res = ERR_NCX_INVALID_VALUE;
        } else {
            server_cb->match_names = match;
            match_names = match;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_ALT_NAMES)) {
        if (ncx_is_true(usestr)) {
            server_cb->alt_names = TRUE;
            alt_names = TRUE;
        } else if (ncx_is_false(usestr)) {
            server_cb->alt_names = FALSE;
            alt_names = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_USE_XMLHEADER)) {
        if (ncx_is_true(usestr)) {
            server_cb->use_xmlheader = TRUE;
            use_xmlheader = TRUE;
        } else if (ncx_is_false(usestr)) {
            server_cb->use_xmlheader = FALSE;
            use_xmlheader = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_USER)) {
        if (!ncx_valid_name2(usestr)) {
            log_error("\nError: must be a valid user name");
            res = ERR_NCX_INVALID_VALUE;
        } else {
            /* save or update the connnect_valset */
            testval = val_find_child(connect_valset,
                                     NULL, 
                                     YANGCLI_USER);
            if (testval) {
                res = val_set_simval(testval,
                                     testval->typdef,
                                     testval->nsid,
                                     testval->name,
                                     usestr);
                if (res != NO_ERR) {
                    log_error("\nError: changing user name failed");
                }
            } else {
                testobj = obj_find_child(connect_valset->obj,
                                         NULL, 
                                         YANGCLI_USER);
                if (testobj) {
                    testval = val_make_simval_obj(testobj,
                                                  usestr,
                                                  &res);
                }
                if (testval) {
                    val_add_child(testval, connect_valset);
                }
            }
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_TEST_OPTION)) {
        if (!xml_strcmp(usestr, NCX_EL_NONE)) {
            server_cb->testoption = OP_TESTOP_NONE;
            testop = OP_TESTOP_NONE;
        } else {            
            testop = op_testop_enum(usestr);
            if (testop != OP_TESTOP_NONE) {
                server_cb->testoption = testop;
                testoption = testop;
            } else {
                log_error("\nError: must be a valid 'test-option'");
                log_error("\n       (none, test-then-set, set, "
                          "test-only)\n");
                res = ERR_NCX_INVALID_VALUE;
            }
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_ERROR_OPTION)) {
        if (!xml_strcmp(usestr, NCX_EL_NONE)) {
            server_cb->erroption = OP_ERROP_NONE;
            erroption = OP_ERROP_NONE;
        } else {            
            errop = op_errop_id(usestr);
            if (errop != OP_ERROP_NONE) {
                server_cb->erroption = errop;
                erroption = errop;
            } else {
                log_error("\nError: must be a valid 'error-option'");
                log_error("\n       (none, stop-on-error, "
                          "continue-on-error, rollback-on-error)\n");
                res = ERR_NCX_INVALID_VALUE;
            }
        }
    } else if (!xml_strcmp(configval->name, NCX_EL_DEFAULT_OPERATION)) {
        mydefop = op_defop_id2(usestr);
        if (mydefop != OP_DEFOP_NOT_SET) {
            server_cb->defop = mydefop;
            defop = mydefop;
        } else {
            log_error("\nError: must be a valid 'default-operation'");
            log_error("\n       (none, merge, "
                      "replace, not-used)\n");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_TIMEOUT)) {
        ncx_init_num(&testnum);
        res = ncx_decode_num(usestr, NCX_BT_UINT32, &testnum);
        if (res == NO_ERR) {
            server_cb->timeout = testnum.u;
            default_timeout = testnum.u;
        } else {
            log_error("\nError: must be valid uint32 value");
        }
        ncx_clean_num(NCX_BT_UINT32, &testnum);
    } else if (!xml_strcmp(configval->name, YANGCLI_OPTIONAL)) {
        if (ncx_is_true(usestr)) {
            optional = TRUE;
            server_cb->get_optional = TRUE;
        } else if (ncx_is_false(usestr)) {
            optional = FALSE;
            server_cb->get_optional = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, NCX_EL_LOGLEVEL)) {
        testloglevel = 
            log_get_debug_level_enum((const char *)usestr);
        if (testloglevel == LOG_DEBUG_NONE) {
            log_error("\nError: value must be valid log-level:");
            log_error("\n       (off, error,"
                      "warn, info, debug, debug2)\n");
            res = ERR_NCX_INVALID_VALUE;
        } else {
            log_set_debug_level(testloglevel);
            server_cb->log_level = testloglevel;
        }
    } else if (!xml_strcmp(configval->name, YANGCLI_FIXORDER)) {
        if (ncx_is_true(usestr)) {
            fixorder = TRUE;
            server_cb->fixorder = TRUE;
        } else if (ncx_is_false(usestr)) {
            fixorder = FALSE;
            server_cb->fixorder = FALSE;
        } else {
            log_error("\nError: value must be 'true' or 'false'");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (!xml_strcmp(configval->name, NCX_EL_WITH_DEFAULTS)) {
        if (!xml_strcmp(usestr, NCX_EL_NONE)) {
            withdefaults = NCX_WITHDEF_NONE;
        } else {
            if (ncx_get_withdefaults_enum(usestr) == NCX_WITHDEF_NONE) {
                log_error("\nError: value must be 'none', "
                          "'report-all', 'trim', or 'explicit'");
                res = ERR_NCX_INVALID_VALUE;
            } else {
                withdefaults = ncx_get_withdefaults_enum(usestr);
                server_cb->withdefaults = withdefaults;
            }
        }
    } else if (!xml_strcmp(configval->name, NCX_EL_INDENT)) {
        ncx_init_num(&testnum);
        res = ncx_decode_num(usestr, NCX_BT_UINT32, &testnum);
        if (res == NO_ERR) {
            if (testnum.u > YANGCLI_MAX_INDENT) {
                log_error("\nError: value must be a uint32 (0..9)");
                res = ERR_NCX_INVALID_VALUE;
            } else {
                /* indent value is valid */
                defindent = (int32)testnum.u;
                server_cb->defindent = defindent;
            }
        } else {
            log_error("\nError: must be valid uint32 value");
        }
        ncx_clean_num(NCX_BT_INT32, &testnum);
    } else {
        /* unknown parameter name */
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* update the variable value for user access */
    if (res == NO_ERR) {
        if (newval) {
            res = var_set_move(server_cb->runstack_context,
                               configval->name, 
                               xml_strlen(configval->name),
                               VAR_TYP_CONFIG, 
                               newval);
        } else {
            res = var_set_from_string(server_cb->runstack_context,
                                      configval->name,
                                      newvalstr, 
                                      VAR_TYP_CONFIG);
        }
        if (res != NO_ERR) {
            log_error("\nError: set result for '%s' failed (%s)",
                          server_cb->result_name, 
                          get_error_string(res));
        }
    }

    if (res == NO_ERR) {
        log_info("\nSystem variable set\n");
    }
    return res;

} /* handle_config_assign */


/********************************************************************
* FUNCTION handle_delete_result
* 
* Delete the specified file, if it is ASCII and a regular file
*
* INPUTS:
*    server_cb == server control block to use
*
* OUTPUTS:
*    server_cb->result_filename will get deleted if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    handle_delete_result (server_cb_t *server_cb)
{
    status_t     res;
    struct stat  statbuf;
    int          statresult;

    res = NO_ERR;

    if (LOGDEBUG2) {
        log_debug2("\n*** delete file result '%s'",
                   server_cb->result_filename);
    }

    /* see if file already exists */
    statresult = stat((const char *)server_cb->result_filename,
                      &statbuf);
    if (statresult != 0) {
        log_error("\nError: assignment file '%s' could not be opened",
                  server_cb->result_filename);
        res = errno_to_status();
    } else if (!S_ISREG(statbuf.st_mode)) {
        log_error("\nError: assignment file '%s' is not a regular file",
                  server_cb->result_filename);
        res = ERR_NCX_OPERATION_FAILED;
    } else {
        statresult = remove((const char *)server_cb->result_filename);
        if (statresult == -1) {
            log_error("\nError: assignment file '%s' could not be deleted",
                      server_cb->result_filename);
            res = errno_to_status();
        }
    }

    clear_result(server_cb);

    return res;

}  /* handle_delete_result */


/********************************************************************
* FUNCTION output_file_result
* 
* Check the filespec string for a file assignment statement
* Save it if it si good
*
* INPUTS:
*    server_cb == server control block to use
* use 1 of these 2 parms:
*    resultval == result to output to file
*    resultstr == result to output as string
*
* OUTPUTS:
*    server_cb->result_filename will get set if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    output_file_result (server_cb_t *server_cb,
                        val_value_t *resultval,
                        const xmlChar *resultstr)
{
    FILE        *fil;
    status_t     res;
    xml_attrs_t  attrs;
    struct stat  statbuf;
    int          statresult;

    res = NO_ERR;

    if (LOGDEBUG2) {
        log_debug2("\n*** output file result to '%s'",
                   server_cb->result_filename);
    }

    /* see if file already exists, unless overwrite_filevars is set */
    if (!server_cb->overwrite_filevars) {
        statresult = stat((const char *)server_cb->result_filename,
                          &statbuf);
        if (statresult == 0) {
            log_error("\nError: assignment file '%s' already exists",
                      server_cb->result_filename);
            clear_result(server_cb);
            return ERR_NCX_DATA_EXISTS;
        }
    }
    
    if (resultval) {
        result_format_t rf = 
            get_file_result_format(server_cb->result_filename);
        switch (rf) {
        case RF_TEXT:
            /* output in text format to the specified file */
            res = log_alt_open((const char *)
                               server_cb->result_filename);
            if (res != NO_ERR) {
                log_error("\nError: assignment file '%s' could "
                          "not be opened (%s)",
                          server_cb->result_filename,
                          get_error_string(res));
            } else {
                val_dump_value_max(resultval,
                                   0,   /* startindent */
                                   server_cb->defindent,
                                   DUMP_VAL_ALT_LOG, /* dumpmode */
                                   server_cb->display_mode,
                                   FALSE,    /* withmeta */
                                   FALSE);   /* configonly */
                log_alt_close();
            }
            break;
        case RF_XML:
            /* output in XML format to the specified file */
            xml_init_attrs(&attrs);
            res = xml_wr_file(server_cb->result_filename,
                              resultval, &attrs, XMLMODE, 
                              server_cb->use_xmlheader,
                              (server_cb->display_mode 
                               == NCX_DISPLAY_MODE_XML_NONS) 
                              ? FALSE : TRUE, 0,
                              server_cb->defindent);
            xml_clean_attrs(&attrs);
            break;
        case RF_JSON:
            /* output in JSON format to the specified file */
            res = json_wr_file(server_cb->result_filename,
                               resultval, 0, server_cb->defindent);
            break;
        case RF_NONE:
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    } else if (resultstr) {
        fil = fopen((const char *)server_cb->result_filename, "w");
        if (fil == NULL) {
            log_error("\nError: assignment file '%s' could "
                      "not be opened",
                      server_cb->result_filename);
            res = errno_to_status();
        } else {
            statresult = fputs((const char *)resultstr, fil);
            if (statresult == EOF) {
                log_error("\nError: assignment file '%s' could "
                          "not be written",
                          server_cb->result_filename);
                res = errno_to_status();
            } else {
                statresult = fputc('\n', fil);  
                if (statresult == EOF) {
                    log_error("\nError: assignment file '%s' could "
                              "not be written",
                              server_cb->result_filename);
                    res = errno_to_status();
                }
            }
        }
        if (fil != NULL) {
            statresult = fclose(fil);
            if (statresult == EOF) {
                log_error("\nError: assignment file '%s' could "
                          "not be closed",
                          server_cb->result_filename);
                res = errno_to_status();
            }
        }
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    clear_result(server_cb);

    return res;

}  /* output_file_result */


/********************************************************************
* FUNCTION check_assign_statement
* 
* Check if the command line is an assignment statement
* 
* E.g.,
*
*   $foo = $bar
*   $foo = get-config filter=@filter.xml
*   $foo = "quoted string literal"
*   $foo = [<inline><xml/></inline>]
*   $foo = @datafile.xml
*
* INPUTS:
*   server_cb == server control block to use
*   line == command line string to expand
*   len  == address of number chars parsed so far in line
*   getrpc == address of return flag to parse and execute an
*            RPC, which will be assigned to the var found
*            in the 'result' module variable
*   fileassign == address of file assign flag
*
* OUTPUTS:
*   *len == number chars parsed in the assignment statement
*   *getrpc == TRUE if the rest of the line represent an
*              RPC command that needs to be evaluated
*   *fileassign == TRUE if the assignment is for a
*                  file output (starts with @)
*                  FALSE if not a file assignment
*
* SIDE EFFECTS:
*    If this is an 'rpc' assignment statement, 
*    (*dorpc == TRUE && *len > 0 && return NO_ERR),
*    then the global result and result_pending variables will
*    be set upon return.  For the other (direct) assignment
*    statement variants, the statement is completely handled
*    here.
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    check_assign_statement (server_cb_t *server_cb,
                            const xmlChar *line,
                            uint32 *len,
                            boolean *getrpc,
                            boolean *fileassign)
{
    const xmlChar         *str, *name, *filespec;
    val_value_t           *curval;
    obj_template_t        *obj;
    val_value_t           *val;
    xmlChar               *tempstr;
    uint32                 nlen, tlen;
    var_type_t             vartype;
    status_t               res;

    /* save start point in line */
    str = line;
    *len = 0;
    *getrpc = FALSE;
    *fileassign = FALSE;
    vartype = VAR_TYP_NONE;
    curval = NULL;

    /* skip leading whitespace */
    while (*str && isspace(*str)) {
        str++;
    }

    if (*str == '@') {
        /* check if valid file assignment is being made */
        *fileassign = TRUE;
        str++;
    }

    if (*str == '$') {
        /* check if a valid variable assignment is being made */
        res = var_check_ref(server_cb->runstack_context,
                            str, ISLEFT, &tlen, &vartype, &name, &nlen);
        if (res != NO_ERR) {
            /* error in the varref */
            return res;
        } else if (tlen == 0) {
            /* should not happen: returned not a varref */
            *getrpc = TRUE;
            return NO_ERR;
        } else if (*fileassign) {
            /* file assignment complex form:
             *
             *    @$foo = bar or @$$foo = bar
             *
             * get the var reference for real because
             * it is supposed to contain the filespec
             * for the output file
             */
            curval = var_get_str(server_cb->runstack_context,
                                 name, nlen, vartype);
            if (curval == NULL) {
                log_error("\nError: file assignment variable "
                          "not found");
                return ERR_NCX_VAR_NOT_FOUND;
            }

            /* variable must be a string */
            if (!typ_is_string(curval->btyp)) {
                log_error("\nError: file assignment variable '%s' "
                          "is wrong type '%s'",
                          curval->name,
                          tk_get_btype_sym(curval->btyp));
                return ERR_NCX_VAR_NOT_FOUND;
            }
            filespec = VAL_STR(curval);
            res = check_filespec(server_cb, filespec, curval->name);
            if (res != NO_ERR) {
                return res;
            }
        } else {
            /* variable reference:
             *
             *     $foo or $$foo
             *
             * check for a valid varref, get the data type, which
             * will also indicate if the variable exists yet
             */
            switch (vartype) {
            case VAR_TYP_SYSTEM:
                log_error("\nError: system variables "
                          "are read-only");
                return ERR_NCX_VAR_READ_ONLY;
            case VAR_TYP_GLOBAL:
            case VAR_TYP_CONFIG:
                curval = var_get_str(server_cb->runstack_context,
                                     name, nlen, vartype);
                break;
            case VAR_TYP_LOCAL:
            case VAR_TYP_SESSION:
                curval = var_get_local_str(server_cb->runstack_context,
                                           name, nlen);
                break;
            default:
                return SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        /* move the str pointer past the variable name */
        str += tlen;
    } else if (*fileassign) {
        /* file assignment, simple form:
         *
         *     @foo.txt = bar
         *
         * get the length of the filename 
         */
        name = str;
        while (*str && !isspace(*str) && *str != '=') {
            str++;
        }
        nlen = (uint32)(str-name);

        /* save the filename in a temp string */
        tempstr = xml_strndup(name, nlen);
        if (tempstr == NULL) {
            return ERR_INTERNAL_MEM;
        }

        /* check filespec and save filename for real */
        res = check_filespec(server_cb, tempstr, NULL);

        m__free(tempstr);

        if (res != NO_ERR) {
            return res;
        }
    } else {
        /* not an assignment statement at all */
        *getrpc = TRUE;
        return NO_ERR;
    }

    /* skip any more whitespace, after the LHS term */
    while (*str && xml_isspace(*str)) {
        str++;
    }

    /* check end of string */
    if (!*str) {
        log_error("\nError: truncated assignment statement");
        clear_result(server_cb);
        return ERR_NCX_DATA_MISSING;
    }

    /* check for the equals sign assignment char */
    if (*str == NCX_ASSIGN_CH) {
        /* move past assignment char */
        str++;
    } else {
        log_error("\nError: equals sign '=' expected");
        clear_result(server_cb);
        return ERR_NCX_WRONG_TKTYPE;
    }

    /* skip any more whitespace */
    while (*str && xml_isspace(*str)) {
        str++;
    }

    /* check EO string == unset command, but only if in a TRUE conditional */
    if (!*str && runstack_get_cond_state(server_cb->runstack_context)) {
        if (*fileassign) {
            /* got file assignment (@foo) =  EOLN
             * treat this as a request to delete the file
             */
            res = handle_delete_result(server_cb);
        } else {
            /* got $foo =  EOLN
             * treat this as a request to unset the variable
             */
            if (vartype == VAR_TYP_SYSTEM ||
                vartype == VAR_TYP_CONFIG) {
                log_error("\nError: cannot remove system variables");
                clear_result(server_cb);
                return ERR_NCX_OPERATION_FAILED;
            }

            /* else try to unset this variable */
            res = var_unset(server_cb->runstack_context, name, nlen, vartype);
        }
        *len = str - line;
        return res;
    }

    /* the variable name and equals sign is parsed
     * and the current char is either '$', '"', '<',
     * or a valid first name
     *
     * if *fileassign:
     *
     *      @foo.xml = blah
     *               ^
     *  else:
     *      $foo = blah
     *           ^
     */
    if (*fileassign) {
        obj = NULL;
    } else {
        obj = (curval) ? curval->obj : NULL;
    }

    /* get the script or CLI input as a new val_value_t struct */
    val = var_check_script_val(server_cb->runstack_context,
                               obj, str, ISTOP, &res);
    if (val) {
        if (obj == NULL) {
            obj = val->obj;
        }

        /* a script value reference was found */
        if (obj == NULL || obj == ncx_get_gen_string()) {
            /* the generic name needs to be overwritten */
            val_set_name(val, name, nlen);
        }

        if (*fileassign) {
            /* file assignment of a variable value 
             *   @foo.txt=$bar  or @$foo=$bar
             */
            res = output_file_result(server_cb, val, NULL);
            val_free_value(val);
        } else {
            /* this is a plain assignment statement
             * first check if the input is VAR_TYP_CONFIG
             */
            if (vartype == VAR_TYP_CONFIG) {
                if (curval==NULL) {
                    val_free_value(val);
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    /* hand off 'val' memory here */
                    res = handle_config_assign(server_cb, curval, val, NULL);
                }
            } else {
                /* val is a malloced struct, pass it over to the
                 * var struct instead of cloning it
                 */
                res = var_set_move(server_cb->runstack_context,
                                   name, nlen, vartype, val);
                if (res != NO_ERR) {
                    val_free_value(val);
                }
            }
        }
    } else if (res==NO_ERR) {
        /* this is as assignment to the results
         * of an RPC function call 
         */
        if (server_cb->result_name) {
            log_error("\nError: result already pending for %s",
                     server_cb->result_name);
            m__free(server_cb->result_name);
            server_cb->result_name = NULL;
        }

        if (!*fileassign) {
            /* save the variable result name */
            server_cb->result_name = xml_strndup(name, nlen);
            if (server_cb->result_name == NULL) {
                *len = 0;
                res = ERR_INTERNAL_MEM;
            } else {
                server_cb->result_vartype = vartype;
            }
        }

        if (res == NO_ERR) {
            *len = str - line;
            *getrpc = TRUE;
        }
    } else {
        /* there was some error in the statement processing */
        *len = 0;
        clear_result(server_cb);
    }

    return res;

}  /* check_assign_statement */


/********************************************************************
 * FUNCTION create_system_var
 * 
 * create a read-only system variable
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   varname == variable name
 *   varval  == variable string value (may be NULL)
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    create_system_var (server_cb_t *server_cb,
                       const char *varname,
                       const char *varval)
{
    status_t  res;

    res = var_set_from_string(server_cb->runstack_context,
                              (const xmlChar *)varname,
                              (const xmlChar *)varval,
                              VAR_TYP_SYSTEM);
    return res;

} /* create_system_var */


/********************************************************************
 * FUNCTION create_config_var
 * 
 * create a read-write system variable
 *
 * INPUTS:
 *   server_cb == server control block
 *   varname == variable name
 *   varval  == variable string value (may be NULL)
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    create_config_var (server_cb_t *server_cb,
                       const xmlChar *varname,
                       const xmlChar *varval)
{
    status_t  res;

    res = var_set_from_string(server_cb->runstack_context,
                              varname, 
                              varval,
                              VAR_TYP_CONFIG);
    return res;

} /* create_config_var */


/********************************************************************
 * FUNCTION init_system_vars
 * 
 * create the read-only system variables
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    init_system_vars (server_cb_t *server_cb)
{
    const char *envstr;
    status_t    res;

    envstr = getenv(NCXMOD_PWD);
    res = create_system_var(server_cb, NCXMOD_PWD, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = (const char *)ncxmod_get_home();
    res = create_system_var(server_cb, USER_HOME, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(ENV_HOST);
    res = create_system_var(server_cb, ENV_HOST, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(ENV_SHELL);
    res = create_system_var(server_cb, ENV_SHELL, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(ENV_USER);
    res = create_system_var(server_cb, ENV_USER, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(ENV_LANG);
    res = create_system_var(server_cb, ENV_LANG, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(NCXMOD_HOME);
    res = create_system_var(server_cb, NCXMOD_HOME, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(NCXMOD_MODPATH);
    res = create_system_var(server_cb, NCXMOD_MODPATH, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(NCXMOD_DATAPATH);
    res = create_system_var(server_cb, NCXMOD_DATAPATH, envstr);
    if (res != NO_ERR) {
        return res;
    }

    envstr = getenv(NCXMOD_RUNPATH);
    res = create_system_var(server_cb, NCXMOD_RUNPATH, envstr);
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

} /* init_system_vars */


/********************************************************************
 * FUNCTION init_config_vars
 * 
 * create the read-write global variables
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    init_config_vars (server_cb_t *server_cb)
{
    val_value_t    *parm;
    const xmlChar  *strval;
    status_t        res;
    xmlChar         numbuff[NCX_MAX_NUMLEN];

    /* $$server = ip-address */
    strval = NULL;
    parm = val_find_child(mgr_cli_valset, NULL, YANGCLI_SERVER);
    if (parm) {
        strval = VAL_STR(parm);
    }
    res = create_config_var(server_cb, YANGCLI_SERVER, strval);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ autoaliases = boolean */
    res = create_config_var(server_cb, YANGCLI_AUTOALIASES, 
                            (autoaliases) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ aliases-file = filespec */
    res = create_config_var(server_cb, YANGCLI_ALIASES_FILE, aliases_file);
    if (res != NO_ERR) {
        return res;
    }

    /* $$autocomp = boolean */
    res = create_config_var(server_cb, YANGCLI_AUTOCOMP, 
                            (autocomp) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ autohistory = boolean */
    res = create_config_var(server_cb, YANGCLI_AUTOHISTORY, 
                            (autohistory) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ autoload = boolean */
    res = create_config_var(server_cb, YANGCLI_AUTOLOAD, 
                            (autoload) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ autouservars = boolean */
    res = create_config_var(server_cb, YANGCLI_AUTOUSERVARS, 
                            (autouservars) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ uservars-file = filespec */
    res = create_config_var(server_cb, YANGCLI_USERVARS_FILE, uservars_file);
    if (res != NO_ERR) {
        return res;
    }

    /* $$baddata = enum */
    res = create_config_var(server_cb, YANGCLI_BADDATA, 
                            ncx_get_baddata_string(baddata));
    if (res != NO_ERR) {
        return res;
    }

    /* $$default-module = string */
    res = create_config_var(server_cb, YANGCLI_DEF_MODULE, default_module);
    if (res != NO_ERR) {
        return res;
    }

    /* $$display-mode = enum */
    res = create_config_var(server_cb, YANGCLI_DISPLAY_MODE, 
                            ncx_get_display_mode_str(display_mode));
    if (res != NO_ERR) {
        return res;
    }

    /* $$echo-replies = boolean */
    res = create_config_var(server_cb, YANGCLI_ECHO_REPLIES, 
                            (echo_replies) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ time-rpcs = boolean */
    res = create_config_var(server_cb, YANGCLI_TIME_RPCS, 
                            (time_rpcs) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ match-names = enum */
    res = create_config_var(server_cb, YANGCLI_MATCH_NAMES, 
                            ncx_get_name_match_string(match_names));
    if (res != NO_ERR) {
        return res;
    }

    /* $$ alt-names = boolean */
    res = create_config_var(server_cb, YANGCLI_ALT_NAMES, 
                            (alt_names) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$ use-xmlheader = boolean */
    res = create_config_var(server_cb, YANGCLI_USE_XMLHEADER, 
                            (use_xmlheader) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$user = string */
    strval = NULL;
    parm = val_find_child(mgr_cli_valset, NULL, YANGCLI_USER);
    if (parm) {
        strval = VAL_STR(parm);
    } else {
        strval = (const xmlChar *)getenv(ENV_USER);
    }
    res = create_config_var(server_cb, YANGCLI_USER, strval);
    if (res != NO_ERR) {
        return res;
    }

    /* $$test-option = enum */
    res = create_config_var(server_cb, YANGCLI_TEST_OPTION,
                            op_testop_name(testoption));
    if (res != NO_ERR) {
        return res;
    }

    /* $$error-optiona = enum */
    res = create_config_var(server_cb, YANGCLI_ERROR_OPTION,
                            op_errop_name(erroption)); 
    if (res != NO_ERR) {
        return res;
    }

    /* $$default-timeout = uint32 */
    sprintf((char *)numbuff, "%u", default_timeout);
    res = create_config_var(server_cb, YANGCLI_TIMEOUT, numbuff);
    if (res != NO_ERR) {
        return res;
    }

    /* $$indent = int32 */
    sprintf((char *)numbuff, "%d", defindent);
    res = create_config_var(server_cb, NCX_EL_INDENT, numbuff);
    if (res != NO_ERR) {
        return res;
    }

    /* $$optional = boolean */
    res = create_config_var(server_cb, YANGCLI_OPTIONAL, 
                            (cur_server_cb->get_optional) 
                            ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$log-level = enum
     * could have changed during CLI processing; do not cache
     */
    res = create_config_var(server_cb, NCX_EL_LOGLEVEL, 
                            log_get_debug_level_string
                            (log_get_debug_level()));
    if (res != NO_ERR) {
        return res;
    }

    /* $$fixorder = boolean */
    res = create_config_var(server_cb, YANGCLI_FIXORDER, 
                            (fixorder) ? NCX_EL_TRUE : NCX_EL_FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* $$with-defaults = enum */
    res = create_config_var(server_cb, YANGCLI_WITH_DEFAULTS,
                            ncx_get_withdefaults_string(withdefaults)); 
    if (res != NO_ERR) {
        return res;
    }

    /* $$default-operation = enum */
    res = create_config_var(server_cb, NCX_EL_DEFAULT_OPERATION,
                            op_defop_name(defop)); 
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

} /* init_config_vars */


/********************************************************************
* FUNCTION process_cli_input
*
* Process the param line parameters against the hardwired
* parmset for the ncxmgr program
*
* INPUTS:
*    server_cb == server control block to use
*    argc == argument count
*    argv == array of command line argument strings
*
* OUTPUTS:
*    global vars that are present are filled in, with parms 
*    gathered or defaults
*
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
static status_t
    process_cli_input (server_cb_t *server_cb,
                       int argc,
                       char *argv[])
{
    obj_template_t        *obj;
    val_value_t           *parm;
    status_t               res;
    ncx_display_mode_t     dmode;
    boolean                defs_done;

    res = NO_ERR;
    mgr_cli_valset = NULL;
    defs_done = FALSE;

    /* find the parmset definition in the registry */
    obj = ncx_find_object(yangcli_mod, YANGCLI_BOOT);
    if (obj == NULL) {
        res = ERR_NCX_NOT_FOUND;
    }

    if (res == NO_ERR) {
        /* check no command line parms */
        if (argc <= 1) {
            mgr_cli_valset = val_new_value();
            if (mgr_cli_valset == NULL) {
                res = ERR_INTERNAL_MEM;
            } else {
                val_init_from_template(mgr_cli_valset, obj);
            }
        } else {
            /* parse the command line against the object template */    
            mgr_cli_valset = cli_parse(server_cb->runstack_context,
                                       argc, 
                                       argv, 
                                       obj,
                                       FULLTEST, 
                                       PLAINMODE,
                                       autocomp,
                                       CLI_MODE_PROGRAM,
                                       &res);
	    defs_done = TRUE;
        }
    }

    if (res != NO_ERR) {
        if (mgr_cli_valset) {
            val_free_value(mgr_cli_valset);
            mgr_cli_valset = NULL;
        }
        return res;
    }

    /* next get any params from the conf file */
    confname = get_strparm(mgr_cli_valset, 
                           YANGCLI_MOD, 
                           YANGCLI_CONFIG);
    if (confname != NULL) {
        res = conf_parse_val_from_filespec(confname, 
                                           mgr_cli_valset,
                                           TRUE, 
                                           TRUE);
    } else {
        res = conf_parse_val_from_filespec(YANGCLI_DEF_CONF_FILE,
                                           mgr_cli_valset,
                                           TRUE, 
                                           FALSE);
    }

    if (res == NO_ERR && defs_done == FALSE) {
        res = val_add_defaults(mgr_cli_valset, NULL, NULL, FALSE);
    }

    if (res != NO_ERR) {
        return res;
    }

    /****************************************************
     * go through the yangcli params in order,
     * after setting up the logging parameters
     ****************************************************/

    /* set the logging control parameters */
    val_set_logging_parms(mgr_cli_valset);

    /* set the file search path parms */
    val_set_path_parms(mgr_cli_valset);

    /* set the warning control parameters */
    val_set_warning_parms(mgr_cli_valset);

    /* set the subdirs parm */
    val_set_subdirs_parm(mgr_cli_valset);

    /* set the protocols parm */
    res = val_set_protocols_parm(mgr_cli_valset);
    if (res != NO_ERR) {
        return res;
    }

    /* set the feature code generation parameters */
    val_set_feature_parms(mgr_cli_valset);

    /* get the server parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_SERVER);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the autoaliases parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_AUTOALIASES);
    if (parm && parm->res == NO_ERR) {
        autoaliases = VAL_BOOL(parm);
    } else {
        autoaliases = TRUE;
    }

    /* get the aliases-file parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_ALIASES_FILE);
    if (parm && parm->res == NO_ERR) {
        aliases_file = VAL_STR(parm);
    } else {
        aliases_file = YANGCLI_DEF_ALIASES_FILE;
    }

    /* get the autocomp parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_AUTOCOMP);
    if (parm && parm->res == NO_ERR) {
        autocomp = VAL_BOOL(parm);
    } else {
        autocomp = TRUE;
    }

    /* get the autohistory parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_AUTOHISTORY);
    if (parm && parm->res == NO_ERR) {
        autohistory = VAL_BOOL(parm);
    } else {
        autohistory = TRUE;
    }

    /* get the autoload parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_AUTOLOAD);
    if (parm && parm->res == NO_ERR) {
        autoload = VAL_BOOL(parm);
    } else {
        autoload = TRUE;
    }

    /* get the autouservars parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_AUTOUSERVARS);
    if (parm && parm->res == NO_ERR) {
        autouservars = VAL_BOOL(parm);
    } else {
        autouservars = TRUE;
    }

    /* get the baddata parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_BADDATA);
    if (parm && parm->res == NO_ERR) {
        baddata = ncx_get_baddata_enum(VAL_ENUM_NAME(parm));
        if (baddata == NCX_BAD_DATA_NONE) {
            SET_ERROR(ERR_INTERNAL_VAL);
            baddata = YANGCLI_DEF_BAD_DATA;
        }
    } else {
        baddata = YANGCLI_DEF_BAD_DATA;
    }

    /* get the batch-mode parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_BATCHMODE);
    if (parm && parm->res == NO_ERR) {
        batchmode = TRUE;
    }

    /* get the default module for unqualified module addesses */
    default_module = get_strparm(mgr_cli_valset, YANGCLI_MOD, 
                                 YANGCLI_DEF_MODULE);

    /* get the display-mode parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_DISPLAY_MODE);
    if (parm && parm->res == NO_ERR) {
        dmode = ncx_get_display_mode_enum(VAL_ENUM_NAME(parm));
        if (dmode != NCX_DISPLAY_MODE_NONE) {
            display_mode = dmode;
        } else {
            display_mode = YANGCLI_DEF_DISPLAY_MODE;
        }
    } else {
        display_mode = YANGCLI_DEF_DISPLAY_MODE;
    }

    /* get the echo-replies parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_ECHO_REPLIES);
    if (parm && parm->res == NO_ERR) {
        echo_replies = VAL_BOOL(parm);
    } else {
        echo_replies = TRUE;
    }

    /* get the time-rpcs parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_TIME_RPCS);
    if (parm && parm->res == NO_ERR) {
        time_rpcs = VAL_BOOL(parm);
    } else {
        time_rpcs = FALSE;
    }

    /* get the fixorder parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_FIXORDER);
    if (parm && parm->res == NO_ERR) {
        fixorder = VAL_BOOL(parm);
    } else {
        fixorder = YANGCLI_DEF_FIXORDER;
    }

    /* get the help flag */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_HELP);
    if (parm && parm->res == NO_ERR) {
        helpmode = TRUE;
    }

    /* help submode parameter (brief/normal/full) */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, NCX_EL_BRIEF);
    if (parm) {
        helpsubmode = HELP_MODE_BRIEF;
    } else {
        /* full parameter */
        parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, NCX_EL_FULL);
        if (parm) {
            helpsubmode = HELP_MODE_FULL;
        } else {
            helpsubmode = HELP_MODE_NORMAL;
        }
    }

    /* get indent param */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, NCX_EL_INDENT);
    if (parm && parm->res == NO_ERR) {
        defindent = (int32)VAL_UINT(parm);
    } else {
        defindent = NCX_DEF_INDENT;
    }

    /* get the password parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_PASSWORD);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the --ncport parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_NCPORT);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the --private-key parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_PRIVATE_KEY);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the --public-key parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_PUBLIC_KEY);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the --transport parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_TRANSPORT);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the --tcp-direct-enable parameter */
    parm = val_find_child(mgr_cli_valset, 
                          YANGCLI_MOD, 
                          YANGCLI_TCP_DIRECT_ENABLE);
    if (parm && parm->res == NO_ERR) {
        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the run-script parameter */
    runscript = get_strparm(mgr_cli_valset, YANGCLI_MOD, YANGCLI_RUN_SCRIPT);

    /* get the run-command parameter */
    runcommand = get_strparm(mgr_cli_valset, YANGCLI_MOD, YANGCLI_RUN_COMMAND);

    /* get the timeout parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_TIMEOUT);
    if (parm && parm->res == NO_ERR) {
        default_timeout = VAL_UINT(parm);

        /* save to the connect_valset parmset */
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    } else {
        default_timeout = YANGCLI_DEF_TIMEOUT;
    }

    /* get the user name */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_USER);
    if (parm && parm->res == NO_ERR) {
        res = add_clone_parm(parm, connect_valset);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get the version parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, NCX_EL_VERSION);
    if (parm && parm->res == NO_ERR) {
        versionmode = TRUE;
    }

    /* get the match-names parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_MATCH_NAMES);
    if (parm && parm->res == NO_ERR) {
        match_names = ncx_get_name_match_enum(VAL_ENUM_NAME(parm));
        if (match_names == NCX_MATCH_NONE) {
            return ERR_NCX_INVALID_VALUE;
        }
    } else {
        match_names = NCX_MATCH_EXACT;
    }

    /* get the alt-names parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_ALT_NAMES);
    if (parm && parm->res == NO_ERR) {
        alt_names = VAL_BOOL(parm);
    } else {
        alt_names = FALSE;
    }

    /* get the force-target parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_FORCE_TARGET);
    if (parm && parm->res == NO_ERR) {
        force_target = VAL_ENUM_NAME(parm);
    } else {
        force_target = NULL;
    }

    /* get the use-xmlheader parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_USE_XMLHEADER);
    if (parm && parm->res == NO_ERR) {
        use_xmlheader = VAL_BOOL(parm);
    } else {
        use_xmlheader = TRUE;
    }

    /* get the uservars-file parameter */
    parm = val_find_child(mgr_cli_valset, YANGCLI_MOD, YANGCLI_USERVARS_FILE);
    if (parm && parm->res == NO_ERR) {
        uservars_file = VAL_STR(parm);
    } else {
        uservars_file = YANGCLI_DEF_USERVARS_FILE;
    }

    return NO_ERR;

} /* process_cli_input */


/********************************************************************
 * FUNCTION load_base_schema 
 * 
 * Load the following YANG modules:
 *   yangcli
 *   yuma-netconf
 *
 * RETURNS:
 *     status
 *********************************************************************/
static status_t
    load_base_schema (void)
{
    status_t   res;

    log_debug2("\nyangcli: Loading NCX yangcli-cli Parmset");

    /* load in the server boot parameter definition file */
    res = ncxmod_load_module(YANGCLI_MOD, 
                             NULL, 
                             NULL,
                             &yangcli_mod);
    if (res != NO_ERR) {
        return res;
    }

    /* load in the NETCONF data types and RPC methods */
    res = ncxmod_load_module(NCXMOD_NETCONF, 
                             NULL, 
                             NULL,
                             &netconf_mod);
    if (res != NO_ERR) {
        return res;
    }

    /* load the netconf-state module to use
     * the <get-schema> operation 
     */
    res = ncxmod_load_module(NCXMOD_IETF_NETCONF_STATE, 
                             NULL,
                             NULL,
                             NULL);
    if (res != NO_ERR) {
        return res;
    }


    return NO_ERR;

}  /* load_base_schema */


/********************************************************************
 * FUNCTION load_core_schema 
 * 
 * Load the following YANG modules:
 *   yuma-xsd
 *   yuma-types
 *
 * RETURNS:
 *     status
 *********************************************************************/
static status_t
    load_core_schema (void)
{
    status_t   res;

    log_debug2("\nNcxmgr: Loading NETCONF Module");

    /* load in the XSD data types */
    res = ncxmod_load_module(XSDMOD, 
                             NULL, 
                             NULL,
                             NULL);
    if (res != NO_ERR) {
        return res;
    }

    /* load in the NCX data types */
    res = ncxmod_load_module(NCXDTMOD, 
                             NULL, 
                             NULL,
                             NULL);
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

}  /* load_core_schema */



/********************************************************************
* FUNCTION namespace_mismatch_warning
* 
* Issue a namespace mismatch warning
*
* INPUTS:
*  module == module name
*  revision == revision date (may be NULL)
*  namespacestr == server expected URI
*  clientns == client provided URI
*********************************************************************/
static void
    namespace_mismatch_warning (const xmlChar *module,
                                const xmlChar *revision,
                                const xmlChar *namespacestr,
                                const xmlChar *clientns)
{
    /* !!! FIXME: need a warning number for suppression */
    log_warn("\nWarning: module namespace URI mismatch:"
             "\n   module:    '%s'"
             "\n   revision:  '%s'"
             "\n   server ns: '%s'"
             "\n   client ns: '%s'",
             module,
             (revision) ? revision : EMPTY_STRING,
             namespacestr,
             clientns);

}  /* namespace_mismatch_warning */


/********************************************************************
* FUNCTION check_module_capabilities
* 
* Check the modules reported by the server
* If autoload is TRUE then load any missing modules
* otherwise just warn which modules are missing
* Also check for wrong module module and version errors
*
* The server_cb->searchresultQ is filled with
* records for each module or deviation specified in
* the module capability URIs.
*
* After determining all the files that the server has,
* the <get-schema> operation is used (if :schema-retrieval
* advertised by the device and --autoload=true)
*
* All the files are copied into the session work directory
* to make sure the correct versions are used when compiling
* these files and applying features and deviations
*
* All files are compiled against the versions of the imports
* advertised in the capabilities, to make sure that imports
* without revision-date statements will still select the
* same revision as the server (INSTEAD OF ALWAYS SELECTING 
* THE LATEST VERSION).
*
* If the device advertises an incomplete set of modules,
* then searches for any missing imports will be done
* using the normal search path, including YUMA_MODPATH.
*
* INPUTS:
*  server_cb == server control block to use
*  scb == session control block
*
*********************************************************************/
static void
    check_module_capabilities (server_cb_t *server_cb,
                               ses_cb_t *scb)
{
    mgr_scb_t              *mscb;
    ncx_module_t           *mod;
    cap_rec_t              *cap;
    const xmlChar          *module, *revision, *namespacestr;
    ncxmod_search_result_t *searchresult, *libresult;
    status_t                res;
    boolean                 retrieval_supported;

    mscb = (mgr_scb_t *)scb->mgrcb;

    log_info("\n\nChecking Server Modules...\n");

    if (!cap_std_set(&mscb->caplist, CAP_STDID_V1)) {
        log_warn("\nWarning: NETCONF v1 capability not found");
    }

    retrieval_supported = cap_set(&mscb->caplist,
                                  CAP_SCHEMA_RETRIEVAL);

    /* check all the YANG modules;
     * build a list of modules that
     * the server needs to get somehow
     * or proceed without them
     * save the results in the server_cb->searchresultQ
     */
    cap = cap_first_modcap(&mscb->caplist);
    while (cap) {
        mod = NULL;
        module = NULL;
        revision = NULL;
        namespacestr = NULL;
        libresult = NULL;

        cap_split_modcap(cap,
                         &module,
                         &revision,
                         &namespacestr);

        if (namespacestr == NULL) {
            /* try the entire base part of the URI if there was
             * no module capability parsed
             */
            namespacestr = cap->cap_uri;
        }

        if (namespacestr == NULL) {
            if (ncx_warning_enabled(ERR_NCX_RCV_INVALID_MODCAP)) {
                log_warn("\nWarning: skipping enterprise capability "
                         "for URI '%s'", 
                         cap->cap_uri);
            }
            cap = cap_next_modcap(cap);
            continue;
        }

        if (module == NULL) {
            /* check if there is a module in the modlibQ that
             * has the same namespace URI as 'namespacestr' base
             */
            libresult = ncxmod_find_search_result(&modlibQ,
                                                  NULL,
                                                  NULL,
                                                  namespacestr);
            if (libresult != NULL) {
                module = libresult->module;
                revision = libresult->revision;
            } else {
                /* nothing found and modname is NULL, so continue */
                if (ncx_warning_enabled(ERR_NCX_RCV_INVALID_MODCAP)) {
                    log_warn("\nWarning: skipping enterprise capability "
                             "for URI '%s'", 
                             cap->cap_uri);
                }
                cap = cap_next_modcap(cap);
                continue;
            }
        }

        mod = ncx_find_module(module, revision);
        if (mod != NULL) {
            /* make sure that the namespace URIs match */
            if (ncx_compare_base_uris(mod->ns, namespacestr)) {
                namespace_mismatch_warning(module,
                                           revision,
                                           namespacestr,
                                           mod->ns);
                /* force a new search or auto-load */
                mod = NULL;
            }
        }

        if (mod == NULL && module != NULL) {
            /* check if there is a module in the modlibQ that
             * has the same namespace URI as 'namespacestr' base
             */
            libresult = ncxmod_find_search_result(&modlibQ,
                                                  NULL,
                                                  NULL,
                                                  namespacestr);
            if (libresult != NULL) {
                module = libresult->module;
                revision = libresult->revision;
            }
        }

        if (mod == NULL) {
            /* module was not found in the module search path
             * of this instance of yangcli
             * try to auto-load the module if enabled
             */
            if (server_cb->autoload) {
                if (libresult) {
                    searchresult = ncxmod_clone_search_result(libresult);
                    if (searchresult == NULL) {
                        log_error("\nError: cannot load file, "
                                  "malloc failed");
                        return;
                    }
                } else {
                    searchresult = ncxmod_find_module(module, revision);
                }
                if (searchresult) {
                    searchresult->cap = cap;
                    if (searchresult->res != NO_ERR) {
                        if (LOGDEBUG2) {
                            log_debug2("\nLocal module search failed (%s)",
                                       get_error_string(searchresult->res));
                        }
                    } else if (searchresult->source) {
                        /* module with matching name, revision
                         * was found on the local system;
                         * check if the namespace also matches
                         */
                        if (searchresult->namespacestr) {
                            if (ncx_compare_base_uris
                                (searchresult->namespacestr, namespacestr)) {
                                /* cannot use this local file because
                                 * it has a different namespace
                                 */
                                namespace_mismatch_warning
                                    (module,
                                     revision,
                                     namespacestr,
                                     searchresult->namespacestr);
                            } else {
                                /* can use the local system file found */
                                searchresult->capmatch = TRUE;
                            }
                        } else {
                            /* this module found is invalid;
                             * has no namespace statement
                             */
                            log_error("\nError: found module '%s' "
                                      "revision '%s' "
                                      "has no namespace-stmt",
                                      module,
                                      (revision) ? revision : EMPTY_STRING);
                        }
                    } else {
                        /* the search result is valid, but no source;
                         * specified module is not available
                         * on the manager platform; see if the
                         * get-schema operation is available
                         */
                        if (!retrieval_supported) {
                            /* no <get-schema> so SOL, do without this module 
                             * !!! FIXME: need warning number 
                             */
                            if (revision != NULL) {
                                log_warn("\nWarning: module '%s' "
                                         "revision '%s' not available",
                                         module,
                                         revision);
                            } else {
                                log_warn("\nWarning: module '%s' "
                                         "(no revision) not available",
                                         module);
                            }
                        } else if (LOGDEBUG) {
                            log_debug("\nautoload: Module '%s' not available,"
                                      " will try <get-schema>",
                                      module);
                        }
                    }

                    /* save the search result no matter what */
                    dlq_enque(searchresult, &server_cb->searchresultQ);
                } else {
                    /* libresult was NULL, so there was no searchresult
                     * ncxmod did not find any YANG module with this namespace
                     * the module is not available
                     */
                    if (!retrieval_supported) {
                        /* no <get-schema> so SOL, do without this module 
                         * !!! need warning number 
                         */
                        if (revision != NULL) {
                            log_warn("\nWarning: module '%s' "
                                     "revision '%s' not available",
                                     module,
                                     revision);
                        } else {
                            log_warn("\nWarning: module '%s' "
                                     "(no revision) not available",
                                     module);
                        }
                    } else {
                        /* setup a blank searchresult so auto-load
                         * will attempt to retrieve it
                         */
                        searchresult = 
                            ncxmod_new_search_result_str(module, 
                                                         revision);
                        if (searchresult) {
                            searchresult->cap = cap;
                            dlq_enque(searchresult, &server_cb->searchresultQ);
                            if (LOGDEBUG) {
                                log_debug("\nyangcli_autoload: Module '%s' "
                                          "not available, will try "
                                          "<get-schema>",
                                          module);
                            }
                        } else {
                            log_error("\nError: cannot load file, "
                                      "malloc failed");
                            return;
                        }
                    }
                }
            } else {
                /* --autoload=false */
                if (LOGINFO) {
                    log_info("\nModule '%s' "
                             "revision '%s' not "
                             "loaded, autoload disabled",
                             module,
                             (revision) ? revision : EMPTY_STRING);
                }
            }
        } else {
            /* since the module was already loaded, it is
             * OK to use, even if --autoload=false
             * just copy the info into a search result record
             * so it will be copied and recompiled with the 
             * correct features and deviations applied
             */
            searchresult = ncxmod_new_search_result_ex(mod);
            if (searchresult == NULL) {
                log_error("\nError: cannot load file, malloc failed");
                return;
            } else {
                searchresult->cap = cap;
                searchresult->capmatch = TRUE;
                dlq_enque(searchresult, &server_cb->searchresultQ);
            }
        }

        /* move on to the next module */
        cap = cap_next_modcap(cap);
    }

    /* get all the advertised YANG data model modules into the
     * session temp work directory that are local to the system
     */
    res = autoload_setup_tempdir(server_cb, scb);
    if (res != NO_ERR) {
        log_error("\nError: autoload setup temp files failed (%s)",
                  get_error_string(res));
    }

    /* go through all the search results (if any)
     * and see if <get-schema> is needed to pre-load
     * the session work directory YANG files
     */
    if (res == NO_ERR &&
        retrieval_supported && 
        server_cb->autoload) {

        /* compile phase will be delayed until autoload
         * get-schema operations are done
         */
        res = autoload_start_get_modules(server_cb, scb);
        if (res != NO_ERR) {
            log_error("\nError: autoload get modules failed (%s)",
                      get_error_string(res));
        }
    }

    /* check autoload state did not start or was not requested */
    if (res == NO_ERR && server_cb->command_mode != CMD_MODE_AUTOLOAD) {
        /* parse and hold the modules with the correct deviations,
         * features and revisions.  The returned modules
         * from yang_parse.c will not be stored in the local module
         * directory -- just used for this one session then deleted
         */
        res = autoload_compile_modules(server_cb, scb);
        if (res != NO_ERR) {
            log_error("\nError: autoload compile modules failed (%s)",
                      get_error_string(res));
        }
    }

} /* check_module_capabilities */


/********************************************************************
* message_timed_out
*
* Check if the request in progress (!) has timed out
* TBD: check for a specific message if N requests per
* session are ever supported
*
* INPUTS:
*   scb == session control block to use
*
* RETURNS:
*   TRUE if any messages have timed out
*   FALSE if no messages have timed out
*********************************************************************/
static boolean
    message_timed_out (ses_cb_t *scb)
{
    mgr_scb_t    *mscb;
    uint32        deletecount;

    mscb = (mgr_scb_t *)scb->mgrcb;

    if (mscb) {
        deletecount = mgr_rpc_timeout_requestQ(&mscb->reqQ);
        if (deletecount) {
            log_error("\nError: request to server timed out");
        }
        return (deletecount) ? TRUE : FALSE;
    }

    /* else mgr_shutdown could have been issued via control-C
     * and the session control block has been deleted
     */
    return FALSE;

}  /* message_timed_out */


/********************************************************************
* FUNCTION get_lock_worked
* 
* Check if the get-locks function ended up with
* all its locks or not
* 
* INPUTS:
*  server_cb == server control block to use
*
*********************************************************************/
static boolean
    get_lock_worked (server_cb_t *server_cb)
{
    ncx_cfg_t  cfgid;

    for (cfgid = NCX_CFGID_RUNNING;
         cfgid <= NCX_CFGID_STARTUP;
         cfgid++) {
        if (server_cb->lock_cb[cfgid].lock_used &&
            server_cb->lock_cb[cfgid].lock_state !=
            LOCK_STATE_ACTIVE) {
            return FALSE;
        }
    }

    return TRUE;

}  /* get_lock_worked */


/********************************************************************
* FUNCTION get_input_line
 *
 * get a line of input text from the appropriate source
 *
 * INPUTS:
 *   server_sb == server control block to use
 *   res == address of return status
 *
 * OUTPUTS:
 *   *res == return status
 *
 * RETURNS:
 *    pointer to the line to use; do not free this memory
*********************************************************************/
static xmlChar *
    get_input_line (server_cb_t *server_cb,
                     status_t *res)
{
    xmlChar        *line;
    boolean         done;
    runstack_src_t  rsrc;
    
    /* get a line of user input
     * this is really a const pointer
     */
    *res = NO_ERR;
    done = FALSE;
    line = NULL;

    while (!done && *res == NO_ERR) {
        /* figure out where to get the next input line */
        rsrc = runstack_get_source(server_cb->runstack_context);

        switch (rsrc) {
        case RUNSTACK_SRC_NONE:
            *res = SET_ERROR(ERR_INTERNAL_VAL);
            break;
        case RUNSTACK_SRC_USER:
            /* block until user enters some input */
            line = get_cmd_line(server_cb, res);
            break;
        case RUNSTACK_SRC_SCRIPT:
            /* get one line of script text */
            line = runstack_get_cmd(server_cb->runstack_context, res);
            break;
        case RUNSTACK_SRC_LOOP:
            /* get one line of script text */
            line = runstack_get_loop_cmd(server_cb->runstack_context,
                                         res);
            if (line == NULL || *res != NO_ERR) {
                if (*res == ERR_NCX_LOOP_ENDED) {
                    /* did not get a loop line, try again */
                    *res = NO_ERR;
                    continue;
                }
                return NULL;
            }
            break;
        default:
            *res = SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (*res == NO_ERR) {
            *res = runstack_save_line(server_cb->runstack_context,
                                      line);
        }
        /* only 1 time through loop in normal conditions */
        done = TRUE;
    }
    return line;

}  /* get_input_line */


/********************************************************************
 * FUNCTION do_bang_recall (local !string)
 * 
 * Do Command line history support operations
 *
 * !sget-config target=running
 * !42
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    line == CLI input in progress
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    do_bang_recall (server_cb_t *server_cb,
                    const xmlChar *line)
{
    status_t            res;

    res = NO_ERR;

    if (line && 
        *line == YANGCLI_RECALL_CHAR && 
        line[1] != 0) {
        if (isdigit(line[1])) {
            /* assume the form is !42 to recall by line number */
            ncx_num_t num;
            ncx_init_num(&num);
            res = ncx_decode_num(&line[1], NCX_BT_UINT64, &num);
            if (res != NO_ERR) {
                log_error("\nError: invalid number '%s'", 
                          get_error_string(res));
            } else {
                res = do_line_recall(server_cb, num.ul);
            }
            ncx_clean_num(NCX_BT_UINT64, &num);
        } else {
            /* assume form is !command string and recall by
             * most recent match of the partial command line given
             */
            res = do_line_recall_string(server_cb, &line[1]);
        }
    } else {
        res = ERR_NCX_MISSING_PARM;
        log_error("\nError: missing recall string\n");
    }

    return res;

}  /* do_bang_recall */


/********************************************************************
* yangcli_stdin_handler
*
* Temp: Calling readline which will block other IO while the user
*       is editing the line.  This is okay for this CLI application
*       but not multi-session applications;
* Need to prevent replies from popping up on the screen
* while new commands are being edited anyway
*
* RETURNS:
*   new program state
*********************************************************************/
static mgr_io_state_t
    yangcli_stdin_handler (void)
{
    server_cb_t     *server_cb;
    xmlChar        *line;
    const xmlChar  *resultstr;
    ses_cb_t       *scb;
    mgr_scb_t      *mscb;
    boolean         getrpc, fileassign, done;
    status_t        res;
    uint32          len;
    runstack_src_t  rsrc;

    /* assuming cur_server_cb will be set dynamically
     * at some point; currently 1 session supported in yangcli
     */
    server_cb = cur_server_cb;

    if (server_cb->returncode == MGR_IO_RC_WANTDATA) {
        if (LOGDEBUG2) {
            log_debug2("\nyangcli: sending dummy <get> keepalive");
        }
        (void)send_keepalive_get(server_cb);
        server_cb->returncode = MGR_IO_RC_NONE;        
    }

    if (server_cb->cli_fn == NULL && !server_cb->climore) {
        init_completion_state(&server_cb->completion_state,
                              server_cb, 
                              CMD_STATE_FULL);
    }

    if (mgr_shutdown_requested()) {
        server_cb->state = MGR_IO_ST_SHUT;
    }

    switch (server_cb->state) {
    case MGR_IO_ST_INIT:
        return server_cb->state;
    case MGR_IO_ST_IDLE:
        break;
    case MGR_IO_ST_CONN_IDLE:
        /* check if session was dropped by remote peer */
        scb = mgr_ses_get_scb(server_cb->mysid);
        if (scb==NULL || scb->state == SES_ST_SHUTDOWN_REQ) {
            if (scb) {
                (void)mgr_ses_free_session(server_cb->mysid);
            }
            clear_server_cb_session(server_cb);
        } else  {
            res = NO_ERR;
            mscb = (mgr_scb_t *)scb->mgrcb;
            ncx_set_temp_modQ(&mscb->temp_modQ);

            /* check locks timeout */
            if (!(server_cb->command_mode == CMD_MODE_NORMAL ||
                  server_cb->command_mode == CMD_MODE_AUTOLOAD)) {

                if (check_locks_timeout(server_cb)) {
                    res = ERR_NCX_TIMEOUT;

                    if (runstack_level(server_cb->runstack_context)) {
                        runstack_cancel(server_cb->runstack_context);
                    }

                    switch (server_cb->command_mode) {
                    break;
                    case CMD_MODE_AUTODISCARD:
                    case CMD_MODE_AUTOLOCK:
                        handle_locks_cleanup(server_cb);
                        if (server_cb->command_mode != CMD_MODE_NORMAL) {
                            return server_cb->state;
                        }
                        break;
                    case CMD_MODE_AUTOUNLOCK:
                        clear_lock_cbs(server_cb);
                        break;
                    case CMD_MODE_AUTOLOAD:
                    case CMD_MODE_NORMAL:
                    default:
                        SET_ERROR(ERR_INTERNAL_VAL);
                    }
                } else if (server_cb->command_mode == CMD_MODE_AUTOLOCK) {
                    if (server_cb->locks_waiting) {
                        server_cb->command_mode = CMD_MODE_AUTOLOCK;
                        done = FALSE;
                        res = handle_get_locks_request_to_server(server_cb,
                                                                FALSE,
                                                                &done);
                        if (done) {
                            /* check if the locks are all good */
                            if (get_lock_worked(server_cb)) {
                                log_info("\nget-locks finished OK");
                                server_cb->command_mode = CMD_MODE_NORMAL;
                                server_cb->locks_waiting = FALSE;
                            } else {
                                log_error("\nError: get-locks failed, "
                                          "starting cleanup");
                                handle_locks_cleanup(server_cb);
                            }
                        }
                    }
                }
            }
        }
        break;
    case MGR_IO_ST_CONN_START:
        /* waiting until <hello> processing complete */
        scb = mgr_ses_get_scb(server_cb->mysid);
        if (scb == NULL) {
            /* session startup failed */
            server_cb->state = MGR_IO_ST_IDLE;
            if (batchmode) {
                mgr_request_shutdown();
                return server_cb->state;
            }
        } else if (scb->state == SES_ST_IDLE 
                   && dlq_empty(&scb->outQ)) {
            /* incoming hello OK and outgoing hello is sent */
            server_cb->state = MGR_IO_ST_CONN_IDLE;
            report_capabilities(server_cb, scb, TRUE, HELP_MODE_NONE);
            check_module_capabilities(server_cb, scb);
            mscb = (mgr_scb_t *)scb->mgrcb;
            ncx_set_temp_modQ(&mscb->temp_modQ);
        } else {
            /* check timeout */
            if (message_timed_out(scb)) {
                server_cb->state = MGR_IO_ST_IDLE;
                if (batchmode) {
                    mgr_request_shutdown();
                    return server_cb->state;
                }
                break;
            } /* else still setting up session */
            return server_cb->state;
        }
        break;
    case MGR_IO_ST_CONN_RPYWAIT:
        /* check if session was dropped by remote peer */
        scb = mgr_ses_get_scb(server_cb->mysid);
        if (scb==NULL || scb->state == SES_ST_SHUTDOWN_REQ) {
            if (scb) {
                (void)mgr_ses_free_session(server_cb->mysid);
            }
            clear_server_cb_session(server_cb);
        } else  {
            res = NO_ERR;
            mscb = (mgr_scb_t *)scb->mgrcb;
            ncx_set_temp_modQ(&mscb->temp_modQ);

            /* check timeout */
            if (message_timed_out(scb)) {
                res = ERR_NCX_TIMEOUT;
            } else if (!(server_cb->command_mode == CMD_MODE_NORMAL ||
                         server_cb->command_mode == CMD_MODE_AUTOLOAD ||
                         server_cb->command_mode == CMD_MODE_SAVE)) {
                if (check_locks_timeout(server_cb)) {
                    res = ERR_NCX_TIMEOUT;
                }
            }

            if (res != NO_ERR) {
                if (runstack_level(server_cb->runstack_context)) {
                    runstack_cancel(server_cb->runstack_context);
                }
                server_cb->state = MGR_IO_ST_CONN_IDLE;

                switch (server_cb->command_mode) {
                case CMD_MODE_NORMAL:
                    break;
                case CMD_MODE_AUTOLOAD:
                    /* even though a timeout occurred
                     * attempt to compile the modules
                     * that are already present
                     */
                    autoload_compile_modules(server_cb, scb);
                    break;
                case CMD_MODE_AUTODISCARD:
                    server_cb->command_mode = CMD_MODE_AUTOLOCK;
                    /* fall through */
                case CMD_MODE_AUTOLOCK:
                    handle_locks_cleanup(server_cb);
                    if (server_cb->command_mode != CMD_MODE_NORMAL) {
                        return server_cb->state;
                    }
                    break;
                case CMD_MODE_AUTOUNLOCK:
                    clear_lock_cbs(server_cb);
                    break;
                case CMD_MODE_SAVE:
                    server_cb->command_mode = CMD_MODE_NORMAL;
                    break;
                default:
                    SET_ERROR(ERR_INTERNAL_VAL);
                }
            } else {
                /* keep waiting for reply */
                return server_cb->state;
            }
        }
        break;
    case MGR_IO_ST_CONNECT:
    case MGR_IO_ST_SHUT:
    case MGR_IO_ST_CONN_CANCELWAIT:
    case MGR_IO_ST_CONN_SHUT:
    case MGR_IO_ST_CONN_CLOSEWAIT:
        /* check timeout */
        scb = mgr_ses_get_scb(server_cb->mysid);
        if (scb==NULL || scb->state == SES_ST_SHUTDOWN_REQ) {
            if (scb) {
                (void)mgr_ses_free_session(server_cb->mysid);
            }
            clear_server_cb_session(server_cb);
        } else if (message_timed_out(scb)) {
            clear_server_cb_session(server_cb);
        }

        /* do not accept chars in these states */
        return server_cb->state;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return server_cb->state;
    }

    /* check if some sort of auto-mode, and the
     * CLI is skipped until that mode is done
     */
    if (server_cb->command_mode != CMD_MODE_NORMAL) {
        return server_cb->state;
    }

    /* check the run-script parameters */
    if (runscript) {
        if (!runscriptdone) {
            runscriptdone = TRUE;
            (void)do_startup_script(server_cb, runscript);
            /* drop through and get input line from runstack */
        }
    } else if (runcommand) {
        if (!runcommanddone) {
            runcommanddone = TRUE;
            (void)do_startup_command(server_cb, runcommand);
            /* exit now in case there was a session started up and a remote
             * command sent as part of run-command.  
             * May need to let write_sessions() run in mgr_io.c.
             * If not, 2nd loop through this fn will hit get_input_line
             */
            return server_cb->state;            
        } else if (batchmode) {
            /* run command is done at this point */
            mgr_request_shutdown();
            return server_cb->state;
        }
    }

    /* get a line of user input
     * this is really a const pointer
     */
    res = NO_ERR;
    line = get_input_line(server_cb, &res);

    /* figure out where to get the next input line */
    rsrc = runstack_get_source(server_cb->runstack_context);

    runstack_clear_cancel(server_cb->runstack_context);

    switch (rsrc) {
    case RUNSTACK_SRC_NONE:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case RUNSTACK_SRC_USER:
        if (line == NULL) {
            /* could have just transitioned from script mode to
             * user mode; check batch-mode exit;
             */
            if (batchmode) {
                mgr_request_shutdown();
            }
            return server_cb->state;
        }
        break;
    case RUNSTACK_SRC_SCRIPT:
        /* get one line of script text */
        if (line == NULL || res != NO_ERR) {
            if (res == ERR_NCX_EOF) {
                ;
            }
            if (batchmode && res != NO_ERR) {
                mgr_request_shutdown();
            }
            return server_cb->state;
        }
        break;
    case RUNSTACK_SRC_LOOP:
        if (line==NULL || res != NO_ERR) {
            if (batchmode && res != NO_ERR) {
                mgr_request_shutdown();
            }
            return server_cb->state;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
        return server_cb->state;
    }        

    /* check bang recall statement */
    if (*line == YANGCLI_RECALL_CHAR) {
        res = do_bang_recall(server_cb, line);
        return server_cb->state;
    }

    /* check if this is an assignment statement */
    res = check_assign_statement(server_cb, 
                                 line, 
                                 &len, 
                                 &getrpc,
                                 &fileassign);
    if (res != NO_ERR) {
        log_error("\nyangcli: Variable assignment failed (%s) (%s)",
                  line, 
                  get_error_string(res));

        if (runstack_level(server_cb->runstack_context)) {
            runstack_cancel(server_cb->runstack_context);
        }

        switch (server_cb->command_mode) {
            break;
        case CMD_MODE_AUTODISCARD:
        case CMD_MODE_AUTOLOCK:
            handle_locks_cleanup(server_cb);
            break;
        case CMD_MODE_AUTOUNLOCK:
            clear_lock_cbs(server_cb);
            break;
        case CMD_MODE_AUTOLOAD:
        case CMD_MODE_NORMAL:
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    } else if (getrpc) {
        res = NO_ERR;
        switch (server_cb->state) {
        case MGR_IO_ST_IDLE:
            /* waiting for top-level commands */
            res = top_command(server_cb, &line[len]);
            break;
        case MGR_IO_ST_CONN_IDLE:
            /* waiting for session commands */
            res = conn_command(server_cb, &line[len]);
            break;
        case MGR_IO_ST_CONN_RPYWAIT:
            /* waiting for RPC reply while more input typed */
            break;
        case MGR_IO_ST_CONN_CANCELWAIT:
            break;
        default:
            break;
        }

        if (res != NO_ERR) {
            if (runstack_level(server_cb->runstack_context)) {
                runstack_cancel(server_cb->runstack_context);
            }
        }

        switch (server_cb->state) {
        case MGR_IO_ST_IDLE:
        case MGR_IO_ST_CONN_IDLE:
            /* check assignment statement active */
            if (server_cb->result_name != NULL || 
                 server_cb->result_filename != NULL) {
                if (server_cb->local_result != NULL) {
                    val_value_t  *resval = server_cb->local_result;
                    server_cb->local_result = NULL;
                    /* pass off malloced local_result here */
                    res = finish_result_assign(server_cb, 
                                               resval,
                                               NULL);
                    /* clear_result already called */
                } else {
                    /* save the filled in value */
                    resultstr = (res == NO_ERR) ? 
                        (const xmlChar *)"ok" :
                        (const xmlChar *)get_error_string(res);
                    res = finish_result_assign(server_cb, 
                                               NULL,
                                               resultstr);
                    /* clear_result already called */
                }
            } else {
                clear_result(server_cb);
            }
            break;
        default:
            ;
        }
    } else {
        log_info("\nOK\n");
    }
    
    return server_cb->state;

} /* yangcli_stdin_handler */


/********************************************************************
 * FUNCTION yangcli_notification_handler
 * 
 * matches callback template mgr_not_cbfn_t
 *
 * INPUTS:
 *   scb == session receiving RPC reply
 *   msg == notification msg that was parsed
 *   consumed == address of return consumed message flag
 *
 *  OUTPUTS:
 *     *consumed == TRUE if msg has been consumed so
 *                  it will not be freed by mgr_not_dispatch
 *               == FALSE if msg has been not consumed so
 *                  it will be freed by mgr_not_dispatch
 *********************************************************************/
static void
    yangcli_notification_handler (ses_cb_t *scb,
                                  mgr_not_msg_t *msg,
                                  boolean *consumed)
{
    server_cb_t   *server_cb;
    mgr_scb_t    *mgrcb;

#ifdef DEBUG
    if (!scb || !msg || !consumed) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    *consumed = FALSE;
    mgrcb = scb->mgrcb;
    if (mgrcb) {
        ncx_set_temp_modQ(&mgrcb->temp_modQ);
    }

    /***  TBD: multi-session support ***/
    server_cb = cur_server_cb;

    /* check the contents of the notification */
    if (msg && msg->notification) {
        if (LOGWARN) {
            gl_normal_io(server_cb->cli_gl);
            if (LOGINFO) {
                log_info("\n\nIncoming notification:");
                val_dump_value_max(msg->notification, 
                                   0,
                                   server_cb->defindent,
                                   DUMP_VAL_LOG,
                                   server_cb->display_mode,
                                   FALSE,
                                   FALSE);
                log_info("\n\n");
            } else if (msg->eventType) {
                log_warn("\n\nIncoming <%s> "
                         "notification\n\n", 
                         msg->eventType->name);
            }
        }

        /* Log the notification by removing it from the message
         * and storing it in the server notification log
         */
        dlq_enque(msg, &server_cb->notificationQ);
        *consumed = TRUE;
    }
    
}  /* yangcli_notification_handler */


/********************************************************************
 * FUNCTION yangcli_init
 * 
 * Init the NCX CLI application
 * 
 * INPUTS:
 *   argc == number of strings in argv array
 *   argv == array of command line strings
 * 
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    yangcli_init (int argc,
                  char *argv[])
{
    obj_template_t       *obj;
    server_cb_t          *server_cb;
    val_value_t          *parm, *modval;
    xmlChar              *savestr, *revision;
    status_t              res;
    uint32                modlen;
    log_debug_t           log_level;
    dlq_hdr_t             savedevQ;
    xmlChar               versionbuffer[NCX_VERSION_BUFFSIZE];

#ifdef YANGCLI_DEBUG
    int   i;
#endif

    /* set the default debug output level */
    log_level = LOG_DEBUG_INFO;

    dlq_createSQue(&savedevQ);

    /* init the module static vars */
    dlq_createSQue(&server_cbQ);
    cur_server_cb = NULL;
    yangcli_mod = NULL;
    netconf_mod = NULL;
    mgr_cli_valset = NULL;
    batchmode = FALSE;
    helpmode = FALSE;
    helpsubmode = HELP_MODE_NONE;
    versionmode = FALSE;
    runscript = NULL;
    runscriptdone = FALSE;
    runcommand = NULL;
    runcommanddone = FALSE;
    dlq_createSQue(&mgrloadQ);
    aliases_file = YANGCLI_DEF_ALIASES_FILE;
    autoaliases = TRUE;
    autocomp = TRUE;
    autohistory = TRUE;
    autoload = TRUE;
    autouservars = TRUE;
    baddata = YANGCLI_DEF_BAD_DATA;
    connect_valset = NULL;
    confname = NULL;
    default_module = NULL;
    default_timeout = 30;
    display_mode = NCX_DISPLAY_MODE_PLAIN;
    fixorder = TRUE;
    optional = FALSE;
    testoption = YANGCLI_DEF_TEST_OPTION;
    erroption = YANGCLI_DEF_ERROR_OPTION;
    defop = YANGCLI_DEF_DEFAULT_OPERATION;
    withdefaults = YANGCLI_DEF_WITH_DEFAULTS;
    echo_replies = TRUE;
    time_rpcs = FALSE;
    uservars_file = YANGCLI_DEF_USERVARS_FILE;
    temp_progcb = NULL;
    dlq_createSQue(&modlibQ);
    dlq_createSQue(&aliasQ);

    /* set the character set LOCALE to the user default */
    setlocale(LC_CTYPE, "");

    /* initialize the NCX Library first to allow NCX modules
     * to be processed.  No module can get its internal config
     * until the NCX module parser and definition registry is up
     */
    res = ncx_init(NCX_SAVESTR, log_level, TRUE, NULL, argc, argv);
    if (res != NO_ERR) {
        return res;
    }

#ifdef YANGCLI_DEBUG
    if (argc>1 && LOGDEBUG2) {
        log_debug2("\nCommand line parameters:");
        for (i=0; i<argc; i++) {
            log_debug2("\n   arg%d: %s", i, argv[i]);
        }
    }
#endif

    /* make sure the Yuma directory
     * exists for saving per session data
     */
    res = ncxmod_setup_yumadir();
    if (res != NO_ERR) {
        log_error("\nError: could not setup yuma dir '%s'",
                  ncxmod_get_yumadir());
        return res;
    }

    /* make sure the Yuma temp directory
     * exists for saving per session data
     */
    res = ncxmod_setup_tempdir();
    if (res != NO_ERR) {
        log_error("\nError: could not setup temp dir '%s/tmp'",
                  ncxmod_get_yumadir());
        return res;
    }

    /* at this point, modules that need to read config
     * params can be initialized
     */

    /* Load the yangcli base module */
    res = load_base_schema();
    if (res != NO_ERR) {
        return res;
    }

    /* Initialize the Netconf Manager Library */
    res = mgr_init();
    if (res != NO_ERR) {
        return res;
    }

    /* set up handler for incoming notifications */
    mgr_not_set_callback_fn(yangcli_notification_handler);

    /* init the connect parmset object template;
     * find the connect RPC method
     * !!! MUST BE AFTER load_base_schema !!!
     */
    obj = ncx_find_object(yangcli_mod, YANGCLI_CONNECT);
    if (obj==NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* set the parmset object to the input node of the RPC */
    obj = obj_find_child(obj, NULL, YANG_K_INPUT);
    if (obj==NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* treat the connect-to-server parmset special
     * it is saved for auto-start plus restart parameters
     * Setup an empty parmset to hold the connect parameters
     */
    connect_valset = val_new_value();
    if (connect_valset==NULL) {
        return ERR_INTERNAL_MEM;
    } else {
        val_init_from_template(connect_valset, obj);
    }

    /* create the program instance temporary directory */
    temp_progcb = ncxmod_new_program_tempdir(&res);
    if (temp_progcb == NULL || res != NO_ERR) {
        return res;
    }

    /* set the CLI handler */
    mgr_io_set_stdin_handler(yangcli_stdin_handler);

    /* create a default server control block */
    server_cb = new_server_cb(YANGCLI_DEF_SERVER);
    if (server_cb==NULL) {
        return ERR_INTERNAL_MEM;
    }
    dlq_enque(server_cb, &server_cbQ);

    cur_server_cb = server_cb;

    /* Get any command line and conf file parameters */
    res = process_cli_input(server_cb, argc, argv);
    if (res != NO_ERR) {
        return res;
    }

    /* check print version */
    if (versionmode || helpmode) {
        res = ncx_get_version(versionbuffer, NCX_VERSION_BUFFSIZE);
        if (res == NO_ERR) {
            log_stdout("\nyangcli version %s\n", versionbuffer);
        } else {
            SET_ERROR(res);
        }
    }

    /* check print help and exit */
    if (helpmode) {
        help_program_module(YANGCLI_MOD, YANGCLI_BOOT, helpsubmode);
    }

    /* check quick exit */
    if (helpmode || versionmode) {
        return NO_ERR;
    }

    /* set any server control block defaults which were supplied
     * in the CLI or conf file
     */
    update_server_cb_vars(server_cb);

    /* Load the NETCONF, XSD, SMI and other core modules */
    if (autoload) {
        res = load_core_schema();
        if (res != NO_ERR) {
            return res;
        }
    }

    /* check if there are any deviation parameters to load first */
    for (modval = val_find_child(mgr_cli_valset, YANGCLI_MOD,
                                 NCX_EL_DEVIATION);
         modval != NULL && res == NO_ERR;
         modval = val_find_next_child(mgr_cli_valset, YANGCLI_MOD,
                                      NCX_EL_DEVIATION,
                                      modval)) {

        res = ncxmod_load_deviation(VAL_STR(modval), &savedevQ);
        if (res != NO_ERR) {
            log_error("\n load deviation failed (%s)", 
                      get_error_string(res));
        } else {
            log_info("\n load OK");
        }
    }

    if (res == NO_ERR) {
        /* check if any explicitly listed modules should be loaded */
        modval = val_find_child(mgr_cli_valset, YANGCLI_MOD, NCX_EL_MODULE);
        while (modval != NULL && res == NO_ERR) {
            log_info("\nyangcli: Loading requested module %s", 
                     VAL_STR(modval));

            revision = NULL;
            savestr = NULL;
            modlen = 0;

            if (yang_split_filename(VAL_STR(modval), &modlen)) {
                savestr = &(VAL_STR(modval)[modlen]);
                *savestr = '\0';
                revision = savestr + 1;
            }

            res = ncxmod_load_module(VAL_STR(modval), revision,
                                     &savedevQ, NULL);
            if (res != NO_ERR) {
                log_error("\n load module failed (%s)", 
                          get_error_string(res));
            } else {
                log_info("\n load OK");
            }

            modval = val_find_next_child(mgr_cli_valset, YANGCLI_MOD,
                                         NCX_EL_MODULE, modval);
        }
    }

    /* discard any deviations loaded from the CLI or conf file */
    ncx_clean_save_deviationsQ(&savedevQ);
    if (res != NO_ERR) {
        return res;
    }

    /* load the system (read-only) variables */
    res = init_system_vars(server_cb);
    if (res != NO_ERR) {
        return res;
    }

    /* load the system config variables */
    res = init_config_vars(server_cb);
    if (res != NO_ERR) {
        return res;
    }

    /* initialize the module library search result queue */
    {
        log_debug_t dbglevel = log_get_debug_level();
        if (LOGDEBUG3) {
            ; 
        } else {
            log_set_debug_level(LOG_DEBUG_NONE);
        }
        res = ncxmod_find_all_modules(&modlibQ);
        log_set_debug_level(dbglevel);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* load the user aliases */
    if (autoaliases) {
        res = load_aliases(aliases_file);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* load the user variables */
    if (autouservars) {
        res = load_uservars(server_cb, uservars_file);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* make sure the startup screen is generated
     * before the auto-connect sequence starts
     */
    do_startup_screen();    

    /* check to see if a session should be auto-started
     * --> if the server parameter is set a connect will
     * --> be attempted
     *
     * The yangcli_stdin_handler will call the finish_start_session
     * function when the user enters a line of keyboard text
     */
    server_cb->state = MGR_IO_ST_IDLE;
    if (connect_valset) {
        parm = val_find_child(connect_valset, YANGCLI_MOD, YANGCLI_SERVER);
        if (parm && parm->res == NO_ERR) {
            res = do_connect(server_cb, NULL, NULL, 0, TRUE);
            if (res != NO_ERR) {
                if (!batchmode) {
                    res = NO_ERR;
                }
            }
        }
    }

    return res;

}  /* yangcli_init */


/********************************************************************
 * FUNCTION yangcli_cleanup
 * 
 * 
 * 
 *********************************************************************/
static void
    yangcli_cleanup (void)
{
    server_cb_t  *server_cb;
    modptr_t     *modptr;
    status_t      res = NO_ERR;

    log_debug2("\nShutting down yangcli\n");

    /* save the user variables */
    if (autouservars && init_done) {
        if (cur_server_cb) {
            res = save_uservars(cur_server_cb, uservars_file);
        } else {
            res = ERR_NCX_MISSING_PARM;
        }
        if (res != NO_ERR) {
            log_error("\nError: yangcli user variables could "
                      "not be saved (%s)", get_error_string(res));
        }
    }

    /* save the user aliases */
    if (autoaliases && init_done) {
        res = save_aliases(aliases_file);
        if (res != NO_ERR) {
            log_error("\nError: yangcli command aliases could "
                      "not be saved (%s)", get_error_string(res));
        }
    }

    while (!dlq_empty(&mgrloadQ)) {
        modptr = (modptr_t *)dlq_deque(&mgrloadQ);
        free_modptr(modptr);
    }

    while (!dlq_empty(&server_cbQ)) {
        server_cb = (server_cb_t *)dlq_deque(&server_cbQ);
        free_server_cb(server_cb);
    }

    /* clean and reset all module static vars */
    if (cur_server_cb) {
        cur_server_cb->state = MGR_IO_ST_NONE;
        cur_server_cb->mysid = 0;
    }

    if (mgr_cli_valset) {
        val_free_value(mgr_cli_valset);
        mgr_cli_valset = NULL;
    }

    if (connect_valset) {
        val_free_value(connect_valset);
        connect_valset = NULL;
    }

    if (default_module) {
        m__free(default_module);
        default_module = NULL;
    }

    if (confname) {
        m__free(confname);
        confname = NULL;
    }

    if (runscript) {
        m__free(runscript);
        runscript = NULL;
    }

    if (runcommand) {
        m__free(runcommand);
        runcommand = NULL;
    }

    /* Cleanup the Netconf Server Library */
    mgr_cleanup();

    /* free this after the mgr_cleanup in case any session is active
     * and needs to be cleaned up before this main temp_progcb is freed
     */
    if (temp_progcb) {
        ncxmod_free_program_tempdir(temp_progcb);
        temp_progcb = NULL;
    }

    /* cleanup the module library search results */
    ncxmod_clean_search_result_queue(&modlibQ);

    free_aliases();

    /* cleanup the NCX engine and registries */
    ncx_cleanup();

}  /* yangcli_cleanup */


/********************************************************************
 * FUNCTION get_rpc_error_tag
 * 
 *  Determine why the RPC operation failed
 *
 * INPUTS:
 *   replyval == <rpc-reply> to use to look for <rpc-error>s
 *
 * RETURNS:
 *   the RPC error code for the <rpc-error> that was found
 *********************************************************************/
static rpc_err_t
    get_rpc_error_tag (val_value_t *replyval)
{
    val_value_t  *errval, *tagval;

    errval = val_find_child(replyval, 
                            NC_MODULE,
                            NCX_EL_RPC_ERROR);
    if (errval == NULL) {
        log_error("\nError: No <rpc-error> elements found");
        return RPC_ERR_NONE;
    }

    tagval = val_find_child(errval, 
                            NC_MODULE,
                            NCX_EL_ERROR_TAG);
    if (tagval == NULL) {
        log_error("\nError: <rpc-error> did not contain an <error-tag>");
        return RPC_ERR_NONE;
    }

    return rpc_err_get_errtag_enum(VAL_ENUM_NAME(tagval));

}  /* get_rpc_error_tag */


/********************************************************************
 * FUNCTION handle_rpc_timing
 * 
 * Get the roundtrip time and print the roundtrip time
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   req == original message request
 *
 *********************************************************************/
static void
    handle_rpc_timing (server_cb_t *server_cb,
                       mgr_rpc_req_t *req)
{
    struct timeval      now;
    long int            sec, usec;
    xmlChar             numbuff[NCX_MAX_NUMLEN];

    if (!server_cb->time_rpcs) {
        return;
    }

    gettimeofday(&now, NULL);

    /* get the resulting delta */
    if (now.tv_usec < req->perfstarttime.tv_usec) {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }

    sec = now.tv_sec - req->perfstarttime.tv_sec;
    usec = now.tv_usec - req->perfstarttime.tv_usec;

    sprintf((char *)numbuff, "%ld.%06ld", sec, usec);

    if (interactive_mode()) {
        log_stdout("\nRoundtrip time: %s seconds\n", numbuff);
        if (log_get_logfile() != NULL) {
            log_write("\nRoundtrip time: %s seconds\n", numbuff);
        }
    } else {
        log_write("\nRoundtrip time: %s seconds\n", numbuff);
    }

}  /* handle_rpc_timing */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION get_autocomp
* 
*  Get the autocomp parameter value
* 
* RETURNS:
*    autocomp boolean value
*********************************************************************/
boolean
    get_autocomp (void)
{
    return autocomp;
}  /* get_autocomp */


/********************************************************************
* FUNCTION get_autoload
* 
*  Get the autoload parameter value
* 
* RETURNS:
*    autoload boolean value
*********************************************************************/
boolean
    get_autoload (void)
{
    return autoload;
}  /* get_autoload */


/********************************************************************
* FUNCTION get_batchmode
* 
*  Get the batchmode parameter value
* 
* RETURNS:
*    batchmode boolean value
*********************************************************************/
boolean
    get_batchmode (void)
{
    return batchmode;
}  /* get_batchmode */


/********************************************************************
* FUNCTION get_default_module
* 
*  Get the default module
* 
* RETURNS:
*    default module value
*********************************************************************/
const xmlChar *
    get_default_module (void)
{
    return default_module;
}  /* get_default_module */


/********************************************************************
* FUNCTION get_runscript
* 
*  Get the runscript variable
* 
* RETURNS:
*    runscript value
*********************************************************************/
const xmlChar *
    get_runscript (void)
{
    return runscript;

}  /* get_runscript */


/********************************************************************
* FUNCTION get_baddata
* 
*  Get the baddata parameter
* 
* RETURNS:
*    baddata enum value
*********************************************************************/
ncx_bad_data_t
    get_baddata (void)
{
    return baddata;
}  /* get_baddata */


/********************************************************************
* FUNCTION get_yangcli_mod
* 
*  Get the yangcli module
* 
* RETURNS:
*    yangcli module
*********************************************************************/
ncx_module_t *
    get_yangcli_mod (void)
{
    return yangcli_mod;
}  /* get_yangcli_mod */


/********************************************************************
* FUNCTION get_mgr_cli_valset
* 
*  Get the CLI value set
* 
* RETURNS:
*    mgr_cli_valset variable
*********************************************************************/
val_value_t *
    get_mgr_cli_valset (void)
{
    return mgr_cli_valset;
}  /* get_mgr_cli_valset */


/********************************************************************
* FUNCTION get_connect_valset
* 
*  Get the connect value set
* 
* RETURNS:
*    connect_valset variable
*********************************************************************/
val_value_t *
    get_connect_valset (void)
{
    return connect_valset;
}  /* get_connect_valset */


/********************************************************************
* FUNCTION get_aliases_file
* 
*  Get the aliases-file value
* 
* RETURNS:
*    aliases_file variable
*********************************************************************/
const xmlChar *
    get_aliases_file (void)
{
    return aliases_file;
}  /* get_aliases_file */


/********************************************************************
* FUNCTION get_uservars_file
* 
*  Get the uservars-file value
* 
* RETURNS:
*    aliases_file variable
*********************************************************************/
const xmlChar *
    get_uservars_file (void)
{
    return uservars_file;
}  /* get_uservars_file */


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
status_t
    replace_connect_valset (const val_value_t *valset)
{
    val_value_t   *findval, *replaceval;

#ifdef DEBUG
    if (valset == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (connect_valset == NULL) {
        connect_valset = val_clone(valset);
        if (connect_valset == NULL) {
            return ERR_INTERNAL_MEM;
        } else {
            return NO_ERR;
        }
    }

    /* go through the connect value set passed in and compare
     * to the existing connect_valset; only replace the
     * parameters that are not already set
     */
    for (findval = val_get_first_child(valset);
         findval != NULL;
         findval = val_get_next_child(findval)) {

        replaceval = val_find_child(connect_valset,
                                    val_get_mod_name(findval),
                                    findval->name);
        if (replaceval == NULL) {
            replaceval = val_clone(findval);
            if (replaceval == NULL) {
                return ERR_INTERNAL_MEM;
            }
            val_add_child(replaceval, connect_valset);
        }
    }

    return NO_ERR;

}  /* replace_connect_valset */


/********************************************************************
* FUNCTION get_mgrloadQ
* 
*  Get the mgrloadQ value pointer
* 
* RETURNS:
*    mgrloadQ variable
*********************************************************************/
dlq_hdr_t *
    get_mgrloadQ (void)
{
    return &mgrloadQ;
}  /* get_mgrloadQ */


/********************************************************************
* FUNCTION get_aliasQ
* 
*  Get the aliasQ value pointer
* 
* RETURNS:
*    aliasQ variable
*********************************************************************/
dlq_hdr_t *
    get_aliasQ (void)
{
    return &aliasQ;
}  /* get_aliasQ */


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
void
    yangcli_reply_handler (ses_cb_t *scb,
                           mgr_rpc_req_t *req,
                           mgr_rpc_rpy_t *rpy)
{
    server_cb_t   *server_cb;
    val_value_t  *val;
    mgr_scb_t    *mgrcb;
    lock_cb_t    *lockcb;
    rpc_err_t     rpcerrtyp;
    status_t      res;
    boolean       anyout, anyerrors, done;
    uint32        usesid;

#ifdef DEBUG
    if (!scb || !req || !rpy) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    res = NO_ERR;
    mgrcb = scb->mgrcb;
    if (mgrcb) {
        usesid = mgrcb->agtsid;
        ncx_set_temp_modQ(&mgrcb->temp_modQ);
    } else {
        usesid = 0;
    }

    /***  TBD: multi-session support ***/
    server_cb = cur_server_cb;

    anyout = FALSE;
    anyerrors = FALSE;

    /* check the contents of the reply */
    if (rpy && rpy->reply) {
        if (val_find_child(rpy->reply, 
                           NC_MODULE,
                           NCX_EL_RPC_ERROR)) {
            if (server_cb->command_mode == CMD_MODE_NORMAL || LOGDEBUG) {
                log_error("\nRPC Error Reply %s for session %u:\n",
                          rpy->msg_id, 
                          usesid);
                val_dump_value_max(rpy->reply, 
                                   0,
                                   server_cb->defindent,
                                   DUMP_VAL_LOG,
                                   server_cb->display_mode,
                                   FALSE,
                                   FALSE);
                log_error("\n");
                anyout = TRUE;
            }
            if (server_cb->time_rpcs) {
                handle_rpc_timing(server_cb, req);
                anyout = TRUE;
            }
            anyerrors = TRUE;
        } else if (val_find_child(rpy->reply, NC_MODULE, NCX_EL_OK)) {
            if (server_cb->command_mode == CMD_MODE_NORMAL || LOGDEBUG2) {
                if (server_cb->echo_replies) {
                    log_info("\nRPC OK Reply %s for session %u:\n",
                             rpy->msg_id, 
                             usesid);
                    anyout = TRUE;
                }
            }
            if (server_cb->time_rpcs) {
                handle_rpc_timing(server_cb, req);
                anyout = TRUE;
            }
        } else if ((server_cb->command_mode == CMD_MODE_NORMAL && LOGINFO) ||
                   (server_cb->command_mode != CMD_MODE_NORMAL && LOGDEBUG2)) {

            if (server_cb->echo_replies) {
                log_info("\nRPC Data Reply %s for session %u:\n",
                         rpy->msg_id, 
                         usesid);
                val_dump_value_max(rpy->reply, 
                                   0,
                                   server_cb->defindent,
                                   DUMP_VAL_LOG,
                                   server_cb->display_mode,
                                   FALSE,
                                   FALSE);
                log_info("\n");
                anyout = TRUE;
            }
            if (server_cb->time_rpcs) {
                handle_rpc_timing(server_cb, req);
                anyout = TRUE;
            }
        } else if (server_cb->time_rpcs) {
            handle_rpc_timing(server_cb, req);
            anyout = TRUE;
        }

        /* output data even if there were errors
         * TBD: use a CLI switch to control whether
         * to save if <rpc-errors> received
         */
        if (server_cb->result_name || server_cb->result_filename) {
            /* save the data element if it exists */
            val = val_first_child_name(rpy->reply, NCX_EL_DATA);
            if (val) {
                val_remove_child(val);
            } else {
                if (val_child_cnt(rpy->reply) == 1) {
                    val = val_get_first_child(rpy->reply);
                    val_remove_child(val);
                } else {
                    /* not 1 child node, so save the entire reply
                     * need a single top-level element to be a
                     * valid XML document
                     */
                    val = rpy->reply;
                    rpy->reply = NULL;
                }
            }

            /* hand off the malloced 'val' node here */
            res = finish_result_assign(server_cb, val, NULL);
        }  else if (LOGINFO && 
                    !anyout && 
                    !anyerrors && 
                    server_cb->command_mode == CMD_MODE_NORMAL &&
                    interactive_mode()) {
            log_stdout("\nOK\n");
        }
    } else {
        log_error("\nError: yangcli: no reply parsed\n");
    }

    /* check if a script is running */
    if ((anyerrors || res != NO_ERR) && 
        runstack_level(server_cb->runstack_context)) {
        runstack_cancel(server_cb->runstack_context);
    }

    if (anyerrors && rpy->reply) {
        rpcerrtyp = get_rpc_error_tag(rpy->reply);
    } else {
        /* default error: may not get used */
        rpcerrtyp = RPC_ERR_OPERATION_FAILED;
    }

    switch (server_cb->state) {
    case MGR_IO_ST_CONN_CLOSEWAIT:
        if (mgrcb->closed) {
            mgr_ses_free_session(server_cb->mysid);
            server_cb->mysid = 0;
            server_cb->state = MGR_IO_ST_IDLE;
        } /* else keep waiting */
        break;
    case MGR_IO_ST_CONN_RPYWAIT:
        server_cb->state = MGR_IO_ST_CONN_IDLE;
        switch (server_cb->command_mode) {
        case CMD_MODE_NORMAL:
            break;
        case CMD_MODE_AUTOLOAD:
            (void)autoload_handle_rpc_reply(server_cb,
                                            scb,
                                            rpy->reply,
                                            anyerrors);
            break;
        case CMD_MODE_AUTOLOCK:
            done = FALSE;
            lockcb = &server_cb->lock_cb[server_cb->locks_cur_cfg];
            if (anyerrors) {
                if (rpcerrtyp == RPC_ERR_LOCK_DENIED) {
                    lockcb->lock_state = LOCK_STATE_TEMP_ERROR;
                } else if (lockcb->config_id == NCX_CFGID_CANDIDATE) {
                    res = send_discard_changes_pdu_to_server(server_cb);
                    if (res != NO_ERR) {
                        handle_locks_cleanup(server_cb);
                    }
                    done = TRUE;
                } else {
                    lockcb->lock_state = LOCK_STATE_FATAL_ERROR;
                    done = TRUE;
                }
            } else {
                lockcb->lock_state = LOCK_STATE_ACTIVE;
            }

            if (!done) {
                res = handle_get_locks_request_to_server(server_cb,
                                                        FALSE,
                                                        &done);
                if (done) {
                    /* check if the locks are all good */
                    if (get_lock_worked(server_cb)) {
                        log_info("\nget-locks finished OK");
                        server_cb->command_mode = CMD_MODE_NORMAL;
                        server_cb->locks_waiting = FALSE;
                    } else {
                        log_error("\nError: get-locks failed, "
                                  "starting cleanup");
                        handle_locks_cleanup(server_cb);
                    }
                } else if (res != NO_ERR) {
                    log_error("\nError: get-locks failed, no cleanup");
                }
            }
            break;
        case CMD_MODE_AUTOUNLOCK:
            lockcb = &server_cb->lock_cb[server_cb->locks_cur_cfg];
            if (anyerrors) {
                lockcb->lock_state = LOCK_STATE_FATAL_ERROR;
            } else {
                lockcb->lock_state = LOCK_STATE_RELEASED;
            }

            (void)handle_release_locks_request_to_server(server_cb,
                                                         FALSE,
                                                         &done);
            if (done) {
                clear_lock_cbs(server_cb);
            }
            break;
        case CMD_MODE_AUTODISCARD:
            lockcb = &server_cb->lock_cb[server_cb->locks_cur_cfg];
            server_cb->command_mode = CMD_MODE_AUTOLOCK;
            if (anyerrors) {
                handle_locks_cleanup(server_cb);
            } else {
                res = handle_get_locks_request_to_server(server_cb,
                                                        FALSE,
                                                        &done);
                if (done) {
                    /* check if the locks are all good */
                    if (get_lock_worked(server_cb)) {
                        log_info("\nget-locks finished OK");
                        server_cb->command_mode = CMD_MODE_NORMAL;
                        server_cb->locks_waiting = FALSE;
                    } else {
                        log_error("\nError: get-locks failed, "
                                  "starting cleanup");
                        handle_locks_cleanup(server_cb);
                    }
                } else if (res != NO_ERR) {
                    log_error("\nError: get-locks failed, no cleanup");
                }
            }            
            break;
        case CMD_MODE_SAVE:
            if (anyerrors) {
                log_warn("\nWarning: Final <copy-config> canceled  because "
                         "<commit> failed");
            } else {
                (void)finish_save(server_cb);
            }
            server_cb->command_mode = CMD_MODE_NORMAL;
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    default:
        break;
    }

    /* free the request and reply */
    mgr_rpc_free_request(req);
    if (rpy) {
        mgr_rpc_free_reply(rpy);
    }

}  /* yangcli_reply_handler */


/********************************************************************
 * FUNCTION finish_result_assign
 * 
 * finish the assignment to result_name or result_filename
 * use 1 of these 2 parms:
 *    resultval == result to output to file
 *    !!! This is a live var that is freed by this function
 *
 *    resultstr == result to output as string
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    finish_result_assign (server_cb_t *server_cb,
                          val_value_t *resultvar,
                          const xmlChar *resultstr)
{
    val_value_t   *configvar;
    status_t       res;

#ifdef DEBUG
    if (server_cb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    if (resultvar != NULL) {
        /* force the name strings to be
         * malloced instead of backptrs to
         * the object name
         */
        res = val_force_dname(resultvar);
        if (res != NO_ERR) {
            val_free_value(resultvar);
        }
    }

    if (res != NO_ERR) {
        ;
    } else if (server_cb->result_filename != NULL) {
        res = output_file_result(server_cb, resultvar, resultstr);
        if (resultvar) {
            val_free_value(resultvar);
        }
    } else if (server_cb->result_name != NULL) {
        if (server_cb->result_vartype == VAR_TYP_CONFIG) {
            configvar = var_get(server_cb->runstack_context,
                                server_cb->result_name,
                                VAR_TYP_CONFIG);
            if (configvar==NULL) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            } else {
                res = handle_config_assign(server_cb, configvar,
                                           resultvar, resultstr);
                if (res == NO_ERR) {
                    log_info("\nOK\n");
                }                    
            }
        } else if (resultvar != NULL) {
            /* save the filled in value
             * hand off the malloced 'resultvar' here
             */
            if (res == NO_ERR) {
                res = var_set_move(server_cb->runstack_context,
                                   server_cb->result_name, 
                                   xml_strlen(server_cb->result_name),
                                   server_cb->result_vartype,
                                   resultvar);
            }
            if (res != NO_ERR) {
                /* resultvar is freed even if error returned */
                log_error("\nError: set result for '%s' failed (%s)",
                          server_cb->result_name, 
                          get_error_string(res));
            } else {
                log_info("\nOK\n");
            }
        } else {
            /* this is just a string assignment */
            res = var_set_from_string(server_cb->runstack_context,
                                      server_cb->result_name,
                                      resultstr, 
                                      server_cb->result_vartype);
            if (res != NO_ERR) {
                log_error("\nyangcli: Error setting variable %s (%s)",
                          server_cb->result_name, 
                          get_error_string(res));
            } else {
                log_info("\nOK\n");
            }
        }
    }

    clear_result(server_cb);

    return res;

}  /* finish_result_assign */


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
void
    report_capabilities (server_cb_t *server_cb,
                         const ses_cb_t *scb,
                         boolean isfirst,
                         help_mode_t mode)
{
    const mgr_scb_t    *mscb;
    const xmlChar      *server;
    const val_value_t  *parm;

    if (!LOGINFO) {
        /* skip unless log level is INFO or higher */
        return;
    }

    mscb = (const mgr_scb_t *)scb->mgrcb;

    parm = val_find_child(server_cb->connect_valset, 
                          YANGCLI_MOD, 
                          YANGCLI_SERVER);
    if (parm && parm->res == NO_ERR) {
        server = VAL_STR(parm);
    } else {
        server = (const xmlChar *)"--";
    }

    log_write("\n\nNETCONF session established for %s on %s",
              scb->username, 
              mscb->target ? mscb->target : server);


    log_write("\n\nClient Session Id: %u", scb->sid);
    log_write("\nServer Session Id: %u", mscb->agtsid);

    if (isfirst || mode > HELP_MODE_BRIEF) {
        log_write("\n\nServer Protocol Capabilities");
        cap_dump_stdcaps(&mscb->caplist);

        log_write("\n\nServer Module Capabilities");
        cap_dump_modcaps(&mscb->caplist);

        log_write("\n\nServer Enterprise Capabilities");
        cap_dump_entcaps(&mscb->caplist);
        log_write("\n");
    }

    log_write("\nProtocol version set to: ");
    switch (ses_get_protocol(scb)) {
    case NCX_PROTO_NETCONF10:
        log_write("RFC 4741 (base:1.0)");
        break;
    case NCX_PROTO_NETCONF11:
        log_write("RFC 6241 (base:1.1)");
        break;
    default:
        log_write("unknown");
    }

    if (!isfirst && (mode <= HELP_MODE_BRIEF)) {
        return;
    }

    log_write("\nDefault target set to: ");

    switch (mscb->targtyp) {
    case NCX_AGT_TARG_NONE:
        if (isfirst) {
            server_cb->default_target = NULL;
        }
        log_write("none");
        break;
    case NCX_AGT_TARG_CANDIDATE:
        if (isfirst) {
            server_cb->default_target = NCX_EL_CANDIDATE;
        }
        log_write("<candidate>");
        break;
    case NCX_AGT_TARG_RUNNING:
        if (isfirst) {
            server_cb->default_target = NCX_EL_RUNNING;
        }
        log_write("<running>");
        break;
    case NCX_AGT_TARG_CAND_RUNNING:
        if (force_target != NULL &&
            !xml_strcmp(force_target, NCX_EL_RUNNING)) {
            /* set to running */
            if (isfirst) {
                server_cb->default_target = NCX_EL_RUNNING;
            }
            log_write("<running> (<candidate> also supported)");
        } else {
            /* set to candidate */
            if (isfirst) {
                server_cb->default_target = NCX_EL_CANDIDATE;
            }
            log_write("<candidate> (<running> also supported)");
        }
        break;
    case NCX_AGT_TARG_LOCAL:
        if (isfirst) {
            server_cb->default_target = NULL;
        }
        log_write("none -- local file");        
        break;
    case NCX_AGT_TARG_REMOTE:
        if (isfirst) {
            server_cb->default_target = NULL;
        }
        log_write("none -- remote file");       
        break;
    default:
        if (isfirst) {
            server_cb->default_target = NULL;
        }
        SET_ERROR(ERR_INTERNAL_VAL);
        log_write("none -- unknown (%d)", mscb->targtyp);
        break;
    }

    log_write("\nSave operation mapped to: ");
    switch (mscb->targtyp) {
    case NCX_AGT_TARG_NONE:
        log_write("none");
        break;
    case NCX_AGT_TARG_CANDIDATE:
    case NCX_AGT_TARG_CAND_RUNNING:
        if (!xml_strcmp(server_cb->default_target,
                        NCX_EL_CANDIDATE)) {
            log_write("commit");
            if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
                log_write(" + copy-config <running> <startup>");
            }
        } else {
            if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
                log_write("copy-config <running> <startup>");
            } else {
                log_write("none");
            }
        }
        break;
    case NCX_AGT_TARG_RUNNING:
        if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
            log_write("copy-config <running> <startup>");
        } else {
            log_write("none");
        }           
        break;
    case NCX_AGT_TARG_LOCAL:
    case NCX_AGT_TARG_REMOTE:
        /* no way to assign these enums from the capabilities alone! */
        if (cap_std_set(&mscb->caplist, CAP_STDID_URL)) {
            log_write("copy-config <running> <url>");
        } else {
            log_write("none");
        }           
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        log_write("none");
        break;
    }

    log_write("\nDefault with-defaults behavior: ");
    if (mscb->caplist.cap_defstyle) {
        log_write("%s", mscb->caplist.cap_defstyle);
    } else {
        log_write("unknown");
    }

    log_write("\nAdditional with-defaults behavior: ");
    if (mscb->caplist.cap_supported) {
        log_write("%s", mscb->caplist.cap_supported);
    } else {
        log_write("unknown");
    }

    log_write("\n");
    
} /* report_capabilities */


/********************************************************************
*                                                                   *
*                       FUNCTION main                               *
*                                                                   *
*********************************************************************/
int main (int argc, char *argv[])
{
    status_t   res;

#ifdef MEMORY_DEBUG
    mtrace();
#endif

    res = yangcli_init(argc, argv);
    init_done = (res == NO_ERR) ? TRUE : FALSE;
    if (res != NO_ERR) {
        log_error("\nyangcli: init returned error (%s)\n", 
                  get_error_string(res));
    } else if (!(helpmode || versionmode)) {
        /* normal run mode */
        res = mgr_io_run();
        if (res != NO_ERR) {
            log_error("\nmgr_io failed (%d)\n", res);
        } else {
            log_write("\n");
        }
    }

    print_errors();

    print_error_count();

    yangcli_cleanup();

    print_error_count();

#ifdef MEMORY_DEBUG
    muntrace();
#endif


    return 0;

} /* main */

/* END yangcli.c */
