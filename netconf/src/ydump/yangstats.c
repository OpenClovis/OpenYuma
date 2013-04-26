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
/*  FILE: yangstats.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
30-may-10    abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_ncxtypes
#include  "ncxtypes.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_yang
#include  "yang.h"
#endif

#ifndef _H_yangconst
#include  "yangconst.h"
#endif

#ifndef _H_yangdump
#include  "yangdump.h"
#endif

#ifndef _H_yangdump_util
#include  "yangdump_util.h"
#endif

#ifndef _H_yangstats
#include  "yangstats.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
/* #define YANGSTATS_DEBUG   1 */

#define YS_WEIGHT_EXTENSION     10
#define YS_WEIGHT_FEATURE       5
#define YS_WEIGHT_GROUPING      3
#define YS_WEIGHT_TYPEDEF       1
#define YS_WEIGHT_DEVIATION     10
#define YS_WEIGHT_IMPORT        5
#define YS_WEIGHT_NOTIFICATION  10
#define YS_WEIGHT_RPC           10
#define YS_WEIGHT_RPCIO         3
#define YS_WEIGHT_ANYXML        30
#define YS_WEIGHT_NP_CONTAINER  10
#define YS_WEIGHT_P_CONTAINER   10
#define YS_WEIGHT_CONFIG        5
#define YS_WEIGHT_STATE         2
#define YS_WEIGHT_AUGMENT       15
#define YS_WEIGHT_USES          10
#define YS_WEIGHT_CHOICE        10
#define YS_WEIGHT_CASE          5
#define YS_WEIGHT_LIST          15
#define YS_WEIGHT_LEAF_LIST     8
#define YS_WEIGHT_LEAF          5
#define YS_WEIGHT_BITS          5
#define YS_WEIGHT_ENUMERATION   3
#define YS_WEIGHT_EMPTY         1
#define YS_WEIGHT_BOOLEAN       1
#define YS_WEIGHT_DECIMAL64     6
#define YS_WEIGHT_BINARY        15
#define YS_WEIGHT_II            10
#define YS_WEIGHT_LEAFREF       12
#define YS_WEIGHT_IDREF         8
#define YS_WEIGHT_UNION         10
#define YS_WEIGHT_NUMBER        2
#define YS_WEIGHT_STRING        4
#define YS_WEIGHT_SUBMODULE     20


/********************************************************************
 * FUNCTION output_stat
 * 
 * Generate a statistics report for 1 module or submodule
 *
 * INPUTS:
 *    cp == conversion parameters to use
 *    scb == session control block to use for output
 *    labelstr == stat label string to use
 *    statval == number to output
 *********************************************************************/
static void
    output_stat (yangdump_cvtparms_t *cp,
                 ses_cb_t *scb,
                 const char *labelstr,
                 uint32 statval)
{
    ses_putchar(scb, '\n');
    ses_putstr(scb, (const xmlChar *)labelstr);
    ses_putchar(scb, ':');
    ses_putchar(scb, ' ');
    sprintf(cp->buff, "%u", statval);
    ses_putstr(scb, (const xmlChar *)cp->buff);

}  /* output_stat */


/********************************************************************
 * FUNCTION output_stats
 * 
 * Generate a statistics report for 1 module or submodule
 *
 * INPUTS:
 *    scb == session control block to use for output
 *    stats == stats block to use
 *    cp == conversion parameters to use
 *********************************************************************/
static void
    output_stats (ses_cb_t *scb,
                  yangdump_stats_t *stats,
                  yangdump_cvtparms_t *cp)
{
    /* --stats=brief */
    output_stat(cp,
                scb, 
                "Complexity score", 
                stats->complexity_score);
                
    output_stat(cp,
                scb,
                "Total Nodes", 
                stats->num_config_objs +
                stats->num_state_objs);

    if (cp->stats == YANGDUMP_REPORT_BRIEF) {
        ses_putchar(scb, '\n');
        return;
    }

    /* --stats=basic */
    output_stat(cp,
                scb, 
                "Extensions", 
                stats->num_extensions);

    output_stat(cp,
                scb,
                "Features", 
                stats->num_features);

    output_stat(cp,
                scb, 
                "Groupings", 
                stats->num_groupings);

    output_stat(cp,
                scb, 
                "Typedefs", 
                stats->num_typedefs);

    output_stat(cp,
                scb,
                "Deviations", 
                stats->num_deviations);

    output_stat(cp,
                scb,
                "Top Data Nodes", 
                stats->num_top_datanodes);

    output_stat(cp,
                scb,
                "Config nodes", 
                stats->num_config_objs);

    output_stat(cp,
                scb,
                "Non-config nodes", 
                stats->num_state_objs);

    if (cp->stats == YANGDUMP_REPORT_BASIC) {
        ses_putchar(scb, '\n');
        return;
    }

    /* --stats=advanced */
    output_stat(cp,
                scb,
                "Mandatory nodes", 
                stats->num_mandatory_nodes);

    output_stat(cp,
                scb,
                "Optional nodes", 
                stats->num_optional_nodes);

    output_stat(cp,
                scb,
                "Notifications", 
                stats->num_notifications);

    output_stat(cp,
                scb,
                "Rpcs", 
                stats->num_rpcs);

    output_stat(cp,
                scb,
                "Rpc inputs", 
                stats->num_inputs);

    output_stat(cp,
                scb,
                "Rpc outputs", 
                stats->num_outputs);

    output_stat(cp,
                scb,
                "Augments",
                stats->num_augments);

    output_stat(cp,
                scb,
                "Uses",
                stats->num_uses);

    output_stat(cp,
                scb,
                "Choices",
                stats->num_choices);

    output_stat(cp, 
                scb,
                "Cases",
                stats->num_cases);

    output_stat(cp,
                scb,
                "Anyxmls",
                stats->num_anyxmls);

    output_stat(cp,
                scb,
                "NP Containers", 
                stats->num_np_containers);

    output_stat(cp,
                scb,
                "P Containers", 
                stats->num_p_containers);

    output_stat(cp,
                scb,
                "Lists",
                stats->num_lists);

    output_stat(cp,
                scb,
                "Leaf-lists", 
                stats->num_leaf_lists);

    output_stat(cp,
                scb,
                "Key leafs", 
                stats->num_key_leafs);

    output_stat(cp,
                scb,
                "Plain leafs", 
                stats->num_plain_leafs);

    if (cp->stats == YANGDUMP_REPORT_ADVANCED) {
        ses_putchar(scb, '\n');
        return;
    }

    /* --stats=all */
    output_stat(cp,
                scb,
                "Imports",
                stats->num_imports);

    output_stat(cp,
                scb,
                "Integral numbers",
                stats->num_numbers);

    output_stat(cp,
                scb,
                "Decimal64s",
                stats->num_dec64s);

    output_stat(cp,
                scb,
                "Enumerations",
                stats->num_enums);

    output_stat(cp,
                scb,
                "Bits",
                stats->num_bits);

    output_stat(cp,
                scb,
                "Booleans",
                stats->num_booleans);

    output_stat(cp,
                scb,
                "Emptys",
                stats->num_emptys);

    output_stat(cp,
                scb,
                "Strings",
                stats->num_strings);

    output_stat(cp,
                scb,
                "Binarys",
                stats->num_binarys);

    output_stat(cp,
                scb,
                "Instance Identifiers",
                stats->num_iis);

    output_stat(cp,
                scb,
                "Leafrefs",
                stats->num_leafrefs);

    output_stat(cp,
                scb,
                "Identityrefs",
                stats->num_idrefs);

    output_stat(cp,
                scb,
                "Unions",
                stats->num_unions);

    ses_putchar(scb, '\n');

}  /* output_stats */


/********************************************************************
 * FUNCTION collect_type_stats
 * 
 * Collect the statistics for 1 YANG leaf or leaf-list data type
 *
 * INPUTS:
 *    obj == object template to check
 *    stats == statistics block to use
 *********************************************************************/
static void
    collect_type_stats (obj_template_t *obj,
                        yangdump_stats_t *stats)
{
    ncx_btype_t   btyp;

    btyp = obj_get_basetype(obj);
    switch (btyp) {
    case NCX_BT_BITS:
        stats->num_bits++;
        stats->complexity_score += YS_WEIGHT_BITS;
        break;
    case NCX_BT_ENUM:
        stats->num_enums++;
        stats->complexity_score += YS_WEIGHT_ENUMERATION;
        break;
    case NCX_BT_EMPTY:
        stats->num_emptys++;
        stats->complexity_score += YS_WEIGHT_EMPTY;
        break;
    case NCX_BT_BOOLEAN:
        stats->num_booleans++;
        stats->complexity_score += YS_WEIGHT_BOOLEAN;
        break;
    case NCX_BT_DECIMAL64:
        stats->num_dec64s++;
        stats->complexity_score += YS_WEIGHT_DECIMAL64;
        break;
    case NCX_BT_BINARY:
        stats->num_binarys++;
        stats->complexity_score += YS_WEIGHT_BINARY;
        break;
    case NCX_BT_INSTANCE_ID:
        stats->num_iis++;
        stats->complexity_score += YS_WEIGHT_II;
        break;
    case NCX_BT_LEAFREF:
        stats->num_leafrefs++;
        stats->complexity_score += YS_WEIGHT_LEAFREF;
        break;
    case NCX_BT_IDREF:
        stats->num_idrefs++;
        stats->complexity_score += YS_WEIGHT_IDREF;
        break;
    case NCX_BT_UNION:
        stats->num_unions++;
        stats->complexity_score += YS_WEIGHT_UNION;
        break;
    default:
        if (typ_is_number(btyp)) {
            stats->num_numbers++;
            stats->complexity_score += YS_WEIGHT_NUMBER;
        } else if (typ_is_string(btyp)) {
            stats->num_strings++;
            stats->complexity_score += YS_WEIGHT_STRING;
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

}  /* collect_type_stats */


/********************************************************************
 * FUNCTION collect_object_stats
 * 
 * Collect the statistics for 1 YANG object
 *
 * INPUTS:
 *    cp == conversion parameters to use
 *    obj == object template to check
 *    stats == statistics block to use
 *********************************************************************/
static void
    collect_object_stats (yangdump_cvtparms_t *cp,
                          obj_template_t *obj,
                          yangdump_stats_t *stats)
{
    obj_template_t    *chobj;
    dlq_hdr_t         *que;
    uint32             level;

    level = obj_get_level(obj);
    stats->complexity_score += level;
    if (obj_is_data_db(obj)) {
        if (level == 1) {
            stats->num_top_datanodes++;
        }
    }

    if (obj_is_mandatory(obj)) {
        stats->num_mandatory_nodes++;
    } else {
        stats->num_optional_nodes++;
    }
    if (obj_get_config_flag(obj)) {
        stats->num_config_objs++;
        stats->complexity_score += YS_WEIGHT_CONFIG;
    } else {
        stats->num_state_objs++;
        stats->complexity_score += YS_WEIGHT_STATE;
    }

    switch (obj->objtype) {
    case OBJ_TYP_ANYXML:
        stats->num_anyxmls++;
        stats->complexity_score += YS_WEIGHT_ANYXML;
        return;
    case OBJ_TYP_CONTAINER:
        if (obj_get_presence_string(obj) != NULL) {
            stats->num_p_containers++;
            stats->complexity_score += YS_WEIGHT_P_CONTAINER;
        } else {
            stats->num_np_containers++;
            stats->complexity_score += YS_WEIGHT_NP_CONTAINER;
        }
        break;
    case OBJ_TYP_LEAF:
        if (obj_is_key(obj)) {
            stats->num_key_leafs++;
        } else {
            stats->num_plain_leafs++;
        }
        stats->complexity_score += YS_WEIGHT_LEAF;
        collect_type_stats(obj, stats);
        return;
    case OBJ_TYP_LEAF_LIST:
        stats->num_leaf_lists++;
        stats->complexity_score += YS_WEIGHT_LEAF_LIST;
        collect_type_stats(obj, stats);
        return;
    case OBJ_TYP_LIST:
        stats->num_lists++;
        stats->complexity_score += YS_WEIGHT_LIST;
        break;
    case OBJ_TYP_CHOICE:
        stats->num_choices++;
        stats->complexity_score += YS_WEIGHT_CHOICE;
        break;
    case OBJ_TYP_CASE:
        stats->num_cases++;
        stats->complexity_score += YS_WEIGHT_CASE;
        break;
    case OBJ_TYP_USES:
        stats->num_uses++;
        stats->complexity_score += YS_WEIGHT_USES;
        return;
    case OBJ_TYP_REFINE:
        /* should not happen */
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    case OBJ_TYP_AUGMENT:
        stats->num_augments++;
        stats->complexity_score += YS_WEIGHT_AUGMENT;
        return;
    case OBJ_TYP_RPC:
        stats->num_rpcs++;
        stats->complexity_score += YS_WEIGHT_RPC;
        break;
    case OBJ_TYP_RPCIO:
        if (!xml_strcmp(obj_get_name(obj), YANG_K_INPUT)) {
            stats->num_inputs++;
        } else {
            stats->num_outputs++;
        } 
        stats->complexity_score += YS_WEIGHT_RPCIO;
        break;
    case OBJ_TYP_NOTIF:
        stats->num_notifications++;
        stats->complexity_score += YS_WEIGHT_NOTIFICATION;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    que = obj_get_datadefQ(obj);
    if (que != NULL) {
        for (chobj = (obj_template_t *)dlq_firstEntry(que);
             chobj != NULL;
             chobj = (obj_template_t *)dlq_nextEntry(chobj)) {
            collect_object_stats(cp, chobj, stats);
        }
    }

}  /* collect_object_stats */


/********************************************************************
 * FUNCTION collect_final_stats
 * 
 * Add in the module stats to the final stats
 *
 * INPUTS:
 *    modstats == module stats struct to use
 *    finstats === final stats struct to use
 *********************************************************************/
static void
    collect_final_stats (yangdump_stats_t *modstats,
                         yangdump_stats_t *finstats)
{
    finstats->complexity_score += modstats->complexity_score;
    finstats->num_extensions += modstats->num_extensions;
    finstats->num_features += modstats->num_features;
    finstats->num_groupings += modstats->num_groupings;
    finstats->num_typedefs += modstats->num_typedefs;
    finstats->num_deviations += modstats->num_deviations;
    finstats->num_top_datanodes += modstats->num_top_datanodes;
    finstats->num_config_objs += modstats->num_config_objs;
    finstats->num_state_objs += modstats->num_state_objs;

    finstats->num_mandatory_nodes += modstats->num_mandatory_nodes;
    finstats->num_optional_nodes += modstats->num_optional_nodes;
    finstats->num_notifications += modstats->num_notifications;
    finstats->num_rpcs += modstats->num_rpcs;
    finstats->num_inputs += modstats->num_inputs;
    finstats->num_outputs += modstats->num_outputs;
    finstats->num_augments += modstats->num_augments;
    finstats->num_uses += modstats->num_uses;
    finstats->num_choices += modstats->num_choices;
    finstats->num_cases += modstats->num_cases;
    finstats->num_anyxmls += modstats->num_anyxmls;
    finstats->num_np_containers += modstats->num_np_containers;
    finstats->num_p_containers += modstats->num_p_containers;
    finstats->num_lists += modstats->num_lists;
    finstats->num_leaf_lists += modstats->num_leaf_lists;
    finstats->num_key_leafs += modstats->num_key_leafs;
    finstats->num_plain_leafs += modstats->num_plain_leafs;

    finstats->num_imports += modstats->num_imports;

    finstats->num_numbers += modstats->num_numbers;
    finstats->num_dec64s += modstats->num_dec64s;
    finstats->num_enums += modstats->num_enums;
    finstats->num_bits += modstats->num_bits;
    finstats->num_booleans += modstats->num_booleans;
    finstats->num_emptys += modstats->num_emptys;
    finstats->num_strings += modstats->num_strings;
    finstats->num_binarys += modstats->num_binarys;
    finstats->num_iis += modstats->num_iis;
    finstats->num_leafrefs += modstats->num_leafrefs;
    finstats->num_idrefs += modstats->num_idrefs;
    finstats->num_unions += modstats->num_unions;

#ifdef YANGSTATS_DEBUG
    /* check if the totals add up */
    {
        uint32 total = 0;
        uint32 items = 0;        

        total = modstats->num_config_objs +
            modstats->num_state_objs;

        if ((modstats->num_mandatory_nodes + 
             modstats->num_optional_nodes) != total) {
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        items += modstats->num_notifications;        
        items += modstats->num_rpcs;
        items += modstats->num_inputs;
        items += modstats->num_outputs;
        items += modstats->num_augments;
        items += modstats->num_uses;
        items += modstats->num_choices;
        items += modstats->num_cases;
        items += modstats->num_anyxmls;
        items += modstats->num_np_containers;
        items += modstats->num_p_containers;
        items += modstats->num_lists;
        items += modstats->num_leaf_lists;
        items += modstats->num_key_leafs;
        items += modstats->num_plain_leafs;

        if (items != total) {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
#endif

}  /* collect_final_stats */


/********************************************************************
 * FUNCTION collect_module_stats
 * 
 * Collect the statistics for 1 module
 *
 * INPUTS:
 *    mod == module struct to check
 *    cp == conversion parameters to use
 *********************************************************************/
static void
    collect_module_stats (ncx_module_t *mod,
                          yangdump_cvtparms_t *cp)
{
    yangdump_stats_t  *stats;
    obj_template_t    *obj;
    uint32             cval;

    stats = cp->cur_stats;

    /* get all the module global counters */
    cval = dlq_count(&mod->extensionQ);
    stats->num_extensions += cval;
    stats->complexity_score += (cval * YS_WEIGHT_EXTENSION);

    cval = dlq_count(&mod->featureQ);
    stats->num_features += cval;
    stats->complexity_score += (cval * YS_WEIGHT_FEATURE);

    cval = dlq_count(&mod->groupingQ);
    stats->num_groupings += cval;
    stats->complexity_score += (cval * YS_WEIGHT_GROUPING);

    cval = dlq_count(&mod->typeQ);
    stats->num_typedefs += cval;
    stats->complexity_score += (cval * YS_WEIGHT_TYPEDEF);

    cval = dlq_count(&mod->deviationQ);
    stats->num_deviations += cval;
    stats->complexity_score += (cval * YS_WEIGHT_DEVIATION);

    cval = dlq_count(&mod->importQ);
    stats->num_imports += cval;
    stats->complexity_score += (cval * YS_WEIGHT_IMPORT);

    /* get the requested object statistics */
    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {
        collect_object_stats(cp, obj, stats);
    }

    /* update the running total if needed */
    if (cp->final_stats != NULL) {
        collect_final_stats(stats, cp->final_stats);
    }

}  /* collect_module_stats */


/*************    E X T E R N A L    F U N C T I O N S   ************/


/********************************************************************
 * FUNCTION yangstats_collect_module_stats
 * 
 * Generate a statistics report for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *********************************************************************/
void
    yangstats_collect_module_stats (yang_pcb_t *pcb,
                                    yangdump_cvtparms_t *cp)
{
    ncx_module_t      *mod;
    yang_node_t       *node;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }

    collect_module_stats(mod, cp);

    if (cp->unified && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                cp->cur_stats->complexity_score += YS_WEIGHT_SUBMODULE;
                collect_module_stats(node->submod, cp);
            }
        }
    }
    
}  /* yangstats_collect_module_stats */


/********************************************************************
 * FUNCTION yangstats_output_module_stats
 * 
 * Generate a statistics report for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block to use for output
 *********************************************************************/
void
    yangstats_output_module_stats (yang_pcb_t *pcb,
                                   yangdump_cvtparms_t *cp,
                                   ses_cb_t *scb)
{
    ncx_module_t      *mod;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
        return;
    }

    ses_putstr(scb, (const xmlChar *)"\nStatistics:");

    /* check subtree mode */
    if (cp->subtree) {
        print_subtree_banner(cp, mod, scb);
    }

    output_stats(scb, cp->cur_stats, cp);
    memset(cp->cur_stats, 0x0, sizeof(yangdump_stats_t));
    cp->stat_reports++;

}  /* yangstats_output_module_stats */


/********************************************************************
 * FUNCTION yangstats_output_final_stats
 * 
 * Generate the final stats report as requested
 * by the --stats and --stat-totals CLI parameters
 *
 * INPUTS:
 *    cp == conversion parameters to use
 *    scb == session control block to use for output
 *********************************************************************/
void
    yangstats_output_final_stats (yangdump_cvtparms_t *cp,
                                  ses_cb_t *scb)
{

    switch (cp->stat_totals) {
    case YANGDUMP_TOTALS_NONE:
        /* skip this step */
        break;
    case YANGDUMP_TOTALS_SUMMARY:
        /* do a summary only if more than 1 module reported */
        if (cp->stat_reports < 2) {
            break;
        }
        /* else fall through */
    case YANGDUMP_TOTALS_SUMMARY_ONLY:
        if (cp->final_stats != NULL) {
            ses_putstr(scb, (const xmlChar *)"\n\nStatistics totals:");
            output_stats(scb, cp->final_stats, cp);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* yangstats_output_final_stats */


/* END yangstats.c */



