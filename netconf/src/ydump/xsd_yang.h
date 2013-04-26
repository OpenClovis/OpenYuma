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
#ifndef _H_xsd_yang
#define _H_xsd_yang

/*  FILE: xsd_yang.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Convert YANG-specific constructs to XSD format
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
04-feb-08    abb      Begun; split from xsd_typ.h

*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
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


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xsd_add_groupings
* 
*   Add the required group nodes
*
* INPUTS:
*    mod == module in progress
*    val == struct parent to contain child nodes for each group
*
* OUTPUTS:
*    val->childQ has entries added for the groupings required
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_groupings (ncx_module_t *mod,
		       val_value_t *val);


/********************************************************************
* FUNCTION xsd_add_objects
* 
*   Add the required element nodes for each object in the module
*   RPC methods and notifications are mixed in with the objects
*
* INPUTS:
*    mod == module in progress
*    val == struct parent to contain child nodes for each object
*
* OUTPUTS:
*    val->childQ has entries added for the RPC methods in this module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_objects (ncx_module_t *mod,
		     val_value_t *val);


/********************************************************************
* FUNCTION xsd_do_typedefs_groupingQ
* 
* Analyze the entire 'groupingQ' within the module struct
* Generate local typedef mappings as needed
*
* INPUTS:
*    mod == module conversion in progress
*    groupingQ == Q of grp_template_t structs to check
*    typnameQ == Q of xsd_typname_t to hold new local type names
*
* OUTPUTS:
*    typnameQ has entries added for each type
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_do_typedefs_groupingQ (ncx_module_t *mod,
			       dlq_hdr_t *groupingQ,
			       dlq_hdr_t *typnameQ);


/********************************************************************
* FUNCTION xsd_do_typedefs_datadefQ
* 
* Analyze the entire 'datadefQ' within the module struct
* Generate local typedef mappings as needed
*
* INPUTS:
*    mod == module conversion in progress
*    datadefQ == Q of obj_template_t structs to check
*    typnameQ == Q of xsd_typname_t to hold new local type names
*
* OUTPUTS:
*    typnameQ has entries added for each type
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_do_typedefs_datadefQ (ncx_module_t *mod,
			      dlq_hdr_t *datadefQ,
			      dlq_hdr_t *typnameQ);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xsd_yang */
