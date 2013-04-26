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
#ifndef _H_agt_ncx
#define _H_agt_ncx

/*  FILE: agt_ncx.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server standard method routines

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
04-feb-06    abb      Begun

*/

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
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

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/

/********************************************************************
* FUNCTION agt_ncx_init
* 
* Initialize the NETCONF Server standard method routines
* 
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    agt_ncx_init (void);


/********************************************************************
* FUNCTION agt_ncx_cleanup
*
* Cleanup the NETCONF Server standard method routines
* 
*********************************************************************/
extern void 
    agt_ncx_cleanup (void);


/********************************************************************
* FUNCTION agt_ncx_cfg_load
*
* Load the specifed config from the indicated source
* Called just once from agt.c at boot or reload time!
*
* This function should only be used to load an empty config
* in CFG_ST_INIT state
*
* INPUTS:
*    cfg = Config template to load data into
*    cfgloc == enum for the config source location
*    cfgparm == string parameter used in different ways 
*               depending on the cfgloc value
*     For cfgloc==CFG_LOC_FILE, this is a system-dependent filespec
* 
* OUTPUTS:
*    errQ contains any rpc_err_rec_t structs (if non-NULL)
*
* RETURNS:
*    overall status; may be the last of multiple error conditions
*********************************************************************/
extern status_t
    agt_ncx_cfg_load (cfg_template_t *cfg,
		      cfg_location_t cfgloc,
		      const xmlChar *cfgparm);


/********************************************************************
* FUNCTION agt_ncx_cfg_save
*
* Save the specified cfg to the its startup source, which should
* be stored in the cfg struct
*
* INPUTS:
*    cfg  = Config template to save from
*    bkup = TRUE if the current startup config should
*           be saved before it is overwritten
*         = FALSE to just overwrite the old startup cfg
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_ncx_cfg_save (cfg_template_t *cfg,
		      boolean bkup);

/********************************************************************
* FUNCTION agt_ncx_cfg_save_inline
*
* Save the specified cfg to the its startup source, which should
* be stored in the cfg struct
*
* INPUTS:
*    source_url == filespec where to save newroot 
*    newroot == value root to save
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_ncx_cfg_save_inline (const xmlChar *source_url,
                             val_value_t *newroot);


/********************************************************************
* FUNCTION agt_ncx_load_backup
*
* Load a backup config into the specified config template
*
* INPUTS:
*    filespec == complete path for the input file
*    cfg == config template to load
*    use_sid == session ID of user to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    agt_ncx_load_backup (const xmlChar *filespec,
                         cfg_template_t *cfg,
                         ses_id_t  use_sid);


/********************************************************************
* FUNCTION agt_ncx_cc_active
*
* Check if a confirmed-commit is active, and the timeout
* may need to be processed
*
* RETURNS:
*    TRUE if confirmed-commit is active
*    FALSE otherwise
*********************************************************************/
extern boolean
    agt_ncx_cc_active (void);


/********************************************************************
* FUNCTION agt_ncx_cc_ses_id
*
* Get the confirmed commit session ID
*
* RETURNS:
*    session ID for the confirmed commit
*********************************************************************/
extern ses_id_t
    agt_ncx_cc_ses_id (void);


/********************************************************************
* FUNCTION agt_ncx_clear_cc_ses_id
*
* Clear the confirmed commit session ID
* This will be called by agt_ses when the current
* session exits during a persistent confirmed-commit
*
*********************************************************************/
extern void
    agt_ncx_clear_cc_ses_id (void);


/********************************************************************
* FUNCTION agt_ncx_cc_persist_id
*
* Get the confirmed commit persist ID
*
* RETURNS:
*    session ID for the confirmed commit
*********************************************************************/
extern const xmlChar *
    agt_ncx_cc_persist_id (void);


/********************************************************************
* FUNCTION agt_ncx_check_cc_timeout
*
* Check if a confirmed-commit has timed out, and needs to be canceled
*
*********************************************************************/
extern void
    agt_ncx_check_cc_timeout (void);


/********************************************************************
* FUNCTION agt_ncx_cancel_confirmed_commit
*
* Cancel the confirmed-commit in progress and rollback
* to the backup-cfg.xml file
*
* INPUTS:
*   scb == session control block making this change, may be NULL
*   event == confirmEvent enumeration value to use
*
*********************************************************************/
extern void
    agt_ncx_cancel_confirmed_commit (ses_cb_t *scb,
                                     ncx_confirm_event_t event);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_ncx */
