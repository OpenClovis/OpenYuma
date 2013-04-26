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
/*  FILE: agt_not.c

   NETCONF Notification Data Model implementation: Agent Side Support
   RFC 5277 version

identifiers:
container /netconf
container /netconf/streams
list /netconf/streams/stream
leaf /netconf/streams/stream/name
leaf /netconf/streams/stream/description
leaf /netconf/streams/stream/replaySupport
leaf /netconf/streams/stream/replayLogCreationTime
notification /replayComplete
notification /notificationComplete


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24feb09      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>
#include  <unistd.h>
#include  <errno.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_not.h"
#include "agt_rpc.h"
#include "agt_ses.h"
#include "agt_tree.h"
#include "agt_util.h"
#include "agt_xpath.h"
#include "cfg.h"
#include "getcb.h"
#include "log.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "rpc.h"
#include "ses.h"
#include "ses_msg.h"
#include "status.h"
#include "tstamp.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define notifications_N_create_subscription \
    (const xmlChar *)"create-subscription"
#define notifications_N_eventTime (const xmlChar *)"eventTime"
#define notifications_N_create_subscription_stream \
    (const xmlChar *)"stream"
#define notifications_N_create_subscription_filter \
    (const xmlChar *)"filter"
#define notifications_N_create_subscription_startTime \
    (const xmlChar *)"startTime"
#define notifications_N_create_subscription_stopTime \
    (const xmlChar *)"stopTime"
#define nc_notifications_N_netconf (const xmlChar *)"netconf"
#define nc_notifications_N_streams (const xmlChar *)"streams"
#define nc_notifications_N_stream (const xmlChar *)"stream"
#define nc_notifications_N_name (const xmlChar *)"name"
#define nc_notifications_N_description \
    (const xmlChar *)"description"
#define nc_notifications_N_replaySupport \
    (const xmlChar *)"replaySupport"
#define nc_notifications_N_replayLogCreationTime \
    (const xmlChar *)"replayLogCreationTime"
#define nc_notifications_N_replayComplete \
    (const xmlChar *)"replayComplete"
#define nc_notifications_N_notificationComplete \
    (const xmlChar *)"notificationComplete"


#define AGT_NOT_SEQID_MOD   (const xmlChar *)"yuma-system"

/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean              agt_not_init_done = FALSE;

/* notifications.yang */
static ncx_module_t         *notifmod;

/* nc-notifications.yang */
static ncx_module_t         *ncnotifmod;

/* Q of agt_not_subscription_t 
 * one entry is created for each create-subscription call
 * if stopTime is given then the subscription will be
 * automatically deleted when all the replay is complete.
 * Otherwise, a subscription is deleted when the session
 * is terminated
 */
static dlq_hdr_t             subscriptionQ;

/* Q of agt_not_msg_t 
 * these are the messages that represent the replay buffer
 * only system-wide notifications are stored in this Q
 * the replayComplete and notificationComplete events are
 * generated special-case, and not stored for replay
 */
static dlq_hdr_t             notificationQ;

/* cached pointer to the <notification> element template */
static obj_template_t *notificationobj;

/* cached pointer to the /notification/eventTime element template */
static obj_template_t *eventTimeobj;

/* cached pointer to the /ncn:replayComplete element template */
static obj_template_t *replayCompleteobj;

/* cached pointer to the /ncn:notificationComplete element template */
static obj_template_t *notificationCompleteobj;

/* cached pointer to the /ncx:sequence-id element template */
static obj_template_t *sequenceidobj;

/* flag to signal quick exit */
static boolean               anySubscriptions;

/* auto-increment message index */
static uint32                msgid;

/* keep track of eventlog size */
static uint32                notification_count;

/********************************************************************
* FUNCTION free_subscription
*
* Clean and free a subscription control block
*
* INPUTS:
*    sub == subscription to delete
*
*********************************************************************/
static void
    free_subscription (agt_not_subscription_t *sub)
{
    if (sub->stream) {
        m__free(sub->stream);
    }
    if (sub->startTime) {
        m__free(sub->startTime);
    }
    if (sub->stopTime) {
        m__free(sub->stopTime);
    }
    if (sub->filterval) {
        val_free_value(sub->filterval);
    }
    if (sub->scb) {
        sub->scb->notif_active = FALSE;
    }

    m__free(sub);

}  /* free_subscription */


/********************************************************************
* FUNCTION new_subscription
*
* Malloc and fill in a new subscription control block
*
* INPUTS:
*     scb == session this subscription is for
*     stream == requested stream ID
*     curTime == current time for creation time
*     startTime == replay start time (may be NULL)
*           !!! THIS MALLOCED NODE IS FREED LATER !!!!
*     stopTime == replayStopTime (may be NULL)
*           !!! THIS MALLOCED NODE IS FREED LATER !!!!
*     futurestop == TRUE if stopTime in the future
*                   FALSE if not set or not in the future
*     filtertype == internal filter type
*     filterval == filter value node passed from PDU
*           !!! THIS MALLOCED NODE IS FREED LATER !!!!
*     selectval == back-ptr into filterval->select value node
*                  only used if filtertype == OP_FILTER_XPATH
*
* RETURNS:
*    pointer to malloced struct or NULL if no memoryerror
*********************************************************************/
static agt_not_subscription_t *
    new_subscription (ses_cb_t *scb,
                      const xmlChar *stream,
                      const xmlChar *curTime,
                      xmlChar *startTime,
                      xmlChar *stopTime,
                      boolean futurestop,
                      op_filtertyp_t  filtertype,
                      val_value_t *filterval,
                      val_value_t *selectval)
{
    agt_not_subscription_t  *sub;
    agt_not_stream_t         streamid;

    if (!xml_strcmp(stream, NCX_DEF_STREAM_NAME)) {
        streamid = AGT_NOT_STREAM_NETCONF;
    } else {
        /*** !!! ***/
        SET_ERROR(ERR_INTERNAL_VAL);
        streamid = AGT_NOT_STREAM_NETCONF;
    }

    sub = m__getObj(agt_not_subscription_t);
    if (!sub) {
        return NULL;
    }
    memset(sub, 0x0, sizeof(agt_not_subscription_t));

    sub->stream = xml_strdup(stream);
    if (!sub->stream) {
        free_subscription(sub);
        return NULL;
    }

    sub->startTime = startTime;
    sub->stopTime = stopTime;
    sub->scb = scb;
    sub->sid = scb->sid;
    sub->streamid = streamid;
    xml_strcpy(sub->createTime, curTime);
    sub->filtertyp = filtertype;
    sub->filterval = filterval;
    sub->selectval = selectval;
    if (futurestop) {
        sub->flags = AGT_NOT_FL_FUTURESTOP;
    }
    sub->state = AGT_NOT_STATE_INIT;

    /* prevent any idle timeout */
    scb->notif_active = TRUE;

    return sub;

}  /* new_subscription */


/********************************************************************
* FUNCTION create_subscription_validate
*
* create-subscription : validate params callback
*
* INPUTS:
*    see rpc/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    create_subscription_validate (ses_cb_t *scb,
                                  rpc_msg_t *msg,
                                  xml_node_t *methnode)
{
    const xmlChar           *stream, *startTime, *stopTime;
    xmlChar                 *starttime_utc, *stoptime_utc;
    val_value_t             *valstream, *valfilter, *valselect;
    val_value_t             *valstartTime, *valstopTime;
    agt_not_subscription_t  *sub, *testsub;
    xmlChar                  tstampbuff[TSTAMP_MIN_SIZE];
    int                      ret;
    status_t                 res, res2, filterres;
    boolean                  futurestop, isnegative;
    op_filtertyp_t           filtertyp;

    res = NO_ERR;
    filterres = NO_ERR;
    filtertyp = OP_FILTER_NONE;
    valstream = NULL;
    valfilter = NULL;
    valselect = NULL;
    valstartTime = NULL;
    valstopTime = NULL;
    startTime = NULL;
    stopTime = NULL;
    starttime_utc = NULL;
    stoptime_utc = NULL;
    stream = NCX_DEF_STREAM_NAME;
    tstamp_datetime(tstampbuff);
    futurestop = FALSE;
    isnegative = FALSE;

    /* get the stream parameter */
    valstream = 
        val_find_child(msg->rpc_input, 
                       AGT_NOT_MODULE1,
                       notifications_N_create_subscription_stream);
    if (valstream) {
        if (valstream->res == NO_ERR) {
            stream = VAL_STR(valstream);
            if (xml_strcmp(NCX_DEF_STREAM_NAME, stream)) {
                /* not the hard-wired NETCONF stream and
                 * no other strams supported at this time
                 * report the error
                 */
                res = ERR_NCX_NOT_FOUND;
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_STRING, 
                                 stream,
                                 NCX_NT_VAL, 
                                 valstream);
            }  /* else name==NETCONF: OK */
        } /* else error already reported */
    } /* else default NETCONF stream is going to be used */

    /* get the filter parameter */
    valfilter = 
        val_find_child(msg->rpc_input, 
                       AGT_NOT_MODULE1,
                       notifications_N_create_subscription_filter);
    if (valfilter) {
        if (valfilter->res == NO_ERR) {
            /* check if the optional filter parameter is ok */
            filterres = agt_validate_filter_ex(scb, msg, valfilter);
            if (filterres == NO_ERR) {
                filtertyp = msg->rpc_filter.op_filtyp;
                if (filtertyp == OP_FILTER_XPATH) {
                    valselect = msg->rpc_filter.op_filter;
                }
            } /* else error already recorded if not NO_ERR */
        }
    }

    /* get the startTime parameter */
    valstartTime = 
        val_find_child(msg->rpc_input, 
                       AGT_NOT_MODULE1,
                       notifications_N_create_subscription_startTime);
    if (valstartTime) {
        if (valstartTime->res == NO_ERR) {
            startTime = VAL_STR(valstartTime);
        }
    } 

    /* get the stopTime parameter */
    valstopTime = 
        val_find_child(msg->rpc_input, 
                       AGT_NOT_MODULE1,
                       notifications_N_create_subscription_stopTime);
    if (valstopTime) {
        if (valstopTime->res == NO_ERR) {
            stopTime = VAL_STR(valstopTime);
        }
    } 

    if (startTime || stopTime) {
        res = NO_ERR;
        /* Normalize the xsd:dateTime strings first */
        if (startTime) {
            isnegative = FALSE;
            starttime_utc = 
                tstamp_convert_to_utctime(startTime,
                                          &isnegative,
                                          &res);
            if (!starttime_utc || isnegative) {
                if (isnegative) {
                    res = ERR_NCX_INVALID_VALUE;
                }
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_STRING, 
                                 startTime,
                                 NCX_NT_VAL, 
                                 valstartTime);
            }
        }

        if (res == NO_ERR && stopTime) {
            isnegative = FALSE;
            res = NO_ERR;
            stoptime_utc = 
                tstamp_convert_to_utctime(stopTime,
                                          &isnegative,
                                          &res);
            if (!stoptime_utc || isnegative) {
                if (isnegative) {
                    res = ERR_NCX_INVALID_VALUE;
                }
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_STRING, 
                                 stopTime,
                                 NCX_NT_VAL, 
                                 valstopTime);
            }
        }

        /* check the start time against 'now' */
        if (res == NO_ERR && starttime_utc) {
            ret = xml_strcmp(starttime_utc, tstampbuff);
            if (ret > 0) {
                res = ERR_NCX_BAD_ELEMENT;
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_STRING, 
                                 startTime,
                                 NCX_NT_VAL, 
                                 valstartTime);
            }  /* else startTime before now (OK) */

            /* check the start time after the stop time */
            if (res == NO_ERR && stoptime_utc) {
                ret = xml_strcmp(starttime_utc, stoptime_utc);
                if (ret > 0) {
                    res = ERR_NCX_BAD_ELEMENT;
                    agt_record_error(scb, 
                                     &msg->mhdr, 
                                     NCX_LAYER_OPERATION, 
                                     res, 
                                     methnode, 
                                     NCX_NT_STRING, 
                                     stopTime,
                                     NCX_NT_VAL, 
                                     valstopTime);
                }  /* else startTime before stopTime (OK) */
            }
        }

        /* check stopTime but no startTime */
        if (res == NO_ERR && stoptime_utc) {
            if (!starttime_utc) {
                res = ERR_NCX_MISSING_ELEMENT;
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res, 
                                 methnode, 
                                 NCX_NT_NONE, 
                                 NULL,
                                 NCX_NT_VAL, 
                                 valstartTime);
            }

            /* treat stopTime in the future as an error */
            ret = xml_strcmp(stoptime_utc, tstampbuff);
            if (ret > 0) {
                futurestop = TRUE;
            }
        }
    }

    /* check if there is already a subscription 
     * present for the specified session;
     * this is an error since there is no way to
     * identify multiple subscriptions per session,
     */
    res2 = NO_ERR;
    for (testsub = (agt_not_subscription_t *)
             dlq_firstEntry(&subscriptionQ);
         testsub != NULL && res2 == NO_ERR;
         testsub = (agt_not_subscription_t *)
             dlq_nextEntry(testsub)) {

        if (testsub->sid == scb->sid) {
            res2 = ERR_NCX_IN_USE;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res2, 
                             methnode, 
                             NCX_NT_NONE, 
                             NULL,
                             NCX_NT_NONE, 
                             NULL);
        }
    }

    /* create a new subscription control block if no errors */
    if (res == NO_ERR && res2 == NO_ERR && filterres == NO_ERR) {
        /* passing off malloced memory here
         *    - starttime_utc
         *    - stoptime_utc
         *    - valfilter
         */
        sub = new_subscription(scb,
                               stream,
                               tstampbuff,
                               starttime_utc,
                               stoptime_utc,
                               futurestop,
                               filtertyp,
                               valfilter,
                               valselect);
        if (!sub) {
            res = ERR_INTERNAL_MEM;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res, 
                             methnode, 
                             NCX_NT_NONE, 
                             NULL,
                             NCX_NT_NONE, 
                             NULL);
        } else {
            if (valfilter) {
                /* passing off this memory now !! 
                 * the new_subscription function 
                 * saved a pointer to valfilter
                 * need to remove it from the 
                 * incoming PDU to prevent it 
                 * from be deleted after the <rpc> is done
                 */
                val_remove_child(valfilter);
            }
            msg->rpc_user1 = sub;
        }

        /* these malloced strings have been
         * stored in the new subscription, if used
         */
        starttime_utc = NULL;
        stoptime_utc = NULL;
    }

    if (starttime_utc) {
        m__free(starttime_utc);
    }
    if (stoptime_utc) {
        m__free(stoptime_utc);
    }

    if (res == NO_ERR) {
        return res2;
    } else {
        return res;
    }

} /* create_subscription_validate */


/********************************************************************
* FUNCTION create_subscription_invoke
*
* create-subscription : invoke callback
*
* INPUTS:
*    see rpc/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    create_subscription_invoke (ses_cb_t *scb,
                                rpc_msg_t *msg,
                                xml_node_t *methnode)
{
    agt_not_subscription_t *sub;
    agt_not_msg_t          *not, *nextnot;
    int                     ret;
    boolean                 done;

    (void)scb;
    (void)methnode;
    sub = (agt_not_subscription_t *)msg->rpc_user1;

    if (sub->startTime) {
        /* this subscription has requested replay
         * go through the notificationQ and set
         * the start replay pointer
         */
        sub->state = AGT_NOT_STATE_REPLAY;
        done = FALSE;
        for (not = (agt_not_msg_t *)
                 dlq_firstEntry(&notificationQ);
             not != NULL && !done;
             not = (agt_not_msg_t *)dlq_nextEntry(not)) {

            ret = xml_strcmp(sub->startTime, not->eventTime);
            if (ret <= 0) {
                sub->firstreplaymsg = not;
                sub->firstreplaymsgid = not->msgid;
                done = TRUE;
            }
        }

        if (!done) {
            /* the startTime is after the last available
             * notification eventTime, so replay is over
             */
            sub->flags |= AGT_NOT_FL_RC_READY;
        } else if (sub->stopTime) {
            /* the sub->firstreplaymsg was set;
             * the subscription has requested to be
             * terminated after a specific time
             */
            if (sub->flags & AGT_NOT_FL_FUTURESTOP) {
                /* just use the last replay buffer entry
                 * as the end-of-replay marker
                 */
                sub->lastreplaymsg = (agt_not_msg_t *)
                    dlq_lastEntry(&notificationQ);
                sub->lastreplaymsgid = 
                    sub->lastreplaymsg->msgid;
            } else {
                /* first check that the start notification
                 * is not already past the requested stopTime
                 */
                ret = xml_strcmp(sub->stopTime,
                                 sub->firstreplaymsg->eventTime);
                if (ret <= 0) {
                    sub->firstreplaymsg = NULL;
                    sub->firstreplaymsgid = 0;
                    sub->flags |= AGT_NOT_FL_RC_READY;
                } else {
                    /* check the notifications after the
                     * start replay node, to find out
                     * which one should be the last replay 
                     */
                    done = FALSE;
                    for (not = sub->firstreplaymsg;
                         not != NULL && !done;
                         not = nextnot) {
                 
                        nextnot = (agt_not_msg_t *)dlq_nextEntry(not);
                        if (nextnot) {
                            ret = xml_strcmp(sub->stopTime, 
                                             nextnot->eventTime);
                        } else {
                            ret = -1;
                        }
                        if (ret < 0) {
                            /* the previous one checked was the winner */
                            sub->lastreplaymsg = not;
                            sub->lastreplaymsgid = not->msgid;
                            done = TRUE;
                        }
                    }

                    if (!done) {
                        sub->lastreplaymsg = (agt_not_msg_t *)
                            dlq_lastEntry(&notificationQ);
                        sub->lastreplaymsgid = 
                            sub->lastreplaymsg->msgid;
                    }
                }                       
            }
        }
    } else {
        /* setup live subscription by setting the
         * lastmsg to the end of the replayQ
         * so none of the buffered notifications
         * are send to this subscription
         */
        sub->state = AGT_NOT_STATE_LIVE;
        sub->lastmsg = (agt_not_msg_t *)
            dlq_lastEntry(&notificationQ);
        if (sub->lastmsg) {
            sub->lastmsgid = sub->lastmsg->msgid;
        }
    }

    dlq_enque(sub, &subscriptionQ);
    anySubscriptions = TRUE;

    if (LOGDEBUG) {
        log_debug("\nagt_not: Started %s subscription on stream "
                  "'%s' for session '%u'",
                  (sub->startTime) ? "replay" : "live",
                  sub->stream, 
                  sub->scb->sid);
    }
    return NO_ERR;

} /* create_subscription_invoke */


/********************************************************************
* FUNCTION expire_subscription
*
* Expire a removed subscription because it has a stopTime
* and no more replay notifications are left to deliver
*
* MUST REMOVE FROM subscriptionQ FIRST !!!
*
* INPUTS:
*    sub == subscription to expire
*
*********************************************************************/
static void
    expire_subscription (agt_not_subscription_t *sub)
{
    if (LOGDEBUG) {
        log_debug("\nagt_not: Removed %s subscription "
                  "for session '%u'",
                  (sub->startTime) ? "replay" : "live",
                  sub->scb->sid);
    }

    free_subscription(sub);
    anySubscriptions = (dlq_empty(&subscriptionQ)) ? FALSE : TRUE;


}  /* expire_subscription */


/********************************************************************
* FUNCTION get_entry_after
*
* Get the entry after the specified msgid
*
* INPUTS:
*    thismsgid == get the first msg with an ID higher than this value
*
* RETURNS:
*    pointer to an notification to use
*    NULL if none found
*********************************************************************/
static agt_not_msg_t *
    get_entry_after (uint32 thismsgid)
{
    agt_not_msg_t *not;

    for (not = (agt_not_msg_t *)dlq_firstEntry(&notificationQ);
         not != NULL;
         not = (agt_not_msg_t *)dlq_nextEntry(not)) {

        if (not->msgid > thismsgid) {
            return not;
        }
    }

    return NULL;

} /* get_entry_after */


/********************************************************************
* FUNCTION send_notification
*
* Send the specified notification to the specified subscription
* If checkfilter==TRUE, then check the filter if any, 
* and only send if it passes
*
* INPUTS:
*   sub == subscription to use
*   notif == notification to use
*   checkfilter == TRUE to check the filter if any
*                  FALSE to bypass the filtering 
*                  (e.g., replayComplete, notificationComplete)
*
* OUTPUTS:
*   message sent to sub->scb if filter passes
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    send_notification (agt_not_subscription_t *sub,
                       agt_not_msg_t *notif,
                       boolean checkfilter)
{
    val_value_t        *topval, *eventTime;
    val_value_t        *eventType, *payloadval, *sequenceid;
    ses_total_stats_t  *totalstats;
    xml_msg_hdr_t       msghdr;
    status_t            res;
    boolean             filterpassed;
    xmlChar             numbuff[NCX_MAX_NUMLEN];

    filterpassed = TRUE;

    totalstats = ses_get_total_stats();

    if (!notif->msg) {
        /* need to construct the notification msg */
        topval = val_new_value();
        if (!topval) {
            log_error("\nError: malloc failed: cannot send notification");
            return ERR_INTERNAL_MEM;
        }
        val_init_from_template(topval, notificationobj);

        eventTime = val_make_simval_obj(eventTimeobj, notif->eventTime, &res);
        if (!eventTime) {
            log_error("\nError: make simval failed (%s): cannot "
                      "send notification", 
                      get_error_string(res));
            val_free_value(topval);
            return res;
        }
        val_add_child(eventTime, topval);

        eventType = val_new_value();
        if (!eventType) {
            log_error("\nError: malloc failed: cannot send notification");
            val_free_value(topval);
            return ERR_INTERNAL_MEM;
        }
        val_init_from_template(eventType, notif->notobj);
        val_add_child(eventType, topval);
        notif->event = eventType;

        /* move the payloadQ: transfer the memory here */
        while (!dlq_empty(&notif->payloadQ)) {
            payloadval = (val_value_t *)dlq_deque(&notif->payloadQ);
            val_add_child(payloadval, eventType);
        }

        /* only use a msgid on a real event, not replay
         * also only use if enabled in the agt_profile
         */
        agt_profile_t *profile = agt_get_profile();
        if (checkfilter && profile->agt_notif_sequence_id) { 
            snprintf((char *)numbuff, sizeof(numbuff), "%u", notif->msgid);
            sequenceid = val_make_simval_obj(sequenceidobj, numbuff, &res);
            if (!sequenceid) {
                log_error("\nError: malloc failed: cannot "
                          "add sequence-id");
            } else {
                val_add_child(sequenceid, topval);
            }
        }

        notif->msg = topval;
    }

    /* create an RPC message header struct */
    xml_msg_init_hdr(&msghdr);

    /* check if any filtering is needed */
    if (checkfilter && sub->filterval) {
        switch (sub->filtertyp) {
        case OP_FILTER_SUBTREE:
            filterpassed = 
                agt_tree_test_filter(&msghdr,
                                     sub->scb,
                                     sub->filterval,
                                     notif->event);
            break;
        case OP_FILTER_XPATH:
            filterpassed = 
                agt_xpath_test_filter(&msghdr,
                                      sub->scb,
                                      sub->selectval,
                                      notif->event);
            break;
        case OP_FILTER_NONE:
        default:
            filterpassed = FALSE;
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (filterpassed) {
            if (LOGDEBUG2) {
                log_debug2("\nagt_not: filter passed");
            }
        } else {
            if (LOGDEBUG) {
                log_debug("\nagt_not: filter failed");
            }
        }
    }

    if (filterpassed) {
        /* send the notification */
        res = ses_start_msg(sub->scb);
        if (res != NO_ERR) {
            log_error("\nError: cannot start notification");
            xml_msg_clean_hdr(&msghdr);
            return res;
        }
        xml_wr_full_val(sub->scb, &msghdr, notif->msg, 0);
        ses_finish_msg(sub->scb);

        sub->scb->stats.outNotifications++;
        totalstats->stats.outNotifications++;

        if (LOGDEBUG) {
            log_debug("\nagt_not: Sent <%s> (%u) on '%s' stream "
                      "for session '%u'",
                      obj_get_name(notif->notobj),
                      notif->msgid,
                      sub->stream,
                      sub->scb->sid);
        }
        if (LOGDEBUG2) {
            log_debug2("\nNotification contents:");
            if (notif->msg) {
                val_dump_value(notif->msg, 
                               ses_indent_count(sub->scb));
            }
            log_debug2("\n");
        }
    }

    xml_msg_clean_hdr(&msghdr);
    return NO_ERR;

}  /* send_notification */


/********************************************************************
* FUNCTION delete_oldest_notification
*
* Clean and free a subscription control block
*
* INPUTS:
*    sub == subscription to delete
*
*********************************************************************/
static void
    delete_oldest_notification (void)
{
    agt_not_msg_t            *msg;
    agt_not_subscription_t   *sub;

    /* get the oldest message in the replay buffer */
    msg = (agt_not_msg_t *)dlq_deque(&notificationQ);
    if (msg == NULL) {
        return;
    }

    /* make sure none of the subscriptions are pointing
     * to this message in tracking the replay progress
     */
    for (sub = (agt_not_subscription_t *)
             dlq_firstEntry(&subscriptionQ);
         sub != NULL;
         sub = (agt_not_subscription_t *)dlq_nextEntry(sub)) {

        if (sub->firstreplaymsg == msg) {
            sub->firstreplaymsg = NULL;
        }
        if (sub->lastreplaymsg == msg) {
            sub->lastreplaymsg = NULL;
        }
        if (sub->lastmsg == msg) {
            sub->lastmsg = NULL;
        }
    }

    if (LOGDEBUG2) {
        log_debug2("\nDeleting oldest notification (id: %u)",
                   msg->msgid);
    }

    agt_not_free_notification(msg);

    if (notification_count > 0) {
        notification_count--;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* delete_oldest_notification */


/********************************************************************
* FUNCTION new_notification
* 
* Malloc and initialize the fields in an agt_not_msg_t
*
* INPUTS:
*   eventType == object template of the event type
*   usemsgid == TRUE if this notification will have
*               a sequence-id, FALSE if not
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static agt_not_msg_t * 
    new_notification (obj_template_t *eventType,
                      boolean usemsgid)
{
    agt_not_msg_t  *not;

    not = m__getObj(agt_not_msg_t);
    if (!not) {
        return NULL;
    }
    (void)memset(not, 0x0, sizeof(agt_not_msg_t));
    dlq_createSQue(&not->payloadQ);
    if (usemsgid) {
        not->msgid = ++msgid;
        if (msgid == 0) {
            /* msgid is wrapping!!! */
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    tstamp_datetime(not->eventTime);
    not->notobj = eventType;
    return not;

}  /* new_notification */


/********************************************************************
* FUNCTION send_replayComplete
*
* Send the <replayComplate> notification
*
* INPUTS:
*    sub == subscription to use
*********************************************************************/
static void
    send_replayComplete (agt_not_subscription_t *sub)
{
    agt_not_msg_t  *not;
    status_t        res;

    not = new_notification(replayCompleteobj, FALSE);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <replayComplete>");
        return;
    }

    res = send_notification(sub, not, FALSE);
    if (res != NO_ERR && NEED_EXIT(res)) {
        sub->state = AGT_NOT_STATE_SHUTDOWN;
    }

    agt_not_free_notification(not);

} /* send_replayComplete */


/********************************************************************
* FUNCTION send_notificationComplete
*
* Send the <notificationComplate> notification
*
* INPUTS:
*    sub == subscription to use
*********************************************************************/
static void
    send_notificationComplete (agt_not_subscription_t *sub)
{
    agt_not_msg_t  *not;
    status_t        res;

    not = new_notification(notificationCompleteobj, FALSE);
    if (!not) {
        log_error("\nError: malloc failed; cannot "
                  "send <notificationComplete>");
        return;
    }

    res = send_notification(sub, not, FALSE);
    if (res != NO_ERR && NEED_EXIT(res)) {
        sub->state = AGT_NOT_STATE_SHUTDOWN;
    }

    agt_not_free_notification(not);

} /* send_notificationComplete */


/********************************************************************
* FUNCTION init_static_vars
*
* Init the static vars
*
*********************************************************************/
static void
    init_static_vars (void)
{
    notifmod = NULL;
    ncnotifmod = NULL;
    notificationobj = NULL;
    eventTimeobj = NULL;
    replayCompleteobj = NULL;
    notificationCompleteobj = NULL;
    sequenceidobj = NULL;
    anySubscriptions = FALSE;
    msgid = 0;
    notification_count = 0;

} /* init_static_vars */


/************* E X T E R N A L    F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_not_init
*
* INIT 1:
*   Initialize the agent notification module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_not_init (void)
{
    agt_profile_t  *agt_profile;
    status_t        res;

    if (agt_not_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    log_debug2("\nagt_not: Loading notifications module");

    agt_profile = agt_get_profile();

    dlq_createSQue(&subscriptionQ);
    dlq_createSQue(&notificationQ);
    init_static_vars();
    agt_not_init_done = TRUE;

    /* load the notifications module */
    res = ncxmod_load_module(AGT_NOT_MODULE1, 
                             NULL, 
                             &agt_profile->agt_savedevQ,
                             &notifmod);
    if (res != NO_ERR) {
        return res;
    }

    /* load the nc-notifications module */
    res = ncxmod_load_module(AGT_NOT_MODULE2, 
                             NULL, 
                             &agt_profile->agt_savedevQ,
                             &ncnotifmod);
    if (res != NO_ERR) {
        return res;
    }

    /* find the object definition for the notification element */
    notificationobj = ncx_find_object(notifmod,
                                      NCX_EL_NOTIFICATION);

    if (!notificationobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    eventTimeobj = obj_find_child(notificationobj,
                                  AGT_NOT_MODULE1,
                                  notifications_N_eventTime);
    if (!eventTimeobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    replayCompleteobj = 
        ncx_find_object(ncnotifmod,
                        nc_notifications_N_replayComplete);
    if (!replayCompleteobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    notificationCompleteobj = 
        ncx_find_object(ncnotifmod,
                        nc_notifications_N_notificationComplete);
    if (!notificationCompleteobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    sequenceidobj = 
        obj_find_child(notificationobj,
                       AGT_NOT_SEQID_MOD,
                       NCX_EL_SEQUENCE_ID);
    if (!sequenceidobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    return NO_ERR;

}  /* agt_not_init */


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
status_t
    agt_not_init2 (void)
{
    obj_template_t  *topobj, *streamsobj, *streamobj;
    obj_template_t  *nameobj, *descriptionobj;
    obj_template_t  *replaySupportobj, *replayLogCreationTimeobj;
    val_value_t           *topval, *streamsval, *streamval, *childval;
    cfg_template_t        *runningcfg;
    status_t               res;
    xmlChar                tstampbuff[TSTAMP_MIN_SIZE];

    if (!agt_not_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    /* set up create-subscription RPC operation */
    res = agt_rpc_register_method(AGT_NOT_MODULE1,
                                  notifications_N_create_subscription,
                                  AGT_RPC_PH_VALIDATE,
                                  create_subscription_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(AGT_NOT_MODULE1,
                                  notifications_N_create_subscription,
                                  AGT_RPC_PH_INVOKE,
                                  create_subscription_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* get the running config to add some static data into */
    runningcfg = cfg_get_config(NCX_EL_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* get all the object nodes first */
    topobj = obj_find_template_top(ncnotifmod, 
                                   AGT_NOT_MODULE2,
                                   nc_notifications_N_netconf);
    if (!topobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    streamsobj = obj_find_child(topobj, 
                                AGT_NOT_MODULE2, 
                                nc_notifications_N_streams);
    if (!streamsobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    streamobj = obj_find_child(streamsobj,
                               AGT_NOT_MODULE2,
                               nc_notifications_N_stream);
    if (!streamobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    nameobj = obj_find_child(streamobj,
                             AGT_NOT_MODULE2,
                             nc_notifications_N_name);
    if (!nameobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    descriptionobj = obj_find_child(streamobj,
                                    AGT_NOT_MODULE2,
                                    nc_notifications_N_description);
    if (!descriptionobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    replaySupportobj = obj_find_child(streamobj,
                                      AGT_NOT_MODULE2,
                                      nc_notifications_N_replaySupport);
    if (!replaySupportobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    replayLogCreationTimeobj 
        = obj_find_child(streamobj,
                         AGT_NOT_MODULE2,
                         nc_notifications_N_replayLogCreationTime);
    if (!replayLogCreationTimeobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* add /netconf */
    topval = val_new_value();
    if (!topval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(topval, topobj);

    /* handing off the malloced memory here */
    val_add_child_sorted(topval, runningcfg->root);

    /* add /netconf/streams */
    streamsval = val_new_value();
    if (!streamsval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(streamsval, streamsobj);
    val_add_child(streamsval, topval);

    /* add /netconf/streams/stream
     * creating only one stram for the default NETCONF entry
     */
    streamval = val_new_value();
    if (!streamval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(streamval, streamobj);
    val_add_child(streamval, streamsval);

    /* add /netconf/streams/stream/name */
    childval = val_make_simval_obj(nameobj,
                                   NCX_DEF_STREAM_NAME,
                                   &res);
    if (!childval) {
        return res;
    }
    val_add_child(childval, streamval);

    /* add /netconf/streams/stream/description */
    childval = val_make_simval_obj(descriptionobj,
                                   NCX_DEF_STREAM_DESCR,
                                   &res);
    if (!childval) {
        return res;
    }
    val_add_child(childval, streamval);

    /* add /netconf/streams/stream/replaySupport */
    childval = val_make_simval_obj(replaySupportobj,
                                   NCX_EL_TRUE,
                                   &res);
    if (!childval) {
        return res;
    }
    val_add_child(childval, streamval);

    /* set replay start time to now */
    tstamp_datetime(tstampbuff);

    /* add /netconf/streams/stream/replayLogCreationTime */
    childval = val_make_simval_obj(replayLogCreationTimeobj,
                                   tstampbuff,
                                   &res);
    if (!childval) {
        return res;
    }
    val_add_child(childval, streamval);

    return NO_ERR;

}  /* agt_not_init2 */


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
void 
    agt_not_cleanup (void)
{
    agt_not_subscription_t *sub;
    agt_not_msg_t          *msg;

    if (agt_not_init_done) {
        init_static_vars();

        agt_rpc_unregister_method(AGT_NOT_MODULE1, 
                                  notifications_N_create_subscription);


        /* clear the subscriptionQ */
        while (!dlq_empty(&subscriptionQ)) {
            sub = (agt_not_subscription_t *)dlq_deque(&subscriptionQ);
            free_subscription(sub);
        }

        /* clear the notificationQ */
        while (!dlq_empty(&notificationQ)) {
            msg = (agt_not_msg_t *)dlq_deque(&notificationQ);
            agt_not_free_notification(msg);
        }

        agt_not_init_done = FALSE;
    }

}  /* agt_not_cleanup */


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
uint32
    agt_not_send_notifications (void)
{
    agt_not_subscription_t  *sub, *nextsub;
    agt_not_msg_t           *not;
    xmlChar                  nowbuff[TSTAMP_MIN_SIZE];
    status_t                 res;
    int                      ret;
    uint32                   notcount;

    if (!anySubscriptions) {
        return 0;
    }

    notcount = 0;
    tstamp_datetime(nowbuff);

    for (sub = (agt_not_subscription_t *)
             dlq_firstEntry(&subscriptionQ);
         sub != NULL;
         sub = nextsub) {

        nextsub = (agt_not_subscription_t *)dlq_nextEntry(sub);

        switch (sub->state) {
        case AGT_NOT_STATE_NONE:
        case AGT_NOT_STATE_INIT:
            SET_ERROR(ERR_INTERNAL_VAL);
            break;
        case AGT_NOT_STATE_REPLAY:
            /* check if replayComplete is ready */
            if (sub->flags & AGT_NOT_FL_RC_READY) {
                /* yes, check if it has already been done */
                if (sub->flags & AGT_NOT_FL_RC_DONE) {
                    /* yes, check if <notificationComplete> is
                     * needed or if the state should be
                     * changed to AGT_NOT_STATE_LIVE
                     */
                    if (sub->stopTime) {
                        /* need to terminate the subscription 
                         * at some point
                         */
                        if (sub->flags & AGT_NOT_FL_FUTURESTOP) {
                            /* switch to timed mode */
                            sub->state = AGT_NOT_STATE_TIMED;
                        } else {
                            /* send <notificationComplete> */
                            sub->flags |= AGT_NOT_FL_NC_READY;
                            send_notificationComplete(sub);
                            sub->flags |= AGT_NOT_FL_NC_DONE;
                            notcount++;

                            /* cleanup subscription next time
                             * through this function 
                             */
                            sub->state = AGT_NOT_STATE_SHUTDOWN;
                        }
                    } else {
                        /* no stopTime, so go to live mode */
                        sub->state = AGT_NOT_STATE_LIVE;
                    }
                } else {
                    /* send <replayComplete> */
                    send_replayComplete(sub);
                    sub->flags |= AGT_NOT_FL_RC_DONE;
                    notcount++;
                    /* figure out the rest next time through fn */
                }
            } else {
                /* still sending replay notifications
                 * figure out which one to send next
                 */
                if (sub->lastmsg) {
                    not = (agt_not_msg_t *)
                        dlq_nextEntry(sub->lastmsg);
                } else if (sub->lastmsgid) {
                    /* use ID, back-ptr was cleared */
                    not = get_entry_after(sub->lastmsgid);
                } else {
                    /* this is the first replay being sent */
                    if (sub->firstreplaymsg) {
                        not = sub->firstreplaymsg;
                    } else {
                        /* use ID, back-ptr was cleared */
                        not = get_entry_after(sub->firstreplaymsgid);
                    }
                }
                if (not) {
                    /* found a replay entry to send */
                    if (!agt_acm_notif_allowed(sub->scb->username,
                                               not->notobj)) {
                        log_debug("\nAccess denied to user '%s' "
                                  "for notification '%s'",
                                  sub->scb->username,
                                  obj_get_name(not->notobj));
                        res = NO_ERR;
                    } else {
                        notcount++;
                        res = send_notification(sub, not, TRUE);
                    }
                    if (res != NO_ERR && NEED_EXIT(res)) {
                        /* treat as a fatal error */
                        sub->state = AGT_NOT_STATE_SHUTDOWN;
                    } else {
                        /* msg sent OK; set up next loop through fn */
                        sub->lastmsg = not;
                        sub->lastmsgid = not->msgid;
                        if (sub->lastreplaymsg == not) {
                            /* this was the last replay to send */
                            sub->flags |= AGT_NOT_FL_RC_READY;
                        } else if (sub->lastreplaymsgid &&
                                   sub->lastreplaymsgid <= not->msgid) {
                            /* this was the last replay to send */
                            sub->flags |= AGT_NOT_FL_RC_READY;
                        }
                    }
                } else {
                    /* nothing left in the replay buffer */
                    sub->flags |= AGT_NOT_FL_RC_READY;
                    send_replayComplete(sub);
                    sub->flags |= AGT_NOT_FL_RC_DONE;
                    notcount++;

                    if (sub->stopTime) {
                        /* need to terminate the subscription 
                         * at some point
                         */
                        if (sub->flags & AGT_NOT_FL_FUTURESTOP) {
                            /* switch to timed mode */
                            sub->state = AGT_NOT_STATE_TIMED;
                        } else {
                            sub->flags |= AGT_NOT_FL_NC_READY;
                        }
                    } else {
                        /* no stopTime, so go to live mode */
                        sub->state = AGT_NOT_STATE_LIVE;
                    }
                }
            }
            break;
        case AGT_NOT_STATE_TIMED:
            if (sub->lastmsg) {
                not = (agt_not_msg_t *)dlq_nextEntry(sub->lastmsg);
            } else if (sub->lastmsgid) {
                /* use ID, back-ptr was cleared */
                not = get_entry_after(sub->lastmsgid);
            } else {
                /* this is the first notification sent */
                not = (agt_not_msg_t *)dlq_firstEntry(&notificationQ);
            }

            res = NO_ERR;
            if (not) {
                sub->lastmsg = not;
                sub->lastmsgid = not->msgid;

                ret = xml_strcmp(sub->stopTime, not->eventTime);

                if (!agt_acm_notif_allowed(sub->scb->username,
                                           not->notobj)) {
                    log_debug("\nAccess denied to user '%s' "
                              "for notification '%s'",
                              sub->scb->username,
                              obj_get_name(not->notobj));
                    res = NO_ERR;
                } else {
                    notcount++;
                    res = send_notification(sub, not, TRUE);
                }

                if (res != NO_ERR && NEED_EXIT(res)) {
                    /* treat as a fatal error */
                    sub->state = AGT_NOT_STATE_SHUTDOWN;
                }
            } else {
                /* there is no notification to send */
                ret = xml_strcmp(sub->stopTime, nowbuff);
            }

            if (ret <= 0) {
                /* the future stopTime has passed;
                 * start killing the subscription next loop
                 * through this function
                 */
                sub->flags |= AGT_NOT_FL_NC_READY;
                sub->state = AGT_NOT_STATE_SHUTDOWN;
            } /* else stopTime still in the future */
            break;
        case AGT_NOT_STATE_LIVE:
            if (sub->lastmsg) {
                not = (agt_not_msg_t *)dlq_nextEntry(sub->lastmsg);
            } else if (sub->lastmsgid) {
                /* use ID, back-ptr was cleared */
                not = get_entry_after(sub->lastmsgid);
            } else {
                /* this is the first notification sent */
                not = (agt_not_msg_t *)dlq_firstEntry(&notificationQ);
            }
            if (not) {
                sub->lastmsg = not;
                sub->lastmsgid = not->msgid;

                if (!agt_acm_notif_allowed(sub->scb->username,
                                           not->notobj)) {
                    log_debug("\nAccess denied to user '%s' "
                              "for notification '%s'",
                              sub->scb->username,
                              obj_get_name(not->notobj));
                    res = NO_ERR;
                } else {
                    notcount++;
                    res = send_notification(sub, not, TRUE);
                }
                if (res != NO_ERR && NEED_EXIT(res)) {
                    /* treat as a fatal error */
                    sub->state = AGT_NOT_STATE_SHUTDOWN;
                }
            } /* else don't do anything */
            break;
        case AGT_NOT_STATE_SHUTDOWN:
            /* terminating the subscription after 
             * the <notificationComplete> is sent,
             * only if the stopTime was set
             */
            if (sub->stopTime) {
                if (!(sub->flags & AGT_NOT_FL_NC_DONE)) {
                    send_notificationComplete(sub);
                    sub->flags |= AGT_NOT_FL_NC_DONE;
                    notcount++;
                    break;
                }
            }
            dlq_remove(sub);
            expire_subscription(sub);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    return notcount;

}  /* agt_not_send_notifications */


/********************************************************************
* FUNCTION agt_not_clean_eventlog
*
* Remove any delivered notifications when the replay buffer
* size is set to zero
*
*********************************************************************/
void
    agt_not_clean_eventlog (void)
{
    const agt_profile_t     *agt_profile;
    agt_not_subscription_t  *sub;
    agt_not_msg_t           *msg, *nextmsg;

    uint32                   lowestmsgid;


    agt_profile = agt_get_profile();

    if (agt_profile->agt_eventlog_size > 0) {
        return;
    }

    if (!anySubscriptions) {
        /* zap everything in the Q, since there
         * are no subscriptions right now
         */
        while (!dlq_empty(&notificationQ)) {
            msg = (agt_not_msg_t *)dlq_deque(&notificationQ);
            agt_not_free_notification(msg);
        }
        return;
    }

    /* find the lowest msgid that has been delivered
     * to all the sessions, and any messages in the Q
     * that are lower than that can be deleted
     */
    lowestmsgid = NCX_MAX_UINT;
    for (sub = (agt_not_subscription_t *)
             dlq_firstEntry(&subscriptionQ);
         sub != NULL;
         sub = (agt_not_subscription_t *)dlq_nextEntry(sub)) {

        if (sub->lastmsgid && sub->lastmsgid < lowestmsgid) {
            lowestmsgid = sub->lastmsgid;
        }
    }

    /* keep deleting the oldest entries until the
     * lowest msg ID is passed yb in the buffer
     */
    for (msg = (agt_not_msg_t *)dlq_firstEntry(&notificationQ);
         msg != NULL;
         msg = nextmsg) {

         nextmsg = (agt_not_msg_t *)dlq_nextEntry(msg);

         if (msg->msgid < lowestmsgid) {
             dlq_remove(msg);
             agt_not_free_notification(msg);
         } else {
             return;
         }
    }
    
}  /* agt_not_clean_eventlog */


/********************************************************************
* FUNCTION agt_not_remove_subscription
*
* Remove and expire a subscription with the specified session ID.
* The session is being removed.
*
* INPUTS:
*    sid == session ID to use
*********************************************************************/
void
    agt_not_remove_subscription (ses_id_t sid)
{

    agt_not_subscription_t  *sub;

    if (!anySubscriptions) {
        return;
    }

    for (sub = (agt_not_subscription_t *)
             dlq_firstEntry(&subscriptionQ);
         sub != NULL;
         sub = (agt_not_subscription_t *)dlq_nextEntry(sub)) {

        if (sub->sid == sid) {
            dlq_remove(sub);
            expire_subscription(sub);
            return;
        }
    }
    
}  /* agt_not_remove_subscription */


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
agt_not_msg_t * 
    agt_not_new_notification (obj_template_t *eventType)
{
#ifdef DEBUG
    if (!eventType) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return new_notification(eventType, TRUE);

}  /* agt_not_new_notification */


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
void 
    agt_not_free_notification (agt_not_msg_t *notif)
{
    val_value_t *val;

#ifdef DEBUG
    if (!notif) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(&notif->payloadQ)) {
        val = (val_value_t *)dlq_deque(&notif->payloadQ);
        val_free_value(val);
    }

    if (notif->msg) {
        val_free_value(notif->msg);
    }

    m__free(notif);

}  /* agt_not_free_notification */


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
void
    agt_not_add_to_payload (agt_not_msg_t *notif,
                            val_value_t *val)
{
#ifdef DEBUG
    if (!notif || !val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    dlq_enque(val, &notif->payloadQ);

}  /* agt_not_add_to_payload */


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
void
    agt_not_queue_notification (agt_not_msg_t *notif)
{
    const agt_profile_t    *agt_profile;

#ifdef DEBUG
    if (!notif) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!agt_not_init_done) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return;
    }

    if (LOGDEBUG2) {
        log_debug2("\nQueing <%s> notification to send (id: %u)",
                   (notif->notobj) ? 
                   obj_get_name(notif->notobj) : (const xmlChar *)"??",
                   notif->msgid);
        if (LOGDEBUG3) {
            log_debug3("\nEvent Payload:");
            val_value_t *payload = (val_value_t *)
                dlq_firstEntry(&notif->payloadQ);
            if (payload == NULL) {
                log_debug3(" none");
            } else {
                for (; payload != NULL; 
                     payload = (val_value_t *)dlq_nextEntry(payload)) {
                    val_dump_value(payload, NCX_DEF_INDENT);
                }
            }
        }
    }

    agt_profile = agt_get_profile();

    if (agt_profile->agt_eventlog_size) {
        if (notification_count < agt_profile->agt_eventlog_size) {
            notification_count++;
        } else {
            delete_oldest_notification();
        }
        dlq_enque(notif, &notificationQ);
    } else {
        /* not tracking the event log size 
         * since the entries will get deleted once 
         * they are sent to all active subscriptions
         */
        dlq_enque(notif, &notificationQ);
    }

}  /* agt_not_queue_notification */


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
boolean
    agt_not_is_replay_event (const obj_template_t *notifobj)
{
#ifdef DEBUG
    if (notifobj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!agt_not_init_done) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return FALSE;
    }

    if (notifobj == replayCompleteobj ||
        notifobj == notificationCompleteobj) {
        return TRUE;
    }

    return FALSE;

}  /* agt_not_is_replay_event */

/* END file agt_not.c */
