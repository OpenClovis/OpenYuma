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
/*  FILE: agt_cli.c

        Set agent CLI parameters

        This module is called before any other agent modules
        have been initialized.  Only core ncx library functions
        can be called while processing agent CLI parameters

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
03feb06      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>

#include "procdefs.h"
#include "agt_cli.h"
#include "cli.h"
#include "conf.h"
#include "help.h"
#include "ncx.h"
#include "ncxconst.h"
#include "obj.h"
#include "status.h"
#include "val.h"
#include "val_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define AGT_CLI_DEBUG 1 */

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                            *
*                                                                   *
*********************************************************************/
static val_value_t *cli_val = NULL;


/********************************************************************
* FUNCTION set_server_profile
* 
* Get the initial agent profile variables for now
* Set the agt_profile data structure
*
* INPUTS:
*   valset == value set for CLI parsing or NULL if none
*   agt_profile == pointer to profile struct to fill in
*
* OUTPUTS:
*   *agt_profile is filled in with params of defaults
*
*********************************************************************/
static void
    set_server_profile (val_value_t *valset,
                       agt_profile_t *agt_profile)
{
    val_value_t  *val;
    uint32        i;
    boolean       done;

    /* check if there is any CLI data to read */
    if (valset == NULL) {
        /* assumes agt_profile already has default values */
        return;
    }

    /* check all the netconfd CLI parameters;
     * follow the order in netconfd.yang since
     * no action will be taken until all params are collected
     *
     * conf=filespec param checked externally
     */

    /* get access-control param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_ACCESS_CONTROL);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_accesscontrol = VAL_ENUM_NAME(val);
    }
    if (agt_profile->agt_accesscontrol) {
        if (!xml_strcmp(agt_profile->agt_accesscontrol,
                        NCX_EL_ENFORCING)) {
            agt_profile->agt_accesscontrol_enum = 
                AGT_ACMOD_ENFORCING;
        } else if (!xml_strcmp(agt_profile->agt_accesscontrol,
                               NCX_EL_PERMISSIVE)) {
            agt_profile->agt_accesscontrol_enum = 
                AGT_ACMOD_PERMISSIVE;
        } else if (!xml_strcmp(agt_profile->agt_accesscontrol,
                               NCX_EL_DISABLED)) {
            agt_profile->agt_accesscontrol_enum = 
                AGT_ACMOD_DISABLED;
        } else if (!xml_strcmp(agt_profile->agt_accesscontrol,
                               NCX_EL_OFF)) {
            agt_profile->agt_accesscontrol_enum = 
                AGT_ACMOD_OFF;
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
            agt_profile->agt_accesscontrol_enum = 
                AGT_ACMOD_ENFORCING;
        }
    } else {
        agt_profile->agt_accesscontrol_enum = AGT_ACMOD_ENFORCING;
    }

    /* get default-style param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_DEFAULT_STYLE);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_defaultStyle = VAL_ENUM_NAME(val);
        agt_profile->agt_defaultStyleEnum = 
            ncx_get_withdefaults_enum(VAL_ENUM_NAME(val));
    }

    /* help parameter checked externally */

    /* the logging parameters might have already been
     * set in the bootstrap CLI (cli_parse_raw)
     * they may get reset here, if the conf file has
     * a different value selected
     */

    /* get delete-empty-npcontainters param */
    val = val_find_child(valset, AGT_CLI_MODULE,
                         AGT_CLI_DELETE_EMPTY_NPCONTAINERS);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_delete_empty_npcontainers = VAL_BOOL(val);
    } else {
        agt_profile->agt_delete_empty_npcontainers = 
            AGT_DEF_DELETE_EMPTY_NP;
    }        

    /* get indent param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_INDENT);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_indent = (int32)VAL_UINT(val);
    }

    /* get log param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_LOG);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_logfile = VAL_STR(val);
    }

    /* get log-append param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_LOGAPPEND);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_logappend = TRUE;
    }

    /* get log-level param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_LOGLEVEL);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_loglevel = 
            log_get_debug_level_enum((const char *)VAL_STR(val));
    }

    /* get leaf-list port parameter */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_PORT);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_ports[0] = VAL_UINT16(val);

        val = val_find_next_child(valset, AGT_CLI_MODULE,
                                  NCX_EL_PORT, val);
        while (val) {
            done = FALSE;
            for (i = 0;         i < AGT_MAX_PORTS && !done; i++) {
                if (agt_profile->agt_ports[i] == VAL_UINT16(val)) {
                    done = TRUE;
                } else if (agt_profile->agt_ports[i] == 0) {
                    agt_profile->agt_ports[i] = VAL_UINT16(val);
                    done = TRUE;
                }
            }
            val = val_find_next_child(valset, AGT_CLI_MODULE,
                                      NCX_EL_PORT, val);
        }
    }

    /* eventlog-size param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_EVENTLOG_SIZE);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_eventlog_size = VAL_UINT(val);
    }

    /* get hello-timeout param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_HELLO_TIMEOUT);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_hello_timeout = VAL_UINT(val);
    }

    /* get idle-timeout param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_IDLE_TIMEOUT);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_idle_timeout = VAL_UINT(val);
    }

    /* max-burst param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_MAX_BURST);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_maxburst = VAL_UINT(val);
    }

    /* running-error param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_RUNNING_ERROR);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val),
                        AGT_CLI_STARTUP_STOP)) {
            agt_profile->agt_running_error = TRUE;
        }
    }

    /* start choice:
     * cli module will check that only 1 of the following 3
     * parms are actually entered
     * start choice 1: get no-startup param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_NOSTARTUP);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_usestartup = FALSE;
    }

    /* start choice 2: OR get startup param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_STARTUP);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_startup = VAL_STR(val);
    }

    /* start choice 3: OR get factory-startup param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_FACTORY_STARTUP);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_factorystartup = TRUE;
    }

    /* startup-error param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_STARTUP_ERROR);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val),
                        AGT_CLI_STARTUP_STOP)) {
            agt_profile->agt_startup_error = TRUE;
        }
    }

    /* superuser param */
    val = val_find_child(valset, AGT_CLI_MODULE, AGT_CLI_SUPERUSER);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_superuser = VAL_STR(val);
    }

    /* get target param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_TARGET);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val), NCX_EL_RUNNING)) {
            agt_profile->agt_targ = NCX_AGT_TARG_RUNNING;
        } else if (!xml_strcmp(VAL_ENUM_NAME(val), 
                               NCX_EL_CANDIDATE)) {
            agt_profile->agt_targ = NCX_AGT_TARG_CANDIDATE;
        }
    }

    /* get the :startup capability setting */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_WITH_STARTUP);
    if (val && val->res == NO_ERR) {
        if (VAL_BOOL(val)) {
            agt_profile->agt_start = NCX_AGT_START_DISTINCT;
            agt_profile->agt_has_startup = TRUE;
        } else {
            agt_profile->agt_start = NCX_AGT_START_MIRROR;
            agt_profile->agt_has_startup = FALSE;
        }
    }

    /* check the subdirs parameter */
    val_set_subdirs_parm(valset);

    /* version param handled externally */

    /* get usexmlorder param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_USEXMLORDER);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_xmlorder = TRUE;
    }

    /* get the :url capability setting */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_WITH_URL);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_useurl = VAL_BOOL(val);
    }

    /* get with-validate param */
    val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_WITH_VALIDATE);
    if (val && val->res == NO_ERR) {
        agt_profile->agt_usevalidate = VAL_BOOL(val);
    }

} /* set_server_profile */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_cli_process_input
*
* Process the param line parameters against the hardwired
* parmset for the netconfd program
*
* INPUTS:
*    argc == argument count
*    argv == array of command line argument strings
*    agt_profile == agent profile struct to fill in
*    showver == address of version return quick-exit status
*    showhelpmode == address of help return quick-exit status
*
* OUTPUTS:
*    *agt_profile is filled in, with parms gathered or defaults
*    *showver == TRUE if user requsted version quick-exit mode
*    *showhelpmode == requested help mode 
*                     (none, breief, normal, full)
*
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
status_t
    agt_cli_process_input (int argc,
                           char *argv[],
                           agt_profile_t *agt_profile,
                           boolean *showver,
                           help_mode_t *showhelpmode)
{
    ncx_module_t          *mod;
    obj_template_t        *obj;
    val_value_t           *valset, *val;
    FILE                  *fp;
    status_t               res;
    boolean                test;

#ifdef DEBUG
    if (!argv || !agt_profile || !showver || !showhelpmode) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *showver = FALSE;
    *showhelpmode = HELP_MODE_NONE;

    /* find the parmset definition in the registry */
    obj = NULL;
    mod = ncx_find_module(AGT_CLI_MODULE, NULL);
    if (mod) {
        obj = ncx_find_object(mod, AGT_CLI_CONTAINER);
    }
    if (!obj) {
        log_error("\nError: netconfd module with CLI definitions not loaded");
        return ERR_NCX_NOT_FOUND;
    }

    /* parse the command line against the object template */
    res = NO_ERR;
    valset = NULL;
    if (argc > 0) {
        valset = cli_parse(NULL,
                           argc, 
                           argv, 
                           obj,
                           FULLTEST, 
                           PLAINMODE, 
                           TRUE, 
                           CLI_MODE_PROGRAM,
                           &res);
        if (res != NO_ERR) {
            if (valset) {
                val_free_value(valset);
            }
            return res;
        }
    }

    if (valset != NULL) {
        /* transfer the parmset values */
        set_server_profile(valset, agt_profile);

        /* next get any params from the conf file */
        val = val_find_child(valset, 
                             AGT_CLI_MODULE, 
                             NCX_EL_CONFIG);
        if (val) {
            if (val->res == NO_ERR) {
                /* try the specified config location */
                agt_profile->agt_conffile = VAL_STR(val);
                res = conf_parse_val_from_filespec(VAL_STR(val), 
                                                   valset, 
                                                   TRUE, 
                                                   TRUE);
                if (res != NO_ERR) {
                    val_free_value(valset);
                    return res;
                } else {
                    /* transfer the parmset values again */
                    set_server_profile(valset, agt_profile);
                }
            }
        } else {
            fp = fopen((const char *)AGT_DEF_CONF_FILE, "r");
            if (fp != NULL) {
                fclose(fp);

                /* use default config location */
                res = conf_parse_val_from_filespec(AGT_DEF_CONF_FILE, 
                                                   valset, 
                                                   TRUE, 
                                                   TRUE);
                if (res != NO_ERR) {
                    val_free_value(valset);
                    return res;
                } else {
                    /* transfer the parmset values again */
                    set_server_profile(valset, agt_profile);
                }
            }
        }

        /* set the logging control parameters */
        val_set_logging_parms(valset);

        /* audit-log-append param */
        val = val_find_child(valset, 
                             AGT_CLI_MODULE, 
                             NCX_EL_AUDIT_LOG_APPEND);
        if (val && val->res == NO_ERR) {
            test = TRUE;
        } else {
            test = FALSE;
        }

        /* audit-log param */
        val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_AUDIT_LOG);
        if (val && val->res == NO_ERR) {
            xmlChar *filespec = ncx_get_source(VAL_STR(val), &res);
            if (filespec == NULL) {
                log_error("\nError: get source for audit log failed");
                return res;
            }
            res = log_audit_open((const char *)filespec, test, TRUE);
            if (res == NO_ERR) {
                if (LOGDEBUG) {
                    log_debug("\nAudit log '%s' opened for %s",
                              filespec,
                              (test) ? "append" : "write");
                }
            } else {
                log_error("\nError: open audit log '%s' failed",
                          filespec);
            }
            m__free(filespec);
            if (res != NO_ERR) {
                return res;
            }
        }

        /* set the file search path parms */
        res = val_set_path_parms(valset);
        if (res != NO_ERR) {
            return res;
        }

        /* set the warning control parameters */
        res = val_set_warning_parms(valset);
        if (res != NO_ERR) {
            return res;
        }

        /* set the feature code generation parameters */
        res = val_set_feature_parms(valset);
        if (res != NO_ERR) {
            return res;
        }

        /* check the subdirs parameter */
        res = val_set_subdirs_parm(valset);
        if (res != NO_ERR) {
            return res;
        }

        /* check the protocols parameter */
        res = val_set_protocols_parm(valset);
        if (res != NO_ERR) {
            return res;
        }

        /* check the system-sorted param */
        val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_SYSTEM_SORTED);
        if (val && val->res == NO_ERR) {
            agt_profile->agt_system_sorted = VAL_BOOL(val);
        }

        /* version param handled externally */

        /* check if version mode requested */
        val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_VERSION);
        *showver = (val) ? TRUE : FALSE;

        /* check if help mode requested */
        val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_HELP);
        if (val) {
            *showhelpmode = HELP_MODE_NORMAL;

            /* help submode parameter (brief/normal/full) */
            val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_BRIEF);
            if (val) {
                *showhelpmode = HELP_MODE_BRIEF;
            } else {
                /* full parameter */
                val = val_find_child(valset, AGT_CLI_MODULE, NCX_EL_FULL);
                if (val) {
                    *showhelpmode = HELP_MODE_FULL;
                }
            }
        }
    }

    /* cleanup and exit
     * handoff the malloced 'valset' memory here 
     */
    cli_val = valset;

    return res;

} /* agt_cli_process_input */


/********************************************************************
* FUNCTION agt_cli_get_valset
*
*   Retrieve the command line parameter set from boot time
*
* RETURNS:
*    pointer to parmset or NULL if none
*********************************************************************/
val_value_t *
    agt_cli_get_valset (void)
{
    return cli_val;
} /* agt_cli_get_valset */


/********************************************************************
* FUNCTION agt_cli_cleanup
*
*   Cleanup the module static data
*
*********************************************************************/
void
    agt_cli_cleanup (void)
{
    if (cli_val) {
        val_free_value(cli_val);
        cli_val = NULL;
    }

} /* agt_cli_cleanup */

/* END file agt_cli.c */
