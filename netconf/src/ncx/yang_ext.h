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
#ifndef _H_yang_ext
#define _H_yang_ext

/*  FILE: yang_ext.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module parser extension statement support

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
05-jan-08    abb      Begun; start from yang_grp.h

*/

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
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
* FUNCTION yang_ext_consume_extension
* 
* Parse the next N tokens as an extension-stmt
* Create an ext_template_t struct and add it to the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* Current token is the 'extension' keyword
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_ext_consume_extension (tk_chain_t *tkc,
				ncx_module_t  *mod);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yang_ext */
