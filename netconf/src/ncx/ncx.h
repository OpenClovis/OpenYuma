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
#ifndef _H_ncx
#define _H_ncx

/*  FILE: ncx.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************



    NCX Module Library Utility Functions


                      Container of Definitions
                        +-----------------+ 
                        |   ncx_module_t  |
                        |   ncxtypes.h    |
                        +-----------------+

            Template/Schema 
          +-----------------+ 
          | obj_template_t  |
          |     obj.h       |
          +-----------------+
             ^          |
             |          |       Value Instances
             |          |     +-----------------+
             |          +---->|   val_value_t   |
             |                |      val.h      |
             |                +-----------------+
             |                               
             |       Data Types
             |   +--------------------+      
             +---|   typ_template_t   |      
                 |       typ.h        |      
                 +--------------------+      

 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
29-oct-05    abb      Begun
20-jul-08    abb      Start YANG rewrite; remove PSD and PS
23-aug-09    abb      Update diagram in header
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_yang
#include "yang.h"
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
* FUNCTION ncx_init
* 
* Initialize the NCX module
*
* INPUTS:
*    savestr == TRUE if parsed description strings that are
*               not needed by the agent at runtime should
*               be saved anyway.  Converters should use this value.
*                 
*            == FALSE if uneeded strings should not be saved.
*               Embedded agents should use this value
*
*    dlevel == desired debug output level
*    logtstamps == TRUE if log should use timestamps
*                  FALSE if not; not used unless 'log' is present
*    startmsg == log_debug2 message to print before starting;
*                NULL if not used;
*    argc == CLI argument count for bootstrap CLI
*    argv == array of CLI parms for bootstrap CLI
*
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
extern status_t 
    ncx_init (boolean savestr,
	      log_debug_t dlevel,
	      boolean logtstamps,
	      const char *startmsg,
	      int argc,
	      char *argv[]);


/********************************************************************
* FUNCTION ncx_cleanup
*
*  cleanup NCX module
*********************************************************************/
extern void 
    ncx_cleanup (void);


/********************************************************************
* FUNCTION ncx_new_module
* 
* Malloc and initialize the fields in a ncx_module_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern ncx_module_t * 
    ncx_new_module (void);


/********************************************************************
* FUNCTION ncx_find_module
*
* Find a ncx_module_t in the ncx_modQ
* These are the modules that are already loaded
*
* INPUTS:
*   modname == module name
*   revision == module revision date
*
* RETURNS:
*  module pointer if found or NULL if not
*********************************************************************/
extern ncx_module_t *
    ncx_find_module (const xmlChar *modname,
		     const xmlChar *revision);


/********************************************************************
* FUNCTION ncx_find_module_que
*
* Find a ncx_module_t in the specified Q
* Check the namespace ID
*
* INPUTS:
*   modQ == module Q to search
*   modname == module name
*   revision == module revision date
*
* RETURNS:
*  module pointer if found or NULL if not
*********************************************************************/
extern ncx_module_t *
    ncx_find_module_que (dlq_hdr_t *modQ,
                         const xmlChar *modname,
                         const xmlChar *revision);


/********************************************************************
* FUNCTION ncx_find_module_que_nsid
*
* Find a ncx_module_t in the specified Q
* Check the namespace ID
*
* INPUTS:
*   modQ == module Q to search
*   nsid == xmlns ID to find
*
* RETURNS:
*  module pointer if found or NULL if not
*********************************************************************/
extern ncx_module_t *
    ncx_find_module_que_nsid (dlq_hdr_t *modQ,
                              xmlns_id_t nsid);


/********************************************************************
* FUNCTION ncx_free_module
* 
* Scrub the memory in a ncx_module_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* use if module was not added to registry
*
* MUST remove this struct from the ncx_modQ before calling
* Does not remove module definitions from the registry
*
* Use the ncx_remove_module function if the module was 
* already successfully added to the modQ and definition registry
*
* INPUTS:
*    mod == ncx_module_t data structure to free
*********************************************************************/
extern void 
    ncx_free_module (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_any_mod_errors
* 
* Check if any of the loaded modules are loaded with non-fatal errors
*
* RETURNS:
*    TRUE if any modules are loaded with non-fatal errors
*    FALSE if all modules present have a status of NO_ERR
*********************************************************************/
extern boolean
    ncx_any_mod_errors (void);


/********************************************************************
* FUNCTION ncx_any_dependency_errors
* 
* Check if any of the imports that this module relies on
* were loadeds are loaded with non-fatal errors
*
* RETURNS:
*    TRUE if any modules are loaded with non-fatal errors
*    FALSE if all modules present have a status of NO_ERR
*********************************************************************/
extern boolean
    ncx_any_dependency_errors (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_find_type
*
* Check if a typ_template_t in the mod->typeQ
*
* INPUTS:
*   mod == ncx_module to check
*   typname == type name
*   useall == TRUE to use all submodules
*             FALSE to only use the ones in the mod->includeQ
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern typ_template_t *
    ncx_find_type (ncx_module_t *mod,
                   const xmlChar *typname,
                   boolean useall);


/********************************************************************
* FUNCTION ncx_find_type_que
*
* Check if a typ_template_t in the mod->typeQ
*
* INPUTS:
*   que == type Q to check
*   typname == type name
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern typ_template_t *
    ncx_find_type_que (const dlq_hdr_t *typeQ,
		       const xmlChar *typname);


/********************************************************************
* FUNCTION ncx_find_grouping
*
* Check if a grp_template_t in the mod->groupingQ
*
* INPUTS:
*   mod == ncx_module to check
*   grpname == group name
*   useall == TRUE to check all existing nodes
*             FALSE to only use includes visible to this [sub]mod
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern grp_template_t *
    ncx_find_grouping (ncx_module_t *mod,
                       const xmlChar *grpname,
                       boolean useall);


/********************************************************************
* FUNCTION ncx_find_grouping_que
*
* Check if a grp_template_t in the specified Q
*
* INPUTS:
*   groupingQ == Queue of grp_template_t to check
*   grpname == group name
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern grp_template_t *
    ncx_find_grouping_que (const dlq_hdr_t *groupingQ,
			   const xmlChar *grpname);


/********************************************************************
* FUNCTION ncx_find_rpc
*
* Check if a rpc_template_t in the mod->rpcQ
*
* INPUTS:
*   mod == ncx_module to check
*   rpcname == RPC name
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t * 
    ncx_find_rpc (const ncx_module_t *mod,
		  const xmlChar *rpcname);


/********************************************************************
* FUNCTION ncx_match_rpc
*
* Check if a rpc_template_t in the mod->rpcQ
*
* INPUTS:
*   mod == ncx_module to check
*   rpcname == RPC name to match
*   retcount == address of return match count
*
* OUTPUTS:
*   *retcount == number of matches found
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t * 
    ncx_match_rpc (const ncx_module_t *mod,
		   const xmlChar *rpcname,
                   uint32 *retcount);


/********************************************************************
* FUNCTION ncx_match_any_rpc
*
* Check if a rpc_template_t in in any module that
* matches the rpc name string and maybe the owner
*
* INPUTS:
*   module == module name to check (NULL == check all)
*   rpcname == RPC name to match
*   retcount == address of return count of matches
*
* OUTPUTS:
*   *retcount == number of matches found
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t * 
    ncx_match_any_rpc (const xmlChar *module,
		       const xmlChar *rpcname,
                       uint32 *retcount);


/********************************************************************
* FUNCTION ncx_match_any_rpc_mod
*
* Check if a rpc_template_t is in the specified module
*
* INPUTS:
*   mod == module struct to check
*   rpcname == RPC name to match
*   retcount == address of return count of matches
*
* OUTPUTS:
*   *retcount == number of matches found
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    ncx_match_any_rpc_mod (ncx_module_t *mod,
                           const xmlChar *rpcname,
                           uint32 *retcount);


/********************************************************************
* FUNCTION ncx_match_rpc_error
*
* Generate an error for multiple matches
*
* INPUTS:
*   mod == module struct to check (may be NULL)
*   modname == module name if mod not set
*           == NULL to match all modules
*   rpcname == RPC name to match
*   match == TRUE to match partial command names
*            FALSE for exact match only
*   firstmsg == TRUE to do the first log_error banner msg
*               FALSE to skip this step
*********************************************************************/
extern void
    ncx_match_rpc_error (ncx_module_t *mod,
                         const xmlChar *modname,
                         const xmlChar *rpcname,
                         boolean match,
                         boolean firstmsg);


/********************************************************************
* FUNCTION ncx_find_any_object
*
* Check if an obj_template_t in in any module that
* matches the object name string
*
* INPUTS:
*   objname == object name to match
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    ncx_find_any_object (const xmlChar *objname);


/********************************************************************
* FUNCTION ncx_match_any_object
*
* Check if an obj_template_t in in any module that
* matches the object name string
*
* INPUTS:
*   objname == object name to match
*   name_match == name match mode enumeration
*   alt_names == TRUE if alternate names should be checked
*                after regular names; FALSE if not
*   retres == address of return status
*
* OUTPUTS:
*   if retres not NULL, *retres set to return status
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    ncx_match_any_object (const xmlChar *objname,
                          ncx_name_match_t name_match,
                          boolean alt_names,
                          status_t *retres);


/********************************************************************
* FUNCTION ncx_find_any_object_que
*
* Check if an obj_template_t in in any module that
* matches the object name string
*
* INPUTS:
*   modQ == Q of modules to check
*   objname == object name to match
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    ncx_find_any_object_que (dlq_hdr_t *modQ,
                             const xmlChar *objname);


/********************************************************************
* FUNCTION ncx_find_object
*
* Find a top level module object
*
* INPUTS:
*   mod == ncx_module to check
*   typname == type name
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    ncx_find_object (ncx_module_t *mod,
		     const xmlChar *objname);


/********************************************************************
* FUNCTION ncx_add_namespace_to_registry
*
* Add the namespace and prefix to the registry
* or retrieve it if already set
* 
* INPUTS:
*   mod == module to add to registry
*   tempmod == TRUE if this is a temporary add mode
*              FALSE if this is a real registry add
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_add_namespace_to_registry (ncx_module_t *mod,
                                   boolean tempmod);



/********************************************************************
* FUNCTION ncx_add_to_registry
*
* Add all the definitions stored in an ncx_module_t to the registry
* This step is deferred to keep the registry stable as possible
* and only add modules in an all-or-none fashion.
* 
* INPUTS:
*   mod == module to add to registry
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_add_to_registry (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_add_to_modQ
*
* Add module to the current module Q
* Used by yangdiff to bypass add_to_registry to support
* N different module trees
*
* INPUTS:
*   mod == module to add to current module Q
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_add_to_modQ (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_is_duplicate
* 
* Search the specific module for the specified definition name.
* This function is for modules in progress which have not been
* added to the registry yet.
*
* INPUTS:
*     mod == ncx_module_t to check
*     defname == name of definition to find
* RETURNS:
*    TRUE if found, FALSE otherwise
*********************************************************************/
extern boolean
    ncx_is_duplicate (ncx_module_t *mod,
		      const xmlChar *defname);


/********************************************************************
* FUNCTION ncx_get_first_module
* 
* Get the first module in the ncx_modQ
* 
* RETURNS:
*   pointer to the first entry or NULL if empty Q
*********************************************************************/
extern ncx_module_t *
    ncx_get_first_module (void);


/********************************************************************
* FUNCTION ncx_get_next_module
* 
* Get the next module in the ncx_modQ
* 
* INPUTS:
*   mod == current module to find next 
*
* RETURNS:
*   pointer to the first entry or NULL if empty Q
*********************************************************************/
extern ncx_module_t *
    ncx_get_next_module (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_first_session_module
* 
* Get the first module in the ncx_sesmodQ
* 
* RETURNS:
*   pointer to the first entry or NULL if empty Q
*********************************************************************/
extern ncx_module_t *
    ncx_get_first_session_module (void);


/********************************************************************
* FUNCTION ncx_get_next_session_module
* 
* Get the next module in the ncx_sesmodQ
* 
* RETURNS:
*   pointer to the first entry or NULL if empty Q
*********************************************************************/
extern ncx_module_t *
    ncx_get_next_session_module (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_modname
* 
* Get the main module name
* 
* INPUTS:
*   mod == module or submodule to get main module name
*
* RETURNS:
*   main module name or NULL if error
*********************************************************************/
extern const xmlChar *
    ncx_get_modname (const ncx_module_t *mod);



/********************************************************************
* FUNCTION ncx_get_mod_nsid
* 
* Get the main module namespace ID
* 
* INPUTS:
*   mod == module or submodule to get main module namespace ID
*
* RETURNS:
*   namespace id number
*********************************************************************/
extern xmlns_id_t
    ncx_get_mod_nsid (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_modversion
* 
* Get the [sub]module version

* INPUTS:
*   mod == module or submodule to get module version
* 
* RETURNS:
*   module version or NULL if error
*********************************************************************/
extern const xmlChar *
    ncx_get_modversion (const ncx_module_t *mod);



/********************************************************************
* FUNCTION ncx_get_modnamespace
* 
* Get the module namespace URI
*
* INPUTS:
*   mod == module or submodule to get module namespace
* 
* RETURNS:
*   module namespace or NULL if error
*********************************************************************/
extern const xmlChar *
    ncx_get_modnamespace (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_modsource
* 
* Get the module filespec source string
*
* INPUTS:
*   mod == module or submodule to use
* 
* RETURNS:
*   module filespec source string
*********************************************************************/
extern const xmlChar *
    ncx_get_modsource (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_mainmod
* 
* Get the main module
* 
* INPUTS:
*   mod == submodule to get main module
*
* RETURNS:
*   main module NULL if error
*********************************************************************/
extern ncx_module_t *
    ncx_get_mainmod (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_first_object
* 
* Get the first object in the datadefQs for the specified module
* Get any object with a name
* 
* INPUTS:
*   mod == module to search for the first object
*
* RETURNS:
*   pointer to the first object or NULL if empty Q
*********************************************************************/
extern obj_template_t *
    ncx_get_first_object (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_next_object
* 
* Get the next object in the specified module
* Get any object with a name
*
* INPUTS:
*    mod == module struct to get the next object from
*    curobj == pointer to the current object to get the next for
*
* RETURNS:
*   pointer to the next object or NULL if none
*********************************************************************/
extern obj_template_t *
    ncx_get_next_object (ncx_module_t *mod,
			 obj_template_t *curobj);


/********************************************************************
* FUNCTION ncx_get_first_data_object
* 
* Get the first database object in the datadefQs 
* for the specified module
* 
* INPUTS:
*   mod == module to search for the first object
*
* RETURNS:
*   pointer to the first object or NULL if empty Q
*********************************************************************/
extern obj_template_t *
    ncx_get_first_data_object (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_next_data_object
* 
* Get the next database object in the specified module
* 
* INPUTS:
*    mod == pointer to module to get object from
*    curobj == pointer to current object to get next from
*
* RETURNS:
*   pointer to the next object or NULL if none
*********************************************************************/
extern obj_template_t *
    ncx_get_next_data_object (ncx_module_t *mod,
			      obj_template_t *curobj);


/********************************************************************
* FUNCTION ncx_new_import
* 
* Malloc and initialize the fields in a ncx_import_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern ncx_import_t * 
    ncx_new_import (void);


/********************************************************************
* FUNCTION ncx_free_import
* 
* Scrub the memory in a ncx_import_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    import == ncx_import_t data structure to free
*********************************************************************/
extern void 
    ncx_free_import (ncx_import_t *import);


/********************************************************************
* FUNCTION ncx_find_import
* 
* Search the importQ for a specified module name
* 
* INPUTS:
*   mod == module to search (mod->importQ)
*   module == module name to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_import (const ncx_module_t *mod,
		     const xmlChar *module);


/********************************************************************
* FUNCTION ncx_find_import_que
* 
* Search the specified importQ for a specified module name
* 
* INPUTS:
*   importQ == Q of ncx_import_t to search
*   module == module name to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_import_que (const dlq_hdr_t *importQ,
                         const xmlChar *module);


/********************************************************************
* FUNCTION ncx_find_import_test
* 
* Search the importQ for a specified module name
* Do not set used flag
*
* INPUTS:
*   mod == module to search (mod->importQ)
*   module == module name to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_import_test (const ncx_module_t *mod,
			  const xmlChar *module);


/********************************************************************
* FUNCTION ncx_find_pre_import
* 
* Search the importQ for a specified prefix value
* 
* INPUTS:
*   mod == module to search (mod->importQ)
*   prefix == prefix string to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_pre_import (const ncx_module_t *mod,
			 const xmlChar *prefix);


/********************************************************************
* FUNCTION ncx_find_pre_import_que
* 
* Search the specified importQ for a specified prefix value
* 
* INPUTS:
*   importQ == Q of ncx_import_t to search
*   prefix == prefix string to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_pre_import_que (const dlq_hdr_t *importQ,
                             const xmlChar *prefix);


/********************************************************************
* FUNCTION ncx_find_pre_import_test
* 
* Search the importQ for a specified prefix value
* Test only, do not set used flag
*
* INPUTS:
*   mod == module to search (mod->importQ)
*   prefix == prefix string to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_import_t * 
    ncx_find_pre_import_test (const ncx_module_t *mod,
			      const xmlChar *prefix);


/********************************************************************
* FUNCTION ncx_locate_modqual_import
* 
* Search the specific module for the specified definition name.
*
* Okay for YANG or NCX
*
*  - typ_template_t (NCX_NT_TYP)
*  - grp_template_t (NCX_NT_GRP)
*  - obj_template_t (NCX_NT_OBJ)
*  - rpc_template_t  (NCX_NT_RPC)
*  - not_template_t (NCX_NT_NOTIF)
*
* INPUTS:
*     pcb == parser control block to use
*     imp == NCX import struct to use
*     defname == name of definition to find
*     *deftyp == specified type or NCX_NT_NONE if any will do
*
* OUTPUTS:
*    imp->mod may get set if not already
*    *deftyp == type retrieved if NO_ERR
*
* RETURNS:
*    pointer to the located definition or NULL if not found
*********************************************************************/
extern void *
    ncx_locate_modqual_import (yang_pcb_t *pcb,
                               ncx_import_t *imp,
			       const xmlChar *defname,
			       ncx_node_t *deftyp);


/********************************************************************
* FUNCTION ncx_new_include
* 
* Malloc and initialize the fields in a ncx_include_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern ncx_include_t * 
    ncx_new_include (void);


/********************************************************************
* FUNCTION ncx_free_include
* 
* Scrub the memory in a ncx_include_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    inc == ncx_include_t data structure to free
*********************************************************************/
extern void 
    ncx_free_include (ncx_include_t *inc);


/********************************************************************
* FUNCTION ncx_find_include
* 
* Search the includeQ for a specified submodule name
* 
* INPUTS:
*   mod == module to search (mod->includeQ)
*   submodule == submodule name to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_include_t * 
    ncx_find_include (const ncx_module_t *mod,
		      const xmlChar *submodule);


/********************************************************************
* FUNCTION ncx_new_binary
*
* Malloc and fill in a new ncx_binary_t struct
*
* INPUTS:
*   none
* RETURNS:
*   pointer to malloced and initialized ncx_binary_t struct
*   NULL if malloc error
*********************************************************************/
extern ncx_binary_t *
    ncx_new_binary (void);


/********************************************************************
* FUNCTION ncx_init_binary
* 
* Init the memory of a ncx_binary_t struct
*
* INPUTS:
*    binary == ncx_binary_t struct to init
*********************************************************************/
extern void
    ncx_init_binary (ncx_binary_t *binary);


/********************************************************************
* FUNCTION ncx_clean_binary
* 
* Scrub the memory of a ncx_binary_t but do not delete it
*
* INPUTS:
*    binary == ncx_binary_t struct to clean
*********************************************************************/
extern void
    ncx_clean_binary (ncx_binary_t *binary);


/********************************************************************
* FUNCTION ncx_free_binary
*
* Free all the memory in a  ncx_binary_t struct
*
* INPUTS:
*   binary == struct to clean and free
*
*********************************************************************/
extern void
    ncx_free_binary (ncx_binary_t *binary);


/********************************************************************
* FUNCTION ncx_new_identity
* 
* Get a new ncx_identity_t struct
*
* INPUTS:
*    none
* RETURNS:
*    pointer to a malloced ncx_identity_t struct,
*    or NULL if malloc error
*********************************************************************/
extern ncx_identity_t * 
    ncx_new_identity (void);


/********************************************************************
* FUNCTION ncx_free_identity
* 
* Free a malloced ncx_identity_t struct
*
* INPUTS:
*    identity == struct to free
*
*********************************************************************/
extern void 
    ncx_free_identity (ncx_identity_t *identity);


/********************************************************************
* FUNCTION ncx_find_identity
* 
* Find a ncx_identity_t struct in the module and perhaps
* any of its submodules
*
* INPUTS:
*    mod == module to search
*    name == identity name to find
*    useall == TRUE if all submodules should be checked
*              FALSE if only visible included submodules
*              should be checked
* RETURNS:
*    pointer to found feature or NULL if not found
*********************************************************************/
extern ncx_identity_t *
    ncx_find_identity (ncx_module_t *mod,
                       const xmlChar *name,
                       boolean useall);

/********************************************************************
* FUNCTION ncx_find_identity_que
* 
* Find a ncx_identity_t struct in the specified Q
*
* INPUTS:
*    identityQ == Q of ncx_identity_t to search
*    name == identity name to find
*
* RETURNS:
*    pointer to found identity or NULL if not found
*********************************************************************/
extern ncx_identity_t *
    ncx_find_identity_que (dlq_hdr_t *identityQ,
			   const xmlChar *name);


/********************************************************************
* FUNCTION ncx_new_filptr
* 
* Get a new ncx_filptr_t struct
*
* INPUTS:
*    none
* RETURNS:
*    pointer to a malloced or cached ncx_filptr_t struct,
*    or NULL if none available
*********************************************************************/
extern ncx_filptr_t *
    ncx_new_filptr (void);


/********************************************************************
* FUNCTION ncx_free_filptr
* 
* Free a new ncx_filptr_t struct or add to the cache if room
*
* INPUTS:
*    filptr == struct to free
* RETURNS:
*    none
*********************************************************************/
extern void 
    ncx_free_filptr (ncx_filptr_t *filptr);


/********************************************************************
* FUNCTION ncx_new_revhist
* 
* Create a revision history entry
*
* RETURNS:
*    malloced revision history entry or NULL if malloc error
*********************************************************************/
extern ncx_revhist_t * 
    ncx_new_revhist (void);


/********************************************************************
* FUNCTION ncx_free_revhist
* 
* Free a revision history entry
*
* INPUTS:
*    revhist == ncx_revhist_t data structure to free
*********************************************************************/
extern void 
    ncx_free_revhist (ncx_revhist_t *revhist);


/********************************************************************
* FUNCTION ncx_find_revhist
* 
* Search the revhistQ for a specified revision
* 
* INPUTS:
*   mod == module to search (mod->importQ)
*   ver == version string to find
*
* RETURNS:
*   pointer to the node if found, NULL if not found
*********************************************************************/
extern ncx_revhist_t * 
    ncx_find_revhist (const ncx_module_t *mod,
		      const xmlChar *ver);


/********************************************************************
* FUNCTION ncx_init_enum
* 
* Init the memory of a ncx_enum_t
*
* INPUTS:
*    enu == ncx_enum_t struct to init
*********************************************************************/
extern void
    ncx_init_enum (ncx_enum_t *enu);


/********************************************************************
* FUNCTION ncx_clean_enum
* 
* Scrub the memory of a ncx_enum_t but do not delete it
*
* INPUTS:
*    enu == ncx_enum_t struct to clean
*********************************************************************/
extern void
    ncx_clean_enum (ncx_enum_t *enu);


/********************************************************************
* FUNCTION ncx_compare_enums
* 
* Compare 2 enum values
*
* INPUTS:
*    enu1 == first  ncx_enum_t check
*    enu2 == second ncx_enum_t check
*   
* RETURNS:
*     -1 if enu1 is < enu2
*      0 if enu1 == enu2
*      1 if enu1 is > enu2

*********************************************************************/
extern int32
    ncx_compare_enums (const ncx_enum_t *enu1,
		       const ncx_enum_t *enu2);


/********************************************************************
* FUNCTION ncx_set_enum
* 
* Parse an enumerated integer string into an ncx_enum_t
* without matching it against any typdef
*
* Mallocs a copy of the enum name, using the enu->dname field
*
* INPUTS:
*    enumval == enum string value to parse
*    retenu == pointer to return enuym variable to fill in
*    
* OUTPUTS:
*    *retenu == enum filled in
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    ncx_set_enum (const xmlChar *enumval,
		  ncx_enum_t *retenu);


/********************************************************************
* FUNCTION ncx_init_bit
* 
* Init the memory of a ncx_bit_t
*
* INPUTS:
*    bit == ncx_bit_t struct to init
*********************************************************************/
extern void
    ncx_init_bit (ncx_bit_t *bit);


/********************************************************************
* FUNCTION ncx_clean_bit
* 
* Scrub the memory of a ncx_bit_t but do not delete it
*
* INPUTS:
*    bit == ncx_bit_t struct to clean
*********************************************************************/
extern void
    ncx_clean_bit (ncx_bit_t *bit);


/********************************************************************
* FUNCTION ncx_compare_bits
* 
* Compare 2 bit values by their schema order position
*
* INPUTS:
*    bitone == first ncx_bit_t check
*    bitone == second ncx_bit_t check
*   
* RETURNS:
*     -1 if bitone is < bittwo
*      0 if bitone == bittwo
*      1 if bitone is > bittwo
*
*********************************************************************/
extern int32
    ncx_compare_bits (const ncx_bit_t *bitone,
		      const ncx_bit_t *bittwo);


/********************************************************************
* FUNCTION ncx_new_typname
* 
*   Malloc and init a typname struct
*
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
extern ncx_typname_t *
    ncx_new_typname (void);


/********************************************************************
* FUNCTION ncx_free_typname
* 
*   Free a typname struct
*
* INPUTS:
*    typnam == ncx_typname_t struct to free
*
*********************************************************************/
extern void
    ncx_free_typname (ncx_typname_t *typnam);


/********************************************************************
* FUNCTION ncx_find_typname
* 
*   Find a typname struct in the specified Q for a typ pointer
*
* INPUTS:
*    que == Q of ncx_typname_t struct to check
*    typ == matching type template to find
*
* RETURNS:
*   name assigned to this type template
*********************************************************************/
extern const xmlChar *
    ncx_find_typname (const typ_template_t *typ,
		      const dlq_hdr_t *que);


/********************************************************************
* FUNCTION ncx_find_typname_type
* 
*   Find a typ_template_t pointer in a typename mapping, 
*   in the specified Q
*
* INPUTS:
*    que == Q of ncx_typname_t struct to check
*    typname == matching type name to find
*
* RETURNS:
*   pointer to the stored typstatus
*********************************************************************/
extern const typ_template_t *
    ncx_find_typname_type (const dlq_hdr_t *que,
			   const xmlChar *typname);


/********************************************************************
* FUNCTION ncx_clean_typnameQ
* 
*   Delete all the Q entries, of typname mapping structs
*
* INPUTS:
*    que == Q of ncx_typname_t struct to delete
*
*********************************************************************/
extern void
    ncx_clean_typnameQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION ncx_get_gen_anyxml
* 
* Get the object template for the NCX generic anyxml container
*
* RETURNS:
*   pointer to generic anyxml object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_anyxml (void);


/********************************************************************
* FUNCTION ncx_get_gen_container
* 
* Get the object template for the NCX generic container
*
* RETURNS:
*   pointer to generic container object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_container (void);


/********************************************************************
* FUNCTION ncx_get_gen_string
* 
* Get the object template for the NCX generic string leaf
*
* RETURNS:
*   pointer to generic string object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_string (void);


/********************************************************************
* FUNCTION ncx_get_gen_empty
* 
* Get the object template for the NCX generic empty leaf
*
* RETURNS:
*   pointer to generic empty object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_empty (void);


/********************************************************************
* FUNCTION ncx_get_gen_root
* 
* Get the object template for the NCX generic root container
*
* RETURNS:
*   pointer to generic root container object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_root (void);


/********************************************************************
* FUNCTION ncx_get_gen_binary
* 
* Get the object template for the NCX generic binary leaf
*
* RETURNS:
*   pointer to generic binary object template
*********************************************************************/
extern obj_template_t *
    ncx_get_gen_binary (void);


/********************************************************************
* FUNCTION ncx_get_layer
*
* translate ncx_layer_t enum to a string
* Get the ncx_layer_t string
*
* INPUTS:
*   layer == ncx_layer_t to convert to a string
*
* RETURNS:
*  const pointer to the string value
*********************************************************************/
extern const xmlChar *
    ncx_get_layer (ncx_layer_t  layer);


/********************************************************************
* FUNCTION ncx_get_name_segment
* 
* Get the name string between the dots
*
* INPUTS:
*    str == scoped string
*    buff == address of return buffer
*    buffsize == buffer size
*
* OUTPUTS:
*    buff is filled in with the namestring segment
*
* RETURNS:
*    current string pointer after operation
*********************************************************************/
extern const xmlChar *
    ncx_get_name_segment (const xmlChar *str,
			  xmlChar  *buff,
			  uint32 buffsize);


/********************************************************************
* FUNCTION ncx_get_cvttyp_enum
* 
* Get the enum for the string name of a ncx_cvttyp_t enum
* 
* INPUTS:
*   str == string name of the enum value 
*
* RETURNS:
*   enum value
*********************************************************************/
extern ncx_cvttyp_t
    ncx_get_cvttyp_enum (const char *str);


/********************************************************************
* FUNCTION ncx_get_status_enum
* 
* Get the enum for the string name of a ncx_status_t enum
* 
* INPUTS:
*   str == string name of the enum value 
*
* RETURNS:
*   enum value
*********************************************************************/
extern ncx_status_t
    ncx_get_status_enum (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_get_status_string
* 
* Get the string for the enum value of a ncx_status_t enum
* 
* INPUTS:
*   status == enum value
*
* RETURNS:
*   string name of the enum value 
*********************************************************************/
extern const xmlChar *
    ncx_get_status_string (ncx_status_t status);


/********************************************************************
* FUNCTION ncx_check_yang_status
* 
* Check the backward compatibility of the 2 YANG status fields
* 
* INPUTS:
*   mystatus == enum value for the node to be tested
*   depstatus == status value of the dependency
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    ncx_check_yang_status (ncx_status_t mystatus,
			   ncx_status_t depstatus);


/********************************************************************
* FUNCTION ncx_save_descr
* 
* Get the value of the save description strings variable
*
* RETURNS:
*    TRUE == descriptive strings should be save
*    FALSE == descriptive strings should not be saved
*********************************************************************/
extern boolean
    ncx_save_descr (void);


/********************************************************************
* FUNCTION ncx_print_errormsg
* 
*   Print an parse error message to STDOUT
*
* INPUTS:
*   tkc == token chain   (may be NULL)
*   mod == module in progress  (may be NULL)
*   res == error status
*
* RETURNS:
*   none
*********************************************************************/
extern void
    ncx_print_errormsg (tk_chain_t *tkc,
			ncx_module_t  *mod,
			status_t     res);


/********************************************************************
* FUNCTION ncx_print_errormsg_ex
* 
*   Print an parse error message to STDOUT (Extended)
*
* INPUTS:
*   tkc == token chain   (may be NULL)
*   mod == module in progress  (may be NULL)
*   res == error status
*   filename == script finespec
*   linenum == script file number
*   fineoln == TRUE if finish with a newline, FALSE if not
*
* RETURNS:
*   none
*********************************************************************/
extern void
    ncx_print_errormsg_ex (tk_chain_t *tkc,
			   ncx_module_t  *mod,
			   status_t     res,
			   const char *filename,
			   uint32 linenum,
			   boolean fineoln);


/********************************************************************
* FUNCTION ncx_conf_exp_err
* 
* Print an error for wrong token, expected a different token
* 
* INPUTS:
*   tkc == token chain
*   result == error code
*   expstr == expected token description
*
*********************************************************************/
extern void
    ncx_conf_exp_err (tk_chain_t  *tkc,
		      status_t result,
		      const char *expstr);


/********************************************************************
* FUNCTION ncx_mod_exp_err
* 
* Print an error for wrong token, expected a different token
* 
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   result == error code
*   expstr == expected token description
*
*********************************************************************/
extern void
    ncx_mod_exp_err (tk_chain_t  *tkc,
		     ncx_module_t *mod,
		     status_t result,
		     const char *expstr);


/********************************************************************
* FUNCTION ncx_mod_missing_err
* 
* Print an error for wrong token, mandatory
* sub-statement is missing
* 
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   stmtstr == parent statement
*   expstr == expected sub-statement
*
*********************************************************************/
extern void
    ncx_mod_missing_err (tk_chain_t  *tkc,
                         ncx_module_t *mod,
                         const char *stmtstr,
                         const char *expstr);


/********************************************************************
* FUNCTION ncx_free_node
* 
* Delete a node based on its type
*
* INPUTS:
*     nodetyp == NCX node type
*     node == node top free
*
*********************************************************************/
extern void
    ncx_free_node (ncx_node_t nodetyp,
		   void *node);


/********************************************************************
* FUNCTION ncx_get_data_class_enum
* 
* Get the enum for the string name of a ncx_data_class_t enum
* 
* INPUTS:
*   str == string name of the enum value 
*
* RETURNS:
*   enum value
*********************************************************************/
extern ncx_data_class_t 
    ncx_get_data_class_enum (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_get_data_class_str
* 
* Get the string value for the ncx_data_class_t enum
* 
* INPUTS:
*   dataclass == enum value to convert
*
* RETURNS:
*   striong value for the enum
*********************************************************************/
extern const xmlChar *
    ncx_get_data_class_str (ncx_data_class_t dataclass);


/********************************************************************
* FUNCTION ncx_get_access_str
* 
* Get the string name of a ncx_access_t enum
* 
* INPUTS:
*   access == enum value
*
* RETURNS:
*   string value
*********************************************************************/
extern const xmlChar * 
    ncx_get_access_str (ncx_access_t max_access);


/********************************************************************
* FUNCTION ncx_get_access_enum
* 
* Get the enum for the string name of a ncx_access_t enum
* 
* INPUTS:
*   str == string name of the enum value 
*
* RETURNS:
*   enum value
*********************************************************************/
extern ncx_access_t
    ncx_get_access_enum (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_get_tclass
* 
* Get the token class
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     tclass enum
*********************************************************************/
extern ncx_tclass_t
    ncx_get_tclass (ncx_btype_t btyp);


/********************************************************************
* FUNCTION ncx_get_tclass
* 
* Get the token class
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     tclass enum
*********************************************************************/
extern boolean
    ncx_valid_name_ch (uint32 ch);


/********************************************************************
* FUNCTION ncx_valid_fname_ch
* 
* Check if an xmlChar is a valid NCX name string first char
*
* INPUTS:
*   ch == xmlChar to check
* RETURNS:
*   TRUE if a valid first name char, FALSE otherwise
*********************************************************************/
extern boolean
    ncx_valid_fname_ch (uint32 ch);


/********************************************************************
* FUNCTION ncx_valid_name
* 
* Check if an xmlChar string is a valid YANG identifier value
*
* INPUTS:
*   str == xmlChar string to check
*   len == length of the string to check (in case of substr)
* RETURNS:
*   TRUE if a valid name string, FALSE otherwise
*********************************************************************/
extern boolean
    ncx_valid_name (const xmlChar *str, 
		    uint32 len);


/********************************************************************
* FUNCTION ncx_valid_name2
* 
* Check if an xmlChar string is a valid NCX name
*
* INPUTS:
*   str == xmlChar string to check (zero-terminated)

* RETURNS:
*   TRUE if a valid name string, FALSE otherwise
*********************************************************************/
extern boolean
    ncx_valid_name2 (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_parse_name
* 
* Check if the next N chars represent a valid NcxName
* Will end on the first non-name char
*
* INPUTS:
*   str == xmlChar string to check
*   len == address of name length
*
* OUTPUTS:
*   *len == 0 if no valid name parsed
*         > 0 for the numbers of chars in the NcxName
*
* RETURNS:
*   status_t  (error if name too long)
*********************************************************************/
extern status_t
    ncx_parse_name (const xmlChar *str,
		    uint32 *len);


/********************************************************************
* FUNCTION ncx_is_true
* 
* Check if an xmlChar string is a string OK for XSD boolean
*
* INPUTS:
*   str == xmlChar string to check
*
* RETURNS:
*   TRUE if a valid boolean value indicating true
*   FALSE otherwise
*********************************************************************/
extern boolean
    ncx_is_true (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_is_false
* 
* Check if an xmlChar string is a string OK for XSD boolean
*
* INPUTS:
*   str == xmlChar string to check
*
* RETURNS:
*   TRUE if a valid boolean value indicating false
*   FALSE otherwise
*********************************************************************/
extern boolean
    ncx_is_false (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_consume_tstring
* 
* Consume a TK_TT_TSTRING with the specified value
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain 
*   mod == module in progress (NULL if none)
*   name == token name
*   opt == TRUE for optional param
*       == FALSE for mandatory param
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_consume_tstring (tk_chain_t *tkc,
			 ncx_module_t  *mod,
			 const xmlChar *name,
			 ncx_opt_t opt);


/********************************************************************
* FUNCTION ncx_consume_name
* 
* Consume a TK_TSTRING that matches the 'name', then
* retrieve the next TK_TSTRING token into the namebuff
* If ctk specified, then consume the specified close token
*
* Store the results in a malloced buffer
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain 
*   mod == module in progress (NULL if none)
*   name == first token name
*   namebuff == ptr to output name string
*   opt == NCX_OPT for optional param
*       == NCX_REQ for mandatory param
*   ctyp == close token (use TK_TT_NONE to skip this part)
*
* OUTPUTS:
*   *namebuff points at the malloced name string
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_consume_name (tk_chain_t *tkc,
		      ncx_module_t *mod,
		      const xmlChar *name,
		      xmlChar **namebuff,
		      ncx_opt_t opt,
		      tk_type_t  ctyp);


/********************************************************************
* FUNCTION ncx_consume_token
* 
* Consume the next token which should be a 1 or 2 char token
* without any value. However this function does not check the value,
* just the token type.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain 
*   mod == module in progress (NULL if none)
*   ttyp == token type
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_consume_token (tk_chain_t *tkc,
		       ncx_module_t *mod,
		       tk_type_t  ttyp);


/********************************************************************
* FUNCTION ncx_new_errinfo
* 
* Malloc and init a new ncx_errinfo_t
*
* RETURNS:
*    pointer to malloced ncx_errinfo_t, or NULL if memory error
*********************************************************************/
extern ncx_errinfo_t *
    ncx_new_errinfo (void);


/********************************************************************
* FUNCTION ncx_init_errinfo
* 
* Init the fields in an ncx_errinfo_t struct
*
* INPUTS:
*    err == ncx_errinfo_t data structure to init
*********************************************************************/
extern void 
    ncx_init_errinfo (ncx_errinfo_t *err);


/********************************************************************
* FUNCTION ncx_clean_errinfo
* 
* Scrub the memory in a ncx_errinfo_t by freeing all
* the sub-fields
*
* INPUTS:
*    err == ncx_errinfo_t data structure to clean
*********************************************************************/
extern void 
    ncx_clean_errinfo (ncx_errinfo_t *err);


/********************************************************************
* FUNCTION ncx_free_errinfo
* 
* Scrub the memory in a ncx_errinfo_t by freeing all
* the sub-fields, then free the errinfo struct
*
* INPUTS:
*    err == ncx_errinfo_t data structure to free
*********************************************************************/
extern void 
    ncx_free_errinfo (ncx_errinfo_t *err);


/********************************************************************
* FUNCTION ncx_errinfo_set
* 
* Check if error-app-tag or error-message set
* Check if the errinfo struct is set or empty
* Checks only the error_app_tag and error_message fields
*
* INPUTS:
*    errinfo == ncx_errinfo_t struct to check
*
* RETURNS:
*   TRUE if at least one field set
*   FALSE if the errinfo struct is empty
*********************************************************************/
extern boolean
    ncx_errinfo_set (const ncx_errinfo_t *errinfo);


/********************************************************************
* FUNCTION ncx_copy_errinfo
* 
* Copy the fields from one errinfo to a blank errinfo
*
* INPUTS:
*    src == struct with starting contents
*    dest == struct to get copy of src contents
*
* OUTPUTS:
*    *dest fields set which are set in src
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_copy_errinfo (const ncx_errinfo_t *src,
		      ncx_errinfo_t *dest);



/********************************************************************
* FUNCTION ncx_get_source_ex
* 
* Get a malloced buffer containing the complete filespec
* for the given input string.  If this is a complete dirspec,
* this this will just strdup the value.
*
* This is just a best effort to get the full spec.
* If the full spec is greater than 1500 bytes,
* then a NULL value (error) will be returned
*
*   - Change ./ --> cwd/
*   - Remove ~/  --> $HOME
*   - add trailing '/' if not present
*
* INPUTS:
*    fspec == input filespec
*    expand_cwd == TRUE if foo should be expanded to /cur/dir/foo
*                  FALSE if not
*    res == address of return status
*
* OUTPUTS:
*   *res == return status, NO_ERR if return is non-NULL
*
* RETURNS:
*   malloced buffer containing possibly expanded full filespec
*********************************************************************/
extern xmlChar *
    ncx_get_source_ex (const xmlChar *fspec,
                       boolean expand_cwd,
                       status_t *res);


/********************************************************************
* FUNCTION ncx_get_source
* 
* Get a malloced buffer containing the complete filespec
* for the given input string.  If this is a complete dirspec,
* this this will just strdup the value.
*
* This is just a best effort to get the full spec.
* If the full spec is greater than 1500 bytes,
* then a NULL value (error) will be returned
*
* This will expand the cwd!
*
*   - Change ./ --> cwd/
*   - Remove ~/  --> $HOME
*   - add trailing '/' if not present
*
* INPUTS:
*    fspec == input filespec
*    res == address of return status
*
* OUTPUTS:
*   *res == return status, NO_ERR if return is non-NULL
*
* RETURNS:
*   malloced buffer containing possibly expanded full filespec
*********************************************************************/
extern xmlChar *
    ncx_get_source (const xmlChar *fspec,
                    status_t *res);


/********************************************************************
* FUNCTION ncx_set_cur_modQ
* 
* Set the current module Q to an alternate (for yangdiff)
* This will be used for module searches usually in ncx_modQ
*
* INPUTS:
*    que == Q of ncx_module_t to use
*********************************************************************/
extern void
    ncx_set_cur_modQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION ncx_reset_modQ
* 
* Set the current module Q to the original ncx_modQ
*
*********************************************************************/
extern void
    ncx_reset_modQ (void);


/********************************************************************
* FUNCTION ncx_set_session_modQ
* 
* !!! THIS HACK IS NEEDED BECAUSE val.c
* !!! USES ncx_find_module sometimes, and
* !!! yangcli sessions are not loaded into the
* !!! main database of modules.
* !!! THIS DOES NOT WORK FOR MULTIPLE CONCURRENT PROCESSES
*
* Set the current session module Q to an alternate (for yangdiff)
* This will be used for module searches usually in ncx_modQ
*
* INPUTS:
*    que == Q of ncx_module_t to use
*********************************************************************/
extern void
    ncx_set_session_modQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION ncx_clear_session_modQ
* 
* !!! THIS HACK IS NEEDED BECAUSE val.c
* !!! USES ncx_find_module sometimes, and
* !!! yangcli sessions are not loaded into the
* !!! main database of modules.
* !!! THIS DOES NOT WORK FOR MULTIPLE CONCURRENT PROCESSES
*
* Clear the current session module Q
*
*********************************************************************/
extern void
    ncx_clear_session_modQ (void);


/********************************************************************
* FUNCTION ncx_set_load_callback
* 
* Set the callback function for a load-module event
*
* INPUT:
*   cbfn == callback function to use
*
*********************************************************************/
extern void
    ncx_set_load_callback (ncx_load_cbfn_t cbfn);


/********************************************************************
* FUNCTION ncx_prefix_different
* 
* Check if the specified prefix pair reference different modules
* 
* INPUT:
*   prefix1 == 1st prefix to check (may be NULL)
*   prefix2 == 2nd prefix to check (may be NULL)
*   modprefix == module prefix to check (may be NULL)
*
* RETURNS:
*   TRUE if prefix1 and prefix2 reference different modules
*   FALSE if prefix1 and prefix2 reference the same modules
*********************************************************************/
extern boolean
    ncx_prefix_different (const xmlChar *prefix1,
			  const xmlChar *prefix2,
			  const xmlChar *modprefix);


/********************************************************************
* FUNCTION ncx_get_baddata_enum
* 
* Check if the specified string matches an ncx_baddata_t enum
* 
* INPUT:
*   valstr == value string to check
*
* RETURNS:
*   enum value if OK
*   NCX_BAD_DATA_NONE if an error
*********************************************************************/
extern ncx_bad_data_t
    ncx_get_baddata_enum (const xmlChar *valstr);


/********************************************************************
* FUNCTION ncx_get_baddata_string
* 
* Get the string for the specified enum value
* 
* INPUT:
*   baddatar == enum value to check
*
* RETURNS:
*   string pointer if OK
*   NULL if an error
*********************************************************************/
extern const xmlChar *
    ncx_get_baddata_string (ncx_bad_data_t baddata);



/********************************************************************
* FUNCTION ncx_get_withdefaults_string
* 
* Get the string for the specified enum value
* 
* INPUT:
*   withdef == enum value to check
*
* RETURNS:
*   string pointer if OK
*   NULL if an error
*********************************************************************/
extern const xmlChar *
    ncx_get_withdefaults_string (ncx_withdefaults_t withdef);


/********************************************************************
* FUNCTION ncx_get_withdefaults_enum
* 
* Get the enum for the specified string value
* 
* INPUT:
*   withdefstr == string value to check
*
* RETURNS:
*   enum value for the string
*   NCX_WITHDEF_NONE if invalid value
*********************************************************************/
extern ncx_withdefaults_t
    ncx_get_withdefaults_enum (const xmlChar *withdefstr);


/********************************************************************
* FUNCTION ncx_get_mod_prefix
* 
* Get the module prefix for the specified module
* 
* INPUT:
*   mod == module to check
*
* RETURNS:
*   pointer to module YANG prefix
*********************************************************************/
extern const xmlChar *
    ncx_get_mod_prefix (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_mod_xmlprefix
* 
* Get the module XML prefix for the specified module
* 
* INPUT:
*   mod == module to check
*
* RETURNS:
*   pointer to module XML prefix
*********************************************************************/
extern const xmlChar *
    ncx_get_mod_xmlprefix (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_display_mode_enum
* 
* Get the enum for the specified string value
* 
* INPUT:
*   dmstr == string value to check
*
* RETURNS:
*   enum value for the string
*   NCX_DISPLAY_MODE_NONE if invalid value
*********************************************************************/
extern ncx_display_mode_t
    ncx_get_display_mode_enum (const xmlChar *dmstr);


/********************************************************************
* FUNCTION ncx_get_display_mode_str
* 
* Get the string for the specified enum value
* 
* INPUT:
*   dmode == enum display mode value to check
*
* RETURNS:
*   string value for the enum
*   NULL if none found
*********************************************************************/
extern const xmlChar *
    ncx_get_display_mode_str (ncx_display_mode_t dmode);


/********************************************************************
* FUNCTION ncx_set_warn_idlen
* 
* Set the warning length for identifiers
* 
* INPUT:
*   warnlen == warning length to use
*
*********************************************************************/
extern void
    ncx_set_warn_idlen (uint32 warnlen);


/********************************************************************
* FUNCTION ncx_get_warn_idlen
* 
* Get the warning length for identifiers
* 
* RETURNS:
*   warning length to use
*********************************************************************/
extern uint32
    ncx_get_warn_idlen (void);


/********************************************************************
* FUNCTION ncx_set_warn_linelen
* 
* Set the warning length for YANG file lines
* 
* INPUT:
*   warnlen == warning length to use
*
*********************************************************************/
extern void
    ncx_set_warn_linelen (uint32 warnlen);


/********************************************************************
* FUNCTION ncx_get_warn_linelen
* 
* Get the warning length for YANG file lines
* 
* RETURNS:
*   warning length to use
*
*********************************************************************/
extern uint32
    ncx_get_warn_linelen (void);


/********************************************************************
* FUNCTION ncx_check_warn_idlen
* 
* Check if the identifier length is greater than
* the specified amount.
*
* INPUTS:
*   tkc == token chain to use (for warning message only)
*   mod == module (for warning message only)
*   id == identifier string to check; must be Z terminated
*
* OUTPUTS:
*    may generate log_warn ouput
*********************************************************************/
extern void
    ncx_check_warn_idlen (tk_chain_t *tkc,
                          ncx_module_t *mod,
                          const xmlChar *id);


/********************************************************************
* FUNCTION ncx_check_warn_linelen
* 
* Check if the line display length is greater than
* the specified amount.
*
* INPUTS:
*   tkc == token chain to use
*   mod == module (used in warning message only
*   linelen == line length to check
*
* OUTPUTS:
*    may generate log_warn ouput
*********************************************************************/
extern void
    ncx_check_warn_linelen (tk_chain_t *tkc,
                            ncx_module_t *mod,
                            const xmlChar *line);


/********************************************************************
* FUNCTION ncx_turn_off_warning
* 
* Add ar warning suppression entry
*
* INPUTS:
*   res == internal status code to suppress
*
* RETURNS:
*   status (duplicates are silently dropped)
*********************************************************************/
extern status_t
    ncx_turn_off_warning (status_t res);


/********************************************************************
* FUNCTION ncx_turn_on_warning
* 
* Remove a warning suppression entry if it exists
*
* INPUTS:
*   res == internal status code to enable
*
* RETURNS:
*   status (duplicates are silently dropped)
*********************************************************************/
extern status_t
    ncx_turn_on_warning (status_t res);


/********************************************************************
* FUNCTION ncx_warning_enabled
* 
* Check if a specific status_t code is enabled
*
* INPUTS:
*   res == internal status code to check
*
* RETURNS:
*   TRUE if warning is enabled
*   FALSE if warning is suppressed
*********************************************************************/
extern boolean
    ncx_warning_enabled (status_t res);


/********************************************************************
* FUNCTION ncx_get_version
* 
* Get the the Yuma version ID string
*
* INPUT:
*    buffer == buffer to hold the version string
*    buffsize == number of bytes in buffer
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_get_version (xmlChar *buffer,
                     uint32 buffsize);

/********************************************************************
* FUNCTION ncx_new_save_deviations
* 
* create a deviation save structure
*
* INPUTS:
*   devmodule == deviations module name
*   devrevision == deviation module revision (optional)
*   devnamespace == deviation module namespace URI value
*   devprefix == local module prefix (optional)
*
* RETURNS:
*   malloced and initialized save_deviations struct, 
*   or NULL if malloc error
*********************************************************************/
extern ncx_save_deviations_t *
    ncx_new_save_deviations (const xmlChar *devmodule,
                             const xmlChar *devrevision,
                             const xmlChar *devnamespace,
                             const xmlChar *devprefix);


/********************************************************************
* FUNCTION ncx_free_save_deviations
* 
* free a deviation save struct
*
* INPUT:
*    savedev == struct to clean and delete
*********************************************************************/
extern void
    ncx_free_save_deviations (ncx_save_deviations_t *savedev);


/********************************************************************
* FUNCTION ncx_clean_save_deviationsQ
* 
* clean a Q of deviation save structs
*
* INPUT:
*    savedevQ == Q of ncx_save_deviations_t to clean
*********************************************************************/
extern void
    ncx_clean_save_deviationsQ (dlq_hdr_t *savedevQ);


/********************************************************************
* FUNCTION ncx_set_error
* 
* Set the fields in an ncx_error_t struct
* When called from NACM or <get> internally, there is no
* module or line number info
*
* INPUTS:
*   tkerr== address of ncx_error_t struct to set
*   mod == [sub]module containing tkerr
*   linenum == current linenum
*   linepos == current column position on the current line
*
* OUTPUTS:
*   *tkerr is filled in
*********************************************************************/
extern void
    ncx_set_error (ncx_error_t *tkerr,
                   ncx_module_t *mod,
                   uint32 linenum,
                   uint32 linepos);


/********************************************************************
* FUNCTION ncx_set_temp_modQ
* 
* Set the temp_modQ for yangcli session-specific module list
*
* INPUTS:
*   modQ == new Q pointer to use
*
*********************************************************************/
extern void
    ncx_set_temp_modQ (dlq_hdr_t *modQ);


/********************************************************************
* FUNCTION ncx_get_temp_modQ
* 
* Get the temp_modQ for yangcli session-specific module list
*
* RETURNS:
*   pointer to the temp modQ, if set
*********************************************************************/
extern dlq_hdr_t *
    ncx_get_temp_modQ (void);


/********************************************************************
* FUNCTION ncx_clear_temp_modQ
* 
* Clear the temp_modQ for yangcli session-specific module list
*
*********************************************************************/
extern void
    ncx_clear_temp_modQ (void);


/********************************************************************
* FUNCTION ncx_get_display_mode
* 
* Get the current default display mode
*
* RETURNS:
*  the current dispay mode enumeration
*********************************************************************/
extern ncx_display_mode_t
    ncx_get_display_mode (void);


/********************************************************************
* FUNCTION ncx_get_confirm_event_str
* 
* Get the string for the specified enum value
* 
* INPUT:
*   event == enum confirm event value to convert
*
* RETURNS:
*   string value for the enum
*   NULL if none found
*********************************************************************/
extern const xmlChar *
    ncx_get_confirm_event_str (ncx_confirm_event_t event);


/********************************************************************
* FUNCTION ncx_mod_revision_count
*
* Find all the ncx_module_t structs in the ncx_modQ
* that have the same module name
*
* INPUTS:
*   modname == module name
*
* RETURNS:
*   count of modules that have this name (exact match)
*********************************************************************/
extern uint32
    ncx_mod_revision_count (const xmlChar *modname);


/********************************************************************
* FUNCTION ncx_mod_revision_count_que
*
* Find all the ncx_module_t structs in the specified queue
* that have the same module name
*
* INPUTS:
*   modQ == queue of ncx_module_t structs to check
*   modname == module name
*
* RETURNS:
*   count of modules that have this name (exact match)
*********************************************************************/
extern uint32
    ncx_mod_revision_count_que (dlq_hdr_t *modQ,
                                const xmlChar *modname);




/********************************************************************
* FUNCTION ncx_get_allincQ
*
* Find the correct Q of yang_node_t for all include files
* that have the same 'belongs-to' value
*
* INPUTS:
*   mod == module to check
*
* RETURNS:
*   pointer to Q  of all include nodes
*********************************************************************/
extern dlq_hdr_t *
    ncx_get_allincQ (ncx_module_t *mod);

/********************************************************************
* FUNCTION ncx_get_const_allincQ
*
* Find the correct Q of yang_node_t for all include files
* that have the same 'belongs-to' value (const version)
*
* INPUTS:
*   mod == module to check
*
* RETURNS:
*   pointer to Q  of all include nodes
*********************************************************************/
extern const dlq_hdr_t *
    ncx_get_const_allincQ (const ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_parent_mod
*
* Find the correct module by checking mod->parent nodes
*
* INPUTS:
*   mod == module to check
*
* RETURNS:
*   pointer to parent module (!! not submodule !!)
*   NULL if none found
*********************************************************************/
extern ncx_module_t *
    ncx_get_parent_mod (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_vtimeout_value
*
* Get the virtual node cache timeout value
*
* RETURNS:
*   number of seconds for the cache timeout; 0 == disabled
*********************************************************************/
extern uint32
    ncx_get_vtimeout_value (void);


/********************************************************************
* FUNCTION ncx_compare_base_uris
*
* Compare the base part of 2 URI strings
*
* INPUTS:
*    str1 == URI string 1
*    str2 == URI string 2
*
* RETURNS:
*   compare of base parts (up to '?')
*   -1, 0 or 1
*********************************************************************/
extern int32
    ncx_compare_base_uris (const xmlChar *str1,
                           const xmlChar *str2);


/********************************************************************
* FUNCTION ncx_get_useprefix
*
* Get the use_prefix value
*
* RETURNS:
*   TRUE if XML prefixes should be used
*   FALSE if XML messages should use default namespace (no prefix)
*********************************************************************/
extern boolean
    ncx_get_useprefix (void);


/********************************************************************
* FUNCTION ncx_set_useprefix
*
* Set the use_prefix value
*
* INPUTS:
*   val == 
*      TRUE if XML prefixes should be used
*      FALSE if XML messages should use default namespace (no prefix)
*********************************************************************/
extern void
    ncx_set_useprefix (boolean val);


/********************************************************************
* FUNCTION ncx_get_system_sorted
*
* Get the system_sorted value
*
* RETURNS:
*   TRUE if system ordered objects should be sorted
*   FALSE if system ordered objects should not be sorted
*********************************************************************/
extern boolean
    ncx_get_system_sorted (void);


/********************************************************************
* FUNCTION ncx_set_system_sorted
*
* Set the system_sorted value
*
* INPUTS:
*   val == 
*     TRUE if system ordered objects should be sorted
*     FALSE if system ordered objects should not be sorted
*********************************************************************/
extern void
    ncx_set_system_sorted (boolean val);


/********************************************************************
* FUNCTION ncx_inc_warnings
*
* Increment the module warning count
*
* INPUTS:
*   mod == module being parsed
*********************************************************************/
extern void
    ncx_inc_warnings (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_cwd_subdirs
*
* Get the CLI parameter value whether to search for modules
* in subdirs of the CWD by default.  Does not affect YUMA_MODPATH
* or other hard-wired searches
*
* RETURNS:
*   TRUE if ncxmod should search for modules in subdirs of the CWD
*   FALSE if ncxmod should not search for modules in subdirs of the CWD
*********************************************************************/
extern boolean
    ncx_get_cwd_subdirs (void);

/********************************************************************
* FUNCTION ncx_protocol_enabled
*
* Check if the specified protocol version is enabled
*
* RETURNS:
*   TRUE if protocol enabled
*   FALSE if protocol not enabled or error
*********************************************************************/
extern boolean
    ncx_protocol_enabled (ncx_protocol_t proto);


/********************************************************************
* FUNCTION ncx_set_protocol_enabled
*
* Set the specified protocol version to be enabled
*
* INPUTS:
*   proto == protocol version to enable
*********************************************************************/
extern void
    ncx_set_protocol_enabled (ncx_protocol_t proto);


/********************************************************************
* FUNCTION ncx_set_use_deadmodQ
*
* Set the usedeadmodQ flag
*
*********************************************************************/
extern void
    ncx_set_use_deadmodQ (void);


/********************************************************************
* FUNCTION ncx_delete_all_obsolete_objects
* 
* Go through all the modules and delete the obsolete nodes
* 
*********************************************************************/
extern void
    ncx_delete_all_obsolete_objects (void);


/********************************************************************
* FUNCTION ncx_delete_mod_obsolete_objects
* 
* Go through one module and delete the obsolete nodes
* 
* INPUTS:
*    mod == module to check
*********************************************************************/
extern void
    ncx_delete_mod_obsolete_objects (ncx_module_t *mod);


/********************************************************************
* FUNCTION ncx_get_name_match_enum
* 
* Get the enum for the string name of a ncx_name_match_t enum
* 
* INPUTS:
*   str == string name of the enum value 
*
* RETURNS:
*   enum value
*********************************************************************/
extern ncx_name_match_t
    ncx_get_name_match_enum (const xmlChar *str);


/********************************************************************
* FUNCTION ncx_get_name_match_string
* 
* Get the string for the ncx_name_match_t enum
* 
* INPUTS:
*   match == enum value 
*
* RETURNS:
*   string value
*********************************************************************/
extern const xmlChar *
    ncx_get_name_match_string (ncx_name_match_t match);


/********************************************************************
* FUNCTION ncx_write_tracefile
* 
* Write a byte to the tracefile
* 
* INPUTS:
*   buff == buffer to write
*   count == number of chars to write
*********************************************************************/
extern void
    ncx_write_tracefile (const char *buff, uint32 count);


/********************************************************************
* FUNCTION ncx_set_top_mandatory_allowed
* 
* Allow or disallow modules with top-level mandatory object
* to be loaded; used by the server when agt_running_error is TRUE
*
* INPUTS:
*   allowed == value to set T: to allow; F: to disallow
*********************************************************************/
extern void
    ncx_set_top_mandatory_allowed (boolean allowed);


/********************************************************************
* FUNCTION ncx_get_top_mandatory_allowed
* 
* Check if top-level mandatory objects are allowed or not
*
* RETURNS:
*   T: allowed; F: disallowed
*********************************************************************/
extern boolean
    ncx_get_top_mandatory_allowed (void);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx */
