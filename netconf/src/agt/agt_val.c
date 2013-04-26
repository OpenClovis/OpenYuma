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
/*  FILE: agt_val.c

   Manage Server callbacks for YANG-based config manipulation

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
20may06      abb      begun
03dec11      abb      convert tree traversal validation to
                      optimized validation task list

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_cfg.h"
#include "agt_commit_complete.h"
#include "agt_ncx.h"
#include "agt_util.h"
#include "agt_val.h"
#include "agt_val_parse.h"
#include "cap.h"
#include "cfg.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncxconst.h"
#include "obj.h"
#include "op.h"
#include "plock.h"
#include "rpc.h"
#include "rpc_err.h"
#include "status.h"
#include "typ.h"
#include "tstamp.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xpath.h"
#include "xpath_yang.h"
#include "xpath1.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* recursive callback function foward decls */
static status_t
    invoke_btype_cb (agt_cbtyp_t cbtyp,
                     op_editop_t editop,
                     ses_cb_t  *scb,
                     rpc_msg_t  *msg,
                     cfg_template_t *target,
                     val_value_t  *newval,
                     val_value_t  *curval,
                     val_value_t  *curparent );


static status_t
    handle_user_callback (agt_cbtyp_t cbtyp,
                          op_editop_t editop,
                          ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          val_value_t *newnode,
                          val_value_t *curnode,
                          boolean lookparent,
                          boolean indelete);

static status_t
    apply_commit_deletes (ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          cfg_template_t *target,
                          val_value_t *candval,
                          val_value_t *runval);



/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/
typedef struct unique_set_t_ {
    dlq_hdr_t  qhdr;
    dlq_hdr_t  uniqueQ;   /* Q of val_unique_t */
    val_value_t *valnode;  /* value tree back-ptr */
} unique_set_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
 * FUNCTION cvt_editop
 * Determine the effective edit (if OP_EDITOP_COMMIT)
 *
 * \param editop the edit operation to check
 * \param newnode the newval
 * \param curnode the curval
 *
 * \return the converted editoperation
 *********************************************************************/
static op_editop_t cvt_editop( op_editop_t editop,
                               const val_value_t *newval,
                               const val_value_t *curval)
{
    op_editop_t retop = editop;
    if (editop == OP_EDITOP_COMMIT) {
        if (newval && curval) {
            /***********************************
             * keep this simple so SIL code does not need to
             * worry about with-defaults basic-mode in use
            if (val_set_by_default(newval)) {
                retop = OP_EDITOP_DELETE;
            } else if (val_set_by_default(curval)) {
                retop = OP_EDITOP_CREATE;
            } else {
                retop = OP_EDITOP_REPLACE;
            }
            ***************************************/
            retop = OP_EDITOP_REPLACE;
        } else if (newval && !curval) {
            retop = OP_EDITOP_CREATE;
        } else if (!newval && curval) {
            retop = OP_EDITOP_DELETE;
        } else {
            retop = OP_EDITOP_REPLACE;  // error: both NULL!!
        }
    }
    return retop;

}  /* cvt_editop */


/********************************************************************
* FUNCTION skip_obj_commit_test
* 
* Check if the commit tests should be skipped for this object
* These tests on the object tree are more strict than the data tree
* because the data tree will never contain instances of obsolete,
* non-database, or disabled objects
*
* INPUTS:
*   obj ==obj object to check
*
* RETURNS:
*   TRUE if object should be skipped
*   FALSE if object should not be skipped
*********************************************************************/
static boolean skip_obj_commit_test(obj_template_t *obj)
{
    if (obj_is_cli(obj) || 
        !obj_has_name(obj) ||
        !obj_is_config(obj) ||
        (obj_is_abstract(obj) && !obj_is_root(obj)) ||
        /* removed because ncx_delete_all_obsolete_objects() has
         * already been called
         * obj_get_status(obj) == NCX_STATUS_OBSOLETE || */
        !obj_is_enabled(obj)) {
        return TRUE;
    }
    return FALSE;

}  /* skip_obj_commit_test */


/********************************************************************
* FUNCTION clean_node
* 
* Clean the value node and its ancestors from editvars and flags
*
* INPUTS:
*   val == node to check
*
*********************************************************************/
static void
    clean_node (val_value_t *val)

{
    if (val == NULL) {
        return;
    }

    if (obj_is_root(val->obj)) {
        return;
    }

    if (val->parent) {
        clean_node(val->parent);
    }

    val_free_editvars(val);

}  /* clean_node */


/********************************************************************
 * Create and store a change-audit record, if needed
 * this function generates a log message if log level 
 * set to LOG_INFO or higher; An audit record for a sysConfigChange
 * event is also generated
 *
 * \note !!! ONLY CALLED FOR RUNNING CONFIG !!!!
 *
 * \param editop edit operation requested
 * \param scb session control block
 * \param msg RPC message in progress
 * \param newnode top new value node involved in edit
 * \param curnode top cur value node involved in edit
 *********************************************************************/
static void
    handle_audit_record (op_editop_t editop,
                         ses_cb_t  *scb,
                         rpc_msg_t *msg,
                         val_value_t *newnode,
                         val_value_t *curnode)
{
    if (editop == OP_EDITOP_LOAD) {
        return;
    } 

    xmlChar *ibuff = NULL;
    const xmlChar *name = NULL;
    xmlChar tbuff[TSTAMP_MIN_SIZE+1];
    ncx_cfg_t cfgid;
    cfg_transaction_id_t txid;
    ses_id_t sid = (scb) ? SES_MY_SID(scb) : 0;
    const xmlChar *username = (scb && scb->username) ? scb->username 
        : (const xmlChar *)"nobody";
    const xmlChar *peeraddr = (scb && scb->peeraddr) ? scb->peeraddr : 
        (const xmlChar *)"localhost";

    editop = cvt_editop(editop, newnode, curnode);

    if (msg && msg->rpc_txcb) {
        cfgid = msg->rpc_txcb->cfg_id;
        name = cfg_get_config_name(cfgid);
        txid = msg->rpc_txcb->txid;
    } else {
        cfgid = NCX_CFGID_RUNNING;
        name = NCX_EL_RUNNING;
        txid = 0;
    }

    /* generate a log record */
    tstamp_datetime(tbuff);

    val_value_t *usenode = newnode ? newnode : curnode;
    if (usenode) {
        status_t res = val_gen_instance_id(NULL, usenode, NCX_IFMT_XPATH1, 
                                           &ibuff);
        if (res != NO_ERR) {
            log_error("\nError: Generate instance id failed (%s)",
                      get_error_string(res));
        }
    }

    log_info( "\nedit-transaction %llu: operation %s on session %d by %s@%s"
              "\n  at %s on target '%s'\n  data: %s\n",
              (unsigned long long)txid, op_editop_name(editop), sid,
              username, peeraddr, tbuff, name, 
              (ibuff) ? (const char *)ibuff : "??" );

    if (cfgid == NCX_CFGID_RUNNING && log_audit_is_open()) {
        log_audit_write( "\nedit-transaction %llu: operation %s on "
                         "session %d by %s@%s\n  at %s on target 'running'"
                         "\n  data: %s\n",
                         (unsigned long long)txid, op_editop_name(editop),
                         sid, username, peeraddr, tbuff,
                         (ibuff) ? (const char *)ibuff : "??");
    }

    /* generate a sysConfigChange notification */
    if ( cfgid == NCX_CFGID_RUNNING && msg && msg->rpc_txcb && ibuff ) {
        agt_cfg_audit_rec_t *auditrec = agt_cfg_new_auditrec(ibuff, editop);
        if ( !auditrec ) {
            log_error("\nError: malloc failed for audit record");
        } else {
            dlq_enque(auditrec, &msg->rpc_txcb->auditQ);
        }
    }

    if (ibuff) {
        m__free(ibuff);
    }

} /* handle_audit_record */


/********************************************************************
* FUNCTION handle_subtree_node_callback
* 
* Compare current node if config
* find the nodes that changed and call the
* relevant user callback functions
*
* INPUTS:
*    scb == session control block invoking the callback
*    msg == RPC message in progress
*    cbtyp == agent callback type
*    editop == edit operation applied to newnode oand/or curnode
*    newnode == new node in operation
*    curnode == current node in operation
*
* RETURNS:
*   status of the operation (usually returned from the callback)
*   NO USER CALLBACK FOUND == NO_ERR
*********************************************************************/
static status_t
    handle_subtree_node_callback (agt_cbtyp_t cbtyp,
                                  op_editop_t editop,
                                  ses_cb_t  *scb,
                                  rpc_msg_t  *msg,
                                  val_value_t *newnode,
                                  val_value_t *curnode)
{
    const xmlChar     *name;
    status_t           res;

    if (newnode != NULL) {
        name = newnode->name;
    } else if (curnode != NULL) {
        name = curnode->name;
    } else {
        name = NULL;
    }

    if (newnode && curnode) {
        int  retval;
        if (newnode->parent && 
            newnode->parent->editop == OP_EDITOP_REPLACE &&
            newnode->parent->editvars && 
            newnode->parent->editvars->operset) {
            /* replace parent was explicitly requested and this 
             * function would not have been called unless something
             * in the replaced node changed.  
             * Invoke the SIL callbacks for the child nodes
             * even if oldval == newval; 
             *
             * Operation must be explicitly set w/ operation="replace"
             * attribute because <copy-config> will also
             * appear to be a replace operation and the unchanged
             * subtrees must be skipped in this mode
             */
            retval = -1;
        } else {
            retval = val_compare_ex(newnode, curnode, TRUE);
            if (retval == 0) {
                /* apply here but nothing to do,
                 * so skip this entire subtree
                 */
                if (LOGDEBUG) {
                    log_debug("\nagt_val: Skipping subtree node "
                              "'%s', no changes",
                              (name) ? name : (const xmlChar *)"--");
                }
            }
        }
        if (retval == 0) {
            return NO_ERR;
        }
    }

    res = handle_user_callback(cbtyp, editop, scb, msg, newnode, 
                               curnode, FALSE,
                               (editop == OP_EDITOP_DELETE) ? TRUE : FALSE);

    return res;

} /* handle_subtree_node_callback */        


/********************************************************************
* FUNCTION handle_subtree_callback
* 
* Search the subtrees and find the correct user callback 
* functions and invoke them
*
* INPUTS:
*    cbtyp == agent callback type
*    editop == edit operation applied to newnode oand/or curnode
*    scb == session control block invoking the callback
*    msg == RPC message in progress
*    newnode == new node in operation
*    curnode == current node in operation
*
* RETURNS:
*   status of the operation (usually returned from the callback)
*   NO USER CALLBACK FOUND == NO_ERR
*********************************************************************/
static status_t
    handle_subtree_callback (agt_cbtyp_t cbtyp,
                             op_editop_t editop,
                             ses_cb_t  *scb,
                             rpc_msg_t  *msg,
                             val_value_t *newnode,
                             val_value_t *curnode)
{
    val_value_t  *newchild, *curchild;
    status_t      res = NO_ERR;

    if (newnode == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    for (newchild = val_get_first_child(newnode);
         newchild != NULL;
         newchild = val_get_next_child(newchild)) {

        if (!obj_is_config(newchild->obj)) {
            continue;
        }

        if (curnode != NULL) {
            curchild = val_first_child_match(curnode, newchild);
        } else {
            curchild = NULL;
        }

        res = handle_subtree_node_callback(cbtyp, editop, scb,
                                           msg, newchild, curchild);
        if (res != NO_ERR) {
            return res;
        }
    }

    if (curnode == NULL) {
        return NO_ERR;
    }

    for (curchild = val_get_first_child(curnode);
         curchild != NULL;
         curchild = val_get_next_child(curchild)) {

        if (!obj_is_config(curchild->obj)) {
            continue;
        }

        newchild = val_first_child_match(newnode, curchild);
        if (newchild == NULL) {
            res = handle_subtree_node_callback(cbtyp, OP_EDITOP_DELETE,
                                               scb, msg, NULL, curchild);
            if (res != NO_ERR) {
                return res;
            }
        }
    }

    return res;

} /* handle_subtree_callback */


/********************************************************************
* FUNCTION delete_children_first
* 
* Doing a delete operation and checking for the 
* sil-delete-children-first flag. Just calling the
* SIL callback functions, not actually removing the
* child nodes in case a rollback is needed
*
* Calls recursively for all child nodes where sil-delete-children-first
* is set, then invokes the SIL callback for the 'curnode' last
*
* INPUTS:
*    cbtyp == agent callback type
*    scb == session control block invoking the callback
*    msg == RPC message in progress
*    curnode == current node in operation
*
* RETURNS:
*   status of the operation (usually returned from the callback)
*   NO USER CALLBACK FOUND == NO_ERR
*********************************************************************/
static status_t
    delete_children_first (agt_cbtyp_t cbtyp,
                           ses_cb_t  *scb,
                           rpc_msg_t  *msg,
                           val_value_t *curnode)
{
    val_value_t  *curchild;
    status_t      res = NO_ERR;

    if (curnode == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (LOGDEBUG2) {
        log_debug2("\nEnter delete_children_first for '%s'", curnode->name);
    }

    for (curchild = val_get_first_child(curnode);
         curchild != NULL;
         curchild = val_get_next_child(curchild)) {

        if (!obj_is_config(curchild->obj)) {
            continue;
        }

        if (obj_is_leafy(curchild->obj)) {
            continue;
        }

        if (obj_is_sil_delete_children_first(curchild->obj)) {
            res = delete_children_first(cbtyp, scb, msg, curchild);
        } else {
            res = handle_subtree_node_callback(cbtyp, OP_EDITOP_DELETE, scb,
                                               msg, NULL, curchild);
        }
        if (res != NO_ERR) {
            return res;
        }
    }

    if (!obj_is_leafy(curnode->obj)) {
        res = handle_subtree_node_callback(cbtyp, OP_EDITOP_DELETE, scb,
                                           msg, NULL, curnode);
    }

    return res;

} /* delete_children_first */


/********************************************************************
* FUNCTION handle_user_callback
* 
* Find the correct user callback function and invoke it
*
* INPUTS:
*    cbtyp == agent callback type
*    editop == edit operation applied to newnode oand/or curnode
*    scb == session control block invoking the callback
*           !!! This parm should not be NULL because only the
*           !!! agt_val_root_check in agt.c calls with scb==NULL and
*           !!! that never invokes any SIL callbacks
*    msg == RPC message in progress
*    newnode == new node in operation
*    curnode == current node in operation
*    lookparent == TRUE if the parent should be checked for
*                  a callback function; FALSE otherwise
*    indelete == TRUE if in middle of a sil-delete-children-first
*                 callback; FALSE if not
* RETURNS:
*   status of the operation (usually returned from the callback)
*   NO USER CALLBACK FOUND == NO_ERR
*********************************************************************/
static status_t
    handle_user_callback (agt_cbtyp_t cbtyp,
                          op_editop_t editop,
                          ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          val_value_t *newnode,
                          val_value_t *curnode,
                          boolean lookparent,
                          boolean indelete)
{
    agt_cb_fnset_t    *cbset;
    val_value_t       *val;
    status_t           res;
    boolean            done;

    if (newnode) {
        val = newnode;
    } else if (curnode) {
        val = curnode;
    } else {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (obj_is_root(val->obj)) {
        return NO_ERR;
    }

    if (LOGDEBUG4) {
        log_debug4("\nChecking for %s user callback for %s edit on %s:%s",
                   agt_cbtype_name(cbtyp),
                   op_editop_name(editop),
                   val_get_mod_name(val),
                   val->name);
    }

    /* no difference during apply stage, and validate
     * stage checks differences before callback invoked
     * so treat remove the same as a delete for user callback purposes
     */
    if (editop == OP_EDITOP_REMOVE) {
        editop = OP_EDITOP_DELETE;
    }

    /* check if this is a child-first delete
     * the child nodes may have SIL callbacks and not the
     * parent, so the recursion is done before the
     * SIL callback is found for the current node
     * Only call during COMMIT phase
     */
    if (obj_is_sil_delete_children_first(val->obj) &&
        !obj_is_leafy(val->obj) &&
        !indelete && 
        editop == OP_EDITOP_DELETE &&
        cbtyp == AGT_CB_COMMIT) {
        res = delete_children_first(cbtyp, scb, msg, curnode);
        return res;
    }

    /* find the right callback function to call */
    done = FALSE;
    while (!done) {
        cbset = NULL;
        if (val->obj && val->obj->cbset) {
            cbset = val->obj->cbset;
        }
        if (cbset != NULL && cbset->cbfn[cbtyp] != NULL) {

            if (LOGDEBUG2) {
                log_debug2("\nFound %s user callback for %s:%s",
                           agt_cbtype_name(cbtyp),
                           op_editop_name(editop),
                           val_get_mod_name(val),
                           val->name);
            }

            editop = cvt_editop(editop, newnode, curnode);

            res = (*cbset->cbfn[cbtyp])(scb, msg, cbtyp, editop, 
                                        newnode, curnode);
            if (val->res == NO_ERR) {
                val->res = res;
            }

            if (LOGDEBUG && res != NO_ERR) {
                log_debug("\n%s user callback failed (%s) for %s on %s:%s",
                          agt_cbtype_name(cbtyp),
                          get_error_string(res),
                          op_editop_name(editop),
                          val_get_mod_name(val),
                          val->name);
            }
            if (res == NO_ERR) {
                switch (editop) {
                case OP_EDITOP_CREATE:
                case OP_EDITOP_MERGE:
                case OP_EDITOP_REPLACE:
                case OP_EDITOP_LOAD:
                    if (cbtyp != AGT_CB_VALIDATE) {
                        res = handle_subtree_callback(cbtyp, editop, scb, msg,
                                                      newnode, curnode);
                    }
                    break;
                default:
                    ;
                }
            }
            return res;
        } else if (!lookparent) {
            done = TRUE;
        } else if (val->parent != NULL &&
                   !obj_is_root(val->parent->obj) &&
                   val_get_nsid(val) == val_get_nsid(val->parent)) {
            val = val->parent;
        } else {
            done = TRUE;
        }
    }

    return NO_ERR;

} /* handle_user_callback */


/********************************************************************
* FUNCTION add_undo_node
* 
* Add an undo node to the msg->undoQ
*
* INPUTS:
*    msg == RPC message in progress
*    editop == edit-config operation attribute value
*    newnode == node from PDU
*    newnode_marker == newnode placeholder
*    curnode == node from database (if any)
*    curnode_marker == curnode placeholder
*    parentnode == parent of curnode (or would be)
* OUTPUTS:
*    agt_cfg_undo_rec_t struct added to msg->undoQ
*
* RETURNS:
*   pointer to new undo record, in case any extra_deleteQ
*   items need to be added; NULL on ERR_INTERNAL_MEM error
*********************************************************************/
static agt_cfg_undo_rec_t *
    add_undo_node (rpc_msg_t *msg,
                   op_editop_t editop,
                   val_value_t *newnode,
                   val_value_t *newnode_marker,
                   val_value_t *curnode,
                   val_value_t *curnode_marker,
                   val_value_t *parentnode)
{
    agt_cfg_undo_rec_t *undo;

    /* create an undo record for this merge */
    undo = agt_cfg_new_undorec();
    if (!undo) {
        return NULL;
    }

    /* save a copy of the current value in case it gets modified
     * in a merge operation;; done for leafs only!!!
     */
    if (curnode && obj_is_leafy(curnode->obj)) {
        undo->curnode_clone = val_clone(curnode);
        if (!undo->curnode_clone) {
            agt_cfg_free_undorec(undo);
            return NULL;
        }
    }

    undo->editop = cvt_editop(editop, newnode, curnode);
    undo->newnode = newnode;
    undo->newnode_marker = newnode_marker;
    undo->curnode = curnode;
    undo->curnode_marker = curnode_marker;
    undo->parentnode = parentnode;

    dlq_enque(undo, &msg->rpc_txcb->undoQ);
    return undo;

} /* add_undo_node */


/********************************************************************
* FUNCTION add_child_clean
* 
*  Add a child value node to a parent value node
*  This is only called by the agent when adding nodes
*  to a target database.
*
*   Pass in the editvar to use
*
*  If the child node being added is part of a choice/case,
*  then all sibling nodes in other cases within the same
*  choice will be deleted
*
*  The insert operation will also be check to see
*  if the child is a list oo a leaf-list, which is ordered-by user
*
*  The default insert mode is always 'last'
*
* INPUTS:
*    editvars == val_editvars_t struct to use (may be NULL)
*    child == node to store in the parent
*    parent == complex value node with a childQ
*    cleanQ == address of Q to receive any agt_cfg_nodeptr_t
*      structs for nodes marked as deleted
*
* OUTPUTS:
*    cleanQ may have nodes added if the child being added
*    is part of a case.  All other cases will be marked as deleted
*    from the parent Q and nodeptrs added to the cleanQ
* RETURNS:
*    status
*********************************************************************/
static status_t
    add_child_clean (val_editvars_t *editvars,
                     val_value_t *child,
                     val_value_t *parent,
                     dlq_hdr_t *cleanQ)
{
    val_value_t  *testval, *nextval;
    agt_cfg_nodeptr_t *nodeptr;

    if (child->casobj) {
        for (testval = val_get_first_child(parent);
             testval != NULL;
             testval = nextval) {

            nextval = val_get_next_child(testval);
            if (testval->casobj && 
                (testval->casobj->parent == child->casobj->parent)) {

                if (testval->casobj != child->casobj) {
                    log_debug3("\nagt_val: clean old case member '%s'"
                               " from parent '%s'",
                               testval->name, parent->name);

                    nodeptr = agt_cfg_new_nodeptr(testval);
                    if (nodeptr == NULL) {
                        return ERR_INTERNAL_MEM;
                    }
                    dlq_enque(nodeptr, cleanQ);
                    VAL_MARK_DELETED(testval);
                }
            }
        }
    }

    child->parent = parent;

    boolean doins = FALSE;
    if (child->obj->objtype == OBJ_TYP_LIST) {
        doins = TRUE;
    } else if (child->obj->objtype == OBJ_TYP_LEAF_LIST) {
        doins = TRUE;
    }

    op_insertop_t insertop = OP_INSOP_NONE;
    if (editvars) {
        insertop = editvars->insertop;
    }
    if (doins) {
        switch (insertop) {
        case OP_INSOP_FIRST:
            testval = val_find_child(parent, val_get_mod_name(child),
                                     child->name);
            if (testval) {
                dlq_insertAhead(child, testval);
            } else {
                val_add_child_sorted(child, parent);
            }
            break;
        case OP_INSOP_LAST:
        case OP_INSOP_NONE:
            val_add_child_sorted(child, parent);
            break;
        case OP_INSOP_BEFORE:
        case OP_INSOP_AFTER:
            /* find the entry specified by the val->insertstr value
             * this is value='foo' for leaf-list and
             * key="[x:foo='bar'][x:foo2=7]" for list
             */
            if (child->obj->objtype == OBJ_TYP_LEAF_LIST ||
                child->obj->objtype == OBJ_TYP_LIST) {

                if (editvars && editvars->insertval) {
                    testval = editvars->insertval;
                    if (editvars->insertop == OP_INSOP_BEFORE) {
                        dlq_insertAhead(child, testval);
                    } else {
                        dlq_insertAfter(child, testval);
                    }
                } else {
                    SET_ERROR(ERR_NCX_INSERT_MISSING_INSTANCE);
                    val_add_child_sorted(child, parent);
                }
            } else {
                /* wrong object type */
                SET_ERROR(ERR_INTERNAL_VAL);
                val_add_child_sorted(child, parent);
            }
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            val_add_child_sorted(child, parent);
        }
    } else {
        val_add_child_sorted(child, parent);
    }
    return NO_ERR;

}   /* add_child_clean */


/********************************************************************
* FUNCTION add_child_node
* 
* Add a child node
*
* INPUTS:
*   child == child value to add
*   marker == newval marker for this child node (if needed_
*             !! consuming this parm here so caller can assume
*             !! memory is transferred even if error returned
*   parent == parent value to add child to
*   msg == RPC messing to use
*   editop == edit operation to save in undo rec
*
* OUTPUTS:
*    child added to parent->v.childQ
*    any other cases removed and either added to undo node or deleted
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_child_node (val_value_t  *child,
                    val_value_t  *marker,
                    val_value_t  *parent,
                    rpc_msg_t *msg,
                    op_editop_t editop)
{
    if (LOGDEBUG3) {
        log_debug3("\nAdd child '%s' to parent '%s'",
                   child->name, 
                   parent->name);
    }

    agt_cfg_undo_rec_t *undo = add_undo_node(msg, editop, child, marker,
                                             NULL, NULL, parent);
    if (undo == NULL) {
        val_free_value(marker);
        return ERR_INTERNAL_MEM;
    }
    undo->apply_res = add_child_clean(child->editvars, child, parent, 
                                      &undo->extra_deleteQ);
    undo->edit_action = AGT_CFG_EDIT_ACTION_ADD;
    return undo->apply_res;

}  /* add_child_node */


/********************************************************************
* FUNCTION move_child_node
* 
* Move a child list or leaf-list node
*
* INPUTS:
*   newchild == new child value to add
*   newchild_marker == new child marker for undo rec if needed
*             !! consuming this parm here so caller can assume
*             !! memory is transferred even if error returned
*   curchild == existing child value to move
*   parent == parent value to move child within
*   msg == message in progress
*   editop == edit operation to save in undo rec
*
* OUTPUTS:
*    child added to parent->v.childQ
*    any other cases removed and either added to undo node or deleted
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    move_child_node (val_value_t  *newchild,
                     val_value_t  *newchild_marker,
                     val_value_t  *curchild,
                     val_value_t  *parent,
                     rpc_msg_t  *msg,
                     op_editop_t editop)
{
    if (LOGDEBUG3) {
        log_debug3("\nMove child '%s' in parent '%s'",
                   newchild->name, 
                   parent->name);
    }

    agt_cfg_undo_rec_t *undo = add_undo_node(msg, editop, newchild, 
                                             newchild_marker, curchild, 
                                             NULL, parent);
    if (undo == NULL) {
        /* no detailed rpc-error is recorded here!!!
         * relying on catch-all operation-failed in agt_rpc_dispatch */
        val_free_value(newchild_marker);
        return ERR_INTERNAL_MEM;
    }

    VAL_MARK_DELETED(curchild);

    undo->apply_res = add_child_clean(newchild->editvars, newchild, parent, 
                                      &undo->extra_deleteQ);
    undo->edit_action = AGT_CFG_EDIT_ACTION_MOVE;
    return undo->apply_res;

}  /* move_child_node */


/********************************************************************
* FUNCTION move_mergedlist_node
* 
* Move a list node that was just merged with
* the contents of the newval; now the curval needs
* to be moved
*
* INPUTS:
*   newchild == new child value with the editvars
*               to use to move curchild
*   curchild == existing child value to move
*   parent == parent value to move child within
*   undo == undo record in progress
*
* OUTPUTS:
*    child added to parent->v.childQ
*    any other cases removed and either added to undo node or deleted
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    move_mergedlist_node (val_value_t  *newchild,
                          val_value_t  *curchild,
                          val_value_t  *parent,
                          agt_cfg_undo_rec_t *undo)
{
    if (LOGDEBUG3) {
        log_debug3("\nMove list '%s' in parent '%s'",
                   newchild->name, 
                   parent->name);
    }

    val_value_t *marker = val_new_deleted_value();
    if (marker == NULL) {
        return ERR_INTERNAL_MEM;
    }
    undo->curnode_marker = marker;
    val_swap_child(marker, curchild);
    undo->apply_res = add_child_clean(newchild->editvars, curchild, parent,
                                      &undo->extra_deleteQ);
    undo->edit_action = AGT_CFG_EDIT_ACTION_MOVE;
    return undo->apply_res;

}  /* move_mergedlist_node */


/********************************************************************
* FUNCTION restore_newnode
* 
* Swap the newnode and newnode_pointers
*
* INPUTS:
*   undo == undo record to use
*********************************************************************/
static void
    restore_newnode (agt_cfg_undo_rec_t *undo)
{
    if (undo->newnode_marker) {
        if (undo->newnode) {
            val_swap_child(undo->newnode, undo->newnode_marker);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        val_free_value(undo->newnode_marker);
        undo->newnode_marker = NULL;
    }

}  /* restore_newnode */


/********************************************************************
* FUNCTION restore_newnode2
* 
* Swap the newnode and newnode_pointers
* There was no undo record created
*
* INPUTS:
*   newval == new value currently not in any tree
*   newval_marker == marker for newval in source tree
*********************************************************************/
static void
    restore_newnode2 (val_value_t *newval,
                      val_value_t *newval_marker)
{
    if (newval && newval_marker) {
        val_swap_child(newval, newval_marker);
    }
    val_free_value(newval_marker);

}  /* restore_newnode2 */


/********************************************************************
* FUNCTION restore_curnode
* 
* Restore the curnode 
*
* INPUTS:
*   undo == undo record to use
*   target == target database being edited
*********************************************************************/
static void
    restore_curnode (agt_cfg_undo_rec_t *undo,
                     cfg_template_t *target)
{
    if (undo->curnode == NULL) {
        return;
    }
    if (VAL_IS_DELETED(undo->curnode)) {
        /* curnode is a complex type that needs to be undeleted */
        VAL_UNMARK_DELETED(undo->curnode);
        undo->free_curnode = FALSE;
        if (undo->commit_res != ERR_NCX_SKIPPED &&
            target->cfg_id == NCX_CFGID_RUNNING) {
            val_check_swap_resnode(undo->newnode, undo->curnode);
        }
    } else if (undo->curnode_clone) {
        /* node is a leaf or leaf-list that was merged */
        if (undo->commit_res != ERR_NCX_SKIPPED &&
            target->cfg_id == NCX_CFGID_RUNNING) {
            val_check_swap_resnode(undo->newnode, undo->curnode_clone);
        }
        if (undo->curnode_marker) {
            /* the curnode was moved so move it back */
            val_remove_child(undo->curnode);
            val_swap_child(undo->curnode_clone, undo->curnode_marker);
            val_free_value(undo->curnode_marker);
            undo->curnode_marker = NULL;
        } else {
            val_swap_child(undo->curnode_clone, undo->curnode);
        }
        undo->curnode_clone = NULL;
        undo->free_curnode = TRUE;
    } else {
        // should not happen 
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* restore_curnode */


/********************************************************************
* FUNCTION restore_extra_deletes
* 
* Restore any extra deleted nodes from other YANG cases
*
* INPUTS:
*   nodeptrQ == Q of agt_cfg_nodeptr_t records to use
*********************************************************************/
static void
    restore_extra_deletes (dlq_hdr_t *nodeptrQ)
{

    while (!dlq_empty(nodeptrQ)) {
        agt_cfg_nodeptr_t *nodeptr = (agt_cfg_nodeptr_t *)dlq_deque(nodeptrQ);
        if (nodeptr && nodeptr->node) {
            VAL_UNMARK_DELETED(nodeptr->node);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        agt_cfg_free_nodeptr(nodeptr);
    }

}  /* restore_extra_deletes */


/********************************************************************
* FUNCTION finish_extra_deletes
* 
* Finish deleting any extra deleted nodes from other YANG cases
*
* INPUTS:
*   cfgid == internal config ID of the target database
*   undo == undo record to use
*********************************************************************/
static void
    finish_extra_deletes (ncx_cfg_t cfgid,
                          agt_cfg_undo_rec_t *undo)
{

    while (!dlq_empty(&undo->extra_deleteQ)) {
        agt_cfg_nodeptr_t *nodeptr = (agt_cfg_nodeptr_t *)
            dlq_deque(&undo->extra_deleteQ);
        if (nodeptr && nodeptr->node) {
            /* mark ancestor nodes dirty before deleting this node */
            if (cfgid == NCX_CFGID_RUNNING) {
                val_clear_dirty_flag(nodeptr->node);
            } else {
                val_set_dirty_flag(nodeptr->node);
            }
            val_remove_child(nodeptr->node);
            val_free_value(nodeptr->node);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        agt_cfg_free_nodeptr(nodeptr);
    }

}  /* finish_extra_deletes */


/********************************************************************
* FUNCTION check_insert_attr
* 
* Check the YANG insert attribute
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   newval == val_value_t from the PDU
*   curparent == current parent node for inserting children
*   
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_insert_attr (ses_cb_t  *scb,
                       rpc_msg_t  *msg,
                       val_value_t  *newval,
                       val_value_t *curparent)
{
    val_value_t   *testval, *simval, *insertval;
    status_t       res = NO_ERR;
    op_editop_t    editop = OP_EDITOP_NONE;

    if (newval == NULL) {
        return NO_ERR;  // no insert attr possible
    }

    editop = newval->editop;
    if (editop == OP_EDITOP_DELETE || editop == OP_EDITOP_REMOVE) {
        /* this error already checked in agt_val_parse */
        return NO_ERR;
    }

    if (newval->obj == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    switch (newval->obj->objtype) {
    case OBJ_TYP_LEAF_LIST:
        /* make sure the insert attr is on a node with a parent
         * this should always be true since the docroot would
         * be the parent of every accessible object instance
         *
         * OK to check insertstr, otherwise errors
         * should already be recorded by agt_val_parse
         */
        if (newval->editvars == NULL ||
            newval->editvars->insertop == OP_INSOP_NONE) {
            return NO_ERR;
        }

        if (!newval->editvars->insertstr) {
            /* insert op already checked in agt_val_parse */
            return NO_ERR;
        }

        if (obj_is_system_ordered(newval->obj)) {
            res = ERR_NCX_UNEXPECTED_INSERT_ATTRS;
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_CONTENT,
                             res,
                             NULL,
                             NCX_NT_STRING,
                             newval->editvars->insertstr,
                             NCX_NT_VAL,
                             newval);
            return res;
        }

        /* validate the insert string against siblings
         * make a value node to compare in the
         * value space instead of the lexicographical space
         */
        simval = val_make_simval_obj(newval->obj,
                                     newval->editvars->insertstr, &res);
        if (res == NO_ERR && simval) {
            testval = val_first_child_match(curparent, simval);
            if (!testval) {
                /* sibling leaf-list with the specified
                 * value was not found (13.7)
                 */
                res = ERR_NCX_INSERT_MISSING_INSTANCE;
            } else {
                newval->editvars->insertval = testval;
            }
        }
                    
        if (simval) {
            val_free_value(simval);
        }
        break;
    case OBJ_TYP_LIST:
        /* there should be a 'key' attribute
         * OK to check insertxpcb, otherwise errors
         * should already be recorded by agt_val_parse
         */
        if (newval->editvars == NULL ||
            newval->editvars->insertop == OP_INSOP_NONE) {
            return NO_ERR;
        }

        if (!newval->editvars->insertstr) {
            /* insert op already checked in agt_val_parse */
            return NO_ERR;
        }

        if (!newval->editvars->insertxpcb) {
            /* insert op already checked in agt_val_parse */
            return NO_ERR;
        }

        if (obj_is_system_ordered(newval->obj)) {
            res = ERR_NCX_UNEXPECTED_INSERT_ATTRS;
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_CONTENT,
                             res,
                             NULL,
                             NCX_NT_STRING,
                             newval->editvars->insertstr,
                             NCX_NT_VAL,
                             newval);
            return res;
        }

        if (newval->editvars->insertxpcb->validateres != NO_ERR) {
            res = newval->editvars->insertxpcb->validateres;
        } else {
            res = xpath_yang_validate_xmlkey(scb->reader,
                                             newval->editvars->insertxpcb,
                                             newval->obj, FALSE);
        }
        if (res == NO_ERR) {
            /* get the list entry that the 'key' attribute
             * referenced. It should be valid objects
             * and well-formed from passing the previous test
             */
            testval = val_make_from_insertxpcb(newval, &res);
            if (res == NO_ERR && testval) {
                val_set_canonical_order(testval);
                insertval = val_first_child_match(curparent,  testval);
                if (!insertval) {
                    /* sibling list with the specified
                     * key value was not found (13.7)
                     */
                    res = ERR_NCX_INSERT_MISSING_INSTANCE;
                } else {
                    newval->editvars->insertval = insertval;
                }
            }
            if (testval) {
                val_free_value(testval);
                testval = NULL;
            }
        }
        break;
    default:
        if (newval->editvars && newval->editvars->insertop != OP_INSOP_NONE) {
            res = ERR_NCX_UNEXPECTED_INSERT_ATTRS;
            agt_record_error(scb,
                             &msg->mhdr,
                             NCX_LAYER_CONTENT,
                             res,
                             NULL,
                             newval->editvars->insertstr ? 
                             NCX_NT_STRING : NCX_NT_NONE,
                             newval->editvars->insertstr,
                             NCX_NT_VAL,
                             newval);
            return res;
        }
    }
             
    if (res != NO_ERR) {
        agt_record_insert_error(scb, &msg->mhdr, res, newval);
    }

    return res;

}  /* check_insert_attr */


/********************************************************************
* FUNCTION commit_delete_allowed
* 
* Check if the current session is allowed to delete
* the node found in the requested commit delete operation
*
* INPUTS:
*   scb == session control block
*   msg == incoming commit rpc_msg_t in progress
*   deleteval == value struct to check
*   isfirst == TRUE if top-level caller; FALSE if recursive call
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhdr.errQ
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    commit_delete_allowed (ses_cb_t  *scb,
                           rpc_msg_t  *msg,
                           val_value_t *deleteval,
                           boolean isfirst)
{
    status_t          res = NO_ERR;

    if (obj_is_root(deleteval->obj)) {
        return NO_ERR;
    }

    /* recurse until the config root is found and check all
     * the nodes top to bottom to see if this random XPath resnode
     * can be deleted by the current session right now
     */
    if (deleteval->parent) {
        res = commit_delete_allowed(scb, msg, deleteval->parent, FALSE);
        if (res != NO_ERR) {
            return res;
        }
    }

    op_editop_t op = (isfirst) ? OP_EDITOP_DELETE : OP_EDITOP_MERGE;

    /* check if access control allows the delete */
    if (!agt_acm_val_write_allowed(&msg->mhdr, scb->username, 
                                   NULL, deleteval, op)) {
        res = ERR_NCX_ACCESS_DENIED;
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, NULL, 
                         NCX_NT_NONE, NULL, NCX_NT_NONE, NULL);
        return res;
    }

    /* check if this curval is partially locked */
    uint32 lockid = 0;
    res = val_write_ok(deleteval, op, SES_MY_SID(scb), TRUE, &lockid);
    if (res != NO_ERR) {
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_CONTENT, res,
                         NULL, NCX_NT_UINT32_PTR, &lockid,
                         NCX_NT_VAL, deleteval);
        return res;
    }

    return res;

}   /* commit_delete_allowed */


/********************************************************************
* FUNCTION check_commit_deletes
* 
* Check the requested commit delete operations
*
* INPUTS:
*   scb == session control block
*   msg == incoming commit rpc_msg_t in progress
*   candval == value struct from the candidate config
*   runval == value struct from the running config
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhdr.errQ
*
* RETURNS:
*   none
*********************************************************************/
static status_t
    check_commit_deletes (ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          val_value_t *candval,
                          val_value_t *runval)
{
    if (candval == NULL || runval == NULL) {
        return NO_ERR;
    }

    log_debug3("\ncheck_commit_deletes: start %s ", runval->name);

    /* go through running config
     * if the matching node is not in the candidate,
     * then delete that node in the running config as well
     */
    status_t res = NO_ERR;
    val_value_t *curval = val_get_first_child(runval);
    for (; curval != NULL; curval = val_get_next_child(curval)) {

        /* check only database config nodes */
        if (obj_is_data_db(curval->obj) && obj_is_config(curval->obj)) {

            /* check if node deleted in source */
            val_value_t *matchval = val_first_child_match(candval, curval);
            if (!matchval) {
                res = commit_delete_allowed(scb, msg, curval, TRUE);
                if (res != NO_ERR) {
                    return res;         /* errors recorded */
                }
            }
        }  /* else skip non-config database node */
    }

    return res;

}   /* check_commit_deletes */


/********************************************************************
* FUNCTION apply_write_val
* 
* Invoke all the AGT_CB_APPLY callbacks for a 
* source and target and write operation
*
* INPUTS:
*   editop == edit operation in effect on the current node
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == config database target 
*   parent == parent value of curval and newval
*   newval == new value to apply
*   curval == current instance of value (may be NULL if none)
*   *done  == TRUE if the node manipulation is done
*          == FALSE if none of the parent nodes have already
*             edited the config nodes; just do user callbacks
*
* OUTPUTS:
*   newval and curval may be moved around to
*       different queues, or get modified
*   *done may be changed from FALSE to TRUE if node is applied here
* RETURNS:
*   status
*********************************************************************/
static status_t
    apply_write_val (op_editop_t  editop,
                     ses_cb_t  *scb,
                     rpc_msg_t  *msg,
                     cfg_template_t *target,
                     val_value_t  *parent,
                     val_value_t  *newval,
                     val_value_t  *curval,
                     boolean      *done)
{
    const xmlChar   *name = NULL;
    agt_cfg_undo_rec_t *undo = NULL;
    val_value_t     *newval_marker = NULL;
    status_t         res = NO_ERR;
    op_editop_t      cur_editop = OP_EDITOP_NONE;
    boolean          applyhere = FALSE, topreplace = FALSE;
    boolean          add_defs_done = FALSE;

    if (newval) {
        cur_editop = (newval->editop == OP_EDITOP_NONE) 
            ? editop : newval->editop;
        name = newval->name;
    } else if (curval) {
        cur_editop = editop;
        name = curval->name;
    } else {
        *done = TRUE;
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* check if this is a top-level replace via <copy-config> or
     * a restore-from-backup replace operation   */
    if (newval && obj_is_root(newval->obj) && 
        msg->rpc_txcb->edit_type == AGT_CFG_EDIT_TYPE_FULL) {
        topreplace = TRUE;
    }

    log_debug4("\napply_write_val: %s start", name);

    /* check if this node needs the edit operation applied */
    if (*done || (newval && obj_is_root(newval->obj))) {
        applyhere = FALSE;
    } else if (cur_editop == OP_EDITOP_COMMIT) {
        /* make sure the VAL_FL_DIRTY flag is set, not just
         * the VAL_FL_SUBTREE_DIRTY flag   */
        applyhere = (newval) ? val_get_dirty_flag(newval) : FALSE;
        *done = applyhere;
    } else if (editop == OP_EDITOP_DELETE || editop == OP_EDITOP_REMOVE) {
        applyhere = TRUE;
        *done = TRUE;
    } else {
        applyhere = agt_apply_this_node(cur_editop, newval, curval);
        *done = applyhere;
    }

    if (applyhere && newval) {
        /* check corner case applying to the config root */
        if (obj_is_root(newval->obj)) {
            ;
        } else if (!typ_is_simple(newval->btyp) && !add_defs_done && 
                   editop != OP_EDITOP_DELETE && editop != OP_EDITOP_REMOVE) {

            res = val_add_defaults(newval, (target) ? target->root : NULL,
                                   curval, FALSE);
            if (res != NO_ERR) {
                log_error("\nError: add defaults failed");
                applyhere = FALSE;
            }
        }
    }

    /* apply the requested edit operation */    
    if (applyhere) {

        if (LOGDEBUG) {
            val_value_t *logval;

            if (curval) {
                logval = curval;
            } else if (newval) {
                logval = newval;
            } else {
                logval = NULL;
            }
            if (logval) {
                xmlChar *buff = NULL;
                res = val_gen_instance_id(&msg->mhdr, logval,
                                          NCX_IFMT_XPATH1, &buff);
                if (res == NO_ERR) {
                    log_debug("\napply_write_val: apply %s on %s",
                              op_editop_name(editop), buff);
                } else {
                    log_debug("\napply_write_val: apply %s on %s", 
                              op_editop_name(editop), name);
                }
                if (buff) {
                    m__free(buff);
                }
            }
        }

        if (target->cfg_id == NCX_CFGID_RUNNING) {
            /* only call SIL-apply once, when the data tree edits
             * are applied to the running config.
             * The candidate target is skipped so the SIL
             * apply callback is not called twice.
             * Also there is no corresponding callback
             * to cleanup the candidate if <discard-changes>
             * is invoked by the client or the server */
            res = handle_user_callback(AGT_CB_APPLY, editop, scb, msg, 
                                       newval, curval, TRUE, FALSE);
            if (res != NO_ERR) {
                /* assuming SIL added an rpc-error already
                 * relying on catch-all operation-failed in agt_rpc_dispatch */
                return res;
            }
        }

        /* exit here if the edited node is a virtual value
         * it is assumed that the user callback handled its own
         * internal representation of this node any subtrees */
        if (curval && val_is_virtual(curval)) {
            /* update the last change time; assume the virtual
             * node contents changed  */
            cfg_update_last_ch_time(target);
            return NO_ERR;
        }

        /* handle recoverable edit;
         * make a deleted node markers for newval or curval if they need
         * to be removed from their parent nodes; this is needed
         * for non-descructive commit from candidate to running
         * or copy from running to startup; the source and target are
         * no longer deleted, just marked as deleted until commit
         * The newnode and curnode pointers MUST be rooted in a data
         * tree until after all SIL callbacks have been invoked  */
        switch (cur_editop) {
        case OP_EDITOP_LOAD:
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            /* leave source tree alone since it will not be re-parented
             * in the target tree */
            break;
        default:
            /* remove newval node; swap with newval_marker for undo */
            if (newval) {
                newval_marker = val_new_deleted_value();
                if (newval_marker) {
                    val_swap_child(newval_marker, newval);
                } else {
                    return ERR_INTERNAL_MEM;
                }
            } // else keep source leaf or leaf-list node in place
        }

        switch (cur_editop) {
        case OP_EDITOP_MERGE:
            if (curval) {
                /* This is a leaf:
                 * The applyhere code will not pick a complex node for
                 * merging if the current node already exists.
                 * The current node is always used instead of the new node
                 * for merge, in case there are any read-only descendant
                 * nodes already     */
                if (newval && newval->editvars && newval->editvars->insertstr) {
                    res = move_child_node(newval, newval_marker, curval,
                                          parent, msg, cur_editop);
                } else if (newval) {
                    /* merging a simple leaf or leaf-list
                     * make a copy of the current node first */
                    undo = add_undo_node(msg, cur_editop, newval, 
                                         newval_marker, curval, NULL, parent);
                    if (undo == NULL) {
                        /* no detailed rpc-error is recorded here!!!
                         * relying on catch-all operation-failed in 
                         * agt_rpc_dispatch */
                        restore_newnode2(newval, newval_marker);
                        return ERR_INTERNAL_MEM;
                     }
                     undo->apply_res = res = val_merge(newval, curval);
                     undo->edit_action = AGT_CFG_EDIT_ACTION_SET;
                     restore_newnode(undo);
                } else {
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                }
            } else if (newval) {
                res = add_child_node(newval, newval_marker, parent, msg, 
                                     cur_editop);
            } else {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
            break;
        case OP_EDITOP_REPLACE:
        case OP_EDITOP_COMMIT:
            if (curval) {
                 if (newval && newval->editvars && 
                     newval->editvars->insertstr) {
                     res = move_child_node(newval, newval_marker, curval,
                                           parent, msg, cur_editop);
                 } else if (newval) {
                     undo = add_undo_node(msg, cur_editop, newval, 
                                          newval_marker, curval, NULL, parent);
                     if (undo == NULL) {
                         /* no detailed rpc-error is recorded here!!!
                          * relying on catch-all operation-failed in 
                          * agt_rpc_dispatch */
                         restore_newnode2(newval, newval_marker);
                         return ERR_INTERNAL_MEM;
                     }

                     if (obj_is_leafy(curval->obj)) {
                         undo->apply_res = res = val_merge(newval, curval);
                         undo->edit_action = AGT_CFG_EDIT_ACTION_SET;
                         restore_newnode(undo);
                     } else {
                         val_insert_child(newval, curval, parent);
                         VAL_MARK_DELETED(curval);
                         undo->edit_action = AGT_CFG_EDIT_ACTION_REPLACE;
                         undo->free_curnode = TRUE;
                     }
                 } else {
                     res = SET_ERROR(ERR_INTERNAL_VAL);
                 }
            } else if (newval) {
                 res = add_child_node(newval, newval_marker, parent, msg,
                                      cur_editop);
            } else {
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
             break;
         case OP_EDITOP_CREATE:
             if (newval && curval) {
                 /* there must be a default leaf */
                 undo = add_undo_node(msg, cur_editop, newval, 
                                      newval_marker, curval, NULL, parent);
                 if (undo == NULL) {
                     /* no detailed rpc-error is recorded here!!!
                      * relying on catch-all operation-failed in 
                      * agt_rpc_dispatch */
                     restore_newnode2(newval, newval_marker);
                     return ERR_INTERNAL_MEM;
                 }
                 undo->apply_res = res = val_merge(newval, curval);
                 undo->edit_action = AGT_CFG_EDIT_ACTION_SET;
                 restore_newnode(undo);
             } else if (newval) {
                 res = add_child_node(newval, newval_marker, parent, msg,
                                      cur_editop);
             } else {
                 res = SET_ERROR(ERR_INTERNAL_VAL);
             }
             break;
         case OP_EDITOP_LOAD:
             assert( newval && "newval NULL in LOAD" );
             undo = add_undo_node(msg, cur_editop, newval, newval_marker, 
                                  curval, NULL, parent);
             if (undo == NULL) {
                 /* no detailed rpc-error is recorded here!!!
                  * relying on catch-all operation-failed in 
                  * agt_rpc_dispatch */
                 restore_newnode2(newval, newval_marker);
                 return ERR_INTERNAL_MEM;
             }
             break;
         case OP_EDITOP_DELETE:
         case OP_EDITOP_REMOVE:
             if (!curval) {
                 /* this was a REMOVE operation NO-OP (?verify?) */
                 break;
             }

             undo = add_undo_node(msg, cur_editop, newval, newval_marker,
                                  curval, NULL, parent);
             if (undo == NULL) {
                 /* no detailed rpc-error is recorded here!!!
                  * relying on catch-all operation-failed in agt_rpc_dispatch */
                 restore_newnode2(newval, newval_marker);
                 return ERR_INTERNAL_MEM;
             }

             if (curval->parent) {
                 val_set_dirty_flag(curval->parent);
             }

             /* check if this is a leaf with a default */
             if (obj_get_default(curval->obj)) {
                 /* convert this leaf to its default value */
                 res = val_delete_default_leaf(curval);
                 undo->edit_action = AGT_CFG_EDIT_ACTION_DELETE_DEFAULT;
             } else {
                 /* mark this node as deleted */
                 VAL_MARK_DELETED(curval);
                 undo->edit_action = AGT_CFG_EDIT_ACTION_DELETE;
                 undo->free_curnode = TRUE;
             }
             break;
         default:
             return SET_ERROR(ERR_INTERNAL_VAL);
         }
     }

     if (res == NO_ERR && applyhere == TRUE && newval && curval &&
         newval->btyp == NCX_BT_LIST && cur_editop == OP_EDITOP_MERGE) {

         if (undo == NULL) {
             undo = add_undo_node(msg, editop, newval, newval_marker,
                                  curval, NULL, parent);
             if (undo == NULL) {
                 restore_newnode2(newval, newval_marker);
                 res = ERR_INTERNAL_MEM;
             }
         }
         if (res == NO_ERR) {
             /* move the list entry after the merge is done */
             res = move_mergedlist_node(newval, curval, parent, undo);
         }
     }

     if (res == NO_ERR && newval && curval) {
         if (topreplace ||
             (editop == OP_EDITOP_COMMIT && !typ_is_simple(newval->btyp) &&
              val_get_subtree_dirty_flag(newval))) {
             res = apply_commit_deletes(scb, msg, target, newval, curval);
         }
     }

    /* TBD: should this be moved to the commit phase somehow, although no
     * recovery is possible during a OP_EDITOP_LOAD at boot-time   */
     if (res == NO_ERR && newval && obj_is_root(newval->obj) &&
         editop == OP_EDITOP_LOAD) {
         /* there is no newval_marker for this root node newval
          * so this operation cannot be undone   */
         val_remove_child(newval);
         cfg_apply_load_root(target, newval);
     }

     return res;

 }  /* apply_write_val */


 /********************************************************************
 * FUNCTION check_withdef_default
 * 
 * Check the wd:default node for correctness
 *
 * INPUTS:
 *   scb == session control block
 *   msg == incoming rpc_msg_t in progress
 *   newval == val_value_t from the PDU
 *   
 * RETURNS:
 *   status
 *********************************************************************/
 static status_t
     check_withdef_default (ses_cb_t  *scb,
                            rpc_msg_t  *msg,
                            val_value_t  *newval)
 {
     status_t  res = NO_ERR;

     if (newval == NULL) {
         return NO_ERR;
     }

     /* check object is a leaf */
     if (newval->obj->objtype != OBJ_TYP_LEAF) {
         res = ERR_NCX_WRONG_NODETYP;
     } else {
         /* check leaf has a schema default */
         const xmlChar *defstr = obj_get_default(newval->obj);
         if (defstr == NULL) {
             res = ERR_NCX_NO_DEFAULT;
         } else {
             /* check value provided is the same as the default */
             int32 ret = val_compare_to_string(newval, defstr, &res);
             if (res == NO_ERR && ret != 0) {
                 res = ERR_NCX_INVALID_VALUE;
             }
         }
         }

    if (res != NO_ERR) {
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_CONTENT, res, NULL, 
                         NCX_NT_VAL, newval, NCX_NT_VAL, newval);
    }

    return res;

}  /* check_withdef_default */


/********************************************************************
 * Invoke the specified agent simple type validate callback.
 *
 * \param editop parent node edit operation
 * \param scb session control block
 * \param msg incoming rpc_msg_t in progress
 * \param newval val_value_t from the PDU, this cannot be null!
 * \param curval current value (if any) from the target config
 * \param curparent current parent value for inserting children
 *   
 * \return status
 *********************************************************************/
static status_t invoke_simval_cb_validate( op_editop_t editop,
                                           ses_cb_t  *scb,
                                           rpc_msg_t  *msg,
                                           val_value_t  *newval,
                                           val_value_t  *curval,
                                           val_value_t *curparent )
{
    assert ( newval && "newval is NULL!" );

    log_debug4( "\ninvoke_simval:validate: %s:%s start", 
                obj_get_mod_name(newval->obj), newval->name);

    /* check and adjust the operation attribute */
    boolean is_validate = msg->rpc_txcb->is_validate;
    ncx_iqual_t iqual = val_get_iqualval(newval);
    op_editop_t cur_editop = OP_EDITOP_NONE;
    op_editop_t cvtop = cvt_editop(editop, newval, curval);
    status_t res = agt_check_editop( cvtop, &cur_editop, newval, curval, 
                                     iqual, ses_get_protocol(scb) );
    if (cur_editop == OP_EDITOP_NONE) {
        cur_editop = editop;
    }
    if (editop != OP_EDITOP_COMMIT && !is_validate) {
        newval->editop = cur_editop;
    }

    if (res == ERR_NCX_BAD_ATTRIBUTE) {
        xml_attr_t             errattr;
        memset(&errattr, 0x0, sizeof(xml_attr_t));
        errattr.attr_ns = xmlns_nc_id();
        errattr.attr_name = NC_OPERATION_ATTR_NAME;
        errattr.attr_val = (xmlChar *)NULL;

        agt_record_attr_error( scb, &msg->mhdr, NCX_LAYER_CONTENT, res, 
                               &errattr, NULL, NULL, NCX_NT_VAL, newval );
        return res;
    } else if (res != NO_ERR) {
        agt_record_error( scb, &msg->mhdr, NCX_LAYER_CONTENT, res, NULL,
                          NCX_NT_NONE, NULL, NCX_NT_VAL, newval );
        return res;
    }

    /* check the operation against the object definition and whether or not 
     * the entry currently exists */
    res = agt_check_max_access(newval, (curval != NULL));
    if ( res != NO_ERR ) {
        agt_record_error( scb, &msg->mhdr, NCX_LAYER_CONTENT, res, NULL, 
                          NCX_NT_VAL, newval, NCX_NT_VAL, newval );
        return res;
    }

    /* make sure the node is not partial locked by another session; there is a 
     * corner case where all the PDU nodes are OP_EDITOP_NONE, and so nodes 
     * that touch a partial lock will never actually request an operation; treat
     * this as an error anyway, since it is too hard to defer the test until 
     * later, and this is a useless corner-case so clients should not do it */
    if (curval) {
        uint32   lockid = 0;
        res = val_write_ok( curval, cur_editop, SES_MY_SID(scb),
                            FALSE, &lockid );
        if (res != NO_ERR) {
            agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res, NULL, 
                              NCX_NT_UINT32_PTR, &lockid, NCX_NT_VAL, curval );
            return res;
        }
    }

    res = check_insert_attr(scb, msg, newval, curparent);
    if (res != NO_ERR) {
        /* error already recorded */
        return res;
    }

    /* check if the wd:default attribute is present and if so, that it is used 
     * properly; this needs to be done after the effective operation is set for 
     * newval */
    if ( val_has_withdef_default(newval) ) {
         res = check_withdef_default( scb, msg, newval );
         if (res != NO_ERR) {
              return res;
         }
    }

    /* check the user callback only if there is some operation in 
     * affect already */
    if ( agt_apply_this_node(cur_editop, newval, curval)) {
        res = handle_user_callback( AGT_CB_VALIDATE, cur_editop,  scb, msg,
                                    newval, curval, TRUE, FALSE );
    }
    return res;
}


/********************************************************************
 * Invoke the specified agent simple type apply / commit_check callback.
 *
 * \param scb session control block
 * \param msg incoming rpc_msg_t in progress
 * \param target cfg_template_t for the config database to write
 * \param newval val_value_t from the PDU
 * \param curval current value (if any) from the target config
 *   
 * \return status
 *********************************************************************/
static status_t 
    invoke_simval_cb_apply_commit_check( op_editop_t editop,
                                         ses_cb_t *scb,
                                         rpc_msg_t *msg,
                                         cfg_template_t *target,
                                         val_value_t *newval,
                                         val_value_t *curval,
                                         val_value_t *curparent )

{
    op_editop_t cureditop = editop;
    boolean done = false;

    if (newval && newval->editop != OP_EDITOP_NONE) {
        cureditop = newval->editop;
    } else if (curval) {
        if (cureditop == OP_EDITOP_NONE) {
            cureditop = curval->editop;
        }
    }

    return apply_write_val( cureditop, scb, msg, target, curparent, 
                            newval, curval, &done );
}


/********************************************************************
 * Invoke all the specified agent simple type callbacks for a 
 * source and target and write operation
 *
 * \param cbtyp callback type being invoked
 * \param editop parent node edit operation
 * \param scb session control block
 * \param msg incoming rpc_msg_t in progress
 * \param target cfg_template_t for the config database to write
 * \param newval val_value_t from the PDU
 * \param curval current value (if any) from the target config
 * \param curparent current parent node for inserting children
 *   
 * \return status
 *********************************************************************/
static status_t
    invoke_simval_cb (agt_cbtyp_t cbtyp,
                      op_editop_t editop,
                      ses_cb_t  *scb,
                      rpc_msg_t  *msg,
                      cfg_template_t *target,                      
                      val_value_t  *newval,
                      val_value_t  *curval,
                      val_value_t *curparent)
{
    status_t res = NO_ERR;
    
    /* check the 'operation' attribute in VALIDATE phase */
    switch (cbtyp) {
    case AGT_CB_VALIDATE:
        res = invoke_simval_cb_validate( editop, scb, msg, newval, curval,
                                         curparent );
        break;

    case AGT_CB_APPLY:
        res = invoke_simval_cb_apply_commit_check( editop, scb, msg, target,
                                                   newval, curval, curparent );
        break;

    case AGT_CB_COMMIT:
    case AGT_CB_ROLLBACK:
    default:
        /* nothing to do for commit or rollback at this time */
        break;
    }

    return res;

}  /* invoke_simval_cb */


/********************************************************************
* FUNCTION check_keys_present
* 
* Check that the specified list entry has all key leafs present
* Generate and record <rpc-error>s for each missing key
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   newval == val_value_t from the PDU
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_keys_present (ses_cb_t  *scb,
                        rpc_msg_t  *msg,
                        val_value_t  *newval)
{
    obj_key_t        *objkey = NULL;
    status_t          res = NO_ERR;

    if (newval == NULL) {
        return NO_ERR;
    }
    if (newval->obj->objtype != OBJ_TYP_LIST) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    for (objkey = obj_first_key(newval->obj);
         objkey != NULL;
         objkey = obj_next_key(objkey)) {

        if (val_find_child(newval, obj_get_mod_name(objkey->keyobj),
                           obj_get_name(objkey->keyobj))) {
            continue;
        }

        res = ERR_NCX_MISSING_KEY;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_CONTENT, 
                         res,
                         NULL, 
                         NCX_NT_OBJ, 
                         objkey->keyobj, /* error-info */
                         NCX_NT_VAL, 
                         newval);  /* error-path */
    }

    return res;

} /* check_keys_present */


/********************************************************************
* FUNCTION invoke_cpxval_cb
* 
* Invoke all the specified agent complex type callbacks for a 
* source and target and write operation
*
* INPUTS:
*   cbtyp == callback type being invoked
*   editop == parent node edit operation
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*   newval == val_value_t from the PDU
*   curval == current value (if any) from the target config
*   curparent == current parent node for adding children
*   done   == TRUE if the node manipulation is done
*          == FALSE if none of the parent nodes have already
*             edited the config nodes; just do user callbacks
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    invoke_cpxval_cb (agt_cbtyp_t cbtyp,
                      op_editop_t  editop,
                      ses_cb_t  *scb,
                      rpc_msg_t  *msg,
                      cfg_template_t *target,
                      val_value_t  *newval,
                      val_value_t  *curval,
                      val_value_t  *curparent,
                      boolean done)
{
    status_t          res = NO_ERR, retres = NO_ERR;
    op_editop_t       cur_editop = OP_EDITOP_NONE, cvtop = OP_EDITOP_NONE;
    boolean           isroot = FALSE, isdirty = TRUE;
    
    /* check the 'operation' attribute in VALIDATE phase */
    switch (cbtyp) {
    case AGT_CB_VALIDATE:
        assert ( newval && "newval is NULL!" );

        isroot = obj_is_root(newval->obj);

        if (LOGDEBUG4 && !isroot) {
            log_debug4("\ninvoke_cpxval:validate: %s:%s start", 
                       obj_get_mod_name(newval->obj),
                       newval->name);
        }

        if (editop == OP_EDITOP_COMMIT) {
            isdirty = val_get_dirty_flag(newval);
        }
        if (!isroot && isdirty) {
            /* check and adjust the operation attribute */
            ncx_iqual_t iqual = val_get_iqualval(newval);

            cvtop = cvt_editop(editop, newval, curval);

            res = agt_check_editop(cvtop, &cur_editop, newval, curval, 
                                   iqual, ses_get_protocol(scb));

            if (editop != OP_EDITOP_COMMIT ) {
                /* need to check the max-access but not over-write
                 * the newval->editop if this is a validate operation
                 * do not care about parent operation in agt_check_max_access
                 * because the operation is expected to be OP_EDITOP_LOAD
                 */
                boolean is_validate = msg->rpc_txcb->is_validate;
                op_editop_t saveop = editop;
                newval->editop = cur_editop;
                if (res == NO_ERR) {
                    res = agt_check_max_access(newval, (curval != NULL));
                }
                if (is_validate) {
                    newval->editop = saveop;
                }
            }
        }

        if (res == NO_ERR) {
            if (!isroot && editop != OP_EDITOP_COMMIT &&
                cur_editop != OP_EDITOP_LOAD) {
                res = check_insert_attr(scb, msg, newval, curparent);
                CHK_EXIT(res, retres);
                res = NO_ERR;   /* any error already recorded */
            }
        }

        if (res != NO_ERR) {
            agt_record_error(scb, &msg->mhdr, NCX_LAYER_CONTENT, res, 
                             NULL, NCX_NT_VAL, newval, NCX_NT_VAL, newval);
            CHK_EXIT(res, retres);
        }

        if (res == NO_ERR && !isroot && newval->btyp == NCX_BT_LIST) {
            /* make sure all the keys are present since this
             * was not done in agt_val_parse.c
             */
            if (editop == OP_EDITOP_NONE && !agt_any_operations_set(newval)) {
                if (LOGDEBUG3) {
                    log_debug3("\nSkipping key_check for '%s'; no edit ops",
                               newval->name);
                }
            } else {
                res = check_keys_present(scb, msg, newval);
                /* errors recorded if any */
            }
        }

        /* check if the wd:default attribute is present and if so,
         * it will be an error
         */
        if (res == NO_ERR && !isroot && editop != OP_EDITOP_COMMIT) {
            if (val_has_withdef_default(newval)) {
                res = check_withdef_default(scb, msg, newval);
            }
        }

        /* any of the errors so far indicate this node is bad
         * setting the val->res to the error will force it to be removed
         * if --startup-error=continue; only apply to the new tree, if any
         */
        if (res != NO_ERR && newval != NULL) {
            VAL_MARK_DELETED(newval);
            newval->res = res;
        }

        /* make sure the node is not partial locked
         * by another session; there is a corner case where
         * all the PDU nodes are OP_EDITOP_NONE, and
         * so nodes that touch a partial lock will never
         * actually request an operation; treat this
         * as an error anyway, since it is too hard
         * to defer the test until later, and this is
         * a useless corner-case so clients should not do it
         */
        if (res == NO_ERR && !isroot) {
            val_value_t *useval = newval ? newval : curval;
            uint32 lockid = 0;
            if (obj_is_root(useval->obj) && cur_editop == OP_EDITOP_NONE) {
                /* do not check OP=none on the config root */
                ;
            } else if (editop == OP_EDITOP_COMMIT && !isdirty) {
                ;  // no write requested on this node
            } else {
                res = val_write_ok(useval, cur_editop, SES_MY_SID(scb),
                                   FALSE, &lockid);
            }
            if (res != NO_ERR) {
                agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, 
                                 NULL, NCX_NT_UINT32_PTR, &lockid, NCX_NT_VAL, 
                                 useval);
            }
        }

        /* check the user callback only if there is some
         * operation in affect already
         */
        if (res == NO_ERR && !isroot && isdirty &&
            agt_apply_this_node(cur_editop, newval, curval)) {
            res = handle_user_callback(AGT_CB_VALIDATE, cur_editop, scb,
                                       msg, newval, curval, TRUE, FALSE);
        }
        if (res != NO_ERR) {
            retres = res;
        }
        break;

    case AGT_CB_APPLY:
        if (newval) {
            cur_editop = newval->editop;
            if (cur_editop == OP_EDITOP_NONE) {
                cur_editop = editop;
            }
            isroot = obj_is_root(newval->obj);
        } else if (curval) {
            isroot = obj_is_root(curval->obj);
            cur_editop = editop;
        } else {
            cur_editop = editop;
        }

        res = apply_write_val(cur_editop, scb, msg, target,
                              curparent, newval, curval, &done);
        if (res != NO_ERR) {
            retres = res;
        }
        break;
    case AGT_CB_COMMIT:
    case AGT_CB_ROLLBACK:
        break;
    default:
        retres = SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* change the curparent parameter to the current nodes
     * before invoking callbacks for the child nodes  */
    if (retres == NO_ERR) {
        if (newval) {
            curparent = newval;
        } else if (curval) {
            curparent = curval;
        } else {
            retres = SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    /* check all the child nodes next */
    if (retres == NO_ERR && !done && curparent) {
        val_value_t      *chval, *curch, *nextch;
        for (chval = val_get_first_child(curparent);
             chval != NULL && retres == NO_ERR;
             chval = nextch) {

            nextch = val_get_next_child(chval);

            if (curval) {
                curch = val_first_child_match(curval, chval);
            } else {
                curch = NULL;
            }

            cur_editop = chval->editop;
            if (cur_editop == OP_EDITOP_NONE) {
                cur_editop = editop;
            }
            res = invoke_btype_cb(cbtyp, cur_editop, scb, msg, target, 
                                  chval, curch, curval);
            //if (chval->res == NO_ERR) {
            //    chval->res = res;
            //}
            CHK_EXIT(res, retres);
        }
    }

    return retres;

}  /* invoke_cpxval_cb */


/********************************************************************
* FUNCTION invoke_btype_cb
* 
* Invoke all the specified agent typedef callbacks for a 
* source and target and write operation
*
* INPUTS:
*   cbtyp == callback type being invoked
*   editop == parent node edit operation
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*   newval == val_value_t from the PDU
*   curval == current value (if any) from the target config
*   curparent == current parent for inserting child nodes
* RETURNS:
*   status
*********************************************************************/
static status_t invoke_btype_cb( agt_cbtyp_t cbtyp,
                                 op_editop_t editop,
                                 ses_cb_t  *scb,
                                 rpc_msg_t  *msg,
                                 cfg_template_t *target,
                                 val_value_t  *newval,
                                 val_value_t  *curval,
                                 val_value_t *curparent )
{
    val_value_t *useval;

    if (newval != NULL) {
        useval = newval;
    } else if (curval != NULL) {
        useval = curval;
    } else {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    boolean is_root = obj_is_root(useval->obj);

    if (editop == OP_EDITOP_COMMIT && !is_root && !val_dirty_subtree(useval)) {
        log_debug2("\nSkipping unaffected %s in %s commit for %s:%s",
                   obj_get_typestr(useval->obj), 
                   agt_cbtype_name(cbtyp),
                   val_get_mod_name(useval), 
                   useval->name);
        return NO_ERR;
    }
    
    val_value_t *v_val = NULL;
    status_t res = NO_ERR;
    boolean has_children = !typ_is_simple(useval->btyp);

    if (cbtyp == AGT_CB_VALIDATE) {
        boolean check_acm = TRUE;
        op_editop_t cvtop = cvt_editop(editop, newval, curval);

        /* check if ACM test needs to be skipped */
        if (is_root) {
            check_acm = FALSE;
        } else if (editop == OP_EDITOP_COMMIT) {
            switch (cvtop) {
            case OP_EDITOP_CREATE:
            case OP_EDITOP_DELETE:
                break;
            case OP_EDITOP_MERGE:
            case OP_EDITOP_REPLACE:
                if (!val_get_dirty_flag(useval)) {
                    check_acm = FALSE;
                }
                break;
            default:
                return SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        /* check if allowed access to this node */
        if (check_acm && scb &&
            !agt_acm_val_write_allowed(&msg->mhdr, scb->username, 
                                       newval, curval, cvtop)) {
            res = ERR_NCX_ACCESS_DENIED;
            agt_record_error(scb,&msg->mhdr, NCX_LAYER_OPERATION, res,
                             NULL, NCX_NT_NONE, NULL, NCX_NT_NONE, NULL);
            return res;
        }

        if (curval && val_is_virtual(curval)) {
            v_val = val_get_virtual_value(scb, curval, &res);

            if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            } else if (res != NO_ERR) {
                return res;
            }
        }

        /* check if there are commit deletes to validate */
        if (has_children && 
            msg->rpc_txcb->edit_type == AGT_CFG_EDIT_TYPE_FULL) {
            res = check_commit_deletes(scb, msg, newval, 
                                       v_val ? v_val : curval);
            if (res != NO_ERR) {
                return res;
            }
        }

    } else if (cbtyp == AGT_CB_APPLY && editop == OP_EDITOP_COMMIT &&
               has_children) {

        if (curval && val_is_virtual(curval)) {
            v_val = val_get_virtual_value(scb, curval, &res);

            if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            } else if (res != NO_ERR) {
                return res;
            }
        }
    }

    /* first traverse all the nodes until leaf nodes are reached */
    if (has_children) {
        res = invoke_cpxval_cb(cbtyp,  editop, scb, msg, target, newval, 
                               (v_val) ? v_val : curval, curparent, FALSE);
    } else {
        res = invoke_simval_cb(cbtyp, editop, scb, msg, target, newval, 
                               (v_val) ? v_val : curval, curparent );
        if (newval != NULL && res != NO_ERR) {
            VAL_MARK_DELETED(newval);
            newval->res = res;
        }
    }

    return res;

}  /* invoke_btype_cb */


/********************************************************************
* FUNCTION commit_edit   AGT_CB_COMMIT
* 
* Finish the database editing for an undo-rec
* The SIL callbacks have already completed; this step cannot fail
*
* INPUTS:
*   scb == session control block to use
*   msg == RPC message in progress
*   undo == agt_cfg_undo_rec_t struct to process
*
*********************************************************************/
static void
    commit_edit (ses_cb_t *scb,
                 rpc_msg_t *msg,
                 agt_cfg_undo_rec_t *undo)
{
    if (undo->editop == OP_EDITOP_LOAD) {
        /* the top-level nodes are kept in the original tree the very
         * first time the running config is loaded from startup  */
        undo->newnode = NULL;
        return;
    }

    handle_audit_record(undo->editop, scb, msg, undo->newnode, undo->curnode);

    /* check if the newval marker was placed in the source tree */
    if (undo->newnode_marker) {
        dlq_remove(undo->newnode_marker);
        val_free_value(undo->newnode_marker);
        undo->newnode_marker = NULL;
    }

    /* check if the curval marker was placed in the target tree */
    if (undo->curnode_marker) {
        dlq_remove(undo->curnode_marker);
        val_free_value(undo->curnode_marker);
        undo->curnode_marker = NULL;
    }

    if (msg->rpc_txcb->cfg_id == NCX_CFGID_RUNNING) {
        switch (undo->editop) {
        case OP_EDITOP_REPLACE:
        case OP_EDITOP_COMMIT:
            val_check_swap_resnode(undo->curnode, undo->newnode);
            break;
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            if (undo->curnode) {
                val_check_delete_resnode(undo->curnode);
            }
            break;
        default:
            ;
        }
    }

    if (undo->newnode) {
        if (msg->rpc_txcb->cfg_id == NCX_CFGID_RUNNING) {
            val_clear_dirty_flag(undo->newnode);
        } else {
            val_set_dirty_flag(undo->newnode);
        }
    }

    if (undo->curnode) {
        if (msg->rpc_txcb->cfg_id == NCX_CFGID_RUNNING) {
            val_clear_dirty_flag(undo->curnode);
        } else {
            val_set_dirty_flag(undo->curnode);
        }
        if (undo->free_curnode) {
            if (VAL_IS_DELETED(undo->curnode)) {
                val_remove_child(undo->curnode);
            }
            if (undo->curnode_clone) {
                /* do not need curnode as a backup for redo-restore */
                val_free_value(undo->curnode);
                undo->curnode = NULL;
            }
        }
    }

    clean_node(undo->newnode);
    clean_node(undo->curnode);

    finish_extra_deletes(msg->rpc_txcb->cfg_id, undo);

    /* rest of cleanup will be done in agt_cfg_free_undorec;
     * Saving curnode_clone or curnode and the entire extra_deleteQ
     * in case this commit needs to be undone by simulating a
     * new transaction wrt/ SIL callbacks   */

}  /* commit_edit */


/********************************************************************
* FUNCTION reverse_edit
* 
* Attempt to invoke SIL callbacks to reverse an edit that
* was already commited; database tree was never undone
* The apply and commit phases completed OK
*
* INPUTS:
*   undo == agt_cfg_undo_rec_t struct to process
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    reverse_edit (agt_cfg_undo_rec_t *undo,
                  ses_cb_t  *scb,
                  rpc_msg_t  *msg)
{
    agt_cfg_transaction_t *txcb = msg->rpc_txcb;
    if (LOGDEBUG3) {
        val_value_t *logval = undo->newnode ? undo->newnode : undo->curnode;
        log_debug3("\nReverse transaction %llu, %s edit on %s:%s",
                   (unsigned long long)txcb->txid,
                   op_editop_name(undo->editop),
                   val_get_mod_name(logval),
                   logval->name);
    }

    val_value_t *newval = NULL;
    val_value_t *curval = NULL;
    op_editop_t editop = OP_EDITOP_NONE;
    status_t res = NO_ERR;
    boolean lookparent = TRUE;
    boolean indelete = FALSE;
    
    switch (undo->edit_action) {
    case AGT_CFG_EDIT_ACTION_NONE:
        log_warn("\nError: undo record has no action set");
        return NO_ERR;

    case AGT_CFG_EDIT_ACTION_ADD:
        /* need to generate a delete for curnode */
        assert( undo->newnode && "undo newnode is NULL!" );
        curval = undo->newnode;
        editop = OP_EDITOP_DELETE;
        indelete = TRUE;
        break;

    case AGT_CFG_EDIT_ACTION_SET:
        /* need to generate a set for the old value */
        assert( undo->newnode && "undo newnode is NULL!" );
        curval = undo->newnode;
        newval = (undo->curnode_clone) ? undo->curnode_clone : undo->curnode;
        assert( newval && "undo curnode is NULL!" );        
        editop = undo->editop;
        break;

    case AGT_CFG_EDIT_ACTION_MOVE:
        /* need to generate a similar operation to move the old entry
         * back to its old location   
         * !!! FIXME: are the editvars correct, and do SIL callbacks
         * !!! look at the editvars?    */
        assert( undo->newnode && "undo newnode is NULL!" );
        assert( undo->curnode && "undo curnode is NULL!" );
        newval = undo->curnode;
        curval = undo->newnode;
        editop = undo->editop;
        break;

    case AGT_CFG_EDIT_ACTION_REPLACE:
        /* need to geerate a replace with the values revered */
        assert( undo->newnode && "undo newnode is NULL!" );
        assert( undo->curnode && "undo curnode is NULL!" );
        newval = undo->curnode;
        curval = undo->newnode;
        editop = undo->editop;
        break;

    case AGT_CFG_EDIT_ACTION_DELETE:
        /* need to generate a create for the deleted value */
        assert( undo->curnode && "undo curnode is NULL!" );        
        newval = undo->curnode;
        editop = OP_EDITOP_CREATE;
        break;

    case AGT_CFG_EDIT_ACTION_DELETE_DEFAULT:
        if (undo->curnode) {
            newval = undo->curnode;
            editop = OP_EDITOP_CREATE;
        } else {
            return NO_ERR;
        }
        break;

    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    res = handle_user_callback(AGT_CB_APPLY, editop, scb, msg, 
                               newval, curval, lookparent, indelete);

    if (res == NO_ERR) {
        res = handle_user_callback(AGT_CB_COMMIT, editop, scb, msg, 
                                   newval, curval, lookparent, indelete);
    }

    if (res == NO_ERR) {
        /* reverse all the extra deletes as create operations */
        editop = OP_EDITOP_CREATE;
        curval = NULL;
        lookparent = FALSE;
        indelete = FALSE;

        agt_cfg_nodeptr_t *nodeptr = (agt_cfg_nodeptr_t *)
            dlq_firstEntry(&undo->extra_deleteQ);
        for (; nodeptr && res == NO_ERR; 
             nodeptr = (agt_cfg_nodeptr_t *)dlq_nextEntry(nodeptr)) {
            if (nodeptr && nodeptr->node) {
                newval = nodeptr->node;
                res = handle_user_callback(AGT_CB_APPLY, editop, scb, msg, 
                                           newval, curval, lookparent, 
                                           indelete);
                if (res == NO_ERR) {
                    res = handle_user_callback(AGT_CB_COMMIT, editop, scb, msg, 
                                               newval, curval, lookparent, 
                                               indelete);
                }
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }

    if (res != NO_ERR) {
        log_error("\nError: SIL rejected reverse edit (%s)",
                  get_error_string(res));
    }

    return res;

} /* reverse_edit */


/********************************************************************
* FUNCTION rollback_edit
* 
* Attempt to rollback an edit 
*
* INPUTS:
*   undo == agt_cfg_undo_rec_t struct to process
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    rollback_edit (agt_cfg_undo_rec_t *undo,
                   ses_cb_t  *scb,
                   rpc_msg_t  *msg,
                   cfg_template_t *target)
{
    val_value_t  *curval = NULL;
    status_t res = NO_ERR;
    ncx_cfg_t cfgid = msg->rpc_txcb->cfg_id;

    if (LOGDEBUG2) {
        val_value_t *logval = undo->newnode ? undo->newnode : undo->curnode;
        log_debug2("\nRollback transaction %llu, %s edit on %s:%s",
                   (unsigned long long)msg->rpc_txcb->txid,
                   op_editop_name(undo->editop),
                   val_get_mod_name(logval),
                   logval->name);
    }

    if (undo->editop == OP_EDITOP_LOAD) {
        log_debug3("\nSkipping rollback edit of LOAD operation");
        undo->newnode = NULL;
        return NO_ERR;
    }

    if (undo->curnode_clone != NULL) {
        curval = undo->curnode_clone;
    } else {
        curval = undo->curnode;
    }

    switch (undo->commit_res) {
    case NO_ERR:
        /* need a redo edit since the SIL already completed the commit */
        res = reverse_edit(undo, scb, msg);
        break;

    case ERR_NCX_SKIPPED:
        /* need a simple undo since the commit fn was never called
         * first call the SIL with the ROLLBACK request
         */
        if (cfgid == NCX_CFGID_RUNNING) {
            undo->rollback_res = handle_user_callback(AGT_CB_ROLLBACK, 
                                                      undo->editop,
                                                      scb, msg, undo->newnode, 
                                                      curval, TRUE, FALSE);
            if (undo->rollback_res != NO_ERR) {
                val_value_t *logval = undo->newnode ? undo->newnode : curval;
                log_warn("\nWarning: SIL rollback for %s:%s in transaction "
                         "%llu failed (%s)",
                         (unsigned long long)msg->rpc_txcb->txid,
                         val_get_mod_name(logval), 
                         logval->name,
                         get_error_string(undo->rollback_res));
                res = undo->rollback_res;
                /* !!! not sure what the SIL callback thinks the resulting data
                 * is going to look like; return to original state anyway */
            }
        }
        break;

    default:
        /* this is the node that failed during the COMMIT callback
         * assuming this SIL does not need any rollback since the
         * failure indicates the commit was not applied;
         * TBD: need to flag SIL partial commit errors and what
         * to do about it                  */
        ;
    }

    /* if the source is from a PDU, then it will get cleanup up when
     * this <rpc> is done.  If it is from <candidate> config then
     * preserve the edit in candidate since it is still a pending edit
     * in case commit is attempted again   */
    //clean_node(undo->newnode);
    //val_clear_dirty_flag(undo->newnode);
    //clean_node(undo->curnode);
    //val_clear_dirty_flag(undo->curnode);

    /* undo the specific operation
     * even if the SIL commit callback returned an error
     * the commit_edit function was never called so this
     * edit needs to be cleaned up       */
    switch (undo->editop) {
    case OP_EDITOP_CREATE:
        /* delete the node from the tree; if there was a default
         * leaf before then restore curval as well   */
        val_remove_child(undo->newnode);
        restore_newnode(undo);
        restore_curnode(undo, target);
        break;
    case OP_EDITOP_DELETE:
    case OP_EDITOP_REMOVE:
        /* no need to restore newnode */
        restore_curnode(undo, target);
        break;
    case OP_EDITOP_MERGE:
    case OP_EDITOP_REPLACE:
        /* check if the old node needs to be swapped back
         * or if the new node is just removed
         */
        restore_curnode(undo, target);

        /* undo newval that was a new or merged child node */
        if (undo->newnode_marker) {
            val_remove_child(undo->newnode);
        }
        restore_newnode(undo);
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    restore_extra_deletes(&undo->extra_deleteQ);

    return res;

}  /* rollback_edit */


/********************************************************************
* FUNCTION attempt_commit
* 
* Attempt complete commit by asking all SIL COMMIT callbacks
* to complete the requested edit
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*
* OUTPUTS:
*   if errors are encountered (such as undo-failed) then they
*   will be added to the msg->mhdr.errQ
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    attempt_commit (ses_cb_t *scb,
                    rpc_msg_t *msg,
                    cfg_template_t *target)
{
    agt_cfg_undo_rec_t  *undo = NULL;
    agt_cfg_transaction_t *txcb = msg->rpc_txcb;

    if (LOGDEBUG) {
        uint32 editcnt = dlq_count(&txcb->undoQ);
        const xmlChar *cntstr = NULL;
        switch (editcnt) {
        case 0:
            break;
        case 1:
            cntstr = (const xmlChar *)"edit";
            break;
        default:
            cntstr = (const xmlChar *)"edits";
        }
        if (editcnt) {
            log_debug("\nStart full commit of transaction %llu: %d"
                      " %s on %s config", txcb->txid,
                      editcnt, cntstr, target->name);
        } else {
            log_debug("\nStart full commit of transaction %llu:"
                      " LOAD operation on %s config",
                      txcb->txid, target->name);
        }
    }

    /* first make sure all SIL callbacks accept the commit */
    if (target->cfg_id == NCX_CFGID_RUNNING) {
        undo = (agt_cfg_undo_rec_t *)dlq_firstEntry(&txcb->undoQ);
        for (; undo != NULL; undo = (agt_cfg_undo_rec_t *)dlq_nextEntry(undo)) {

            if (undo->curnode && undo->curnode->parent == NULL) {
                /* node was actually removed in delete operation */
                undo->curnode->parent = undo->parentnode;
            }

            undo->commit_res = 
                handle_user_callback(AGT_CB_COMMIT, undo->editop, scb, msg, 
                                     undo->newnode, undo->curnode, TRUE, FALSE);
            if (undo->commit_res != NO_ERR) {
                val_value_t *logval = undo->newnode ? undo->newnode : 
                    undo->curnode;
                if (LOGDEBUG) {
                    log_debug("\nHalting commit %llu: %s:%s SIL returned error"
                              " (%s)",
                              (unsigned long long)txcb->txid,
                              val_get_mod_name(logval), 
                              logval->name,
                              get_error_string(undo->commit_res));
                }
                return undo->commit_res;
            }
        }
    }

    /* all SIL commit callbacks accepted and finalized the commit
     * now go through and finalize the edit; this step should not fail 
     * first, finish deleting any false when-stmt nodes then commit edits */
    while (!dlq_empty(&txcb->deadnodeQ)) {
        agt_cfg_nodeptr_t *nodeptr = (agt_cfg_nodeptr_t *)
            dlq_deque(&txcb->deadnodeQ);
        if (nodeptr && nodeptr->node) {
            /* mark ancestor nodes dirty before deleting this node */
            val_remove_child(nodeptr->node);
            val_free_value(nodeptr->node);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        agt_cfg_free_nodeptr(nodeptr);
    }
    undo = (agt_cfg_undo_rec_t *)dlq_firstEntry(&txcb->undoQ);
    for (; undo != NULL; undo = (agt_cfg_undo_rec_t *)dlq_nextEntry(undo)) {
        commit_edit(scb, msg, undo);
    }

    /* update the last change time
     * this will only apply here to candidate or running
     */
    cfg_update_last_ch_time(target);
    cfg_update_last_txid(target, txcb->txid);
    cfg_set_dirty_flag(target);

    agt_profile_t *profile = agt_get_profile();
    profile->agt_config_state = AGT_CFG_STATE_OK;

    if (LOGDEBUG) {
        log_debug("\nComplete commit OK of transaction %llu"
                  " on %s database",  (unsigned long long)txcb->txid,
                  target->name);
    }

    return NO_ERR;

}  /* attempt_commit */


/********************************************************************
* FUNCTION attempt_rollback
* 
* Attempt to rollback a transaction attempt
*  if commit not tried:
*   All edits have a status of NCX_ERR_SKIPPED
*  if commit tried:
*   There are N edits that succeeded and commit_res == NO_ERR
*   There is 1 edit that the SIL callback rejected with an error
*   There are M edits with a commit_res of NCX_ERR_SKIPPED
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    attempt_rollback (ses_cb_t  *scb,
                      rpc_msg_t  *msg,
                      cfg_template_t *target)
{

    status_t res = NO_ERR;
    agt_cfg_transaction_t *txcb = msg->rpc_txcb;

    if (LOGDEBUG) {
        uint32 editcnt = dlq_count(&txcb->undoQ);
        const xmlChar *cntstr = NULL;
        switch (editcnt) {
        case 0:
            break;
        case 1:
            cntstr = (const xmlChar *)"edit";
            break;
        default:
            cntstr = (const xmlChar *)"edits";
        }
        if (editcnt) {
            log_debug("\nStart full rollback of transaction %llu: %d"
                      " %s on %s config", txcb->txid,
                      editcnt, cntstr, target->name);
        } else {
            log_debug("\nStart full rollback of transaction %llu:"
                      " LOAD operation on %s config",
                      txcb->txid, target->name);
        }
    }

    restore_extra_deletes(&txcb->deadnodeQ);

    agt_cfg_undo_rec_t  *undo = (agt_cfg_undo_rec_t *)
        dlq_firstEntry(&txcb->undoQ);
    for (; undo != NULL; undo = (agt_cfg_undo_rec_t *)dlq_nextEntry(undo)) {
        /* rollback the edit operation */
        undo->rollback_res = rollback_edit(undo, scb, msg, target);
        if (undo->rollback_res != NO_ERR) {
            res = undo->rollback_res;
        }
    }

    return res;

}  /* attempt_rollback */


/********************************************************************
* FUNCTION handle_callback
* 
* Invoke all the specified agent typedef callbacks for a 
* source and target and write operation
*
* INPUTS:
*   cbtyp == callback type being invoked
*   editop == parent node edit operation
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*   newval == val_value_t from the PDU
*   curval == current value (if any) from the target config
*   curparent == current parent node for inserting children
* RETURNS:
*   status
*********************************************************************/
static status_t
    handle_callback (agt_cbtyp_t cbtyp,
                     op_editop_t editop,
                     ses_cb_t  *scb,
                     rpc_msg_t  *msg,
                     cfg_template_t *target,
                     val_value_t  *newval,
                     val_value_t  *curval,
                     val_value_t *curparent)
{
    assert( msg && "msg is NULL!" );
    assert( msg->rpc_txcb && "txcb is NULL!" );

    agt_cfg_transaction_t *txcb = msg->rpc_txcb;
    status_t res = NO_ERR;

    if (LOGDEBUG2) {
        if (editop == OP_EDITOP_DELETE) {
            log_debug2("\n\n***** start commit_deletes for "
                       "session %d, transaction %llu *****\n",
                       scb ? SES_MY_SID(scb) : 0, 
                       (unsigned long long)txcb->txid);
        } else {
            log_debug2("\n\n***** start %s callback phase for "
                       "session %d, transaction %llu *****\n",
                       agt_cbtype_name(cbtyp), scb ? SES_MY_SID(scb) : 0,
                       (unsigned long long)txcb->txid);
        }
    }

    /* get the node to check */
    if (newval == NULL && curval == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* check if trying to write to a config=false node */
    if (newval != NULL && !val_is_config_data(newval)) {
        res = ERR_NCX_ACCESS_READ_ONLY;
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                         NULL, NCX_NT_NONE, NULL, NCX_NT_VAL, newval);
        return res;
    }

    /* check if trying to delete a config=false node */
    if (newval == NULL && curval != NULL && !val_is_config_data(curval)) {
        res = ERR_NCX_ACCESS_READ_ONLY;
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, NULL, 
                         NCX_NT_NONE, NULL, NCX_NT_VAL, curval);
        return res;
    }

    /* this is a config node so check the operation further */
    switch (cbtyp) {
    case AGT_CB_VALIDATE:
    case AGT_CB_APPLY:
        /* keep checking until all the child nodes have been processed */
        res = invoke_btype_cb(cbtyp, editop, scb, msg, target, newval, curval,
                              curparent);
        if (cbtyp == AGT_CB_APPLY) {
            txcb->apply_res = res;
        }
        break;
    case AGT_CB_COMMIT:
        txcb->commit_res = res = attempt_commit(scb, msg, target);
        if (res != NO_ERR) {
            txcb->rollback_res = attempt_rollback(scb, msg, target);
            if (txcb->rollback_res != NO_ERR) {
                log_warn("\nWarning: Recover rollback of transaction "
                         "%llu failed (%s)",
                         (unsigned long long)txcb->txid,
                         get_error_string(txcb->rollback_res));
             }
        }
        break;
    case AGT_CB_ROLLBACK:
        txcb->rollback_res = res = attempt_rollback(scb, msg, target);
        if (res != NO_ERR) {
            log_warn("\nWarning: Apply rollback of transaction "
                     "%llu failed (%s)",
                     (unsigned long long)txcb->txid,
                     get_error_string(res));
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}  /* handle_callback */


/********************************************************************
* FUNCTION apply_commit_deletes
* 
* Apply the requested commit delete operations
*
* Invoke all the AGT_CB_COMMIT callbacks for a 
* source and target and write operation
*
* INPUTS:
*   scb == session control block
*   msg == incoming commit rpc_msg_t in progress
*   target == target database (NCX_CFGID_RUNNING)
*   candval == value struct from the candidate config
*   runval == value struct from the running config
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhsr.errQ
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    apply_commit_deletes (ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          cfg_template_t *target,
                          val_value_t *candval,
                          val_value_t *runval)
{
    val_value_t      *curval, *nextval;
    status_t          res = NO_ERR;

    if (candval == NULL || runval == NULL) {
        return NO_ERR;
    }

    /* go through running config
     * if the matching node is not in the candidate,
     * then delete that node in the running config as well
     */
    for (curval = val_get_first_child(runval);
         curval != NULL && res == NO_ERR; 
         curval = nextval) {

        nextval = val_get_next_child(curval);

        /* check only database config nodes */
        if (obj_is_data_db(curval->obj) &&
            obj_is_config(curval->obj)) {

            /* check if node deleted in source */
            val_value_t *matchval = val_first_child_match(candval, curval);
            if (!matchval) {
                /* prevent the agt_val code from ignoring this node */
                val_set_dirty_flag(curval);

                /* deleted in the source, so delete in the target */
                res = handle_callback(AGT_CB_APPLY, OP_EDITOP_DELETE, 
                                      scb, msg, target, NULL, curval, runval);
            }
        }  /* else skip non-config database node */
    }

    return res;

}   /* apply_commit_deletes */


/********************************************************************
* FUNCTION compare_unique_testsets
* 
* Compare 2 Qs of val_unique_t structs
*
* INPUTS:
*   uni1Q == Q of val_unique_t structs for value1
*   uni2Q == Q of val_unique_t structs for value2
*
* RETURNS:
*   TRUE if compare test is equal
*   FALSE if compare test not equal
*********************************************************************/
static boolean
    compare_unique_testsets (dlq_hdr_t *uni1Q,
                             dlq_hdr_t *uni2Q)
{
    /* compare the 2 Qs of values */
    val_unique_t *uni1 = (val_unique_t *)dlq_firstEntry(uni1Q);
    val_unique_t *uni2 = (val_unique_t *)dlq_firstEntry(uni2Q);

    while (uni1 && uni2) {
        status_t res = NO_ERR;
        boolean cmpval = xpath1_compare_nodeset_results(uni1->pcb,
                                                        uni1->pcb->result,
                                                        uni2->pcb->result,
                                                        &res);
        if (res != NO_ERR) {
            // log_error()!!
            return FALSE;
        }
        if (!cmpval) {
            return FALSE;
        }
        uni1 = (val_unique_t *)dlq_nextEntry(uni1);
        uni2 = (val_unique_t *)dlq_nextEntry(uni2);
    }
    return TRUE;

} /* compare_unique_testsets */


/********************************************************************
 * FUNCTION new_unique_set
 * Malloc and init a new unique test set
 *
 * \return the malloced data structure
 *********************************************************************/
static unique_set_t * new_unique_set (void)
{
    unique_set_t *uset = m__getObj(unique_set_t);
    if (uset == NULL) {
        return NULL;
    }

    memset(uset, 0x0, sizeof(unique_set_t));
    dlq_createSQue(&uset->uniqueQ);
    return uset;
}  /* new_unique_set */


/********************************************************************
 * FUNCTION free_unique_set
 * Clean and free a unique test set
 *
 * \param uset the data structure to clean and free
 *********************************************************************/
static void free_unique_set (unique_set_t *uset)
{
    if (uset == NULL) {
        return;
    }

    val_unique_t *unival;
    while (!dlq_empty(&uset->uniqueQ)) {
        unival = (val_unique_t *)dlq_deque(&uset->uniqueQ);
        val_free_unique(unival);
    }
    m__free(uset);

}   /* free_unique_set */


/********************************************************************
* FUNCTION make_unique_testset
* 
* Construct a Q of val_unique_t records, each with the
* current value of the corresponding obj_unique_comp_t
* If a node is not present the test will be stopped,
* ERR_NCX_CANCELED will be returned in the status parm,
* Any nodes in the resultQ will be left there
* and need to be deleted
*
* INPUTS:
*   curval == value to run test for
*   unidef == obj_unique_t to process
*   root == XPath docroot to use
*   resultQ == Queue header to store the val_unique_t records
*   freeQ == Queue of free val_unique_t records to check first
*
* OUTPUTS:
*   resultQ has zero or more val_unique_t structs added
*   which need to be freed with the val_free_unique fn
*
* RETURNS:
*   status of the operation, NO_ERR if all nodes found and stored OK
*********************************************************************/
static status_t 
    make_unique_testset (val_value_t *curval,
                         obj_unique_t *unidef,
                         val_value_t *root,
                         dlq_hdr_t *resultQ,
                         dlq_hdr_t *freeQ)
{
    /* need to get a non-const pointer to the module */
    if (curval->obj == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    ncx_module_t       *mod = curval->obj->tkerr.mod;
    status_t            retres = NO_ERR;

    obj_unique_comp_t  *unicomp = obj_first_unique_comp(unidef);
    if (!unicomp) {
        /* really should not happen */
        return ERR_NCX_CANCELED;
    }

    /* for each unique component, get the descendant
     * node that is specifies and save it in a val_unique_t
     */
    while (unicomp && retres == NO_ERR) {
        if (unicomp->isduplicate) {
            unicomp = obj_next_unique_comp(unicomp);
            continue;
        }

        xpath_pcb_t *targpcb = NULL;
        status_t res = xpath_find_val_unique(curval, mod, unicomp->xpath,
                                             root, FALSE, &targpcb);
        if (res != NO_ERR) {
            retres = ERR_NCX_CANCELED;
            xpath_free_pcb(targpcb);
            continue;
        }

        /* skip the entire unique test for this list entry
         * if this is an empty node-set */
        if (xpath_get_first_resnode(targpcb->result) == NULL) {
            retres = ERR_NCX_CANCELED;
            xpath_free_pcb(targpcb);
            continue;
        }

        val_unique_t *unival = (val_unique_t *)dlq_deque(freeQ);
        if (!unival) {
            unival = val_new_unique();
            if (!unival) {
                retres = ERR_INTERNAL_MEM;
                xpath_free_pcb(targpcb);
                continue;
            }
        }

        unival->pcb = targpcb;
        dlq_enque(unival, resultQ);

        unicomp = obj_next_unique_comp(unicomp);
    }

    return retres;

} /* make_unique_testset */


/********************************************************************
* FUNCTION one_unique_stmt_check
* 
* Run the unique-stmt test for the specified list object type
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   ct == commit test record to use
*   root == XPath docroot to use
*   unidef == obj_unique_t to process
*   uninum == ordinal ID for this unique-stmt
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    one_unique_stmt_check (ses_cb_t *scb,
                           xml_msg_hdr_t *msg,
                           agt_cfg_commit_test_t *ct,
                           val_value_t *root,
                           obj_unique_t *unidef,
                           uint32 uninum)
{
    dlq_hdr_t        uniQ, freeQ, usetQ;
    val_unique_t    *unival;
    unique_set_t    *uset;

    assert( ct && "ct is NULL!" );
    assert( ct->result && "result is NULL!" );
    assert( unidef && "unidef is NULL!" );
    assert( unidef->xpath && "xpath is NULL!" );

    status_t retres = NO_ERR;
    dlq_createSQue(&uniQ);   // Q of val_unique_t 
    dlq_createSQue(&freeQ);  // Q of val_unique_t
    dlq_createSQue(&usetQ);  // Q of unique_set_t

    if (LOGDEBUG3) {
        log_debug3("\nunique_chk: %s:%s start unique # %u (%s)", 
                   obj_get_mod_name(ct->obj), obj_get_name(ct->obj),
                   uninum, unidef->xpath);
    }

    /* create a unique test set for each list instance that has
     * complete tuple to test; skip all instances with partial tuples */
    xpath_resnode_t *resnode = xpath_get_first_resnode(ct->result);
    for (; resnode && retres == NO_ERR; 
         resnode = xpath_get_next_resnode(resnode)) {

        val_value_t *valnode = xpath_get_resnode_valptr(resnode);

        status_t res = make_unique_testset(valnode, unidef, root, &uniQ,
                                           &freeQ);
        if (res == NO_ERR) {
            /* this valnode provided a complete tuple so it will be
             * used in the compare test   */
            uset = new_unique_set();
            if (uset == NULL) {
                retres = ERR_INTERNAL_MEM;
                continue;
            }
            dlq_block_enque(&uniQ, &uset->uniqueQ);
            uset->valnode = valnode;
            dlq_enque(uset, &usetQ);
        } else if (res == ERR_NCX_CANCELED) {
            dlq_block_enque(&uniQ, &freeQ);
        } else {
            retres = res;
        }
    }

    /* done with these 2 queues */
    while (!dlq_empty(&uniQ)) {
        unival = (val_unique_t *)dlq_deque(&uniQ);
        val_free_unique(unival);
    }
    while (!dlq_empty(&freeQ)) {
        unival = (val_unique_t *)dlq_deque(&freeQ);
        val_free_unique(unival);
    }

    if (retres == NO_ERR) {
        /* go through all the test sets and compare them to each other
         * this is a brute force compare N to N+1 .. last moving N
         * through the list until all entries have been compared to
         * each other and all unique violations recorded;
         * As compare is done, delete old set1 since no longer needed */
        while (!dlq_empty(&usetQ)) {
            unique_set_t *set1 = (unique_set_t *)dlq_deque(&usetQ);
            if (set1 && set1->valnode->res != ERR_NCX_UNIQUE_TEST_FAILED) {
                unique_set_t *set2 = (unique_set_t *)dlq_firstEntry(&usetQ);
                for (; set2; set2 = (unique_set_t *)dlq_nextEntry(set2)) {
                    if (set2->valnode->res == ERR_NCX_UNIQUE_TEST_FAILED) {
                        // already compared this to rest of list instances
                        // if it is flagged with a unique-test failed error
                        continue;
                    }
                    if (compare_unique_testsets(&set1->uniqueQ, 
                                                &set2->uniqueQ)) {
                        /* 2 lists have the same values so generate an error */
                        agt_record_unique_error(scb, msg, set1->valnode,
                                                &set1->uniqueQ);
                        set1->valnode->res = ERR_NCX_UNIQUE_TEST_FAILED;
                        retres = ERR_NCX_UNIQUE_TEST_FAILED;
                    }
                }
            }
            free_unique_set(set1);
        }
    }

    while (!dlq_empty(&usetQ)) {
        uset = (unique_set_t *)dlq_deque(&usetQ);
        free_unique_set(uset);
    }

    return retres;

} /* one_unique_stmt_check */


/********************************************************************
* FUNCTION unique_stmt_check
* 
* Check for the proper uniqueness of the tuples within
* the set of list instances for the specified node
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   ct == commit test record to use
*   root == XPath docroot to use
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    unique_stmt_check (ses_cb_t *scb,
                       xml_msg_hdr_t *msg,
                       agt_cfg_commit_test_t *ct,
                       val_value_t *root)
{
    obj_unique_t *unidef = obj_first_unique(ct->obj);
    uint32 uninum = 0;
    status_t retres = NO_ERR;

    /* try to catch as many errors as possible, so keep going
     * even if an error recorded already     */
    while (unidef) {
        ++uninum;

        if (unidef->isconfig) {
            status_t res = one_unique_stmt_check(scb, msg, ct, root,
                                                 unidef, uninum);
            CHK_EXIT(res, retres);
        }

        unidef = obj_next_unique(unidef);
    }

    return retres;

}  /* unique_stmt_check */


/********************************************************************
* FUNCTION instance_xpath_check
* 
* Check the leafref or instance-identifier leaf or leaf-list node
* Test 'require-instance' for the NCX_BT_LEAFREF
* and NCX_BT_INSTANCE_ID data types
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   val == value node to check
*   root == config root for 'val'
*   layer == NCX layer calling this function (for error purposes only)
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 

*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    instance_xpath_check (ses_cb_t *scb,
                          xml_msg_hdr_t *msg,
                          val_value_t *val,
                          val_value_t *root,
                          ncx_layer_t layer)
{
    xpath_result_t      *result;
    xpath_pcb_t         *xpcb;
    ncx_errinfo_t       *errinfo;
    typ_def_t           *typdef;
    boolean              constrained, fnresult;
    status_t             res, validateres;

    res = NO_ERR;
    errinfo = NULL;
    typdef = obj_get_typdef(val->obj);

    switch (val->btyp) {
    case NCX_BT_LEAFREF:
        /* do a complete parsing to retrieve the
         * instance that matched; this is always constrained
         * to 1 of the instances that exists at commit-time
         */
        xpcb = typ_get_leafref_pcb(typdef);
        if (!val->xpathpcb) {
            val->xpathpcb = xpath_clone_pcb(xpcb);
            if (!val->xpathpcb) {
                res = ERR_INTERNAL_MEM;
            }
        }

        if (res == NO_ERR) {
            assert( scb && "scb is NULL!" );
            result = xpath1_eval_xmlexpr(scb->reader, val->xpathpcb, 
                                         val, root, FALSE, TRUE, &res);
            if (result && res == NO_ERR) {
                /* check result: the string value in 'val'
                 * must match one of the values in the
                 * result set
                 */
                fnresult = 
                    xpath1_compare_result_to_string(val->xpathpcb, result, 
                                                    VAL_STR(val), &res);

                if (res == NO_ERR && !fnresult) {
                    /* did not match any of the 
                     * current instances  (13.5)
                     */
                    res = ERR_NCX_MISSING_VAL_INST;
                }
            }
            xpath_free_result(result);
        }

        if (res != NO_ERR) {
            val->res = res;
            agt_record_error_errinfo(scb, msg, layer, res, NULL, NCX_NT_OBJ,
                                     val->obj, NCX_NT_VAL, val, errinfo);
        }
        break;
    case NCX_BT_INSTANCE_ID:
        /* do a complete parsing to retrieve the
         * instance that matched, just checking the
         * require-instance flag
         */
        result = NULL;
        constrained = typ_get_constrained(typdef);

        validateres = val->xpathpcb->validateres;
        if (validateres == NO_ERR) {
            assert( scb && "scb is NULL!" );
            result = xpath1_eval_xmlexpr(scb->reader, val->xpathpcb,
                                         val, root, FALSE, FALSE, &res);
            if (result) {
                xpath_free_result(result);
            }

            if (!constrained) {
                if (!NEED_EXIT(res)) {
                    res = NO_ERR;
                }
            }

            if (res != NO_ERR) {
                val->res = res;
                agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ,
                                 val->obj, NCX_NT_VAL, val);
            }
        }
        break;
    default:
        ;
    }

    return res;
    
}  /* instance_xpath_check */


/********************************************************************
* FUNCTION instance_check
* 
* Check for the proper number of object instances for
* the specified value struct. Checks the direct accessible
* children of 'val' only!!!
* 
* The top-level value set passed cannot represent a choice
* or a case within a choice. 
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   obj == object template for child node in valset to check
*   val == val_value_t list, leaf-list, or container to check
*   valroot == root node of the database
*   layer == NCX layer calling this function (for error purposes only)
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    instance_check (ses_cb_t *scb,
                    xml_msg_hdr_t *msg,
                    obj_template_t *obj,
                    val_value_t *val,
                    val_value_t *valroot,
                    ncx_layer_t layer)
{
    /* skip this node if it is non-config */
    if (!obj_is_config(val->obj)) {
        if (LOGDEBUG3) {
            log_debug3("\ninstance_chk: skipping r/o node '%s:%s'",
                       obj_get_mod_name(val->obj),
                       val->name);
        }
        return NO_ERR;
    }

    /* check if the child node is config
     * this function only tests server requirements
     * and ignores mandatory read-only nodes;
     * otherwise the test on candidate would always fail
     */
    if (!obj_is_config(obj)) {
        if (LOGDEBUG3) {
            log_debug3("\ninstance_chk: skipping r/o node '%s:%s'",
                       obj_get_mod_name(obj),
                       obj_get_name(obj));
        }
        return NO_ERR;
    }

    val_value_t *errval = NULL;
    xmlChar *instbuff = NULL;
    status_t res = NO_ERR, res2 = NO_ERR;

    // do not need to check again to see if conditional object is present
    //iqual = val_get_cond_iqualval(val, valroot, obj);
    ncx_iqual_t iqual = obj_get_iqualval(obj);
    boolean cond = FALSE;
    boolean whendone = FALSE;
    uint32 whencnt = 0;
    boolean minerr = FALSE;
    boolean maxerr = FALSE;
    uint32 minelems = 0;
    uint32 maxelems = 0;
    boolean minset = obj_get_min_elements(obj, &minelems);
    boolean maxset = obj_get_max_elements(obj, &maxelems);

    uint32 cnt = val_instance_count(val, obj_get_mod_name(obj), 
                                    obj_get_name(obj));

    if (LOGDEBUG3) {
        if (!minset) {
            switch (iqual) {
            case NCX_IQUAL_ONE:
            case NCX_IQUAL_1MORE:
                minelems = 1;
                break;
            case NCX_IQUAL_ZMORE:
            case NCX_IQUAL_OPT:
                minelems = 0;
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        if (!maxset) {
            switch (iqual) {
            case NCX_IQUAL_ONE:
            case NCX_IQUAL_OPT:
                maxelems = 1;
                break;
            case NCX_IQUAL_1MORE:
            case NCX_IQUAL_ZMORE:
                maxelems = 0;
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        char buff[NCX_MAX_NUMLEN];
        if (maxelems) {
            snprintf(buff, sizeof(buff), "%u", maxelems);
        }

        log_debug3("\ninstance_check '%s:%s' against '%s:%s'\n"
                   "    (cnt=%u, min=%u, max=%s)",
                   obj_get_mod_name(obj),
                   obj_get_name(obj),
                   obj_get_mod_name(val->obj),
                   val->name, 
                   cnt, 
                   minelems, 
                   maxelems ? buff : "unbounded");
    }

    if (minset) {
        if (cnt < minelems) {
            if (cnt == 0) {
                /* need to check if this node is conditional because
                 * of when stmt, and if so, whether when-cond is false  */
                res = val_check_obj_when(val, valroot, NULL, obj, 
                                         &cond, &whencnt);
                if (res == NO_ERR) {
                    whendone = TRUE;
                    if (whencnt && !cond) {
                        if (LOGDEBUG2) {
                            log_debug2("\nwhen_chk: skip false when-stmt for "
                                       "node '%s:%s'", obj_get_mod_name(obj), 
                                       obj_get_name(obj));
                        }
                        return NO_ERR;
                    }
                } else {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_NONE, 
                                     NULL, NCX_NT_VAL, errval);
                    return res;
                }
            }

            /* not enough instances error */
            minerr = TRUE;
            res = ERR_NCX_MIN_ELEMS_VIOLATION;
            val->res = res;

            if (cnt) {
                /* use the first child instance as the
                 * value node for the error-path
                 */
                errval = val_find_child(val, obj_get_mod_name(obj),
                                        obj_get_name(obj));
                agt_record_error(scb, msg, layer, res, NULL, NCX_NT_NONE, 
                                 NULL, NCX_NT_VAL, errval);


            } else {
                /* need to construct a string error-path */
                instbuff = NULL;
                res2 = val_gen_split_instance_id(msg, val, NCX_IFMT_XPATH1,
                                                 obj_get_nsid(obj),
                                                 obj_get_name(obj),
                                                 FALSE, &instbuff);
                if (res2 == NO_ERR && instbuff) {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_NONE, 
                                     NULL, NCX_NT_STRING, instbuff);
                } else {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_NONE, 
                                     NULL, NCX_NT_VAL, val);
                }
                if (instbuff) {
                    m__free(instbuff);
                }
            }
        }
    }

    if (maxset) {
        if (cnt > maxelems) {
            maxerr = TRUE;
            res = ERR_NCX_MAX_ELEMS_VIOLATION;
            val->res = res;

            /* too many instances error
             * need to find all the extra instances
             * and mark the extras as errors or they will
             * not get removed later
             */
            val_set_extra_instance_errors(val, obj_get_mod_name(obj),
                                          obj_get_name(obj),
                                          maxelems);
            /* use the first extra child instance as the
             * value node for the error-path
             * this should always find a node, since 'cnt' > 0 
             */
            errval = val_find_child(val, obj_get_mod_name(obj),
                                    obj_get_name(obj));
            uint32 i = 1;
            while (errval && i <= maxelems) {
                errval = val_get_next_child(errval);
                i++;
            }
            agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ, obj, 
                             NCX_NT_VAL, (errval) ? errval : val);
        }
    }

    switch (iqual) {
    case NCX_IQUAL_ONE:
    case NCX_IQUAL_1MORE:
        if (cnt == 0 && !minerr) {
            /* need to check if this node is conditional because
             * of when stmt, and if so, whether when-cond is false  */
            if (!whendone) {
                cond = FALSE;
                whencnt = 0;
                res = val_check_obj_when(val, valroot, NULL, obj, 
                                         &cond, &whencnt);
                if (res == NO_ERR) {
                    if (whencnt && !cond) {
                        if (LOGDEBUG2) {
                            log_debug2("\nwhen_chk: skip false when-stmt for "
                                       "node '%s:%s'", obj_get_mod_name(obj), 
                                       obj_get_name(obj));
                        }
                        return NO_ERR;
                    }
                } else {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_NONE, 
                                     NULL, NCX_NT_VAL, errval);
                    return res;
                }
            }

            /* make sure this object is not an NP container with
             * mandatory descendant nodes which also have a when-stmt
             * !!! For now, not clear if when-stmt and mandatory-stmt
             * !!! together can be enforced if the context node does
             * !!! not exist; creating a dummy context node changes
             * !!! the data model.    */
            if (obj_is_np_container(obj) &&
                !obj_is_mandatory_when_ex(obj, TRUE)) {
                /* this NP container is mandatory but there are when-stmts
                 * that go with the mandatory nodes, so do not generate
                 * an error for this NP container.  If this container
                 * is non-empty then the when-tests will be run for
                 * those runs after test for this node exits  */
                ;
            } else {
                /* missing single parameter (13.5) */
                res = ERR_NCX_MISSING_VAL_INST;
                val->res = res;

                /* need to construct a string error-path */
                instbuff = NULL;
                res2 = val_gen_split_instance_id(msg, val, NCX_IFMT_XPATH1,
                                                 obj_get_nsid(obj),
                                                 obj_get_name(obj),
                                                 FALSE, &instbuff);
                if (res2 == NO_ERR && instbuff) {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ,
                                     obj, NCX_NT_STRING, instbuff);
                } else {
                    agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ,
                                     obj, NCX_NT_VAL, val);
                }

                if (instbuff) {
                    m__free(instbuff);
                }
            }
        }
        if (iqual == NCX_IQUAL_1MORE) {
            break;
        }
        /* else fall through */
    case NCX_IQUAL_OPT:
        if (cnt > 1 && !maxerr) {
            /* too many parameters */
            val_set_extra_instance_errors(val, obj_get_mod_name(obj),
                                          obj_get_name(obj), 1);
            res = ERR_NCX_EXTRA_VAL_INST;
            val->res = res;

            /* use the first extra child instance as the
             * value node for the error-path
             */
            errval = val_find_child(val, obj_get_mod_name(obj),
                                    obj_get_name(obj));
            uint32 i = 1;
            while (errval && i < maxelems) {
                errval = val_get_next_child(errval);
                i++;
            }
            agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ,
                             obj, NCX_NT_VAL, (errval) ? errval : val);
        }
        break;
    case NCX_IQUAL_ZMORE:
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        val->res = res;
        agt_record_error(scb, msg, layer, res, NULL, NCX_NT_OBJ, obj, 
                         NCX_NT_VAL, val);

    }

    return res;
    
}  /* instance_check */


/********************************************************************
* FUNCTION choice_check_agt
* 
* Agent version of ncx/val_util.c/choice_check
*
* Check a val_value_t struct against its expected OBJ
* for instance validation:
*
*    - choice validation: 
*      only one case allowed if the data type is choice
*      Only issue errors based on the instance test qualifiers
*
* The input is checked against the specified obj_template_t.
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   choicobj == object template for the choice to check
*   val == parent val_value_t list or container to check
*   valroot == root node of the database
*   layer == NCX layer calling this function (for error purposes only)
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    choice_check_agt (ses_cb_t  *scb,
                      xml_msg_hdr_t *msg,
                      obj_template_t *choicobj,
                      val_value_t *val,
                      val_value_t *valroot,
                      ncx_layer_t   layer)
{
    status_t               res = NO_ERR, retres = NO_ERR;

    if (LOGDEBUG3) {
        log_debug3("\nchoice_check_agt: check '%s:%s' against '%s:%s'",
                   obj_get_mod_name(choicobj),
                   obj_get_name(choicobj), 
                   obj_get_mod_name(val->obj),
                   val->name);
    }

    /* Go through all the child nodes for this object
     * and look for choices against the value set to see if each 
     * a choice case is present in the correct number of instances.
     *
     * The current value could never be a OBJ_TYP_CHOICE since
     * those nodes are not stored in the val_value_t tree
     * Instead, it is the parent of the choice object,
     * and the accessible case nodes will be child nodes
     * of that complex parent type
     */
    val_value_t *chval = val_get_choice_first_set(val, choicobj);
    if (!chval) {
        if (obj_is_mandatory(choicobj)) {
            /* error missing choice (13.6) */
            res = ERR_NCX_MISSING_CHOICE;
            agt_record_error(scb, msg, layer, res, NULL, NCX_NT_VAL, val, 
                                 NCX_NT_VAL, val);
        }
        return res;
    }

    /* else a choice was selected
     * first make sure all the mandatory case 
     * objects are present
     */
    obj_template_t *testobj = obj_first_child(chval->casobj);
    for (; testobj != NULL; testobj = obj_next_child(testobj)) {
        res = instance_check(scb, msg, testobj, val, valroot, layer);
        CHK_EXIT(res, retres);
        /* errors already recorded if other than NO_ERR */
    }

    /* check if any objects from other cases are present */
    val_value_t *testval = val_get_choice_next_set(choicobj, chval);
    while (testval) {
        if (testval->casobj != chval->casobj) {
            /* error: extra case object in this choice */
            retres = res = ERR_NCX_EXTRA_CHOICE;
            agt_record_error(scb, msg, layer, res, NULL, 
                             NCX_NT_OBJ, choicobj, NCX_NT_VAL, testval);
        }
        testval = val_get_choice_next_set(choicobj, testval);
    }

    if (val->res == NO_ERR) {
        val->res = retres;
    }

    return retres;

}  /* choice_check_agt */


/********************************************************************
 * Perform a must statement check of the object tree.
* Check for any must-stmts in the object tree and validate the Xpath
* expression against the complete database 'root' and current
* context node 'curval'.
* 
* \param scb session control block (may be NULL; no session stats)
* \param msg xml_msg_hdr t from msg in progress, NULL means no RPC-ERROR 
             recording.
* \param root val_value_t or the target database root to validate
* \param curval val_value_t for the current context node in the tree
* \return NO_ERR if there are no validation errors.

*********************************************************************/
static status_t must_stmt_check ( ses_cb_t *scb,
                                  xml_msg_hdr_t *msg,
                                  val_value_t *root,
                                  val_value_t *curval )
{
    status_t res = NO_ERR;

    obj_template_t *obj = curval->obj;
    if ( !obj_is_config(obj) ) {
        return NO_ERR;
    }

    dlq_hdr_t *mustQ = obj_get_mustQ(obj);
    if ( !mustQ || dlq_empty( mustQ ) ) {
        /* There are no must statements, return NO ERR */
        return NO_ERR;
    }

    /* execute all the must tests top down, so* foo/bar errors are reported 
     * before /foo/bar/child */
    xpath_pcb_t *must = (xpath_pcb_t *)dlq_firstEntry(mustQ);
    for ( ; must; must = (xpath_pcb_t *)dlq_nextEntry(must)) {
        res = NO_ERR;
        xpath_result_t *result = xpath1_eval_expr( must, curval, root, FALSE, 
                                                   TRUE, &res );
        if ( !result || res != NO_ERR ) {
            log_error("\nmust_chk: failed for %s:%s (%s) expr '%s'", 
                    obj_get_mod_name(obj), curval->name, get_error_string(res),
                    must->exprstr );

            if (res == NO_ERR) {
                res = ERR_INTERNAL_VAL;
                SET_ERROR( res );
            }
        } else if (!xpath_cvt_boolean(result)) {
            log_debug2("\nmust_chk: false for %s:%s expr '%s'", 
                    obj_get_mod_name(obj), curval->name, must->exprstr);

            res = ERR_NCX_MUST_TEST_FAILED;
        } else {
            log_debug3("\nmust_chk: OK for %s:%s expr '%s'", 
                    obj_get_mod_name(obj), curval->name, must->exprstr);
        }

        if ( NO_ERR != res ) {
            agt_record_error_errinfo(scb, msg, NCX_LAYER_CONTENT, res, NULL, 
                    NCX_NT_STRING, must->exprstr, NCX_NT_VAL, curval, 
                    ( ( ncx_errinfo_set( &must->errinfo) ) ? &must->errinfo 
                                                           : NULL ) );

            if ( terminate_parse( res ) )  { 
                return res; 
            }
            curval->res = res;
        }
        xpath_free_result(result);
    }

    return res;
}  /* must_stmt_check */


/********************************************************************
* FUNCTION run_when_stmt_check
* 
* Run one when-stmt test on a node

* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   root == val_value_t or the target database root to validate
*   curval == val_value_t for the current context node in the tree
*   configmode == TRUE to test config-TRUE
*                 FALSE to test config=FALSE
*   deleteQ  == address of Q for deleted descendants
*   deleteme == address of return delete flag
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*    txcb->deadnodeQ will have any deleted instance entries
*         due to false when-stmt expressions
*
* RETURNS:
*   status of the operation, NO_ERR or ERR_INTERNAL_MEM most likely
*********************************************************************/
static status_t 
    run_when_stmt_check (ses_cb_t *scb,
                         xml_msg_hdr_t *msg,
                         agt_cfg_transaction_t *txcb,
                         val_value_t *root,
                         val_value_t *curval)
{
    obj_template_t        *obj = curval->obj;
    status_t               res = NO_ERR;
    boolean                condresult = FALSE;
    uint32                 whencount = 0;

    res = val_check_obj_when(curval->parent, root, curval, obj, &condresult,
                             &whencount);
    if (res != NO_ERR) {
        log_error("\nError: when_check: failed for %s:%s (%s)",
                  obj_get_mod_name(obj),
                  obj_get_name(obj),
                  get_error_string(res));
        agt_record_error(scb, msg, NCX_LAYER_OPERATION, res, NULL, 
                         NCX_NT_NONE, NULL, NCX_NT_VAL, curval);
    } else if (whencount == 0) {
        if (LOGDEBUG) {
            log_debug("\nwhen_chk: no when-tests found for node '%s:%s'", 
                       obj_get_mod_name(obj), curval->name);
        }
    } else if (!condresult) {
        if (LOGDEBUG2) {
            log_debug2("\nwhen_chk: test false for node '%s:%s'", 
                       obj_get_mod_name(obj), curval->name);
        }
        /* check if this session is allowed to delete this node
         * and make sure this node is not partially locked  */
        if (!agt_acm_val_write_allowed(msg, scb->username, NULL, curval,
                                       OP_EDITOP_DELETE)) {
            res = ERR_NCX_ACCESS_DENIED;
            agt_record_error(scb, msg, NCX_LAYER_OPERATION, res,
                             NULL, NCX_NT_NONE, NULL, NCX_NT_NONE, NULL);
            return res;
        }

        uint32 lockid = 0;
        res = val_write_ok(curval, OP_EDITOP_DELETE, SES_MY_SID(scb),
                           TRUE, &lockid);
        if (res != NO_ERR) {
            agt_record_error(scb, msg, NCX_LAYER_OPERATION, res, 
                             NULL, NCX_NT_UINT32_PTR, &lockid, 
                             NCX_NT_VAL, curval);
            return res;
        }

        /* need to delete the current value */
        agt_cfg_nodeptr_t *nodeptr = agt_cfg_new_nodeptr(curval);
        if (nodeptr == NULL) {
            res = ERR_INTERNAL_MEM;
            agt_record_error(scb, msg, NCX_LAYER_OPERATION, res, 
                             NULL, NCX_NT_NONE, NULL, NCX_NT_NONE, NULL);
            return res;
        }
        log_debug("\nMarking false when-stmt as deleted %s:%s",
                  obj_get_mod_name(obj),
                  obj_get_name(obj));
                  
        dlq_enque(nodeptr, &txcb->deadnodeQ);
        VAL_MARK_DELETED(curval);
    } else {
        if (LOGDEBUG3) {
            log_debug3("\nwhen_chk: test passed for node '%s:%s'", 
                       obj_get_mod_name(obj), curval->name);
        }
    }
    return res;

}  /* run_when_stmt_check */


/********************************************************************
* FUNCTION rpc_when_stmt_check
* 
* Check for any when-stmts in the object tree and validate the Xpath
* expression against the complete database 'root' and current
* context node 'curval'.  ONLY USED FOR RPC INPUT CHECKING!!
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   rpcmsg == rpc msg header for audit purposes
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   root == val_value_t or the target database root to validate
*   curval == val_value_t for the current context node in the tree
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    rpc_when_stmt_check (ses_cb_t *scb,
                         rpc_msg_t *rpcmsg,
                         xml_msg_hdr_t *msg,
                         val_value_t *root,
                         val_value_t *curval)
{
    obj_template_t  *obj = curval->obj;
    status_t         retres = NO_ERR;
    boolean          condresult = FALSE;
    uint32           whencount = 0;

    retres = val_check_obj_when(curval->parent, root, curval, obj, &condresult,
                                &whencount);
    if (retres != NO_ERR) {
        log_error("\nError: when_check: failed for %s:%s (%s)",
                  obj_get_mod_name(obj), obj_get_name(obj),
                  get_error_string(retres));
        agt_record_error(scb, msg, NCX_LAYER_OPERATION, retres, NULL, 
                         NCX_NT_NONE, NULL, NCX_NT_VAL, curval);
        return retres;
    } else if (whencount && !condresult) {
        retres = ERR_NCX_RPC_WHEN_FAILED;
        agt_record_error(scb, msg, NCX_LAYER_OPERATION,
                         retres, NULL, NCX_NT_VAL,
                         curval, NCX_NT_VAL, curval);
        return retres;
    } else {
        if (LOGDEBUG3 && whencount) {
            log_debug3("\nwhen_chk: test passed for node '%s:%s'", 
                       obj_get_mod_name(obj), curval->name);
        }
    }

    /* recurse for every child node until leafs are hit */
    val_value_t *chval = val_get_first_child(curval);
    for (; chval != NULL && retres == NO_ERR; 
         chval = val_get_next_child(chval)) {

        if (!obj_is_config(chval->obj)) {
            continue;
        }

        if (obj_is_root(chval->obj)) {
            /* do not dive into a <config> node parameter
             * while processing an RPC input node
             */
            continue;
        }

        retres = rpc_when_stmt_check(scb, rpcmsg, msg, root, chval);
    }

    return retres;
    
}  /* rpc_when_stmt_check */


/********************************************************************
 * Perform a must statement check of the object tree.
 * USED FOR RPC INPUT TESTING ONLY
* Check for any must-stmts in the object tree and validate the Xpath
* expression against the complete database 'root' and current
* context node 'curval'.
* 
* \param scb session control block (may be NULL; no session stats)
* \param msg xml_msg_hdr t from msg in progress, NULL means no RPC-ERROR 
             recording.
* \param root val_value_t or the target database root to validate
* \param curval val_value_t for the current context node in the tree
* \return NO_ERR if there are no validation errors.

*********************************************************************/
static status_t rpc_must_stmt_check ( ses_cb_t *scb,
                                      xml_msg_hdr_t *msg,
                                      val_value_t *root,
                                      val_value_t *curval )
{
    status_t res;
    status_t firstError = NO_ERR;

    obj_template_t *obj = curval->obj;
    if ( !obj_is_config(obj) ) {
        return NO_ERR;
    }

    dlq_hdr_t *mustQ = obj_get_mustQ(obj);
    if ( !mustQ || dlq_empty( mustQ ) ) {
        /* There are no must statements, return NO ERR */
        return NO_ERR;
    }

    /* execute all the must tests top down, so* foo/bar errors are reported 
     * before /foo/bar/child */
    xpath_pcb_t *must = (xpath_pcb_t *)dlq_firstEntry(mustQ);
    for ( ; must; must = (xpath_pcb_t *)dlq_nextEntry(must)) {
        res = NO_ERR;
        xpath_result_t *result = xpath1_eval_expr( must, curval, root, FALSE, 
                                                   TRUE, &res );
        if ( !result || res != NO_ERR ) {
            log_error("\nrpc_must_chk: failed for %s:%s (%s) expr '%s'", 
                    obj_get_mod_name(obj), curval->name, get_error_string(res),
                    must->exprstr );

            if (res == NO_ERR) {
                res = ERR_INTERNAL_VAL;
                SET_ERROR( res );
            }
        } else if (!xpath_cvt_boolean(result)) {
            log_debug2("\nrpc_must_chk: false for %s:%s expr '%s'", 
                    obj_get_mod_name(obj), curval->name, must->exprstr);

            res = ERR_NCX_MUST_TEST_FAILED;
        } else {
            log_debug3("\nrpc_must_chk: OK for %s:%s expr '%s'", 
                    obj_get_mod_name(obj), curval->name, must->exprstr);
        }

        if ( NO_ERR != res ) {
            agt_record_error_errinfo(scb, msg, NCX_LAYER_CONTENT, res, NULL, 
                    NCX_NT_STRING, must->exprstr, NCX_NT_VAL, curval, 
                    ( ( ncx_errinfo_set( &must->errinfo) ) ? &must->errinfo 
                                                           : NULL ) );

	        if ( terminate_parse( res ) )  { 
	            return res; 
	        }
            curval->res = res;
            firstError = ( firstError != NO_ERR) ? firstError : res;
        }
        xpath_free_result(result);
    }

    /* recurse for every child node until leafs are hit but only if the parent 
     * must test did not fail will check sibling nodes even if some must-test
     * already failed; this provides complete errors during validate and 
     * load_running_config */
    if ( firstError == NO_ERR ) {
        val_value_t *chval = val_get_first_child(curval);

        for ( ; chval; chval = val_get_next_child(chval)) {
             /* do not dive into <config> parameters and hit database 
              * must-stmts by mistake */
            if ( !obj_is_root(chval->obj) ) {
                res = rpc_must_stmt_check(scb, msg, root, chval);

                if ( NO_ERR != res ) {
	               if ( terminate_parse( res ) )  { 
	                   return res; 
	               }
                   firstError = ( firstError != NO_ERR) ? firstError : res;
                }
            }
        }
    }

    return firstError;

}  /* rpc_must_stmt_check */


/********************************************************************
 * FUNCTION prep_commit_test_node
 *
 * Update the commit test instances
 *
 * \param scb session control block
 * \param msghdr XML message header in progress
 * \param txcb transaction control block to use
 * \param ct commit test to use
 * \param root <config> node to check
 *
 * \return status
 *********************************************************************/
static status_t 
    prep_commit_test_node ( ses_cb_t  *scb,
                            xml_msg_hdr_t *msghdr,
                            agt_cfg_transaction_t *txcb,
                            agt_cfg_commit_test_t *ct,
                            val_value_t *root )
{
    status_t res = NO_ERR;

   /* first get all the instances of this object if needed */
    if (ct->result) {
        if (ct->result_txid == txcb->txid) {
            log_debug3("\nReusing XPath result for %s", ct->objpcb->exprstr);
        } else {
            /* TBD: figure out if older TXIDs are still valid
             * for this XPath expression  */
            log_debug3("\nGet all instances of %s", ct->objpcb->exprstr);
            xpath_free_result(ct->result);
            ct->result = NULL;
        }
    }

    if (ct->result == NULL) {
        ct->result_txid = 0;
        ct->result = xpath1_eval_expr(ct->objpcb, root, root, FALSE, 
                                      TRUE, &res);
        if (res != NO_ERR || ct->result->restype != XP_RT_NODESET) {
            if (res == NO_ERR) {
                res = ERR_NCX_WRONG_NODETYP;
            }
            agt_record_error(scb, msghdr, NCX_LAYER_CONTENT, res, NULL,
                             NCX_NT_NONE, NULL, NCX_NT_OBJ, ct->obj);
        } else {
            ct->result_txid = txcb->txid;
        }
    }

    return res;

} /* prep_commit_test_node */


/********************************************************************
 * 
 * Delete all the nodes that have false when-stmt exprs
 * Also delete empty NP-containers
 *
 * \param scb session control block (may be NULL)
 * \param msghdr XML message header in progress
 * \param txcb transaction control block to use
 * \param root root from the target database to use
 * \param retcount address of return deletecount
 * \return status
 *********************************************************************/
static status_t delete_dead_nodes ( ses_cb_t  *scb,
                                    xml_msg_hdr_t *msghdr,
                                    agt_cfg_transaction_t *txcb,
                                    val_value_t *root,
                                    uint32 *retcount )
{
    *retcount = 0;

    agt_profile_t *profile = agt_get_profile();
    agt_cfg_commit_test_t *ct = (agt_cfg_commit_test_t *)
        dlq_firstEntry(&profile->agt_commit_testQ);

    for (; ct != NULL; ct = (agt_cfg_commit_test_t *)dlq_nextEntry(ct)) {
        uint32  tests = ct->testflags & AGT_TEST_FL_WHEN;
        if (tests == 0) {
            /* no when-stmt tests needed for this node */
            continue;
        }

        status_t res = prep_commit_test_node(scb, msghdr, txcb, ct, root);
        if (res != NO_ERR) {
            return res;
        }

        /* run all relevant tests on each node in the result set */
        xpath_resnode_t *resnode = xpath_get_first_resnode(ct->result);
        xpath_resnode_t *nextnode = NULL;
        for (; resnode != NULL; resnode = nextnode) {
            nextnode = xpath_get_next_resnode(resnode);

            val_value_t *valnode = xpath_get_resnode_valptr(resnode);
            res = run_when_stmt_check(scb, msghdr, txcb, root, valnode);
            if (res != NO_ERR) {
                /* treat any when delete error as terminate transaction */
                return res;
            } else if (VAL_IS_DELETED(valnode)) {
                /* this node has just been flagged when=FALSE so
                 * remove this resnode from the result so it will
                 * not be reused by a commit test   */
                xpath_delete_resnode(resnode);
                (*retcount)++;
            }
        }
    }

    return NO_ERR;

}  /* delete_dead_nodes */


/********************************************************************
* FUNCTION check_parent_tests
* 
* Check if the specified object has any commit tests
* to record in the parent node
*
* INPUTS:
*  obj == object to check
*  flags == address of return testflags
*
* OUTPUTS:
*   *flags is set if any tests are needed
*
*********************************************************************/
static void
    check_parent_tests (obj_template_t *obj,
                        uint32 *flags)
{
    uint32 numelems = 0;
    switch (obj->objtype) {
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
        if (obj_get_min_elements(obj, &numelems)) {
            if (numelems > 1) {
                *flags |= AGT_TEST_FL_MIN_ELEMS;
            } // else AGT_TEST_FL_MANDATORY will check for 1 instance
        }
        numelems = 0;
        if (obj_get_max_elements(obj, &numelems)) {
            *flags |= AGT_TEST_FL_MAX_ELEMS;
        }
        break;
    case OBJ_TYP_LEAF:
    case OBJ_TYP_CONTAINER:
        *flags |= AGT_TEST_FL_MAX_ELEMS;
        break;       
    case OBJ_TYP_CHOICE:
        *flags |= AGT_TEST_FL_CHOICE;
        break;
    default:
        ;
    }
    if (obj_is_mandatory(obj) && !obj_is_key(obj)) {
        *flags |= AGT_TEST_FL_MANDATORY;
    }

}  /* check_parent_tests */


/********************************************************************
* FUNCTION add_obj_commit_tests
* 
* Check if the specified object and all its descendants 
* have any commit tests to record in the commit_testQ
* Tests are added in top-down order
* TBD: cascade XPath lookup results to nested commmit tests
*
* Tests done in the parent node:
*   AGT_TEST_FL_MIN_ELEMS
*   AGT_TEST_FL_MAX_ELEMS
*   AGT_TEST_FL_MANDATORY
*
* Tests done in the object node:
*   AGT_TEST_FL_MUST
*   AGT_TEST_FL_UNIQUE
*   AGT_TEST_FL_XPATH_TYPE
*   AGT_TEST_FL_WHEN  (part of delete_dead_nodes, not this fn)
*
* INPUTS:
*  obj == object to check
*  commit_testQ == address of queue to use to fill with
*                  agt_cfg_commit_test_t structs
*  rootflags == address of return root node testflags
*
* OUTPUTS:
*  commit_testQ will contain an agt_cfg_commit_test_t struct
*  for each descendant-or-self object found with 
*  commit-time validation tests
*  *rootflags will be altered if node is top-level and
*   needs instance testing
* RETURNS:
*  status of the operation, NO_ERR unless internal errors found
*  or malloc error
*********************************************************************/
static status_t
    add_obj_commit_tests (obj_template_t *obj,
			  dlq_hdr_t *commit_testQ,
                          uint32 *rootflags)
{
    if (skip_obj_commit_test(obj)) {
        return NO_ERR;
    }

    status_t res = NO_ERR;

    /* check for tests in active config nodes, bottom-up traversal */
    uint32 testflags = 0;
    dlq_hdr_t *mustQ = obj_get_mustQ(obj);
    ncx_btype_t btyp = obj_get_basetype(obj);
    obj_template_t *chobj;

    if (!(obj->objtype == OBJ_TYP_CHOICE || obj->objtype == OBJ_TYP_CASE)) {
        if (mustQ && !dlq_empty(mustQ)) {
            testflags |= AGT_TEST_FL_MUST;
        }
    
        if (obj_has_when_stmts(obj)) {
            testflags |= AGT_TEST_FL_WHEN;
        }
    }

    obj_unique_t *unidef = obj_first_unique(obj);
    boolean done = FALSE;
    for (; unidef && !done; unidef = obj_next_unique(unidef)) {
        if (unidef->isconfig) {
            testflags |= AGT_TEST_FL_UNIQUE;
            done = TRUE;
        }
    }

    if (obj_is_top(obj)) {
        /* need to check if this is a top-level node that
         * has instance requirements (mandatory/min/max)
         */
        check_parent_tests(obj, rootflags);
    }

    if (obj->objtype == OBJ_TYP_CHOICE || obj->objtype == OBJ_TYP_CASE) {
        /* do not create a commit test record for a choice or case
         * since they will never be in the data tree, so XPath
         * will never find them */
        testflags = 0;
    } else {
        /* set the instance or MANDATORY test bits in the parent,
         * not in the child nodes for each commit test
         * check all child nodes to determine if parent needs
         * to run instance_check
         */
        for (chobj = obj_first_child(obj); 
             chobj != NULL; 
             chobj = obj_next_child(chobj)) {

            if (skip_obj_commit_test(chobj)) {
                continue;
            }
            check_parent_tests(chobj, &testflags);
        }
    
        typ_def_t *typdef = obj_get_typdef(obj);
        if (btyp == NCX_BT_LEAFREF || 
            (btyp == NCX_BT_INSTANCE_ID && typ_get_constrained(typdef))) {
            /* need to check XPath-based leaf to see that the referenced
             * instance exists in the target data tree
             */
            testflags |= AGT_TEST_FL_XPATH_TYPE;
        }
    }

    if (testflags) {
        /* need a commit test record for this object */
        agt_cfg_commit_test_t *ct = agt_cfg_new_commit_test();
        if (ct == NULL) {
            return ERR_INTERNAL_MEM;
        } 

        ct->objpcb = xpath_new_pcb(NULL, NULL);
        if (ct->objpcb == NULL) {
            agt_cfg_free_commit_test(ct);
            return ERR_INTERNAL_MEM;
        }

        res = obj_gen_object_id_xpath(obj, &ct->objpcb->exprstr);
        if (res != NO_ERR) {
            agt_cfg_free_commit_test(ct);
            return res;
        }

        ct->obj = obj;
        ct->btyp = btyp;
        ct->testflags = testflags;
        dlq_enque(ct, commit_testQ);
        if (LOGDEBUG4) {
            log_debug4("\nAdded commit_test record for %s", 
                       ct->objpcb->exprstr);
        }
    }

    for (chobj = obj_first_child(obj);
         chobj != NULL;
         chobj = obj_next_child(chobj)) {
        res = add_obj_commit_tests(chobj, commit_testQ, rootflags);
        if (res != NO_ERR) {
            return res;
        }
    }

    return res;

} /* add_obj_commit_tests */


/********************************************************************
* FUNCTION check_prune_obj
* 
* Check if the specified object test can be reduced or eliminated
* from the current commit test
*
* INPUTS:
*    obj == commit test object to check; must not be root!
*    rootval == config root to check
*    curflags == current test flags
* RETURNS:
*  new testflags or unchanged test flags
*********************************************************************/
static uint32
    check_prune_obj (obj_template_t *obj,
                     val_value_t *rootval,
                     uint32 curflags)
{
    /* for now just check the top level object to see if
     * this subtree could have possibly changed;
     * TBD: come up with fast way to check more, that does
     * take as much time as just using XPath to get these objects  */
    boolean done = FALSE;
    obj_template_t *testobj = obj;
    while (!done) {
        if (testobj->parent && !obj_is_root(testobj->parent)) {
            testobj = testobj->parent;
        } else {
            done = TRUE;
        }
    }

    /* find this object in the rootval */
    val_value_t *testval = val_find_child(rootval, obj_get_mod_name(testobj),
                                          obj_get_name(testobj));
    if (testval) {
        switch (testobj->objtype) {
        case OBJ_TYP_LEAF:
        case OBJ_TYP_ANYXML:
        case OBJ_TYP_CONTAINER:
            if (val_dirty_subtree(testval)) {
                /* do not skip this node */
                return curflags;
            }
            break;
        default:
            done = FALSE;
            while (!done) {
                if (val_dirty_subtree(testval)) {
                    /* do not skip this node */
                    return curflags;
                } else {
                    testval = 
                        val_find_next_child(rootval, obj_get_mod_name(testobj),
                                            obj_get_name(testobj), testval);
                    if (testval == NULL) {
                        done = TRUE;
                    }
                }
            }
        }
    }

    /* skip this node for all tests, since no instances of
     * the top-level object exist which might have been changed
     * Relying on the fact that the root-check is always done for
     * the root node (on the top-level YANG data nodes, so
     * any instance or mandatory tests on the top-node will
     * always be done     */
    return 0;

} /* check_prune_obj */


/********************************************************************
* FUNCTION check_no_prune_curedit
* 
* Check if the specified object test is needed from the current 
* commit test, based on the current edit in the undo record
*
* INPUTS:
*   undo == edit operation to check
*   obj == commit test object to check; must not be root!
* RETURNS:
*  TRUE if found and test needed; FALSE if not found
*********************************************************************/
static boolean
    check_no_prune_curedit (agt_cfg_undo_rec_t *undo,
                            obj_template_t *obj)
{
    obj_template_t *editobj = (undo->newnode) ? undo->newnode->obj :
        ((undo->curnode) ? undo->curnode->obj : NULL);
    if (editobj == NULL) {
        return FALSE;
    }

    /* see if the commit test object is a descendant of
     * the edit object, but only if not deleting the edit
     * object; do not trigger descendant test on a delete  */
    boolean done = (undo->editop == OP_EDITOP_DELETE);
    obj_template_t *testobj = obj;
    while (!done) {
        if (editobj == testobj) {
            return TRUE;   // do not prune this object test
        }

        if (testobj->parent && !obj_is_root(testobj->parent)) {
            testobj = testobj->parent;
        } else {
            done = TRUE;
        }
    }

    /* see if the edit object is a descendant of the commit test object */
    done = FALSE;
    while (!done) {
        if (editobj == obj) {
            return TRUE;   // do not prune this object test
        }

        if (editobj->parent && !obj_is_root(editobj->parent)) {
            editobj = editobj->parent;
        } else {
            done = TRUE;
        }
    }

    return FALSE;

} /* check_no_prune_curedit */


/********************************************************************
* FUNCTION prune_obj_commit_tests
* 
* Check if the specified object test can be reduced or elimated
* from the current commit test
*
* INPUTS:
*
* OUTPUTS:
*  *testflags will be altered
* RETURNS:
*  new (or unchanged) test flags
*********************************************************************/
static uint32
    prune_obj_commit_tests (agt_cfg_transaction_t *txcb,
                            agt_cfg_commit_test_t *ct,
                            val_value_t *rootval,
                            uint32 curflags)
{
    if (curflags & AGT_TEST_FL_MUST) {
        /* do not prune a must-stmt test since the XPath expression
         * may reference any arbitrary data node    */
        return curflags;
    }

    agt_cfg_undo_rec_t *undo = NULL;
    if (txcb->cfg_id == NCX_CFGID_RUNNING) {
        if (txcb->commitcheck) {
            /* called from <commit> 
             * look for dirty or subtree dirty flags in the rootval
             * this rootval is reallt candidate->root, so it is
             * OK to check the dirty flags    */
            curflags = check_prune_obj(ct->obj, rootval, curflags);
        } else {
            if (txcb->edit_type == AGT_CFG_EDIT_TYPE_FULL) {
                /* called from <validate> on external <config> element;
                 * <copy-config> is not supported on the running config
                 * so there is nothing to do for now; do not prune;
                 * force a full test for <validate>   */
            } else if (txcb->edit_type == AGT_CFG_EDIT_TYPE_PARTIAL) {
                /* called from <edit-config> on running config;
                 * check the edit list; only nodes that have been
                 * edited need to run a commit test   */
                undo = (agt_cfg_undo_rec_t *)dlq_firstEntry(&txcb->undoQ);
                while (undo) {
                    if (check_no_prune_curedit(undo, ct->obj)) {
                        return curflags;
                    }
                    undo = (agt_cfg_undo_rec_t *)dlq_nextEntry(undo);
                }
                curflags = 0;
            } // else unknown edit-type! do not prune
        }
    } else if (txcb->cfg_id == NCX_CFGID_CANDIDATE) {
        if (txcb->edit_type == AGT_CFG_EDIT_TYPE_FULL) {
            /* called from <validate> operation on the <candidate> config */
            curflags = check_prune_obj(ct->obj, rootval, curflags);
        } else if (txcb->edit_type == AGT_CFG_EDIT_TYPE_PARTIAL) {
            /* called from <edit-config> with test-option=set-then-test
             * first check all the current edits; because the dirty flags
             * for any edits from this <rpc> have not been set yet */
            undo = (agt_cfg_undo_rec_t *)dlq_firstEntry(&txcb->undoQ);
            while (undo) {
                if (check_no_prune_curedit(undo, ct->obj)) {
                    return curflags;
                }
                undo = (agt_cfg_undo_rec_t *)dlq_nextEntry(undo);
            }

            /* did not find the object in the current edits so check
             * pending edits (dirty flags) on the candidate->root target */
            curflags = check_prune_obj(ct->obj, rootval, curflags);
        } // else unknown edit-type! do not prune!
    } // else unknown config target! do not prune!

    return curflags;

} /* prune_obj_commit_tests */


/********************************************************************
* FUNCTION run_instance_check
* 
* Check for the proper number of object instances for
* the specified value struct.
* 
* The top-level value set passed cannot represent a choice
* or a case within a choice.
*
* This function is intended for validating PDUs (RPC requests)
* during the PDU processing.  It does not check the instance
* count or must-stmt expressions for any <config> (ncx:root)
* container.  This must be done with the agt_val_root_check function.
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   valset == val_value_t list, leaf-list, or container to check
*   valroot == root node of the database
*   layer == NCX layer calling this function (for error purposes only)
*   all_levels == TRUE to recurse for all levels
*              == FALSE to just check the curent level
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    run_instance_check (ses_cb_t *scb,
                        xml_msg_hdr_t *msg,
                        val_value_t *valset,
                        val_value_t *valroot,
                        ncx_layer_t layer,
                        boolean all_levels)
{
    assert( valset && "valset is NULL!" );

    if (LOGDEBUG4) {
        log_debug4("\nrun_instance_check: %s:%s start", 
                   obj_get_mod_name(valset->obj),
                   valset->name);
    }

    status_t res = NO_ERR, retres = NO_ERR;
    obj_template_t *obj = valset->obj;

    if (skip_obj_commit_test(obj)) {
        return NO_ERR;
    }

    if (all_levels && val_child_cnt(valset)) {
        val_value_t *chval = val_get_first_child(valset);
        for (; chval != NULL; chval = val_get_next_child(chval)) {

            if (!(obj_is_root(chval->obj) || obj_is_leafy(chval->obj))) {
                /* recurse for all object types except root, leaf, leaf-list */
                res = run_instance_check(scb, msg, chval, valroot, 
                                         layer, all_levels);
                CHK_EXIT(res, retres);
            }
        }
    }

    /* check all the child nodes for correct number of instances */
    obj_template_t *chobj = obj_first_child(obj);
    for (; chobj != NULL; chobj = obj_next_child(chobj)) {
        
        if (obj_is_root(chobj)) {
            continue;
        }

        if (chobj->objtype == OBJ_TYP_CHOICE) {
            res = choice_check_agt(scb, msg, chobj, valset, valroot, layer);
        } else {
            res = instance_check(scb, msg, chobj, valset, valroot, layer);
        }
        if (res != NO_ERR && valset->res == NO_ERR) {
            valset->res = res;
        }
        CHK_EXIT(res, retres);
    }

    return retres;
    
}  /* run_instance_check */


/********************************************************************
* FUNCTION run_obj_commit_tests
* 
* Run selected validation tests from section 8 of RFC 6020
*
* INPUTS:
*   profile == agt_profile pointer
*   scb == session control block (may be NULL; no session stats)
*   msghdr == XML message header in progress 
*          == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   ct == commit test record to use (NULL == root test)
*   val == context node in data tree to use
*   root == root of the data tree to use
*   testmask == bitmask of the tests that are requested
*
* OUTPUTS:
*   if msghdr not NULL:
*      msghdr->msg_errQ may have rpc_err_rec_t 
*      structs added to it which must be freed by the 
*      caller with the rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    run_obj_commit_tests (agt_profile_t *profile,
                          ses_cb_t *scb,
                          xml_msg_hdr_t *msghdr,
                          agt_cfg_commit_test_t *ct,
                          val_value_t *val,
                          val_value_t *root,
                          uint32 testmask)
{
    status_t res, retres = NO_ERR;
    uint32   testflags = (ct) ? ct->testflags : profile->agt_rootflags;
    
    testflags &= testmask;

    if (testflags) {
        if (LOGDEBUG2) {
            log_debug2("\nrun obj_commit_tests for %s", 
                       (ct) ? ct->objpcb->exprstr : (const xmlChar *)"/");
        }
    } else {
        if (LOGDEBUG4) {
            log_debug4("\nskip obj_commit_tests for %s", 
                       (ct) ? ct->objpcb->exprstr : (const xmlChar *)"/");
        }
        return NO_ERR;
    }

    /* go through all the commit test objects that might need
     * to be checked for this object
     */
    if (testflags & AGT_TEST_INSTANCE_MASK) {

        /* need to run the instance tests on the 'val' node */
        if (ct) {
            res = run_instance_check(scb, msghdr, val, root,
                                     NCX_LAYER_CONTENT, FALSE);
            CHK_EXIT(res, retres);
        } else {
            /* TBD: figure out how to use the gen_root object
             * by moving all the data_db nodes into gen_root datadefQ
             * This requires moving them from mod->datadefQ so that
             * these 2 functions below will no longer work
             * Too much code uses this for-loop search approach
             * so this change is still TBD */
            ncx_module_t *mod = ncx_get_first_module();
            for (; mod != NULL; mod = ncx_get_next_module(mod)) {
                obj_template_t *obj = ncx_get_first_data_object(mod);
                for (; obj != NULL; obj = ncx_get_next_data_object(mod, obj)) {

                    if (skip_obj_commit_test(obj)) {
                        continue;
                    }

                    /* just run instance check on the top level object */
                    if (obj->objtype == OBJ_TYP_CHOICE) {
                        res = choice_check_agt(scb, msghdr, obj, val, root,
                                               NCX_LAYER_CONTENT);
                    } else {
                        res = instance_check(scb, msghdr, obj, val, root,
                                             NCX_LAYER_CONTENT);
                    }
                    CHK_EXIT(res, retres);
                }
            }
        }
    }

    if (testflags & AGT_TEST_FL_MUST) {
        res = must_stmt_check(scb, msghdr, root, val);
        if (res != NO_ERR) {
            CHK_EXIT(res, retres);
        }
    }

    if (testflags & AGT_TEST_FL_XPATH_TYPE) {
        res = instance_xpath_check(scb, msghdr, val, root, NCX_LAYER_CONTENT);
        if (res != NO_ERR) {
            CHK_EXIT(res, retres);
        }
    }

    // defer AGT_TEST_FL_UNIQUE and process entire resnode list at once

    return retres;
    
}  /* run_obj_commit_tests */


/********************************************************************
* FUNCTION run_obj_unique_tests
* 
* Run unique-stmt tests from section 7.8.3 of RFC 6020
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msghdr == XML message header in progress 
*          == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   ct == commit test record to use (NULL == root test)
*   root == docroot for XPath
* OUTPUTS:
*   if msghdr not NULL:
*      msghdr->msg_errQ may have rpc_err_rec_t 
*      structs added to it which must be freed by the 
*      caller with the rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
static status_t 
    run_obj_unique_tests (ses_cb_t *scb,
                          xml_msg_hdr_t *msghdr,
                          agt_cfg_commit_test_t *ct,
                          val_value_t *root)
{
    status_t res = NO_ERR;

    if (ct->testflags & AGT_TEST_FL_UNIQUE) {
        log_debug2("\nrun unique tests for %s", ct->objpcb->exprstr);
        res = unique_stmt_check(scb, msghdr, ct, root);
    }

    return res;
    
}  /* run_obj_unique_tests */


/******************* E X T E R N   F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_val_rpc_xpath_check
* 
* Check for any nodes which are present
* but have false when-stmts associated
* with the node.  These are errors and
* need to be flagged as unknown-element
* 
* Any false nodes will be removed from the input PDU
* and discarded, after the error is recorded.
* This prevents false positives or negatives in
* the agt_val_instance_check, called after this function
*
* Also checks any false must-stmts for nodes
* which are present (after false when removal)
* These are flagged as 'must-violation' errors
* as per YANG, 13.4
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   rpcmsg == RPC msg header for audit purposes
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   rpcinput == RPC input node conceptually under rpcroot
*               except this rpcinput has no parent node
*               so a fake one will be termporarily added 
*               to prevent false XPath validation errors
*   rpcroot == RPC method node. 
*              The conceptual parent of this node 
*              is used as the document root (/rpc == /)
*
* OUTPUTS:
*   if false nodes found under :
*     they are deleted
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation
*   NO_ERR if no false when or must statements found
*********************************************************************/
status_t 
    agt_val_rpc_xpath_check (ses_cb_t *scb,
                             rpc_msg_t *rpcmsg,
                             xml_msg_hdr_t *msg,
                             val_value_t *rpcinput,
                             obj_template_t *rpcroot)
{
    val_value_t  *method = NULL;
    status_t      res = NO_ERR;

#ifdef DEBUG
    if (!rpcinput || !rpcroot) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (LOGDEBUG3) {
        log_debug3("\nagt_val_rpc_xpathchk: %s:%s start", 
                   obj_get_mod_name(rpcroot),
                   obj_get_name(rpcroot));
    }

    /* make a dummy tree to align with the XPath code */
    method = val_new_value();
    if (!method) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(method, rpcroot);

    /* add the rpc/method-name/input node */
    val_add_child(rpcinput, method);

    /* just check for when-stmt deletes once, since it is an error */
    res = rpc_when_stmt_check(scb, rpcmsg, msg, method, rpcinput);

    /* check if any must expressions apply to
     * descendent-or-self nodes in the rpcinput node */
    if (res == NO_ERR) {
        res = rpc_must_stmt_check(scb, msg, method, rpcinput);
    }

    /* cleanup fake RPC method parent node */
    val_remove_child(rpcinput);
    val_free_value(method);

    return res;

} /* agt_val_rpc_xpath_check */


/********************************************************************
* FUNCTION agt_val_instance_check
* 
* Check for the proper number of object instances for
* the specified value struct.
* 
* The top-level value set passed cannot represent a choice
* or a case within a choice.
*
* This function is intended for validating PDUs (RPC requests)
* during the PDU processing.  It does not check the instance
* count or must-stmt expressions for any <config> (ncx:root)
* container.  This must be dome with the agt_val_root_check function.
*
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msg == xml_msg_hdr t from msg in progress 
*       == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   valset == val_value_t list, leaf-list, or container to check
*   valroot == root node of the database
*   layer == NCX layer calling this function (for error purposes only)
*
* OUTPUTS:
*   if msg not NULL:
*      msg->msg_errQ may have rpc_err_rec_t structs added to it 
*      which must be freed by the called with the 
*      rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
status_t 
    agt_val_instance_check (ses_cb_t *scb,
                            xml_msg_hdr_t *msg,
                            val_value_t *valset,
                            val_value_t *valroot,
                            ncx_layer_t layer)
{
    return run_instance_check(scb, msg, valset, valroot, layer, TRUE);
    
}  /* agt_val_instance_check */


/********************************************************************
* FUNCTION agt_val_root_check
* 
* !!! Full database validation !!!
* Check for the proper number of object instances for
* the specified configuration database
* Check must and when statements
* Check empty NP containers
* Check choices (selected case, and it is complete)
*
* Tests are divided into 3 groups:
*    A) top-level nodes (child of conceptual <config> root
*    B) parent data node for child instance tests (mand/min/max)
*    C) data node referential tests (must/unique/leaf)
*
* Test pruning
*   The global variable agt_profile.agt_rootflags is used
*   to determine if any type (A) commit tests are needed
*
*   The global variable agt_profile.agt_config_state is
*   used to force complete testing; There are 3 states:
*       AGT_CFG_STATE_INIT : running config has not been
*         validated, or validation-in-progress
*       AGT_CFG_STATE_OK : running config has been validated
*         and it passed all validation checks
*       AGT_CFG_STATE_BAD: running config validation has been
*         attempted and it failed; running config is not valid!
*         The server will shutdown if
*   The target nodes in the undoQ records
* INPUTS:
*   scb == session control block (may be NULL; no session stats)
*   msghdr == XML message header in progress
*        == NULL MEANS NO RPC-ERRORS ARE RECORDED
*   txcb == transaction control block
*   root == val_value_t for the target config being checked
*
* OUTPUTS:
*   if mshdr not NULL:
*      msghdr->msg_errQ may have rpc_err_rec_t 
*      structs added to it which must be freed by the 
*      caller with the rpc_err_free_record function
*
* RETURNS:
*   status of the operation, NO_ERR if no validation errors found
*********************************************************************/
status_t 
    agt_val_root_check (ses_cb_t *scb,
                        xml_msg_hdr_t *msghdr,
                        agt_cfg_transaction_t *txcb,
                        val_value_t *root)
{
    log_debug3("\nagt_val_root_check: start");

    assert ( txcb && "txcb is NULL!" );
    assert ( root && "root is NULL!" );
    assert ( root->obj && "root->obj is NULL!" );
    assert ( obj_is_root(root->obj) && "root obj not config root!" );

    agt_profile_t *profile = agt_get_profile();
    status_t res = NO_ERR, retres = NO_ERR;

    /* the commit check is always run on the root because there
     * are operations such as <validate> and <copy-config> that
     * make it impossible to flag the 'root-dirty' condition 
     * during editing  */
    res = run_obj_commit_tests(profile, scb, msghdr, NULL, root, root,
                               AGT_TEST_ALL_COMMIT_MASK);
    if (res != NO_ERR) {
        profile->agt_load_top_rootcheck_errors = TRUE;
        CHK_EXIT(res, retres);
    }

    /* go through all the commit test objects that might need
     * to be checked for this commit    */
    agt_cfg_commit_test_t *ct = (agt_cfg_commit_test_t *)
        dlq_firstEntry(&profile->agt_commit_testQ);
    for (; ct != NULL; ct = (agt_cfg_commit_test_t *)dlq_nextEntry(ct)) {

        uint32  tests = ct->testflags & AGT_TEST_ALL_COMMIT_MASK;

        /* prune entry that has not changed in a valid config
         * this will only work for <commit> and <edit-config>
         * on the running config, because otherwise there will
         * not be any edits recorded in the txcb->undoQ or any
         * nodes marked dirty in val->flags     */
        if (profile->agt_config_state == AGT_CFG_STATE_OK) {
            tests = prune_obj_commit_tests(txcb, ct, root, tests);
        }

        if (tests == 0) {
            /* no commit tests needed for this node */
            if (LOGDEBUG3) {
                log_debug3("\nrun_root_check: skip commit test %s:%s",
                           obj_get_mod_name(ct->obj),
                           obj_get_name(ct->obj));
            }
            continue;
        }

        res = prep_commit_test_node(scb, msghdr, txcb, ct, root);
        if (res != NO_ERR) {
            CHK_EXIT(res, retres);
            continue;
        }

        /* run all relevant tests on each node in the result set */
        xpath_resnode_t *resnode = xpath_get_first_resnode(ct->result);
        for (; resnode != NULL; resnode = xpath_get_next_resnode(resnode)) {

            val_value_t *valnode = xpath_get_resnode_valptr(resnode);

            res = run_obj_commit_tests(profile, scb, msghdr, ct, valnode, 
                                       root, tests);
            if (res != NO_ERR) {
                valnode->res = res;
                profile->agt_load_rootcheck_errors = TRUE;
                CHK_EXIT(res, retres);
            }
        }

        /* check if any unique tests, which are handled all at once
         * instead of one instance at a time  */
        res = run_obj_unique_tests(scb, msghdr, ct, root);
        if (res != NO_ERR) {
            profile->agt_load_rootcheck_errors = TRUE;
            CHK_EXIT(res, retres);
        }
    }

    log_debug3("\nagt_val_root_check: end");

    return retres;

}  /* agt_val_root_check */


/********************************************************************
* FUNCTION agt_val_validate_write
* 
* Validate the requested <edit-config> write operation
*
* Check all the embedded operation attributes against
* the default-operation and maintained current operation.
*
* Invoke all the user AGT_CB_VALIDATE callbacks for a 
* 'new value' and 'existing value' pairs, for a given write operation, 
*
* These callbacks are invoked bottom-up, so the first step is to
* step through all the child nodes and traverse the
* 'new' data model (from the PDU) all the way to the leaf nodes
*
* The operation attribute is checked against the real data model
* on the way down the tree, and the user callbacks are invoked
* bottom-up on the way back.  This way, the user callbacks can
* share sub-tree validation routines, and perhaps add additional
* <rpc-error> information, based on the context and specific errors
* reported from 'below'.
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write 
*          == NULL for no actual write acess (validate only)
*   valroot == the val_value_t struct containing the root
*              (NCX_BT_CONTAINER, ncx:root)
*              datatype representing the config root with
*              proposed changes to the target
*   editop == requested start-state write operation
*             (usually from the default-operation parameter)
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added to the msg->rpc_errQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    agt_val_validate_write (ses_cb_t  *scb,
                            rpc_msg_t  *msg,
                            cfg_template_t *target,
                            val_value_t  *valroot,
                            op_editop_t  editop)
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );
    assert( valroot && "valroot is NULL!" );
    assert( valroot->obj && "valroot obj is NULL!" );
    assert( obj_is_root(valroot->obj) && "valroot obj not root!" );

    status_t  res = NO_ERR;

    if (target) {
        /* check the lock first */
        res = cfg_ok_to_write(target, scb->sid);
        if (res != NO_ERR) {
            agt_record_error(scb, &msg->mhdr, NCX_LAYER_CONTENT, res,
                             NULL, NCX_NT_NONE, NULL, NCX_NT_VAL, valroot);
            return res;
        }
    }

    /* the <config> root is just a value node of type 'root'
     * traverse all nodes and check the <edit-config> request
     */
    res = handle_callback(AGT_CB_VALIDATE, editop, scb, msg, target, valroot,
                          (target) ? target->root : NULL,
                          (target) ? target->root : valroot);

    return res;

}  /* agt_val_validate_write */


/********************************************************************
* FUNCTION agt_val_apply_write
* 
* Apply the requested write operation
*
* Invoke all the AGT_CB_APPLY callbacks for a 
* source and target and write operation
*
* TBD: support for handling nested parmsets independently
*      of the parent parmset is not supported.  This means
*      that independent parmset instances nested within the
*      parent parmset all have to be applied, or none applied
*
* INPUTS:
*   scb == session control block
*   msg == incoming rpc_msg_t in progress
*   target == cfg_template_t for the config database to write
*   pducfg == the 'root' value struct that represents the
*             tree of changes to apply to the target
*   editop == requested start-state write operation
*             (usually from the default-operation parameter)
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added to the msg->mhsr.errQ
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_val_apply_write (ses_cb_t  *scb,
                         rpc_msg_t  *msg,
                         cfg_template_t *target,
                         val_value_t    *pducfg,
                         op_editop_t  editop)
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );
    assert( msg->rpc_txcb && "txcb is NULL!" );
    assert( target && "target is NULL!" );
    assert( pducfg && "pducfg is NULL!" );
    assert( obj_is_root(pducfg->obj) && "pducfg root is NULL!" );

    /* start with the config root, which is a val_value_t node */
    status_t res = handle_callback(AGT_CB_APPLY, editop, scb, msg, target, 
                                   pducfg, target->root, target->root);

    if (res == NO_ERR) {
        boolean done = FALSE;
        while (!done) {
            /* need to delete all the false when-stmt config objects 
             * and then see if the config is valid.  This is done in
             * candidate or running in apply phase; not evaluated at commit
             * time like must-stmt or unique-stmt    */
            uint32 delcount = 0;
            res = delete_dead_nodes(scb, &msg->mhdr, msg->rpc_txcb, 
                                    target->root, &delcount);
            if (res != NO_ERR || delcount == 0) {
                done = TRUE;
            }
        }
    }

    if (res == NO_ERR && msg->rpc_txcb->rootcheck) {
        res = agt_val_root_check(scb, &msg->mhdr, msg->rpc_txcb, target->root);
    }

    if (res == NO_ERR) {
        /* complete the operation */
        res = handle_callback(AGT_CB_COMMIT, editop, scb, msg, target, 
                              pducfg, target->root, target->root);
    } else {
        /* rollback the operation */
        status_t res2 = handle_callback(AGT_CB_ROLLBACK, editop, scb, msg, 
                                        target, pducfg, target->root, 
                                        target->root);
        if (res2 != NO_ERR) {
            log_error("\nagt_val: Rollback failed (%s)\n",
                      get_error_string(res2));
        }
    }

    return res;

}  /* agt_val_apply_write */


/********************************************************************
* FUNCTION agt_val_apply_commit
* 
* Apply the requested commit operation
*
* Invoke all the AGT_CB_COMMIT callbacks for a 
* source and target and write operation
*
* INPUTS:
*   scb == session control block
*   msg == incoming commit rpc_msg_t in progress
*   source == cfg_template_t for the source (candidate)
*   target == cfg_template_t for the config database to 
*             write (running)
*   save_nvstore == TRUE if the mirrored NV-store
*                   should be updated after the commit is done
*                   FALSE if this is the start of a confirmed-commit
*                   so the NV-store update is deferred
*                   Never save to NV-store if :startup is supported
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhdr.errQ
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_val_apply_commit (ses_cb_t  *scb,
                          rpc_msg_t  *msg,
                          cfg_template_t *source,
                          cfg_template_t *target,
                          boolean save_nvstore)
    
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );
    assert( msg->rpc_txcb && "txcb is NULL!" );
    assert( source && "source is NULL!" );
    assert( target && "target is NULL!" );

    agt_profile_t    *profile = agt_get_profile();
    status_t          res = NO_ERR;

#ifdef ALLOW_SKIP_EMPTY_COMMIT
    /* usually only save if the source config was touched */
    if (!cfg_get_dirty_flag(source)) {
        boolean no_startup_errors = dlq_empty(&target->load_errQ);

        /* no need to merge the candidate into the running
         * config because there are no changes in the candidate
         */
        if (profile->agt_has_startup) {
            /* separate copy-config required to rewrite
             * the startup config file
             */
            if (LOGDEBUG) {
                log_debug("\nSkipping commit, candidate not dirty");
            }
        } else {
            /* there is no distinct startup; need to mirror now;
             * check if the running config had load errors
             * so overwriting it even if the candidate is
             * clean makes sense
             */
            if (no_startup_errors) {
                if (LOGDEBUG) {
                    log_debug("\nSkipping commit, candidate not dirty");
                }
            } else {
                if (LOGINFO) {
                    log_debug("\nSkipping commit, but saving running "
                              "to NV-storage due to load errors");
                }
                res = agt_ncx_cfg_save(target, FALSE);
                if (res != NO_ERR) {
                    /* write to NV-store failed */
                    agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, 
                                     NULL, NCX_NT_CFG, target, NCX_NT_NONE, 
                                     NULL);
                }
            }
        }
        return res;
    }
#endif

    /* check any top-level deletes */
    //res = apply_commit_deletes(scb, msg, target, source->root, target->root);
    if (res == NO_ERR) {
        /* apply all the new and modified nodes */
        res = handle_callback(AGT_CB_APPLY, OP_EDITOP_COMMIT, scb, msg, 
                              target, source->root, target->root, target->root);
    }

    if (res==NO_ERR) {
        /* complete the transaction */
        res = handle_callback(AGT_CB_COMMIT, OP_EDITOP_COMMIT, scb, msg, 
                              target, source->root, target->root, target->root);

        if ( NO_ERR == res ) {
            res = agt_commit_complete();
        }
    } else {
        /* rollback the transaction */
        status_t res2 = handle_callback(AGT_CB_ROLLBACK, OP_EDITOP_COMMIT,
                                        scb, msg, target, source->root,
                                        target->root, target->root);
        if (res2 != NO_ERR) {
            log_error("\nError: rollback failed (%s)", 
                      get_error_string(res2));
        }
    }

    if (res == NO_ERR && !profile->agt_has_startup) {
        if (save_nvstore) {
            res = agt_ncx_cfg_save(target, FALSE);
            if (res != NO_ERR) {
                /* write to NV-store failed */
                agt_record_error(scb,&msg->mhdr, NCX_LAYER_OPERATION, res, 
                                 NULL, NCX_NT_CFG, target, NCX_NT_NONE, NULL);
            } else {
                /* don't clear the dirty flags in running
                 * unless the save to file  worked
                 */
                val_clean_tree(target->root);
            }
        } else if (LOGDEBUG2) {
            log_debug2("\nagt_val: defer NV-save after commit "
                       "until confirmed");
        }
    }

    return res;

}  /* agt_val_apply_commit */


/********************************************************************
* FUNCTION agt_val_check_commit_edits
* 
* Check if the requested commit operation
* would cause any ACM or partial lock violations 
* in the running config
* Invoke all the AGT_CB_VALIDATE callbacks for a 
* source and target and write operation
*
* !!! Only works for the <commit> operation for applying
* !!! the candidate to the running config; relies on value
* !!! flags VAL_FL_SUBTREE_DIRTY and VAL_FL_DIRTY
*
* INPUTS:
*   scb == session control block
*   msg == incoming commit rpc_msg_t in progress
*   source == cfg_template_t for the source (candidate)
*   target == cfg_template_t for the config database to 
*             write (running)
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhdr.errQ
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_val_check_commit_edits (ses_cb_t  *scb,
                                rpc_msg_t  *msg,
                                cfg_template_t *source,
                                cfg_template_t *target)
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );
    assert( msg->rpc_txcb && "txcb is NULL!" );
    assert( source && "source is NULL!" );
    assert( target && "target is NULL!" );

    /* usually only save if the source config was touched */
    if (!cfg_get_dirty_flag(source)) {
        /* no need to check for partial-lock violations */
        return NO_ERR;
    }

    status_t res = handle_callback(AGT_CB_VALIDATE, OP_EDITOP_COMMIT, 
                                   scb, msg, target, source->root, 
                                   target->root, target->root);
    return res;

}  /* agt_val_check_commit_edits */


/********************************************************************
* FUNCTION agt_val_delete_dead_nodes
* 
* Mark nodes deleted for each false when-stmt from <validate>
* INPUTS:
*   scb == session control block
*   msg == incoming validate rpc_msg_t in progress
*   root == value tree to check for deletions
*
* OUTPUTS:
*   rpc_err_rec_t structs may be malloced and added 
*   to the msg->mhdr.errQ
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_val_delete_dead_nodes (ses_cb_t  *scb,
                               rpc_msg_t  *msg,
                               val_value_t *root)
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );
    assert( msg->rpc_txcb && "txcb is NULL!" );
    assert( root && "root is NULL!" );

    status_t res = NO_ERR;
    boolean done = FALSE;
    while (!done) {
        /* need to delete all the false when-stmt config objects 
         * and then see if the config is valid.  This is done in
         * candidate or running in apply phase; not evaluated at commit
         * time like must-stmt or unique-stmt    */
        uint32 delcount = 0;
        res = delete_dead_nodes(scb, &msg->mhdr, msg->rpc_txcb, root, 
                                &delcount);
        if (res != NO_ERR || delcount == 0) {
            done = TRUE;
        }
    }

    return res;

} /* agt_val_delete_dead_nodes */


/********************************************************************
* FUNCTION agt_val_add_module_commit_tests
* 
* !!! Initialize module data node validation !!!
* !!! Must call after all modules are initially loaded !!!
* 
* Find all the data node objects in the specified module
* that need some sort database referential integrity test
*
* !!! dynamic features are not supported.  Any disabled objects
* due to false if-feature stmts will be skipped.  No support yet
* to add or remove tests from the commit_testQ when obj_is_enabled()
* return value changes.
*
* INPUTS:
*  mod == module to check and add tests
*
* SIDE EFFECTS:
*  agt_profile->commit_testQ will contain an agt_cfg_commit_test_t struct
*  for each object found with commit-time validation tests
*   
* RETURNS:
*   status of the operation, NO_ERR unless internal errors found
*   or malloc error
*********************************************************************/
status_t 
    agt_val_add_module_commit_tests (ncx_module_t *mod)
{
    assert( mod && "mod is NULL!" );

    agt_profile_t *profile = agt_get_profile();
    status_t res = NO_ERR;

    obj_template_t *obj = ncx_get_first_data_object(mod);
    for (; obj != NULL; obj = ncx_get_next_data_object(mod, obj)) {

        res = add_obj_commit_tests(obj, &profile->agt_commit_testQ,
                                   &profile->agt_rootflags);
        
        if (res != NO_ERR) {
            return res;
        }
    }
    return res;

} /* agt_val_add_module_commit_tests */


/********************************************************************
* FUNCTION agt_val_init_commit_tests
* 
* !!! Initialize full database validation !!!
* !!! Must call after all modules are initially loaded !!!
* !!! Must be called before load_running_config is called !!!
* 
* Find all the data node objects that need some sort
* database referential integrity test
* For :candidate this is done during the
* validate of the <commit> RPC
* For :writable-running, this is done during
* <edit-config> or <copy-config>
*
* SIDE EFFECTS:
*    all commit_test_t records created are stored in the
*    agt_profile->commit_testQ
*
* RETURNS:
*   status of the operation, NO_ERR unless internal errors found
*   or malloc error
*********************************************************************/
status_t 
    agt_val_init_commit_tests (void)
{
    status_t        res = NO_ERR;
    
    /* check all the modules in the system for top-level database objects */
    ncx_module_t *mod = ncx_get_first_module();
    for (; mod != NULL; mod = ncx_get_next_module(mod)) {

        res = agt_val_add_module_commit_tests(mod);
        if (res != NO_ERR) {
            return res;
        }
    }

    return res;
    
}  /* agt_val_init_commit_tests */


/* END file agt_val.c */

