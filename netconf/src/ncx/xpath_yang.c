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
/*  FILE: xpath_yang.c

    Schema and data model Xpath search support for
    YANG specific 'XPath-like' expressions.
    This module provides validation only.

    Once this module validates that the YANG-specific
    syntax is followed, then xpath1.c can be
    used to evaluate the expression and generate
    a node-set result.

   The following YANG features are supported:

    - augment target
    - refine target
    - leafref path target
    - key attribute for insert operation

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13nov08      abb      begun; split out from xpath.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>

#include  "procdefs.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "obj.h"
#include "tk.h"
#include "typ.h"
#include "val.h"
#include "xmlns.h"
#include "xpath.h"
#include "xpath_yang.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                         V A R I A B L E S                         *
*                                                                   *
*********************************************************************/

/********************************************************************
 * Utility function for selecting the object template to use.
 *
 * \param pcb the parser control block
 * \param  res the status of the selection 
 * \return useobj the selected  object template 
 ********************************************************************/
static obj_template_t** select_pcb_object( xpath_pcb_t* pcb, 
                                           status_t* res )
{
    switch (pcb->curmode) {
    case XP_CM_TARGET:
        *res = NO_ERR;
        return &pcb->targobj;

    case XP_CM_ALT:
        *res = NO_ERR;
        return &pcb->altobj;


    case XP_CM_KEYVAR:
        *res = NO_ERR;
        return &pcb->varobj;


    default:
        *res = ERR_INTERNAL_VAL;
        return NULL;
    }
}

/********************************************************************
 * Utility function for conditionally calling ncx_print_errormsg 
 * depending on pcb->logerrors.
 *
 * \param pcb the parser control block
 * \param mod the module 
 * \param res the status error
 * \return the status error
 ********************************************************************/
static status_t  pcb_print_errormsg( xpath_pcb_t* pcb, 
                                     ncx_module_t *mod, 
                                     status_t res )
{
    if ( pcb->logerrors ) {
        ncx_print_errormsg( pcb->tkc, mod, res );
    }
    return res;
}

/********************************************************************
 * Utility function for conditionally calling log_error 
 * depending on pcb->logerrors.
 *
 * \param pcb the parser control block
 * \param fmtstr the error message format string
 ********************************************************************/
static void pcb_log_error( xpath_pcb_t* pcb, 
                           const char* fmtstr, ... )
{
    if ( pcb->logerrors ) {
        va_list args;
        va_start(args, fmtstr);
        vlog_error( fmtstr, args );
        va_end( args );
    }
}

/********************************************************************
 * Utility function for conditionally calling log_error 
 * depending on pcb->logerrors.
 *
 * \param pcb the parser control block
 * \param mod the module 
 * \param res the status error
 * \param fmtstr the error message format string
 * \return the status error
 ********************************************************************/
static status_t pcb_log_error_msg( xpath_pcb_t* pcb, 
                               ncx_module_t *mod, 
                               status_t res,
                               const char* fmtstr, ... )
{
    if ( pcb->logerrors ) {
        ncx_print_errormsg( pcb->tkc, mod, res );

        va_list args;
        va_start(args, fmtstr);
        vlog_error( fmtstr, args );
        va_end( args );
    }
    return res;
}


/********************************************************************
 * Utility function for getting the target module for the nsid.
 *
 * \param nsid the nsid
 * \return the corresponding module.
 ********************************************************************/
static ncx_module_t* get_target_module_for_nsid( xmlns_id_t nsid )
{
    const xmlChar* modname = xmlns_get_module(nsid);
    if (modname) {
        return ncx_find_module(modname, NULL);
    }
    return NULL;
}

/********************************************************************
 * Utility function for testing if a target object is a leaf list
 *
 * \param pcb the parser control block in progress|
 * \return true if the target object is a leaf list 
 *********************************************************************/
static bool targ_obj_is_leaflist( xpath_pcb_t* pcb )
{
    return (pcb->targobj && pcb->targobj->objtype == OBJ_TYP_LEAF_LIST);
}

/********************************************************************
 * Utility function for getting the target module for selecting the
 * target module.
 *
 * \param pcb the parser control block in progress
 * \param prefix the prefix value used if any
 * \param nsid the namespace id for the module
 * \param laxnamespaces flag indicating if lax namespaces are being used.
 * \return the corresponding module.
 ********************************************************************/
static ncx_module_t* select_target_module( xpath_pcb_t* pcb, 
                                      const xmlChar* prefix,
                                      xmlns_id_t nsid, 
                                      boolean laxnamespaces,
                                      status_t* res )
{
    *res = NO_ERR;
    ncx_module_t* targmod = NULL;

    if( pcb->source == XP_SRC_XML || !pcb->tkerr.mod ) {
        if (nsid) {
            dlq_hdr_t *temp_modQ = ncx_get_temp_modQ();
            if (nsid == xmlns_nc_id() || !temp_modQ ) {
                targmod = get_target_module_for_nsid( nsid );
            } else if ( temp_modQ ) {
                /* try the temp Q first */
                targmod = ncx_find_module_que_nsid(temp_modQ, nsid);
                if ( !targmod ) {
                    targmod = get_target_module_for_nsid( nsid );
                }
            }

            if ( !targmod ) {
                *res = ERR_NCX_DEF_NOT_FOUND;
                pcb_log_error( pcb, "\nError: module not found in expr '%s'",
                               pcb->exprstr );
            }
        } else if (!laxnamespaces) {
            *res = ERR_NCX_UNKNOWN_NAMESPACE;
            pcb_log_error( pcb, "\nError: no namespace in expr '%s'", 
                           pcb->exprstr);
        }
    } else {
        *res = xpath_get_curmod_from_prefix( prefix, pcb->tkerr.mod, &targmod);
        if (*res != NO_ERR) {
            if ( !prefix && laxnamespaces) {
                *res = NO_ERR;
            } else {
                pcb_log_error( pcb, "\nError: Module for prefix '%s' not found",
                               (prefix) ? prefix : EMPTY_STRING );
            }
        }
    }

    return targmod;
}

/********************************************************************
 * Utility function for finding any object based on the node name.
 *
 * \param nodename the name of the object to find.
 * \return the corresponding object template.
 ********************************************************************/
static obj_template_t* find_any_object( const xmlChar* nodename )
{
    obj_template_t *foundobj = NULL;
    dlq_hdr_t *temp_modQ = ncx_get_temp_modQ();

    if ( temp_modQ ) {
        foundobj = ncx_find_any_object_que( temp_modQ, nodename );
     }

     if ( !foundobj ) {
         foundobj = ncx_find_any_object(nodename);
     }

     return foundobj;
}

/********************************************************************
 * Utility function for finding any object from root, using  the node name
 * and nsid
 *
 * \param nodename the name of the object to find.
 * \param nsid the nsid
 * \return the corresponding object template.
 ********************************************************************/
static obj_template_t* find_object_from_root( const xmlChar* nodename, 
                                              xmlns_id_t nsid )
{
    obj_template_t *foundobj = find_any_object( nodename );

     if ( foundobj && nsid && obj_get_nsid(foundobj) != nsid ) {
         /* did not match the specified namespace ID */
         foundobj = NULL;
     }

     return foundobj;
}

static obj_template_t* ensure_found_object_is_rpc( obj_template_t* foundobj )
{
    if (foundobj && foundobj->objtype == OBJ_TYP_RPC) {
        return foundobj;
     }
    return NULL;
}

/********************************************************************
 * Utility function for finding a top level object if the prefix or
 * nsid is not specified.
 *
 * \param pcb the parser control block in progress
 * \param prefix the prefix value used if any
 * \param nsid the namespace id for the module
 * \param nodename the name of the node to find
 * \param targobj the target module
 * \return the corresponding object template
 ********************************************************************/
static obj_template_t* find_top_level_object( xpath_pcb_t* pcb, 
                                       const xmlChar* prefix,
                                       xmlns_id_t nsid, 
                                       const xmlChar* nodename )
{
    obj_template_t* foundobj = NULL;

    if ( !pcb->targobj ) {
        pcb->targobj = pcb->obj;
    }

    if ( !pcb->targobj ) {
        return NULL;
    }


    if (obj_is_root(pcb->targobj)) {
        foundobj = find_object_from_root( nodename, nsid );
    } else if ( obj_get_nsid(pcb->targobj) == xmlns_nc_id() && 
                ( !xml_strcmp(obj_get_name(pcb->targobj), NCX_EL_RPC)) ) { 
        /* find an RPC method with the nodename */
        if ( prefix && nsid == 0) {
            nsid = xmlns_find_ns_by_prefix(prefix);
        }

        if ( !nsid ) {
            foundobj = find_any_object( nodename );
        } else {
            ncx_module_t* targmod = NULL;
            dlq_hdr_t *temp_modQ = ncx_get_temp_modQ();
            if (temp_modQ) {
                targmod = ncx_find_module_que_nsid(temp_modQ, nsid);
            } else {
                targmod = get_target_module_for_nsid( nsid );
            }

            if ( targmod ) {
                foundobj = ncx_find_object( targmod, nodename);
            }
        }

        foundobj = ensure_found_object_is_rpc( foundobj );
    } else {
        foundobj = obj_find_child( pcb->targobj, obj_get_mod_name(pcb->targobj),
                                   nodename);
        if ( !foundobj && !prefix ) {
            foundobj = obj_find_child( pcb->targobj, NULL, nodename);
        }
    }
    return foundobj;
}

/********************************************************************
 * Get the object identifier based on the ueeobj
 *
 * \param useobj the object being used
 * \param targnod the target module
 * \param nodename the node name to find (must be present)
 * \param laxnamespaces flag indicating if lax namespaces are being used.
 * \return status
 *********************************************************************/

static obj_template_t* find_object_from_use_obj( obj_template_t* useobj, 
                                                 ncx_module_t* targmod,
                                                 const xmlChar* nodename,
                                                 boolean laxnamespaces )
{
    obj_template_t* foundobj = NULL;

    if (obj_is_root(useobj)) {
        if ( targmod ) {
            foundobj = obj_find_template_top( targmod, ncx_get_modname(targmod),
                                              nodename);
        }
    } else if ( obj_get_nsid(useobj) == xmlns_nc_id() &&
                !xml_strcmp(obj_get_name(useobj), NCX_EL_RPC)) {
        /* get the rpc template ode for this object */
        if ( targmod ) {
            foundobj = obj_find_template_top(targmod, ncx_get_modname(targmod),
                                          nodename);
            foundobj = ensure_found_object_is_rpc( foundobj );
        }
    } else {
        /* get child node of this object */
        if ( targmod ) {
            foundobj = obj_find_child( useobj, ncx_get_modname(targmod),
                                       nodename);
        }
        else if ( !foundobj && laxnamespaces ) {
            foundobj = obj_find_child(useobj, NULL, nodename);
        }
    }
    return foundobj;
}

/********************************************************************
 * Get the object identifier associated with
 * QName in an leafref XPath expression
 *
 * \note Error messages are printed by this function!!
 *       Do not duplicate error messages upon error return
 *
 * \param pcb the parser control block in progress
 * \param prefix the prefix value used if any
 * \param nsid the namespace id for the module
 * \param nodename the node name to find (must be present)
 * \return status
 *********************************************************************/
static status_t set_next_objnode( xpath_pcb_t *pcb,
                                  const xmlChar *prefix,
                                  xmlns_id_t  nsid,
                                  const xmlChar *nodename )
{
    /* TBD: change this to an agt_profile 'namespaces' */
    boolean              laxnamespaces = TRUE;

    /* set the pcb target to get the result */
    status_t res = NO_ERR;
    obj_template_t** useobj = select_pcb_object( pcb, &res );
    if ( res != NO_ERR ) {
        return pcb_print_errormsg( pcb, pcb->tkerr.mod, res );
    }

    /* get the module from the NSID or the prefix */
    ncx_module_t* targmod = select_target_module( pcb, prefix, nsid, 
                                                  laxnamespaces, &res );
    if ( res != NO_ERR ) {
        return pcb_print_errormsg( pcb, pcb->tkerr.mod, res );
    }

    /* check if no NSID or prefix used: instead of rejecting
     * the request, check any top-level object, if allowed
     * by the laxnamespaces parameter
     */
    obj_template_t *foundobj = NULL;
    if ( !targmod && laxnamespaces && ( !nsid || !prefix )) {
        foundobj = find_top_level_object( pcb, prefix, nsid, nodename );
        if ( !foundobj ) {
            return pcb_log_error_msg(pcb, pcb->tkerr.mod, ERR_NCX_DEF_NOT_FOUND,
                      "\nError: No object match for node '%s' in expr '%s'", 
                      nodename, pcb->exprstr);
        }
    } else if ( *useobj ) {
        foundobj = find_object_from_use_obj( *useobj, targmod, nodename, 
                                              laxnamespaces ); 
    } else if ( pcb->curmode == XP_CM_KEYVAR || pcb->curmode == XP_CM_ALT ) {
        /* setting object for the first time, get child node of the current 
         * context object */
        if ( pcb->targobj && targmod ) {
            foundobj = obj_find_child( pcb->targobj, ncx_get_modname(targmod), 
                                       nodename );
        }
    } else if ( ( pcb->flags & XP_FL_ABSPATH ) ) {
        /* setting object for the first time get top-level object from 
         * object module */
        if ( targmod ) {
            foundobj = obj_find_template_top(targmod, ncx_get_modname(targmod),
                                             nodename);
        }
    } else {
        /* setting object for the first time but the context node is a leaf, 
         * so there is no possible child node of the start object */
        log_debug2( "\n%s setting object for the first time but the context "
                    "node is a leaf, so there is no possible child node "
                    "of the start object", __func__ );
    }

    if ( !foundobj ) {
        if (prefix) {
            pcb_log_error( pcb, "\nError: object not found '%s:%s'", prefix, 
                               nodename);
        } else {
            pcb_log_error( pcb, "\nError: object not found '%s'", nodename);
        }
        return pcb_print_errormsg( pcb, pcb->objmod, 
                                   ERR_NCX_DEF_NOT_FOUND );
    }

    *useobj = foundobj;
    return NO_ERR;
} /* set_next_objnode */

/********************************************************************
* FUNCTION move_up_obj
* 
* Get the parent object identifier
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t move_up_obj (xpath_pcb_t *pcb)
{
    obj_template_t  *foundobj = NULL;

    status_t res = NO_ERR;
    obj_template_t** useobj = select_pcb_object( pcb, &res );
    if ( NO_ERR != res ) {
        pcb_print_errormsg(pcb, pcb->objmod, res);
        return res;
    }

    if (*useobj) {
        /* get child node of this object */
        if ((*useobj)->parent) {
            foundobj = obj_get_real_parent(*useobj);
        } else if (*useobj != pcb->docroot) {
            foundobj = pcb->docroot;
        }
    } else {
        /* setting object for the first time
         * get parent node of the the start object 
         */
        foundobj = obj_get_real_parent(pcb->obj);
        if (foundobj == NULL) {
            foundobj = pcb->docroot;
        }
    }

    if (foundobj == NULL) {
        res = ERR_NCX_DEF_NOT_FOUND;
        if (pcb->logerrors) {
            log_error("\nError: parent not found for object '%s'",
                      obj_get_name(*useobj));
            ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
        }
    } else {
        *useobj = foundobj;
    }

    return res;

} /* move_up_obj */

/********************************************************************
 * retrieve the associated node name for the current tk_tt_period
 * pcb token.
 *
 * \param pcb the parser control block in progress|
 * \param res the result status
 * \return the associated node name
 *********************************************************************/
static const xmlChar* parse_node_identifier_tk_tt_period( xpath_pcb_t* pcb,
                                                          status_t* res )
{
    if (pcb->flags & XP_FL_INSTANCEID) {
        if ( !targ_obj_is_leaflist( pcb ) ) {
            *res = ERR_NCX_INVALID_VALUE;
        } else {
            *res = NO_ERR;
            return obj_get_name(pcb->targobj);
        }
    } else {
       *res = pcb_log_error_msg( pcb, NULL, ERR_NCX_WRONG_TKTYPE,
                                 "\nError: '.' not allowed here" );
    }
    return NULL;
}

/********************************************************************
 * retrieve the associated node name for the current tk_tt_string
 * pcb token.
 *
 * \param pcb the parser control block in progress|
 * \param prefix the prefix
 * \param nsid the nsid
 * \param res the result status
 * \return the associated node name
 *********************************************************************/
static const xmlChar* parse_node_identifier_tk_tt_string( xpath_pcb_t* pcb, 
                                            const xmlChar** prefix,
                                            xmlns_id_t* nsid,
                                            status_t* res )
{
    /* pfix:identifier */
    *prefix = TK_CUR_MOD(pcb->tkc);

    if (pcb->source != XP_SRC_XML) {
        if (pcb->tkerr.mod) {
            if ( xml_strcmp(pcb->tkerr.mod->prefix, *prefix)) {
                ncx_import_t* import = ncx_find_pre_import( pcb->tkerr.mod, 
                                                            *prefix);
                if (!import) {
                    *res = pcb_log_error_msg( pcb, pcb->tkerr.mod, 
                            ERR_NCX_PREFIX_NOT_FOUND, 
                            "\nError: '.' not allowed here" );
                    return NULL;
                } else {
                    ncx_module_t* testmod = ncx_find_module( import->module, 
                                                             import->revision );
                    if (testmod) {
                        *nsid = testmod->nsid;
                    }
                }
            }
        } else {
            *nsid = xmlns_find_ns_by_prefix(*prefix);
            if ( *nsid == XMLNS_NULL_NS_ID ) {
                *res = pcb_log_error_msg( pcb, pcb->tkerr.mod, 
                       ERR_NCX_PREFIX_NOT_FOUND, 
                       "\nError: module for prefix '%s' not found", *prefix );
                return NULL;
            }
        }
    } else {
        *res = xml_get_namespace_id( pcb->reader, *prefix, 
                                     TK_CUR_MODLEN(pcb->tkc), nsid);
        if ( *res != NO_ERR ) {
            *res = pcb_log_error_msg( pcb, pcb->tkerr.mod, *res,
                    "\nError: unknown XML prefix '%s'", *prefix );
            return NULL;
        }
    }

    /* save the NSID in the token for printing later */
    TK_CUR_NSID(pcb->tkc) = *nsid;
    return TK_CUR_VAL(pcb->tkc);
}

/********************************************************************
 * Parse the leafref node-identifier string
 * It has already been tokenized
 *
 * \note Error messages are printed by this function!!
 *       Do not duplicate error messages upon error return
 *
 * node-identifier        = [prefix ":"] identifier
 *
 * \param pcb the parser control block in progress|
 * \return the status result
 *********************************************************************/
static status_t parse_node_identifier (xpath_pcb_t *pcb)
{
    const xmlChar *nodename = NULL;
    const xmlChar *prefix = NULL;
    xmlns_id_t nsid = 0;

    /* get the next token in the step, node-identifier */
    status_t res = TK_ADV(pcb->tkc);
    if (res != NO_ERR) {
        pcb_print_errormsg(pcb, pcb->tkerr.mod, res);
        return res;
    }

    switch (TK_CUR_TYP(pcb->tkc)) {
    case TK_TT_PERIOD:
        nodename = parse_node_identifier_tk_tt_period( pcb, &res );
        break;

    case TK_TT_MSTRING:
        nodename = parse_node_identifier_tk_tt_string( pcb, &prefix, &nsid, 
                                                      &res );
        break;

        /* fall through to check QName */
    case TK_TT_TSTRING:
        nodename = TK_CUR_VAL(pcb->tkc);
        break;
    default:
        res = ERR_NCX_WRONG_TKTYPE;
        if (pcb->logerrors) {
            ncx_mod_exp_err(pcb->tkc, pcb->tkerr.mod, res, "node identifier");
        }
        break;
    }

    /* FIXME Should this function simply return NO_ERR at the top if
     * pcb->obj is NULL */
    if ( res == NO_ERR && pcb->obj ) {
        res = set_next_objnode( pcb, prefix, nsid, nodename );
    }

    return res;

}  /* parse_node_identifier */


/********************************************************************
* FUNCTION parse_current_fn
* 
* Parse the leafref current-function token sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* current-function-invocation = 'current()'
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_current_fn (xpath_pcb_t *pcb)
{
    status_t     res;

    /* get the function name 'current' */
    res = xpath_parse_token(pcb, TK_TT_TSTRING);
    if (res != NO_ERR) {
        return res;
    }
    if (xml_strcmp(TK_CUR_VAL(pcb->tkc),
                   (const xmlChar *)"current")) {
        res = ERR_NCX_WRONG_VAL;
        if (pcb->logerrors) {
            ncx_mod_exp_err(pcb->tkc, 
                            pcb->tkerr.mod, 
                            res,
                            "current() function");
        }
        return res;
    }

    /* get the left paren '(' */
    res = xpath_parse_token(pcb, TK_TT_LPAREN);
    if (res != NO_ERR) {
        return res;
    }

    /* get the right paren ')' */
    res = xpath_parse_token(pcb, TK_TT_RPAREN);
    if (res != NO_ERR) {
        return res;
    }

    /* assign the context node to the start object node */
    if (pcb->obj) {
        switch (pcb->curmode) {
        case XP_CM_TARGET:
            pcb->targobj = pcb->obj;
            break;
        case XP_CM_ALT:
            pcb->altobj = pcb->obj;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    return NO_ERR;

}  /* parse_current_fn */


/********************************************************************
* FUNCTION parse_path_key_expr
* 
* Parse the leafref *path-key-expr token sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* path-key-expr          = current-function-invocation "/"
*                          rel-path-keyexpr
*
* rel-path-keyexpr       = 1*(".." "/") *(node-identifier "/")
*                          node-identifier
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_path_key_expr (xpath_pcb_t *pcb)
{
    tk_type_t    nexttyp;
    status_t     res;
    boolean      done;

    res = NO_ERR;
    done = FALSE;

    pcb->altobj = NULL;
    pcb->curmode = XP_CM_ALT;

    /* get the current function call 'current()' */
    res = parse_current_fn(pcb);
    if (res != NO_ERR) {
        return res;
    }

    /* get the path separator '/' */
    res = xpath_parse_token(pcb, TK_TT_FSLASH);
    if (res != NO_ERR) {
        return res;
    }

    /* make one loop for each step of the first part of
     * the rel-path-keyexpr; the '../' token pairs
     */
    while (!done) {
        /* get the parent marker '..' */
        res = xpath_parse_token(pcb, TK_TT_RANGESEP);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* get the path separator '/' */
        res = xpath_parse_token(pcb, TK_TT_FSLASH);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* move the context node up one level */
        if (pcb->obj) {
            res = move_up_obj(pcb);
            if (res != NO_ERR) {
                done = TRUE;
                continue;
            }
        }

        /* check the next token;
         * it may be the start of another '../' pair
         */
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp != TK_TT_RANGESEP) {
            done = TRUE;
        } /* else keep going to the next '../' pair */
    }

    if (res != NO_ERR) {
        return res;
    }

    /* get the node-identifier sequence */
    done = FALSE;
    while (!done) {
        res = parse_node_identifier(pcb);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* check the next token;
         * it may be the fwd slash to start another step
         */
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp != TK_TT_FSLASH) {
            done = TRUE;
        } else {
            res = xpath_parse_token(pcb, TK_TT_FSLASH);
            if (res != NO_ERR) {
                done = TRUE;
            }
        }
    }

    return res;

}  /* parse_path_key_expr */


/********************************************************************
* FUNCTION get_key_number
* 
* Get the key number for the specified key
* It has already been tokenized
*
* INPUTS:
*    obj == list object to check
*    keyobj == key leaf object to find 
*
* RETURNS:
*   int32   [0..N-1] key number or -1 if not a key in this obj
*********************************************************************/
static int32
    get_key_number (obj_template_t *obj,
                    obj_template_t *keyobj)
{
    obj_key_t       *key;
    int32            keynum;

    keynum = 0;

    key = obj_first_key(obj);
    while (key) {
        if (key->keyobj == keyobj) {
            return keynum;
        }

        keynum++;
        key = obj_next_key(key);
    }

    return -1;

} /* get_key_number */


/********************************************************************
* FUNCTION parse_path_predicate
* 
* Parse the leafref *path-predicate token sequence
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* path-predicate         = "[" *WSP path-equality-expr *WSP "]"
*
* path-equality-expr     = node-identifier *WSP "=" *WSP path-key-expr
*
* path-key-expr          = current-function-invocation "/"
*                          rel-path-keyexpr
*
* node-identifier        = [prefix ":"] identifier
*
* current-function-invocation = 'current()'
*
* For instance-identifiers (XP_FL_INSTANCEID) the 
* following syntax is used:
*
* path-predicate        = "[" *WSP instpath-equality-expr *WSP "]"
*
* instpath-equality-expr = node-identifier *WSP "=" *WSP quoted-string
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_path_predicate (xpath_pcb_t *pcb)
{
#define MAX_KEYS   64
    tk_type_t              nexttyp;
    status_t               res;
    boolean                done, leaflist;
    uint64                 keyflags, keybit;
    uint32                 keytotal, keycount, loopcount;
    int32                  keynum;

    res = NO_ERR;
    done = FALSE;
    keyflags = 0;
    keytotal = 0;
    keycount = 0;
    loopcount = 0;
    leaflist = FALSE;

    if (pcb->targobj && pcb->targobj->objtype == OBJ_TYP_LIST) {
        keytotal = obj_key_count(pcb->targobj);
        if (keytotal > MAX_KEYS) {
            if (pcb->logerrors && 
                ncx_warning_enabled(ERR_NCX_MAX_KEY_CHECK)) {
                log_warn("\nWarning: Only first %u keys in list '%s'"
                         " can be checked in XPath expression", 
                         MAX_KEYS,
                         obj_get_name(pcb->obj));
            } else if (pcb->objmod != NULL) {
                ncx_inc_warnings(pcb->objmod);
            }
        } else if (keytotal == 0) {
            res = ERR_NCX_INVALID_VALUE;
            log_error("\nError: Key predicates found for list "
                      "'%s' which does not define any keys", 
                      obj_get_name(pcb->targobj));
            return res;
        }
    } else if ((pcb->flags & XP_FL_INSTANCEID) && 
               targ_obj_is_leaflist( pcb ) ) {
        keytotal = 1;
    }
    
    /* make one loop for each step */
    while (!done) {

        leaflist = FALSE;

        /* only used if pcb->obj set and validating expr */
        loopcount++;

        /* get the left bracket '[' */
        res = xpath_parse_token(pcb, TK_TT_LBRACK);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        pcb->varobj = NULL;
        pcb->curmode = XP_CM_KEYVAR;

        /* get the node-identifier next */
        res = parse_node_identifier(pcb);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* validate the variable object foo in [foo = expr] */
        if (pcb->obj && pcb->varobj) {
            if ((pcb->flags & XP_FL_INSTANCEID) &&
                pcb->varobj->objtype == OBJ_TYP_LEAF_LIST) {
                if (!(pcb->targobj && pcb->targobj == pcb->varobj)) {
                    res = ERR_NCX_WRONG_INDEX_TYPE;
                    if (pcb->logerrors) {
                        log_error("\nError: wrong index type '%s' for "
                                  "leaf-list '%s'",
                                  obj_get_typestr(pcb->varobj),
                                  obj_get_name(pcb->targobj));
                        ncx_print_errormsg(pcb->tkc, NULL, res);
                    }
                } else {
                    leaflist = TRUE;
                }
            } else if (pcb->varobj->objtype != OBJ_TYP_LEAF) {
                res = ERR_NCX_WRONG_TYPE;
                if (pcb->logerrors) {
                    log_error("\nError: path predicate found is %s '%s'",
                              obj_get_typestr(pcb->varobj),
                              obj_get_name(pcb->varobj));
                    ncx_mod_exp_err(pcb->tkc, 
                                    pcb->objmod, 
                                    res, 
                                    "leaf");
                }
                done = TRUE;
                continue;
            }

            if (leaflist) {
                keynum = 0;
            } else {
                keynum = get_key_number(pcb->targobj, pcb->varobj);
            }
            if (keynum == -1) {
                SET_ERROR(ERR_INTERNAL_VAL);
            } else if (keynum < MAX_KEYS) {
                keybit = (uint64)(1 << keynum);
                if (keyflags & keybit) {
                    res = ERR_NCX_EXTRA_PARM;
                    if (pcb->logerrors) {
                        log_error("\nError: key '%s' already specified "
                                  "in XPath expression for object '%s'",
                                  obj_get_name(pcb->varobj),
                                  obj_get_name(pcb->targobj));
                        ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                    }
                } else {
                    keycount++;
                    keyflags |= keybit;
                }
            } else {
                if (pcb->logerrors &&
                    ncx_warning_enabled(ERR_NCX_MAX_KEY_CHECK)) {
                    log_warn("\nWarning: Key '%s' skipped "
                             "in validation test",
                             obj_get_name(pcb->varobj));
                } else if (pcb->objmod != NULL) {
                    ncx_inc_warnings(pcb->objmod);
                }
            }
        } 

        /* get the equals sign '=' */
        res = xpath_parse_token(pcb, TK_TT_EQUAL);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        if (pcb->flags & 
            (XP_FL_INSTANCEID | XP_FL_SCHEMA_INSTANCEID)) {

            pcb->altobj = NULL;

            /* parse literal */
            res = TK_ADV(pcb->tkc);
            if (res == NO_ERR) {
                if (!(TK_CUR_TYP(pcb->tkc) == TK_TT_QSTRING ||
                      TK_CUR_TYP(pcb->tkc) == TK_TT_SQSTRING)) {
                    res = ERR_NCX_WRONG_TKTYPE;
                }
            }

            if (res != NO_ERR) {
                if (pcb->logerrors) {
                    log_error("\nError: invalid predicate in "
                              "expression '%s'", pcb->exprstr);
                    ncx_print_errormsg(pcb->tkc, NULL, res);
                }
                done = TRUE;
                continue;
            }
        } else {
            /* get the path-key-expr next */
            res = parse_path_key_expr(pcb);
            if (res != NO_ERR) {
                done = TRUE;
                continue;
            }
        }

        /* check the object specified in the key expression */
        if (pcb->obj && pcb->altobj) {
            if (pcb->altobj->objtype != OBJ_TYP_LEAF) {
                res = ERR_NCX_WRONG_TYPE;
                if (pcb->logerrors) {
                    log_error("\nError: path target found is %s '%s'",
                              obj_get_typestr(pcb->altobj),
                              obj_get_name(pcb->altobj));
                    ncx_mod_exp_err(pcb->tkc, 
                                    pcb->objmod, 
                                    res, 
                                    "leaf");
                }
                done = TRUE;
                continue;
            } else {
                /* check the predicate for errors */
                if (pcb->altobj == pcb->obj) {
                    res = ERR_NCX_DEF_LOOP;
                    if (pcb->logerrors) {
                        log_error("\nError: path target '%s' is set to "
                                  "the target object",
                                  obj_get_name(pcb->altobj));
                        ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                    }
                    done = TRUE;
                    continue;
                } else if (pcb->altobj == pcb->varobj) {
                    res = ERR_NCX_DEF_LOOP;
                    if (pcb->logerrors) {
                        log_error("\nError: path target '%s' is set to "
                                  "the key leaf object",
                                  obj_get_name(pcb->altobj));
                        ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                    }
                    done = TRUE;
                    continue;
                }
            }
        }

        /* get the right bracket ']' */
        res = xpath_parse_token(pcb, TK_TT_RBRACK);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        pcb->curmode = XP_CM_TARGET;

        /* check the next token;
         * it may be the start of another path-predicate '['
         */
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp != TK_TT_LBRACK) {
            done = TRUE;
        } /* else keep going to the next path-predicate */
    }

    if (pcb->obj) {
        if (loopcount != keytotal) {
            if (keycount < keytotal) {
                if (pcb->flags & XP_FL_SCHEMA_INSTANCEID ||
                    pcb->source == XP_SRC_LEAFREF) {
                    /* schema-instance allowed to skip keys 
                     * leafref path-expr allowed to skip keys
                     */
                    ;   
                } else {
                    /* regular instance-identifier must have all keys */
                    res = ERR_NCX_MISSING_INDEX;
                    if (pcb->logerrors) {
                        log_error("\nError: missing key components in"
                                  " XPath expression for list '%s'",
                                  obj_get_name(pcb->targobj));
                        ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                    }
                }
            } else {
                res = ERR_NCX_EXTRA_VAL_INST;
                if (pcb->logerrors) {
                    log_error("\nError: extra key components in"
                              " XPath expression for list '%s'",
                              obj_get_name(pcb->targobj));
                    ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                }
            }
        }
    }

    return res;

}  /* parse_path_predicate */


/********************************************************************
* FUNCTION parse_absolute_path
* 
* Parse the leafref path-arg string
* It has already been tokenized
*
* The exact syntax below is not followed!!!
* The first '/' to start the absolute path
* is allowed to be omitted in XPath, so
* allow that here as well
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* absolute-path          = 1*("/" (node-identifier *path-predicate))
*
* path-predicate         = "[" *WSP path-equality-expr *WSP "]"
*
* path-equality-expr     = node-identifier *WSP "=" *WSP path-key-expr
*
* path-key-expr          = current-function-invocation "/"
*                          rel-path-keyexpr
*
* node-identifier        = [prefix ":"] identifier
*
* current-function-invocation = 'current()'
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_absolute_path (xpath_pcb_t *pcb)
{
    tk_type_t    nexttyp;
    status_t     res;
    boolean      done, first;

    res = NO_ERR;
    done = FALSE;
    first = TRUE;

    /* make one loop for each step */
    while (!done) {
        /* get  the first token in the step, '/' */
        if (first) {
            /* allow the first '/' to be omitted */
            if (tk_next_typ(pcb->tkc) == TK_TT_FSLASH) {
                res = xpath_parse_token(pcb, TK_TT_FSLASH);
            }
            first = FALSE;
        } else {
            /* get  the first token in the step, '/' */
            res = xpath_parse_token(pcb, TK_TT_FSLASH);
        }
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        if (pcb->flags & XP_FL_SCHEMA_INSTANCEID &&
            tk_next_typ(pcb->tkc) == TK_TT_NONE) {
            /* corner-case: a schema-instance-string
             * is allowed to be a single forward slash
             * to indicate the entire document
             */
            pcb->targobj = pcb->docroot;
            done = TRUE;
            continue;
        }

        /* get the node-identifier next */
        res = parse_node_identifier(pcb);
        if (res != NO_ERR) {
            done = TRUE;
            continue;
        }

        /* check the next token;
         * it may be the start of a path-predicate '['
         * or the start of another step '/'
         */
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp == TK_TT_LBRACK) {
            res = parse_path_predicate(pcb);
            if (res != NO_ERR) {
                done = TRUE;
                continue;
            }

            nexttyp = tk_next_typ(pcb->tkc);
            if (nexttyp != TK_TT_FSLASH) {
                done = TRUE;
            }
        } else if (nexttyp != TK_TT_FSLASH) {
            done = TRUE;
        } /* else keep going to the next step */
    }

    /* check that the string ended properly */
    if (res == NO_ERR) {
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp != TK_TT_NONE) {
            res = ERR_NCX_INVALID_TOKEN;
            if (pcb->logerrors) {
                log_error("\nError: wrong token at end of absolute-path '%s'",
                          tk_get_token_name(nexttyp));
                ncx_print_errormsg(pcb->tkc, 
                                   pcb->tkerr.mod, 
                                   res);
            }
        }
    }

    return res;

}  /* parse_absolute_path */


/********************************************************************
* FUNCTION parse_relative_path
* 
* Parse the leafref relative-path string
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* relative-path          = descendant-path /
*                          (".." "/"
*                          *relative-path)
*
* descendant-path        = node-identifier *path-predicate
*                          absolute-path
*
* Real implementation:
*
* relative-path          = *(".." "/") descendant-path
*
* descendant-path        = node-identifier *path-predicate
*                          [absolute-path]
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_relative_path (xpath_pcb_t *pcb)
{
    tk_type_t    nexttyp;
    status_t     res;

    res = NO_ERR;

    /* check the next token;
     * it may be the start of a '../' pair or a
     * descendant-path (node-identifier)
     */
    nexttyp = tk_next_typ(pcb->tkc);
    while (nexttyp == TK_TT_RANGESEP && res == NO_ERR) {
        res = xpath_parse_token(pcb, TK_TT_RANGESEP);
        if (res == NO_ERR) {
            res = xpath_parse_token(pcb, TK_TT_FSLASH);
            if (res == NO_ERR) {
                if (pcb->obj) {
                    res = move_up_obj(pcb);
                }

                /* check the next token;
                 * it may be the start of another ../ pair
                 * or a node identifier
                 */
                nexttyp = tk_next_typ(pcb->tkc);
            }
        }
    }

    if (res == NO_ERR) {
        /* expect a node identifier first */
        res = parse_node_identifier(pcb);
        if (res == NO_ERR) {
            /* check the next token;
             * it may be the start of a path-predicate '['
             * or the start of another step '/'
             */
            nexttyp = tk_next_typ(pcb->tkc);
            if (nexttyp == TK_TT_LBRACK) {
                res = parse_path_predicate(pcb);
            }

            if (res == NO_ERR) {
                /* check the next token;
                 * it may be the start of another step '/'
                 */
                nexttyp = tk_next_typ(pcb->tkc);
                if (nexttyp == TK_TT_FSLASH) {
                    res = parse_absolute_path(pcb);
                }
            }
        }
    }
            
    return res;

}  /* parse_relative_path */


/********************************************************************
* FUNCTION parse_path_arg
* 
* Parse the leafref path-arg string
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* path-arg-str           = < a string which matches the rule
*                            path-arg >
*
* path-arg               = absolute-path / relative-path
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_path_arg (xpath_pcb_t *pcb)
{
    tk_type_t  nexttyp;

    nexttyp = tk_next_typ(pcb->tkc);
    if (nexttyp == TK_TT_FSLASH) {
        pcb->flags |= XP_FL_ABSPATH;
        return parse_absolute_path(pcb);
    } else if (pcb->obj != NULL && pcb->obj == pcb->docroot) {
        pcb->flags |= XP_FL_ABSPATH;
        return parse_absolute_path(pcb);
    } else {
        return parse_relative_path(pcb);
    }

}  /* parse_path_arg */

/********************************************************************
* FUNCTION parse_keyexpr
* 
* Parse the key attribute expression
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* keyexprr          = 1*path-predicate
*
* path-predicate         = "[" *WSP path-equality-expr *WSP "]"
*
* path-equality-expr     = node-identifier *WSP "=" *WSP quoted-string
*
* INPUTS:
*    pcb == parser control block in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    parse_keyexpr (xpath_pcb_t *pcb)
{
    tk_type_t    nexttyp;
    status_t     res;
    boolean      done;

    res = NO_ERR;
    done = FALSE;

    /* make one loop for each step */
    while (!done) {
        /* check the next token;
         * it may be the start of a path-predicate '['
         * or the start of another step '/'
         */
        nexttyp = tk_next_typ(pcb->tkc);
        if (nexttyp == TK_TT_LBRACK) {
            res = parse_path_predicate(pcb);
            if (res != NO_ERR) {
                done = TRUE;
            }
        } else if (nexttyp == TK_TT_NONE) {
            done = TRUE;
            continue;
        } else {
            res = ERR_NCX_INVALID_VALUE;
            done = TRUE;
            if (pcb->logerrors) {
                log_error("\nError: wrong token in key-expr '%s'",
                          pcb->exprstr);
                ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, res);
            }
        }
    }

    return res;

}  /* parse_keyexpr */


/************    E X T E R N A L   F U N C T I O N S    ************/


/********************************************************************
* FUNCTION xpath_yang_parse_path
* 
* Parse the leafref path as a leafref path
*
* DOES NOT VALIDATE PATH NODES USED IN THIS PHASE
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used on all objects, even in groupings
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*    tkc == parent token chain (may be NULL)
*    mod == module in progress
*    obj == context node
*    source == context for this expression
*              XP_SRC_LEAFREF or XP_SRC_INSTANCEID
*    pcb == initialized xpath parser control block
*           for the leafref path; use xpath_new_pcb
*           to initialize before calling this fn.
*           The pcb->exprstr field must be set
*
* OUTPUTS:
*   pcb->tkc is filled and then validated for well-formed
*   leafref or instance-identifier syntax
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_parse_path (tk_chain_t *tkc,
                           ncx_module_t *mod,
                           xpath_source_t source,
                           xpath_pcb_t *pcb)
{
    return xpath_yang_parse_path_ex(tkc, mod, source, pcb, TRUE);

}  /* xpath_yang_parse_path */


/********************************************************************
* FUNCTION xpath_yang_parse_path_ex
* 
* Parse the leafref path as a leafref path
*
* DOES NOT VALIDATE PATH NODES USED IN THIS PHASE
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used on all objects, even in groupings
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*    tkc == parent token chain (may be NULL)
*    mod == module in progress
*    obj == context node
*    source == context for this expression
*              XP_SRC_LEAFREF or XP_SRC_INSTANCEID
*    pcb == initialized xpath parser control block
*           for the leafref path; use xpath_new_pcb
*           to initialize before calling this fn.
*           The pcb->exprstr field must be set
*    logerrors == TRUE to log errors; 
*              == FALSE to suppress error messages
* OUTPUTS:
*   pcb->tkc is filled and then validated for well-formed
*   leafref or instance-identifier syntax
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_parse_path_ex (tk_chain_t *tkc,
                              ncx_module_t *mod,
                              xpath_source_t source,
                              xpath_pcb_t *pcb,
                              boolean logerrors)
{
    status_t       res;
    uint32         linenum, linepos;

#ifdef DEBUG
    if (!pcb || !pcb->exprstr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    switch (source) {
    case XP_SRC_LEAFREF:
    case XP_SRC_INSTANCEID:
    case XP_SRC_SCHEMA_INSTANCEID:
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    pcb->curmode = XP_CM_TARGET;
    pcb->logerrors = logerrors;
    if (tkc && tkc->cur) {
        linenum = TK_CUR_LNUM(tkc);
        linepos = TK_CUR_LPOS(tkc);
    } else {
        linenum = pcb->tkerr.linenum;
        linepos = pcb->tkerr.linepos;
    }

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(mod, 
                                            pcb->exprstr, 
                                            linenum,
                                            linepos,
                                            &res);
        if (!pcb->tkc || res != NO_ERR) {
            if (pcb->logerrors) {
                log_error("\nError: Invalid path string '%s'",
                          pcb->exprstr);
                ncx_print_errormsg(tkc, mod, res);
            }
            return res;
        }
    }

    /* the module that contains the leafref is the one
     * that will always be used to resolve prefixes
     * within the XPath expression
     */
    pcb->tkerr.mod = mod;
    pcb->source = source;
    if (source == XP_SRC_INSTANCEID) {
        pcb->flags |= XP_FL_INSTANCEID;
    } else if (source == XP_SRC_SCHEMA_INSTANCEID) {
        pcb->flags |= XP_FL_SCHEMA_INSTANCEID;
    }
        
    /* since the pcb->obj is not set, this validation
     * phase will skip identifier tests, predicate tests
     * and completeness tests
     */
    pcb->parseres = parse_path_arg(pcb);

    /* the expression will not be processed further if the
     * parseres is other than NO_ERR
     */
    return pcb->parseres;

}  /* xpath_yang_parse_path_ex */


/********************************************************************
* FUNCTION xpath_yang_validate_path
* 
* Validate the previously parsed leafref path
*   - QNames are valid
*   - object structure referenced is valid
*   - objects are all 'config true'
*   - target object is a leaf
*   - leafref represents a single instance
*
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used only on cooked (real) objects
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*        == NULL if no object in progress
*    obj == object using the leafref data type
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    schemainst == TRUE if ncx:schema-instance string
*               == FALSE to use the pcb->source field 
*                  to determine the exact parse mode
*    leafobj == address of the return target object
*
* OUTPUTS:
*   *leafobj == the target leaf found by parsing the path (NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_validate_path (ncx_module_t *mod,
                              obj_template_t *obj,
                              xpath_pcb_t *pcb,
                              boolean schemainst,
                              obj_template_t **leafobj)
{
    return xpath_yang_validate_path_ex(mod, obj, pcb, schemainst, 
                                       leafobj, TRUE);

}  /* xpath_yang_validate_path */


/********************************************************************
* FUNCTION xpath_yang_validate_path_ex
* 
* Validate the previously parsed leafref path
*   - QNames are valid
*   - object structure referenced is valid
*   - objects are all 'config true'
*   - target object is a leaf
*   - leafref represents a single instance
* 
* A 2-pass validation is used in case the path expression
* is defined within a grouping.  This pass is
* used only on cooked (real) objects
*
* Called after all 'uses' and 'augment' expansion
* so validation against cooked object tree can be done
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    mod == module containing the 'obj' (in progress)
*        == NULL if no object in progress
*    obj == object using the leafref data type
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    schemainst == TRUE if ncx:schema-instance string
*               == FALSE to use the pcb->source field 
*                  to determine the exact parse mode
*    leafobj == address of the return target object
*    logerrors == TRUE to log errors, FALSE to suppress errors
*              val_parse uses FALSE if basetype == NCX_BT_UNION
*
* OUTPUTS:
*   *leafobj == the target leaf found by parsing the path (NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_validate_path_ex (ncx_module_t *mod,
                                 obj_template_t *obj,
                                 xpath_pcb_t *pcb,
                                 boolean schemainst,
                                 obj_template_t **leafobj,
                                 boolean logerrors)
{
    status_t          res;
    boolean           doerror;
    ncx_btype_t       btyp;

#ifdef DEBUG
    if (!obj || !pcb || !leafobj || !pcb->exprstr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!pcb->tkc) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    if (schemainst) {
        /* relax errors for missing keys in instance-identifier */
        pcb->flags |= XP_FL_SCHEMA_INSTANCEID;
    }

    pcb->logerrors = logerrors;
    *leafobj = NULL;

    if (pcb->parseres != NO_ERR) {
        /* errors already reported, skip this one */
        return NO_ERR;
    }

    pcb->docroot = ncx_get_gen_root();
    if (!pcb->docroot) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    tk_reset_chain(pcb->tkc);
    pcb->objmod = mod;
    pcb->obj = obj;
    pcb->targobj = NULL;
    pcb->altobj = NULL;
    pcb->varobj = NULL;
    pcb->curmode = XP_CM_TARGET;

    /* validate the XPath expression against the 
     * full cooked object tree
     */
    pcb->validateres = parse_path_arg(pcb);

    /* check leafref is config but target is not */
    if (pcb->validateres == NO_ERR && pcb->targobj) {
        /* return this object even if errors
         * it will get ignored unless return NO_ERR
         */
        *leafobj = pcb->targobj;

        /* make sure the config vs. non-config rules are followed
         * if obj is config, it can point at only config targobj
         * if obj not config, it can point at any targobj node
         */
        if (obj_get_config_flag(obj) &&
            !obj_get_config_flag(pcb->targobj)) {

            doerror = TRUE;

            btyp = obj_get_basetype(obj);

            /* only some instance-identifier and leafref 
             * objects will ever return TRUE 
             */
            if ((btyp == NCX_BT_INSTANCE_ID || 
                 btyp == NCX_BT_LEAFREF) && 
                !typ_get_constrained(obj_get_ctypdef(obj))) {
                doerror = FALSE;
            }

            /* yangcli_util/get_instanceid_parm sets the
             * object field to the config root instead
             * of the leafref object, so this hack is used
             * to suppress the error intended for leafrefs
             */
            if (obj_is_root(obj)) {
                doerror = FALSE;
            }

            if (doerror) {
                res = ERR_NCX_NOT_CONFIG;
                if (pcb->logerrors) {
                    log_error("\nError: XPath target '%s' for leafref '%s'"
                              " must be a config object",
                              obj_get_name(pcb->targobj),
                              obj_get_name(obj));
                    ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                }
                pcb->validateres = res;
            }
        }

        /* check for a leaf, then if that is OK */
        if (pcb->source == XP_SRC_LEAFREF) {
            if (!obj_is_leafy(pcb->targobj)) {
                res = ERR_NCX_INVALID_VALUE;
                if (pcb->logerrors) {
                    log_error("\nError: invalid path target %s '%s'",
                              obj_get_typestr(pcb->targobj),
                              obj_get_name(pcb->targobj));
                    ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
                }
                pcb->validateres = res;
            }
        }

        /* this test is probably not worth doing,
         * but check for the loop anyway
         * special case: allow docroot as return target
         */
        if (pcb->targobj == pcb->obj &&
            pcb->targobj != pcb->docroot) {
            res = ERR_NCX_DEF_LOOP;
            if (pcb->logerrors) {
                log_error("\nError: path target '%s' is set to "
                          "the target object",
                          obj_get_name(pcb->targobj));
                ncx_print_errormsg(pcb->tkc, pcb->objmod, res);
            }
            pcb->validateres = res;
        }

        /**** NEED TO CHECK THE CORNER CASE WHERE A LEAFREF
         **** AND/OR INSTANCE-IDENTIFIER COMBOS CAUSE A LOOP
         **** THAT WILL PREVENT AGENT VALIDATION FROM COMPLETING
         ****/
    }

    return pcb->validateres;

}  /* xpath_yang_validate_path */


/********************************************************************
* FUNCTION xpath_yang_validate_xmlpath
* 
* Validate an instance-identifier expression
* within an XML PDU context
*
* INPUTS:
*    reader == XML reader to use
*    pcb == initialized XPath parser control block
*           with a possibly unchecked pcb->exprstr.
*           This function will  call tk_tokenize_xpath_string
*           if it has not already been called.
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*                !!! use FALSE unless DEBUG mode !!!
*    targobj == address of return target object
*     
* OUTPUTS:
*   *targobj is set to the object that this instance-identifier
*    references, if NO_ERR
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_validate_xmlpath (xmlTextReaderPtr reader,
                                 xpath_pcb_t *pcb,
                                 obj_template_t *pathobj,
                                 boolean logerrors,
                                 obj_template_t **targobj)
{
    status_t  res;

#ifdef DEBUG
    if (!reader || !pcb || !targobj || !pcb->exprstr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    *targobj = NULL;
    pcb->logerrors = logerrors;

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(NULL, 
                                            pcb->exprstr, 
                                            0, 
                                            0, 
                                            &res);
    }

    if (!pcb->tkc || res != NO_ERR) {
        if (pcb->logerrors) {
            log_error("\nError: Invalid path string '%s'",
                      pcb->exprstr);
        }
        pcb->parseres = res;
        return res;
    }

    pcb->docroot = ncx_get_gen_root();
    if (!pcb->docroot) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (pathobj && obj_is_schema_instance_string(pathobj)) {
        pcb->flags |= XP_FL_SCHEMA_INSTANCEID;
    } else {
        pcb->flags |= XP_FL_INSTANCEID;
    }

    pcb->obj = pcb->docroot;
    pcb->reader = reader;

    pcb->source = XP_SRC_XML;
    pcb->objmod = NULL;
    pcb->val = NULL;
    pcb->val_docroot = NULL;
    pcb->targobj = NULL;
    pcb->altobj = NULL;
    pcb->varobj = NULL;
    pcb->curmode = XP_CM_TARGET;
    pcb->flags |= XP_FL_ABSPATH;

    /* validate the XPath expression against the 
     * full cooked object tree
     */
    pcb->validateres = parse_absolute_path(pcb);

    /* check leafref is config but target is not */
    if (pcb->validateres == NO_ERR && pcb->targobj) {
        *targobj = pcb->targobj;
    }

    pcb->reader = NULL;
    return pcb->validateres;

}  /* xpath_yang_validate_xmlpath */


/********************************************************************
* FUNCTION xpath_yang_validate_xmlkey
* 
* Validate a key XML attribute value given in
* an <edit-config> operation with an 'insert' attribute
* Check that a complete set of predicates is present
* for the specified list of leaf-list
*
* INPUTS:
*    reader == XML reader to use
*    pcb == initialized XPath parser control block
*           with a possibly unchecked pcb->exprstr.
*           This function will  call tk_tokenize_xpath_string
*           if it has not already been called.
*    obj == list or leaf-list object associated with
*           the pcb->exprstr predicate expression
*           (MAY be NULL if first-pass parsing and
*           object is not known yet -- parsed in XML attribute)
*    logerrors == TRUE if log_error and ncx_print_errormsg
*                  should be used to log XPath errors and warnings
*                 FALSE if internal error info should be recorded
*                 in the xpath_result_t struct instead
*                !!! use FALSE unless DEBUG mode !!!
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_validate_xmlkey (xmlTextReaderPtr reader,
                                xpath_pcb_t *pcb,
                                obj_template_t *obj,
                                boolean logerrors)
{
    status_t  res;

#ifdef DEBUG
    if (!reader || !pcb || !pcb->exprstr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    pcb->logerrors = logerrors;

    if (pcb->tkc) {
        tk_reset_chain(pcb->tkc);
    } else {
        pcb->tkc = tk_tokenize_xpath_string(NULL, 
                                            pcb->exprstr, 
                                            0, 
                                            0, 
                                            &res);
    }

    if (!pcb->tkc || res != NO_ERR) {
        if (pcb->logerrors) {
            log_error("\nError: Invalid path string '%s'",
                      pcb->exprstr);
        }
        pcb->parseres = res;
        return res;
    }

    pcb->docroot = ncx_get_gen_root();
    if (!pcb->docroot) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    pcb->obj = obj;
    pcb->reader = reader;
    pcb->flags = XP_FL_INSTANCEID;
    pcb->source = XP_SRC_XML;
    pcb->objmod = NULL;
    pcb->val = NULL;
    pcb->val_docroot = NULL;
    pcb->targobj = obj;
    pcb->altobj = NULL;
    pcb->varobj = NULL;
    pcb->curmode = XP_CM_TARGET;

    /* validate the XPath expression against the 
     * full cooked object tree
     */
    pcb->validateres = parse_keyexpr(pcb);

    pcb->reader = NULL;

    return pcb->validateres;

}  /* xpath_yang_validate_xmlkey */


/********************************************************************
* FUNCTION xpath_yang_make_instanceid_val
* 
* Make a value subtree out of an instance-identifier
* Used by yangcli to send PDUs from CLI target parameters
*
* The XPath pcb must be previously parsed and found valid
* It must be an instance-identifier value, 
* not a leafref path
*
* INPUTS:
*    pcb == the leafref parser control block, possibly
*           cloned from from the typdef
*    retres == address of return status (may be NULL)
*    deepest == address of return deepest node created (may be NULL)
*
* OUTPUTS:
*   if (non-NULL)
*       *retres == return status
*       *deepest == pointer to end of instance-id chain node
* RETURNS:
*   malloced value subtree representing the instance-identifier
*   in internal val_value_t data structures
*********************************************************************/
val_value_t *
    xpath_yang_make_instanceid_val (xpath_pcb_t *pcb,
                                    status_t *retres,
                                    val_value_t **deepest)
{
    val_value_t          *childval, *top, *curtop;
    obj_template_t *curobj, *childobj;
    const xmlChar        *objprefix, *objname, *modname;
    status_t              res;
    boolean               done, done2;

#ifdef DEBUG
    if (!pcb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (!pcb->tkc) {
        if (retres) {
            *retres = SET_ERROR(ERR_INTERNAL_VAL);
        }
        return NULL;
    }
#endif

    if (retres) {
        *retres = NO_ERR;
    }
    if (deepest) {
        *deepest = NULL;
    }

    top = NULL;
    curtop = NULL;

    if (pcb->parseres != NO_ERR) {
        /* errors already reported, skip this one */
        if (retres) {
            *retres = pcb->parseres;
        }
        return NULL;
    }

    /* get first token */
    tk_reset_chain(pcb->tkc);
    res = TK_ADV(pcb->tkc);
    if (res != NO_ERR) {
        if (retres) {
            *retres = res;
        }
        return NULL;
    }

    /* get the start object */
    if (TK_CUR_TYP(pcb->tkc) == TK_TT_FSLASH) {
        /* absolute path, use objroot to start */
        curobj = pcb->docroot;
        res = TK_ADV(pcb->tkc);

        if (res == ERR_NCX_EOF && 
            (pcb->flags & XP_FL_SCHEMA_INSTANCEID)) {
            /* return NO_ERR and NULL to indicate that the
             * docroot is requested
             */
            return NULL;
        }

        if (res != NO_ERR) {
            if (retres) {
                *retres = res;
            }
            return NULL;
        }
    } else {
        /* relative path, use context object to start */
        curobj = pcb->obj;
    }
    if (!curobj) {
        SET_ERROR(ERR_INTERNAL_VAL);
        if (retres) {
            *retres = ERR_INTERNAL_VAL;
        }
        return NULL;
    }

    /* get all the path steps */
    res = NO_ERR;
    done = FALSE;
    while (!done && res == NO_ERR) {
        /* this is expected to be a QName or LCName identifier */
        objprefix = TK_CUR_MOD(pcb->tkc);
        if (objprefix) {
            modname = xmlns_get_module
                (xmlns_find_ns_by_prefix(objprefix));
        } else {
            modname = NULL;
        }
        objname = TK_CUR_VAL(pcb->tkc);

        /* find the child node and add it to the curtop */
        if (obj_is_root(curobj)) {
            if (modname) {
                childobj = 
                    ncx_find_object(ncx_find_module(modname, NULL), 
                                    objname);
            } else {
                childobj = ncx_find_any_object(objname);
            }
        } else {
            childobj = obj_find_child(curobj, modname, objname);
        }
        if (!childobj) {
            res = ERR_NCX_DEF_NOT_FOUND;
            if (retres) {
                *retres = res;
            }
            continue;
        }

        childval = val_new_value();
        if (!childval) {
            res = ERR_INTERNAL_MEM;
            if (retres) {
                *retres = res;
            }
            continue;
        }

        /* if this is the last node, then treat it as a select
         * node since no value will be set; force type empty
         */
        if (tk_next_typ(pcb->tkc) == TK_TT_NONE &&
            obj_get_datadefQ(childobj) == NULL) {
            /* terminal leaf or leaf-list, create an empty node */
            const xmlChar *name = obj_get_name(childobj);

            val_init_from_template(childval,
                                   ncx_get_gen_empty());
            val_set_name(childval, name, xml_strlen(name));
            val_change_nsid(childval, obj_get_nsid(childobj));
        } else {
            val_init_from_template(childval, childobj);
        }

        if (curtop) {
            val_add_child(childval, curtop);
        } else {
            top = childval;
        }
        curtop = childval;
        curobj = childobj;
        if (deepest) {
            *deepest = curtop;
        }

        /* move on to the next token */
        res = TK_ADV(pcb->tkc);
        if (res != NO_ERR) {
            /* reached end of the line at an OK point */
            res = NO_ERR;
            done = TRUE;
            continue;
        }

        switch (TK_CUR_TYP(pcb->tkc)) {
        case TK_TT_FSLASH:
            /* normal identifier expected next, go back to start */
            res = TK_ADV(pcb->tkc);
            continue;
        case TK_TT_LBRACK:
            /* predicate expected next, keep going */
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            continue;
        }

        done2 = FALSE;
        while (!done2 && res == NO_ERR) {
            /* got a start of predicate, get the key leaf name */
            res = TK_ADV(pcb->tkc);
            if (res != NO_ERR) {
                continue;
            }

            /* this is expected to be a QName or LCName identifier */
            objprefix = TK_CUR_MOD(pcb->tkc);
            if (objprefix) {
                modname = xmlns_get_module
                    (xmlns_find_ns_by_prefix(objprefix));
            } else {
                modname = NULL;
            }
            objname = TK_CUR_VAL(pcb->tkc);

            /* find the child node and add it to the curtop */
            childobj = obj_find_child(curobj, modname, objname);
            if (!childobj) {
                res = ERR_NCX_DEF_NOT_FOUND;
                continue;
            }
            childval = val_new_value();
            if (!childval) {
                res = ERR_INTERNAL_MEM;
                continue;
            }
            val_init_from_template(childval, childobj);
            val_add_child(childval, curtop);

            /* get the = sign */
            res = TK_ADV(pcb->tkc);
            if (res != NO_ERR) {
                continue;
            }
            if (TK_CUR_TYP(pcb->tkc) != TK_TT_EQUAL) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
                continue;
            }

            /* get the key leaf value */
            res = TK_ADV(pcb->tkc);
            if (res != NO_ERR) {
                continue;
            }
            if (!TK_CUR_STR(pcb->tkc)) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
                continue;
            }

            /* set the new leaf with the value */
            res = val_set_simval(childval,
                                 obj_get_typdef(childobj),
                                 obj_get_nsid(childobj),
                                 obj_get_name(childobj),
                                 TK_CUR_VAL(pcb->tkc));
            if (res != NO_ERR) {
                continue;
            }

            /* get the closing predicate bracket */
            res = TK_ADV(pcb->tkc);
            if (res != NO_ERR) {
                continue;
            }
            if (TK_CUR_TYP(pcb->tkc) != TK_TT_RBRACK) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
                continue;
            }

            /* check any more predicates or path steps */
            res = TK_ADV(pcb->tkc);
            if (res != NO_ERR) {
                /* no more steps, stopped at an OK spot */
                done = TRUE;
                done2 = TRUE;
                res = NO_ERR;
                continue;
            }

            switch (TK_CUR_TYP(pcb->tkc)) {
            case TK_TT_LBRACK:
                /* get another predicate */
                break;
            case TK_TT_FSLASH:
                /* done with predicates for now 
                 * setup the next path step
                 */
                res = TK_ADV(pcb->tkc);
                if (res != NO_ERR) {
                    continue;
                }
                done2 = TRUE;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }


    /* cleanup and exit */
    if (res != NO_ERR && top) {
        val_free_value(top);
        top = NULL;
    }

    if (LOGDEBUG2 && top) {
        log_debug2("\nval_inst for expr '%s'\n", 
                   pcb->exprstr);
        val_dump_value(top, NCX_DEF_INDENT);
    }

    if (retres != NULL) {
        *retres = res;
    }

    return top;

}  /* xpath_yang_make_instanceid_val */


/********************************************************************
* FUNCTION xpath_yang_get_namespaces
* 
* Get the namespace URI IDs used in the specified
* XPath expression;
* 
* usually an instance-identifier or schema-instance node
* but this function simply reports all the TK_TT_MSTRING
* tokens that have an nsid set
*
* The XPath pcb must be previously parsed and found valid
*
* INPUTS:
*    pcb == the XPath parser control block to use
*    nsid_array == address of return array of xmlns_id_t
*    max_nsids == number of NSIDs that can be held
*    num_nsids == address of return number of NSIDs
*                 written to the buffer. No duplicates
*                 will be present in the buffer
*
* OUTPUTS:
*    nsid_array[0..*num_nsids] == returned NSIDs used
*       in the XPath expression
*    *num_nsids == number of NSIDs written to the array
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_yang_get_namespaces (const xpath_pcb_t *pcb,
                               xmlns_id_t *nsid_array,
                               uint32 max_nsids,
                               uint32 *num_nsids)
{
    const tk_token_t     *tk;
    boolean               done, found;
    uint32                i, next;
    xmlns_id_t            cur_nsid;
    status_t              res;

#ifdef DEBUG
    if (!pcb || !nsid_array || !num_nsids) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!pcb->tkc) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    if (max_nsids == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    if (pcb->parseres != NO_ERR) {
        return  pcb->parseres;
    }

    res = NO_ERR;
    next = 0;
    *num_nsids = 0;

    done = FALSE;
    for (tk = (const tk_token_t *)dlq_firstEntry(&pcb->tkc->tkQ);
         tk != NULL && !done;
         tk = (const tk_token_t *)dlq_nextEntry(tk)) {

        /* only MSTRING and QVARBIND tokens have relevant NSID fields
         * as used in instance-identifier or schema-instance
         * path syntax
         */
        switch (tk->typ) {
        case TK_TT_MSTRING:
        case TK_TT_QVARBIND:
        case TK_TT_NCNAME_STAR:
            break;
        default:
            continue;
        }

        cur_nsid = tk->nsid;
        if (cur_nsid == 0) {
            /* this token never had an NSID set */
            continue;
        }

        /* check if this NSID already recorded */
        found = FALSE;
        for (i = 0; i < next && !found; i++) {
            if (nsid_array[i] == cur_nsid) {
                found = TRUE;
            }
        }
        if (found) {
            continue;
        }

        /* need to add this entry
         * check if there would be a buffer overflow
         */
        if (next >= max_nsids) {
            res = ERR_BUFF_OVFL;
            done = TRUE;
        } else {
            nsid_array[next++] = cur_nsid;
        }
    }

    *num_nsids = next;

    return res;

}  /* xpath_yang_get_namespaces */


/* END xpath_yang.c */
