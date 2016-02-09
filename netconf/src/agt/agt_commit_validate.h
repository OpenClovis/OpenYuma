#ifndef _H_agt_commit_validate_cb
#define _H_agt_commit_validate_cb

/*  FILE: agt_cb.h
*********************************************************************
* P U R P O S E
*********************************************************************
    NETCONF Server Commit Validate callback handler
    This file contains functions to support registering, 
    unregistering and execution of commit validate callbacks. 

*/

#include <xmlstring.h>

#include "procdefs.h"
#include "status.h"

#include "procdefs.h"
#include "agt.h"
#include "ses.h"
#include "ncx.h"
#include "ncxconst.h"


#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
* T Y P E D E F S
*********************************************************************/
/** Typedef of the commit_validate callback */
typedef status_t (*agt_commit_validate_cb_t)(ses_cb_t *scb, xml_msg_hdr_t *msghdr, val_value_t *root);

/********************************************************************
* F U N C T I O N S
*********************************************************************/

/**
 * Initialise the callback commit module.
 */
extern void agt_commit_validate_init( void );

/**
 * Cleanup the callback commit module.
 */
extern void agt_commit_validate_cleanup( void );

/**
 * Register a commit validate callback.
 * This function registers a commit-validate callback that will be
 * called after all changes to the candidate database have been
 * validated. The function will be called before that final SIL commit
 * operation. If a commit validate operation is already registered for
 * the module it will be replaced.
 *
 * \param modname the name of the module registering the callback
 * \param cb the commit complete function.
 * \return the status of the operation.
 */
extern status_t agt_commit_validate_register( const xmlChar *modname,
                                              agt_commit_validate_cb_t cb );

/**
 * Unregister a commit validate callback.
 * This function unregisters a commit-validate callback.
 *
 * \param modname the name of the module unregistering the callback
 */
extern void agt_commit_validate_unregister( const xmlChar *modname );

/**
 * Validate a commit operation.
 * This function simply calls each registered commit validate
 * callback. If a commit validate operation fails the status of the
 * failing operation is returned immediately and no further commit
 * validate callbacks are made.
 *
 * \return the ERR_OK or the status of the first failing callback.
 */
extern status_t agt_commit_validate( ses_cb_t *scb, xml_msg_hdr_t *msghdr, val_value_t *root );

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif // _H_agt_commit_validate_cb

