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
#ifndef _H_agt_cli
#define _H_agt_cli

/*  FILE: agt_cli.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NETCONF Server Command Line Interface handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
27-oct-06    abb      Begun
01-aug-08    abb      Convert from NCX PSD to YANG OBJ

*/

#ifndef _H_agt
#include "agt.h"
#endif

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			C O N S T A N T S
*								    *
*********************************************************************/

#define AGT_CLI_MODULE     (const xmlChar *)"netconfd"
#define AGT_CLI_CONTAINER  (const xmlChar *)"netconfd"


#define AGT_CLI_NOSTARTUP (const xmlChar *)"no-startup"
#define AGT_CLI_STARTUP   (const xmlChar *)"startup"
#define AGT_CLI_FACTORY_STARTUP   (const xmlChar *)"factory-startup"
#define AGT_CLI_RUNNING_ERROR (const xmlChar *)"running-error"
#define AGT_CLI_STARTUP_ERROR (const xmlChar *)"startup-error"
#define AGT_CLI_STARTUP_STOP  (const xmlChar *)"stop"
#define AGT_CLI_DELETE_EMPTY_NPCONTAINERS \
    (const xmlChar *)"delete-empty-npcontainers"

#define AGT_CLI_SUPERUSER NCX_EL_SUPERUSER

#define AGT_CLI_MAX_BURST NCX_EL_MAX_BURST

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION agt_cli_process_input
*
* Process the param line parameters against the hardwired
* parmset for the netconfd program
*
* INPUTS:
*    argc == argument count
*    argv == array of command line argument strings
*    agt_profile == server profile struct to fill in
*    showver == address of version return quick-exit status
*    showhelpmode == address of help return quick-exit status
*
* OUTPUTS:
*    *agt_profile is filled in, with parms gathered or defaults
*    *showver == TRUE if user requsted version quick-exit mode
*    *showhelpmode == requested help mode 
*                     (none, breief, normal, full)
*
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
extern status_t
    agt_cli_process_input (int argc,
			   char *argv[],
			   agt_profile_t *agt_profile,
			   boolean *showver,
			   help_mode_t *showhelpmode);


/********************************************************************
* FUNCTION agt_cli_get_valset
*
*   Retrieve the command line parameter set from boot time
*
* RETURNS:
*    pointer to parmset or NULL if none
*********************************************************************/
extern val_value_t *
    agt_cli_get_valset (void);


/********************************************************************
* FUNCTION agt_cli_cleanup
*
*   Cleanup the module static data
*
*********************************************************************/
extern void
    agt_cli_cleanup (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_cli */
