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
/*  FILE: agt.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
04nov05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef STATIC_SERVER
#include <dlfcn.h>
#endif

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_cfg.h"
#include "agt_commit_complete.h"
#include "agt_cli.h"
#include "agt_connect.h"
#include "agt_hello.h"
#include "agt_if.h"
#include "agt_ncx.h"
#include "agt_not.h"
#include "agt_plock.h"
#include "agt_proc.h"
#include "agt_rpc.h"
#include "agt_ses.h"
#include "agt_signal.h"
#include "agt_state.h"
#include "agt_sys.h"
#include "agt_val.h"
#include "agt_time_filter.h"
#include "agt_timer.h"
#include "agt_util.h"
#include "agt_yuma_arp.h"
#include "log.h"
#include "ncx.h"
#include "ncx_str.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "status.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define TX_ID_MODULE    (const xmlChar *)"yuma-system"
#define TX_ID_OBJECT    (const xmlChar *)"transaction-id"


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                            *
*                                                                   *
*********************************************************************/
static boolean            agt_init_done = FALSE;
static agt_profile_t      agt_profile;
static boolean            agt_shutdown;
static boolean            agt_shutdown_started;
static ncx_shutdowntyp_t  agt_shutmode;
static dlq_hdr_t          agt_dynlibQ;

/********************************************************************
* FUNCTION init_server_profile
* 
* Hardwire the initial server profile variables
*
* OUTPUTS:
*  *agt_profile is filled in with params of defaults
*
*********************************************************************/
static void
    init_server_profile (void)
{
    memset(&agt_profile, 0x0, sizeof(agt_profile_t));

    /* init server state variables stored here */
    dlq_createSQue(&agt_profile.agt_savedevQ);
    dlq_createSQue(&agt_profile.agt_commit_testQ);
    agt_profile.agt_config_state = AGT_CFG_STATE_INIT;

    /* Set the default values for the user parameters
     * these may be overridden from the command line;
     */
    agt_profile.agt_targ = NCX_AGT_TARG_CANDIDATE;
    agt_profile.agt_start = NCX_AGT_START_MIRROR;
    agt_profile.agt_loglevel = log_get_debug_level();
    agt_profile.agt_log_acm_reads = FALSE;
    agt_profile.agt_log_acm_writes = TRUE;

    /* T: <validate> op on candidate will call all SILs 
     * F: only SILs for nodes changed in the candidate will
     * be called; only affects <validate> command   */
    agt_profile.agt_validate_all = TRUE;

    agt_profile.agt_has_startup = FALSE;
    agt_profile.agt_usestartup = TRUE;
    agt_profile.agt_factorystartup = FALSE;
    agt_profile.agt_startup_error = FALSE;
    agt_profile.agt_running_error = FALSE;
    agt_profile.agt_logappend = FALSE;
    agt_profile.agt_xmlorder = FALSE;
    agt_profile.agt_deleteall_ok = FALSE;
    agt_profile.agt_stream_output = TRUE;

    /* this flag is no longer supported -- it must be
     * set to false for XPath to work correctly
     */
    agt_profile.agt_delete_empty_npcontainers = FALSE;

    /* this flag adds the <sequence-id> leaf to notifications
     * It was default true for yuma through 2.2-3 release
     * Starting 2.2-4 it is removed by default since the XSD
     * in RFC 5277 does not allow extensions
     */
    agt_profile.agt_notif_sequence_id = FALSE;

    agt_profile.agt_accesscontrol = NULL;
    agt_profile.agt_conffile = NULL;
    agt_profile.agt_logfile = NULL;
    agt_profile.agt_startup = NULL;
    agt_profile.agt_defaultStyle = NCX_EL_EXPLICIT;
    agt_profile.agt_superuser = NULL;
    agt_profile.agt_eventlog_size = 1000;
    agt_profile.agt_maxburst = 10;
    agt_profile.agt_hello_timeout = 300;
    agt_profile.agt_idle_timeout = 3600;
    agt_profile.agt_linesize = 72;
    agt_profile.agt_indent = 1;
    agt_profile.agt_usevalidate = TRUE;
    agt_profile.agt_useurl = TRUE;
    agt_profile.agt_defaultStyleEnum = NCX_WITHDEF_EXPLICIT;
    agt_profile.agt_accesscontrol_enum = AGT_ACMOD_ENFORCING;
    agt_profile.agt_system_sorted = AGT_DEF_SYSTEM_SORTED;

} /* init_server_profile */


/********************************************************************
* FUNCTION clean_server_profile
* 
* Clean the server profile variables
*
* OUTPUTS:
*  *agt_profile is filled in with params of defaults
*
*********************************************************************/
static void
    clean_server_profile (void)
{
    ncx_clean_save_deviationsQ(&agt_profile.agt_savedevQ);

    while (!dlq_empty(&agt_profile.agt_commit_testQ)) {
        agt_cfg_commit_test_t *commit_test = (agt_cfg_commit_test_t *)
            dlq_deque(&agt_profile.agt_commit_testQ);
        agt_cfg_free_commit_test(commit_test);
    }

    if (agt_profile.agt_startup_txid_file) {
        m__free(agt_profile.agt_startup_txid_file);
        agt_profile.agt_startup_txid_file = NULL;
    }

} /* clean_server_profile */


/********************************************************************
* FUNCTION load_running_config
* 
* Load the NV startup config into the running config
* 
* INPUTS:
*   startup == startup filespec provided by the user
*           == NULL if not set by user 
*              (use default name and specified search path instead)
*   loaded == address of return config loaded flag
*
* OUTPUTS:
*   *loaded == TRUE if some config file was loaded
*
*   The <running> config is loaded from NV-storage,
*   if the NV-storage <startup> config can be found an read
* RETURNS:
*   status
*********************************************************************/
static status_t
    load_running_config (const xmlChar *startup,
                         boolean *loaded)
{
    cfg_template_t  *cfg;
    xmlChar         *fname;
    agt_profile_t   *profile;
    status_t         res;

    res = NO_ERR;
    *loaded = FALSE;
    profile = agt_get_profile();

    cfg = cfg_get_config(NCX_CFG_RUNNING);
    if (!cfg) {
        log_error("\nagt: No running config found!!");
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* use the user-set startup or default filename */
    if (startup) {
        /* relative filespec, use search path */
        fname = ncxmod_find_data_file(startup, FALSE, &res);
    } else {
        /* search for the default startup-cfg.xml filename */
        fname = ncxmod_find_data_file(NCX_DEF_STARTUP_FILE, FALSE, &res);
    }

    /* check if error finding the filespec */
    if (!fname) {
        if (startup) {
            if (res == NO_ERR) {
                res = ERR_NCX_MISSING_FILE;
            }
            log_error("\nError: Startup config file (%s) not found (%s).",
                      startup, get_error_string(res));
            return res;
        } else {
            log_info("\nDefault startup config file (%s) not found."
                     "\n   Booting with default running configuration!\n",
                     NCX_DEF_STARTUP_FILE);
            return NO_ERR;
        }
    } else if (LOGDEBUG2) {
        log_debug2("\nFound startup config: '%s'", fname);
    }
    
    /* try to load the config file that was found or given */
    res = agt_ncx_cfg_load(cfg, CFG_LOC_FILE, fname);
    if (res == ERR_XML_READER_START_FAILED) {
        log_error("\nagt: Error: Could not open startup config file"
                  "\n     (%s)\n", fname);
    } else if (res != NO_ERR) {
        /* if an error is returned then it was a hard error
         * since the startup_error and running_error profile
         * variables have already been accounted for.
         * An error in the setup or in the AGT_RPC_PH_INVOKE phase
         * of the <load-config> operation occurred
         */
        log_error("\nError: load startup config failed (%s)",
                  get_error_string(res));
        if (!dlq_empty(&cfg->load_errQ)) {
            *loaded = TRUE;
        }
    } else {
        /* assume OK or startup and running continue; if 1 or both is not
         * set then the server will exit anyway and the config state
         * will not matter
         */
        profile->agt_config_state = AGT_CFG_STATE_OK;

        *loaded = TRUE;

        boolean errdone = FALSE;
        boolean errcontinue = FALSE;

        if (profile->agt_load_validate_errors) {
            if (profile->agt_startup_error) {
                /* quit if any startup validation errors */
                log_error("\nError: validation errors occurred loading the "
                          "<running> database\n   from NV-storage"
                          " (%s)\n", fname);
                errdone = TRUE;
            } else {
                /* continue if any startup errors */
                log_warn("\nWarning: validation errors occurred loading "
                         "the <running> database\n   from NV-storage"
                         " (%s)\n", fname);
                errcontinue = TRUE;
            }
        }
        if (!errdone && profile->agt_load_rootcheck_errors) {
            if (profile->agt_startup_error) {
                /* quit if any startup root-check validation errors */
                log_error("\nError: root-check validation errors "
                          "occurred loading the <running> database\n"
                          "   from NV-storage (%s)\n", fname);
                errdone = TRUE;
            } else {
                /* continue if any root-check validation errors */
                log_warn("\nWarning: root-check validation errors "
                         "occurred loading the <running> database\n"
                         "   from NV-storage (%s)\n", fname);
                errcontinue = TRUE;
            }
        }
        if (!errdone && profile->agt_load_top_rootcheck_errors) {
            if (profile->agt_running_error) {
                /* quit if any top-level root-check validation errors */
                log_error("\nError: top-node root-check validation errors "
                          "occurred loading the <running> database\n"
                          "   from NV-storage (%s)\n", fname);
                errdone = TRUE;
            } else {
                /* continue if any startup errors */
                log_warn("\nWarning: top-node root-check validation errors "
                         "occurred loading the <running> database\n"
                         "   from NV-storage (%s)\n", fname);
                profile->agt_config_state = AGT_CFG_STATE_BAD;
                errcontinue = TRUE;
            }
        }
        if (!errdone && profile->agt_load_apply_errors) {
            /* quit if any apply-to-running SIL errors */
            errdone = TRUE;
            log_error("\nError: fatal errors "
                      "occurred loading the <running> database "
                      "from NV-storage\n     (%s)\n", fname);
        }

        if (errdone) {
            res = ERR_NCX_OPERATION_FAILED;
        } else if (errcontinue) {
            val_purge_errors_from_root(cfg->root);
            log_info("\nagt: Startup config loaded after pruning error nodes\n"
                     "Source: %s\n", fname);
        } else {
            log_info("\nagt: Startup config loaded OK\n     Source: %s\n",
                     fname);
        }
    }

    if (LOGDEBUG) {
        log_debug("\nContents of %s configuration:", cfg->name);
        val_dump_value(cfg->root, 0);
        log_debug("\n");
    }

    if (fname) {
        m__free(fname);
    }

    return res;

} /* load_running_config */


/********************************************************************
* FUNCTION new_dynlib_cb
* 
* Malloc and init a new dynamic library control block
*
* RETURNS:
*    malloced struct or NULL if malloc error
*********************************************************************/
static agt_dynlib_cb_t *
    new_dynlib_cb (void)
{
    agt_dynlib_cb_t *dynlib;

    dynlib = m__getObj(agt_dynlib_cb_t);
    if (dynlib == NULL) {
        return NULL;
    }
    memset(dynlib, 0x0, sizeof(agt_dynlib_cb_t));
    return dynlib;

} /* new_dynlib_cb */


/********************************************************************
* FUNCTION free_dynlib_cb
* 
* Clean and free a new dynamic library control block
*
* INPUTS:
*     dynlib == dynamic library control block to free
*********************************************************************/
static void
    free_dynlib_cb (agt_dynlib_cb_t *dynlib)
{

    if (dynlib->modname) {
        m__free(dynlib->modname);
    }
    if (dynlib->revision) {
        m__free(dynlib->revision);
    }
    m__free(dynlib);

} /* free_dynlib_cb */


/********************************************************************
* FUNCTION set_initial_transaction_id
*
* Set the last_transaction_id for the running config
* Will check for sys:transaction-id leaf in config->root
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    set_initial_transaction_id (void)
{
    cfg_template_t *cfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (cfg == NULL) {
        return ERR_NCX_CFG_NOT_FOUND;
    }
    if (cfg->root == NULL) {
        return ERR_NCX_EMPTY_VAL;
    }

    agt_profile_t *profile = agt_get_profile();
    status_t res = NO_ERR;
    boolean  foundfile = FALSE;

    /* figure out which transaction ID file to use */
    if (profile->agt_startup_txid_file == NULL) {
        /* search for the default startup-cfg.xml filename */
        xmlChar *fname = ncxmod_find_data_file(NCX_DEF_STARTUP_TXID_FILE, 
                                               FALSE, &res);
        if (fname == NULL || res != NO_ERR || *fname == 0) {
            /* need to set the default startup transaction ID file name */
            log_debug("\nSetting initial transaction ID file to default");
            if (fname) {
                m__free(fname);
            }
            res = NO_ERR;
            fname = agt_get_startup_filespec(&res);
            if (fname == NULL || res != NO_ERR) {
                if (res == NO_ERR) {
                    res = ERR_NCX_OPERATION_FAILED;
                }
                log_error("\nFailed to set initial transaction ID file (%s)",
                          get_error_string(res));
                if (fname) {
                    m__free(fname);
                }
                return res;
            }
            /* get dir part and append the TXID file name to it */
            uint32 fnamelen = xml_strlen(fname);
            xmlChar *str = &fname[fnamelen - 1];
            while (str >= fname && *str && *str != NCX_PATHSEP_CH) {
                str--;
            }
            if (*str != NCX_PATHSEP_CH) {
                log_error("\nFailed to set initial transaction ID file");
                m__free(fname);
                return ERR_NCX_INVALID_VALUE;
            }

            /* copy base filespec + 1 for the path-sep-ch */
            uint32 baselen = (uint32)(str - fname) + 1;
            uint32 newlen =  baselen + xml_strlen(NCX_DEF_STARTUP_TXID_FILE);
            xmlChar *newstr = m__getMem(newlen + 1);
            if (newstr == NULL) {
                m__free(fname);
                return ERR_INTERNAL_MEM;
            }
            str = newstr;
            str += xml_strncpy(str, fname, baselen);
            xml_strcpy(str, NCX_DEF_STARTUP_TXID_FILE);
            m__free(fname);
            profile->agt_startup_txid_file = newstr; // pass off memory here
        } else {
            foundfile = TRUE;
            profile->agt_startup_txid_file = fname; // pass off memory here
        }
    }

    /* initialize the starting transaction ID in the config module */
    res = agt_cfg_init_transactions(profile->agt_startup_txid_file, foundfile);
    if (res != NO_ERR) {
        log_error("\nError: cfg-init transaction ID failed (%s)",
                  get_error_string(res));
    }
    return res;

} /* set_initial_transaction_id */


/**************    E X T E R N A L   F U N C T I O N S **********/


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
status_t 
    agt_init1 (int argc,
               char *argv[],
               boolean *showver,
               help_mode_t *showhelpmode)
{
    status_t  res;

    if (agt_init_done) {
        return NO_ERR;
    }

    log_debug3("\nServer Init Starting...");

    res = NO_ERR;

    /* initialize shutdown variables */
    agt_shutdown = FALSE;
    agt_shutdown_started = FALSE;
    agt_shutmode = NCX_SHUT_NONE;
    dlq_createSQue(&agt_dynlibQ);

    *showver = FALSE;
    *showhelpmode = HELP_MODE_NONE;

    /* allow cleanup to run even if this fn does not complete */
    agt_init_done = TRUE;

    init_server_profile();

    /* setup $HOME/.yuma directory */
    res = ncxmod_setup_yumadir();
    if (res != NO_ERR) {
        return res;
    }

    /* get the command line params and also any config file params */
    res = agt_cli_process_input(argc, argv, &agt_profile, showver, 
                                showhelpmode);
    if (res != NO_ERR) {
        return res;
    } /* else the agt_profile is filled in */

    /* check if quick exit mode */
    if (*showver || (*showhelpmode != HELP_MODE_NONE)) {
        return NO_ERR;
    }

    /* loglevel and log file already set */
    return res;

} /* agt_init1 */


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
status_t 
    agt_init2 (void)
{
    val_value_t        *clivalset;
    ncx_module_t       *retmod;
    val_value_t        *val;
    agt_dynlib_cb_t    *dynlib;
    xmlChar            *savestr, *revision, savechar;
    cfg_template_t     *cfg;
    status_t            res;
    uint32              modlen;
    boolean             startup_loaded;

    log_debug3("\nServer Init-2 Starting...");

    startup_loaded = FALSE;

    /* init user callback support */
    agt_cb_init();
    agt_commit_complete_init();

    /* initial signal handler first to allow clean exit */
    agt_signal_init();

    /* initialize the server timer service */
    agt_timer_init();
    
    /* initialize the RPC server callback structures */
    res = agt_rpc_init();
    if (res != NO_ERR) {
        return res;
    }

    /* initialize the NCX connect handler */
    res = agt_connect_init();
    if (res != NO_ERR) {
        return res;
    }

    /* initialize the NCX hello handler */
    res = agt_hello_init();
    if (res != NO_ERR) {
        return res;
    }

    /* setup an empty <running> config 
     * The config state is still CFG_ST_INIT
     * so no user access can occur yet (except OP_LOAD by root)
     */
    res = cfg_init_static_db(NCX_CFGID_RUNNING);
    if (res != NO_ERR) {
        return res;
    }

    /* set the 'ordered-by system' sorted/not-sorted flag */
    ncx_set_system_sorted(agt_profile.agt_system_sorted);

    /* set the 'top-level mandatory objects allowed' flag */
    ncx_set_top_mandatory_allowed(!agt_profile.agt_running_error);

    /*** All Server profile parameters should be set by now ***/

    /* must set the server capabilities after the profile is set */
    res = agt_cap_set_caps(agt_profile.agt_targ, 
                           agt_profile.agt_start,
                           agt_profile.agt_defaultStyle);
    if (res != NO_ERR) {
        return res;
    }

    /* setup the candidate config if it is used */
    if (agt_profile.agt_targ==NCX_AGT_TARG_CANDIDATE) {
        res = cfg_init_static_db(NCX_CFGID_CANDIDATE);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* setup the startup config if it is used */
    if (agt_profile.agt_start==NCX_AGT_START_DISTINCT) {
        res = cfg_init_static_db(NCX_CFGID_STARTUP);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* initialize the server access control model */
    res = agt_acm_init();
    if (res != NO_ERR) {
        return res;
    }

    /* initialize the session handler data structures */
    agt_ses_init();

    /* load the system module */
    res = agt_sys_init();
    if (res != NO_ERR) {
        return res;
    }

    /* load the NETCONF state monitoring data model module */
    res = agt_state_init();
    if (res != NO_ERR) {
        return res;
    }

    /* load the NETCONF Notifications data model module */
    res = agt_not_init();
    if (res != NO_ERR) {
        return res;
    }

    /* load the NETCONF /proc monitoring data model module */
    res = agt_proc_init();
    if (res != NO_ERR) {
        return res;
    }

    /* load the partial lock module */
    res = y_ietf_netconf_partial_lock_init
        (y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock,
         NULL);
    if (res != NO_ERR) {
        return res;
    }

    /* load the NETCONF interface monitoring data model module */
    res = agt_if_init();
    if (res != NO_ERR) {
        return res;
    }

    /* initialize the NCX server core callback functions.
     * the schema (yuma-netconf.yang) for these callbacks was 
     * already loaded in the common ncx_init
     * */
    res = agt_ncx_init();
    if (res != NO_ERR) {
        return res;
    }

    /* load the yuma-time-filter module */
    res = y_yuma_time_filter_init
        (y_yuma_time_filter_M_yuma_time_filter, NULL);
    if (res != NO_ERR) {
        return res;
    }

    /* load the yuma-arp module */
    res = y_yuma_arp_init(y_yuma_arp_M_yuma_arp, NULL);
    if (res != NO_ERR) {
        return res;
    }

    /* check the module parameter set from CLI or conf file
     * for any modules to pre-load
     */
    res = NO_ERR;
    clivalset = agt_cli_get_valset();
    if (clivalset) {

        if (LOGDEBUG) {
            log_debug("\n\nnetconfd final CLI + .conf parameters:\n");
            val_dump_value_max(clivalset, 0, NCX_DEF_INDENT, DUMP_VAL_LOG,
                               NCX_DISPLAY_MODE_PLAIN, FALSE, /* withmeta */
                               TRUE /* config only */);
            log_debug("\n");
        }

        /* first check if there are any deviations to load */
        val = val_find_child(clivalset, NCXMOD_NETCONFD, NCX_EL_DEVIATION);
        while (val) {
            res = ncxmod_load_deviation(VAL_STR(val), 
                                        &agt_profile.agt_savedevQ);
            if (res != NO_ERR) {
                return res;
            } else {
                val = val_find_next_child(clivalset,
                                          NCXMOD_NETCONFD,
                                          NCX_EL_DEVIATION,
                                          val);
            }
        }

        val = val_find_child(clivalset, NCXMOD_NETCONFD, NCX_EL_MODULE);

        /* attempt all dynamically loaded modules */
        while (val && res == NO_ERR) {
            /* see if the revision is present in the
             * module parameter or not
             */
            modlen = 0;
            revision = NULL;
            savestr = NULL;
            savechar = '\0';

            if (yang_split_filename(VAL_STR(val), &modlen)) {
                savestr = &(VAL_STR(val)[modlen]);
                savechar = *savestr;
                *savestr = '\0';
                revision = savestr + 1;
            }

            retmod = ncx_find_module(VAL_STR(val), revision);
            if (retmod == NULL) {
#ifdef STATIC_SERVER
                /* load just the module
                 * SIL initialization is assumed to be
                 * handled elsewhere
                 */
                res = ncxmod_load_module(VAL_STR(val),
                                         revision,
                                         &agt_profile.agt_savedevQ,
                                         &retmod);
#else
                /* load the SIL and it will load its own module */
                res = agt_load_sil_code(VAL_STR(val), revision, FALSE);
                if (res == ERR_NCX_SKIPPED) {
                    log_warn("\nWarning: SIL code for module '%s' not found",
                             VAL_STR(val));
                    res = ncxmod_load_module(VAL_STR(val),
                                             revision,
                                             &agt_profile.agt_savedevQ,
                                             &retmod);
                }
#endif
            } else {
                log_info("\nCLI: Skipping 'module' parm '%s', already loaded",
                         VAL_STR(val));
            }

            if (savestr != NULL) {
                *savestr = savechar;
            }

            if (res == NO_ERR) {
                val = val_find_next_child(clivalset,
                                          NCXMOD_NETCONFD,
                                          NCX_EL_MODULE,
                                          val);
            }
        }
    }

    /*** ALL INITIAL YANG MODULES SHOULD BE LOADED AT THIS POINT ***/
    if (res != NO_ERR) {
        log_error("\nError: one or more modules could not be loaded");
        return ERR_NCX_OPERATION_FAILED;
    }

    if (ncx_any_mod_errors()) {
        log_error("\nError: one or more pre-loaded YANG modules"
                  " has fatal errors\n");
        return ERR_NCX_OPERATION_FAILED;
    }

    /* set the initial module capabilities in the server <hello> message */
    res = agt_cap_set_modules(&agt_profile);
    if (res != NO_ERR) {
        return res;
    }

    /* prune all the obsolete objects */
    ncx_delete_all_obsolete_objects();

    /* safe to create the commit-task object list now */
    res = agt_val_init_commit_tests();
    if (res != NO_ERR) {
        return res;
    }

    /* get or initialize the startup and running transaction-id */
    res = set_initial_transaction_id();
    if (res != NO_ERR) {
        return res;
    }

    /************* L O A D   R U N N I N G   C O N F I G ***************/

    /* load the NV startup config into the running config if it exists */
    if (agt_profile.agt_factorystartup) {
        /* force the startup to be the factory startup */
        log_info("\nagt: Startup configuration skipped due "
                 "to factory-startup CLI option\n");
    } else if (agt_profile.agt_usestartup) {
        if (LOGDEBUG2) {
            log_debug2("\nAttempting to load running config from startup");
        }
        res = load_running_config(agt_profile.agt_startup, &startup_loaded);
        if (res != NO_ERR) {
            return res;
        }
    } else {
        log_info("\nagt: Startup configuration skipped due "
                 "to no-startup CLI option\n");
    }

    /**  P H A S E   2   I N I T  ****  
     **  add system-config and non-config data
     **  check existing startup config and add factory default
     ** nodes as needed  
     ** Note: if startup_loaded == TRUE then all the config nodes
     ** that may get added during init2 will not get checked
     ** by agt_val_root_check().  It is assumed that SIL modules
     ** will not add invalid data nodes; Any such nodes will only
     ** be detected if a client causes a root check (commit
     ** or edit running config */

    /* load the nacm access control DM module */
    res = agt_acm_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load any system module non-config data */
    res = agt_sys_init2();
    if (res != NO_ERR) {
        return res;
    }
    
    /* load any server state monitoring data */
    res = agt_state_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load any partial lock runtime data */
    res = y_ietf_netconf_partial_lock_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load the notifications callback functions and data */
    res = agt_not_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load the /proc monitoring callback functions and data */
    res = agt_proc_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load the interface monitoring callback functions and data */
    res = agt_if_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* TBD: load the time filter callbacks
     * this currently does not do anything
     */
    res = y_yuma_time_filter_init2();
    if (res != NO_ERR) {
        return res;
    }

    /* load the yuma arp callbacks */
    res = y_yuma_arp_init2();
    if (res != NO_ERR) {
        return res;
    }

#ifdef STATIC_SERVER
    /* init phase 2 for static modules should be 
     * initialized here
     */
#else
    for (dynlib = (agt_dynlib_cb_t *)dlq_firstEntry(&agt_dynlibQ);
         dynlib != NULL;
         dynlib = (agt_dynlib_cb_t *)dlq_nextEntry(dynlib)) {

        if (dynlib->init_status == NO_ERR &&
            !dynlib->init2_done &&
            !dynlib->cleanup_done) {

            if (LOGDEBUG2) {
                log_debug2("\nCalling SIL Init2 fn for module '%s'",
                           dynlib->modname);
            }
            res = (*dynlib->init2fn)();
            dynlib->init2_status = res;
            dynlib->init2_done = TRUE;

            if (res != NO_ERR) {
                log_error("\nError: SIL init2 function "
                          "failed for module '%s' (%s)",
                          dynlib->modname,
                          get_error_string(res));
                return res;

                /* TBD: should cleanup be called and somehow
                 * remove the module that was loaded?
                 * Currently abort entire server startup
                 */
            }
        }
    }
#endif

    /* add any top-level defaults to the running config
     * that were not covered by SIL callbacks
     */
    for (retmod = ncx_get_first_module();
         retmod != NULL;
         retmod = ncx_get_next_module(retmod)) {
        res = agt_set_mod_defaults(retmod);
        if (res != NO_ERR) {
            log_error("\nError: Set mod defaults for module '%s' failed (%s)",
                      retmod->name,
                      get_error_string(res));
            return res;
        }
    }

    /* dump the running config at boot-time */
    if (LOGDEBUG3) {
        if (startup_loaded) {
            log_debug3("\nRunning config contents after all init done");
        } else {
            log_debug3("\nFactory default running config contents");
        }
        cfg = cfg_get_config_id(NCX_CFGID_RUNNING);
        val_dump_value_max(cfg->root, 
                           0,
                           NCX_DEF_INDENT,
                           DUMP_VAL_LOG,
                           NCX_DISPLAY_MODE_PREFIX,
                           FALSE,
                           TRUE);
        
        log_debug3("\n");
    }

    if (startup_loaded) {
        /* the config was loaded */
        /* TBD: run top-level mandatory object checks */
    } else {
        /* the factory default config was used;
         * if so, then the root has not been checked at all 
         * this is needed in case there are modules with top-level
         * mandatory nodes; if so, and init2 phase did not add them
         * then the running config will be invalid */
        xml_msg_hdr_t  mhdr; 
        xml_msg_init_hdr(&mhdr);
        cfg = cfg_get_config_id(NCX_CFGID_RUNNING);

        /* allocate a transaction control block */
        agt_cfg_transaction_t *txcb = 
            agt_cfg_new_transaction(NCX_CFGID_RUNNING, AGT_CFG_EDIT_TYPE_FULL,
                                    FALSE, FALSE, &res);
        if (txcb == NULL || res != NO_ERR) {
            if (res == NO_ERR) {
                res = ERR_NCX_OPERATION_FAILED;
            }
        } else {
            res = agt_val_root_check(NULL, &mhdr, txcb, cfg->root);
            if (res != NO_ERR) {
                agt_profile.agt_load_rootcheck_errors = TRUE;
                agt_profile.agt_config_state = AGT_CFG_STATE_BAD;
                if (agt_profile.agt_running_error) {
                    /* quit if any running config errors */
                    log_error("\nError: validation errors occurred loading the "
                              "factory default database\n");
                } else {
                    /* continue if any running errors */
                    log_warn("\nWarning: validation errors occurred "
                             "loading the factory default database\n");
                    res = NO_ERR;
                }

                /* move any error messages to the config error Q */
                dlq_block_enque(&mhdr.errQ, &cfg->load_errQ);
            } else {
                agt_profile.agt_config_state = AGT_CFG_STATE_OK;
            }
        }
        xml_msg_clean_hdr(&mhdr);
        agt_cfg_free_transaction(txcb);

        /* check to see if factory default startup mode is active */
        if (agt_profile.agt_factorystartup && res == NO_ERR) {
            log_info("\nSaving factory default config to startup config");
            res = agt_ncx_cfg_save(cfg, FALSE);
            if (res == NO_ERR) {
                agt_profile.agt_factorystartup = FALSE;
            }
        }

        if (res != NO_ERR) {
            return res;
        }
    }

    /* allow users to access the configuration databases now */
    cfg_set_state(NCX_CFGID_RUNNING, CFG_ST_READY);

    /* set the correct configuration target */
    if (agt_profile.agt_targ==NCX_AGT_TARG_CANDIDATE) {
        res = cfg_fill_candidate_from_running();
        if (res != NO_ERR) {
            return res;
        }
        cfg_set_state(NCX_CFGID_CANDIDATE, CFG_ST_READY);
        cfg_set_target(NCX_CFGID_CANDIDATE);
    } else {
        cfg_set_target(NCX_CFGID_RUNNING);
    }

    /* setup the startup config only if used */
    if (agt_profile.agt_start==NCX_AGT_START_DISTINCT) {
        cfg_set_state(NCX_CFGID_STARTUP, CFG_ST_READY);
    }

    /* data modules can be accessed now, and still added
     * and deleted dynamically as well
     *
     * No sessions can actually be started until the 
     * agt_ncxserver_run function is called from netconfd_run
     */

    return NO_ERR;

}  /* agt_init2 */


/********************************************************************
* FUNCTION agt_cleanup
*
* Cleanup the Server Library
* 
* TBD -- put platform-specific server cleanup here
*
*********************************************************************/
void
    agt_cleanup (void)
{
    agt_dynlib_cb_t *dynlib;

    if (agt_init_done) {
        log_debug3("\nServer Cleanup Starting...\n");

        /* cleanup all the dynamically loaded modules */
        while (!dlq_empty(&agt_dynlibQ)) {
            dynlib = (agt_dynlib_cb_t *)dlq_deque(&agt_dynlibQ);
#ifndef STATIC_SERVER
            if (!dynlib->cleanup_done) {
                (*dynlib->cleanupfn)();
            }
            dlclose(dynlib->handle);
#endif
            free_dynlib_cb(dynlib);
        }
                

        clean_server_profile();
        agt_acm_cleanup();
        agt_ncx_cleanup();
        agt_hello_cleanup();
        agt_cli_cleanup();
        agt_sys_cleanup();
        agt_state_cleanup();
        agt_not_cleanup();
        agt_proc_cleanup();
        y_ietf_netconf_partial_lock_cleanup();
        agt_if_cleanup();
        y_yuma_time_filter_cleanup();
        y_yuma_arp_cleanup();
        agt_ses_cleanup();
        agt_cap_cleanup();
        agt_rpc_cleanup();
        agt_signal_cleanup();
        agt_timer_cleanup();
        agt_connect_cleanup();
        agt_commit_complete_cleanup();
        agt_cb_cleanup();

        print_errors();

        log_audit_close();
        log_close();

        agt_init_done = FALSE;
    }
}   /* agt_cleanup */


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
agt_profile_t *
    agt_get_profile (void)
{
    return &agt_profile;

} /* agt_get_profile */


/********************************************************************
* FUNCTION agt_request_shutdown
* 
* Request some sort of server shutdown
* 
* INPUTS:
*   mode == requested shutdown mode
*
*********************************************************************/
void
    agt_request_shutdown (ncx_shutdowntyp_t mode)
{
#ifdef DEBUG
    if (mode <= NCX_SHUT_NONE || mode > NCX_SHUT_EXIT) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    /* don't allow the shutdown mode to change in mid-stream */
    if (agt_shutdown_started) {
        return;
    }

    agt_shutdown = TRUE;
    agt_shutdown_started = TRUE;
    agt_shutmode = mode;

}  /* agt_request_shutdown */


/********************************************************************
* FUNCTION agt_shutdown_requested
* 
* Check if some sort of server shutdown is in progress
* 
* RETURNS:
*    TRUE if shutdown mode has been started
*
*********************************************************************/
boolean
    agt_shutdown_requested (void)
{
    return agt_shutdown;

}  /* agt_shutdown_requested */


/********************************************************************
* FUNCTION agt_shutdown_mode_requested
* 
* Check what shutdown mode was requested
* 
* RETURNS:
*    shutdown mode
*
*********************************************************************/
ncx_shutdowntyp_t
    agt_shutdown_mode_requested (void)
{
    return agt_shutmode;

}  /* agt_shutdown_mode_requested */


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
const xmlChar *
    agt_cbtype_name (agt_cbtyp_t cbtyp)
{
    switch (cbtyp) {
    case AGT_CB_VALIDATE:
        return (const xmlChar *)"validate";
    case AGT_CB_APPLY:
        return (const xmlChar *)"apply";
    case AGT_CB_COMMIT:
        return (const xmlChar *)"commit";
    case AGT_CB_ROLLBACK:
        return (const xmlChar *)"rollback";
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return (const xmlChar *)"invalid";
    }
}  /* agt_cbtyp_name */


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
status_t
    agt_load_sil_code (const xmlChar *modname,
                       const xmlChar *revision,
                       boolean cfgloaded)
{
    agt_dynlib_cb_t     *dynlib;
    void                *handle;
    agt_sil_init_fn_t    initfn;
    agt_sil_init2_fn_t   init2fn;
    agt_sil_cleanup_fn_t cleanupfn;
    char                *error;
    xmlChar             *buffer, *p, *pathspec;
    uint32               bufflen;
    status_t             res;

    assert ( modname && "param modname is NULL" );

    res = NO_ERR;
    handle = NULL;
    
    /* create a buffer for the library name
     * and later for function names
     */
    bufflen = xml_strlen(modname) + 32;
    buffer = m__getMem(bufflen);
    if (buffer == NULL) {
        return ERR_INTERNAL_MEM;
    }

    p = buffer;
    p += xml_strcpy(p, (const xmlChar *)"lib");    
    p += xml_strcpy(p, modname);
    xml_strcpy(p, (const xmlChar *)".so");

    /* try to find it directly for loading */
    pathspec = ncxmod_find_sil_file(buffer, FALSE, &res);
    if (pathspec == NULL) {
        m__free(buffer);
        return ERR_NCX_SKIPPED;
    }

    handle = dlopen((const char *)pathspec, RTLD_NOW);
    if (handle == NULL) {
        log_error("\nError: could not open '%s' (%s)\n", 
                  pathspec,
                  dlerror());
        m__free(buffer);
        m__free(pathspec);
        return ERR_NCX_OPERATION_FAILED;
    } else if (LOGDEBUG2) {
        log_debug2("\nOpened SIL (%s) OK", pathspec);
    }

    m__free(pathspec);
    pathspec = NULL;

    p = buffer;
    p += xml_strcpy(p, Y_PREFIX);
    p += ncx_copy_c_safe_str(p, modname);
    xml_strcpy(p, INIT_SUFFIX);

    initfn = dlsym(handle, (const char *)buffer);
    error = dlerror();
    if (error != NULL) {
        log_error("\nError: could not open '%s' (%s)\n", 
                  buffer,
                  dlerror());
        m__free(buffer);
        dlclose(handle);
        return ERR_NCX_OPERATION_FAILED;
    }

    xml_strcpy(p, INIT2_SUFFIX);
    init2fn = dlsym(handle, (const char *)buffer);
    error = dlerror();
    if (error != NULL) {
        log_error("\nError: could not open '%s' (%s)\n", 
                  buffer,
                  dlerror());
        m__free(buffer);
        dlclose(handle);
        return ERR_NCX_OPERATION_FAILED;
    }

    xml_strcpy(p, CLEANUP_SUFFIX);
    cleanupfn = dlsym(handle, (const char *)buffer);
    error = dlerror();
    if (error != NULL) {
        log_error("\nError: could not open '%s' (%s)\n", 
                  buffer,
                  dlerror());
        m__free(buffer);
        dlclose(handle);
        return ERR_NCX_OPERATION_FAILED;
    }

    if (LOGDEBUG2) {
        log_debug2("\nLoaded SIL functions OK");
    }

    m__free(buffer);
    buffer = NULL;

    dynlib = new_dynlib_cb();
    if (dynlib == NULL) {
        log_error("\nError: dynlib CB malloc failed");
        dlclose(handle);
        return ERR_INTERNAL_MEM;
    }

    dynlib->handle = handle;

    dynlib->modname = xml_strdup(modname);
    if (dynlib->modname == NULL) {
        log_error("\nError: dynlib CB malloc failed");
        dlclose(handle);
        free_dynlib_cb(dynlib);
        return ERR_INTERNAL_MEM;
    }

    if (revision) {
        dynlib->revision = xml_strdup(revision);
        if (dynlib->revision == NULL) {
            log_error("\nError: dynlib CB malloc failed");
            dlclose(handle);
            free_dynlib_cb(dynlib);
            return ERR_INTERNAL_MEM;
        }
    }
        
    dynlib->initfn = initfn;
    dynlib->init2fn = init2fn;
    dynlib->cleanupfn = cleanupfn;
    dlq_enque(dynlib, &agt_dynlibQ);

    res = (*initfn)(modname, revision);
    dynlib->init_status = res;
    if (res != NO_ERR) {
        log_error("\nError: SIL init function "
                  "failed for module %s (%s)",
                  modname,
                  get_error_string(res));
        (*cleanupfn)();
        dynlib->cleanup_done = TRUE;
        return res;
    } else if (LOGDEBUG2) {
        log_debug2("\nRan SIL init function OK for module '%s'",
                   modname);
    }

    if (cfgloaded) {
        res = (*init2fn)();
        dynlib->init2_done = TRUE;
        dynlib->init2_status = res;
        if (res != NO_ERR) {
            log_error("\nError: SIL init2 function "
                      "failed for module %s (%s)",
                      modname,
                      get_error_string(res));
            (*cleanupfn)();
            dynlib->cleanup_done = TRUE;
            return res;
        } else if (LOGDEBUG2) {
            log_debug2("\nRan SIL init2 function OK for module '%s'",
                       modname);
        }
    }

    return NO_ERR;
            
}  /* agt_load_sil_code */
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
boolean
    agt_advertise_module_needed (const xmlChar *modname)
{
    if (!xml_strcmp(modname, NCXMOD_NETCONF)) {
        return FALSE;
    }

    if (!xml_strcmp(modname, NCXMOD_IETF_NETCONF)) {
        return FALSE;
    }

    if (!xml_strcmp(modname, NCX_EL_XSD)) {
        return FALSE;
    }

    if (!xml_strcmp(modname, NCXMOD_NETCONFD)) {
        return FALSE;
    }

    return TRUE;

} /* agt_advertise_module_needed */


/* END file agt.c */
