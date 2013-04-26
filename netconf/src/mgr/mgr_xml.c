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
/*  FILE: mgr_xml.c

    Manager XML Reader interface

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11feb07      abb      begun; split from xml_util.c


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
#include <xmlreader.h>

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_mgr_xml
#include "mgr_xml.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif


/********************************************************************
*                                                                   *
*                          C O N S T A N T S                        *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/************** E X T E R N A L   F U N C T I O N S   **************/


/********************************************************************
* FUNCTION mgr_xml_consume_node
* 
* Parse the next node and return its namespace, type and name
* The xml_init_node or xml_clean_node API must be called before
* this function for the node parameter
*
* There are 2 types of XML element start nodes 
*   - empty node                          (XML_NT_EMPTY)
*   - start of a simple or complex type   (XML_NT_START)
*
* There is one string content node for simpleType content
*   - string node                         (XML_NT_STRING)
*
* There is one end node to end both simple and complex types
*   - end node                            (XML_NT_END)
*
* If nodetyp==XML_NT_EMPTY, then no further nodes will occur
* for this element. This node may contain attributes. The
* naming parameters will all be set.
*
* If nodetyp==XML_NT_START, then the caller should examine
* the schema for that start node.  
* For complex types, the next node is probably another XML_NT_START.
* For simple types, the next node will be XML_NT_STRING,
* followed by an XML_NT_END node. This node may contain attributes. 
* The naming parameters will all be set.
*
* If the nodetype==XML_NT_STRING, then the simval and simlen
* fields will be set.  There are no attributes or naming parameters
* for this node type.
*
* IF the nodetype==XML_NT_END, then no further nodes for this element
* will occur.  This node should not contain attributes.
* All of the naming parameters will be set. The xml_endnode_match
* function should be used to confirm that the XML_NT_START and
* XML_NT_END nodes are paired correctly.
*
* The node pointer for the reader will be advanced before the
* node is read.
* 
* INPUTS:
*   reader == XmlReader already initialized from File, Memory,
*             or whatever
*   node    == pointer to an initialized xml_node_t struct
*              to be filled in
*   
* OUTPUTS:
*   *node == xml_node_t struct filled in 
*   reader will be advanced
*
* RETURNS:
*   status of the operation
*   Try to fail on fatal errors only
*********************************************************************/
status_t 
    mgr_xml_consume_node (xmlTextReaderPtr reader,
                          xml_node_t      *node)
{
    return xml_consume_node(reader, node, TRUE, TRUE);

}  /* mgr_xml_consume_node */


status_t 
    mgr_xml_consume_node_nons (xmlTextReaderPtr reader,
                               xml_node_t      *node)
{
    return xml_consume_node(reader, node, FALSE, TRUE);

}  /* mgr_xml_consume_node_nons */



status_t 
    mgr_xml_consume_node_noadv (xmlTextReaderPtr reader,
                                xml_node_t      *node)
{
    return xml_consume_node(reader, node, TRUE, FALSE);

}  /* mgr_xml_consume_node_noadv */


/********************************************************************
* FUNCTION mgr_xml_skip_subtree
* 
* Already encountered an error, so advance nodes until the
* matching start-node is reached or a terminating error occurs
*   - end of input
*   - start depth level reached
*
* INPUTS:
*   reader == XmlReader already initialized from File, Memory,
*             or whatever
*   startnode  == xml_node_t of the start node of the sub-tree to skip
* RETURNS:
*   status of the operation
* SIDE EFFECTS:
*   the xmlreader state is advanced until the current node is the
*   end node of the specified start node or a fatal error occurs
*********************************************************************/
status_t 
    mgr_xml_skip_subtree (xmlTextReaderPtr reader,
                          const xml_node_t *startnode)
{
    xml_node_t       node;
    const xmlChar   *qname, *badns;
    uint32           len;
    int              ret, depth, nodetyp;
    xmlns_id_t       nsid;
    boolean          done, justone;
    status_t         res;

#ifdef DEBUG
    if (!reader || !startnode) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    justone = FALSE;

    switch (startnode->nodetyp) {
    case XML_NT_START:
        break;
    case XML_NT_EMPTY:
        return NO_ERR;
    case XML_NT_STRING:
        justone = TRUE;
        break;
    case XML_NT_END:
        return NO_ERR;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    xml_init_node(&node);
    res = mgr_xml_consume_node_noadv(reader, &node);
    if (res == NO_ERR) {
        res = xml_endnode_match(startnode, &node);
        if (res == NO_ERR) {
            xml_clean_node(&node);
            return NO_ERR;
        }
    }

    xml_clean_node(&node);
    if (justone) {
        return NO_ERR;
    }

    done = FALSE;
    while (!done) {

        /* advance the node pointer */
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            /* fatal error */
            return ERR_XML_READER_EOF;
        }

        /* get the node depth to match the end node correctly */
        depth = xmlTextReaderDepth(reader);
        if (depth == -1) {
            /* not sure if this can happen, treat as fatal error */
            return ERR_XML_READER_INTERNAL;
        } else if (depth <= startnode->depth) {
            /* this depth override will cause errors to be ignored
             *   - wrong namespace in matching end node
             *   - unknown namespace in matching end node
             *   - wrong name in 'matching' end node
             */
            done = TRUE;
        }

        /* get the internal nodetype, check it and convert it */
        nodetyp = xmlTextReaderNodeType(reader);

        /* get the element QName */
        qname = xmlTextReaderConstName(reader);
        if (qname) {
            /* check for namespace prefix in the name 
             * only error is 'unregistered namespace ID'
             * which doesn't matter in this case
             */
            nsid = 0;
            (void)xml_check_ns(reader, qname, &nsid, &len, &badns);
        } else {
            qname = (const xmlChar *)"";
        }

        /* check the normal case to see if the search is done */
        if (depth == startnode->depth &&
            !xml_strcmp(qname, startnode->qname) &&
            nodetyp == XML_ELEMENT_DECL) {
            done = TRUE;
        }

#ifdef XML_UTIL_DEBUG
        log_debug3("\nxml_skip: %s L:%d T:%s",
               qname, depth, xml_get_node_name(nodetyp));
#endif
    }

    return NO_ERR;

}  /* mgr_xml_skip_subtree */


/* END mgr_xml.c */
