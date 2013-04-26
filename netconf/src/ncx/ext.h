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
#ifndef _H_ext
#define _H_ext

/*  FILE: ext.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Extension Handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
05-jan-08    abb      Begun

*/

#include <xmlstring.h>
#include <xmlregexp.h>

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
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

/* One YANG 'extension' definition -- language extension template */
typedef struct ext_template_t_ {
    dlq_hdr_t        qhdr;
    xmlChar         *name;
    xmlChar         *descr;
    xmlChar         *ref;
    xmlChar         *arg;
    xmlns_id_t       nsid;
    ncx_status_t     status;
    boolean          argel;
    boolean          used;                  /* needed by yangdiff */
    dlq_hdr_t        appinfoQ;              /* Q of ncx_appinfo_t */
    ncx_error_t      tkerr;
} ext_template_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION ext_new_template
* 
* Malloc and initialize the fields in a ext_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern ext_template_t *
    ext_new_template (void);


/********************************************************************
* FUNCTION ext_free_template
* 
* Scrub the memory in a ext_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    ext == ext_template_t data structure to free
*********************************************************************/
extern void 
    ext_free_template (ext_template_t *ext);


/********************************************************************
* FUNCTION ext_clean_extensionQ
* 
* Clean a queue of ext_template_t structs
*
* INPUTS:
*    que == Q of ext_template_t data structures to free
*********************************************************************/
extern void
    ext_clean_extensionQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION ext_find_extension
* 
* Search a queue of ext_template_t structs for a given name
*
* INPUTS:
*    que == Q of ext_template_t data structures to search
*    name == name string to find
*
* RETURNS:
*   pointer to found entry, or NULL if not found
*********************************************************************/
extern ext_template_t *
    ext_find_extension (ncx_module_t *mod,
                        const xmlChar *name);


/********************************************************************
* FUNCTION ext_find_extension_que
* 
* Find an ext_template_t struct in the specified Q
*
* INPUTS:
*    extensionQ == Q of ext_template_t to search
*    name == extension name to find
*
* RETURNS:
*    pointer to found extension or NULL if not found
*********************************************************************/
extern ext_template_t *
    ext_find_extension_que (dlq_hdr_t *extensionQ,
                            const xmlChar *name);


/********************************************************************
* FUNCTION ext_find_extension_all
* 
* Search a module of ext_template_t structs for a given name
* Check all submodules as well
*
* INPUTS:
*    mod == module to check
*    name == name string to find
*
* RETURNS:
*   pointer to found entry, or NULL if not found
*********************************************************************/
extern ext_template_t *
    ext_find_extension_all (ncx_module_t *mod,
                            const xmlChar *name);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ext */
