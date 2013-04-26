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
#ifndef _H_xmlns
#define _H_xmlns
/*  FILE: xmlns.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    XML namespace support

    Applications will register namespaces in order to process
    XML requests containing elements in different namespaces,
    as required by the NETCONF protocol and XML 1.0.

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
30-apr-05    abb      Begun.
*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define XMLNS_NULL_NS_ID          0

#define XMLNS                     ((const xmlChar *)"xmlns")
#define XMLNS_LEN                 5

#define XMLNS_SEPCH               ':'

/* only compare if both elements have namespaces specified
 * If either one is 'no namespace', then it is considered a match
 */
#define XMLNS_EQ(NS1,NS2)     (((NS1) && (NS2)) && ((NS1)==(NS2)))

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/* integer handle for registered namespaces */
typedef uint32 xmlns_id_t;


/* represents one QName data element */
typedef struct xmlns_qname_t_ {
    xmlns_id_t       nsid;
    const xmlChar   *name;
} xmlns_qname_t;

/* represents one registered namespace */
typedef struct xmlns_t_ {
    xmlns_id_t   ns_id;
    xmlChar     *ns_pfix;
    xmlChar     *ns_name;
    xmlChar     *ns_module;
    struct ncx_module_t_ *ns_mod;
} xmlns_t;


/* represents one namespace prefix mapping */
typedef struct xmlns_pmap_t_ {
    dlq_hdr_t      qhdr;
    xmlns_id_t     nm_id;
    xmlChar       *nm_pfix;
    boolean        nm_topattr;
} xmlns_pmap_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION xmlns_init
*
* Initialize the module static variables
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void 
    xmlns_init (void);


/********************************************************************
* FUNCTION xmlns_cleanup
*
* Cleanup module static data
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
extern void 
    xmlns_cleanup (void);


/********************************************************************
* FUNCTION xmlns_register_ns
* 
* Register the specified namespace.  Each namespace or prefix
* can only be registered once.  An entry must be removed and
* added back in order to change it.
*
* INPUTS:
*    ns == namespace name
*    pfix == namespace prefix to use (if none provided but needed)
*    modname == name string of module associated with this NS
*    modptr == back-ptr to ncx_module_t struct (may be NULL)
*
* OUTPUTS:
*    *ns_id contains the ID assigned to the namespace
*
* RETURNS:
*    status, NO_ERR if all okay
*********************************************************************/
extern status_t 
    xmlns_register_ns (const xmlChar *ns,
		       const xmlChar *pfix,
		       const xmlChar *modname,
		       void *modptr,
		       xmlns_id_t *ns_id);


/********************************************************************
* FUNCTION xmlns_get_ns_prefix
*
* Get the prefix for the specified namespace 
*
* INPUTS:
*    ns_id == namespace ID
* RETURNS:
*    pointer to prefix or NULL if bad params
*********************************************************************/
extern const xmlChar * 
    xmlns_get_ns_prefix (xmlns_id_t ns_id);


/********************************************************************
* FUNCTION xmlns_get_ns_name
*
* Get the name for the specified namespace 
*
* INPUTS:
*    ns_id == namespace ID
* RETURNS:
*    pointer to name or NULL if bad params
*********************************************************************/
extern const xmlChar * 
    xmlns_get_ns_name (xmlns_id_t ns_id);


/********************************************************************
* FUNCTION xmlns_find_ns_by_module
*
* Find the NS ID from its module name that registered it
*
* INPUTS:
*    modname == module name string to find
*
* RETURNS:
*    namespace ID or XMLNS_NULL_NS_ID if error
*********************************************************************/
extern xmlns_id_t
    xmlns_find_ns_by_module (const xmlChar *modname);


/********************************************************************
* FUNCTION xmlns_find_ns_by_prefix
*
* Find the NS ID from its prefix
*
* INPUTS:
*    pfix == pointer to prefix string
* RETURNS:
*    namespace ID or XMLNS_NULL_NS_ID if error
*********************************************************************/
extern xmlns_id_t  
    xmlns_find_ns_by_prefix (const xmlChar *pfix);


/********************************************************************
* FUNCTION xmlns_find_ns_by_name
*
* Find the NS ID from its name
*
* INPUTS:
*    name == pointer to name string
* RETURNS:
*    namespace ID or XMLNS_NULL_NS_ID if error
*********************************************************************/
extern xmlns_id_t  
    xmlns_find_ns_by_name (const xmlChar *name);


/********************************************************************
* FUNCTION xmlns_find_ns_by_name_str
*
* Find the NS ID from its name (counted string version)
*
* INPUTS:
*    name == pointer to name string
*    namelen == length of name string
*
* RETURNS:
*    namespace ID or XMLNS_NULL_NS_ID if error
*********************************************************************/
extern xmlns_id_t  
    xmlns_find_ns_by_name_str (const xmlChar *name,
			       uint32 namelen);


/********************************************************************
* FUNCTION xmlns_nc_id
*
* Get the ID for the NETCONF namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    NETCONF NS ID or 0 if not found
*********************************************************************/
extern xmlns_id_t  
    xmlns_nc_id (void);


/********************************************************************
* FUNCTION xmlns_ncx_id
*
* Get the ID for the NETCONF Extensions namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    NETCONF-X NS ID or 0 if not found
*********************************************************************/
extern xmlns_id_t  
    xmlns_ncx_id (void);


/********************************************************************
* FUNCTION xmlns_ns_id
*
* Get the ID for the XMLNS namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    XMLNS NS ID or 0 if not found
*********************************************************************/
extern xmlns_id_t  
    xmlns_ns_id (void);


/********************************************************************
* FUNCTION xmlns_inv_id
*
* Get the INVALID namespace ID 
*
* INPUTS:
*    none
* RETURNS:
*    INVALID NS ID or 0 if not set yet
*********************************************************************/
extern xmlns_id_t  
    xmlns_inv_id (void);


/********************************************************************
* FUNCTION xmlns_xs_id
*
* Get the ID for the XSD namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    XSD NS ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_xs_id (void);


/********************************************************************
* FUNCTION xmlns_xsi_id
*
* Get the ID for the XSD Instance (XSI) namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    XSI ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_xsi_id (void);


/********************************************************************
* FUNCTION xmlns_xml_id
*
* Get the ID for the 1998 XML namespace or 0 if it doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    XML ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_xml_id (void);


/********************************************************************
* FUNCTION xmlns_ncn_id
*
* Get the ID for the NETCONF Notifications namespace or 0 if it 
* doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    NCN ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_ncn_id (void);


/********************************************************************
* FUNCTION xmlns_yang_id
*
* Get the ID for the YANG namespace or 0 if it 
* doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    YANG ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_yang_id (void);


/********************************************************************
* FUNCTION xmlns_yin_id
*
* Get the ID for the YIN namespace or 0 if it 
* doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    YIN ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_yin_id (void);


/********************************************************************
* FUNCTION xmlns_wildcard_id
*
* Get the ID for the base:1.1 wildcard namespace or 0 if it 
* doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    Wildcard ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_wildcard_id (void);


/********************************************************************
* FUNCTION xmlns_wda_id
*
* Get the ID for the wd:default XML attribute namespace or 0 if it 
* doesn't exist
*
* INPUTS:
*    none
* RETURNS:
*    with-defaults default attribute namespace ID or 0 if not found
*********************************************************************/
extern xmlns_id_t 
    xmlns_wda_id (void);


/********************************************************************
* FUNCTION xmlns_get_module
*
* get the module name of the namespace ID
* get module name that registered this namespace
*
* INPUTS:
*    nsid == namespace ID to check
* RETURNS:
*    none
*********************************************************************/
extern const xmlChar *
    xmlns_get_module (xmlns_id_t  nsid);


/********************************************************************
* FUNCTION xmlns_get_modptr
*
* get the module pointer for the namespace ID
*
* INPUTS:
*    nsid == namespace ID to check
* RETURNS:
*    void * cast of the module or NULL
*********************************************************************/
extern void *
    xmlns_get_modptr (xmlns_id_t nsid);


/********************************************************************
* FUNCTION xmlns_set_modptrs
*
* get the module pointer for the namespace ID
*
* INPUTS:
*    modname == module owner name to find
*    modptr == ncx_module_t back-ptr to set
*
*********************************************************************/
extern void
    xmlns_set_modptrs (const xmlChar *modname,
		       void *modptr);


/********************************************************************
* FUNCTION xmlns_new_pmap
*
* malloc and initialize a new xmlns_pmap_t struct
*
* INPUTS:
*   buffsize == size of the prefix buffer to allocate 
*               within this pmap (0 == do not malloc yet)
*
* RETURNS:
*    pointer to new struct or NULL if malloc error
*********************************************************************/
extern xmlns_pmap_t *
    xmlns_new_pmap (uint32 buffsize);


/********************************************************************
* FUNCTION xmlns_free_pmap
*
* free a xmlns_pmap_t struct
*
* INPUTS:
*   pmap == prefix map struct to free
*
*********************************************************************/
extern void
    xmlns_free_pmap (xmlns_pmap_t *pmap);


/********************************************************************
* FUNCTION xmlns_new_qname
*
* malloc and initialize a new xmlns_qname_t struct
*
* RETURNS:
*    pointer to new struct or NULL if malloc error
*********************************************************************/
extern xmlns_qname_t *
    xmlns_new_qname (void);


/********************************************************************
* FUNCTION xmlns_free_qname
*
* free a xmlns_qname_t struct
*
* INPUTS:
*   qname == QName struct to free
*
*********************************************************************/
extern void
    xmlns_free_qname (xmlns_qname_t *qname);


/********************************************************************
* FUNCTION xmlns_ids_equal
*
* compare 2 namespace IDs only if they are both non-zero
* and return TRUE if they are equal
*
* INPUTS:
*   ns1 == namespace ID 1
*   ns2 == namespace ID 2
*
* RETURNS:
*  TRUE if equal or both IDs are not zero
*  FALSE if both IDs are non-zero and they are different
*********************************************************************/
extern boolean
    xmlns_ids_equal (xmlns_id_t ns1,
		     xmlns_id_t ns2);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_xmlns */
