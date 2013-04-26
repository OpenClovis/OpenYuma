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
/*  FILE: xsd.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
15nov06      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>
#include <xmlreader.h>

#include "procdefs.h"
#include "cfg.h"
#include "dlq.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "rpc.h"
#include "status.h"
#include "typ.h"
#include "val.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_val.h"
#include "xsd.h"
#include "xsd_typ.h"
#include "xsd_util.h"
#include "xsd_yang.h"
#include "yang.h"
#include "yangdump.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION add_one_prefix
* 
*   Add one prefix clause, after checking if it is needed first
*
* INPUTS:
*   pcb == parser control block
*   modname == import module name to check
*   revision == module revision to find (may be NULL)
*   top_attrs == pointer to receive the top element attributes
*               HACK: rpc_agt needs the top-level attrs to
*               be in xml_attr_t format, not val_value_t metaval format
*
* OUTPUTS:
*   *retattrs = may have an attribute added to this Q
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_one_prefix (yang_pcb_t *pcb,
                    const xmlChar *modname,
                    const xmlChar *revision,
                    xml_attrs_t *top_attrs)
{
    const xmlChar *cstr;
    ncx_module_t  *immod;
    xml_attr_t    *test;
    status_t       res;
    boolean        found;

    res = NO_ERR;
            
    /* don't use the NCX XSD module, use XSD directly */
    if (!xml_strcmp(modname, (const xmlChar *)"xsd")) {
        return NO_ERR;
    }

    /* first load the module to get the NS ID 
     * It will probably be loaded anyway to find external names.
     */
    immod = ncx_find_module(modname, revision);
    if (!immod) {
        res = ncxmod_load_module(modname, revision, pcb->savedevQ, &immod);
    }
    if (!immod) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* make sure this NS has not already been added */
    found = FALSE;
    for (test = xml_first_attr(top_attrs);
         test != NULL && !found;
         test = xml_next_attr(test)) {
        if (immod->nsid == test->attr_xmlns_ns) {
            /* the xmlns_ns field is only set when the attr
             * is an xmlns declaration
             */
            found = TRUE;
        }
    }

    if (!found) {
        cstr = xmlns_get_ns_prefix(immod->nsid);
        res = xml_add_xmlns_attr(top_attrs, immod->nsid, cstr);
    }

    return res;

}  /* add_one_prefix */


/********************************************************************
* FUNCTION add_module_defs
* 
* Add the module definitions for the main module or a submodule
*
* INPUTS:
*   mod == module in progress
*   val == val_value_t to receive child nodes
*
* OUTPUTS:
*   val->v.childQ may have entries added
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_module_defs (ncx_module_t *mod,
                     val_value_t *val)
{
    status_t           res;

    /* convert NCX/YANG types to XSD types */
    res = xsd_add_types(mod, val);
    if (res != NO_ERR) {
        return res;
    }
    
    /* setup the local typedefs in all groupings
     * after this fn call, there might be malloced data in the
     * mod->typnameQ that has to be freed before exiting
     */
    res = xsd_load_typenameQ(mod);
    if (res != NO_ERR) {
        return res;
    }

    /* convert YANG local types to XSD types */
    res = xsd_add_local_types(mod, val);
    if (res != NO_ERR) {
        return res;
    }

    /* convert YANG groupings to XSD groups */
    res = xsd_add_groupings(mod, val);
    if (res != NO_ERR) {
        return res;
    }

    /* convert YANG Objects to XSD elements */
    res = xsd_add_objects(mod, val);
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

}   /* add_module_defs */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION xsd_convert_module
* 
*   Convert a [sub]module to XSD 1.0 format
*   module entry point: convert an entire module to a value struct
*   representing an XML element which represents an XSD
*
* INPUTS:
*   mod == module to convert
*   cp == conversion parms struct to use
*   retval == address of val_value_t to receive a malloced value struct
*             representing the XSD.  Extra mallocs will be avoided by
*             referencing the mod_module_t strings.  
*             Do not remove the module from the registry while this
*             val struct is being used!!!
*   top_attrs == pointer to receive the top element attributes
*               HACK: rpc_agt needs the top-level attrs to
*               be in xml_attr_t format, not val_value_t metaval format
*
* OUTPUTS:
*   *retval = malloced return value; MUST FREE THIS LATER
*   *top_attrs = filled in attrs queue
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xsd_convert_module (yang_pcb_t *pcb,
                        ncx_module_t *mod,
                        yangdump_cvtparms_t *cp,
                        val_value_t **retval,
                        xml_attrs_t *top_attrs)
{
    val_value_t       *val, *annot;
    const xmlChar     *cstr;
    xmlChar           *str;
    ncx_import_t      *import;
    yang_node_t       *node;
    yang_import_ptr_t *impptr; 
    status_t           res;
    xmlns_id_t         xsd_id, ncx_id, xsi_id, nc_id, ncn_id;
    boolean            needed;

    xsd_id = xmlns_xs_id();
    *retval = NULL;
    needed = FALSE;

    /* create a struct named 'schema' to hold the entire result */
    val = xml_val_new_struct(XSD_SCHEMA, xsd_id);
    if (!val) {
        return ERR_INTERNAL_MEM;
    }

    /* set the XSD NS for schema */
    cstr = xmlns_get_ns_prefix(xsd_id);
    res = xml_add_xmlns_attr(top_attrs, xsd_id, cstr);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    cstr = (mod->ns) ? mod->ns : EMPTY_STRING;

    /* Set the xmlns directive for the target namespace
     * !!! This is set as the default namespace, and code in other
     * !!! modules is hardwired to know that the default namespace
     * !!! is set to the target namespace
     */
    if (mod->nsid) {
        res = xml_add_xmlns_attr(top_attrs, mod->nsid, NULL);
    } else {
        res = xml_add_xmlns_attr_string(top_attrs, cstr, NULL);
    }
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    /* set the target namespace */
    res = xml_add_attr(top_attrs, 0, XSD_TARG_NS, cstr);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    if (cp->schemaloc) {
        /* set the XSI NS (for schemaLocation) */
        xsi_id = xmlns_xsi_id();
        cstr = xmlns_get_ns_prefix(xsi_id);
        res = xml_add_xmlns_attr(top_attrs, xsi_id, cstr);
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }

        /* set the schema location */
        str = xsd_make_schema_location(mod, cp->schemaloc, cp->versionnames);
        if (str) {
            res = xml_add_attr(top_attrs, xsi_id, XSD_LOC, str);
            m__free(str);
            if (res != NO_ERR) {
                val_free_value(val);
                return res;
            }
        } else {
            val_free_value(val);
            return ERR_INTERNAL_MEM;
        }
    }

    /* set the elementFormDefault */
    res = xml_add_attr(top_attrs, 0, XSD_EF_DEF, XSD_QUAL);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    /* set the attributeFormDefault */
    res = xml_add_attr(top_attrs, 0, XSD_AF_DEF, XSD_UNQUAL);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    /* set the language attribute */
    res = xml_add_attr(top_attrs, 0, XSD_LANG, XSD_EN);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    /* set the version attribute */
    if (mod->version) {
        res = xml_add_attr(top_attrs, 0, NCX_EL_VERSION, mod->version);
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }
    }

    /* Add the NCX namespace for appinfo stuff;
     * not sure if it will get used
     */
    ncx_id = xmlns_ncx_id();
    if (ncx_id != mod->nsid) {
        res = xml_add_xmlns_attr(top_attrs, ncx_id, 
                                 xmlns_get_ns_prefix(ncx_id));
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }
    }

    /* add the NETCONF NS if any RPC or OBJECT definitions */
    nc_id = xmlns_nc_id();
    if (!dlq_empty(&mod->datadefQ)) {
        res = xml_add_xmlns_attr(top_attrs, nc_id, 
                                 xmlns_get_ns_prefix(nc_id));
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }
    }

    /* add the NETCONF Notification NS if any Notification definitions */
    if (obj_any_notifs(&mod->datadefQ)) {
        needed = TRUE;
    } else if (cp->unified && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (obj_any_notifs(&node->submod->datadefQ)) {
                needed = TRUE;
            }
        }
    } else {
        needed = FALSE;
    }
    if (needed) {
        ncn_id = xmlns_ncn_id();
        if (mod->nsid != ncn_id) {
            res = xml_add_xmlns_attr(top_attrs, 
                                     ncn_id, 
                                     xmlns_get_ns_prefix(ncn_id));
            if (res != NO_ERR) {
                val_free_value(val);
                return res;
            }
        }
    }

    /* add xmlns declarations for all the imported modules */
    if (cp->unified && mod->ismod) {
        for (impptr = (yang_import_ptr_t *)dlq_firstEntry(&mod->saveimpQ);
             impptr != NULL;
             impptr = (yang_import_ptr_t *)dlq_nextEntry(impptr)) {
            res = add_one_prefix(pcb, impptr->modname, impptr->revision, 
                                 top_attrs);
            if (res != NO_ERR) {
                val_free_value(val);
                return res;
            }
        }
    } else {
        for (import = (ncx_import_t *)dlq_firstEntry(&mod->importQ);
             import != NULL;
             import = (ncx_import_t *)dlq_nextEntry(import)) {

            res = add_one_prefix(pcb, import->module, import->revision, 
                                 top_attrs);
            if (res != NO_ERR) {
                val_free_value(val);
                return res;
            }
        }
    }

    /* add an annotation node if there are any 
     * module meta clauses or appinfo clauses present
     * There will always be at least one auto-generated
     * documentation element
     */
    annot = xml_val_new_struct(XSD_ANNOTATION, xsd_id);
    if (!annot) {
        val_free_value(val);
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(annot, val);
    }

    /* generate all the module meta-data for the XSD schema element */
    res = xsd_add_mod_documentation(mod, annot);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    /* convert YANG imports to XSD imports */
    if (cp->schemaloc) {
        res = xsd_add_imports(pcb, mod, cp, val);
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }
    }

    if (!cp->unified) {
        /* convert YANG includes to XSD includes */
        res = xsd_add_includes(mod, cp, val);
        if (res != NO_ERR) {
            val_free_value(val);
            return res;
        }
    }

    res = add_module_defs(mod, val);
    if (res != NO_ERR) {
        val_free_value(val);
        return res;
    }

    if (cp->unified && mod->ismod) {
        for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                res = add_module_defs(node->submod, val);
                if (res != NO_ERR) {
                    val_free_value(val);
                    return res;
                }
            }
        }        
    }

    *retval = val;
    return NO_ERR;

}   /* xsd_convert_module */


/********************************************************************
* FUNCTION xsd_load_typenameQ
* 
*   Generate unique type names for the entire module
*   including any submodules that were parsed as well
*
* INPUTS:
*   mod == module to process
*
* OUTPUTS:
*   *val = malloced return value
*   *retattrs = malled attrs queue
* RETURNS:
*   status
*********************************************************************/
status_t
    xsd_load_typenameQ (ncx_module_t *mod)
{
    yang_node_t   *node;
    status_t       res;

    res = NO_ERR;

    /* setup the local typedefs in all groupings
     * after this fn call, there might be malloced data in the
     * mod->typnameQ that has to be freed before exiting
     */
    for (node = (yang_node_t *)dlq_firstEntry(&mod->allincQ);
         node != NULL && res==NO_ERR;
         node = (yang_node_t *)dlq_nextEntry(node)) {

        res = xsd_do_typedefs_groupingQ(node->submod,
                                        &node->submod->groupingQ,
                                        &mod->typnameQ);

        /* setup the local typedefs in all objects */
        if (res == NO_ERR) {
            res = xsd_do_typedefs_datadefQ(node->submod,
                                           &node->submod->datadefQ,
                                           &mod->typnameQ);
        }
    }

    /* setup the local typedefs in all objects */
    if (res == NO_ERR) {
        res = xsd_do_typedefs_datadefQ(mod, &mod->datadefQ, &mod->typnameQ);
    }

    if (res == NO_ERR) {
        res = xsd_do_typedefs_groupingQ(mod, &mod->groupingQ, &mod->typnameQ);
    }

    return res;

}  /* xsd_load_typenameQ */


/* END file xsd.c */
