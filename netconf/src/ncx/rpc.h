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
#ifndef _H_rpc
#define _H_rpc
/*  FILE: rpc.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol remote procedure call common definitions
    This module is only used by the agent at this time

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
01-may-05    abb      Begun.
*/

#include <xmlstring.h>


#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
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

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/
#define RPC_STR_REQ     "rpc"
#define RPC_STR_RPY     "rpc-reply"
#define RPC_STR_ERR     "rpc-error"
#define RPC_STR_MSG_ID  "message-id"
#define RPC_STR_GRP_ID  "group-id"


#define RPC_ERR_QUEUE(MSG) &(MSG)->mhdr.errQ


/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/


/* Type of the <rpc-reply> data source */
typedef enum rpc_data_t_ {
    RPC_DATA_NONE,
    RPC_DATA_STD,
    RPC_DATA_YANG
} rpc_data_t;


/* NETCONF Server and Client RPC Request/Reply Message Header */
typedef struct rpc_msg_t_ {
    dlq_hdr_t        qhdr;

    /* generic XML message header */
    xml_msg_hdr_t    mhdr; 

    /* incoming: top-level rpc element data */
    xml_attrs_t     *rpc_in_attrs;     /* borrowed from <rpc> elem */

    /* incoming: 
     * 2nd-level method name element data, used in agt_output_filter
     * to check get or get-config; cannot import obj.h here!
     */
    struct obj_template_t_ *rpc_method; 

    /* incoming: SERVER RPC processing state */
    int              rpc_agt_state;        /* agt_rpc_phase_t */
    op_errop_t       rpc_err_option;
    op_editop_t      rpc_top_editop;
    val_value_t     *rpc_input;

    /* incoming:
     * hooks for method routines to save context or whatever 
     */
    void           *rpc_user1;
    void           *rpc_user2;
    uint32          rpc_returncode;   /* for nested callbacks */

    /* incoming: get method reply handling builtin 
     * If the rpc_datacb is non-NULL then it will be used as a
     * callback to generate the rpc-reply inline, instead of
     * buffering the output.  
     * The rpc_data and rpc_filter parameters are optionally used
     * by the rpc_datacb function to generate a reply.
     */
    rpc_data_t      rpc_data_type;          /* type of data reply */
    void           *rpc_datacb;              /* agt_rpc_data_cb_t */
    dlq_hdr_t       rpc_dataQ;       /* data reply: Q of val_value_t */
    op_filter_t     rpc_filter;        /* backptrs for get* methods */

    /* incoming: agent database edit transaction control block
     * must be freed by an upper layer if set to malloced data
     */
    struct agt_cfg_transaction_t_ *rpc_txcb;

    /* load-config parse-error and --startup-error=continue
     * flag if the val_purge_errors_from_root function is needed
     */
    boolean         rpc_parse_errors;

} rpc_msg_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION rpc_new_msg
*
* Malloc and initialize a new rpc_msg_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
extern rpc_msg_t * 
    rpc_new_msg (void);


/********************************************************************
* FUNCTION rpc_new_out_msg
*
* Malloc and initialize a new rpc_msg_t struct for output
* or for dummy use
*
* INPUTS:
*   none
* RETURNS:
*   pointer to struct or NULL or memory error
*********************************************************************/
extern rpc_msg_t * 
    rpc_new_out_msg (void);


/********************************************************************
* FUNCTION rpc_free_msg
*
* Free all the memory used by the specified rpc_msg_t
*
* INPUTS:
*   msg == rpc_msg_t to clean and delete
* RETURNS:
*   none
*********************************************************************/
extern void 
    rpc_free_msg (rpc_msg_t *msg);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_rpc */
