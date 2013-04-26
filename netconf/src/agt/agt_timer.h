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
#ifndef _H_agt_timer
#define _H_agt_timer

/*  FILE: agt_timer.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Handle timer services for the server


*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
23-jan-07    abb      Begun

*/

#include <time.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* timer callback function
 *
 * Process the timer expired event
 *
 * INPUTS:
 *    timer_id == timer identifier 
 *    cookie == context pointer, such as a session control block,
 *            passed to agt_timer_set function (may be NULL)
 *
 * RETURNS:
 *     0 == normal exit
 *    -1 == error exit, delete timer upon return
 */
typedef int (*agt_timer_fn_t) (uint32  timer_id,
			       void *cookie);


typedef struct agt_timer_cb_t_ {
    dlq_hdr_t       qhdr;
    boolean         timer_periodic;
    uint32          timer_id;
    agt_timer_fn_t  timer_cbfn;
    time_t          timer_start_time;
    uint32          timer_duration;   /* seconds */
    void           *timer_cookie;
} agt_timer_cb_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_timer_init
*
* Initialize the agt_timer module
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern void
    agt_timer_init (void);


/********************************************************************
* FUNCTION agt_timer_cleanup
*
* Cleanup the agt_timer module.
*
*********************************************************************/
extern void 
    agt_timer_cleanup (void);


/********************************************************************
* FUNCTION agt_timer_handler
*
* Handle an incoming server timer polling interval
* main routine called by agt_signal_handler
*
*********************************************************************/
extern void
    agt_timer_handler (void);


/********************************************************************
* FUNCTION agt_timer_create
*
* Malloc and start a new timer control block
*
* INPUTS:
*   seconds == number of seconds to wait between polls
*   is_periodic == TRUE if periodic timer
*                  FALSE if a 1-event timer
*   timer_fn == address of callback function to invoke when
*               the timer poll event occurs
*   cookie == address of user cookie to pass to the timer_fn
*   ret_timer_id == address of return timer ID
*
* OUTPUTS:
*  *ret_timer_id == timer ID for the allocated timer, 
*    if the return value is NO_ERR
*
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern status_t
    agt_timer_create (uint32   seconds,
                      boolean is_periodic,
                      agt_timer_fn_t  timer_fn,
                      void *cookie,
                      uint32 *ret_timer_id);


/********************************************************************
* FUNCTION agt_timer_restart
*
* Restart a timer with a new timeout value.
* If this is a periodic timer, then the interval
* will be changed to the new value.  Otherwise
* a 1-shot timer will just be reset to the new value
*
* INPUTS:
*   timer_id == timer ID to reset
*   seconds == new timeout value
*
* RETURNS:
*   status, NO_ERR if all okay,
*********************************************************************/
extern status_t
    agt_timer_restart (uint32 timer_id,
                       uint32 seconds);


/********************************************************************
* FUNCTION agt_timer_delete
*
* Remove and delete a timer control block
* periodic timers need to be deleted to be stopped
* 1-shot timers will be deleted automatically after
* they expire and the callback is invoked
*
* INPUTS:
*   timer_id == timer ID to destroy
*
*********************************************************************/
extern void
    agt_timer_delete (uint32  timer_id);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_timer */
