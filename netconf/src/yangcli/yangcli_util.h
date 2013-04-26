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
#ifndef _H_yangcli_util
#define _H_yangcli_util

/*  FILE: yangcli_util.h
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
27-mar-09    abb      Begun; moved from yangcli.c

*/


#include <xmlstring.h>

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifndef _H_var
#include "var.h"
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
* FUNCTION is_top_command
* 
* Check if command name is a top command
* Must be full name
*
* INPUTS:
*   rpcname == command name to check
*
* RETURNS:
*   TRUE if this is a top command
*   FALSE if not
*********************************************************************/
extern boolean
    is_top_command (const xmlChar *rpcname);


/********************************************************************
* FUNCTION new_modptr
* 
*  Malloc and init a new module pointer block
* 
* INPUTS:
*    mod == module to cache in this struct
*    malloced == TRUE if mod is malloced
*                FALSE if mod is a back-ptr
*    feature_list == feature list from capability
*    deviation_list = deviations list from capability
*
* RETURNS:
*   malloced modptr_t struct or NULL of malloc failed
*********************************************************************/
extern modptr_t *
    new_modptr (ncx_module_t *mod,
		ncx_list_t *feature_list,
		ncx_list_t *deviation_list);


/********************************************************************
* FUNCTION free_modptr
* 
*  Clean and free a module pointer block
* 
* INPUTS:
*    modptr == mod pointer block to free
*              MUST BE REMOVED FROM ANY Q FIRST
*
*********************************************************************/
extern void
    free_modptr (modptr_t *modptr);


/********************************************************************
* FUNCTION find_modptr
* 
*  Find a specified module name
* 
* INPUTS:
*    modptrQ == Q of modptr_t to check
*    modname == module name to find
*    revision == module revision (may be NULL)
*
* RETURNS:
*   pointer to found entry or NULL if not found
*
*********************************************************************/
extern modptr_t *
    find_modptr (dlq_hdr_t *modptrQ,
                 const xmlChar *modname,
                 const xmlChar *revision);


/********************************************************************
* FUNCTION clear_server_cb_session
* 
*  Clean the current session data from an server control block
* 
* INPUTS:
*    server_cb == control block to use for clearing
*                the session data
*********************************************************************/
extern void
    clear_server_cb_session (server_cb_t *server_cb);


/********************************************************************
* FUNCTION is_top
* 
* Check the state and determine if the top or conn
* mode is active
* 
* INPUTS:
*   server state to use
*
* RETURNS:
*  TRUE if this is TOP mode
*  FALSE if this is CONN mode (or associated states)
*********************************************************************/
extern boolean
    is_top (mgr_io_state_t state);


/********************************************************************
* FUNCTION use_servercb
* 
* Check if the server_cb should be used for modules right now
*
* INPUTS:
*   server_cb == server control block to check
*
* RETURNS:
*   TRUE to use server_cb
*   FALSE if not
*********************************************************************/
extern boolean
    use_servercb (server_cb_t *server_cb);



/********************************************************************
* FUNCTION find_module
* 
*  Check the server_cb for the specified module; if not found
*  then try ncx_find_module
* 
* INPUTS:
*    server_cb == control block to free
*    modname == module name
*
* RETURNS:
*   pointer to the requested module
*      using the registered 'current' version
*   NULL if not found
*********************************************************************/
extern ncx_module_t *
    find_module (server_cb_t *server_cb,
		 const xmlChar *modname);


/********************************************************************
* FUNCTION get_strparm
* 
* Get the specified string parm from the parmset and then
* make a strdup of the value
*
* INPUTS:
*   valset == value set to check if not NULL
*   modname == module defining parmname
*   parmname  == name of parm to get
*
* RETURNS:
*   pointer to string !!! THIS IS A MALLOCED COPY !!!
*********************************************************************/
extern xmlChar *
    get_strparm (val_value_t *valset,
		 const xmlChar *modname,
		 const xmlChar *parmname);


/********************************************************************
* FUNCTION findparm
* 
* Get the specified string parm from the parmset and then
* make a strdup of the value
*
* INPUTS:
*   valset == value set to search
*   modname == optional module name defining the parameter to find
*   parmname  == name of parm to get, or partial name to get
*
* RETURNS:
*   pointer to val_value_t if found
*********************************************************************/
extern val_value_t *
    findparm (val_value_t *valset,
	      const xmlChar *modname,
	      const xmlChar *parmname);


/********************************************************************
* FUNCTION add_clone_parm
* 
*  Create a parm 
* 
* INPUTS:
*   val == value to clone and add
*   valset == value set to add parm into
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    add_clone_parm (const val_value_t *val,
		    val_value_t *valset);


/********************************************************************
* FUNCTION is_yangcli_ns
* 
*  Check the namespace and make sure this is an YANGCLI command
* 
* INPUTS:
*   ns == namespace ID to check
*
* RETURNS:
*  TRUE if this is the YANGCLI namespace ID
*********************************************************************/
extern boolean
    is_yangcli_ns (xmlns_id_t ns);


/********************************************************************
 * FUNCTION clear_result
 * 
 * clear out the pending result info
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 *********************************************************************/
extern void
    clear_result (server_cb_t *server_cb);


/********************************************************************
* FUNCTION check_filespec
* 
* Check the filespec string for a file assignment statement
* Save it if it si good
*
* INPUTS:
*    server_cb == server control block to use
*    filespec == string to check
*    varname == variable name to use in log_error
*              if this is complex form
*
* OUTPUTS:
*    server_cb->result_filename will get set if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    check_filespec (server_cb_t *server_cb,
		    const xmlChar *filespec,
		    const xmlChar *varname);


/********************************************************************
 * FUNCTION get_instanceid_parm
 * 
 * Validate an instance identifier parameter
 * Return the target object
 * Return a value struct from root containing
 * all the predicate assignments in the stance identifier
 *
 * INPUTS:
 *    target == XPath expression for the instance-identifier
 *    schemainst == TRUE if ncx:schema-instance string
 *                  FALSE if instance-identifier
 *    configonly == TRUE if there should be an error given
 *                  if the target does not point to a config node
 *                  FALSE if the target can be config false
 *    targobj == address of return target object for this expr
 *    targval == address of return pointer to target value
 *               node within the value subtree returned
 *    retres == address of return status
 *
 * OUTPUTS:
 *    *targobj == the object template for the target
 *    *targval == the target node within the returned subtree
 *                from root
 *    *retres == return status for the operation
 *
 * RETURNS:
 *   If NO_ERR:
 *     malloced value node representing the instance-identifier
 *     from root to the targobj
 *  else:
 *    NULL, check *retres
 *********************************************************************/
extern val_value_t *
    get_instanceid_parm (const xmlChar *target,
			 boolean schemainst,
                         boolean configonly,
			 obj_template_t **targobj,
			 val_value_t **targval,
			 status_t *retres);


/********************************************************************
* FUNCTION get_file_result_format
* 
* Check the filespec string for a file assignment statement
* to see if it is text, XML, or JSON
*
* INPUTS:
*    filespec == string to check
*
* RETURNS:
*   result format enumeration; RF_NONE if some error
*********************************************************************/
extern result_format_t
    get_file_result_format (const xmlChar *filespec);


/********************************************************************
* FUNCTION interactive_mode
* 
*  Check if the program is in interactive mode
* 
* RETURNS:
*   TRUE if insteractive mode, FALSE if batch mode
*********************************************************************/
extern boolean
    interactive_mode (void);


/********************************************************************
 * FUNCTION init_completion_state
 * 
 * init the completion_state struct for a new command
 *
 * INPUTS:
 *    completion_state == record to initialize
 *    server_cb == server control block to use
 *    cmdstate ==initial  calling state
 *********************************************************************/
extern void
    init_completion_state (completion_state_t *completion_state,
			   server_cb_t *server_cb,
			   command_state_t  cmdstate);


/********************************************************************
 * FUNCTION set_completion_state
 * 
 * set the completion_state struct for a new mode or sub-command
 *
 * INPUTS:
 *    completion_state == record to set
 *    rpc == rpc operation in progress (may be NULL)
 *    parm == parameter being filled in
 *    cmdstate ==current calling state
 *********************************************************************/
extern void
    set_completion_state (completion_state_t *completion_state,
			  obj_template_t *rpc,
			  obj_template_t *parm,
			  command_state_t  cmdstate);


/********************************************************************
 * FUNCTION set_completion_state_curparm
 * 
 * set the current parameter in the completion_state struct
 *
 * INPUTS:
 *    completion_state == record to set
 *    parm == parameter being filled in
 *********************************************************************/
extern void
    set_completion_state_curparm (completion_state_t *completion_state,
				  obj_template_t *parm);


/********************************************************************
* FUNCTION xpath_getvar_fn
 *
 * see ncx/xpath.h -- matches xpath_getvar_fn_t template
 *
 * Callback function for retrieval of a variable binding
 * 
 * INPUTS:
 *   pcb   == XPath parser control block in use
 *   varname == variable name requested
 *   res == address of return status
 *
 * OUTPUTS:
 *  *res == return status
 *
 * RETURNS:
 *    pointer to the ncx_var_t data structure
 *    for the specified varbind
*********************************************************************/
extern ncx_var_t *
    xpath_getvar_fn (struct xpath_pcb_t_ *pcb,
                     const xmlChar *varname,
                     status_t *res);

/********************************************************************
* FUNCTION get_netconf_mod
* 
*  Get the netconf module
*
* INPUTS:
*   server_cb == server control block to use
* 
* RETURNS:
*    netconf module
*********************************************************************/
extern ncx_module_t *
    get_netconf_mod (server_cb_t *server_cb);


/********************************************************************
* FUNCTION clone_old_parm
* 
*  Clone a parameter value from the 'old' value set
*  if it exists there, and add it to the 'new' value set
*  only if the new value set does not have this parm
*
* The old and new pvalue sets must be complex types 
*  NCX_BT_LIST, NCX_BT_CONTAINER, or NCX_BT_ANYXML
*
* INPUTS:
*   oldvalset == value set to copy from
*   newvalset == value set to copy into
*   parm == object template to find and copy
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    clone_old_parm (val_value_t *oldvalset,
                    val_value_t *newvalset,
                    obj_template_t *parm);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangcli_util */
