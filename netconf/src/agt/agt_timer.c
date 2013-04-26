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
/*  FILE: agt_timer.c

    Handle timer services
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
23jan07      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_timer.h"
#include "log.h"
#include "ncx.h"
#include "status.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define AGT_TIMER_SKIP_COUNT 10

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                            *
*                                                                   *
*********************************************************************/

static boolean agt_timer_init_done = FALSE;

static dlq_hdr_t   timer_cbQ;

static uint32      next_id;

static uint32      skip_count;

/********************************************************************
* FUNCTION get_timer_id
*
* Allocate a timer ID
*
* INPUTS:
*   none
* RETURNS:
*   timer_id or zero if none available
*********************************************************************/
static agt_timer_cb_t *
    find_timer_cb (uint32 timer_id)
{
    agt_timer_cb_t *timer_cb;

    for (timer_cb = (agt_timer_cb_t *)
             dlq_firstEntry(&timer_cbQ);
         timer_cb != NULL;
         timer_cb = (agt_timer_cb_t *)
             dlq_nextEntry(timer_cb)) {
        if (timer_cb->timer_id == timer_id) {
            return timer_cb;
        }
    }
    return NULL;

}  /* find_timer_cb */


/********************************************************************
* FUNCTION get_timer_id
*
* Allocate a timer ID
*
* INPUTS:
*   none
* RETURNS:
*   timer_id or zero if none available
*********************************************************************/
static uint32
    get_timer_id (void)
{
    agt_timer_cb_t *timer_cb;
    uint32          id;

    if (next_id < NCX_MAX_UINT) {
        return next_id++;
    }

    if (dlq_empty(&timer_cbQ)) {
        next_id = 1;
        return next_id++;
    }

    id = 1;
    while (id < NCX_MAX_UINT) {
        timer_cb = find_timer_cb(id);
        if (timer_cb == NULL) {
            return id;
        }
        id++;
    }
    return 0;

} /* get_timer_id */


/********************************************************************
* FUNCTION new_timer_cb
*
* Malloc and init a new timer control block
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
static agt_timer_cb_t *
    new_timer_cb (void)
{
    agt_timer_cb_t *timer_cb;

    timer_cb = m__getObj(agt_timer_cb_t);
    if (timer_cb == NULL) {
        return NULL;
    }
    memset(timer_cb, 0x0, sizeof(agt_timer_cb_t));
    return timer_cb;

} /* new_timer_cb */


/********************************************************************
* FUNCTION free_timer_cb
*
* Clean and free a timer control block
*
* INPUTS:
*   timer_cb == control block to free
*
*********************************************************************/
static void
    free_timer_cb (agt_timer_cb_t *timer_cb)
{

    m__free(timer_cb);

} /* free_timer_cb */


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
void
    agt_timer_init (void)
{
    if (!agt_timer_init_done) {
        dlq_createSQue(&timer_cbQ);
        next_id = 1;
        skip_count = 0;
        agt_timer_init_done = TRUE;
    }

} /* agt_timer_init */


/********************************************************************
* FUNCTION agt_timer_cleanup
*
* Cleanup the agt_timer module.
*
*********************************************************************/
void 
    agt_timer_cleanup (void)
{
    agt_timer_cb_t *timer_cb;

    if (agt_timer_init_done) {
        while (!dlq_empty(&timer_cbQ)) {
            timer_cb = (agt_timer_cb_t *)dlq_deque(&timer_cbQ);
            free_timer_cb(timer_cb);
        }
        agt_timer_init_done = FALSE;
    }

} /* agt_timer_cleanup */


/********************************************************************
* FUNCTION agt_timer_handler
*
* Handle an incoming agent timer polling interval
* main routine called by agt_signal_handler
*
*********************************************************************/
void 
    agt_timer_handler (void)
{
    agt_timer_cb_t  *timer_cb, *next_timer_cb;
    time_t           timenow;
    double           timediff;
    int              retval;

    if (skip_count < AGT_TIMER_SKIP_COUNT) {
        skip_count++;
        return;
    } else {
        skip_count = 0;
    }

    (void)time(&timenow);

    for (timer_cb = (agt_timer_cb_t *)dlq_firstEntry(&timer_cbQ);
         timer_cb != NULL;
         timer_cb = next_timer_cb) {

        next_timer_cb = (agt_timer_cb_t *)dlq_nextEntry(timer_cb);

        timediff = difftime(timenow, timer_cb->timer_start_time);
        if (timediff >= (double)timer_cb->timer_duration) {
            if (LOGDEBUG3) {
                log_debug3("\nagt_timer: timer %u popped",
                           timer_cb->timer_id);
            }

            retval = (*timer_cb->timer_cbfn)(timer_cb->timer_id,
                                             timer_cb->timer_cookie);
            if (retval != 0 || !timer_cb->timer_periodic) {
                /* destroy this timer */
                dlq_remove(timer_cb);
                free_timer_cb(timer_cb);
            } else {
                /* reset this periodic timer */
                (void)time(&timer_cb->timer_start_time);
            }
        }
    }

} /* agt_timer_handler */


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
status_t
    agt_timer_create (uint32 seconds,
                      boolean is_periodic,
                      agt_timer_fn_t  timer_fn,
                      void *cookie,
                      uint32 *ret_timer_id)
{
    agt_timer_cb_t *timer_cb;
    uint32          timer_id;

#ifdef DEBUG
    if (timer_fn == NULL || ret_timer_id == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (seconds == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    *ret_timer_id = 0;
    timer_id = get_timer_id();
    if (timer_id == 0) {
        return ERR_NCX_RESOURCE_DENIED;
    }

    timer_cb = new_timer_cb();
    if (timer_cb == NULL) {
        return ERR_INTERNAL_MEM;
    }

    *ret_timer_id = timer_id;
    timer_cb->timer_id = timer_id;
    timer_cb->timer_periodic = is_periodic;
    timer_cb->timer_cbfn = timer_fn;
    (void)time(&timer_cb->timer_start_time);
    timer_cb->timer_duration = seconds;
    timer_cb->timer_cookie = cookie;

    dlq_enque(timer_cb, &timer_cbQ);
    return NO_ERR;

} /* agt_timer_create */


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
status_t
    agt_timer_restart (uint32 timer_id,
                       uint32 seconds)
{
    agt_timer_cb_t *timer_cb;

#ifdef DEBUG
    if (seconds == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    timer_cb = find_timer_cb(timer_id);
    if (timer_cb == NULL) {
        return ERR_NCX_NOT_FOUND;
    }

    (void)time(&timer_cb->timer_start_time);
    timer_cb->timer_duration = seconds;
    return NO_ERR;

} /* agt_timer_restart */


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
void
    agt_timer_delete (uint32 timer_id)
{
    agt_timer_cb_t *timer_cb;

    timer_cb = find_timer_cb(timer_id);
    if (timer_cb == NULL) {
        log_warn("\nagt_timer: delete unknown timer '%u'",
                 timer_id);
        return;
    }

    dlq_remove(timer_cb);
    free_timer_cb(timer_cb);

} /* agt_timer_delete */


/* END file agt_timer.c */


