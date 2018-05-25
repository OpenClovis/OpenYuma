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
#ifndef _H_yang_parse
#define _H_yang_parse

/*  FILE: yang_parse.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module parser module


    Data type conversion

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
26-oct-07    abb      Begun; start from ncx_parse.h

*/

#include <xmlstring.h>

#ifndef _H_status
#include "status.h"
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
* FUNCTION yang_parse_from_filespec
* 
* Parse a file as a YANG module
*
* Error messages are printed by this function!!
*
* INPUTS:
*   filespec == absolute path or relative path
*               This string is used as-is without adjustment.
*   pcb == parser control block used as very top-level struct
*   ptyp == parser call type
*            YANG_PT_TOP == called from top-level file
*            YANG_PT_INCLUDE == called from an include-stmt in a file
*            YANG_PT_IMPORT == called from an import-stmt in a file
*   isyang == TRUE if a YANG file is expected
*             FALSE if a YIN file is expected
*
* OUTPUTS:
*   an ncx_module is filled out and validated as the file
*   is parsed.  If no errors:
*     TOP, IMPORT:
*        the module is loaded into the definition registry with 
*        the ncx_add_to_registry function
*     INCLUDE:
*        the submodule is loaded into the top-level module,
*        specified in the pcb
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_parse_from_filespec (const xmlChar *filespec,
                              yang_pcb_t *pcb,
			      yang_parsetype_t ptyp,
                              boolean isyang);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yang_parse */
