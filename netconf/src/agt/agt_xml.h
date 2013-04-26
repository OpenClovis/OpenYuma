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
#ifndef _H_agt_xml
#define _H_agt_xml
/*  FILE: agt_xml.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server XML Reader interface

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-oct-05    abb      begun
2-jan-06     abb      rewrite xml_consume_* API to use simpler 
                      xml_node_t
11-feb-07    abb      moved consume_node fns to agt_xml.h
22-aug-08    abb      changed reader parameter to the session
                      control block to record error stats;
                      also errQ to msghdr to record xmlns directives 
*/


#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/



/********************************************************************
* FUNCTION agt_xml_consume_node
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
* The node pointer for the scb->reader will be advanced before the
* node is read.
* 
* INPUTS:
*   scb == session control block containing XmlTextReader
*
*   node    == pointer to an initialized xml_node_t struct
*              to be filled in
*   layer == protocol layer of caller (only used if errQ != NULL)
*    msghdr  == msg hdr w/ Q to get any rpc-errors as found, 
*               NULL if not used
*   
* OUTPUTS:
*   *node == xml_node_t struct filled in 
*   *errQ may have errors appended to it
*   
* RETURNS:
*   status of the operation
*   Try to fail on fatal errors only
*********************************************************************/
extern status_t 
    agt_xml_consume_node (ses_cb_t *scb,
			  xml_node_t *node,
			  ncx_layer_t layer,
			  xml_msg_hdr_t *msghdr);


/* consume node but do not generate an EOF error if seen */
extern status_t 
    agt_xml_consume_node_noeof (ses_cb_t *scb,
				xml_node_t *node,
				ncx_layer_t layer,
				xml_msg_hdr_t *msghdr);


/* consume node but do not generate namespace errors if seen 
 * needed to process subtree filters properly
 */
extern status_t 
    agt_xml_consume_node_nons (ses_cb_t *scb,
			       xml_node_t *node,
			       ncx_layer_t layer,
			       xml_msg_hdr_t *msghdr);

/* consume node but do not advance the node pointer */
extern status_t 
    agt_xml_consume_node_noadv (ses_cb_t *scb,
				xml_node_t *node,
				ncx_layer_t layer,
				xml_msg_hdr_t *msghdr);


/********************************************************************
* FUNCTION agt_xml_skip_subtree
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
extern status_t 
    agt_xml_skip_subtree (ses_cb_t *scb,
			  const xml_node_t *startnode);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_xml */

