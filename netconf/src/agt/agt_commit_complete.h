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
#ifndef _H_agt_commit_complete_cb
#define _H_agt_commit_complete_cb

/*  FILE: agt_cb.h
*********************************************************************
* P U R P O S E
*********************************************************************
    NETCONF Server Commit Complete callback handler
    This file contains functions to support registering, 
    unregistering and execution of commit complete callbacks. 

*********************************************************************
* C H A N G E	 H I S T O R Y
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-oct-11    mp       First draft.
*/

#include <xmlstring.h>

#include "procdefs.h"
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
* T Y P E D E F S
*********************************************************************/
/** Typedef of the commit_complete callback */
typedef status_t (*agt_commit_complete_cb_t)(void);

/********************************************************************
* F U N C T I O N S
*********************************************************************/

/**
 * Initialise the callback commit module.
 */
extern void agt_commit_complete_init( void );

/**
 * Cleanup the callback commit module.
 */
extern void agt_commit_complete_cleanup( void );

/**
 * Register a commit complete callback.
 * This function registers a commit-complete callback that will be
 * called after all changes to the candidate database have been
 * committed. The function will be called after that final SIL commit
 * operation. If a commit complete operation is already registered for
 * the module it will be replaced.
 *
 * \param modname the name of the module registering the callback
 * \param cb the commit complete function.
 * \return the status of the operation.
 */
extern status_t agt_commit_complete_register( const xmlChar *modname,
                                              agt_commit_complete_cb_t cb );

/**
 * Unregister a commit complete callback.
 * This function unregisters a commit-complete callback.
 *
 * \param modname the name of the module unregistering the callback
 */
extern void agt_commit_complete_unregister( const xmlChar *modname );

/**
 * Complete a commit operation.
 * This function simply calls each registered commit complete
 * callback. If a commit complete operation fails the status of the
 * failing operation is returned immediately and no further commit
 * complete callbacks are made.
 *
 * \return the ERR_OK or the status of the first failing callback.
 */
extern status_t agt_commit_complete( void );

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif // _H_agt_commit_complete_cb

