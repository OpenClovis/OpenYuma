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
#ifndef _H_yang
#define _H_yang

/*  FILE: yang.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module parser utilities

    
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
15-nov-07    abb      Begun; start from yang_parse.c

*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* YANG parser mode entry types */
typedef enum yang_parsetype_t_ {
    YANG_PT_NONE,
    YANG_PT_TOP,          /* called from top level [sub]module */
    YANG_PT_INCLUDE,         /* called from module include-stmt */
    YANG_PT_IMPORT            /* called from module import-stmt */
} yang_parsetype_t;


/* YANG statement types for yang_stmt_t order struct */
typedef enum yang_stmttype_t_ {
    YANG_ST_NONE,
    YANG_ST_TYPEDEF,            /* node is typ_template_t */
    YANG_ST_GROUPING,           /* node is grp_template_t */
    YANG_ST_EXTENSION,          /* node is ext_template_t */
    YANG_ST_OBJECT,             /* node is obj_template_t */
    YANG_ST_IDENTITY,           /* node is ncx_identity_t */
    YANG_ST_FEATURE,             /* node is ncx_feature_t */
    YANG_ST_DEVIATION          /* node is ncx_deviation_t */
} yang_stmttype_t;


/* YANG statement node to track top-level statement order for doc output */
typedef struct yang_stmt_t_ {
    dlq_hdr_t        qhdr;
    yang_stmttype_t  stmttype;
    /* these node pointers are back-pointers and not malloced here */
    union s_ {
        typ_template_t  *typ;
        grp_template_t  *grp;
        ext_template_t  *ext;
        obj_template_t  *obj;
        ncx_identity_t  *identity;
        ncx_feature_t   *feature;
        obj_deviation_t *deviation;
    } s;
} yang_stmt_t;


/* YANG node entry to track if a module has been used already */
typedef struct yang_node_t_ {
    dlq_hdr_t     qhdr;
    const xmlChar *name;    /* module name in imp/inc/failed */
    const xmlChar *revision;   /* revision date in imp/inc/failed */
    ncx_module_t  *mod;     /* back-ptr to module w/ imp/inc */
    ncx_module_t  *submod;             /* submod for allincQ */
    xmlChar       *failed;  /* saved name for failed entries */
    xmlChar       *failedrev;  /* saved revision for failed entries */
    status_t       res;         /* saved result for 'failed' */
    ncx_error_t    tkerr;
} yang_node_t;


/* YANG import pointer node to track all imports used */
typedef struct yang_import_ptr_t_ {
    dlq_hdr_t    qhdr;
    xmlChar     *modname;
    xmlChar     *modprefix;
    xmlChar     *revision;
} yang_import_ptr_t;


/* YANG parser control block
 *
 * top level parse can be for a module or a submodule
 *
 * The allimpQ is a cache of pointers to all the imports that
 * have been processed.  This is used by yangdump for
 * document processing and error detection. 
 *
 * The impchainQ is really a stack of imports that
 * are being processed, used for import loop detection
 * 
 * The allincQ is a cache of all the includes that have been
 * processed.  Includes can be nested, and are only parsed once,
 *
 * The incchainQ is really a stack of includes that
 * are being processed, used for include loop detection
 *
 * The failedQ is a list of all the modules that had fatal
 * errors, and have already been processed.  This is used
 * to prevent error messages from imports to be printed more
 * than once, and speeds up validation processing
 */
typedef struct yang_pcb_t_ {
    struct ncx_module_t_ *top;        /* top-level file */
    struct ncx_module_t_ *retmod;   /* nested [sub]mod being returned */
    struct ncx_module_t_ *parentparm;   /* parent passed for submodule */
    const xmlChar *revision;        /* back-ptr to rev to match */
    boolean       with_submods;
    boolean       stmtmode;      /* save top-level stmt order */
    boolean       diffmode;        /* TRUE = yangdiff old ver */
    boolean       deviationmode;  /* TRUE if keeping deviations only */
    boolean       searchmode;  /* TRUE if just getting ns & version */
    boolean       parsemode;  /* TRUE if full parse but no reg-load */
    boolean       keepmode;    /* TRUE to keep new mod even if loaded */
    boolean       topfound;    /* TRUE if top found, not added */
    boolean       topadded;    /* TRUE if top in registry; F: need free */
    boolean       savetkc;     /* TRUE if tkc should be kept in tkc */
    boolean       docmode;     /* TRUE if saving strings in origtkQ */
    dlq_hdr_t     allimpQ;          /* Q of yang_import_ptr_t */

    dlq_hdr_t    *savedevQ;  /* ptr to Q of ncx_save_deviations_t */
    tk_chain_t   *tkc;              /* live or NULL parse chain  */

    /* 2 Qs of yang_node_t */
    dlq_hdr_t     impchainQ;      /* cur chain of import used */
    dlq_hdr_t     failedQ;       /* load mod or submod failed */
} yang_pcb_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION yang_consume_semiapp
* 
* consume a stmtsep clause
* Consume a semi-colon to end a simple clause, or
* consume a vendor extension
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   appinfoQ == queue to receive any vendor extension found
*
* OUTPUTS:
*   *appinfoQ has the extesnion added if any
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_semiapp (tk_chain_t *tkc,
			  ncx_module_t *mod,
			  dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_string
* 
* consume 1 string token
* Consume a YANG string token in any of the 3 forms
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*   if field not NULL,
*        *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_string (tk_chain_t *tkc,
			 ncx_module_t *mod,
			 xmlChar **field);


/********************************************************************
* FUNCTION yang_consume_keyword
* 
* consume 1 YANG keyword or vendor extension keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   prefix == address of field to get the prefix string (may be NULL)
*   field == address of field to get the name string (may be NULL)
*
* OUTPUTS:
*   if prefix not NULL,
*        *prefix is set with a malloced string in NO_ERR
*   if field not NULL,
*        *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_keyword (tk_chain_t *tkc,
			  ncx_module_t *mod,
			  xmlChar **prefix,
			  xmlChar **field);


/********************************************************************
* FUNCTION yang_consume_nowsp_string
* 
* consume 1 string without any whitespace
* Consume a YANG string token in any of the 3 forms
* Check No whitespace allowed!
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*   if field not NULL,
*      *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_nowsp_string (tk_chain_t *tkc,
			       ncx_module_t *mod,
			       xmlChar **field);


/********************************************************************
* FUNCTION yang_consume_id_string
* 
* consume an identifier-str token
* Consume a YANG string token in any of the 3 forms
* Check that it is a valid YANG identifier string

* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*  if field not NULL:
*       *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_id_string (tk_chain_t *tkc,
			    ncx_module_t *mod,
			    xmlChar **field);


/********************************************************************
* FUNCTION yang_consume_pid_string
* 
* consume an identifier-ref-str token
* Consume a YANG string token in any of the 3 forms
* Check that it is a valid identifier-ref-string
* Get the prefix if any is set
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   prefix == address of prefix string if any is used (may be NULL)
*   field == address of field to get the string (may be NULL)
*
* OUTPUTS:
*  if field not NULL:
*       *field is set with a malloced string in NO_ERR
*   current token is advanced
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_pid_string (tk_chain_t *tkc,
			     ncx_module_t *mod,
			     xmlChar **prefix,
			     xmlChar **field);


/********************************************************************
* FUNCTION yang_consume_error_stmts
* 
* consume the range. length. pattern. must error info extensions
* Parse the sub-section as a sub-section for error-app-tag
* and error-message clauses
*
* Current token is the starting left brace for the sub-section
* that is extending a range, length, pattern, or must statement
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   errinfo == pointer to valid ncx_errinfo_t struct
*              The struct will be filled in by this fn
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *errinfo filled in with any clauses found
*   *appinfoQ filled in with any extensions found
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_error_stmts (tk_chain_t  *tkc,
			      ncx_module_t *mod,
			      ncx_errinfo_t *errinfo,
			      dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_descr
* 
* consume one descriptive string clause
* Parse the description or reference statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_descr (tk_chain_t  *tkc,
			ncx_module_t *mod,
			xmlChar **str,
			boolean *dupflag,
			dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_pid
* 
* consume one [prefix:]name clause
* Parse the rest of the statement (2 forms):
*
*        keyword [prefix:]name;
*
*        keyword [prefix:]name { appinfo }
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   prefixstr == prefix string to set  (may be NULL)
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_pid (tk_chain_t  *tkc,
		      ncx_module_t *mod,
		      xmlChar **prefixstr,
		      xmlChar **str,
		      boolean *dupflag,
		      dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_strclause
* 
* consume one normative string clause
* Parse the string-parameter-based statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   str == string to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_strclause (tk_chain_t  *tkc,
			    ncx_module_t *mod,
			    xmlChar **str,
			    boolean *dupflag,
			    dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_status
* 
* consume one status clause
* Parse the status statement
*
* Current token is the 'status' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   stat == status object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *stat set to the value if stat not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_status (tk_chain_t  *tkc,
			 ncx_module_t *mod,
			 ncx_status_t *status,
			 boolean *dupflag,
			 dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_ordered_by
* 
* consume one ordered-by clause
* Parse the ordered-by statement
*
* Current token is the 'ordered-by' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   ordsys == ordered-by object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *ordsys set to the value if not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_ordered_by (tk_chain_t  *tkc,
			     ncx_module_t *mod,
			     boolean *ordsys,
			     boolean *dupflag,
			     dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_max_elements
* 
* consume one max-elements clause
* Parse the max-elements statement
*
* Current token is the 'max-elements' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   maxelems == max-elements object to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *maxelems set to the value if not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_max_elements (tk_chain_t  *tkc,
			       ncx_module_t *mod,
			       uint32 *maxelems,
			       boolean *dupflag,
			       dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_must
* 
* consume one must-stmt into mustQ
* Parse the must statement
*
* Current token is the 'must' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   mustQ  == address of Q of xpath_pcb_t structs to store
*             a new malloced xpath_pcb_t 
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   must entry malloced and  added to mustQ (if NO_ERR)
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    yang_consume_must (tk_chain_t  *tkc,
		       ncx_module_t *mod,
		       dlq_hdr_t *mustQ,
		       dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_when
* 
* consume one when-stmt into obj->when
* Parse the when statement
*
* Current token is the 'when' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   obj    == obj_template_t of the parent object of this 'when'
*   whenflag == address of boolean set-once flag for the when-stmt
*               an error will be generated if the value passed
*               in is TRUE.  Set the initial value to FALSE
*               before first call, and do not change it after that
* OUTPUTS:
*   obj->when malloced and dilled in
*   *whenflag set if not set already
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    yang_consume_when (tk_chain_t  *tkc,
		       ncx_module_t *mod,
		       obj_template_t *obj,
		       boolean        *whenflag);


/********************************************************************
* FUNCTION yang_consume_iffeature
* 
* consume one if-feature-stmt into iffeatureQ
* Parse the if-feature statement
*
* Current token is the 'if-feature' keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   iffeatureQ  == Q of ncx_iffeature_t to hold the new if-feature
*   appinfoQ  == queue to store appinfo (extension usage)
*
* OUTPUTS:
*   ncx_iffeature_t malloced and added to iffeatureQ
*   maybe ncx_appinfo_t structs added to appinfoQ
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    yang_consume_iffeature (tk_chain_t *tkc,
			    ncx_module_t *mod,
			    dlq_hdr_t *iffeatureQ,
			    dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_boolean
* 
* consume one boolean clause
* Parse the boolean parameter based statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   boolval == boolean value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *str set to the value if str not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_boolean (tk_chain_t  *tkc,
			  ncx_module_t *mod,
			  boolean *boolval,
			  boolean *dupflag,
			  dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_int32
* 
* consume one int32 clause
* Parse the int32 based parameter statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   num    == int32 value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *num set to the value if num not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_int32 (tk_chain_t  *tkc,
			ncx_module_t *mod,
			int32 *num,
			boolean *dupflag,
			dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_consume_uint32
* 
* consume one uint32 clause
* Parse the uint32 based parameter statement
*
* Current token is the starting keyword
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc    == token chain
*   mod    == module in progress
*   num    == uint32 value to set  (may be NULL)
*   dupflag == flag to check if entry already found (may be NULL)
*   appinfoQ == Q to hold any extensions found (may be NULL)
*
* OUTPUTS:
*   *num set to the value if num not NULL
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_consume_uint32 (tk_chain_t  *tkc,
			 ncx_module_t *mod,
			 uint32 *num,
			 boolean *dupflag,
			 dlq_hdr_t *appinfoQ);


/********************************************************************
* FUNCTION yang_find_imp_typedef
* 
* Find the specified imported typedef
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == type name to use
*   tkerr == error record to use in error messages (may be NULL)
*   typ  == address of return typ_template_t pointer
*
* OUTPUTS:
*   *typ set to the found typ_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_find_imp_typedef (yang_pcb_t *pcb,
                           tk_chain_t  *tkc,
			   ncx_module_t *mod,
			   const xmlChar *prefix,
			   const xmlChar *name,
			   ncx_error_t *tkerr,
			   typ_template_t **typ);


/********************************************************************
* FUNCTION yang_find_imp_grouping
* 
* Find the specified imported grouping
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == grouping name to use
*   tkerr == error record to use in error messages (may be NULL)
*   grp  == address of return grp_template_t pointer
*
* OUTPUTS:
*   *grp set to the found grp_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_find_imp_grouping (yang_pcb_t *pcb,
                            tk_chain_t  *tkc,
			    ncx_module_t *mod,
			    const xmlChar *prefix,
			    const xmlChar *name,
			    ncx_error_t *tkerr,
			    grp_template_t **grp);


/********************************************************************
* FUNCTION yang_find_imp_extension
* 
* Find the specified imported extension
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == extension name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   ext  == address of return ext_template_t pointer
*
* OUTPUTS:
*   *ext set to the found ext_template_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_find_imp_extension (yang_pcb_t *pcb,
                             tk_chain_t  *tkc,
			     ncx_module_t *mod,
			     const xmlChar *prefix,
			     const xmlChar *name,
			     ncx_error_t *tkerr,
			     ext_template_t **ext);


/********************************************************************
* FUNCTION yang_find_imp_feature
* 
* Find the specified imported feature
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == feature name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   feature  == address of return ncx_feature_t pointer
*
* OUTPUTS:
*   *feature set to the found ncx_feature_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_find_imp_feature (yang_pcb_t *pcb,
                           tk_chain_t  *tkc,
			   ncx_module_t *mod,
			   const xmlChar *prefix,
			   const xmlChar *name,
			   ncx_error_t *tkerr,
			   ncx_feature_t **feature);


/********************************************************************
* FUNCTION yang_find_imp_identity
* 
* Find the specified imported identity
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   pcb == parser control block to use
*   tkc    == token chain
*   mod    == module in progress
*   prefix  == prefix value to use
*   name == feature name to use
*   tkerr == error struct to use in error messages (may be NULL)
*   identity  == address of return ncx_identity_t pointer
*
* OUTPUTS:
*   *identity set to the found ncx_identity_t, if NO_ERR
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    yang_find_imp_identity (yang_pcb_t *pcb,
                            tk_chain_t  *tkc,
			    ncx_module_t *mod,
			    const xmlChar *prefix,
			    const xmlChar *name,
			    ncx_error_t *tkerr,
			    ncx_identity_t **identity);


/********************************************************************
* FUNCTION yang_check_obj_used
* 
* generate warnings if local typedefs/groupings not used
* Check if the local typedefs and groupings are actually used
* Generate warnings if not used
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain in progress
*   mod == module in progress
*   typeQ == typedef Q to check
*   grpQ == pgrouping Q to check
*
*********************************************************************/
extern void
    yang_check_obj_used (tk_chain_t *tkc,
			 ncx_module_t *mod,
			 dlq_hdr_t *typeQ,
			 dlq_hdr_t *grpQ);


/********************************************************************
* FUNCTION yang_check_imports_used
* 
* generate warnings if imports not used
* Check if the imports statements are actually used
* Check if the  import is newer than the importing module
* Generate warnings if so
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain in progress
*   mod == module in progress
*
*********************************************************************/
extern void
    yang_check_imports_used (tk_chain_t *tkc,
			     ncx_module_t *mod);


/********************************************************************
* FUNCTION yang_new_node
* 
* Create a new YANG parser node
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_node_t *
    yang_new_node (void);


/********************************************************************
* FUNCTION yang_free_node
* 
* Delete a YANG parser node
*
* INPUTS:
*  node == parser node to delete
*
*********************************************************************/
extern void
    yang_free_node (yang_node_t *node);


/********************************************************************
* FUNCTION yang_clean_nodeQ
* 
* Delete all the node entries in a YANG parser node Q
*
* INPUTS:
*    que == Q of yang_node_t to clean
*
*********************************************************************/
extern void
    yang_clean_nodeQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION yang_find_node
* 
* Find a YANG parser node in the specified Q
*
* INPUTS:
*    que == Q of yang_node_t to check
*    name == module name to find
*    revision == module revision date (may be NULL)
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
extern yang_node_t *
    yang_find_node (const dlq_hdr_t *que,
		    const xmlChar *name,
		    const xmlChar *revision);


/********************************************************************
* FUNCTION yang_dump_nodeQ
* 
* log_debug output for contents of the specified nodeQ
*
* INPUTS:
*    que == Q of yang_node_t to check
*    name == Q name (may be NULL)
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
extern void
    yang_dump_nodeQ (dlq_hdr_t *que,
		     const char *name);


/********************************************************************
* FUNCTION yang_new_pcb
* 
* Create a new YANG parser control block
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_pcb_t *
    yang_new_pcb (void);


/********************************************************************
* FUNCTION yang_free_pcb
* 
* Delete a YANG parser control block
*
* INPUTS:
*    pcb == parser control block to delete
*
*********************************************************************/
extern void
    yang_free_pcb (yang_pcb_t *pcb);


/********************************************************************
* FUNCTION yang_new_typ_stmt
* 
* Create a new YANG stmt node for a typedef
*
* INPUTS:
*   typ == type template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_typ_stmt (typ_template_t *typ);


/********************************************************************
* FUNCTION yang_new_grp_stmt
* 
* Create a new YANG stmt node for a grouping
*
* INPUTS:
*   grp == grouping template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_grp_stmt (grp_template_t *grp);



/********************************************************************
* FUNCTION yang_new_ext_stmt
* 
* Create a new YANG stmt node for an extension
*
* INPUTS:
*   ext == extension template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_ext_stmt (ext_template_t *ext);


/********************************************************************
* FUNCTION yang_new_obj_stmt
* 
* Create a new YANG stmt node for an object
*
* INPUTS:
*   obj == object template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_obj_stmt (obj_template_t *obj);


/********************************************************************
* FUNCTION yang_new_id_stmt
* 
* Create a new YANG stmt node for an identity
*
* INPUTS:
*   identity == identity template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_id_stmt (ncx_identity_t *identity);


/********************************************************************
* FUNCTION yang_new_feature_stmt
* 
* Create a new YANG stmt node for a feature definition
*
* INPUTS:
*   feature == feature template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_feature_stmt (ncx_feature_t *feature);


/********************************************************************
* FUNCTION yang_new_deviation_stmt
* 
* Create a new YANG stmt node for a deviation definition
*
* INPUTS:
*   deviation == deviation template to use for new statement struct
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_stmt_t *
    yang_new_deviation_stmt (obj_deviation_t *deviation);


/********************************************************************
* FUNCTION yang_free_stmt
* 
* Delete a YANG statement node
*
* INPUTS:
*    stmt == yang_stmt_t node to delete
*
*********************************************************************/
extern void
    yang_free_stmt (yang_stmt_t *stmt);


/********************************************************************
* FUNCTION yang_clean_stmtQ
* 
* Delete a Q of YANG statement node
*
* INPUTS:
*    que == Q of yang_stmt_t node to delete
*
*********************************************************************/
extern void
    yang_clean_stmtQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION yang_validate_date_string
* 
* Validate a YANG date string for a revision entry
*
* Expected format is:
*
*    YYYY-MM-DD
*
* Error messages are printed by this function
*
* INPUTS:
*    tkc == token chain in progress
*    mod == module in progress
*    tkerr == error struct to use (may be NULL to use tkc->cur)
*    datestr == string to validate
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    yang_validate_date_string (tk_chain_t *tkc,
			      ncx_module_t *mod,
			      ncx_error_t *tkerr,
			      const xmlChar *datestr);


/********************************************************************
* FUNCTION yang_skip_statement
* 
* Skip past the current invalid statement, starting at
* an invalid keyword
*
* INPUTS:
*    tkc == token chain in progress
*    mod == module in progress
*********************************************************************/
extern void
    yang_skip_statement (tk_chain_t *tkc,
			 ncx_module_t *mod);


/********************************************************************
* FUNCTION yang_top_keyword
* 
* Check if the string is a top-level YANG keyword
*
* INPUTS:
*    keyword == string to check
*
* RETURNS:
*   TRUE if a top-level YANG keyword, FALSE otherwise
*********************************************************************/
extern boolean
    yang_top_keyword (const xmlChar *keyword);


/********************************************************************
* FUNCTION yang_new_import_ptr
* 
* Create a new YANG import pointer node
*
* INPUTS:
*   modname = module name to set
*   modprefix = module prefix to set
*   revision = revision date to set
*
* RETURNS:
*   pointer to new and initialized struct, NULL if memory error
*********************************************************************/
extern yang_import_ptr_t *
    yang_new_import_ptr (const xmlChar *modname,
			 const xmlChar *modprefix,
			 const xmlChar *revision);


/********************************************************************
* FUNCTION yang_free_impptr_ptr
* 
* Delete a YANG import pointer node
*
* INPUTS:
*  impptr == import pointer node to delete
*
*********************************************************************/
extern void
    yang_free_import_ptr (yang_import_ptr_t *impptr);


/********************************************************************
* FUNCTION yang_clean_import_ptrQ
* 
* Delete all the node entries in a YANG import pointer node Q
*
* INPUTS:
*    que == Q of yang_import_ptr_t to clean
*
*********************************************************************/
extern void
    yang_clean_import_ptrQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION yang_find_import_ptr
* 
* Find a YANG import pointer node in the specified Q
*
* INPUTS:
*    que == Q of yang_import_ptr_t to check
*    name == module name to find
*
* RETURNS:
*    pointer to found node, NULL if not found
*********************************************************************/
extern yang_import_ptr_t *
    yang_find_import_ptr (dlq_hdr_t *que,
			  const xmlChar *name);


/********************************************************************
* FUNCTION yang_compare_revision_dates
* 
* Compare 2 revision strings, which either may be NULL
*
* INPUTS:
*   revstring1 == revision string 1 (may be NULL)
*   revstring2 == revision string 2 (may be NULL)
*
* RETURNS:
*    -1 if revision 1 < revision 2
*    0 of they are the same
*    +1 if revision1 > revision 2   
*********************************************************************/
extern int32
    yang_compare_revision_dates (const xmlChar *revstring1,
				 const xmlChar *revstring2);


/********************************************************************
* FUNCTION yang_make_filename
* 
* Malloc and construct a YANG filename 
*
* INPUTS:
*   modname == [sub]module name
*   revision == [sub]module revision date (may be NULL)
*   isyang == TRUE for YANG extension
*             FALSE for YIN extension
*
* RETURNS:
*    malloced and filled in string buffer with filename
*    NULL if any error
*********************************************************************/
extern xmlChar *
    yang_make_filename (const xmlChar *modname,
                        const xmlChar *revision,
                        boolean isyang);


/********************************************************************
* FUNCTION yang_copy_filename
* 
* Construct a YANG filename into a provided buffer
*
* INPUTS:
*   modname == [sub]module name
*   revision == [sub]module revision date (may be NULL)
*   buffer == buffer to copy filename into
*   bufflen == number of bytes available in buffer
*   isyang == TRUE for YANG extension
*             FALSE for YIN extension
*
* RETURNS:
*    malloced and filled in string buffer with filename
*    NULL if any error
*********************************************************************/
extern status_t
    yang_copy_filename (const xmlChar *modname,
                        const xmlChar *revision,
                        xmlChar *buffer,
                        uint32 bufflen,
                        boolean isyang);


/********************************************************************
* FUNCTION yang_split_filename
* 
* Split a module parameter into its filename components
*
* INPUTS:
*   filename == filename string
*   modnamelen == address of return modname length
*
* OUTPUTS:
*   *modnamelen == module name length
*
* RETURNS:
*    TRUE if module@revision form was found
*    FALSE if not, and ignore the output because a
*      different form of the module parameter was used
*********************************************************************/
extern boolean
    yang_split_filename (const xmlChar *filename,
                         uint32 *modnamelen);


/********************************************************************
* FUNCTION yang_fileext_is_yang
* 
* Check if the filespec ends with the .yang extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .yang file extension found
*    FALSE if not
*********************************************************************/
extern boolean
    yang_fileext_is_yang (const xmlChar *filename);


/********************************************************************
* FUNCTION yang_fileext_is_yin
* 
* Check if the filespec ends with the .yin extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .yin file extension found
*    FALSE if not
*********************************************************************/
extern boolean
    yang_fileext_is_yin (const xmlChar *filename);


/********************************************************************
* FUNCTION yang_fileext_is_xml
* 
* Check if the filespec ends with the .xml extension
*
* INPUTS:
*   filename == filename string
*
* RETURNS:
*    TRUE if .xml file extension found
*    FALSE if not
*********************************************************************/
extern boolean
    yang_fileext_is_xml (const xmlChar *filename);


/********************************************************************
* FUNCTION yang_final_memcheck
* 
* Check the node malloc and free counts
*********************************************************************/
extern void
    yang_final_memcheck (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yang */
