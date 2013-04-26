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
#ifndef _H_yangdump
#define _H_yangdump

/*  FILE: yangdump.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  Exports yangdump.yang conversion CLI parameter struct
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
01-mar-08    abb      Begun; moved from ncx/ncxtypes.h
16-apr-11    abb      Add support for string token preservation
*/

#include <xmlstring.h>

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/
#define PROGNAME            "yangdump"

#define EMPTY_STRING        (const xmlChar *)""

#define EL_A                (const xmlChar *)"a"
#define EL_BODY             (const xmlChar *)"body"
#define EL_DIV              (const xmlChar *)"div"
#define EL_H1               (const xmlChar *)"h1"
#define EL_PRE              (const xmlChar *)"pre"
#define EL_SPAN             (const xmlChar *)"span"
#define EL_UL               (const xmlChar *)"ul"
#define EL_LI               (const xmlChar *)"li"

#define CL_YANG             (const xmlChar *)"yang"
#define CL_TOCMENU          (const xmlChar *)"tocmenu"
#define CL_TOCPLAIN         (const xmlChar *)"tocplain"
#define CL_DADDY            (const xmlChar *)"daddy"

#define ID_NAV              (const xmlChar *)"nav"

#define OBJVIEW_RAW     "raw"
#define OBJVIEW_COOKED  "cooked"

/* this should match the buffer size in ncx/tk.h */
#define YANGDUMP_BUFFSIZE   0xffff

#define YANGDUMP_DEF_OUTPUT   "stdout"

#define YANGDUMP_DEF_CONFIG   (const xmlChar *)"/etc/yuma/yangdump.conf"

#define YANGDUMP_DEF_TOC      (const xmlChar *)"menu"

#define YANGDUMP_DEF_OBJVIEW  OBJVIEW_RAW

#define YANGDUMP_MOD          (const xmlChar *)"yangdump"
#define YANGDUMP_CONTAINER    (const xmlChar *)"yangdump"

#define YANGDUMP_PARM_CONFIG        (const xmlChar *)"config"
#define YANGDUMP_PARM_DEFNAMES      (const xmlChar *)"defnames"
#define YANGDUMP_PARM_DEPENDENCIES  (const xmlChar *)"dependencies"
#define YANGDUMP_PARM_DEVIATION     (const xmlChar *)"deviation"
#define YANGDUMP_PARM_EXPORTS       (const xmlChar *)"exports"
#define YANGDUMP_PARM_FORMAT        (const xmlChar *)"format"
#define YANGDUMP_PARM_HTML_DIV      (const xmlChar *)"html-div"
#define YANGDUMP_PARM_HTML_TOC      (const xmlChar *)"html-toc"
#define YANGDUMP_PARM_IDENTIFIERS   (const xmlChar *)"identifiers"
#define YANGDUMP_PARM_INDENT        (const xmlChar *)"indent"
#define YANGDUMP_PARM_MODULE        (const xmlChar *)"module"
#define YANGDUMP_PARM_MODVERSION    (const xmlChar *)"modversion"
#define YANGDUMP_PARM_VERSIONNAMES  (const xmlChar *)"versionnames"
#define YANGDUMP_PARM_OUTPUT        (const xmlChar *)"output"
#define YANGDUMP_PARM_OBJVIEW       (const xmlChar *)"objview"
#define YANGDUMP_PARM_XSD_SCHEMALOC (const xmlChar *)"xsd-schemaloc"
#define YANGDUMP_PARM_SIMURLS       (const xmlChar *)"simurls"
#define YANGDUMP_PARM_SUBTREE       (const xmlChar *)"subtree"
#define YANGDUMP_PARM_STATS         (const xmlChar *)"stats"
#define YANGDUMP_PARM_STAT_TOTALS   (const xmlChar *)"totals"
#define YANGDUMP_PARM_TREE_IDENTIFIERS   (const xmlChar *)"tree-identifiers"
#define YANGDUMP_PARM_URLSTART      (const xmlChar *)"urlstart"
#define YANGDUMP_PARM_UNIFIED       (const xmlChar *)"unified"

#define YANGDUMP_PV_STATS_NONE         (const xmlChar *)"none"
#define YANGDUMP_PV_STATS_BRIEF        (const xmlChar *)"brief"
#define YANGDUMP_PV_STATS_BASIC        (const xmlChar *)"basic"
#define YANGDUMP_PV_STATS_ADVANCED     (const xmlChar *)"advanced"
#define YANGDUMP_PV_STATS_ALL          (const xmlChar *)"all"

#define YANGDUMP_PV_TOTALS_NONE        (const xmlChar *)"none"
#define YANGDUMP_PV_TOTALS_SUMMARY     (const xmlChar *)"summary"
#define YANGDUMP_PV_TOTALS_SUMMARY_ONLY  (const xmlChar *)"summary-only"


/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/* --stats CLI parameter */
typedef enum yangdump_statreport_t_ {
    YANGDUMP_REPORT_NONE,  /* significant value */
    YANGDUMP_REPORT_BRIEF,
    YANGDUMP_REPORT_BASIC,
    YANGDUMP_REPORT_ADVANCED,
    YANGDUMP_REPORT_ALL
} yangdump_statreport_t;

/* --totals CLI parameter */
typedef enum yangdump_totals_t_ {
    YANGDUMP_TOTALS_NONE,   /* significant value */
    YANGDUMP_TOTALS_SUMMARY,
    YANGDUMP_TOTALS_SUMMARY_ONLY
} yangdump_totals_t;

/* this structure represents all the statistics that can
 * be reported with the --stats parameter, and can be collected
 * by the yangdump program.
 */
typedef struct yangdump_stats_t_ {
    /* brief */
    uint32     complexity_score;

    /* total nodes derived from num_config_objs
     * + num_state_objs
     */

    /* basic */
    uint32     num_extensions;
    uint32     num_features;
    uint32     num_groupings;
    uint32     num_typedefs;
    uint32     num_deviations;
    uint32     num_top_datanodes;
    uint32     num_config_objs;
    uint32     num_state_objs;

    /* advanced */
    uint32     num_mandatory_nodes;
    uint32     num_optional_nodes;
    uint32     num_notifications;
    uint32     num_rpcs;
    uint32     num_inputs;
    uint32     num_outputs;
    uint32     num_augments;
    uint32     num_uses;
    uint32     num_choices;
    uint32     num_cases;
    uint32     num_anyxmls;
    uint32     num_np_containers;
    uint32     num_p_containers;
    uint32     num_lists;
    uint32     num_leaf_lists;
    uint32     num_key_leafs;
    uint32     num_plain_leafs;

    /* all */
    uint32     num_imports;
    uint32     num_numbers;
    uint32     num_dec64s;
    uint32     num_enums;
    uint32     num_bits;
    uint32     num_booleans;
    uint32     num_emptys;
    uint32     num_strings;
    uint32     num_binarys;
    uint32     num_iis;
    uint32     num_leafrefs;
    uint32     num_idrefs;    
    uint32     num_unions;
} yangdump_stats_t;


/* struct of yangdump conversion parameters */
typedef struct yangdump_cvtparms_t_ {
    /* external parameters */
    char           *module;   /* malloced due to subtree design */
    char           *curmodule;  /* not malloced when 2+ modules */
    const char     *output;
    const char     *subtree;
    const char     *objview;
    const xmlChar  *schemaloc;
    const xmlChar  *urlstart;
    const xmlChar  *modpath;
    const xmlChar  *config;
    const xmlChar  *html_toc;
    const xmlChar  *css_file;
    int32           indent;
    uint32          modcount;
    uint32          subtreecount;
    ncx_cvttyp_t    format;
    boolean         helpmode;
    help_mode_t     helpsubmode;
    boolean         defnames;
    boolean         dependencies;
    boolean         exports;
    boolean         collect_stats;
    yangdump_statreport_t stats;
    yangdump_totals_t stat_totals;
    boolean         showerrorsmode;
    boolean         identifiers;
    boolean         tree_identifiers;
    boolean         html_div;
    boolean         modversion;
    boolean         subdirs;
    boolean         versionnames;
    boolean         output_isdir;
    boolean         simurls;
    boolean         unified;
    boolean         versionmode;
    boolean         rawmode;
    boolean         allowcode;
    boolean         onemodule;   /* processing MODULE, force unified */
    /* internal vars */
    xmlChar        *full_output;
    ncx_module_t   *mod;
    char           *srcfile;
    char           *buff;
    val_value_t    *cli_val;
    yangdump_stats_t *cur_stats;
    yangdump_stats_t *final_stats;
    uint32          stat_reports;
    uint32          bufflen;
    boolean         firstdone;
    boolean         isuser;
    dlq_hdr_t       savedevQ;
    tk_chain_t     *tkc;   /* if docmode for format=html|yang */
} yangdump_cvtparms_t;

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_yangdump */
