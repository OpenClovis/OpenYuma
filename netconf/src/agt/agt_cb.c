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
/*  FILE: agt_cb.c

   Manage Agent callbacks for data model manipulation

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
16apr07      abb      begun; split out from agt_ps.c
01aug08      abb      rewrite for YANG OBJ only data tree

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "agt_cb.h"
#include "agt_util.h"
#include "obj.h"
#include "xpath.h"
#include "yang.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
********************************************************************/


/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/

typedef enum agt_cb_status_t_ {
    AGTCB_STAT_NONE,
    AGTCB_STAT_INIT_DONE,
    AGTCB_STAT_LOADED,
    AGTCB_STAT_LOAD_FAILED
} agt_cb_status_t;


typedef struct agt_cb_modhdr_t_ {
    dlq_hdr_t         qhdr;
    const xmlChar    *modname;
    const xmlChar    *modversion;
    dlq_hdr_t         callbackQ;    /* Q of agt_cb_set_t */
    agt_cb_status_t   loadstatus;
    status_t          status;
} agt_cb_modhdr_t;

typedef struct agt_cb_set_t_ {
    dlq_hdr_t          qhdr;
    agt_cb_modhdr_t   *parent;
    const xmlChar     *defpath;
    const xmlChar     *version;
    agt_cb_fnset_t     cbset;
    agt_cb_status_t    loadstatus;
    status_t           status;
} agt_cb_set_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
boolean    agt_cb_init_done = FALSE;
dlq_hdr_t  modhdrQ;


/********************************************************************
* FUNCTION free_callback
* 
* Clean and free a callback struct
*
* INPUTS:
*   callback == agt_cb_set_t struct to clean and free
*
*********************************************************************/
static void
    free_callback (agt_cb_set_t *callback)
{
    m__free(callback);

}   /* free_callback */


/********************************************************************
* FUNCTION new_callback
* 
* Malloc and init a new callback header
*
* INPUTS:
*   defpath == Xpath OID for definition object target
*   version == expected revision data for this object
*   cbset == set of callback functions to copy
*
* RETURNS:
*   malloced and initialized callback header struct
*********************************************************************/
static agt_cb_set_t *
    new_callback (agt_cb_modhdr_t  *parent,
                  const xmlChar *defpath,
                  const xmlChar *version,
                  const agt_cb_fnset_t *cbset)
{

    agt_cb_set_t *callback;
    agt_cbtyp_t   cbtyp;

    callback = m__getObj(agt_cb_set_t);
    if (!callback) {
        return NULL;
    }

    memset(callback, 0x0, sizeof(agt_cb_set_t));

    callback->defpath = defpath;
    callback->version = version;
    callback->parent = parent;
    callback->loadstatus = AGTCB_STAT_INIT_DONE;
    callback->status = NO_ERR;

    for (cbtyp = AGT_CB_VALIDATE;
         cbtyp <= AGT_CB_ROLLBACK;
         cbtyp++) {
        callback->cbset.cbfn[cbtyp] = cbset->cbfn[cbtyp];
    }

    return callback;

}   /* new_callback */


/********************************************************************
* FUNCTION free_modhdr
* 
* Clean and free a modhdr struct
*
* INPUTS:
*   modhdr == agt_cb_modhdr_t struct to clean and free
*
*********************************************************************/
static void
    free_modhdr (agt_cb_modhdr_t *modhdr)
{
    agt_cb_set_t  *callback;

    while (!dlq_empty(&modhdr->callbackQ)) {
        callback = (agt_cb_set_t *)
            dlq_deque(&modhdr->callbackQ);
        free_callback(callback);
    }

    m__free(modhdr);

}   /* free_modhdr */


/********************************************************************
* FUNCTION find_callback
* 
* Find a callback record in a modhdr record
*
* INPUTS:
*   modhdr == agt_cb_modhdr_t struct to check
*   defpath == definition path string to find
*
* RETURNS:
*   pointer to found callback record or NULL if not found
*********************************************************************/
static agt_cb_set_t *
    find_callback (agt_cb_modhdr_t *modhdr,
                   const xmlChar *defpath)
{
    agt_cb_set_t *callback;
    int           ret;

    for (callback = (agt_cb_set_t *)
             dlq_firstEntry(&modhdr->callbackQ);
         callback != NULL;
         callback = (agt_cb_set_t *)dlq_nextEntry(callback)) {

         ret = xml_strcmp(defpath, callback->defpath);
         if (ret == 0) {
             return callback;
         } else if (ret < 0) {
             return NULL;
         }
    }
    return NULL;

}   /* find_callback */


/********************************************************************
* FUNCTION add_callback
* 
* Add a callback record in a modhdr record
*
* INPUTS:
*   modhdr == agt_cb_modhdr_t struct to add to
*   callback == callback definition struct to add
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_callback (agt_cb_modhdr_t *modhdr,
                  agt_cb_set_t *callback)
{
    agt_cb_set_t *cb;
    int           ret;

    for (cb = (agt_cb_set_t *)
             dlq_firstEntry(&modhdr->callbackQ);
         cb != NULL;
         cb = (agt_cb_set_t *)dlq_nextEntry(cb)) {

         ret = xml_strcmp(callback->defpath, cb->defpath);
         if (ret == 0) {
             return ERR_NCX_ENTRY_EXISTS;
         } else if (ret < 0) {
             dlq_insertAhead(callback, cb);
             return NO_ERR;
         }
    }
    dlq_enque(callback, &modhdr->callbackQ);
    return NO_ERR;

}   /* add_callback */


/********************************************************************
* FUNCTION new_modhdr
* 
* Malloc and init a new module callback header
*
* INPUTS:
*   modname == module name
*
* RETURNS:
*   malloced and initialized module header struct
*********************************************************************/
static agt_cb_modhdr_t *
    new_modhdr (const xmlChar *modname)
{

    agt_cb_modhdr_t *modhdr;

    modhdr = m__getObj(agt_cb_modhdr_t);
    if (!modhdr) {
        return NULL;
    }

    memset(modhdr, 0x0, sizeof(agt_cb_modhdr_t));
    dlq_createSQue(&modhdr->callbackQ);

    modhdr->modname = modname;
    modhdr->loadstatus = AGTCB_STAT_INIT_DONE;
    modhdr->status = NO_ERR;

    return modhdr;

}   /* new_modhdr */


/********************************************************************
* FUNCTION find_modhdr
* 
* Find a module header record in the global Q
*
* INPUTS:
*   modname == module name to find
*
* RETURNS:
*   pointer to found modhdr record or NULL if not found
*********************************************************************/
static agt_cb_modhdr_t *
    find_modhdr (const xmlChar *modname)
{
    agt_cb_modhdr_t *modhdr;
    int           ret;

    for (modhdr = (agt_cb_modhdr_t *)dlq_firstEntry(&modhdrQ);
         modhdr != NULL;
         modhdr = (agt_cb_modhdr_t *)dlq_nextEntry(modhdr)) {

         ret = xml_strcmp(modname, modhdr->modname);
         if (ret == 0) {
             return modhdr;
         } else if (ret < 0) {
             return NULL;
         }
    }
    return NULL;

}   /* find_modhdr */


/********************************************************************
* FUNCTION add_modhdr
* 
* Add a module header record to the global Q
*
* INPUTS:
*   modhdr == agt_cb_modhdr_t struct to add
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_modhdr (agt_cb_modhdr_t *modhdr)
{
    agt_cb_modhdr_t *mh;
    int              ret;

    for (mh = (agt_cb_modhdr_t *)dlq_firstEntry(&modhdrQ);
         mh != NULL;
         mh = (agt_cb_modhdr_t *)dlq_nextEntry(mh)) {

         ret = xml_strcmp(modhdr->modname, mh->modname);
         if (ret == 0) {
             return ERR_NCX_ENTRY_EXISTS;
         } else if (ret < 0) {
             dlq_insertAhead(modhdr, mh);
             return NO_ERR;
         }
    }
    dlq_enque(modhdr, &modhdrQ);
    return NO_ERR;

}   /* add_modhdr */


/********************************************************************
* FUNCTION load_callbacks
* 
* Register an object specific callback function
*
* INPUTS:
*   mod == module that defines the target object for
*              these callback functions 
*   modhdr == agt_cb module header for this module
*   callback == agt_cb callback structure to load
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    load_callbacks (ncx_module_t *mod,
                    agt_cb_modhdr_t *modhdr,
                    agt_cb_set_t *callback)
{
    obj_template_t     *obj;
    status_t            res;
    int                 ret;

    /* else module found */
    modhdr->loadstatus = AGTCB_STAT_LOADED;
    modhdr->modversion = mod->version;

    /* check if the version loaded is acceptable for this callback */
    if (callback->version) {
        ret = yang_compare_revision_dates(modhdr->modversion, 
                                          callback->version);
        if (ret != 0) {
            res = ERR_NCX_WRONG_VERSION;
            callback->loadstatus = AGTCB_STAT_LOAD_FAILED;
            callback->status = res;

            log_error("\nError: load callbacks failed for module '%s'"
                      "\n  wrong version: got '%s', need '%s'", 
                      mod->name, 
                      mod->version, 
                      callback->version);

            return res;
        }
    }

    /* find the object template for this callback */
    res = xpath_find_schema_target_int(callback->defpath, &obj);
    if (res == NO_ERR) {
        /* set the callbacks in the object */
        obj->cbset = &callback->cbset;
        callback->loadstatus = AGTCB_STAT_LOADED;
        callback->status = NO_ERR;

        log_debug2("\nagt_cb: load OK for mod '%s', def '%s'",
                   mod->name, callback->defpath);
    } else {
        callback->loadstatus = AGTCB_STAT_LOAD_FAILED;
        callback->status = res;

        log_error("\nError: load callbacks failed for module '%s'"
                  "\n  '%s' for defpath '%s'",
                  mod->name, 
                  get_error_string(res), 
                  callback->defpath);

    }
        
    return res;

}  /* load_callbacks */


/********************************************************************
* FUNCTION check_module_pending
* 
* Check if module callbacks have been registered for
* a module that was just added.  Set any callbacks as
* needed in the object tree
*
* INPUTS:
*    mod == newly added module
*
*********************************************************************/
static void
    check_module_pending (ncx_module_t *mod)
{
    agt_cb_modhdr_t *modhdr;
    agt_cb_set_t    *callback;

    log_debug2("\nagt_cb: got new module '%s', rev '%s'",
               mod->name, mod->version);

    modhdr = find_modhdr(mod->name);
    if (!modhdr) {
        return;
    }

    for (callback = (agt_cb_set_t *)
             dlq_firstEntry(&modhdr->callbackQ);
         callback != NULL;
         callback = (agt_cb_set_t *)
             dlq_nextEntry(callback)) {
        (void)load_callbacks(mod, modhdr, callback);
    }

}   /* check_module_pending */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_cb_init
* 
* Init the agent callback module
*
*********************************************************************/
void
    agt_cb_init (void)
{
    if (!agt_cb_init_done) {
        dlq_createSQue(&modhdrQ);
        agt_cb_init_done = TRUE;
    }

    ncx_set_load_callback(check_module_pending);

}   /* agt_cb_init */


/********************************************************************
* FUNCTION agt_cb_cleanup
* 
* CLeanup the agent callback module
*
*********************************************************************/
void
    agt_cb_cleanup (void)
{
    agt_cb_modhdr_t *modhdr;

    if (agt_cb_init_done) {
        while (!dlq_empty(&modhdrQ)) {
            modhdr = (agt_cb_modhdr_t *)dlq_deque(&modhdrQ);
            free_modhdr(modhdr);
        }
        agt_cb_init_done = FALSE;
    }
}   /* agt_cb_cleanup */


/********************************************************************
* FUNCTION agt_cb_register_callback
* 
* Register an object specific callback function
* use the same fn for all callback phases 
* all phases will be invoked
*
* INPUTS:
*   modname == module that defines the target object for
*              these callback functions 
*   defpath == Xpath with default (or no) prefixes
*              defining the object that will get the callbacks
*   version == exact module revision date expected
*              if condition not met then an error will
*              be logged  (TBD: force unload of module!)
*           == NULL means use any version of the module
*   cbfn    == address of callback function to use for
*              all callback phases
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    agt_cb_register_callback (const xmlChar *modname,
                              const xmlChar *defpath,
                              const xmlChar *version,
                              const agt_cb_fn_t cbfn)
{
    agt_cb_modhdr_t    *modhdr;
    agt_cb_set_t       *callback;
    ncx_module_t       *mod;
    status_t            res;
    agt_cbtyp_t         cbtyp;
    agt_cb_fnset_t      cbset;

#ifdef DEBUG
    if (!modname || !defpath || !cbfn) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    modhdr = find_modhdr(modname);
    if (!modhdr) {
        modhdr = new_modhdr(modname);
        if (!modhdr) {
            return ERR_INTERNAL_MEM;
        }
        res = add_modhdr(modhdr);
        if (res != NO_ERR) {
            free_modhdr(modhdr);
            return res;
        }
    }

    callback = find_callback(modhdr, defpath);
    if (callback) {
        return SET_ERROR(ERR_NCX_DUP_ENTRY);
    }

    memset(&cbset, 0x0, sizeof(agt_cb_fnset_t));
    for (cbtyp = AGT_CB_VALIDATE;
         cbtyp <= AGT_CB_ROLLBACK;
         cbtyp++) {
        cbset.cbfn[cbtyp] = cbfn;
    }

    callback = new_callback(modhdr, 
                            defpath, 
                            version, 
                            &cbset);
    if (!callback) {
        return ERR_INTERNAL_MEM;
    }

    res = add_callback(modhdr, callback);
    if (res != NO_ERR) {
        return res;
    }

    /* data structures in place, now check if the module
     * is loaded yet
     */
    mod = ncx_find_module(modname, version);
    if (!mod) {
        /* module not present yet */
        return NO_ERR;
    }

    res = load_callbacks(mod, modhdr, callback);
        
    return res;

}  /* agt_cb_register_callback */


/********************************************************************
* FUNCTION agt_cb_register_callbacks
* 
* Register an object specific callback function
* setup array of callbacks, could be different or NULL
* to skip that phase
*
* INPUTS:
*   modname == module that defines the target object for
*              these callback functions 
*   defpath == Xpath with default (or no) prefixes
*              defining the object that will get the callbacks
*   version == exact module revision date expected
*              if condition not met then an error will
*              be logged  (TBD: force unload of module!)
*           == NULL means use any version of the module
*   cbfnset == address of callback function set to copy
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    agt_cb_register_callbacks (const xmlChar *modname,
                               const xmlChar *defpath,
                               const xmlChar *version,
                               const agt_cb_fnset_t *cbfnset)
{
    agt_cb_modhdr_t    *modhdr;
    agt_cb_set_t       *callback;
    ncx_module_t       *mod;
    status_t            res;

#ifdef DEBUG
    if (!modname || !defpath || !cbfnset) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    modhdr = find_modhdr(modname);
    if (!modhdr) {
        modhdr = new_modhdr(modname);
        if (!modhdr) {
            return ERR_INTERNAL_MEM;
        }
        res = add_modhdr(modhdr);
        if (res != NO_ERR) {
            free_modhdr(modhdr);
            return res;
        }
    }

    callback = find_callback(modhdr, defpath);
    if (callback) {
        return SET_ERROR(ERR_NCX_DUP_ENTRY);
    }

    callback = new_callback(modhdr, 
                            defpath, 
                            version, 
                            cbfnset);
    if (!callback) {
        return ERR_INTERNAL_MEM;
    }

    res = add_callback(modhdr, callback);
    if (res != NO_ERR) {
        return res;
    }

    /* data structures in place, now check if the module
     * is loaded yet
     */
    mod = ncx_find_module(modname, version);
    if (!mod) {
        /* module not present yet */
        return NO_ERR;
    }

    res = load_callbacks(mod, modhdr, callback);
        
    return res;

}  /* agt_cb_register_callbacks */


/********************************************************************
* FUNCTION agt_cb_unregister_callback
* 
* Unregister all callback functions for a specific object
*
* INPUTS:
*   modname == module containing the object for this callback
*   defpath == definition XPath location
*
* RETURNS:
*   none
*********************************************************************/
void
    agt_cb_unregister_callbacks (const xmlChar *modname,
                                 const xmlChar *defpath)
{
    agt_cb_modhdr_t  *modhdr;
    agt_cb_set_t     *callback;
    obj_template_t   *obj;
    status_t          res;

#ifdef DEBUG
    if (!modname || !defpath) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    modhdr = find_modhdr(modname);
    if (!modhdr) {
        /* entry not found; assume it is an early exit cleanup */
        return;
    }

    callback = find_callback(modhdr, defpath);
    if (!callback) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    dlq_remove(callback);
    free_callback(callback);

    if (dlq_empty(&modhdr->callbackQ)) {
        dlq_remove(modhdr);
        free_modhdr(modhdr);
    }

    res = xpath_find_schema_target_int(defpath, &obj);
    if (res == NO_ERR) {
        obj->cbset = NULL;
    }

}  /* agt_cb_unregister_callbacks */


/* END file agt_cb.c */
