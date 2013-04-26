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
#ifndef _H_help
#define _H_help

/*  FILE: help.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Print help text for various templates

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
05-oct-07    abb      Begun

*/

#include <xmlstring.h>

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

#define HELP_MODE_BRIEF_MAX            60
#define HELP_MODE_NORMAL_MAX           100


/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/

typedef enum help_mode_t_ {
    HELP_MODE_NONE,
    HELP_MODE_BRIEF,
    HELP_MODE_NORMAL,
    HELP_MODE_FULL
} help_mode_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION help_program_module
*
* Print the full help text for an entire program module to STDOUT
*
* INPUTS:
*    modname == module name without file suffix
*    cliname == name of CLI parmset within the modname module
*
*********************************************************************/
extern void
    help_program_module (const xmlChar *modname,
			 const xmlChar *cliname,
			 help_mode_t mode);


/********************************************************************
* FUNCTION help_data_module
*
* Print the full help text for an entire data module to STDOUT
*
* INPUTS:
*    mod     == data module struct
*    mode == help mode requested
*********************************************************************/
extern void
    help_data_module (const ncx_module_t *mod,
		      help_mode_t mode);


/********************************************************************
* FUNCTION help_type
*
* Print the full help text for a data type to STDOUT
*
* INPUTS:
*    typ     == type template struct
*    mode == help mode requested
*********************************************************************/
extern void
    help_type (const typ_template_t *typ,
	       help_mode_t mode);


/********************************************************************
* FUNCTION help_object
*
* Print the full help text for a RPC method template to STDOUT
*
* INPUTS:
*    obj     == object template
*    mode == help mode requested
*********************************************************************/
extern void
    help_object (obj_template_t *obj,
		 help_mode_t mode);


/********************************************************************
 * FUNCTION help_write_lines
 * 
 * write some indented output to STDOUT
 *
 * INPUTS:
 *    str == string to print; 'indent' number of spaces
 *           will be added to each new line
 *    indent == indent count
 *    startnl == TRUE if start with a newline, FALSE otherwise
 *********************************************************************/
extern void
    help_write_lines (const xmlChar *str,
		      uint32 indent,
		      boolean startnl);


/********************************************************************
 * FUNCTION help_write_lines_max
 * 
 * write some indented output to STDOUT
 *
 * INPUTS:
 *    str == string to print; 'indent' number of spaces
 *           will be added to each new line
 *    indent == indent count
 *    startnl == TRUE if start with a newline, FALSE otherwise
 *    maxlen == 0..N max number of chars to output
 *********************************************************************/
extern void
    help_write_lines_max (const xmlChar *str,
			  uint32 indent,
			  boolean startnl,
			  uint32 maxlen);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_help */
