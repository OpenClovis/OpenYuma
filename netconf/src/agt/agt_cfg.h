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
#ifndef _H_agt_cfg
#define _H_agt_cfg

/*  FILE: agt_cfg.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Manage Server configuration edit transactions

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22dec11      abb      begun; split out from ncx/rpc ...

*/

#ifndef _H_agt
#include "agt.h"
#endif

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h.h"
#endif

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/


/* classify the config edit action type */
typedef enum agt_cfg_edit_action_t_ {
    AGT_CFG_EDIT_ACTION_NONE,
    AGT_CFG_EDIT_ACTION_ADD,
    AGT_CFG_EDIT_ACTION_SET,
    AGT_CFG_EDIT_ACTION_MOVE,
    AGT_CFG_EDIT_ACTION_REPLACE,
    AGT_CFG_EDIT_ACTION_DELETE,
    AGT_CFG_EDIT_ACTION_DELETE_DEFAULT
} agt_cfg_edit_action_t;


/* classify the config edit type */
typedef enum agt_cfg_edit_type_t_ {
    AGT_CFG_EDIT_TYPE_NONE,
    AGT_CFG_EDIT_TYPE_FULL,   // REPLACE = full database replace
    AGT_CFG_EDIT_TYPE_PARTIAL // REPLACE = subtree replace
} agt_cfg_edit_type_t;


/* struct representing 1 configuration database edit transaction
 * - A NETCONF transaction is any write attempt to a specific config.
 * 
 * - Each <edit-config> on candidate or running is a separate transaction.
 *
 * - A commit is a separate transaction, and all individual edits to
 *   candidate are ignored, and a new transaction change set is calculated.
 *
 * - Save from running to startup is a separate transaction.
 *
 * - The validate operation (or edit-config in test-only mode)
 *   will cause a transaction ID to be used, even though no writes
 *   are ever done.  The txid must be the same for validate and
 *   apply/commit phases.
 *
 * Each transaction gets a new auto-incremented transaction ID
 * which is saved in a file across reboots.  It is only updated
 * when the NV-storage version of the config is written, or upon
 * a clean exit. This means that a program or computer crash could cause
 * transaction IDs to be reused upon a restart, that were previously 
 * used for candidate or running.
 *
 * Incremental rollback based on undoQ contents is still TBD,
 * so qhdr and last_transaction_id are not really used yet
 */
typedef struct agt_cfg_transaction_t_ {
    dlq_hdr_t            qhdr;
    cfg_transaction_id_t txid;
    agt_cfg_edit_type_t  edit_type;
    ncx_cfg_t            cfg_id;
    status_t             apply_res;
    status_t             commit_res;
    status_t             rollback_res;
    boolean              start_bad;   // running config has errors
    boolean              rootcheck;
    boolean              commitcheck;
    boolean              is_validate;

    /* each distinct effective edit point in the data tree will
     * have a separate undo record in the undoQ   */
    dlq_hdr_t            undoQ;       /* Q of agt_cfg_undo_rec_t */

    /* TBD: this is redundant and can be derived from the undoQ
     * contains edit record highlights used in the sysConfigChange
     * notification and netconfd audit log    */
    dlq_hdr_t            auditQ;      /* Q of agt_cfg_audit_rec_t */

    /* contains nodes marked as deleted by the delete_dead_nodes test */
    dlq_hdr_t            deadnodeQ;   /* Q of agt_cfg_nodeptr_t */
} agt_cfg_transaction_t;


/* struct of pointers to extra deleted objects
 * This occurs when a new case within a choice is selected
 * and any nodes from other cases are deleted
 */
typedef struct agt_cfg_nodeptr_t_ {
    dlq_hdr_t       qhdr;
    val_value_t    *node; 
} agt_cfg_nodeptr_t;


/* struct of params to undo an edit opration
 * The actual nodes used depend on the edit operation value
 */
typedef struct agt_cfg_undo_rec_t_ {
    dlq_hdr_t       qhdr;
    boolean         free_curnode;
    op_editop_t     editop;
    agt_cfg_edit_action_t edit_action;
    val_value_t    *newnode; 
    val_value_t    *newnode_marker; 
    val_value_t    *curnode;
    val_value_t    *curnode_marker; 
    val_value_t    *curnode_clone;
    val_value_t    *parentnode; 
    dlq_hdr_t       extra_deleteQ;   /* Q of agt_cfg_nodeptr_t */
    status_t        apply_res;
    status_t        commit_res;
    status_t        rollback_res;

} agt_cfg_undo_rec_t;


/* struct of params to use when generating sysConfigChange
 * notification.
 */
typedef struct agt_cfg_audit_rec_t_ {
    dlq_hdr_t       qhdr;
    xmlChar        *target;
    op_editop_t     editop;
} agt_cfg_audit_rec_t;


/* struct for the commit-time tests for a single object */
typedef struct agt_cfg_commit_test_t_ {
    dlq_hdr_t          qhdr;
    obj_template_t    *obj;
    xpath_pcb_t       *objpcb;
    xpath_result_t    *result;
    cfg_transaction_id_t result_txid;
    ncx_btype_t        btyp;
    uint32             testflags;  /* AGT_TEST_FL_FOO bits */
} agt_cfg_commit_test_t;



/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_cfg_new_transaction
*
* Malloc and initialize agt_cfg_transaction_t struct
*
* INPUTS:
*    cfgid == config ID to use
*    edit_type == database edit type
*    rootcheck == TRUE if root_check needs to be done before commit
*                 during agt_val_apply_write; FALSE if it is done
*                 manually via agt_val_root_check
*    is_validate == TRUE if this is a <validate> operation
*                   the target data nodes will not be altered
*                   at all during the transaction
*                   FALSE if this is some sort of real edit
*    res == address of return status
* OUTPUTS:
*    *res == return status
* RETURNS:
*    malloced transaction struct; need to call agt_cfg_free_transaction
*********************************************************************/
agt_cfg_transaction_t *
    agt_cfg_new_transaction (ncx_cfg_t cfgid,
                             agt_cfg_edit_type_t edit_type,
                             boolean rootcheck,
                             boolean is_validate,
                             status_t *res);


/********************************************************************
* FUNCTION agt_cfg_free_transaction
*
* Clean and free a agt_cfg_transaction_t struct
*
* INPUTS:
*    txcb == transaction struct to free
*********************************************************************/
extern void
    agt_cfg_free_transaction (agt_cfg_transaction_t *txcb);


/********************************************************************
* FUNCTION agt_cfg_init_transactions
*
* Initialize the transaction ID functionality
*
* INPUTS:
*   txidfile == TXID filespec to use
*   foundfile == TRUE if file found; FALSE if this is first write
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_cfg_init_transactions (const xmlChar *txidfile,
                               boolean foundfile);


/********************************************************************
* FUNCTION agt_cfg_txid_in_progress
*
* Return the ID of the current transaction ID in progress
*
* INPUTS:
*    cfgid == config ID to check
* RETURNS:
*    txid of transaction in progress or 0 if none
*********************************************************************/
extern cfg_transaction_id_t
    agt_cfg_txid_in_progress (ncx_cfg_t cfgid);


/********************************************************************
* FUNCTION agt_cfg_new_undorec
*
* Malloc and initialize a new agt_cfg_undo_rec_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
extern agt_cfg_undo_rec_t *
    agt_cfg_new_undorec (void);


/********************************************************************
* FUNCTION agt_cfg_init_undorec
*
* Initialize a new agt_cfg_undo_rec_t struct
*
* INPUTS:
*   undo == cfg_undo_rec_t memory to initialize
* RETURNS:
*   none
*********************************************************************/
extern void
    agt_cfg_init_undorec (agt_cfg_undo_rec_t *undo);


/********************************************************************
* FUNCTION agt_cfg_free_undorec
*
* Free all the memory used by the specified agt_cfg_undo_rec_t
*
* INPUTS:
*   undo == agt_cfg_undo_rec_t to clean and delete
* RETURNS:
*   none
*********************************************************************/
extern void
    agt_cfg_free_undorec (agt_cfg_undo_rec_t *undorec);


/********************************************************************
* FUNCTION agt_cfg_clean_undorec
*
* Clean all the memory used by the specified agt_cfg_undo_rec_t
* but do not free the struct itself
*
*  !!! The caller must free internal pointers that were malloced
*  !!! instead of copied.  This function does not check them!!!
*  
* INPUTS:
*   undo == agt_cfg_undo_rec_t to clean
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_cfg_clean_undorec (agt_cfg_undo_rec_t *undo);


/********************************************************************
* FUNCTION agt_cfg_new_auditrec
*
* Malloc and initialize a new agt_cfg_audit_rec_t struct
*
* INPUTS:
*   target == i-i string of edit target
*   editop == edit operation enum
*
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
extern agt_cfg_audit_rec_t *
    agt_cfg_new_auditrec (const xmlChar *target,
                          op_editop_t editop);


/********************************************************************
* FUNCTION agt_cfg_free_auditrec
*
* Free all the memory used by the specified agt_cfg_audit_rec_t
*
* INPUTS:
*   auditrec == agt_cfg_audit_rec_t to clean and delete
*
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_cfg_free_auditrec (agt_cfg_audit_rec_t *auditrec);


/********************************************************************
* FUNCTION agt_cfg_new_commit_test
*
* Malloc a agt_cfg_commit_test_t struct
*
* RETURNS:
*   malloced commit test struct or NULL if ERR_INTERNAL_MEM
*********************************************************************/
extern agt_cfg_commit_test_t *
    agt_cfg_new_commit_test (void);


/********************************************************************
* FUNCTION agt_cfg_free_commit_test
*
* Free a previously malloced agt_cfg_commit_test_t struct
*
* INPUTS:
*    commit_test == commit test record to free
*
*********************************************************************/
extern void
    agt_cfg_free_commit_test (agt_cfg_commit_test_t *commit_test);


/********************************************************************
* FUNCTION agt_cfg_new_nodeptr
*
* Malloc and initialize a new agt_cfg_nodeptr_t struct
*
* INPUTS:
*   node == node to point at
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
extern agt_cfg_nodeptr_t *
    agt_cfg_new_nodeptr (val_value_t *node);


/********************************************************************
* FUNCTION agt_cfg_free_nodeptr
*
* Free all the memory used by the specified agt_cfg_nodeptr_t
*
* INPUTS:
*   nodeptr == agt_cfg_nodeptr_t to clean and delete
*********************************************************************/
extern void 
    agt_cfg_free_nodeptr (agt_cfg_nodeptr_t *nodeptr);

/********************************************************************
* FUNCTION agt_cfg_update_txid
*
* Update the TXID file with the latest value transaction ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    agt_cfg_update_txid (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_cfg */
