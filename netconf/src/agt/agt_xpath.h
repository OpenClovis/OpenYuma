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
#ifndef _H_agt_xpath
#define _H_agt_xpath

/*  FILE: agt_xpath.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Server XPath filter processing for select attribute in
    <filter> element in <get> and <get-config> operations

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
27-jan-09    abb      Begun

*/

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_val
#include "val.h"
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
* FUNCTION agt_xpath_output_filter
*
* Evaluate the XPath filter against the specified 
* config root, and output the result of the
* <get> or <get-config> operation to the specified session
*
* INPUTS:
*    scb == session control block
*    msg == rpc_msg_t in progress
*    cfg == config target to check against
*    getop  == TRUE if this is a <get> and not a <get-config>
*              The target is expected to be the <running> 
*              config, and all state data will be available for the
*              filter output.
*              FALSE if this is a <get-config> and only the 
*              specified target in available for filter output
*    indent == start indent amount
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_xpath_output_filter (ses_cb_t *scb,
			     rpc_msg_t *msg,
			     const cfg_template_t *cfg,
			     boolean getop,
			     int32 indent);


/********************************************************************
* FUNCTION agt_xpath_test_filter
*
* Evaluate the XPath filter against the specified 
* config root, and just test if there would be any
* <eventType> element generated at all
*
* Does not write any output based on the XPath evaluation
*
* INPUTS:
*    msghdr == message header in progress (for access control)
*    scb == session control block
*    selectval == filter value struct to use
*    val == top of value tree to compare the filter against
*        !!! not written -- not const in XPath in case
*        !!! a set-by-xpath function ever implemented
*
* RETURNS:
*    status
*********************************************************************/
extern boolean
    agt_xpath_test_filter (xml_msg_hdr_t *msghdr,
                           ses_cb_t *scb,
                           const val_value_t *selectval,
                           val_value_t *val);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_xpath */
