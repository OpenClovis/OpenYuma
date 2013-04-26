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
#ifndef _H_agt_state
#define _H_agt_state
/*  FILE: agt_state.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF State Monitoring Data Model Module support

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
24-feb-09    abb      Begun.
*/

#include <xmlstring.h>

#ifndef _H_agt_not
#include "agt_not.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define AGT_STATE_MODULE      (const xmlChar *)"ietf-netconf-monitoring"

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
* FUNCTION agt_state_init
*
* INIT 1:
*   Initialize the server state monitor module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_state_init (void);


/********************************************************************
* FUNCTION agt_state_init2
*
* INIT 2:
*   Initialize the monitoring data structures
*   This must be done after the <running> config is loaded
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_state_init2 (void);


/********************************************************************
* FUNCTION agt_state_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_state_cleanup (void);


/********************************************************************
* FUNCTION agt_state_add_session
*
* Add a session entry to the netconf-state DM
*
* INPUTS:
*   scb == session control block to use for the info
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_state_add_session (ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_state_remove_session
*
* Remove a session entry from the netconf-state DM
*
* INPUTS:
*   sid == session ID to find and delete
*
*********************************************************************/
extern void
    agt_state_remove_session (ses_id_t sid);


/********************************************************************
* FUNCTION agt_state_add_module_schema
*
* Add a schema entry to the netconf-state DM
*
* INPUTS:
*   mod == module to add
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_state_add_module_schema (ncx_module_t *mod);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif


#endif            /* _H_agt_state */
