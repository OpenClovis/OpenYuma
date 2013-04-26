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
#ifndef _H_agt_connect
#define _H_agt_connect

/*  FILE: agt_connect.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Handle the <ncx-connect> (top-level) element.
    This message is used for thin clients to connect
    to the ncxserver. 

   Client --> SSH2 --> OpenSSH.subsystem(netconf) -->
 
      ncxserver_connect --> AF_LOCAL/ncxserver.sock -->

      ncxserver.listen --> top_dispatch -> ncx_connect_handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
15-jan-07    abb      Begun

*/

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_status
#include "status.h"
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
* FUNCTION agt_connect_init
*
* Initialize the agt_connect module
* Adds the agt_connect_dispatch function as the handler
* for the NCX <ncx-connect> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern status_t 
    agt_connect_init (void);


/********************************************************************
* FUNCTION agt_connect_cleanup
*
* Cleanup the agt_connect module.
* Unregister the top-level NCX <ncx-connect> element
*
*********************************************************************/
extern void 
    agt_connect_cleanup (void);


/********************************************************************
* FUNCTION agt_connect_dispatch
*
* Handle an incoming <ncx-connect> request
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
extern void
    agt_connect_dispatch (ses_cb_t *scb,
			  xml_node_t *top);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_connect */
