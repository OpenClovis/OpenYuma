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
#ifndef _H_yang_grp
#define _H_yang_grp

/*  FILE: yang_grp.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module parser grouping statement support

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-dec-07    abb      Begun; start from yang_typ.h

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

#ifndef _H_obj
#include "obj.h"
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
* FUNCTION yang_grp_consume_grouping
* 
* 2nd pass parsing
* Parse the next N tokens as a grouping-stmt
* Create a grp_template_t struct and add it to the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'grouping' keyword
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == module in progress
*   que == queue will get the grp_template_t 
*   parent == parent object or NULL if top-level grouping-stmt
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_grp_consume_grouping (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
			       ncx_module_t  *mod,
			       dlq_hdr_t *que,
			       obj_template_t *parent);


/********************************************************************
* FUNCTION yang_grp_resolve_groupings
* 
* 3rd pass parsing
* Analyze the entire 'groupingQ' within the module struct
* Finish all the clauses within this struct that
* may have been defered because of possible forward references
*
* Any uses or augment within the grouping is deferred
* until later passes because of forward references
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*   parent == obj_template containing this groupingQ
*          == NULL if this is a module-level groupingQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_grp_resolve_groupings (yang_pcb_t *pcb,
                                tk_chain_t *tkc,
				ncx_module_t  *mod,
				dlq_hdr_t *groupingQ,
				obj_template_t *parent);


/********************************************************************
* FUNCTION yang_grp_resolve_complete
* 
* 4th pass parsing
* Analyze the entire 'groupingQ' within the module struct
* Expand any uses and augment statements within the group and 
* validate as much as possible
*
* Completes processing for all the groupings
* sust that it is safe to expand any uses clauses
* within objects, via the yang_obj_resolve_final fn
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*   parent == parent object contraining groupingQ (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_grp_resolve_complete (yang_pcb_t *pcb,
                               tk_chain_t *tkc,
			       ncx_module_t  *mod,
			       dlq_hdr_t *groupingQ,
			       obj_template_t *parent);


/********************************************************************
* FUNCTION yang_grp_resolve_final
* 
* Analyze the entire 'groupingQ' within the module struct
* Check final warnings etc.
*
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   groupingQ == Q of grp_template_t structs to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_grp_resolve_final (yang_pcb_t *pcb,
                            tk_chain_t *tkc,
			    ncx_module_t  *mod,
			    dlq_hdr_t *groupingQ);


/********************************************************************
* FUNCTION yang_grp_check_nest_loop
* 
* Check the 'uses' object and determine if it is contained
* within the group being used.
*
*    grouping A {
*      uses A;
*    }
*
*    grouping B {
*      container C {
*        grouping BB {
*          uses B;
*        }
*      }
*    } 
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain from parsing (needed for error msgs)
*   mod == module in progress
*   obj == 'uses' obj_template containing the ref to 'grp'
*   grp == grp_template_t that this 'obj' is using
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_grp_check_nest_loop (tk_chain_t *tkc,
			      ncx_module_t  *mod,
			      obj_template_t *obj,
			      grp_template_t *grp);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yang_grp */
