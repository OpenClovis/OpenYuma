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
#ifndef _H_mgr_hello
#define _H_mgr_hello
/*  FILE: mgr_hello.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol hello message

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
17-may-05    abb      Begun.
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
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/
#define NC_HELLO_STR    "hello"
#define NC_SESSION_ID   "session-id"

/********************************************************************
*                                                                   *
*                                    T Y P E S                      *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION mgr_hello_init
*
* Initialize the mgr_hello module
* Adds the mgr_hello_dispatch function as the handler
* for the NETCONF <hello> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern status_t 
    mgr_hello_init (void);


/********************************************************************
* FUNCTION mgr_hello_cleanup
*
* Cleanup the mgr_hello module.
* Unregister the top-level NETCONF <hello> element
*
*********************************************************************/
extern void 
    mgr_hello_cleanup (void);


/********************************************************************
* FUNCTION mgr_hello_dispatch
*
* Handle an incoming <hello> message from the client
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
extern void 
    mgr_hello_dispatch (ses_cb_t *scb,
			xml_node_t *top);


/********************************************************************
* FUNCTION mgr_hello_send
*
* Send the manager <hello> message to the agent on the 
* specified session
*
* INPUTS:
*   scb == session control block
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    mgr_hello_send (ses_cb_t *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_mgr_hello */

