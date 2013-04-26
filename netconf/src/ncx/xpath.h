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
#ifndef _H_xpath
#define _H_xpath

/*  FILE: xpath.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Schema and data model Xpath search support

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
30-dec-07    abb      Begun
06-jul-11    abb      add wildcard match support

*/

#include <xmlstring.h>
#include <xmlreader.h>
#include <xmlregexp.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_var
#include "var.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* max size of the pcb->result_cacheQ */
#define XPATH_RESULT_CACHE_MAX     16

/* max size of the pcb->resnode_cacheQ */
#define XPATH_RESNODE_CACHE_MAX     64


/* XPath 1.0 sec 2.2 AxisName */
#define XP_AXIS_ANCESTOR           (const xmlChar *)"ancestor"
#define XP_AXIS_ANCESTOR_OR_SELF   (const xmlChar *)"ancestor-or-self"
#define XP_AXIS_ATTRIBUTE          (const xmlChar *)"attribute"
#define XP_AXIS_CHILD              (const xmlChar *)"child"
#define XP_AXIS_DESCENDANT         (const xmlChar *)"descendant"
#define XP_AXIS_DESCENDANT_OR_SELF (const xmlChar *)"descendant-or-self"
#define XP_AXIS_FOLLOWING          (const xmlChar *)"following"
#define XP_AXIS_FOLLOWING_SIBLING  (const xmlChar *)"following-sibling"
#define XP_AXIS_NAMESPACE          (const xmlChar *)"namespace"
#define XP_AXIS_PARENT             (const xmlChar *)"parent"
#define XP_AXIS_PRECEDING          (const xmlChar *)"preceding"
#define XP_AXIS_PRECEDING_SIBLING  (const xmlChar *)"preceding-sibling"
#define XP_AXIS_SELF               (const xmlChar *)"self"

/* Xpath 1.0 Function library + current() from XPath 2.0 */
#define XP_FN_BOOLEAN              (const xmlChar *)"boolean"
#define XP_FN_CEILING              (const xmlChar *)"ceiling"
#define XP_FN_CONCAT               (const xmlChar *)"concat"
#define XP_FN_CONTAINS             (const xmlChar *)"contains"
#define XP_FN_COUNT                (const xmlChar *)"count"
#define XP_FN_CURRENT              (const xmlChar *)"current"
#define XP_FN_FALSE                (const xmlChar *)"false"
#define XP_FN_FLOOR                (const xmlChar *)"floor"
#define XP_FN_ID                   (const xmlChar *)"id"
#define XP_FN_LANG                 (const xmlChar *)"lang"
#define XP_FN_LAST                 (const xmlChar *)"last"
#define XP_FN_LOCAL_NAME           (const xmlChar *)"local-name"
#define XP_FN_NAME                 (const xmlChar *)"name"
#define XP_FN_NAMESPACE_URI        (const xmlChar *)"namespace-uri"
#define XP_FN_NORMALIZE_SPACE      (const xmlChar *)"normalize-space"
#define XP_FN_NOT                  (const xmlChar *)"not"
#define XP_FN_NUMBER               (const xmlChar *)"number"
#define XP_FN_POSITION             (const xmlChar *)"position"
#define XP_FN_ROUND                (const xmlChar *)"round"
#define XP_FN_STARTS_WITH          (const xmlChar *)"starts-with"
#define XP_FN_STRING               (const xmlChar *)"string"
#define XP_FN_STRING_LENGTH        (const xmlChar *)"string-length"
#define XP_FN_SUBSTRING            (const xmlChar *)"substring"
#define XP_FN_SUBSTRING_AFTER      (const xmlChar *)"substring-after"
#define XP_FN_SUBSTRING_BEFORE     (const xmlChar *)"substring-before"
#define XP_FN_SUM                  (const xmlChar *)"sum"
#define XP_FN_TRANSLATE            (const xmlChar *)"translate"
#define XP_FN_TRUE                 (const xmlChar *)"true"

/* yuma function extensions */
#define XP_FN_MODULE_LOADED        (const xmlChar *)"module-loaded"
#define XP_FN_FEATURE_ENABLED      (const xmlChar *)"feature-enabled"


/* XPath NodeType values */
#define XP_NT_COMMENT              (const xmlChar *)"comment"
#define XP_NT_TEXT                 (const xmlChar *)"text"
#define XP_NT_PROCESSING_INSTRUCTION \
    (const xmlChar *)"processing-instruction"
#define XP_NT_NODE                 (const xmlChar *)"node"

/* XPath 1.0 operator names */
#define XP_OP_AND                  (const xmlChar *)"and"
#define XP_OP_OR                   (const xmlChar *)"or"
#define XP_OP_DIV                  (const xmlChar *)"div"
#define XP_OP_MOD                  (const xmlChar *)"mod"


/* Special URL to XPath translation */
#define XP_URL_ESC_WILDCARD     '-'

/* XPath control block flag definitions */

/* If dynnode is present, then at least one component
 * within the entire XPath expression is variable.
 * (e.g  ../parent == 'fred'  or $foo + 1
 *
 * If not set, then the entire expression is constant
 * (e.g.,  34 mod 2 or 48 and 'fred')
 */
#define XP_FL_DYNNODE            bit0


/* during XPath evaluation, skipping the rest of a
 * FALSE AND expression
 */
#define XP_FL_SKIP_FAND          bit1


/* during XPath evaluation, skipping the rest of a
 * TRUE OR expression
 */
#define XP_FL_SKIP_TOR           bit2


/* used by xpath_leafref.c to keep track of path type */
#define XP_FL_ABSPATH            bit3


/* used for YANG/NETCONF to auto-filter any non-config nodes
 * that are matched by an XPath wildcard mechanism
 */
#define XP_FL_CONFIGONLY         bit4

/* used to indicate the top object node is set
 * FALSE to indicate that all the ncx_module_t datadefQs
 * need to be searched instead
 */
#define XP_FL_USEROOT             bit5

/* used to restrict the XPath expression to the YANG
 * instance-identifier syntax
 */
#define XP_FL_INSTANCEID          bit6


/* used to restrict the XPath expression to an 
 * ncx:schema-instance string syntax
 */
#define XP_FL_SCHEMA_INSTANCEID          bit7


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* XPath expression result type */
typedef enum xpath_restype_t_ {
    XP_RT_NONE,
    XP_RT_NODESET,
    XP_RT_NUMBER,
    XP_RT_STRING,
    XP_RT_BOOLEAN
} xpath_restype_t;

/* XPath dynamic parsing mode for leafref */
typedef enum xpath_curmode_t_ {
    XP_CM_NONE,
    XP_CM_TARGET,
    XP_CM_ALT,
    XP_CM_KEYVAR
} xpath_curmode_t;


/* document root type */
typedef enum xpath_document_t_ {
    XP_DOC_NONE,
    XP_DOC_DATABASE,
    XP_DOC_RPC,
    XP_DOC_RPC_REPLY,
    XP_DOC_NOTIFICATION
} xpath_document_t;


/* XPath expression source type */
typedef enum xpath_source_t_ {
    XP_SRC_NONE,
    XP_SRC_LEAFREF,
    XP_SRC_YANG,
    XP_SRC_INSTANCEID,
    XP_SRC_SCHEMA_INSTANCEID,
    XP_SRC_XML
} xpath_source_t;


/* XPath expression operation type */
typedef enum xpath_exop_t_ {
    XP_EXOP_NONE,
    XP_EXOP_AND,         /* keyword 'and' */
    XP_EXOP_OR,          /* keyword 'or' */
    XP_EXOP_EQUAL,       /* equals '=' */
    XP_EXOP_NOTEQUAL,    /* bang equals '!=' */
    XP_EXOP_LT,          /* left angle bracket '<' */
    XP_EXOP_GT,          /* right angle bracket '>' */
    XP_EXOP_LEQUAL,      /* l. angle-equals '<= */
    XP_EXOP_GEQUAL,      /* r. angle-equals '>=' */
    XP_EXOP_ADD,         /* plus sign '+' */
    XP_EXOP_SUBTRACT,    /* minus '-' */
    XP_EXOP_MULTIPLY,    /* asterisk '*' */
    XP_EXOP_DIV,         /* keyword 'div' */
    XP_EXOP_MOD,         /* keyword 'mod' */
    XP_EXOP_NEGATE,      /* unary '-' */
    XP_EXOP_UNION,       /* vert. bar '|' */
    XP_EXOP_FILTER1,     /* fwd slash '/' */
    XP_EXOP_FILTER2      /* double fwd slash (C++ comment) */
} xpath_exop_t;


/* XPath expression node types */
typedef enum xpath_nodetype_t_ {
    XP_EXNT_NONE,
    XP_EXNT_COMMENT,
    XP_EXNT_TEXT,
    XP_EXNT_PROC_INST,
    XP_EXNT_NODE
} xpath_nodetype_t;


/* xpath_getvar_fn_t
 *
 * Callback function for retrieval of a variable binding
 * 
 * INPUTS:
 *   pcb   == XPath parser control block in use
 *   varname == variable name requested
 *   res == address of return status
 *
 * OUTPUTS:
 *  *res == return status
 *
 * RETURNS:
 *    pointer to the ncx_var_t data structure
 *    for the specified varbind (malloced and filled in)
 */
typedef ncx_var_t *
    (*xpath_getvar_fn_t) (struct xpath_pcb_t_ *pcb,
                          const xmlChar *varname,
                          status_t *res);


/* XPath result node struct */
typedef struct xpath_resnode_t_ {
    dlq_hdr_t             qhdr;
    boolean               dblslash;
    int64                 position;
    int64                 last;   /* only set in context node */
    union node_ {
	obj_template_t *objptr;
	val_value_t    *valptr;
    } node;
} xpath_resnode_t;


/* XPath expression result */
typedef struct xpath_result_t_ {
    dlq_hdr_t            qhdr;        /* in case saved in a Q */
    xpath_restype_t      restype;
    boolean              isval;   /* matters if XP_RT_NODESET */
    int64                last;    /* used with XP_RT_NODESET */
    union r_ {
	dlq_hdr_t         nodeQ;       /* Q of xpath_resnode_t */
	boolean           boo; 
	ncx_num_t         num;
	xmlChar          *str;
    } r;

    status_t             res;
} xpath_result_t;


/* XPath parser control block */
typedef struct xpath_pcb_t_ {
    dlq_hdr_t            qhdr;           /* in case saved in a Q */
    tk_chain_t          *tkc;               /* chain for exprstr */
    xmlChar             *exprstr;           /* YANG XPath string */
    xmlTextReaderPtr     reader;            /* get NS inside XML */

    /* the prefixes in the QNames in the exprstr MUST be resolved
     * in different contexts.  
     *
     * For must/when/leafref XPath, the prefix is a module prefix
     * which must match an import statement in the 'mod' import Q
     *
     * For XML context (NETCONF PDU 'select' attribute)
     * the prefix is part of an extended name, representing
     * XML namespace for the module that defines that node
     */
    xpath_source_t       source;
    ncx_errinfo_t        errinfo;            /* must error extras */
    boolean              logerrors;     /* T: use log_error F: agt */
    boolean              missing_errors; /* T: missing node is error */

    /* these parms are used to parse leafref path-arg 
     * limited object tree syntax allowed only
     */
    obj_template_t  *targobj;       /* bptr to result object */
    obj_template_t  *altobj;     /* bptr to pred. RHS object */
    obj_template_t  *varobj;  /* bptr to key-expr LHS object */
    xpath_curmode_t        curmode;     /* select targ/alt/var obj */

    /* these parms are used by leafref and XPath1 parsing */
    obj_template_t    *obj;            /* bptr to start object */
    ncx_module_t      *objmod;        /* module containing obj */
    obj_template_t    *docroot;        /* bptr to <config> obj */
    val_value_t       *val;                  /* current() node */
    val_value_t       *val_docroot;        /* cfg->root for db */
    xpath_document_t   doctype;

    /* these parms are used for XPath1 processing
     * against a target database 
     */
    uint32              flags;
    xpath_result_t     *result;

    /* additive XPath1 context back- pointer to current 
     * step results; initially NULL and modified until
     * the expression is done
     */
    xpath_resnode_t      context;      /* relative context */
    xpath_resnode_t      orig_context;  /* for current() fn */

    /* The getvar_fn callback function may be set to allow
     * user variables to be supported in this XPath expression
     */
    xpath_getvar_fn_t    getvar_fn;
    void                *cookie;

    /* The varbindQ may be used instead of the getvar_fn
     * to store user variables to be supported in 
     * this XPath expression
     */
    dlq_hdr_t            varbindQ;

    /* The function Q is a copy of the global Q
     * It is not hardwired in case app-specific extensions
     * are added later -- array of xpath_fncb_t
     */
    const struct xpath_fncb_t_ *functions; 

    /* Performance Caches
     * The xpath_result_t and xpath_resnode_t structs
     * are used in many intermediate operations
     *
     * These Qs are used to cache these structs
     * instead of calling malloc and free constantly
     *
     * The XPATH_RESULT_CACHE_MAX and XPATH_RESNODE_CACHE_MAX
     * constants are used to control the max cache sizes
     * This is not user-configurable (TBD).
     */
    dlq_hdr_t           result_cacheQ;  /* Q of xpath_result_t */
    dlq_hdr_t           resnode_cacheQ;  /* Q of xpath_resnode_t */
    uint32              result_count;
    uint32              resnode_count;


    /* first and second pass parsing results
     * the next phase will not execute until
     * all previous phases have a NO_ERR status
     */
    status_t             parseres;
    status_t             validateres;
    status_t             valueres;

    /* saved error info for the agent to process */
    ncx_error_t          tkerr;
    boolean              seen;      /* yangdiff support */
} xpath_pcb_t;


/* XPath function prototype */
typedef xpath_result_t *
    (*xpath_fn_t) (xpath_pcb_t *pcb,
		   dlq_hdr_t *parmQ,   /* Q of xpath_result_t */
		   status_t *res);


/* XPath function control block */
typedef struct xpath_fncb_t_ {
    const xmlChar     *name;
    xpath_restype_t    restype;
    int32              parmcnt;   /* -1 == N, 0..N == actual cnt */
    xpath_fn_t         fn;
} xpath_fncb_t;


/* Value or object node walker fn callback parameters */
typedef struct xpath_walkerparms_t_ {
    dlq_hdr_t         *resnodeQ;
    int64              callcount;
    status_t           res;
} xpath_walkerparms_t;


/* Value node compare walker fn callback parameters */
typedef struct xpath_compwalkerparms_t_ {
    xpath_result_t    *result2;
    xmlChar           *cmpstring;
    ncx_num_t         *cmpnum;
    xmlChar           *buffer;
    uint32             buffsize;
    xpath_exop_t       exop;
    boolean            cmpresult;
    status_t           res;
} xpath_compwalkerparms_t;


/* Value node stringify walker fn callback parameters */
typedef struct xpath_stringwalkerparms_t_ {
    xmlChar           *buffer;
    uint32             buffsize;
    uint32             buffpos;
    status_t           res;
} xpath_stringwalkerparms_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xpath_find_schema_target
* 
* find target, save in *targobj
* Follow the absolute-path or descendant-node path expression
* and return the obj_template_t that it indicates, and the
* que that the object is in
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block to use
*    tkc == token chain in progress  (may be NULL: errmsg only)
*    mod == module in progress
*    obj == augment object initiating search, NULL to start at top
*    datadefQ == Q of obj_template_t containing 'obj'
*    target == Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*    targQ == address of return target queue (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*      *targQ == datadefQ Q header which contains targobj
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_find_schema_target (yang_pcb_t *pcb,
                              tk_chain_t *tkc,
			      ncx_module_t *mod,
			      obj_template_t *obj,
			      dlq_hdr_t  *datadefQ,
			      const xmlChar *target,
			      obj_template_t **targobj,
			      dlq_hdr_t **targQ);


/********************************************************************
* FUNCTION xpath_find_schema_target_err
* 
* find target, save in *targobj, use the errtk if error
* Same as xpath_find_schema_target except a token struct
* is provided to use for the error token, instead of 'obj'
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    tkc == token chain in progress (may be NULL: errmsg only)
*    mod == module in progress
*    obj == augment object initiating search, NULL to start at top
*    datadefQ == Q of obj_template_t containing 'obj'
*    target == Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*    targQ == address of return target queue (may be NULL)
*    tkerr == error struct to use if any messages generated
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*      *targQ == datadefQ header for targobj
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_find_schema_target_err (yang_pcb_t *pcb,
                                  tk_chain_t *tkc,
				  ncx_module_t *mod,
				  obj_template_t *obj,
				  dlq_hdr_t  *datadefQ,
				  const xmlChar *target,
				  obj_template_t **targobj,
				  dlq_hdr_t **targQ,
				  ncx_error_t *tkerr);


/********************************************************************
* FUNCTION xpath_find_schema_target_int
* 
* internal find target, without any error reporting
* Follow the absolute-path expression
* and return the obj_template_t that it indicates
*
* Internal access version
* Error messages are not printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    target == absolute Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_find_schema_target_int (const xmlChar *target,
				  obj_template_t **targobj);


/********************************************************************
* FUNCTION xpath_find_val_target
* 
* used by cfg.c to find parms in the value struct for
* a config file (ncx:cli)
*
* Follow the absolute-path Xpath expression as used
* internally to identify a config DB node
* and return the val_value_t that it indicates
*
* Expression must be the node-path from root for
* the desired node.
*
* Error messages are logged by this function
*
* INPUTS:
*    startval == top-level start element to search
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == Xpath expression string to evaluate
*    targval == address of return value  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs and value node found:
*      *targval == target value node
*   If non-NULL targval and error exit:
*      *targval == last good node visited in expression (if any)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_find_val_target (val_value_t *startval,
			   ncx_module_t *mod,
			   const xmlChar *target,
			   val_value_t **targval);


/********************************************************************
* FUNCTION xpath_find_val_unique
* 
* called by server to find a descendant value node
* based  on a relative-path sub-clause of a unique-stmt
*
* Follow the relative-path Xpath expression as used
* internally to identify a config DB node
* and return the val_value_t that it indicates
*
* Error messages are logged by this function
* only if logerrors is TRUE
*
* INPUTS:
*    startval == starting context node (contains unique-stmt)
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == Xpath expression string to evaluate
*    root = XPath docroot to use
*    logerrors == TRUE to use log_error, FALSE to skip it
*    retpcb == address of return value
*
* OUTPUTS:
*   if value node found:
*      *retpcb == malloced XPath PCB with result
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_find_val_unique (val_value_t *startval,
                           ncx_module_t *mod,
                           const xmlChar *target,
                           val_value_t *root,
                           boolean logerrors,
                           xpath_pcb_t **retpcb);


/********************************************************************
* FUNCTION xpath_new_pcb
* 
* malloc a new XPath parser control block
* xpathstr is allowed to be NULL, otherwise
* a strdup will be made and exprstr will be set
*
* Create and initialize an XPath parser control block
*
* INPUTS:
*   xpathstr == XPath expression string to save (a copy will be made)
*            == NULL if this step should be skipped
*   getvar_fn == callback function to retirieve an XPath
*                variable binding
*                NULL if no variables are used
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
extern xpath_pcb_t *
    xpath_new_pcb (const xmlChar *xpathstr,
                   xpath_getvar_fn_t  getvar_fn);


/********************************************************************
* FUNCTION xpath_new_pcb_ex
* 
* malloc a new XPath parser control block
* xpathstr is allowed to be NULL, otherwise
* a strdup will be made and exprstr will be set
*
* Create and initialize an XPath parser control block
*
* INPUTS:
*   xpathstr == XPath expression string to save (a copy will be made)
*            == NULL if this step should be skipped
*   getvar_fn == callback function to retirieve an XPath
*                variable binding
*                NULL if no variables are used
*   cookie == runstack context pointer to use, cast as a cookie
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
extern xpath_pcb_t *
    xpath_new_pcb_ex (const xmlChar *xpathstr,
                      xpath_getvar_fn_t  getvar_fn,
                      void *cookie);


/********************************************************************
* FUNCTION xpath_clone_pcb
* 
* Clone an XPath PCB for a  must clause copy
* copy from typdef to object for leafref
* of object to value for NETCONF PDU processing
*
* INPUTS:
*    srcpcb == struct with starting contents
*
* RETURNS:
*   new xpatyh_pcb_t clone of the srcmust, NULL if malloc error
*   It will not be processed or parsed.  Only the starter
*   data will be set
*********************************************************************/
extern xpath_pcb_t *
    xpath_clone_pcb (const xpath_pcb_t *srcpcb);


/********************************************************************
* FUNCTION xpath_find_pcb
* 
* Find an XPath PCB
* find by exact match of the expressions string
*
* INPUTS:
*    pcbQ == Q of xpath_pcb_t structs to check
*    exprstr == XPath expression string to find
*
* RETURNS:
*   pointer to found xpath_pcb_t or NULL if not found
*********************************************************************/
extern xpath_pcb_t *
    xpath_find_pcb (dlq_hdr_t *pcbQ, 
		    const xmlChar *exprstr);


/********************************************************************
* FUNCTION xpath_free_pcb
* 
* Free a malloced XPath parser control block
*
* INPUTS:
*   pcb == pointer to parser control block to free
*********************************************************************/
extern void
    xpath_free_pcb (xpath_pcb_t *pcb);


/********************************************************************
* FUNCTION xpath_new_result
* 
* malloc an XPath result
* Create and initialize an XPath result struct
*
* INPUTS:
*   restype == the desired result type
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
extern xpath_result_t *
    xpath_new_result (xpath_restype_t restype);


/********************************************************************
* FUNCTION xpath_init_result
* 
* Initialize an XPath result struct
* malloc an XPath result node
*
* INPUTS:
*   result == pointer to result struct to initialize
*   restype == the desired result type
*********************************************************************/
extern void 
    xpath_init_result (xpath_result_t *result,
		       xpath_restype_t restype);


/********************************************************************
* FUNCTION xpath_free_result
* 
* Free a malloced XPath result struct
*
* INPUTS:
*   result == pointer to result struct to free
*********************************************************************/
extern void
    xpath_free_result (xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_clean_result
* 
* Clean an XPath result struct
*
* INPUTS:
*   result == pointer to result struct to clean
*********************************************************************/
extern void
    xpath_clean_result (xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_new_resnode
* 
* Create and initialize an XPath result node struct
*
* INPUTS:
*   restype == the desired result type
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
extern xpath_resnode_t *
    xpath_new_resnode (void);


/********************************************************************
* FUNCTION xpath_init_resnode
* 
* Initialize an XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to initialize
*********************************************************************/
extern void 
    xpath_init_resnode (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_free_resnode
* 
* Free a malloced XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to free
*********************************************************************/
extern void
    xpath_free_resnode (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_delete_resnode
* 
* Delete and free a malloced XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to free
*********************************************************************/
extern void
    xpath_delete_resnode (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_clean_resnode
* 
* Clean an XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to clean
*********************************************************************/
extern void
    xpath_clean_resnode (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_get_curmod_from_prefix
* 
* Get the correct module to use for a given prefix
*
* INPUTS:
*    prefix == string to check
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    targmod == address of return module
*
* OUTPUTS:
*    *targmod == target moduke to use
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_get_curmod_from_prefix (const xmlChar *prefix,
				  ncx_module_t *mod,
				  ncx_module_t **targmod);


/********************************************************************
* FUNCTION xpath_get_curmod_from_prefix_str
* 
* Get the correct module to use for a given prefix
* Unended string version
*
* INPUTS:
*    prefix == string to check
*    prefixlen == length of prefix
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    targmod == address of return module
*
* OUTPUTS:
*    *targmod == target moduke to use
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_get_curmod_from_prefix_str (const xmlChar *prefix,
				      uint32 prefixlen,
				      ncx_module_t *mod,
				      ncx_module_t **targmod);


/********************************************************************
* FUNCTION xpath_parse_token
* 
* Parse the XPath token sequence for a specific token type
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    tktyp == expected token type
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xpath_parse_token (xpath_pcb_t *pcb,
		       tk_type_t  tktype);


/********************************************************************
* FUNCTION xpath_cvt_boolean
* 
* Convert an XPath result to a boolean answer
*
* INPUTS:
*    result == result struct to convert to boolean
*
* RETURNS:
*   TRUE or FALSE depending on conversion
*********************************************************************/
extern boolean
    xpath_cvt_boolean (const xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_cvt_number
* 
* Convert an XPath result to a number answer
*
* INPUTS:
*    result == result struct to convert to a number
*    num == pointer to ncx_num_t to hold the conversion result
*
* OUTPUTS:
*   *num == numeric result from conversion
*
*********************************************************************/
extern void
    xpath_cvt_number (const xpath_result_t *result,
		      ncx_num_t *num);


/********************************************************************
* FUNCTION xpath_cvt_string
* 
* Convert an XPath result to a string answer
*
* INPUTS:
*    pcb == parser control block to use
*    result == result struct to convert to a number
*    str == pointer to xmlChar * to hold the conversion result
*
* OUTPUTS:
*   *str == pointer to malloced string from conversion
*
* RETURNS:
*   status; could get an ERR_INTERNAL_MEM error or NO_RER
*********************************************************************/
extern status_t
    xpath_cvt_string (xpath_pcb_t *pcb,
		      const xpath_result_t *result,
		      xmlChar **str);


/********************************************************************
* FUNCTION xpath_get_resnodeQ
* 
* Get the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnodeQ or NULL if some error
*********************************************************************/
extern dlq_hdr_t *
    xpath_get_resnodeQ (xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_get_first_resnode
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
extern xpath_resnode_t *
    xpath_get_first_resnode (xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_get_next_resnode
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
extern xpath_resnode_t *
    xpath_get_next_resnode (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_get_resnode_valptr
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
extern val_value_t *
    xpath_get_resnode_valptr (xpath_resnode_t *resnode);


/********************************************************************
* FUNCTION xpath_get_varbindQ
* 
* Get the varbindQ from a parser control block struct
*
* INPUTS:
*    pcb == parser control block to use
*
* RETURNS:
*   pointer to varbindQ or NULL if some error
*********************************************************************/
extern dlq_hdr_t *
    xpath_get_varbindQ (xpath_pcb_t *pcb);


/********************************************************************
* FUNCTION xpath_move_nodeset
* 
* Move the nodes from a nodeset reult into the
* target nodeset result.
* This is needed to support partial lock
* because multiple select expressions are allowed
* for the same partial lock
*
* INPUTS:
*    srcresult == XPath result nodeset source
*    destresult == XPath result nodeset target
*
* OUTPUTS:
*    srcresult nodes will be moved to the target result
*********************************************************************/
extern void
    xpath_move_nodeset (xpath_result_t *srcresult,
                        xpath_result_t *destresult);



/********************************************************************
* FUNCTION xpath_nodeset_empty
* 
* Check if the result is an empty nodeset
*
* INPUTS:
*    result == XPath result to check
*
* RETURNS:
*    TRUE if this is an empty nodeset
*    FALSE if not empty or not a nodeset
*********************************************************************/
extern boolean
    xpath_nodeset_empty (const xpath_result_t *result);


/********************************************************************
* FUNCTION xpath_nodeset_swap_valptr
* 
* Check if the result has the oldval ptr and if so,
* replace it with the newval ptr
*
* INPUTS:
*    result == result struct to check
*    oldval == value ptr to find
*    newval == new value replace it with if oldval is found
*
*********************************************************************/
extern void
    xpath_nodeset_swap_valptr (xpath_result_t *result,
                               val_value_t *oldval,
                               val_value_t *newval);


/********************************************************************
* FUNCTION xpath_nodeset_delete_valptr
* 
* Check if the result has the oldval ptr and if so,
* delete it
*
* INPUTS:
*    result == result struct to check
*    oldval == value ptr to find
*
*********************************************************************/
extern void
    xpath_nodeset_delete_valptr (xpath_result_t *result,
                                 val_value_t *oldval);


/********************************************************************
* FUNCTION xpath_convert_url_to_path
* 
* Convert a URL format path to XPath format path
*
* INPUTS:
*    urlpath == URL path string to convert to XPath
*    match_names == enum for selected match names mode
*    alt_naming == TRUE if alt-name and cli-drop-node-name
*      containers should be checked
*    wildcards == TRUE if wildcards allowed instead of key values
*                 FALSE if the '-' wildcard mechanism not allowed
*    withkeys == TRUE if keys are expected
*                FALSE if just nodes are expected
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced string containing XPath expression from conversion
*   NULL if some error
*********************************************************************/
extern xmlChar *
    xpath_convert_url_to_path (const xmlChar *urlpath,
                               ncx_name_match_t match_names,
                               boolean alt_naming,
                               boolean wildcards,
                               boolean withkeys,
                               status_t *res);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xpath */
