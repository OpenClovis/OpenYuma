/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * Copyright (c) 2012, YumaWorks, Inc., All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: ydump.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
01-nov-06    abb      begun
26-mar-10    abb      split out into library ydump

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "procdefs.h"
#include "c.h"
#include "cli.h"
#include "conf.h"
#include "cyang.h"
#include "h.h"
#include "help.h"
#include "html.h"
#include "log.h"
#include "ncx.h"
#include "ncx_feature.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "sql.h"
#include "status.h"
#include "tg2.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "xsd.h"
#include "xsd_util.h"
#include "yang.h"
#include "yangconst.h"
#include "yangdump.h"
#include "yangdump_util.h"
#include "yangstats.h"
#include "yangyin.h"
#include "ydump.h"


/********************************************************************
* FUNCTION init_cvtparms
*
* Initialize a preallocated yangdump_cvtparms_t struct
*
* INPUTS:
*    cp == conversion parms struct to init
*    allowcode == TRUE to allow code generation
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    init_cvtparms (yangdump_cvtparms_t *cp,
                   boolean allowcode)
{

    /* memset(cp, 0x0, sizeof(yangdump_cvtparms_t)); */
    cp->allowcode = allowcode;
    dlq_createSQue(&cp->savedevQ);
    cp->buff = m__getMem(YANGDUMP_BUFFSIZE);
    if (!cp->buff) {
        return ERR_INTERNAL_MEM;
    }
    cp->bufflen = YANGDUMP_BUFFSIZE;
    return NO_ERR;

} /* init_cvtparms */


/********************************************************************
* FUNCTION init_cvtparms_stats
*
* Initialize a preallocated yangdump_cvtparms_t struct
* for statistics collection
*
* INPUTS:
*    cp == conversion parms struct to init
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    init_cvtparms_stats (yangdump_cvtparms_t *cp)
{
    cp->cur_stats = m__getObj(yangdump_stats_t);
    if (cp->cur_stats == NULL) {
        return ERR_INTERNAL_MEM;
    }
    memset(cp->cur_stats, 0x0, sizeof(yangdump_stats_t));

    if (cp->stat_totals != YANGDUMP_TOTALS_NONE) {
        cp->final_stats = m__getObj(yangdump_stats_t);
        if (cp->final_stats == NULL) {
            return ERR_INTERNAL_MEM;
        }
        memset(cp->final_stats, 0x0, sizeof(yangdump_stats_t));
    }

    return NO_ERR;

} /* init_cvtparms_stats */


/********************************************************************
* FUNCTION clean_cvtparms
*
* Clean a preallocated yangdump_cvtparms_t struct
* but do not free it
*
* INPUTS:
*    cp == conversion parms struct to clean
*
*********************************************************************/
static void
    clean_cvtparms (yangdump_cvtparms_t *cp)
{
    if (cp->cli_val) {
        val_free_value(cp->cli_val);
    }
    if (cp->mod) {
        ncx_free_module(cp->mod);
    }
    if (cp->srcfile) {
        m__free(cp->srcfile);
    }
    if (cp->module) {
        m__free(cp->module);
    }
    if (cp->full_output) {
        m__free(cp->full_output);
    }
    if (cp->buff) {
        m__free(cp->buff);
    }
    if (cp->cur_stats) {
        m__free(cp->cur_stats);
    }
    if (cp->final_stats) {
        m__free(cp->final_stats);
    }

    ncx_clean_save_deviationsQ(&cp->savedevQ);

    memset(cp, 0x0, sizeof(yangdump_cvtparms_t));

} /* clean_cvtparms */


/********************************************************************
* FUNCTION pr_err
*
* Print an error message
*
* INPUTS:
*    res == error result
*
*********************************************************************/
static void
    pr_err (status_t  res)
{
    const char *msg;

    msg = get_error_string(res);
    log_error("\n%s: Error exit (%s)\n", PROGNAME, msg);

} /* pr_err */


/********************************************************************
* FUNCTION pr_usage
*
* Print a usage message
*
*********************************************************************/
static void
    pr_usage (void)
{
    log_error("\nError: No parameters entered."
              "\nTry '%s --help' for usage details\n", PROGNAME);

} /* pr_usage */


/********************************************************************
* FUNCTION process_cli_input
*
* Process the param line parameters against the hardwired
* parmset for the yangdump program
*
* get all the parms and store them in the cvtparms struct
*
* INPUTS:
*    argc == argument count
*    argv == array of command line argument strings
*    cp == address of returned values
*
* OUTPUTS:
*    parmset values will be stored in *cvtparms if NO_ERR
*    errors will be written to STDOUT
*
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
static status_t
    process_cli_input (int argc,
                       char *argv[],
                       yangdump_cvtparms_t  *cp)
{
    obj_template_t        *obj;
    ncx_module_t          *mod;
    val_value_t           *valset, *val;
    status_t               res;
    boolean                defs_done;

    res = NO_ERR;
    valset = NULL;
    obj = NULL;
    defs_done = FALSE;

    /* find the CLI container definition */
    mod = ncx_find_module(YANGDUMP_MOD, NULL);
    if (mod != NULL) {
        obj = ncx_find_object(mod, YANGDUMP_CONTAINER);
    }

    if (obj == NULL) {
        log_error("\nError: yangdump module with CLI definitions not loaded");
        res = ERR_NCX_NOT_FOUND;
    }

    /* parse the command line against the object template */
    if (res == NO_ERR) {
        if (argv != NULL) {
            valset = cli_parse(NULL,
                               argc, 
                               argv, 
                               obj,
                               FULLTEST, 
                               PLAINMODE, 
                               TRUE,
                               CLI_MODE_PROGRAM,
                               &res);
            defs_done = TRUE;
        } else {
            valset = val_new_value();
            if (valset == NULL) {
                return ERR_INTERNAL_MEM;
            }
            val_init_from_template(valset, obj);
        }
    }
            
    if (res != NO_ERR) {
        if (valset != NULL) {
            val_free_value(valset);
        }
        return res;
    } else if (valset == NULL) {
        pr_usage();
        return ERR_NCX_SKIPPED;
    } else {
        cp->cli_val = valset;
    }

    /* next get any params from the conf file */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_CONFIG);
    if (val) {
        if (val->res == NO_ERR) {
            /* try the specified config location */
            cp->config = VAL_STR(val);
            res = conf_parse_val_from_filespec(cp->config, 
                                               valset, 
                                               TRUE, 
                                               TRUE);
            if (res != NO_ERR) {
                return res;
            }
        }
    } else {
        /* try default config location */
        res = conf_parse_val_from_filespec(YANGDUMP_DEF_CONFIG,
                                           valset, 
                                           TRUE, 
                                           FALSE);
        if (res != NO_ERR) {
            return res;
        }
    }

    if (!defs_done) {
        res = val_add_defaults(valset, NULL, NULL, FALSE);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* set the logging control parameters */
    val_set_logging_parms(valset);

    /* set the file search path parms */
    val_set_path_parms(valset);

    /* set the warning control parameters */
    val_set_warning_parms(valset);

    /* set the feature code generation parameters */
    val_set_feature_parms(valset);

    /*** ORDER DOES NOT MATTER FOR REST OF PARAMETERS ***/

    /* defnames parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_DEFNAMES);
    if (val && val->res == NO_ERR) {
        cp->defnames = VAL_BOOL(val);
    } else {
        cp->defnames = FALSE;
    }

    /* dependencies parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_DEPENDENCIES);
    if (val && val->res == NO_ERR) {
        cp->dependencies = TRUE;
    }

    /* exports parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_EXPORTS);
    if (val && val->res == NO_ERR) {
        cp->exports = TRUE;
    }

    /* format parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_FORMAT);
    if (val && val->res == NO_ERR) {
        /* format -- use string provided */
        cp->format = 
            ncx_get_cvttyp_enum((const char *)VAL_ENUM_NAME(val));
    } else {
        cp->format = NCX_CVTTYP_NONE;
    }

    /* identifiers parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_IDENTIFIERS);
    if (val && val->res == NO_ERR) {
        cp->identifiers = TRUE;
    }

    /* indent parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_INDENT);
    if (val && val->res == NO_ERR) {
        cp->indent = (int32)VAL_UINT(val);
    } else {
        cp->indent = NCX_DEF_INDENT;
    }

    /* help parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         NCX_EL_HELP);
    if (val && val->res == NO_ERR) {
        cp->helpmode = TRUE;
    }

    /* help submode parameter (brief/normal/full) */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         NCX_EL_BRIEF);
    if (val && val->res == NO_ERR) {
        cp->helpsubmode = HELP_MODE_BRIEF;
    } else {
        /* full parameter */
        val = val_find_child(valset, YANGDUMP_MOD, NCX_EL_FULL);
        if (val && val->res == NO_ERR) {
            cp->helpsubmode = HELP_MODE_FULL;
        } else {
            cp->helpsubmode = HELP_MODE_NORMAL;
        }
    }

    /* html-div parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_HTML_DIV);
    if (val && val->res == NO_ERR) {
        cp->html_div = VAL_BOOL(val);
    } else {
        cp->html_div = FALSE;
    }

    /* html-toc parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_HTML_TOC);
    if (val && val->res == NO_ERR) {
        cp->html_toc = VAL_STR(val);
    } else {
        cp->html_toc = YANGDUMP_DEF_TOC;
    }

    /* module parameter */
    cp->modcount = val_instance_count(valset, 
                                      NULL, 
                                      YANGDUMP_PARM_MODULE);

    /* modversion parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_MODVERSION);
    if (val && val->res == NO_ERR) {
        cp->modversion = TRUE;
    }

    /* subdirs parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         NCX_EL_SUBDIRS);
    if (val && val->res == NO_ERR) {
        cp->subdirs = VAL_BOOL(val);
    } else {
        cp->subdirs = TRUE;
    }

    /* tree-identifiers parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_TREE_IDENTIFIERS);
    if (val && val->res == NO_ERR) {
        cp->tree_identifiers = TRUE;
    }

    /* versionnames */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_VERSIONNAMES);
    if (val && val->res == NO_ERR) {
        cp->versionnames = VAL_BOOL(val);
    } else {
        cp->versionnames = TRUE;
    }

    /* objview parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_OBJVIEW);
    if (val && val->res == NO_ERR) {
        cp->objview = (const char *)VAL_STR(val);
    } else {
        cp->objview = YANGDUMP_DEF_OBJVIEW;
    }

    /* rawmode derived parameter */
    cp->rawmode = strcmp(cp->objview, OBJVIEW_RAW) ? FALSE : TRUE;

    /* output parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_OUTPUT);
    if (val && val->res == NO_ERR) {
        /* output -- use filename provided */
        cp->output = (const char *)VAL_STR(val);
        res = NO_ERR;
        cp->full_output = ncx_get_source_ex(VAL_STR(val), FALSE, &res);
        if (!cp->full_output) {
            return res;
        }
        cp->output_isdir = ncxmod_test_subdir(cp->full_output);
    }

    /* show-errors parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         NCX_EL_SHOW_ERRORS);
    if (val && val->res == NO_ERR) {
        cp->showerrorsmode = TRUE;
    }

    /* simurls parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_SIMURLS);
    if (val && val->res == NO_ERR) {
        cp->simurls = TRUE;
    }

    /* stats parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_STATS);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val),
                        YANGDUMP_PV_STATS_NONE)) {
            cp->stats = YANGDUMP_REPORT_NONE;
        } else {
            cp->collect_stats = TRUE;
            if (!xml_strcmp(VAL_ENUM_NAME(val),
                        YANGDUMP_PV_STATS_BRIEF)) {
                cp->stats = YANGDUMP_REPORT_BRIEF;
            } else if (!xml_strcmp(VAL_ENUM_NAME(val),
                                   YANGDUMP_PV_STATS_BASIC)) {
                cp->stats = YANGDUMP_REPORT_BASIC;
            } else if (!xml_strcmp(VAL_ENUM_NAME(val),
                                   YANGDUMP_PV_STATS_ADVANCED)) {
                cp->stats = YANGDUMP_REPORT_ADVANCED;
            } else if (!xml_strcmp(VAL_ENUM_NAME(val),
                                   YANGDUMP_PV_STATS_ALL)) {
                cp->stats = YANGDUMP_REPORT_ALL;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }

    /* stat-totals parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_STAT_TOTALS);
    if (val && val->res == NO_ERR) {
        if (!xml_strcmp(VAL_ENUM_NAME(val),
                        YANGDUMP_PV_TOTALS_NONE)) {
            cp->stat_totals = YANGDUMP_TOTALS_NONE;
        } else {
            if (!xml_strcmp(VAL_ENUM_NAME(val),
                        YANGDUMP_PV_TOTALS_SUMMARY)) {
                if (cp->collect_stats) {
                    cp->stat_totals = YANGDUMP_TOTALS_SUMMARY;
                }
            } else if (!xml_strcmp(VAL_ENUM_NAME(val),
                                   YANGDUMP_PV_TOTALS_SUMMARY_ONLY)) {
                if (cp->collect_stats) {
                    cp->stat_totals = YANGDUMP_TOTALS_SUMMARY_ONLY;
                }
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    } else if (val == NULL) {
        if (cp->collect_stats) {
            cp->stat_totals = YANGDUMP_TOTALS_SUMMARY;
        }
    }

    /* subtree parameter */
    cp->subtreecount = val_instance_count(valset, 
                                          NULL, 
                                          YANGDUMP_PARM_SUBTREE);

    /* unified parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_UNIFIED);
    if (val && val->res == NO_ERR) {
        cp->unified = VAL_BOOL(val);
    } else {
        cp->unified = FALSE;
    }

    /* urlstart parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_URLSTART);
    if (val && val->res == NO_ERR) {
        cp->urlstart = VAL_STR(val);
    }

    /* version parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         NCX_EL_VERSION);
    if (val && val->res == NO_ERR) {
        cp->versionmode = TRUE;
    }

    /* xsd-schemaloc parameter */
    val = val_find_child(valset, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_XSD_SCHEMALOC);
    if (val && val->res == NO_ERR) {
        cp->schemaloc = VAL_STR(val);
    }

    return NO_ERR;

} /* process_cli_input */


/********************************************************************
 * FUNCTION get_output_session
 * 
 * Malloc and init an output session.
 * Open the correct output file if needed
 *
 * INPUTS:
 *    pcb == parser control block
 *    cp == conversion parameter struct to use
 *    res == address of return status
 *
 * OUTPUTS:
 *  *res == return status
 *
 * RETURNS:
 *   pointer to new session control block, or NULL if some error
 *********************************************************************/
static ses_cb_t *
    get_output_session (yang_pcb_t *pcb,
                        yangdump_cvtparms_t *cp,
                        status_t  *res)
{
    ncx_module_t    *mod;
    FILE            *fp;
    ses_cb_t        *scb;
    xmlChar         *namebuff;

    fp = NULL;
    scb = NULL;
    mod = NULL;
    namebuff = NULL;
    *res = NO_ERR;

    if (pcb != NULL) {
        mod = pcb->top;
        if (mod == NULL) {
            *res = SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        }
    }

    /* open the output file if not STDOUT */
    if (*res == NO_ERR) {
        if (mod != NULL &&
            ((cp->defnames && cp->format != NCX_CVTTYP_NONE) || 
             (cp->output && cp->output_isdir))) {
            namebuff = xsd_make_output_filename(mod, cp);
            if (!namebuff) {
                *res = ERR_INTERNAL_MEM;
            } else {
                fp = fopen((const char *)namebuff, "w");
                if (!fp) {
                    *res = ERR_FIL_OPEN;
                }
            }
        } else if (cp->output) {
            if (!cp->firstdone) {
                fp = fopen(cp->output, "w");
                cp->firstdone = TRUE;
            } else if (cp->subtree) {
                fp = fopen(cp->output, "a");
            } else {
                fp = fopen(cp->output, "w");
            }
            if (!fp) {
                *res = ERR_FIL_OPEN;
            }
        }
    }

    /* get a dummy session control block */
    if (*res == NO_ERR) {
        scb = ses_new_dummy_scb();
        if (!scb) {
            *res = ERR_INTERNAL_MEM;
        } else {
            scb->fp = fp;
            ses_set_mode(scb, SES_MODE_TEXT);
        }
    } 

    if (namebuff) {
        m__free(namebuff);
    }

    return scb;

}  /* get_output_session */


/********************************************************************
 * FUNCTION output_one_module_exports
 * 
 * Generate a list of top-level symbols for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    scb == session control block to use for output
 *    buff == scratch buffer to use 
 *********************************************************************/
static void
    output_one_module_exports (ncx_module_t *mod,
                               ses_cb_t *scb,
                               char *buff)
{
    typ_template_t    *typ;
    grp_template_t    *grp;
    obj_template_t    *obj;
    ext_template_t    *ext;
    ncx_feature_t     *feature;
    boolean            anyout = FALSE;

    if (mod->ismod) {
        anyout = TRUE;
        sprintf(buff, "\nnamespace %s", mod->ns);
        ses_putstr(scb, (const xmlChar *)buff);
        if (mod->prefix) {
            sprintf(buff, "\nprefix %s", mod->prefix);
            ses_putstr(scb, (const xmlChar *)buff);
        } else {
            sprintf(buff, "\nprefix %s", mod->name);
            ses_putstr(scb, (const xmlChar *)buff);
        }
    }

    for (typ = (typ_template_t *)dlq_firstEntry(&mod->typeQ);
         typ != NULL;
         typ = (typ_template_t *)dlq_nextEntry(typ)) {
        sprintf(buff, "\ntypedef %s", typ->name);
        ses_putstr(scb, (const xmlChar *)buff);
        anyout = TRUE;
    }

    for (grp = (grp_template_t *)dlq_firstEntry(&mod->groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {
        sprintf(buff, "\ngrouping %s", grp->name);
        ses_putstr(scb, (const xmlChar *)buff);
        anyout = TRUE;
    }

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {
        switch (obj->objtype) {
        case OBJ_TYP_AUGMENT:
        case OBJ_TYP_USES:
            break;
        default:
            if (obj_is_cloned(obj) && !obj_is_augclone(obj)) {
                break;
            }
            sprintf(buff, "\n%s %s", obj_get_typestr(obj),
                    obj_get_name(obj));
            ses_putstr(scb, (const xmlChar *)buff);
            anyout = TRUE;
        }
    }

    for (ext = (ext_template_t *)dlq_firstEntry(&mod->extensionQ);
         ext != NULL;
         ext = (ext_template_t *)dlq_nextEntry(ext)) {
        sprintf(buff, "\nextension %s", ext->name);
        ses_putstr(scb, (const xmlChar *)buff);
        anyout = TRUE;
    }

    for (feature = (ncx_feature_t *)dlq_firstEntry(&mod->featureQ);
         feature != NULL;
         feature = (ncx_feature_t *)dlq_nextEntry(feature)) {
        sprintf(buff, "\nfeature %s", feature->name);
        ses_putstr(scb, (const xmlChar *)buff);
        anyout = TRUE;
    }

    if (anyout) {
        ses_putchar(scb, '\n');
    }

}   /* output_one_module_exports */


/********************************************************************
 * FUNCTION output_module_exports
 * 
 * Generate a list of top-level symbols for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == coversion param struct to use
 *    scb == session control block to use for output
 *********************************************************************/
static void
    output_module_exports (yang_pcb_t *pcb,
                           yangdump_cvtparms_t *cp,
                           ses_cb_t *scb)
{
    ncx_module_t      *mod;
    yang_node_t       *node;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }


    ses_putstr(scb, (const xmlChar *)"\nexports:");

    /* check subtree mode */
    if (cp->subtree) {
        print_subtree_banner(cp, mod, scb);
    }

    output_one_module_exports(mod, scb, cp->buff);

    if ((cp->unified || cp->onemodule) && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                output_one_module_exports(node->submod, scb, cp->buff);
            }
        }
    }

}   /* output_module_exports */


/********************************************************************
 * FUNCTION output_one_module_dependencies
 * 
 * Generate a list of dependencies for a module
 *
 * INPUTS:
 *    mod == module in progress
 *    scb == session control block to use for output
 *    cp == conversion parameters in use
 *********************************************************************/
static void
    output_one_module_dependencies (ncx_module_t *mod,
                                    ses_cb_t *scb,
                                    yangdump_cvtparms_t *cp)
{
    ncx_module_t      *testmod;
    yang_import_ptr_t *impptr;
    yang_node_t       *node; 
    boolean            anyout = FALSE;

    for (impptr = (yang_import_ptr_t *)dlq_firstEntry(&mod->saveimpQ);
         impptr != NULL;
         impptr = (yang_import_ptr_t *)dlq_nextEntry(impptr)) {

        testmod = ncx_find_module(impptr->modname,
                                  impptr->revision);
        sprintf(cp->buff, "\nimport %s", impptr->modname);
        ses_putstr(scb,(const xmlChar *)cp->buff);
        if (testmod && testmod->version) {
            sprintf(cp->buff, "@%s", testmod->version);
            ses_putstr(scb,(const xmlChar *)cp->buff);                
        }
        anyout = TRUE;
    }
    if (!cp->unified) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {

            sprintf(cp->buff, "\ninclude %s", node->name);
            ses_putstr(scb,(const xmlChar *)cp->buff);
            if (node->submod && node->submod->version) {
                sprintf(cp->buff, "@%s", node->submod->version);
                ses_putstr(scb,(const xmlChar *)cp->buff);
            }
            anyout = TRUE;
        }
    }

    if (anyout) {
        ses_putchar(scb, '\n');
    }

}   /* output_one_module_dependencies */


/********************************************************************
 * FUNCTION output_module_dependencies
 * 
 * Generate a list of dependencies for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block to write output
 *********************************************************************/
static void
    output_module_dependencies (yang_pcb_t *pcb,
                                yangdump_cvtparms_t *cp,
                                ses_cb_t *scb)
{
    ncx_module_t      *mod;
    yang_node_t       *node; 

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }

    ses_putstr(scb, (const xmlChar *)"\ndependencies:");

    /* check subtree mode */
    if (cp->subtree) {
        print_subtree_banner(cp, mod, scb);
    }

    output_one_module_dependencies(mod, scb, cp);

    if ((cp->unified || cp->onemodule) && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                output_one_module_dependencies(node->submod, scb, cp);
            }
        }
    }

}   /* output_module_dependencies */


/********************************************************************
 * FUNCTION output_identifiers
 * 
 * Generate a list of object identifiers for an object
 * YANG support only!!!
 *
 * INPUTS:
 *    datadefQ == Q of obj_template_t to use
 *    scb == session control block for output
 *    buff == scratch buffer to use 
 *    bufflen == size of buff in bytes
 *    treeformat == TRUE for tree format; FALSE for list forma
 *    startindent == startindent amount
 *    indent == indent amount
 *    helpmode == verbosity level
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    output_identifiers (dlq_hdr_t *datadefQ,
                        ses_cb_t *scb,
                        xmlChar *buff,
                        uint32 bufflen,
                        boolean treeformat,
                        uint32 startindent,
                        uint32 indent,
                        help_mode_t helpmode)
{
    status_t res = NO_ERR;
    obj_template_t *obj;
    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj)) {
            continue;  /* skip uses and augment */
        }

        if (!treeformat) {
            uint32 reallen = 0;
            if (helpmode == HELP_MODE_FULL) {
                res = obj_copy_object_id_mod(obj, buff, bufflen, &reallen);
            } else {
                res = obj_copy_object_id(obj, buff, bufflen, &reallen);
            }
            if (res != NO_ERR) {
                log_error("\nError: copy object ID failed (%s)",
                          get_error_string(res));
                return res;
            }
        }

        ses_putchar(scb, '\n');
        if (treeformat) {
            uint32 i;
            for (i=0; i < startindent; i++) {
                ses_putchar(scb, ' ');
            }
        }
        ses_putstr(scb, obj_get_typestr(obj));
        ses_putchar(scb, ' ');
        if (treeformat) {
            ses_putstr(scb, obj_get_name(obj));
        } else {
            ses_putstr(scb, buff);
        }

        dlq_hdr_t *childQ = obj_get_datadefQ(obj);
        if (childQ) {
            res = output_identifiers(childQ, scb, buff, bufflen,
                                     treeformat, startindent+indent,
                                     indent, helpmode);
            if (res != NO_ERR) {
                return res;
            }
        }
    }

    return NO_ERR;

}   /* output_identifiers */


/********************************************************************
 * FUNCTION output_one_module_identifiers
 * 
 * Generate a list of object identifiers for a module
 * YANG support only!!!
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    scb == session control block to use for output
 *    buff == scratch buffer to use 
 *    bufflen == size of buff in bytes
 *    treeformat == TRUE for tree format; FALSE for list format
 *    indent == indent amount
 *    helpmode == verbosity level
 *********************************************************************/
static void
    output_one_module_identifiers (ncx_module_t *mod,
                                   ses_cb_t *scb,
                                   xmlChar *buff,
                                   uint32 bufflen,
                                   boolean treeformat,
                                   uint32 indent,
                                   help_mode_t helpmode)
{
    status_t  res;

    res = output_identifiers(&mod->datadefQ, 
                             scb, 
                             buff, 
                             bufflen,
                             treeformat,
                             indent,
                             indent,
                             helpmode);
    if (res == NO_ERR) {
        ses_putchar(scb, '\n');
    }

}   /* output_one_module_identifiers */


/********************************************************************
 * FUNCTION output_module_identifiers
 * 
 * Generate a list of object identifiers for a module
 * YANG support only!!!
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block for output
 *    treeformat == TRUE for tree format; FALSE for list format
 *    helpmode == verbosity level
 *********************************************************************/
static void
    output_module_identifiers (yang_pcb_t *pcb,
                               yangdump_cvtparms_t *cp,
                               ses_cb_t *scb,
                               boolean treeformat,
                               help_mode_t helpmode)
{
    ncx_module_t      *mod;
    yang_node_t       *node; 

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }

    ses_putstr(scb, (const xmlChar *)"\nidentifiers:");

    /* check subtree mode */
    if (cp->subtree) {
        print_subtree_banner(cp, mod, scb);
    }

    output_one_module_identifiers(mod, 
                                  scb,
                                  (xmlChar *)cp->buff,
                                  cp->bufflen,
                                  treeformat,
                                  cp->indent,
                                  helpmode);

    if ((cp->unified || cp->onemodule) && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                output_one_module_identifiers(node->submod, 
                                              scb, 
                                              (xmlChar *)cp->buff,
                                              cp->bufflen,
                                              treeformat,
                                              cp->indent,
                                              helpmode);
            }
        }
    }

}   /* output_module_identifiers */


/********************************************************************
 * FUNCTION output_one_module_version
 * 
 * Generate the module or submodule version
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    scb == session control block to use for output
 *    buff == scratch buffer to use 
 *********************************************************************/
static void
    output_one_module_version (ncx_module_t *mod,
                               ses_cb_t *scb,
                               char *buff)
{
    if (mod->ismod) {
        sprintf(buff, "\nmodule %s", mod->name);
    } else {
        sprintf(buff, "\nsubmodule %s", mod->name);
    }
    ses_putstr(scb, (const xmlChar *)buff);

    sprintf(buff, "@%s", (mod->version) ? mod->version : NCX_EL_NONE);
    ses_putstr(scb, (const xmlChar *)buff);

    if (mod->source && LOGDEBUG2) {
        ses_putchar(scb, ' ');
        ses_putstr(scb, mod->source);
    }

}   /* output_one_module_version */


/********************************************************************
 * FUNCTION output_module_version
 * 
 * Generate the module version
 * YANG support only!!!
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block for output
 *********************************************************************/
static void
    output_module_version (yang_pcb_t *pcb,
                           yangdump_cvtparms_t *cp,
                           ses_cb_t *scb)
{
    ncx_module_t      *mod;
    yang_node_t       *node; 

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }

    ses_putstr(scb, (const xmlChar *)"\nmodversion:");
    output_one_module_version(mod, scb, cp->buff);

    if (cp->unified && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                output_one_module_version(node->submod, scb, cp->buff);
            }
        }
    }
    ses_putchar(scb, '\n');

}   /* output_module_version */


/********************************************************************
 * FUNCTION copy_module
 * 
 * Copy the source file to a different location and possibly
 * change the filename
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block for output
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    copy_module (yang_pcb_t *pcb,
                 yangdump_cvtparms_t *cp,
                 ses_cb_t *scb)
{
    ncx_module_t      *mod;
    FILE              *srcfile;
    boolean            done;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        return ERR_NCX_MOD_NOT_FOUND;
    }

    /* open the YANG source file for reading */
    srcfile = fopen((const char *)mod->source, "r");
    if (!srcfile) {
        return ERR_NCX_MISSING_FILE;
    }

    done = FALSE;
    while (!done) {
        if (!fgets((char *)cp->buff, (int)cp->bufflen, srcfile)) {
            /* read line failed, not an error */
            done = TRUE;
            continue;
        }
        ses_putstr(scb, (const xmlChar *)cp->buff);
    }

    fclose(srcfile);
    return NO_ERR;

}   /* copy_module */


/********************************************************************
 * FUNCTION print_score_banner
 * 
 *  Generate the banner for the yangdump results
 *  
 * INPUTS:
 *    pcb == parser control block to use
 *
 *********************************************************************/
static void
    print_score_banner (yang_pcb_t *pcb)
{
    log_write("\n*** %s\n*** %u Errors, %u Warnings\n", 
              pcb->top->source, 
              pcb->top->errors, 
              pcb->top->warnings);

}   /* print_score_banner */


/********************************************************************
 * FUNCTION format_allowed
 * 
 *  Check if the conversion format is allowed for yangdump or
 *  yangdumpcode
 *  
 * INPUTS:
 *    cp == conversion parameters
 *
 * RETURNS:
 *    TRUE if format conversion allowed; FALSE if not
 * 
 *********************************************************************/
static boolean
    format_allowed (const yangdump_cvtparms_t *cp)
{

    if (cp->allowcode) {
        return TRUE;
    }

    switch (cp->format) {
    case NCX_CVTTYP_NONE:
    case NCX_CVTTYP_XSD:
    case NCX_CVTTYP_HTML:
    case NCX_CVTTYP_YANG:
    case NCX_CVTTYP_COPY:
    case NCX_CVTTYP_YIN:
        return TRUE;
    case NCX_CVTTYP_SQL:
    case NCX_CVTTYP_SQLDB:
    case NCX_CVTTYP_H:
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
    case NCX_CVTTYP_YH:
    case NCX_CVTTYP_YC:
    case NCX_CVTTYP_UH:
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_TG2:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return FALSE;

}   /* format_allowed */


/********************************************************************
 * FUNCTION convert_one
 * 
 *  Validate and then perhaps convert one module to the specified format
 *  The global params in cvtparms are used (should change that!)
 *
 * INPUTS:
 *    cp == parameter block to use
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    convert_one (yangdump_cvtparms_t *cp)
{
    ses_cb_t          *scb;
    ncx_module_t      *mainmod;
    val_value_t       *val;
    yang_pcb_t        *pcb;
    xmlChar           *modname, *namebuff, *savestr, savechar;
    const xmlChar     *revision;
    xml_attrs_t        attrs;
    status_t           res;
    boolean            bannerdone;
    uint32             modlen;

    scb = NULL;
    mainmod = NULL;
    revision = NULL;
    savestr = NULL;
    savechar = '\0';
    bannerdone = FALSE;
    modlen = 0;
    modname = (xmlChar *)cp->curmodule;
    res = NO_ERR;

    if (yang_split_filename(modname, &modlen)) {
        savestr = &modname[modlen];
        savechar = *savestr;
        *savestr = '\0';
        revision = savestr + 1;
    }

    /* load in the requested module to convert */
    pcb = ncxmod_load_module_ex(modname,
                                revision,
                                cp->unified, /* with_submods */
                                /* savetkc for yin, yang, html */
                                !cp->rawmode || 
                                cp->format == NCX_CVTTYP_YIN ||
                                cp->format == NCX_CVTTYP_YANG ||
                                cp->format == NCX_CVTTYP_HTML,
                                TRUE,    /* keepmode to force new load */
                                /* docmode for --format=html or yang */
                                cp->format == NCX_CVTTYP_YANG ||
                                cp->format == NCX_CVTTYP_HTML,
                                &cp->savedevQ,
                                &res);

    if (savestr != NULL) {
        *savestr = savechar;
    }

    if (res == ERR_NCX_SKIPPED) {
        if (pcb) {
            yang_free_pcb(pcb);
        }
        return NO_ERR;
    } else if (res != NO_ERR) {
        if (pcb && pcb->top) {
            print_score_banner(pcb);
        } else if (res == ERR_NCX_MOD_NOT_FOUND) {
            log_error("\nError: [sub]module '%s' not found\n",
                      modname);
        } else if (LOGDEBUG2) {
            /* invalid module name and/or revision date */
            if (revision) {
                log_debug2("\n[sub]module '%s' revision '%s' "
                          "not loaded", 
                           modname, 
                           revision);
            } else {
                log_debug2("\n[sub]module '%s' not loaded", 
                           modname);
            }
            log_debug2(" (%s)\n", get_error_string(res));
        }
        if (!pcb || !pcb->top || pcb->top->errors) {
            if (pcb) {
                yang_free_pcb(pcb);
            }
            return res;
        } else {
            /* just warnings reported */
            res = NO_ERR;
        }
    } else if (pcb && pcb->top) {
        if (LOGINFO) {
            /* generate banner everytime yangdump runs */
            print_score_banner(pcb);
            bannerdone = TRUE;            
        } else {
            if (!(cp->format == NCX_CVTTYP_XSD ||
                  cp->format == NCX_CVTTYP_HTML ||
                  cp->format == NCX_CVTTYP_H ||
                  cp->format == NCX_CVTTYP_C ||
                  cp->format == NCX_CVTTYP_CPP_TEST ||
                  cp->format == NCX_CVTTYP_UC ||
                  cp->format == NCX_CVTTYP_UH ||
                  cp->format == NCX_CVTTYP_YC ||
                  cp->format == NCX_CVTTYP_YH ||
                  cp->format == NCX_CVTTYP_TG2)) {
                print_score_banner(pcb);
                bannerdone = TRUE;
            }
        }
    }

    /* check if output session needed, any reports requested or
     * any format except XSD, and NONE means a session will be used
     * to write output.
     */
    if (cp->modversion || 
        cp->exports || 
        cp->dependencies || 
        cp->identifiers || 
        cp->tree_identifiers ||
        cp->collect_stats || 
        !(cp->format == NCX_CVTTYP_NONE || 
          cp->format == NCX_CVTTYP_XSD)) {

        scb = get_output_session(pcb, cp, &res);
        if (!scb || res != NO_ERR) {
            log_error("\nError: open session failed (%s)\n",
                      get_error_string(res));
            yang_free_pcb(pcb);
            if (scb) {
                ses_free_scb(scb);
            }
            return res;
        }

        if (cp->indent != ses_indent_count(scb)) {
            ses_set_indent(scb, cp->indent);
        }
    }

    /* check if modversion and other report modes */
    if (scb) {
        if (cp->modversion) {
            output_module_version(pcb, cp, scb);
        }

        if (cp->exports) {
            output_module_exports(pcb, cp, scb);
        }

        if (cp->dependencies) {
            output_module_dependencies(pcb, cp, scb);
        }

        if (cp->identifiers) {
            output_module_identifiers(pcb, cp, scb, FALSE, cp->helpsubmode);
        }

        if (cp->tree_identifiers) {
            output_module_identifiers(pcb, cp, scb, TRUE, cp->helpsubmode);
        }

        if (cp->collect_stats) {
            yangstats_collect_module_stats(pcb, cp);
            if (cp->stat_totals != YANGDUMP_TOTALS_SUMMARY_ONLY) {
                yangstats_output_module_stats(pcb, cp, scb);
            }
        }
    }

    /* get the namespace info for submodules */
    if (cp->format == NCX_CVTTYP_XSD ||
        cp->format == NCX_CVTTYP_SQLDB ||
        cp->format == NCX_CVTTYP_TG2 ||
        cp->format == NCX_CVTTYP_HTML) {

        if (!pcb->top->ismod) {
            /* need to load the entire module now,
             * in order to get a namespace ID for
             * the main module, which is also used in the submodule
             * !!! DO NOT KNOW THE REVISION OF THE PARENT !!!
             */
            res = ncxmod_load_module(ncx_get_modname(pcb->top), 
                                     NULL, 
                                     &cp->savedevQ,
                                     &mainmod);
            if (res != NO_ERR) {
                log_error("\nError: main module '%s' had errors."
                          "\n       XSD conversion of '%s' terminated.",
                          ncx_get_modname(pcb->top),
                          pcb->top->sourcefn);
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                /* make a copy of the module namespace URI
                 * so the XSD targetNamespace will be generated
                 */
                pcb->top->ns = mainmod->ns;
                pcb->top->nsid = mainmod->nsid;
            }
        }
    }

    /* should be ready to convert the requested [sub]module now */
    if (res == NO_ERR && pcb->top) {
        /* check the type of translation requested */
        switch (cp->format) {
        case NCX_CVTTYP_NONE:
            if (ncx_any_dependency_errors(pcb->top)) {
                if (ncx_warning_enabled(ERR_NCX_DEPENDENCY_ERRORS)) {
                    log_warn("\nWarning: one or more modules "
                             "imported into '%s' "
                             "had errors", 
                             pcb->top->sourcefn);
                }
            }
            if (LOGDEBUG2) {
                log_debug2("\nFile '%s' compiled without errors",
                           pcb->top->sourcefn);
            }
            break;
        case NCX_CVTTYP_XSD:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       XSD conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                val = NULL;
                xml_init_attrs(&attrs);
                ncx_set_useprefix(TRUE);
                res = xsd_convert_module(pcb,
                                         pcb->top, 
                                         cp, 
                                         &val, 
                                         &attrs);
                if (res != NO_ERR) {
                    pr_err(res);
                } else {
                    if (cp->defnames || (cp->output && cp->output_isdir)) {
                        namebuff = xsd_make_output_filename(pcb->top, cp);
                        if (!namebuff) {
                            res = ERR_INTERNAL_MEM;
                        } else {
                            /* output to the expanded file name from
                             * the specified file, or generated
                             * with the default name
                             */
                            res = xml_wr_file(namebuff,
                                              val,
                                              &attrs,
                                              DOCMODE,
                                              WITHHDR,
                                              TRUE,
                                              0,
                                              cp->indent);
                            m__free(namebuff);
                        }
                    } else if (cp->output) {
                        /* output to the specified file */
                        res = xml_wr_file((const xmlChar *)cp->output,
                                          val,
                                          &attrs,
                                          DOCMODE,
                                          WITHHDR,
                                          TRUE,
                                          0,
                                          cp->indent);
                    } else {
                        /* output to the specified file */
                        res = 
                            xml_wr_check_open_file(stdout,
                                                   val,
                                                   &attrs,
                                                   DOCMODE, 
                                                   WITHHDR,
                                                   TRUE,
                                                   0,
                                                   cp->indent,
                                                   NULL);
                    }
                }
                if (res != NO_ERR) {
                    log_error("\nError: cannot write output file '%s'",
                              (cp->output) ? cp->output : "stdout");
                }
                xml_clean_attrs(&attrs);
                if (val) {
                    val_free_value(val);
                }
            }
            break;
        case NCX_CVTTYP_SQL:
            res = ERR_NCX_OPERATION_NOT_SUPPORTED;
            pr_err(res);
            break;
        case NCX_CVTTYP_SQLDB:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       SQL object database conversion of "
                          "'%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = sql_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_COPY:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       Copy source for '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = copy_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_HTML:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       HTML conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                cp->tkc = pcb->tkc;
                res = html_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_H:
        case NCX_CVTTYP_UH:
        case NCX_CVTTYP_YH:
            cp->isuser = (cp->format == NCX_CVTTYP_UH) ? TRUE : FALSE;
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       H file conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = h_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_UC:
        case NCX_CVTTYP_C:
        case NCX_CVTTYP_CPP_TEST:
        case NCX_CVTTYP_YC:
            cp->isuser = (cp->format == NCX_CVTTYP_UC) ? TRUE : FALSE;
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       C file conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = c_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_YANG:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       YANG conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                cp->tkc = pcb->tkc;
                res = cyang_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_YIN:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       YIN conversion of '%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = yangyin_convert_module(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;
        case NCX_CVTTYP_TG2:
            if (ncx_any_dependency_errors(pcb->top)) {
                log_error("\nError: one or more imported modules had errors."
                          "\n       TG2 source code conversion of "
                          "'%s' terminated.",
                          pcb->top->sourcefn);
                res = ERR_NCX_IMPORT_ERRORS;
                ncx_print_errormsg(NULL, pcb->top, res);
            } else {
                res = tg2_convert_module_model(pcb, cp, scb);
                if (res != NO_ERR) {
                    pr_err(res);
                }
            }
            break;

        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            pr_err(res);
        }
    }

    if (res != NO_ERR || LOGDEBUG2) {
        if (pcb && pcb->top && !bannerdone) {
            print_score_banner(pcb);
        } else if (res != NO_ERR) {
            log_write("\n");
        }
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }

    if (scb) {
        ses_free_scb(scb);
    }

    return res;

}  /* convert_one */


/********************************************************************
 * FUNCTION subtree_callback
 * 
 * Handle the current filename in the subtree traversal
 * Parse the module and generate.
 *
 * Follows ncxmod_callback_fn_t template
 *
 * INPUTS:
 *   fullspec == absolute or relative path spec, with filename and ext.
 *               this regular file exists, but has not been checked for
 *               read access of 
 *   cookie == yangcli conversion parms
 *
 * RETURNS:
 *    status
 *
 *    Return fatal error to stop the traversal or NO_ERR to
 *    keep the traversal going.  Do not return any warning or
 *    recoverable error, just log and move on
 *********************************************************************/
static status_t
    subtree_callback (const char *fullspec,
                      void *cookie)
{
    yangdump_cvtparms_t *cp;
    status_t             res;
    
    res = NO_ERR;
    cp = cookie;

    if (cp->module) {
        m__free(cp->module);
    }
    cp->module = (char *)ncx_get_source((const xmlChar *)fullspec, &res);
    if (!cp->module) {
        return res;
    }
    cp->curmodule = cp->module;

    log_debug2("\nStart subtree file:\n%s\n", cp->module);
    res = convert_one(cp);
    if (res != NO_ERR) {
        if (!NEED_EXIT(res)) {
            res = NO_ERR;
        }
    }

    return res;

}  /* subtree_callback */


/************    E X T E R N A L    F U N C T I O N S   ************/


/********************************************************************
 * FUNCTION ydump_init
 * 
 * Parse all CLI parameters and fill in the conversion parameters
 * 
 * INPUTS:
 *   argc == number of argv parameters
 *   argv == array of CLI parameters (may be NULL to skip)
 *   allowcode == TRUE if code generation should be allowed
 *                FALSE otherwise
 *   cvtparms == address of conversion parms to fill in
 *
 * OUTPUTS:
 *  *cvtparms filled in, maybe partial if errors
 *
 * RETURNS:
 *    status
 *********************************************************************/
status_t 
    ydump_init (int argc,
                char *argv[],
                boolean allowcode,
                yangdump_cvtparms_t *cvtparms)
{
    status_t       res;
    xmlns_id_t     nsid;

#ifdef DEBUG
    if (cvtparms == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    memset(cvtparms, 0x0, sizeof(yangdump_cvtparms_t));

    /* initialize the NCX Library first to allow NCX modules
     * to be processed.  No module can get its internal config
     * until the NCX module parser and definition registry is up
     * parm(TRUE) indicates all description clauses should be saved
     * Set debug cutoff filter to user errors
     */
    res = ncx_init(TRUE,
                   LOG_DEBUG_INFO,
                   FALSE,
                   NULL,
                   argc, 
                   argv);

    if (res == NO_ERR) {
        res = init_cvtparms(cvtparms, allowcode);
    }

    if (res == NO_ERR) {
        /* load in the YANGDUMP converter parmset definition file */
        res = ncxmod_load_module(YANGDUMP_MOD, 
                                 NULL, 
                                 NULL,
                                 NULL);
    }

    /* Initialize the Notifications namespace */
    if (res == NO_ERR) {
        res = xmlns_register_ns(NCN_URN, 
                                NCN_PREFIX, 
                                NCX_MODULE, 
                                NULL, 
                                &nsid);
    }

    if (res == NO_ERR) {
        res = process_cli_input(argc, argv, cvtparms);
    }
        
    if (res == NO_ERR && cvtparms->collect_stats) {
        res = init_cvtparms_stats(cvtparms);
    }

    if (res != NO_ERR && res != ERR_NCX_SKIPPED) {
        pr_err(res);
    }
    
    return res;

}  /* ydump_init */


/********************************************************************
* FUNCTION ydump_main
*
* Run the yangdump conversion, according to the specified
* yangdump conversion parameters set
*
* INPUTS:
*   cvtparms == conversion parameters to use
*
* OUTPUTS:
*   the conversion (if any) will be done and output to
*   the specified files, or STDOUT
*
* RETURNS:
*   status
*********************************************************************/
status_t
    ydump_main (yangdump_cvtparms_t *cvtparms)
{
    val_value_t  *val;
    const char   *progname;
    ses_cb_t     *scb;
    status_t      res;
    boolean       done, quickexit;
    xmlChar       buffer[NCX_VERSION_BUFFSIZE];

    done = FALSE;

    progname = "yangdump";

    quickexit = cvtparms->helpmode ||
        cvtparms->versionmode ||
        cvtparms->showerrorsmode;

    if (cvtparms->versionmode || cvtparms->showerrorsmode) {
        res = ncx_get_version(buffer, NCX_VERSION_BUFFSIZE);
        if (res == NO_ERR) {

            log_write("\n%s %s", progname, buffer);
            if (cvtparms->versionmode) {
                log_write("\n");
            }
        } else {
            SET_ERROR(res);
        }
    }

    if (cvtparms->helpmode) {
        help_program_module(YANGDUMP_MOD, 
                            YANGDUMP_CONTAINER, 
                            cvtparms->helpsubmode);
    }
    if (cvtparms->showerrorsmode) {
        log_write(" errors and warnings\n");
        print_error_messages();
    }

    if (quickexit) {
        return NO_ERR;
    }

    /* check if the requested format is allowed */
    if (!format_allowed(cvtparms)) {
        log_error("\nError: The requested conversion format "
                  "is not supported by yangdump.");
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    /* check if subdir search suppression is requested */
    if (!cvtparms->subdirs) {
        ncxmod_set_subdirs(FALSE);
    }

    if (LOGINFO) {
        /* generate banner everytime yangdump runs */
        write_banner();
    } else {
        if (!(cvtparms->format == NCX_CVTTYP_XSD ||
              cvtparms->format == NCX_CVTTYP_HTML ||
              cvtparms->format == NCX_CVTTYP_H ||
              cvtparms->format == NCX_CVTTYP_C ||
              cvtparms->format == NCX_CVTTYP_CPP_TEST)) {
            write_banner();
        }
    }

    /* first check if there are any deviations to load */
    res = NO_ERR;
    val = val_find_child(cvtparms->cli_val, 
                         YANGDUMP_MOD, 
                         YANGDUMP_PARM_DEVIATION);
    while (val) {
        res = ncxmod_load_deviation(VAL_STR(val), 
                                    &cvtparms->savedevQ);
        if (NEED_EXIT(res)) {
            val = NULL;
        } else {
            val = val_find_next_child(cvtparms->cli_val,
                                      YANGDUMP_MOD,
                                      YANGDUMP_PARM_DEVIATION,
                                      val);
        }
    }

    /* save intermediate files to prevent expanded augments, etc.
     * from pointing at deleted modules
     */
    ncx_set_use_deadmodQ();

    /* force the --feature-enable-default=true for generating C code */
    switch (cvtparms->format) {
    case NCX_CVTTYP_H:
    case NCX_CVTTYP_UH:
    case NCX_CVTTYP_YH:
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_YC:
    case NCX_CVTTYP_CPP_TEST:
        ncx_set_feature_enable_default(TRUE);
        break;
    default:
        ;
    }

    /* convert one file or N files or 1 subtree */
    res = NO_ERR;
    val = val_find_child(cvtparms->cli_val, YANGDUMP_MOD, 
                         YANGDUMP_PARM_MODULE);
    while (val) {
        done = TRUE;
        cvtparms->curmodule = (char *)VAL_STR(val);
        cvtparms->onemodule = TRUE;
        res = convert_one(cvtparms);
        if (NEED_EXIT(res)) {
            val = NULL;
        } else {
            val = val_find_next_child(cvtparms->cli_val,
                                      YANGDUMP_MOD,
                                      YANGDUMP_PARM_MODULE,
                                      val);
        }
    }

    cvtparms->onemodule = FALSE;
    if (res == NO_ERR &&
        cvtparms->subtreecount >= 1) {
        if (cvtparms->format == NCX_CVTTYP_XSD ||
            cvtparms->format == NCX_CVTTYP_HTML ||
            cvtparms->format == NCX_CVTTYP_YANG ||
            cvtparms->format == NCX_CVTTYP_COPY) {
            /* force separate file names in subtree mode */
            cvtparms->defnames = TRUE;
        }
                   
        val = val_find_child(cvtparms->cli_val, 
                             YANGDUMP_MOD, 
                             YANGDUMP_PARM_SUBTREE);
        while (val) {
            done = TRUE;

            /* this var cvtparms->subtree will be non-NULL
             * if there are any subtrees to process;
             * this will cause subtree mode to be true, so
             * a banner is printed after every file
             */
            cvtparms->subtree = (const char *)VAL_STR(val);

            if (ncxmod_test_subdir((const xmlChar *)cvtparms->subtree)) {
                res = ncxmod_process_subtree(cvtparms->subtree,
                                             subtree_callback,
                                             cvtparms);
            } else {
                res = ERR_NCX_NOT_FOUND;
                log_error("\nError: directory '%s' not found\n",
                          cvtparms->subtree);
            }
            if (NEED_EXIT(res)) {
                val = NULL;
            } else {
                val = val_find_next_child(cvtparms->cli_val,
                                          YANGDUMP_MOD,
                                          YANGDUMP_PARM_SUBTREE,
                                          val);
            }
        }
    }

    if (res == NO_ERR && !done) {
        res = ERR_NCX_MISSING_PARM;
        log_error("\n%s: Error: missing parameter (%s or %s)\n",
                  progname,
                  NCX_EL_MODULE, 
                  NCX_EL_SUBTREE);
    }

    if (res == NO_ERR && cvtparms->final_stats != NULL) {
        scb = get_output_session(NULL, cvtparms, &res);
        if (scb != NULL) {
            yangstats_output_final_stats(cvtparms, scb);
            ses_free_scb(scb);
        }
    }
        
    return res;

}  /* ydump_main */


/********************************************************************
 * FUNCTION ydump_cleanup
 * 
 * INPUTS:
 *  cvtparms == conversion parameters to clean but not free
 * 
 *********************************************************************/
void
    ydump_cleanup (yangdump_cvtparms_t *cvtparms)
{
#ifdef DEBUG
    if (cvtparms == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    clean_cvtparms(cvtparms);

    /* cleanup the NCX engine and registries */
    ncx_cleanup();

}  /* ydump_cleanup */


/* END ydump.c */



