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
#ifndef _H_xsd_typ
#define _H_xsd_typ

/*  FILE: xsd_typ.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Convert NCX Types to XSD format
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-nov-06    abb      Begun; split from xsd.c

*/

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_typ
#include "typ.h"
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
* FUNCTION xsd_add_types
* 
*   Add the required type nodes
*
* INPUTS:
*    mod == module in progress
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->childQ has entries added for the types in this module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_types (const ncx_module_t *mod,
		   val_value_t *val);


/********************************************************************
* FUNCTION xsd_add_local_types
* 
*   Add the required type nodes for the local types in the typnameQ
*   YANG Only -- NCX does not have local types
*
* INPUTS:
*    mod == module in progress
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->childQ has entries added for the types in this module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_local_types (const ncx_module_t *mod,
			 val_value_t *val);


/********************************************************************
* FUNCTION xsd_finish_simpleType
* 
*   Generate a simpleType elment based on the typdef
*   Note: NCX_BT_ENAME is considered a
*         complexType for XSD conversion purposes
*
* INPUTS:
*    mod == module in progress
*    typdef == typ def for the typ_template struct to convert
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->v.childQ has entries added for the types in this module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_finish_simpleType (const ncx_module_t *mod,
			   typ_def_t *typdef,
			   val_value_t *val);


/********************************************************************
* FUNCTION xsd_finish_namedType
* 
*   Generate a complexType elment based on the typdef for a named type
*   The tclass must be NCX_CL_NAMED for the typdef data struct
*
*   Note: The "+=" type extension mechanism in NCX is not supportable
*         in XSD for strings, so it must not be used at this time.
*         It is not supported for any data type, even enum.
*
* INPUTS:
*    mod == module in progress
*    typdef == typ def for the typ_template struct to convert
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->v.childQ has entries added for the types in this module
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_finish_namedType (const ncx_module_t *mod,
			  typ_def_t *typdef,
			  val_value_t *val);


/********************************************************************
* FUNCTION xsd_finish_union
* 
*   Generate the XSD for a union element 
*
* INPUTS:
*    mod == module in progress
*    typdef == typdef for the list
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->v.childQ has entries added for the nodes in this typdef
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_finish_union (const ncx_module_t *mod,
		      typ_def_t *typdef,
		      val_value_t *val);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xsd_typ */
