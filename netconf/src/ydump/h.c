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
/*  FILE: h.c

    Generate H file output from YANG module

   Identifier type codes:

      Features        --> 'F'
      Identities      --> 'I'
      Data Nodes      --> 'N'
      Typedef         --> 'T'

   Identifier format:

     <cvt-module-name>_<identifier-type>_<cvt-identifier>

   Identifiers are converted as follows:

     '.' -->  '_'
     '-' -->  '_'

   Collisions on pathalogical collisions are not supported yet


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
28mar09      abb      begun

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
#include "c.h"
#include "c_util.h"
#include "dlq.h"
#include "ext.h"
#include "h.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "rpc.h"
#include "ses.h"
#include "status.h"
#include "typ.h"
#include "yang.h"
#include "yangconst.h"
#include "yangdump.h"
#include "yangdump_util.h"


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
*                       V A R I A B L E S                            *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION write_end_cplusplus
* 
* Generate the end __cplusplus wrapper
*
* INPUTS:
*   scb == session control block to use for writing
*
*********************************************************************/
static void
    write_end_cplusplus (ses_cb_t *scb)
{
    ses_putstr(scb, (const xmlChar *)"\n#ifdef __cplusplus");
    ses_putstr(scb, (const xmlChar *)"\n} /* end extern 'C' */");
    ses_putstr(scb, (const xmlChar *)"\n#endif");

}  /* write_end_cplusplus */


/********************************************************************
* FUNCTION write_start_cplusplus
* 
* Generate the start __cplusplus wrapper
*
* INPUTS:
*   scb == session control block to use for writing
*
*********************************************************************/
static void
    write_start_cplusplus (ses_cb_t *scb)
{
    ses_putstr(scb, (const xmlChar *)"\n#ifdef __cplusplus");
    ses_putstr(scb, (const xmlChar *)"\nextern \"C\" {");
    ses_putstr(scb, (const xmlChar *)"\n#endif");

}  /* write_start_cplusplus */


/********************************************************************
* FUNCTION write_h_object_typdef
* 
* Generate the H file typdefs definitions for 1 data node
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == obj_template_t to use
*   cdefQ == Q of c_define_t structs to check for identity binding
**********************************************************************/
static void
    write_h_object_typdef (ses_cb_t *scb,
                           obj_template_t *obj,
                           dlq_hdr_t *cdefQ)
{
    c_define_t              *cdef;
    obj_template_t          *childobj;
    ncx_btype_t              btyp;
    boolean                  isleaflist, isanyxml;

    isleaflist = FALSE;
    isanyxml = FALSE;

    /* typedefs not needed for simple objects */
    if (obj->objtype == OBJ_TYP_LEAF) {
        return;
    } else if (obj->objtype == OBJ_TYP_LEAF_LIST) {
        isleaflist = TRUE;
    } else if (obj->objtype == OBJ_TYP_ANYXML) {
        isanyxml = TRUE;
    }

    cdef = find_path_cdefine(cdefQ, obj);
    if (cdef == NULL) {
        SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return;
    }

    btyp = obj_get_basetype(obj);

    ses_putchar(scb, '\n');
    write_h_iffeature_start(scb, &obj->iffeatureQ);

    /* generate the YOID as a comment */
    write_c_oid_comment(scb, obj);

    /* start a new line and a C type definition */
    ses_putstr(scb, START_TYPEDEF);

    if (isleaflist || isanyxml) {
        ses_putstr(scb, STRUCT);
    } else {
        /* print the C top-level data type */
        switch (btyp) {
        case NCX_BT_CONTAINER:
        case NCX_BT_CASE:
        case NCX_BT_LIST:
            ses_putstr(scb, STRUCT);
            break;
        case NCX_BT_CHOICE:
            ses_putstr(scb, UNION);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        }
    }
    ses_putchar(scb, ' ');

    /* the first 'real' name has an underscore on the end */
    ses_putstr(scb, cdef->idstr);
    ses_putchar(scb, '_');

    /* start the guts of the typedef */
    ses_putstr(scb, START_BLOCK);

    /* generate a line for a Q header or a Queue */
    if (obj->objtype == OBJ_TYP_LIST || 
        obj->objtype == OBJ_TYP_LEAF_LIST ||
        isanyxml) {
        write_qheader(scb);
    }

    if (isleaflist) { 
        /* generate a line for the leaf-list data type */
        ses_indent(scb, ses_indent_count(scb));
        write_c_objtype_ex(scb, obj, cdefQ, ';', FALSE, FALSE);
    } else if (!isanyxml) {
        /* generate a line for each child node */
        for (childobj = obj_first_child(obj);
             childobj != NULL;
             childobj = obj_next_child(childobj)) {

            if (!obj_has_name(childobj) || 
                obj_is_cli(childobj) ||
                obj_is_abstract(childobj)) {
                continue;
            }

            write_h_iffeature_start(scb, &childobj->iffeatureQ);

            if (childobj->objtype == OBJ_TYP_LEAF_LIST) {
                ses_indent(scb, ses_indent_count(scb));
                ses_putstr(scb, QUEUE);
                ses_putchar(scb, ' ');
                write_c_safe_str(scb, obj_get_name(childobj));
                ses_putchar(scb, ';');
            } else {
                ses_indent(scb, ses_indent_count(scb));
                write_c_objtype_ex(scb, childobj,  cdefQ, ';',
                                   FALSE, FALSE);
            }

            write_h_iffeature_end(scb, &childobj->iffeatureQ);

        }
    }

    ses_putstr(scb, END_BLOCK);
    ses_putchar(scb, ' ');
    ses_putstr(scb, cdef->idstr);
    ses_putchar(scb, ';');

    write_h_iffeature_end(scb, &obj->iffeatureQ);

}  /* write_h_object_typdef */


/********************************************************************
* FUNCTION write_h_objects
* 
* Generate the YANG for the specified datadefQ
*
* INPUTS:
*   scb == session control block to use for writing
*   datadefQ == Q of obj_template_t to use
*   typcdefQ == Q of typename binding c_define_t structs
*   cp == conversion parms to use
*********************************************************************/
static void
    write_h_objects (ses_cb_t *scb,
                     dlq_hdr_t *datadefQ,
                     dlq_hdr_t *typcdefQ,
                     const yangdump_cvtparms_t *cp)
{
    obj_template_t    *obj;
    dlq_hdr_t         *childdatadefQ;

    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (!obj_has_name(obj) || obj_is_cli(obj) || obj_is_abstract(obj)) {
            continue;
        }

        childdatadefQ = obj_get_datadefQ(obj);
        if (childdatadefQ) {
            write_h_objects(scb, childdatadefQ, typcdefQ, cp);
        }

        write_h_object_typdef(scb, obj, typcdefQ);
    }

}  /* write_h_objects */


/********************************************************************
* FUNCTION write_h_identity
* 
* Generate the #define for 1 identity
*
* INPUTS:
*   scb == session control block to use for writing
*   identity == ncx_identity_t to use
*   cp == conversion parms to use
*
*********************************************************************/
static void
    write_h_identity (ses_cb_t *scb,
                      const ncx_identity_t *identity,
                      const yangdump_cvtparms_t *cp)
{
    /* define the identity constant */
    ses_putstr(scb, POUND_DEFINE);
    write_identifier(scb, 
                     identity->tkerr.mod->name,
                     BAR_ID,
                     identity->name,
                     cp->isuser);
    ses_putchar(scb, ' ');
    ses_putstr(scb, BAR_CONST);
    ses_putchar(scb, '"');
    ses_putstr(scb, identity->name);
    ses_putchar(scb, '"');

}  /* write_h_identity */


/********************************************************************
* FUNCTION write_h_identities
* 
* Generate the H file delcs for the specified identityQ
*
* INPUTS:
*   scb == session control block to use for writing
*   identityQ == que of ncx_identity_t to use
*   cp == conversion parms to use
*********************************************************************/
static void
    write_h_identities (ses_cb_t *scb,
                        const dlq_hdr_t *identityQ,
                        const yangdump_cvtparms_t *cp)
{

    const ncx_identity_t *identity;

    if (dlq_empty(identityQ)) {
        return;
    }

    for (identity = (const ncx_identity_t *)
             dlq_firstEntry(identityQ);
         identity != NULL;
         identity = (const ncx_identity_t *)
             dlq_nextEntry(identity)) {

        write_h_identity(scb, identity, cp);
    }
    ses_putchar(scb, '\n');

}  /* write_h_identities */


/********************************************************************
* FUNCTION write_h_oid
* 
* Generate the #define for 1 object node identifier
*
* INPUTS:
*   scb == session control block to use for writing
*   cdef == c_define_t struct to use
*
*********************************************************************/
static void
    write_h_oid (ses_cb_t *scb,
                 const c_define_t *cdef)
{
    /* define the identity constant */
    ses_putstr(scb, POUND_DEFINE);
    ses_putstr(scb, cdef->idstr);
    ses_putchar(scb, ' ');
    ses_putstr(scb, BAR_CONST);
    write_c_str(scb, cdef->valstr, 2);

}  /* write_h_oid */


/********************************************************************
* FUNCTION write_h_oids
* 
* Generate the #defines for the specified #define statements
* for name identifier strings
*
* INPUTS:
*   scb == session control block to use for writing
*   cdefineQ == que of c_define_t structs to use
*
*********************************************************************/
static void
    write_h_oids (ses_cb_t *scb,
                  const dlq_hdr_t *cdefineQ)
{
    const c_define_t     *cdef;

    for (cdef = (const c_define_t *)dlq_firstEntry(cdefineQ);
         cdef != NULL;
         cdef = (const c_define_t *)dlq_nextEntry(cdef)) {

        write_h_oid(scb, cdef);
    }

}  /* write_h_oids */


/********************************************************************
* FUNCTION save_h_oids
* 
* Save the identity to name bindings for the object names
*
* INPUTS:
*   scb == session control block to use for writing
*   cdefineQ == que of c_define_t structs to use
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    save_h_oids (ses_cb_t *scb,
                 const dlq_hdr_t *datadefQ,
                 dlq_hdr_t *cdefineQ)
{
    const dlq_hdr_t      *childQ;
    const obj_template_t *obj;
    status_t              res;

    if (dlq_empty(datadefQ)) {
        return NO_ERR;
    }

    for (obj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {

        if (obj_has_name(obj) && 
            !obj_is_cli(obj) &&
            !obj_is_abstract(obj)) {

            if (obj->objtype != OBJ_TYP_RPCIO) {
                res = save_oid_cdefine(cdefineQ,
                                       obj_get_mod_name(obj),
                                       obj_get_name(obj));
                if (res != NO_ERR) {
                    return res;
                }
            }

            childQ = obj_get_cdatadefQ(obj);
            if (childQ) {
                res = save_h_oids(scb, childQ, cdefineQ);
                if (res != NO_ERR) {
                    return res;
                }
            }
        }
    }

    return NO_ERR;

}  /* save_h_oids */


/********************************************************************
* FUNCTION write_h_feature
* 
* Generate the #define for 1 feature statement
*
* INPUTS:
*   scb == session control block to use for writing
*   feature == ncx_feature_t to use
*********************************************************************/
static void
    write_h_feature (ses_cb_t *scb,
                     const ncx_feature_t *feature)
{
    write_h_iffeature_start(scb, &feature->iffeatureQ);

    /* define feature constant */
    ses_putstr(scb, (const xmlChar *)"\n/* Feature ");
    ses_putstr(scb,  feature->tkerr.mod->name);
    ses_putchar(scb, ':');
    ses_putstr(scb,  feature->name);
    ses_putstr(scb, (const xmlChar *)"\n * Comment out to disable */");
    ses_putstr(scb, POUND_DEFINE);
    write_identifier(scb, feature->tkerr.mod->name, BAR_FEAT,
                     feature->name, TRUE);
    ses_putchar(scb, ' ');
    ses_putchar(scb, '1');

    write_h_iffeature_end(scb, &feature->iffeatureQ);
    ses_putchar(scb, '\n');

}  /* write_h_feature */


/********************************************************************
* FUNCTION write_h_features
* 
* Generate the H file decls for the specified featureQ
*
* INPUTS:
*   scb == session control block to use for writing
*   featureQ == que of ncx_feature_t to use
*
*********************************************************************/
static void
    write_h_features (ses_cb_t *scb,
                      const dlq_hdr_t *featureQ)
{
    const ncx_feature_t *feature;

    if (dlq_empty(featureQ)) {
        return;
    }

    for (feature = (const ncx_feature_t *)
             dlq_firstEntry(featureQ);
         feature != NULL;
         feature = (const ncx_feature_t *)
             dlq_nextEntry(feature)) {

        write_h_feature(scb, feature);
    }
    ses_putchar(scb, '\n');

}  /* write_h_features */


/********************************************************************
* FUNCTION write_h_includes
* 
* Write the H file #include statements
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
static void
    write_h_includes (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp)
{
    boolean                   needrpc, neednotif;

    needrpc = need_rpc_includes(mod, cp);
    neednotif = need_notif_includes(mod, cp);

    /* add xmlChar include */
    write_ext_include(scb, (const xmlChar *)"xmlstring.h");

    if (cp->isuser) {
        write_ncx_include(scb, (const xmlChar *)"agt");
        write_ncx_include(scb, (const xmlChar *)"agt_cb");

        if (neednotif) {
            write_ncx_include(scb, (const xmlChar *)"agt_not");
        }
        
        if (needrpc) {
            write_ncx_include(scb, (const xmlChar *)"agt_rpc");
        }
    }

    write_ncx_include(scb, (const xmlChar *)"dlq");
    write_ncx_include(scb, (const xmlChar *)"ncxtypes");
    write_ncx_include(scb, (const xmlChar *)"op");

    if (cp->isuser) {
        write_ncx_include(scb, (const xmlChar *)"rpc");
        write_ncx_include(scb, (const xmlChar *)"ses");
    }

    write_ncx_include(scb, (const xmlChar *)"status");
    write_ncx_include(scb, (const xmlChar *)"val");

    if (cp->format == NCX_CVTTYP_UH) {
        write_cvt_include(scb, ncx_get_modname(mod), NCX_CVTTYP_YH);
    }

    /* includes for submodules */
    if (!cp->unified) {
        const ncx_include_t      *inc;
        for (inc = (const ncx_include_t *)
                 dlq_firstEntry(&mod->includeQ);
             inc != NULL;
             inc = (const ncx_include_t *)dlq_nextEntry(inc)) {

            write_ncx_include(scb, inc->submodule);
        }
    }

    ses_putchar(scb, '\n');
} /* write_h_includes */


/********************************************************************
* FUNCTION write_h_file
* 
* Generate the module start and header
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    write_h_file (ses_cb_t *scb,
                  ncx_module_t *mod,
                  const yangdump_cvtparms_t *cp)
{
    yang_node_t *node;
    const xmlChar *prefix;
    dlq_hdr_t    cdefineQ, typenameQ, objnameQ;
    status_t     res = NO_ERR;

    switch (cp->format) {
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_CPP_TEST:
    case NCX_CVTTYP_H:
        prefix = EMPTY_STRING;
        break;
    case NCX_CVTTYP_YC:
    case NCX_CVTTYP_YH:
        prefix = Y_PREFIX;
        break;
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_UH:
        prefix = U_PREFIX;
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    dlq_createSQue(&cdefineQ);
    dlq_createSQue(&typenameQ);
    dlq_createSQue(&objnameQ);

    /* Write the start of the H file */
    ses_putstr(scb, POUND_IFNDEF);
    ses_putstr(scb, BAR_H);
    ses_putstr(scb, prefix);
    write_c_safe_str(scb, mod->name);
    ses_putstr(scb, POUND_DEFINE);
    ses_putstr(scb, BAR_H);
    ses_putstr(scb, prefix);
    write_c_safe_str(scb, mod->name);

    write_c_header(scb, mod, cp);

    write_h_includes(scb, mod, cp);

    write_start_cplusplus(scb);

    if (!cp->isuser) {
        ses_putchar(scb, '\n');
        /* mod name */
        ses_putstr(scb, POUND_DEFINE);
        write_identifier(scb, mod->name, BAR_MOD, mod->name, FALSE);
        ses_putchar(scb, ' ');
        ses_putstr(scb, BAR_CONST);
        ses_putchar(scb, '"');
        ses_putstr(scb, mod->name);
        ses_putchar(scb, '"');

        /* mod version */
        ses_putstr(scb, POUND_DEFINE);
        write_identifier(scb, mod->name, BAR_REV, mod->name, FALSE);
        ses_putchar(scb, ' ');
        ses_putstr(scb, BAR_CONST);
        ses_putchar(scb, '"');
        if (mod->version) {
            ses_putstr(scb, mod->version);
        }
        ses_putchar(scb, '"');
        ses_putchar(scb, '\n');
    }

    if (cp->format == NCX_CVTTYP_H || cp->format == NCX_CVTTYP_UH) {
        /* 1) features */
        write_h_features(scb, &mod->featureQ);
        if (cp->unified && mod->ismod) {
            for (node = (yang_node_t *)
                     dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (yang_node_t *)dlq_nextEntry(node)) {
                if (node->submod) {
                    write_h_features(scb, &node->submod->featureQ);
                }
            }
        }
    }

    if (!cp->isuser) {
        /* 2) identities */
        write_h_identities(scb, &mod->identityQ, cp);
        if (cp->unified && mod->ismod) {
            for (node = (yang_node_t *)
                     dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (yang_node_t *)dlq_nextEntry(node)) {
                if (node->submod) {
                    write_h_identities(scb, &node->submod->identityQ, cp);
                }
            }
        }

        /* 3) object node identifiers */
        res = save_h_oids(scb, &mod->datadefQ, &cdefineQ);
        if (res == NO_ERR) {
            if (cp->unified && mod->ismod) {
                for (node = (yang_node_t *)
                         dlq_firstEntry(&mod->allincQ);
                     node != NULL && res == NO_ERR;
                     node = (yang_node_t *)dlq_nextEntry(node)) {
                    if (node->submod) {
                        res = save_h_oids(scb, 
                                          &node->submod->datadefQ, 
                                          &cdefineQ);
                    }
                }
            }
        }

        if (res == NO_ERR) {
            write_h_oids(scb, &cdefineQ);
        }
    }

    /* 4) typedefs for objects */
    if (res == NO_ERR) {
        res = save_c_objects(mod, &mod->datadefQ, &typenameQ, C_MODE_TYPEDEF);
        if (res == NO_ERR) {
            if (cp->unified && mod->ismod) {
                for (node = (yang_node_t *)
                         dlq_firstEntry(&mod->allincQ);
                     node != NULL && res == NO_ERR;
                     node = (yang_node_t *)dlq_nextEntry(node)) {
                    if (node->submod) {
                        res = save_c_objects(node->submod,
                                             &node->submod->datadefQ, 
                                             &typenameQ,
                                             C_MODE_TYPEDEF);
                    }
                }
            }
        }
    }

    if (res == NO_ERR && 
        (cp->format == NCX_CVTTYP_H || cp->format == NCX_CVTTYP_UH)) {
        /* write typedefs for objects, not actually used in SIL code */
        write_h_objects(scb, &mod->datadefQ, &typenameQ, cp);
        if (cp->unified && mod->ismod) {
            for (node = (yang_node_t *)
                     dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (yang_node_t *)dlq_nextEntry(node)) {
                if (node->submod) {
                    write_h_objects(scb, &node->submod->datadefQ, 
                                    &typenameQ, cp);
                }
            }
        }
    }

    if (res == NO_ERR) {
        res = save_all_c_objects(mod, cp, &objnameQ, C_MODE_CALLBACK);
        if (res == NO_ERR) {
            c_write_fn_prototypes(mod, cp, scb, &objnameQ);
        }
    }

    write_end_cplusplus(scb);

    /* Write the end of the H file */
    ses_putchar(scb, '\n');
    ses_putstr(scb, POUND_ENDIF);

    clean_cdefineQ(&cdefineQ);
    clean_cdefineQ(&typenameQ);
    clean_cdefineQ(&objnameQ);

    return res;

} /* write_h_file */


/*********     E X P O R T E D   F U N C T I O N S    **************/


/********************************************************************
* FUNCTION h_convert_module
* 
*  Generate the SIL H file code for the specified module(s)
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*
* RETURNS:
*   status
*********************************************************************/
status_t
    h_convert_module (yang_pcb_t *pcb,
                      const yangdump_cvtparms_t *cp,
                      ses_cb_t *scb)
{
    ncx_module_t  *mod;
    status_t       res;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        return SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
    }

    /* do not generate C code for obsolete objects */
    ncx_delete_all_obsolete_objects();

    res = write_h_file(scb, mod, cp);

    return res;

}   /* h_convert_module */

/* END file h.c */
