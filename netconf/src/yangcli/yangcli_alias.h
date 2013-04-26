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
#ifndef _H_yangcli_alias
#define _H_yangcli_alias

/*  FILE: yangcli_alias.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  implement yangcli alias command
  
 
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
 * FUNCTION do_alias (local RPC)
 * 
 * alias
 * alias def
 * alias def=def-value
 *
 * Handle the alias command, based on the parameter
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the alias command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_alias (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len);


/********************************************************************
 * FUNCTION do_aliases (local RPC)
 * 
 * aliases
 * aliases clear
 * aliases show
 * aliases load[=filespec]
 * aliases save[=filespec]
 *
 * Handle the aliases command, based on the parameter
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the aliases command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_aliases (server_cb_t *server_cb,
                obj_template_t *rpc,
                const xmlChar *line,
                uint32  len);


/********************************************************************
 * FUNCTION do_unset (local RPC)
 * 
 * unset def
 *
 * Handle the unset command; remove the specified alias
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the unset command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_unset (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len);


/********************************************************************
 * FUNCTION show_aliases
 * 
 * Output all the alias values
 *
 *********************************************************************/
extern void
    show_aliases (void);


/********************************************************************
 * FUNCTION show_alias
 * 
 * Output 1 alias by name
 *
 * INPUTS:
 *    name == name of alias to show
 *********************************************************************/
extern void
    show_alias (const xmlChar *name);


/********************************************************************
 * FUNCTION free_aliases
 * 
 * Free all the command aliases
 *
 *********************************************************************/
extern void
    free_aliases (void);


/********************************************************************
 * FUNCTION load_aliases
 * 
 * Load the aliases from the specified filespec
 *
 * INPUT:
 *   fspec == input filespec to use (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    load_aliases (const xmlChar *fspec);


/********************************************************************
 * FUNCTION save_aliases
 * 
 * Save the aliases to the specified filespec
 *
 * INPUT:
 *   fspec == output filespec to use  (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    save_aliases (const xmlChar *fspec);


/********************************************************************
 * FUNCTION expand_alias
 * 
 * Check if the first token is an alias name
 * If so, construct a new command line with the alias contents
 *
 * INPUT:
 *   line == command line to check and possibly convert
 *   res == address of return status
 *
 * OUTPUTS:
 *   *res == return status (ERR_NCX_SKIPPED if nothing done)
 *
 * RETURNS:
 *   pointer to malloced command string if *res==NO_ERR
 *   NULL if *res==ERR_NCX_SKIPPED or some real error
 *********************************************************************/
extern xmlChar *
    expand_alias (xmlChar *line,
                  status_t *res);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_alias */
