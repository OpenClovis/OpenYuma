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
#ifndef _H_agt_cap
#define _H_agt_cap

/*  FILE: agt_cap.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server capabilities handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
03-feb-06    abb      Begun; split out from base/cap.h

*/

#ifndef _H_agt
#include "agt.h"
#endif

#ifndef _H_cap
#include "cap.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
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
* FUNCTION agt_cap_cleanup
*
* Clean the NETCONF server capabilities
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void 
    agt_cap_cleanup (void);


/********************************************************************
* FUNCTION agt_cap_set_caps
*
* Initialize the NETCONF server capabilities
*
* INPUTS:
*    agttarg == the target of edit-config for this server
*    agtstart == the type of startup configuration for this server
*    defstyle == default with-defaults style for the entire server
*
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
extern status_t 
    agt_cap_set_caps (ncx_agttarg_t  agttarg,
		      ncx_agtstart_t agtstart,
		      const xmlChar *defstyle);


/********************************************************************
* FUNCTION agt_cap_set_modules
*
* Initialize the NETCONF server capabilities modules list
* MUST call after agt_cap_set_caps
*
* INPUTS:
*   profile == server profile control block to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_cap_set_modules (agt_profile_t *profile);


/********************************************************************
* FUNCTION agt_cap_add_module
*
* Add a module at runtime, after the initial set has been set
* MUST call after agt_cap_set_caps
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    agt_cap_add_module (ncx_module_t *mod);


/********************************************************************
* FUNCTION agt_cap_get_caps
*
* Get the NETCONF server capabilities
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the server caps list
*********************************************************************/
extern cap_list_t * 
    agt_cap_get_caps (void);


/********************************************************************
* FUNCTION agt_cap_get_capsval
*
* Get the NETCONF server capabilities in val_value_t format
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the server caps list
*********************************************************************/
extern val_value_t * 
    agt_cap_get_capsval (void);


/********************************************************************
* FUNCTION agt_cap_std_set
*
* Check if the STD capability is set for the server
*
* INPUTS:
*    cap == ID of capability to check
*
* RETURNS:
*    TRUE is STD cap set, FALSE otherwise
*********************************************************************/
extern boolean
    agt_cap_std_set (cap_stdid_t cap);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_cap */
