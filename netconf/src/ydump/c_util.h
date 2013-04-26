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
#ifndef _H_c_util
#define _H_c_util

/*  FILE: c_util.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  YANGDUMP C code generation utilities
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
24-oct-09    abb      Begun

*/

#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_ses
#include "ses.h"
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

#define START_COMMENT (const xmlChar *)"\n/* "
#define END_COMMENT   (const xmlChar *)" */"

#define START_BLOCK  (const xmlChar *)" {"
#define END_BLOCK    (const xmlChar *)"\n}"

#define BAR_H         (const xmlChar *)"_H_"
#define DOT_H         (const xmlChar *)".h"
#define BAR_FEAT      (const xmlChar *)"F"
#define BAR_ID        (const xmlChar *)"I"
#define BAR_NODE      (const xmlChar *)"N"
#define BAR_MOD       (const xmlChar *)"M"
#define BAR_REV       (const xmlChar *)"R"
#define DEF_TYPE      (const xmlChar *)"T"


#define BAR_CONST     (const xmlChar *)"(const xmlChar *)"


#define POUND_DEFINE  (const xmlChar *)"\n#define "
#define POUND_ENDIF   (const xmlChar *)"\n#endif"
#define POUND_IF      (const xmlChar *)"\n#if "
#define POUND_IFDEF   (const xmlChar *)"\n#ifdef "
#define POUND_IFNDEF  (const xmlChar *)"\n#ifndef "
#define POUND_INCLUDE (const xmlChar *)"\n#include "
#define POUND_ELSE    (const xmlChar *)"\n#else"

#define START_DEFINED (const xmlChar *)"defined("
#define START_TYPEDEF (const xmlChar *)"\ntypedef "


#define INT8          (const xmlChar *)"int8"
#define INT16         (const xmlChar *)"int16"
#define INT32         (const xmlChar *)"int32"
#define INT64         (const xmlChar *)"int64"

#define UINT8         (const xmlChar *)"uint8"
#define UINT16        (const xmlChar *)"uint16"
#define UINT32        (const xmlChar *)"uint32"
#define UINT64        (const xmlChar *)"uint64"

#define STRING        (const xmlChar *)"xmlChar *"

#define IDREF         (const xmlChar *)"val_idref_t *"

#define BOOLEAN       (const xmlChar *)"boolean"
#define FLOAT         (const xmlChar *)"float"
#define DOUBLE        (const xmlChar *)"double"
#define BITS          (const xmlChar *)"ncx_list_t"

#define STRUCT        (const xmlChar *)"struct"
#define UNION         (const xmlChar *)"union"

#define QHEADER       (const xmlChar *)"dlq_hdr_t qhdr;"

#define QUEUE         (const xmlChar *)"dlq_hdr_t"

#define START_LINE    (const xmlChar *)"\n    "

#define PARM_DSTVAL   (const xmlChar *)"dstval"

#define PARM_ERRORVAL (const xmlChar *)"errorval"

#define FN_BANNER_START (const xmlChar *)\
    "\n/*******************************************************"\
    "*************\n* FUNCTION "

#define FN_BANNER_LN (const xmlChar *)"\n* "

#define FN_BANNER_INPUT (const xmlChar *)"\n* INPUTS:\n* "

#define FN_BANNER_RETURN (const xmlChar *)"\n* RETURNS:\n* "

#define FN_BANNER_RETURN_STATUS (const xmlChar *)\
    "\n* RETURNS:\n*     error status"

#define FN_BANNER_END (const xmlChar *)\
    "\n********************************************************************/"


#define FN_INIT_STATIC_VARS (const xmlChar *)"init_static_vars"

#define FN_EDIT (const xmlChar *)"edit"

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

typedef enum c_mode_t_ {
    C_MODE_NONE,
    C_MODE_OID,
    C_MODE_TYPEDEF,
    C_MODE_CALLBACK,
    C_MODE_VARNAME
} c_mode_t;

/* ID to name string binding for #define statements */
typedef struct c_define_t_ {
    dlq_hdr_t        qhdr;
    xmlChar         *idstr;
    xmlChar         *typstr;  /* when c_def for object */
    xmlChar         *varstr;  /* when c_def for object */
    xmlChar         *valstr;
    obj_template_t  *obj;   /* back-ptr to object for typdef */
} c_define_t;


/********************************************************************
*                                                                   *
*                         F U N C T I O N S                         *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION need_rpc_includes
* 
* Check if the include-stmts for RPC methods are needed
*
* INPUTS:
*   mod == module in progress
*   cp == conversion parameters to use
*
* RETURNS:
*  TRUE if RPCs found
*  FALSE if no RPCs found
*********************************************************************/
extern boolean
    need_rpc_includes (const ncx_module_t *mod,
                       const yangdump_cvtparms_t *cp);


/********************************************************************
* FUNCTION need_notif_includes
* 
* Check if the include-stmts for notifications are needed
*
* INPUTS:
*   mod == module in progress
*   cp == conversion parameters to use
*
* RETURNS:
*   TRUE if notifcations found
*   FALSE if no notifications found
*********************************************************************/
extern boolean
    need_notif_includes (const ncx_module_t *mod,
                         const yangdump_cvtparms_t *cp);


/********************************************************************
* FUNCTION write_c_safe_str
* 
* Generate a string token at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   strval == string value
*********************************************************************/
extern void
    write_c_safe_str (ses_cb_t *scb,
                      const xmlChar *strval);


/********************************************************************
* FUNCTION write_c_str
* 
* Generate a string token at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   strval == string value
*   quotes == quotes style (0, 1, 2)
*********************************************************************/
extern void
    write_c_str (ses_cb_t *scb,
                 const xmlChar *strval,
                 uint32 quotes);


/********************************************************************
* FUNCTION write_c_simple_str
* 
* Generate a simple clause on 1 line
*
* INPUTS:
*   scb == session control block to use for writing
*   kwname == keyword name
*   strval == string value
*   indent == indent count to use
*   quotes == quotes style (0, 1, 2)
*********************************************************************/
extern void
    write_c_simple_str (ses_cb_t *scb,
                        const xmlChar *kwname,
                        const xmlChar *strval,
                        int32 indent,
                        uint32 quotes);


/********************************************************************
*
* FUNCTION write_identifier
* 
* Generate an identifier
*
*  #module_DEFTYPE_idname
*
* INPUTS:
*   scb == session control block to use for writing
*   modname == module name start-string to use
*   defpart == internal string for deftype part (may be NULL)
*   idname == identifier name
*   isuser == TRUE if USER SIL file
*             FALSE if YUMA SIL file
*********************************************************************/
extern void
    write_identifier (ses_cb_t *scb,
                      const xmlChar *modname,
                      const xmlChar *defpart,
                      const xmlChar *idname,
                      boolean isuser);


/********************************************************************
* FUNCTION write_ext_include
* 
* Generate an include statement for an external file
*
*  #include <foo,h>
*
* INPUTS:
*   scb == session control block to use for writing
*   hfile == H file name == file name to include (foo.h)
*
*********************************************************************/
extern void
    write_ext_include (ses_cb_t *scb,
                       const xmlChar *hfile);


/********************************************************************
* FUNCTION write_ncx_include
* 
* Generate an include statement for an NCX file
*
*  #ifndef _H_foo
*  #include "foo,h"
*
* INPUTS:
*   scb == session control block to use for writing
*   modname == module name to include (foo)
*
*********************************************************************/
extern void
    write_ncx_include (ses_cb_t *scb,
                       const xmlChar *modname);


/********************************************************************
* FUNCTION write_cvt_include
* 
* Generate an include statement for an NCX split SIL file
* based on the format type
*
* INPUTS:
*   scb == session control block to use for writing
*   modname == module name to include (foo)
*   cvttyp == format enum to use
*********************************************************************/
extern void
    write_cvt_include (ses_cb_t *scb,
                       const xmlChar *modname,
                       ncx_cvttyp_t cvttyp);


/********************************************************************
* FUNCTION write_qheader
* 
* Generate a QHEADER with indentation
*
*  \n{indentcnt}QHEADER
*
* INPUTS:
*   scb == session control block to use for writing
*
*********************************************************************/
extern void
    write_qheader (ses_cb_t *scb);


/********************************************************************
* FUNCTION save_oid_cdefine
* 
* Generate a #define binding for a definition and save it in the
* specified Q of c_define_t structs
*
* INPUTS:
*   cdefineQ == Q of c_define_t structs to use
*   modname == module name to use
*   defname == object definition name to use
*
* OUTPUTS:
*   a new c_define_t is allocated and added to the cdefineQ
*   if returning NO_ERR;
*
* RETURNS:
*   status; duplicate C identifiers not supported yet
*      foo-1 -->  foo_1
*      foo.1 -->  foo_1
*   An error message will be generated if this type of error occurs
*********************************************************************/
extern status_t
    save_oid_cdefine (dlq_hdr_t *cdefineQ,
                      const xmlChar *modname,
                      const xmlChar *defname);


/********************************************************************
* FUNCTION save_path_cdefine
* 
* Generate a #define binding for a definition and save it in the
* specified Q of c_define_t structs
*
* INPUTS:
*   cdefineQ == Q of c_define_t structs to use
*   modname == base module name to use
*   obj == object struct to use to generate path
*   cmode == mode to use
*
* OUTPUTS:
*   a new c_define_t is allocated and added to the cdefineQ
*   if returning NO_ERR;
*
* RETURNS:
*   status; duplicate C identifiers not supported yet
*      foo-1/a/b -->  foo_1_a_b
*      foo.1/a.2 -->  foo_1_a_b
*   An error message will be generated if this type of error occurs
*********************************************************************/
extern status_t
    save_path_cdefine (dlq_hdr_t *cdefineQ,
                       const xmlChar *modname,
                       obj_template_t *obj,
                       c_mode_t cmode);


/********************************************************************
* FUNCTION find_path_cdefine
* 
* Find a #define binding for a definition in the
* specified Q of c_define_t structs
*
* INPUTS:
*   cdefineQ == Q of c_define_t structs to use
*   obj == object struct to find
*
* RETURNS:
*   pointer to found entry
*   NULL if not found
*********************************************************************/
extern c_define_t *
    find_path_cdefine (dlq_hdr_t *cdefineQ,
                       const obj_template_t *obj);


/********************************************************************
* FUNCTION clean_cdefineQ
* 
* Clean a Q of c_define_t structs
*
* INPUTS:
*   cdefineQ == Q of c_define_t structs to use
*********************************************************************/
extern void
    clean_cdefineQ (dlq_hdr_t *cdefineQ);



/********************************************************************
* FUNCTION write_c_header
* 
* Write the C file header
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
extern void
    write_c_header (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp);


/********************************************************************
* FUNCTION write_c_footer
* 
* Write the C file footer
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*********************************************************************/
extern void
    write_c_footer (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp);


/*******************************************************************
* FUNCTION write_c_objtype
* 
* Generate the C data type for the NCX data type
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*
**********************************************************************/
extern void
    write_c_objtype (ses_cb_t *scb,
                     const obj_template_t *obj);




/*******************************************************************
* FUNCTION write_c_objtype_ex
* 
* Generate the C data type for the NCX data type
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*   cdefQ == Q of c_define_t to check for obj
*   endchar == char to use at end (semi-colon, comma, right-paren)
*   isconst == TRUE if a const pointer is needed
*              FALSE if pointers should not be 'const'
*   needstar == TRUE if this object reference is a reference
*               or a pointer to the data type
*               FALSE if this is a direct usage of the object data type
*               !! only affects complex types, not simple types
**********************************************************************/
extern void
    write_c_objtype_ex (ses_cb_t *scb,
                        const obj_template_t *obj,
                        dlq_hdr_t  *cdefQ,
                        xmlChar endchar,
                        boolean isconst,
                        boolean needstar);


/*******************************************************************
* FUNCTION write_c_objtype_max
* 
* Generate the C data type for the NCX data type
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*   cdefQ == Q of c_define_t to check for obj
*   endchar == char to use at end (semi-colon, comma, right-paren)
*   isconst == TRUE if a const pointer is needed
*              FALSE if pointers should not be 'const'
*   needstar == TRUE if this object reference is a reference
*               or a pointer to the data type
*               FALSE if this is a direct usage of the object data type
*               !! only affects complex types, not simple types
*   usename == TRUE to use object name as the variable name
*              FALSE to use the idstr as the variable name
*   useprefix == TRUE to use object name as the variable name
*              FALSE to use the idstr as the variable name;
*              ignored if usename == TRUE
*   isuser == TRUE if format is NCX_CVTTYP_UC or NCX_CVTTYP_UH
*             FALSE otherwise;  ignored if useprefix == FALSE
*   isvar == TRUE if cdef->varstr should be used
*            FALSE if cdef->idstr should be used;
*            ignored if usename == TRUE
**********************************************************************/
extern void
    write_c_objtype_max (ses_cb_t *scb,
                         const obj_template_t *obj,
                         dlq_hdr_t *cdefQ,
                         xmlChar endchar,
                         boolean isconst,
                         boolean needstar,
                         boolean usename,
                         boolean useprefix,
                         boolean isuser,
                         boolean isvar);


/*******************************************************************
* FUNCTION write_c_val_macro_type
* 
* Generate the C VAL_FOO macro name for the data type
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*
**********************************************************************/
extern void
    write_c_val_macro_type (ses_cb_t *scb,
                            const obj_template_t *obj);


/*******************************************************************
* FUNCTION write_c_oid_comment
* 
* Generate the object OID as a comment line
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*
**********************************************************************/
extern void
    write_c_oid_comment (ses_cb_t *scb,
                         const obj_template_t *obj);


/********************************************************************
* FUNCTION save_c_objects
* 
* save the path name bindings for C typdefs
*
* INPUTS:
*   mod == module in progress
*   datadefQ == que of obj_template_t to use
*   savecdefQ == Q of c_define_t structs to use
*   cmode == C code generating mode to use
*
* OUTPUTS:
*   savecdefQ may get new structs added
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    save_c_objects (ncx_module_t *mod,
                    dlq_hdr_t *datadefQ,
                    dlq_hdr_t *savecdefQ,
                    c_mode_t cmode);


/********************************************************************
* FUNCTION save_all_c_objects
* 
* save the path name bindings for C typdefs for mod and all submods
*
* INPUTS:
*   mod == module in progress
*   cp == conversion parameters to use
*   savecdefQ == Q of c_define_t structs to use
*   cmode == C code generating mode to use
*
* OUTPUTS:
*   savecdefQ may get new structs added
*
* RETURNS:
*  status
*********************************************************************/
extern status_t
    save_all_c_objects (ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        dlq_hdr_t *savecdefQ,
                        c_mode_t cmode);


/********************************************************************
* FUNCTION skip_c_top_object
* 
* Check if a top-level object should be skipped
* in C file SIL code generation
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*  TRUE if object should be skipped
*  FALSE if object should not be skipped
*********************************************************************/
extern boolean
    skip_c_top_object (obj_template_t *obj);


/********************************************************************
* FUNCTION write_c_key_params
* 
* Write all the keys in C function aprameter list format
*
* INPUTS:
*   scb == session to use
*   obj == object to start from (ancestor-or-self)
*   objnameQ == Q of name-to-idstr mappings
*   keycount == number of key leafs expected; used to
*               identify last key to suppress ending comma
*   startindent == start indent count
*
*********************************************************************/
extern void
    write_c_key_params(ses_cb_t *scb,
                       obj_template_t *obj, 
                       dlq_hdr_t *objnameQ,
                       uint32 keycount,
                       int32 startindent);


/********************************************************************
* FUNCTION write_c_key_vars
* 
* Write all the local key variables in the SIL C function
*
* INPUTS:
*   scb == session to use
*   obj == object to start from (ancestor-or-self)
*   objnameQ == Q of name-to-idstr mappings
*   parmname == name of parameter used in C code
*   keycount == number of key leafs expected; used to
*               identify last key to suppress ending comma
*   startindent == start indent count
*
*********************************************************************/
extern void
    write_c_key_vars (ses_cb_t *scb, 
                      obj_template_t *obj, 
                      dlq_hdr_t *objnameQ, 
                      const xmlChar *parmname,
                      uint32 keycount,
                      int32 startindent);


/********************************************************************
* FUNCTION write_c_key_values
* 
* Write all the keys in call-C-function-to-get-key-value format
*
* INPUTS:
*   scb == session to use
*   obj == object to start from (ancestor-or-self)
*   objnameQ == Q of name-to-idstr mappings
*   keycount == number of key leafs expected; used to
*               identify last key to suppress ending comma
*   startindent == start indent count
*
*********************************************************************/
extern void
    write_c_key_values (ses_cb_t *scb, 
                        obj_template_t *obj, 
                        dlq_hdr_t *objnameQ, 
                        uint32 keycount,
                        int32 startindent);


/********************************************************************
* FUNCTION write_h_iffeature_start
* 
* Generate the start C for 1 if-feature conditional;
*
* INPUTS:
*   scb == session control block to use for writing
*   iffeatureQ == Q of ncx_feature_t to use
*
*********************************************************************/
extern void
    write_h_iffeature_start (ses_cb_t *scb,
                             const dlq_hdr_t *iffeatureQ);


/********************************************************************
* FUNCTION write_h_iffeature_end
* 
* Generate the end C for 1 if-feature conditiona;
*
* INPUTS:
*   scb == session control block to use for writing
*   iffeatureQ == Q of ncx_feature_t to use
*
*********************************************************************/
extern void
    write_h_iffeature_end (ses_cb_t *scb,
                           const dlq_hdr_t *iffeatureQ);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_c_util */
