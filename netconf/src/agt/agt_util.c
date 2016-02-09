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
/*  FILE: agt_util.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24may06      abb      begun; split out from agt_ncx.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_cap.h"
#include "agt_rpc.h"
#include "agt_rpcerr.h"
#include "agt_tree.h"
#include "agt_util.h"
#include "agt_xpath.h"
#include "agt_val.h"
#include "cfg.h"
#include "dlq.h"
#include "getcb.h"
#include "log.h"
#include "ncx.h"
#include "ncxmod.h"
#include "ncx_feature.h"
#include "ncxconst.h"
#include "obj.h"
#include "op.h"
#include "rpc.h"
#include "rpc_err.h"
#include "ses.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_msg.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "xpath.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

typedef struct agt_keywalker_parms_t_ {
    val_value_t  *lastkey;
    val_value_t  *retkey;
    boolean       usenext;
    boolean       done;
}  agt_keywalker_parms_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/********************************************************************
* FUNCTION is_default
*
* Check if the node is set to the default value
*
* INPUTS:
*    withdef == type of default requested
*    val == node to check
*
* RETURNS:
*    TRUE if set to the default value (by user or agent)
*    FALSE if no default applicable or not set to default
*********************************************************************/
static boolean is_default (ncx_withdefaults_t withdef, val_value_t *val)
{
    boolean retval;

    retval = FALSE;

    switch (withdef) {
    case NCX_WITHDEF_REPORT_ALL:
    case NCX_WITHDEF_REPORT_ALL_TAGGED:
        break;
    case NCX_WITHDEF_TRIM:
        retval = val_is_default(val);
        break;
    case NCX_WITHDEF_EXPLICIT:
        retval = val_set_by_default(val);
        break;
    case NCX_WITHDEF_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    return retval;

} /* is_default */

/********************************************************************
* FUNCTION check_withdef
*
* Check the with-defaults flag
*
* INPUTS:
*    withdef == requested with-defaults action
*    node == value node to check
*
* RETURNS:
*    TRUE if node should be output
*    FALSE if node is filtered out
*********************************************************************/
static boolean check_withdef (ncx_withdefaults_t withdef, val_value_t *node)
{
    const agt_profile_t     *profile;
    boolean                  ret;
    ncx_withdefaults_t       defwithdef;

    /* check if defaults are suppressed */
    ret = TRUE;
    switch (withdef) {
    case NCX_WITHDEF_NONE:
        profile = agt_get_profile();
        defwithdef = profile->agt_defaultStyleEnum;
        if (is_default(defwithdef, node)) {
            ret = FALSE;
        }
        break;
    case NCX_WITHDEF_REPORT_ALL:
    case NCX_WITHDEF_REPORT_ALL_TAGGED:
    case NCX_WITHDEF_TRIM:
    case NCX_WITHDEF_EXPLICIT:
        if (is_default(withdef, node)) {
            ret = FALSE;
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return ret;

} /* check_withdef */


/********************************************************************
 * Make a default leaf and add it to the running config
 *
 * \param parentval parent complex type to add the default as the new last 
 *                  child (could be cfg->root)
 * \param defobj object template for the default leaf
 * \param defstr string containing the default value to use
 * \return status
 *********************************************************************/
static status_t add_default_leaf( val_value_t *parentval, 
        obj_template_t *defobj, const xmlChar *defstr )
{
    val_value_t    *newval;
    status_t res = NO_ERR;

    newval = val_find_child( parentval, obj_get_mod_name(defobj),
                             obj_get_name(defobj) );
    if ( newval ) {
        /* node already exists */
        return NO_ERR;
    }

    newval = val_make_simval_obj(defobj, defstr, &res);
    if ( !newval )
    {
        return ( res == NO_ERR ? ERR_INTERNAL_PTR : res );
    }

    if (res != NO_ERR) {
        val_free_value(newval);
        return res;
    } 
    
    newval->flags |= VAL_FL_DEFSET;
    val_add_child(newval, parentval);

    return res;
}  /* add_default_leaf */

/********************************************************************
 * Get the next expected key value in the ancestor chain
 * val_walker_fn prototype
 *
 * \param val the value node found in descendant search.
 * \param cookie1 a cookie value passed to start of walk.
 * \param cookie2 a cookie value passed to start of walk.
 * \return TRUE if walk should continue, FALSE if walk should terminate.
 */
static boolean get_key_value ( val_value_t *val, void *cookie1, void *cookie2 )
{
    assert( val && "val is NULL" );
    assert( cookie1 && "val is NULL" );
    (void)cookie2;

    agt_keywalker_parms_t *parms = (agt_keywalker_parms_t *)cookie1;

    if (parms->done) {
        return FALSE;
    } else if (parms->lastkey == NULL || parms->usenext) {
        parms->lastkey = val;
        parms->retkey = val;
        parms->done = TRUE;
        return FALSE;
    } else if (parms->lastkey == val) {
        parms->usenext = TRUE;
        return TRUE;
    }

    return TRUE;
}  /* get_key_value */

/********************************************************************
 * Get the filter type
 *
 * \param filter the input filter
 * \return the type of filter
 *
 *********************************************************************/
static op_filtertyp_t get_filter_type( val_value_t* filter )
{
    /* setup the filter parameters */
    val_value_t* filtertype = val_find_meta( filter, 0, NCX_EL_TYPE );
    if ( !filtertype ) {
        /* should not happen; the default is subtree */
        return OP_FILTER_SUBTREE;
    } 
    
    return op_filtertyp_id( VAL_STR(filtertype) );
}

/********************************************************************
 * validate an xpath filter the filter type
 *
 * \param scb session control block
 * \param msg rpc_msg_t in progress
 * \param filter the filter element to use
 * \return the type of filter
 *
 *********************************************************************/
static status_t validate_xpath_filter( ses_cb_t* scb, rpc_msg_t* msg, 
                                       val_value_t* filter )
{
    val_value_t *sel = val_find_meta(filter, 0, NCX_EL_SELECT);
    status_t res = NO_ERR;

    if (!sel || !sel->xpathpcb) {
        res = ERR_NCX_MISSING_ATTRIBUTE;
    } else if (sel->xpathpcb->parseres != NO_ERR) {
        res = sel->xpathpcb->parseres;
    }

    if ( NO_ERR == res ) {
        msg->rpc_filter.op_filtyp = OP_FILTER_XPATH;
        msg->rpc_filter.op_filter = sel;
    }
    else {
        xml_attr_t selattr;
        memset(&selattr, 0x0, sizeof(xml_attr_t));
        selattr.attr_ns = 0;
        selattr.attr_name = NCX_EL_SELECT;

        agt_record_attr_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                               &selattr, NULL, NULL, NCX_NT_VAL, filter );
    }
    return res;
}

/********************************************************************
 * validate a subtree filter the filter type
 *
 * \param msg rpc_msg_t in progress
 * \param filter the filter element to use
 * \return the type of filter
 *
 *********************************************************************/
static status_t validate_subtree_filter( rpc_msg_t* msg,
                                         val_value_t* filter )
{
    msg->rpc_filter.op_filtyp = OP_FILTER_SUBTREE;
    msg->rpc_filter.op_filter = filter;
    return NO_ERR;
}

/************  E X T E R N A L    F U N C T I O N S    **************/

/********************************************************************
* FUNCTION agt_get_cfg_from_parm
*
* Get the cfg_template_t for the config in the target param
*
* INPUTS:
*    parmname == parameter to get from (e.g., target)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    retcfg == address of return cfg pointer
* 
* OUTPUTS:
*   *retcfg is set to the address of the cfg_template_t struct 
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_get_cfg_from_parm (const xmlChar *parmname,
                           rpc_msg_t *msg,
                           xml_node_t *methnode,
                           cfg_template_t  **retcfg)
{
    agt_profile_t     *profile;
    cfg_template_t    *cfg;
    val_value_t       *val;
    val_value_t       *errval;
    const xmlChar     *cfgname;
    status_t           res;

#ifdef DEBUG
    if (!parmname || !msg || !methnode || !retcfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    val = val_find_child(msg->rpc_input, 
                         val_get_mod_name(msg->rpc_input), 
                         parmname);
    if (!val || val->res != NO_ERR) {
        if (!val) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = val->res;
        }
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_VAL, 
                         msg->rpc_input);
        return res;
    }

    errval = val;
    cfgname = NULL;
    cfg = NULL;
    res = NO_ERR;
    
    /* got some value in *val */
    switch (val->btyp) {
    case NCX_BT_STRING:
        cfgname = VAL_STR(val);
        break;
    case NCX_BT_EMPTY:
        cfgname = val->name;
        break;
    case NCX_BT_CONTAINER:
        val = val_get_first_child(val);
        if (val) {
            switch (val->btyp) {
            case NCX_BT_STRING:
                if (!xml_strcmp(val->name, NCX_EL_URL)) {
                    return ERR_NCX_FOUND_URL;
                } else {
                    cfgname = VAL_STR(val);
                }
                break;
            case NCX_BT_EMPTY:
                cfgname = val->name;
                break;
            case NCX_BT_CONTAINER:
                if (!xml_strcmp(parmname, NCX_EL_SOURCE) &&
                    !xml_strcmp(val->name, NCX_EL_CONFIG)) {
                    return ERR_NCX_FOUND_INLINE;
                } else {
                    res = ERR_NCX_INVALID_VALUE;
                }
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }           
        break;
    default:
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    if (cfgname != NULL && res == NO_ERR) {
        /* check if the <url> param was given */
        if (!xml_strcmp(cfgname, NCX_EL_URL)) {
            profile = agt_get_profile();
            if (profile->agt_useurl) {
                return ERR_NCX_FOUND_URL;
            } else {
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
            }
        } else {
            /* get the config template from the config name */
            cfg = cfg_get_config(cfgname);
            if (!cfg) {
                res = ERR_NCX_CFG_NOT_FOUND;
            } else {
                *retcfg = cfg;
            }
        }
    }

    if (res != NO_ERR) {
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         (cfgname) ? NCX_NT_STRING : NCX_NT_NONE,
                         (const void *)cfgname, 
                         NCX_NT_VAL, 
                         errval);

    }

    return res;

} /* agt_get_cfg_from_parm */


/********************************************************************
* FUNCTION agt_get_inline_cfg_from_parm
*
* Get the val_value_t node for the inline config element
*
* INPUTS:
*    parmname == parameter to get from (e.g., source)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    retval == address of return value node pointer
* 
* OUTPUTS:
*   *retval is set to the address of the val_value_t struct 
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_get_inline_cfg_from_parm (const xmlChar *parmname,
                                  rpc_msg_t *msg,
                                  xml_node_t *methnode,
                                  val_value_t  **retval)
{
    val_value_t       *val, *childval;
    val_value_t       *errval;
    status_t           res;

#ifdef DEBUG
    if (!parmname || !msg || !methnode || !retval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    val = val_find_child(msg->rpc_input, 
                         val_get_mod_name(msg->rpc_input), 
                         parmname);
    if (!val || val->res != NO_ERR) {
        if (!val) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = val->res;
        }
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_VAL, 
                         msg->rpc_input);
        return res;
    }

    errval = val;
    res = NO_ERR;
    
    /* got some value in *val */
    switch (val->btyp) {
    case NCX_BT_STRING:
    case NCX_BT_EMPTY:
        res = ERR_NCX_INVALID_VALUE;
        break;
    case NCX_BT_CONTAINER:
        childval = val_get_first_child(val);
        if (childval) {
            if (!xml_strcmp(childval->name, NCX_EL_CONFIG)) {
                *retval = childval;
                return NO_ERR;
            } else {
                errval = childval;
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            res = ERR_NCX_INVALID_VALUE;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_VAL, 
                         errval);
    }

    return res;

} /* agt_get_inline_cfg_from_parm */


/********************************************************************
* FUNCTION agt_get_url_from_parm
*
* Get the URL string for the config in the target param
*
* INPUTS:
*    parmname == parameter to get from (e.g., target)
*    msg == incoming rpc_msg_t in progress
*    methnode == XML node for RPC method (for errors)
*    returl == address of return URL string pointer
*    retval == address of value node from input used
* OUTPUTS:
*   *returl is set to the address of the URL string
*   pointing to the memory inside the found parameter
*   *retval is set to parm node found, if return NO_ERR
* RETURNS:
*    status
*********************************************************************/
status_t 
    agt_get_url_from_parm (const xmlChar *parmname,
                           rpc_msg_t *msg,
                           xml_node_t *methnode,
                           const xmlChar **returl,
                           val_value_t **retval)
{
    agt_profile_t     *profile;
    val_value_t       *val;
    val_value_t       *errval;
    status_t           res;

#ifdef DEBUG
    if (!parmname || !msg || !methnode || !returl || !retval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *retval = NULL;

    val = val_find_child(msg->rpc_input, val_get_mod_name(msg->rpc_input), 
                         parmname);
    if (!val || val->res != NO_ERR) {
        if (!val) {
            res = ERR_NCX_MISSING_PARM;
        } else {
            res = val->res;
        }
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_VAL, 
                         msg->rpc_input);
        return res;
    }

    errval = val;
    res = NO_ERR;
    
    /* got some value in *val */
    switch (val->btyp) {
    case NCX_BT_STRING:
        if (xml_strcmp(parmname, NCX_EL_URL)) {
            res = ERR_NCX_INVALID_VALUE;
        } else {
            *returl = VAL_STR(val);
        }
        break;
    case NCX_BT_EMPTY:
        res = ERR_NCX_INVALID_VALUE;
        break;
    case NCX_BT_CONTAINER:
        val = val_get_first_child(val);
        if (val) {
            errval = val;
            switch (val->btyp) {
            case NCX_BT_STRING:
                if (xml_strcmp(val->name, NCX_EL_URL)) {
                    res = ERR_NCX_INVALID_VALUE;                    
                } else {
                    *returl = VAL_STRING(val);
                }
                break;
            case NCX_BT_EMPTY:
                res = ERR_NCX_INVALID_VALUE;
                break;
            case NCX_BT_CONTAINER:
                res = ERR_NCX_INVALID_VALUE;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }           
        break;
    default:
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    if (res == NO_ERR) {
        /* check if the <url> param was given */
        profile = agt_get_profile();
        if (!profile->agt_useurl) {
            res = ERR_NCX_OPERATION_NOT_SUPPORTED;
            *returl = NULL;
        }
    }

    if (res != NO_ERR) {
        agt_record_error(NULL, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         NCX_NT_NONE,
                         NULL,
                         NCX_NT_VAL, 
                         errval);
    } else {
        *retval = errval;
    }

    return res;

} /* agt_get_url_from_parm */


/********************************************************************
* FUNCTION agt_get_filespec_from_url
*
* Check the URL and get the filespec part out of it
*
* INPUTS:
*    urlstr == URL to check
*    res == address of return status
* 
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*    malloced URL string; must be freed by caller!!
*    NULL if some error
*********************************************************************/
xmlChar *
    agt_get_filespec_from_url (const xmlChar *urlstr,
                               status_t *res)
{
    const xmlChar *str;
    xmlChar       *retstr;
    uint32         schemelen, urlstrlen;

#ifdef DEBUG
    if (urlstr == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    schemelen = xml_strlen(AGT_FILE_SCHEME);
    urlstrlen = xml_strlen(urlstr);

    if (urlstrlen <= (schemelen+1)) {
        *res = ERR_NCX_INVALID_VALUE;
        return NULL;
    }
        
    /* only the file scheme file:///foospec is supported at this time */
    if (xml_strncmp(urlstr, AGT_FILE_SCHEME, schemelen)) {
        *res = ERR_NCX_INVALID_VALUE;
        return NULL;
    }

    /* convert URL to a regular string */
    /****/

    /* check for whitespace and other chars */
    str = &urlstr[schemelen];
    while (*str) {
        if (xml_isspace(*str) || *str==';' || *str==NCXMOD_PSCHAR) {
            *res = ERR_NCX_INVALID_VALUE;
            return NULL;
        }
        str++;
    }

    retstr = xml_strdup(&urlstr[schemelen]);
    if (retstr == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    *res = NO_ERR;
    return retstr;

}  /* agt_get_filespec_from_url */


/********************************************************************
* FUNCTION agt_get_parmval
*
* Get the identified val_value_t for a given parameter
* Used for error purposes!
* INPUTS:
*    parmname == parameter to get
*    msg == incoming rpc_msg_t in progress
*
* RETURNS:
*    status
*********************************************************************/
const val_value_t *
    agt_get_parmval (const xmlChar *parmname,
                     rpc_msg_t *msg)
{
    val_value_t *val;

    val =  val_find_child(msg->rpc_input,
                          val_get_mod_name(msg->rpc_input),
                          parmname);
    return val;

} /* agt_get_parmval */


/********************************************************************
* FUNCTION agt_record_error
*
* Generate an rpc_err_rec_t and save it in the msg
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured               <error-type>
*    res == internal error code                      <error-app-tag>
*    xmlnode == XML node causing error  <bad-element>   <error-path> 
*            == NULL if not available 
*    parmtyp == type of node in 'error_info'
*    error_info == error data, specific to 'res'        <error-info>
*               == NULL if not available (then nodetyp ignored)
*    nodetyp == type of node in 'error_path'
*    error_path == internal data node with the error       <error-path>
*            == NULL if not available or not used  
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL
*
* RETURNS:
*    none
*********************************************************************/
void
    agt_record_error (ses_cb_t *scb,
                      xml_msg_hdr_t *msghdr,
                      ncx_layer_t layer,
                      status_t  res,
                      const xml_node_t *xmlnode,
                      ncx_node_t parmtyp,
                      const void *error_info,
                      ncx_node_t nodetyp,
                      void *error_path)
{
    agt_record_error_errinfo(scb, msghdr, layer, res, xmlnode, parmtyp, 
                             error_info, nodetyp, error_path, NULL);

} /* agt_record_error */


/********************************************************************
* FUNCTION agt_record_error_errinfo
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured               <error-type>
*    res == internal error code                      <error-app-tag>
*    xmlnode == XML node causing error  <bad-element>   <error-path> 
*            == NULL if not available 
*    parmtyp == type of node in 'error_info'
*    error_info == error data, specific to 'res'        <error-info>
*               == NULL if not available (then nodetyp ignored)
*    nodetyp == type of node in 'error_path'
*    error_path == internal data node with the error       <error-path>
*            == NULL if not available or not used  
*    errinfo == error info record to use
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
void
    agt_record_error_errinfo (ses_cb_t *scb,
                              xml_msg_hdr_t *msghdr,
                              ncx_layer_t layer,
                              status_t  res,
                              const xml_node_t *xmlnode,
                              ncx_node_t parmtyp,
                              const void *error_info,
                              ncx_node_t nodetyp,
                              void *error_path,
                              const ncx_errinfo_t *errinfo)
{
    rpc_err_rec_t      *err = NULL;
    dlq_hdr_t          *errQ = (msghdr) ? &msghdr->errQ : NULL;
    xmlChar            *pathbuff = NULL;

    /* dump some error info to the log */
    if (LOGDEBUG3) {
        log_debug3("\nagt_record_error for session %u:",
                   scb ? SES_MY_SID(scb) : 0);
        
        if (xmlnode) {
            if (xmlnode->qname) {
                log_debug3(" xml: %s", xmlnode->qname);
            } else {
                log_debug3(" xml: %s:%s", 
                           xmlns_get_ns_prefix(xmlnode->nsid),
                           xmlnode->elname ? 
                           xmlnode->elname : (const xmlChar *)"--");
            }
        }
        if (nodetyp == NCX_NT_VAL && error_path) {
            const val_value_t *logval = (const val_value_t *)error_path;
            log_debug3(" error-path object:");
            if (obj_is_root(logval->obj)) {
                log_debug3(" <nc:config> root\n");
            } else {
                log_debug3(" <%s:%s>\n", val_get_mod_name(logval), 
                           logval->name);
            }
        }
    }

    /* generate an error only if there is a Q to hold the result */
    if (errQ) {
        /* get the error-path */
        if (error_path) {
            status_t res2;
            switch (nodetyp) {
            case NCX_NT_STRING:
                pathbuff = xml_strdup((const xmlChar *)error_path);
                break;
            case NCX_NT_VAL:
                res2 = val_gen_instance_id_ex(msghdr, 
                                              (val_value_t *)error_path, 
                                              NCX_IFMT_XPATH1, 
                                              FALSE, &pathbuff);
                if (res2 != NO_ERR) {
                    log_error("\nError: Generate instance id failed (%s)",
                              get_error_string(res2));
                }
                break;
            case NCX_NT_OBJ:
                res2 = obj_gen_object_id(error_path, &pathbuff);
                if (res2 != NO_ERR) {
                    log_error("\nError: Generate object id failed (%s)",
                              get_error_string(res2));
                }
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        err = agt_rpcerr_gen_error_ex(layer, res, xmlnode, parmtyp, error_info, 
                                      pathbuff, errinfo, nodetyp, error_path);
        if (err) {
            /* pass off pathbuff memory here */
            dlq_enque(err, errQ);
        } else {
            if (pathbuff) {
                m__free(pathbuff);
            }
        }
    }

} /* agt_record_error_errinfo */


/********************************************************************
* FUNCTION agt_record_attr_error
*
* Generate an rpc_err_rec_t and save it in the msg
*
* INPUTS:
*    scb == session control block
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    layer == netconf layer error occured
*    res == internal error code
*    xmlattr == XML attribute node causing error
*               (NULL if not available)
*    xmlnode == XML node containing the attr
*    badns == bad namespace string value
*    nodetyp == type of node in 'errnode'
*    errnode == internal data node with the error
*            == NULL if not used
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*
* RETURNS:
*    none
*********************************************************************/
void
    agt_record_attr_error (ses_cb_t *scb,
                           xml_msg_hdr_t *msghdr,
                           ncx_layer_t layer,
                           status_t  res,
                           const xml_attr_t *xmlattr,
                           const xml_node_t *xmlnode,
                           const xmlChar *badns,
                           ncx_node_t nodetyp,
                           const void *errnode)
{
    rpc_err_rec_t      *err;
    xmlChar            *buff;
    dlq_hdr_t          *errQ;
    const val_value_t  *errnodeval;

    (void)scb;
    errQ = (msghdr) ? &msghdr->errQ : NULL;
    errnodeval = NULL;

    if (errQ) {
        buff = NULL;
        if (errnode) {
            if (nodetyp==NCX_NT_STRING) {
                buff = xml_strdup((const xmlChar *)errnode);
            } else if (nodetyp==NCX_NT_VAL) {
                errnodeval = (const val_value_t *)errnode;
                status_t res2 = val_gen_instance_id_ex(msghdr, 
                                                       errnodeval,
                                                       NCX_IFMT_XPATH1, 
                                                       FALSE,
                                                       &buff);
                if (res2 != NO_ERR) {
                    log_error("\nError: Generate instance id failed (%s)",
                              get_error_string(res2));
                }
            }
        }
        err = agt_rpcerr_gen_attr_error(layer, res, xmlattr, xmlnode, 
                                        errnodeval, badns, buff);
        if (err) {
            dlq_enque(err, errQ);
        } else {
            if (buff) {
                m__free(buff);
            }
        }
    }

} /* agt_record_attr_error */


/********************************************************************
* FUNCTION agt_record_insert_error
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* For YANG 'missing-instance' error-app-tag
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    res == internal error code                      <error-app-tag>
*    errval == value node generating the insert error
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
void
    agt_record_insert_error (ses_cb_t *scb,
                             xml_msg_hdr_t *msghdr,
                             status_t  res,
                             val_value_t *errval)
{
    rpc_err_rec_t       *err;
    dlq_hdr_t           *errQ;
    xmlChar             *pathbuff;

#ifdef DEBUG
    if (!errval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    errQ = (msghdr) ? &msghdr->errQ : NULL;

    /* dump some error info to the log */
    if (LOGDEBUG3) {
        log_debug3("\nagt_record_insert_error: ");
        val_dump_value(errval, 
                       (scb) ? ses_indent_count(scb) : NCX_DEF_INDENT);
        log_debug3("\n");
    }

    /* generate an error only if there is a Q to hold the result */
    if (errQ) {
        /* get the error-path */
        pathbuff = NULL;
        status_t res2 = val_gen_instance_id_ex(msghdr, errval, NCX_IFMT_XPATH1, 
                                               FALSE, &pathbuff);
        err = agt_rpcerr_gen_insert_error(NCX_LAYER_CONTENT, res, errval, 
                                          (res2 == NO_ERR) ? pathbuff : NULL);
        if (err) {
            dlq_enque(err, errQ);
        } else {
            if (pathbuff) {
                m__free(pathbuff);
            }
        }
    }

} /* agt_record_insert_error */


/********************************************************************
* FUNCTION agt_record_unique_error
*
* Generate an rpc_err_rec_t and save it in the msg
* Use the provided error info record for <rpc-error> fields
*
* For YANG 'data-not-unique' error-app-tag
*
* INPUTS:
*    scb == session control block 
*        == NULL and no stats will be recorded
*    msghdr == XML msg header with error Q
*          == NULL, no errors will be recorded!
*    errval == list value node that contains the unique-stmt
*    valuniqueQ == Q of val_unique_t structs for error-info
*
* OUTPUTS:
*   errQ has error message added if no malloc errors
*   scb->stats may be updated if scb non-NULL

* RETURNS:
*    none
*********************************************************************/
void
    agt_record_unique_error (ses_cb_t *scb,
                             xml_msg_hdr_t *msghdr,
                             val_value_t *errval,
                             dlq_hdr_t  *valuniqueQ)
{
    rpc_err_rec_t       *err;
    dlq_hdr_t           *errQ;
    xmlChar             *pathbuff;
    status_t             interr;

#ifdef DEBUG
    if (!errval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    interr = ERR_NCX_UNIQUE_TEST_FAILED;
    errQ = (msghdr) ? &msghdr->errQ : NULL;

    /* dump some error info to the log */
    if (LOGDEBUG3) {
        log_debug3("\nagt_record_unique_error: ");
        val_dump_value(errval, 
                       (scb) ? ses_indent_count(scb) : NCX_DEF_INDENT);
        log_debug3("\n");
    }

    /* generate an error only if there is a Q to hold the result */
    if (errQ) {
        /* get the error-path */
        pathbuff = NULL;
        /* this instance ID is never sourced from the <rpc>
         * so the plain 'stop_at_root' variant is used here */
        status_t res = val_gen_instance_id(msghdr, errval, NCX_IFMT_XPATH1, 
                                           &pathbuff);
        err = agt_rpcerr_gen_unique_error(msghdr, NCX_LAYER_CONTENT, 
                                          interr, valuniqueQ,
                                          (res == NO_ERR) ? pathbuff : NULL);
        if (err) {
            dlq_enque(err, errQ);
        } else {
            if (pathbuff) {
                m__free(pathbuff);
            }
        }
    }

} /* agt_record_unique_error */


/********************************************************************
 * Validate the <filter> parameter if present.
 * If the filter is valid msg->rpc_filter is filled in;
 *
 * \param scb session control block
 * \param msg rpc_msg_t in progress
 * \return status
 *********************************************************************/
status_t agt_validate_filter ( ses_cb_t *scb,
                               rpc_msg_t *msg )
{
    assert( scb && "scb is NULL" );
    assert( msg && "msg is NULL" );

    /* filter parm is optional */
    val_value_t *filter = val_find_child( msg->rpc_input, NC_MODULE, 
                                          NCX_EL_FILTER );
    if (!filter) {
        msg->rpc_filter.op_filtyp = OP_FILTER_NONE;
        msg->rpc_filter.op_filter = NULL;
        return NO_ERR;   /* not an error */
    } 

    return agt_validate_filter_ex( scb, msg, filter );
} /* agt_validate_filter */


/********************************************************************
 * Validate the <filter> parameter if present
 * If the filter is valid msg->rpc_filter is filled in;
 *
 * \param scb session control block
 * \param msg rpc_msg_t in progress
 * \param filter the filter element to use
 * \return status
 *********************************************************************/
status_t agt_validate_filter_ex( ses_cb_t *scb, rpc_msg_t *msg, 
                                 val_value_t *filter )
{
    assert( scb && "scb is NULL" );
    assert( msg && "msg is NULL" );

    /* filter parm is optional */
    if ( !filter || filter->res != NO_ERR ) {
        return NO_ERR;
    }

    status_t res = NO_ERR;
    op_filtertyp_t  filtyp = get_filter_type( filter );;

    /* check if the select attribute is needed */
    switch (filtyp) {
    case OP_FILTER_SUBTREE:
        res = validate_subtree_filter( msg, filter );
        break;

    case OP_FILTER_XPATH:
        res = validate_xpath_filter( scb, msg, filter );
        break;

    default:
        res = ERR_NCX_INVALID_VALUE;
        agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res, NULL,
                          NCX_NT_NONE, NULL, NCX_NT_VAL, filter );
    }

    if ( NO_ERR == res && LOGDEBUG3 ) {
        log_debug3("\nagt_util_validate_filter:");
        val_dump_value(msg->rpc_input, 0);
    }

    return res;
} /* agt_validate_filter_ex */


/********************************************************************
* val_nodetest_fn_t callback
* Used by the <get-config> operation to return any type of 
* configuration data
*
* \param withdef - see ncx/val_util.h   (val_nodetest_fn_t)
* \param realtest - see ncx/val_util.h   (val_nodetest_fn_t)
* \param node - see ncx/val_util.h   (val_nodetest_fn_t)
* \return *    TRUE if config; FALSE if non-config
*********************************************************************/
boolean agt_check_config ( ncx_withdefaults_t withdef,
                           boolean realtest,
                           val_value_t *node )
{
    if (realtest) {
        if (node->dataclass == NCX_DC_CONFIG) {
            return check_withdef(withdef, node);
        } 
        
        /* not a node that should be saved with a copy-config to NVRAM */
        return FALSE;
    } 
    
    if (node->obj) {
        return obj_is_config(node->obj);
    }

    return TRUE;
} /* agt_check_config */


/********************************************************************
* FUNCTION agt_check_default
*
* val_nodetest_fn_t callback
*
* Used by the <get*> operation to return only values
* not set to the default
*
* INPUTS:
*    see ncx/val_util.h   (val_nodetest_fn_t)
*
* RETURNS:
*    status
*********************************************************************/
boolean
    agt_check_default (ncx_withdefaults_t withdef,
                       boolean realtest,
                       val_value_t *node)
{
    boolean ret;

    ret = TRUE;

    /* check if defaults are suppressed */
    if (realtest) {
        if (is_default(withdef, node)) {
            ret = FALSE;
        }
    }

    return ret;

} /* agt_check_default */


/********************************************************************
* FUNCTION agt_check_save
*
* val_nodetest_fn_t callback
*
* Used by agt_ncx_cfg_save function to filter just what
* is supposed to be saved in the <startup> config file
*
* INPUTS:
*    see ncx/val_util.h   (val_nodetest_fn_t)
*
* RETURNS:
*    status
*********************************************************************/
boolean
    agt_check_save (ncx_withdefaults_t withdef,
                    boolean realtest,
                    val_value_t *node)
{
    boolean ret;

    ret = TRUE;

    if (realtest) {
        if (node->dataclass==NCX_DC_CONFIG) {
            if (is_default(withdef, node)) {
                ret = FALSE;
            }
        } else {
            /* not a node that should be saved with a copy-config 
             * to NVRAM
             */
            ret = FALSE;
        }
    } else if (node->obj != NULL) {
        ret = obj_is_config(node->obj);
    }

    return ret;

} /* agt_check_save */


/********************************************************************
* FUNCTION agt_output_filter
*
* output the proper data for the get or get-config operation
* generate the data that matched the subtree or XPath filter
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_output_filter (ses_cb_t *scb,
                       rpc_msg_t *msg,
                       int32 indent)
{
    cfg_template_t  *source;
    ncx_filptr_t    *top;
    boolean          getop;
    status_t         res;

    getop = !xml_strcmp(obj_get_name(msg->rpc_method), 
                        NCX_EL_GET);
    if (getop) {
        source = cfg_get_config_id(NCX_CFGID_RUNNING);
    } else {
        source = (cfg_template_t *)msg->rpc_user1;
    }
    if (!source) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    if (source->root == NULL) {
        /* if a database was deleted, it will
         * have a NULL root until it gets replaced
         */
        return NO_ERR;
    }

    res = NO_ERR;

    switch (msg->rpc_filter.op_filtyp) {
    case OP_FILTER_NONE:
        switch (msg->mhdr.withdef) {
        case NCX_WITHDEF_REPORT_ALL:
        case NCX_WITHDEF_REPORT_ALL_TAGGED:
            /* return everything */
            if (getop) {
                /* all config and state data */
                xml_wr_val(scb, &msg->mhdr, source->root, indent);
            } else {
                /* all config nodes */
                xml_wr_check_val(scb, &msg->mhdr, source->root, indent, 
                                 agt_check_config);
            }
            break;
        case NCX_WITHDEF_TRIM:
        case NCX_WITHDEF_EXPLICIT:
            /* with-defaults=false: return only non-defaults */
            if (getop) {
                /* all non-default config and state data */             
                xml_wr_check_val(scb, &msg->mhdr, source->root, indent,
                                 agt_check_default);
            } else {
                /* all non-default config data */
                xml_wr_check_val(scb, &msg->mhdr, source->root, indent, 
                                 agt_check_config);
            }
            break;
        case NCX_WITHDEF_NONE:
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case OP_FILTER_SUBTREE:
        if (source->root) {
            top = agt_tree_prune_filter(scb, msg, source, getop);
            if (top) {
                agt_tree_output_filter(scb, msg, top, indent, getop);
                ncx_free_filptr(top);
                break;
            }
        }
        break;
    case OP_FILTER_XPATH:
        if (source->root) {
            res = agt_xpath_output_filter(scb, msg, source, getop, indent);
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_PTR);
    }
    return res;
                
} /* agt_output_filter */


/********************************************************************
* FUNCTION agt_output_schema
*
* generate the YANG file contents for the get-schema operation
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_output_schema (ses_cb_t *scb,
                       rpc_msg_t *msg,
                       int32 indent)
{
    ncx_module_t    *findmod;
    FILE            *fil;
    char            *buffer;
    boolean          done;
    status_t         res;

    buffer = m__getMem(NCX_MAX_LINELEN+1);
    if (!buffer) {
        return ERR_INTERNAL_MEM;
    }
    memset(buffer, 0x0, NCX_MAX_LINELEN+1);

    findmod = (ncx_module_t *)msg->rpc_user1;
    /*** ignoring the format for now; assume YANG ***/

    res = NO_ERR;
    fil = fopen((const char *)findmod->source, "r");
    if (fil) {
        ses_putstr(scb, (const xmlChar *)"\n");
        done = FALSE;
        while (!done) {
            if (fgets(buffer, NCX_MAX_LINELEN, fil)) {
                ses_putcstr(scb, (const xmlChar *)buffer, indent);
            } else {
                fclose(fil);
                done = TRUE;
            }
        }
    } else {
        res = ERR_FIL_OPEN;
    }

    m__free(buffer);

    return res;
                
} /* agt_output_schema */


/********************************************************************
* FUNCTION agt_output_empty
*
* output no data for the get or get-config operation
* because the if-modified-since fileter did not pass
*
* INPUTS:
*    see rpc/agt_rpc.h   (agt_rpc_data_cb_t)
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_output_empty (ses_cb_t *scb,
                      rpc_msg_t *msg,
                      int32 indent)
{

    scb = scb;   /* not used warning */
    msg = msg;  /* not used warning */
    indent = indent; /* not used warning */
    return NO_ERR;
                
} /* agt_output_empty */


/********************************************************************
* FUNCTION agt_check_max_access
* 
* Check if the max-access for a parameter is exceeded
*
* INPUTS:
*   newval == value node from PDU to check
*   cur_exists == TRUE if the corresponding node in the target exists
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_check_max_access (val_value_t *newval,
                          boolean cur_exists)
{
    op_editop_t   op;
    ncx_access_t  acc;
    boolean       iskey;

    if (newval == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    op = newval->editop;
    acc = obj_get_max_access(newval->obj);
    iskey = obj_is_key(newval->obj);

    switch (op) {
    case OP_EDITOP_NONE:
        return NO_ERR;
    case OP_EDITOP_MERGE:
        switch (acc) {
        case NCX_ACCESS_NONE:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RO:
            return ERR_NCX_ACCESS_READ_ONLY;
        case NCX_ACCESS_RW:
            /* edit but not create is allowed */
            return (cur_exists) ? NO_ERR : ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RC:
            return NO_ERR;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    case OP_EDITOP_REPLACE:
        switch (acc) {
        case NCX_ACCESS_NONE:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RO:
            return ERR_NCX_ACCESS_READ_ONLY;
        case NCX_ACCESS_RW:
            return (cur_exists) ? NO_ERR : ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RC:
            return NO_ERR;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    case OP_EDITOP_CREATE:
        switch (acc) {
        case NCX_ACCESS_NONE:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RO:
            return ERR_NCX_ACCESS_READ_ONLY;
        case NCX_ACCESS_RW:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RC:
            /* create/delete allowed */
            return NO_ERR;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    case OP_EDITOP_DELETE:
    case OP_EDITOP_REMOVE:
        switch (acc) {
        case NCX_ACCESS_NONE:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RO:
            return ERR_NCX_ACCESS_READ_ONLY;
        case NCX_ACCESS_RW:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RC:
            /* create/delete allowed */
            if (iskey) {
                if (newval->parent) {
                    op_editop_t pop = newval->parent->editop;
                    if (pop == OP_EDITOP_DELETE || pop == OP_EDITOP_REMOVE) {
                        /* delete operation is derived from parent */
                        return NO_ERR;
                    } else {
                        /* deleting the key leaf but not the parent */
                        return ERR_NCX_NO_ACCESS_MAX;
                    }
                } else {
                    /* key leaf with no parent?? */
                    return SET_ERROR(ERR_INTERNAL_VAL);
                }
            }
            return NO_ERR;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    case OP_EDITOP_LOAD:
        /* allow for agent loading of read-write objects */
        switch (acc) {
        case NCX_ACCESS_NONE:
            return ERR_NCX_NO_ACCESS_MAX;
        case NCX_ACCESS_RO:
            return ERR_NCX_ACCESS_READ_ONLY;
        case NCX_ACCESS_RW:
        case NCX_ACCESS_RC:
            /* create/edit/delete allowed */
            return NO_ERR;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);     
    }
    /*NOTREACHED*/

} /* agt_check_max_access */


/********************************************************************
* FUNCTION agt_check_editop
* 
* Check if the edit operation is okay
*
* INPUTS:
*   pop == parent edit operation in affect
*          Starting at the data root, the parent edit-op
*          is derived from the default-operation parameter
*   cop == address of child operation (MAY BE ADJUSTED ON EXIT!!!)
*          This is the edit-op field in the new node corresponding
*          to the curnode position in the data model
*   newnode == pointer to new node in the edit-config PDU
*   curnode == pointer to the current node in the data model
*              being affected by this operation, if any.
*           == NULL if no current node exists
*   iqual == effective instance qualifier for this value
*   proto == protocol in use
*
* OUTPUTS:
*   *cop may be adjusted to simplify further processing,
*    based on the following reduction algorithm:
*
*    create, replace, and delete operations are 'sticky'.
*    Once set, any nested edit-ops must be valid
*    within the context of the fixed operation (create or delete)
*    and the child operation gets changed to the sticky parent edit-op
*
*   if the parent is create or delete, and the child
*   is merge or replace, then the child can be ignored
*   and will be changed to the parent, since the create
*   or delete must be carried through all the way to the leaves
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_check_editop (op_editop_t   pop,
                      op_editop_t  *cop,
                      val_value_t *newnode,
                      val_value_t *curnode,
                      ncx_iqual_t iqual,
                      ncx_protocol_t proto)
{
    status_t           res;
    agt_profile_t     *profile;

    res = NO_ERR;
    profile = agt_get_profile();

    /* adjust the child if it has not been set 
     * this heuristic saves the real operation on the way down
     * the tree as each node is checked for a new operation value 
     *
     * This will either be the default-operation or a value
     * set in an anscestor node with an operation attribute
     *
     * If this is the internal startup mode (OP_EDITOP_LOAD)
     * then the load edit-op will get set and quick exit
     */
    if (*cop==OP_EDITOP_NONE) {
        *cop = pop;
    }

    /* check remove allowed */
    if (*cop == OP_EDITOP_REMOVE || pop == OP_EDITOP_REMOVE) {
        if (proto != NCX_PROTO_NETCONF11) {
            return ERR_NCX_PROTO11_NOT_ENABLED;
        }
    }

    /* check the child editop against the parent editop */
    switch (*cop) {
    case OP_EDITOP_NONE:
        /* no operation set in the child or the parent yet
         * need to check the entire subtree from this node
         * to see if any operations set at all; if so then
         * it matters if the corresponding node exists
         * at this level of the data tree
         */
        if (curnode) {
            res = NO_ERR;
        } else if (newnode) {
            if (agt_any_operations_set(newnode)) {
                res = ERR_NCX_DATA_MISSING;
            } else {
                res = NO_ERR;
            }
        } else {
            /* should not happen */
            res = NO_ERR;
        }
        break;
    case OP_EDITOP_MERGE:
    case OP_EDITOP_REPLACE:
        switch (pop) {
        case OP_EDITOP_NONE:
            /* this child contains the merge or replace operation 
             * attribute; which may be an index node; although
             * loose from a DB API POV, NETCONF will allow an
             * entry to be renamed via a merge or replace edit-op
             */
            break;
        case OP_EDITOP_MERGE:
        case OP_EDITOP_REPLACE:
            /* merge or replace inside merge or replace is okay */
            break;
        case OP_EDITOP_CREATE:
            /* a merge or replace within a create is okay
             * but it is really a create because the parent
             * operation is an explicit create, so the current
             * node is not allowed to exist yet, unless multiple
             * instances of the node are allowed
             */
            *cop = OP_EDITOP_CREATE;
            if (curnode) {
                switch (iqual) {
                case NCX_IQUAL_ONE:
                case NCX_IQUAL_OPT:
                    switch (profile->agt_defaultStyleEnum) {
                    case NCX_WITHDEF_REPORT_ALL:
                        res = ERR_NCX_DATA_EXISTS;
                        break;
                    case NCX_WITHDEF_TRIM:
                        if (!val_is_default(curnode)) {
                            res = ERR_NCX_DATA_EXISTS;
                        }
                        break;
                    case NCX_WITHDEF_EXPLICIT:
                        if (!val_set_by_default(curnode)) {
                            res = ERR_NCX_DATA_EXISTS;
                        }
                        break;
                    default:
                        res = SET_ERROR(ERR_INTERNAL_VAL);
                    }
                    break;
                default:
                    ;
                }
            }
            break;
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            /* this is an error since the merge or replace
             * cannot be performed and its parent node is
             * also getting deleted at the same time
             */
            res = ERR_NCX_DATA_MISSING;
            break;
        case OP_EDITOP_LOAD:
            /* LOAD op not allowed here */
            res = ERR_NCX_BAD_ATTRIBUTE;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case OP_EDITOP_CREATE:
        /* the child op is an explicit create
         * the current node cannot exist unless multiple
         * instances are allowed; depends on the defaults style
         */
        if (curnode) {
            switch (curnode->obj->objtype) {
            case OBJ_TYP_LEAF:
                switch (profile->agt_defaultStyleEnum) {
                case NCX_WITHDEF_REPORT_ALL:
                    res = ERR_NCX_DATA_EXISTS;
                    break;
                case NCX_WITHDEF_TRIM:
                    if (!val_is_default(curnode)) {
                        res = ERR_NCX_DATA_EXISTS;
                    }
                    break;
                case NCX_WITHDEF_EXPLICIT:
                    if (!val_set_by_default(curnode)) {
                        res = ERR_NCX_DATA_EXISTS;
                    }
                    break;
                default:
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                }
                break;
            default:
                res = ERR_NCX_DATA_EXISTS;
            }
            if (res != NO_ERR) {
                return res;
            }
        }

        /* check the create op against the parent edit-op */
        switch (pop) {
        case OP_EDITOP_NONE:
            /* make sure the create edit-op is in a correct place */
            res = (val_create_allowed(newnode)) ?
                NO_ERR : ERR_NCX_OPERATION_FAILED;
            break;
        case OP_EDITOP_MERGE:
        case OP_EDITOP_REPLACE:
            /* create within merge or replace okay since these
             * operations silently create any missing nodes
             * and the curnode test already passed
             */
            break;
        case OP_EDITOP_CREATE:
            /* create inside create okay */
            break;
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            /* create inside a delete is an error */
            res = ERR_NCX_DATA_MISSING;
            break;
        case OP_EDITOP_LOAD:
            /* LOAD op not allowed here */
            res = ERR_NCX_BAD_ATTRIBUTE;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case OP_EDITOP_REMOVE:
    case OP_EDITOP_DELETE:
        /* explicit delete means the current node must exist
         * unlike a remove which removes nodes if they exist,
         * without any error checking for curnode exists
         */
        if (curnode == NULL) {
            if (*cop == OP_EDITOP_DELETE) {
                /* delete on non-existing node is always an error */
                res = ERR_NCX_DATA_MISSING;
            }
        } else {
            /* check if the node is really present for delete */
            switch (profile->agt_defaultStyleEnum) {
            case NCX_WITHDEF_REPORT_ALL:
                break;
            case NCX_WITHDEF_TRIM:
                if (*cop == OP_EDITOP_DELETE && 
                    val_is_default(curnode)) {
                    res = ERR_NCX_DATA_MISSING;
                }
                break;
            case NCX_WITHDEF_EXPLICIT:
                if (*cop == OP_EDITOP_DELETE && 
                    val_set_by_default(curnode)) {
                    res = ERR_NCX_DATA_MISSING;
                }
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }

            if (res != NO_ERR) {
                return res;
            }

            /* check the delete against the parent edit-op */
            switch (pop) {
            case OP_EDITOP_NONE:
                res = (val_delete_allowed(curnode))
                    ? NO_ERR : ERR_NCX_BAD_ATTRIBUTE;
                break;
            case OP_EDITOP_MERGE:
                /* delete within merge or ok */
                break;
            case OP_EDITOP_REPLACE:
                /* this is a corner case; delete within a replace
                 * the application could have just left this node
                 * out instead, but allow this form too
                 */
                break;
            case OP_EDITOP_CREATE:
                /* create within a delete always an error */
                res = ERR_NCX_DATA_MISSING;
                break;
            case OP_EDITOP_DELETE:
            case OP_EDITOP_REMOVE:
                /* delete within delete always okay */
                break;
            case OP_EDITOP_LOAD:
                /* LOAD op not allowed here */
                res = ERR_NCX_BAD_ATTRIBUTE;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case OP_EDITOP_LOAD:
        if (pop != OP_EDITOP_LOAD) {
            res = ERR_NCX_BAD_ATTRIBUTE;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }
    return res;

} /* agt_check_editop */


/********************************************************************
* FUNCTION agt_enable_feature
* 
* Enable a YANG feature in the agent
* This will not be detected by any sessions in progress!!!
* It will take affect the next time a <hello> message
* is sent by the agent
*
* INPUTS:
*   modname == module name containing the feature
*   featurename == feature name to enable
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_enable_feature (const xmlChar *modname,
                        const xmlChar *featurename)
{

    ncx_module_t   *mod;
    ncx_feature_t  *feature;

#ifdef DEBUG
    if (!modname || !featurename) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    mod = ncx_find_module(modname, NULL);
    if (!mod) {
        return ERR_NCX_MOD_NOT_FOUND;
    }

    feature = ncx_find_feature(mod, featurename);
    if (!feature) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    feature->enabled = TRUE;
    return NO_ERR;

} /* agt_enable_feature */


/********************************************************************
* FUNCTION agt_disable_feature
* 
* Disable a YANG feature in the agent
* This will not be detected by any sessions in progress!!!
* It will take affect the next time a <hello> message
* is sent by the agent
*
* INPUTS:
*   modname == module name containing the feature
*   featurename == feature name to disable
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_disable_feature (const xmlChar *modname,
                         const xmlChar *featurename)
{
    ncx_module_t   *mod;
    ncx_feature_t  *feature;

#ifdef DEBUG
    if (!modname || !featurename) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    mod = ncx_find_module(modname, NULL);
    if (!mod) {
        return ERR_NCX_MOD_NOT_FOUND;
    }

    feature = ncx_find_feature(mod, featurename);
    if (!feature) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    feature->enabled = FALSE;
    return NO_ERR;

} /* agt_disable_feature */

status_t
    agt_check_feature (const xmlChar *modname,
                         const xmlChar *featurename,
                         int* enabled)
{
    ncx_module_t   *mod;
    ncx_feature_t  *feature;

#ifdef DEBUG
    if (!modname || !featurename) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    mod = ncx_find_module(modname, NULL);
    if (!mod) {
        return ERR_NCX_MOD_NOT_FOUND;
    }

    feature = ncx_find_feature(mod, featurename);
    if (!feature) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    *enabled = feature->enabled;
    return NO_ERR;

}


/********************************************************************
* make a val_value_t struct for a specified leaf or leaf-list
*
* \param parentobj parent object to find child leaf object
* \param leafname name of leaf to find (namespace hardwired)
* \param leafstrval== string version of value to set for leaf
* \param res address of return status
* \return :malloced value struct or NULL if some error
*********************************************************************/
val_value_t* agt_make_leaf ( obj_template_t *parentobj,
                             const xmlChar *leafname,
                             const xmlChar *leafstrval,
                             status_t *res )
{
    assert( parentobj && "parentobj is NULL" );
    assert( leafname && "leafname is NULL" );
    assert( res && "res is NULL" );

    
    obj_template_t *leafobj = obj_find_child( parentobj, 
            obj_get_mod_name(parentobj), leafname );

    if ( !leafobj ) {
        *res =ERR_NCX_DEF_NOT_FOUND;
        return NULL;
    }

    if (!(leafobj->objtype == OBJ_TYP_LEAF ||
          leafobj->objtype == OBJ_TYP_LEAF_LIST)) {
        *res = ERR_NCX_WRONG_TYPE;
        return NULL;
    }

    val_value_t *leafval = val_make_simval_obj( leafobj, leafstrval, res );
    return leafval;
}  /* agt_make_leaf */

/********************************************************************
 * make a val_value_t struct for a specified leaf or leaf-list
 *
 * \param parentobj parent object to find child leaf object
 * \param leafname name of leaf to find (namespace hardwired)
 * \param leafval number value for leaf
 * \param res address of return status
* \return :malloced value struct or NULL if some error
 *********************************************************************/
val_value_t * agt_make_uint_leaf ( obj_template_t *parentobj,
                                   const xmlChar *leafname,
                                   uint32 leafval,
                                   status_t *res )
{
    assert( parentobj && "parentobj is NULL" );
    assert( leafname && "leafname is NULL" );
    assert( res && "res is NULL" );

    xmlChar numbuff[NCX_MAX_NUMLEN];
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%u", leafval);

    return agt_make_leaf( parentobj, leafname, numbuff, res); 
}  /* agt_make_uint_leaf */

/********************************************************************
 * make a val_value_t struct for a specified leaf or leaf-list
 *
 * \param parentobj parent object to find child leaf object
 * \param leafname name of leaf to find (namespace hardwired)
 * \param leafval number value for leaf
 * \param res address of return status
* \return :malloced value struct or NULL if some error
 *********************************************************************/
val_value_t *agt_make_int_leaf( obj_template_t *parentobj, 
                                const xmlChar *leafname,
                                int32 leafval,
                                status_t *res )
{
    assert( parentobj && "parentobj is NULL" );
    assert( leafname && "leafname is NULL" );
    assert( res && "res is NULL" );

    xmlChar numbuff[NCX_MAX_NUMLEN];
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%d", leafval);

    return agt_make_leaf(parentobj, leafname, numbuff, res);
}  /* agt_make_int_leaf */

/********************************************************************
 * make a val_value_t struct for a specified leaf or leaf-list
 *
 * \param parentobj parent object to find child leaf object
 * \param leafname name of leaf to find (namespace hardwired)
 * \param leafval number value for leaf
 * \param res address of return status
* \return :malloced value struct or NULL if some error
 *********************************************************************/
val_value_t *agt_make_uint64_leaf ( obj_template_t *parentobj,
                                    const xmlChar *leafname,
                                    uint64 leafval,
                                    status_t *res)
{
    assert( parentobj && "parentobj is NULL" );
    assert( leafname && "leafname is NULL" );
    assert( res && "res is NULL" );

    xmlChar numbuff[NCX_MAX_NUMLEN];
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%llu", 
             (long long unsigned int) leafval);

    return agt_make_leaf( parentobj, leafname, numbuff, res); 
}  /* agt_make_uint64_leaf */


/********************************************************************
 * make a val_value_t struct for a specified leaf or leaf-list
 *
 * \param parentobj parent object to find child leaf object
 * \param leafname name of leaf to find (namespace hardwired)
 * \param leafval number value for leaf
 * \param res address of return status
* \return :malloced value struct or NULL if some error
 *********************************************************************/
val_value_t *agt_make_int64_leaf( obj_template_t *parentobj,
                                  const xmlChar *leafname,
                                  int64 leafval,
                                  status_t *res )
{
    assert( parentobj && "parentobj is NULL" );
    assert( leafname && "leafname is NULL" );
    assert( res && "res is NULL" );

    xmlChar numbuff[NCX_MAX_NUMLEN];
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%lld", (long long int)leafval);

    return agt_make_leaf( parentobj, leafname, numbuff, res); 
}  /* agt_make_int64_leaf */


/********************************************************************
* FUNCTION agt_make_idref_leaf
*
* make a val_value_t struct for a specified leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   leafval == identityref value for leaf
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
val_value_t *
    agt_make_idref_leaf (obj_template_t *parentobj,
                         const xmlChar *leafname,
                         const val_idref_t *leafval,
                         status_t *res)
{
    obj_template_t *leafobj;
    val_value_t    *newval;

#ifdef DEBUG
    if (parentobj == NULL || leafname == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    leafobj = obj_find_child(parentobj,
                             obj_get_mod_name(parentobj),
                             leafname);
    if (!leafobj) {
        *res =ERR_NCX_DEF_NOT_FOUND;
        return NULL;
    }
    if (!(leafobj->objtype == OBJ_TYP_LEAF ||
          leafobj->objtype == OBJ_TYP_LEAF_LIST)) {
        *res = ERR_NCX_WRONG_TYPE;
        return NULL;
    }
    if (obj_get_basetype(leafobj) != NCX_BT_IDREF) {
        *res = ERR_NCX_WRONG_TYPE;
        return NULL;
    }
        
    newval = val_new_value();
    if (newval == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    val_init_from_template(newval, leafobj);

    newval->v.idref.name = xml_strdup(leafval->name);
    if (newval->v.idref.name == NULL) {
        val_free_value(newval);
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    newval->v.idref.nsid = leafval->nsid;
    newval->v.idref.identity = leafval->identity;

    return newval;

}  /* agt_make_idref_leaf */


/********************************************************************
* FUNCTION agt_make_list
*
* make a val_value_t struct for a specified list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   listame == name of list object to find (namespace hardwired)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct for the list or NULL if some error
*********************************************************************/
val_value_t *
    agt_make_list (obj_template_t *parentobj,
                   const xmlChar *listname,
                   status_t *res)
{
    return agt_make_object(parentobj, listname, res);

}  /* agt_make_list */


/********************************************************************
* FUNCTION agt_make_object
*
* make a val_value_t struct for a specified node
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   objname == name of the object to find (namespace hardwired)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct for the list or NULL if some error
*********************************************************************/
val_value_t *
    agt_make_object (obj_template_t *parentobj,
                     const xmlChar *objname,
                     status_t *res)
{
    obj_template_t  *obj;
    val_value_t     *val;

    assert( parentobj && "parentobj == NULL!" );
    assert( objname && "objname == NULL!" );
    assert( res && "res == NULL!" );

    obj = obj_find_child(parentobj, obj_get_mod_name(parentobj), objname);
    if (!obj) {
        *res =ERR_NCX_DEF_NOT_FOUND;
        return NULL;
    }

    val = val_new_value();
    if (val == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    val_init_from_template(val, obj);

    return val;

}  /* agt_make_object */


/********************************************************************
* FUNCTION agt_make_virtual_leaf
*
* make a val_value_t struct for a specified virtual 
* leaf or leaf-list
*
INPUTS:
*   parentobj == parent object to find child leaf object
*   leafname == name of leaf to find (namespace hardwired)
*   callbackfn == get callback function to install
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced value struct or NULL if some error
*********************************************************************/
val_value_t *
    agt_make_virtual_leaf (obj_template_t *parentobj,
                           const xmlChar *leafname,
                           getcb_fn_t callbackfn,
                           status_t *res)
{
    obj_template_t  *leafobj;
    val_value_t     *leafval;

#ifdef DEBUG
    if (parentobj == NULL || 
        leafname == NULL ||
        callbackfn == NULL ||
        res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    
    leafobj = obj_find_child(parentobj,
                             obj_get_mod_name(parentobj),
                             leafname);
    if (!leafobj) {
        *res =ERR_NCX_DEF_NOT_FOUND;
        return NULL;
    }
    if (!(leafobj->objtype == OBJ_TYP_LEAF ||
          leafobj->objtype == OBJ_TYP_LEAF_LIST)) {
        *res = ERR_NCX_WRONG_TYPE;
        return NULL;
    }

    leafval = val_new_value();
    if (!leafval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_virtual(leafval, callbackfn, leafobj);

    return leafval;

}  /* agt_make_virtual_leaf */


/********************************************************************
* FUNCTION agt_add_top_virtual
*
* make a val_value_t struct for a specified virtual 
* top-level data node
*
* INPUTS:
*   obj == object node of the virtual data node to create
*   callbackfn == get callback function to install
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_add_top_virtual (obj_template_t *obj,
                         getcb_fn_t callbackfn)
{
    val_value_t     *rootval, *nodeval;

#ifdef DEBUG
    if (obj == NULL || callbackfn == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    rootval = cfg_get_root(NCX_CFGID_RUNNING);
    if (rootval == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    nodeval = val_new_value();
    if (!nodeval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_virtual(nodeval, callbackfn, obj);
    val_add_child_sorted(nodeval, rootval);
    return NO_ERR;

}  /* agt_add_top_virtual */


/********************************************************************
* FUNCTION agt_add_top_container
*
* make a val_value_t struct for a specified 
* top-level container data node.  This is used
* by SIL functions to create a top-level container
* which may have virtual nodes within it
*
* TBD: fine-control over SIL C code generation to
* allow mix of container virtual callback plus
* child node virtual node callbacks
*
INPUTS:
*   obj == object node of the container data node to create
*   val == address of return val pointer
*
* OUTPUTS:
*   *val == pointer to node created in the database
*           this is not live memory! It will be freed
*           by the database management code
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_add_top_container (obj_template_t *obj,
                           val_value_t **val)
{
    val_value_t     *rootval, *nodeval;

#ifdef DEBUG
    if (obj == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    rootval = cfg_get_root(NCX_CFGID_RUNNING);
    if (rootval == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    nodeval = val_new_value();
    if (!nodeval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(nodeval, obj);
    val_add_child_sorted(nodeval, rootval);
    if (val != NULL) {
        *val = nodeval;
    }
    return NO_ERR;

}  /* agt_add_top_container */


/********************************************************************
* FUNCTION agt_add_container
*
* make a val_value_t struct for a specified 
* nested container data node.  This is used
* by SIL functions to create a nested container
* which may have virtual nodes within it
*
* TBD: fine-control over SIL C code generation to
* allow mix of container virtual callback plus
* child node virtual node callbacks
*
INPUTS:
*   modname == module name defining objname
*   objname == name of object node to create
*   parentval == parent value node to add container to
*   val == address of return val pointer
*
* OUTPUTS:
*   *val == pointer to node created in the database
*           this is not live memory! It will be freed
*           by the database management code
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_add_container (const xmlChar *modname,
                       const xmlChar *objname,
                       val_value_t *parentval,
                       val_value_t **val)
{
    val_value_t     *nodeval;
    obj_template_t  *obj;

#ifdef DEBUG
    if (objname == NULL || parentval == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    obj = obj_find_child(parentval->obj, modname, objname);
    if (obj == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    nodeval = val_new_value();
    if (!nodeval) {
        return ERR_INTERNAL_MEM;
    }

    val_init_from_template(nodeval, obj);
    val_add_child_sorted(nodeval, parentval);
    if (val != NULL) {
        *val = nodeval;
    }
    return NO_ERR;

}  /* agt_add_container */


/********************************************************************
* FUNCTION agt_init_cache
*
* init a cache pointer during the init2 callback
*
* INPUTS:
*   modname == name of module defining the top-level object
*   objname == name of the top-level database object
*   res == address of return status
*
* OUTPUTS:
*   *res is set to the return status
*
* RETURNS:
*   pointer to object value node from running config,
*   or NULL if error or not found
*********************************************************************/
val_value_t *
    agt_init_cache (const xmlChar *modname,
                    const xmlChar *objname,
                    status_t *res)
{
    cfg_template_t  *cfg;
    val_value_t     *retval;

#ifdef DEBUG
    if (modname == NULL || objname == NULL || res==NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    cfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (cfg == NULL) {
        *res = ERR_NCX_CFG_NOT_FOUND;
        return NULL;
    }
    if (cfg->root == NULL) {
        *res = NO_ERR;
        return NULL;
    }

    retval = val_find_child(cfg->root, modname, objname);

    *res = NO_ERR;
    return retval;

}  /* agt_init_cache */


/********************************************************************
* FUNCTION agt_check_cache
*
* check if a cache pointer needs to be changed or NULLed out
*
INPUTS:
*   cacheptr == address of pointer to cache value node
*   newval == newval from the callback function
*   curval == curval from the callback function
*   editop == editop from the callback function
*
* OUTPUTS:
*   *cacheptr may be changed, depending on the operation
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_check_cache (val_value_t **cacheptr,
                     val_value_t *newval,
                     val_value_t *curval,
                     op_editop_t editop)
{

#ifdef DEBUG
    if (cacheptr == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (editop) {
    case OP_EDITOP_MERGE:
        if (newval && curval) {
            if (typ_is_simple(newval->btyp)) {
                *cacheptr = newval;
            } else {
                *cacheptr = curval;
            }
        } else if (newval) {
            *cacheptr = newval;
        } else if (curval) {
            *cacheptr = curval;
        } else {
            *cacheptr = NULL;
        }
        break;
    case OP_EDITOP_REPLACE:
    case OP_EDITOP_CREATE:
        *cacheptr = newval;
        break;
    case OP_EDITOP_DELETE:
    case OP_EDITOP_REMOVE:
        *cacheptr = NULL;
        break;
    case OP_EDITOP_LOAD:
    case OP_EDITOP_COMMIT:
        *cacheptr = newval;
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    return NO_ERR;

}  /* agt_check_cache */



/********************************************************************
* FUNCTION agt_new_xpath_pcb
*
* Get a new XPath parser control block and
* set up the server variable bindings
*
* INPUTS:
*   scb == session evaluating the XPath expression
*   expr == expression string to use (may be NULL)
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced and initialied xpath_pcb_t structure
*   NULL if some error
*********************************************************************/
xpath_pcb_t *
    agt_new_xpath_pcb (ses_cb_t *scb,
                       const xmlChar *expr,
                       status_t *res)
{
    val_value_t   *userval;
    xpath_pcb_t   *pcb;
    dlq_hdr_t     *varbindQ;

#ifdef DEBUG
    if (scb == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (scb->username == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    pcb = xpath_new_pcb(expr, NULL);
    if (pcb == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    userval = val_make_string(0, AGT_USER_VAR, scb->username);
    if (userval == NULL) {
        xpath_free_pcb(pcb);
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    varbindQ = xpath_get_varbindQ(pcb);

    /* pass off userval memory here, even if error returned */
    *res = var_set_move_que(varbindQ, AGT_USER_VAR, userval);
    if (*res != NO_ERR) {
        xpath_free_pcb(pcb);
        pcb = NULL;
    } /* else userval memory stored in varbindQ now */

    return pcb;
    
}  /* agt_new_xpath_pcb */


/********************************************************************
* FUNCTION agt_get_startup_filespec
*
* Figure out where to store the startup file
*
* INPUTS:
*   res == address of return status
*
* OUTPUTS:
*   *res == return status

* RETURNS:
*   malloced and filled in filespec string; must be freed by caller
*   NULL if malloc error
*********************************************************************/
xmlChar *
    agt_get_startup_filespec (status_t *res)
{
    cfg_template_t    *startup, *running;    
    const xmlChar     *yumahome;
    xmlChar           *filename;

#ifdef DEBUG
    if (res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    running = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (running == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    startup = cfg_get_config_id(NCX_CFGID_STARTUP);

    yumahome = ncxmod_get_yuma_home();

    /* get the right filespec to use
     *
     * 1) use the startup filespec
     * 2) use the running filespec
     * 3) use $YUMA_HOME/data/startup-cfg.xml
     * 4) use $HOME/.yuma/startup-cfg.xml
     */
    if (startup && startup->src_url) {
        filename = xml_strdup(startup->src_url);
        if (filename == NULL) {
            *res = ERR_INTERNAL_MEM;
        }
    } else if (running && running->src_url) {
        filename = xml_strdup(running->src_url);
        if (filename == NULL) {
            *res = ERR_INTERNAL_MEM;
        }
    } else if (yumahome != NULL) {
        filename = ncx_get_source(NCX_YUMA_HOME_STARTUP_FILE, res);
    } else {
        filename = ncx_get_source(NCX_DOT_YUMA_STARTUP_FILE, res);
    }

    return filename;

}  /* agt_get_startup_filespec */


/********************************************************************
* FUNCTION agt_get_target_filespec
*
* Figure out where to store the URL target file
*
* INPUTS:
*   target_url == target url spec to use; this is
*                 treated as a relative pathspec, and
*                 the appropriate data directory is used
*                 to create this file
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced and filled in filespec string; must be freed by caller
*   NULL if some error
*********************************************************************/
xmlChar *
    agt_get_target_filespec (const xmlChar *target_url,
                             status_t *res)
{
    cfg_template_t    *startup, *running;    
    const xmlChar     *yumahome;
    xmlChar           *filename, *tempbuff, *str;
    uint32             len;

#ifdef DEBUG
    if (target_url == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;
    filename = NULL;
    tempbuff = NULL;

    running = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (running == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    startup = cfg_get_config_id(NCX_CFGID_STARTUP);

    yumahome = ncxmod_get_yuma_home();

    /* get the right filespec to use
     *
     * 1) use the startup filespec
     * 2) use the running filespec
     * 3) use $YUMA_HOME/data/startup-cfg.xml
     * 4) use $HOME/.yuma/startup-cfg.xml
     */
    if (startup && startup->src_url) {
        len = ncxmod_get_pathlen_from_filespec(startup->src_url);
        filename = m__getMem(len + xml_strlen(target_url) + 1);
        if (filename == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            str = filename;
            str += xml_strncpy(str, startup->src_url, len);
            xml_strcpy(str, target_url);
        }
    } else if (running && running->src_url) {
        len = ncxmod_get_pathlen_from_filespec(running->src_url);
        filename = m__getMem(len + xml_strlen(target_url) + 1);
        if (filename == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            str = filename;
            str += xml_strncpy(str, running->src_url, len);
            xml_strcpy(str, target_url);
        }
    } else if (yumahome != NULL) {
        len = xml_strlen(NCX_YUMA_HOME_STARTUP_DIR);
        tempbuff = m__getMem(len + xml_strlen(target_url) + 1);
        if (tempbuff == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            filename = ncx_get_source(tempbuff, res);
        }
    } else {
        len = xml_strlen(NCX_DOT_YUMA_STARTUP_DIR);
        tempbuff = m__getMem(len + xml_strlen(target_url) + 1);
        if (tempbuff == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            filename = ncx_get_source(tempbuff, res);
        }
    }

    if (tempbuff != NULL) {
        m__free(tempbuff);
    }

    return filename;

}  /* agt_get_target_filespec */


/********************************************************************
* FUNCTION agt_set_mod_defaults
*
* Check for any top-level config leafs that have a default
* value, and add them to the running configuration.
*
* INPUTS:
*   mod == module that was just added and should be used
*          to check for top-level database leafs with a default
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_set_mod_defaults (ncx_module_t *mod)
{
    cfg_template_t    *running;
    obj_template_t    *defobj, *defcase, *childobj;
    const xmlChar     *defstr;
    status_t           res;

#ifdef DEBUG
    if (mod == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    running = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (running == NULL || running->root == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    res = NO_ERR;

    for (defobj = ncx_get_first_data_object(mod);
         defobj != NULL && res == NO_ERR;
         defobj = ncx_get_next_data_object(mod, defobj)) {

        /* only care about top-level leafs and choices */
        if (defobj->objtype == OBJ_TYP_CHOICE) {
            defcase = obj_get_default_case(defobj);
            if (defcase != NULL) {
                /* check the default case for any default leafs,
                 * there should be at least one of them
                 */
                for (childobj = obj_first_child(defcase);
                     childobj != NULL;
                     childobj = obj_next_child(childobj)) {

                    /* only care about config leafs */
                    /* !!! should dive into choices with default cases !!! */
                    if (childobj->objtype == OBJ_TYP_LEAF &&
                        obj_get_config_flag(childobj)) {
                        /* only care about leafs with default values */
                        defstr = obj_get_default(childobj);
                        if (defstr != NULL) {
                            /* create this top-level leaf */
                            res = add_default_leaf(running->root, 
                                                   childobj,
                                                   defstr);
                        }
                    }
                }
            }
        } else if (defobj->objtype == OBJ_TYP_LEAF &&
                   obj_get_config_flag(defobj)) {
            /* only care about config leafs with default values */
            defstr = obj_get_default(defobj);
            if (defstr != NULL) {
                /* create this top-level leaf */
                res = add_default_leaf(running->root, 
                                       defobj,
                                       defstr);
            }
        }
    }

    return res;

} /* agt_set_mod_defaults */


/********************************************************************
* FUNCTION agt_set_with_defaults
*
* Check if the <with-defaults> parameter is set
* in the request message, and if so, is it 
* one of the server's supported values
*
* If not, then record an error
* If so, then set the msg->mhdr.withdef enum
*
* INPUTS:
*   scb == session control block to use
*   msg == rpc message in progress
*   methnode == XML node for the method name
*
* OUTPUTS:
*   msg->mhdr.withdef set if NO_ERR
*   rpc-error recorded if any error detected
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_set_with_defaults (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode)
{
    val_value_t     *parm;
    status_t         res;

#ifdef DEBUG
    if (scb == NULL || msg == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#else
    (void)scb;
#endif

    (void)methnode;
    res = NO_ERR;

    /* check the with-defaults parameter */
    parm = val_find_child(msg->rpc_input,
                          NCXMOD_WITH_DEFAULTS, 
                          NCX_EL_WITH_DEFAULTS);
    if (parm == NULL) {
        /* optional parameter not found;
         * the proper with-defaults enum should
         * have been set when the msg was initialized
         */
        return NO_ERR;
    }

    if (parm->res != NO_ERR) {
        /* error should already be recorded */
        return parm->res;
    }

    msg->mhdr.withdef = 
        ncx_get_withdefaults_enum(VAL_ENUM_NAME(parm));

    return res;

}  /* agt_set_with_defaults */


/********************************************************************
 * Get the next expected key value in the ancestor chain
 * Used in Yuma SIL code to invoke User SIL callbacks with key values.
 * The caller should maintain the lastkey as a state-var as the anscestor,
 * keys are traversed. If lastkey == NULL the first key is returned.
 *
 * \param startval value node to start from
 * \param lastkey address of last key leaf found in ancestor chain.
 * \return  value node to use (do not free!!!)
 *********************************************************************/
val_value_t* agt_get_key_value ( val_value_t *startval,
                                 val_value_t **lastkey )
{
    assert( startval && "startval is NULL" );
    assert( lastkey && "lastkey is NULL" );

    agt_keywalker_parms_t  parms;
    parms.lastkey = *lastkey;
    parms.retkey = NULL;
    parms.usenext = FALSE;
    parms.done = FALSE;

    if ( LOGDEBUG3 ) {
        log_debug3( "\nStart key walk for %s", startval->name );
        if ( *lastkey ) {
            log_debug3("  lastkey=%s", (*lastkey)->name);
        }
    }

    val_traverse_keys(startval, (void *)&parms, NULL, get_key_value);

    if ( LOGDEBUG3 ) {
        log_debug3("\nEnd key walk for %s:", startval->name);
        if (parms.retkey) {
            log_debug3("  retkey:\n");
            val_dump_value(parms.retkey, 2);
        }
    }

    *lastkey = parms.retkey;
    return parms.retkey;
}  /* agt_get_key_value */


/********************************************************************
* FUNCTION agt_add_top_node_if_missing
* 
* SIL Phase 2 init:  Check the running config to see if
* the specified node is there; if not create one
* return the node either way
* INPUTS:
*   mod == module containing object
*   objname == object name
*   added   == address of return added flag
*   res     == address of return status
*
* OUTPUTS:
*   *added == TRUE if new node was added
*   *res == return status, return NULL unless NO_ERR
*
* RETURNS:
*   if *res==NO_ERR,
*      pointer to new or existing node for modname:objname
*      this is added to running config and does not have to
*      be deleted.  
*      !!!! No MRO nodes or default nodes have been added !!!!
*********************************************************************/
val_value_t *
    agt_add_top_node_if_missing (ncx_module_t *mod,
                                 const xmlChar *objname,
                                 boolean *added,
                                 status_t *res)
{
    obj_template_t        *nodeobj;
    cfg_template_t        *runningcfg;
    val_value_t           *nodeval;
    const xmlChar         *modname;
#ifdef DEBUG
    if (!mod || !objname || !added || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;
    *added = FALSE;
    modname = ncx_get_modname(mod);

    /* make sure the running config root is set */
    runningcfg = cfg_get_config(NCX_EL_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    nodeobj = obj_find_template_top(mod, modname, objname);
    if (nodeobj == NULL) {
        *res = SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return NULL;
    }

    /* check to see if the /nacm branch already exists
     * if so, then skip all this init stuff
     */
    nodeval = val_find_child(runningcfg->root, modname, objname);
    if (nodeval == NULL) {
        /* did not find the /modname:objname node so create one;
         * get all the static object nodes first 
         */

        /* create the static structure for the /nacm data model
         * start with the top node /nacm
         */
        nodeval = val_new_value();
        if (nodeval == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            *added = TRUE;
            val_init_from_template(nodeval, nodeobj);
            
            /* handing off the malloced memory here */
            val_add_child_sorted(nodeval, runningcfg->root);
        }
    }

    return nodeval;

}  /* agt_add_top_node_if_missing */


/********************************************************************
* FUNCTION agt_any_operations_set
*
* Check the new node and all descendants
* for any operation attibutes prsent
*
* INPUTS:
*   val == value node to check
*
* RETURNS:
*   TRUE if any operation attributes found
*   FALSE if no operation attributes found
*/
boolean
    agt_any_operations_set (val_value_t *val)
{
    val_value_t  *childval;

    if (val->editvars && val->editvars->operset) {
        return TRUE;
    }

    for (childval = val_get_first_child(val);
         childval != NULL;
         childval = val_get_next_child(childval)) {
        boolean anyset = agt_any_operations_set(childval);
        if (anyset) {
            return TRUE;
        }
    }

    return FALSE;

}  /* agt_any_operations_set */


/********************************************************************
* FUNCTION agt_apply_this_node
* 
* Check if the write operation applies to the current node
*
* INPUTS:
*    editop == edit operation value
*    newnode == pointer to new node (if any)
*    curnode == pointer to current value node (if any)
*                (just used to check if non-NULL or compare leaf)
*
* RETURNS:
*    TRUE if the current node needs the write operation applied
*    FALSE if this is a NO=OP node (either explicit or special merge)
*********************************************************************/
boolean
    agt_apply_this_node (op_editop_t editop,
                         const val_value_t *newnode,
                         const val_value_t *curnode)
{
    boolean retval = FALSE;
    switch (editop) {
    case OP_EDITOP_NONE:
        /* never apply here when operation is none */
        break;
    case OP_EDITOP_MERGE:
        /* if no current node then always merge here
         * merge child nodes into an existing complex type
         * except for the index nodes, which are kept
         */
        if (!curnode) {
            retval = TRUE;
        } else {
            /* if this is a leaf and not an index leaf, then
             * apply the merge here, if value changed
             */
            if (curnode && !curnode->index) {
                if (typ_is_simple(obj_get_basetype(curnode->obj))) {
                    if (newnode == NULL) {
                        retval = TRUE;  // should not happen!
                    } else if (newnode->editvars != NULL &&
                               newnode->editvars->insertop != OP_INSOP_NONE) {
                        retval = TRUE;
                    } else if (val_compare(newnode, curnode) != 0) {
                        retval = TRUE;
                    } 
                } /* else this is a complex node, keep checking
                   * all the descendents in case an insert operation
                   * is present in a nested list or leaf list
                   * this will compare the same for a move op
                   */
            }
        }
        break;
    case OP_EDITOP_REPLACE:
        if (curnode == NULL) {
            retval = TRUE;
        } else if (!obj_is_root(curnode->obj)) {
            /* apply here if value has changed */
            if (newnode == NULL) {
                SET_ERROR(ERR_INTERNAL_VAL);
            } else if (newnode->editvars != NULL &&
                       newnode->editvars->insertop != OP_INSOP_NONE) {
                retval = TRUE;
            } else if (val_compare_max(newnode, curnode, TRUE, TRUE, TRUE)
                       != 0) {
                retval = TRUE;
            } /* else if this is a complex node, keep checking
               * all the descendents in case an insert operation
               * is present in a nested list or leaf list
               * this will compare the same for a move op
               */
        }
        break;
    case OP_EDITOP_COMMIT:
    case OP_EDITOP_CREATE:
    case OP_EDITOP_LOAD:
    case OP_EDITOP_DELETE:
    case OP_EDITOP_REMOVE:
        retval = TRUE;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return retval;

} /* agt_apply_this_node */


/* END file agt_util.c */
