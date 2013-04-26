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
#ifndef _H_agt_tree
#define _H_agt_tree

/*  FILE: agt_tree.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Server subtree filter processing for <filter> element in
    <get> and <get-config> operations

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
16-jun-06    abb      Begun

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

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_tree_prune_filter
*
* get and get-config step 1
* Need to evaluate the entire subtree filter and remove nodes
* which are not in the result set
*
* The filter subtree usually starts out as type NCX_BT_CONTAINER
* but a single select node could be present instead.
*
* The <filter> subtree starts out as type NCX_BT_CONTAINER (root)
* and is converted by val_parse as follows:
*
*   Subtree Filter:
*     NCX_BT_ANY -- start type
*   changed to real types during parsing
*     NCX_BT_CONTAINER   -->  container node
*     NCX_BT_EMPTY    -->  select node
*     NCX_BT_STRING  -->  content select node
*
* INPUTS:
*    scb == session control block
*         == NULL if access control should not be applied
*    msg == rpc_msg_t in progress
*    cfg == config target to check against
*    getop  == TRUE if this is a <get> and not a <get-config>
*              The target is expected to be the <running> 
*              config, and all state data will be available for the
*              filter output.
*              FALSE if this is a <get-config> and only the 
*              specified target in available for filter output
*
* OUTPUTS:
*    msg->rpc_filter.op_filter is pruned as needed by setting
*    the VAL_FL_FILTERED bit.in the val->flags field
*    for the start of subtrees which failed the filter test.
*    Only nodes which result in non-NULL output should
*    remain unchanged at the end of this procedure.
*
* RETURNS:
*    pointer to generated tree of matching nodes
*    NULL if no match
*********************************************************************/
extern ncx_filptr_t *
    agt_tree_prune_filter (ses_cb_t *scb,
			   rpc_msg_t *msg,
			   const cfg_template_t *cfg,
			   boolean getop);


/********************************************************************
* FUNCTION agt_tree_output_filter
*
* get and get-config step 2
* Output the pruned subtree filter to the specified session
*
* INPUTS:
*    scb == session control block
*         == NULL if access control should not be applied
*    msg == rpc_msg_t in progress
*    top == ncx_filptr tree to output
*    indent == start indent amount
*    getop == TRUE if <get>, FALSE if <get-config>
*
* RETURNS:
*    none
*********************************************************************/
extern void
    agt_tree_output_filter (ses_cb_t *scb,
			    rpc_msg_t *msg,
			    ncx_filptr_t *top,
			    int32 indent,
			    boolean getop);


/********************************************************************
* FUNCTION agt_tree_test_filter
*
* notification filter evaluation
* Need to evaluate the entire subtree filter and return 'FALSE'
* if any nodes in the filter are not in the result set
* (this is probably a notification payload)
*
* INPUTS:
*    msghdr == message in progress; needed for access control
*    scb == session control block; needed for access control
*    filter == subtree filter to use
*    topval == value tree to check against
*
* RETURNS:
*    TRUE if the filter matched; notification can be sent
*    FALSE if the filter did not match; notification not sent
*********************************************************************/
extern boolean
    agt_tree_test_filter (xml_msg_hdr_t *msghdr,
                          ses_cb_t *scb,
                          val_value_t *filter,
                          val_value_t *topval);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_tree */
