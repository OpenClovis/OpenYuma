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
#ifndef _H_yang_typ
#define _H_yang_typ

/*  FILE: yang_typ.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module parser typedef and type statement support

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
15-nov-07    abb      Begun; start from yang_parse.c

*/

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION yang_typ_consume_type
* 
* Parse the next N tokens as a type clause
* Add to the typ_template_t struct in progress
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'type' keyword
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod   == module in progress
*   intypdef == struct that will get the type info
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_consume_type (yang_pcb_t *pcb,
                           tk_chain_t *tkc,
			   ncx_module_t  *mod,
			   typ_def_t *typdef);


/********************************************************************
* FUNCTION yang_typ_consume_metadata_type
* 
* Parse the next token as a type declaration
* for an ncx:metadata definition
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod   == module in progress
*   intypdef == struct that will get the type info
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_consume_metadata_type (yang_pcb_t *pcb,
                                    tk_chain_t *tkc,
				    ncx_module_t  *mod,
				    typ_def_t *intypdef);


/********************************************************************
* FUNCTION yang_typ_consume_typedef
* 
* Parse the next N tokens as a typedef clause
* Create a typ_template_t struct and add it to the specified module
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'typedef' keyword
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain
*   mod == module in progress
*   que == queue will get the typ_template_t 
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_consume_typedef (yang_pcb_t *pcb,
                              tk_chain_t *tkc,
			      ncx_module_t  *mod,
			      dlq_hdr_t *que);


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs
* 
* Analyze the entire typeQ within the module struct
* Finish all the named types and range clauses,
* which were left unfinished due to possible forward references
*
* Check all the types in the Q
* If a type has validation errors, it will be removed
* from the typeQ
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_resolve_typedefs (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
			       ncx_module_t  *mod,
			       dlq_hdr_t *typeQ,
			       obj_template_t *parent);


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs_final
* 
* Analyze the entire typeQ within the module struct
* Finish all default value checking that was not done before
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_resolve_typedefs_final (tk_chain_t *tkc,
                                     ncx_module_t *mod,
                                     dlq_hdr_t *typeQ);


/********************************************************************
* FUNCTION yang_typ_resolve_typedefs_grp
* 
* Analyze the entire typeQ within the module struct
* Finish all the named types and range clauses,
* which were left unfinished due to possible forward references
*
* Check all the types in the Q
* If a type has validation errors, it will be removed
* from the typeQ
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typeQ == Q of typ_template_t structs t0o check
*   parent == obj_template containing this typeQ
*          == NULL if this is a module-level typeQ
*   grp == grp_template containing this typedef
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_resolve_typedefs_grp (yang_pcb_t *pcb,
                                   tk_chain_t *tkc,
				   ncx_module_t  *mod,
				   dlq_hdr_t *typeQ,
				   obj_template_t *parent,
				   grp_template_t *grp);


/********************************************************************
* FUNCTION yang_typ_resolve_type
* 
* Analyze the typdef within a single leaf or leaf-list statement
* Finish if a named type, and/or range clauses present,
* which were left unfinished due to possible forward references
*
* Algorithm for checking named types in 4 separate loops:
*   1) resolve all open type name references
*   2) check for any name loops in all named types
*   3) check that all base types and builtin types
*      in each type chain are correct.  Also check that
*      all restrictions given are correct for that type
*   4) Check all range clauses and resolve all min/max
*      keyword uses to decimal numbers and validate that
*      each range is well-formed.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typdef == typdef struct from leaf or leaf-list to check
*   defval == default value string for this leaf (may be NULL)
*   obj == obj_template containing this typdef
*       == NULL if this is a top-level union typedef,
*          checking its nested unnamed type clauses
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_resolve_type (yang_pcb_t *pcb,
                           tk_chain_t *tkc,
			   ncx_module_t  *mod,
			   typ_def_t *typdef,
                           const xmlChar *defval,
			   obj_template_t *obj);


/********************************************************************
* FUNCTION yang_typ_resolve_type_final
* 
* Check default values for XPath types
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   typdef == typdef struct from leaf or leaf-list to check
*   defval == default value string for this leaf (may be NULL)
*   obj == obj_template containing this typdef
*       == NULL if this is a top-level union typedef,
*          checking its nested unnamed type clauses
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_resolve_type_final (tk_chain_t *tkc,
                                 ncx_module_t  *mod,
                                 typ_def_t *typdef,
                                 const xmlChar *defval,
                                 obj_template_t *obj);


/********************************************************************
* FUNCTION yang_typ_rangenum_ok
* 
* Check a typdef for range definitions
* and check if the specified number passes all
* the range checks (if any)
*
* INPUTS:
*   typdef == typdef to check
*   num == number to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_typ_rangenum_ok (typ_def_t *typdef,
			  const ncx_num_t *num);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yang_typ */
