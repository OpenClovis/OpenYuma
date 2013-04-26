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
#ifndef _H_val
#define _H_val

/*  FILE: val.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Value Node Basic Support

  Value nodes used in thoughout the system are complex
  'automation depositories', which contain all the glue
  to automate the NETCONF functions.

  Almost all value node variants provide user callback
  hooks for CRUD operations on the node.  The read
  operations are usually driven from centrally stored
  data, unless the value node is a 'virtual' value.

  Basic Value Node Usage:
  -----------------------

  1a) Malloc a new value with val_new_value()
        or
  1b)  Initialize a static val_value_t with val_init_value

  2) Bind the value to an object template:
      val_init_from_template

      or use val_make_simval to combine steps 1a and 2

  3) set simple values with various functions, such as
     val_set_simval

  4) When constructing complex values, use val_add_child
     to add them to the parent

  5a) Use val_free_value to free the memory for a value
 
  5b) Use val_clean_value to clean and reuse a value struct

  Internal Value Nodes
  --------------------

  A special developer-level feature to assign arbitrary
  internal values, not in the encoded format. 
  Not used within the configuration database.
  (More TBD)

  External Value Nodes
  --------------------

  The yangcli program allows the user to assign values
  from user and system defined script variables to a
  value node.    Not used within the configuration database.
  (More TBD)

  Virtual Value Nodes
  -------------------

  If a value node does not store its data locally, then it
  is called a virtual node.  Callback functions are used
  for almost all protocol operation support.


*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
19-dec-05    abb      Begun
21jul08      abb      start obj-based rewrite

*/

#include <xmlstring.h>
#include <time.h>

#include "dlq.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "op.h"
#include "plock_cb.h"
#include "status.h"
#include "typ.h"
#include "xml_util.h"
#include "xmlns.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* max number of concurrent partial locks by the same session */
#define VAL_MAX_PLOCKS  4

/* maximum number of bytes in a number string */
#define VAL_MAX_NUMLEN  NCX_MAX_NUMLEN

/* constants used in generating C and Xpath instance ID strings */
#define VAL_BINDEX_CH     '['
#define VAL_EINDEX_CH     ']'

#define VAL_BENUM_CH      '('
#define VAL_EENUM_CH      ')'
#define VAL_INST_SEPCH    '.'
#define VAL_INDEX_SEPCH   ','
#define VAL_INDEX_CLI_SEPCH   ' '
#define VAL_QUOTE_CH      '\''
#define VAL_DBLQUOTE_CH   '\"'
#define VAL_EQUAL_CH      '='
#define VAL_XPATH_SEPCH   '/'

#define VAL_XPATH_INDEX_SEPSTR (const xmlChar *)"]["
#define VAL_XPATH_INDEX_SEPLEN 2

/* display instead of readl password contents */
#define VAL_PASSWORD_STRING  (const xmlChar *)"****"

/* val_value_t flags field */

/* if set the duplicates-ok test has been done */
#define VAL_FL_DUPDONE   bit0

/* if set the duplicates-ok test was OK */
#define VAL_FL_DUPOK     bit1

/* if set, this value was added by val_add_defaults */
#define VAL_FL_DEFSET    bit2

/* if set, value is actually for an XML attribute */
#define VAL_FL_META      bit3

/* if set, value has been edited or added */
#define VAL_FL_DIRTY     bit4

/* if set, value is a list which has unique-stmt already failed */
#define VAL_FL_UNIDONE   bit5

/* if set, value has been checked to see if it is the default value */
#define VAL_FL_DEFVALSET bit6

/* if set, value is set to the YANG default value;
 * only use if VAL_FL_DEFVALSET is 1
 */
#define VAL_FL_DEFVAL    bit7

/* if set, PDU value had the with-defaults wd:attribute
 * set to true
 */
#define VAL_FL_WITHDEF   bit8

/* if set, value has been deleted or moved and awaiting commit or rollback */
#define VAL_FL_DELETED   bit9

/* if set, there was an edit operation in a descendant node;
 * Used by agt_val_root_check to prune trees for faster processing
 */
#define VAL_FL_SUBTREE_DIRTY bit10

/* set the virtualval lifetime to 3 seconds */
#define VAL_VIRTUAL_CACHE_TIME   3


/* macros to access simple value types */
#define VAL_BOOL(V)    ((V)->v.boo)

#define VAL_EMPTY(V)    ((V)->v.boo)

#define VAL_DOUBLE(V)  ((V)->v.num.d)

#define VAL_STRING(V)     ((V)->v.str)

#define VAL_BINARY(V)     ((V)->v.str)

#define VAL_ENU(V)     (&(V)->v.enu)

#define VAL_ENUM(V)    ((V)->v.enu.val)

#define VAL_ENUM_NAME(V)    ((V)->v.enu.name)

#define VAL_FLAG(V)    ((V)->v.boo)

#define VAL_LONG(V)    ((V)->v.num.l)

#define VAL_INT(V)     ((V)->v.num.i)

#define VAL_INT8(V)    ((int8)((V)->v.num.i))

#define VAL_INT16(V)   ((int16)((V)->v.num.i))

#define VAL_INT32(V)   ((V)->v.num.i)

#define VAL_INT64(V)  ((V)->v.num.l)

#define VAL_STR(V)     ((V)->v.str)

#define VAL_INSTANCE_ID(V) ((V)->v.str)

#define VAL_IDREF(V)    (&(V)->v.idref)

#define VAL_IDREF_NSID(V)    ((V)->v.idref.nsid)

#define VAL_IDREF_NAME(V)    ((V)->v.idref.name)

#define VAL_UINT(V)    ((V)->v.num.u)

#define VAL_UINT8(V)    ((uint8)((V)->v.num.u))

#define VAL_UINT16(V)   ((uint16)((V)->v.num.u))

#define VAL_UINT32(V)   ((V)->v.num.u)

#define VAL_UINT64(V)   ((V)->v.num.ul)

#define VAL_ULONG(V)    ((V)->v.num.ul)

#define VAL_DEC64(V)   ((V)->v.num.dec.val)

#define VAL_LIST(V)    ((V)->v.list)

#define VAL_BITS VAL_LIST

#define VAL_EXTERN(V)  ((V)->v.fname)

#define VAL_IS_DELETED(V) ((V)->flags & VAL_FL_DELETED)

#define VAL_MARK_DELETED(V) (V)->flags |= VAL_FL_DELETED

#define VAL_UNMARK_DELETED(V) (V)->flags &= ~VAL_FL_DELETED


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* one QName for the NCX_BT_IDREF value */
typedef struct val_idref_t_ {
    xmlns_id_t  nsid;
    /* if nsid == INV_ID then this is entire QName */
    xmlChar    *name;
    const ncx_identity_t  *identity;  /* ID back-ptr if found */
} val_idref_t;


/* one set of edit-in-progress variables for one value node */
typedef struct val_editvars_t_ {
    /* these fields are only used in modified values before they are 
     * actually added to the config database (TBD: move into struct)
     * curparent == parent of curnode for merge
     */
    struct val_value_t_  *curparent;      
    //op_editop_t    editop;            /* effective edit operation */
    op_insertop_t  insertop;             /* YANG insert operation */
    xmlChar       *insertstr;          /* saved value or key attr */
    struct xpath_pcb_t_ *insertxpcb;       /* key attr for insert */
    struct val_value_t_ *insertval;                   /* back-ptr */
    boolean        iskey;                     /* T: key, F: value */
    boolean        operset;                  /* nc:operation here */
    void          *pcookie;                /* user pointer cookie */
    int            icookie;                /* user integer cookie */
} val_editvars_t;


/* one value to match one type */
typedef struct val_value_t_ {
    dlq_hdr_t      qhdr;

    /* common fields */
    struct obj_template_t_ *obj;        /* bptr to object def */
    typ_def_t *typdef;              /* bptr to typdef if leaf */
    const xmlChar   *name;                /* back pointer to elname */
    xmlChar         *dname;          /* AND malloced name if needed */
    struct val_value_t_ *parent;       /* back-ptr to parent if any */
    xmlns_id_t     nsid;              /* namespace ID for this node */
    ncx_btype_t    btyp;                 /* base type of this value */

    uint32         flags;                  /* internal status flags */
    ncx_data_class_t dataclass;             /* config or state data */

    /* YANG does not support user-defined meta-data but NCX does.
     * The <edit-config>, <get> and <get-config> operations 
     * use attributes in the RPC parameters, the metaQ is still used
     *
     * The ncx:metadata extension allows optional attributes
     * to be added to object nodes for anyxml, leaf, leaf-list,
     * list, and container nodes.  The config property will
     * be inherited from the object that contains the metadata
     *
     * This is used mostly for RPC input parameters
     * and is strongly discouraged.  Full edit-config
     * support is not provided for metdata
     */
    dlq_hdr_t        metaQ;                      /* Q of val_value_t */

    /* value editing variables */
    val_editvars_t  *editvars;               /* edit-vars from attrs */
    op_editop_t      editop;                 /* needed for all edits */ 
    status_t         res;                       /* validation result */

    /* Used by Agent only:
     * if this field is non-NULL, then the entire value node
     * is actually a placeholder for a dynamic read-only object
     * and all read access is done via this callback function;
     * the real data type is getcb_fn_t *
     */
    void *getcb;

    /* if this field is non-NULL, then a malloced value struct
     * representing the real value retrieved by 
     * val_get_virtual_value, is cached here for <get>/<get-config>
     */
    struct val_value_t_ *virtualval;
    time_t               cachetime;

    /* these fields are used for NCX_BT_LIST */
    struct val_index_t_ *index;   /* back-ptr/flag in use as index */
    dlq_hdr_t       indexQ;    /* Q of val_index_t or ncx_filptr_t */

    /* this field is used for NCX_BT_CHOICE 
     * If set, the object path for this node is really:
     *    $this --> casobj --> casobj.parent --> $this.parent
     * the OBJ_TYP_CASE and OBJ_TYP_CHOICE nodes are skipped
     * inside an XML instance document
     */
    struct obj_template_t_   *casobj;

    /* these fields are for NCX_BT_LEAFREF
     * NCX_BT_INSTANCE_ID, or tagged ncx:xpath 
     * value stored in v union as a string
     */
    struct xpath_pcb_t_            *xpathpcb;

    /* back-ptr to the partial locks that are held
     * against this node
     */
    plock_cb_t  *plock[VAL_MAX_PLOCKS];

    /* union of all the NCX-specific sub-types
     * note that the following invisible constructs should
     * never show up in this struct:
     *     NCX_BT_CHOICE
     *     NCX_BT_CASE
     *     NCX_BT_UNION
     */
    union v_ {
	/* complex types have a Q of val_value_t representing
	 * the child nodes with values
	 *   NCX_BT_CONTAINER
	 *   NCX_BT_LIST
	 */
        dlq_hdr_t   childQ;         

        /* Numeric data types:
	 *   NCX_BT_INT8, NCX_BT_INT16,
	 *   NCX_BT_INT32, NCX_BT_INT64
	 *   NCX_BT_UINT8, NCX_BT_UINT16
	 *   NCX_BT_UINT32, NCX_BT_UINT64
	 *   NCX_BT_DECIMAL64, NCX_BT_FLOAT64 
	 */
	ncx_num_t   num; 

	/* String data types:
	 *   NCX_BT_STRING
	 *   NCX_BT_INSTANCE_ID
	 */
	ncx_str_t  str; 

	val_idref_t idref;

	ncx_binary_t binary;              /* NCX_BT_BINARY */
	ncx_list_t list;      /* NCX_BT_BITS, NCX_BT_SLIST */
	boolean    boo;    /* NCX_BT_EMPTY, NCX_BT_BOOLEAN */
	ncx_enum_t enu;       /* NCX_BT_UNION, NCX_BT_ENUM */
	xmlChar   *fname;                 /* NCX_BT_EXTERN */
	xmlChar   *intbuff;               /* NCX_BT_INTERN */
    } v;
} val_value_t;


/* Struct marking the parsing of an instance identifier
 * The position of this record in the val_value_t indexQ
 * represents the order the identifers were parsed
 * Since table and container data structures must always
 * appear in the specified field order, this will be the
 * same order for all well-formed entries.
 *
 * Each of these records will point to one nested val_value_t
 * record in the val_value_t childQ
 */
typedef struct val_index_t_ {
    dlq_hdr_t     qhdr;
    val_value_t  *val;      /* points to a child node */
} val_index_t;


/* one unique-stmt component test value node */
typedef struct val_unique_t_ {
    dlq_hdr_t     qhdr;
    struct xpath_pcb_t_ *pcb;  // live XPath CB w/ result
} val_unique_t;


/* test callback function to check if a value node 
 * should be cloned 
 *
 * INPUTS:
 *   val == value node to check
 *
 * RETURNS:
 *   TRUE if OK to be cloned
 *   FALSE if not OK to be cloned (skipped instead)
 */
typedef boolean
    (*val_test_fn_t) (const val_value_t *val);


/* child or descendant node search walker function
 *
 * INPUTS:
 *   val == value node found in descendant search
 *   cookie1 == cookie1 value passed to start of walk
 *   cookie2 == cookie2 value passed to start of walk
 *
 * RETURNS:
 *   TRUE if walk should continue
 *   FALSE if walk should terminate 
 */
typedef boolean
    (*val_walker_fn_t) (val_value_t *val,
			void *cookie1,
			void *cookie2);


typedef enum val_dumpvalue_mode_t_ {
    DUMP_VAL_NONE,
    DUMP_VAL_STDOUT,
    DUMP_VAL_LOG,
    DUMP_VAL_ALT_LOG
} val_dumpvalue_mode_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION val_new_value
* 
* Malloc and initialize the fields in a val_value_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern val_value_t *
    val_new_value (void);


/********************************************************************
* FUNCTION val_init_complex
* 
* Initialize the fields in a complex val_value_t
* this is deprecated and should only be called 
* by val_init_from_template
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the malloced struct to initialize
*********************************************************************/
extern void
    val_init_complex (val_value_t *val, 
		      ncx_btype_t btyp);


/********************************************************************
* FUNCTION val_init_virtual
* 
* Special function to initialize a virtual value node
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the malloced struct to initialize
*   cbfn == get callback function to use
*   obj == object template to use
*********************************************************************/
extern void
    val_init_virtual (val_value_t *val,
		      void *cbfn,
		      struct obj_template_t_ *obj);


/********************************************************************
* FUNCTION val_init_from_template
* 
* Initialize a value node from its object template
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the initialized value struct to bind
*   obj == object template to use
*********************************************************************/
extern void
    val_init_from_template (val_value_t *val,
			    struct obj_template_t_ *obj);


/********************************************************************
* FUNCTION val_free_value
* 
* Scrub the memory in a val_value_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    val == val_value_t to delete
*********************************************************************/
extern void 
    val_free_value (val_value_t *val);


/********************************************************************
* FUNCTION val_set_name
* 
* Set (or reset) the name of a value struct
*
* INPUTS:
*    val == val_value_t data structure to check
*    name == name string to set
*    namelen == length of name string
*********************************************************************/
extern void 
    val_set_name (val_value_t *val,
		  const xmlChar *name,
		  uint32 namelen);


/********************************************************************
* FUNCTION val_force_dname
* 
* Set (or reset) the name of a value struct
* Set all descendant nodes as well
* Force dname to be used, not object name backptr
*
* INPUTS:
*    val == val_value_t data structure to check
*    name == name string to set
*    namelen == length of name string
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_force_dname (val_value_t *val);


/********************************************************************
* FUNCTION val_set_qname
* 
* Set (or reset) the name and namespace ID of a value struct
*
* INPUTS:
*    val == val_value_t data structure to check
*    nsid == namespace ID to set
*    name == name string to set
*    namelen == length of name string
*********************************************************************/
extern void 
    val_set_qname (val_value_t *val,
		   xmlns_id_t   nsid,
		   const xmlChar *name,
		   uint32 namelen);


/********************************************************************
* FUNCTION val_string_ok
* 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_string_ok (typ_def_t *typdef,
		   ncx_btype_t  btyp,
		   const xmlChar *strval);


/********************************************************************
* FUNCTION val_string_ok_errinfo
* 
* retrieve the YANG custom error info if any 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
* Retrieve the configured error info struct if any error
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*    errinfo == address of return errinfo block (may be NULL)
*
* OUTPUTS:
*   if non-NULL: 
*       *errinfo == error record to use if return error
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_string_ok_errinfo (typ_def_t *typdef,
			   ncx_btype_t  btyp,
			   const xmlChar *strval,
			   ncx_errinfo_t **errinfo);


/********************************************************************
* FUNCTION val_string_ok_ex
* 
* retrieve the YANG custom error info if any 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
* Retrieve the configured error info struct if any error
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*    errinfo == address of return errinfo block (may be NULL)
*    logerrors == TRUE to log errors
*              == FALSE to not log errors (use for NCX_BT_UNION)
* OUTPUTS:
*   if non-NULL: 
*       *errinfo == error record to use if return error
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_string_ok_ex (typ_def_t *typdef,
                      ncx_btype_t btyp,
                      const xmlChar *strval,
                      ncx_errinfo_t **errinfo,
                      boolean logerrors);


/********************************************************************
* FUNCTION val_list_ok
* 
* Check a list to make sure the all the strings are valid based
* on the specified typdef
*
* validate all the ncx_lmem_t entries in the list
* against the specified typdef.  Mark any errors
* in the ncx_lmem_t flags field of each member
* in the list with an error
*
* INPUTS:
*    typdef == typ_def_t for the designated list type
*    btyp == base type (NCX_BT_SLIST or NCX_BT_BITS)
*    list == list struct with ncx_lmem_t structs to check
*
* OUTPUTS:
*   If return other than NO_ERR:
*     each list->lmem.flags field may contain bits set
*     for errors:
*        NCX_FL_RANGE_ERR: size out of range
*        NCX_FL_VALUE_ERR  value not permitted by value set, 
*                          or pattern
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_list_ok (typ_def_t *typdef,
		 ncx_btype_t btyp,
		 ncx_list_t *list);


/********************************************************************
* FUNCTION val_list_ok_errinfo
* 
* Check a list to make sure the all the strings are valid based
* on the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated list type
*    btyp == base type (NCX_BT_SLIST or NCX_BT_BITS)
*    list == list struct with ncx_lmem_t structs to check
*    errinfo == address of return rpc-error info struct
*
* OUTPUTS:
*   If return other than NO_ERR:
*     *errinfo contains the YANG specified error info, if any*   
*     each list->lmem.flags field may contain bits set
*     for errors:
*        NCX_FL_RANGE_ERR: size out of range
*        NCX_FL_VALUE_ERR  value not permitted by value set, 
*                          or pattern
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_list_ok_errinfo (typ_def_t *typdef,
			 ncx_btype_t btyp,
			 ncx_list_t *list,
			 ncx_errinfo_t **errinfo);

  
/********************************************************************
* FUNCTION val_enum_ok
* 
* Check an enumerated integer string to make sure the value 
* is valid based on the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    enumval == enum string value to check
*    retval == pointer to return integer variable
*    retstr == pointer to return string name variable
* 
* OUTPUTS:
*    *retval == integer value of enum
*    *retstr == pointer to return string name variable 
* 
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_enum_ok (typ_def_t *typdef,
		 const xmlChar *enumval,
		 int32 *retval,
		 const xmlChar **retstr);


/********************************************************************
* FUNCTION val_bit_ok
* 
* Check a bit name is valid for the typedef
*
* INPUTS:
*    typdef == typ_def_t for the designated bits type
*    bitname == bit name value to check
*    position == address of return bit struct position value
*
* OUTPUTS:
*  if non-NULL:
*     *position == bit position value
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_bit_ok (typ_def_t *typdef,
		const xmlChar *bitname,
		uint32 *position);


/********************************************************************
* FUNCTION val_idref_ok
* 
* Check if an identityref QName is valid for the typedef
* The QName must match an identity that has the same base
* as specified in the typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated identityref type
*    qname == QName or local-name string to check
*    nsid == namespace ID from XML node for NS of QName
*            this NSID will be used and not the prefix in qname
*            it was parsed in the XML node and is not a module prefix
*    name == address of return local name part of QName
*    id == address of return identity, if found
*
* OUTPUTS:
*  if non-NULL:
*     *name == pointer into the qname string at the start of
*              the local name part
*     *id == pointer to ncx_identity_t found
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_idref_ok (typ_def_t *typdef,
		  const xmlChar *qname,
		  xmlns_id_t nsid,
		  const xmlChar **name,
		  const ncx_identity_t **id);


/********************************************************************
* FUNCTION val_parse_idref
* 
* Parse a CLI BASED identityref QName into its various parts
*
* INPUTS:
*    mod == module containing the default-stmt (or NULL if N/A)
*    qname == QName or local-name string to parse
*    nsid == address of return namespace ID of the module
*            indicated by the prefix. If mod==NULL then
*            a prefix MUST be present
*    name == address of return local name part of QName
*    id == address of return identity, if found
*
* OUTPUTS:
*  if non-NULL:
*     *nsid == namespace ID for the prefix part of the QName
*     *name == pointer into the qname string at the start of
*              the local name part
*     *id == pointer to ncx_identity_t found (if any, not an error)
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_parse_idref (ncx_module_t *mod,
		     const xmlChar *qname,
		     xmlns_id_t  *nsid,
		     const xmlChar **name,
		     const ncx_identity_t **id);


/********************************************************************
* FUNCTION val_range_ok
* 
* Check a number to see if it is in range or not
* Could be a number or size range
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    btyp == base type of num
*    num == number to check
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_range_ok (typ_def_t *typdef,
		  ncx_btype_t  btyp,
		  const ncx_num_t *num);


/********************************************************************
* FUNCTION val_range_ok_errinfo
* 
* Check a number to see if it is in range or not
* Could be a number or size range
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    btyp == base type of num
*    num == number to check
*    errinfo == address of return error struct
*
* OUTPUTS:
*   if non-NULL:
*     *errinfo == errinfo record on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_range_ok_errinfo (typ_def_t *typdef,
			  ncx_btype_t  btyp,
			  const ncx_num_t *num,
			  ncx_errinfo_t **errinfo);


/********************************************************************
* FUNCTION val_pattern_ok
* 
* Check a string against all the patterns in a big AND expression
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    strval == string value to check
* 
* RETURNS:
*    NO_ERR if pattern OK or no patterns found to check; error otherwise
*********************************************************************/
extern status_t
    val_pattern_ok (typ_def_t *typdef,
		    const xmlChar *strval);


/********************************************************************
* FUNCTION val_pattern_ok_errinfo
* 
* Check a string against all the patterns in a big AND expression
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    strval == string value to check
*    errinfo == address of return errinfo struct for err-pattern
*
* OUTPUTS:
*   *errinfo set to error info struct if any, and if error exit
*
* RETURNS:
*    NO_ERR if pattern OK or no patterns found to check; 
*    error otherwise, and *errinfo will be set if the pattern
*    that failed has any errinfo defined in it
*********************************************************************/
extern status_t
    val_pattern_ok_errinfo (typ_def_t *typdef,
			    const xmlChar *strval,
			    ncx_errinfo_t **errinfo);


/********************************************************************
* FUNCTION val_simval_ok
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_simval_ok (typ_def_t *typdef,
		   const xmlChar *simval);
		   


/********************************************************************
* FUNCTION val_simval_ok_errinfo
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_simval_ok_errinfo (typ_def_t *typdef,
			   const xmlChar *simval,
			   ncx_errinfo_t **errinfo);


/********************************************************************
* FUNCTION val_simval_ok_ex
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*    mod == module in progress to use for idref and other
*           strings with prefixes in them
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_simval_ok_ex (typ_def_t *typdef,
                      const xmlChar *simval,
                      ncx_errinfo_t **errinfo,
                      ncx_module_t *mod);


/********************************************************************
* FUNCTION val_simval_ok_max
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*    mod == module in progress to use for idref and other
*           strings with prefixes in them
*    logerrors == TRUE to log errors; FALSE to not log errors
*                (use FALSE for NCX_BT_UNION)
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_simval_ok_max (typ_def_t *typdef,
                       const xmlChar *simval,
                       ncx_errinfo_t **errinfo,
                       ncx_module_t *mod,
                       boolean logerrors);


/********************************************************************
* FUNCTION val_union_ok
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_union_ok (typ_def_t *typdef,
		  const xmlChar *strval,
		  val_value_t *retval);


/********************************************************************
* FUNCTION val_union_ok_errinfo
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*    errinfo == address of error struct
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*   *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_union_ok_errinfo (typ_def_t *typdef,
			  const xmlChar *strval,
			  val_value_t *retval,
			  ncx_errinfo_t **errinfo);


/********************************************************************
* FUNCTION val_union_ok_ex
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*    errinfo == address of error struct
*    mod == module in progress, if any
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*   *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_union_ok_ex (typ_def_t *typdef,
                     const xmlChar *strval,
                     val_value_t *retval,
                     ncx_errinfo_t **errinfo,
                     ncx_module_t *mod);


/********************************************************************
* FUNCTION val_get_metaQ
* 
* Get the meta Q header for the value
* 
* INPUTS:
*    val == value node to check
*
* RETURNS:
*   pointer to the metaQ for this value
*********************************************************************/
extern dlq_hdr_t *
    val_get_metaQ (val_value_t  *val);


/********************************************************************
* FUNCTION val_get_first_meta
* 
* Get the first metaQ entry from the specified Queue
* 
* INPUTS:
*    queue == queue of meta-vals to check
*
* RETURNS:
*   pointer to the first meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
extern val_value_t *
    val_get_first_meta (dlq_hdr_t *queue);


/********************************************************************
* FUNCTION val_get_first_meta_val
* 
* Get the first metaQ entry from the specified Queue
* 
* INPUTS:
*    value node to get the metaQ from
*
* RETURNS:
*   pointer to the first meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
extern val_value_t *
    val_get_first_meta_val (val_value_t *val);


/********************************************************************
* FUNCTION val_get_next_meta
* 
* Get the next metaQ entry from the specified entry
* 
* INPUTS:
*    curnode == current meta-var node
*
* RETURNS:
*   pointer to the next meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
extern val_value_t *
    val_get_next_meta (val_value_t *curmeta);


/********************************************************************
* FUNCTION val_meta_empty
* 
* Check if the metaQ is empty for the value node
*
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the metaQ for the value is empty
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_meta_empty (val_value_t *val);


/********************************************************************
* FUNCTION val_find_meta
* 
* Get the corresponding meta data node 
* 
* INPUTS:
*    val == value to check for metadata
*    nsid == namespace ID of 'name'; 0 == don't use
*    name == name of metadata variable name
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_find_meta (val_value_t *val,
		   xmlns_id_t   nsid,
		   const xmlChar *name);


/********************************************************************
* FUNCTION val_meta_match
* 
* Return true if the corresponding attribute exists and has
* the same value 
* 
* INPUTS:
*    val == value to check for metadata
*    metaval == value to match in the val->metaQ 
*
* RETURNS:
*   TRUE if the specified attr if found and has the same value
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_meta_match (val_value_t *val,
		    val_value_t *metaval);


/********************************************************************
* FUNCTION val_metadata_inst_count
* 
* Get the number of instances of the specified attribute
*
* INPUTS:
*     val == value to check for metadata instance count
*     nsid == namespace ID of the meta data variable
*     name == name of the meta data variable
*
* RETURNS:
*   number of instances found in val->metaQ
*********************************************************************/
extern uint32
    val_metadata_inst_count (val_value_t  *val,
			     xmlns_id_t nsid,
			     const xmlChar *name);


/********************************************************************
* FUNCTION val_dump_value
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
extern void
    val_dump_value (val_value_t *val,
		    int32 startindent);


/********************************************************************
* FUNCTION val_dump_value_ex
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*    display_mode == display mode to use
*********************************************************************/
extern void
    val_dump_value_ex (val_value_t *val,
                       int32 startindent,
                       ncx_display_mode_t display_mode);


/********************************************************************
* FUNCTION val_dump_alt_value
* 
* Printf the specified val_value_t struct to 
* the alternate logfile
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
extern void
    val_dump_alt_value (val_value_t *val,
			int32 startindent);


/********************************************************************
* FUNCTION val_stdout_value
* 
* Printf the specified val_value_t struct to stdout
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
extern void
    val_stdout_value (val_value_t *val,
		      int32 startindent);

/********************************************************************
* FUNCTION val_stdout_value_ex
* 
* Printf the specified val_value_t struct to stdout
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*    display_mode == display mode to use
*********************************************************************/
extern void
    val_stdout_value_ex (val_value_t *val,
                         int32 startindent,
                         ncx_display_mode_t display_mode);


/********************************************************************
* FUNCTION val_dump_value_max
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to dump
*    startindent == start indent char count
*    indent_amount == number of spaces for each indent
*    dumpmode == logging mode to use
*    display_mode == formatting mode for display
*    with_meta == TRUE if metaQ should be printed
*                 FALSE to skip meta data
*    configonly == TRUE if config only nodes should be displayed
*                  FALSE if all nodes should be displayed
*********************************************************************/
extern void
    val_dump_value_max (val_value_t *val,
                        int32 startindent,
                        int32 indent_amount,
                        val_dumpvalue_mode_t dumpmode,
                        ncx_display_mode_t display_mode,
                        boolean with_meta,
                        boolean configonly);


/********************************************************************
* FUNCTION val_set_string
* 
* set a generic string using the builtin string typdef
* Set an initialized val_value_t as a simple type
* namespace set to 0 !!!
* use after calling val_new_value
*
* INPUTS:
*    val == value to set
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
extern status_t 
    val_set_string (val_value_t  *val,
		    const xmlChar *valname,
		    const xmlChar *valstr);


/********************************************************************
* FUNCTION val_set_string2
* 
* set a string with any typdef
* Set an initialized val_value_t as a simple type
* namespace set to 0 !!!
*
* Will check if the string is OK for the typdef!
*
* INPUTS:
*    val == value to set
*    valname == name of simple value
*    typdef == parm typdef (may be NULL)
*    valstr == simple value encoded as a string
*    valstrlen == length of valstr to use
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    val_set_string2 (val_value_t  *val,
		     const xmlChar *valname,
		     typ_def_t *typdef,
		     const xmlChar *valstr,
		     uint32 valstrlen);


/********************************************************************
* FUNCTION val_reset_empty
* 
* Recast an already initialized value as an NCX_BT_EMPTY
* clean a value and set it to empty type
* used by yangcli to delete leafs
*
* INPUTS:
*    val == value to set
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
extern status_t 
    val_reset_empty (val_value_t  *val);


/********************************************************************
* FUNCTION val_set_simval
* 
* set any simple value with any typdef
* Set an initialized val_value_t as a simple type
*
* INPUTS:
*    val == value to set
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
extern status_t 
    val_set_simval (val_value_t  *val,
		    typ_def_t *typdef,
		    xmlns_id_t    nsid,
		    const xmlChar *valname,
		    const xmlChar *valstr);


/********************************************************************
* FUNCTION val_set_simval_str
* 
* set any simple value with any typdef, and a counted string
* Set an initialized val_value_t as a simple type
*
* The string value will be converted to a value
* struct format and checked against the provided typedef
*
* Handles the following data types:
* 
*  NCX_BT_INT8
*  NCX_BT_INT16
*  NCX_BT_INT32
*  NCX_BT_INT64
*  NCX_BT_UINT8
*  NCX_BT_UINT16
*  NCX_BT_UINT32
*  NCX_BT_UINT64
*  NCX_BT_DECIMAL64
*  NCX_BT_FLOAT64
*  NCX_BT_BINARY
*  NCX_BT_STRING
*  NCX_BT_INSTANCE_ID
*  NCX_BT_ENUM
*  NCX_BT_EMPTY
*  NCX_BT_BOOLEAN
*  NCX_BT_SLIST
*  NCX_BT_BITS
*  NCX_BT_UNION
*  NCX_BT_LEAFREF
*  NCX_BT_IDREF
*
* INPUTS:
*    val == value to set
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valnamelen == length of name string to compare
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
extern status_t 
    val_set_simval_str (val_value_t  *val,
			typ_def_t *typdef,
			xmlns_id_t    nsid,
			const xmlChar *valname,
			uint32 valnamelen,
			const xmlChar *valstr);


/********************************************************************
* FUNCTION val_make_simval
* 
* Create and set a val_value_t as a simple type
* same as val_set_simval, but malloc the value first
*
* INPUTS:
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valstr == simple value encoded as a string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced and filled in val_value_t struct
*    NULL if some error
*********************************************************************/
extern val_value_t *
    val_make_simval (typ_def_t *typdef,
		     xmlns_id_t    nsid,
		     const xmlChar *valname,
		     const xmlChar *valstr,
		     status_t  *res);


/********************************************************************
* FUNCTION val_make_string
* 
* Malloc and set a val_value_t as a generic NCX_BT_STRING
* namespace set to 0 !!!
*
* INPUTS:
*    nsid == namespace ID to use
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* RETURNS:
*   malloced val struct filled in; NULL if malloc or strdup failed
*********************************************************************/
extern val_value_t *
    val_make_string (xmlns_id_t nsid,
		     const xmlChar *valname,
		     const xmlChar *valstr);


/********************************************************************
* FUNCTION val_merge
* 
* Merge src val into dest val (! MUST be same type !)
* Any meta vars in src are also merged into dest
*
* This function is not used to merge complex objects
* !!! For typ_is_simple() only !!!
*
* INPUTS:
*    src == val to merge from
*    dest == val to merge into
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_merge (const val_value_t *src,
	       val_value_t *dest);


/********************************************************************
* FUNCTION val_clone
* 
* Clone a specified val_value_t struct and sub-trees
*
* INPUTS:
*    val == value to clone
*
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
extern val_value_t *
    val_clone (const val_value_t *val);


/********************************************************************
* FUNCTION val_clone2
* 
* Clone a specified val_value_t struct and sub-trees
* but not the editvars
*
* INPUTS:
*    val == value to clone
*   
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
extern val_value_t *
    val_clone2 (const val_value_t *val);


/********************************************************************
* FUNCTION val_clone_config_data
* 
* Clone a specified val_value_t struct and sub-trees
* Filter with the val_is_config_data callback function
* pass in a config node, such as <config> root
* will call val_clone_test with the val_is_config_data
* callbacck function
*
* INPUTS:
*    val == config data value to clone
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
extern val_value_t *
    val_clone_config_data (const val_value_t *val,
			   status_t *res);


/********************************************************************
* FUNCTION val_replace
* 
* Replace a specified val_value_t struct and sub-trees
* !!! this can be destructive to the source 'val' parameter !!!!
*
* INPUTS:
*    val == value to clone from
*    copy == address of value to replace
*
* OUTPUTS:
*   *copy has been deleted and reforms with the contents of 'val'
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_replace (val_value_t *val,
		 val_value_t *copy);


/********************************************************************
* FUNCTION val_replace_str
* 
* Replace a specified val_value_t struct with a string type
*
* INPUTS:
*    str == value to clone from; may be NULL
*    stringlen == number of chars to use from str, if not NULL
*    copy == address of value to replace
*
* OUTPUTS:
*   *copy has been deleted and reforms with the contents of 'val'
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_replace_str (const xmlChar *str,
                     uint32 stringlen,
                     val_value_t *copy);


/********************************************************************
* FUNCTION val_add_child
* 
*   Add a child value node to a parent value node
*   Simply makes a new last child!!!
*   Does not check siblings!!!  
*   Relies on val_set_canonical_order
*
*   To modify existing extries, use val_add_child_sorted instead!!
*
* INPUTS:
*    child == node to store in the parent
*    parent == complex value node with a childQ
*
*********************************************************************/
extern void
    val_add_child (val_value_t *child,
		   val_value_t *parent);


/********************************************************************
* FUNCTION val_add_child_sorted
* 
*   Add a child value node to a parent value node
*   in the proper place
*
* INPUTS:
*    child == node to store in the parent
*    parent == complex value node with a childQ
*
*********************************************************************/
extern void
    val_add_child_sorted (val_value_t *child,
                          val_value_t *parent);


/********************************************************************
* FUNCTION val_insert_child
* 
*   Insert a child value node to a parent value node
*
* INPUTS:
*    child == node to store in the parent
*    current == current node to insert after; 
*               NULL to make new first entry
*    parent == complex value node with a childQ
*
*********************************************************************/
extern void
    val_insert_child (val_value_t *child,
		      val_value_t *current,
		      val_value_t *parent);


/********************************************************************
* FUNCTION val_remove_child
* 
*   Remove a child value node from its parent value node
*
* INPUTS:
*    child == node to store in the parent
*
*********************************************************************/
extern void
    val_remove_child (val_value_t *child);


/********************************************************************
* FUNCTION val_swap_child
* 
*   Swap a child value node with a current value node
*
* INPUTS:
*    newchild == node to store in the place of the curchild
*    curchild == node to remove from the parent
*
*********************************************************************/
extern void
    val_swap_child (val_value_t *newchild,
		    val_value_t *curchild);


/********************************************************************
* FUNCTION val_first_child_match
* 
* Get the first instance of the corresponding child node 
* 
* INPUTS:
*    parent == parent value to check
*    child == child value to find (e.g., from a NETCONF PDU) 
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_first_child_match (val_value_t *parent,
			   val_value_t *child);


/********************************************************************
* FUNCTION val_next_child_match
* 
* Get the next instance of the corresponding child node 
* 
* INPUTS:
*    parent == parent value to check
*    child == child value to find (e.g., from a NETCONF PDU) 
*    curmatch == current child of parent that matched
*                
* RETURNS:
*   pointer to the next child match if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_next_child_match (val_value_t *parent,
			  val_value_t *child,
			  val_value_t *curmatch);


/********************************************************************
* FUNCTION val_get_first_child
* 
* Get the child node
* 
* INPUTS:
*    parent == parent complex type to check
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_get_first_child (const val_value_t *parent);


/********************************************************************
* FUNCTION val_get_next_child
* 
* Get the next child node
* 
* INPUTS:
*    curchild == current child node
*
* RETURNS:
*   pointer to the next child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_get_next_child (const val_value_t *curchild);


/********************************************************************
* FUNCTION val_find_child
* 
* Find the first instance of the specified child node
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_find_child (const val_value_t  *parent,
		    const xmlChar  *modname,
		    const xmlChar *childname);


/********************************************************************
* FUNCTION val_find_child_que
* 
* Find the first instance of the specified child node in the
* specified child Q
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_find_child_que (const dlq_hdr_t *childQ,
                        const xmlChar *modname,
                        const xmlChar *childname);


/********************************************************************
* FUNCTION val_match_child
* 
* Match the first instance of the specified child node
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_match_child (const val_value_t  *parent,
		     const xmlChar  *modname,
		     const xmlChar *childname);


/********************************************************************
* FUNCTION val_find_next_child
* 
* Find the next instance of the specified child node
* 
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*    curchild == current child of this object type to start search
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
extern val_value_t *
    val_find_next_child (const val_value_t  *parent,
			 const xmlChar  *modname,
			 const xmlChar *childname,
			 const val_value_t *curchild);


/********************************************************************
* FUNCTION val_first_child_name
* 
* Get the first corresponding child node instance, by name
* find first -- really for resolve index function
* 
* INPUTS:
*    parent == parent complex type to check
*    name == child name to find
*
* RETURNS:
*   pointer to the FIRST match if found, or NULL if not found
*********************************************************************/
extern val_value_t *
    val_first_child_name (val_value_t *parent,
			  const xmlChar *name);


/********************************************************************
* FUNCTION val_first_child_qname
* 
* Get the first corresponding child node instance, by QName
* 
* INPUTS:
*    parent == parent complex type to check
*    nsid == namespace ID to use, 0 for any
*    name == child name to find
*
* RETURNS:
*   pointer to the first match if found, or NULL if not found
*********************************************************************/
extern val_value_t *
    val_first_child_qname (val_value_t *parent,
			   xmlns_id_t   nsid,
			   const xmlChar *name);


/********************************************************************
* FUNCTION val_next_child_qname
* 
* Get the next corresponding child node instance, by QName
* 
* INPUTS:
*    parent == parent complex type to check
*    nsid == namespace ID to use, 0 for any
*    name == child name to find
*    curchild == current child match to start from
*
* RETURNS:
*   pointer to the next match if found, or NULL if not found
*********************************************************************/
extern val_value_t *
    val_next_child_qname (val_value_t *parent,
			  xmlns_id_t   nsid,
			  const xmlChar *name,
			  val_value_t *curchild);


/********************************************************************
* FUNCTION val_first_child_string
* 
* find first name value pair
* Get the first corresponding child node instance, by name
* and by string value.
* Child node must be a base type of 
*   NCX_BT_STRING
*   NCX_BT_INSTANCE_ID
*   NCX_BT_LEAFREF
*
* INPUTS:
*    parent == parent complex type to check
*    name == child name to find
*    strval == string value to find 
* RETURNS:
*   pointer to the FIRST match if found, or NULL if not found
*********************************************************************/
extern val_value_t *
    val_first_child_string (val_value_t *parent,
			    const xmlChar *name,
			    const xmlChar *strval);


/********************************************************************
* FUNCTION val_child_cnt
* 
* Get the number of child nodes present
* get number of child nodes present -- for choice checking
* 
* INPUTS:
*    parent == parent complex type to check
*
* RETURNS:
*   the number of child nodes found
*********************************************************************/
extern uint32
    val_child_cnt (val_value_t *parent);


/********************************************************************
* FUNCTION val_child_inst_cnt
* 
* Get the corresponding child instance count by name
* get instance count -- for instance qualifer checking
* 
* INPUTS:
*    parent == parent complex type to check
*    modname == module name defining the child (may be NULL)
*    name == child name to find and count
*
* RETURNS:
*   the instance count
*********************************************************************/
extern uint32
    val_child_inst_cnt (const val_value_t *parent,
			const xmlChar *modname,
			const xmlChar *name);


/********************************************************************
* FUNCTION val_find_all_children
* 
* Find all occurances of the specified node(s)
* within the children of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == node to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of child node to find
*              == NULL to match any child name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    val_find_all_children (val_walker_fn_t walkerfn,
			   void *cookie1,
			   void *cookie2,
			   val_value_t *startnode,
			   const xmlChar *modname,
			   const xmlChar *name,
			   boolean configonly,
			   boolean textmode);


/********************************************************************
* FUNCTION val_find_all_ancestors
* 
* Find all the ancestor instances of the specified node
* within the path to root from the current node;
* use the filter criteria provided
*
* INPUTS:
*
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == node to start search at
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of ancestor node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    val_find_all_ancestors (val_walker_fn_t walkerfn,
			    void *cookie1,
			    void *cookie2,
			    val_value_t *startnode,
			    const xmlChar *modname,
			    const xmlChar *name,
			    boolean configonly,			    
			    boolean textmode,
			    boolean orself);


/********************************************************************
* FUNCTION val_find_all_descendants
* 
* Find all occurances of the specified node
* within the current subtree. The walker fn will
* be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == startnode complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of descendant node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    forceall == TRUE to invoke the descendant callbacks
*                even if fncalled was true from the current
*               (parent) node;  FALSE to skip descendants
*               if fncalled was TRUE
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    val_find_all_descendants (val_walker_fn_t walkerfn,
			      void *cookie1,
			      void *cookie2,
			      val_value_t *startnode,
			      const xmlChar *modname,
			      const xmlChar *name,
			      boolean configonly,
			      boolean textmode,
			      boolean orself,
			      boolean forceall);


/********************************************************************
* FUNCTION val_find_all_pfaxis
* 
* Find all occurances of the specified node
* for the specified preceding or following axis
*
*   preceding::*
*   following::*
*
* within the current subtree. The walker fn will
* be called for each match.  Because the callbacks
* will be done in sequential order, starting from
* the 
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    topnode == topnode used as the relative root for
*               calculating node position
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 1 level is checked, not their descendants
*    textmode == TRUE if just testing for text() nodes
*                name modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    val_find_all_pfaxis (val_walker_fn_t walkerfn,
			 void *cookie1,
			 void *cookie2,
			 val_value_t *startnode,
			 const xmlChar *modname,
			 const xmlChar *name,
			 boolean configonly,
			 boolean dblslash,
			 boolean textmode,
			 ncx_xpath_axis_t axis);
			      

/********************************************************************
* FUNCTION val_find_all_pfsibling_axis
* 
* Find all occurances of the specified node
* for the specified axis
*
*   preceding-sibling::*
*   following-sibling::*
*
* within the current subtree. The walker fn will
* be called for each match.  Because the callbacks
* will be done in sequential order, starting from
* the 
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == starting sibling node to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
extern boolean
    val_find_all_pfsibling_axis (val_walker_fn_t  walkerfn,
				 void *cookie1,
				 void *cookie2,
				 val_value_t *startnode,
				 const xmlChar *modname,
				 const xmlChar *name,
				 boolean configonly,
				 boolean dblslash,
				 boolean textmode,
				 ncx_xpath_axis_t axis);


/********************************************************************
* FUNCTION val_get_axisnode
* 
* Find the specified node based on the context node,
* a context position and an axis
*
*   ancestor::*
*   ancestor-or-self::*
*   child::*
*   descendant::*
*   descendant-or-self::*
*   following::*
*   following-sibling::*
*   preceding::*
*   preceding-sibling::*
*   self::*
*
* INPUTS:
*    startnode == context node to run tests from
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*    position == position to find in the specified axis
*
* RETURNS:
*   pointer to found value or NULL if none
*********************************************************************/
extern val_value_t *
    val_get_axisnode (val_value_t *startnode,
		      const xmlChar *modname,
		      const xmlChar *name,
		      boolean configonly,
		      boolean dblslash,
		      boolean textmode,
		      ncx_xpath_axis_t axis,
		      int64 position);


/********************************************************************
* FUNCTION val_get_child_inst_id
* 
* Get the instance ID for this child node within the parent context
* 
* INPUTS:
*    parent == parent complex type to check
*    child == child node to find ID for
*
* RETURNS:
*   the instance ID num (1 .. N), or 0 if some error
*********************************************************************/
extern uint32
    val_get_child_inst_id (const val_value_t *parent,
			   const val_value_t *child);


/********************************************************************
* FUNCTION val_liststr_count
* 
* Get the number of strings in the list type
* 
* INPUTS:
*    val == value to check
*
* RETURNS:
*  number of list entries; also zero for error
*********************************************************************/
extern uint32
    val_liststr_count (const val_value_t *val);


/********************************************************************
* FUNCTION val_index_match
* 
* Check 2 val_value structs for the same instance ID
* 
* The node data types must match, and must be
*    NCX_BT_LIST
*
* All index components must exactly match.
* 
* INPUTS:
*    val1 == first value to index match
*    val2 == second value to index match
*
* RETURNS:
*   TRUE if the index chains match
*********************************************************************/
extern boolean
    val_index_match (const val_value_t *val1,
		     const val_value_t *val2);


/********************************************************************
* FUNCTION val_index_compare
* 
* Check 2 val_value structs for the same instance ID
* 
* The node data types must match, and must be
*    NCX_BT_LIST
*
* INPUTS:
*    val1 == first value to index match
*    val2 == second value to index match
*
* RETURNS:
*   -1 , - or 1 for compare value
*********************************************************************/
extern int
    val_index_compare (const val_value_t *val1,
                       const val_value_t *val2);


/********************************************************************
* FUNCTION val_compare_max
* 
* Compare 2 val_value_t struct value contents
* Check all or config only
* Check just child nodes or all descendant nodes
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*    configonly == TRUE to compare config=true nodes only
*                  FALSE to compare all nodes
*    childonly == TRUE to look just 1 level for comparison
*                 FALSE to compare all descendant nodes of complex types
*    editing == TRUE to compare for editing
*               FALSE to compare just the values, so a set by
*               default and value=default are the same value
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
extern int32
    val_compare_max (const val_value_t *val1,
                     const val_value_t *val2,
                     boolean configonly,
                     boolean childonly,
                     boolean editing);


/********************************************************************
* FUNCTION val_compare_ex
* 
* Compare 2 val_value_t struct value contents
* Check all or config only
*
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*    configonly == TRUE to compare config=true nodes only
*                  FALSE to compare all nodes
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
extern int32
    val_compare_ex (const val_value_t *val1,
                    const val_value_t *val2,
                    boolean configonly);


/********************************************************************
* FUNCTION val_compare
* 
* Compare 2 val_value_t struct value contents
*
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
extern int32
    val_compare (const val_value_t *val1,
		 const val_value_t *val2);


/********************************************************************
* FUNCTION val_compare_to_string
* 
* Compare a val_value_t struct value contents to a string
* 
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    strval2 == second value to check 
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
extern int32
    val_compare_to_string (const val_value_t *val1,
			   const xmlChar *strval2,
			   status_t *res);


/********************************************************************
* FUNCTION val_compare_for_replace
* 
* Compare 2 val_value_t struct value contents
* for the nc:operation=replace procedures
* Only check the child nodes to see if the
* config nodes are the same
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == new value to check
*    val2 == current value to check
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
extern int32
    val_compare_for_replace (const val_value_t *val1,
                             const val_value_t *val2);


/********************************************************************
* FUNCTION val_sprintf_simval_nc
* 
* Sprintf the xmlChar string NETCONF representation of a simple value
*
* buff is allowed to be NULL; if so, then this fn will
* just return the length of the string (w/o EOS ch) 
*
* USAGE:
*  call 1st time with a NULL buffer to get the length
*  call the 2nd time with a buffer of the returned length
*
* !!!! DOES NOT CHECK BUFF OVERRUN IF buff is non-NULL !!!!
*
* INPUTS:
*    buff == buffer to write (NULL means get length only)
*    val == value to print
*    len == address of return length
*
* OUTPUTS:
*   *len == number of bytes written (or just length if buff == NULL)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_sprintf_simval_nc (xmlChar *buff,
			   const val_value_t  *val,
			   uint32  *len);


/********************************************************************
* FUNCTION val_make_sprintf_string
*
* Malloc a buffer and then sprintf the xmlChar string 
* NETCONF representation of a simple value
*
* INPUTS:
*    val == value to print
*
* RETURNS:
*   malloced buffer with string represetation of the
*      'val' value node
*   NULL if some error
*********************************************************************/
extern xmlChar *
    val_make_sprintf_string (const val_value_t *val);


/********************************************************************
* FUNCTION val_resolve_scoped_name
* 
* Find the scoped identifier in the specified complex value
* 
* E.g.: foo.bar.baz
*
* INPUTS:
*    val == complex type to check
*    name == scoped name string of a nested node to find
*    chval == address of return child val
*
* OUTPUTS:
*    *chval is set to the value of the found local scoped 
*      child member, if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
extern status_t 
    val_resolve_scoped_name (val_value_t *val,
			     const xmlChar *name,
			     val_value_t **chval);


/********************************************************************
* FUNCTION val_get_iqualval
* 
* Get the effective instance qualifier value for this value
* 
* INPUTS:
*    val == value construct to check
*
* RETURNS:
*   iqual value
*********************************************************************/
extern ncx_iqual_t 
    val_get_iqualval (const val_value_t *val);


/********************************************************************
* FUNCTION val_duplicates_allowed
* 
* Determine if duplicates are allowed for the given val type
* The entire definition chain is checked to see if a 'no-duplicates'
*
* The default is config, so some sort of named type or parameter
* must be declared to create a non-config data element
*
* Fishing order:
*  1) typdef chain
*  2) parm definition
*  3) parmset definition
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value is classified as configuration
*     FALSE if the value is not classified as configuration
*********************************************************************/
extern boolean
    val_duplicates_allowed (val_value_t *val);


/********************************************************************
* FUNCTION val_has_content
* 
* Determine if there is a value or any child nodes for this val
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value has some content
*     FALSE if the value does not have any content
*********************************************************************/
extern boolean
    val_has_content (const val_value_t *val);


/********************************************************************
* FUNCTION val_has_index
* 
* Determine if this value has an index
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value has an index
*     FALSE if the value does not have an index
*********************************************************************/
extern boolean
    val_has_index (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_first_index
* 
* Get the first index entry, if any for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*    pointer to first val_index_t node,  NULL if none
*********************************************************************/
extern val_index_t *
    val_get_first_index (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_next_index
* 
* Get the next index entry, if any for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*    pointer to next val_index_t node,  NULL if none
*********************************************************************/
extern val_index_t *
    val_get_next_index (const val_index_t *valindex);



/********************************************************************
* FUNCTION val_parse_meta
* 
* Parse the metadata descriptor against the typdef
* Check only that the value is ok, not instance count
*
* INPUTS:
*     typdef == typdef to check
*     attr == XML attribute to check
*     retval == initialized val_value_t to fill in
*
* OUTPUTS:
*   *retval == filled in if return is NO_ERR
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    val_parse_meta (typ_def_t *typdef,
		    xml_attr_t *attr,
		    val_value_t *retval);


/********************************************************************
* FUNCTION val_set_extern
* 
* Setup an NCX_BT_EXTERN value
*
* INPUTS:
*     val == value to setup
*     fname == filespec string to set as the value
*********************************************************************/
extern void
    val_set_extern (val_value_t  *val,
		    xmlChar *fname);


/********************************************************************
* FUNCTION val_set_intern
* 
* Setup an NCX_BT_INTERN value
*
* INPUTS:
*     val == value to setup
*     intbuff == internal buffer to set as the value
*********************************************************************/
extern void
    val_set_intern (val_value_t  *val,
		    xmlChar *intbuff);


/********************************************************************
* FUNCTION val_fit_oneline
* 
* Check if the XML encoding for the specified val_value_t
* should take one line or more than one line
*
* Simple types should not use more than one line or introduce
* any extra whitespace in any simple content element
* 
* !!!The calculation includes the XML start and end tags!!!
*
*  totalsize: <foo:node>value</foo:node>  == 26
*
* INPUTS:
*   val == value to check
*   linelen == length of line to check against
*   
* RETURNS:
*   TRUE if the val is a type that should or must fit on one line
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_fit_oneline (const val_value_t *val,
                     uint32 linesize);


/********************************************************************
* FUNCTION val_create_allowed
* 
* Check if the specified value is allowed to have a
* create edit-config operation attribute
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is allowed to have the edit-op
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_create_allowed (const val_value_t *val);


/********************************************************************
* FUNCTION val_delete_allowed
* 
* Check if the specified value is allowed to have a
* delete edit-config operation attribute
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is allowed to have the edit-op
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_delete_allowed (const val_value_t *val);


/********************************************************************
* FUNCTION val_is_config_data
* 
* Check if the specified value is a config DB object instance
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a config DB object instance
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_is_config_data (const val_value_t *val);


/********************************************************************
* FUNCTION val_is_virtual
* 
* Check if the specified value is a virtual value
* such that a 'get' callback function is required
* to access the real value contents
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a virtual value
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_is_virtual (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_virtual_value
* 
* Get the value of a value node
* The top-level value is provided by the caller
* and must be malloced with val_new_value
* before calling this function
* 
* must free the return val; not cached
*
* If the val->getcb is NULL, then an error will be returned
*
* Caller should check for *res == ERR_NCX_SKIPPED
* This will be returned if virtual value has no
* instance at this time.
*
*  !!! DO NOT SAVE THE RETURN VALUE LONGER THAN THE 
*  !!! VIRTUAL VALUE CACHE TIMEOUT VALUE
*
* INPUTS:
*   session == session CB ptr cast as void *
*              that is getting the virtual value
*   val == virtual value to get value for
*   res == pointer to output function return status value
*
* OUTPUTS:
*    val->virtualval will be set with the cached return value
*    *res == the function return status
*
* RETURNS:
*   A malloced and filled in val_value_t struct
*   This value is cached in the val->virtualval pointer
*   and will be freed when the cache is replaced or when
*   val is freed
*********************************************************************/
extern val_value_t *
    val_get_virtual_value (void *session,  /* really ses_cb_t *   */
			   val_value_t *val,
			   status_t *res);


/********************************************************************
* FUNCTION val_is_default
* 
* Check if the specified value is set to the YANG default value
* 
* INPUTS:
*   val == value to check
*
* SIDE EFFECTS:
*   val->flags may be adjusted
*         VAL_FL_DEFVALSET will be set if not set already
*         VAL_FL_DEFVAL will be set or cleared if 
*            VAL_FL_DEFSETVAL is not already set,
*            after determining if the value == its default
*
* RETURNS:
*   TRUE if the val is set to the default value
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_is_default (val_value_t *val);


/********************************************************************
* FUNCTION val_is_real
* 
* Check if the specified value is a real value
*
*  return TRUE if not virtual or NCX_BT_EXTERN or NCX_BT_INTERN)
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a real value
*   FALSE otherwise
*********************************************************************/
extern boolean
    val_is_real (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_parent_nsid
* 
*    Try to get the parent namespace ID
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   namespace ID of parent, or 0 if not found or not a value parent
*********************************************************************/
extern xmlns_id_t 
    val_get_parent_nsid (const val_value_t *val);


/********************************************************************
* FUNCTION val_instance_count
* 
* Count the number of instances of the specified object name
* in the parent value struct.  This only checks the first
* level under the parent, not the entire subtree
*
*
* INPUTS:
*   val == value to check
*   modname == name of module which defines the object to count
*              NULL (do not check module names)
*   objname == name of object to count
*
* RETURNS:
*   number of instances found
*********************************************************************/
extern uint32
    val_instance_count (val_value_t  *val,
			const xmlChar *modname,
			const xmlChar *objname);


/********************************************************************
* FUNCTION val_set_extra_instance_errors
* 
* mark ERR_NCX_EXTRA_VAL_INST errors for nodes > 'maxelems'
* Count the number of instances of the specified object name
* in the parent value struct.  This only checks the first
* level under the parent, not the entire subtree
* Set the val-res status for all instances beyond the 
* specified 'maxelems' count to ERR_NCX_EXTRA_VAL_INST
*
* INPUTS:
*   val == value to check
*   modname == name of module which defines the object to count
*              NULL (do not check module names)
*   objname == name of object to count
*   maxelems == number of allowed instances
*
*********************************************************************/
extern void
    val_set_extra_instance_errors (val_value_t  *val,
				   const xmlChar *modname,
				   const xmlChar *objname,
				   uint32 maxelems);


/********************************************************************
* FUNCTION val_need_quotes
* 
* Check if a string needs to be quoted to be output
* within a conf file or ncxcli stdout output
*
* INPUTS:
*    str == string to check
*
* RETURNS:
*    TRUE if double quoted string is needed
*    FALSE if not needed
*********************************************************************/
extern boolean
    val_need_quotes (const xmlChar *str);


/********************************************************************
* FUNCTION val_all_whitespace
* 
* Check if a string is all whitespace
*
* INPUTS:
*    str == string to check
*
* RETURNS:
*    TRUE if string is all whitespace or empty length
*    FALSE if non-whitespace char found
*********************************************************************/
extern boolean
    val_all_whitespace (const xmlChar *str);


/********************************************************************
* FUNCTION val_match_metaval
* 
* Match the specific attribute value and namespace ID
*
* INPUTS:
*     attr == attr to check
*     nsid == mamespace ID to match against
*     name == attribute name to match against
*
* RETURNS:
*     TRUE if attr is a match; FALSE otherwise
*********************************************************************/
extern boolean
    val_match_metaval (const xml_attr_t *attr,
		       xmlns_id_t  nsid,
		       const xmlChar *name);


/********************************************************************
* FUNCTION val_get_dirty_flag
* 
* Get the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
extern boolean
    val_get_dirty_flag (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_subtree_dirty_flag
* 
* Get the subtree dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is subtree dirty, false otherwise
*********************************************************************/
extern boolean
    val_get_subtree_dirty_flag (const val_value_t *val);


/********************************************************************
* FUNCTION val_set_dirty_flag
* 
* Set the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
extern void
    val_set_dirty_flag (val_value_t *val);


/********************************************************************
* FUNCTION val_clear_dirty_flag
* 
* Clear the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
extern void
    val_clear_dirty_flag (val_value_t *val);



/********************************************************************
* FUNCTION val_dirty_subtree
* 
* Check the dirty or subtree_dirty flag
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty or any subtree may be dirty, false otherwise
*********************************************************************/
extern boolean
    val_dirty_subtree (const val_value_t *val);


/********************************************************************
 * FUNCTION val_clean_tree
 * 
 * Clear the dirty flag and the operation for all
 * nodes within a value struct
 *
 * INPUTS:
 *   val == value node to clean
 *
 * OUTPUTS:
 *   val and all its child nodes (if any) are cleaned
 *     val->flags: VAL_FL_DIRTY bit cleared to 0
 *     val->editop: cleared to OP_EDITOP_NONE
 *     val->curparent: cleared to NULL
 *********************************************************************/
extern void
    val_clean_tree (val_value_t *val);


/********************************************************************
* FUNCTION val_get_nest_level
* 
* Get the next level of the value
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     nest level from the root
*********************************************************************/
extern uint32
    val_get_nest_level (val_value_t *val);


/********************************************************************
* FUNCTION val_get_first_leaf
* 
* Get the first leaf or leaflist node in the 
* specified value tree
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     pointer to first leaf found within this val struct
*    pointer to val if val is a leaf
*********************************************************************/
extern val_value_t *
    val_get_first_leaf (val_value_t *val);


/********************************************************************
* FUNCTION val_get_mod_name
* 
* Get the module name associated with this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
extern const xmlChar *
    val_get_mod_name (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_mod_prefix
* 
* Get the module prefix associated with this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
extern const xmlChar *
    val_get_mod_prefix (const val_value_t *val);


/********************************************************************
* FUNCTION val_get_nsid
* 
* Get the namespace ID for the specified value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
extern xmlns_id_t
    val_get_nsid (const val_value_t *val);


/********************************************************************
* FUNCTION val_change_nsid
* 
* Change the namespace ID fora value node and all its descendants
*
* INPUTS:
*     val == value node to change
*    nsid == new namespace ID to use
*
*********************************************************************/
extern void
    val_change_nsid (val_value_t *val,
		     xmlns_id_t nsid);


/********************************************************************
* FUNCTION val_make_from_insertxpcb
* 
* Make a val_value_t for a list, with the
* child nodes for key leafs, specified in the
* key attribute string given to the insert operation
*
* INPUTS:
*   sourceval == list val_value_t from the PDU with the insertxpcb
*               to process 
*   status == address of return status (may be NULL, ignored)
*
* OUTPUTS:
*   if non-NULL:
*      *status == return status
*
* RETURNS:
*   malloced list val_value_t struct with converted value
*********************************************************************/
extern val_value_t *
    val_make_from_insertxpcb (val_value_t  *sourceval,
			      status_t *res);


/********************************************************************
* FUNCTION val_new_unique
* 
* Malloc and initialize the fields in a val_unique_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern val_unique_t * 
    val_new_unique (void);


/********************************************************************
* FUNCTION val_free_unique
* 
* CLean and free a val_unique_t struct
*
* INPUTS:
*    valuni == val_unique struct to free
*********************************************************************/
extern void
    val_free_unique (val_unique_t *valuni);


/********************************************************************
* FUNCTION val_get_typdef
* 
* Get the typdef field for a value struct
*
* INPUTS:
*    val == val_value_t struct to use
*
* RETURNS:
*   pointer to the typdef or NULL if none
*********************************************************************/
extern const typ_def_t *
    val_get_typdef (const val_value_t *val);


/********************************************************************
* FUNCTION val_set_by_default
* 
* Check if the value was set by val_add_defaults
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if set by default
*   FALSE if set explicitly by some user or the ctartup config
*********************************************************************/
extern boolean
    val_set_by_default (const val_value_t *val);


/********************************************************************
* FUNCTION val_has_withdef_default
* 
* Check if the value contained the wd:default attribute
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if wd:default was set to true
*   FALSE if wd:default attribute was not set to true
*********************************************************************/
extern boolean
    val_has_withdef_default (const val_value_t *val);


/********************************************************************
* FUNCTION val_set_withdef_default
* 
* Set the value flags as having the wd:default attribute
*
* INPUTS:
*    val == val_value_t struct to set
*
*********************************************************************/
extern void
    val_set_withdef_default (val_value_t *val);


/********************************************************************
* FUNCTION val_is_metaval
* 
* Check if the value is a meta-val (XML attribute)
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if val is a meta-val
*   FALSE if val is not a meta-val
*********************************************************************/
extern boolean
    val_is_metaval (const val_value_t *val);


/********************************************************************
* FUNCTION val_move_chidren
* 
* Move all the child nodes from src to dest
* Source and dest must both be containers!
*
* INPUTS:
*    srcval == source val_value_t struct to move
*    destval == destination value struct ot use
*
*********************************************************************/
extern void
    val_move_children (val_value_t *srcval,
                       val_value_t *destval);


/********************************************************************
* FUNCTION val_cvt_generic
* 
* Convert all the database object pointers to
* generic object pointers to decouple a user
* variable in yangcli from the server-specific
* object definition (which goes away when the
* session is terminated)
*
* !!! Need to assume the val->obj pointer is already
* !!! invalid.  This can happen to yangcli when a 
* !!! session is dropped and there are vars that
* !!! reference YANG objects from the session
*
* INPUTS:
*    val == val_value_t struct to convert to generic
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_cvt_generic (val_value_t *val);


/********************************************************************
* FUNCTION val_set_pcookie
* 
* Set the SIL pointer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*    pcookie == pointer cookie value to set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_set_pcookie (val_value_t *val,
                     void *pcookie);


/********************************************************************
* FUNCTION val_set_icookie
* 
* Set the SIL integer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*    icookie == integer cookie value to set
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_set_icookie (val_value_t *val,
                     int icookie);


/********************************************************************
* FUNCTION val_get_pcookie
* 
* Get the SIL pointer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*
* RETURNS:
*    pointer cookie value or NULL if none
*********************************************************************/
extern void *
    val_get_pcookie (val_value_t *val);


/********************************************************************
* FUNCTION val_get_icookie
* 
* Get the SIL integer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*
* RETURNS:
*    integer cookie value or 0 if none
*********************************************************************/
extern int
    val_get_icookie (val_value_t *val);


/********************************************************************
* FUNCTION val_delete_default_leaf
* 
* Do the internal work to setup a delete of
* a default leaf
*
* INPUTS:
*    val == val_value_t struct to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    val_delete_default_leaf (val_value_t *val);


/********************************************************************
* FUNCTION val_force_empty
* 
* Convert a simple node to an empty type
*
* INPUTS:
*    val == val_value_t struct to use
*
*********************************************************************/
extern void
    val_force_empty (val_value_t *val);


/********************************************************************
* FUNCTION val_move_fields_for_xml
* 
* Move or copy the internal fields from one val to another
* for xml_wr purposes
*
* INPUTS:
*    srcval == source val_value_t struct to move from
*    destval == destination val to move to
*    movemeta == TRUE if metaQ should be transferred
*********************************************************************/
extern void
    val_move_fields_for_xml (val_value_t *srcval,
                             val_value_t *destval,
                             boolean movemeta);


/********************************************************************
* FUNCTION val_get_first_key
* 
* Get the first key record if this is a list with a key-stmt
*
* INPUTS:
*   val == value node to check
*
*********************************************************************/
extern val_index_t *
    val_get_first_key (val_value_t *val);


/********************************************************************
* FUNCTION val_get_next_key
* 
* Get the next key record if this is a list with a key-stmt
*
* INPUTS:
*   curkey == current key node
*
*********************************************************************/
extern val_index_t *
    val_get_next_key (val_index_t *curkey);

/********************************************************************
* FUNCTION val_remove_key
* 
* Remove a key pointer because the key is invalid
* Free the key pointer
*
* INPUTS:
*   keyval == value node to find, remove and free
*
*********************************************************************/
extern void
    val_remove_key (val_value_t *keyval);


/********************************************************************
* FUNCTION val_new_deleted_value
* 
* Malloc and initialize the fields in a val_value_t to be used
* as a deleted node marker
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern val_value_t * 
    val_new_deleted_value (void);


/********************************************************************
* FUNCTION val_new_editvars
* 
* Malloc and initialize the val->editvars field
*
* INPUTS:
*    val == val_value_t data structure to use
*
* OUTPUTS:
*    val->editvars is malloced and initialized
* 
* RETURNS:
*   status
*********************************************************************/
extern status_t
    val_new_editvars (val_value_t *val);


/********************************************************************
* FUNCTION val_free_editvars
* 
* Free the editing variables for the value node
*
* INPUTS:
*    val == val_value_t data structure to use
*
* OUTPUTS:
*    val->editvars is freed if set
*    val->editop set to OP_EDITOP_NONE
*********************************************************************/
extern void
    val_free_editvars (val_value_t *val);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_val */
