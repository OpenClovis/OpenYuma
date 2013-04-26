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
#ifndef _H_cap
#define _H_cap
/*  FILE: cap.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol capabilities

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
28-apr-05    abb      Begun.
*/
#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define MAX_STD_CAP_NAME_LEN  31

#define CAP_VERSION_LEN  15

/* NETCONF Base Protocol Capability String (base:1.0) */
#define CAP_BASE_URN ((const xmlChar *) \
		      "urn:ietf:params:netconf:base:1.0")

/* NETCONF Base Protocol Capability String (base:1.1) */
#define CAP_BASE_URN11 ((const xmlChar *) \
                        "urn:ietf:params:netconf:base:1.1")


/* NETCONF Capability Identifier Base String */
#define CAP_URN ((const xmlChar *)"urn:ietf:params:netconf:capability:")

/* NETCONF Capability Identifier Base String Implemented by Juniper */
#define CAP_J_URN \
    (const xmlChar *)"urn:ietf:params:xml:ns:netconf:capability:"

#define CAP_JUNOS \
    (const xmlChar *)"http://xml.juniper.net/netconf/junos/1.0"

#define CAP_SEP_CH   '/'


/************************************************************
 *                                                          *
 * The following 2 sets of definitions must be kept aligned *
 *                                                          *
 ************************************************************/

/* fast lookup -- standard capability bit ID */
#define CAP_BIT_V1            bit0
#define CAP_BIT_WR_RUN        bit1
#define CAP_BIT_CANDIDATE     bit2
#define CAP_BIT_CONF_COMMIT   bit3
#define CAP_BIT_ROLLBACK_ERR  bit4
#define CAP_BIT_VALIDATE      bit5
#define CAP_BIT_STARTUP       bit6
#define CAP_BIT_URL           bit7
#define CAP_BIT_XPATH         bit8
#define CAP_BIT_NOTIFICATION  bit9
#define CAP_BIT_INTERLEAVE    bit10
#define CAP_BIT_PARTIAL_LOCK  bit11
#define CAP_BIT_WITH_DEFAULTS bit12
#define CAP_BIT_V11           bit13
#define CAP_BIT_CONF_COMMIT11 bit14
#define CAP_BIT_VALIDATE11    bit15

/* put the version numbers in the capability names for now */
#define CAP_NAME_V1                 ((const xmlChar *)"base:1.0")
#define CAP_NAME_WR_RUN             ((const xmlChar *)"writable-running:1.0")
#define CAP_NAME_CANDIDATE          ((const xmlChar *)"candidate:1.0")
#define CAP_NAME_CONF_COMMIT        ((const xmlChar *)"confirmed-commit:1.0")
#define CAP_NAME_ROLLBACK_ERR       ((const xmlChar *)"rollback-on-error:1.0")
#define CAP_NAME_VALIDATE           ((const xmlChar *)"validate:1.0")
#define CAP_NAME_STARTUP            ((const xmlChar *)"startup:1.0")
#define CAP_NAME_URL                ((const xmlChar *)"url:1.0")
#define CAP_NAME_XPATH              ((const xmlChar *)"xpath:1.0")
#define CAP_NAME_NOTIFICATION       ((const xmlChar *)"notification:1.0")
#define CAP_NAME_INTERLEAVE         ((const xmlChar *)"interleave:1.0")
#define CAP_NAME_PARTIAL_LOCK       ((const xmlChar *)"partial-lock:1.0")
#define CAP_NAME_WITH_DEFAULTS      ((const xmlChar *)"with-defaults:1.0")
#define CAP_NAME_V11                ((const xmlChar *)"base:1.1")
#define CAP_NAME_VALIDATE11         ((const xmlChar *)"validate:1.1")
#define CAP_NAME_CONF_COMMIT11      ((const xmlChar *)"confirmed-commit:1.1")

/* some YANG capability details */
#define CAP_REVISION_EQ        (const xmlChar *)"revision="
#define CAP_MODULE_EQ          (const xmlChar *)"module="
#define CAP_FEATURES_EQ        (const xmlChar *)"features="
#define CAP_DEVIATIONS_EQ      (const xmlChar *)"deviations="
#define CAP_SCHEME_EQ          (const xmlChar *)"scheme="
#define CAP_PROTOCOL_EQ        (const xmlChar *)"protocol="
#define CAP_BASIC_EQ           (const xmlChar *)"basic-mode="
#define CAP_SUPPORTED_EQ       (const xmlChar *)"also-supported="


#define CAP_SCHEMA_RETRIEVAL \
    (const xmlChar *)"urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring"


/********************************************************************
*                                                                   *
*                                    T Y P E S                      *
*                                                                   *
*********************************************************************/

/* NETCONF capability subject types */
typedef enum cap_subjtyp_t_ {
    CAP_SUBJTYP_NONE,
    CAP_SUBJTYP_PROT,       /* capability is a protocol extension */
    CAP_SUBJTYP_DM,                 /* capability is a data model */
    CAP_SUBJTYP_OTHER      /* capability is other than prot or DM */
} cap_subjtyp_t;


/* enumerated list of standard capability IDs */
typedef enum cap_stdid_t_ {
    CAP_STDID_V1,
    CAP_STDID_WRITE_RUNNING,
    CAP_STDID_CANDIDATE,
    CAP_STDID_CONF_COMMIT,
    CAP_STDID_ROLLBACK_ERR,
    CAP_STDID_VALIDATE,
    CAP_STDID_STARTUP,
    CAP_STDID_URL,
    CAP_STDID_XPATH,
    CAP_STDID_NOTIFICATION,
    CAP_STDID_INTERLEAVE,
    CAP_STDID_PARTIAL_LOCK,
    CAP_STDID_WITH_DEFAULTS,
    CAP_STDID_V11,
    CAP_STDID_VALIDATE11,
    CAP_STDID_CONF_COMMIT11,
    CAP_STDID_LAST_MARKER
} cap_stdid_t;


/* 1 capabilities list */
typedef struct cap_list_t_ {
    uint32          cap_std;           /* bitset of std caps */
    xmlChar        *cap_schemes;       /* URL capability protocol list */
    xmlChar        *cap_defstyle;      /* with-defaults 'basic' parm */
    xmlChar        *cap_supported;     /* with-defaults 'also-supported' parm */
    dlq_hdr_t       capQ;              /* queue of non-std caps */
} cap_list_t;


/* array of this structure for list of standard capabilities */
typedef struct cap_stdrec_t_ {
    cap_stdid_t     cap_idnum;
    uint32          cap_bitnum;
    const xmlChar  *cap_name;
} cap_stdrec_t;


/* queue of this structure for list of enterprise capabilities */
typedef struct cap_rec_t_ {
    dlq_hdr_t      cap_qhdr;
    cap_subjtyp_t  cap_subject;
    xmlChar       *cap_uri;
    xmlChar       *cap_namespace;
    xmlChar       *cap_module;
    xmlChar       *cap_revision;
    ncx_list_t     cap_feature_list;
    ncx_list_t     cap_deviation_list;
} cap_rec_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION cap_new_caplist
*
* Malloc and initialize the fields in a cap_list_t struct
*
* INPUTS:
*    none
* RETURNS:
*    malloced cap_list or NULL if memory error
*********************************************************************/
extern cap_list_t *
    cap_new_caplist (void);


/********************************************************************
* FUNCTION cap_init_caplist
*
* Initialize the fields in a pre-allocated cap_list_t struct
* memory for caplist already allocated -- this just inits fields
*
* INPUTS:
*    caplist == struct to initialize
* RETURNS:
*    status, should always be NO_ERR
*********************************************************************/
extern void
    cap_init_caplist (cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_clean_caplist
*
* Clean the fields in a pre-allocated cap_list_t struct
* Memory for caplist not deallocated -- this just cleans fields
*
* INPUTS:
*    caplist == struct to clean
* RETURNS:
*    none, silent programming errors ignored 
*********************************************************************/
extern void 
    cap_clean_caplist (cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_free_caplist
*
* Clean the fields in a pre-allocated cap_list_t struct
* Then free the caplist memory
*
* INPUTS:
*    caplist == struct to free
*
*********************************************************************/
extern void 
    cap_free_caplist (cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_add_std
*
* Add a standard protocol capability to the list
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    capstd == the standard capability ID
* RETURNS:
*    status, should always be NO_ERR
*********************************************************************/
extern status_t 
    cap_add_std (cap_list_t *caplist, 
		 cap_stdid_t   capstd);


/********************************************************************
* FUNCTION cap_add_stdval
*
* Add a standard protocol capability to the list (val_value_t version)
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    capstd == the standard capability ID
* OUTPUTS:
*    status
*********************************************************************/
extern status_t
    cap_add_stdval (val_value_t *caplist,
		    cap_stdid_t   capstd);


/********************************************************************
* FUNCTION cap_add_std_string
*
* Add a standard protocol capability to the list by URI string
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    uri == the string holding the capability URI
*
* RETURNS:
*    status, NO_ERR if valid STD capability 
*    ERR_NCX_SKIPPED if this is not a standard capability
*    any other result is a non-recoverable error
*********************************************************************/
extern status_t 
    cap_add_std_string (cap_list_t *caplist, 
			const xmlChar *uri);


/********************************************************************
* FUNCTION cap_add_module_string
*
* Add a standard protocol capability to the list by URI string
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    uri == the URI string holding the capability identifier
*
* RETURNS:
*    status, NO_ERR if valid STD capability 
*    ERR_NCX_SKIPPED if this is not a module capability
*    any other result is a non-recoverable error
*********************************************************************/
extern status_t 
    cap_add_module_string (cap_list_t *caplist, 
			   const xmlChar *uri);


/********************************************************************
* FUNCTION cap_add_url
*
* Add the #url capability to the list
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    scheme_list == the scheme list for the :url capability
*
* RETURNS:
*    status, should always be NO_ERR
*********************************************************************/
extern status_t 
    cap_add_url (cap_list_t *caplist, 
		 const xmlChar *scheme_list);


/********************************************************************
* FUNCTION cap_add_urlval
*
* Add the :url capability to the list
* value struct version
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    scheme_list == the list of schemes supported
*
* OUTPUTS:
*    status
*********************************************************************/
extern status_t
    cap_add_urlval (val_value_t *caplist,
                    const xmlChar *scheme_list);
    

/********************************************************************
* FUNCTION cap_add_withdef
*
* Add the :with-defaults capability to the list
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    defstyle == the basic-mode with-default style
*    
* RETURNS:
*    status, should always be NO_ERR
*********************************************************************/
extern status_t 
    cap_add_withdef (cap_list_t *caplist, 
		     const xmlChar *defstyle);


/********************************************************************
* FUNCTION cap_add_withdefval
*
* Add the :with-defaults capability to t`he list
* value struct version
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    defstyle == the basic-mode with-default style
*
* OUTPUTS:
*    status
*********************************************************************/
extern status_t
    cap_add_withdefval (val_value_t *caplist,
			const xmlChar *defstyle);


/********************************************************************
* FUNCTION cap_add_ent
*
* Add an enterprise capability to the list
*
* INPUTS:
*    caplist == capability list that will contain the module caps
*    uristr == URI string to add
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    cap_add_ent (cap_list_t *caplist, 
		 const xmlChar *uristr);


/********************************************************************
* FUNCTION cap_add_modval
*
* Add a module capability to the list (val_value_t version)
*
* INPUTS:
*    caplist == capability list that will contain the enterprise cap 
*    mod == module to add
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    cap_add_modval (val_value_t *caplist, 
		    ncx_module_t *mod);


/********************************************************************
* FUNCTION cap_add_devmodval
*
* Add a deviation module capability to the list (val_value_t version)
*
* INPUTS:
*    caplist == capability list that will contain the enterprise cap 
*    savedev == save_deviations struct to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    cap_add_devmodval (val_value_t *caplist, 
                       ncx_save_deviations_t *savedev);


/********************************************************************
* FUNCTION cap_std_set
*
* fast search of standard protocol capability set
*
* INPUTS:
*    caplist == capability list to check
*    capstd == the standard capability ID
* RETURNS:
*    TRUE if indicated std capability is set, FALSE if not
*********************************************************************/
extern boolean 
    cap_std_set (const cap_list_t *caplist,
		 cap_stdid_t capstd);


/********************************************************************
* FUNCTION cap_set
*
* linear search of capability list, will check for std uris as well
*
* INPUTS:
*    caplist == capability list to check
*    capuri == the capability URI to find
* RETURNS:
*    TRUE if indicated capability is set, FALSE if not
*********************************************************************/
extern boolean 
    cap_set (const cap_list_t *caplist,
	     const xmlChar *capuri);


/********************************************************************
* FUNCTION cap_get_protos
*
* get the #url capability protocols list if it exists
* get the protocols field for the :url capability
*
* INPUTS:
*    caplist == capability list to check
* RETURNS:
*    pointer to protocols string if any, or NULL if not
*********************************************************************/
extern const xmlChar *
    cap_get_protos (cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_dump_stdcaps
*
* debug function
* Printf the standard protocol capabilities list
*
* INPUTS:
*    caplist == capability list to print
*
*********************************************************************/
extern void
    cap_dump_stdcaps (const cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_dump_modcaps
*
* Printf the standard data model module capabilities list
* debug function
*
* INPUTS:
*    caplist == capability list to print
*********************************************************************/
extern void
    cap_dump_modcaps (const cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_dump_entcaps
*
* Printf the enterprise capabilities list
* debug function
*
* INPUTS:
*    caplist == capability list to print
*
*********************************************************************/
extern void
    cap_dump_entcaps (const cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_first_modcap
*
* Get the first module capability in the list
*
* INPUTS:
*    caplist == capability list to check
*
* RETURNS:
*  pointer to first record, to use for next record
*  NULL if no first record
*********************************************************************/
extern cap_rec_t *
    cap_first_modcap (cap_list_t *caplist);


/********************************************************************
* FUNCTION cap_next_modcap
*
* Get the next module capability in the list
*
* INPUTS:
*    curcap == current mod_cap entry
*
* RETURNS:
*  pointer to next record
*  NULL if no next record
*********************************************************************/
extern cap_rec_t *
    cap_next_modcap (cap_rec_t *curcap);


/********************************************************************
* FUNCTION cap_split_modcap
*
* Split the modcap string into 3 parts
*
* INPUTS:
*    cap ==  capability rec to parse
*    module == address of return module name
*    revision == address of return module revision date string
*    namespacestr == address of return module namespace
*
* OUTPUTS:
*    *module == return module name
*    *revision == return module revision date string
*    *namespacestr == return module namepsace
*
* RETURNS:
*    status
*********************************************************************/
extern void
    cap_split_modcap (cap_rec_t *cap,
		      const xmlChar **module,
		      const xmlChar **revision,
		      const xmlChar **namespacestr);


/********************************************************************
* FUNCTION cap_make_moduri
*
* Malloc and construct a module URI for the specified module
* make the module URI string (for sysCapabilityChange event)
*
* INPUTS:
*    mod ==  module to use
*
* RETURNS:
*    malloced string containing the URI !!! must be freed later !!!
*********************************************************************/
extern xmlChar *
    cap_make_moduri (ncx_module_t *mod);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_cap */
