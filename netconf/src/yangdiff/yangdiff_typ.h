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
#ifndef _H_yangdiff_typ
#define _H_yangdiff_typ

/*  FILE: yangdiff_typ.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Report differences related to YANG typedefs and types
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
10-jul-08    abb      Split out from yangdiff.c

*/

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_yangdiff
#include "yangdiff.h"
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
* FUNCTION type_changed
*
* Check if the type clause and sub-clauses changed at all
*
* INPUTS:
*   cp == compare parameters to use
*   oldtypdef == old type def struct to check
*   newtypdef == new type def struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
extern uint32
    type_changed (yangdiff_diffparms_t *cp,
		  typ_def_t *oldtypdef,
		  typ_def_t *newtypdef);


/********************************************************************
 * FUNCTION typedef_changed
 * 
 *  Check if a (nested) typedef changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtyp == old typ_template_t to use
 *    newtyp == new typ_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
extern uint32
    typedef_changed (yangdiff_diffparms_t *cp,
		     typ_template_t *oldtyp,
		     typ_template_t *newtyp);


/********************************************************************
 * FUNCTION typedefQ_changed
 * 
 *  Check if a (nested) Q of typedefs changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old typ_template_t to use
 *    newQ == Q of new typ_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
extern uint32
    typedefQ_changed (yangdiff_diffparms_t *cp,
		      dlq_hdr_t *oldQ,
		      dlq_hdr_t *newQ);


/********************************************************************
 * FUNCTION output_typedefQ_diff
 * 
 *  Output the differences report for a Q of typedef definitions
 *  Not always called for top-level typedefs; Can be called
 *  for nested typedefs
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old typ_template_t to use
 *    newQ == Q of new typ_template_t to use
 *
 *********************************************************************/
extern void
    output_typedefQ_diff (yangdiff_diffparms_t *cp,
			  dlq_hdr_t *oldQ,
			  dlq_hdr_t *newQ);


/********************************************************************
 * FUNCTION output_one_type_diff
 * 
 *  Output the differences report for one type section
 *  within a leaf, leaf-list, or typedef definition
 *
 * type_changed should be called first to determine
 * if the type actually changed.  Otherwise a 'M typedef foo'
 * output line will result and be a false positive
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldtypdef == old internal typedef
 *    newtypdef == new internal typedef
 *
 *********************************************************************/
extern void
    output_one_type_diff (yangdiff_diffparms_t *cp,
			  typ_def_t *oldtypdef,
			  typ_def_t *newtypdef);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangdiff_typ */
