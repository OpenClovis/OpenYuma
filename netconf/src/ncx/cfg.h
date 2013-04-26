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
#ifndef _H_cfg
#define _H_cfg
/*  FILE: cfg.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NCX configuration database manager

    Configuration segments are stored in sequential order.

   NEW YANG OBJECT MODEL
   =====================

    Configuration database (running, candidate, startup, etc.)
      +
      |
      +-- (root: /)
           +
           |
           +--- object X  (netconf, security, routing, etc.)
           |         |
           |         +---- object A , B, C
           |           
           +--- object Y
           |         |
           |         +---- object D , E
           V


   MODULE USAGE
   ============

   1) call cfg_init()

   2) call cfg_init_static_db for the various hard-wired databases
      that need to be created by the agent

   3) call cfg_apply_load_root() with the startup database contents
      to load into the running config

   4) call cfg_set_target() [NCX_CFGID_CANDIDATE or NCX_CFGID_RUNNING]

   4a) call cfg_fill_candidate_from_running if the target is candidate;
       after agt.c/load_running_config is called

   5) call cfg_set_state() to setup config db access, when ready
      for NETCONF operations

   6) Use cfg_ok_to_* functions to test config access

   7) use cfg_lock and cfg_unlock as desired to set global write locks

   8) use cfg_release_locks when a session terminates

   9) call cfg_cleanup() when program is terminating

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
15-apr-06    abb      Begun.
29-may-08    abb      Converted NCX database model to simplified
                      YANG object model
*/
#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_plock
#include "plock.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

/* bit definitions for the cfg_template->flags field */
#define CFG_FL_TARGET       bit0
#define CFG_FL_DIRTY        bit1

#define CFG_INITIAL_TXID (cfg_transaction_id_t)0

/********************************************************************
*                                                                   *
*                        T Y P E S                                  *
*                                                                   *
*********************************************************************/


/* current configuration state */
typedef enum cfg_state_t_ {
    CFG_ST_NONE,                  /* not set */
    CFG_ST_INIT,                  /* init in progress */
    CFG_ST_READY,                 /* ready and no locks */
    CFG_ST_PLOCK,                 /* partial lock active */
    CFG_ST_FLOCK,                 /* full lock active */
    CFG_ST_CLEANUP                /* cleanup in progress */
} cfg_state_t;


/* classify the config source */
typedef enum cfg_source_t_ {
    CFG_SRC_NONE,
    CFG_SRC_INTERNAL,
    CFG_SRC_NETCONF,
    CFG_SRC_CLI,
    CFG_SRC_SNMP,
    CFG_SRC_HTTP,
    CFG_SRC_OTHER
} cfg_source_t;


/* classify the config location */
typedef enum cfg_location_t_ {
    CFG_LOC_NONE,
    CFG_LOC_INTERNAL,
    CFG_LOC_FILE,
    CFG_LOC_NAMED,
    CFG_LOC_LOCAL_URL,
    CFG_LOC_REMOTE_URL
} cfg_location_t;


/* transaction is scoped to single session write operation on a config */
typedef uint64 cfg_transaction_id_t;


/* struct representing 1 configuration database */
typedef struct cfg_template_t_ {
    ncx_cfg_t      cfg_id;
    cfg_location_t cfg_loc;
    cfg_state_t    cfg_state;
    cfg_transaction_id_t last_txid;
    cfg_transaction_id_t cur_txid;
    xmlChar       *name;
    xmlChar       *src_url;
    xmlChar        lock_time[TSTAMP_MIN_SIZE];
    xmlChar        last_ch_time[TSTAMP_MIN_SIZE];
    uint32         flags;
    ses_id_t       locked_by;
    cfg_source_t   lock_src;
    dlq_hdr_t      load_errQ;    /* Q of rpc_err_rec_t */
    dlq_hdr_t      plockQ;          /* Q of plock_cb_t */
    val_value_t   *root;          /* btyp == NCX_BT_CONTAINER */
} cfg_template_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION cfg_init
*
* Initialize the config manager
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void
    cfg_init (void);


/********************************************************************
* FUNCTION cfg_cleanup
*
* Cleanup the config manager
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void
    cfg_cleanup (void);


/********************************************************************
* FUNCTION cfg_init_static_db
*
* Initialize the specified static configuration slot
*
* INPUTS:
*    id   == cfg ID to intialize
*    
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_init_static_db (ncx_cfg_t cfg_id);


/********************************************************************
* FUNCTION cfg_new_template
*
* Malloc and initialize a cfg_template_t struct
*
* INPUTS:
*    name == cfg name
*    cfg_id   == cfg ID
* RETURNS:
*    malloced struct or NULL if some error
*    This struct needs to be freed by the caller
*********************************************************************/
extern cfg_template_t *
    cfg_new_template (const xmlChar *name,
		      ncx_cfg_t cfg_id);


/********************************************************************
* FUNCTION cfg_free_template
*
* Clean and free the cfg_template_t struct
*
* INPUTS:
*    cfg = cfg_template_t to clean and free
* RETURNS:
*    none
*********************************************************************/
extern void
    cfg_free_template (cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_set_state
*
* Change the state of the specified static config
*
* INPUTS:
*    cfg_id = Config ID to change
*    new_state == new config state to set 
* RETURNS:
*    none
*********************************************************************/
extern void
    cfg_set_state (ncx_cfg_t cfg_id,
		   cfg_state_t new_state);


/********************************************************************
* FUNCTION cfg_get_state
*
* Get the state of the specified static config
*
* INPUTS:
*    cfg_id = Config ID
* RETURNS:
*    config state  (CFG_ST_NONE if some error)
*********************************************************************/
extern cfg_state_t
    cfg_get_state (ncx_cfg_t cfg_id);


/********************************************************************
* FUNCTION cfg_get_config
*
* Get the config struct from its name
*
* INPUTS:
*    cfgname = Config Name
* RETURNS:
*    pointer to config struct or NULL if not found
*********************************************************************/
extern cfg_template_t *
    cfg_get_config (const xmlChar *cfgname);


/********************************************************************
* FUNCTION cfg_get_config_name
*
* Get the config name from its ID
*
* INPUTS:
*    cfgid == config ID
* RETURNS:
*    pointer to config name or NULL if not found
*********************************************************************/
extern const xmlChar *
    cfg_get_config_name (ncx_cfg_t cfgid);


/********************************************************************
* FUNCTION cfg_get_config_id
*
* Get the config struct from its ID
*
* INPUTS:
*    cfgid == config ID
* RETURNS:
*    pointer to config struct or NULL if not found
*********************************************************************/
extern cfg_template_t *
    cfg_get_config_id (ncx_cfg_t cfgid);


/********************************************************************
* FUNCTION cfg_set_target
*
* Set the CFG_FL_TARGET flag in the specified config
*
* INPUTS:
*    cfg_id = Config ID to set as a valid target
*
*********************************************************************/
extern void
    cfg_set_target (ncx_cfg_t cfg_id);


/********************************************************************
* FUNCTION cfg_fill_candidate_from_running
*
* Fill the <candidate> config with the config contents
* of the <running> config
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_fill_candidate_from_running (void);


/********************************************************************
* FUNCTION cfg_fill_candidate_from_startup
*
* Fill the <candidate> config with the config contents
* of the <startup> config
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_fill_candidate_from_startup (void);


/********************************************************************
* FUNCTION cfg_fill_candidate_from_inline
*
* Fill the <candidate> config with the config contents
* of the <config> inline XML node
*
* INPUTS:
*   newroot == new root for the candidate config
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_fill_candidate_from_inline (val_value_t *newroot);


/********************************************************************
* FUNCTION cfg_set_dirty_flag
*
* Mark the config as 'changed'
*
* INPUTS:
*    cfg == configuration template to set
*
*********************************************************************/
extern void
    cfg_set_dirty_flag (cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_get_dirty_flag
*
* Get the config dirty flag value
*
* INPUTS:
*    cfg == configuration template to check
*
*********************************************************************/
extern boolean
    cfg_get_dirty_flag (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_ok_to_lock
*
* Check if the specified config can be locked right now
* for global lock only
*
* INPUTS:
*    cfg = Config template to check 
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_ok_to_lock (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_ok_to_unlock
*
* Check if the specified config can be unlocked right now
* by the specified session ID; for global lock only
*
* INPUTS:
*    cfg = Config template to check 
*    sesid == session ID requesting to unlock the config
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_ok_to_unlock (const cfg_template_t *cfg,
		      ses_id_t sesid);


/********************************************************************
* FUNCTION cfg_ok_to_read
*
* Check if the specified config can be read right now
*
* INPUTS:
*    cfg = Config template to check 
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_ok_to_read (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_ok_to_write
*
* Check if the specified config can be written right now
* by the specified session ID
*
* This is not an access control check,
* only locks and config state will be checked
*
* INPUTS:
*    cfg = Config template to check 
*    sesid == session ID requesting to write to the config
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_ok_to_write (const cfg_template_t *cfg,
		     ses_id_t sesid);


/********************************************************************
* FUNCTION cfg_is_global_locked
*
* Check if the specified config has ab active global lock
*
* INPUTS:
*    cfg = Config template to check 
*
* RETURNS:
*    TRUE if global lock active, FALSE if not
*********************************************************************/
extern boolean
    cfg_is_global_locked (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_is_partial_locked
*
* Check if the specified config has any active partial locks
*
* INPUTS:
*    cfg = Config template to check 
*
* RETURNS:
*    TRUE if partial lock active, FALSE if not
*********************************************************************/
extern boolean
    cfg_is_partial_locked (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_get_global_lock_info
*
* Get the current global lock info
*
* INPUTS:
*    cfg = Config template to check 
*    sid == address of return session ID
*    locktime == address of return locktime pointer
*
* OUTPUTS:
*    *sid == session ID of lock holder
*    *locktime == pointer to lock time string
*
* RETURNS:
*    status, NCX_ERR_SKIPPED if not locked
*********************************************************************/
extern status_t
    cfg_get_global_lock_info (const cfg_template_t *cfg,
			      ses_id_t  *sid,
			      const xmlChar **locktime);


/********************************************************************
* FUNCTION cfg_lock
*
* Lock the specified config.
* This will not really have an effect unless the
* CFG_FL_TARGET flag in the specified config is also set
*
* INPUTS:
*    cfg = Config template to lock
*    locked_by == session ID of the lock owner
*    lock_src == enum classifying the lock source
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_lock (cfg_template_t *cfg,
	      ses_id_t locked_by,
	      cfg_source_t  lock_src);


/********************************************************************
* FUNCTION cfg_unlock
*
* Unlock the specified config.
*
* INPUTS:
*    cfg = Config template to unlock
*    locked_by == session ID of the lock owner
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_unlock (cfg_template_t *cfg,
		ses_id_t locked_by);


/********************************************************************
* FUNCTION cfg_release_locks
*
* Release any configuration locks held by the specified session
*
* INPUTS:
*    sesid == session ID to check for
*
*********************************************************************/
extern void
    cfg_release_locks (ses_id_t sesid);


/********************************************************************
* FUNCTION cfg_release_partial_locks
*
* Release any configuration locks held by the specified session
*
* INPUTS:
*    sesid == session ID to check for
*
*********************************************************************/
extern void
    cfg_release_partial_locks (ses_id_t sesid);


/********************************************************************
* FUNCTION cfg_get_lock_list
*
* Get a list of all the locks held by a session
*
* INPUTS:
*    sesid == session ID to check for any locks
*    retval == pointer to malloced and initialized NCX_BT_SLIST
*
* OUTPUTS:
*   *retval is filled in with any lock entryies or left empty
*   if none found
* 
*********************************************************************/
extern void
    cfg_get_lock_list (ses_id_t sesid,
		       val_value_t *retval);


/********************************************************************
* FUNCTION cfg_apply_load_root
*
* Apply the AGT_CB_APPLY function for the OP_EDITOP_LOAD operation
*
* INPUTS:
*    cfg == config target
*    newroot == new config tree
*
*********************************************************************/
extern void
    cfg_apply_load_root (cfg_template_t *cfg,
			 val_value_t *newroot);


/********************************************************************
* FUNCTION cfg_update_last_ch_time
*
* Update the last-modified timestamp
*
* INPUTS:
*    cfg == config target
*********************************************************************/
extern void
    cfg_update_last_ch_time (cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_update_last_txid
*
* Update the last good transaction ID
*
* INPUTS:
*    cfg == config target
*    txid == trnasaction ID to use
*********************************************************************/
extern void
    cfg_update_last_txid (cfg_template_t *cfg,
                          cfg_transaction_id_t txid);


/********************************************************************
* FUNCTION cfg_add_partial_lock
*
* Add a partial lock the specified config.
* This will not really have an effect unless the
* CFG_FL_TARGET flag in the specified config is also set
* For global lock only
*
* INPUTS:
*    cfg = Config template to lock
*    plcb == partial lock control block, already filled
*            out, to add to this configuration
*
* RETURNS:
*    status; if NO_ERR then the memory in plcb is handed off
*    and will be released when cfg_remove_partial_lock
*    is called for 'plcb'
*********************************************************************/
extern status_t
    cfg_add_partial_lock (cfg_template_t *cfg,
                          plock_cb_t *plcb);


/********************************************************************
* FUNCTION cfg_find_partial_lock
*
* Find a partial lock in the specified config.
*
* INPUTS:
*    cfg = Config template to use
*    lockid == lock-id for the plcb to find
*
* RETURNS:
*   pointer to the partial lock control block found
*   NULL if not found
*********************************************************************/
extern plock_cb_t *
    cfg_find_partial_lock (cfg_template_t *cfg,
                           plock_id_t lockid);


/********************************************************************
* FUNCTION cfg_first_partial_lock
*
* Get the first partial lock in the specified config.
*
* INPUTS:
*    cfg = Config template to use
*
* RETURNS:
*   pointer to the first partial lock control block
*   NULL if none exist at this time
*********************************************************************/
extern plock_cb_t *
    cfg_first_partial_lock (cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_next_partial_lock
*
* Get the next partial lock in the specified config.
*
* INPUTS:
*    curplockcb == current lock control block; get next CB
*
* RETURNS:
*   pointer to the next partial lock control block
*   NULL if none exist at this time
*********************************************************************/
extern plock_cb_t *
    cfg_next_partial_lock (plock_cb_t *curplockcb);


/********************************************************************
* FUNCTION cfg_delete_partial_lock
*
* Remove a partial lock from the specified config.
*
* INPUTS:
*    cfg = Config template to use
*    lockid == lock-id for the plcb  to remove
*
*********************************************************************/
extern void
    cfg_delete_partial_lock (cfg_template_t *cfg,
                             plock_id_t lockid);


/********************************************************************
* FUNCTION cfg_ok_to_partial_lock
*
* Check if the specified config can be locked right now
* for partial lock only
*
* INPUTS:
*    cfg = Config template to check 
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    cfg_ok_to_partial_lock (const cfg_template_t *cfg);


/********************************************************************
* FUNCTION cfg_get_root
*
* Get the config root for the specified config
*
* INPUTS:
*    cfgid == config ID to get root from
* 
* RETURNS:
*    config root or NULL if none or error
*********************************************************************/
extern val_value_t *
    cfg_get_root (ncx_cfg_t cfgid);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_cfg */
