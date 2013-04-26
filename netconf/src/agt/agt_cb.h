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
#ifndef _H_agt_cb
#define _H_agt_cb

/*  FILE: agt_cb.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server Data Model callback handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
16-apr-07    abb      Begun; split out from agt_ps.h
01-aug-08    abb      Remove NCX specific stuff; YANG only now
*/

#include "agt.h"
#include "op.h"
#include "rpc.h"
#include "ses.h"
#include "status.h"
#include "val.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/
/* symbolic tags for agt_cb_register_callback 'forall' boolean */
#define FORALL  TRUE
#define FORONE  FALSE


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* Callback function for server object handler 
 * Used to provide a callback sub-mode for
 * a specific named object
 * 
 * INPUTS:
 *   scb == session control block making the request
 *   msg == incoming rpc_msg_t in progress
 *   cbtyp == reason for the callback
 *   editop == the parent edit-config operation type, which
 *             is also used for all other callbacks
 *             that operate on objects
 *   newval == container object holding the proposed changes to
 *           apply to the current config, depending on
 *           the editop value. Will not be NULL.
 *   curval == current container values from the <running> 
 *           or <candidate> configuration, if any. Could be NULL
 *           for create and other operations.
 *
 * RETURNS:
 *    status:
 */
typedef status_t 
    (*agt_cb_fn_t) (ses_cb_t  *scb,
		    rpc_msg_t *msg,
		    agt_cbtyp_t cbtyp,
		    op_editop_t  editop,
		    val_value_t  *newval,
		    val_value_t  *curval);


/* set of server object callback functions */
typedef struct agt_cb_fnset_t_ {
    agt_cb_fn_t    cbfn[AGT_NUM_CB];
} agt_cb_fnset_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/

/********************************************************************
* FUNCTION agt_cb_init
* 
* Init the server callback module
*
*********************************************************************/
extern void
    agt_cb_init (void);


/********************************************************************
* FUNCTION agt_cb_cleanup
* 
* Cleanup the server callback module
*
*********************************************************************/
extern void
    agt_cb_cleanup (void);


/********************************************************************
* FUNCTION agt_cb_register_callback
* 
* Register an object specific callback function
* use the same fn for all callback phases 
* all phases will be invoked
*
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
extern status_t 
    agt_cb_register_callback (const xmlChar *modname,
			      const xmlChar *defpath,
			      const xmlChar *version,
			      const agt_cb_fn_t cbfn);


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
extern status_t 
    agt_cb_register_callbacks (const xmlChar *modname,
			       const xmlChar *defpath,
			       const xmlChar *version,
			       const agt_cb_fnset_t *cbfnset);


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
extern void
    agt_cb_unregister_callbacks (const xmlChar *modname,
				 const xmlChar *defpath);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_cb */
