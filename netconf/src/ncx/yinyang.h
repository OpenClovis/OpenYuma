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
#ifndef _H_yinyang
#define _H_yinyang

/*  FILE: yinyang.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Convert YIN format to YANG format for input

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
12-dec-09    abb      Begun;

*/

#include <xmlstring.h>

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
* FUNCTION yinyang_convert_token_chain
* 
* Try to open a file as a YIN formatted XML file
* and convert it to a token chain for processing
* by yang_parse_from_token_chain
*
* INPUTS:
*   sourcespec == file spec for the YIN file to read
*   res == address of return status
*
* OUTPUTS:
*   *res == status of the operation
*
* RETURNS:
*   malloced token chain containing the converted YIN file
*********************************************************************/
extern tk_chain_t *
    yinyang_convert_token_chain (const xmlChar *sourcespec,
                                 status_t *res);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yinyang */
