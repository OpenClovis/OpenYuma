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
/*  FILE: mgr_val_parse.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11feb06      abb      begun; hack, clone agent code and remove
                      all the rpc-error handling code; later a proper
                      libxml2 docPtr interface will be used instead

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
#include "b64.h"
#include "cfg.h"
#include "def_reg.h"
#include "dlq.h"
#include "log.h"
#include "mgr.h"
#include "mgr_val_parse.h"
#include "mgr_xml.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncx_list.h"
#include "ncxconst.h"
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

#ifdef DEBUG
#define MGR_VAL_PARSE_DEBUG 1
#endif


/* forward declaration for recursive calls */
static status_t 
    parse_btype (ses_cb_t  *scb,
                 obj_template_t *obj,
                 const xml_node_t *startnode,
                 val_value_t  *retval);

static status_t 
    parse_btype_split (ses_cb_t  *scb,
                       obj_template_t *obj,
                       obj_template_t *output,
                       const xml_node_t *startnode,
                       val_value_t  *retval);


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
 *   xmlnode == xml_node_t to fill in
 *
 * OUTPUTS:
 *   *xmlnode filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    get_xml_node (ses_cb_t  *scb,
                  xml_node_t *xmlnode)
{
    status_t   res;

    res = mgr_xml_consume_node(scb->reader, xmlnode);
    return res;

}   /* get_xml_node */


/********************************************************************
 * FUNCTION gen_index_chain
 * 
 * Create an index chain for the just-parsed table or container struct
 *
 * INPUTS:
 *   instart == first obj_key_t in the chain to process
 *   val == the just parsed list entry with the childQ containing
 *          nodes to check as index nodes
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    gen_index_chain (obj_key_t *instart,
                     val_value_t *val)
{
    obj_key_t    *key;
    status_t      res = NO_ERR;

    /* 0 or more index components expected */
    for (key = instart; key != NULL; key = obj_next_key(key)) {
        res = val_gen_index_comp(key, val);
        if (res != NO_ERR) {
            return res;
        }
    }

    return res;

}   /* gen_index_chain */


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
 * FUNCTION parse_any
 * 
 * Parse the XML input as an 'any' type 
 *
 * INPUTS:
 *   scb == session in progress
 *   startnode == XML to start processing at
 *   retval == address of value struct to fill in
 *
 * OUTPUTS:
 *   *retval filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_any (ses_cb_t  *scb,
               const xml_node_t *startnode,
               val_value_t  *retval)
{
    val_value_t             *chval;
    status_t                 res, res2;
    boolean                  done, getstrend;
    xml_node_t               nextnode;

    /* init local vars */
    chval = NULL;
    res = NO_ERR;
    res2 = NO_ERR;
    done = FALSE;
    getstrend = FALSE;

    xml_init_node(&nextnode);

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
    }

    if (res == NO_ERR) {
        /* at this point have either a simple type or a complex type
         * get the next node which could be any type 
         */
        res = mgr_xml_consume_node_nons(scb->reader, &nextnode);
    }

    if (res == NO_ERR) {

#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_any: expecting any node type");
            xml_dump_node(&nextnode);
        }
#endif

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
    }

    /* check if processing a simple type as a string */
    if (getstrend) {
        /* need to get the endnode for startnode then exit */
        xml_clean_node(&nextnode);
        res2 = mgr_xml_consume_node_nons(scb->reader, &nextnode);
        if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nparse_any: expecting end node for %s", 
                           startnode->qname);
                xml_dump_node(&nextnode);
            }
#endif
            res2 = xml_endnode_match(startnode, &nextnode);
        }
    }

    /* check if there were any errors in the startnode */
    if (res != NO_ERR || res2 != NO_ERR) {
        xml_clean_node(&nextnode);
        return (res==NO_ERR) ? res2 : res;
    }

    if (getstrend) {
        xml_clean_node(&nextnode);
        return NO_ERR;
    }

    /* if we get here, then the startnode is a container */
    while (!done) {
        /* At this point have a nested start node
         *  Allocate a new val_value_t for the child value node 
         */
        res = NO_ERR;
        chval = val_new_child_val(nextnode.nsid, 
                                  nextnode.elname, 
                                  TRUE, 
                                  retval, 
                                  get_editop(&nextnode),
                                  ncx_get_gen_anyxml());
        if (!chval) {
            res = ERR_INTERNAL_MEM;
        }

        /* check any error setting up the child node */
        if (res == NO_ERR) {
            /* recurse through and get whatever nodes are present
             * in the child node; call it an 'any' type
             * make sure this function gets called again
             * so the namespace errors can be ignored properly ;-)
             *
             * Cannot call this function directly or the
             * XML attributes will not get processed
             */
            res = parse_btype(scb, 
                              ncx_get_gen_anyxml(), 
                              &nextnode, 
                              chval);
            chval->res = res;
            xml_clean_node(&nextnode);
            val_add_child(chval, retval);
            chval = NULL;
        }

        /* record any error, if not already done */
        if (res != NO_ERR) {
            xml_clean_node(&nextnode);
            if (chval) {
                val_free_value(chval);
            }
            return res;
        }

        /* get the next node */
        res = mgr_xml_consume_node_nons(scb->reader, &nextnode);
        if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nparse_any: expecting start, empty, or end node");
                xml_dump_node(&nextnode);
            }
#endif
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

} /* parse_any */


/********************************************************************
 * FUNCTION parse_enum
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
 *   scb == session in progress
 *   obj == object template to parse against
 *   startnode == XML to start processing at
 *   retval == address of value struct to fill in
 *
 * OUTPUTS:
 *   *retval filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_enum (ses_cb_t  *scb,
                obj_template_t *obj,
                const xml_node_t *startnode,
                val_value_t  *retval)
{
    xml_node_t         valnode, endnode;
    status_t           res, res2;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    res2 = NO_ERR;

    val_init_from_template(retval, obj);

    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj), 
                         NULL, 
                         XML_NT_START); 
    if (res == NO_ERR) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, &valnode);
    }

    if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_enum: expecting string node");
            xml_dump_node(&valnode);
        }
#endif

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            break;
        case XML_NT_STRING:
            /* get the non-whitespace string here */
            res = val_enum_ok(obj_get_typdef(obj), 
                              valnode.simval, 
                              &retval->v.enu.val, 
                              &retval->v.enu.name);
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }

        /* get the matching end node for startnode */
        res2 = get_xml_node(scb, &endnode);
        if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nparse_enum: expecting end for %s", 
                           startnode->qname);
                xml_dump_node(&endnode);
            }
#endif
            res2 = xml_endnode_match(startnode, &endnode);
        }
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_enum */


/********************************************************************
 * FUNCTION parse_empty
 * For NCX_BT_EMPTY
 *
 * Parse the XML input as an 'empty' or 'boolean' type 
 * e.g.:
 *
 *  <foo/>
 * <foo></foo>
 * <foo>   </foo>
 *
 * INPUTS:
 *   scb == session in progress
 *   obj == object template to parse against
 *   startnode == XML to start processing at
 *   retval == address of value struct to fill in
 *
 * OUTPUTS:
 *   *retval filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_empty (ses_cb_t  *scb,
                 obj_template_t *obj,
                 const xml_node_t *startnode,
                 val_value_t  *retval)
{
    xml_node_t        endnode;
    status_t          res;

    /* init local vars */
    xml_init_node(&endnode);

    val_init_from_template(retval, obj);

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
            res = get_xml_node(scb, &endnode);
            if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
                if (LOGDEBUG4) {
                    log_debug4("\nparse_empty: expecting end for %s", 
                       startnode->qname);
                    xml_dump_node(&endnode);
                }
#endif
                res = xml_endnode_match(startnode, &endnode);
                if (res != NO_ERR) {
                    if (endnode.nodetyp != XML_NT_STRING ||
                        !xml_isspace_str(endnode.simval)) {
                        res = ERR_NCX_WRONG_NODETYP;
                    } else {
                        /* that was an empty string -- try again */
                        xml_clean_node(&endnode);
                        res = get_xml_node(scb, &endnode);
                        if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
                            if (LOGDEBUG4) {
                                log_debug4("\nparse_empty: expecting "
                                           "end for %s", 
                                           startnode->qname);
                                xml_dump_node(&endnode);
                            }
#endif
                            res = xml_endnode_match(startnode, &endnode);
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
    }

    xml_clean_node(&endnode);
    return res;

} /* parse_empty */


/********************************************************************
 * FUNCTION parse_boolean
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
 *   scb == session in progress
 *   obj == object template to parse against
 *   startnode == XML to start processing at
 *   retval == address of value struct to fill in
 *
 * OUTPUTS:
 *   *retval filled in
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t 
    parse_boolean (ses_cb_t  *scb,
                   obj_template_t *obj,
                   const xml_node_t *startnode,
                   val_value_t  *retval)
{
    xml_node_t         valnode, endnode;
    status_t           res, res2;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    res2 = NO_ERR;

    val_init_from_template(retval, obj);

    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj), 
                         NULL, 
                         XML_NT_START); 
    if (res == NO_ERR) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, &valnode);
    }

    if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_boolean: expecting string node.");
            xml_dump_node(&valnode);
        }
#endif

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            break;
        case XML_NT_STRING:
            /* get the non-whitespace string here */
            if (ncx_is_true(valnode.simval)) {
                retval->v.boo = TRUE;
            } else if (ncx_is_false(valnode.simval)) {
                retval->v.boo = FALSE;
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }

        /* get the matching end node for startnode */
        res2 = get_xml_node(scb, &endnode);
        if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
            if (LOGDEBUG4) {
                log_debug4("\nparse_boolean: expecting end for %s", 
                           startnode->qname);
                xml_dump_node(&endnode);
            }
#endif
            res2 = xml_endnode_match(startnode, &endnode);
        }
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_boolean */


/********************************************************************
* FUNCTION parse_num
* 
* Parse the XML input as a numeric data type 
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     obj == object template for this number type
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
    parse_num (ses_cb_t  *scb,
               obj_template_t *obj,
               ncx_btype_t  btyp,
               const xml_node_t *startnode,
               val_value_t  *retval)
{
    ncx_errinfo_t       *errinfo;
    xml_node_t           valnode, endnode;
    status_t             res, res2;
    ncx_numfmt_t         numfmt;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    errinfo = NULL;

    val_init_from_template(retval, obj);
    
    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj),
                         NULL, 
                         XML_NT_START); 
    if (res == NO_ERR) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, &valnode);
    }

    if (res != NO_ERR) {
        /* fatal error */
        xml_clean_node(&valnode);
        return res;
    }

#ifdef MGR_VAL_PARSE_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\nparse_num: expecting string node.");
        xml_dump_node(&valnode);
    }
#endif

    /* validate the number content */
    switch (valnode.nodetyp) {
    case XML_NT_START:
        res = ERR_NCX_WRONG_NODETYP_CPX;
        break;
    case XML_NT_STRING:
        /* convert the non-whitespace string to a number */
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
        break;
    default:
        res = ERR_NCX_WRONG_NODETYP;
    }

    /* get the matching end node for startnode */
    res2 = get_xml_node(scb, &endnode);
    if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_num: expecting end for %s", 
                       startnode->qname);
            xml_dump_node(&endnode);
        }
#endif
        res2 = xml_endnode_match(startnode, &endnode);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_num */


/********************************************************************
* FUNCTION parse_string
* 
* Parse the XML input as a string data type 
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     obj == object template for this string type
*     btyp == base type of the expected string type
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
    parse_string (ses_cb_t  *scb,
                  obj_template_t *obj,
                  ncx_btype_t  btyp,
                  const xml_node_t *startnode,
                  val_value_t  *retval)
{
    ncx_errinfo_t  *errinfo;
    typ_template_t *listtyp;
    obj_template_t *targobj;
    xpath_result_t *result;
    xml_node_t      valnode, endnode;
    status_t        res, res2;
    boolean         empty;
    ncx_btype_t     listbtyp;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    empty = FALSE;
    errinfo = NULL;
    listbtyp = NCX_BT_NONE;
    listtyp = NULL;

    val_init_from_template(retval, obj);

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
        res = get_xml_node(scb, &valnode);
        if (res == NO_ERR && valnode.nodetyp == XML_NT_END) {
            empty = TRUE;
        }
    }
  
    /* check empty string corner case */
    if (empty) {
        if (btyp==NCX_BT_SLIST || btyp==NCX_BT_BITS) {
            /* no need to check the empty list */ ;
        } else if (btyp == NCX_BT_INSTANCE_ID) {
            res = ERR_NCX_INVALID_VALUE;
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

    if (res != NO_ERR || empty) {
        xml_clean_node(&valnode);
        return res;
    }

#ifdef MGR_VAL_PARSE_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\nparse_string: expecting string node.");
        xml_dump_node(&valnode);
    }
#endif

    /* validate the string content */
    switch (valnode.nodetyp) {
    case XML_NT_START:
        res = ERR_NCX_WRONG_NODETYP_CPX;
        break;
    case XML_NT_STRING:
        switch (btyp) {
        case NCX_BT_SLIST:
        case NCX_BT_BITS:
            if (btyp==NCX_BT_SLIST) {
                /* get the list of strings, then check them */
                listtyp = typ_get_listtyp(obj_get_typdef(obj));
                listbtyp = typ_get_basetype(&listtyp->typdef);
            } else {
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
            retval->xpathpcb = xpath_new_pcb(valnode.simval, NULL);
            if (!retval->xpathpcb) {
                res = ERR_INTERNAL_MEM;
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
            if (res != NO_ERR) {
                res = ERR_NCX_INVALID_XPATH_EXPR;
            }
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
        case NCX_BT_INSTANCE_ID:
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
    }

    /* get the matching end node for startnode */
    res2 = get_xml_node(scb, &endnode);
    if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_string: expecting end for %s", 
                       startnode->qname);
            xml_dump_node(&endnode);
        }
#endif
        res2 = xml_endnode_match(startnode, &endnode);
        
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_string */


/********************************************************************
 * FUNCTION parse_idref
 * 
 * Parse the XML input as an 'identityref' type 
 * e.g..
 *
 * <foo>acme:card-type3</foo>
 * 
 * INPUTS:
 *     scb == session control block
 *            Input is read from scb->reader.
 *     obj == object template for this identityref type
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
    parse_idref (ses_cb_t  *scb,
                 obj_template_t *obj,
                 const xml_node_t *startnode,
                 val_value_t  *retval)
{
    const xmlChar         *str;
    xml_node_t             valnode, endnode;
    status_t               res, res2;
    boolean                enddone;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    enddone = FALSE;
    res = NO_ERR;
    res2 = NO_ERR;

    val_init_from_template(retval, obj);

    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj), 
                         NULL, 
                         XML_NT_START); 
    if (res == NO_ERR) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, &valnode);
    }

    if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_idref: expecting string node");
            xml_dump_node(&valnode);
        }
#endif

        /* validate the node type and enum string or number content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
        case XML_NT_EMPTY:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            break;
        case XML_NT_STRING:
            if (val_all_whitespace(valnode.simval)) {
                res = ERR_NCX_INVALID_VALUE;
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
                    /* save name given anyway or NULL string will 
                     * get printed */
                    retval->v.idref.name = xml_strdup(valnode.simval);
                    if (!retval->v.idref.name) {
                        res = ERR_INTERNAL_MEM;
                    }
                }                    
            }
            break;
        case XML_NT_END:
            enddone = TRUE;
            res = ERR_NCX_INVALID_VALUE;
            break;
        default:
            res = SET_ERROR(ERR_NCX_WRONG_NODETYP);
            enddone = TRUE;
        }

        /* get the matching end node for startnode */
        if (!enddone) {
            res2 = get_xml_node(scb, &endnode);
            if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
                if (LOGDEBUG4) {
                    log_debug4("\nparse_idref: expecting end for %s", 
                               startnode->qname);
                    xml_dump_node(&endnode);
                }
#endif
                res2 = xml_endnode_match(startnode, &endnode);
            }
        }
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);
    return res;

} /* parse_idref */



/********************************************************************
 * FUNCTION parse_union
 * 
 * Parse the XML input as a 'union' type 
 * e.g..
 *
 * <foo>fred</foo>
 * <foo>11</foo>
 *
 * INPUTS:
 *     scb == session control block
 *            Input is read from scb->reader.
 *     obj == object template for this string type
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
    parse_union (ses_cb_t  *scb,
                 obj_template_t *obj,
                 const xml_node_t *startnode,
                 val_value_t  *retval)
{
    ncx_errinfo_t       *errinfo;
    xml_node_t           valnode, endnode;
    status_t             res, res2;
    boolean              stopnow;

    /* init local vars */
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    res2 = NO_ERR;
    errinfo = NULL;
    stopnow = FALSE;

    val_init_from_template(retval, obj);

    /* make sure the startnode is correct */
    res = xml_node_match(startnode, 
                         obj_get_nsid(obj), 
                         NULL, 
                         XML_NT_START); 
    if (res == NO_ERR) {
        /* get the next node which should be a string node */
        res = get_xml_node(scb, &valnode);
    }

    if (res == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_union: expecting string or number node.");
            xml_dump_node(&valnode);
        }
#endif

        /* validate the node type and union node content */
        switch (valnode.nodetyp) {
        case XML_NT_START:
            res = ERR_NCX_WRONG_NODETYP_CPX;
            stopnow = TRUE;
            break;
        case XML_NT_STRING:
            /* get the non-whitespace string here */
            res = val_union_ok_errinfo(obj_get_typdef(obj), 
                                       valnode.simval, 
                                       retval, 
                                       &errinfo);
            break;
        case XML_NT_END:
            stopnow = TRUE;
            res = val_union_ok_errinfo(obj_get_typdef(obj), 
                                       EMPTY_STRING, 
                                       retval, 
                                       &errinfo);
            break;
        default:
            stopnow = TRUE;
            SET_ERROR(ERR_INTERNAL_VAL);
            res = ERR_NCX_WRONG_NODETYP;
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

            /* get the matching end node for startnode */
            res2 = get_xml_node(scb, &endnode);
            if (res2 == NO_ERR) {
#ifdef MGR_VAL_PARSE_DEBUG
                if (LOGDEBUG4) {
                    log_debug4("\nparse_union: expecting end for %s", 
                               startnode->qname);
                    xml_dump_node(&endnode);
                }
#endif
                res2 = xml_endnode_match(startnode, &endnode);
            }
        }
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&valnode);
    xml_clean_node(&endnode);

    return res;

} /* parse_union */


/********************************************************************
 * FUNCTION parse_complex
 * 
 * Parse the XML input as a complex type
 *
 * Handles the following base types:
 *   NCX_BT_CONTAINER
 *   NCX_BT_LIST
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
 *     scb == session control block
 *            Input is read from scb->reader.
 *     obj == object template for this complex type
 *     output == foo-rpc/output object, if any
 *     btyp == base type of the expected complex type
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
    parse_complex (ses_cb_t  *scb,
                   obj_template_t *obj,
                   obj_template_t *output,
                   ncx_btype_t btyp,
                   const xml_node_t *startnode,
                   val_value_t  *retval)
{
    obj_template_t       *chobj, *curchild, *curtop;
    obj_template_t       *outchobj;
    mgr_scb_t            *mscb;
    val_value_t          *chval;
    dlq_hdr_t            *force_modQ;
    xml_node_t            chnode;
    status_t              res, retres;
    boolean               done, empty, errmode;
    ncx_btype_t           chbtyp;

    /* setup local vars */
    chobj = NULL;
    curtop = NULL;
    curchild = NULL;
    outchobj = NULL;
    res = NO_ERR;
    retres = NO_ERR;
    done = FALSE;
    empty = FALSE;
    chbtyp = NCX_BT_NONE;
    mscb = (mgr_scb_t *)scb->mgrcb;

    if (mscb == NULL || dlq_empty(&mscb->temp_modQ)) {
        force_modQ = NULL;
    } else {
        force_modQ = &mscb->temp_modQ;
    }

    val_init_from_template(retval, obj);

    /* check the XML start node type */
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

    if (res == NO_ERR) {
        /* start setting up the return value */
        retval->editop = get_editop(startnode);

        /* setup the first child in the complex object
         * Allowed be NULL in some cases so do not check
         * the result yet
         */
        chobj = obj_first_child(obj);
        outchobj = (output) ? obj_first_child(output) : NULL;
    } else {
        return res;
    }

    if (empty) {
        return NO_ERR;
    }

    xml_init_node(&chnode);

    /* go through each child node until the parent end node */
    while (!done) {
        /* init per-loop vars */
        empty = FALSE;
        chval = NULL;
        errmode = FALSE;

        /* get the next node which should be a child or end node */
        res = get_xml_node(scb, &chnode);
        if (res != NO_ERR) {
            if (res == ERR_NCX_UNKNOWN_NS && chnode.elname) {
                retval->res = res;
                errmode = TRUE;
                res = NO_ERR;
            }  else {
                done = TRUE;
                continue;
            }
        }

#ifdef MGR_VAL_PARSE_DEBUG
        if (LOGDEBUG4) {
            log_debug4("\nparse_complex: expecting "
                       "start-child or end node.");
            xml_dump_node(&chnode);
        }
#endif
        /* validate the child member node type */
        switch (chnode.nodetyp) {
        case XML_NT_START:
        case XML_NT_EMPTY:
            /* any namespace OK for now, 
             * check in obj_get_child_node 
             */
            break;
        case XML_NT_STRING:
            res = ERR_NCX_WRONG_NODETYP_SIM;
            break;
        case XML_NT_END:
            res = xml_endnode_match(startnode, &chnode);
            if (res == NO_ERR) {
                /* no error exit */
                done = TRUE;
                continue;
            }
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
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
            curchild = NULL;
            if (!errmode) {
                res = obj_get_child_node(obj, 
                                         chobj, 
                                         &chnode, 
                                         FALSE,
                                         force_modQ,
                                         &curtop, 
                                         &curchild);
            }
            if (res != NO_ERR && output) {
                /* 2) try child of output object */
                res = obj_get_child_node(output, 
                                         outchobj, 
                                         &chnode, 
                                         FALSE,
                                         force_modQ,
                                         &curtop, 
                                         &curchild);
            }



            if (!curchild || res != NO_ERR) {
                if (ncx_warning_enabled(ERR_NCX_USING_ANYXML)) {
                    log_warn("\nWarning: '%s' has no child node "
                             "'%s'. Using anyxml",
                             retval->name, 
                             chnode.qname);
                }
                curchild = ncx_get_gen_anyxml();
                res = NO_ERR;
                errmode = TRUE;
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
            chval = 
                val_new_child_val((errmode) ?
                                  xmlns_inv_id() : 
                                  obj_get_nsid(curchild),
                                  (errmode) ? 
                                  chnode.elname : 
                                  obj_get_name(curchild), 
                                  errmode, 
                                  retval, 
                                  get_editop(&chnode),
                                  curchild);
            if (!chval) {
                res = ERR_INTERNAL_MEM;
            }
        }

        /* check any errors in setting up the child node */
        if (res != NO_ERR) {
            retres = res;


            /* try to skip just the child node sub-tree */
            xml_clean_node(&chnode);
            if (chval) {
                val_free_value(chval);
                chval = NULL;
            }
            if (NEED_EXIT(res) || res == ERR_XML_READER_EOF) {
                done = TRUE;
            } else {
                /* skip the entire value subtree */
                (void)mgr_xml_skip_subtree(scb->reader, startnode);
            }
            continue;
        }

        /* recurse through and get whatever nodes are present
         * in the child node
         */
        res = parse_btype(scb, curchild, &chnode, chval);
        chval->res = res;
        val_add_child(chval, retval);
        if (res == NO_ERR) {
            /* setup the next child unless the current child
             * is a list
             */
            if (chbtyp != NCX_BT_LIST) {
                /* setup next child if the cur child 
                 * is 0 - 1 instance 
                 */
                switch (obj_get_iqualval(curchild)) {
                case NCX_IQUAL_ONE:
                case NCX_IQUAL_OPT:
                    if (chobj) {
                        chobj = obj_next_child(chobj);
                    }
                    if (outchobj) {
                        outchobj = obj_next_child(outchobj);
                    }
                    break;
                default:
                    break;
                }
            }
        } else {
            /* did not parse the child node correctly */
            retres = res;
            if (NEED_EXIT(res) || res==ERR_XML_READER_EOF) {
                done = TRUE;
                continue;
            }
        }
        xml_clean_node(&chnode);

        /* check if this is the internal load-config RPC method,
         * in which case there will not be an end tag for the 
         * <load-config> start tag passed to this function.  
         * Need to quick exit to prevent reading past the 
         * end of the XML config file,
         * which just contains the <config> element
         *
         * know there is just one parameter named <config> so just
         * go through the loop once because the </load-config> tag
         * will not be present
         */
        if ((startnode->nsid == xmlns_ncx_id() || 
             startnode->nsid == 0) && 
            !xml_strcmp(startnode->elname, NCX_EL_LOAD_CONFIG)) {
            done = TRUE;
        }
    }

    /* check if the index ID needs to be set */
    if (retres==NO_ERR && btyp==NCX_BT_LIST) {
        res = gen_index_chain(obj_first_key(obj), retval);
        if (res != NO_ERR) {
            retres = res;
        }
    }

    xml_clean_node(&chnode);
    return retres;

} /* parse_complex */


/********************************************************************
* FUNCTION parse_metadata
* 
* Parse all the XML attributes in the specified xml_node_t struct
*
* Only XML_NT_START or XML_NT_EMPTY nodes will have attributes
*
* INPUTS:
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
    parse_metadata (obj_template_t *obj,
                    const xml_node_t *node,
                    boolean nserr,
                    val_value_t  *retval)
{
    obj_metadata_t    *meta;
    typ_def_t         *metadef;
    xml_attr_t        *attr;
    val_value_t       *metaval;
    xmlns_id_t         ncid, yangid, xmlid, wdaid;
    status_t           res, retres;
    boolean            iskey, isvalue, islang, islastmodified;

    retres = NO_ERR;
    ncid =  xmlns_nc_id();
    yangid =  xmlns_yang_id();
    xmlid = xmlns_xml_id();
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
        meta = NULL;
        metadef = NULL;
        iskey = FALSE;
        isvalue = FALSE;
        islang = FALSE;
        islastmodified = FALSE;

        /* check qualified and unqualified operation attribute,
         * then the 'xmlns' attribute, then a defined attribute
         */
        if (val_match_metaval(attr, ncid, NC_OPERATION_ATTR_NAME)) {
            retval->editop = op_editop_id(attr->attr_val);
            if (retval->editop == OP_EDITOP_NONE) {
                retres = ERR_NCX_INVALID_VALUE;
            }
            continue;
        } else if (val_match_metaval(attr, yangid, YANG_K_INSERT)) {
            if (retval->editvars == NULL) {
                res = val_new_editvars(retval);
                if (res != NO_ERR) {
                    return res;
                }
            }

            retval->editvars->insertop = op_insertop_id(attr->attr_val);
            if (retval->editvars->insertop == OP_INSOP_NONE) {
                retres = ERR_NCX_INVALID_VALUE;
            }
            continue;
        } else if (val_match_metaval(attr, yangid, YANG_K_KEY)) {
            iskey = TRUE;
        } else if (val_match_metaval(attr, yangid, YANG_K_VALUE)) {
            isvalue = TRUE;
        } else if (val_match_metaval(attr, xmlid, 
                                     (const xmlChar *)"lang")) {
            islang = TRUE;
        } else if (val_match_metaval(attr, wdaid, YANG_K_DEFAULT)) {
            if (ncx_is_true(attr->attr_val)) {
                val_set_withdef_default(retval);
            } else if (ncx_is_false(attr->attr_val)) {
                ;
            } else {
                log_error("\nError: wd:default attribute has "
                          "invalid value '%s'\n",
                          attr->attr_val);
                res = ERR_NCX_INVALID_VALUE;
            }
            continue;
        } else if (val_match_metaval(attr, 0, NCX_EL_LAST_MODIFIED)) {
            islastmodified = TRUE;
        } else {
            /* find the attribute definition in this typdef */
            meta = obj_find_metadata(obj, attr->attr_name);
            if (meta) {
                metadef = meta->typdef;
            } else if (!nserr) {
                metadef = typ_get_basetype_typdef(NCX_BT_STRING);
            }
        }

        if (iskey || isvalue || islang || islastmodified) {
            metadef = typ_get_basetype_typdef(NCX_BT_STRING);
        }

        if (metadef) {
            /* alloc a new value struct for rhe attribute */
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
        } else {
            log_warn("\nWarning: unknown attribute found %s='%s'", 
                     attr->attr_qname,
                     attr->attr_val);
        }
        if (res != NO_ERR) {
            retres = res;
        }
    }
    return retres;

} /* parse_metadata */


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
    metadata_inst_check (val_value_t  *val)
{
    obj_metadata_t *meta;
    uint32                cnt;
    xmlns_id_t            yangid;

    yangid = xmlns_yang_id();

    /* first check the inst count of the YANG attributes
     * may not need to do this at all if XmlTextReader
     * rejects duplicate XML attributes
     */
    cnt = val_metadata_inst_count(val, yangid, YANG_K_KEY);
    if (cnt > 1) {
        return ERR_NCX_EXTRA_ATTR;
    }

    cnt = val_metadata_inst_count(val, yangid, YANG_K_VALUE);
    if (cnt > 1) {
        return ERR_NCX_EXTRA_ATTR;
    }

    if (!val->obj) {
        return NO_ERR;
    }

    for (meta = obj_first_metadata(val->obj);
         meta != NO_ERR;
         meta = obj_next_metadata(meta)) {

        cnt = val_metadata_inst_count(val, meta->nsid, meta->name);

        /* check the instance qualifier from the metadata
         * continue the loop if there is no error
         */
        if (cnt > 1) {
            return ERR_NCX_EXTRA_ATTR;
        }
    }
    return NO_ERR;

} /* metadata_inst_check */


/********************************************************************
* FUNCTION parse_btype
* 
* Switch to dispatch to specific base type handler
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     obj == object template to use for parsing
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
    parse_btype (ses_cb_t  *scb,
                 obj_template_t *obj,
                 const xml_node_t *startnode,
                 val_value_t  *retval)
{
    ncx_btype_t  btyp;
    status_t     res, res2, res3;
    boolean      nserr;

    btyp = obj_get_basetype(obj);

    /* get the attribute values from the start node */
    retval->nsid = startnode->nsid;

    /* check namespace errors except if the type is ANY */
    nserr = (btyp != NCX_BT_ANY);

    /* parse the attributes, if any; do not quick exit on this error */
    res2 = parse_metadata(obj, startnode, nserr, retval);

    /* continue to parse the startnode depending on the base type 
     * to record as many errors as possible
     */
    switch (btyp) {
    case NCX_BT_ANY:
        res = parse_any(scb, startnode, retval);
        break;
    case NCX_BT_ENUM:
        res = parse_enum(scb, 
                         obj, 
                         startnode, 
                         retval);
        break;
    case NCX_BT_EMPTY:
        res = parse_empty(scb, 
                          obj, 
                          startnode, 
                          retval);
        break;
    case NCX_BT_BOOLEAN:
        res = parse_boolean(scb, 
                            obj, 
                            startnode, 
                            retval);
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
        res = parse_num(scb, 
                        obj, 
                        btyp, 
                        startnode, 
                        retval);
        break;
    case NCX_BT_LEAFREF:
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
    case NCX_BT_INSTANCE_ID:
        res = parse_string(scb, 
                           obj, 
                           btyp, 
                           startnode, 
                           retval);
        break;
    case NCX_BT_IDREF:
        res = parse_idref(scb, 
                          obj, 
                          startnode, 
                          retval);
        break;
    case NCX_BT_UNION:
        res = parse_union(scb, 
                          obj, 
                          startnode, 
                          retval);
        break;
    case NCX_BT_CONTAINER:
    case NCX_BT_LIST:
        res = parse_complex(scb, 
                            obj, 
                            NULL,
                            btyp, 
                            startnode, 
                            retval);
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* set the config flag for this value */
    res3 = NO_ERR;

    if (res == NO_ERR && res2 == NO_ERR) {
        /* this has to be after the retval typdef is set */
        res3 = metadata_inst_check(retval);
    }

    if (res != NO_ERR) {
        retval->res = res;
        return res;
    } else if (res2 != NO_ERR) {
        retval->res = res2;
        return res2;
    }

    retval->res = res3;
    return res3;

} /* parse_btype */


/********************************************************************
* FUNCTION parse_btype_split
* 
* Switch to dispatch to specific base type handler
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     obj == object template to use for parsing
      output == foo-rpc/output object (if any)
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
    parse_btype_split (ses_cb_t  *scb,
                       obj_template_t *obj,
                       obj_template_t *output,
                       const xml_node_t *startnode,
                       val_value_t  *retval)
{
    ncx_btype_t  btyp;
    status_t     res, res2, res3;
    boolean      nserr;

    btyp = obj_get_basetype(obj);

    /* get the attribute values from the start node */
    retval->nsid = startnode->nsid;

    /* check namespace errors except if the type is ANY */
    nserr = (btyp != NCX_BT_ANY);

    /* parse the attributes, if any; do not quick exit on this error */
    res2 = parse_metadata(obj, startnode, nserr, retval);

    /* continue to parse the startnode depending on the base type 
     * to record as many errors as possible
     */
    switch (btyp) {
    case NCX_BT_ANY:
        res = parse_any(scb, startnode, retval);
        break;
    case NCX_BT_ENUM:
        res = parse_enum(scb, 
                         obj, 
                         startnode, 
                         retval);
        break;
    case NCX_BT_EMPTY:
        res = parse_empty(scb, 
                          obj, 
                          startnode, 
                          retval);
        break;
    case NCX_BT_BOOLEAN:
        res = parse_boolean(scb, 
                            obj, 
                            startnode, 
                            retval);
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
        res = parse_num(scb, 
                        obj, 
                        btyp, 
                        startnode, 
                        retval);
        break;
    case NCX_BT_LEAFREF:
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
    case NCX_BT_INSTANCE_ID:
        res = parse_string(scb, 
                           obj, 
                           btyp, 
                           startnode, 
                           retval);
        break;
    case NCX_BT_UNION:
        res = parse_union(scb, 
                          obj, 
                          startnode, 
                          retval);
        break;
    case NCX_BT_CONTAINER:
    case NCX_BT_LIST:
        res = parse_complex(scb, 
                            obj, 
                            output, 
                            btyp, 
                            startnode, 
                            retval);
        break;
    default:
        log_error("\nError: got invalid btype '%d'", btyp);
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* set the config flag for this value */
    res3 = NO_ERR;

    if (res == NO_ERR && res2 == NO_ERR) {
        /* this has to be after the retval typdef is set */
        res3 = metadata_inst_check(retval);
    }

    if (res != NO_ERR) {
        retval->res = res;
        return res;
    } else if (res2 != NO_ERR) {
        retval->res = res2;
        return res2;
    }

    retval->res = res3;
    return res3;

} /* parse_btype_split */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION mgr_val_parse
* 
* parse a value for a YANG type from a NETCONF PDU XML stream 
*
* Parse NETCONF PDU sub-contents into value fields
* This module does not enforce complex type completeness.
* Different subsets of configuration data are permitted
* in several standard (and any proprietary) RPC methods
*
* A seperate parsing phase is used to validate the input
* contained in the returned val_value_t struct.
*
* This parsing phase checks that simple types are complete
* and child members of complex types are valid (but maybe 
* missing or incomplete child nodes.
*
* INPUTS:
*     scb == session control block
*     obj == obj_template_t for the object type to parse
*     startnode == top node of the parameter to be parsed
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    mgr_val_parse (ses_cb_t  *scb,
                   obj_template_t *obj,
                   const xml_node_t *startnode,
                   val_value_t  *retval)
{
    const xml_node_t  *usenode;
    status_t  res;
    xml_node_t  topnode;

#ifdef DEBUG
    if (scb == NULL || obj == NULL || retval == NULL) {
        /* non-recoverable error */
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef MGR_VAL_PARSE_DEBUG
    if (LOGDEBUG3) {
        log_debug3("\nmgr_val_parse: %s:%s btyp:%s", 
                   obj_get_mod_prefix(obj),
                   obj_get_name(obj), 
                   tk_get_btype_sym(obj_get_basetype(obj)));
    }
#endif

    res = NO_ERR;
    usenode = NULL;
    xml_init_node(&topnode);

    if (startnode == NULL) {
        res = get_xml_node(scb, &topnode);
        if (res == NO_ERR) {
            val_set_name(retval, 
                         topnode.elname,
                         xml_strlen(topnode.elname));
            val_change_nsid(retval, topnode.nsid);
            usenode = &topnode;
        }
    } else {
        usenode = startnode;
    }

    /* get the element values */
    if (res == NO_ERR) {
        res = parse_btype(scb, obj, usenode, retval);
    }

    xml_clean_node(&topnode);
        
    return res;

}  /* mgr_val_parse */


/********************************************************************
* FUNCTION mgr_val_parse_reply
*
* parse an <rpc-reply> element 
* parse a value for a YANG type from a NETCONF PDU XML stream 
* Use the RPC object output type to parse any data
*
* Parse NETCONF PDU sub-contents into value fields
* This module does not enforce complex type completeness.
* Different subsets of configuration data are permitted
* in several standard (and any proprietary) RPC methods
*
* A seperate parsing phase is used to validate the input
* contained in the returned val_value_t struct.
*
* This parsing phase checks that simple types are complete
* and child members of complex types are valid (but maybe 
* missing or incomplete child nodes.
*
* INPUTS:
*     scb == session control block
*     obj == obj_template_t for the top-level reply to parse
*     rpc == RPC template to use for any data in the output
*     startnode == top node of the parameter to be parsed
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    mgr_val_parse_reply (ses_cb_t  *scb,
                         obj_template_t *obj,
                         obj_template_t *rpc,
                         const xml_node_t *startnode,
                         val_value_t  *retval)
{
    obj_template_t  *output;
    status_t  res;

#ifdef DEBUG
    if (!scb || !obj || !startnode || !retval) {
        /* non-recoverable error */
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef MGR_VAL_PARSE_DEBUG
    if (LOGDEBUG3) {
        log_debug3("\nmgr_val_parse_reply: %s:%s btyp:%s", 
                   obj_get_mod_prefix(obj),
                   obj_get_name(obj), 
                   tk_get_btype_sym(obj_get_basetype(obj)));
    }
#endif

    output = (rpc) ? obj_find_child(rpc, NULL, NCX_EL_OUTPUT) : NULL;

    /* get the element values */
    res = parse_btype_split(scb, 
                            obj, 
                            output, 
                            startnode, 
                            retval);
    
    return res;

}  /* mgr_val_parse_reply */


/********************************************************************
* FUNCTION mgr_val_parse_notification
* 
* parse a <notification> element
* parse a value for a YANG type from a NETCONF PDU XML stream 
* Use the notification object output type to parse any data
*
* This parsing phase checks that simple types are complete
* and child members of complex types are valid (but maybe 
* missing or incomplete child nodes.
*
* INPUTS:
*     scb == session control block
*     notobj == obj_template_t for the top-level notification
*     startnode == top node of the parameter to be parsed
*     retval ==  val_value_t that should get the results of the parsing
*     
* OUTPUTS:
*    *retval will be filled in
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    mgr_val_parse_notification (ses_cb_t  *scb,
                                obj_template_t *notobj,
                                const xml_node_t *startnode,
                                val_value_t  *retval)
{
    status_t  res;

#ifdef DEBUG
    if (!scb || !notobj || !startnode || !retval) {
        /* non-recoverable error */
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef MGR_VAL_PARSE_DEBUG
    if (LOGDEBUG3) {
        log_debug3("\nmgr_val_parse_notification: start");
    }
#endif

    /* get the element values */
    res = parse_btype_split(scb, 
                            notobj, 
                            NULL, 
                            startnode, 
                            retval);

    return res;

}  /* mgr_val_parse_notification */


/* END file mgr_val_parse.c */
