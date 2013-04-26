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
#ifndef _H_plock
#define _H_plock
/*  FILE: plock.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    RFC 57517 partial lock support


*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
21-jun-10    abb      Begun.

*/

#ifndef _H_plock_cb
#include "plock_cb.h"
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
*                        T Y P E S                                  *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION plock_get_id
*
* Get the lock ID for this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   the lock ID for this lock
*********************************************************************/
extern plock_id_t
    plock_get_id (plock_cb_t *plcb);


/********************************************************************
* FUNCTION plock_get_sid
*
* Get the session ID holding this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   session ID that owns this lock
*********************************************************************/
extern uint32
    plock_get_sid (plock_cb_t *plcb);


/********************************************************************
* FUNCTION plock_get_timestamp
*
* Get the timestamp of the lock start time
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   timestamp in date-time format
*********************************************************************/
extern const xmlChar *
    plock_get_timestamp (plock_cb_t *plcb);



/********************************************************************
* FUNCTION plock_get_final_result
*
* Get the session ID holding this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   session ID that owns this lock
*********************************************************************/
extern xpath_result_t *
    plock_get_final_result (plock_cb_t *plcb);


/********************************************************************
* FUNCTION plock_get_first_select
*
* Get the first select XPath control block for the partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   pointer to first xpath_pcb_t for the lock
*********************************************************************/
extern xpath_pcb_t *
    plock_get_first_select (plock_cb_t *plcb);


/********************************************************************
* FUNCTION plock_get_next_select
*
* Get the next select XPath control block for the partial lock
*
* INPUTS:
*   xpathpcb == current select block to use
*
* RETURNS:
*   pointer to first xpath_pcb_t for the lock
*********************************************************************/
extern xpath_pcb_t *
    plock_get_next_select (xpath_pcb_t *xpathpcb);


/********************************************************************
* FUNCTION plock_add_select
*
* Add a select XPath control block to the partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*   xpathpcb == xpath select block to add
*   result == result struct to add
*
*********************************************************************/
extern void
    plock_add_select (plock_cb_t *plcb,
                      xpath_pcb_t *xpathpcb,
                      xpath_result_t *result);


/********************************************************************
* FUNCTION plock_make_final_result
*
* Create a final XPath result for all the partial results
*
* This does not add the partial lock to the target config!
* This is an intermediate step!
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*    status; NCX_ERR_INVALID_VALUE if the final nodeset is empty
*********************************************************************/
extern status_t
    plock_make_final_result (plock_cb_t *plcb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_plock */
