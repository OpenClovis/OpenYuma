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
#ifndef _H_yangcli_save
#define _H_yangcli_save

/*  FILE: yangcli_save.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
13-augr-09    abb      Begun

*/

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*		      F U N C T I O N S 			    *
*								    *
*********************************************************************/


/********************************************************************
 * FUNCTION do_save
 * 
 * INPUTS:
 *    server_cb == server control block to use
 *
 * OUTPUTS:
 *   copy-config and/or commit operation will be sent to server
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_save (server_cb_t *server_cb);


/********************************************************************
 * FUNCTION finish_save
 * 
 * INPUTS:
 *    server_cb == server control block to use
 *
 * OUTPUTS:
 *   copy-config will be sent to server
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    finish_save (server_cb_t *server_cb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_save */
