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
#ifndef _H_yangcli_uservars
#define _H_yangcli_uservars

/*  FILE: yangcli_uservars.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  implement yangcli uservars command
  
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
01-oct-11    abb      Begun;

*/

#include <xmlstring.h>

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
*								    *
*		      F U N C T I O N S 			    *
*								    *
*********************************************************************/


/********************************************************************
 * FUNCTION do_uservars (local RPC)
 * 
 * uservars clear
 * uservars load[=filespec]
 * uservars save[=filespec]
 *
 * Handle the uservars command
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the uservars command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_uservars (server_cb_t *server_cb,
                 obj_template_t *rpc,
                 const xmlChar *line,
                 uint32  len);



/********************************************************************
 * FUNCTION load_uservars
 * 
 * Load the user variables from the specified filespec
 *
 * INPUT:
 *   server_cb == server control block to use
 *   fspec == input filespec to use (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    load_uservars (server_cb_t *server_cb,
                   const xmlChar *fspec);


/********************************************************************
 * FUNCTION save_uservars
 * 
 * Save the uservares to the specified filespec
 *
 * INPUT:
 *   server_cb == server control block to use
 *   fspec == output filespec to use  (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    save_uservars (server_cb_t *server_cb,
                   const xmlChar *fspec);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_uservars */
