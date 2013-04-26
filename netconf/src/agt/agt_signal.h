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
#ifndef _H_agt_signal
#define _H_agt_signal

/*  FILE: agt_signal.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Handle interrupt signals for the server


*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22-jan-07    abb      Begun

*/

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_signal_init
*
* Initialize the agt_signal module
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
extern void
    agt_signal_init (void);


/********************************************************************
* FUNCTION agt_signal_cleanup
*
* Cleanup the agt_signal module.
*
*********************************************************************/
extern void 
    agt_signal_cleanup (void);


/********************************************************************
* FUNCTION agt_signal_handler
*
* Handle an incoming interrupt signal
*
* INPUTS:
*   intr == interrupt numer
*
*********************************************************************/
extern void
    agt_signal_handler (int intr);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_signal */
