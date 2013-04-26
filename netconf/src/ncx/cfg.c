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
/*  FILE: cfg.c

   CFG locking:
     - if a partial lock exists then a global lock will fail
     - if a global lock exists then the same session ID
       can still add partial locks
     - Only VAL_MAX_PARTIAL_LOCKS con-current partial locks
       can be held for the same value node; MUST be at least 2
     - A partial lock will fail if any part of the subtree
       is already partial-locked by another session
     - CFG_ST_READY to CFG_ST_FLOCK is always OK
     - CFG_ST_FLOCK to/from CFG_ST_PLOCK is not allowed
     - partial-lock(s) or global lock can be active, but not both
     - deleting a session will cause all locks for that session
       to be deleted.  Any global locks on the candidate
       will cause a discard-changes
           
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
15apr06      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "procdefs.h"
#include "cfg.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncx_list.h"
#include "ncx_num.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "plock.h"
#include "plock_cb.h"
#include "rpc.h"
#include "rpc_err.h"
#include "ses.h"
#include "status.h"
#include "tstamp.h"
#include "val.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xpath.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#define CFG_NUM_STATIC 3

#define CFG_DATETIME_LEN 64

#define MAX_CFGID   NCX_CFGID_STARTUP

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
static boolean cfg_init_done = FALSE;

static cfg_template_t  *cfg_arr[CFG_NUM_STATIC];


/********************************************************************
* FUNCTION get_template
*
* Get the config template from its name
*
* INPUTS:
*    name == config name
* RETURNS:
*    pointer config template or NULL if not found
*********************************************************************/
static cfg_template_t *
    get_template (const xmlChar *cfgname)
{
    ncx_cfg_t id;

    for (id = NCX_CFGID_RUNNING; id <= MAX_CFGID; id++) {
        if (!cfg_arr[id]) {
            continue;
        }
        if (!xml_strcmp(cfg_arr[id]->name, cfgname)) {
            return cfg_arr[id];
        }
    }
    return NULL;

} /* get_template */


/********************************************************************
* FUNCTION free_template
*
* Clean and free the cfg_template_t struct
*
* INPUTS:
*    cfg = cfg_template_t to clean and free
* RETURNS:
*    none
*********************************************************************/
static void
    free_template (cfg_template_t *cfg)
{

    rpc_err_rec_t  *err;
    plock_cb_t     *plock;

#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (cfg->root) {
        val_free_value(cfg->root);
    }

    if (cfg->name) {
        m__free(cfg->name);
    }
    if (cfg->src_url) {
        m__free(cfg->src_url);
    }

    while (!dlq_empty(&cfg->load_errQ)) {
        err = (rpc_err_rec_t *)dlq_deque(&cfg->load_errQ);
        rpc_err_free_record(err);
    }

    while (!dlq_empty(&cfg->plockQ)) {
        plock = (plock_cb_t *)dlq_deque(&cfg->plockQ);
        plock_cb_free(plock);
    }

    m__free(cfg);

} /* free_template */


/********************************************************************
* FUNCTION new_template
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
static cfg_template_t *
    new_template (const xmlChar *name,
                  ncx_cfg_t cfg_id)
{
    ncx_module_t          *mod;
    cfg_template_t        *cfg;
    obj_template_t        *cfgobj;

    cfgobj = NULL;
    mod = ncx_find_module(NCXMOD_NETCONF, NULL);
    if (mod) {
        cfgobj = ncx_find_object(mod, NCX_EL_CONFIG);
    }
    if (!cfgobj) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    cfg = m__getObj(cfg_template_t);
    if (!cfg) {
        return NULL;
    }

    memset(cfg, 0x0, sizeof(cfg_template_t));

    dlq_createSQue(&cfg->load_errQ);
    dlq_createSQue(&cfg->plockQ);

    cfg->name = xml_strdup(name);
    if (!cfg->name) {
        free_template(cfg);
        return NULL;
    }

    /* give each config a valid last changed time */
    cfg_update_last_ch_time(cfg);

    cfg->cfg_id = cfg_id;
    cfg->cfg_state = CFG_ST_INIT;

    if (cfg_id != NCX_CFGID_CANDIDATE) {
        cfg->root = val_new_value();
        if (!cfg->root) {
            free_template(cfg);
            return NULL;
        }
        
        /* finish setting up the <config> root value */
        val_init_from_template(cfg->root, cfgobj);
    }  /* else root will be set next with val_clone_config_data */

    return cfg;

} /* new_template */




/***************** E X P O R T E D    F U N C T I O N S  ***********/


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
void
    cfg_init (void)
{

    uint32  i;

    if (!cfg_init_done) {

        for (i=0; i<CFG_NUM_STATIC; i++) {
            cfg_arr[i] = NULL;
        }

        cfg_init_done = TRUE;
    }

} /* cfg_init */


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
void
    cfg_cleanup (void)
{
    ncx_cfg_t   id;

    if (cfg_init_done) {
        for (id=NCX_CFGID_RUNNING; id <= MAX_CFGID; id++) {
            if (cfg_arr[id]) {
                free_template(cfg_arr[id]);
                cfg_arr[id] = NULL;
            }
        }
        cfg_init_done = FALSE;
    }

} /* cfg_cleanup */


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
status_t
    cfg_init_static_db (ncx_cfg_t cfg_id)
{
    cfg_template_t   *cfg;
    const xmlChar    *name;

    if (!cfg_init_done) {
        cfg_init();
    }

#ifdef DEBUG
    if (cfg_id > MAX_CFGID) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    if (cfg_arr[cfg_id]) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* get the hard-wired config name */
    switch (cfg_id) {
    case NCX_CFGID_RUNNING:
        name = NCX_CFG_RUNNING;
        break;
    case NCX_CFGID_CANDIDATE:
        name = NCX_CFG_CANDIDATE;
        break;
    case NCX_CFGID_STARTUP:
        name = NCX_CFG_STARTUP;
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    cfg = new_template(name, cfg_id);
    if (!cfg) {
        return ERR_INTERNAL_MEM;
    }

    cfg_arr[cfg_id] = cfg;
    return NO_ERR;

} /* cfg_init_static_db */


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
cfg_template_t *
    cfg_new_template (const xmlChar *name,
                      ncx_cfg_t cfg_id)
{
    cfg_template_t *cfg;

    cfg = m__getObj(cfg_template_t);
    if (!cfg) {
        return NULL;
    }

    memset(cfg, 0x0, sizeof(cfg_template_t));

    cfg->name = xml_strdup(name);
    if (!cfg->name) {
        m__free(cfg);
        return NULL;
    }

    cfg->cfg_id = cfg_id;
    cfg->cfg_state = CFG_ST_INIT;
    dlq_createSQue(&cfg->load_errQ);
    /* root is still NULL; indicates empty cfg */

    return cfg;

} /* cfg_new_template */


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
void
    cfg_free_template (cfg_template_t *cfg)
{
#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    free_template(cfg);

} /* cfg_free_template */


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
void
    cfg_set_state (ncx_cfg_t cfg_id,
                   cfg_state_t  new_state)
{
#ifdef DEBUG
    if (cfg_id > MAX_CFGID) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
    if (!cfg_arr[cfg_id]) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    cfg_arr[cfg_id]->cfg_state = new_state;

} /* cfg_set_state */


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
cfg_state_t
    cfg_get_state (ncx_cfg_t cfg_id)
{
#ifdef DEBUG
    if (cfg_id > MAX_CFGID) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return CFG_ST_NONE;
    }
#endif

    if (!cfg_arr[cfg_id]) {
        return CFG_ST_NONE;
    }

    return cfg_arr[cfg_id]->cfg_state;

} /* cfg_get_state */


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
cfg_template_t *
    cfg_get_config (const xmlChar *cfgname)
{
#ifdef DEBUG
    if (!cfgname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return get_template(cfgname);

} /* cfg_get_config */



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
const xmlChar *
    cfg_get_config_name (ncx_cfg_t cfgid)
{
    cfg_template_t *cfg = cfg_get_config_id(cfgid);
    if (cfg) {
        return cfg->name;
    }
    return NULL;

}   /* cfg_get_config_name */


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
cfg_template_t *
    cfg_get_config_id (ncx_cfg_t cfgid)
{

    if (cfgid <= MAX_CFGID) {
        return cfg_arr[cfgid];
    }
    return NULL;

} /* cfg_get_config_id */


/********************************************************************
* FUNCTION cfg_set_target
*
* Set the CFG_FL_TARGET flag in the specified config
*
* INPUTS:
*    cfg_id = Config ID to set as a valid target
*
*********************************************************************/
void
    cfg_set_target (ncx_cfg_t cfg_id)
{
#ifdef DEBUG
    if (cfg_id > MAX_CFGID) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
    if (!cfg_arr[cfg_id]) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    cfg_arr[cfg_id]->flags |= CFG_FL_TARGET;

} /* cfg_set_target */


/********************************************************************
* FUNCTION cfg_fill_candidate_from_running
*
* Fill the <candidate> config with the config contents
* of the <running> config
*
* RETURNS:
*    status
*********************************************************************/
status_t
    cfg_fill_candidate_from_running (void)
{
    cfg_template_t  *running, *candidate;
    status_t         res;

#ifdef DEBUG
    if (!cfg_arr[NCX_CFGID_RUNNING] ||
        !cfg_arr[NCX_CFGID_CANDIDATE]) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    running = cfg_arr[NCX_CFGID_RUNNING];
    candidate = cfg_arr[NCX_CFGID_CANDIDATE];

    if (!running->root) {
        return ERR_NCX_DATA_MISSING;
    }

    if (candidate->root) {
        val_free_value(candidate->root);
        candidate->root = NULL;
    }

    res = NO_ERR;
    candidate->root = val_clone_config_data(running->root, &res);
    candidate->flags &= ~CFG_FL_DIRTY;
    candidate->last_txid = running->last_txid;
    candidate->cur_txid = 0;
    return res;

} /* cfg_fill_candidate_from_running */


/********************************************************************
* FUNCTION cfg_fill_candidate_from_startup
*
* Fill the <candidate> config with the config contents
* of the <startup> config
*
* RETURNS:
*    status
*********************************************************************/
status_t
    cfg_fill_candidate_from_startup (void)
{
    cfg_template_t  *startup, *candidate;
    status_t         res;

#ifdef DEBUG
    if (!cfg_arr[NCX_CFGID_CANDIDATE] ||
        !cfg_arr[NCX_CFGID_STARTUP]) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    startup = cfg_arr[NCX_CFGID_STARTUP];
    candidate = cfg_arr[NCX_CFGID_CANDIDATE];

    if (!startup->root) {
        return ERR_NCX_DATA_MISSING;
    }

    if (candidate->root) {
        val_free_value(candidate->root);
        candidate->root = NULL;
    }

    res = NO_ERR;
    candidate->root = val_clone2(startup->root);
    if (candidate->root == NULL) {
        res = ERR_INTERNAL_MEM;
    }
    candidate->flags &= ~CFG_FL_DIRTY;
    candidate->last_txid = startup->last_txid;
    candidate->cur_txid = 0;

    return res;

} /* cfg_fill_candidate_from_startup */


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
status_t
    cfg_fill_candidate_from_inline (val_value_t *newroot)
{
    cfg_template_t  *candidate;
    status_t         res;

#ifdef DEBUG
    if (newroot == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!cfg_arr[NCX_CFGID_CANDIDATE]) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    candidate = cfg_arr[NCX_CFGID_CANDIDATE];

    if (candidate->root) {
        val_free_value(candidate->root);
        candidate->root = NULL;
    }

    res = NO_ERR;
    candidate->root = val_clone_config_data(newroot, &res);
    candidate->flags &= ~CFG_FL_DIRTY;

    return res;

} /* cfg_fill_candidate_from_inline */


/********************************************************************
* FUNCTION cfg_set_dirty_flag
*
* Mark the config as 'changed'
*
* INPUTS:
*    cfg == configuration template to set
*
*********************************************************************/
void
    cfg_set_dirty_flag (cfg_template_t *cfg)
{
#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    cfg->flags |= CFG_FL_DIRTY;

}  /* cfg_set_dirty_flag */


/********************************************************************
* FUNCTION cfg_get_dirty_flag
*
* Get the config dirty flag value
*
* INPUTS:
*    cfg == configuration template to check
*
*********************************************************************/
boolean
    cfg_get_dirty_flag (const cfg_template_t *cfg)
{
#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (cfg->flags & CFG_FL_DIRTY) ? TRUE : FALSE;

}  /* cfg_get_dirty_flag */


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
status_t
    cfg_ok_to_lock (const cfg_template_t *cfg)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (cfg->cfg_state) {
    case CFG_ST_READY:
        if (cfg->cfg_id == NCX_CFGID_CANDIDATE) {
            /* lock cannot be granted if any changes
             * to the candidate are already made
             */
            res = (cfg_get_dirty_flag(cfg)) ? 
                ERR_NCX_CANDIDATE_DIRTY : NO_ERR;
        } else {
            /* lock can be granted if state is ready */
            res = NO_ERR;
        }
        break;
    case CFG_ST_PLOCK:
    case CFG_ST_FLOCK:
        /* full or partial lock already held by a session */
        res = ERR_NCX_LOCK_DENIED;
        break;
    case CFG_ST_NONE:
    case CFG_ST_INIT:
    case CFG_ST_CLEANUP:
        /* config is in a state where locks cannot be granted */
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);

    }

    return res;

} /* cfg_ok_to_lock */


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
status_t
    cfg_ok_to_unlock (const cfg_template_t *cfg,
                      ses_id_t sesid)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (cfg->cfg_state) {
    case CFG_ST_PLOCK:
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    case CFG_ST_FLOCK:
        /* check if global lock is granted to the correct user */
        if (cfg->locked_by == sesid) {
            res = NO_ERR;
        } else {
            res = ERR_NCX_NO_ACCESS_LOCK;
        }
        break;
    case CFG_ST_NONE:
    case CFG_ST_INIT:
    case CFG_ST_READY:
    case CFG_ST_CLEANUP:
        /* config is not in a locked state */
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* cfg_ok_to_unlock */


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
status_t
    cfg_ok_to_read (const cfg_template_t *cfg)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    switch (cfg->cfg_state) {
    case CFG_ST_PLOCK:
    case CFG_ST_FLOCK:
    case CFG_ST_INIT:
    case CFG_ST_READY:
        res = NO_ERR;
        break;
    case CFG_ST_NONE:
    case CFG_ST_CLEANUP:
        /* config is not in a writable state */
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        break;
    }

    return res;

} /* cfg_ok_to_read */


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
status_t
    cfg_ok_to_write (const cfg_template_t *cfg,
                     ses_id_t sesid)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    /* check if this is a writable target,
     * except during agent boot
     * except for the <startup> config
     */
    if (cfg->cfg_state != CFG_ST_INIT) {
        switch (cfg->cfg_id) {
        case NCX_CFGID_RUNNING:
        case NCX_CFGID_CANDIDATE:
        case NCX_CFGID_STARTUP:
            break;
        default:
            if (!(cfg->flags & CFG_FL_TARGET)) {
                res = ERR_NCX_NOT_WRITABLE;
            }
        }       
    }

    if (res != NO_ERR) {
        return res;
    }

    /* check the current config state */
    switch (cfg->cfg_state) {
    case CFG_ST_PLOCK:
        /* partial lock is always OK for root node access */
        res = NO_ERR;
        break;
    case CFG_ST_FLOCK:
        if (cfg->locked_by == sesid) {
            res = NO_ERR;
        } else {
            res = ERR_NCX_NO_ACCESS_LOCK;
        }
        break;
    case CFG_ST_INIT:
    case CFG_ST_READY:
        res = NO_ERR;
        break;
    case CFG_ST_NONE:
    case CFG_ST_CLEANUP:
        /* config is not in a writable state */
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        res = ERR_NCX_OPERATION_FAILED;
        break;
    }

    return res;

} /* cfg_ok_to_write */


/********************************************************************
* FUNCTION cfg_is_global_locked
*
* Check if the specified config has an active global lock
*
* INPUTS:
*    cfg = Config template to check 
*
* RETURNS:
*    TRUE if global lock active, FALSE if not
*********************************************************************/
boolean
    cfg_is_global_locked (const cfg_template_t *cfg)
{

#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (cfg->cfg_state == CFG_ST_FLOCK) ? TRUE : FALSE;

} /* cfg_is_global_locked */


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
boolean
    cfg_is_partial_locked (const cfg_template_t *cfg)
{

#ifdef DEBUG
    if (!cfg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (cfg->cfg_state == CFG_ST_PLOCK) ? TRUE : FALSE;

} /* cfg_is_partial_locked */


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
status_t
    cfg_get_global_lock_info (const cfg_template_t *cfg,
                              ses_id_t  *sid,
                              const xmlChar **locktime)
{

#ifdef DEBUG
    if (!cfg || !sid || !locktime) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *sid = 0;
    *locktime = NULL;

    if (cfg->cfg_state == CFG_ST_FLOCK) {
        *sid = cfg->locked_by;
        *locktime = cfg->lock_time;
        return NO_ERR;
    } else {
        return ERR_NCX_SKIPPED;
    }
    /*NOTREACHED*/

} /* cfg_get_global_lock_info */


/********************************************************************
* FUNCTION cfg_lock
*
* Lock the specified config.
* This will not really have an effect unless the
* CFG_FL_TARGET flag in the specified config is also set
* For global lock only
*
* INPUTS:
*    cfg = Config template to lock
*    locked_by == session ID of the lock owner
*    lock_src == enum classifying the lock source
* RETURNS:
*    status
*********************************************************************/
status_t
    cfg_lock (cfg_template_t *cfg,
              ses_id_t locked_by,
              cfg_source_t  lock_src)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = cfg_ok_to_lock(cfg);

    if (res == NO_ERR) {
        cfg->cfg_state = CFG_ST_FLOCK;
        cfg->locked_by = locked_by;
        cfg->lock_src = lock_src;
        tstamp_datetime(cfg->lock_time);
    }

    return res;

} /* cfg_lock */


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
status_t
    cfg_unlock (cfg_template_t *cfg,
                ses_id_t locked_by)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = cfg_ok_to_unlock(cfg, locked_by);

    if (res == NO_ERR) {
        cfg->cfg_state = CFG_ST_READY;
        cfg->locked_by = 0;
        cfg->lock_src = CFG_SRC_NONE;

        /* sec 8.3.5.2 requires a discard-changes
         * when a lock is released on the candidate
         */
        if (cfg->cfg_id == NCX_CFGID_CANDIDATE) {
            res = cfg_fill_candidate_from_running();
        }
    }

    return res;

} /* cfg_unlock */


/********************************************************************
* FUNCTION cfg_release_locks
*
* Release any configuration locks held by the specified session
*
* INPUTS:
*    sesid == session ID to check for
*
*********************************************************************/
void
    cfg_release_locks (ses_id_t sesid)
{
    uint32  i;
    cfg_template_t *cfg;
    status_t        res;

    if (!cfg_init_done) {
        return;
    }

    if (sesid == 0) {
        return;
    }

    /* check for partial locks */
    cfg_release_partial_locks(sesid);

    /* check for global locks */
    for (i=0; i<CFG_NUM_STATIC; i++) {
        cfg = cfg_arr[i];
        if (cfg != NULL && cfg->locked_by == sesid) {
            cfg->cfg_state = CFG_ST_READY;
            cfg->locked_by = 0;
            cfg->lock_src = CFG_SRC_NONE;
            log_info("\ncfg forced unlock on %s config, "
                     "held by session %d",
                     cfg->name, 
                     sesid);

            /* sec 8.3.5.2 requires a discard-changes
             * when a lock is released on the candidate
             */
            if (cfg->cfg_id == NCX_CFGID_CANDIDATE) {
                res = cfg_fill_candidate_from_running();
                if (res != NO_ERR) {
                    log_error("\nError: discard-changes failed (%s)",
                              get_error_string(res));
                }
            }
        }
    }


} /* cfg_release_locks */


/********************************************************************
* FUNCTION cfg_release_partial_locks
*
* Release any configuration locks held by the specified session
*
* INPUTS:
*    sesid == session ID to check for
*
*********************************************************************/
void
    cfg_release_partial_locks (ses_id_t sesid)
{
    cfg_template_t *cfg;
    plock_cb_t     *plcb, *nextplcb;
    ses_id_t        plock_sid;

    if (!cfg_init_done) {
        return;
    }

    cfg = cfg_arr[NCX_CFGID_RUNNING];
    if (cfg == NULL) {
        return;
    }

    for (plcb = (plock_cb_t *)dlq_firstEntry(&cfg->plockQ);
         plcb != NULL;
         plcb = nextplcb) {

        nextplcb = (plock_cb_t *)dlq_nextEntry(plcb);
        plock_sid = plock_get_sid(plcb);

        if (plock_sid == sesid) {
            log_info("\ncfg forced partial unlock (id:%u) "
                     "on running config, "
                     "held by session %d",
                     plock_get_id(plcb),
                     plock_sid);
            dlq_remove(plcb);
            if (cfg->root != NULL) {
                val_clear_partial_lock(cfg->root, plcb);
            }
            plock_cb_free(plcb);
        }
    }

} /* cfg_release_partial_locks */


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
void
    cfg_get_lock_list (ses_id_t sesid,
                       val_value_t *retval)
{

    ncx_lmem_t *lmem;
    uint32      i;

#ifdef DEBUG
    if (!retval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* dummy session ID does not bother with locks */
    if (!sesid) {
        return;
    }

    for (i=0; i<CFG_NUM_STATIC; i++) {
        if (cfg_arr[i] && cfg_arr[i]->locked_by == sesid) {
            lmem = ncx_new_lmem();
            if (lmem) {
                lmem->val.str = xml_strdup(cfg_arr[i]->name);
                if (lmem->val.str) {
                    ncx_insert_lmem(&retval->v.list, lmem, NCX_MERGE_LAST);
                } else {
                    ncx_free_lmem(lmem, NCX_BT_STRING);
                }
            }
        }
    }

} /* cfg_get_lock_list */


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
void
    cfg_apply_load_root (cfg_template_t *cfg,
                         val_value_t *newroot)
{
    assert ( cfg && "cfg is NULL!" );

    if (cfg->root && val_child_cnt(cfg->root)) {
        log_warn("\nWarning: config root already has child nodes");
    }

    cfg_update_last_ch_time(cfg);

    if (cfg->root) {
        val_free_value(cfg->root);
    }
    cfg->root = newroot;

} /* cfg_apply_load_root */


/********************************************************************
* FUNCTION cfg_update_last_ch_time
*
* Update the last-modified timestamp
*
* INPUTS:
*    cfg == config target
*********************************************************************/
void
    cfg_update_last_ch_time (cfg_template_t *cfg)
{
    tstamp_datetime(cfg->last_ch_time);

} /* cfg_update_last_ch_time */


/********************************************************************
* FUNCTION cfg_update_last_txid
*
* Update the last good transaction ID
*
* INPUTS:
*    cfg == config target
*    txid == trnasaction ID to use
*********************************************************************/
void
    cfg_update_last_txid (cfg_template_t *cfg,
                          cfg_transaction_id_t txid)
{
    cfg->last_txid = txid;

} /* cfg_update_last_txid */


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
status_t
    cfg_add_partial_lock (cfg_template_t *cfg,
                          plock_cb_t *plcb)
{
    status_t  res;

#ifdef DEBUG
    if (cfg == NULL || plcb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = cfg_ok_to_partial_lock(cfg);

    if (res == NO_ERR) {
        cfg->cfg_state = CFG_ST_PLOCK;
        dlq_enque(plcb, &cfg->plockQ);
    }

    return res;

}  /* cfg_add_partial_lock */


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
plock_cb_t *
    cfg_find_partial_lock (cfg_template_t *cfg,
                           plock_id_t lockid)
{
    plock_cb_t   *plock;

#ifdef DEBUG
    if (cfg == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (plock = (plock_cb_t *)dlq_firstEntry(&cfg->plockQ);
         plock != NULL;
         plock = (plock_cb_t *)dlq_nextEntry(plock)) {

        if (plock_get_id(plock) == lockid) {
            return plock;
        }
    }
    return NULL;

}  /* cfg_find_partial_lock */


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
plock_cb_t *
    cfg_first_partial_lock (cfg_template_t *cfg)
{
#ifdef DEBUG
    if (cfg == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (plock_cb_t *)dlq_firstEntry(&cfg->plockQ);

}  /* cfg_first_partial_lock */


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
plock_cb_t *
    cfg_next_partial_lock (plock_cb_t *curplockcb)
{
#ifdef DEBUG
    if (curplockcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (plock_cb_t *)dlq_nextEntry(curplockcb);

}  /* cfg_next_partial_lock */



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
void
    cfg_delete_partial_lock (cfg_template_t *cfg,
                             plock_id_t lockid)
{
    plock_cb_t   *plock, *nextplock;

#ifdef DEBUG
    if (cfg == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (cfg->cfg_state != CFG_ST_PLOCK) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif
    
    for (plock = (plock_cb_t *)dlq_firstEntry(&cfg->plockQ);
         plock != NULL;
         plock = nextplock) {

        nextplock = (plock_cb_t *)dlq_nextEntry(plock);

        if (plock_get_id(plock) == lockid) {
            dlq_remove(plock);
            if (cfg->root != NULL) {
                val_clear_partial_lock(cfg->root, plock);
            }
            plock_cb_free(plock);
            if (dlq_empty(&cfg->plockQ)) {
                cfg->cfg_state = CFG_ST_READY;
            } else {
                cfg->cfg_state = CFG_ST_PLOCK;
            }
            return;
        }
    }

}  /* cfg_delete_partial_lock */


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
status_t
    cfg_ok_to_partial_lock (const cfg_template_t *cfg)
{
    status_t  res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (cfg->cfg_id != NCX_CFGID_RUNNING) {
        return ERR_NCX_LOCK_DENIED;
    }

    switch (cfg->cfg_state) {
    case CFG_ST_READY:
    case CFG_ST_PLOCK:
        res = NO_ERR;
        break;
    case CFG_ST_FLOCK:
    case CFG_ST_NONE:
    case CFG_ST_INIT:
    case CFG_ST_CLEANUP:
        /* config is in a state where locks cannot be granted */
        res = ERR_NCX_NO_ACCESS_STATE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* cfg_ok_to_partial_lock */


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
val_value_t *
    cfg_get_root (ncx_cfg_t cfgid)
{
    cfg_template_t  *cfg = NULL;

    if (cfgid <= MAX_CFGID) {
        cfg = cfg_arr[cfgid];
    }
    if (cfg != NULL) {
        return cfg->root;
    }
    return NULL;

} /* cfg_get_root */


/* END file cfg.c */
