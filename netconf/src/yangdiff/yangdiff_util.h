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
#ifndef _H_yangdiff_util
#define _H_yangdiff_util

/*  FILE: yangdiff_util.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Report differences utilities
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
10-jul-08    abb      Split out from yangdiff.c

*/

#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_yangdiff
#include "yangdiff.h"
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
* FUNCTION indent_in
*
* Move curindent to the right
*
* INPUTS:
*   cp == compare paraeters to use
*
*********************************************************************/
extern void
    indent_in (yangdiff_diffparms_t *cp);


/********************************************************************
* FUNCTION indent_out
*
* Move curindent to the left
*
* INPUTS:
*   cp == compare paraeters to use
*
*********************************************************************/
extern void
    indent_out (yangdiff_diffparms_t *cp);


/********************************************************************
* FUNCTION str_field_changed
*
* Check if the string field changed at all
*
* INPUTS:
*   fieldname == fieldname to use (may be NULL)
*   oldstr == old string to check (may be NULL)
*   newstr == new string to check (may be NULL)
*   isrev == TRUE if revision mode, FALSE otherwise
*   cdb == pointer to change description block to fill in (may be NULL)
*
* OUTPUTS:
*   *cdb is filled in, if cdb is non-NULL
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*   
*********************************************************************/
extern uint32
    str_field_changed (const xmlChar *fieldname,
		       const xmlChar *oldstr,
		       const xmlChar *newstr,
		       boolean isrev,
		       yangdiff_cdb_t *cdb);


/********************************************************************
* FUNCTION bool_field_changed
*
* Check if the boolean field changed at all
*
* INPUTS:
*   oldbool == old boolean to check
*   newbool == new boolean to check
*   isrev == TRUE if revision mode, FALSE otherwise
*   cdb == pointer to change description block to fill in
*
* OUTPUTS:
*   *cdb is filled in
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*   
*********************************************************************/
extern uint32
    bool_field_changed (const xmlChar *fieldname,
		       boolean oldbool,
		       boolean newbool,
		       boolean isrev,
			yangdiff_cdb_t *cdb);


/********************************************************************
* FUNCTION status_field_changed
*
* Check if the status field changed at all
*
* INPUTS:
*   oldstat == old status to check
*   newstat == new status to check
*   isrev == TRUE if revision mode, FALSE otherwise
*   cdb == pointer to change description block to fill in
*
* OUTPUTS:
*   *cdb is filled in
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
extern uint32
    status_field_changed (const xmlChar *fieldname,
			  ncx_status_t oldstat,
			  ncx_status_t newstat,
			  boolean isrev,
			  yangdiff_cdb_t *cdb);


/********************************************************************
* FUNCTION prefix_field_changed
*
* Check if the status field changed at all
*
* INPUTS:
*   oldmod == old module to check
*   newmod == new module to check
*   oldprefix == old prefix to check
*   newprefix == new prefix to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
extern uint32
    prefix_field_changed (const ncx_module_t *oldmod,
			  const ncx_module_t *newmod,
			  const xmlChar *oldprefix,
			  const xmlChar *newprefix);


/********************************************************************
 * FUNCTION output_val
 * 
 *  Output one value; will add truncate!
 *
 * INPUTS:
 *    cp == parameter block to use
 *    strval == str value to output
 *********************************************************************/
extern void
    output_val (yangdiff_diffparms_t *cp,
		const xmlChar *strval);


/********************************************************************
 * FUNCTION output_mstart_line
 * 
 *  Output one line for a comparison where the change is 'modify'
 *  Just print the field name and identifier
 *
 * INPUTS:
 *    cp == parameter block to use
 *    fieldname == fieldname that is different to print
 *    val == string value to print (may be NULL)
 *    isid == TRUE if val is an ID
 *         == FALSE if val should be single-quoted
 *         (ignored in val == NULL)
 *********************************************************************/
extern void
    output_mstart_line (yangdiff_diffparms_t *cp,
			const xmlChar *fieldname,
			const xmlChar *val,
			boolean isid);


/********************************************************************
 * FUNCTION output_cdb_line
 * 
 *  Output one line for a comparison, from a change description block
 *
 * INPUTS:
 *    cp == parameter block to use
 *    cdb == change descriptor block to use
 *********************************************************************/
extern void
    output_cdb_line (yangdiff_diffparms_t *cp,
		     const yangdiff_cdb_t *cdb);


/********************************************************************
 * FUNCTION output_diff
 * 
 *  Output one detailed difference
 *
 * INPUTS:
 *    cp == parameter block to use
 *    fieldname == fieldname that is different to print
 *    oldval == string value for old rev (may be NULL)
 *    newval == string value for new rev (may be NULL)
 *    isid == TRUE if value is really an identifier, not just a string
 *
 *********************************************************************/
extern void
    output_diff (yangdiff_diffparms_t *cp,
		 const xmlChar *fieldname,
		 const xmlChar *oldval,
		 const xmlChar *newval,
		 boolean isid);


/********************************************************************
 * FUNCTION output_errinfo_diff
 * 
 *  Output the differences report for one ncx_errinfo_t struct
 *  Used for may clauses, such as must, range, length, pattern;
 *  within a leaf, leaf-list, or typedef definition
 *
 * INPUTS:
 *    cp == parameter block to use
 *    olderr == old err info struct (may be NULL)
 *    newerr == new err info struct (may be NULL)
 *
 *********************************************************************/
extern void
    output_errinfo_diff (yangdiff_diffparms_t *cp,
			 const ncx_errinfo_t *olderr,
			 const ncx_errinfo_t *newerr);


/********************************************************************
* FUNCTION errinfo_changed
*
* Check if the ncx_errinfo_t struct changed at all
*
* INPUTS:
*   olderr == old error struct to check
*   newerr == new error struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
extern uint32
    errinfo_changed (const ncx_errinfo_t *olderr,
		     const ncx_errinfo_t *newerr);


/********************************************************************
* FUNCTION iffeature_changed
*
* Check if the ncx_iffeature_t struct changed at all
*
* INPUTS:
*   modprefix == module prefix value to use as default
*   oldif == old if-feature struct to check
*   newif == new if-feature struct to check
*
* RETURNS:
*   1 if field changed
*   0 if field not changed
*********************************************************************/
extern uint32
    iffeature_changed (const xmlChar *modprefix,
                       const ncx_iffeature_t *oldif,
                       const ncx_iffeature_t *newif);


/********************************************************************
 * FUNCTION iffeatureQ_changed
 * 
 *  Check if a Q of must-stmt definitions changed
 *
 * INPUTS:
 *    modprefix == module prefix in use
 *    oldQ == Q of old ncx_iffeature_t to use
 *    newQ == Q of new ncx_iffeature_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
extern uint32
    iffeatureQ_changed (const xmlChar *modprefix,
                        dlq_hdr_t *oldQ,
                        dlq_hdr_t *newQ);


/********************************************************************
 * FUNCTION output_iffeatureQ_diff
 * 
 *  Output any changes in a Q of if-feature-stmt definitions
 *
 * INPUTS:
 *    cp == comparison paraeters to use
 *    modprefix == module prefix in use
 *    oldQ == Q of old ncx_iffeature_t to use
 *    newQ == Q of new ncx_iffeature_t to use
 *
 *********************************************************************/
extern void
    output_iffeatureQ_diff (yangdiff_diffparms_t *cp,
                            const xmlChar *modprefix,
                            dlq_hdr_t *oldQ,
                            dlq_hdr_t *newQ);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangdiff_util */
