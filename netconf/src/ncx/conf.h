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
#ifndef _H_conf
#define _H_conf

/*  FILE: conf.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Text Config file parser

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22-oct-07    abb      Begun

*/

#include <xmlstring.h>

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
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION conf_parse_from_filespec
* 
* Parse a file as an NCX text config file against
* a specific parmset definition.  Fill in an
* initialized parmset (and could be partially filled in)
*
* Error messages are printed by this function!!
*
* If a value is already set, and only one value is allowed
* then the 'keepvals' parameter will control whether that
* value will be kept or overwitten
*
* INPUTS:
*   filespec == absolute path or relative path
*               This string is used as-is without adjustment.
*   val       == value struct to fill in, must be initialized
*                already with val_new_value or val_init_value
*   keepvals == TRUE if old values should always be kept
*               FALSE if old vals should be overwritten
*   fileerr == TRUE to generate a missing file error
*              FALSE to return NO_ERR instead, if file not found
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    conf_parse_val_from_filespec (const xmlChar *filespec,
				  val_value_t *val,
				  boolean keepvals,
				  boolean fileerr);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_conf */
