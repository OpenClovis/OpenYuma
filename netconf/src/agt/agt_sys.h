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
#ifndef _H_agt_sys
#define _H_agt_sys
/*  FILE: agt_sys.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF system.yang DM module support

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
04-jun-09    abb      Begun.
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tstamp
#include "tstamp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define AGT_SYS_MODULE     (const xmlChar *)"yuma-system"


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
* FUNCTION agt_sys_init
*
* INIT 1:
*   Initialize the server notification module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_sys_init (void);


/********************************************************************
* FUNCTION agt_sys_init2
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
    agt_sys_init2 (void);


/********************************************************************
* FUNCTION agt_sys_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_sys_cleanup (void);


/********************************************************************
* FUNCTION agt_sys_send_sysSessionStart
*
* Queue the <sysSessionStart> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
extern void
    agt_sys_send_sysSessionStart (const ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_sys_send_sysSessionEnd
*
* Queue the <sysSessionEnd> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   termreason == enum for the terminationReason leaf
*   killedby == session-id for killedBy leaf if termreason == "killed"
*               ignored otherwise
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
extern void
    agt_sys_send_sysSessionEnd (const ses_cb_t *scb,
				ses_term_reason_t termreason,
				ses_id_t killedby);


/********************************************************************
* FUNCTION agt_sys_send_sysConfigChange
*
* Queue the <sysConfigChange> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   auditrecQ == Q of rpc_audit_rec_t structs to use
*                for the notification payload contents
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
extern void
    agt_sys_send_sysConfigChange (const ses_cb_t *scb,
                                  dlq_hdr_t *auditrecQ);


/********************************************************************
* FUNCTION agt_sys_send_sysCapabilityChange
*
* Send a <sysCapabilityChange> event for a module
* being added
*
* Queue the <sysCapabilityChange> notification
*
* INPUTS:
*   changed_by == session control block that made the
*                 change to add this module
*             == NULL if the server made the change
*   is_add    == TRUE if the capability is being added
*                FALSE if the capability is being deleted
*   capstr == capability string that was added or deleted
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
extern void
    agt_sys_send_sysCapabilityChange (ses_cb_t *changed_by,
                                      boolean is_add,
                                      const xmlChar *capstr);


/********************************************************************
* FUNCTION agt_sys_send_sysConfirmedCommit
*
* Queue the <sysConfirmedCommit> notification
*
* INPUTS:
*   scb == session control block to use for payload values
*   event == enum for the confirmEvent leaf
*
* OUTPUTS:
*   notification generated and added to notificationQ
*
*********************************************************************/
extern void
    agt_sys_send_sysConfirmedCommit (const ses_cb_t *scb,
                                     ncx_confirm_event_t event);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif


#endif            /* _H_agt_sys */
