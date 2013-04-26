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
#ifndef _H_yangcli_cmd
#define _H_yangcli_cmd

/*  FILE: yangcli_cmd.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
11-apr-09    abb      Begun; moved from yangcli.c

*/


#include <xmlstring.h>

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
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
* FUNCTION top_command
* 
* Top-level command handler
*
* INPUTS:
*   server_cb == server control block to use
*   line == input command line from user
*
* OUTPUTS:
*    state may be changed or other action taken
*    the line buffer is NOT consumed or freed by this function
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    top_command (server_cb_t *server_cb,
		 xmlChar *line);


/********************************************************************
* FUNCTION conn_command
* 
* Connection level command handler
*
* INPUTS:
*   server_cb == server control block to use
*   line == input command line from user
*
* OUTPUTS:
*    state may be changed or other action taken
*    the line buffer is NOT consumed or freed by this function
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    conn_command (server_cb_t *server_cb,
		  xmlChar *line);


/********************************************************************
 * FUNCTION do_startup_script
 * 
 * Process run-script CLI parameter
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   runscript == name of the script to run (could have path)
 *
 * SIDE EFFECTS:
 *   runstack start with the runscript script if no errors
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_startup_script (server_cb_t *server_cb,
                       const xmlChar *runscript);


/********************************************************************
 * FUNCTION do_startup_command
 * 
 * Process run-command CLI parameter
 *
 * INPUTS:
 *   server_cb == server control block to use
 *   runcommand == command string to run
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_startup_command (server_cb_t *server_cb,
                        const xmlChar *runcommand);


/********************************************************************
* FUNCTION get_cmd_line
* 
*  Read the current runstack context and construct
*  a command string for processing by do_run.
*    - Extended lines will be concatenated in the
*      buffer.  If a buffer overflow occurs due to this
*      concatenation, an error will be returned
* 
* INPUTS:
*   server_cb == server control block to use
*   res == address of status result
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to the command line to process (should treat as CONST !!!)
*   NULL if some error
*********************************************************************/
extern xmlChar *
    get_cmd_line (server_cb_t *server_cb,
		  status_t *res);


/********************************************************************
 * FUNCTION do_connect
 * 
 * INPUTS:
 *   server_cb == server control block to use
 *   rpc == rpc header for 'connect' command
 *   line == input text from readline call, not modified or freed here
 *   start == byte offset from 'line' where the parse RPC method
 *            left off.  This is eiother empty or contains some 
 *            parameters from the user
 *   climode == TRUE if starting from CLI and should try
 *              to connect right away if the mandatory parameters
 *              are present
 *
 * OUTPUTS:
 *   connect_valset parms may be set 
 *   create_session may be called
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_connect (server_cb_t *server_cb,
		obj_template_t *rpc,
		const xmlChar *line,
		uint32 start,
                boolean climode);


/********************************************************************
* FUNCTION parse_def
* 
* Definitions have two forms:
*   def       (default module used)
*   module:def (explicit module name used)
*   prefix:def (if prefix-to-module found, explicit module name used)
*
* Parse the possibly module-qualified definition (module:def)
* and find the template for the requested definition
*
* INPUTS:
*   server_cb == server control block to use
*   dtyp == definition type 
*       (NCX_NT_OBJ or  NCX_NT_TYP)
*   line == input command line from user
*   len == address of output var for number of bytes parsed
*   retres == address of return status
*
* OUTPUTS:
*    *dtyp is set if it started as NONE
*    *len == number of bytes parsed
*    *retres == return status
*
* RETURNS:
*   pointer to the found definition template or NULL if not found
*********************************************************************/
extern void *
    parse_def (server_cb_t *server_cb,
	       ncx_node_t *dtyp,
	       xmlChar *line,
	       uint32 *len,
               status_t *retres);


/********************************************************************
* FUNCTION send_keepalive_get
* 
* Send a <get> operation to the server to keep the session
* from getting timed out; server sent a keepalive request
* and SSH will drop the session unless data is sent
* within a configured time
*
* INPUTS:
*    server_cb == server control block to use
*
* OUTPUTS:
*    state may be changed or other action taken
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    send_keepalive_get (server_cb_t *server_cb);


/********************************************************************
 * FUNCTION get_valset
 * 
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the command being processed
 *    line == CLI input in progress
 *    res == address of status result
 *
 * OUTPUTS:
 *    *res is set to the status
 *
 * RETURNS:
 *   malloced valset filled in with the parameters for 
 *   the specified RPC
 *
 *********************************************************************/
extern val_value_t *
    get_valset (server_cb_t *server_cb,
		obj_template_t *rpc,
		const xmlChar *line,
		status_t  *res);


/********************************************************************
 * FUNCTION do_line_recall (execute the recall local RPC)
 * 
 * recall n 
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    num == entry number of history entry entry to recall
 * 
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_line_recall (server_cb_t *server_cb,
                    unsigned long num);


/********************************************************************
 * FUNCTION do_line_recall_string
 * 
 * bang recall support
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    line  == command line to recall
 * 
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_line_recall_string (server_cb_t *server_cb,
                           const xmlChar *line);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_cmd */
