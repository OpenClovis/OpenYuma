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
#ifndef _H_var
#define _H_var

/*  FILE: var.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************


 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
23-aug-07    abb      Begun
09-mar-09    abb      Add more support for yangcli
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_runstack
#include "runstack.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/


/* top or parm values for the istop parameter */
#define  ISTOP   TRUE
#define  ISPARM  FALSE


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* different types of variables supported */
typedef enum var_type_t_ {
    VAR_TYP_NONE,
    VAR_TYP_SESSION,
    VAR_TYP_LOCAL,
    VAR_TYP_CONFIG,
    VAR_TYP_GLOBAL,
    VAR_TYP_SYSTEM,
    VAR_TYP_QUEUE
} var_type_t;


/* struct of NCX user variable mapping for yangcli */
typedef struct ncx_var_t_ {
    dlq_hdr_t     hdr;
    var_type_t    vartype;
    xmlns_id_t    nsid;     /* set to zero if not used */
    xmlChar      *name;
    val_value_t  *val;
} ncx_var_t;


/* values for isleft parameter in var_check_ref */

typedef enum var_side_t_ {
    ISRIGHT,
    ISLEFT
} var_side_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION var_free
* 
* Free a ncx_var_t struct
* 
* INPUTS:
*   var == var struct to free
* 
*********************************************************************/
extern void
    var_free (ncx_var_t *var);


/********************************************************************
* FUNCTION var_clean_varQ
* 
* Clean a Q of ncx_var_t
* 
* INPUTS:
*   varQ == Q of var structs to free
* 
*********************************************************************/
extern void
    var_clean_varQ (dlq_hdr_t *varQ);


/********************************************************************
* FUNCTION var_clean_type_from_varQ
* 
* Clean all entries of one type from a Q of ncx_var_t
* 
* INPUTS:
*   varQ == Q of var structs to free
*   vartype == variable type to delete
*********************************************************************/
extern void
    var_clean_type_from_varQ (dlq_hdr_t *varQ, 
                              var_type_t vartype);


/********************************************************************
* FUNCTION var_set_str
* 
* Find and set (or create a new) global user variable
* 
* INPUTS:
*   rcxt == runstack context to use to find the var
*   name == var name to set
*   namelen == length of name
*   value == var value to set
*   vartype == variable type
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_str (runstack_context_t *rcxt,
                 const xmlChar *name,
		 uint32 namelen,
		 const val_value_t *value,
		 var_type_t vartype);


/********************************************************************
* FUNCTION var_set
* 
* Find and set (or create a new) global user variable
* 
* INPUTS:
*   rcxt == runstack context to use to find the var
*   name == var name to set
*   value == var value to set
*   vartype == variable type
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set (runstack_context_t *rcxt,
             const xmlChar *name,
	     const val_value_t *value,
	     var_type_t vartype);


/********************************************************************
* FUNCTION var_set_str_que
* 
* Find and set (or create a new) global user variable
* 
* INPUTS:
*   varQ == variable binding Q to use instead of runstack
*   name == var name to set
*   namelen == length of name
*   value == var value to set
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_str_que (dlq_hdr_t  *varQ,
		     const xmlChar *name,
		     uint32 namelen,
		     const val_value_t *value);


/********************************************************************
* FUNCTION var_set_que
* 
* Find and set (or create a new) Q-based user variable
* 
* INPUTS:
*   varQ == varbind Q to use
*   name == var name to set
*   value == var value to set
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_que (dlq_hdr_t *varQ,
		 const xmlChar *name,
		 const val_value_t *value);


/********************************************************************
* FUNCTION var_set_move_que
* 
* Find or create and set a Q-based user variable
* 
* INPUTS:
*   varQ == varbind Q to use
*   name == var name to set
*   value == var value to set (pass off memory, do not clone!)
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_move_que (dlq_hdr_t *varQ,
                      const xmlChar *name,
                      val_value_t *value);


/********************************************************************
* FUNCTION var_set_move
* 
* Find and set (or create a new) global user variable
* Use the provided entry which will be freed later
* This function will not clone the value like var_set
*
* INPUTS:
*   rcxt == runstack context to use to find the var
*   name == var name to set
*   namelen == length of name string
*   vartype == variable type
*   value == var value to set
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_move (runstack_context_t *rcxt,
                  const xmlChar *name,
		  uint32 namelen,
		  var_type_t vartype,
		  val_value_t *value);


/********************************************************************
* FUNCTION var_set_sys
* 
* Find and set (or create a new) global system variable
* 
* INPUTS:
*   rcxt == runstack context to use to find the var
*   name == var name to set
*   value == var value to set
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_sys (runstack_context_t *rcxt,
                 const xmlChar *name,
		 const val_value_t *value);


/********************************************************************
* FUNCTION var_set_from_string
* 
* Find and set (or create a new) global user variable
* from a string value instead of a val_value_t struct
*
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to set
*   valstr == value string to set
*   vartype == variable type
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_set_from_string (runstack_context_t *rcxt,
                         const xmlChar *name,
			 const xmlChar *valstr,
			 var_type_t vartype);


/********************************************************************
* FUNCTION var_unset
* 
* Find and remove a local or global user variable
*
* !!! This function does not try global if local fails !!!
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to unset
*   namelen == length of name string
*   vartype == variable type
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    var_unset (runstack_context_t *rcxt,
               const xmlChar *name,
	       uint32 namelen,
	       var_type_t vartype);


/********************************************************************
* FUNCTION var_unset_que
* 
* Find and remove a Q-based user variable
* 
* INPUTS:
*   varQ == Q of ncx_var_t to use
*   name == var name to unset
*   namelen == length of name string
*   nsid == namespace ID to check if non-zero
* 
*********************************************************************/
extern status_t
    var_unset_que (dlq_hdr_t *varQ,
		   const xmlChar *name,
		   uint32 namelen,
		   xmlns_id_t  nsid);


/********************************************************************
* FUNCTION var_get_str
* 
* Find a global user variable
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
*   namelen == length of name
*   vartype == variable type
*
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get_str (runstack_context_t *rcxt,
                 const xmlChar *name,
		 uint32 namelen,
		 var_type_t vartype);


/********************************************************************
* FUNCTION var_get
* 
* Find a local or global user variable
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
*   vartype == variable type
* 
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get (runstack_context_t *rcxt,
             const xmlChar *name,
	     var_type_t vartype);


/********************************************************************
* FUNCTION var_get_type_str
* 
* Find a user variable; get its var type
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
*   namelen == length of name
*   globalonly == TRUE to check only the global Q
*                 FALSE to check local, then global Q
*
* RETURNS:
*   var type if found, or VAR_TYP_NONE
*********************************************************************/
extern var_type_t
    var_get_type_str (runstack_context_t *rcxt,
                      const xmlChar *name,
		      uint32 namelen,
		      boolean globalonly);


/********************************************************************
* FUNCTION var_get_type
* 
* Get the var type of a specified var name
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
*   globalonly == TRUE to check only the global Q
*                 FALSE to check local, then global Q
*
* RETURNS:
*   var type or VAR_TYP_NONE if not found
*********************************************************************/
extern var_type_t
    var_get_type (runstack_context_t *rcxt,
                  const xmlChar *name,
		  boolean globalonly);


/********************************************************************
* FUNCTION var_get_str_que
* 
* Find a global user variable
* 
* INPUTS:
*   varQ == queue of ncx_var_t to use
*   name == var name to get
*   namelen == length of name
*   nsid == namespace ID  for name (0 if not used)
*
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get_str_que (dlq_hdr_t *varQ,
		     const xmlChar *name,
		     uint32 namelen,
		     xmlns_id_t nsid);


/********************************************************************
* FUNCTION var_get_que
* 
* Find a Q-based user variable
* 
* INPUTS:
*   varQ == Q of ncx_var_t to use
*   name == var name to get
*   nsid == namespace ID  for name (0 if not used)
* 
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get_que (dlq_hdr_t *varQ,
		 const xmlChar *name,
		 xmlns_id_t nsid);


/********************************************************************
* FUNCTION var_get_que_raw
* 
* Find a Q-based user variable; return the var struct instead
* of just the value
* 
* INPUTS:
*   varQ == Q of ncx_var_t to use
*   nsid == namespace ID to match (0 if not used)
*   name == var name to get
* 
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern ncx_var_t *
    var_get_que_raw (dlq_hdr_t *varQ,
		     xmlns_id_t  nsid,
		     const xmlChar *name);


/********************************************************************
* FUNCTION var_get_local
* 
* Find a local user variable
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
* 
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get_local (runstack_context_t *rcxt,
                   const xmlChar *name);


/********************************************************************
* FUNCTION var_get_local_str
* 
* Find a local user variable, count-based name string
* 
* INPUTS:
*   rcxt == runstack context to use
*   name == var name to get
* 
* RETURNS:
*   pointer to value, or NULL if not found
*********************************************************************/
extern val_value_t *
    var_get_local_str (runstack_context_t *rcxt,
                       const xmlChar *name,
		       uint32 namelen);


/********************************************************************
* FUNCTION var_check_ref
* 
* Check if the immediate command sub-string is a variable
* reference.  If so, return the (vartype, name, namelen)
* tuple that identifies the reference.  Also return
* the total number of chars consumed from the input line.
* 
* E.g.,
*
*   $foo = get-config filter=@filter.xml
*
* INPUTS:
*   rcxt == runstack context to use
*   line == command line string to expand
*   isleft == TRUE if left hand side of an expression
*          == FALSE if right hand side ($1 type vars allowed)
*   len  == address of number chars parsed so far in line
*   vartype == address of return variable Q type
*   name == address of string start return val
*   namelen == address of name length return val
*
* OUTPUTS:
*   *len == number chars consumed by this function
*   *vartype == variable type enum
*   *name == start of name string
*   *namelen == length of *name string
*
* RETURNS:
*    status   
*********************************************************************/
extern status_t
    var_check_ref (runstack_context_t *rcxt,
                   const xmlChar *line,
		   var_side_t side,
		   uint32   *len,
		   var_type_t *vartype,
		   const xmlChar **name,
		   uint32 *namelen);


/********************************************************************
* FUNCTION var_get_script_val
* 
* Create or fill in a val_value_t struct for a parameter assignment
* within the script processing mode
*
* See ncxcli.c for details on the script syntax
*
* INPUTS:
*   rcxt == runstack context to use
*   obj == expected type template 
*          == NULL and will be set to NCX_BT_STRING for
*             simple types
*   val == value to fill in :: val->obj MUST be set
*          == NULL to create a new one
*   strval == string value to check
*   istop == TRUE (ISTOP) if calling from top level assignment
*            An unquoted string is the start of a command
*         == FALSE (ISPARM) if calling from a parameter parse
*            An unquoted string is just a string
*   res == address of status result
*
* OUTPUTS:
*   *res == status
*
* RETURNS:
*   If error, then returns NULL;
*   If no error, then returns pointer to new val or filled in 'val'
*********************************************************************/
extern val_value_t *
    var_get_script_val (runstack_context_t *rcxt,
                        obj_template_t *obj,
			val_value_t *val,
			const xmlChar *strval,
			boolean istop,
			status_t *res);


/********************************************************************
* FUNCTION var_get_script_val_ex
* 
* Create or fill in a val_value_t struct for a parameter assignment
* within the script processing mode
* Allow external values
*
* See ncxcli.c for details on the script syntax
*
* INPUTS:
*   rcxt == runstack context to use
*   parentobj == container or list real node parent of 'obj'
*          == NULL and will be set to NCX_BT_STRING for
*             simple types
*   obj == expected type template 
*          == NULL and will be set to NCX_BT_STRING for
*             simple types
*   val == value to fill in :: val->obj MUST be set
*          == NULL to create a new one
*   strval == string value to check
*   istop == TRUE (ISTOP) if calling from top level assignment
*            An unquoted string is the start of a command
*         == FALSE (ISPARM) if calling from a parameter parse
*            An unquoted string is just a string
*   fillval == value from yangcli, could be NCX_BT_EXTERN;
*              used instead of strval!
*           == NULL: not used
*   res == address of status result
*   
* OUTPUTS:
*   *res == status
*
* RETURNS:
*   If error, then returns NULL;
*   If no error, then returns pointer to new val or filled in 'val'
*********************************************************************/
extern val_value_t *
    var_get_script_val_ex (runstack_context_t *rcxt,
                           obj_template_t *parentobj,
                           obj_template_t *obj,
                           val_value_t *val,
                           const xmlChar *strval,
                           boolean istop,
                           val_value_t *fillval,
                           status_t *res);


/********************************************************************
* FUNCTION var_check_script_val
* 
* Create a val_value_t struct for a parameter assignment
* within the script processing mode, if a var ref is found
*
* See yangcli documentation for details on the script syntax
*
* INPUTS:
*   rcxt == runstack context to use
*   obj == expected object template 
*          == NULL and will be set to NCX_BT_STRING for
*             simple types
*   strval == string value to check
*   istop == TRUE if calling from top level assignment
*            An unquoted string is the start of a command
*         == FALSE if calling from a parameter parse
*            An unquoted string is just a string
*   res == address of status result
*
* OUTPUTS:
*   *res == status
*
* RETURNS:
*   If no error, then returns pointer to new malloced val 
*   If error, then returns NULL
*********************************************************************/
extern val_value_t *
    var_check_script_val (runstack_context_t *rcxt,
                          obj_template_t *obj,
			  const xmlChar *strval,
			  boolean istop,
			  status_t *res);


/********************************************************************
* FUNCTION var_cvt_generic
* 
* Cleanup after a yangcli session has ended
*
* INPUTS:
*   varQ == Q of ncx_var_t to cleanup and change to generic
*           object pointers
*
*********************************************************************/
extern void
    var_cvt_generic (dlq_hdr_t *varQ);


/********************************************************************
* FUNCTION var_find
* 
* Find a complete var struct for use with XPath
*
* INPUTS:
*   rcxt == runstack context to use
*   varname == variable name string
*   nsid == namespace ID for varname (0 is OK)
*
* RETURNS:
*   pointer to ncx_var_t for the first match found (local or global)
*********************************************************************/
extern ncx_var_t *
    var_find (runstack_context_t *rcxt,
              const xmlChar *varname,
              xmlns_id_t nsid);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_var */
