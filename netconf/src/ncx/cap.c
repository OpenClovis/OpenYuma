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
/*  FILE: cap.c

   The capabilities module constructs two versions/
  
   The cap_list_t version is the internal struct used 
   by the agent or manager.

   The val_value_t version is used to cache the capabilities
   that will actually be used within an rpc_msg_t when the
   manager or agent actually sends a hello message.

   Debugging and schema-discovery module support is also
   provided.

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
28apr05      abb      begun
20sep07      abb      add support for schema-discovery data model

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_cap
#include  "cap.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_ncx
#include  "ncx.h"
#endif

#ifndef _H_ncx_feature
#include  "ncx_feature.h"
#endif

#ifndef _H_ncx_list
#include  "ncx_list.h"
#endif

#ifndef _H_ncxconst
#include  "ncxconst.h"
#endif

#ifndef _H_ncxtypes
#include  "ncxtypes.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xmlns
#include  "xmlns.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_xml_val
#include  "xml_val.h"
#endif

#ifndef _H_yangconst
#include  "yangconst.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* controls extra attributes in the <capability> element */
/* #define USE_EXTENDED_HELLO 0 */

#define URL_START  (const xmlChar *)"xsd/"


/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/* hardwired list of NETCONF v1.0 capabilities */
static cap_stdrec_t stdcaps[] =
{
  { CAP_STDID_V1, CAP_BIT_V1, CAP_NAME_V1 },
  { CAP_STDID_WRITE_RUNNING, CAP_BIT_WR_RUN, CAP_NAME_WR_RUN },
  { CAP_STDID_CANDIDATE, CAP_BIT_CANDIDATE, 
    CAP_NAME_CANDIDATE },
  { CAP_STDID_CONF_COMMIT, CAP_BIT_CONF_COMMIT, 
    CAP_NAME_CONF_COMMIT },
  { CAP_STDID_ROLLBACK_ERR, CAP_BIT_ROLLBACK_ERR, 
    CAP_NAME_ROLLBACK_ERR },
  { CAP_STDID_VALIDATE, CAP_BIT_VALIDATE, CAP_NAME_VALIDATE },
  { CAP_STDID_STARTUP, CAP_BIT_STARTUP, CAP_NAME_STARTUP },
  { CAP_STDID_URL, CAP_BIT_URL, CAP_NAME_URL },
  { CAP_STDID_XPATH, CAP_BIT_XPATH, CAP_NAME_XPATH },
  { CAP_STDID_NOTIFICATION, CAP_BIT_NOTIFICATION, 
    CAP_NAME_NOTIFICATION },
  { CAP_STDID_INTERLEAVE, CAP_BIT_INTERLEAVE, 
    CAP_NAME_INTERLEAVE },
  { CAP_STDID_PARTIAL_LOCK, CAP_BIT_PARTIAL_LOCK, 
    CAP_NAME_PARTIAL_LOCK },
  { CAP_STDID_WITH_DEFAULTS, CAP_BIT_WITH_DEFAULTS, 
    CAP_NAME_WITH_DEFAULTS },
  { CAP_STDID_V11, CAP_BIT_V11, CAP_NAME_V11 },
  { CAP_STDID_VALIDATE11, CAP_BIT_VALIDATE11, CAP_NAME_VALIDATE11 },
  { CAP_STDID_CONF_COMMIT11, CAP_BIT_CONF_COMMIT11, CAP_NAME_CONF_COMMIT11 },
  { CAP_STDID_LAST_MARKER, 0x0, 
    (const xmlChar *)"" } /* end-of-list marker */
};


/********************************************************************
* FUNCTION new_cap
*
* Malloc and init a new cap_rec_t struct
*
* RETURNS:
*    malloced cap rec or NULL if error
*********************************************************************/
static cap_rec_t *
    new_cap (void)
{

    cap_rec_t  *cap;

    cap = m__getObj(cap_rec_t);
    if (!cap) {
        return NULL;
    }
    memset(cap, 0x0, sizeof(cap_rec_t));

    ncx_init_list(&cap->cap_feature_list, NCX_BT_STRING);
    ncx_init_list(&cap->cap_deviation_list, NCX_BT_STRING);

    return cap;

}  /* new_cap */


/********************************************************************
* FUNCTION free_cap
*
* Clean and free the fields in a pre-allocated cap_rec_t struct
*
* INPUTS:
*    cap == struct to free
* RETURNS:
*    none, silent programming errors ignored 
*********************************************************************/
static void 
    free_cap (cap_rec_t *cap)
{
    if (cap->cap_uri) {
        m__free(cap->cap_uri);
    }
    if (cap->cap_namespace) {
        m__free(cap->cap_namespace);
    }
    if (cap->cap_module) {
        m__free(cap->cap_module);
    }
    if (cap->cap_revision) {
        m__free(cap->cap_revision);
    }

    ncx_clean_list(&cap->cap_feature_list);
    ncx_clean_list(&cap->cap_deviation_list);

    m__free(cap);

}  /* free_cap */


/********************************************************************
* FUNCTION get_features_len
*
* Get the name length of each enabled feature
*
* INPUTS:
*    mod == original module
*    feature == feature found
*    cookie == original cookie
*
* RETURNS:
*    TRUE if processing should continue, FALSE if done
*********************************************************************/
static boolean
    get_features_len (const ncx_module_t *mod,
                      ncx_feature_t *feature,
                      void *cookie)
{
    uint32  *count;

    (void)mod;
    count = (uint32 *)cookie;
    *count += (xml_strlen(feature->name) + 1);
    return TRUE;

}  /* get_features_len */


/********************************************************************
* FUNCTION add_features
*
* Add a name, string for each enabled feature
*
* INPUTS:
*    mod == original module
*    feature == feature found
*    cookie == original cookie
*
* RETURNS:
*    TRUE if processing should continue, FALSE if done
*********************************************************************/
static boolean
    add_features (const ncx_module_t *mod,
                  ncx_feature_t *feature,
                  void *cookie)
{
    xmlChar  **str;

    (void)mod;
    str = (xmlChar **)cookie;
    *str += xml_strcpy(*str, feature->name);
    **str = (xmlChar)',';
    (*str)++;
    return TRUE;

}  /* add_features */


/********************************************************************
* FUNCTION make_mod_urn_ex
*
* Construct and malloc a module capability URN string
*
* From yang-07: section 5.6.4:

   The namespace URI is advertised as a capability in the NETCONF
   <hello> message to indicate support for the YANG module by a NETCONF
   server.  The capability URI advertised MUST be on the form:

     capability-string   = namespace-uri [ parameter-list ]
     parameter-list      = "?" parameter *( "&" parameter )
     parameter           = revision-parameter /
                           module-parameter /
                           feature-parameter /
                           deviation-parameter
     revision-parameter  = "revision=" revision-date
     module-parameter    = "module=" module-name
     feature-parameter   = "features=" feature *( "," feature )
     deviation-parameter = "deviations=" deviation *( "," deviation )

   Where "revision-date" is the revision of the module (see
   Section 7.1.9) that the NETCONF server implements, "module-name" is
   the name of module as it appears in the "module" statement (see
   Section 7.1), "namespace-uri" is the namespace for the module as it
   appears in the "namespace" statement, "feature" is the name of an
   optional feature implemented by the device (see Section 7.18.1), and
   "deviation" is the name of a module defining device deviations (see
   Section 7.18.3).

   In the parameter list, each named parameter MUST occur at most once.

* INPUTS:
*    mod == module to use
*
* RETURNS:
*    status
*********************************************************************/
static xmlChar *
    make_mod_urn_ex (const xmlChar *modname,
                     const xmlChar *revision,
                     const xmlChar *namespace,
                     ncx_module_t *mod)
{
    xmlChar              *str, *p;
    ncx_lmem_t           *listmember;
    uint32                len, feature_count, deviation_count;

    feature_count = 0;
    deviation_count = 0;

    /* get the length of the string needed by doing a dry run */
    len = xml_strlen(namespace);
    len++;   /* '?' char */

    len += xml_strlen(CAP_MODULE_EQ);
    len += xml_strlen(modname);

    if (revision) {
        len++;   /* & char */
        len += xml_strlen(CAP_REVISION_EQ);
        len += xml_strlen(revision);
    }

    if (mod) {
        feature_count = ncx_feature_count(mod, TRUE);
        if (feature_count > 0) {
            len++;   /* & char */
            len += xml_strlen(CAP_FEATURES_EQ);
            len += (feature_count-1);   /* all the commas */

            /* add in all the enabled features length */
            ncx_for_all_features(mod, get_features_len, &len, TRUE);
        }

        deviation_count = ncx_list_cnt(&mod->devmodlist);
        if (deviation_count > 0) {
            len++;   /* & char */
            len += xml_strlen(CAP_DEVIATIONS_EQ);
            len += (deviation_count-1);   /* all the commas */
            
            for (listmember = ncx_first_lmem(&mod->devmodlist);
                 listmember != NULL;
                 listmember = (ncx_lmem_t *)dlq_nextEntry(listmember)) {

                len += xml_strlen(listmember->val.str);
            }
        }
    }

    /* get a string to hold the result */
    str = m__getMem(len+1);
    if (!str) {
        return NULL;
    }

    /* repeat the previous steps for real */
    p = str;
    p += xml_strcpy(p, namespace);
    *p++ = (xmlChar)'?';

    p += xml_strcpy(p, CAP_MODULE_EQ);
    p += xml_strcpy(p, modname);

    if (revision) {
        *p++ = (xmlChar)'&';
        p += xml_strcpy(p, CAP_REVISION_EQ);
        p += xml_strcpy(p, revision);
    }

    if (mod) {
        if (feature_count > 0) {
            *p++ = (xmlChar)'&';
            p += xml_strcpy(p, CAP_FEATURES_EQ);

            /* add in all the enabled 'feature-name,' strings */
            ncx_for_all_features(mod, add_features, &p, TRUE);

            /* the last entry will have a comma that has to be removed */
            *--p = 0;
        }

        if (deviation_count > 0) {
            *p++ = (xmlChar)'&';
            p += xml_strcpy(p, CAP_DEVIATIONS_EQ);

            for (listmember = ncx_first_lmem(&mod->devmodlist);
                 listmember != NULL;
                 listmember = (ncx_lmem_t *)dlq_nextEntry(listmember)) {

                p += xml_strcpy(p, listmember->val.str);
                *p++ = ',';
            }

            /* the last entry will have a comma that has to be removed */
            *--p = 0;
        }
    }

    return str;

}  /* make_mod_urn_ex */ 


/********************************************************************
* FUNCTION make_mod_urn
*
* Construct and malloc a module capability URN string
*
* INPUTS:
*    mod == module to use
*
* RETURNS:
*    status
*********************************************************************/
static xmlChar *
    make_mod_urn (ncx_module_t *mod)
{

    return make_mod_urn_ex(mod->name,
                           mod->version,
                           mod->ns,
                           mod);

}  /* make_mod_urn */ 


/********************************************************************
* FUNCTION make_devmod_urn
*
* Construct and malloc a module capability URN string
* for a deviations module, from a save deviations struct
* These are the same as regular modules externally,
* but internally, they are not fully parsed.
*
* If the deviations module contains any YANG statements
* other than deviation-stmts, then it MUST be loaded
* into the system with ncxmod_load_module.
* If so, then a regular module capability URI MUST already
* be generated for that module INSTEAD of this function
*
* INPUTS:
*    savedev == save deviations struct to use
*
* RETURNS:
*    status
*********************************************************************/
static xmlChar *
    make_devmod_urn (ncx_save_deviations_t *savedev)
{

    return make_mod_urn_ex(savedev->devmodule,
                           savedev->devrevision,
                           savedev->devnamespace,
                           NULL);


}  /* make_devmod_urn */ 


/********************************************************************
* FUNCTION parse_uri_parm
*
* Parse a read-only substring as a foo=bar parameter
* Setup the next string after that is done
*
* INPUTS:
*    parmname = start of the sub-string, stopped on 
*               what should be the start of a parameter
*    parmnamelen == address of parm name return length
*    parmval == address of return parmval pointer
*    parmvallen == address of return parmval length
*    nextparmname == address of return pointer to next parmname
*
* RETURNS:
*    status, NO_ERR if valid parm-pair parsed
*********************************************************************/
static status_t 
    parse_uri_parm (const xmlChar *parmname,
                    uint32 *parmnamelen,
                    const xmlChar **parmval,
                    uint32 *parmvallen,
                    const xmlChar **nextparmname)
{
    const xmlChar  *str, *equal;

    *parmnamelen = 0;
    *parmval = NULL;
    *parmvallen = 0;
    *nextparmname = NULL;

    /* find the equals sign after the parameter name */
    equal = parmname;
    while (*equal && *equal != (xmlChar)'=') {
        equal++;
    }
    if (!*equal || equal == parmname) {
        /* error: skip to next parm or EOS */
        while (*equal && *equal != (xmlChar)'&') {
            equal++;
        }
        if (*equal) {
            /* stopped on ampersand */
            *nextparmname = ++equal;
        }
        return ERR_NCX_INVALID_VALUE;
    }

    /* OK: got an equals sign after some non-zero string */
    *parmnamelen = (uint32)(equal - parmname);
    *parmval = str = equal+1;

    while (*str && *str != (xmlChar)'&') {
        str++;
    }

    *parmvallen = (uint32)(str - *parmval);
    if (*str) {
        /* stopped on ampersand so another param is expected */
        *nextparmname = str+1;
    }
    return NO_ERR;

}  /* parse_uri_parm */


/************** E X T E R N A L   F U N C T I O N S ************/


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
cap_list_t *
    cap_new_caplist (void)
{
    cap_list_t *caplist;

    caplist = m__getObj(cap_list_t);
    if (caplist) {
        cap_init_caplist(caplist);
    }
    return caplist;

}  /* cap_new_caplist */


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
void
    cap_init_caplist (cap_list_t *caplist) 
{
#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    
    memset(caplist, 0x0, sizeof(cap_list_t));
    dlq_createSQue(&caplist->capQ);

}  /* cap_init_caplist */


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
void 
    cap_clean_caplist (cap_list_t *caplist)
{
    cap_rec_t  *cap;

#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    caplist->cap_std = 0;

    if (caplist->cap_schemes) {
        m__free(caplist->cap_schemes);
        caplist->cap_schemes = NULL;
    }

    if (caplist->cap_defstyle) {
        m__free(caplist->cap_defstyle);
        caplist->cap_defstyle = NULL;
    }

    if (caplist->cap_supported) {
        m__free(caplist->cap_supported);
        caplist->cap_supported = NULL;
    }

    /* drain the capability Q and free the memory */
    cap = (cap_rec_t *)dlq_deque(&caplist->capQ);
    while (cap != NULL) {
        free_cap(cap);
        cap = (cap_rec_t *)dlq_deque(&caplist->capQ);
    }

}  /* cap_clean_caplist */


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
void 
    cap_free_caplist (cap_list_t *caplist)
{
#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    cap_clean_caplist(caplist);
    m__free(caplist);

}  /* cap_free_caplist */


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
status_t 
    cap_add_std (cap_list_t *caplist, cap_stdid_t   capstd)
{
#ifdef DEBUG
    if (!caplist) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (capstd < CAP_STDID_LAST_MARKER) {
        m__setbit(caplist->cap_std, stdcaps[capstd].cap_bitnum); 
        return NO_ERR;
    } else {
        return ERR_NCX_WRONG_VAL;
    }
}  /* cap_add_std */


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
status_t
    cap_add_stdval (val_value_t *caplist,
                    cap_stdid_t   capstd)
{
    val_value_t     *capval;
    xmlChar         *str, *p;
    const xmlChar   *pfix, *cap;
    uint32           len;

#ifdef DEBUG
    if (!caplist || capstd >= CAP_STDID_LAST_MARKER) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* setup the string */
    if (capstd==CAP_STDID_V1) {
        pfix = CAP_BASE_URN;
        cap = NULL;
        len = xml_strlen(pfix);
    } else if (capstd==CAP_STDID_V11) {
        pfix = CAP_BASE_URN11;
        cap = NULL;
        len = xml_strlen(pfix);
    } else {
        pfix = CAP_URN;
        cap = stdcaps[capstd].cap_name;
        len = xml_strlen(pfix) + xml_strlen(cap);
    }

    /* make the string */
    str = m__getMem(len+1);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* concat the capability name if not the base string */
    p = str;
    p += xml_strcpy(str, pfix);
    if (cap) {
        xml_strcpy(p, cap);
    }

    /* make the capability element */
    capval = xml_val_new_string(NCX_EL_CAPABILITY,
                                xmlns_nc_id(), 
                                str);
    if (!capval) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    }

    val_add_child(capval, caplist);
    return NO_ERR;

}  /* cap_add_stdval */


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
status_t 
    cap_add_std_string (cap_list_t *caplist, 
                        const xmlChar *uri)
{
    const xmlChar *str;
    uint32         caplen, namelen, schemelen, basiclen;
    cap_stdid_t    stdid;

#ifdef DEBUG
    if (!caplist || !uri) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* the base:1.0 capability is a different form than the rest */
    if (!xml_strcmp(uri, CAP_BASE_URN)) {
        return cap_add_std(caplist, CAP_STDID_V1);
    }

    /* the base:1.1 capability is a different form than the rest */
    if (!xml_strcmp(uri, CAP_BASE_URN11)) {
        return cap_add_std(caplist, CAP_STDID_V11);
    }

    /* hack: support juniper servers which send the NETCONF
     * XML namespace instead of the NETCONF base URI
     * as the protocol indicator
     */
    if (!xml_strcmp(uri, NC_URN)) {
        return cap_add_std(caplist, CAP_STDID_V1);
    }

    caplen = xml_strlen(CAP_URN);
    if (!xml_strncmp(uri, CAP_URN, caplen)) {
        /* matched the standard capability prefix string;
         * get the suffix with the capability name and version 
         */
        str = uri + caplen;
    } else {
        caplen = xml_strlen(CAP_J_URN); 
        if (!xml_strncmp(uri, CAP_J_URN, caplen)) {
            /* matched the juniper standard capability prefix string;
             * get the suffix with the capability name and version 
             */
            str = uri + caplen;
        } else {
            return ERR_NCX_SKIPPED;
        }
    }

    /* go through the standard capability suffix strings */
    for (stdid=CAP_STDID_WRITE_RUNNING;
         stdid < CAP_STDID_LAST_MARKER; 
         stdid++) {

        switch (stdid) {
        case CAP_STDID_URL:
            namelen = xml_strlen(stdcaps[stdid].cap_name);
            if (!xml_strncmp(str, 
                             stdcaps[stdid].cap_name,
                             namelen)) {
                str += namelen;
                if (*str == (xmlChar)'?') {
                    str++;

                    /* first check standard scheme= string */
                    schemelen = xml_strlen(CAP_SCHEME_EQ);
                    if (!xml_strncmp(str,
                                     CAP_SCHEME_EQ,
                                     schemelen)) {
                        str += schemelen;
                        if (*str) {
                            return cap_add_url(caplist, str);
                        }
                    } else {
                        /* check juniper bug: protocol= */
                        schemelen = xml_strlen(CAP_PROTOCOL_EQ);
                        if (!xml_strncmp(str,
                                         CAP_PROTOCOL_EQ,
                                         schemelen)) {
                            str += schemelen;
                            if (*str) {
                                return cap_add_url(caplist, str);
                            }
                        }
                    }
                }
            }
            break;
        case CAP_STDID_WITH_DEFAULTS:
            namelen = xml_strlen(stdcaps[stdid].cap_name);
            if (!xml_strncmp(str, stdcaps[stdid].cap_name,
                             namelen)) {
                str += namelen;
                if (*str == (xmlChar)'?') {
                    str++;
                    basiclen = xml_strlen(CAP_BASIC_EQ);
                    if (!xml_strncmp(str,
                                     CAP_BASIC_EQ,
                                     basiclen)) {
                        str += basiclen;
                        if (*str) {
                            return cap_add_withdef(caplist, str);
                        }
                    }
                }
            }
            break;
        default:
            if (!xml_strcmp(str, stdcaps[stdid].cap_name)) {
                return cap_add_std(caplist, stdid);
            }
        }
    }
    return ERR_NCX_SKIPPED;

}  /* cap_add_std_string */


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
status_t 
    cap_add_module_string (cap_list_t *caplist, 
                           const xmlChar *uri)
{
    cap_rec_t     *cap;
    const xmlChar *qmark, *parmname, *parmval, *nextparmname;
    const xmlChar *module, *revision, *features, *deviations;
    xmlChar       *liststr, *commastr;
    uint32         parmnamelen, parmvallen, baselen, i;
    uint32         modulelen, revisionlen, featureslen, deviationslen;
    boolean        curmod, currev, curfeat, curdev, done, usewarning;
    status_t       res;
    xmlns_id_t     foundnsid;

#ifdef DEBUG
    if (!caplist || !uri) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* look for the end of the URI, for any parameters */
    qmark = uri;
    while (*qmark && *qmark != (xmlChar)'?') {
        qmark++;
    }
    if (!*qmark) {
        return ERR_NCX_SKIPPED;
    }

    baselen = (uint32)(qmark-uri);

    res = NO_ERR;
    module = NULL;
    revision = NULL;
    features = NULL;
    deviations = NULL;
    modulelen = 0;
    revisionlen = 0;
    deviationslen = 0;
    featureslen = 0;
    curmod = FALSE;
    currev = FALSE;
    curfeat = FALSE;
    curdev = FALSE;
    parmnamelen = 0;
    usewarning = ncx_warning_enabled(ERR_NCX_RCV_INVALID_MODCAP);

    /* lookup this namespace to see if it is already loaded */
    foundnsid = xmlns_find_ns_by_name_str(uri, baselen);

    /* setup the start of the parameter name for each loop iteration */
    parmname = qmark + 1;

    parmval = NULL;
    nextparmname = parmname;

    done = FALSE;
    while (!done) {
        if (nextparmname) {
            parmname = nextparmname;
        } else {
            done = TRUE;
            continue;
        }

        res = parse_uri_parm(parmname, 
                             &parmnamelen, 
                             &parmval, 
                             &parmvallen, 
                             &nextparmname);
        if (res != NO_ERR) {
            if (usewarning) {
                log_warn("\nWarning: skipping invalid "
                         "parameter syntax (%s) "
                         "in capability URI '%s'", 
                         get_error_string(res), 
                         uri);
            }
            if (NEED_EXIT(res)) {
                done = TRUE;
            }
            continue;
        }

        /* check that the parameter name matches one of the expected names */
        if (!xml_strncmp(parmname, 
                         YANG_K_MODULE, 
                         xml_strlen(YANG_K_MODULE))) {
            if (curmod) {
                if (usewarning) {
                    log_warn("\nWarning: skipping duplicate "
                             "'module' parameter "
                             "in capability URI '%s'", 
                             uri);
                }
            } else {
                curmod = TRUE;
                module = parmval;
                modulelen = parmvallen;
            }
        } else if (!xml_strncmp(parmname, 
                                YANG_K_REVISION, 
                                xml_strlen(YANG_K_REVISION))) {
            if (currev) {
                if (usewarning) {
                    log_warn("\nWarning: skipping duplicate "
                             "'revision' parameter "
                             "in capability URI '%s'", 
                             uri);
                }
            } else {
                currev = TRUE;
                revision = parmval;
                revisionlen = parmvallen;
            }
        } else if (!xml_strncmp(parmname, 
                                YANG_K_FEATURES, 
                                xml_strlen(YANG_K_FEATURES))) {
            if (curfeat) {
                if (usewarning) {
                    log_warn("\nWarning: skipping duplicate "
                             "'features' parameter "
                             "in capability URI '%s'", 
                             uri);
                }
            } else {
                curfeat = TRUE;
                features = parmval;
                featureslen = parmvallen;
            }
        } else if (!xml_strncmp(parmname, 
                                YANG_K_DEVIATIONS, 
                                xml_strlen(YANG_K_DEVIATIONS))) {
            if (curdev) {
                if (usewarning) {
                    log_warn("\nWarning: skipping duplicate "
                             "'deviations' parameter "
                             "in capability URI '%s'", 
                             uri);
                }
            } else {
                curdev = TRUE;
                deviations = parmval;
                deviationslen = parmvallen;
            }
        } else if (usewarning) {
            /* skip over this unknown parameter */
            log_warn("\nWarning: skipping unknown parameter '");
            for (i=0; i<parmnamelen; i++) {
                log_warn("%c", parmname[i]);
            }
            log_warn("=");
            for (i=0; i<parmvallen; i++) {
                log_warn("%c", parmval[i]);
            }
            log_warn("' in capability URI '%s'", uri);
        }
    }

    if (NEED_EXIT(res)) {
        return res;
    }

    if (!module) {
        if (revision || features || deviations) {
            if (usewarning) {
                log_warn("\nWarning: 'module' parameter "
                         "missing from possible "
                         "capability URI '%s'", 
                         uri);
            }
        }
    }

    /* assume this is a module capability URI
     * sp malloc a new capability record and save it
     */
    cap = new_cap();
    if (!cap) {
        return ERR_INTERNAL_MEM;
    }
    cap->cap_uri = xml_strdup(uri);
    if (!cap->cap_uri) {
        free_cap(cap);
        return ERR_INTERNAL_MEM;
    }

    cap->cap_namespace = xml_strndup(uri, baselen);
    if (!cap->cap_namespace) {
        free_cap(cap);
        return ERR_INTERNAL_MEM;
    }

    if (module != NULL && modulelen > 0) {
        cap->cap_module = xml_strndup(module, modulelen);
        if (!cap->cap_module) {
            free_cap(cap);
            return ERR_INTERNAL_MEM;
        }
    }

    /* check wrong module namespace base URI */
    if (foundnsid) {
        parmname = xmlns_get_module(foundnsid);
        if (cap->cap_module != NULL && xml_strcmp(parmname, cap->cap_module)) {
            if (usewarning) {
                log_warn("\nWarning: capability base URI mismatch, "
                         "got '%s' not '%s''", 
                         cap->cap_module,  
                         parmname);
            }
        }
    }

    if (revision) {
        cap->cap_revision = xml_strndup(revision, revisionlen);
        if (!cap->cap_revision) {
            free_cap(cap);
            return ERR_INTERNAL_MEM;
        }
    } else if (usewarning) {
        log_warn("\nWarning: 'revision' parameter missing from "
                 "capability URI '%s'", 
                 uri);
    }

    if (features) {
        liststr = xml_strndup(features, featureslen);
        if (!liststr) {
            free_cap(cap);
            return ERR_INTERNAL_MEM;
        }

        commastr = liststr;
        while (*commastr) {
            if (*commastr == (xmlChar)',') {
                *commastr = (xmlChar)' ';
            }
            commastr++;
        }

        res = ncx_set_list(NCX_BT_STRING, 
                           liststr, 
                           &cap->cap_feature_list);
        m__free(liststr);
        if (res != NO_ERR) {
            free_cap(cap);
            return res;
        }
    }

    if (deviations) {
        liststr = xml_strndup(deviations, deviationslen);
        if (!liststr) {
            free_cap(cap);
            return ERR_INTERNAL_MEM;
        }

        commastr = liststr;
        while (*commastr) {
            if (*commastr == (xmlChar)',') {
                *commastr = (xmlChar)' ';
            }
            commastr++;
        }

        res = ncx_set_list(NCX_BT_STRING, 
                           liststr, 
                           &cap->cap_deviation_list);
        m__free(liststr);
        if (res != NO_ERR) {
            free_cap(cap);
            return res;
        }
    }

    cap->cap_subject = CAP_SUBJTYP_DM;
    dlq_enque(cap, &caplist->capQ);
    return NO_ERR;

}  /* cap_add_module_string */


/********************************************************************
* FUNCTION cap_add_url
*
* Add the #url capability to the list
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    scheme_list == the protocol list for the :url capability
*
* RETURNS:
*    status, should always be NO_ERR
*********************************************************************/
status_t 
    cap_add_url (cap_list_t *caplist, 
                 const xmlChar *scheme_list)
{
#ifdef DEBUG
    if (!caplist || !scheme_list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    m__setbit(caplist->cap_std, stdcaps[CAP_STDID_URL].cap_bitnum);
    caplist->cap_schemes = xml_strdup(scheme_list);
    if (!caplist->cap_schemes) {
        return ERR_INTERNAL_MEM;
    }
    return NO_ERR;

} /* cap_add_url */


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
status_t
    cap_add_urlval (val_value_t *caplist,
                    const xmlChar *scheme_list)
{
    val_value_t          *capval;
    xmlChar              *str, *p;
    const xmlChar        *pfix, *cap;
    const xmlChar        *scheme;
    uint32                len;

#ifdef DEBUG
    if (!caplist || !scheme_list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* setup the string 
     *   <capability-uri>:url:1.0?scheme=file
     */
    pfix = CAP_URN;
    scheme = CAP_SCHEME_EQ;
    cap = stdcaps[CAP_STDID_URL].cap_name;

    /* get the total length */
    len = xml_strlen(pfix) + 
        xml_strlen(cap) + 1 +
        xml_strlen(scheme) + 
        xml_strlen(scheme_list) + 1;

    /* make the string */
    str = m__getMem(len+1);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* build the capability string */
    p = str;
    p += xml_strcpy(p, pfix);
    p += xml_strcpy(p, cap);
    *p++ = '?';
    p += xml_strcpy(p, scheme);
    xml_strcpy(p, scheme_list);

    /* make the capability element */
    capval = xml_val_new_string(NCX_EL_CAPABILITY,
                                xmlns_nc_id(), 
                                str);
    if (!capval) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    }

    val_add_child(capval, caplist);
    return NO_ERR;

}  /* cap_add_urlval */


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
status_t 
    cap_add_withdef (cap_list_t *caplist, 
                     const xmlChar *defstyle)
{
    const xmlChar *str;
    uint32         featureslen, basiclen;

#ifdef DEBUG
    if (!caplist || !defstyle) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    m__setbit(caplist->cap_std, 
              stdcaps[CAP_STDID_WITH_DEFAULTS].cap_bitnum);


    str = defstyle;
    while (*str && *str != '&') {
        str++;
    }
    if (*str) {
        featureslen = xml_strlen(CAP_SUPPORTED_EQ);
        basiclen = (uint32)(str - defstyle);
        if (!xml_strncmp(++str, 
                         CAP_SUPPORTED_EQ,
                         featureslen)) {
            str += featureslen;
            caplist->cap_supported = xml_strdup(str);
            if (!caplist->cap_supported) {
                return ERR_INTERNAL_MEM;
            }
            caplist->cap_defstyle = 
                xml_strndup(defstyle, basiclen);
            if (!caplist->cap_defstyle) {
                return ERR_INTERNAL_MEM;
            }
        }
    } else {
        caplist->cap_defstyle = xml_strdup(defstyle);
        if (!caplist->cap_defstyle) {
            return ERR_INTERNAL_MEM;
        }
    }

    return NO_ERR;

} /* cap_add_withdef */


/********************************************************************
* FUNCTION cap_add_withdefval
*
* Add the :with-defaults capability to the list
* value struct version
*
* INPUTS:
*    caplist == capability list that will contain the standard cap 
*    defstyle == the basic-mode with-default style
*
* OUTPUTS:
*    status
*********************************************************************/
status_t
    cap_add_withdefval (val_value_t *caplist,
                        const xmlChar *defstyle)
{
#define SUPPORTED_BUFFSIZE 64

    val_value_t          *capval;
    xmlChar              *str, *p;
    const xmlChar        *pfix, *cap;
    const xmlChar        *basic, *supported;
    uint32                len;
    ncx_withdefaults_t    withdef;
    xmlChar               buffer[SUPPORTED_BUFFSIZE];

#ifdef DEBUG
    if (!caplist || !defstyle) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* setup the string 
     * <capability-uri>:with-defaults:1.0?basic-mode=<defstyle>
     *    &also-supported=<other-two-enums>
     */
    pfix = CAP_URN;
    basic = CAP_BASIC_EQ;
    supported = CAP_SUPPORTED_EQ;
    cap = stdcaps[CAP_STDID_WITH_DEFAULTS].cap_name;

    /* figure out what the also-supported parameter value
     * should be and fill the buffer
     */
    withdef = ncx_get_withdefaults_enum(defstyle);
    if (withdef == NCX_WITHDEF_NONE) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    str = buffer;
    switch (withdef) {
    case NCX_WITHDEF_REPORT_ALL:
        str += xml_strcpy(str, NCX_EL_TRIM);
        *str++ = ',';
        str += xml_strcpy(str, NCX_EL_EXPLICIT);
        *str++ = ',';
        xml_strcpy(str, NCX_EL_REPORT_ALL_TAGGED);
        break;
    case NCX_WITHDEF_TRIM:
        str += xml_strcpy(str, NCX_EL_EXPLICIT);
        *str++ = ',';
        str += xml_strcpy(str, NCX_EL_REPORT_ALL);
        *str++  = ',';
        xml_strcpy(str, NCX_EL_REPORT_ALL_TAGGED);
        break;
    case NCX_WITHDEF_EXPLICIT:
        str += xml_strcpy(str, NCX_EL_TRIM);
        *str++ = ',';
        str += xml_strcpy(str, NCX_EL_REPORT_ALL);
        *str++ = ',';
        xml_strcpy(str, NCX_EL_REPORT_ALL_TAGGED);
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* get the total length */
    len = xml_strlen(pfix) + 
        xml_strlen(cap) + 
        1 +
        xml_strlen(basic) + 
        xml_strlen(defstyle) + 
        1 +
        xml_strlen(supported) + 
        xml_strlen(buffer);

    /* make the string */
    str = m__getMem(len+1);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* build the capability string */
    p = str;
    p += xml_strcpy(p, pfix);
    p += xml_strcpy(p, cap);
    *p++ = '?';
    p += xml_strcpy(p, basic);
    p += xml_strcpy(p, defstyle);
    *p++ = '&';
    p += xml_strcpy(p, supported);
    xml_strcpy(p, buffer);

    /* make the capability element */
    capval = xml_val_new_string(NCX_EL_CAPABILITY,
                                xmlns_nc_id(), 
                                str);
    if (!capval) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    }

    val_add_child(capval, caplist);
    return NO_ERR;

}  /* cap_add_withdefval */


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
status_t 
    cap_add_ent (cap_list_t *caplist, 
                 const xmlChar *uristr)
{
    cap_rec_t    *cap;

#ifdef DEBUG
    if (!caplist || !uristr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    cap = new_cap();
    if (!cap) {
        return ERR_INTERNAL_MEM;
    }

    /* fill in the cap_rec_t struct */
    cap->cap_subject = CAP_SUBJTYP_OTHER;
    cap->cap_uri = xml_strdup(uristr);
    if (!cap->cap_uri) {
        free_cap(cap);
        return ERR_INTERNAL_MEM;
    }

    dlq_enque(cap, &caplist->capQ);
    return NO_ERR;

}  /* cap_add_ent */ 


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
status_t 
    cap_add_modval (val_value_t *caplist, 
                    ncx_module_t *mod)
{
    xmlChar      *str;
    val_value_t  *capval;

#ifdef DEBUG
    if (!caplist || !mod) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!mod->name || !mod->ns || !mod->ismod) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }   
#endif

    /* construct the module URN string */
    str = make_mod_urn(mod);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* make the capability element */
    capval = xml_val_new_string(NCX_EL_CAPABILITY,
                                xmlns_nc_id(), 
                                str);
    if (!capval) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    }

    val_add_child(capval, caplist);

    return NO_ERR;

}  /* cap_add_modval */ 


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
status_t 
    cap_add_devmodval (val_value_t *caplist, 
                       ncx_save_deviations_t *savedev)
{
    xmlChar      *str;
    val_value_t  *capval;

#ifdef DEBUG
    if (!caplist || !savedev) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!savedev->devmodule || !savedev->devnamespace) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }   
#endif

    /* construct the module URN string */
    str = make_devmod_urn(savedev);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* make the capability element */
    capval = xml_val_new_string(NCX_EL_CAPABILITY,
                                xmlns_nc_id(), 
                                str);
    if (!capval) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    }

    val_add_child(capval, caplist);

    return NO_ERR;

}  /* cap_add_devmodval */ 


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
boolean 
    cap_std_set (const cap_list_t *caplist, 
                 cap_stdid_t capstd)
{
    if (!caplist) {
        return FALSE;
    }
    if (capstd < CAP_STDID_LAST_MARKER) {
        return (m__bitset(caplist->cap_std, 
                          stdcaps[capstd].cap_bitnum))
            ? TRUE : FALSE;
    } else {
        return FALSE;
    }
}  /* cap_std_set */


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
boolean 
    cap_set (const cap_list_t *caplist, 
             const xmlChar *capuri) 
{
    const xmlChar    *str;
    cap_rec_t        *cap;
    int               i;
    uint32            len, capurilen;

    if (!caplist || !capuri) {
        return FALSE;
    }

    capurilen = xml_strlen(capuri);

    /* check if this is the NETCONF V1 Base URN capability */
    if (!xml_strcmp(capuri, NC_URN)) {
        return (m__bitset(caplist->cap_std, 
                          stdcaps[CAP_STDID_V1].cap_bitnum)) ?
            TRUE : FALSE;
    }

    /* check if this is a NETCONF standard capability */
    len = xml_strlen(CAP_URN);
    if ((capurilen > len+1) && 
        !xml_strncmp(capuri, 
                     (const xmlChar *)CAP_URN, 
                     len)) {

        /* set str to the 'capability-name:version-number' */
        str = &capuri[len];    

        /* check all the capability names */
        for (i=1; i<CAP_STDID_LAST_MARKER; i++) {
            if (!xml_strcmp(str, stdcaps[i].cap_name)) {
                return (m__bitset(caplist->cap_std, 
                                  stdcaps[i].cap_bitnum)) ?
                    TRUE : FALSE;
            }
        }
    }

    /* check the enterprise capability queue
     * try exact match first
     */
    for (cap=(cap_rec_t *)dlq_firstEntry(&caplist->capQ);
         cap != NULL; cap=(cap_rec_t *)dlq_nextEntry(cap)) {
        if (!xml_strcmp(cap->cap_uri, capuri)) {
            return TRUE;
        }
    }

    /* check the enterprise capability queue
     * match the beginning part, not requiring a complete match
     */
    for (cap=(cap_rec_t *)dlq_firstEntry(&caplist->capQ);
         cap != NULL; cap=(cap_rec_t *)dlq_nextEntry(cap)) {
        if (!xml_strncmp(cap->cap_uri, capuri, capurilen)) {
            return TRUE;
        }
    }

    return FALSE;

}  /* cap_set */


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
const xmlChar *
    cap_get_protos (cap_list_t *caplist)
{
#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (const xmlChar *)caplist->cap_schemes;

} /* cap_get_protos */


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
void
    cap_dump_stdcaps (const cap_list_t *caplist)
{
    cap_stdid_t  capid;

#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    for (capid = CAP_STDID_V1;
         capid < CAP_STDID_LAST_MARKER;  
         capid++) {

        if (cap_std_set(caplist, capid)) {
            log_write("\n   %s", stdcaps[capid].cap_name);
        }
    }

} /* cap_dump_stdcaps */


/********************************************************************
* FUNCTION cap_dump_modcaps
*
* Printf the standard data model module capabilities list
* debug function
*
* INPUTS:
*    caplist == capability list to print
*********************************************************************/
void
    cap_dump_modcaps (const cap_list_t *caplist)
{
    const cap_rec_t *cap;
    const ncx_lmem_t *lmem;
    boolean anycaps;

#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    anycaps = FALSE;

    for (cap = (cap_rec_t *)dlq_firstEntry(&caplist->capQ);
         cap != NULL;
         cap = (cap_rec_t *)dlq_nextEntry(cap)) {

        if (cap->cap_subject != CAP_SUBJTYP_DM) {
            continue;
        }

        anycaps = TRUE;
        if (cap->cap_module && cap->cap_revision) {
            log_write("\n   %s@%s", cap->cap_module, cap->cap_revision);
        } else if (cap->cap_revision) {
            log_write("\n   ??@%s", cap->cap_revision);
        } else if (cap->cap_module) {
            log_write("\n   %s", cap->cap_module);
        } else if (cap->cap_namespace) {
            log_write("\n   %s", cap->cap_namespace);            
        } else if (cap->cap_uri) {
            log_write("\n   %s", cap->cap_uri);            
        } else {
            log_write("\n   ??");            
        }
        
        if (!dlq_empty(&cap->cap_feature_list.memQ)) {
            log_write("\n      Features: ");
            for (lmem = (ncx_lmem_t *)
                     dlq_firstEntry(&cap->cap_feature_list.memQ);
                 lmem != NULL;
                 lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {

                log_write("\n         %s ", lmem->val.str);
            }
        }

        if (!dlq_empty(&cap->cap_deviation_list.memQ)) {
            log_write("\n      Deviations: ");
            for (lmem = (ncx_lmem_t *)
                     dlq_firstEntry(&cap->cap_deviation_list.memQ);
                 lmem != NULL;
                 lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {

                log_write("\n         %s ", lmem->val.str);
            }
        }
    }

    if (!anycaps) {
        log_write("\n   None");
    }

} /* cap_dump_modcaps */


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
void
    cap_dump_entcaps (const cap_list_t *caplist)
{
    const cap_rec_t *cap;
    boolean  anycaps;

#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    anycaps = FALSE;

    for (cap = (cap_rec_t *)dlq_firstEntry(&caplist->capQ);
         cap != NULL;
         cap = (cap_rec_t *)dlq_nextEntry(cap)) {

        if (cap->cap_subject != CAP_SUBJTYP_DM) {
            anycaps = TRUE;
            log_write("\n   %s", cap->cap_uri);
        }
    }

    if (!anycaps) {
        log_write("\n   None");
    }

} /* cap_dump_entcaps */


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
cap_rec_t *
    cap_first_modcap (cap_list_t *caplist)
{
#ifdef DEBUG
    if (!caplist) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (cap_rec_t *)dlq_firstEntry(&caplist->capQ);

} /* cap_first_modcap */


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
cap_rec_t *
    cap_next_modcap (cap_rec_t *curcap)
{
#ifdef DEBUG
    if (!curcap) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (cap_rec_t *)dlq_nextEntry(curcap);

} /* cap_next_modcap */


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
void
    cap_split_modcap (cap_rec_t *cap,
                      const xmlChar **module,
                      const xmlChar **revision,
                      const xmlChar **namespacestr)
{

#ifdef DEBUG
    if (!cap || !module || !revision || !namespacestr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    *module = cap->cap_module;
    *revision = cap->cap_revision;
    *namespacestr = cap->cap_namespace;

} /* cap_split_modcap */


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
xmlChar *
    cap_make_moduri (ncx_module_t *mod)
{

#ifdef DEBUG
    if (!mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return make_mod_urn(mod);

} /* cap_make_moduri */


/* END file cap.c */
