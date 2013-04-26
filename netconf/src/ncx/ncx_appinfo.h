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
#ifndef _H_ncx_appinfo
#define _H_ncx_appinfo

/*  FILE: ncx_appinfo.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  NCX Module Library Extension (Application Info) Utility Functions

 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
17-feb-10    abb      Begun; split out from ncx.c
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_appinfo
#include "ncx_appinfo.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_yang
#include "yang.h"
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
* FUNCTION ncx_new_appinfo
* 
* Create an appinfo entry
*
* INPOUTS:
*   isclone == TRUE if this is for a cloned object
*
* RETURNS:
*    malloced appinfo entry or NULL if malloc error
*********************************************************************/
extern ncx_appinfo_t * 
    ncx_new_appinfo (boolean isclone);


/********************************************************************
* FUNCTION ncx_free_appinfo
* 
* Free an appinfo entry
*
* INPUTS:
*    appinfo == ncx_appinfo_t data structure to free
*********************************************************************/
extern void 
    ncx_free_appinfo (ncx_appinfo_t *appinfo);


/********************************************************************
* FUNCTION ncx_find_appinfo
* 
* Find an appinfo entry by name (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    appinfoQ == pointer to Q of ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
extern ncx_appinfo_t *
    ncx_find_appinfo (dlq_hdr_t *appinfoQ,
		      const xmlChar *prefix,
		      const xmlChar *varname);


/********************************************************************
* FUNCTION ncx_find_const_appinfo
* 
* Find an appinfo entry by name (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    appinfoQ == pointer to Q of ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
extern const ncx_appinfo_t *
    ncx_find_const_appinfo (const dlq_hdr_t *appinfoQ,
                            const xmlChar *prefix,
                            const xmlChar *varname);


/********************************************************************
* FUNCTION ncx_find_next_appinfo
* 
* Find the next instance of an appinfo entry by name
* (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    current == pointer to current ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
extern const ncx_appinfo_t *
    ncx_find_next_appinfo (const ncx_appinfo_t *current,
			   const xmlChar *prefix,
			   const xmlChar *varname);


/********************************************************************
* FUNCTION ncx_find_next_appinfo2
* 
* Find the next instance of an appinfo entry by name
* (First match is returned)
* The entry returned is not removed from the Q
*
* INPUTS:
*    current == pointer to current ncx_appinfo_t data structure to check
*    prefix == module prefix that defines the extension 
*            == NULL to pick the first match (not expecting
*               appinfo name collisions)
*    varname == name string of the appinfo variable to find
*
* RETURNS:
*    pointer to the ncx_appinfo_t struct for the entry if found
*    NULL if the entry is not found
*********************************************************************/
extern ncx_appinfo_t *
    ncx_find_next_appinfo2 (ncx_appinfo_t *current,
                            const xmlChar *prefix,
                            const xmlChar *varname);


/********************************************************************
* FUNCTION ncx_clone_appinfo
* 
* Clone an appinfo value
*
* INPUTS:
*    appinfo ==  ncx_appinfo_t data structure to clone
*
* RETURNS:
*    pointer to the malloced ncx_appinfo_t struct clone of appinfo
*    NULL if a malloc error
*********************************************************************/
extern ncx_appinfo_t *
    ncx_clone_appinfo (ncx_appinfo_t *appinfo);


/********************************************************************
* FUNCTION ncx_clean_appinfoQ
* 
* Check an initialized appinfoQ for any entries
* Remove them from the queue and delete them
*
* INPUTS:
*    appinfoQ == Q of ncx_appinfo_t data structures to free
*********************************************************************/
extern void 
    ncx_clean_appinfoQ (dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION ncx_consume_appinfo
* 
* Check if an appinfo clause is present
*
* Save in appinfoQ if non-NULL
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress (NULL if none)
*   appinfoQ  == queue to use for any found entries (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_consume_appinfo (tk_chain_t *tkc,
			 ncx_module_t  *mod,
			 dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION ncx_consume_appinfo2
* 
* Check if an appinfo clause is present
* Do not backup the current token
* The TK_TT_MSTRING token has not been seen yet
* Called from yang_consume_semiapp
*
* Save in appinfoQ if non-NULL
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod    == ncx_module_t in progress (NULL if none)
*   appinfoQ  == queue to use for any found entries (may be NULL)
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_consume_appinfo2 (tk_chain_t *tkc,
			  ncx_module_t  *mod,
			  dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION ncx_resolve_appinfoQ
* 
* Validate all the appinfo clauses present in the specified Q
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc == token chain
*   mod == ncx_module_t in progress
*   appinfoQ == queue to check
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    ncx_resolve_appinfoQ (yang_pcb_t *pcb,
                          tk_chain_t *tkc,
			  ncx_module_t  *mod,
			  dlq_hdr_t *appinfoQ);



/********************************************************************
* FUNCTION ncx_get_appinfo_value
* 
* Get the value string from an appinfo struct
*
* INPUTS:
*    appinfo ==  ncx_appinfo_t data structure to use
*
* RETURNS:
*    pointer to the string value if name
*    NULL if no value
*********************************************************************/
extern const xmlChar *
    ncx_get_appinfo_value (const ncx_appinfo_t *appinfo);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx_appinfo */

