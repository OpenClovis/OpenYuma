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
#ifndef _H_mgr_ses
#define _H_mgr_ses
/*  FILE: mgr_ses.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF Session Manager definitions module

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
08-feb-07    abb      Begun; started from agt_ses.h
*/

#include <xmlstring.h>

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
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
* FUNCTION mgr_ses_init
*
* Initialize the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    mgr_ses_init (void);


/********************************************************************
* FUNCTION mgr_ses_cleanup
*
* Cleanup the session manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    mgr_ses_cleanup (void);


/********************************************************************
* FUNCTION mgr_ses_new_session
*
* Create a new session control block and
* start a NETCONF session with to the specified server
*
* After this functions returns OK, the session state will
* be in HELLO_WAIT state. An server <hello> must be received
* before any <rpc> requests can be sent
*
* The pubkeyfile and privkeyfile parameters will be used first.
* If this fails, the password parameter will be checked
*
* INPUTS:
*   user == user name
*   password == user password
*   pubkeyfile == filespec for client public key
*   privkeyfile == filespec for client private key
*   target == ASCII IP address or DNS hostname of target
*   port == NETCONF port number to use, or 0 to use defaults
*   transport == enum for the transport to use (SSH or TCP)
*   progcb == temp program instance control block,
*          == NULL if a session temp files control block is not
*             needed
*   retsid == address of session ID output
*   getvar_cb == XPath get varbind callback function
*   protocols_parent == pointer to val_value_t that may contain
*         a session-specific --protocols variable to set
*         == NULL if not used
*
* OUTPUTS:
*   *retsid == session ID, if no error
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    mgr_ses_new_session (const xmlChar *user,
                         const xmlChar *password,
                         const char *pubkeyfile,
                         const char *privkeyfile,
                         const xmlChar *target,
                         uint16 port,
                         ses_transport_t transport,
                         ncxmod_temp_progcb_t *progcb,
                         ses_id_t *retsid,
                         xpath_getvar_fn_t getvar_fn,
                         val_value_t *protocols_parent);


/********************************************************************
* FUNCTION mgr_ses_free_session
*
* Free a real session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
extern void
    mgr_ses_free_session (ses_id_t sid);


/********************************************************************
* FUNCTION mgr_ses_new_dummy_session
*
* Create a dummy session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized dummy SCB, or NULL if malloc error
*********************************************************************/
extern ses_cb_t *
    mgr_ses_new_dummy_session (void);


/********************************************************************
* FUNCTION mgr_ses_free_dummy_session
*
* Free a dummy session control block
*
* INPUTS:
*   scb == session control block to free
*
*********************************************************************/
extern void
    mgr_ses_free_dummy_session (ses_cb_t *scb);


/********************************************************************
* FUNCTION mgr_ses_process_first_ready
*
* Check the readyQ and process the first message, if any
*
* RETURNS:
*     TRUE if a message was processed
*     FALSE if the readyQ was empty
*********************************************************************/
extern boolean
    mgr_ses_process_first_ready (void);


/********************************************************************
* FUNCTION mgr_ses_fill_writeset
*
* Drain the ses_msg outreadyQ and set the specified fdset
* Used by mgr_ncxserver write_fd_set
*
* INPUTS:
*    fdset == pointer to fd_set to fill
*    maxfdnum == pointer to max fd int to fill in
*
* OUTPUTS:
*    *fdset is updated in
*    *maxfdnum may be updated
*
* RETURNS:
*    number of ready-for-output sessions found
*********************************************************************/

extern uint32
    mgr_ses_fill_writeset (fd_set *fdset,
			   int *maxfdnum);


/********************************************************************
* FUNCTION mgr_ses_get_first_outready
*
* Get the first ses_msg outreadyQ entry
* Will remove the entry from the Q
*
* RETURNS:
*    pointer to the session control block of the
*    first session ready to write output to a socket
*    or the screen or a file
*********************************************************************/

extern ses_cb_t *
    mgr_ses_get_first_outready (void);


/********************************************************************
* FUNCTION mgr_ses_get_first_session
*
* Get the first active session
*
* RETURNS:
*    pointer to the session control block of the
*    first active session found
*********************************************************************/
extern ses_cb_t *
    mgr_ses_get_first_session (void);


/********************************************************************
* FUNCTION mgr_ses_get_next_session
*
* Get the next active session
*
* INPUTS:
*   curscb == current session control block to use
*
* RETURNS:
*    pointer to the session control block of the
*    next active session found
*
*********************************************************************/
extern ses_cb_t *
    mgr_ses_get_next_session (ses_cb_t *curscb);


/********************************************************************
* FUNCTION mgr_ses_readfn
*
* Read callback function for a manager SSH2 session
*
* INPUTS:
*   s == session control block
*   buff == buffer to fill
*   bufflen == length of buff
*   erragain == address of return EAGAIN flag
*
* OUTPUTS:
*   *erragain == TRUE if ret value < zero means 
*                read would have blocked (not really an error)
*                FALSE if the ret value is not < zero, or
*                if it is, it is a real error
*
* RETURNS:
*   number of bytes read; -1 for error; 0 for connection closed
*********************************************************************/
extern ssize_t
    mgr_ses_readfn (void *scb,
		    char *buff,
		    size_t bufflen,
                    boolean *erragain);


/********************************************************************
* FUNCTION mgr_ses_writefn
*
* Write callback function for a manager SSH2 session
*
* INPUTS:
*   s == session control block
*
* RETURNS:
*   status of the write operation
*********************************************************************/
extern status_t
    mgr_ses_writefn (void *scb);


/********************************************************************
* FUNCTION mgr_ses_get_scb
*
* Retrieve the session control block
*
* INPUTS:
*   sid == manager session ID (not agent assigned ID)
*
* RETURNS:
*   pointer to session control block or NULL if invalid
*********************************************************************/
extern ses_cb_t *
    mgr_ses_get_scb (ses_id_t sid);


/********************************************************************
* FUNCTION mgr_ses_get_mscb
*
* Retrieve the manager session control block
*
* INPUTS:
*   scb == session control block to use
*
* RETURNS:
*   pointer to manager session control block or NULL if invalid
*********************************************************************/
extern mgr_scb_t *
    mgr_ses_get_mscb (ses_cb_t *scb);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_mgr_ses */
