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
#ifndef _H_agt_val_parse
#define _H_agt_val_parse

/*  FILE: agt_val_parse.h
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
11-feb-06    abb      Begun
06-aug-08    abb      Rewrite for OBJ/VAL only model in YANG
                     Remove app, parmset, and parm from database model
*/

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_typ
#include "typ.h"
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
extern status_t
    agt_val_parse_nc (ses_cb_t  *scb,
		      xml_msg_hdr_t *msg,
		      obj_template_t *obj,
		      const xml_node_t *startnode,
		      ncx_data_class_t  parentdc,
		      val_value_t *retval);


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
extern void
    agt_val_parse_test (const char *testfile);
#endif

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_val_parse */
