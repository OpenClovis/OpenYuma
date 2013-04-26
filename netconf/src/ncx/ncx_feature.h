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
#ifndef _H_ncx_feature
#define _H_ncx_feature

/*  FILE: ncx_feature.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Module Library YANG Feature Utility Functions

 
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
* FUNCTION ncx_feature_init
* 
* Init the ncx_feature module
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void
    ncx_feature_init (void);


/********************************************************************
* FUNCTION ncx_feature_cleanup
* 
* Cleanup the ncx_feature module
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void
    ncx_feature_cleanup (void);


/********************************************************************
* FUNCTION ncx_new_iffeature
* 
* Get a new ncx_iffeature_t struct
*
* INPUTS:
*    none
* RETURNS:
*    pointer to a malloced ncx_iffeature_t struct,
*    or NULL if malloc error
*********************************************************************/
extern ncx_iffeature_t * 
    ncx_new_iffeature (void);


/********************************************************************
* FUNCTION ncx_free_iffeature
* 
* Free a malloced ncx_iffeature_t struct
*
* INPUTS:
*    iff == struct to free
*
*********************************************************************/
extern void 
    ncx_free_iffeature (ncx_iffeature_t *iffeature);


/********************************************************************
* FUNCTION ncx_clone_iffeature
* 
* Clone a new ncx_iffeature_t struct
*
* INPUTS:
*    srciff == ifffeature struct to clone
* RETURNS:
*    pointer to a malloced ncx_iffeature_t struct,
*    or NULL if malloc error
*********************************************************************/
extern ncx_iffeature_t *
    ncx_clone_iffeature (ncx_iffeature_t *srciff);


/********************************************************************
* FUNCTION ncx_clean_iffeatureQ
* 
* Clean a Q of malloced ncx_iffeature_t struct
*
* INPUTS:
*    iffeatureQ == address of Q to clean
*
*********************************************************************/
extern void 
    ncx_clean_iffeatureQ (dlq_hdr_t *iffeatureQ);


/********************************************************************
* FUNCTION ncx_find_iffeature
* 
* Search a Q of ncx_iffeature_t structs for a match
*
* INPUTS:
*    iffeatureQ == address of Q to search
*    prefix == prefix to check for
*              a NULL value indicates the current module
*    name == feature name string to find
*********************************************************************/
extern ncx_iffeature_t *
    ncx_find_iffeature (dlq_hdr_t *iffeatureQ,
			const xmlChar *prefix,
			const xmlChar *name,
			const xmlChar *modprefix);


/********************************************************************
* FUNCTION ncx_new_feature
* 
* Get a new ncx_feature_t struct
*
* INPUTS:
*    none
* RETURNS:
*    pointer to a malloced ncx_feature_t struct,
*    or NULL if malloc error
*********************************************************************/
extern ncx_feature_t * 
    ncx_new_feature (void);


/********************************************************************
* FUNCTION ncx_free_feature
* 
* Free a malloced ncx_feature_t struct
*
* INPUTS:
*    feature == struct to free
*
*********************************************************************/
extern void 
    ncx_free_feature (ncx_feature_t *feature);


/********************************************************************
* FUNCTION ncx_find_feature
* 
* Find a ncx_feature_t struct in the module and perhaps
* any of its submodules
*
* INPUTS:
*    mod == module to search
*    name == feature name to find
*
* RETURNS:
*    pointer to found feature or NULL if not found
*********************************************************************/
extern ncx_feature_t *
    ncx_find_feature (ncx_module_t *mod,
		      const xmlChar *name);


/********************************************************************
* FUNCTION ncx_find_feature_que
* 
* Find a ncx_feature_t struct in the specified Q
*
* INPUTS:
*    featureQ == Q of ncx_feature_t to search
*    name == feature name to find
*
* RETURNS:
*    pointer to found feature or NULL if not found
*********************************************************************/
extern ncx_feature_t *
    ncx_find_feature_que (dlq_hdr_t *featureQ,
			  const xmlChar *name);


/********************************************************************
* FUNCTION ncx_find_feature_all
* 
* Find a ncx_feature_t struct in the module and perhaps
* any of its submodules
*
* INPUTS:
*    mod == module to search
*    name == feature name to find
*
* RETURNS:
*    pointer to found feature or NULL if not found
*********************************************************************/
extern ncx_feature_t *
    ncx_find_feature_all (ncx_module_t *mod,
                          const xmlChar *name);


/********************************************************************
* FUNCTION ncx_for_all_features
* 
* Execute a callback function for all features in this module
* and any submodules
*
* INPUTS:
*    mod == module to search for features
*    cbfn == feature callback function
*    cookie == cookie value to pass to each iteration of the callback
*    enabledonly == TRUE if only callbacks for enabled features
*                   FALSE if all features should invoke callbacks
*********************************************************************/
extern void
    ncx_for_all_features (const ncx_module_t *mod,
			  ncx_feature_cbfn_t  cbfn,
			  void *cookie,
			  boolean enabledonly);


/********************************************************************
* FUNCTION ncx_feature_count
* 
* Get the total feature count for this module
* and any submodules
*
* INPUTS:
*    mod == module to search for features
*    enabledonly == TRUE to only count enabled features
*                   FALSE to count all features
*
* RETURNS:
*   total number of features
*********************************************************************/
extern uint32
    ncx_feature_count (const ncx_module_t *mod,
		       boolean enabledonly);


/********************************************************************
* FUNCTION ncx_feature_enabled
* 
* Check if the specified feature and any referenced
* if-features are enabled
*
* INPUTS:
*    feature == feature to check
*
* RETURNS:
*   TRUE if feature is completely enabled
*   FALSE if feature is not enabled, or partially enabled
*********************************************************************/
extern boolean
    ncx_feature_enabled (const ncx_feature_t *feature);


/********************************************************************
* FUNCTION ncx_feature_enabled_str
* 
* Find a ncx_feature_t struct and check if it is enabled
*
* INPUTS:
*    modname == name of module to search
*    revision == module revision string (may be NULL)
*    name == feature name to find
*
* RETURNS:
*    TRUE if feature is enabled
*********************************************************************/
extern boolean
    ncx_feature_enabled_str (const xmlChar *modname,
                             const xmlChar *revision,
                             const xmlChar *name);


/********************************************************************
* FUNCTION ncx_set_feature_enable_default
* 
* Set the feature_enable_default flag
*
* INPUTS:
*   flag == feature enabled flag value
*********************************************************************/
extern void
    ncx_set_feature_enable_default (boolean flag);


/********************************************************************
* FUNCTION ncx_set_feature_code_default
* 
* Set the feature_code_default enumeration
*
* !!! THIS FUNCTION IS DEPRECATED!!!
* !!! The --feature-code and --feature-code-default parameters are ignored
* !!! Feature code generation is not controlled by this parameter 
*
* INPUTS:
*   code == feature code value
*********************************************************************/
extern void
    ncx_set_feature_code_default (ncx_feature_code_t code);


/********************************************************************
* FUNCTION ncx_set_feature_code_entry
* 
* Create or set a feature_entry struct for the specified 
* feature code parameter
*
* !!! THIS FUNCTION IS DEPRECATED!!!
* !!! The --feature-code and --feature-code-default parameters are ignored
* !!! Feature code generation is not controlled by this parameter 
*
* INPUTS:
*   featstr == feature parameter string
*   featcode == ncx_feature_code_t enumeration to set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_set_feature_code_entry (const xmlChar *featstr,
                                ncx_feature_code_t featcode);


/********************************************************************
* FUNCTION ncx_set_feature_enable_entry
* 
* Create or set a feature_entry struct for the specified 
* feature enabled parameter
*
* Called from CLI/conf handler code
*
* INPUTS:
*   featstr == feature parameter string
*   flag == enabled flag
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_set_feature_enable_entry (const xmlChar *featstr,
                                  boolean flag);


/********************************************************************
* FUNCTION ncx_set_feature_enable
* 
* Create or set a feature_entry struct for the specified 
* feature enabled parameter
*
* Called from SIL init code
*
* INPUTS:
*   modname == name of module defining the feature
*   name == feature name
*   flag == feature enabled flag
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    ncx_set_feature_enable (const xmlChar *modname,
                            const xmlChar *name,
                            boolean flag);


/********************************************************************
* FUNCTION ncx_set_feature_parms
* 
* Check if any feature parameters were set for the specified 
* feature struct
*
* INPUTS:
*   feature == feature struct to check
*
* OUTPUTS:
*   feature->code and/or feature->enabled may be set
*********************************************************************/
extern void
    ncx_set_feature_parms (ncx_feature_t *feature);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncx_feature */

