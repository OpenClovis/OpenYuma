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
#ifndef _H_mgr
#define _H_mgr

/*  FILE: mgr.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Manager message handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
03-feb-06    abb      Begun

*/

/* used by the manager for the SSH2 interface */
#include <libssh2.h>

#ifndef _H_cap
#include "cap.h"
#endif

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_var
#include "var.h"
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

#define MGR_MAX_REQUEST_ID 0xfffffffe

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/


/* extension to the ses_cb_t for a manager session */
typedef struct mgr_scb_t_ {

    /* agent info */
    ncx_agttarg_t   targtyp;
    ncx_agtstart_t  starttyp;
    cap_list_t      caplist;
    uint32          agtsid;   /* agent assigned session ID */
    boolean         closed;

    /* temp directory for downloaded modules */
    ncxmod_temp_progcb_t *temp_progcb;
    ncxmod_temp_sescb_t  *temp_sescb;
    dlq_hdr_t             temp_modQ;   /* Q of ncx_module_t */
    ncx_list_t            temp_ync_features;  

    /* running config cached info */
    val_value_t    *root;
    xmlChar        *chtime;
    val_value_t    *lastroot;
    xmlChar        *lastchtime;

    /* transport info */
    xmlChar         *target;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;
    int              returncode;

    /* RPC request info */
    uint32           next_id;
    dlq_hdr_t        reqQ;

    /* XPath variable binding callback function */
    xpath_getvar_fn_t   getvar_fn;

} mgr_scb_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION mgr_init
* 
* Initialize the Manager Library
* 
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    mgr_init (void);


/********************************************************************
* FUNCTION mgr_cleanup
*
* Cleanup the Manager Library
* 
*********************************************************************/
extern void 
    mgr_cleanup (void);


/********************************************************************
* FUNCTION mgr_new_scb
* 
* Malloc and Initialize the Manager Session Control Block
* 
* RETURNS:
*   manager session control block struct or NULL if malloc error
*********************************************************************/
extern mgr_scb_t *
    mgr_new_scb (void);


/********************************************************************
* FUNCTION mgr_init_scb
* 
* Initialize the Manager Session Control Block
* 
* INPUTS:
*   mscb == manager session control block struct to initialize
*
*********************************************************************/
extern void
    mgr_init_scb (mgr_scb_t *mscb);


/********************************************************************
* FUNCTION mgr_free_scb
* 
* Clean and Free a Manager Session Control Block
* 
* INPUTS:
*   mscb == manager session control block struct to free
*********************************************************************/
extern void
    mgr_free_scb (mgr_scb_t *mscb);


/********************************************************************
* FUNCTION mgr_clean_scb
* 
* Clean a Manager Session Control Block
* 
* INPUTS:
*   mscb == manager session control block struct to clean
*********************************************************************/
extern void
    mgr_clean_scb (mgr_scb_t *mscb);


/********************************************************************
* FUNCTION mgr_request_shutdown
* 
* Request a manager shutdown
* 
*********************************************************************/
extern void
    mgr_request_shutdown (void);


/********************************************************************
* FUNCTION mgr_shutdown_requested
* 
* Check if a manager shutdown is in progress
* 
* RETURNS:
*    TRUE if shutdown mode has been started
*
*********************************************************************/
extern boolean
    mgr_shutdown_requested (void);


/********************************************************************
* FUNCTION mgr_set_getvar_fn
* 
* Set the getvar_fn callback for the session
* 
* INPUTS:
*   sid == manager session ID to use
*   getvar_fn == function to use 
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    mgr_set_getvar_fn (ses_id_t  sid,
                       xpath_getvar_fn_t getvar_fn);


/********************************************************************
* FUNCTION mgr_print_libssh2_version
* 
* Print the version of libssh2 used by the manager
* Indenting must already be done!
*
* INPUTS:
*   tolog == TRUE to print to log; FALSE to print to stdout
*********************************************************************/
extern void
    mgr_print_libssh2_version (boolean tolog);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_mgr */
