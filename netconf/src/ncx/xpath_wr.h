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
#ifndef _H_xpath_wr
#define _H_xpath_wr

/*  FILE: xpath_wr.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Write an XPath expression to a session output
    in normalized format

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-apr-09    abb      Begun

*/

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
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
* FUNCTION xpath_wr_expr
* 
* Write the specified XPath expression to the current session
* using the default prefixes
*
* The XPath pcb must be previously parsed and found valid
*
* INPUTS:
*    scb == session control block to use
*    msg == message header to use
*    xpathval == the value containing the XPath expr to write
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_wr_expr (ses_cb_t *scb,
		   val_value_t *xpathval);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xpath_wr */
