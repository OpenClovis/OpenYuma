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
#ifndef _H_mgr_val_parse
#define _H_mgr_val_parse

/*  FILE: mgr_val_parse.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Parameter Value Parser Module

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
18-feb-07    abb      Begun; start from agt_val_parse.c

*/

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
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
extern status_t 
    mgr_val_parse (ses_cb_t  *scb,
		   obj_template_t *obj,
		   const xml_node_t *startnode,
		   val_value_t  *retval);


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
extern status_t 
    mgr_val_parse_reply (ses_cb_t  *scb,
			 obj_template_t *obj,
			 obj_template_t *rpc,
			 const xml_node_t *startnode,
			 val_value_t  *retval);


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
extern status_t 
    mgr_val_parse_notification (ses_cb_t  *scb,
				obj_template_t *notobj,
				const xml_node_t *startnode,
				val_value_t  *retval);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_mgr_val_parse */
