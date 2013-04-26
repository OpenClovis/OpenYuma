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
#ifndef _H_yangcli_autoload
#define _H_yangcli_autoload

/*  FILE: yangcli_autoload.h
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

#include <xmlstring.h>

#ifndef _H_ses
#include "ses.h"
#endif

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
* FUNCTION autoload_setup_tempdir
* 
* copy all the advertised YANG modules into the
* session temp files directory
*
* DOES NOT COPY FILES UNLESS THE 'source' FIELD IS SET
* IN THE SEARCH RESULT (ONLY LOCAL FILES COPIED)
*
* INPUTS:
*   server_cb == server session control block to use
*   scb == current session in progress
*
* OUTPUTS:
*   $HOME/.yuma/tmp/<progdir>/<sesdir>/ filled with
*   the the specified YANG files that are already available
*   on this system.
*   
*   These search records will be removed from the 
*   server_cb->searchresultQ and modptr records 
*   added to the server_cb->modptrQ
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    autoload_setup_tempdir (server_cb_t *server_cb,
                            ses_cb_t *scb);


/********************************************************************
* FUNCTION autoload_start_get_modules
* 
* Start the MGR_SES_IO_CONN_AUTOLOAD state
*
* Go through all the search result records and 
* make sure all files are present.  Try to use the
* <get-schema> operation to fill in any missing modules
*
* INPUTS:
*   server_cb == server session control block to use
*   scb == session control block to use
*
* OUTPUTS:
*   $HOME/.yuma/tmp/<progdir>/<sesdir>/ filled with
*   the the specified YANG files that are ertrieved from
*   the device with <get-schema>
*   
* RETURNS:
*    status
*********************************************************************/
extern status_t
    autoload_start_get_modules (server_cb_t *server_cb,
                                ses_cb_t *scb);


/********************************************************************
* FUNCTION autoload_handle_rpc_reply
* 
* Handle the current <get-schema> response
*
* INPUTS:
*   server_cb == server session control block to use
*   scb == session control block to use
*   reply == data node from the <rpc-reply> PDU
*   anyerrors == TRUE if <rpc-error> detected instead
*                of <data>
*             == FALSE if no <rpc-error> elements detected
*
* OUTPUTS:
*   $HOME/.yuma/tmp/<progdir>/<sesdir>/ filled with
*   the the specified YANG files that was retrieved from
*   the device with <get-schema>
*
*   Next request is started, or autoload process is completed
*   and the command_mode is changed back to CMD_MODE_NORMAL
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    autoload_handle_rpc_reply (server_cb_t *server_cb,
                               ses_cb_t *scb,
                               val_value_t *reply,
                               boolean anyerrors);


/********************************************************************
* FUNCTION autoload_compile_modules
* 
* Go through all the search result records and parse
* the modules that the device advertised.
* DOES NOT LOAD THESE MODULES INTO THE MODULE DIRECTORY
* THE RETURNED ncx_module_t STRUCT IS JUST FOR ONE SESSION USE
*
* Apply the deviations and features specified in
* the search result cap back-ptr, to the module 
*
* INPUTS:
*   server_cb == server session control block to use
*   scb == session control block to use
*
* OUTPUTS:
*   $HOME/.yuma/tmp/<progdir>/<sesdir>/ filled with
*   the the specified YANG files that are already available
*   on this system.
*   
*   These search records will be removed from the 
*   server_cb->searchresultQ and modptr records 
*   added to the server_cb->modptrQ
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    autoload_compile_modules (server_cb_t *server_cb,
                              ses_cb_t *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_autoload */
