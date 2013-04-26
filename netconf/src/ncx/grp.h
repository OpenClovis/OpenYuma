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
#ifndef _H_grp
#define _H_grp

/*  FILE: grp.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Grouping Handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
09-dec-07    abb      Begun

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


/* One YANG 'grouping' definition -- sibling set template */
typedef struct grp_template_t_ {
    dlq_hdr_t        qhdr;
    xmlChar         *name;
    xmlChar         *descr;
    xmlChar         *ref;
    ncx_error_t      tkerr; 
    void            *parent;      /* const back-ptr to parent obj */
    struct grp_template_t_ *parentgrp;    /* direct parent is grp */
    xmlns_id_t       nsid;
    boolean          used;
    boolean          istop;
    boolean          expand_done;
    ncx_status_t     status;
    uint32           grpindex;         /* used for XSD generation */
    dlq_hdr_t        typedefQ;             /* Q of typ_template_t */
    dlq_hdr_t        groupingQ;            /* Q of grp_template_t */
    dlq_hdr_t        datadefQ;             /* Q of obj_template_t */
    dlq_hdr_t        appinfoQ;              /* Q of ncx_appinfo_t */

} grp_template_t;



/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION grp_new_template
* 
* Malloc and initialize the fields in a grp_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern grp_template_t *
    grp_new_template (void);


/********************************************************************
* FUNCTION grp_free_template
* 
* Scrub the memory in a grp_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    grp == grp_template_t data structure to free
*********************************************************************/
extern void 
    grp_free_template (grp_template_t *grp);


/********************************************************************
* FUNCTION grp_clean_groupingQ
* 
* Clean a queue of grp_template_t structs
*
* INPUTS:
*    que == Q of grp_template_t data structures to free
*********************************************************************/
extern void
    grp_clean_groupingQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION grp_has_typedefs
* 
* Check if the grouping contains any typedefs
*
* INPUTS:
*    grp == grp_template_t struct to check
*
* RETURNS:
*    TRUE if any embedded typedefs
*    FALSE if no embedded typedefs
*********************************************************************/
extern boolean
    grp_has_typedefs (const grp_template_t *grp);


/********************************************************************
* FUNCTION grp_get_mod_name
* 
* Get the module name for a grouping
*
* INPUTS:
*    grp == grp_template_t struct to check
*
* RETURNS:
*    const pointer to module name
*********************************************************************/
extern const xmlChar *
    grp_get_mod_name (const grp_template_t *grp);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_grp */
