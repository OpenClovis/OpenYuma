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
#ifndef _H_agt_hello
#define _H_agt_hello

/*  FILE: agt_hello.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Handle the NETCONF <hello> (top-level) element.

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
19-jan-07    abb      Begun

*/

#ifndef _H_ses
#include "ses.h"
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
* FUNCTION agt_hello_init
*
* Initialize the agt_hello module
* Adds the agt_hello_dispatch function as the handler
* for the NETCONF <hello> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern status_t 
    agt_hello_init (void);


/********************************************************************
* FUNCTION agt_hello_cleanup
*
* Cleanup the agt_hello module.
* Unregister the top-level NETCONF <hello> element
*
*********************************************************************/
extern void 
    agt_hello_cleanup (void);


/********************************************************************
* FUNCTION agt_hello_dispatch
*
* Handle an incoming <hello> message from the client
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
extern void
    agt_hello_dispatch (ses_cb_t *scb,
			xml_node_t *top);


/********************************************************************
* FUNCTION agt_hello_send
*
* Send the server <hello> message to the manager on the 
* specified session
*
* INPUTS:
*   scb == session control block
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_hello_send (ses_cb_t *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_hello */
