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
#ifndef _H_getcb
#define _H_getcb

/*  FILE: getcb.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Data Model Get Operation callback handler
    for virtual data that is not stored in a val_value_t tree.

    These callback functions are used to provide temporary
    val_value_t structures that MUST BE FREED by the caller
    after they are used!!!

    The <get> callback function has several sub-mode functions to
    support the retrieval of virtual data, mixed with the real data
    tree of a configuration database.

    Submode 1: GETCB_GET_METAQ

       Retrieve a Queue header of val_value_t structs
       representing the attributes for the specified value node


    Submode x: GETCB_GET_LEAFVAL

      Retrieve the simple value contents of a virtual value leaf node


*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
16-apr-07    abb      Begun; split out from agt_ps.h

*/

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
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

/* placeholder for expansion modes */
typedef enum getcb_mode_t_ {
    GETCB_NONE,
    GETCB_GET_VALUE
} getcb_mode_t;


/* getcb_fn_t
 *
 * Callback function for agent node get handler 
 * 
 * INPUTS:
 *   scb    == session that issued the get (may be NULL)
 *             can be used for access control purposes
 *   cbmode == reason for the callback
 *   virval == place-holder node in the data model for
 *              this virtual value node
 *   dstval == pointer to value output struct
 *
 * OUTPUTS:
 *  *fil may be adjusted depending on callback reason
 *  *dstval should be filled in, depending on the callback reason
 *
 * RETURNS:
 *    status:
 */
typedef status_t 
    (*getcb_fn_t) (ses_cb_t *scb,
		   getcb_mode_t cbmode,
		   const val_value_t *virval,
		   val_value_t *dstval);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_getcb */
