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
#ifndef _H_mgr_cap
#define _H_mgr_cap

/*  FILE: mgr_cap.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Manager capabilities handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
03-feb-06    abb      Begun; split out from base/cap.h

*/

#ifndef _H_cap
#include "cap.h"
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
* FUNCTION mgr_cap_cleanup
*
* Clean the NETCONF manager capabilities
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void 
    mgr_cap_cleanup (void);


/********************************************************************
* FUNCTION mgr_cap_set_caps
*
* Initialize the NETCONF manager capabilities
*
* TEMP !!! JUST SET EVERYTHING, WILL REALLY GET FROM MGR STARTUP
*
* INPUTS:
*    none
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
extern status_t 
    mgr_cap_set_caps (void);


/********************************************************************
* FUNCTION mgr_cap_get_caps
*
* Get the NETCONF manager capabilities
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the manager caps list
*********************************************************************/
extern cap_list_t * 
    mgr_cap_get_caps (void);


/********************************************************************
* FUNCTION mgr_cap_get_capsval
*
* Get the NETCONF manager capabilities ain val_value_t format
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the manager caps list
*********************************************************************/
extern val_value_t * 
    mgr_cap_get_capsval (void);


/********************************************************************
* FUNCTION mgr_cap_get_ses_capsval
*
* Get the NETCONF manager capabilities ain val_value_t format
* for a specific session, v2 supports base1.0 and/or base1.1
* INPUTS:
*    scb == session control block to use
* RETURNS:
*    MALLOCED pointer to the manager caps list to use
*    and then discard with val_free_value
*********************************************************************/
extern val_value_t * 
    mgr_cap_get_ses_capsval (ses_cb_t *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_mgr_cap */
