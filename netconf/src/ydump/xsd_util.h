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
#ifndef _H_xsd_util
#define _H_xsd_util

/*  FILE: xsd_util.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Utility functions for converting NCX modules to XSD format
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-nov-06    abb      Begun; split from xsd.c

*/

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

#define XSD_ABSTRACT       (const xmlChar *)"abstract"
#define XSD_AF_DEF         (const xmlChar *)"attributeFormDefault"
#define XSD_ANNOTATION     (const xmlChar *)"annotation"
#define XSD_ANY            (const xmlChar *)"any"
#define XSD_ANY_TYPE       (const xmlChar *)"anyType"
#define XSD_APP_SUFFIX     (const xmlChar *)"AppType"
#define XSD_ATTRIBUTE      (const xmlChar *)"attribute"
#define XSD_BANNER_0       (const xmlChar *)"Converted from NCX file '"
#define XSD_BANNER_0Y      (const xmlChar *)"Converted from YANG file '"
#define XSD_BANNER_0END    (const xmlChar *)"' by yangdump version "
#define XSD_BANNER_1       (const xmlChar *)"\n\nModule: "
#define XSD_BANNER_1S      (const xmlChar *)"\n\nSubmodule: "
#define XSD_BANNER_1B      (const xmlChar *)"\nBelongs to module: "
#define XSD_BANNER_2       (const xmlChar *)"\nOrganization: "
#define XSD_BANNER_3       (const xmlChar *)"\nVersion: "
#define XSD_BANNER_4       (const xmlChar *)"\nCopyright: "
#define XSD_BANNER_5       (const xmlChar *)"\nContact: "
#define XSD_BASE           (const xmlChar *)"base"
#define XSD_BASE64         (const xmlChar *)"base64Binary"
#define XSD_BT             (const xmlChar *)"BT"
#define XSD_CPX_CON        (const xmlChar *)"complexContent"
#define XSD_CPX_TYP        (const xmlChar *)"complexType"
#define XSD_DATA_INLINE    (const xmlChar *)"dataInlineType"
#define XSD_DOCUMENTATION  (const xmlChar *)"documentation"
#define XSD_DRAFT_URL   \
    (const xmlChar *)"http://www.ietf.org/internet-drafts/"
#define XSD_EF_DEF         (const xmlChar *)"elementFormDefault"
#define XSD_ELEMENT        (const xmlChar *)"element"
#define XSD_EN             (const xmlChar *)"en"
#define XSD_EXTENSION      (const xmlChar *)"extension"
#define XSD_ENUMERATION    (const xmlChar *)"enumeration"
#define XSD_FALSE          (const xmlChar *)"false"
#define XSD_FIELD          (const xmlChar *)"field"
#define XSD_FIXED          (const xmlChar *)"fixed"
#define XSD_GROUP          (const xmlChar *)"group"
#define XSD_HEX_BINARY     (const xmlChar *)"hexBinary"
#define XSD_ITEMTYPE       (const xmlChar *)"itemType"
#define XSD_KEY            (const xmlChar *)"key"
#define XSD_LANG           (const xmlChar *)"xml:lang"
#define XSD_LAX            (const xmlChar *)"lax"
#define XSD_LENGTH         (const xmlChar *)"length"
#define XSD_LIST           (const xmlChar *)"list"
#define XSD_LOC            (const xmlChar *)"schemaLocation"
#define XSD_MEMBERTYPES    (const xmlChar *)"memberTypes"
#define XSD_MAX_INCL       (const xmlChar *)"maxInclusive"
#define XSD_MAX_LEN        (const xmlChar *)"maxLength"
#define XSD_MAX_OCCURS     (const xmlChar *)"maxOccurs"
#define XSD_MIN_INCL       (const xmlChar *)"minInclusive"
#define XSD_MIN_LEN        (const xmlChar *)"minLength"
#define XSD_MIN_OCCURS     (const xmlChar *)"minOccurs"
#define XSD_NAMESPACE      (const xmlChar *)"namespace"
#define XSD_NOTIF_CONTENT  (const xmlChar *)"notificationContent"
#define XSD_NOTIF_CTYPE    (const xmlChar *)"NotificationContentType"
#define XSD_OPTIONAL       (const xmlChar *)"optional"
#define XSD_OTHER          (const xmlChar *)"##other"
#define XSD_OUTPUT_TYPEEXT (const xmlChar *)"_output_type__"
#define XSD_PROC_CONTENTS  (const xmlChar *)"processContents"
#define XSD_PS_SUFFIX      (const xmlChar *)"PSType"
#define XSD_QNAME          (const xmlChar *)"QName"
#define XSD_QUAL           (const xmlChar *)"qualified"
#define XSD_REQUIRED       (const xmlChar *)"required"
#define XSD_REF            (const xmlChar *)"ref"
#define XSD_RFC_URL        (const xmlChar *)"http://www.ietf.org/rfc/rfc"
#define XSD_RPC_OP         (const xmlChar *)"rpcOperation"
#define XSD_RPC_OPTYPE     (const xmlChar *)"rpcOperationType"
#define XSD_RESTRICTION    (const xmlChar *)"restriction"
#define XSD_SCHEMA         (const xmlChar *)"schema"
#define XSD_SELECTOR       (const xmlChar *)"selector"
#define XSD_SEQUENCE       (const xmlChar *)"sequence"
#define XSD_SIM_CON        (const xmlChar *)"simpleContent"
#define XSD_SIM_TYP        (const xmlChar *)"simpleType"
#define XSD_SUB_GRP        (const xmlChar *)"substitutionGroup"
#define XSD_TARG_NS        (const xmlChar *)"targetNamespace"
#define XSD_TRUE           (const xmlChar *)"true"
#define XSD_UNBOUNDED      (const xmlChar *)"unbounded"
#define XSD_UNION          (const xmlChar *)"union"
#define XSD_UNIQUE         (const xmlChar *)"unique"
#define XSD_UNIQUE_SUFFIX  (const xmlChar *)"_Unique"
#define XSD_UNQUAL         (const xmlChar *)"unqualified"
#define XSD_USE            (const xmlChar *)"use"
#define XSD_VALUE          (const xmlChar *)"value"
#define XSD_ZERO           (const xmlChar *)"0"


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/
typedef struct xsd_keychain_t_ {
    dlq_hdr_t    qhdr;
    val_value_t *val;
} xsd_keychain_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xsd_typename
* 
*   Get the corresponding XSD type name for the NCX base type
*
* INPUTS:
*    btyp == base type
*
* RETURNS:
*   const pointer to the XSD base type name string, NULL if error
*********************************************************************/
extern const xmlChar *
    xsd_typename (ncx_btype_t btyp);


/********************************************************************
* FUNCTION xsd_make_basename
* 
*   Malloc a string buffer and create a basename string
*   for a base type and metaSimpleType pair
*
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    nsid == namespace ID to use
*    name == condition clause (may be NULL)
*
* RETURNS:
*   malloced value string or NULL if malloc error
*********************************************************************/
extern xmlChar *
    xsd_make_basename (const typ_template_t *typ);


/********************************************************************
* FUNCTION xsd_make_enum_appinfo
* 
*   Go through the appinfoQ and add a child node to the
*   parent struct, which should be named 'appinfo'
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    enuval == enum value
*    appinfoQ == queue to check
*    status == definition status
*
* RETURNS:
*   malloced value containing all the appinfo or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xsd_make_enum_appinfo (int32 enuval,
			   const dlq_hdr_t  *appinfoQ,
			   ncx_status_t status);


/********************************************************************
* FUNCTION xsd_make_bit_appinfo
* 
*   Go through the appinfoQ and add a child node to the
*   parent struct, which should be named 'appinfo'
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    bitupos == bit position
*    appinfoQ == queue to check
*    status == definition status
*
* RETURNS:
*   malloced value containing all the appinfo or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xsd_make_bit_appinfo (uint32 bitpos,
			  const dlq_hdr_t  *appinfoQ,
			  ncx_status_t status);


/********************************************************************
* FUNCTION xsd_make_err_appinfo
* 
*   Make an appinfo node for the error-app-tag and or
*   error-message extensions
*
* INPUTS:
*    ref == reference clause (may be NULL)
*    errmsg == error msg string (1 may be NULL)
*    errtag == error app tag string (1 may be NULL)
*
* RETURNS:
*   malloced value containing the requested appinfo 
*   or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xsd_make_err_appinfo (const xmlChar *ref,
			  const xmlChar *errmsg,
			  const xmlChar *errtag);


/********************************************************************
* FUNCTION xsd_make_schema_location
* 
*   Get the schemaLocation string for this module
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    mod == module
*    schemaloc == CLI schmeloc parameter for URL start string
*    versionnames == TRUE if version in filename requested (CLI default)
*                 == FALSE for no version names in the filenames
*
* RETURNS:
*   malloced string or NULL if malloc error
*********************************************************************/
extern xmlChar *
    xsd_make_schema_location (const ncx_module_t *mod,
			      const xmlChar *schemaloc,
			      boolean versionnames);


/********************************************************************
* FUNCTION xsd_make_output_filename
* 
* Construct an output filename spec, based on the 
* conversion parameters
*
* INPUTS:
*    mod == module
*    cp == conversion parameters to use
*
* RETURNS:
*   malloced string or NULL if malloc error
*********************************************************************/
extern xmlChar *
    xsd_make_output_filename (const ncx_module_t *mod,
			      const yangdump_cvtparms_t *cp);


/********************************************************************
* FUNCTION xsd_do_documentation
* 
*   Create a value struct representing the description node
*   This is complete; The val_free_value function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    descr == reference clause
*    val == struct to contain added clause
*
* OUTPUT:
*   val->v.childQ has entries added if descr is non-NULL
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_do_documentation (const xmlChar *descr,
			  val_value_t *val);


/********************************************************************
* FUNCTION xsd_do_reference
* 
*   Create a value struct representing the reference node
*   This is complete; The val_free_value function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    ref == reference clause
*    val == struct to contain added clause
*
* OUTPUT:
*   val->v.childQ has entries added if descr and ref are non-NULL
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_do_reference (const xmlChar *ref,
		      val_value_t *val);


/********************************************************************
* FUNCTION xsd_add_mod_documentation
* 
*   Create multiple <documentation> elements representing the 
*   module-level documentation node
*
* INPUTS:
*    mod == module in progress
*    annot == annotation node to add the documentation node into
*
* OUTPUTS:
*    annot->a.childQ has nodes added if NO_ERR
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    xsd_add_mod_documentation (const ncx_module_t *mod,
			       val_value_t *annot);


/********************************************************************
* FUNCTION xsd_add_imports
* 
*   Add the required imports nodes
*
* INPUTS:
*    pcb == parser control block
*    mod == module in progress
*    cp == conversion parameters in use
*    val == struct parent to contain child nodes for each import
*
* OUTPUTS:
*    val->childQ has entries added for the imports required
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_imports (yang_pcb_t *pcb,
                     const ncx_module_t *mod,
		     const yangdump_cvtparms_t *cp,
		     val_value_t *val);


/********************************************************************
* FUNCTION xsd_add_includes
* 
*   Add the required include nodes
*
* INPUTS:
*    mod == module in progress
*    cp == conversion parameters in use
*    val == struct parent to contain child nodes for each include
*
* OUTPUTS:
*    val->childQ has entries added for the includes required
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_includes (const ncx_module_t *mod,
		      const yangdump_cvtparms_t *cp,
		      val_value_t *val);


/********************************************************************
* FUNCTION xsd_new_element
* 
*   Set up a new struct as an element
*
* INPUTS:
*    mod == module conversion in progress
*    name == element name
*    typdef == typ def for child node in progress
*    parent == typ def for parent node in progress (may be NULL)
*    hasnodes == TRUE if there are child nodes for this element
*             == FALSE if this is an empty node, maybe with attributes
*    hasindex == TRUE if this type has an index clause
*             == FALSE if this type does not have an index clause
*    
* RETURNS:
*   new element data struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xsd_new_element (const ncx_module_t *mod,
		     const xmlChar *name,
		     const typ_def_t *typdef,
		     const typ_def_t *parent,
		     boolean hasnodes,
		     boolean hasindex);


/********************************************************************
* FUNCTION xsd_new_leaf_element
* 
*   Set up a new YANG leaf or leaf-list as an element
*
* INPUTS:
*    mod == module conversion in progress
*    obj == object to use (leaf of leaf-list)
*    hasnodes == TRUE if a struct should be used
*                FALSE if an empty element should be used
*    addtype == TRUE if a type attribute should be added
*               FALSE if type attribute should not be added
*    iskey == TRUE if a this is a key leaf
*             FALSE if this is not a key leaf
*
* RETURNS:
*   new element data struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xsd_new_leaf_element (const ncx_module_t *mod,
			  const obj_template_t *obj,
			  boolean hasnodes,
			  boolean addtype,
			  boolean iskey);


/********************************************************************
* FUNCTION xsd_add_parmtype_attr
* 
*   Generate a type attribute for a type template
*
* INPUTS:
*    targns == targetNamespace ID
*    typ == type template for the parm 
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->metaQ has an entry added for this attribute
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_parmtype_attr (xmlns_id_t targns,
			   const typ_template_t *typ,
			   val_value_t *val);

/**** REMOVED *** no deep keys in YANG!
 * extern status_t
 *    xsd_add_key (const xmlChar *name,
 *		 const typ_def_t *typdef,
 *		 val_value_t *val);
 */


/********************************************************************
* FUNCTION xsd_do_annotation
* 
*   Add an annotation element if needed
*   Deprecated -- used by NCX only
*
* INPUTS:
*    descr == description string (may be NULL)
*    ref == reference string (may be NULL)
*    condition == condition string (may be NULL)
*    units == units clause contents (or NULL if not used)
*    maxacc == max-access clause (NONE == omit)
*    status == status clause (NONE or CURRENT to omit
*    appinfoQ  == queue of cx_appinfo_t records (may be NULL)
*    val == struct parent to contain child nodes for this annotation
*
* OUTPUTS:
*    val->v.childQ has an entry added for the annotation if
*    any of the content parameters are actually present
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_do_annotation (const xmlChar *descr,
                       const xmlChar *ref,
                       const xmlChar *condition,
                       const xmlChar *units,
                       ncx_access_t maxacc,
                       ncx_status_t status,
                       const dlq_hdr_t *appinfoQ,
                       val_value_t  *val);


/********************************************************************
* FUNCTION xsd_make_obj_annotation
* 
*   Make an annotation element for an object, if needed
*
* INPUTS:
*    obj == obj_template to check
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced object value struct
*    NULL if nothing to do (*res == NO_ERR)
*    NULL if some error (*res != NO_ERR)
*********************************************************************/
extern val_value_t *
    xsd_make_obj_annotation (obj_template_t *obj,
			     status_t  *res);


/********************************************************************
* FUNCTION xsd_do_type_annotation
* 
*   Add an annotation element for a typedef, if needed
*   
* INPUTS:
*    typ == typ_template to check
*    val == value struct to add nodes to
*
* OUTPUTS:
*    val->v.childQ has an <annotation> node added, if needed
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    xsd_do_type_annotation (const typ_template_t *typ,
			    val_value_t *val);


/********************************************************************
* FUNCTION xsd_make_group_annotation
* 
*   Make an annotation element for a grouping, if needed
*
* INPUTS:
*    grp == grp_template to check
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced object value struct
*    NULL if nothing to do (*res == NO_ERR)
*    NULL if some error (*res != NO_ERR)
*********************************************************************/
extern val_value_t *
    xsd_make_group_annotation (const grp_template_t *grp,
			       status_t  *res);


/********************************************************************
* FUNCTION xsd_add_aughook
* 
* Create an xs:any hook to add augments
*  
* INPUTS:
*    val == val_value_t struct to add new last child leaf 
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_aughook (val_value_t *val);


/********************************************************************
* FUNCTION xsd_make_rpc_output_typename
* 
*   Create a type name for the RPC function output data structure
*
* INPUTS:
*    obj == reference clause (may be NULL)
*    
* RETURNS:
*   malloced buffer with the typename
*********************************************************************/
extern xmlChar *
    xsd_make_rpc_output_typename (const obj_template_t *obj);


/********************************************************************
* FUNCTION xsd_add_type_attr
* 
*   Generate a type attribute
*
* INPUTS:
*    mod == module in progress
*           contains targetNamespace ID
*    typdef == typ_def for the typ_template struct to use for 
*              the attribute list source
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->metaQ has an entry added for this attribute
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xsd_add_type_attr (const ncx_module_t *mod,
		       const typ_def_t *typdef,
		       val_value_t *val);


/********************************************************************
* FUNCTION test_basetype_attr
* 
*   Test for the OK generate a type or base attribute
*
* INPUTS:
*    mod == module in progress
*    typdef == typ_def for the typ_template struct to use for 
*              the attribute list source
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    test_basetype_attr (const ncx_module_t *mod,
                        const typ_def_t *typdef);


/********************************************************************
* FUNCTION get_next_seqnum
* 
*  Get a unique integer
*
* RETURNS:
*   next seqnum
*********************************************************************/
extern uint32
    get_next_seqnum (void);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xsd_util */
