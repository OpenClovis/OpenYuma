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
#ifndef _H_agt_acm
#define _H_agt_acm

/*  FILE: agt_acm.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server Access Control handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
03-feb-06    abb      Begun
14-may-09    abb      add per-msg cache to speed up performance
*/

#include <xmlstring.h>

#ifndef _H_agt
#include "agt.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_obj
#include "obj.h"
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

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
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

/* flags fields for the agt_acm_cache_t */
#define FL_ACM_DEFREAD_SET      bit0
#define FL_ACM_DEFREAD_OK       bit1
#define FL_ACM_DEFWRITE_SET     bit2
#define FL_ACM_DEFWRITE_OK      bit3
#define FL_ACM_DEFEXEC_SET      bit4
#define FL_ACM_DEFEXEC_OK       bit5
#define FL_ACM_MODRULES_SET     bit6
#define FL_ACM_DATARULES_SET    bit7
#define FL_ACM_CACHE_VALID      bit8


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* 1 group that the user is a member */
typedef struct agt_acm_group_t_ {
    dlq_hdr_t         qhdr;
    xmlns_id_t        groupnsid;
    const xmlChar    *groupname;
} agt_acm_group_t;

/* list of group identities that the user is a member */
typedef struct agt_acm_usergroups_t_ {
    dlq_hdr_t         qhdr;
    xmlChar          *username;
    dlq_hdr_t         groupQ;   /* Q of agt_acm_group_t */
} agt_acm_usergroups_t;

/* cache for 1 NACM moduleRule entry */
typedef struct agt_acm_modrule_t_ {
    dlq_hdr_t       qhdr;
    xmlns_id_t      nsid;
    val_value_t    *modrule;  /* back-ptr */
} agt_acm_modrule_t;

/* cache for 1 NACM dataRule entry */
typedef struct agt_acm_datarule_t_ {
    dlq_hdr_t           qhdr;
    xpath_pcb_t        *pcb;
    xpath_result_t     *result;
    val_value_t        *datarule;   /* back-ptr */
} agt_acm_datarule_t;

/* NACM cache control block */
typedef struct agt_acm_cache_t_ {
    agt_acm_usergroups_t *usergroups;
    val_value_t          *nacmroot;     /* back-ptr */
    val_value_t          *rulesval;     /* back-ptr */
    uint32                groupcnt;
    uint32                flags;
    agt_acmode_t          mode;
    dlq_hdr_t             modruleQ;     /* Q of agt_acm_modrule_t */
    dlq_hdr_t             dataruleQ;    /* Q of agt_acm_datarule_t */
} agt_acm_cache_t;

    
/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/

/********************************************************************
* FUNCTION agt_acm_init
* 
* Initialize the NETCONF Server access control module
* 
* INPUTS:
*   none
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    agt_acm_init (void);


/********************************************************************
* FUNCTION agt_acm_init2
* 
* Phase 2:
*   Initialize the nacm.yang configuration data structures
* 
* INPUTS:
*   none
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    agt_acm_init2 (void);


/********************************************************************
* FUNCTION agt_acm_cleanup
*
* Cleanup the NETCONF Server access control module
* 
*********************************************************************/
extern void 
    agt_acm_cleanup (void);


/********************************************************************
* FUNCTION agt_acm_rpc_allowed
*
* Check if the specified user is allowed to invoke an RPC
* 
* INPUTS:
*   msg == XML header in incoming message in progress
*   user == user name string
*   rpcobj == obj_template_t for the RPC method to check
*
* RETURNS:
*   TRUE if user allowed invoke this RPC; FALSE otherwise
*********************************************************************/
extern boolean 
    agt_acm_rpc_allowed (xml_msg_hdr_t *msg,
			 const xmlChar *user,
			 const obj_template_t *rpcobj);


/********************************************************************
* FUNCTION agt_acm_notif_allowed
*
* Check if the specified user is allowed to receive
* a notification event
* 
* INPUTS:
*   user == user name string
*   notifobj == obj_template_t for the notification event to check
*
* RETURNS:
*   TRUE if user allowed receive this notification event;
*   FALSE otherwise
*********************************************************************/
extern boolean 
    agt_acm_notif_allowed (const xmlChar *user,
                           const obj_template_t *notifobj);


/********************************************************************
* FUNCTION agt_acm_val_write_allowed
*
* Check if the specified user is allowed to access a value node
* The val->obj template will be checked against the val->editop
* requested access and the user's configured max-access
* 
* INPUTS:
*   msg == XML header from incoming message in progress
*   user == user name string
*   newval  == val_value_t in progress to check
*                (may be NULL, if curval set)
*   curval  == val_value_t in progress to check
*                (may be NULL, if newval set)
*   editop == requested CRUD operation
*
* RETURNS:
*   TRUE if user allowed this level of access to the value node
*********************************************************************/
extern boolean 
    agt_acm_val_write_allowed (xml_msg_hdr_t *msg,
			       const xmlChar *user,
			       const val_value_t *newval,
			       const val_value_t *curval,
                               op_editop_t editop);


/********************************************************************
* FUNCTION agt_acm_val_read_allowed
*
* Check if the specified user is allowed to read a value node
* 
* INPUTS:
*   msg == XML header from incoming message in progress
*   user == user name string
*   val  == val_value_t in progress to check
*
* RETURNS:
*   TRUE if user allowed read access to the value node
*********************************************************************/
extern boolean 
    agt_acm_val_read_allowed (xml_msg_hdr_t *msg,
			      const xmlChar *user,
			      const val_value_t *val);


/********************************************************************
* FUNCTION agt_acm_init_msg_cache
*
* Malloc and initialize an agt_acm_cache_t struct
* and attach it to the incoming message
*
* INPUTS:
*   scb == session control block to use
*   msg == message to use
*
* OUTPUTS:
*   scb->acm_cache pointer may be set, if it was NULL
*   msg->acm_cache pointer set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_acm_init_msg_cache (ses_cb_t *scb,
                            xml_msg_hdr_t *msg);


/********************************************************************
* FUNCTION agt_acm_clear_msg_cache
*
* Clear an agt_acm_cache_t struct
* attached to the specified message
*
* INPUTS:
*   msg == message to use
*
* OUTPUTS:
*   msg->acm_cache pointer is freed and set to NULL
*
*********************************************************************/
extern void
    agt_acm_clear_msg_cache (xml_msg_hdr_t *msg);


/********************************************************************
* FUNCTION agt_acm_clear_session_cache
*
* Clear an agt_acm_cache_t struct in a session control block
*
* INPUTS:
*   scb == sesion control block to use
*
* OUTPUTS:
*   scb->acm_cache pointer is freed and set to NULL
*
*********************************************************************/
extern void agt_acm_clear_session_cache (ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_acm_invalidate_session_cache
*
* Mark an agt_acm_cache_t struct in a session control block
* as invalid so it will be refreshed next use
*
* INPUTS:
*   scb == sesion control block to use
*
*********************************************************************/
extern void agt_acm_invalidate_session_cache (ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_acm_session_cache_valid
*
* Check if the specified session NACM cache is valid
*
* INPUTS:
*   scb == session to check
*
* RETURNS:
*   TRUE if session acm_cache is valid
*   FALSE if session acm_cache is NULL or not valid
*********************************************************************/
extern boolean
    agt_acm_session_cache_valid (const ses_cb_t *scb);


/********************************************************************
* FUNCTION agt_acm_session_is_superuser
*
* Check if the specified session is the superuser
*
* INPUTS:
*   scb == session to check
*
* RETURNS:
*   TRUE if session is for the superuser
*   FALSE if session is not for the superuser
*********************************************************************/
extern boolean
    agt_acm_session_is_superuser (const ses_cb_t *scb);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_acm */
