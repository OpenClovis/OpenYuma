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
#ifndef _H_c
#define _H_c

/*  FILE: c.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Convert YANG module to C file format 
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
27-oct-09    abb      Begun

*/

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
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
* FUNCTION c_convert_module
* 
* Generate the SIL C code for the specified module(s)
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    c_convert_module (yang_pcb_t *pcb,
		      const yangdump_cvtparms_t *cp,
		      ses_cb_t *scb);


/********************************************************************
* FUNCTION c_write_fn_prototypes
* 
* Generate the SIL H code for the external function definitions
* in the specified module, already parsed and H file generation
* is in progress
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*   objnameQ == Q of c_define_t mapping structs to use
*
*********************************************************************/
extern void
    c_write_fn_prototypes (ncx_module_t *mod,
                           const yangdump_cvtparms_t *cp,
                           ses_cb_t *scb,
                           dlq_hdr_t *objnameQ);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_c */
