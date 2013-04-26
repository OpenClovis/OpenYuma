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
#ifndef _H_typ
#define _H_typ

/*  FILE: typ.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Parameter Type Handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22-oct-05    abb      Begun
13-oct-08    abb      Moved pattern from typ_sval_t to ncx_pattern_t 
                      to support N patterns per typdef
*/

#include <xmlstring.h>

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* decimal64 constants */
#define TYP_DEC64_MIN_DIGITS   1
#define TYP_DEC64_MAX_DIGITS   18

/* typ_rangedef_t flags field */
#define TYP_FL_LBINF     bit0         /* lower bound = -INF */
#define TYP_FL_LBINF2    bit1         /* lower bound = INF */
#define TYP_FL_UBINF     bit2         /* upper bound = INF */
#define TYP_FL_UBINF2    bit3         /* upper bound = -INF */
#define TYP_FL_LBMIN     bit4         /* lower bound is set to 'min' */
#define TYP_FL_LBMAX     bit5         /* lower bound is set to 'max' */
#define TYP_FL_UBMAX     bit6         /* upper bound is set to 'max' */
#define TYP_FL_UBMIN     bit7         /* upper bound is set to 'min' */

#define TYP_RANGE_FLAGS  0xff

/* typ_enum_t flags field */
#define TYP_FL_ESET      bit0         /* value explicitly set */
#define TYP_FL_ISBITS    bit1         /* enum really used in bits */
#define TYP_FL_SEEN      bit2         /* used by yangdiff */

/* typ_sval_t flags field */
#define TYP_FL_USTRING   bit0         /* value is ustring, not string */

/* typ_named_t flags field */
#define TYP_FL_REPLACE   bit0         /* Replace if set; extend if not */



/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* type parser used in 3 separate modes */
typedef enum typ_pmode_t_ {
    TYP_PM_NONE,
    TYP_PM_NORMAL,    /* normal parse mode */
    TYP_PM_INDEX,     /* index clause parse mode */
    TYP_PM_MDATA      /* metadata clause parse mode */
} typ_pmode_t;


/* one list member
 * stored in simple.queue of instance-qualified strings 
 */
typedef struct typ_listval_t_ {
    dlq_hdr_t     qhdr;
    dlq_hdr_t     strQ;                        /* Q of typ_sval_t */
    ncx_iqual_t  iqual;   /* instance qualifier for this listval */
} typ_listval_t;


/* one member of a range definition -- stored in simple.rangeQ 
 *
 * range components in YANG are not allowed to appear out of order
 * and they are not allowed to overlap.
 *
 * A single number component is represented
 * as a range where lower bound == upper bound
 *
 *  (1 | 5 | 10) saved as (1..1 | 5..5 | 10..10)
 *
 * symbolic range terms are stored in flags, and processed
 * in the 2nd pass of the compiler:
 *
 * lower bound:
 *    min   TYP_FL_LBMIN
 *    max   TYP_FL_LBMAX
 *   -INF   TYP_FL_LBINF
 *    INF   TYP_FL_LBINF2
 *
 * upper bound:
 *    min   TYP_FL_UBMIN
 *    max   TYP_FL_UBMAX
 *    INF   TYP_FL_UBINF
 *   -INF   TYP_FL_UBINF2
 *
 * If range parsed with btyp == NCX_BT_NONE, then lbstr
 * and ubstr will be used (for numbers).  Otherwise
 * the number bounds of the range part are stored in lb and ub
 *
 * The entire range string is saved in rangestr for ncxdump
 * and yangcli help text
 *
 * The token associated with the range part is saved for
 * error messages in YANG phase 2 compiling
 */
typedef struct typ_rangedef_t_ {
    dlq_hdr_t   qhdr;
    xmlChar    *rangestr;        /* saved in YANG only */
    ncx_num_t   lb;              /* lower bound */
    ncx_num_t   ub;              
    ncx_btype_t btyp;
    uint32      flags;
    xmlChar    *lbstr;           /* saved if range deferred */
    xmlChar    *ubstr;           /* saved if range deferred */
    ncx_error_t tkerr;
} typ_rangedef_t;


/* one ENUM typdef value -- stored in simple.valQ
 * Used for NCX_BT_ENUM and NCX_BT_BITS data type
 */
typedef struct typ_enum_t_ {
    dlq_hdr_t     qhdr;
    xmlChar      *name;
    xmlChar      *descr;
    xmlChar      *ref;
    ncx_status_t  status;
    int32         val;
    uint32        pos;
    uint32        flags;
    dlq_hdr_t     appinfoQ;
} typ_enum_t;


/* one STRING typdef value, pattern value 
 *  -- stored in simple.valQ 
 */
typedef struct typ_sval_t_ {
    dlq_hdr_t  qhdr;
    ncx_str_t val;
    uint32    flags;
} typ_sval_t;

/* one range description */
typedef struct typ_range_t_ {
    xmlChar         *rangestr;
    dlq_hdr_t        rangeQ;            /* Q of typ_rangedef_t */
    ncx_errinfo_t    range_errinfo;
    ncx_status_t     res;
    ncx_error_t      tkerr;
} typ_range_t;

/* YANG pattern struct : N per typedef and also
 * across N typdefs in a chain: all are ANDed together like RelaxNG
 * instead of ORed together within the same type step like XSD
 */
typedef struct typ_pattern_t_ {
    dlq_hdr_t       qhdr;
    xmlRegexpPtr    pattern;
    xmlChar        *pat_str;
    ncx_errinfo_t   pat_errinfo;
} typ_pattern_t;


/* YANG identityref struct
 * the value is an identity-stmt QName
 * that has a base-stmt that resolves to the same value
 */
typedef struct typ_idref_t {
    xmlChar        *baseprefix;
    xmlChar        *basename;
    ncx_identity_t *base;     /* back-ptr to base (if found ) */
    const xmlChar  *modname;   /* back-ptr to the main mod name */
} typ_idref_t;

/* NCX_CL_SIMPLE
 *
 * The following enums defined in ncxconst.h are supported in this struct
 *  NCX_BT_BITS -- set of bit definitions (like list of enum)
 *  NCX_BT_BOOLEAN -- true, 1, false, 0
 *  NCX_BT_ENUM -- XSD like enumeration
 *  NCX_BT_EMPTY -- empty element type like <ok/>
 *  NCX_BT_INT8 -- 8 bit signed integer value
 *  NCX_BT_INT16 -- 16 bit signed integer value
 *  NCX_BT_INT32 -- 32 bit signed integer value
 *  NCX_BT_INT64 -- 64 bit signed integer value
 *  NCX_BT_UINT8 -- 8 bit unsigned integer value
 *  NCX_BT_UINT16 -- 16 bit unsigned integer value
 *  NCX_BT_UINT32 -- 32 bit unsigned integer value
 *  NCX_BT_UINT64 -- 64 bit unsigned integer value
 *  NCX_BT_DECIMAL64 -- 64 bit fixed-point number value
 *  NCX_BT_FLOAT64 -- 64 bit floating-point number value
 *  NCX_BT_STRING -- char string
 *  NCX_BT_BINARY -- binary string (base64 from RFC 4648)
 *  NCX_BT_LEAFREF -- YANG leafref (XPath path expression)
 *  NCX_BT_IDREF -- YANG identityref (QName)
 *  NCX_BT_INSTANCE_ID -- YANG instance-identifier (XPath expression)
 *  NCX_BT_SLIST -- simple list of string or number type (xsd:list)
 *  NCX_BT_UNION -- C-type union of any simtype except leafref and empty
 */
typedef struct typ_simple_t_ {
    ncx_btype_t      btyp;                             /* NCX base type */
    struct typ_template_t_ *listtyp;       /* template for NCX_BT_SLIST */
    struct xpath_pcb_t_   *xleafref;   /* saved for NCX_BT_LEAFREF only */

    /* pointer to resolved typedef for NCX_BT_LEAFREF/NCX_BT_INSTANCE_ID */
    struct typ_def_t_ *xrefdef;    

    boolean          constrained;     /* set when require-instance=TRUE */
    typ_range_t      range;     /* for all num types and string length  */
    typ_idref_t      idref;                    /* for NCX_BT_IDREF only */
    dlq_hdr_t        valQ;     /* bit, enum, string, list vals/patterns */
    dlq_hdr_t        metaQ;              /* Q of obj_template_t structs */
    dlq_hdr_t        unionQ;   /* Q of typ_unionnode_t for NCX_BT_UNION */
    dlq_hdr_t        patternQ;  /* Q of ncx_pattern_t for NCX_BT_STRING */
    ncx_strrest_t    strrest;   /* string/type restriction type in valQ */
    uint32           flags;
    uint32           maxbit;                /* max bit position in valQ */
    uint32           maxenum;                 /* max enum value in valQ */
    uint8            digits;           /* fraction-digits for decimal64 */
} typ_simple_t;


/* NCX_CL_NAMED
 *
 * typ_named_t
 *      - user defined type using another named type (not a base type)
 *        and therefore pointing to another typ_template (typ)
 *      - also used for derived type from another named type or
 *        extending the parent type in some way (newtyp)
 */
typedef struct typ_named_t_ {
    struct typ_template_t_ *typ;               /* derived-from type */
    struct typ_def_t_      *newtyp;    /* opt. additional semantics */
    uint32                  flags;
} typ_named_t;


/* NCX_CL_REF
 *
 * typ_ref_t
 *  struct pointing to another typ_def_t
 *  used internally within inline data types
 *  (child nodes or scoped index nodes of complex types)
 */
typedef struct typ_ref_t_ {
    struct typ_def_t_ *typdef;                 /* fwd pointer */
} typ_ref_t;


/* Union of all the typdef variants */
typedef union typ_def_u_t_ {
    ncx_btype_t     base;
    typ_simple_t    simple;
    typ_named_t     named;
    typ_ref_t       ref;
} typ_def_u_t;


/* Discriminated union for all data typedefs.
 * This data structure is repeated in every
 * node of every data structure, starting with
 * the typ_def_t struct in the type template
 */
typedef struct typ_def_t_ {
    ncx_tclass_t     tclass;
    ncx_iqual_t      iqual;
    ncx_access_t     maxaccess;
    ncx_data_class_t dataclass;
    ncx_merge_t      mergetype;
    xmlChar         *prefix;            /* pfix used in type field */
    xmlChar         *typenamestr;   /* typename used in type field */
    dlq_hdr_t        appinfoQ;             /* Q of ncx_appinfo_t */
    ncx_error_t      tkerr;
    typ_def_u_t      def;
    uint32           linenum;
} typ_def_t;


/* One YANG 'type' definition -- top-level type template */
typedef struct typ_template_t_ {
    dlq_hdr_t    qhdr;
    xmlChar     *name;
    xmlChar     *descr;
    xmlChar     *ref;
    xmlChar     *defval;
    xmlChar     *units;
    xmlns_id_t   nsid;
    boolean      used;
    ncx_status_t status;
    typ_def_t    typdef;
    dlq_hdr_t    appinfoQ;
    void        *grp;       /* const back-ptr to direct grp parent */
    status_t     res;
    ncx_error_t  tkerr;
} typ_template_t;


/* One YANG union node 
 * One of the 2 pointers (typ or typdef will be NULL
 * If a named type is used, then 'typ' is active
 * If an inline type is used, then typdef is active
 */
typedef struct typ_unionnode_t_ {
    dlq_hdr_t     qhdr;
    typ_template_t *typ;      /* not malloced, just back-ptr */
    typ_def_t      *typdef;   /* malloced for unnamed inline type */
    boolean         seen;     /* needed for yangdiff */
} typ_unionnode_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION typ_load_basetypes
* 
* Create typ_template_t structs for the base types
* Must be called before any modules are loaded
* load the typ_template_t structs for the ncx_btype_t types
* MUST be called during ncx_init startup
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    typ_load_basetypes (void);


/********************************************************************
* FUNCTION typ_unload_basetypes
* 
* Unload and destroy the typ_template_t structs for the base types
* unload the typ_template_t structs for the ncx_btype_t types
* SHOULD be called during ncx_cleanup
*
*********************************************************************/
extern void
    typ_unload_basetypes (void);


/********************************************************************
* FUNCTION typ_new_template
* 
* Malloc and initialize the fields in a typ_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern typ_template_t *
    typ_new_template (void);


/********************************************************************
* FUNCTION typ_free_template
* 
* Scrub the memory in a typ_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    typ == typ_template_t to delete
*********************************************************************/
extern void 
    typ_free_template (typ_template_t *typ);


/********************************************************************
* FUNCTION typ_new_typdef
* 
* Malloc and initialize the fields in a typ_def_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
extern typ_def_t *
    typ_new_typdef (void);


/********************************************************************
* FUNCTION typ_init_typdef
* 
* init a pre-allocated typdef (done first)
* Initialize the fields in a typ_def_t
* !! Still need to call typ_init_simple
* !! when the actual builting type is determined
*
* INPUTS:
*   typdef == pointer to the struct to initialize
*********************************************************************/
extern void 
    typ_init_typdef (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_init_simple
* 
* Init a typ_simple_t struct inside a typ_def_t
* init a simple data type after typ_init_typdef
*
* INPUTS:
*     typdef == pointer to the typ_def_t  struct to init
*     as a NCX_CL_SIMPLE variant
*********************************************************************/
extern void
    typ_init_simple (typ_def_t  *tdef, 
		     ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_init_named
* 
* Init a typ_named_t struct inside a typ_def_t
* init a named data type after typ_init_typdef
*
* INPUTS:
*     typdef == pointer to the typ_def_t  struct to init
*     as a NCX_CL_SIMPLE variant
*********************************************************************/
extern void
    typ_init_named (typ_def_t  *tdef);



/********************************************************************
* FUNCTION typ_free_typdef
* 
* Scrub the memory in a typ_def_t by freeing all
* Then free the typdef itself
*
* INPUTS:
*    typdef == typ_def_t to delete
*********************************************************************/
extern void 
    typ_free_typdef (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_clean_typdef
* 
* Clean a typ_def_t struct, but do not delete it
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to clean
*********************************************************************/
extern void
    typ_clean_typdef (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_set_named_typdef
* 
* Set the fields in a named typedef (used by YANG parser)
*
* INPUTS:
*   typdef == type def struct to set
*   imptyp == named type to set within 'typ.typdef'
*
*********************************************************************/
extern void
    typ_set_named_typdef (typ_def_t *typdef,
			  typ_template_t *imptyp);


/********************************************************************
* FUNCTION typ_get_named_typename
* 
* Get the type name of the named typ
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to type name, NULL if some error
*********************************************************************/
extern const xmlChar *
    typ_get_named_typename (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_named_type_linenum
* 
* Get the line number of the type template of the named type
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to type name, NULL if some error
*********************************************************************/
extern uint32
    typ_get_named_type_linenum (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_set_new_named
* 
* Create a new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to setup
*     btyp == builtin type (NCX_BT_NONE if local type extension)
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    typ_set_new_named (typ_def_t  *typdef, 
		       ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_get_new_named
* 
* Access the new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to new typ_def_t or NULL if none
*********************************************************************/
extern typ_def_t *
    typ_get_new_named (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_cget_new_named
* 
* Access the new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to new typ_def_t or NULL if none
*********************************************************************/
extern const typ_def_t *
    typ_cget_new_named (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_set_simple_typdef
* 
* Set the fields in a simple typedef (used by YANG parser)
*
* INPUTS:
*   typ == type template to set
*   btyp == builtin type to set within 'typ.typdef'
*
*********************************************************************/
extern void
    typ_set_simple_typdef (typ_template_t *typ,
			   ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_new_enum
* 
* Alloc and Init a typ_enum_t struct
* malloc and init an enumeration descriptor, strdup name ptr
*
* INPUTS:
*   name == name string for the enumeration
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*   Note that the enum integer value is initialized to zero
*********************************************************************/
extern typ_enum_t *
    typ_new_enum (const xmlChar *name);


/********************************************************************
* FUNCTION typ_new_enum2
* 
* Alloc and Init a typ_enum_t struct
* Use the string value as-=is, instead of mallocing a new one
* malloc and init an enumeration descriptor, pass off name ptr
* 
* INPUTS:
*   name == name string for the enumeration (will get free-ed later!!)
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*   Note that the enum integer value is initialized to zero
*********************************************************************/
extern typ_enum_t *
    typ_new_enum2 (xmlChar *name);


/********************************************************************
* FUNCTION typ_free_enum
* 
* Free a typ_enum_t struct
* free an enumeration descriptor
*
* INPUTS:
*   en == enum struct to free
*********************************************************************/
extern void
    typ_free_enum (typ_enum_t *en);


/********************************************************************
* FUNCTION typ_new_rangedef
* 
* Alloc and Init a typ_rangedef_t struct (range-stmt)
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern typ_rangedef_t *
    typ_new_rangedef (void);


/********************************************************************
* FUNCTION typ_free_rangedef
* 
* Free a typ_rangedef_t struct (range-stmt)
* 
* INPUTS:
*   rv == rangeval struct to delete
*   btyp == base type of range (float and double have malloced strings)
*********************************************************************/
extern void
    typ_free_rangedef (typ_rangedef_t *rv, 
		       ncx_btype_t  btyp);


/********************************************************************
* FUNCTION typ_normalize_rangeQ
* 
* Start with a valid rangedef chain
* Combine any consecutive range definitions like
*   1..4|5|6|7..9  would break replaced with 1..9
* concat consecutive rangedef sections for integral numbers
*
* Not done for NCX_BT_FLOAT64 data types
*
* INPUTS:
*   rangeQ == Q of typ_rangeval_t structs to normalize
*   btyp == base type of range
*********************************************************************/
extern void
    typ_normalize_rangeQ (dlq_hdr_t *rangeQ,
			  ncx_btype_t  btyp);


/********************************************************************
* FUNCTION typ_get_rangeQ
*
* Return the rangeQ for the given typdef
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
extern dlq_hdr_t *
    typ_get_rangeQ (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_rangeQ_con
*
* Return the rangeQ for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the rangeQ from this typdef, or NULL if none
*********************************************************************/
extern dlq_hdr_t *
    typ_get_rangeQ_con (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_crangeQ
*
* Return the rangeQ for the given typdef
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
extern const dlq_hdr_t *
    typ_get_crangeQ (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_rangeQ_con
*
* Return the rangeQ for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the rangeQ from this typdef, or NULL if none
*********************************************************************/
extern const dlq_hdr_t *
    typ_get_crangeQ_con (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_range_con
*
* Return the range struct for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range struct for this typdef
*********************************************************************/
extern typ_range_t *
    typ_get_range_con (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_crange_con
*
* Return the range struct for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range struct for this typdef
*********************************************************************/
extern const typ_range_t *
    typ_get_crange_con (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_rangestr
*
* Return the range string for the given typdef chain
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range string for this typdef, NULL if none
*********************************************************************/
extern const xmlChar *
    typ_get_rangestr (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_first_rangedef
*
* Return the lower bound range definition struct
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typ_def == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
extern const typ_rangedef_t *
    typ_first_rangedef (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_first_rangedef_con
*
* Return the lower bound range definition struct
* Constain search to this typdef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
extern const typ_rangedef_t *
    typ_first_rangedef_con (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_rangebounds_con
*
* Return the lower and upper bound range number
* Constain search to this typdef
* deprecated -- does not support multi-part ranges
*
* INPUTS:
*    typ_def == typ def struct to check
*    btyp == pointer to output range number type
*    lb == pointer to output lower bound number
*    ub == pointer to output upper bound number
*
* OUTPUTS:
*   *btyp == the type of number in the return type, if non-NULL
*   *lb == lower bound number
*   *ub == upper bound number
*
* RETURNS:
*   status, NO_ERR == something found
*********************************************************************/
extern status_t
    typ_get_rangebounds_con (const typ_def_t *typdef,
			     ncx_btype_t *btyp,
			     const ncx_num_t **lb,
			     const ncx_num_t **ub);


/********************************************************************
* FUNCTION typ_get_strrest
* 
* Get the string restrinvtion type set for this typdef
*
* INPUTS:
*   typdef == typdef to check
*
* RETURNS:
*   string restrinction enumeration value
*********************************************************************/
extern ncx_strrest_t 
    typ_get_strrest (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_set_strrest
* 
* Set the string restrinvtion type set for this typdef
*
* INPUTS:
*   typdef == typdef to check
*   strrest == string restriction enum value to set
*
*********************************************************************/
extern void
    typ_set_strrest (typ_def_t *typdef,
		     ncx_strrest_t strrest);


/********************************************************************
* FUNCTION typ_new_sval
* 
* Alloc and Init a typ_sval_t struct
* malloc and init a string descriptor
*
* INPUTS:
*   str == string value inside token to copy
*   btyp == type of string (NCX_BT_STRING/LIST, OSTRING/OLIST)
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern typ_sval_t *
    typ_new_sval (const xmlChar *str,
		  ncx_btype_t  btyp);


/********************************************************************
* FUNCTION typ_free_sval
* 
* Free a typ_sval_t struct
* free a string descriptor
*
* INPUTS:
*   sv == typ_sval_t struct to free
*********************************************************************/
extern void
    typ_free_sval (typ_sval_t *sv);


/********************************************************************
* FUNCTION typ_new_listval
* 
* Alloc and Init a typ_listval_t struct
* malloc and init a list descriptor
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern typ_listval_t *
    typ_new_listval (void);


/********************************************************************
* FUNCTION typ_free_listval
* 
* Free a typ_listval_t struct
* free a list descriptor
*
* INPUTS:
*   lv == typ_listval_t struct to free
*********************************************************************/
extern void
    typ_free_listval (typ_listval_t *lv);


/********************************************************************
* FUNCTION typ_get_range_type
* 
* Get the correct typ_rangedef_t data type for the
* indicated base type
* get the proper range base type to use for a given base type
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     base type enum of the range data type
*********************************************************************/
extern ncx_btype_t 
    typ_get_range_type (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_get_basetype
* 
* Get the final base type of the specified typ_def_t
* Follow any typdef links and get the actual base type of 
* the specified typedef 
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base type of final typ_def_t
*********************************************************************/
extern ncx_btype_t
    typ_get_basetype (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_name
* 
* Get the name for the specified typdef
*
* INPUTS:
*     typdef == type definition to  check
*
* RETURNS:
*     type name or empty string if some error
*********************************************************************/
extern const xmlChar *
    typ_get_name (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_basetype_name
* 
* Get the name of the final base type of the specified typ_template_t
*
* INPUTS:
*     typ == template containing the typdef to  check
*
* RETURNS:
*     base type name of final embedded typ_def_t
*********************************************************************/
extern const xmlChar *
    typ_get_basetype_name (const typ_template_t  *typ);


/********************************************************************
* FUNCTION typ_get_parenttype_name
* 
* Get the final base type of the specified typ_def_t
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base type of final typ_def_t
*********************************************************************/
extern const xmlChar *
    typ_get_parenttype_name (const typ_template_t  *typ);


/********************************************************************
* FUNCTION typ_get_base_class
* 
* Follow any typdef links and get the class of the base typdef
* for the specified typedef 
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base class of final typ_def_t
*********************************************************************/
extern ncx_tclass_t
    typ_get_base_class (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_basetype_typ
* 
* Get the default typ_template_t for the specified base type
* Get the default type template for the specified base type
*
* INPUTS:
*     btyp == base type to get
* RETURNS:
*     pointer to the type template for the specified basetype
*********************************************************************/
extern typ_template_t *
    typ_get_basetype_typ (ncx_btype_t  btyp);


/********************************************************************
* FUNCTION typ_get_basetype_typdef
* 
* Get the default typdef for the specified base type
*
* INPUTS:
*     btyp == base type to get
* RETURNS:
*     pointer to the typdef for the specified basetype
*********************************************************************/
extern typ_def_t *
    typ_get_basetype_typdef (ncx_btype_t  btyp);


/********************************************************************
* FUNCTION typ_get_parent_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Ignores current named type even if if has new restrictions
* Get the parent typdef for NCX_CL_NAMED and NCX_CL_REF
* Returns NULL for all other classes
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
extern typ_def_t *
    typ_get_parent_typdef (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_parent_type
* 
* Get the next typ_template_t in a chain -- for NCX_CL_NAMED only
*
* INPUTS:
*     typ == type template to check
* RETURNS:
*     pointer to next non-empty typ_template_t for a named type
*********************************************************************/
extern const typ_template_t *
    typ_get_parent_type (const typ_template_t  *typ);


/********************************************************************
* FUNCTION typ_get_cparent_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Ignores current named type even if if has new restrictions
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
extern const typ_def_t *
    typ_get_cparent_typdef (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_next_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Get the next typdef in the chain for NCX_CL_NAMED or NCX_CL_REF
* Returns the input typdef for all other typdef classes
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
extern typ_def_t *
    typ_get_next_typdef (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_base_typdef
* 
* Get the base typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* get the real typdef that describes the type, if the
* input is one of the 'pointer' typdef classes. Otherwise,
* just return the input typdef
*
* INPUTS:
*     typdef == typdef to check
* RETURNS:
*     pointer to base typ_def_t
*********************************************************************/
extern typ_def_t *
    typ_get_base_typdef (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_cbase_typdef
* 
* Get the base typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
*
* INPUTS:
*     typdef == typdef to check
* RETURNS:
*     pointer to base typ_def_t
*********************************************************************/
extern const typ_def_t *
    typ_get_cbase_typdef (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_qual_typdef
* 
* Get the final typ_def_t of the specified typ_def_t
* based on the qualifier
* Get the next typdef in the chain for NCX_CL_NAMED or NCX_CL_REF
* Skip any named types without the specific restriction defined
*
* Returns the input typdef for simple typdef classes
*
* INPUTS:
*     typdef == typdef to check
*     squal == type of search qualifier desired
*          NCX_SQUAL_NONE == get first non-empty typdef
*          NCX_SQUAL_RANGE == find the first w/ range definition
*          NCX_SQUAL_VAL == find the first w/ stringval/pattern def
*          NCX_SQUAL_META == find the first typdef w/ meta-data def
*          
* RETURNS:
*     pointer to found typ_def_t or NULL if none found
*********************************************************************/
extern typ_def_t *
    typ_get_qual_typdef (typ_def_t  *typdef,
			 ncx_squal_t squal);


/********************************************************************
* FUNCTION typ_get_cqual_typdef
* 
* Get the final typ_def_t of the specified typ_def_t
* based on the qualifier
* INPUTS:
*     typdef == typdef to check
*     squal == type of search qualifier desired
*          NCX_SQUAL_NONE == get first non-empty typdef
*          NCX_SQUAL_RANGE == find the first w/ range definition
*          NCX_SQUAL_VAL == find the first w/ stringval/pattern def
*          NCX_SQUAL_META == find the first typdef w/ meta-data def
*          
* RETURNS:
*     pointer to found typ_def_t or NULL if none found
*********************************************************************/
extern const typ_def_t *
    typ_get_cqual_typdef (const typ_def_t  *typdef,
			  ncx_squal_t  squal);


/********************************************************************
* FUNCTION typ_find_appinfo
*
* Find the specified appinfo variable by its prefix and name
*
* INPUTS:
*  typdef ==  typedef to check
*  prefix == module prefix (may be NULL)
*  name == name of the appinfo var to find
*
* RETURNS:
*   pointer to found appinfo struct or NULL if not found
*********************************************************************/
extern const ncx_appinfo_t *
    typ_find_appinfo (const typ_def_t *typdef,
		      const xmlChar *prefix,
		      const xmlChar *name);


/********************************************************************
* FUNCTION typ_find_appinfo_con
*
* Find the specified appinfo name, constrained to the current typdef
*
* INPUTS:
*  typdef ==  typedef to check
*  prefix == appinfo module prefix (may be NULL)
*  name == name of the appinfo var to find
*
* RETURNS:
*   pointer to found appinfo struct or NULL if not found
*********************************************************************/
extern const ncx_appinfo_t *
    typ_find_appinfo_con (const typ_def_t *typdef,
			  const xmlChar *prefix,
			  const xmlChar *name);


/********************************************************************
* FUNCTION typ_is_xpath_string
*
* Find the ncx:xpath extension within the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:xpath extension found
*   FALSE otherwise
*********************************************************************/
extern boolean
    typ_is_xpath_string (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_is_qname_string
*
* Find the ncx:qname extension within the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:qname extension found
*   FALSE otherwise
*********************************************************************/
extern boolean
    typ_is_qname_string (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_is_schema_instance_string
*
* Find the ncx:schema-instance extension within 
* the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:schema-instance extension found
*   FALSE otherwise
*********************************************************************/
extern boolean
    typ_is_schema_instance_string (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_defval
*
* Find the default value string for the specified type template
* get default from template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   pointer to found defval string or NULL if none
*********************************************************************/
extern const xmlChar * 
    typ_get_defval (const typ_template_t *typ);


/********************************************************************
* FUNCTION typ_get_default
*
* Check if this typdef has a default value defined
* get default from typdef
*
* INPUTS:
*   typdef == typ_def_t struct to check
* RETURNS:
*   pointer to default or NULL if there is none
*********************************************************************/
extern const xmlChar *
    typ_get_default (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_iqualval
*
* Find the instance qualifier value enum for the specified type template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   iqual value enum
*********************************************************************/
extern ncx_iqual_t 
    typ_get_iqualval (const typ_template_t *typ);


/********************************************************************
* FUNCTION typ_get_iqualval_def
*
* Find the instance qualifier value enum for the specified type template
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   iqual value enum
*********************************************************************/
extern ncx_iqual_t
    typ_get_iqualval_def (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_units
*
* Find the units string for the specified type template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   pointer to found units string or NULL if none
*********************************************************************/
extern const xmlChar * 
    typ_get_units (const typ_template_t *typ);


/********************************************************************
* FUNCTION typ_get_units_from_typdef
*
* Find the units string for the specified typdef template
* Follow any NCX_CL_NAMED typdefs and check for a units
* clause in the the nearest ancestor typdef
* get units from named type if any
*
* INPUTS:
*  typdef == typ_def_t struct to check
*
* RETURNS:
*   pointer to found units string or NULL if none
*********************************************************************/
extern const xmlChar * 
    typ_get_units_from_typdef (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_has_children
*
* Check if this is a data type that uses the val.v.childQ
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if the childQ is used, FALSE otherwise
*********************************************************************/
extern boolean
    typ_has_children (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_has_index
*
* Check if this is a data type that has an index
*
* INPUTS:
*   btyp == base type enum
*
* RETURNS:
*   TRUE if the indexQ is used, FALSE otherwise
*********************************************************************/
extern boolean
    typ_has_index (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_is_simple
*
* Check if this is a simple data type
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if this is a simple data type, FALSE otherwise
*********************************************************************/
extern boolean
    typ_is_simple (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_is_xsd_simple
*
* Check if this is a simple data type in XSD encoding
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if this is a simple data type, FALSE otherwise
*********************************************************************/
extern boolean
    typ_is_xsd_simple (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_first_enumdef
*
* Get the first enum def struct
* looks past named typedefs to base typedef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
extern typ_enum_t *
    typ_first_enumdef (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_next_enumdef
*
* Get the next enum def struct
*
* INPUTS:
*    enumdef == typ enum struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
extern typ_enum_t *
    typ_next_enumdef (typ_enum_t *enumdef);


/********************************************************************
* FUNCTION typ_first_enumdef2
*
* Get the first enum def struct
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
extern typ_enum_t *
    typ_first_enumdef2 (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_first_con_enumdef
*
* Get the first enum def struct
* constrained to this typdef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
extern const typ_enum_t *
    typ_first_con_enumdef (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_find_enumdef
*
* Get the specified enum def struct
*
* INPUTS:
*    ebQ == enum/bits Q to check
*    name == name of the enum to find
* RETURNS:
*   pointer to the specified enum def of NULL if none
*********************************************************************/
extern typ_enum_t *
    typ_find_enumdef (dlq_hdr_t *ebQ,
		      const xmlChar *name);


/********************************************************************
* FUNCTION typ_enumdef_count
*
* Get the number of typ_enum_t Q entries
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   number of entries
*********************************************************************/
extern uint32
    typ_enumdef_count (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_first_strdef
*
* Get the first string def struct
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first string def of NULL if none
*********************************************************************/
extern const typ_sval_t *
    typ_first_strdef (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_maxrows
* 
* Get the maxrows value if it exists or zero if not
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     max number of rows or zero if not applicable
*********************************************************************/
extern uint32
    typ_get_maxrows (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_maxaccess
*
* Find the max-access value for the specified typdef
* Follow named types to see if any parent typdef has a 
* maxaccess clause, if none found in the parameter
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   maxaccess enumeration
*********************************************************************/
extern ncx_access_t
    typ_get_maxaccess (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_dataclass
*
* Find the data-class value for the specified typdef
* Follow named types to see if any parent typdef has a 
* data-class clause, if none found in the parameter
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   data class enumeration
*********************************************************************/
extern ncx_data_class_t
    typ_get_dataclass (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_mergetype
*
*  Get the merge type for a specified type def
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   merge type enumeration
*********************************************************************/
extern ncx_merge_t
    typ_get_mergetype (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_nsid
*
* Return the namespace ID
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
extern xmlns_id_t 
    typ_get_nsid (const typ_template_t *typ);


/********************************************************************
* FUNCTION typ_get_listtyp
*
* Return the typ_template for the list type, if the supplied
* typ_template contains a list typ_def, or named type chain
*    leads to a NCX_BT_SLIST or NCX_BT_BITS typdef
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
extern typ_template_t *
    typ_get_listtyp (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_clisttyp
*
* Return the typ_template for the list type, if the supplied
* typ_template contains a list typ_def, or named type chain
*    leads to a NCX_BT_SLIST typdef
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
extern const typ_template_t *
    typ_get_clisttyp (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_new_unionnode
* 
* Alloc and Init a typ_unionnode_t struct
*
* INPUTS:
*   typ == pointer to type template for this union node
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
extern typ_unionnode_t *
    typ_new_unionnode (typ_template_t *typ);


/********************************************************************
* FUNCTION typ_free_unionnode
* 
* Free a typ_unionnode_t struct
*
* INPUTS:
*   un == union node to free
*********************************************************************/
extern void
    typ_free_unionnode (typ_unionnode_t *un);


/********************************************************************
* FUNCTION typ_get_unionnode_ptr
* 
* Get the proper typdef pointer from a unionnode
*
* INPUTS:
*   un == union node to check
*
* RETURNS:
*   pointer to the typ_def_t inside
*********************************************************************/
extern typ_def_t *
    typ_get_unionnode_ptr (typ_unionnode_t *un);


/********************************************************************
* FUNCTION typ_first_unionnode
* 
* Get the first union node in the queue for a given typdef
*
* INPUTS:
*   typdef == pointer to type definition for the union node
*
* RETURNS:
*   pointer to first typ_unionnode struct or NULL if none
*********************************************************************/
extern typ_unionnode_t *
    typ_first_unionnode (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_first_con_unionnode
* 
* Get the first union node in the queue for a given typdef
* constrained
*
* INPUTS:
*   typdef == pointer to type definition for the union node
*
* RETURNS:
*   pointer to first typ_unionnode struct or NULL if none
*********************************************************************/
extern const typ_unionnode_t *
    typ_first_con_unionnode (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_is_number
* 
* Check if the base type is numeric
*
* INPUTS:
*    btype == basetype enum to check
*
* RETURNS:
*    TRUE if base type is numeric
*    FALSE if some other type
*********************************************************************/
extern boolean
    typ_is_number (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_is_string
* 
* Check if the base type is a simple string (not list)
*
* INPUTS:
*    btyp == base type enum to check
*
* RETURNS:
*    TRUE if base type is textual
*    FALSE if some other type
*********************************************************************/
extern boolean
    typ_is_string (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_is_enum
* 
* Check if the base type is an enumeration
*
* INPUTS:
*    btyp == base type enum to check
*
* RETURNS:
*    TRUE if base type is an enumeration
*    FALSE if some other type
*********************************************************************/
extern boolean
    typ_is_enum (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_new_pattern
* 
*   Malloc and init a pattern struct
*
* INPUTS:
*  pat_str == pattern string to copy and save
*
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
extern typ_pattern_t *
    typ_new_pattern (const xmlChar *pat_str);


/********************************************************************
* FUNCTION typ_free_pattern
* 
*   Free a pattern struct
*   Must be freed from any Q before calling this function
*
* INPUTS:
*    pat == typ_pattern_t struct to free
*
*********************************************************************/
extern void
    typ_free_pattern (typ_pattern_t *pat);


/********************************************************************
* FUNCTION typ_compile_pattern
* 
* Compile a pattern as into a regex_t struct
*
* INPUTS:
*     btyp == base type of the string
*     sv == ncx_sval_t holding the pattern to compile
*
* OUTPUTS:
*     pat->pattern is set if NO_ERR
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    typ_compile_pattern (typ_pattern_t *pat);


/********************************************************************
* FUNCTION typ_get_first_pattern
* 
* Get the first pattern struct for a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
extern typ_pattern_t *
    typ_get_first_pattern (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_next_pattern
* 
* Get the next pattern struct for a typdef
*
* INPUTS:
*   curpat  == current typ_pattern_t to check
*
* RETURNS:
*   pointer to next pattern struct or NULL if none
*********************************************************************/
extern typ_pattern_t *
    typ_get_next_pattern (typ_pattern_t *curpat);


/********************************************************************
* FUNCTION typ_get_first_cpattern
* 
* Get the first pattern struct for a typdef
* Const version
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
extern const typ_pattern_t *
    typ_get_first_cpattern (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_next_cpattern
* 
* Get the next pattern struct for a typdef
* Const version
*
* INPUTS:
*   curpat  == current typ_pattern_t to check
*
* RETURNS:
*   pointer to next pattern struct or NULL if none
*********************************************************************/
extern const typ_pattern_t *
    typ_get_next_cpattern (const typ_pattern_t *curpat);


/********************************************************************
* FUNCTION typ_get_pattern_count
* 
* Get the number of pattern structs in a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   count of the typ_pattern_t structs found
*********************************************************************/
extern uint32
    typ_get_pattern_count (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_range_errinfo
* 
* Get the range errinfo for a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
extern ncx_errinfo_t *
    typ_get_range_errinfo (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_clean_typeQ
* 
* Clean a queue of typ_template_t structs
*
* INPUTS:
*     que == Q of typ_template_t to clean
*
*********************************************************************/
extern void
    typ_clean_typeQ (dlq_hdr_t *que);


/********************************************************************
* FUNCTION typ_ok_for_inline_index
* 
* Check if the base type is okay to use in an inline index decl
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
extern boolean
    typ_ok_for_inline_index (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_ok_for_metadata
* 
* Check if the base type is okay to use in an XML attribute
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
extern boolean
    typ_ok_for_metadata (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_ok_for_index
* 
* Check if the base type is okay to use in an index decl
*
* INPUTS:
*     typdef == type def struct to check
*
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
extern boolean
    typ_ok_for_index (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_ok_for_union
* 
* Check if the base type is okay to use in an union decl
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
extern boolean
    typ_ok_for_union (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_ok
* 
* Check if the typdef chain has any errors
* Checks the named types in the typdef chain to
* see if they were already flagged as invalid
*
* INPUTS:
*     typdef == starting typdef to check
* RETURNS:
*     TRUE if okay, FALSE if any errors so far
*********************************************************************/
extern boolean
    typ_ok (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_ok_for_xsdlist
* 
* Check if the base type is okay to use in an ncx:xsdlist typedef
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
extern boolean
    typ_ok_for_xsdlist (ncx_btype_t btyp);


/********************************************************************
* FUNCTION typ_get_leafref_path
* 
*   Get the path argument for the leafref data type
*
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the path argument or NULL if some error
*********************************************************************/
extern const xmlChar *
    typ_get_leafref_path (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_leafref_path_addr
* 
*   Get the address of the path argument for the leafref data type
*
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the path argument or NULL if some error
*********************************************************************/
extern const void *
    typ_get_leafref_path_addr (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_leafref_pcb
* 
*   Get the XPath parser control block for the leafref data type
*   returns xpath_pcb_t but cannot import due to H file loop
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the PCB struct or NULL if some error
*********************************************************************/
extern struct xpath_pcb_t_ *
    typ_get_leafref_pcb (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_constrained
* 
*   Get the constrained true/false field for the data type
*   leafref or instance-identifier constrained flag
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
* RETURNS:
*    TRUE if constrained; FALSE if not
*********************************************************************/
extern boolean
    typ_get_constrained (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_set_xref_typdef
* 
*   Set the target typdef for a leafref or instance-identifier
*   NCX_BT_LEAFREF or NCX_BT_INSTANCE_ID
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
*********************************************************************/
extern void
    typ_set_xref_typdef (typ_def_t *typdef,
			 typ_def_t *target);


/********************************************************************
* FUNCTION typ_get_xref_typdef
* 
*   Get the xrefdef target typdef from a leafref 
*   or instance-identifier
*   NCX_BT_LEAFREF or NCX_BT_INSTANCE_ID
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
* RETURNS:
*    pointer to the PCB struct or NULL if some error
*********************************************************************/
extern typ_def_t *
    typ_get_xref_typdef (typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_has_subclauses
* 
*   Check if the specified typdef has any sub-clauses
*   Used by yangdump to reverse-engineer the YANG from the typdef
*   If any appinfo clauses present, then the result will be TRUE
*
* INPUTS:
*    typdef == typdef to check
*
* RETURNS:
*    TRUE if any sub-clauses, FALSE otherwise
*********************************************************************/
extern boolean
    typ_has_subclauses (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_get_idref
* 
* Get the idref field if this is an NCX_BT_IDREF typdef
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     pointer to idref field or NULL if wrong type
*********************************************************************/
extern typ_idref_t *
    typ_get_idref (typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_cidref
* 
* Get the idref field if this is an NCX_BT_IDREF typdef
* Const version
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     pointer to idref field or NULL if wrong type
*********************************************************************/
extern const typ_idref_t *
    typ_get_cidref (const typ_def_t  *typdef);


/********************************************************************
* FUNCTION typ_get_fraction_digits
* 
* Get the fraction-digits field from the typdef chain
* typdef must be an NCX_BT_DECIMAL64 or 0 will be returned
* valid values are 1..18
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     number of fixed decimal digits expected (1..18)
*     0 if some error
*********************************************************************/
extern uint8
    typ_get_fraction_digits (const typ_def_t *typdef);


/********************************************************************
* FUNCTION typ_set_fraction_digits
* 
* Set the fraction-digits field from the typdef chain
*
* INPUTS:
*   typdef == typdef to set (must be TYP_CL_SIMPLE)
*   digits == digits value to set
*
* RETURNS:
*     status
*********************************************************************/
extern status_t
    typ_set_fraction_digits (typ_def_t *typdef,
			     uint8 digits);


/********************************************************************
* FUNCTION typ_get_typ_linenum
* 
* Get the line number for the typ_template_t
*
* INPUTS:
*   typ == type template to check

* RETURNS:
*   line number
*********************************************************************/
extern uint32
    typ_get_typ_linenum (const typ_template_t  *typ);


/********************************************************************
* FUNCTION typ_get_typdef_linenum
* 
* Get the line number for the typ_def_t
*
* INPUTS:
*   typ == typdef to check

* RETURNS:
*   line number
*********************************************************************/
extern uint32
    typ_get_typdef_linenum (const typ_def_t  *typdef);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_typ */
