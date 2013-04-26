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
/*  FILE: agt_val_parse.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11feb06      abb      begun
06aug08      abb      YANG redesign

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#include <xmlstring.h>
#include <xmlreader.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_ses.h"
#include "agt_top.h"
#include "agt_util.h"
#include "agt_val_parse.h"
#include "agt_xml.h"
#include "b64.h"
#include "cfg.h"
#include "def_reg.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncx_list.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "obj.h"
#include "status.h"
#include "tk.h"
#include "typ.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xpath.h"
#include "xpath_yang.h"
#include "xpath1.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* forward declaration for recursive calls */
static status_t 
    parse_btype_nc (ses_cb_t  *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    const xml_node_t *startnode,
                    ncx_data_class_t parentdc,
                    val_value_t  *retval);


/********************************************************************
* FUNCTION parse_error_subtree
* 
* Generate an error during parmset processing for an element
* Add rpc_err_rec_t structs to the msg->errQ
*
* INPUTS:
*   scb == session control block (NULL means call is a no-op)
*   msg = xml_msg_hdr_t from msg in progress  (NULL means call is a no-op)
*   startnode == parent start node to match on exit
*         If this is NULL then the reader will not be advanced
*   errnode == error node being processed
*         If this is NULL then the current node will be
*         used if it can be retrieved with no errors,
*         and the node has naming properties.
*   errcode == error status_t of initial internal error
*         This will be used to pick the error-tag and
*         default error message
*   errnodetyp == internal VAL_PCH node type used for error_parm
*   error_parm == pointer to attribute name or namespace name, etc.
*         used in the specific error in agt_rpcerr.c
*   intnodetyp == internal VAL_PCH node type used for intnode
*   intnode == internal VAL_PCH node used for error_path
* RETURNS:
*   status of the operation; only fatal errors will be returned
*********************************************************************/
static status_t 
    parse_error_subtree (ses_cb_t *scb,
                         xml_msg_hdr_t *msg,
                         const xml_node_t *startnode,
                         const xml_node_t *errnode,
                         status_t errcode,
                         ncx_node_t errnodetyp,                            
                         const void *error_parm,
                         ncx_node_t intnodetyp,
                         void *intnode)
{
    status_t        res;

    res = NO_ERR;

    agt_record_error(scb, 
                     msg, 
                     NCX_LAYER_OPERATION, 
                     errcode, 
                     errnode, 
                     errnodetyp, 
                     error_parm, 
                     intnodetyp, 
                     intnode);

    if (scb && startnode) {
        res = agt_xml_skip_subtree(scb, startnode);
    }

    return res;

}  /* parse_error_subtree */


/********************************************************************
* FUNCTION parse_error_subtree_errinfo
* 
* Generate an error during parmset processing for an element
* Add rpc_err_rec_t structs to the msg->errQ
*
* INPUTS:
*   scb == session control block (NULL means call is a no-op)
*   msg = xml_msg_hdr_t from msg in progress  (NULL means call is a no-op)
*   startnode == parent start node to match on exit
*         If this is NULL then the reader will not be advanced
*   errnode == error node being processed
*         If this is NULL then the current node will be
*         used if it can be retrieved with no errors,
*         and the node has naming properties.
*   errcode == error status_t of initial internal error
*         This will be used to pick the error-tag and
*         default error message
*   errnodetyp == internal VAL_PCH node type used for error_parm
*   error_parm == pointer to attribute name or namespace name, etc.
*         used in the specific error in agt_rpcerr.c
*   intnodetyp == internal VAL_PCH node type used for intnode
*   intnode == internal VAL_PCH node used for error_path
*   errinfo == errinfo struct to use for <rpc-error> details
*
* RETURNS:
*   status of the operation; only fatal errors will be returned
*********************************************************************/
static status_t 
    parse_error_subtree_errinfo (ses_cb_t *scb,
                                 xml_msg_hdr_t *msg,
                                 const xml_node_t *startnode,
                                 const xml_node_t *errnode,
                                 status_t errcode,
                                 ncx_node_t errnodetyp,            
                                 const void *error_parm,
                                 ncx_node_t intnodetyp,
                                 void *intnode,
                                 ncx_errinfo_t *errinfo)
{
    status_t        res;

    res = NO_ERR;

    agt_record_error_errinfo(scb, 
                             msg, 
                             NCX_LAYER_OPERATION, 
                             errcode, 
                             errnode, 
                             errnodetyp, 
                             error_parm, 
                             intnodetyp, 
                             intnode, 
                             errinfo);

    if (scb && startnode) {
        res = agt_xml_skip_subtree(scb, startnode);
    }

    return res;

}  /* parse_error_subtree_errinfo */


/********************************************************************
 * FUNCTION get_xml_node
 * 
 * Get the next (or maybe current) XML node in the reader stream
 * This hack needed because xmlTextReader cannot look ahead or
 * back up during processing.
 * 
 * The YANG leaf-list is really a collection of sibling nodes
 * and there is no way to tell where it ends without reading
 * past the end of it.
 *
 * This hack relies on the fact that a top-levelleaf-list could
 * never show up in a real NETCONF PDU
 *
 * INPUTS:
 *   scb == session control block
 *   msg == xml_msg_hdr_t in progress (NULL means don't record errors)
 *   xmlnode == xml_node_t to fill in
 *   usens == TRUE if regular NS checking mode
 *         == FALSE if _nons version should be used
 *
 * OUTPUTS:
 *   *xmlnode filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    get_xml_node (ses_cb_t  *scb,
                  xml_msg_hdr_t *msg,
                  xml_node_t *xmlnode,
                  boolean usens)

{
    status_t   res;

    /* get a new node */
    if (usens) {
        res = agt_xml_consume_node(scb, 
                                   xmlnode,
                                   NCX_LAYER_OPERATION, 
                                   msg);
    } else {
        res = agt_xml_consume_node_nons(scb, 
                                        xmlnode,
                                        NCX_LAYER_OPERATION, 
                                        msg);
    }
    return res;

}   /* get_xml_node */


/********************************************************************
 * FUNCTION get_editop
 * 
 * Check the node for operation="foo" attribute
 * and convert its value to an op_editop_t enum
 *
 * INPUTS:
 *   node == xml_node_t to check
 * RETURNS:
 *   editop == will be OP_EDITOP_NONE if explicitly set,
 *             not-present, or error
 *********************************************************************/
static op_editop_t
    get_editop (const xml_node_t  *node)
{
    const xml_attr_t  *attr;

    attr = xml_find_ro_attr(node, xmlns_nc_id(), NC_OPERATION_ATTR_NAME);
    if (!attr) {
        return OP_EDITOP_NONE;
    }
    return op_editop_id(attr->attr_val);

} /* get_editop */


/********************************************************************
 * FUNCTION pick_dataclass
 * 
 * Pick the correct data class for this value node
 *
 * INPUTS:
 *   parentdc == parent data class
 *   obj == object template definition struct for the value node
 *
 * RETURNS:
 *   data class for this value node
 *********************************************************************/
static ncx_data_class_t
    pick_dataclass (ncx_data_class_t parentdc,
                    obj_template_t *obj)
{
    boolean  ret, setflag;

    setflag = FALSE;
    ret = obj_get_config_flag2(obj, &setflag);

    if (setflag) {
        return (ret) ? NCX_DC_CONFIG : NCX_DC_STATE;
    } else {
        return parentdc;
    }
    /*NOTREACHED*/

} /* pick_dataclass */


/********************************************************************
 * FUNCTION parse_any_nc
 * 
 * Parse the XML input as an 'any' type 
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_any_nc (ses_cb_t  *scb,
                  xml_msg_hdr_t *msg,
                  const xml_node_t *startnode,
                  ncx_data_class_t parentdc,
                  val_value_t  *retval)
{
    const xml_node_t        *errnode;
    status_t                 res;
    boolean                  done, getstrend, errdone;
    xml_node_t               nextnode;

    /* init local vars */
    errnode = startnode;
    res = NO_ERR;
    done = FALSE;
    getstrend = FALSE;
    errdone = FALSE;
    xml_init_node(&nextnode);

    retval->dataclass = parentdc;

    /* make sure the startnode is correct */
    switch (startnode->nodetyp) {
    case XML_NT_START:
        /* do not set the object template yet, in case
         * there is a <foo></foo> form of empty element
         * or another start (child) node
         */
        break;
    case XML_NT_EMPTY:
        /* treat this 'any' is an 'empty' data type  */
        val_init_from_template(retval, ncx_get_gen_empty());
        retval->v.boo = TRUE;
        retval->nsid = startnode->nsid;
        return NO_ERR;
    default:
        res = ERR_NCX_WRONG_NODETYP;
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, msg, startnode, errnode, res, 
                                  NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
        xml_clean_node(&nextnode);
        return res;
    }

    /* at this point have either a simple type or a complex type
     * get the next node which could be any type 
     */
    res = get_xml_node(scb, msg, &nextnode, FALSE);
    if (res != NO_ERR) {
        xml_clean_node(&nextnode);
        return res;
    }

    log_debug3("\nparse_any: expecting any node type");
    if (LOGDEBUG4) {
        xml_dump_node(&nextnode);
    }

    /* decide the base type from the child node type */
    switch (nextnode.nodetyp) {
    case XML_NT_START:
    case XML_NT_EMPTY:
        /* A nested start or empty element means the parent is
         * treated as a 'container' data type
         */
        val_init_from_template(retval, ncx_get_gen_container());
        retval->nsid = startnode->nsid;
        break;
    case XML_NT_STRING:
        /* treat this string child node as the string
         * content for the parent node
         */
        val_init_from_template(retval, ncx_get_gen_string());
        retval->nsid = startnode->nsid;
        retval->v.str = xml_strdup(nextnode.simval);
        res = (retval->v.str) ? NO_ERR : ERR_INTERNAL_MEM;
        getstrend = TRUE;
        break;
    case XML_NT_END:
        res = xml_endnode_match(startnode, &nextnode);
        if (res == NO_ERR) {
            /* treat this start + end pair as an 'empty' data type */
            val_init_from_template(retval, ncx_get_gen_empty());
            retval->v.boo = TRUE;
            retval->nsid = startnode->nsid;
            xml_clean_node(&nextnode);
            return NO_ERR;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
        errnode = &nextnode;
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, msg, startnode, errnode, res, 
                                  NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
        xml_clean_node(&nextnode);
        return res;
    }

    /* check if processing a simple type as a string */
    if (getstrend) {
        /* need to get the endnode for startnode then exit */
        xml_clean_node(&nextnode);
        res = get_xml_node(scb, msg, &nextnode, FALSE);
        if (res == NO_ERR) {
            log_debug3("\nparse_any: expecting end node for %s", 
                       startnode->qname);
            if (LOGDEBUG4) {
                xml_dump_node(&nextnode);
            }
            res = xml_endnode_match(startnode, &nextnode);
        } else {
            errdone = TRUE;
        }

        if (res == NO_ERR) {
            xml_clean_node(&nextnode);
            return NO_ERR;
        }

        errnode = &nextnode;
        if (!errdone) {
            /* add rpc-error to msg->errQ */
            (void)parse_error_subtree(scb, msg, startnode, errnode, res, 
                                      NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
        }
        xml_clean_node(&nextnode);
        return res;
    }

    /* if we get here, then the startnode is a container */
    while (!done) {
        /* At this point have a nested start node
         *  Allocate a new val_value_t for the child value node 
         */
        val_value_t *chval;
        chval = val_new_child_val(nextnode.nsid, nextnode.elname, TRUE, retval, 
                                  get_editop(&nextnode), ncx_get_gen_anyxml());
        if (!chval) {
            res = ERR_INTERNAL_MEM;
            /* add rpc-error to msg->errQ */
            (void)parse_error_subtree(scb, msg, startnode, errnode, res, 
                                      NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
            xml_clean_node(&nextnode);
            return res;
        }

        val_add_child(chval, retval);

        /* recurse through and get whatever nodes are present
         * in the child node; call it an 'any' type
         * make sure this function gets called again
         * so the namespace errors can be ignored properly ;-)
         *
         * Cannot call this function directly or the
         * XML attributes will not get processed
         */
        res = parse_btype_nc(scb, msg, ncx_get_gen_anyxml(), &nextnode, 
                             retval->dataclass, chval);
        chval->res = res;
        xml_clean_node(&nextnode);

        if (res != NO_ERR) {
            return res;
        }

        /* get the next node */
        res = get_xml_node(scb, msg, &nextnode, FALSE);
        if (res == NO_ERR) {
            log_debug3("\nparse_any: expecting start, empty, or end node");
            if (LOGDEBUG4) {
                xml_dump_node(&nextnode);
            }
            res = xml_endnode_match(startnode, &nextnode);
            if (res == NO_ERR) {
                done = TRUE;
            }            
        } else {
            /* error already recorded */
            done = TRUE;
        }
    }

    xml_clean_node(&nextnode);
    return res;

} /* parse_any_nc */


/********************************************************************
 * FUNCTION parse_enum_nc
 * 
 * Parse the XML input as a 'enumeration' type 
 * e.g..
 *
 * <foo>fred</foo>
 * 
 * These NCX variants are no longer supported:
 *    <foo>11</foo>
 *    <foo>fred(11)</foo>
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_enum_nc (ses_cb_t  *scb,
                   xml_msg_hdr_t *msg,
                   obj_template_t *obj,
                   const xml_node_t *startnode,
                   ncx_data_class_t parentdc,
                   val_value_t  *retval)
{
    const xml_node_t  *errnode;
    const xmlChar     *badval;
    xml_node_t         valnode, endnode;
    status_t           res, res2, res3;
    boolean            errdone, empty, enddone;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errnode = startnode;
    errdone = FALSE;
    empty = FALSE;
    enddone = FALSE;
    badval = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    res3 = NO_ERR;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    if (retval->editop == OP_EDITOP_DELETE ||
        retval->editop == OP_EDITOP_REMOVE) {
        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            break;
        case XML_NT_START:
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }
    } else {
        /* make sure the startnode is correct */
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj), 
                             NULL, 
                             XML_NT_START); 
    }

    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    if (res == NO_ERR && !empty) {
        log_debug3("\nparse_enum: expecting string node");
        if (LOGDEBUG4) {
            xml_dump_node(&valnode);
        }

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            break;
        case XML_NT_EMPTY:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            enddone = TRUE;
            break;
        case XML_NT_STRING:
            if (val_all_whitespace(valnode.simval) &&
                (retval->editop == OP_EDITOP_DELETE ||
                 retval->editop == OP_EDITOP_REMOVE)) {
                res = NO_ERR;
            } else {
                /* get the non-whitespace string here */
                res = val_enum_ok(obj_get_typdef(obj), 
                                  valnode.simval, 
                                  &retval->v.enu.val, 
                                  &retval->v.enu.name);
            }
            if (res == NO_ERR) {
                /* record the value even if there are errors after this */
                retval->btyp = NCX_BT_ENUM;
            } else {
                badval = valnode.simval;
            }
            break;
        case XML_NT_END:
            enddone = TRUE;
            if (retval->editop == OP_EDITOP_DELETE ||
                retval->editop == OP_EDITOP_REMOVE) {
                retval->btyp = NCX_BT_ENUM;
                /* leave value empty, OK for leaf */
            } else {
                res = ERR_NCX_INVALID_VALUE;
                errnode = &valnode;
            }
            break;
        default:
            res = SET_ERROR(ERR_NCX_WRONG_NODETYP);
            errnode = &valnode;
            enddone = TRUE;
        }

        /* get the matching end node for startnode */
        if (!enddone) {
            res2 = get_xml_node(scb, msg, &endnode, TRUE);
            if (res2 == NO_ERR) {
                log_debug3("\nparse_enum: expecting end for %s", 
                           startnode->qname);
                if (LOGDEBUG4) {
                    xml_dump_node(&endnode);
                }
                res2 = xml_endnode_match(startnode, &endnode);
                if (res2 != NO_ERR) {
                    errnode = &endnode;
                }
            } else {
                errdone = TRUE;
            }
        }
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, 
                                  msg, 
                                  startnode,
                                  errnode, 
                                  res, 
                                  NCX_NT_STRING, 
                                  badval, 
                                  NCX_NT_VAL, 
                                  retval);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_enum_nc */


/********************************************************************
 * FUNCTION parse_empty_nc
 * For NCX_BT_EMPTY
 *
 * Parse the XML input as an 'empty' or 'boolean' type 
 * e.g.:
 *
 *  <foo/>
 * <foo></foo>
 * <foo>   </foo>
 *
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_empty_nc (ses_cb_t  *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    const xml_node_t *startnode,
                    ncx_data_class_t parentdc,
                    val_value_t  *retval)
{
    const xml_node_t *errnode;
    xml_node_t        endnode;
    status_t          res;
    boolean           errdone;

    /* init local vars */
    xml_init_node(&endnode);
    errnode = startnode;
    errdone = FALSE;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    /* validate the node type and enum string or number content */
    switch (startnode->nodetyp) {
    case XML_NT_EMPTY:
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj),
                             NULL, 
                             XML_NT_NONE);
        break;
    case XML_NT_START:
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj),
                             NULL, 
                             XML_NT_NONE);
        if (res == NO_ERR) {
            res = get_xml_node(scb, msg, &endnode, TRUE);
            if (res != NO_ERR) {
                errdone = TRUE;
            } else {
                log_debug3("\nparse_empty: expecting end for %s", 
                           startnode->qname);
                if (LOGDEBUG4) {
                    xml_dump_node(&endnode);
                }
                res = xml_endnode_match(startnode, &endnode);
                if (res != NO_ERR) {
                    if (endnode.nodetyp != XML_NT_STRING ||
                        !xml_isspace_str(endnode.simval)) {
                        errnode = &endnode;
                        res = ERR_NCX_WRONG_NODETYP;
                    } else {
                        /* that was an empty string -- try again */
                        xml_clean_node(&endnode);
                        res = get_xml_node(scb, msg, &endnode, TRUE);
                        if (res == NO_ERR) {
                            log_debug3("\nparse_empty: expecting "
                                       "end for %s", 
                                       startnode->qname);
                            if (LOGDEBUG4) {
                                xml_dump_node(&endnode);
                            }
                            res = xml_endnode_match(startnode, &endnode);
                            if (res != NO_ERR) {
                                errnode = &endnode;
                            }
                        } else {
                            errdone = TRUE;
                        }
                    }
                }
            }
        }
        break;
    default:
        res = ERR_NCX_WRONG_NODETYP;
    }

    /* record the value if no errors */
    if (res == NO_ERR) {
        retval->v.boo = TRUE;
    } else if (!errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, 
                                  msg, 
                                  startnode,
                                  errnode, 
                                  res, 
                                  NCX_NT_NONE, 
                                  NULL, 
                                  NCX_NT_VAL, 
                                  retval);
    }

    xml_clean_node(&endnode);
    return res;

} /* parse_empty_nc */


/********************************************************************
 * FUNCTION parse_boolean_nc
 * 
 * Parse the XML input as a 'boolean' type 
 * e.g..
 *
 * <foo>true</foo>
 * <foo>false</foo>
 * <foo>1</foo>
 * <foo>0</foo>
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_boolean_nc (ses_cb_t  *scb,
                      xml_msg_hdr_t *msg,
                      obj_template_t *obj,
                      const xml_node_t *startnode,
                      ncx_data_class_t parentdc,
                      val_value_t  *retval)
{
    const xml_node_t  *errnode;
    const xmlChar     *badval;
    xml_node_t         valnode, endnode;
    status_t           res, res2, res3;
    boolean            errdone, empty;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errnode = startnode;
    errdone = FALSE;
    badval = NULL;
    empty = FALSE;
    res = NO_ERR;
    res2 = NO_ERR;
    res3 = NO_ERR;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    /* make sure the startnode is correct */
    if (retval->editop == OP_EDITOP_DELETE ||
        retval->editop == OP_EDITOP_REMOVE) {
        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            break;
        case XML_NT_START:
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }
    } else {
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj), 
                             NULL, 
                             XML_NT_START); 
    }

    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    if (res == NO_ERR && !empty) {

        log_debug3("\nparse_boolean: expecting string node.");
        if (LOGDEBUG4) {
            xml_dump_node(&valnode);
        }

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            break;
        case XML_NT_STRING:
            /* get the non-whitespace string here */
            if (ncx_is_true(valnode.simval)) {
                retval->v.boo = TRUE;
            } else if (ncx_is_false(valnode.simval)) {
                retval->v.boo = FALSE;
            } else {
                res = ERR_NCX_INVALID_VALUE;
                badval = valnode.simval;
            }
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
            errnode = &valnode;
        }

        /* get the matching end node for startnode */
        res2 = get_xml_node(scb, msg, &endnode, TRUE);
        if (res2 == NO_ERR) {
            log_debug3("\nparse_boolean: expecting end for %s", 
                       startnode->qname);
            if (LOGDEBUG4) {
                xml_dump_node(&endnode);
            }
            res2 = xml_endnode_match(startnode, &endnode);
            if (res2 != NO_ERR) {
                errnode = &endnode;
            }
        } else {
            errdone = TRUE;
        }
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, 
                                  msg, 
                                  startnode,
                                  errnode, 
                                  res, 
                                  NCX_NT_STRING, 
                                  badval, 
                                  NCX_NT_VAL, 
                                  retval);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_boolean_nc */


/********************************************************************
* FUNCTION parse_num_nc
* 
* Parse the XML input as a numeric data type 
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     msg == incoming RPC message
*            Errors are appended to msg->errQ
*     obj == object template for this number type
*     btyp == base type of the expected ordinal number type
*     startnode == top node of the parameter to be parsed
*            Parser function will attempt to consume all the
*            nodes until the matching endnode is reached
*     parentdc == data class of the parent node
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*    msg->errQ may be appended with new errors or warnings
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    parse_num_nc (ses_cb_t  *scb,
                  xml_msg_hdr_t *msg,
                  obj_template_t *obj,
                  ncx_btype_t  btyp,
                  const xml_node_t *startnode,
                  ncx_data_class_t parentdc,
                  val_value_t  *retval)
{
    const xml_node_t    *errnode;
    const xmlChar       *badval;
    ncx_errinfo_t       *errinfo;
    xml_node_t           valnode, endnode;
    status_t             res, res2, res3;
    boolean              errdone, empty, endtagdone;
    ncx_numfmt_t         numfmt;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    badval = NULL;
    errnode = startnode;
    errdone = FALSE;
    empty = FALSE;
    endtagdone = FALSE;
    errinfo = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    res3 = NO_ERR;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);
    
    /* make sure the startnode is correct */
    if (retval->editop == OP_EDITOP_DELETE ||
        retval->editop == OP_EDITOP_REMOVE) {
        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            break;
        case XML_NT_START:
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }
    } else {
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj), 
                             NULL, 
                             XML_NT_START); 
    }

    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    log_debug3("\nparse_num: expecting string node.");
    if (LOGDEBUG4) {
        xml_dump_node(&valnode);
    }

    if (res == NO_ERR && !empty) {
        /* validate the number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            break;
        case XML_NT_STRING:
            /* convert the non-whitespace string to a number
             * XML numbers are only allowed to be in decimal
             * or real format, not octal or hex
             */
            numfmt = ncx_get_numfmt(valnode.simval);
            if (numfmt == NCX_NF_OCTAL) {
                numfmt = NCX_NF_DEC;
            }
            if (numfmt == NCX_NF_DEC || numfmt == NCX_NF_REAL) {
                if (btyp == NCX_BT_DECIMAL64) {
                    res = ncx_convert_dec64(valnode.simval, 
                                            numfmt,
                                            obj_get_fraction_digits(obj),
                                            &retval->v.num);
                } else {
                    res = ncx_convert_num(valnode.simval, 
                                          numfmt,
                                          btyp, 
                                          &retval->v.num);
                }
            }  else {
                res = ERR_NCX_WRONG_NUMTYP;
            }
            if (res == NO_ERR) {
                res = val_range_ok_errinfo(obj_get_typdef(obj), 
                                           btyp, 
                                           &retval->v.num, 
                                           &errinfo);
            }
            if (res != NO_ERR) {
                badval = valnode.simval;
            }
            break;
        case XML_NT_END:
            /* got a start-end NULL string
             * treat as an invalid value 
             */
            res = ERR_NCX_INVALID_VALUE;
            badval = EMPTY_STRING;
            errnode = &valnode;
            endtagdone = TRUE;

            /* not trying to match the end node; 
             * skip that error; the XML parser should have
             * already complained if the wrong end tag was present
             */
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
            errnode = &valnode;
        }

        /* get the matching end node for startnode */
        if (!endtagdone) {
            res2 = get_xml_node(scb, msg, &endnode, TRUE);
            if (res2 == NO_ERR) {
                log_debug3("\nparse_num: expecting end for %s", 
                           startnode->qname);
                if (LOGDEBUG4) {
                    xml_dump_node(&endnode);
                }
                res2 = xml_endnode_match(startnode, &endnode);
                if (res2 != NO_ERR) {
                    errnode = &endnode;
                }
            } else {
                errdone = TRUE;
            }
        }
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree_errinfo(scb, 
                                          msg, 
                                          startnode,
                                          errnode, 
                                          res, 
                                          NCX_NT_STRING, 
                                          badval, 
                                          NCX_NT_VAL, 
                                          retval, 
                                          errinfo);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_num_nc */


/********************************************************************
* FUNCTION parse_string_nc
* 
* Parse the XML input as a string data type 
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     msg == incoming RPC message
*            Errors are appended to msg->errQ
*     obj == object template for this string type
*     btyp == base type of the expected ordinal number type
*     startnode == top node of the parameter to be parsed
*            Parser function will attempt to consume all the
*            nodes until the matching endnode is reached
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*    msg->errQ may be appended with new errors or warnings
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    parse_string_nc (ses_cb_t  *scb,
                     xml_msg_hdr_t *msg,
                     obj_template_t *obj,
                     ncx_btype_t  btyp,
                     const xml_node_t *startnode,
                     ncx_data_class_t parentdc,
                     val_value_t  *retval)
{
    const xml_node_t     *errnode;
    const xmlChar        *badval;
    typ_template_t       *listtyp;
    ncx_errinfo_t        *errinfo;
    obj_template_t       *targobj;
    xpath_result_t       *result; 
    xml_node_t            valnode, endnode;
    status_t              res, res2, res3;
    boolean               errdone, empty;
    ncx_btype_t           listbtyp;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errnode = startnode;
    badval = NULL;
    errdone = FALSE;
    empty = FALSE;
    errinfo = NULL;
    res2 = NO_ERR;
    res3 = NO_ERR;
    listbtyp = NCX_BT_NONE;
    listtyp = NULL;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj),
                         NULL, 
                         XML_NT_START); 
    if (res != NO_ERR) {
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj),
                             NULL, 
                             XML_NT_EMPTY); 
        if (res == NO_ERR) {
            empty = TRUE;
        }
    }

    /* get the value string node */
    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        } else if (valnode.nodetyp == XML_NT_END) {
            empty = TRUE;
        }
    }
  
    /* check empty string corner case */
    if (empty) {
        if (retval->editop == OP_EDITOP_DELETE ||
            retval->editop == OP_EDITOP_REMOVE) {
            if (obj->objtype == OBJ_TYP_LEAF) {
                res = NO_ERR;
            } else {
                res = ERR_NCX_MISSING_VAL_INST;
            }
        } else if (btyp==NCX_BT_INSTANCE_ID) {
            res = ERR_NCX_INVALID_VALUE;
        } else if (btyp==NCX_BT_SLIST || btyp==NCX_BT_BITS) {
            /* no need to check the empty list */  ;
        } else {
            /* check the empty string */
            res = val_string_ok_errinfo(obj_get_typdef(obj), 
                                        btyp, 
                                        EMPTY_STRING,
                                        &errinfo);
            retval->v.str = xml_strdup(EMPTY_STRING);
            if (!retval->v.str) {
                res = ERR_INTERNAL_MEM;
            }
        }
    }

    if (res != NO_ERR) {
        if (!errdone) {
            /* add rpc-error to msg->errQ */
            (void)parse_error_subtree_errinfo(scb, 
                                              msg, 
                                              startnode,
                                              errnode, 
                                              res, 
                                              NCX_NT_NONE, 
                                              NULL, 
                                              NCX_NT_VAL, 
                                              retval, 
                                              errinfo);
        }
        xml_clean_node(&valnode);
        return res;
    }

    if (empty) {
        res2 = NO_ERR;
        xml_clean_node(&valnode);
        if (retval->v.str == NULL) {
            retval->v.str = xml_strdup(EMPTY_STRING);
            if (retval->v.str == NULL) {
                res2 = ERR_INTERNAL_MEM;
            }
        }
        if (res2 == NO_ERR && obj_is_key(retval->obj)) {
            res2 = val_gen_key_entry(retval);
        }
        return res2;
    }


    log_debug3("\nparse_string: expecting string node.");
    if (LOGDEBUG4) {
        xml_dump_node(&valnode);
    }

    /* validate the string data type content */
    switch (valnode.nodetyp) {
    case XML_NT_START:
        res = ERR_NCX_WRONG_NODETYP_CPX;
        errnode = &valnode;
        break;
    case XML_NT_STRING:
        switch (btyp) {
        case NCX_BT_SLIST:
        case NCX_BT_BITS:
            if (btyp==NCX_BT_SLIST) {
                /* get the list of strings, then check them */
                listtyp = typ_get_listtyp(obj_get_typdef(obj));
                listbtyp = typ_get_basetype(&listtyp->typdef);
            } else if (btyp==NCX_BT_BITS) {
                listbtyp = NCX_BT_BITS;
            }

            res = ncx_set_list(listbtyp, 
                               valnode.simval, 
                               &retval->v.list);
            if (res == NO_ERR) {
                if (btyp == NCX_BT_SLIST) {
                    res = ncx_finish_list(&listtyp->typdef, 
                                          &retval->v.list);
                } else {
                    res = ncx_finish_list(obj_get_typdef(obj),
                                          &retval->v.list);
                }
            }

            if (res == NO_ERR) {
                res = val_list_ok_errinfo(obj_get_typdef(obj), 
                                          btyp, 
                                          &retval->v.list,
                                          &errinfo);
            }
            break;
        default:
            if (!obj_is_xpath_string(obj)) {
                /* check the non-whitespace string */
                res = val_string_ok_errinfo(obj_get_typdef(obj), 
                                            btyp, 
                                            valnode.simval, 
                                            &errinfo);
                break;
            }  /* else fall through and parse XPath string */
        case NCX_BT_INSTANCE_ID:
            res = NO_ERR;
            retval->xpathpcb = agt_new_xpath_pcb(scb,
                                                 valnode.simval,
                                                 &res);
            if (!retval->xpathpcb) {
                ; /* res already set */
            } else if (btyp == NCX_BT_INSTANCE_ID ||
                       obj_is_schema_instance_string(obj)) {
                /* do a first pass parsing to resolve all
                 * the prefixes and check well-formed XPath
                 */
                res = xpath_yang_validate_xmlpath(scb->reader,
                                                  retval->xpathpcb,
                                                  obj,
                                                  FALSE,
                                                  &targobj);
            } else {
                result = 
                    xpath1_eval_xmlexpr(scb->reader,
                                        retval->xpathpcb,
                                        NULL,
                                        NULL,
                                        FALSE,
                                        FALSE,
                                        &res);
                // not interested in the actual result, just the status in res!
                xpath_free_result(result);
            }
        }

        if (res != NO_ERR) {
            badval = valnode.simval;
        }

        /* record the value even if there are errors */
        switch (btyp) {
        case NCX_BT_BINARY:
            if (valnode.simval) {
                /* result is going to be less than the encoded length */
                retval->v.binary.ustr = m__getMem(valnode.simlen);
                retval->v.binary.ubufflen = valnode.simlen;
                if (!retval->v.binary.ustr) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    res = b64_decode(valnode.simval, 
                                     valnode.simlen,
                                     retval->v.binary.ustr, 
                                     retval->v.binary.ubufflen,
                                     &retval->v.binary.ustrlen);
                }
            }
            break;
        case NCX_BT_STRING:
        case NCX_BT_INSTANCE_ID:  /* make copy for now, optimize later */
        case NCX_BT_LEAFREF:
            if (valnode.simval) {
                retval->v.str = xml_strdup(valnode.simval);
                if (!retval->v.str) {
                    res = ERR_INTERNAL_MEM;
                }
            }
            break;
        case NCX_BT_SLIST:
        case NCX_BT_BITS:
            break;   /* value already set */
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    default:
        res = ERR_NCX_WRONG_NODETYP;
        errnode = &valnode;
    }

    /* get the matching end node for startnode */
    res2 = get_xml_node(scb, msg, &endnode, TRUE);
    if (res2 == NO_ERR) {
        log_debug3("\nparse_string: expecting end for %s", 
                   startnode->qname);
        if (LOGDEBUG4) {
            xml_dump_node(&endnode);
        }
        res2 = xml_endnode_match(startnode, &endnode);
        
    } else {
        errdone = TRUE;
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree_errinfo(scb, 
                                          msg, 
                                          startnode,
                                          errnode, 
                                          res, 
                                          NCX_NT_STRING,
                                          badval, 
                                          NCX_NT_VAL, 
                                          retval, 
                                          errinfo);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_string_nc */


/********************************************************************
 * FUNCTION parse_idref_nc
 * 
 * Parse the XML input as an 'identityref' type 
 * e.g..
 *
 * <foo>acme:card-type3</foo>
 * 
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_idref_nc (ses_cb_t  *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    const xml_node_t *startnode,
                    ncx_data_class_t parentdc,
                    val_value_t  *retval)
{
    const xml_node_t      *errnode;
    const xmlChar         *badval, *str;
    xml_node_t             valnode, endnode;
    status_t               res, res2, res3;
    boolean                errdone, empty, enddone;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errnode = startnode;
    errdone = FALSE;
    empty = FALSE;
    enddone = FALSE;
    badval = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    res3 = NO_ERR;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    if (retval->editop == OP_EDITOP_DELETE ||
        retval->editop == OP_EDITOP_REMOVE) {
        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            break;
        case XML_NT_START:
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
            errnode = startnode;
        }
    } else {
        /* make sure the startnode is correct */
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj), 
                             NULL, 
                             XML_NT_START); 
    }

    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    if (res == NO_ERR && !empty) {
        log_debug3("\nparse_idref: expecting string node");
        if (LOGDEBUG4) {
            xml_dump_node(&valnode);
        }

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            break;
        case XML_NT_EMPTY:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            enddone = TRUE;
            break;
        case XML_NT_STRING:
            if (val_all_whitespace(valnode.simval) &&
                (retval->editop == OP_EDITOP_DELETE ||
                 retval->editop == OP_EDITOP_REMOVE)) {
                res = NO_ERR;
            } else {
                retval->v.idref.nsid = valnode.contentnsid;

                /* get the name field and verify identity is valid
                 * for the identity base in the typdef
                 */
                str = NULL;
                res = val_idref_ok(obj_get_typdef(obj), 
                                   valnode.simval,
                                   retval->v.idref.nsid,
                                   &str, 
                                   &retval->v.idref.identity);
                if (str) {
                    retval->v.idref.name = xml_strdup(str);
                    if (!retval->v.idref.name) {
                        res = ERR_INTERNAL_MEM;
                    }
                } else {
                    /* save the name for error purposes or else
                     * a NULL string will get printed 
                     */
                    retval->v.idref.name = xml_strdup(valnode.simval);
                    if (!retval->v.idref.name) {
                        res = ERR_INTERNAL_MEM;
                    }
                }
            }
            if (res == NO_ERR) {
                /* record the value even if there are errors after this */
                retval->btyp = NCX_BT_IDREF;
            } else {
                badval = valnode.simval;
            }
            break;
        case XML_NT_END:
            enddone = TRUE;
            if (retval->editop == OP_EDITOP_DELETE ||
                retval->editop == OP_EDITOP_REMOVE) {
                retval->btyp = NCX_BT_IDREF;
            } else {
                res = ERR_NCX_INVALID_VALUE;
                errnode = &valnode;
            }
            break;
        default:
            res = SET_ERROR(ERR_NCX_WRONG_NODETYP);
            errnode = &valnode;
            enddone = TRUE;
        }

        /* get the matching end node for startnode */
        if (!enddone) {
            res2 = get_xml_node(scb, msg, &endnode, TRUE);
            if (res2 == NO_ERR) {
                log_debug3("\nparse_idref: expecting end for %s", 
                           startnode->qname);
                if (LOGDEBUG4) {
                    xml_dump_node(&endnode);
                }
                res2 = xml_endnode_match(startnode, &endnode);
                if (res2 != NO_ERR) {
                    errnode = &endnode;
                }
            } else {
                errdone = TRUE;
            }
        }
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, 
                                  msg, 
                                  startnode,
                                  errnode, 
                                  res, 
                                  NCX_NT_STRING, 
                                  badval, 
                                  NCX_NT_VAL, 
                                  retval);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_idref_nc */


/********************************************************************
 * FUNCTION parse_union_nc
 * 
 * Parse the XML input as a 'union' type 
 * e.g..
 *
 * <foo>fred</foo>
 * <foo>11</foo>
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_union_nc (ses_cb_t  *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    const xml_node_t *startnode,
                    ncx_data_class_t parentdc,
                    val_value_t  *retval)
{
    const xml_node_t    *errnode;
    const xmlChar       *badval;
    ncx_errinfo_t       *errinfo;
    xml_node_t           valnode, endnode;
    status_t             res, res2, res3;
    boolean              errdone, stopnow, empty;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errnode = startnode;
    errdone = FALSE;
    empty = FALSE;
    badval = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    res3 = NO_ERR;
    errinfo = NULL;
    stopnow = FALSE;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    /* make sure the startnode is correct */
    if (retval->editop == OP_EDITOP_DELETE ||
        retval->editop == OP_EDITOP_REMOVE) {
        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            break;
        case XML_NT_START:
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }
    } else {
        res = xml_node_match(startnode, 
                             obj_get_nsid(obj), 
                             NULL, 
                             XML_NT_START); 
    }

    if (res == NO_ERR && !empty) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, msg, &valnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
        }
    }

    if (res == NO_ERR && !empty) {
        log_debug3("\nparse_union: expecting string or number node.");
        if (LOGDEBUG4) {
            xml_dump_node(&valnode);
        }

        /* validate the node type and union node content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            errnode = &valnode;
            stopnow = TRUE;
            break;
        case XML_NT_STRING:
            /* get the non-whitespace string here */
            res = val_union_ok_errinfo(obj_get_typdef(obj), 
                                       valnode.simval, 
                                       retval, 
                                       &errinfo);
            if (res != NO_ERR) {
                badval = valnode.simval;
            }
            break;
        case XML_NT_END:
            stopnow = TRUE;
            res = val_union_ok_errinfo(obj_get_typdef(obj), 
                                       EMPTY_STRING, 
                                       retval, 
                                       &errinfo);
            if (res != NO_ERR) {
                badval = EMPTY_STRING;
            }
            break;
        default:
            stopnow = TRUE;
            SET_ERROR(ERR_INTERNAL_VAL);
            res = ERR_NCX_WRONG_NODETYP;
            errnode = &valnode;
        }

        if (stopnow) {
            res2 = val_set_simval(retval, 
                                  retval->typdef,
                                  retval->nsid,
                                  retval->name,
                                  EMPTY_STRING);
        } else {
            res = val_set_simval(retval, 
                                 retval->typdef,
                                 retval->nsid,
                                 retval->name,
                                 valnode.simval);
            if (res != NO_ERR) {
                errnode = &valnode;
            }

            /* get the matching end node for startnode */
            res2 = get_xml_node(scb, msg, &endnode, TRUE);
            if (res2 == NO_ERR) {
                log_debug3("\nparse_union: expecting end for %s", 
                           startnode->qname);
                if (LOGDEBUG4) {
                    xml_dump_node(&endnode);
                }
                res2 = xml_endnode_match(startnode, &endnode);
                if (res2 != NO_ERR) {
                    errnode = &endnode;
                }
            } else {
                errdone = TRUE;
            }
        }
    }

    /* check if this is a list index, add it now so
     * error-path info will be as complete as possible
     */
    if (obj_is_key(retval->obj)) {
        res3 = val_gen_key_entry(retval);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    if (res == NO_ERR) {
        res = res3;
    }

    /* check if any errors; record the first error */
    if ((res != NO_ERR) && !errdone) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree_errinfo(scb, 
                                          msg, 
                                          startnode,
                                          errnode, 
                                          res, 
                                          NCX_NT_STRING, 
                                          badval, 
                                          NCX_NT_VAL, 
                                          retval, 
                                          errinfo);
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_union_nc */


/********************************************************************
 * FUNCTION parse_complex_nc
 * 
 * Parse the XML input as a complex type
 *
 * Handles the following base types:
 *   NCX_BT_CONTAINER
 *   NCX_BT_LIST
 *  May also work for
 *   NCX_BT_CHOICE
 *   NCX_BT_CASE
 *
 * E.g., container:
 *
 * <foo>
 *   <a>blah</a>
 *   <b>7</b>
 *   <c/>
 * </foo>
 *
 * In an instance document, containers and lists look 
 * the same.  The validation is different of course, but the
 * parsing is basically the same.
 *
 * INPUTS:
 *   see parse_btype_nc parameter list
 *   errfromchild == address of return flag for origin of error
 *
 * OUTPUTS:
 *   *errfromchild == TRUE if the return code is from a child node
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_complex_nc (ses_cb_t  *scb,
                      xml_msg_hdr_t *msg,
                      obj_template_t *obj,
                      const xml_node_t *startnode,
                      ncx_data_class_t parentdc,
                      val_value_t  *retval,
                      boolean *errfromchild)
{
    const xml_node_t     *errnode;
    obj_template_t *chobj, *topchild, *curchild;
    const agt_profile_t  *profile;
    xml_node_t            chnode;
    status_t              res, res2, retres;
    boolean               done, errdone, empty, xmlorder;
    ncx_btype_t           chbtyp;

    /* setup local vars */
    *errfromchild = FALSE;
    errnode = startnode;
    chobj = NULL;
    topchild = NULL;
    curchild = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    retres = NO_ERR;
    done = FALSE;
    errdone = FALSE;
    empty = FALSE;
    chbtyp = NCX_BT_NONE;

    profile = agt_get_profile();
    if (!profile) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    xmlorder = profile->agt_xmlorder;

    val_init_from_template(retval, obj);
    retval->dataclass = pick_dataclass(parentdc, obj);

    /* do not really need to validate the start node type
     * since it is probably a virtual container at the
     * very start, and after that, a node match must have
     * occurred to call this function recursively
     */
    switch (startnode->nodetyp) {
    case XML_NT_START:
        break;
    case XML_NT_EMPTY:
        empty = TRUE;
        break;
    case XML_NT_STRING:
    case XML_NT_END:
        res = ERR_NCX_WRONG_NODETYP_SIM;
        break;
    default:
        res = ERR_NCX_WRONG_NODETYP;
    }

    if (res != NO_ERR) {
        /* add rpc-error to msg->errQ */
        (void)parse_error_subtree(scb, msg, startnode, errnode, res, 
                                  NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
        return res;
    }

    /* start setting up the return value */
    retval->editop = get_editop(startnode);

    if (empty) {
        return NO_ERR;
    }

    /* setup the first child in the complex object
     * Allowed be NULL in some cases so do not check
     * the result yet
     */
    chobj = obj_first_child(obj);

    xml_init_node(&chnode);

    /* go through each child node until the parent end node */
    while (!done) {
        /* init per-loop vars */
        val_value_t *chval;
        res2 = NO_ERR;
        errdone = FALSE;
        empty = FALSE;

        /* get the next node which should be a child or end node */
        res = get_xml_node(scb, msg, &chnode, TRUE);
        if (res != NO_ERR) {
            errdone = TRUE;
            if (res != ERR_NCX_UNKNOWN_NS) {
                done = TRUE;
            }
        } else {
            log_debug3("\nparse_complex: expecting start-child "
                       "or end node.");
            if (LOGDEBUG4) {
                xml_dump_node(&chnode);
            }

            /* validate the child member node type */
            switch (chnode.nodetyp) {
            case XML_NT_START:
            case XML_NT_EMPTY:
                /* any namespace OK for now, check it in get_child_node */
                break;
            case XML_NT_STRING:
                res = ERR_NCX_WRONG_NODETYP_SIM;
                errnode = startnode;
                break;
            case XML_NT_END:
                res = xml_endnode_match(startnode, &chnode);
                if (res != NO_ERR) {
                    errnode = &chnode;
                } else {
                    /* no error exit */
                    done = TRUE;
                    continue;
                }
                break;
            default:
                res = ERR_NCX_WRONG_NODETYP;
                errnode = &chnode;
            }
        }

        /* if we get here, there is a START or EMPTY node
         * that could be a valid child node
         *
         * if xmlorder enforced then check if the 
         * node is the correct child node
         *
         * if no xmlorder, then check for any valid child
         */
        if (res==NO_ERR) {
            res = obj_get_child_node(obj, chobj, &chnode, xmlorder,
                                     NULL, &topchild, &curchild);
            if (res != NO_ERR) {
                /* have no expected child element to match to 
                 * this XML node
                 * if xmlorder == TRUE, then this could indicate 
                 * an extra instance of the last child node
                 */
                res = ERR_NCX_UNKNOWN_OBJECT;
                errnode = &chnode;
            }
        }

        /* try to setup a new child node */
        if (res == NO_ERR) {
            /* save the child base type */
            chbtyp = obj_get_basetype(curchild);

            /* at this point, the 'curchild' template matches the
             * 'chnode' namespace and name;
             * Allocate a new val_value_t for the child value node
             */
            chval = val_new_child_val(obj_get_nsid(curchild),
                                      obj_get_name(curchild), 
                                      FALSE, 
                                      retval, 
                                      get_editop(&chnode),
                                      curchild);
            if (!chval) {
                res = ERR_INTERNAL_MEM;
            }
        }

        /* check any errors in setting up the child node */
        if (res != NO_ERR) {
            /* try to skip just the child node sub-tree */
            if (res != ERR_XML_READER_EOF) {
                if (errdone) {
                    if (scb) {
                        res2 = agt_xml_skip_subtree(scb, &chnode);
                    }
                } else {
                    /* add rpc-error to msg->errQ */
                    res2 = parse_error_subtree(scb, msg, &chnode, errnode, 
                                               res, NCX_NT_NONE, NULL, 
                                               NCX_NT_VAL, retval);
                }
            }
            xml_clean_node(&chnode);
            if (res2 != NO_ERR) {
                /* skip child didn't work, now skip the entire value subtree */
                (void)agt_xml_skip_subtree(scb, startnode);
                return res;
            } else {
                /* skip child worked, go on to next child, parse for errors */
                retres = res;
                if (NEED_EXIT(res) || res==ERR_XML_READER_EOF) {
                    done = TRUE;
                }
                continue;
            }
        }

        /* recurse through and get whatever nodes are present
         * in the child node
         */
        val_add_child(chval, retval);
        res = parse_btype_nc(scb, msg, curchild, &chnode, 
                             retval->dataclass, chval);
        // chval->res = res; already done
        if (res != NO_ERR) {
            retres = res;
            *errfromchild = TRUE;
            if (NEED_EXIT(res) || res==ERR_XML_READER_EOF) {
                done = TRUE;
                continue;
            }
        }

        /* setup the next child unless the current child
         * is a list
         */
        if (chbtyp != NCX_BT_LIST) {
            /* setup next child if the cur child is 0 - 1 instance */
            switch (obj_get_iqualval(curchild)) {
            case NCX_IQUAL_ONE:
            case NCX_IQUAL_OPT:
                if (chobj) {
                    chobj = obj_next_child(chobj);
                }
                break;
            default:
                break;
            }
        }
        xml_clean_node(&chnode);

        /* check if this is the special internal load-config RPC method,
         * in which case there will not be an end tag for the <load-config>
         * start tag passed to this function.  Need to quick exit
         * to prevent reading past the end of the XML config file,
         * which just contains the <config> element
         *
         * know there is just one parameter named <config> so just
         * go through the loop once because the </load-config> tag
         * will not be present
         */
        if ((startnode->nsid == 0 ||
             startnode->nsid == xmlns_nc_id() ||
             startnode->nsid == xmlns_ncx_id() ||
             startnode->nsid == xmlns_find_ns_by_name(NC_MODULE)) &&
            !xml_strcmp(startnode->elname, NCX_EL_LOAD_CONFIG)) {
            done = TRUE;
        }
    }

    xml_clean_node(&chnode);
    return retres;

} /* parse_complex_nc */


/********************************************************************
* FUNCTION parse_metadata_nc
* 
* Parse all the XML attributes in the specified xml_node_t struct
*
* Only XML_NT_START or XML_NT_EMPTY nodes will have attributes
*
* INPUTS:
*     scb == session control block
*     msg == incoming RPC message
*            Errors are appended to msg->errQ
*     obj == object template containing meta-data definition
*     nserr == TRUE if namespace errors should be checked
*           == FALSE if not, and any attribute is accepted 
*     node == node of the parameter maybe with attributes to be parsed
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*    msg->errQ may be appended with new errors or warnings
*    retval->editop contains the last value of the operation attribute
*      seen, if any; will be OP_EDITOP_NONE if not set
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    parse_metadata_nc (ses_cb_t *scb,
                       xml_msg_hdr_t *msg,
                       obj_template_t *obj,
                       const xml_node_t *node,
                       boolean nserr,
                       val_value_t  *retval)
{
    obj_metadata_t       *meta;
    typ_def_t            *metadef;
    xml_attr_t           *attr;
    val_value_t          *metaval;
    xmlns_id_t            ncid, yangid, wdaid;
    status_t              res, retres;


    retres = NO_ERR;
    ncid =  xmlns_nc_id();
    yangid =  xmlns_yang_id();
    wdaid = xmlns_wda_id();

    /* go through all the attributes in the node and convert
     * to val_value_t structs
     * find the correct typedef to match each attribute
     */
    for (attr = xml_get_first_attr(node);
         attr != NULL;
         attr = xml_next_attr(attr)) {

        if (attr->attr_dname && 
            !xml_strncmp(attr->attr_dname, 
                         XMLNS, xml_strlen(XMLNS))) {
            /* skip this 'xmlns' attribute */
            continue;   
        }

        /* init per-loop locals */
        res = NO_ERR;
        metadef = NULL;

        if (retval->editvars == NULL) {
            res = val_new_editvars(retval);
            if (res != NO_ERR) {
                agt_record_error(scb, msg, NCX_LAYER_CONTENT, res, NULL,
                                 NCX_NT_NONE, NULL, NCX_NT_VAL, retval);
                return res;
            }
        }

        /* check qualified and unqualified operation attribute,
         * then the 'xmlns' attribute, then a defined attribute
         */
        if (val_match_metaval(attr, ncid, NC_OPERATION_ATTR_NAME)) {
            retval->editop = op_editop_id(attr->attr_val);
            if (retval->editop == OP_EDITOP_NONE) {
                res = ERR_NCX_INVALID_VALUE;
            } else {
                retval->editvars->operset = TRUE;
                continue;
            }
        } else if (val_match_metaval(attr, yangid, YANG_K_INSERT)) {
            retval->editvars->insertop = op_insertop_id(attr->attr_val);
            if (retval->editvars->insertop == OP_INSOP_NONE) {
                res = ERR_NCX_INVALID_VALUE;
            } else {
                retval->editvars->operset = TRUE;
                continue;
            }
        } else if (val_match_metaval(attr, yangid, YANG_K_KEY)) {
            if (!retval->editvars->insertstr) {
                retval->editvars->iskey = TRUE;
                retval->editvars->insertstr = xml_strdup(attr->attr_val);
                if (!retval->editvars->insertstr) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    /* hand off the XPath parser control block
                     * which has already resolved the XML
                     * namespace prefixes to NSID numbers
                     */
                    retval->editvars->insertxpcb = attr->attr_xpcb;
                    attr->attr_xpcb = NULL;
                    retval->editvars->operset = TRUE;
                    continue;
                }
            } else {
                /* insert and key both entered */
                res = ERR_NCX_EXTRA_ATTR;
            }
        } else if (val_match_metaval(attr, yangid, YANG_K_VALUE)) {
            if (!retval->editvars->insertstr) {
                retval->editvars->iskey = FALSE;
                retval->editvars->insertstr = xml_strdup(attr->attr_val);
                if (!retval->editvars->insertstr) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    retval->editvars->operset = TRUE;
                    continue;
                }
            } else {
                /* insert and key both entered */
                res = ERR_NCX_EXTRA_ATTR;
            }
        } else if (val_match_metaval(attr, wdaid, YANG_K_DEFAULT)) {
            if (ncx_is_true(attr->attr_val)) {
                val_set_withdef_default(retval);
                continue;
            } else if (ncx_is_false(attr->attr_val)) {
                continue;
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            /* find the attribute definition in this typdef */
            meta = obj_find_metadata(obj, attr->attr_name);
            if (meta) {
                metadef = meta->typdef;
            } else if (!nserr) {
                metadef = typ_get_basetype_typdef(NCX_BT_STRING);
            }
        }

        if (metadef) {
            /* alloc a new value struct for the attribute
             * do not save the operation, insert, key, or
             * value attributes from NETCONF and YANG
             * these must not persist in the database contents
             */
            metaval = val_new_value();
            if (!metaval) {
                res = ERR_INTERNAL_MEM;
            } else {
                /* parse the attribute string against the typdef */
                res = val_parse_meta(metadef, attr, metaval);
                if (res == NO_ERR) {
                    dlq_enque(metaval, &retval->metaQ);
                } else {
                    val_free_value(metaval);
                }
            }
        } else if (res == NO_ERR) {
            res = ERR_NCX_UNKNOWN_ATTRIBUTE;
        }

        if (res != NO_ERR) {
            agt_record_attr_error(scb, 
                                  msg, 
                                  NCX_LAYER_OPERATION, 
                                  res,  
                                  attr, 
                                  node, 
                                  NULL, 
                                  NCX_NT_VAL, 
                                  retval);
            CHK_EXIT(res, retres);
        }
    }
    return retres;

} /* parse_metadata_nc */


/********************************************************************
* FUNCTION metadata_inst_check
* 
* Validate that all the XML attributes in the specified 
* xml_node_t struct are pesent in appropriate numbers
*
* Since attributes are unordered, they all have to be parsed
* before they can be checked for instance count
*
* INPUTS:
*     scb == session control block
*     msg == incoming RPC message
*            Errors are appended to msg->errQ
*     val == value to check for metadata errors
*     
* OUTPUTS:
*    msg->errQ may be appended with new errors or warnings
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    metadata_inst_check (ses_cb_t *scb,
                         xml_msg_hdr_t *msg,
                         val_value_t  *val)
{
    obj_metadata_t *meta;
    obj_type_t            objtype;
    uint32                insertcnt, cnt, checkcnt;
    status_t              res = NO_ERR;
    xmlns_qname_t         qname;
    xmlns_id_t            yangid = xmlns_yang_id();
    op_insertop_t         insop = OP_INSOP_NONE;

    if (!val->obj) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    objtype = val->obj->objtype;

    /* figure out how many key/value attributes are allowed */
    if (val->editvars) {
        insop = val->editvars->insertop;
    }

    switch (insop) {
    case OP_INSOP_NONE:
        insertcnt = 0;
        checkcnt = 1;
        break;
    case OP_INSOP_FIRST:
    case OP_INSOP_LAST:
        insertcnt = 1;
        checkcnt = 0;
        break;
    case OP_INSOP_BEFORE:
    case OP_INSOP_AFTER:
        insertcnt = 1;
        checkcnt = 1;
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* check if delete, these attrs not allowed at all then */
    if (val->editop == OP_EDITOP_DELETE ||
        val->editop == OP_EDITOP_REMOVE) {
        checkcnt = 0;
    }

    /* check if 'insert' entered and not allowed */
    if (insertcnt && 
        (!(objtype==OBJ_TYP_LIST || objtype==OBJ_TYP_LEAF_LIST))) {
        res = ERR_NCX_EXTRA_ATTR;
        qname.nsid = yangid;
        qname.name = YANG_K_INSERT;
        agt_record_error(scb, 
                         msg, 
                         NCX_LAYER_CONTENT, 
                         res, 
                         NULL, 
                         NCX_NT_QNAME, 
                         &qname, 
                         NCX_NT_VAL, 
                         val);
    }

    /* check the inst count of the YANG attributes
     * key attribute allowed for list only
     */
    if (res == NO_ERR && val->editvars && val->editvars->insertstr) {
        if (val->editvars->iskey) {
            if (!checkcnt || val->obj->objtype != OBJ_TYP_LIST) {
                res = ERR_NCX_EXTRA_ATTR;
                qname.nsid = yangid;
                qname.name = YANG_K_KEY;
                agt_record_error(scb, 
                                 msg, 
                                 NCX_LAYER_CONTENT, 
                                 res, 
                                 NULL, 
                                 NCX_NT_QNAME, 
                                 &qname, 
                                 NCX_NT_VAL, 
                                 val);
            }
        } else {
            if (!checkcnt || val->obj->objtype != OBJ_TYP_LEAF_LIST) {
                res = ERR_NCX_EXTRA_ATTR;
                qname.nsid = yangid;
                qname.name = YANG_K_VALUE;
                agt_record_error(scb, 
                                 msg, 
                                 NCX_LAYER_CONTENT,
                                 res, 
                                 NULL,
                                 NCX_NT_QNAME,
                                 &qname, 
                                 NCX_NT_VAL,
                                 val);
            }
        }
    }

    /* check missing value or key attributes */
    if (res == NO_ERR && val->editvars && !val->editvars->insertstr &&
        (val->editvars->insertop==OP_INSOP_BEFORE ||
         val->editvars->insertop==OP_INSOP_AFTER)) {

        if (val->obj->objtype == OBJ_TYP_LEAF_LIST) {
            /* the value attribute is missing */
            res = ERR_NCX_MISSING_ATTR;
            qname.nsid = yangid;
            qname.name = YANG_K_VALUE;
            agt_record_error(scb,
                             msg,
                             NCX_LAYER_CONTENT,
                             res, 
                             NULL,
                             NCX_NT_QNAME,
                             &qname, 
                             NCX_NT_VAL,
                             val);
        } else if (val->obj->objtype == OBJ_TYP_LIST) {
            /* the key attribute is missing */
            res = ERR_NCX_MISSING_ATTR;
            qname.nsid = yangid;
            qname.name = YANG_K_KEY;
            agt_record_error(scb,
                             msg,
                             NCX_LAYER_CONTENT,
                             res, 
                             NULL,
                             NCX_NT_QNAME,
                             &qname, 
                             NCX_NT_VAL,
                             val);
        }
    }

    /* check more than 1 of each specified metaval
     * probably do not need to do this because the
     * xmlTextReader parser might barf on the invalid XML
     * way before this test gets to run
     */
    for (meta = obj_first_metadata(val->obj);
         meta != NULL;
         meta = obj_next_metadata(meta)) {

        /* check the instance qualifier from the metadata
         * continue the loop if there is no error
         */
        cnt = val_metadata_inst_count(val, meta->nsid, meta->name);
        if (cnt > 1) {
            res = ERR_NCX_EXTRA_ATTR;
            qname.nsid = meta->nsid;
            qname.name = meta->name;
            agt_record_error(scb,
                             msg,
                             NCX_LAYER_CONTENT, 
                             res,
                             NULL,
                             NCX_NT_QNAME,
                             &qname, 
                             NCX_NT_VAL,
                             val);
        }
    }

    return res;

} /* metadata_inst_check */


/********************************************************************
* FUNCTION parse_btype_nc
* 
* Switch to dispatch to specific base type handler
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     msg == incoming RPC message
*            Errors are appended to msg->errQ
*     obj == object template to use for parsing
*     startnode == top node of the parameter to be parsed
*            Parser function will attempt to consume all the
*            nodes until the matching endnode is reached
*     parentdc == data class of the parent node, which will get
*                 used if it is not explicitly set for the typdef
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*    msg->errQ may be appended with new errors or warnings
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    parse_btype_nc (ses_cb_t  *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    const xml_node_t *startnode,
                    ncx_data_class_t parentdc,
                    val_value_t  *retval)
{
    ncx_btype_t  btyp;
    status_t     res, res2, res3;
    boolean      nserr, errfromchild = FALSE;

    btyp = obj_get_basetype(obj);

    /* get the attribute values from the start node */
    retval->editop = OP_EDITOP_NONE;
    retval->nsid = startnode->nsid;

    /* check namespace errors except if the type is ANY */
    nserr = (btyp != NCX_BT_ANY);

    if (retval->obj == NULL) {
        /* in case the parse_metadata_nc function
         * wants to record an error
         */
        retval->obj = ncx_get_gen_anyxml();
    }

    /* parse the attributes, if any */
    res2 = parse_metadata_nc(scb, msg, obj, startnode, nserr, retval);

    /* continue to parse the startnode depending on the base type 
     * to record as many errors as possible
     */
    switch (btyp) {
    case NCX_BT_ANY:
        res = parse_any_nc(scb, msg, startnode, parentdc, retval);
        break;
    case NCX_BT_ENUM:
        res = parse_enum_nc(scb, msg, obj, startnode, parentdc, retval);
        break;
    case NCX_BT_EMPTY:
        res = parse_empty_nc(scb, msg, obj, startnode, parentdc, retval);
        break;
    case NCX_BT_BOOLEAN:
        res = parse_boolean_nc(scb, msg, obj, startnode, parentdc, retval);
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        res = parse_num_nc(scb, msg, obj, btyp, startnode, parentdc, retval);
        break;
    case NCX_BT_LEAFREF:
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
    case NCX_BT_INSTANCE_ID:
        res = parse_string_nc(scb, msg, obj, btyp, startnode, parentdc, retval);
        break;
    case NCX_BT_IDREF:
        res = parse_idref_nc(scb, msg, obj, startnode, parentdc, retval);
        break;
    case NCX_BT_UNION:
        res = parse_union_nc(scb, msg, obj, startnode, parentdc, retval);
        break;
    case NCX_BT_CONTAINER:
    case NCX_BT_LIST:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        res = parse_complex_nc(scb, msg, obj, startnode, parentdc, 
                               retval, &errfromchild);
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* set the config flag for this value */
    res3 = NO_ERR;

    if (res == NO_ERR  && res2 == NO_ERR) {
        /* this has to be after the retval typdef is set */
        res3 = metadata_inst_check(scb, msg, retval);
    }

    if (res != NO_ERR) {
        if (!errfromchild) {
            retval->res = res;
        }
        return res;
    } else if (res2 != NO_ERR) {
        if (!errfromchild) {
            retval->res = res;
        }
        retval->res = res2;
        return res2;
    }

    if (!errfromchild) {
        retval->res = res3;
    }
    return res3;

} /* parse_btype_nc */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_val_parse_nc
* 
* Parse NETCONF PDU sub-contents into value fields
* This module does not enforce complex type completeness.
* Different subsets of configuration data are permitted
* in several standard (and any proprietary) RPC methods
*
* Makes sure that only allowed value strings
* or child nodes (and their values) are entered.
*
* Defaults are not added to any objects
* Missing objects are not checked
*
* A seperate parsing phase is used to fully validate the input
* contained in the returned val_value_t struct.
*
* This parsing phase checks that simple types are complete
* and child members of complex types are valid (but maybe 
* missing or incomplete child nodes.
*
* Note that the NETCONF inlineFilterType will be parsed
* correctly because it is type 'anyxml'.  This type is
* parsed as a struct, and no validation other well-formed
* XML is done.
*
* INPUTS:
*     scb == session control block
*     msg == incoming RPC message
*     obj == obj_template_t for the object to parse
*     startnode == top node of the parameter to be parsed
*     parentdc == parent data class
*                 For the first call to this function, this will
*                 be the data class of a parameter.
*     retval == address of initialized return value
*     
* OUTPUTS:
*    *status will be filled in
*     msg->errQ may be appended with new errors
*
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_val_parse_nc (ses_cb_t  *scb,
                      xml_msg_hdr_t *msg,
                      obj_template_t *obj,
                      const xml_node_t *startnode,
                      ncx_data_class_t  parentdc,
                      val_value_t  *retval)
{
    status_t res;

#ifdef DEBUG
    if (!scb || !msg || !obj || !startnode || !retval) {
        /* non-recoverable error */
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (LOGDEBUG3) {
        log_debug3("\nagt_val_parse: %s:%s btyp:%s", 
                   obj_get_mod_prefix(obj),
                   obj_get_name(obj), 
                   tk_get_btype_sym(obj_get_basetype(obj)));
    }

    /* get the element values */
    res = parse_btype_nc(scb, msg, obj, startnode, parentdc, retval);
    return res;

}  /* agt_val_parse_nc */


#ifdef DEBUG
/********************************************************************
* FUNCTION agt_val_parse_test
* 
* scaffold code to get agt_val_parse tested 
*
* INPUTS:
*   testfile == NETCONF PDU in a test file
*
*********************************************************************/
void
    agt_val_parse_test (const char *testfile)
{
    ses_cb_t  *scb;
    status_t   res;

    /* create a dummy session control block */
    scb = agt_ses_new_dummy_session();
    if (!scb) {
        SET_ERROR(ERR_INTERNAL_MEM);
        return;
    }

    /* open the XML reader */
    res = xml_get_reader_from_filespec(testfile, &scb->reader);
    if (res != NO_ERR) {
        SET_ERROR(res);
        agt_ses_free_dummy_session(scb);
        return;
    }

    /* dispatch the test PDU, note scb might be deleted! */
    agt_top_dispatch_msg(&scb);

    /* clean up and exit */
    if ( scb )
    {
        agt_ses_free_dummy_session(scb);
    }

}  /* agt_val_parse_test */
#endif



/* END file agt_val_parse.c */
