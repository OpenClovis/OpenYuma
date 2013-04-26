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
#ifndef _H_ydump
#define _H_ydump

/*  FILE: ydump.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  Exports yangdump.yang conversion CLI parameter struct
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
01-mar-08    abb      Begun; moved from ncx/ncxtypes.h

*/

#include <xmlstring.h>

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/********************************************************************
 * FUNCTION ydump_init
 * 
 * Parse all CLI parameters and fill in the conversion parameters
 * 
 * INPUTS:
 *   argc == number of argv parameters
 *   argv == array of CLI parameters
 *   allowcode == TRUE if code generation should be allowed
 *                FALSE otherwise
 *   cvtparms == address of conversion parms to fill in
 *
 * OUTPUTS:
 *  *cvtparms filled in, maybe partial if errors
 *
 * RETURNS:
 *    status
 *********************************************************************/
extern status_t 
    ydump_init (int argc,
                char *argv[],
                boolean allowcode,
                yangdump_cvtparms_t *cvtparms);


/********************************************************************
* FUNCTION ydump_main
*
* Run the yangdump conversion, according to the specified
* yangdump conversion parameters set
*
* INPUTS:
*   cvtparms == conversion parameters to use
*
* OUTPUTS:
*   the conversion (if any) will be done and output to
*   the specified files, or STDOUT
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ydump_main (yangdump_cvtparms_t *cvtparms);


/********************************************************************
 * FUNCTION ydump_cleanup
 * 
 * INPUTS:
 *  cvtparms == conversion parameters to clean but not free
 * 
 *********************************************************************/
extern void
    ydump_cleanup (yangdump_cvtparms_t *cvtparms);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_ydump */
