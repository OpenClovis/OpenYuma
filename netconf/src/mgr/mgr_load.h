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
#ifndef _H_mgr_load
#define _H_mgr_load
/*  FILE: mgr_load.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   Load Data File into an object template

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
27-jun-11    abb      Begun;
*/

#include <xmlstring.h>

#ifndef _H_obj
#include "obj.h"
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
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION mgr_load_extern_file
*
* Read an external file and convert it into a real data structure
*
* INPUTS:
*   filespec == XML filespec to load
*   targetobj == template to use to validate XML and encode as this object
*                !! must not be ncx:root!!
*             == NULL to use OBJ_TYP_ANYXML as the target object template
*   res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*     malloced val_value_t struct filled in with the contents
*********************************************************************/
extern val_value_t *
    mgr_load_extern_file (const xmlChar *filespec,
                          obj_template_t *targetobj,
                          status_t *res);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_mgr_load */
