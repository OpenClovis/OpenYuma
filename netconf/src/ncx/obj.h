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
#ifndef _H_obj
#define _H_obj

/*  FILE: obj.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Data Object Support

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
09-dec-07    abb      Begun
21jul08      abb      start obj-based rewrite

*/

#include <xmlstring.h>
#include <xmlregexp.h>

#include "grp.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "status.h"
#include "tk.h"
#include "rpc.h"
#include "typ.h"
#include "xmlns.h"
#include "xml_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* default vaule for config statement */
#define OBJ_DEF_CONFIG      TRUE

/* default vaule for mandatory statement */
#define OBJ_DEF_MANDATORY   FALSE

/* flags field in obj_template_t */

/* object is cloned from a grouping, for a uses statement */
#define OBJ_FL_CLONE        bit0

 /* def is cloned flag
  *   == 0 : obj.def.foo is malloced, but the typdef is cloned
  *   == 1 : obj.def.foo: foo is cloned
  */
#define OBJ_FL_DEFCLONE     bit1   

/* clone source
 *   == 0 : cloned object from uses
 *   == 1 : cloned object from augment
 */
#define OBJ_FL_AUGCLONE     bit2

/* object is marked for deletion */
#define OBJ_FL_DELETED      bit3

/* object is conditional, via a when-stmt expression */
#define OBJ_FL_CONDITIONAL  bit4

/* object is a top-level definition within a module or submodule */
#define OBJ_FL_TOP          bit5

/* object was entered with a 'kw name;' format and is 
 * considered empty by the yangdump program
 */
#define OBJ_FL_EMPTY        bit6

/* object has been visited by the yangdiff program */
#define OBJ_FL_SEEN         bit7

/* object marked as changed by the yangdiff program */
#define OBJ_FL_DIFF         bit8

/* object is marked as ncx:hidden */
#define OBJ_FL_HIDDEN       bit9

/* object is marked as ncx:root */
#define OBJ_FL_ROOT         bit10

/* object is marked as a password */
#define OBJ_FL_PASSWD       bit11

/* object is marked as a CLI-only node */
#define OBJ_FL_CLI          bit12

/* object is marked as an XSD list data type */
#define OBJ_FL_XSDLIST      bit13

/* OBJ_TYP_LEAF object is being uses as a key */
#define OBJ_FL_KEY          bit14

/* object is marked as abstract: not CLI or config data */
#define OBJ_FL_ABSTRACT     bit15

/* object is marked as config set */
#define OBJ_FL_CONFSET      bit16

/* object config value */
#define OBJ_FL_CONFIG       bit17

/* object is marked as mandatory set */
#define OBJ_FL_MANDSET      bit18

/* object mandatory value */
#define OBJ_FL_MANDATORY    bit19

/* object used in a unique-stmt within a list */
#define OBJ_FL_UNIQUE       bit20

/* object data type is an XPath string */
#define OBJ_FL_XPATH        bit21

/* object data type is a QName string */
#define OBJ_FL_QNAME        bit22

/* object data type is a schema-instance string */
#define OBJ_FL_SCHEMAINST   bit23

/* object is tagged ncx:secure */
#define OBJ_FL_SECURE       bit24

/* object is tagged ncx:very-secure */
#define OBJ_FL_VERY_SECURE  bit25

/* object is tagged ncx:default-parm-equals-ok */
#define OBJ_FL_CLI_EQUALS_OK  bit26

/* object is tagged ncx:sil-delete-children-first */
#define OBJ_FL_SIL_DELETE_CHILDREN_FIRST  bit27

/* object is tagged as ncx:user-write with no create access */
#define OBJ_FL_BLOCK_CREATE bit28

/* object is tagged as ncx:user-write with no update access */
#define OBJ_FL_BLOCK_UPDATE bit29

/* object is tagged as ncx:user-write with no delete access */
#define OBJ_FL_BLOCK_DELETE bit30


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/


/* enumeration for different YANG data def statement types
 * the enum order is significant!!! do not change!!!
 */
typedef enum obj_type_t_ {
    OBJ_TYP_NONE,
    OBJ_TYP_ANYXML,
    OBJ_TYP_CONTAINER,
    OBJ_TYP_LEAF,
    OBJ_TYP_LEAF_LIST,
    OBJ_TYP_LIST,       /* last real database object */
    OBJ_TYP_CHOICE,
    OBJ_TYP_CASE,       /* last named database object */
    OBJ_TYP_USES,
    OBJ_TYP_REFINE,            /* child of uses only */
    OBJ_TYP_AUGMENT,
    OBJ_TYP_RPC,
    OBJ_TYP_RPCIO,
    OBJ_TYP_NOTIF
} obj_type_t;


/* enumeration for different YANG augment statement types */
typedef enum obj_augtype_t_ {
    OBJ_AUGTYP_NONE,
    OBJ_AUGTYP_RPCIN,
    OBJ_AUGTYP_RPCOUT,
    OBJ_AUGTYP_CASE,
    OBJ_AUGTYP_DATA
} obj_augtype_t;


/* One YANG list key component */
typedef struct obj_key_t_ {
    dlq_hdr_t       qhdr;
    struct obj_template_t_ *keyobj;
    boolean         seen;   /* used by yangdiff */
} obj_key_t;


/* One component in a YANG list unique target */
typedef struct obj_unique_comp_t_ {
    dlq_hdr_t               qhdr;
    struct obj_template_t_ *unobj;
    xmlChar                 *xpath;       /* saved unique str for this obj */
    boolean                  isduplicate;  /* will be ignored by server */
} obj_unique_comp_t;


/* One component in a YANG list unique target */
typedef struct obj_unique_t_ {
    dlq_hdr_t       qhdr;
    xmlChar        *xpath;       /* complete saved unique str */
    dlq_hdr_t       compQ;          /* Q of obj_unique_comp_t */
    boolean         seen;               /* needed by yangdiff */
    boolean         isconfig;      /* constraint is on config */
    ncx_error_t     tkerr;
} obj_unique_t;


/* One YANG 'container' definition */
typedef struct obj_container_t_ {
    xmlChar       *name;
    xmlChar       *descr;
    xmlChar       *ref;
    xmlChar       *presence;
    dlq_hdr_t     *typedefQ;       /* Q of typ_template_t */
    dlq_hdr_t     *groupingQ;      /* Q of grp_template_t */
    dlq_hdr_t     *datadefQ;       /* Q of obj_template_t */
    boolean        datadefclone;
    ncx_status_t   status;
    dlq_hdr_t      mustQ;             /* Q of xpath_pcb_t */
    struct obj_template_t_ *defaultparm;
} obj_container_t;


/* One YANG 'leaf' or 'anyxml' definition */
typedef struct obj_leaf_t_ {
    xmlChar       *name;
    xmlChar       *units;
    xmlChar       *defval;
    xmlChar       *descr;
    xmlChar       *ref;
    typ_def_t     *typdef;
    ncx_status_t   status;
    dlq_hdr_t      mustQ;              /* Q of xpath_pcb_t */
    struct obj_template_t_ *leafrefobj;
} obj_leaf_t;


/* One YANG 'leaf-list' definition */
typedef struct obj_leaflist_t_ {
    xmlChar       *name;
    xmlChar       *units;
    xmlChar       *descr;
    xmlChar       *ref;
    typ_def_t     *typdef;
    boolean        ordersys; /* ordered-by system or user */
    boolean        minset;
    uint32         minelems;
    boolean        maxset;
    uint32         maxelems;
    ncx_status_t   status;
    dlq_hdr_t      mustQ;              /* Q of xpath_pcb_t */
    struct obj_template_t_ *leafrefobj;
} obj_leaflist_t;


/* One YANG 'list' definition */
typedef struct obj_list_t_ {
    xmlChar       *name;
    xmlChar       *keystr;
    xmlChar       *descr;
    xmlChar       *ref;
    dlq_hdr_t     *typedefQ;         /* Q of typ_template_t */
    dlq_hdr_t     *groupingQ;       /* Q of grp_template_t */
    dlq_hdr_t     *datadefQ;        /* Q of obj_template_t */
    dlq_hdr_t      keyQ;                 /* Q of obj_key_t */
    dlq_hdr_t      uniqueQ;           /* Q of obj_unique_t */
    boolean        datadefclone;
    boolean        ordersys;   /* ordered-by system or user */
    boolean        minset;
    uint32         minelems;
    boolean        maxset;
    uint32         maxelems;
    ncx_status_t   status;
    dlq_hdr_t      mustQ;              /* Q of xpath_pcb_t */
    ncx_error_t    keytkerr;
} obj_list_t;


/* One YANG 'choice' definition */
typedef struct obj_choice_t_ {
    xmlChar       *name;
    xmlChar       *defval;
    xmlChar       *descr;
    xmlChar       *ref;
    dlq_hdr_t     *caseQ;             /* Q of obj_template_t */
    boolean        caseQclone;
    ncx_status_t   status;
} obj_choice_t;


/* One YANG 'case' definition */
typedef struct obj_case_t_ {
    xmlChar        *name;
    xmlChar        *descr;
    xmlChar        *ref;
    dlq_hdr_t      *datadefQ;         /* Q of obj_template_t */
    boolean         nameclone;
    boolean         datadefclone;
    ncx_status_t    status;
} obj_case_t;


/* YANG uses statement struct */
typedef struct obj_uses_t_ {
    xmlChar          *prefix;
    xmlChar          *name;
    xmlChar          *descr;
    xmlChar          *ref;
    grp_template_t   *grp;      /* const back-ptr to grouping */
    dlq_hdr_t        *datadefQ;         /* Q of obj_template_t */
    ncx_status_t      status;
    boolean           expand_done;
} obj_uses_t;


/* YANG refine statement struct */
typedef struct obj_refine_t_ {
    xmlChar          *target;
    struct obj_template_t_ *targobj;

    /* the token for each sub-clause is saved because
     * when the refine-stmt is parsed, the target is not
     * known yet so picking the correct variant
     * such as refine-leaf-stmts or refine-list-stmts
     * needs to wait until the resolve phase
     */
    xmlChar          *descr;
    ncx_error_t       descr_tkerr;
    xmlChar          *ref;
    ncx_error_t       ref_tkerr;
    xmlChar          *presence;
    ncx_error_t       presence_tkerr;
    xmlChar          *def;
    ncx_error_t       def_tkerr;
    /* config and confset are in the object flags */
    ncx_error_t       config_tkerr;
    /* mandatory and mandset are in the object flags */
    ncx_error_t       mandatory_tkerr;
    uint32            minelems;
    ncx_error_t       minelems_tkerr;   /* also minset */
    uint32            maxelems;
    ncx_error_t       maxelems_tkerr;   /* also maxset */
    dlq_hdr_t         mustQ;
} obj_refine_t;


/* YANG input-stmt or output-stmt struct */
typedef struct obj_rpcio_t_ {
    xmlChar           *name;                 /* input or output */
    dlq_hdr_t          typedefQ;         /* Q of typ_template_t */
    dlq_hdr_t          groupingQ;        /* Q of gtp_template_t */
    dlq_hdr_t          datadefQ;         /* Q of obj_template_t */
    struct obj_template_t_ *defaultparm;
} obj_rpcio_t;


/* YANG rpc-stmt struct; used for augment and name collision detect */
typedef struct obj_rpc_t_ {
    xmlChar           *name;
    xmlChar           *descr;
    xmlChar           *ref;
    ncx_status_t       status;
    dlq_hdr_t          typedefQ;         /* Q of typ_template_t */
    dlq_hdr_t          groupingQ;        /* Q of gtp_template_t */
    dlq_hdr_t          datadefQ;         /* Q of obj_template_t */

    /* internal fields for manager and agent */
    xmlns_id_t      nsid;
    boolean          supported;    /* mod loaded, not implemented */    
} obj_rpc_t;


/* YANG augment statement struct */
typedef struct obj_augment_t_ {
    xmlChar          *target;
    xmlChar          *descr;
    xmlChar          *ref;
    struct obj_template_t_ *targobj;
    obj_augtype_t     augtype;
    ncx_status_t      status;
    dlq_hdr_t         datadefQ;         /* Q of obj_template_t */
} obj_augment_t;


/* One YANG 'notification' clause definition */
typedef struct obj_notif_t_ {
    xmlChar          *name;
    xmlChar          *descr;
    xmlChar          *ref;
    ncx_status_t      status;
    dlq_hdr_t         typedefQ;         /* Q of typ_template_t */
    dlq_hdr_t         groupingQ;        /* Q of gtp_template_t */
    dlq_hdr_t         datadefQ;          /* Q of obj_template_t */
} obj_notif_t;

/* back-pointer to inherited if-feature statements */
typedef struct obj_iffeature_ptr_t_ {
    dlq_hdr_t            qhdr;
    ncx_iffeature_t     *iffeature;
} obj_iffeature_ptr_t;


/* back-pointer to inherited when statements */
typedef struct obj_xpath_ptr_t_ {
    dlq_hdr_t            qhdr;
    struct xpath_pcb_t_  *xpath;
} obj_xpath_ptr_t;


/* One YANG data-def-stmt */
typedef struct obj_template_t_ {
    dlq_hdr_t      qhdr;
    obj_type_t     objtype;
    uint32         flags;              /* see OBJ_FL_* definitions */
    ncx_error_t    tkerr;
    grp_template_t *grp;          /* non-NULL == in a grp.datadefQ */

    /* 3 back pointers */
    struct obj_template_t_ *parent;
    struct obj_template_t_ *usesobj;
    struct obj_template_t_ *augobj;

    struct xpath_pcb_t_    *when;           /* optional when clause */
    dlq_hdr_t               metadataQ;       /* Q of obj_metadata_t */
    dlq_hdr_t               appinfoQ;         /* Q of ncx_appinfo_t */
    dlq_hdr_t               iffeatureQ;     /* Q of ncx_iffeature_t */

    dlq_hdr_t          inherited_iffeatureQ;   /* Q of obj_iffeature_ptr_t */
    dlq_hdr_t          inherited_whenQ;     /* Q of obj_xpath_ptr_t */

    /* cbset is agt_rpc_cbset_t for RPC or agt_cb_fnset_t for OBJ */
    void                   *cbset;   

    /* object module and namespace ID 
     * assigned at runtime
     * this can be changed over and over as a
     * uses statement is expanded.  The final
     * expansion into a real object will leave
     * the correct value in place
     */
    struct ncx_module_t_ *mod;
    xmlns_id_t            nsid;

    union def_ {
	obj_container_t   *container;
	obj_leaf_t        *leaf;
	obj_leaflist_t    *leaflist;
	obj_list_t        *list;
	obj_choice_t      *choic;
	obj_case_t        *cas;
	obj_uses_t        *uses;
	obj_refine_t      *refine;
	obj_augment_t     *augment;
	obj_rpc_t         *rpc;
	obj_rpcio_t       *rpcio;
	obj_notif_t       *notif;
    } def;

} obj_template_t;


/* One YANG metadata (XML attribute) node */
typedef struct obj_metadata_t_ {
    dlq_hdr_t      qhdr;
    struct obj_template_t_ *parent;     /* obj containing metadata */
    xmlChar       *name;
    typ_def_t     *typdef;
    xmlns_id_t     nsid;                 /* in case parent == NULL */
    ncx_error_t    tkerr;
} obj_metadata_t;


/* type of deviation for each deviate entry */
typedef enum obj_deviate_arg_t_ {
    OBJ_DARG_NONE,
    OBJ_DARG_ADD,
    OBJ_DARG_DELETE,
    OBJ_DARG_REPLACE,
    OBJ_DARG_NOT_SUPPORTED
}  obj_deviate_arg_t;


/* YANG deviate statement struct */
typedef struct obj_deviate_t_ {
    dlq_hdr_t          qhdr;

    /* the error info for each sub-clause is saved because
     * when the deviation-stmt is parsed, the target is not
     * known yet so picking the correct variant
     * such as type-stmt or refine-list-stmts
     * needs to wait until the resolve phase
     *
     */
    ncx_error_t       tkerr;
    boolean           empty;
    obj_deviate_arg_t arg;
    ncx_error_t       arg_tkerr;
    typ_def_t        *typdef;
    ncx_error_t       type_tkerr;
    xmlChar          *units;
    ncx_error_t       units_tkerr;
    xmlChar          *defval;
    ncx_error_t       default_tkerr;
    boolean           config;
    ncx_error_t       config_tkerr;
    boolean           mandatory;
    ncx_error_t       mandatory_tkerr;
    uint32            minelems;
    ncx_error_t       minelems_tkerr;   /* also minset */
    uint32            maxelems;
    ncx_error_t       maxelems_tkerr;   /* also maxset */
    dlq_hdr_t         mustQ;     /* Q of xpath_pcb_t */
    dlq_hdr_t         uniqueQ;  /* Q of obj_unique_t */
    dlq_hdr_t         appinfoQ;  /* Q of ncx_appinfo_t */
} obj_deviate_t;


/* YANG deviate statement struct */
typedef struct obj_deviation_t_ {
    dlq_hdr_t             qhdr;
    xmlChar              *target;
    xmlChar              *targmodname;
    obj_template_t       *targobj;
    xmlChar              *descr;
    xmlChar              *ref;
    ncx_error_t           tkerr;
    xmlChar              *devmodname;  /* set if not the targmod */
    boolean               empty;
    status_t              res;
    dlq_hdr_t             deviateQ;   /* Q of obj_deviate_t */
    dlq_hdr_t             appinfoQ;  /* Q of ncx_appinfo_t */
} obj_deviation_t;


/* child or descendant node search walker function
 *
 * INPUTS:
 *   obj == object node found in descendant search
 *   cookie1 == cookie1 value passed to start of walk
 *   cookie2 == cookie2 value passed to start of walk
 *
 * RETURNS:
 *   TRUE if walk should continue
 *   FALSE if walk should terminate 
 */
typedef boolean
    (*obj_walker_fn_t) (obj_template_t *obj,
			void *cookie1,
			void *cookie2);


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION obj_new_template
* 
* Malloc and initialize the fields in a an object template
*
* INPUTS:
*   objtype == the specific object type to create
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern obj_template_t *
    obj_new_template (obj_type_t objtype);


/********************************************************************
* FUNCTION obj_free_template
* 
* Scrub the memory in a obj_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    obj == obj_template_t data structure to free
*********************************************************************/
extern void 
    obj_free_template (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_find_template
* 
* Find an object with the specified name
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
extern obj_template_t *
    obj_find_template (dlq_hdr_t  *que,
		       const xmlChar *modname,
		       const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_template_con
* 
* Find an object with the specified name
* Return a const pointer; used by yangdump
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
extern const obj_template_t *
    obj_find_template_con (dlq_hdr_t  *que,
			   const xmlChar *modname,
			   const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_template_test
* 
* Find an object with the specified name
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
extern obj_template_t *
    obj_find_template_test (dlq_hdr_t  *que,
			    const xmlChar *modname,
			    const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_template_top
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files visible to this module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    obj_find_template_top (ncx_module_t *mod,
			   const xmlChar *modname,
			   const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_template_top_ex
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files visible to this module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*   match_names == enum for selected match names mode
*   alt_names == TRUE if alt-name should be checked in addition
*                to the YANG node name
*             == FALSE to check YANG names only
*   dataonly == TRUE to check just data nodes
*               FALSE to check all nodes
*   retres == address of return status
*
* OUTPUTS:
*   if retres not NULL, *retres set to return status
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    obj_find_template_top_ex (ncx_module_t *mod,
                              const xmlChar *modname,
                              const xmlChar *objname,
                              ncx_name_match_t match_names,
                              boolean alt_names,
                              boolean dataonly,
                              status_t *retres);


/********************************************************************
* FUNCTION obj_find_template_all
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files used within the entire main module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern obj_template_t *
    obj_find_template_all (ncx_module_t *mod,
                           const xmlChar *modname,
                           const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_child
* 
* Find a child object with the specified Qname
*
* !!! This function checks for accessible names only!!!
* !!! That means child nodes of choice->case will be
* !!! present instead of the choice name or case name
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
extern obj_template_t *
    obj_find_child (obj_template_t  *obj,
		    const xmlChar *modname,
		    const xmlChar *objname);


/********************************************************************
* FUNCTION obj_find_child_ex
* 
* Find a child object with the specified Qname
* extended match modes
*
* !!! This function checks for accessible names only!!!
* !!! That means child nodes of choice->case will be
* !!! present instead of the choice name or case name
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*    match_names == enum for selected match names mode
*    alt_names == TRUE if alt-name should be checked in addition
*                to the YANG node name
*             == FALSE to check YANG names only
*    dataonly == TRUE to check just data nodes
*                FALSE to check all nodes
*    retres == address of return status
*
* OUTPUTS:
*   if retres not NULL, *retres set to return status
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
extern obj_template_t *
    obj_find_child_ex (obj_template_t  *obj,
                       const xmlChar *modname,
                       const xmlChar *objname,
                       ncx_name_match_t match_names,
                       boolean alt_names,
                       boolean dataonly,
                       status_t *retres);


/********************************************************************
* FUNCTION obj_find_child_str
* 
* Find a child object with the specified Qname
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find, not Z-terminated
*    objnamelen == length of objname string
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
extern obj_template_t *
    obj_find_child_str (obj_template_t  *obj,
			const xmlChar *modname,
			const xmlChar *objname,
			uint32 objnamelen);


/********************************************************************
* FUNCTION obj_match_child_str
* 
* Match a child object with the specified Qname
* Find first command that matches all N chars of objname
*
* !!! This function checks for accessible names only!!!
* !!! That means child nodes of choice->case will be
* !!! present instead of the choice name or case name
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find, not Z-terminated
*    objnamelen == length of objname string
*    matchcount == address of return parameter match count
*                  (may be NULL)
* OUTPUTS:
*   if non-NULL:
*    *matchcount == number of parameters that matched
*                   only the first match will be returned
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
extern obj_template_t *
    obj_match_child_str (obj_template_t *obj,
			 const xmlChar *modname,
			 const xmlChar *objname,
			 uint32 objnamelen,
			 uint32 *matchcount);


/********************************************************************
* FUNCTION obj_first_child
* 
* Get the first child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_first_child (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_last_child
* 
* Get the last child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_last_child (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_next_child
* 
* Get the next child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_next_child (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_previous_child
* 
* Get the previous child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_previous_child (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_first_child_deep
* 
* Get the first child object if the specified object
* has any children.  Look past choices and cases to
* the real nodes within them
*
*  !!!! SKIPS OVER AUGMENT AND USES AND CHOICES AND CASES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_first_child_deep (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_next_child_deep
* 
* Get the next child object if the specified object
* has any children.  Look past choice and case nodes
* to the real nodes within them
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check
*
* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
extern obj_template_t *
    obj_next_child_deep (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_find_all_children
* 
* Find all occurances of the specified node(s)
* within the children of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath expression
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    childname == name of child node to find
*              == NULL to match any child name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    obj_find_all_children (ncx_module_t *exprmod,
			   obj_walker_fn_t walkerfn,
			   void *cookie1,
			   void *cookie2,
			   obj_template_t *startnode,
			   const xmlChar *modname,
			   const xmlChar *childname,
			   boolean configonly, 
			   boolean textmode,
			   boolean useroot);


/********************************************************************
* FUNCTION obj_find_all_ancestors
* 
* Find all occurances of the specified node(s)
* within the ancestors of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of ancestor node to find
*              == NULL to match any ancestor name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    obj_find_all_ancestors (ncx_module_t *exprmod,
			    obj_walker_fn_t walkerfn,
			    void *cookie1,
			    void *cookie2,
			    obj_template_t *startnode,
			    const xmlChar *modname,
			    const xmlChar *name,
			    boolean configonly,
			    boolean textmode,
			    boolean useroot,
			    boolean orself,
			    boolean *fncalled);


/********************************************************************
* FUNCTION obj_find_all_descendants
* 
* Find all occurances of the specified node(s)
* within the descendants of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath expression
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of descendant node to find
*              == NULL to match any descendant name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    obj_find_all_descendants (ncx_module_t *exprmod,
			      obj_walker_fn_t walkerfn,
			      void *cookie1,
			      void *cookie2,
			      obj_template_t *startnode,
			      const xmlChar *modname,
			      const xmlChar *name,
			      boolean configonly,
			      boolean textmode,
			      boolean useroot,
			      boolean orself,
			      boolean *fncalled);


/********************************************************************
* FUNCTION obj_find_all_pfaxis
* 
* Find all occurances of the specified preceding
* or following node(s).  Could also be
* within the descendants of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == starting sibling node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*
*    name == name of preceding or following node to find
*         == NULL to match any name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 1 level is checked
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    obj_find_all_pfaxis (ncx_module_t *exprmod,
			 obj_walker_fn_t walkerfn,
			 void *cookie1,
			 void *cookie2,
			 obj_template_t *startnode,
			 const xmlChar *modname,
			 const xmlChar *name,
			 boolean configonly,
			 boolean dblslash,
			 boolean textmode,
			 boolean useroot,
			 ncx_xpath_axis_t axis,
			 boolean *fncalled);


/********************************************************************
* FUNCTION obj_find_case
* 
* Find a specified case arm by name
*
* INPUTS:
*    choic == choice struct to check
*    modname == name of the module that added this case (may be NULL)
*    casname == name of the case to find
*
* RETURNS:
*    pointer to obj_case_t for requested case, NULL if not found
*********************************************************************/
extern obj_case_t *
    obj_find_case (obj_choice_t *choic,
		   const xmlChar *modname,
		   const xmlChar *casname);



/********************************************************************
* FUNCTION obj_new_rpcio
* 
* Malloc and initialize the fields in a an obj_rpcio_t
* Fields are setup within the new obj_template_t, based
* on the values in rpcobj
*
* INPUTS:
*   rpcobj == parent OBJ_TYP_RPC template
*   name == name string of the node (input or output)
*
* RETURNS:
*    pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern obj_template_t * 
    obj_new_rpcio (obj_template_t *rpcobj,
		   const xmlChar *name);


/********************************************************************
* FUNCTION obj_clean_datadefQ
* 
* Clean and free all the obj_template_t structs in the specified Q
*
* INPUTS:
*    datadefQ == Q of obj_template_t to clean
*********************************************************************/
extern void
    obj_clean_datadefQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION obj_find_type
*
* Check if a typ_template_t in the obj typedefQ hierarchy
*
* INPUTS:
*   obj == obj_template using the typedef
*   typname == type name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern typ_template_t *
    obj_find_type (obj_template_t *obj,
		   const xmlChar *typname);


/********************************************************************
* FUNCTION obj_first_typedef
*
* Get the first local typedef for this object, if any
*
* INPUTS:
*   obj == obj_template to use
*
* RETURNS:
*  pointer to first typ_template_t struct if present, NULL otherwise
*********************************************************************/
extern typ_template_t *
    obj_first_typedef (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_find_grouping
*
* Check if a grp_template_t in the obj groupingQ hierarchy
*
* INPUTS:
*   obj == obj_template using the grouping
*   grpname == grouping name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern grp_template_t *
    obj_find_grouping (obj_template_t *obj,
		       const xmlChar *grpname);

/********************************************************************
* FUNCTION obj_first_grouping
*
* Get the first local grouping if any
*
* INPUTS:
*   obj == obj_template to use
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
extern grp_template_t *
    obj_first_grouping (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_set_named_type
* 
* Resolve type test 
* Called during phase 2 of module parsing
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typname == name field from typ->name  (may be NULL)
*   typdef == typdef in progress
*   parent == obj_template containing this typedef
*          == NULL if this is the top-level, use mod->typeQ
*   grp == grp_template containing this typedef
*          == NULL if the typedef is not contained in a grouping
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    obj_set_named_type (tk_chain_t *tkc,
			ncx_module_t *mod,
			const xmlChar *typname,
			typ_def_t *typdef,
			obj_template_t *parent,
			grp_template_t *grp);


/********************************************************************
* FUNCTION obj_clone_template
*
* Clone an obj_template_t
* Copy the pointers from the srcobj into the new obj
*
* If the mobj is non-NULL, then the non-NULL revisable
* fields in the mobj struct will be merged into the new object
*
* INPUTS:
*   mod == module struct that is defining the new cloned data
*          this may be different than the module that will
*          contain the cloned data (except top-level objects)
*   srcobj == obj_template to clone
*             !!! This struct MUST NOT be deleted!!!
*             !!! Unless all of its clones are also deleted !!!
*   mobjQ == merge object Q (may be NULL)
*           datadefQ to check for OBJ_TYP_REFINE nodes
*           If the target of the refine node matches the
*           srcobj (e.g., from same grouping), then the
*           sub-clauses in that refinement-stmt that
*           are allowed to be revised will be checked
*
* RETURNS:
*   pointer to malloced clone obj_template_t
*   NULL if malloc failer error or internal error
*********************************************************************/
extern obj_template_t *
    obj_clone_template (ncx_module_t *mod,
			obj_template_t *srcobj,
			dlq_hdr_t *mobjQ);


/********************************************************************
* FUNCTION obj_clone_template_case
*
* Clone an obj_template_t but make sure it is wrapped
* in a OBJ_TYP_CASE layer
*
* Copy the pointers from the srcobj into the new obj
*
* Create an OBJ_TYP_CASE wrapper if needed,
* for a short-case-stmt data def 
*
* If the mobj is non-NULL, then the non-NULL revisable
* fields in the mobj struct will be merged into the new object
*
* INPUTS:
*   mod == module struct that is defining the new cloned data
*          this may be different than the module that will
*          contain the cloned data (except top-level objects)
*   srcobj == obj_template to clone
*             !!! This struct MUST NOT be deleted!!!
*             !!! Unless all of its clones are also deleted !!!
*   mobjQ == Q of obj_refine_t objects to merge (may be NULL)
*           only fields allowed to be revised will be checked
*           even if other fields are set in this struct
*
* RETURNS:
*   pointer to malloced clone obj_template_t
*   NULL if malloc failer error or internal error
*********************************************************************/
extern obj_template_t *
    obj_clone_template_case (ncx_module_t *mod,
			     obj_template_t *srcobj,
			     dlq_hdr_t *mobjQ);


/********************    obj_unique_t   ********************/


/********************************************************************
* FUNCTION obj_new_unique
* 
* Alloc and Init a obj_unique_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern obj_unique_t *
    obj_new_unique (void);


/********************************************************************
* FUNCTION obj_init_unique
* 
* Init a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to init
*********************************************************************/
extern void
    obj_init_unique (obj_unique_t *un);


/********************************************************************
* FUNCTION obj_free_unique
* 
* Free a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to free
*********************************************************************/
extern void
    obj_free_unique (obj_unique_t *un);


/********************************************************************
* FUNCTION obj_clean_unique
* 
* Clean a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to clean
*********************************************************************/
extern void
    obj_clean_unique (obj_unique_t *un);


/********************************************************************
* FUNCTION obj_new_unique_comp
* 
* Alloc and Init a obj_unique_comp_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern obj_unique_comp_t *
    obj_new_unique_comp (void);


/********************************************************************
* FUNCTION obj_free_unique_comp
* 
* Free a obj_unique_comp_t struct
*
* INPUTS:
*   unc == obj_unique_comp_t struct to free
*********************************************************************/
extern void
    obj_free_unique_comp (obj_unique_comp_t *unc);


/********************************************************************
* FUNCTION obj_find_unique
* 
* Find a specific unique-stmt
*
* INPUTS:
*    que == queue of obj_unique_t to check
*    xpath == relative path expression for the
*             unique node to find
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
extern obj_unique_t *
    obj_find_unique (dlq_hdr_t *que,
		     const xmlChar *xpath);


/********************************************************************
* FUNCTION obj_first_unique
* 
* Get the first unique-stmt for a list
*
* INPUTS:
*   listobj == (list) object to check for unique structs
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
extern obj_unique_t *
    obj_first_unique (obj_template_t *listobj);



/********************************************************************
* FUNCTION obj_next_unique
* 
* Get the next unique-stmt for a list
*
* INPUTS:
*  un == current unique node
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
extern obj_unique_t *
    obj_next_unique (obj_unique_t *un);


/********************************************************************
* FUNCTION obj_first_unique_comp
* 
* Get the first identifier in a unique-stmt for a list
*
* INPUTS:
*   un == unique struct to check
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
extern obj_unique_comp_t *
    obj_first_unique_comp (obj_unique_t *un);


/********************************************************************
* FUNCTION obj_next_unique_comp
* 
* Get the next unique-stmt component for a list
*
* INPUTS:
*  uncomp == current unique component node
*
* RETURNS:
*   pointer to next entry or NULL if none
*********************************************************************/
extern obj_unique_comp_t *
    obj_next_unique_comp (obj_unique_comp_t *uncomp);


/********************************************************************
* FUNCTION obj_new_key
* 
* Alloc and Init a obj_key_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern obj_key_t *
    obj_new_key (void);


/********************************************************************
* FUNCTION obj_free_key
* 
* Free a obj_key_t struct
*
* INPUTS:
*   key == obj_key_t struct to free
*********************************************************************/
extern void
    obj_free_key (obj_key_t *key);


/********************************************************************
* FUNCTION obj_find_key
* 
* Find a specific key component by key leaf identifier name
* Assumes deep keys are not supported!!!
*
* INPUTS:
*   que == Q of obj_key_t to check
*   keycompname == key component name to find
*
* RETURNS:
*   pointer to found key component or NULL if not found
*********************************************************************/
extern obj_key_t *
    obj_find_key (dlq_hdr_t *que,
		  const xmlChar *keycompname);


/********************************************************************
* FUNCTION obj_find_key2
* 
* Find a specific key component, check for a specific node
* in case deep keys are supported, and to check for duplicates
*
* INPUTS:
*   que == Q of obj_key_t to check
*   keyobj == key component object to find
*
* RETURNS:
*   pointer to found key component or NULL if not found
*********************************************************************/
extern obj_key_t *
    obj_find_key2 (dlq_hdr_t *que,
		   obj_template_t *keyobj);


/********************************************************************
* FUNCTION obj_first_key
* 
* Get the first key record
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   pointer to first key component or NULL if not found
*********************************************************************/
extern obj_key_t *
    obj_first_key (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_first_ckey
* 
* Get the first key record: Const version
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   pointer to first key component or NULL if not found
*********************************************************************/
extern const obj_key_t *
    obj_first_ckey (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_next_key
* 
* Get the next key record
*
* INPUTS:
*   objkey == current key record
*
* RETURNS:
*   pointer to next key component or NULL if not found
*********************************************************************/
extern obj_key_t *
    obj_next_key (obj_key_t *objkey);


/********************************************************************
* FUNCTION obj_next_ckey
* 
* Get the next key record: Const version
*
* INPUTS:
*   objkey == current key record
*
* RETURNS:
*   pointer to next key component or NULL if not found
*********************************************************************/
extern const obj_key_t *
    obj_next_ckey (const obj_key_t *objkey);


/********************************************************************
* FUNCTION obj_key_count
* 
* Get the number of keys for this object
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   number of keys in the obj_key_t Q
*********************************************************************/
extern uint32
    obj_key_count (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_key_count_to_root
* 
* Check ancestor-or-self nodes until root reached
* Find all lists; Count the number of keys
*
* INPUTS:
*   obj == object to start check from
* RETURNS:
*   number of keys in ancestor-or-self nodes
*********************************************************************/
extern uint32
    obj_key_count_to_root (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_traverse_keys
* 
* Check ancestor-or-self nodes until root reached
* Find all lists; For each list, starting with the
* closest to root, invoke the callback function
* for each of the key objects in order
*
* INPUTS:
*   obj == object to start check from
*   cookie1 == cookie1 to pass to the callback function
*   cookie2 == cookie2 to pass to the callback function
*   walkerfn == walker callback function
*           returns FALSE to terminate traversal
*
*********************************************************************/
extern void
    obj_traverse_keys (obj_template_t *obj,
                       void *cookie1,
                       void *cookie2,
                       obj_walker_fn_t walkerfn);


/********************************************************************
* FUNCTION obj_any_rpcs
* 
* Check if there are any RPC methods in the datadefQ
*
* INPUTS:
*   que == Q of obj_template_t to check
*
* RETURNS:
*   TRUE if any OBJ_TYP_RPC found, FALSE if not
*********************************************************************/
extern boolean
    obj_any_rpcs (const dlq_hdr_t *datadefQ);


/********************************************************************
* FUNCTION obj_any_notifs
* 
* Check if there are any notifications in the datadefQ
*
* INPUTS:
*   que == Q of obj_template_t to check
*
* RETURNS:
*   TRUE if any OBJ_TYP_NOTIF found, FALSE if not
*********************************************************************/
extern boolean
    obj_any_notifs (const dlq_hdr_t *datadefQ);


/********************************************************************
* FUNCTION obj_new_deviate
* 
* Malloc and initialize the fields in a an object deviate statement
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern obj_deviate_t *
    obj_new_deviate (void);


/********************************************************************
* FUNCTION obj_free_deviate
* 
* Clean and free an object deviate statement
*
* INPUTS:
*   deviate == pointer to the struct to clean and free
*********************************************************************/
extern void
    obj_free_deviate (obj_deviate_t *deviate);


/********************************************************************
* FUNCTION obj_get_deviate_arg
* 
* Get the deviate-arg string from its enumeration
*
* INPUTS:
*   devarg == enumeration to convert
* RETURNS:
*   const string version of the enum
*********************************************************************/
extern const xmlChar *
    obj_get_deviate_arg (obj_deviate_arg_t devarg);


/********************************************************************
* FUNCTION obj_new_deviation
* 
* Malloc and initialize the fields in a an object deviation statement
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern obj_deviation_t *
    obj_new_deviation (void);


/********************************************************************
* FUNCTION obj_free_deviation
* 
* Clean and free an object deviation statement
*
* INPUTS:
*   deviation == pointer to the struct to clean and free
*********************************************************************/
extern void
    obj_free_deviation (obj_deviation_t *deviation);


/********************************************************************
* FUNCTION obj_clean_deviationQ
* 
* Clean and free an Q of object deviation statements
*
* INPUTS:
*   deviationQ == pointer to Q of the structs to clean and free
*********************************************************************/
extern void
    obj_clean_deviationQ (dlq_hdr_t *deviationQ);


/********************************************************************
* FUNCTION obj_gen_object_id
* 
* Malloc and Generate the object ID for an object node
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_gen_object_id (const obj_template_t *obj,
		       xmlChar  **buff);


/********************************************************************
 * Malloc and Generate the object ID for an object node
 * Remove all conceptual OBJ_TYP_CHOICE and OBJ_TYP_CASE nodes
 * so the resulting string will represent the structure of the
 * value tree for XPath searching
 *
 * \param obj the node to generate the instance ID for
 * \param buff the pointer to address of buffer to use
 * \return status
 *********************************************************************/
extern status_t 
    obj_gen_object_id_xpath (const obj_template_t *obj,
                             xmlChar  **buff);


/********************************************************************
 * Malloc and Generate the object ID for a unique-stmt test
 *
 * \param obj the node to generate the instance ID for
 * \param stopobj the ancestor node to stop at
 * \param buff the pointer to address of buffer to use
 * \return status
 *********************************************************************/
extern status_t obj_gen_object_id_unique (const obj_template_t *obj, 
                                          const obj_template_t *stopobj,
                                          xmlChar  **buff);


/********************************************************************
* FUNCTION obj_gen_object_id_code
* 
* Malloc and Generate the object ID for an object node
* for C code usage
* generate a unique name for C code; handles augments
*
* INPUTS:
*   mod == current module in progress
*   obj == node to generate the instance ID for
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_gen_object_id_code (ncx_module_t *mod,
                            const obj_template_t *obj,
                            xmlChar  **buff);


/********************************************************************
* FUNCTION obj_copy_object_id
* 
* Generate the object ID for an object node and copy to the buffer
* copy an object ID to a buffer
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   buff == buffer to use
*   bufflen == size of buff
*   reallen == address of return length of actual identifier 
*               (may be NULL)
*
* OUTPUTS
*   buff == filled in with the object ID
*  if reallen not NULL:
*     *reallen == length of identifier, even if error occurred
*  
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_copy_object_id (const obj_template_t *obj,
			xmlChar  *buff,
			uint32 bufflen,
			uint32 *reallen);


/********************************************************************
* FUNCTION obj_copy_object_id_mod
* 
* Generate the object ID for an object node and copy to the buffer
* copy an object ID to a buffer; Use modname in object identifier
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   buff == buffer to use
*   bufflen == size of buff
*   reallen == address of return length of actual identifier 
*               (may be NULL)
*
* OUTPUTS
*   buff == filled in with the object ID
*  if reallen not NULL:
*     *reallen == length of identifier, even if error occurred
*  
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_copy_object_id_mod (const obj_template_t *obj,
                            xmlChar  *buff,
                            uint32 bufflen,
                            uint32 *reallen);


/********************************************************************
* FUNCTION obj_gen_aughook_id
* 
* Malloc and Generate the augment hook element name for
* the specified object. This will be a child node of the
* specified object.
* 
* INPUTS:
*   obj == node to generate the augment hook ID for
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_gen_aughook_id (const obj_template_t *obj,
			xmlChar  **buff);


/********************************************************************
* FUNCTION obj_get_name
* 
* Get the name field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the name field, NULL if some error or unnamed
*********************************************************************/
extern const xmlChar * 
    obj_get_name (const obj_template_t *obj);



/********************************************************************
* FUNCTION obj_set_name
* 
* Set the name field for this obj
*
* INPUTS:
*   obj == the specific object to set or change the name
*   objname == new name string to use
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    obj_set_name (obj_template_t *obj,
                  const xmlChar *objname);


/********************************************************************
* FUNCTION obj_has_name
* 
* Check if the specified object type has a name
*
* this function is used throughout the code to 
* filter out uses and augment nodes from the
* real nodes.  Those are the only YANG nodes that
* do not have a name assigned to them
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   TRUE if obj has a name
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_has_name (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_has_text_content
* 
* Check if the specified object type has a text content
* for XPath purposes
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   TRUE if obj has text content
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_has_text_content (const obj_template_t *obj);



/********************************************************************
* FUNCTION obj_get_status
* 
* Get the status field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG status clause for this object
*********************************************************************/
extern ncx_status_t
    obj_get_status (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_description
* 
* Get the description field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
extern const xmlChar *
    obj_get_description (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_alt_description
* 
* Get the alternate description field for this obj
* Check if any 'info', then 'help' appinfo nodes present
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
extern const xmlChar *
    obj_get_alt_description (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_description_addr
* 
* Get the address of the description field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
extern const void *
    obj_get_description_addr (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_reference
* 
* Get the reference field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG reference string for this object
*********************************************************************/
extern const xmlChar *
    obj_get_reference (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_reference_addr
* 
* Get the reference field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG reference string for this object
*********************************************************************/
extern const void *
    obj_get_reference_addr (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_config
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
#define obj_is_config obj_get_config_flag_deep


/********************************************************************
* FUNCTION obj_get_config_flag
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
extern boolean
    obj_get_config_flag (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_config_flag2
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*   setflag == address of return config-stmt set flag
*
* OUTPUTS:
*   *setflag == TRUE if the config-stmt is set in this
*               node, or if it is a top-level object
*            == FALSE if the config-stmt is inherited from its parent
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
extern boolean
    obj_get_config_flag2 (const obj_template_t *obj,
			  boolean *setflag);


/********************************************************************
* FUNCTION obj_get_max_access
*
* Get the NCX max-access enum for an obj_template_t 
* Return the explicit value or the inherited value
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   ncx_access_t enumeration
*********************************************************************/
extern ncx_access_t
    obj_get_max_access (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_appinfoQ
* 
* Get the appinfoQ for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the appinfoQ for this object
*********************************************************************/
extern dlq_hdr_t *
    obj_get_appinfoQ (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_mustQ
* 
* Get the mustQ for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the mustQ for this object
*********************************************************************/
extern dlq_hdr_t *
    obj_get_mustQ (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_typestr
*
* Get the name of the object type
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   name string for this object type
*********************************************************************/
extern const xmlChar *
    obj_get_typestr (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_datadefQ
*
* Get the datadefQ (or caseQ) if this object has one
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*    pointer to Q of obj_template, or NULL if none
*********************************************************************/
extern dlq_hdr_t *
    obj_get_datadefQ (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_cdatadefQ
*
* Get a const pointer to the datadefQ (or caseQ) if this object has one
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*    pointer to Q of obj_template, or NULL if none
*********************************************************************/
extern const dlq_hdr_t *
    obj_get_cdatadefQ (const obj_template_t *obj);



/********************************************************************
* FUNCTION obj_get_default
* 
* Get the default value for the specified object
* Only OBJ_TYP_LEAF objtype is supported
* If the leaf has nodefault, then the type is checked
* Choice defaults are ignored.
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   pointer to default value string or NULL if none
*********************************************************************/
extern const xmlChar *
    obj_get_default (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_default_case
* 
* Get the default case for the specified OBJ_TYP_CHOICE object
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   pointer to default case object template OBJ_TYP_CASE
*********************************************************************/
extern obj_template_t *
    obj_get_default_case (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_level
* 
* Get the nest level for the specified object
* Top-level is '1'
* Does not count groupings as a level
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   level that this object is located, by checking the parent chain
*********************************************************************/
extern uint32
    obj_get_level (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_has_typedefs
* 
* Check if the object has any nested typedefs in it
* This will obly be called if the object is defined in a
* grouping.
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if any nested typedefs, FALSE otherwise
*********************************************************************/
extern boolean
    obj_has_typedefs (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_typdef
* 
* Get the typdef for the leaf or leaf-list
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the typdef or NULL if this object type does not
*    have a typdef
*********************************************************************/
extern typ_def_t *
    obj_get_typdef (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_ctypdef
* 
* Get the typdef for the leaf or leaf-list : Const version
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the typdef or NULL if this object type does not
*    have a typdef
*********************************************************************/
extern const typ_def_t *
    obj_get_ctypdef (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_basetype
* 
* Get the NCX base type enum for the object type
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    base type enumeration
*********************************************************************/
extern ncx_btype_t
    obj_get_basetype (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_mod_prefix
* 
* Get the module prefix for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod prefix
*********************************************************************/
extern const xmlChar *
    obj_get_mod_prefix (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_mod_xmlprefix
* 
* Get the module prefix for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod XML prefix
*********************************************************************/
extern const xmlChar *
    obj_get_mod_xmlprefix (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_mod_name
* 
* Get the module name for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod prefix
*********************************************************************/
extern const xmlChar *
    obj_get_mod_name (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_mod
* 
* Get the module pointer for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to module
*********************************************************************/
extern ncx_module_t *
    obj_get_mod (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_mod_version
* 
* Get the module version for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod version or NULL if none
*********************************************************************/
extern const xmlChar *
    obj_get_mod_version (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_type_name
* 
* Get the typename for an object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to type name string
*********************************************************************/
extern const xmlChar *
    obj_get_type_name (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_nsid
* 
* Get the namespace ID for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    namespace ID
*********************************************************************/
extern xmlns_id_t
    obj_get_nsid (const obj_template_t *);


/********************************************************************
* FUNCTION obj_get_iqualval
* 
* Get the instance qualifier for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    instance qualifier enumeration
*********************************************************************/
extern ncx_iqual_t
    obj_get_iqualval (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_iqualval_ex
* 
* Get the instance qualifier for this object
*
* INPUTS:
*    obj  == object to check
*    required == value to use for 'is_mandatory()' logic
*
* RETURNS:
*    instance qualifier enumeration
*********************************************************************/
extern ncx_iqual_t
    obj_get_iqualval_ex (obj_template_t  *obj,
			 boolean required);


/********************************************************************
* FUNCTION obj_get_min_elements
* 
* Get the min-elements clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*    minelems == address of return min-elements value
*
* OUTPUTS:
*   *minelems == min-elements value if it is set for this object
*   
* RETURNS:
*    TRUE if min-elements is set, FALSE if not or N/A
*********************************************************************/
extern boolean
    obj_get_min_elements (obj_template_t  *obj,
			  uint32 *minelems);


/********************************************************************
* FUNCTION obj_get_max_elements
* 
* Get the max-elements clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*    maxelems == address of return max-elements value
*
* OUTPUTS:
*   *maxelems == max-elements value if it is set for this object
*
* RETURNS:
*    TRUE if max-elements is set, FALSE if not or N/A
*********************************************************************/
extern boolean
    obj_get_max_elements (obj_template_t  *obj,
			  uint32 *maxelems);


/********************************************************************
* FUNCTION obj_get_units
* 
* Get the units clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to units clause, or NULL if none
*********************************************************************/
extern const xmlChar *
    obj_get_units (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_parent
* 
* Get the parent of the current object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
extern obj_template_t *
    obj_get_parent (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_cparent
* 
* Get the parent of the current object
* CONST POINTER VERSION
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
extern const obj_template_t *
    obj_get_cparent (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_real_parent
* 
* Get the parent of the current object;
* skip OBJ_TYP_CHOICE and OBJ_TYP_CASE
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
extern obj_template_t *
    obj_get_real_parent (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_get_presence_string
*
* Get the present-stmt value, if any
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to string
*   NULL if none
*********************************************************************/
extern const xmlChar *
    obj_get_presence_string (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_presence_string_field
*
* Get the address ot the presence-stmt value, if any
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to address of presence string
*   NULL if none
*********************************************************************/
extern void *
    obj_get_presence_string_field (const obj_template_t *obj);


/********************************************************************
 * FUNCTION obj_get_child_node
 * 
 * Get the correct child node for the specified parent and
 * current XML node
 * complex logic for finding the right module namespace
 * and child node, given the current context
 *
 *
 * INPUTS:
 *    obj == parent object template
 *    chobj == current child node (may be NULL if the
 *             xmlorder param is FALSE
 *     xmlorder == TRUE if should follow strict XML element order
 *              == FALSE if sibling node order errors should be 
 *                 ignored; find child nodes out of order
 *                 and check too-many-instances later
 *    curnode == current XML start or empty node to check
 *    force_modQ == Q of ncx_module_t to check, if set
 *               == NULL and the xmlns registry of module pointers
 *                  will be used instead (except netconf.yang)
 *    rettop == address of return topchild object
 *    retobj == address of return object to use
 *
 * OUTPUTS:
 *    *rettop set to top-level found object if return OK
 *     and currently within a choice
 *    *retobj set to found object if return OK
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    obj_get_child_node (obj_template_t *obj,
			obj_template_t *chobj,
			const xml_node_t *curnode,
			boolean xmlorder,
                        dlq_hdr_t *force_modQ,
			obj_template_t **rettop,
			obj_template_t **retobj);


/********************************************************************
* FUNCTION obj_get_child_count
*
* Get the number of child nodes the object has
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   number of child nodes
*********************************************************************/
extern uint32
    obj_get_child_count (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_default_parm
* 
* Get the ncx:default-parm object for this object
* Only supported for OBJ_TYP_CONTAINER and OBJ_TYP_RPCIO (input)
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the name field, NULL if some error or unnamed
*********************************************************************/
extern obj_template_t * 
    obj_get_default_parm (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_config_flag_deep
*
* get config flag during augment expand
* Get the config flag for an obj_template_t 
* Go all the way up the tree until an explicit
* set node or the root is found
*
* Used by get_list_key because the config flag
* of the parent is not set yet when a key leaf is expanded
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*********************************************************************/
extern boolean
    obj_get_config_flag_deep (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_config_flag_check
*
* get config flag during YANG module checking
* Used by yang_obj.c to make sure ncx:root objects
* are not treated as 'config', like obj_get_config_deep
*
* INPUTS:
*   obj == obj_template to check
*   ingrp == address if in grouping flag
*
* OUTPUTS:
*   *ingrp == TRUE if hit grouping top without finding
*             a definitive answer
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   !!! ignore if *ingrp == TRUE
*********************************************************************/
extern boolean
    obj_get_config_flag_check (const obj_template_t *obj,
                               boolean *ingrp);


/********************************************************************
* FUNCTION obj_get_fraction_digits
* 
* Get the fraction-digits field from the object typdef
*
* INPUTS:
*     obj == object template to  check
*
* RETURNS:
*     number of fixed decimal digits expected (1..18)
*     0 if some error
*********************************************************************/
extern uint8
    obj_get_fraction_digits (const obj_template_t  *obj);

/********************************************************************
* FUNCTION obj_get_first_iffeature
* 
* Get the first if-feature clause (if any) for the specified object
*
* INPUTS:
*     obj == object template to  check
*
* RETURNS:
*     pointer to first if-feature struct
*     NULL if none available
*********************************************************************/
extern const ncx_iffeature_t *
    obj_get_first_iffeature (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_next_iffeature
* 
* Get the next if-feature clause (if any)
*
* INPUTS:
*     iffeature == current iffeature struct
*
* RETURNS:
*     pointer to next if-feature struct
*     NULL if none available
*********************************************************************/
extern const ncx_iffeature_t *
    obj_get_next_iffeature (const ncx_iffeature_t *iffeature);


/********************************************************************
* FUNCTION obj_is_leaf
* 
* Check if object is a proper leaf
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    TRUE if proper leaf
*    FALSE if not
*********************************************************************/
extern boolean
    obj_is_leaf (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_is_leafy
* 
* Check if object is a proper leaf or leaflist 
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    TRUE if proper leaf or leaf-list
*    FALSE if not
*********************************************************************/
extern boolean
    obj_is_leafy (const obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_is_mandatory
*
* Figure out if the obj is YANG mandatory or not
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is mandatory
*   FALSE if object is not mandatory
*********************************************************************/
extern boolean
    obj_is_mandatory (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_mandatory_when_ex
*
* Figure out if the obj is YANG mandatory or not
* Check the when-stmts, not just mandatory-stmt
*
* INPUTS:
*   obj == obj_template to check
*   config_only == TRUE to check config only and ignore non-config
*               == FALSE to check mandatory confoig or non-config
* RETURNS:
*   TRUE if object is mandatory
*   FALSE if object is not mandatory
*********************************************************************/
extern boolean
    obj_is_mandatory_when_ex (obj_template_t *obj,
                              boolean config_only);


/********************************************************************
* FUNCTION obj_is_mandatory_when
*
* Figure out if the obj is YANG mandatory or not
* Check the when-stmts, not just mandatory-stmt
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is mandatory
*   FALSE if object is not mandatory
*********************************************************************/
extern boolean
    obj_is_mandatory_when (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_cloned
*
* Figure out if the obj is a cloned object, inserted via uses
* or augment statements
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is cloned
*   FALSE if object is not cloned
*********************************************************************/
extern boolean
    obj_is_cloned (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_augclone
*
* Figure out if the obj is a cloned object, inserted via an
* augment statement
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is sourced from an augment
*   FALSE if object is not sourced from an augment
*********************************************************************/
extern boolean
    obj_is_augclone (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_refine
*
* Figure out if the obj is a refinement object, within a uses-stmt
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is a refinement
*   FALSE if object is not a refinement
*********************************************************************/
extern boolean
    obj_is_refine (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_data
* 
* Check if the object is defined within data or within a
* notification or RPC instead
* 
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if data object (could be in a grouping or real data)
*   FALSE if defined within notification or RPC (or some error)
*********************************************************************/
extern boolean
    obj_is_data (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_data_db
* 
* Check if the object is some sort of data
* Constrained to only check the config DB objects,
* not any notification or RPC objects
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if data object (could be in a grouping or real data)
*   FALSE if defined within notification or RPC (or some error)
*********************************************************************/
extern boolean
    obj_is_data_db (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_in_rpc
* 
* Check if the object is in an rpc/input section
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if /rpc/input object
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_in_rpc (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_in_rpc_reply
* 
* Check if the object is in an rpc-reply/output section
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if /rpc-reply/output object
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_in_rpc_reply (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_in_notif
* 
* Check if the object is in a notification
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if /notification object
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_in_notif (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_rpc
* 
* Check if the object is an RPC method
* 
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if RPC method
*   FALSE if not an RPC method
*********************************************************************/
extern boolean
    obj_is_rpc (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_notif
* 
* Check if the object is a notification
* 
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if notification
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_notif (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_empty
*
* Check if object was entered in empty fashion:
*   list foo;
*   uses grpx;
*
** INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is empty of subclauses
*   FALSE if object is not empty of subclauses
*********************************************************************/
extern boolean
    obj_is_empty (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_match
* 
* Check if one object is a match in identity with another one
*
* INPUTS:
*    obj1  == first object to match
*    obj2  == second object to match
*
* RETURNS:
*    TRUE is a match, FALSE otherwise
*********************************************************************/
extern boolean
    obj_is_match (const obj_template_t  *obj1,
		  const obj_template_t *obj2);


/********************************************************************
* FUNCTION obj_is_hidden
*
* Check if object is marked as a hidden object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:hidden
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_hidden (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_root
*
* Check if object is marked as a root object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:root
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_root (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_password
*
* Check if object is marked as a password object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:password
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_password (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_xsdlist
*
* Check if object is marked as an XSD list
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:xsdlist
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_xsdlist (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_cli
*
* Check if object is marked as a CLI object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:cli
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_cli (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_key
*
* Check if object is being used as a key leaf within a list
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is a key leaf
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_key (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_abstract
*
* Check if object is being used as an object identifier or error-info
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:abstract
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_abstract (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_xpath_string
*
* Check if object is an XPath string
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:xpath
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_xpath_string (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_schema_instance_string
*
* Check if object is a schema-instance string
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:schema-instance
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_schema_instance_string (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_secure
*
* Check if object is tagged ncx:secure
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:secure
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_secure (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_very_secure
*
* Check if object is tagged ncx:very-secure
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:very-secure
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_very_secure (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_system_ordered
*
* Check if the object is system or user-ordered
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is system ordered
*   FALSE if object is user-ordered
*********************************************************************/
extern boolean
    obj_is_system_ordered (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_np_container
*
* Check if the object is an NP-container
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is an NP-container
*   FALSE if object is not an NP-container
*********************************************************************/
extern boolean
    obj_is_np_container (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_enabled
* 
* Check any if-feature statement that may
* cause the specified object to be invisible
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    TRUE if object is enabled
*    FALSE if any if-features are present and FALSE
*********************************************************************/
extern boolean
    obj_is_enabled (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_single_instance
* 
* Check if the object is a single instance of if it
* allows multiple instances; check all of the
* ancestors if needed
*
* INPUTS:
*    obj == object template to check
* RETURNS:
*    TRUE if object is a single instance object
*    FALSE if multiple instances are allowed
*********************************************************************/
extern boolean
    obj_is_single_instance (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_short_case
* 
* Check if the object is a short case statement
*
* INPUTS:
*    obj == object template to check
*
* RETURNS:
*    TRUE if object is a 1 object case statement
*    FALSE otherwise
*********************************************************************/
extern boolean
    obj_is_short_case (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_top
* 
* Check if the object is top-level object within
* the YANG module that defines it
*
* INPUTS:
*    obj == object template to check
*
* RETURNS:
*    TRUE if obj is a top-level object
*    FALSE otherwise
*********************************************************************/
extern boolean
    obj_is_top (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_ok_for_cli
*
* Figure out if the obj is OK for current CLI implementation
* Top object must be a container
* Child objects must be only choices of leafs,
* plain leafs, or leaf lists are allowed
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is OK for CLI
*   FALSE if object is not OK for CLI
*********************************************************************/
extern boolean
    obj_ok_for_cli (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_has_children
*
* Check if there are any accessible nodes within the object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if there are any accessible children
*   FALSE if no datadb child nodes found
*********************************************************************/
extern boolean
    obj_has_children (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_has_ro_children
*
* Check if there are any accessible read-only nodes within the object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if there are any accessible read-only children
*   FALSE if no datadb read-only child nodes found
*********************************************************************/
extern boolean
    obj_has_ro_children (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_rpc_has_input
*
* Check if the RPC object has any real input children
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if there are any input children
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_rpc_has_input (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_rpc_has_output
*
* Check if the RPC object has any real output children
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if there are any output children
*   FALSE otherwise
*********************************************************************/
extern boolean
    obj_rpc_has_output (obj_template_t *obj);


/********************************************************************
 * FUNCTION obj_has_when_stmts
 * 
 * Check if any when-stmts apply to this object
 * Does not check if they are true, just any when-stmts present
 *
 * INPUTS:
 *    obj == object template to check
 *
 * RETURNS:
 *   TRUE if object has any when-stmts associated with it
 *   FALSE otherwise
 *********************************************************************/
extern boolean
    obj_has_when_stmts (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_new_metadata
* 
* Malloc and initialize the fields in a an obj_metadata_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern obj_metadata_t * 
    obj_new_metadata (void);


/********************************************************************
* FUNCTION obj_free_metadata
* 
* Scrub the memory in a obj_metadata_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    meta == obj_metadata_t data structure to free
*********************************************************************/
extern void 
    obj_free_metadata (obj_metadata_t *meta);


/********************************************************************
* FUNCTION obj_add_metadata
* 
* Add the filled out object metadata definition to the object
*
* INPUTS:
*    meta == obj_metadata_t data structure to add
*    obj == object template to add meta to
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    obj_add_metadata (obj_metadata_t *meta,
		      obj_template_t *obj);


/********************************************************************
* FUNCTION obj_find_metadata
* 
* Find the object metadata definition in the object
*
* INPUTS:
*    obj == object template to check
*    name == name of obj_metadata_t data structure to find
*
* RETURNS:
*    pointer to found entry, NULL if not found
*********************************************************************/
extern obj_metadata_t *
    obj_find_metadata (const obj_template_t *obj,
		       const xmlChar *name);


/********************************************************************
* FUNCTION obj_first_metadata
* 
* Get the first object metadata definition in the object
*
* INPUTS:
*    obj == object template to check
*
* RETURNS:
*    pointer to first entry, NULL if none
*********************************************************************/
extern obj_metadata_t *
    obj_first_metadata (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_next_metadata
* 
* Get the next object metadata definition in the object
*
* INPUTS:
*    meta == current meta object template
*
* RETURNS:
*    pointer to next entry, NULL if none
*********************************************************************/
extern obj_metadata_t *
    obj_next_metadata (const obj_metadata_t *meta);


/********************************************************************
 * FUNCTION obj_sort_children
 * 
 * Check all the child nodes of the specified object
 * and rearrange them into alphabetical order,
 * based on the element local-name.
 *
 * ONLY SAFE TO USE FOR ncx:cli CONTAINERS
 * YANG DATA CONTENT ORDER NEEDS TO BE PRESERVED
 *
 * INPUTS:
 *    obj == object template to reorder
 *********************************************************************/
extern void
    obj_sort_children (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_set_ncx_flags
*
* Check the NCX appinfo extensions and set flags as needed
*
** INPUTS:
*   obj == obj_template to check
*
* OUTPUTS:
*   may set additional bits in the obj->flags field
*
*********************************************************************/
extern void
    obj_set_ncx_flags (obj_template_t *obj);



/********************************************************************
* FUNCTION obj_enabled_child_count
*
* Get the count of the number of enabled child nodes
* for the object template
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   number of enabled child nodes
*********************************************************************/
extern uint32
    obj_enabled_child_count (obj_template_t *obj);


/********************************************************************
* FUNCTION obj_dump_child_list
*
* Dump the object names in a datadefQ -- just child level
* uses log_debug for writing
*
* INPUTS:
*   datadefQ == Q of obj_template_t to dump
*   startindent == start-indent columns
*   indent == indent amount
*********************************************************************/
extern void
    obj_dump_child_list (dlq_hdr_t *datadefQ,
                         uint32  startindent,
                         uint32 indent);


/********************************************************************
* FUNCTION obj_get_keystr
*
* Get the key string for this list object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to key string or NULL if none or not a list
*********************************************************************/
extern const xmlChar *
    obj_get_keystr (obj_template_t *obj);



/********************************************************************
* FUNCTION obj_delete_obsolete
*
* Delete any obsolete child nodes within the specified object subtree
*
* INPUTS:
*   objQ == Q of obj_template to check
*
*********************************************************************/
extern void
    obj_delete_obsolete (dlq_hdr_t  *objQ);


/********************************************************************
* FUNCTION obj_get_altname
*
* Get the alt-name for this object, if any
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to alt-name of NULL if none
*********************************************************************/
extern const xmlChar *
    obj_get_altname (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_get_leafref_targobj
* 
* Get the target object for a leafref leaf or leaf-list
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the target object or NULL if this object type does not
*    have a leafref target object
*********************************************************************/
extern obj_template_t *
    obj_get_leafref_targobj (obj_template_t  *obj);

/********************************************************************
 * Get the target object for an augments object
 * \param obj the object to check
 * \return pointer to the augment context target object
 *   or NULL if this object type does not have an augment target object
 *********************************************************************/
extern obj_template_t *
    obj_get_augment_targobj (obj_template_t  *obj);


/********************************************************************
* FUNCTION obj_is_cli_equals_ok
*
* Check if object is marked as ncx:default-parm-equals-ok
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:default-parm-equals-ok
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_cli_equals_ok (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_sil_delete_children_first
*
* Check if object is marked as ncx:sil-delete-children-first
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked as ncx:sil-delete-children-first
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_sil_delete_children_first (const obj_template_t *obj);


/********************************************************************
 * Add a child object to the specified complex node
 *
 * \param child the obj_template to add
 * \param parent the obj_template of the parent
 *********************************************************************/
extern void 
    obj_add_child (obj_template_t *child, obj_template_t *parent);


/********************************************************************
* FUNCTION obj_is_block_user_create
*
* Check if object is marked as ncx:user-write with create 
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user create access
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_block_user_create (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_block_user_update
*
* Check if object is marked as ncx:user-write with update
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user update access
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_block_user_update (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_is_block_user_delete
*
* Check if object is marked as ncx:user-write with delete
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user delete access
*   FALSE if not
*********************************************************************/
extern boolean
    obj_is_block_user_delete (const obj_template_t *obj);


/********************************************************************
* FUNCTION obj_new_iffeature_ptr
*
* Malloc and initialize a new obj_iffeature_ptr_t struct
*
* INPUTS:
*  iff == iffeature to point at
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
extern obj_iffeature_ptr_t *
    obj_new_iffeature_ptr (ncx_iffeature_t *iff);


/********************************************************************
* FUNCTION obj_free_iffeature_ptr
*
* Free an obj_iffeature_ptr_t struct
*
* INPUTS:
*   iffptr == struct to free
*********************************************************************/
extern void obj_free_iffeature_ptr (obj_iffeature_ptr_t *iffptr);


/********************************************************************
 * Get first if-feature pointer
 *
 * \param obj the obj_template to check
 * \return pointer to first entry or NULL if none
 *********************************************************************/
extern obj_iffeature_ptr_t *
    obj_first_iffeature_ptr (obj_template_t *obj);


/********************************************************************
 * Get the next if-feature pointer
 *
 * \param iffptr the current iffeature ptr struct
 * \return pointer to next entry or NULL if none
 *********************************************************************/
extern obj_iffeature_ptr_t *
    obj_next_iffeature_ptr (obj_iffeature_ptr_t *iffptr);


/********************************************************************
* FUNCTION obj_new_xpath_ptr
*
* Malloc and initialize a new obj_xpath_ptr_t struct
*
* INPUTS:
*   xpath == Xpath PCB to point at
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
extern obj_xpath_ptr_t *
    obj_new_xpath_ptr (struct xpath_pcb_t_ *xpath);


/********************************************************************
* FUNCTION obj_free_xpath_ptr
*
* Free an obj_xpath_ptr_t struct
*
* INPUTS:
*   xptr == struct to free
*********************************************************************/
extern void obj_free_xpath_ptr (obj_xpath_ptr_t *xptr);


/********************************************************************
 * Get first xpath pointer struct
 *
 * \param obj the obj_template to check
 * \return pointer to first entry or NULL if none
 *********************************************************************/
extern obj_xpath_ptr_t *
    obj_first_xpath_ptr (obj_template_t *obj);


/********************************************************************
 * Get the next xpath pointer struct
 *
 * \param xptr the current xpath ptr struct
 * \return pointer to next entry or NULL if none
 *********************************************************************/
extern obj_xpath_ptr_t *
    obj_next_xpath_ptr (obj_xpath_ptr_t *xptr);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_obj */
