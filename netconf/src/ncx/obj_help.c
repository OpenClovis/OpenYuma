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
/*  FILE: obj_help.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
17aug08      abb      begun; split out from obj.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

/*
#ifndef _H_ncx
#include "ncx.h"
#endif
*/

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_obj_help
#include "obj_help.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/



/********************************************************************
* FUNCTION dump_typdef_data
*
* Dump some contents from a  typ_def_t struct for help text
*
* INPUTS:
*   obj == obj_template_t to dump help for
*   mode == requested help mode
*   indent == start indent count
*********************************************************************/
static void
    dump_typdef_data (obj_template_t *obj,
                      help_mode_t mode,
                      uint32 indent)
{
    const xmlChar    *datastr;
    typ_def_t  *typdef, *basetypdef, *testdef;
    typ_enum_t *tenum;
    typ_pattern_t *pattern;
    xmlChar           numbuff[NCX_MAX_NUMLEN];
    ncx_btype_t       btyp;

    if (mode == HELP_MODE_BRIEF) {
        return;
    }

    typdef = obj_get_typdef(obj);
    if (!typdef) {
        return;
    }
    basetypdef = typ_get_base_typdef(typdef);

    btyp = typ_get_basetype(typdef);

    datastr = typ_get_rangestr(typdef);
    if (datastr) {
        if (typ_is_string(btyp)) {
            help_write_lines((const xmlChar *)"length: ", indent, TRUE);
        } else {
            help_write_lines((const xmlChar *)"range: ", indent, TRUE);
        }
        help_write_lines(datastr, 0, FALSE);
    }

    testdef = typdef;
    while (testdef) {
        for (pattern = typ_get_first_pattern(testdef);
             pattern != NULL;
             pattern = typ_get_next_pattern(pattern)) {

            help_write_lines((const xmlChar *)"pattern: ", indent, TRUE);
            help_write_lines(pattern->pat_str, 0, FALSE);
        }
        testdef = typ_get_parent_typdef(testdef);
    }

    switch (btyp) {
    case NCX_BT_ENUM:
    case NCX_BT_BITS:
        if (btyp == NCX_BT_ENUM) {
            help_write_lines((const xmlChar *)"enum values:",
                             indent, 
                             TRUE);
        } else {
            help_write_lines((const xmlChar *)"bit values:",
                             indent, 
                             TRUE);
        }
        
        for (tenum = typ_first_enumdef(basetypdef);
             tenum != NULL;
             tenum = typ_next_enumdef(tenum)) {
            if (mode == HELP_MODE_NORMAL) {
                help_write_lines((const xmlChar *)" ", 0, FALSE);
                help_write_lines(tenum->name, 0, FALSE);
            } else {
                help_write_lines(tenum->name,
                                 indent+NCX_DEF_INDENT, 
                                 TRUE);
                help_write_lines((const xmlChar *)"(", 0, FALSE);

                if (btyp == NCX_BT_ENUM) {
                    snprintf((char *)numbuff, sizeof(numbuff), "%d",
                             tenum->val);
                } else {
                    snprintf((char *)numbuff, sizeof(numbuff), "%u",
                             tenum->pos);
                }

                help_write_lines(numbuff, 0, FALSE);
                help_write_lines((const xmlChar *)")", 0, FALSE);
                if (tenum->descr) {
                    help_write_lines(tenum->descr, 
                                     indent+(2*NCX_DEF_INDENT), 
                                     TRUE);
                }
                if (tenum->ref) {
                    help_write_lines(tenum->ref, 
                                     indent+(2*NCX_DEF_INDENT), 
                                     TRUE);
                }
            }
        }
        break;
    case NCX_BT_LEAFREF:
        datastr = typ_get_leafref_path(basetypdef);
        if (datastr) {
            help_write_lines((const xmlChar *)"leafref path:",
                             indent, 
                             TRUE);
            help_write_lines(datastr, indent+NCX_DEF_INDENT, TRUE);
        }
        break;
    default:
        ;
    }

    
}  /* dump_typdef_data */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION obj_dump_template
*
* Dump the contents of an obj_template_t struct for help text
*
* INPUTS:
*   obj == obj_template to dump help for
*   mode == requested help mode
*   nestlevel == number of levels from the top-level
*                that should be printed; 0 == all levels
*   indent == start indent count
*********************************************************************/
void
    obj_dump_template (obj_template_t *obj,
                       help_mode_t mode,
                       uint32 nestlevel,
                       uint32 indent)
{
    const xmlChar        *val, *keystr;
    obj_template_t       *testobj;
    uint32                count, objnestlevel;
    char                  numbuff[NCX_MAX_NUMLEN];
    boolean               normalpass, neednewline;

#ifdef DEBUG
    if (mode > HELP_MODE_FULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    if (obj == NULL) {
        /* could be called because disabled first child found */
        return;
    }

    if (!obj_has_name(obj) || !obj_is_enabled(obj)) {
        return;
    }

    if (mode == HELP_MODE_NONE) {
        return;
    }

    objnestlevel = obj_get_level(obj);

    if (mode == HELP_MODE_BRIEF && 
        nestlevel > 0 &&
        objnestlevel > nestlevel) {
        return;
    }

    normalpass = (nestlevel && (objnestlevel > nestlevel))
        ? FALSE : TRUE;

    if (obj->objtype == OBJ_TYP_RPCIO || 
        obj->objtype == OBJ_TYP_RPC) {
        help_write_lines(obj_get_name(obj), indent, TRUE);
        count = 0;
    } else if (obj->objtype == OBJ_TYP_CASE) {
        count = obj_enabled_child_count(obj);
    } else {
        count = 2;
    }
    if (count > 1) {
        help_write_lines(obj_get_typestr(obj), indent, TRUE);
        help_write_lines((const xmlChar *)" ", 0, FALSE);
        help_write_lines(obj_get_name(obj), 0, FALSE);
    }

    neednewline = FALSE;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPC:
    case OBJ_TYP_RPCIO:
    case OBJ_TYP_NOTIF:
        break;
    default:
        /* print type and default for leafy and choice objects */
        if (obj->objtype != OBJ_TYP_CHOICE) {
            help_write_lines((const xmlChar *)" [", 0, FALSE); 
            help_write_lines((const xmlChar *)
                             obj_get_type_name(obj),
                             0, 
                             FALSE);
            help_write_lines((const xmlChar *)"]", 0, FALSE); 
        }

        if (mode != HELP_MODE_BRIEF) {
            val = obj_get_default(obj);
            if (val != NULL) {
                help_write_lines((const xmlChar *)" [d:", 0, FALSE); 
                help_write_lines(val, 0, FALSE);
                help_write_lines((const xmlChar *)"]", 0, FALSE); 
            }
            if (obj_is_mandatory(obj)) {
                help_write_lines((const xmlChar *)" <Mandatory>", 
                                 0, 
                                 FALSE);
            }
        }
    }

    if (normalpass) {
        val = obj_get_description(obj);
        if (val == NULL) {
            val = obj_get_alt_description(obj);
        }
        if (val != NULL) {
            switch (mode) {
            case HELP_MODE_BRIEF:
                if (obj->objtype == OBJ_TYP_RPC || 
                    obj->objtype == OBJ_TYP_NOTIF) {
                    help_write_lines_max(val, 
                                         indent+NCX_DEF_INDENT,
                                         TRUE, 
                                         HELP_MODE_BRIEF_MAX); 
                }
                break;
            case HELP_MODE_NORMAL:
                help_write_lines_max(val, 
                                     indent+NCX_DEF_INDENT,
                                     TRUE, 
                                     HELP_MODE_NORMAL_MAX); 
                break;
            case HELP_MODE_FULL:
                help_write_lines(val, indent+NCX_DEF_INDENT, TRUE); 
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
                return;
            }
        }
    }

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        switch (mode) {
        case HELP_MODE_BRIEF:
            neednewline = TRUE;
            break;
        case HELP_MODE_NORMAL:
            if (obj->def.container->presence) {
                help_write_lines((const xmlChar *)" (P)", 
                                 0, 
                                 FALSE); 
            } else {
                help_write_lines((const xmlChar *)" (NP)", 
                                 0, 
                                 FALSE); 
            }
            testobj = obj_get_default_parm(obj);
            if (testobj) {
                help_write_lines((const xmlChar *)"default parameter: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(obj_get_name(testobj), 0, FALSE);
            }
            break;
        case HELP_MODE_FULL:
            if (obj->def.container->presence) {
                help_write_lines((const xmlChar *)"presence: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(obj->def.container->presence, 0, FALSE);
            }
            testobj = obj_get_default_parm(obj);
            if (testobj) {
                help_write_lines((const xmlChar *)"default parameter: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(obj_get_name(testobj), 0, FALSE);
            }
            if (mode == HELP_MODE_FULL) {
                /*** add mustQ ***/;
            }
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }
        obj_dump_datadefQ(obj->def.container->datadefQ, 
                          mode, 
                          nestlevel, 
                          indent+NCX_DEF_INDENT);
        break;
    case OBJ_TYP_ANYXML:
        /* nothing interesting in the typdef to report */
        neednewline = TRUE;
        break;
    case OBJ_TYP_LEAF:
        switch (mode) {
        case HELP_MODE_BRIEF:
            break;
        case HELP_MODE_NORMAL:
            if (normalpass) {
                dump_typdef_data(obj, 
                                 mode, 
                                 indent+NCX_DEF_INDENT);
            }
            break;
        case HELP_MODE_FULL:
            dump_typdef_data(obj,
                             mode, 
                             indent+NCX_DEF_INDENT);
            val = obj_get_units(obj);
            if (val) {
                help_write_lines((const xmlChar *)"units: ", 
                                 indent+NCX_DEF_INDENT,
                                 TRUE); 
                help_write_lines(val, 0, FALSE);
            }
            break;
        default:
            ;
        }
        break;
    case OBJ_TYP_LEAF_LIST:
        switch (mode) {
        case HELP_MODE_NORMAL:
            if (normalpass) {
                dump_typdef_data(obj,
                                 mode, 
                                 indent+NCX_DEF_INDENT);
            }
            break;
        case HELP_MODE_FULL:
            dump_typdef_data(obj,
                             mode, 
                             indent+NCX_DEF_INDENT);
            val = obj_get_units(obj);
            if (val) {
                help_write_lines((const xmlChar *)"units: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(val, 0, FALSE);
            }
            if (!obj->def.leaflist->ordersys) {
                help_write_lines((const xmlChar *)"ordered-by: user", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
            } else {
                help_write_lines((const xmlChar *)"ordered-by: system", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
            }
            if (obj->def.leaflist->minset) {
                help_write_lines((const xmlChar *)"min-elements: ", 
                                 indent+NCX_DEF_INDENT,
                                 TRUE); 
                snprintf(numbuff, sizeof(numbuff), "%u",
                         obj->def.leaflist->minelems);
                help_write_lines((const xmlChar *)numbuff, 0, FALSE);
            }
            if (obj->def.leaflist->maxset) {
                help_write_lines((const xmlChar *)"max-elements: ", 
                                 indent+NCX_DEF_INDENT,
                                 TRUE); 
                snprintf(numbuff, sizeof(numbuff), "%u",
                         obj->def.leaflist->maxelems);
                help_write_lines((const xmlChar *)numbuff, 0, FALSE);
            }
            break;
        default:
            ;
        }
        break;
    case OBJ_TYP_CHOICE:
        if (mode == HELP_MODE_BRIEF) {
            neednewline = TRUE;
            break;
        }
        count = obj_enabled_child_count(obj);
        if (count) {
            obj_dump_datadefQ(obj_get_datadefQ(obj),
                              mode, 
                              nestlevel, 
                              indent+NCX_DEF_INDENT);
        }
        break;
    case OBJ_TYP_CASE:
        if (mode == HELP_MODE_BRIEF) {
            neednewline = TRUE;
            break;
        }
        count = obj_enabled_child_count(obj);
        if (count > 1) {
            obj_dump_datadefQ(obj_get_datadefQ(obj), 
                              mode, 
                              nestlevel,
                              indent+NCX_DEF_INDENT);
        } else if (count == 1) {
            testobj = obj_first_child(obj);
            if (testobj) {
                obj_dump_template(testobj,
                                  mode, 
                                  nestlevel,
                                  indent);
            }
        } /* else skip this case */
        break;
    case OBJ_TYP_LIST:
        switch (mode) {
        case HELP_MODE_BRIEF:
            break;
        case HELP_MODE_NORMAL:
        case HELP_MODE_FULL:
            keystr = obj_get_keystr(obj);
            if (keystr != NULL) {
                help_write_lines((const xmlChar *)"key: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(keystr, 0, FALSE);
            }
            if (!obj->def.list->ordersys) {
                help_write_lines((const xmlChar *)"ordered-by: user", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
            }

            if (mode == HELP_MODE_NORMAL) {
                break;
            }

            if (obj->def.list->minset) {
                help_write_lines((const xmlChar *)"min-elements: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                snprintf(numbuff, sizeof(numbuff), "%u",
                         obj->def.list->minelems);
                help_write_lines((const xmlChar *)numbuff, 0, FALSE);
            }
            if (obj->def.list->maxset) {
                help_write_lines((const xmlChar *)"max-elements: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                snprintf(numbuff, sizeof(numbuff), "%u",
                         obj->def.list->maxelems);
                help_write_lines((const xmlChar *)numbuff, 0, FALSE);
            }
            break;
        default:
            ;
        }
        if (mode != HELP_MODE_BRIEF) {
            obj_dump_datadefQ(obj_get_datadefQ(obj), 
                              mode, 
                              nestlevel,
                              indent+NCX_DEF_INDENT);
        }
        break;
    case OBJ_TYP_RPC:
        testobj = obj_find_child(obj, NULL, YANG_K_INPUT);
        if (testobj && obj_enabled_child_count(testobj)) {
            obj_dump_template(testobj,
                              mode,
                              nestlevel,
                              indent+NCX_DEF_INDENT);
        }

        testobj = obj_find_child(obj, NULL, YANG_K_OUTPUT);
        if (testobj && obj_enabled_child_count(testobj)) {
            obj_dump_template(testobj,
                              mode,
                              nestlevel,
                              indent+NCX_DEF_INDENT);
        }
        help_write_lines((const xmlChar *)"\n", 0, FALSE);
        break;
    case OBJ_TYP_RPCIO:
        if (mode != HELP_MODE_BRIEF) {
            testobj = obj_get_default_parm(obj);
            if (testobj && obj_is_enabled(testobj)) {
                help_write_lines((const xmlChar *)"default parameter: ", 
                                 indent+NCX_DEF_INDENT, 
                                 TRUE); 
                help_write_lines(obj_get_name(testobj), 0, FALSE);
            } else {
                neednewline = FALSE;
            }
        }
        obj_dump_datadefQ(obj_get_datadefQ(obj), 
                          mode, 
                          nestlevel,
                          indent+NCX_DEF_INDENT);
        break;
    case OBJ_TYP_NOTIF:
        obj_dump_datadefQ(obj_get_datadefQ(obj),
                          mode, 
                          nestlevel, 
                          indent+NCX_DEF_INDENT);
        break;
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_USES:
    case OBJ_TYP_REFINE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (neednewline) {
        help_write_lines(NULL, 0, TRUE);
    }

}   /* obj_dump_template */


/********************************************************************
* FUNCTION obj_dump_datadefQ
*
* Dump the contents of a datadefQ for debugging
*
* INPUTS:
*   datadefQ == Q of obj_template to dump
*   mode    ==  desired help mode (brief, normal, full)
*   nestlevel == number of levels from the top-level
*                that should be printed; 0 == all levels
*   indent == start indent count
*********************************************************************/
void
    obj_dump_datadefQ (dlq_hdr_t *datadefQ,
                       help_mode_t mode,
                       uint32 nestlevel,
                       uint32 indent)
{
    obj_template_t  *obj;
    uint32           objnestlevel;

#ifdef DEBUG
    if (!datadefQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (mode > HELP_MODE_FULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    if (mode == HELP_MODE_NONE) {
        return;
    }

    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) ||
            !obj_is_enabled(obj)) {
            continue;
        }

        objnestlevel = obj_get_level(obj);
        if (mode == HELP_MODE_BRIEF && objnestlevel > nestlevel) {
            continue;
        }

        obj_dump_template(obj, mode, nestlevel, indent);

        switch (obj->objtype) {
        case OBJ_TYP_RPCIO:
        case OBJ_TYP_CHOICE:
            break;
        case OBJ_TYP_CASE:
            switch (obj_enabled_child_count(obj)) {
            case 0:
                break;
            case 1:
                help_write_lines((const xmlChar *)"\n", 0, FALSE);
                break;
            default:
                break;
            }
            break;
        default:
            help_write_lines((const xmlChar *)"\n", 0, FALSE);
        }
    }

}   /* obj_dump_datadefQ */


/* END obj_help.c */
