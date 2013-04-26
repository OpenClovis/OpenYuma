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
#ifndef _H_ses_msg
#define _H_ses_msg
/*  FILE: ses_msg.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF Session Message Common definitions module

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
20-jan-07    abb      Begun.
*/

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_ses
#include  "ses.h"
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
* FUNCTION ses_msg_init
*
* Initialize the session message manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    ses_msg_init (void);



/********************************************************************
* FUNCTION ses_msg_cleanup
*
* Cleanup the session message manager module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    ses_msg_cleanup (void);


/********************************************************************
* FUNCTION ses_msg_new_msg
*
* Malloc a new session message control header
*
* INPUTS:
*   msg == address of ses_msg_t pointer that will be set
*
* OUTPUTS:
*   *msg == malloced session message struct (if NO_ERR return)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ses_msg_new_msg (ses_msg_t **msg);


/********************************************************************
* FUNCTION ses_msg_free_msg
*
* Free the session message and all its buffer chunks
*
* INPUTS:
*   scb == session control block owning the message
*   msg == message to free (already removed from any Q)
*
*********************************************************************/
extern void
    ses_msg_free_msg (ses_cb_t *scb,
		      ses_msg_t *msg);


/********************************************************************
* FUNCTION ses_msg_new_buff
*
* Malloc a new session buffer chuck
*
* Note that the buffer memory is not cleared after each use
* since this is not needed for byte stream IO
*
* INPUTS:
*   scb == session control block to malloc a new message for
*   outbuff == TRUE if this is for outgoing message
*              FALSE if this is for incoming message
*   buff == address of ses_msg_buff_t pointer that will be set
*
* OUTPUTS:
*   *buff == malloced session buffer chunk (if NO_ERR return)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ses_msg_new_buff (ses_cb_t *scb, 
                      boolean outbuff,
                      ses_msg_buff_t **buff);


/********************************************************************
* FUNCTION ses_msg_free_buff
*
* Free the session buffer chunk
*
* INPUTS:
*   scb == session control block owning the message
*   buff == buffer to free (already removed from any Q)
*
* RETURNS:
*   none
*********************************************************************/
extern void
    ses_msg_free_buff (ses_cb_t *scb,
		       ses_msg_buff_t *buff);


/********************************************************************
* FUNCTION ses_msg_write_buff
*
* Add some text to the message buffer
*
* INPUTS:
*   scb == session control block to use
*   buff == buffer to write to
*   ch  == xmlChar to write
*
* RETURNS:
*   status_t
*
*********************************************************************/
extern status_t
    ses_msg_write_buff (ses_cb_t *scb,
                        ses_msg_buff_t *buff,
                        uint32 ch);


/********************************************************************
* FUNCTION ses_msg_send_buffs
*
* Send multiple buffers to the session client socket
* Tries to send one packet at maximum MTU
*
* INPUTS:
*   scb == session control block
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ses_msg_send_buffs (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_msg_new_output_buff
*
* Put the current outbuff on the outQ
* Put the session on the outreadyQ if it is not already there
* Try to allocate a new buffer for the session
*
* INPUTS:
*   scb == session control block
*
* OUTPUTS:
*   scb->outbuff, scb->outready, and scb->outQ will be changed
*   
* RETURNS:
*   status, could return malloc or buffers exceeded error
*********************************************************************/
extern status_t
    ses_msg_new_output_buff (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_msg_make_inready
*
* Put the session on the inreadyQ if it is not already there
*
* INPUTS:
*   scb == session control block
*
* OUTPUTS:
*   scb->inready will be queued on the inreadyQ
*********************************************************************/
extern void
    ses_msg_make_inready (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_msg_make_outready
*
* Put the session on the outreadyQ if it is not already there
*
* INPUTS:
*   scb == session control block
*
* OUTPUTS:
*   scb->outready will be queued on the outreadyQ
*********************************************************************/
extern void
    ses_msg_make_outready (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_msg_finish_outmsg
*
* Put the outbuff in the outQ if non-empty
* Put the session on the outreadyQ if it is not already there
*
* INPUTS:
*   scb == session control block
*
* OUTPUTS:
*   scb->outready will be queued on the outreadyQ
*********************************************************************/
extern void
    ses_msg_finish_outmsg (ses_cb_t *scb);


/********************************************************************
* FUNCTION ses_msg_get_first_inready
*
* Dequeue the first entry in the inreadyQ, if any
*
* RETURNS:
*    first entry in the inreadyQ or NULL if none
*********************************************************************/
extern ses_ready_t *
    ses_msg_get_first_inready (void);


/********************************************************************
* FUNCTION ses_msg_get_first_outready
*
* Dequeue the first entry in the outreadyQ, if any
*
* RETURNS:
*    first entry in the outreadyQ or NULL if none
*********************************************************************/
extern ses_ready_t *
    ses_msg_get_first_outready (void);


/********************************************************************
* FUNCTION ses_msg_dump
*
* Dump the message contents
*
* INPUTS:
*   msg == message to dump
*   text == start text before message dump (may be NULL)
*
*********************************************************************/
extern void
    ses_msg_dump (const ses_msg_t *msg,
		  const xmlChar *text);


/********************************************************************
* FUNCTION ses_msg_add_framing
*
* Add the base:1.1 framing chars to the buffer and adjust
* the buffer size pointers
*
* INPUTS:
*   scb == session control block
*   buff == buffer control block
*
* OUTPUTS:
*   framing chars added to buff->buff
*
*********************************************************************/
 extern void
    ses_msg_add_framing (ses_cb_t *scb,
                         ses_msg_buff_t *buff);


/********************************************************************
* FUNCTION ses_msg_init_buff
*
* Init the buffer fields
*
* INPUTS:
*   scb == session control block
*   outbuff == TRUE if oupput buffer; FALSE if input buffer
*   buff == buffer to send
*********************************************************************/
extern void
    ses_msg_init_buff (ses_cb_t *scb,
                       boolean outbuff,
                       ses_msg_buff_t *buff);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_ses_msg */
