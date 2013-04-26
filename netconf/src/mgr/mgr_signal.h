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
#ifndef _H_mgr_signal
#define _H_mgr_signal

/*  FILE: mgr_signal.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Handle interrupt signals for the manager


*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
20-feb-07    abb      Begun

*/

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

/* don't rely on GNU extension being defined */
typedef void (*sighandler_t)(int signum);


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


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
extern void
    mgr_signal_init (void);


/********************************************************************
* FUNCTION mgr_signal_cleanup
*
* Cleanup the mgr_signal module.
*
*********************************************************************/
extern void 
    mgr_signal_cleanup (void);


/********************************************************************
* FUNCTION mgr_signal_handler
*
* Handle an incoming interrupt signal
*
* INPUTS:
*   intr == interrupt numer
*
*********************************************************************/
extern void
    mgr_signal_handler (int intr);


/********************************************************************
* FUNCTION mgr_signal_install_break_handler (control-c)
*
* Install an application-specific handler for the break interrupt
*
* INPUTS:
*   handler == interrupt handler to call when control-C is entered
*
*********************************************************************/
extern void 
    mgr_signal_install_break_handler (sighandler_t handler);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_mgr_signal */
