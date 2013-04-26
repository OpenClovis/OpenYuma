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
#ifndef _H_yangcli_cond
#define _H_yangcli_cond

/*  FILE: yangcli_cond.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Conditional commands for script looping and if-stmt
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
27-apr-10    abb      Begun

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
 * FUNCTION do_while (local RPC)
 * 
 * Handle the while command; start a new loopcb context
 *
 * while expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_while (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len);


/********************************************************************
 * FUNCTION do_if (local RPC)
 * 
 * Handle the if command; start a new ifcb context
 *
 * if expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_if (server_cb_t *server_cb,
           obj_template_t *rpc,
           const xmlChar *line,
           uint32  len);



/********************************************************************
 * FUNCTION do_elif (local RPC)
 * 
 * Handle the if command; start a new ifcb context
 *
 * elif expr='xpath-str' [docroot=$foo]
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_elif (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len);


/********************************************************************
 * FUNCTION do_else (local RPC)
 * 
 * Handle the else command; start another conditional sub-block
 * to the current if-block
 *
 *   else
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_else (server_cb_t *server_cb,
             obj_template_t *rpc,
             const xmlChar *line,
             uint32  len);


/********************************************************************
 * FUNCTION do_end (local RPC)
 * 
 * Handle the end command; end an if or while block
 *
 *   end
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the show command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_end (server_cb_t *server_cb,
            obj_template_t *rpc,
            const xmlChar *line,
            uint32  len);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_cond */
