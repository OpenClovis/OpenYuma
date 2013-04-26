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
#ifndef _H_xsd
#define _H_xsd

/*  FILE: xsd.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  Convert NCX module to XSD format
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
15-nov-06    abb      Begun

*/

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
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
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION xsd_convert_module
* 
*   Convert a [sub]module to XSD 1.0 format
*   module entry point: convert an entire module to a value struct
*   representing an XML element which represents an XSD
*
* INPUTS:
*   mod == module to convert
*   cp == conversion parms struct to use
*   retval == address of val_value_t to receive a malloced value struct
*             representing the XSD.  Extra mallocs will be avoided by
*             referencing the mod_module_t strings.  
*             Do not remove the module from the registry while this
*             val struct is being used!!!
*   top_attrs == pointer to receive the top element attributes
*               HACK: rpc_agt needs the top-level attrs to
*               be in xml_attr_t format, not val_value_t metaval format
*
* OUTPUTS:
*   *retval = malloced return value; MUST FREE THIS LATER
*   *top_attrs = filled in attrs queue
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    xsd_convert_module (yang_pcb_t *pcb,
                        ncx_module_t *mod,
                        yangdump_cvtparms_t *cp,
                        val_value_t **retval,
                        xml_attrs_t  *top_attrs);


/********************************************************************
* FUNCTION xsd_load_typenameQ
* 
*   Generate unique type names for the entire module
*   including any submodules that were parsed as well
*
* INPUTS:
*   mod == module to process
*
* OUTPUTS:
*   *val = malloced return value
*   *retattrs = malled attrs queue
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_load_typenameQ (ncx_module_t *mod);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_xsd */
