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
#ifndef _H_agt
#define _H_agt

/*  FILE: agt.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server message handler

     - NETCONF PDUs

        - PDU instance document parsing

        - Object instance syntax validation

        - Basic object-level referential integrity

        - Value instance document parsing 

        - Value instance document syntax validation

        - Value instance count validation

        - Value instance default value insertion


    The netconfd server <edit-config> handler registers callbacks
    with the agt_val module that can be called IN ADDITION
    to the automated callback, for referential integrity checking,
    resource reservation, etc.

    NCX Write Model
    ---------------

    netconfd uses a 3 pass callback model to process 
    <edit-config> PDUs.
    
  Pass 1:  Validation : AGT_CB_VALIDATE

     The target database is not touched during this phase.

       - XML parsing
       - data type validation
       - pattern and regular expression matching
       - dynamic instance validation
       - instance count validation
       - choice and choice group validation
       - insertion by default validation
       - nested 'operation' attribute validation

    If any errors occur within a value node (within the 
    /config/ container) then that entire value node
    is considered unusable and it is not applied.

    If continue-on-error is requested, then all sibling nodes
    will be processed if the validation tests all pass.

  Pass 2: Apply : AGT_CB_APPLY

    On a per-node granularity, an operation is processed only
    if no errors occurred during the validation phase.

    The target database is modified.  User callbacks should
    not emit external PDUs (e.g., BGP changes) until the COMMIT
    phase.  The automated PDU processing will make non-destructive
    edits during this phase, even if the 'rollback-on-error'
    error-option is not requested (in order to simplify the code).

    If an error-option other than continue-on-error is requested,
    then any error returned during this phase will cause further
    'apply' phase processing to be terminated.

    If error-option is 'stop-on-error' then phase 3 is not executed
    if this phase terminates with an error.

  Pass 3-OK: Commit (Positive Outcome) :  AGT_CB_COMMIT
       
    The database edits are automatically completed during this phase.
    The user callbacks must not edit the database -- only modify
    their own data structures, send PDUs, etc.

  Pass 3-ERR: Rollback (Negative Outcome) : AGT_CB_ROLLBACK

    If rollback-on-error is requested, then this phase will be
    executed for only the application nodes (and sibling parmsets 
    within the errored application) that have already executed
    the 'apply' phase.

    

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
04-nov-05    abb      Begun
02-aug-08    abb      Convert from NCX typdef to YANG object design
03-oct-08    abb      Convert to YANG only, remove app, parmset, 
                      and parm layers in the NETCONF database
*/

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

#define AGT_NUM_CB   (AGT_CB_ROLLBACK+1)

/* ncxserver magic cookie hack */
#define NCX_SERVER_MAGIC \
  "x56o8937ab17eg922z34rwhobskdbyswfehkpsqq3i55a0an960ccw24a4ek864aOpal1t2p"

#define AGT_MAX_PORTS  4

#define AGT_DEF_CONF_FILE   (const xmlChar *)"/etc/yuma/netconfd.conf"

/* this behavior used to set to TRUE, before 1.15-1 */
#define AGT_DEF_DELETE_EMPTY_NP   FALSE

/* this is over-ridden by the --system-sorted CLI parameter */
#define AGT_DEF_SYSTEM_SORTED     FALSE

#define AGT_USER_VAR        (const xmlChar *)"user"

#define AGT_URL_SCHEME_LIST (const xmlChar *)"file"

#define AGT_FILE_SCHEME     (const xmlChar *)"file:///"


/* tests for agt_commit_test_t flags field */
#define AGT_TEST_FL_MIN_ELEMS  bit0
#define AGT_TEST_FL_MAX_ELEMS  bit1
#define AGT_TEST_FL_MANDATORY  bit2
#define AGT_TEST_FL_MUST       bit3
#define AGT_TEST_FL_UNIQUE     bit4
#define AGT_TEST_FL_XPATH_TYPE bit5
#define AGT_TEST_FL_CHOICE     bit6

/* not really a commit test; done during delete_dead_nodes() */
#define AGT_TEST_FL_WHEN       bit7

/* mask for all the commit tests */
#define AGT_TEST_ALL_COMMIT_MASK (bit0|bit1|bit2|bit3|bit4|bit5|bit6)

/* mask for the subset of instance tests */
#define AGT_TEST_INSTANCE_MASK (bit0|bit1|bit2|bit6)



/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* matches access-control enumeration in netconfd.yang */
typedef enum agt_acmode_t_ {
    AGT_ACMOD_NONE,
    AGT_ACMOD_ENFORCING,
    AGT_ACMOD_PERMISSIVE,
    AGT_ACMOD_DISABLED,
    AGT_ACMOD_OFF
} agt_acmode_t;


/* server config state used in agt_val_root_check */
typedef enum agt_config_state_t_ {
    AGT_CFG_STATE_NONE,
    AGT_CFG_STATE_INIT,
    AGT_CFG_STATE_OK,
    AGT_CFG_STATE_BAD
} agt_config_state_t;


/* enumeration of the different server callback types 
 * These are used are array indices so there is no dummy zero enum
 * AGT_CB_TEST_APPLY has been replaced by AGT_CB_APPLY
 * during the edit-config procedure
 * AGT_CB_COMMIT_CHECK has been replaced by AGT_CB_VALIDATE
 * during the commit procedure
 */
typedef enum agt_cbtyp_t_ {
    AGT_CB_VALIDATE,               /* P1: write operation validate */
    AGT_CB_APPLY,                     /* P2: write operation apply */
    AGT_CB_COMMIT,               /* P3-pos: write operation commit */
    AGT_CB_ROLLBACK            /* P3-neg: write operation rollback */
} agt_cbtyp_t;


/* hardwire some of the server profile parameters
 * because they are needed before the NCX engine is running
 * They cannot be changed after boot-time.
 */
typedef struct agt_profile_t_ {
    ncx_agttarg_t       agt_targ;
    ncx_agtstart_t      agt_start;
    log_debug_t         agt_loglevel;
    boolean             agt_log_acm_reads;
    boolean             agt_log_acm_writes;
    boolean             agt_validate_all;
    boolean             agt_has_startup;
    boolean             agt_usestartup;   /* --no-startup flag */
    boolean             agt_factorystartup;   /* --factory-startup flag */
    boolean             agt_startup_error;  /* T: stop, F: continue */
    boolean             agt_running_error;  /* T: stop, F: continue */
    boolean             agt_logappend;
    boolean             agt_xmlorder;
    boolean             agt_deleteall_ok;   /* TBD: not implemented */
    boolean             agt_stream_output;   /* d:true; no CLI support yet */
    boolean             agt_delete_empty_npcontainers;     /* d: false */
    boolean             agt_notif_sequence_id;    /* d: false */
    const xmlChar      *agt_accesscontrol;
    const xmlChar      *agt_conffile;
    const xmlChar      *agt_logfile;
    const xmlChar      *agt_startup;
    const xmlChar      *agt_defaultStyle;
    const xmlChar      *agt_superuser;
    uint32              agt_eventlog_size;
    uint32              agt_maxburst;
    uint32              agt_hello_timeout;
    uint32              agt_idle_timeout;
    uint32              agt_linesize;
    int32               agt_indent;
    boolean             agt_usevalidate;          /* --with-validate */
    boolean             agt_useurl;                    /* --with-url */
    boolean             agt_system_sorted;
    ncx_withdefaults_t  agt_defaultStyleEnum;
    agt_acmode_t        agt_accesscontrol_enum;
    uint16              agt_ports[AGT_MAX_PORTS];

    /****** state variables; TBD: move out of profile ******/

    /* root commit test flags */
    uint32              agt_rootflags;

    /* server config state */
    agt_config_state_t  agt_config_state;

    /* server load-config errors */
    boolean             agt_load_validate_errors;
    boolean             agt_load_rootcheck_errors;
    boolean             agt_load_top_rootcheck_errors;
    boolean             agt_load_apply_errors;

    /* Q of malloced ncx_save_deviations_t */
    dlq_hdr_t           agt_savedevQ;  

    /* Q of malloced agt_commit_test_t */
    dlq_hdr_t           agt_commit_testQ;  

    /* cached location of startup transaction ID file */
    xmlChar            *agt_startup_txid_file;

} agt_profile_t;


/* SIL init function */
typedef status_t (*agt_sil_init_fn_t)(const xmlChar *modname,
                                      const xmlChar *revision);

/* SIL init2 function */
typedef status_t (*agt_sil_init2_fn_t)(void);

/* SIL cleanup function */
typedef void (*agt_sil_cleanup_fn_t)(void);

/* struct to keep track of the dynamic libraries
 * opened by the 'load' command
 */
typedef struct agt_dynlib_cb_t_ {
    dlq_hdr_t             qhdr;
    void                 *handle;
    xmlChar              *modname;
    xmlChar              *revision;
    ncx_module_t         *mod;
    agt_sil_init_fn_t     initfn;
    agt_sil_init2_fn_t    init2fn;
    agt_sil_cleanup_fn_t  cleanupfn;
    status_t              init_status;
    status_t              init2_status;
    boolean               init2_done;
    boolean               cleanup_done;
} agt_dynlib_cb_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_init1
* 
* Initialize the Server Library: stage 1: CLI and profile
* 
* TBD -- put platform-specific server init here
*
* INPUTS:
*   argc == command line argument count
*   argv == array of command line strings
*   showver == address of version return quick-exit status
*   showhelp == address of help return quick-exit status
*
* OUTPUTS:
*   *showver == TRUE if user requsted version quick-exit mode
*   *showhelp == TRUE if user requsted help quick-exit mode
*
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    agt_init1 (int argc,
	       char *argv[],
	       boolean *showver,
	       help_mode_t *showhelpmode);

/********************************************************************
* FUNCTION agt_init2
* 
* Initialize the Server Library
* The agt_profile is set and the object database is
* ready to have YANG modules loaded
*
* RPC and data node callbacks should be installed
* after the module is loaded, and before the running config
* is loaded.
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_init2 (void);


/********************************************************************
* FUNCTION agt_cleanup
*
* Cleanup the Server Library
* 
*********************************************************************/
extern void 
    agt_cleanup (void);


/********************************************************************
* FUNCTION agt_get_profile
* 
* Get the server profile struct
* 
* INPUTS:
*   none
* RETURNS:
*   pointer to the server profile
*********************************************************************/
extern agt_profile_t *
    agt_get_profile (void);


/********************************************************************
* FUNCTION agt_request_shutdown
* 
* Request some sort of server shutdown
* 
* INPUTS:
*   mode == requested shutdown mode
*
*********************************************************************/
extern void
    agt_request_shutdown (ncx_shutdowntyp_t mode);

/********************************************************************
* FUNCTION agt_shutdown_requested
* 
* Check if some sort of server shutdown is in progress
* 
* RETURNS:
*    TRUE if shutdown mode has been started
*
*********************************************************************/
extern boolean
    agt_shutdown_requested (void);


/********************************************************************
* FUNCTION agt_shutdown_mode_requested
* 
* Check what shutdown mode was requested
* 
* RETURNS:
*    shutdown mode
*
*********************************************************************/
extern ncx_shutdowntyp_t
    agt_shutdown_mode_requested (void);


/********************************************************************
* FUNCTION agt_cbtype_name
* 
* Get the string for the server callback phase
* 
* INPUTS:
*   cbtyp == callback type enum
*
* RETURNS:
*    const string for this enum
*********************************************************************/
extern const xmlChar *
    agt_cbtype_name (agt_cbtyp_t cbtyp);


#ifndef STATIC_SERVER
/********************************************************************
* FUNCTION agt_load_sil_code
* 
* Load the Server Instrumentation Library for the specified module
* 
* INPUTS:
*   modname == name of the module to load
*   revision == revision date of the module to load (may be NULL)
*   cfgloaded == TRUE if running config has already been done
*                FALSE if running config not loaded yet
*
* RETURNS:
*    const string for this enum
*********************************************************************/
extern status_t
    agt_load_sil_code (const xmlChar *modname,
                       const xmlChar *revision,
                       boolean cfgloaded);
#endif


/********************************************************************
* FUNCTION agt_advertise_module_needed
*
* Check if the module should be advertised or not
* Hard-wired hack at this time
*
* INPUTS:
*    modname == module name to check
* RETURNS:
*    none
*********************************************************************/
extern boolean
    agt_advertise_module_needed (const xmlChar *modname);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt */
