#ifndef _H_agt_not_queue_notification_cb
#define _H_agt_not_queue_notification_cb

/*  FILE: agt_not_queue_notification_cb.h
*********************************************************************
* P U R P O S E
*********************************************************************
    NETCONF Server agt_not_queue_notification callback handler
    This file contains functions to support registering, 
    unregistering and execution of agt_not_queue_notification callbacks. 

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
/** Typedef of the queue_notification callback */
typedef status_t (*agt_not_queue_notification_cb_t)(agt_not_msg_t *notif);

/********************************************************************
* F U N C T I O N S
*********************************************************************/

/**
 * Initialise the callback commit module.
 */
extern void agt_not_queue_notification_cb_init( void );

/**
 * Cleanup the callback commit module.
 */
extern void agt_not_queue_notification_cb_cleanup( void );

/**
 * Register a queue notification callback.
 * This function registers a queue-notification callback that will be
 * called when any module calls agt_not_queue_notification
 *
 * \param modname the name of the module registering the callback
 * \param cb the commit complete function.
 * \return the status of the operation.
 */
extern status_t agt_not_queue_notification_cb_register( const xmlChar *modname,
                                              agt_not_queue_notification_cb_t cb );

/**
 * Unregister a queue notification callback.
 * This function unregisters a queue-notification callback.
 *
 * \param modname the name of the module unregistering the callback
 */
extern void agt_not_queue_notification_cb_unregister( const xmlChar *modname );

/**
 * This function simply calls each registered queue notification
 * callback. If a operation fails the status of the
 * failing operation is returned immediately and no further queue
 * notification callbacks are made.
 *
 * \return the ERR_OK or the status of the first failing callback.
 */
extern status_t agt_not_queue_notification_cb( agt_not_msg_t *notif );

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif // _H_agt_not_queue_notification_cb

