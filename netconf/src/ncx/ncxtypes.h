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
#ifndef _H_ncxtypes
#define _H_ncxtypes

/*  FILE: ncxtypes.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Contains NCX typedefs
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-nov-05    abb      Begun; split from ncx.h
04-feb-06    abb      Move base/nc.h constants into this file
10-nov-07    abb      Split out from ncxconst.h
*/

#include <xmlstring.h>
#include <xmlregexp.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/

/* NCX Access Control 'max-access' enumeration values 
 * Note that access control is applied to the session
 * and the max-access applies to the user max-access,
 * not the agent max-access.  Only the agent may create
 * or write an object which is classified as 'read-only'.
 *
 * Access is grouped into the following catagories.
 *    
 *      none
 *      read-only
 *      read-write
 *      read-create
 *
 *   Merge and Replace are handled as follows:
 *
 *   Requested operation              Effective Access
 *   -------------------------------------------------
 *   merge empty with no-cur-node      --> no-op
 *   merge empty with cur-node         --> no-op
 *   merge non-empty with no-cur-node  --> create
 *   merge non-empty with cur-node     --> write
 *
 *   replace empty into no-cur-node      --> no-op
 *   replace empty into cur-node         --> delete
 *   replace non-empty into no-cur-node  --> create
 *   replace non-empty into cur-node     --> write

 *   Create and Delete are grouped together for access control
 *   purposes, and called 'read-create'
 *
 *   Obviously, the max-access of the ancestor nodes affect any 
 *   given nested node, but the enumerations are not purely hierarchical.
 *   
 *   Access      Nested Behavior
 *   -------------------------------
 *   [none]      Check the ancestor chain until an explicit value
 *               is found (or the parmset root is reached, where the
 *               default is read-create)
 *
 *   read-only   User may only read this node and all child nodes.
 *               All nested access is also read-only, overriding any
 *               explicit setting of any child nodes
 *
 *   read-write  User may read and write this node and all child nodes,
 *               depending on the max-access of the child node.
 *               Any type of nested access may appear in any child
 *               nodes.  A read-write container may have child nodes
 *               which are read-create objects.  This implies the agent
 *               must create the object in order for the user to write it.
 *
 *   read-create User may have unrestricted access to this node and all
 *               child nodes, depending on the max-access of the child
 *               nodes. Any type of nested access may appear in any 
 *               child nodes.
 */
typedef enum ncx_access_t_ {
    NCX_ACCESS_NONE,    /* enum not explicitly set */
    NCX_ACCESS_RO,      /* read-only */
    NCX_ACCESS_RW,      /* read-write (create/delete not allowed) */
    NCX_ACCESS_RC       /* read-create (all access) */
} ncx_access_t;


/* default max-access is all */
#define NCX_DEF_ACCESS NCX_ACCESS_RC


/* NCX Persistence Control
 *
 * Enum mapping of 'data-class' field in the 'parm' and 'type' clauses
 *
 * This data type controls NV-storage and whether a node is returned
 * in the 'get-config' operation and stored in the startup config.
 *
 */
typedef enum ncx_data_class_t_ {
    NCX_DC_NONE,
    NCX_DC_CONFIG,        /* persistent config */
    NCX_DC_STATE          /* state or statistics */
} ncx_data_class_t;


/* enumeration of the built-in NCX types 
 * These types cannot be overridden and cannot be imported 
 */
typedef enum ncx_btype_t_ {
    NCX_BT_NONE,
    NCX_BT_ANY,
    NCX_BT_BITS,
    NCX_BT_ENUM,
    NCX_BT_EMPTY,
    NCX_BT_BOOLEAN,
    NCX_BT_INT8,
    NCX_BT_INT16,
    NCX_BT_INT32,
    NCX_BT_INT64,
    NCX_BT_UINT8,
    NCX_BT_UINT16,
    NCX_BT_UINT32,
    NCX_BT_UINT64,   
    NCX_BT_DECIMAL64,
    NCX_BT_FLOAT64,   /* hidden: just for XPath */
    NCX_BT_STRING,
    NCX_BT_BINARY,
    NCX_BT_INSTANCE_ID,
    NCX_BT_UNION,
    NCX_BT_LEAFREF,
    NCX_BT_IDREF,
    NCX_BT_SLIST,                 /* ncx:xsdlist extension */

    NCX_BT_CONTAINER,
    NCX_BT_CHOICE,
    NCX_BT_CASE,                   /* not a real type */
    NCX_BT_LIST,
    NCX_BT_EXTERN,                 /* not a real type */
    NCX_BT_INTERN                  /* not a real type */

} ncx_btype_t;


#define NCX_FIRST_DATATYPE NCX_BT_ANY
#define NCX_LAST_DATATYPE  NCX_BT_LIST
#define NCX_NUM_BASETYPES  (NCX_LAST_DATATYPE-NCX_FIRST_DATATYPE+1)


/* Enumeration of the basic value type classifications */
typedef enum ncx_tclass_t_ {
    NCX_CL_NONE,               
    NCX_CL_BASE,                         /* a built-in base type */
    NCX_CL_SIMPLE,               /* a restriction of a base type */
    NCX_CL_COMPLEX,                            /* a complex type */
    NCX_CL_NAMED,               /* a restriction of a named type */
    NCX_CL_REF             /* internal reference to another type */
} ncx_tclass_t;


/* Enumeration of the different types of index components 
 * YANG ONLY SUPPORTS NCX_IT_LOCAL
 */
typedef enum ncx_indextyp_t_ {
    NCX_IT_NONE,               
    NCX_IT_INLINE,          /* index simple type declared inline */
    NCX_IT_NAMED,            /* index named type declared inline */
    NCX_IT_LOCAL,               /* local member within the table */
    NCX_IT_SLOCAL,       /* scoped local member within the table */
    NCX_IT_REMOTE,                       /* unscoped remote name */
    NCX_IT_SREMOTE                         /* scoped remote name */
} ncx_indextyp_t;


/* NCX Internal Node Types
 * 
 * nodes in the value trees can be different types 
 * These enums are used to generate instance IDs and
 * classify data passed to the agt_record_*error functions
 */
typedef enum ncx_node_t_ {
    NCX_NT_NONE,
    NCX_NT_TYP,                                /* typ_template_t */
    NCX_NT_GRP,                                /* grp_template_t */
    NCX_NT_VAL,                                   /* val_value_t */
    NCX_NT_OBJ,                                /* obj_template_t */
    NCX_NT_ERRINFO,                 /* ncx_errinfo_t, error only */
    NCX_NT_STRING,                      /* xmlChar *, error only */
    NCX_NT_CFG,                  /* cfg_template_t *, error only */
    NCX_NT_INDEX,                     /* obj_key_t *, error only */
    NCX_NT_QNAME,                 /* xmlns_qname_t *, error only */
    NCX_NT_UINT32_PTR,                 /* session ID, error only */
    NCX_NT_IMPORT_CB,                            /* ncx_import_t */
    NCX_NT_INCLUDE_CB,                          /* ncx_include_t */
    NCX_NT_REVISION_CB,                         /* ncx_revhist_t */
    NCX_NT_EXTENSION_CB,                       /* ext_template_t */
    NCX_NT_FEATURE_CB,                          /* ncx_feature_t */
    NCX_NT_IDENTITY_CB,                        /* ncx_identity_t */
    NCX_NT_TYPDEF_CB                                /* typ_def_t */
} ncx_node_t;


/* The instance qualifier types are borrowed from ABNF and RelaxNG */
typedef enum ncx_iqual_t_ {
    NCX_IQUAL_NONE,                         /* value not set */
    NCX_IQUAL_ONE,                          /* no iqual == 1 */
    NCX_IQUAL_OPT,                          /* '?' == 0 or 1 */
    NCX_IQUAL_1MORE,                     /* '+' == 1 or more */
    NCX_IQUAL_ZMORE                      /* '*' == 0 or more */
}  ncx_iqual_t;

/* The merge type for the NETCONF merge operation */
typedef enum ncx_merge_t_ {
    NCX_MERGE_NONE,                         /* value not set */
    NCX_MERGE_FIRST,
    NCX_MERGE_LAST,
    NCX_MERGE_SORT
}  ncx_merge_t;


/* typdef search qualifier list */
typedef enum ncx_squal_t_ {
    NCX_SQUAL_NONE,
    NCX_SQUAL_RANGE,
    NCX_SQUAL_VAL,
    NCX_SQUAL_META,
    NCX_SQUAL_APPINFO
}  ncx_squal_t;


/* Enumeration of string restriction types */
typedef enum ncx_strrest_t_ {
    NCX_SR_NONE,
    NCX_SR_PATTERN,
    NCX_SR_ENUM,
    NCX_SR_BIT
}  ncx_strrest_t;


/* Enumeration of number format types */
typedef enum ncx_numfmt_t_ {
    NCX_NF_NONE,
    NCX_NF_OCTAL,
    NCX_NF_DEC,  
    NCX_NF_HEX, 
    NCX_NF_REAL
}  ncx_numfmt_t;


/* Enumeration of NETCONF protocol layers */
typedef enum ncx_layer_t_ {
    NCX_LAYER_NONE,
    NCX_LAYER_TRANSPORT,  
    NCX_LAYER_RPC, 
    NCX_LAYER_OPERATION,
    NCX_LAYER_CONTENT
}  ncx_layer_t;


/* enum to identify the agent native target */
typedef enum ncx_agttarg_t_ {
    NCX_AGT_TARG_NONE,    
    NCX_AGT_TARG_CANDIDATE,
    NCX_AGT_TARG_RUNNING,
    NCX_AGT_TARG_LOCAL,   /* TBD */
    NCX_AGT_TARG_REMOTE,   /* TBD */
    NCX_AGT_TARG_CAND_RUNNING  /* TBD */
} ncx_agttarg_t;


/* enum to identify the agent native startup mode */
typedef enum ncx_agtstart_t_ {
    NCX_AGT_START_NONE,    
    NCX_AGT_START_MIRROR,
    NCX_AGT_START_DISTINCT
} ncx_agtstart_t;


/* enumeration of the different program shutdown modes */
typedef enum ncx_shutdowntyp_t_ {
    NCX_SHUT_NONE,
    NCX_SHUT_RESET,
    NCX_SHUT_RELOAD,
    NCX_SHUT_RESTART,
    NCX_SHUT_EXIT
} ncx_shutdowntyp_t;


/* hardwire the 3 standard configs */
typedef enum ncx_cfg_t_ {
    NCX_CFGID_RUNNING,
    NCX_CFGID_CANDIDATE,
    NCX_CFGID_STARTUP
} ncx_cfg_t;


/* instance string format types */
typedef enum ncx_instfmt_t_ {
    NCX_IFMT_NONE,
    NCX_IFMT_C,
    NCX_IFMT_XPATH1,       /* single-quote Xpath for filter */
    NCX_IFMT_XPATH2,    /* double-quote Xpath for error-path */
    NCX_IFMT_CLI
} ncx_instfmt_t;


/* enumeration for different NETCONF message types */
typedef enum ncx_msgtyp_t_ {
    NCX_MSGTYP_NONE,
    NCX_MSGTYP_HELLO,
    NCX_MSGTYP_RPCREQ,
    NCX_MSGTYP_RPCRPY,
    NCX_MSGTYP_NOTIF
} ncx_msgtyp_t;

/* enumeration for different YANG data-def status values */
typedef enum ncx_status_t_ {
    NCX_STATUS_NONE,
    NCX_STATUS_CURRENT,
    NCX_STATUS_DEPRECATED,
    NCX_STATUS_OBSOLETE
} ncx_status_t;


/* enumeration for CLI handling of bad input data 
 * used by yangcli, all others use NCX_BAD_DATA_ERROR
 *
 * NCX_BAD_DATA_IGNORE to silently accept invalid input values
 * NCX_BAD_DATA_WARN to warn and accept invalid input values
 * NCX_BAD_DATA_CHECK to prompt user to keep or re-enter value
 * NCX_BAD_DATA_ERROR to prompt user to re-enter value
 */
typedef enum ncx_bad_data_t_ {
    NCX_BAD_DATA_NONE,
    NCX_BAD_DATA_IGNORE,
    NCX_BAD_DATA_WARN,
    NCX_BAD_DATA_CHECK,
    NCX_BAD_DATA_ERROR
} ncx_bad_data_t;


/* enumeration of val_dump_value display modes */
typedef enum ncx_display_mode_t_ {
    NCX_DISPLAY_MODE_NONE,
    NCX_DISPLAY_MODE_PLAIN,
    NCX_DISPLAY_MODE_PREFIX,
    NCX_DISPLAY_MODE_MODULE,
    NCX_DISPLAY_MODE_XML,
    NCX_DISPLAY_MODE_XML_NONS,
    NCX_DISPLAY_MODE_JSON
} ncx_display_mode_t;


/* XPath expression axis types */
typedef enum ncx_xpath_axis_t_ {
    XP_AX_NONE,
    XP_AX_ANCESTOR,
    XP_AX_ANCESTOR_OR_SELF,
    XP_AX_ATTRIBUTE,
    XP_AX_CHILD,
    XP_AX_DESCENDANT,
    XP_AX_DESCENDANT_OR_SELF,
    XP_AX_FOLLOWING,
    XP_AX_FOLLOWING_SIBLING,
    XP_AX_NAMESPACE,
    XP_AX_PARENT,
    XP_AX_PRECEDING,
    XP_AX_PRECEDING_SIBLING,
    XP_AX_SELF
} ncx_xpath_axis_t;


/* Node name match modes */
typedef enum ncx_name_match_t_ {
    NCX_MATCH_NONE,
    NCX_MATCH_EXACT,
    NCX_MATCH_EXACT_NOCASE,
    NCX_MATCH_ONE,
    NCX_MATCH_ONE_NOCASE,
    NCX_MATCH_FIRST,
    NCX_MATCH_FIRST_NOCASE
} ncx_name_match_t;



typedef struct ncx_dec64_t_ {
    int64  val;        /* adjusted number to fit in 64 bits */
    uint8  digits;      /* number of decimal digits 1 .. 18 */
    uint8  zeroes;      /* number of leading zeroes 0 .. 17 */
} ncx_dec64_t;


/* union of all the basic number types
 * if float not supported, then it is stored as an int64
 */
typedef union ncx_num_t_ {
    int32  i;                            /* NCX_BT_INT */
    int64  l;                           /* NCX_BT_LONG */
    uint32 u;                           /* NCX_BT_UINT */
    uint64 ul;                         /* NCX_BT_ULONG */
    
#ifdef HAS_FLOAT             /* 'd' used only in XPath */
    double d;                         /* NCX_BT_DOUBLE */
#else
    int64  d;                         /* NCX_BT_DOUBLE */
#endif
    ncx_dec64_t  dec;              /* NCX_BT_DECIMAL64 */
} ncx_num_t;


/* string alias for data types:
 *  NCX_BT_STRING 
 *  NCX_BT_OSTRING
 *  NCX_BT_ENAME
 */
typedef xmlChar * ncx_str_t;

typedef const xmlChar * ncx_const_str_t;

/* one NCX_BT_ENUM enumeration value (user may enter 1 of 3 forms) */
typedef struct ncx_enum_t_ {
    const xmlChar *name;     /* bptr to typ_enum_t or dname */
    xmlChar       *dname;    /* malloced enum (value not checked) */
    int32          val;
} ncx_enum_t;


/* one NCX_BT_BITS bit value */
typedef struct ncx_bit_t_ {
    const xmlChar *name;     /* bptr to typ_enum_t.name or this.dname */
    xmlChar       *dname;    /* malloced bit name (value not checked) */
    uint32         pos;      /* position value */
} ncx_bit_t;


/* NCX list member: list of string or number */
typedef struct ncx_lmem_t_ {
    dlq_hdr_t     qhdr;
    union val_ {
	ncx_num_t    num;
	ncx_str_t    str;
	ncx_enum_t   enu;
	ncx_bit_t    bit;
	boolean      boo;
    } val;
    uint32   flags;
} ncx_lmem_t;


/* header for a NCX List */
typedef struct ncx_list_t_ {
    ncx_btype_t  btyp;
    dlq_hdr_t     memQ;                /* Q of ncx_lmem_t */
} ncx_list_t;


/* NCX base64 string node for YANG 'binary' built-in type */
typedef struct ncx_binary_t_ {
    unsigned char   *ustr;            /* binary string */
    uint32           ubufflen;    /* binary buffer len */
    uint32           ustrlen;    /* binary buffer used */
} ncx_binary_t;


/* struct to remember error info 
 * tkc->cur_err will be checked before tkc->cur
 * for error information
 */
typedef struct ncx_error_t_ {
    struct ncx_module_t_  *mod;
    uint32                 linenum;
    uint32                 linepos;
} ncx_error_t;


/* YANG extension usage entry */
typedef struct ncx_appinfo_t_ {
    dlq_hdr_t               qhdr;
    xmlChar                *prefix;
    xmlChar                *name;
    xmlChar                *value;
    struct ext_template_t_ *ext;
    dlq_hdr_t              *appinfoQ;
    boolean                 isclone;
    ncx_error_t             tkerr;
} ncx_appinfo_t;


/* YANG revision entry */
typedef struct ncx_revhist_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *version;
    xmlChar            *descr;
    xmlChar            *ref;
    status_t            res;
    ncx_error_t         tkerr;
} ncx_revhist_t;


/* YANG if-feature entry */
typedef struct ncx_iffeature_t_ {
    dlq_hdr_t              qhdr;
    xmlChar               *prefix;
    xmlChar               *name;
    struct ncx_feature_t_ *feature;
    ncx_error_t            tkerr;
    boolean                seen;          /* for yangdiff */
} ncx_iffeature_t;


typedef enum ncx_feature_code_t_ {
    NCX_FEATURE_CODE_NONE,    /* enum not explicitly set */
    NCX_FEATURE_CODE_STATIC,  /* compile-time if-feature code */
    NCX_FEATURE_CODE_DYNAMIC  /* run-time if-feature code */
} ncx_feature_code_t;


/* NCX session protocol versions supported */
typedef enum ncx_protocol_t_ {
    NCX_PROTO_NONE,
    NCX_PROTO_NETCONF10,      /* RFC 4741 base:1.0  */
    NCX_PROTO_NETCONF11       /* RFC xxxx base:1.1 */
} ncx_protocol_t;


/* YANG feature entry */
typedef struct ncx_feature_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *name;
    xmlChar            *descr;
    xmlChar            *ref;
    ncx_status_t        status;
    dlq_hdr_t           iffeatureQ;   /* Q of ncx_iffeature_t */
    dlq_hdr_t           appinfoQ;       /* Q of ncx_appinfo_t */
    status_t            res;    /* may be stored with errors */
    boolean             enabled;
    ncx_feature_code_t  code;    /* dynamic or static code-gen */
    ncx_error_t         tkerr;
    boolean             seen;          /* for yangdiff */
} ncx_feature_t;

/* struct for holding r/o pointer to generic internal node 
 * for filtering purposes
 */
typedef struct ncx_filptr_t_ {
    dlq_hdr_t             qhdr;
    struct val_value_t_  *node;        /* read-only backptr */
    struct val_value_t_  *virtualnode;  /* virtual val backptr */
    dlq_hdr_t             childQ;  /* Q of ncx_filptr_t */
} ncx_filptr_t;


/* back pointer to a YANG identity
 * used to create an inline tree of valid values
 * for an identity used as a base
 *
 * This inline Q record will be linked in to the
 * childQ of the base identity when the identity
 * containing this struct is using it as a base
 *
 * This thread is only used for client help,
 * to easily list all the QName values that are permitted
 * for a identityref leaf
 */
typedef struct ncx_idlink_t_ {
    dlq_hdr_t  qhdr;
    struct ncx_identity_t_ *identity;
    boolean    inq;
} ncx_idlink_t;


/* YANG identity entry */
typedef struct ncx_identity_t_ {
    dlq_hdr_t             qhdr;
    struct ncx_identity_t_ *base;      /* back-ptr to base id */
    xmlChar              *name;
    xmlChar              *baseprefix;
    xmlChar              *basename;
    xmlChar              *descr;
    xmlChar              *ref;
    ncx_status_t          status;
    dlq_hdr_t             childQ;          /* Q of ncx_idlink_t */
    dlq_hdr_t             appinfoQ;       /* Q of ncx_appinfo_t */
    status_t              res;     /* may be stored with errors */
    boolean               isroot;    /* base==NULL not an error */
    ncx_idlink_t          idlink;
    ncx_error_t           tkerr;
    boolean               seen;                  /* for yangcli */
} ncx_identity_t;


/* representation of one module or submodule during and after parsing */
typedef struct ncx_module_t_ {
    dlq_hdr_t         qhdr;
    xmlChar          *name; 
    xmlChar          *version;
    xmlChar          *organization;
    xmlChar          *contact_info;
    xmlChar          *descr;
    xmlChar          *ref;
    xmlChar          *ns;       /* malloc:main, copy:submod */
    xmlChar          *prefix;   /* may be empty in a submod */
    xmlChar          *xmlprefix;   /* will be diff. if duplicate */
    xmlChar          *source;              /* full filespec */
    xmlChar          *belongs;             /* set if submod */

    const xmlChar    *sourcefn;      /* ptr to fn in source */
    const xmlChar    *belongsver;    /* back ptr to mod ver */

    xmlns_id_t        nsid;            /* assigned by xmlns */
    uint32            langver;
    boolean           ismod;     /* module/submodule keyword */
    boolean           stmtmode;       /* T: save yang_stmt_t */
    boolean           added;         /* T: don't free on err */
    boolean           defaultrev;  /* T: use for default ver */
    status_t          status;         /* module parse result */
    uint32            errors;            /* yangdump results */
    uint32            warnings;          /* yangdump results */

    dlq_hdr_t         revhistQ;        /* Q of ncx_revhist_t */
    dlq_hdr_t         importQ;          /* Q of ncx_import_t */
    dlq_hdr_t         includeQ;        /* Q of ncx_include_t */
    dlq_hdr_t         typeQ;          /* Q of typ_template_t */
    dlq_hdr_t         groupingQ;      /* Q of grp_template_t */
    dlq_hdr_t         datadefQ;       /* Q of obj_template_t */
    dlq_hdr_t         extensionQ;     /* Q of ext_template_t */
    dlq_hdr_t         deviationQ;    /* Q of obj_deviation_t */
    dlq_hdr_t         featureQ;        /* Q of ncx_feature_t */
    dlq_hdr_t         identityQ;      /* Q of ncx_identity_t */
    dlq_hdr_t         appinfoQ;        /* Q of ncx_appinfo_t */
    dlq_hdr_t         typnameQ;        /* Q of ncx_typname_t */
    dlq_hdr_t         saveimpQ;    /* Q of yang_import_ptr_t */   
                                  /* saved from pcb->allimpQ */
    dlq_hdr_t         stmtQ;             /* Q of yang_stmt_t */
                             /* saved for top, yang, docmode */

    struct ncx_module_t_ *parent;  /* if this is a submodule */

    /* Q of yang_node_t to track all submodules */
    dlq_hdr_t         allincQ;     /* used if parent == NULL */

    dlq_hdr_t         incchainQ;   /* used if parent == NULL */

    ncx_list_t        devmodlist;     /* for deviations list */

} ncx_module_t;


/* enumeration for different NCX module conversion output types */
typedef enum ncx_cvttyp_t_ {
    NCX_CVTTYP_NONE,
    NCX_CVTTYP_XSD,
    NCX_CVTTYP_SQL,
    NCX_CVTTYP_SQLDB,
    NCX_CVTTYP_HTML,
    NCX_CVTTYP_YANG,
    NCX_CVTTYP_COPY,
    NCX_CVTTYP_H,
    NCX_CVTTYP_C,
    NCX_CVTTYP_CPP_TEST,
    NCX_CVTTYP_YIN,
    NCX_CVTTYP_TG2,
    NCX_CVTTYP_YH,
    NCX_CVTTYP_YC,
    NCX_CVTTYP_UH,
    NCX_CVTTYP_UC
} ncx_cvttyp_t;


/* enum for with-defaults enum values */
typedef enum ncx_withdefaults_t_ { 
    NCX_WITHDEF_NONE,
    NCX_WITHDEF_REPORT_ALL,
    NCX_WITHDEF_REPORT_ALL_TAGGED,
    NCX_WITHDEF_TRIM,
    NCX_WITHDEF_EXPLICIT
} ncx_withdefaults_t;


/* One 'import' clause in YANG */
typedef struct ncx_import_t_ {
    dlq_hdr_t           qhdr;
    xmlChar            *module;
    xmlChar            *prefix;
    xmlChar            *revision;
    ncx_module_t       *mod;                   /* back-ptr */
    boolean             used;
    boolean             usexsd;        /* FALSE if duplicate */
    boolean       force_yuma_nc; /* HACK: TRUE if yuma-nc replacing ietf-nc */
    dlq_hdr_t           appinfoQ;
    ncx_error_t         tkerr;
    status_t            res;   /* set to NO_ERR if the import was parsed OK */
} ncx_import_t;


/* One 'include' clause, YANG only */
typedef struct ncx_include_t_ {
    dlq_hdr_t             qhdr;
    xmlChar              *submodule;
    xmlChar              *revision;
    struct ncx_module_t_ *submod;                 /* back-ptr */
    boolean               usexsd;       /* FALSE if duplicate */
    dlq_hdr_t             appinfoQ;
    ncx_error_t           tkerr;
} ncx_include_t;


/* enum for REQUIRED vs. OPTIONAL token */
typedef enum ncx_opt_t_ { 
    NCX_REQ,                              /* clause is required */
    NCX_OPT                               /* clause is optional */
} ncx_opt_t;


/* enum for WHITESPACE ALLOWED vs. WHITESPACE NOT ALLOWED string */
typedef enum ncx_strtyp_t_ { 
    NCX_WSP,                 /* whitespace allowed: quoted string */
    NCX_NO_WSP         /* whitespace not allowed: unquoted string */
} ncx_strtyp_t;


/* YANG must statement struct */
typedef struct ncx_errinfo_t_ {
    dlq_hdr_t         qhdr;
    xmlChar          *descr;
    xmlChar          *ref;
    xmlChar          *error_app_tag;
    xmlChar          *error_message;
    boolean           seen;                     /* for yangdiff */
} ncx_errinfo_t;


/* keep track of the typenames used for local typedefs
 * only used by ncxdump to generate XSDs
 */
typedef struct ncx_typname_t_ {
    dlq_hdr_t                        qhdr;
    struct typ_template_t_          *typ;
    const xmlChar                   *typname;
    xmlChar                         *typname_malloc;
} ncx_typname_t;


/* user function callback template when a module is
 * loaded into the system
 *
 * ncx_load_cbfn_t
 * 
 *  Run an instrumentation-defined function 
 *  for a 'module-loaded' event
 *
 * INPUTS:
 *   mod == module that was added to the registry
 *
 */
typedef void (*ncx_load_cbfn_t) (ncx_module_t *mod);


/* user function callback template to traverse all module
 * features for a specified module
 *
 * ncx_feature_cbfn_t
 * 
 *  Run an instrumentation-defined function 
 *  for each feature found in the module (enabled or not)
 *
 * INPUTS:
 *    mod == module originally passed to the main 
 *           function, which contains the 'feature'
 *    feature == feature being processed in the traversal
 *    cookie == cookie originally passed to the main function
 *
 * RETURNS:
 *    TRUE if processing should continue
 *    FALSE if feature traversal should terminate
 */
typedef boolean (*ncx_feature_cbfn_t) (const ncx_module_t *mod,
				       ncx_feature_t *feature,
				       void *cookie);


/* enum for get-schema format type enum values
 * matches the schema-format identities in RFC 6022
 */
typedef enum ncx_modformat_t_ { 
    NCX_MODFORMAT_NONE,
    NCX_MODFORMAT_XSD,
    NCX_MODFORMAT_YANG,
    NCX_MODFORMAT_YIN,
    NCX_MODFORMAT_RNG,
    NCX_MODFORMAT_RNC
} ncx_modformat_t;


/* used with obj_deviation_t to defer object lookups */
typedef struct ncx_save_deviations_t_ {
    dlq_hdr_t    qhdr;
    dlq_hdr_t    importQ;        /* Q of ncx_import_t */
    dlq_hdr_t    deviationQ;  /* Q of obj_deviation_t */
    xmlChar     *devmodule;
    xmlChar     *devrevision;
    xmlChar     *devnamespace;
    xmlChar     *devprefix;
} ncx_save_deviations_t;


/* type of confirmEvent in the sysConfirmedCommit notification */
typedef enum ncx_confirm_event_t_ {
    NCX_CC_EVENT_NONE,
    NCX_CC_EVENT_START,
    NCX_CC_EVENT_CANCEL,
    NCX_CC_EVENT_TIMEOUT,
    NCX_CC_EVENT_EXTEND,
    NCX_CC_EVENT_COMPLETE
} ncx_confirm_event_t;

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncxtypes */
