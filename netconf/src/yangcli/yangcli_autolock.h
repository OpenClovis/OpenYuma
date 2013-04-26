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
#ifndef _H_yangcli_autolock
#define _H_yangcli_autolock

/*  FILE: yangcli_autolock.h
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

#ifndef _H_obj
#include "obj.h"
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
 * FUNCTION do_get_locks (local RPC)
 * 
 * get all the locks on the server
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the history command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_get_locks (server_cb_t *server_cb,
                  obj_template_t *rpc,
                  const xmlChar *line,
                  uint32  len);


/********************************************************************
 * FUNCTION do_release_locks (local RPC)
 * 
 * release all the locks on the server
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the history command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_release_locks (server_cb_t *server_cb,
                      obj_template_t *rpc,
                      const xmlChar *line,
                      uint32  len);


/********************************************************************
* FUNCTION handle_get_locks_request_to_server
* 
* Send the first <lock> operation to the server
* in a get-locks command
*
* INPUTS:
*   server_cb == server control block to use
*   first == TRUE if this is the first call; FALSE otherwise
*   done == address of return final done flag
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*    *done == TRUE when the return code is NO_ERR and all
*                 the locks are granted
*             FALSE otherwise
* RETURNS:
*    status; if NO_ERR then check *done flag
*            otherwise done is true on any error
*********************************************************************/
extern status_t
    handle_get_locks_request_to_server (server_cb_t *server_cb,
                                        boolean first,
                                        boolean *done);


/********************************************************************
* FUNCTION handle_release_locks_request_to_server
* 
* Send an <unlock> operation to the server
* in a get-locks command teardown or a release-locks
* operation
*
* INPUTS:
*   server_cb == server control block to use
*   first == TRUE if this is the first call; FALSE otherwise
*   done == address of return final done flag
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*    *done == TRUE when the return code is NO_ERR and all
*                 the locks are granted
*             FALSE otherwise
* RETURNS:
*    status; if NO_ERR then check *done flag
*            otherwise done is true on any error
*********************************************************************/
extern status_t
    handle_release_locks_request_to_server (server_cb_t *server_cb,
                                            boolean first,
                                            boolean *done);


/********************************************************************
* FUNCTION handle_locks_cleanup
* 
* Deal with the cleanup for the get-locks or release-locks
*
* INPUTS:
*   server_cb == server control block to use
*
* OUTPUTS:
*    server_cb->state may be changed or other action taken
*
*********************************************************************/
extern void
    handle_locks_cleanup (server_cb_t *server_cb);


/********************************************************************
* FUNCTION check_locks_timeout
* 
* Check if the locks_timeout is active and if it expired yet
*
* INPUTS:
*   server_cb == server control block to use
*
* RETURNS:
*   TRUE if locks_timeout expired
*   FALSE if no timeout has occurred
*********************************************************************/
extern boolean
    check_locks_timeout (server_cb_t *server_cb);


/********************************************************************
* FUNCTION send_discard_changes_pdu_to_server
* 
* Send a <discard-changes> operation to the server
*
* INPUTS:
*   server_cb == server control block to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    send_discard_changes_pdu_to_server (server_cb_t *server_cb);



/********************************************************************
* FUNCTION clear_lock_cbs
* 
* Clear the lock state info in all the lock control blocks
* in the specified server_cb
* 
* INPUTS:
*  server_cb == server control block to use
*
*********************************************************************/
extern void
    clear_lock_cbs (server_cb_t *server_cb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_autolock */
