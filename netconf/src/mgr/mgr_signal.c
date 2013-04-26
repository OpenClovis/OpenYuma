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
/*  FILE: mgr_signal.c

    Handle interrupt signals for the manager
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
20jan07      abb      begun; start from agt_signal.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <signal.h>
#include  <unistd.h>
#include  <sys/time.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_signal
#include "mgr_signal.h"
#endif

#ifdef NOT_YET
#ifndef _H_mgr_timer
#include "mgr_timer.h"
#endif
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define MGR_SIGNAL_DEBUG 1
#endif

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean mgr_signal_init_done = FALSE;
static sighandler_t sh_int, sh_hup, sh_term, sh_pipe, sh_alarm, ctl_c;


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION mgr_signal_init
*
* Initialize the mgr_signal module
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
void
    mgr_signal_init (void)
{
    if (!mgr_signal_init_done) {
        ctl_c = NULL;

        /* setup mgr_signal_handler */
        sh_int = signal(SIGINT, mgr_signal_handler);
        sh_hup = signal(SIGHUP, mgr_signal_handler);
        sh_term = signal(SIGTERM, mgr_signal_handler);
        sh_pipe = signal(SIGPIPE, mgr_signal_handler);
        sh_alarm = signal(SIGALRM, mgr_signal_handler);
        mgr_signal_init_done = TRUE;
    }

} /* mgr_signal_init */


/********************************************************************
* FUNCTION mgr_signal_cleanup
*
* Cleanup the mgr_signal module.
*
*********************************************************************/
void 
    mgr_signal_cleanup (void)
{
    if (mgr_signal_init_done) {
        /* restore previous handlers */
        signal(SIGINT, sh_int);
        signal(SIGHUP, sh_hup);
        signal(SIGTERM, sh_term);
        signal(SIGPIPE, sh_pipe);
        signal(SIGALRM, sh_alarm);
        ctl_c = NULL;
        mgr_signal_init_done = FALSE;
    }

} /* mgr_signal_cleanup */


/********************************************************************
* FUNCTION mgr_signal_handler
*
* Handle an incoming interrupt signal
*
* INPUTS:
*   intr == interrupt numer
*
*********************************************************************/
void 
    mgr_signal_handler (int intr)
{
    switch (intr) {
    case SIGINT:
        /* control-C */
        if (ctl_c) {
            (*ctl_c)(intr);
        } else {
            mgr_request_shutdown();
        }
        break;
    case SIGHUP:
        /* hang up treated as reset */
        mgr_request_shutdown();
        break;
    case SIGTERM:
        /* kill -1 */
        mgr_request_shutdown();
        break;
    case SIGPIPE:
        /* broken pipe ignored */
        break;
    case SIGALRM:
        /* dispatch to the agent timer handler */
        /* mgr_timer_handler(); */
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* mgr_signal_handler */


/********************************************************************
* FUNCTION mgr_signal_install_break_handler (control-c)
*
* Install an application-specific handler for the break interrupt
*
* INPUTS:
*   handler == interrupt handler to call when control-C is entered
*
*********************************************************************/
void 
    mgr_signal_install_break_handler (sighandler_t handler)
{
    ctl_c = handler;

} /* mgr_signal_install_break_handler */


/* END file mgr_signal.c */


