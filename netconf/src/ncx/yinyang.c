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
/*  FILE: yinyang.c


   YIN to YANG conversion for input

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
12dec09      abb      begun


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yin
#include "yin.h"
#endif

#ifndef _H_yinyang
#include "yinyang.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define GET_NODE(S, N) xml_consume_node((S)->reader, N, FALSE, TRUE)



/********************************************************************
* FUNCTION parse_arg
* 
* Parse the XML input as a YIN element argument string 
*
* <foo>fred</foo>
*
* INPUTS:
*     scb == session control block
*            Input is read from scb->reader.
*     startnode == top node of the parameter to be parsed
*            Parser function will attempt to consume all the
*            nodes until the matching endnode is reached
*     tkc == token chain to add tokens into
*     
* RETURNS:
*   status
*********************************************************************/
static status_t 
    parse_arg (ses_cb_t  *scb,
               tk_chain_t  *tkc,
               xmlns_id_t nsid,
               const xmlChar *argname)
{
    xml_node_t      startnode, valnode, endnode;
    status_t        res, res2;
    boolean         empty;


    /* init local vars */
    xml_init_node(&startnode);
    xml_init_node(&valnode);
    xml_init_node(&endnode);
    empty = FALSE;

    res = GET_NODE(scb, &startnode);
    if (res != NO_ERR) {
        return res;
    }

    /* make sure the startnode is correct */
    res = xml_node_match(&startnode, 
                         nsid,
                         argname,
                         XML_NT_START); 
    if (res != NO_ERR) {
        res = xml_node_match(&startnode, 
                             nsid,
                             argname,
                             XML_NT_EMPTY); 
        if (res == NO_ERR) {
            empty = TRUE;
        }
    }

    if (res != NO_ERR) {
        xml_clean_node(&startnode);
        return res;
    }

    /* check if an empty string was given */
    if (empty) {
        res = tk_add_string_token(tkc, EMPTY_STRING);
        xml_clean_node(&startnode);
        return res;
    }
                            
    /* else got a start node, get the value string node */
    res = GET_NODE(scb, &valnode);
    if (res == NO_ERR) {
        if (valnode.nodetyp == XML_NT_END) {
            /* check correct end-node */
            res = xml_endnode_match(&startnode, &valnode);
            if (res == NO_ERR) {
                /* got 2-token empty instead of value string */
                res = tk_add_string_token(tkc, EMPTY_STRING);
            }
            xml_clean_node(&startnode);
            xml_clean_node(&valnode);
            return res;
        }
    } else {
        xml_clean_node(&startnode);
        xml_clean_node(&valnode);
        return res;
    }

    /* record the string content as a token */
    if (valnode.nodetyp == XML_NT_STRING) {
        res = tk_add_string_token(tkc, valnode.simval);
    } else {
        res = ERR_NCX_WRONG_NODETYP_CPX;
    }

    /* get the matching end node for startnode */
    res2 = GET_NODE(scb, &endnode);
    if (res2 == NO_ERR) {
        res2 = xml_endnode_match(&startnode, &endnode);
    }

    if (res == NO_ERR) {
        res = res2;
    }

    xml_clean_node(&startnode);
    xml_clean_node(&valnode);
    xml_clean_node(&endnode);

    return res;

} /* parse_arg */


/********************************************************************
* FUNCTION parse_yin_stmt
* 
* Try to parse a YIN statement from the XML reader
* 
* INPUTS:
* 
*
* OUTPUTS:
*   *res == status of the operation
*
* RETURNS:
*   malloced token chain containing the converted YIN file
*********************************************************************/
static status_t
    parse_yin_stmt (ses_cb_t *scb,
                    tk_chain_t *tkc,
                    xml_node_t *startnode)
{
    const yin_mapping_t    *mapping;
    xml_attr_t             *attr;
    status_t                res;
    xmlns_id_t              yin_id;
    boolean                 empty, done, first;
    xml_node_t              nextnode;

    res = NO_ERR;
    yin_id = xmlns_yin_id();

    /* the startnode is determined to be the
     * start of a YIN statement
     */
    switch (startnode->nodetyp) {
    case XML_NT_START:
        empty = FALSE;
        break;
    case XML_NT_EMPTY:
        empty = TRUE;
        break;
    default:
        return ERR_NCX_WRONG_NODETYP;
    }

    if (startnode->nsid == yin_id) {
        /* this should be a YANG keyword */
        mapping = yin_find_mapping(startnode->elname);
        if (mapping == NULL) {
            return ERR_NCX_DEF_NOT_FOUND;
        }

        /* generate the YANG keyword token */
        res = tk_add_id_token(tkc, mapping->keyword);
        if (res != NO_ERR) {
            return res;
        }

        /* check for the optional argument string */
        if (mapping->argname != NULL) {
            /* a parameter sring is expected */
            if (mapping->elem) {
                /* a simple element is expected */
                if (empty) {
                    /* startnode is empty; 
                     * child element parameter is missing 
                     */
                    return ERR_NCX_MISSING_ELEMENT;
                }

                /* need to get the argument as an element */
                res = parse_arg(scb,
                                tkc,
                                yin_id,
                                mapping->argname);
            } else {
                /* get the attribute value for the  argument string */
                attr = xml_find_attr(startnode,
                                     yin_id,
                                     mapping->argname);
                if (attr == NULL) {
                    return ERR_NCX_DEF_NOT_FOUND;
                }
                
                res = tk_add_string_token(tkc, attr->attr_val);
            }
            if (res != NO_ERR) {
                return res;
            }
        }
    } else {
        /* YANG extension usage found */
        res = tk_add_pid_token(tkc, 
                               startnode->qname,
                               (uint32)
                               (startnode->elname - startnode->qname - 1),
                               startnode->elname);
        if (res != NO_ERR) {
            return res;
        }

        /* fish for an argument attribute */
        attr = xml_get_first_attr(startnode);
        done = FALSE;
        while (!done && attr) {
            if (attr->attr_xmlns_ns) {
                attr = xml_next_attr(attr);
            } else {
                done = TRUE;
            }
        }

        switch (startnode->nodetyp) {
        case XML_NT_EMPTY:
            empty = TRUE;
            if (attr) {
                res = tk_add_string_token(tkc, attr->attr_val);
            }
            break;
        case XML_NT_START:
            empty = FALSE;
            if (attr) {
                res = tk_add_string_token(tkc, attr->attr_val);
            } else {
                res = parse_arg(scb,
                                tkc,
                                startnode->nsid,
                                NULL);
            }
            break;
        default:
            res = ERR_NCX_WRONG_NODETYP;
        }
    }

    if (empty) {
        res = tk_add_semicol_token(tkc);
        return res;
    }

    /* if we get here, the startnode is a start-tag
     * and all the sibling elements (if namy) are
     * sub-statements of the startnode statement
     */
    done = FALSE;
    first = TRUE;
    xml_init_node(&nextnode);
    while (!done && res == NO_ERR) {

        /* get the next XML node */
        res = GET_NODE(scb, &nextnode);
        if (res != NO_ERR) {
            continue;
        }

        /* got a start node and now there could be
         * another start node or an end node;
         * a string should not be found because that would
         * indicate a wrong encoding or mixed content
         */
        switch (nextnode.nodetyp) {
        case XML_NT_START:
        case XML_NT_EMPTY:
            if (first) {
                res = tk_add_lbrace_token(tkc);
            }

            if (res == NO_ERR) {
                res = parse_yin_stmt(scb, tkc, &nextnode);
            }
            break;
        case XML_NT_END:
            done = TRUE;
            res = xml_endnode_match(startnode, &nextnode);
            if (res == NO_ERR) {
                if (first) {
                    res = tk_add_semicol_token(tkc);
                } else {
                    res = tk_add_rbrace_token(tkc);
                }
            }
            break;
        default:
            done = TRUE;
            res = ERR_NCX_WRONG_NODETYP;
        }
        xml_clean_node(&nextnode);
        first = FALSE;
    }

    return res;

} /* parse_yin_stmt */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION yinyang_convert_token_chain
* 
* Try to open a file as a YIN formatted XML file
* and convert it to a token chain for processing
* by yang_parse_from_token_chain
*
* INPUTS:
*   sourcespec == file spec for the YIN file to read
*   res == address of return status
*
* OUTPUTS:
*   *res == status of the operation
*
* RETURNS:
*   malloced token chain containing the converted YIN file
*********************************************************************/
tk_chain_t *
    yinyang_convert_token_chain (const xmlChar *sourcespec,
                                 status_t *res)
{
    tk_chain_t       *tkc;
    ses_cb_t         *scb;
    xml_node_t        startnode;

#ifdef DEBUG
    if (sourcespec == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    scb = ses_new_dummy_scb();
    if (scb == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    tkc = tk_new_chain();
    if (tkc == NULL) {
        *res = ERR_INTERNAL_MEM;
        ses_free_scb(scb);
        return NULL;
    }

    tk_setup_chain_yin(tkc, sourcespec);

    xml_init_node(&startnode);

    /* open the XML reader */
    *res = xml_get_reader_from_filespec((const char *)sourcespec, 
                                        &scb->reader);
    if (*res == NO_ERR) {
        *res = GET_NODE(scb, &startnode);
        if (*res == NO_ERR) {
            *res = parse_yin_stmt(scb, tkc, &startnode);
        }
    }

    /* free the reader and the session control block */
    ses_free_scb(scb);
    xml_clean_node(&startnode);

    if (LOGDEBUG3) {
        tk_dump_chain(tkc);
    }

    if (*res == NO_ERR) {
        return tkc;
    } else {
        tk_free_chain(tkc);
        return NULL;
    }

}  /* yinyang_convert_token_chain */


/* END file yinyang.c */
