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
#ifndef _H_def_reg
#define _H_def_reg

/*  FILE: def_reg.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

   Definition Registry module

   Provides fast tiered lookup for data structures used to
   process NCX messages.

   The data structures 'pointed to' by these registry entries
   are not managed in this module.  Deleting a registry entry
   will not delete the 'pointed to' data structure.

   Entry types

   NS: 
     Namespace to Module Lookup
     Key: namespace URI
     Data: module name and back pointer

   FD:
     File Desscriptor ID to Session Control Block Ptr
     Key: File Descriptor Index
     Data: Session Ptr attached to that FD

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-oct-05    abb      Begun
11-nov-05    abb      re-design to add OWNER as top-level instead of APP
11-feb-06    abb      remove application layer; too complicated
12-jul-07    abb      changed owner-based definitions to
                      module-based definitions throughout all code
                      Change OWNER to MODULE
19-feb-09    abb      Give up on module/mod_def/cfg_def because
                      too complicated once import-by-revision added
                      Too much memory usage as well.
*/

#include <xmlstring.h>

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
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
* FUNCTION def_reg_init
* 
* Initialize the def_reg module
*
* RETURNS:
*    none
*********************************************************************/
extern void 
    def_reg_init (void);


/********************************************************************
* FUNCTION def_reg_cleanup
* 
* Cleanup all the malloced memory in this module
* and return the module to an uninitialized state
* After this fn, the def_reg_init fn could be called again
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void 
    def_reg_cleanup (void);


/*********************** NS ***************************/


/********************************************************************
* FUNCTION def_reg_add_ns
*   
* add one xmlns_t to the registry
*
* INPUTS:
*    ns == namespace record to add
* RETURNS:
*    status of the operation
*********************************************************************/
extern status_t 
    def_reg_add_ns (xmlns_t  *ns);


/********************************************************************
* FUNCTION def_reg_find_ns
*   
* find one xmlns_t in the registry
* find a xmlns_t by its value (name)
*
* INPUTS:
*    nsname == namespace ID to find
* RETURNS:
*    pointer to xmlns_t or NULL if not found
*********************************************************************/
extern xmlns_t * 
    def_reg_find_ns (const xmlChar *nsname);


/********************************************************************
* FUNCTION def_reg_del_ns
*   
* unregister a xmlns_t
* delete one ncx_module from the registry
*
* INPUTS:
*    nsname == namespace name to delete
* RETURNS:
*    none
*********************************************************************/
extern void
    def_reg_del_ns (const xmlChar *nsname);


/*********************** SCB ***************************/


/********************************************************************
* FUNCTION def_reg_add_scb
*
* add one FD to SCB mapping to the registry
*
* INPUTS:
*    fd == file descriptor to add
*    session == ses_cb_t for the session
* RETURNS:
*    status of the operation
*********************************************************************/
extern status_t 
    def_reg_add_scb (int fd,
		     ses_cb_t *scb);


/********************************************************************
* FUNCTION def_reg_find_scb
*   
* find one FD-to-SCB mapping in the registry
*
* INPUTS:
*    fd == file descriptor ID to find
* RETURNS:
*    pointer to ses_cb_t or NULL if not found
*********************************************************************/
extern ses_cb_t * 
    def_reg_find_scb (int fd);


/********************************************************************
* FUNCTION def_reg_del_scb
*   
* delete one FD to SCB mapping from the registry
*
* INPUTS:
*    fd == file descriptor index to delete
* RETURNS:
*    none
*********************************************************************/
extern void
    def_reg_del_scb (int fd);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_def_reg */
