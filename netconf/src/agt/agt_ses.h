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
#ifndef _H_agt_ses
#define _H_agt_ses
/*  FILE: agt_ses.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF Session Server definitions module

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
06-jun-06    abb      Begun.
*/

#include <sys/select.h>

#include <xmlstring.h>

#ifndef _H_getcb
#include "getcb.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

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
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION agt_ses_init
*
* INIT 1:
*   Initialize the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_ses_init (void);


/********************************************************************
* FUNCTION agt_ses_cleanup
*
* Cleanup the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_ses_cleanup (void);


/********************************************************************
* FUNCTION agt_ses_new_dummy_session
*
* Create a dummy session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized dummy SCB, or NULL if malloc error
*********************************************************************/
extern ses_cb_t *
    agt_ses_new_dummy_session (void);


/********************************************************************
* FUNCTION agt_ses_set_dummy_session_acm
*
* Set the session ID and username of the user that
* will be responsible for the rollback if needed
*
* INPUTS:
*   dummy_session == session control block to change
*   use_sid == Session ID to use for the rollback
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_ses_set_dummy_session_acm (ses_cb_t *dummy_session,
                                   ses_id_t  use_sid);



/********************************************************************
* FUNCTION agt_ses_free_dummy_session
*
* Free a dummy session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
extern void
    agt_ses_free_dummy_session (ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_ses_new_session
*
* Create a real server session control block
*
* INPUTS:
*   transport == the transport type
*   fd == file descriptor number to use for IO 
* RETURNS:
*   pointer to initialized SCB, or NULL if some error
*   This pointer is stored in the session table, so it does
*   not have to be saved by the caller
*********************************************************************/
extern ses_cb_t *
    agt_ses_new_session (ses_transport_t transport,
			 int fd);


/********************************************************************
* FUNCTION agt_ses_free_session
*
* Free a real session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
extern void
    agt_ses_free_session (ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_ses_session_id_valid
*
* Check if a session-id is for an active session
*
* INPUTS:
*   sid == session ID to check
* RETURNS:
*   TRUE if active session; FALSE otherwise
*********************************************************************/
extern boolean
    agt_ses_session_id_valid (ses_id_t  sid);


/********************************************************************
* FUNCTION agt_ses_request_close
*
* Start the close of the specified session
*
* INPUTS:
*   scb == the session to close
*   killedby == session ID executing the kill-session or close-session
*   termreason == termination reason code
*
* RETURNS:
*   none
*********************************************************************/
extern void
    agt_ses_request_close (ses_cb_t *scb,
			   ses_id_t killedby,
			   ses_term_reason_t termreason);


/********************************************************************
* FUNCTION agt_ses_kill_session
*
* Kill the specified session
*
* INPUTS:
*   scb == the session to close
*   killedby == session ID executing the kill-session or close-session
*   termreason == termination reason code
*
* RETURNS:
*   none
*********************************************************************/
extern void
    agt_ses_kill_session (ses_cb_t *scb,
			  ses_id_t killedby,
			  ses_term_reason_t termreason);


/********************************************************************
* FUNCTION agt_ses_process_first_ready
*
* Check the readyQ and process the first message, if any
*
* RETURNS:
*     TRUE if a message was processed
*     FALSE if the readyQ was empty
*********************************************************************/
extern boolean
    agt_ses_process_first_ready (void);


/********************************************************************
* FUNCTION agt_ses_check_timeouts
*
* Check if any sessions need to be dropped because they
* have been idle too long.
*
*********************************************************************/
extern void
    agt_ses_check_timeouts (void);


/********************************************************************
* FUNCTION agt_ses_ssh_port_allowed
*
* Check if the port number used for SSH connect is okay
*
* RETURNS:
*     TRUE if port allowed
*     FALSE if port not allowed
*********************************************************************/
extern boolean
    agt_ses_ssh_port_allowed (uint16 port);


/********************************************************************
* FUNCTION agt_ses_fill_writeset
*
* Drain the ses_msg outreadyQ and set the specified fdset
* Used by agt_ncxserver write_fd_set
*
* INPUTS:
*    fdset == pointer to fd_set to fill
*    maxfdnum == pointer to max fd int to fill in
*
* OUTPUTS:
*    *fdset is updated in
*    *maxfdnum may be updated
*********************************************************************/
extern void
    agt_ses_fill_writeset (fd_set *fdset,
			   int *maxfdnum);


/********************************************************************
* FUNCTION agt_ses_get_inSessions
*
* <get> operation handler for the inSessions counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_inSessions (ses_cb_t *scb,
			    getcb_mode_t cbmode,
			    const val_value_t *virval,
			    val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_inBadHellos
*
* <get> operation handler for the inBadHellos counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_inBadHellos (ses_cb_t *scb,
			     getcb_mode_t cbmode,
			     const val_value_t *virval,
			     val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_inRpcs
*
* <get> operation handler for the inRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_inRpcs (ses_cb_t *scb,
			getcb_mode_t cbmode,
			const val_value_t *virval,

			val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_inBadRpcs
*
* <get> operation handler for the inBadRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_inBadRpcs (ses_cb_t *scb,
			   getcb_mode_t cbmode,
			   const val_value_t *virval,
			   val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_outRpcErrors
*
* <get> operation handler for the outRpcErrors counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_outRpcErrors (ses_cb_t *scb,
			      getcb_mode_t cbmode,
			      const val_value_t *virval,
			      val_value_t  *dstval);



/********************************************************************
* FUNCTION agt_ses_get_outNotifications
*
* <get> operation handler for the outNotifications counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_outNotifications (ses_cb_t *scb,
				  getcb_mode_t cbmode,
				  const val_value_t *virval,
				  val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_droppedSessions
*
* <get> operation handler for the droppedSessions counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_droppedSessions (ses_cb_t *scb,
                                 getcb_mode_t cbmode,
                                 const val_value_t *virval,
                                 val_value_t  *dstval);



/********************************************************************
* FUNCTION agt_ses_get_session_inRpcs
*
* <get> operation handler for the session/inRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_session_inRpcs (ses_cb_t *scb,
                                getcb_mode_t cbmode,
                                const val_value_t *virval,
                                val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_session_inBadRpcs
*
* <get> operation handler for the inBadRpcs counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_session_inBadRpcs (ses_cb_t *scb,
                                   getcb_mode_t cbmode,
                                   const val_value_t *virval,
                                   val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_session_outRpcErrors
*
* <get> operation handler for the outRpcErrors counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_session_outRpcErrors (ses_cb_t *scb,
                                      getcb_mode_t cbmode,
                                      const val_value_t *virval,
                                      val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_get_session_outNotifications
*
* <get> operation handler for the outNotifications counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_ses_get_session_outNotifications (ses_cb_t *scb,
                                          getcb_mode_t cbmode,
                                          const val_value_t *virval,
                                          val_value_t  *dstval);


/********************************************************************
* FUNCTION agt_ses_invalidate_session_acm_caches
*
* Invalidate all session ACM caches so they will be rebuilt
* TBD:: optimize and figure out exactly what needs to change
*
*********************************************************************/
extern void
    agt_ses_invalidate_session_acm_caches (void);

/********************************************************************
* FUNCTION agt_ses_get_session_for_id
*
* get the session for the supplied sid
*
* INPUTS:
*    ses_id_t the id of the session to get
*
* RETURNS:
*    ses_cb_t* or NULL
*    
*********************************************************************/
extern ses_cb_t* 
    agt_ses_get_session_for_id(ses_id_t sid);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_agt_ses */
