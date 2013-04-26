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
#ifndef _H_agt_not
#define _H_agt_not
/*  FILE: agt_not.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF Notifications DM module support

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
30-may-09    abb      Begun.
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

#define AGT_NOT_MODULE1     (const xmlChar *)"notifications"
#define AGT_NOT_MODULE2     (const xmlChar *)"nc-notifications"

/* agt_not_subscription_t flags */


/* if set, stopTime is in the future */
#define AGT_NOT_FL_FUTURESTOP  bit0

/* if set, replayComplete is ready to be sent */
#define AGT_NOT_FL_RC_READY    bit1

/* if set replayComplete has been sent */
#define AGT_NOT_FL_RC_DONE     bit2

/* if set, notificationComplete is ready to be sent */
#define AGT_NOT_FL_NC_READY    bit3

/* if set, notificationComplete has been sent */
#define AGT_NOT_FL_NC_DONE     bit4


/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/* per-subscription state */
typedef enum agt_not_state_t_ {
    /* none == cleared memory; not set */
    AGT_NOT_STATE_NONE,

    /* init == during initialization */
    AGT_NOT_STATE_INIT,

    /* replay == during sending of selected replay buffer */
    AGT_NOT_STATE_REPLAY,

    /* timed == stopTime in future, after replayComplete
     * but before notificationComplete
     */
    AGT_NOT_STATE_TIMED,

    /* live == live delivery mode (no stopTime given) */
    AGT_NOT_STATE_LIVE,

    /* shutdown == forced shutdown mode; 
     * notificationComplete is being sent
     */
    AGT_NOT_STATE_SHUTDOWN
} agt_not_state_t;


/* server supported stream types */
typedef enum agt_not_stream_t_ {
    AGT_NOT_STREAM_NONE,
    AGT_NOT_STREAM_NETCONF
} agt_not_stream_t;


/* one notification message that will be sent to all
 * subscriptions and kept in the replay buffer (notificationQ)
 */
typedef struct agt_not_msg_t_ {
    dlq_hdr_t                qhdr;
    obj_template_t          *notobj;
    dlq_hdr_t                payloadQ;  /* Q of val_value_t */
    uint32                   msgid;
    xmlChar                  eventTime[TSTAMP_MIN_SIZE];
    val_value_t             *msg;     /* /notification element */
    val_value_t             *event;  /* ptr inside msg for filter */
} agt_not_msg_t;


/* one subscription that will be kept in the subscriptionQ */
typedef struct agt_not_subscription_t_ {
    dlq_hdr_t             qhdr;
    ses_cb_t             *scb;              /* back-ptr to session */
    ses_id_t              sid;              /* used when scb deleted */
    xmlChar              *stream;
    agt_not_stream_t      streamid;
    op_filtertyp_t        filtertyp;
    val_value_t          *filterval;
    val_value_t          *selectval;
    xmlChar               createTime[TSTAMP_MIN_SIZE];
    xmlChar              *startTime;       /* converted to UTC */
    xmlChar              *stopTime;        /* converted to UTC */
    uint32                flags;
    agt_not_msg_t        *firstreplaymsg;   /* back-ptr; could be zapped */
    agt_not_msg_t        *lastreplaymsg;    /* back-ptr; could be zapped */
    agt_not_msg_t        *lastmsg;          /* back-ptr; could be zapped */
    uint32                firstreplaymsgid; /* w/firstreplaymsg is deleted */
    uint32                lastreplaymsgid;  /* w/lastreplaymsg is deleted */
    uint32                lastmsgid;        /* w/ lastmsg is deleted */
    agt_not_state_t       state;
} agt_not_subscription_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION agt_not_init
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
    agt_not_init (void);


/********************************************************************
* FUNCTION agt_not_init2
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
    agt_not_init2 (void);


/********************************************************************
* FUNCTION agt_not_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_not_cleanup (void);


/********************************************************************
* FUNCTION agt_not_send_notifications
*
* Send out some notifications to the configured subscriptions
* if needed.
*
* Simple design:
*   go through all the subscriptions and send at most one 
*   notification to each one if needed.  This will build
*   in some throttling based on the ncxserver select loop
*   timeout (or however this function is called).
*
* OUTPUTS:
*     notifications may be written to some active sessions
*
* RETURNS:
*    number of notifications sent;
*    used for simple burst throttling
*********************************************************************/
extern uint32
    agt_not_send_notifications (void);


/********************************************************************
* FUNCTION agt_not_clean_eventlog
*
* Remove any delivered notifications when the replay buffer
* size is set to zero
*
*********************************************************************/
extern void
    agt_not_clean_eventlog (void);


/********************************************************************
* FUNCTION agt_not_remove_subscription
*
* Remove and expire a subscription with the specified session ID.
* The session is being removed.
*
* INPUTS:
*    sid == session ID to use
*********************************************************************/
extern void
    agt_not_remove_subscription (ses_id_t sid);


/********************************************************************
* FUNCTION agt_not_new_notification
* 
* Malloc and initialize the fields in an agt_not_msg_t
*
* INPUTS:
*   eventType == object template of the event type
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern agt_not_msg_t * 
    agt_not_new_notification (obj_template_t *eventType);


/********************************************************************
* FUNCTION agt_not_free_notification
* 
* Scrub the memory in an agt_not_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    notif == agt_not_template_t to delete
*********************************************************************/
extern void 
    agt_not_free_notification (agt_not_msg_t *notif);


/********************************************************************
* FUNCTION agt_not_add_to_payload
*
* Queue the specified value node into the payloadQ
* for the specified notification
*
* INPUTS:
*   notif == notification to send
*   val == value to add to payloadQ
*            !!! THIS IS LIVE MALLOCED MEMORY PASSED OFF
*            !!! TO THIS FUNCTION.  IT WILL BE FREED LATER
*            !!! DO NOT CALL val_free_value
*            !!! AFTER THIS CALL
*
* OUTPUTS:
*   val added to the notif->payloadQ
*
*********************************************************************/
extern void
    agt_not_add_to_payload (agt_not_msg_t *notif,
			    val_value_t *val);


/********************************************************************
* FUNCTION agt_not_queue_notification
*
* Queue the specified notification in the replay log.
* It will be sent to all the active subscriptions
* as needed.
*
* INPUTS:
*   notif == notification to send
*            !!! THIS IS LIVE MALLOCED MEMORY PASSED OFF
*            !!! TO THIS FUNCTION.  IT WILL BE FREED LATER
*            !!! DO NOT CALL agt_not_free_notification
*            !!! AFTER THIS CALL
*
* OUTPUTS:
*   message added to the notificationQ
*
*********************************************************************/
extern void
    agt_not_queue_notification (agt_not_msg_t *notif);


/********************************************************************
* FUNCTION agt_not_is_replay_event
*
* Check if the specified notfication is the replayComplete
* or notificationComplete notification events
*
* INPUTS:
*   notifobj == notification object to check
*
* RETURNS:
*   TRUE if the notification object is one of the special
*        replay buffer events
*   FALSE otherwise
*********************************************************************/
extern boolean
    agt_not_is_replay_event (const obj_template_t *notifobj);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_agt_not */
