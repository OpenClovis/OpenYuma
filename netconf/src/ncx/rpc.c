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
/*  FILE: rpc.c

   NETCONF RPC Operations

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
09nov05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>
#include  <assert.h>

#include  "procdefs.h"
#include  "cfg.h"
#include  "ncx.h"
#include  "obj.h"
#include  "op.h"
#include  "rpc.h"
#include  "rpc_err.h"
#include  "xmlns.h"
#include  "xml_msg.h"
#include  "xml_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


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


/******************* E X T E R N   F U N C T I O N S ***************/


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
rpc_msg_t *
    rpc_new_msg (void)
{
    rpc_msg_t *msg;

    msg = m__getObj(rpc_msg_t);
    if (!msg) {
        return NULL;
    }

    memset(msg, 0x0, sizeof(rpc_msg_t));
    xml_msg_init_hdr(&msg->mhdr);
    dlq_createSQue(&msg->rpc_dataQ);

    msg->rpc_input = val_new_value();
    if (!msg->rpc_input) {
        rpc_free_msg(msg);
        return NULL;
    }

    msg->rpc_top_editop = OP_EDITOP_MERGE;

    return msg;

} /* rpc_new_msg */


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
rpc_msg_t *
    rpc_new_out_msg (void)
{
    rpc_msg_t *msg;

    msg = rpc_new_msg();
    if (!msg) {
        return NULL;
    }

    msg->rpc_in_attrs = NULL;

    return msg;

} /* rpc_new_out_msg */


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
void rpc_free_msg (rpc_msg_t *msg)
{
    if (!msg) {
        return;
    }

    xml_msg_clean_hdr(&msg->mhdr);

    //msg->rpc_in_attrs = NULL;
    //msg->rpc_method = NULL;
    //msg->rpc_agt_state = 0;

    /* clean input parameter set */
    if (msg->rpc_input) {
        val_free_value(msg->rpc_input);
    }
    //msg->rpc_user1 = NULL;
    //msg->rpc_user2 = NULL;

    //msg->rpc_filter.op_filtyp = OP_FILTER_NONE;
    //msg->rpc_filter.op_filter = NULL;
    //msg->rpc_datacb = NULL;

    /* clean data queue */
    while (!dlq_empty(&msg->rpc_dataQ)) {
        val_value_t *val = (val_value_t *)dlq_deque(&msg->rpc_dataQ);
        val_free_value(val);
    }

    m__free(msg);

} /* rpc_free_msg */


/* END file rpc.c */
