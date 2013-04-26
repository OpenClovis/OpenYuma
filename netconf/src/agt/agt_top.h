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
#ifndef _H_agt_top
#define _H_agt_top

/*  FILE: agt_top.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server Top Element module

    Manage callback registry for received XML messages

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
30-dec-05    abb      Begun

*/

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
* FUNCTION agt_top_dispatch_msg
* 
* Find the appropriate top node handler and call it
* called by the transport manager (through the session manager)
* when a new message is detected
*
* INPUTS:
*   scb == session control block containing the xmlreader
*          set at the start of an incoming message.
*
* NOTES:
*   This function might de-allocate the scb, if it does scb will be
*   set to NULL
*
* RETURNS:
*  none
*********************************************************************/
extern void
    agt_top_dispatch_msg (ses_cb_t  **scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_top */
