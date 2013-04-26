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
#ifndef _H_mgr_top
#define _H_mgr_top

/*  FILE: mgr_top.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Manager Top Element module

    Manage callback registry for received XML messages

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
11-feb-07    abb      Begun; start from agt_top.c

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
* FUNCTION mgr_top_dispatch_msg
* 
* Find the appropriate top node handler and call it
*
* INPUTS:
*   scb == session control block containing the xmlreader
*          set at the start of an incoming message.
*
* RETURNS:
*  none
*********************************************************************/
extern void
    mgr_top_dispatch_msg (ses_cb_t  *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_mgr_top */
