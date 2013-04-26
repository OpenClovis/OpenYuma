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
/*  FILE: xsd_util.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24nov06      abb      begun; split from xsd.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_num
#include "ncx_num.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tstamp
#include "tstamp.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xml_val
#include "xml_val.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xsd_util
#include "xsd_util.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#define DATETIME_BUFFSIZE  64


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
static uint32 seqnum = 1;


/********************************************************************
* FUNCTION make_include_location
* 
*   Get the schemaLocation string for this module
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* !!! TEMP: Location Hardwired to the following string!!!
*
*   <schemaloc>/<module>.xsd
*
* INPUTS:
*    mod == module
*
* RETURNS:
*   malloced string or NULL if malloc error
*********************************************************************/
static xmlChar *
    make_include_location (const xmlChar *modname,
                           const xmlChar *schemaloc)
{
    xmlChar *str, *str2;
    uint32   len, slen, inlen;

    len = inlen = (schemaloc) ? xml_strlen(schemaloc) : 0;
    slen = (uint32)((inlen && schemaloc[inlen-1] != '/') ? 1 : 0);
    len += slen;
    len += xml_strlen(modname);
    len += 6;    /* 1 backslash, file ext and terminating zero */ 

    str = m__getMem(len);
    if (!str) {
        return NULL;
    }

    str2 = str;
    if (schemaloc && *schemaloc) {
        str2 += xml_strcpy(str2, schemaloc);
        if (slen) {
            *str2++ = '/';
        }
    }
    str2 += xml_strcpy(str2, modname);
    *str2++ = '.';
    str2 += xml_strcpy(str2, NCX_XSD_EXT);
    return str;

}   /* make_include_location */


/********************************************************************
* FUNCTION add_revhist
* 
*   Add the revision history entry or just return the size required
*
* INPUTS:
*   mod == module in progress
*   appinfo == appinfo value struct to add revision child nodes
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    add_revhist (const ncx_module_t *mod,
                 val_value_t *appinfo)
{
    const ncx_revhist_t *rh;
    val_value_t         *rev, *chnode;
    xmlns_id_t           ncx_id;

    ncx_id = xmlns_ncx_id();

    for (rh = (const ncx_revhist_t *)dlq_firstEntry(&mod->revhistQ);
         rh != NULL;
         rh = (const ncx_revhist_t *)dlq_nextEntry(rh)) {

        if (!rh->version && !rh->descr) {
            SET_ERROR(ERR_INTERNAL_VAL);
            continue;
        }

        rev = xml_val_new_struct(YANG_K_REVISION, ncx_id);
        if (!rev) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(rev, appinfo);   /* add early */
        }

        if (rh->version) {
            chnode = xml_val_new_cstring(NCX_EL_VERSION, ncx_id,
                                         rh->version);
            if (!chnode) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chnode, rev);
            }
        }
        if (rh->descr) {
            chnode = xml_val_new_cstring(YANG_K_DESCRIPTION, ncx_id,
                                         rh->descr);
            if (!chnode) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chnode, rev);
            }
        }
    }

    return NO_ERR;

}   /* add_revhist */


/********************************************************************
* FUNCTION add_appinfoQ
* 
*   Add all the ncx_appinfo_t structs in the specified Q
*   into one <appinfo> element.  Look up each extension
*   for encoding rules.  They should have been validated
*   already.
*
* INPUTS:
*    appinfoQ == Q of ncx_appinfo_t use
*    appval == address of container node to 
*              get the ncx_appinfo_t elements
*
* OUTPUTS:
*    annot->v.childQ has 1 entry added for each appinfo
*               may be a nested element if the appinfo
*               has additional entries in its appinfoQ
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    add_appinfoQ (const dlq_hdr_t *appinfoQ,
                  val_value_t  *appval)
{
    val_value_t          *newval, *chval;
    const ncx_appinfo_t  *appinfo;
    xmlns_id_t            ncx_id;
    status_t              res;

    ncx_id = xmlns_ncx_id();
    
    /* add an element for each appinfo record */
    for (appinfo = (const ncx_appinfo_t *)dlq_firstEntry(appinfoQ);
         appinfo != NULL;
         appinfo = (const ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        if (!appinfo->ext) {
            /* NCX parsed appinfo */
            if (appinfo->value) {
                newval = xml_val_new_cstring(appinfo->name,
                                             ncx_id, appinfo->value);
            } else {
                newval = xml_val_new_flag(appinfo->name, ncx_id);
            }
            if (!newval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(newval, appval);
            }
            continue;
        }

        /* else a YANG parsed appinfo */
        if ((appinfo->ext->arg && appinfo->ext->argel) || 
            !dlq_empty(appinfo->appinfoQ)) {
            newval = xml_val_new_struct(appinfo->ext->name,
                                        appinfo->ext->nsid);
        } else {
            newval = xml_val_new_flag(appinfo->ext->name,
                                      appinfo->ext->nsid);
        }
        if (!newval) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(newval, appval);
        }

        if (appinfo->ext->arg) {
            if (appinfo->ext->argel) {
                chval = xml_val_new_cstring(appinfo->ext->arg,
                                            appinfo->ext->nsid, 
                                            appinfo->value);
                if (!chval) {
                    return ERR_INTERNAL_MEM;
                } else {
                    val_add_child(chval, newval);
                }
            } else {
                res = xml_val_add_cattr(appinfo->ext->arg,
                                        appinfo->ext->nsid,
                                        appinfo->value, newval);
                if (res != NO_ERR) {
                    return res;
                }
            }
        }

        if (!dlq_empty(appinfo->appinfoQ)) {
            res = add_appinfoQ(appinfo->appinfoQ, newval);
            if (res != NO_ERR) {
                return res;
            }
        }
    }

    return NO_ERR;

}  /* add_appinfoQ */


/********************************************************************
* FUNCTION make_reference
* 
*   Create a value struct representing the documentation node
*   Add this element to an <appinfo> element
*
* INPUTS:
*    ref == reference clause (may be NULL)
*
* RETURNS:
*   value struct or NULL if malloc error
*********************************************************************/
static val_value_t *
    make_reference (const xmlChar *ref)
{
    val_value_t    *retval, *textval, *urlval;
    const xmlChar  *str, *p;
    xmlChar        *buff, *q;
    uint32          len, numlen, i;
    xmlns_id_t      ncx_id;

    if (!ref) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();
    len = xml_strlen(ref);

    retval = xml_val_new_struct(YANG_K_REFERENCE,  ncx_id);
    if (!retval) {
        return NULL;
    }

    textval = xml_val_new_cstring(NCX_EL_TEXT, ncx_id, ref);
    if (!textval) {
        val_free_value(retval);
        return NULL;
    } else {
        val_add_child(textval, retval);
    }

    /* check if the reference is to an RFC and if so generate
     * a 'url' element as well
     */
    if (len > 4 && 
        !xml_strncmp(ref, (const xmlChar *)"RFC ", 4)) {
        str = &ref[4];
        p = str;
        while (*p && isdigit(*p)) {
            p++;
        }
        numlen = (uint32)(p-str);

        /* IETF RFC URLs are currently hard-wired 4 digit number */
        if (numlen && (numlen <= 4) && (len >= numlen+4)) {
            buff = m__getMem(xml_strlen(XSD_RFC_URL) + 9);
            if (!buff) {
                val_free_value(retval);
                return NULL;
            }

            q = buff;
            q += xml_strcpy(q, XSD_RFC_URL);
            for (i = 4 - numlen; i; i--) {
                *q++ = '0';
            }
            for (i=0; i<numlen; i++) {
                *q++ = *str++;
            } 
            xml_strcpy(q, (const xmlChar *)".txt");

            urlval = xml_val_new_string(NCX_EL_URL, ncx_id, buff);
            if (!urlval) {
                m__free(buff);
                val_free_value(retval);
                return NULL;
            } else {
                val_add_child(urlval, retval);
            }
        }
    } else if (len > 6 &&
               !xml_strncmp(ref, (const xmlChar *)"draft-", 6)) {
        str = &ref[6];
        while (*str && 
               (!xml_isspace(*str)) && 
               (*str != ';') && 
               (*str != ':') && 
               (*str != ',')) {
            str++;
        }

        if (str != &ref[6] && *str == '.') {    \
            str--;
        }

        numlen = (uint32)(str-ref);
        buff = m__getMem(xml_strlen(XSD_DRAFT_URL) + numlen + 1);
        if (!buff) {
            val_free_value(retval);
            return NULL;
        }
        q = buff;
        q += xml_strcpy(q, XSD_DRAFT_URL);
        q += xml_strncpy(q, ref, numlen);

        urlval = xml_val_new_string(NCX_EL_URL, ncx_id, buff);
        if (!urlval) {
            m__free(buff);
            val_free_value(retval);
            return NULL;
        } else {
            val_add_child(urlval, retval);
        }
    }

    return retval;

}   /* make_reference */


/********************************************************************
* FUNCTION make_obj_appinfo
* 
*   Make an appinfo element for an object, if needed
*
* INPUTS:
*    obj == obj_template to check
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced appinfo struct
*    NULL if nothing to do (*res == NO_ERR)
*    NULL if some error (*res != NO_ERR)
*********************************************************************/
static val_value_t *
    make_obj_appinfo (obj_template_t *obj,
                      status_t  *res)
{
    val_value_t          *newval, *appval, *mustval, *refval;
    dlq_hdr_t            *mustQ, *appinfoQ, *datadefQ;
    xpath_pcb_t          *must;
    ncx_errinfo_t        *errinfo;
    obj_template_t       *outputobj;
    const xmlChar        *units, *presence, *ref;
    xmlChar              *buff;
    ncx_status_t          status;
    xmlns_id_t            ncx_id;
    boolean               needed, config_needed, config;

#ifdef DEBUG
    if (!obj || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    outputobj = NULL;
    needed = FALSE;
    config_needed = FALSE;
    ncx_id = xmlns_ncx_id();
    *res = ERR_INTERNAL_MEM;    /* setup only error for now */

    status = obj_get_status(obj);
    config = obj_get_config_flag2(obj, &config_needed);
    mustQ = obj_get_mustQ(obj);
    appinfoQ = obj_get_appinfoQ(obj);

    if (mustQ && !dlq_empty(mustQ)) {
        needed = TRUE;
    }
    if (appinfoQ && !dlq_empty(appinfoQ)) {
        needed = TRUE;
    }

    switch (obj->objtype) {
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
        needed = TRUE;
        break;
    case OBJ_TYP_USES:
        needed = FALSE;
        break;
    case OBJ_TYP_AUGMENT:
        needed = (obj->flags & OBJ_FL_TOP) ? TRUE : FALSE;
        break;
    case OBJ_TYP_RPC:
        datadefQ = obj_get_datadefQ(obj);
        outputobj = obj_find_template(datadefQ,
                                      NULL, 
                                      YANG_K_OUTPUT);
        if (outputobj) {
            needed = TRUE;
        }
        break;
    case OBJ_TYP_RPCIO:
        break;
    case OBJ_TYP_NOTIF:
        break;
    default:
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (!needed) {
        /* appinfo not needed so exit now */
        *res = NO_ERR;
        return NULL;
    }

    /* appinfo may still not be needed for many object types
     * If not then delete appval at the end
     */
    needed = FALSE;

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    /* add choice or case name if needed */
    if (obj->objtype==OBJ_TYP_CHOICE) {
        needed = TRUE;
        newval = xml_val_new_cstring(NCX_EL_CHOICE_NAME, 
                                     ncx_id, 
                                     obj_get_name(obj));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    } else if (obj->objtype==OBJ_TYP_CASE) {
        needed = TRUE;
        newval = xml_val_new_cstring(NCX_EL_CASE_NAME, 
                                     ncx_id, 
                                     obj_get_name(obj));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add status clause if needed */
    if (status != NCX_STATUS_NONE && status != NCX_STATUS_CURRENT) {
        needed = TRUE;
        newval = xml_val_new_cstring(YANG_K_STATUS, 
                                     ncx_id, 
                                     ncx_get_status_string(status));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add config clause if needed */
    if (config_needed) {
        needed = TRUE;
        newval = xml_val_new_cstring(YANG_K_CONFIG, 
                                     ncx_id, 
                                     config ? NCX_EL_TRUE : NCX_EL_FALSE);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add when clause if needed */
    if (obj->when && obj->when->exprstr) {
        needed = TRUE;
        newval = xml_val_new_cstring(YANG_K_WHEN, 
                                     ncx_id,
                                     obj->when->exprstr);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add must entries if needed */
    if (mustQ) {
        for (must = (xpath_pcb_t *)dlq_firstEntry(mustQ);
             must != NULL;
             must = (xpath_pcb_t *)dlq_nextEntry(must)) {

            errinfo = &must->errinfo;
            needed = TRUE;

            mustval = xml_val_new_struct(YANG_K_MUST, ncx_id);
            if (!mustval) {
                val_free_value(appval);
                return NULL;
            } else {
                val_add_child(mustval, appval);
            }

            if (must->exprstr) {
                newval = xml_val_new_cstring(NCX_EL_XPATH, 
                                             ncx_id,
                                             must->exprstr);
                if (!newval) {
                    val_free_value(appval);
                    return NULL;
                } else {
                    val_add_child(newval, mustval);
                }
            }

            if (errinfo->descr) {
                newval = xml_val_new_cstring(YANG_K_DESCRIPTION,
                                             ncx_id, 
                                             errinfo->descr);
                if (!newval) {
                    val_free_value(appval);
                    return NULL;
                } else {
                    val_add_child(newval, mustval);
                }
            }

            if (errinfo->ref) {
                newval = xml_val_new_cstring(YANG_K_REFERENCE,
                                             ncx_id, 
                                             errinfo->ref);
                if (!newval) {
                    val_free_value(appval);
                    return NULL;
                } else {
                    val_add_child(newval, mustval);
                }
            }

            if (errinfo->error_app_tag) {
                newval = xml_val_new_cstring(YANG_K_ERROR_APP_TAG,
                                             ncx_id,
                                             errinfo->error_app_tag);
                if (!newval) {
                    val_free_value(appval);
                    return NULL;
                } else {
                    val_add_child(newval, mustval);
                }
            }

            if (errinfo->error_message) {
                newval = xml_val_new_cstring(YANG_K_ERROR_MESSAGE,
                                             ncx_id,
                                             errinfo->error_message);
                if (!newval) {
                    val_free_value(appval);
                    return NULL;
                } else {
                    val_add_child(newval, mustval);
                }
            }
        }
    }

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        presence = obj_get_presence_string(obj);
        if (presence) {
            needed = TRUE;
            newval = xml_val_new_cstring(YANG_K_PRESENCE,
                                         ncx_id,
                                         presence);
            if (!newval) {
                val_free_value(appval);
                return NULL;
            } else {
                val_add_child(newval, appval);
            }
        }
        break;
    case OBJ_TYP_LEAF:
        units = obj_get_units(obj);
        if (units) {
            needed = TRUE;
            newval = xml_val_new_cstring(YANG_K_UNITS,
                                         ncx_id,
                                         units);
            if (!newval) {
                val_free_value(appval);
                return NULL;
            } else {
                val_add_child(newval, appval);
            }
        }
        /* fall through */
    case OBJ_TYP_ANYXML:
        if (obj->flags & OBJ_FL_MANDSET) {
            needed = TRUE;
            newval = xml_val_new_cstring(YANG_K_MANDATORY,
                                         ncx_id,
                                         (obj->flags & OBJ_FL_MANDATORY)
                                         ? NCX_EL_TRUE : NCX_EL_FALSE);
            if (!newval) {
                *res = ERR_INTERNAL_MEM;
                val_free_value(appval);
                return NULL;
            } else {
                val_add_child(newval, appval);
            }
        }
        break;
    case OBJ_TYP_LEAF_LIST:
        units = obj_get_units(obj);
        if (units) {
            needed = TRUE;
            newval = xml_val_new_cstring(YANG_K_UNITS, 
                                         ncx_id,
                                         units);
            if (!newval) {
                val_free_value(appval);
                return NULL;
            } else {
                val_add_child(newval, appval);
            }
        }
        newval = xml_val_new_cstring(YANG_K_ORDERED_BY, 
                                     ncx_id,
                                     obj_is_system_ordered(obj) ?
                                     YANG_K_SYSTEM : YANG_K_USER);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            needed = TRUE;
            val_add_child(newval, appval);
        }
        break;
    case OBJ_TYP_LIST:
        needed = TRUE;
        newval = xml_val_new_cstring(YANG_K_ORDERED_BY, 
                                     ncx_id,
                                     obj_is_system_ordered(obj) ?
                                     YANG_K_SYSTEM : YANG_K_USER);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
        break;
    case OBJ_TYP_CHOICE:
        if (obj->flags & OBJ_FL_MANDSET) {
            needed = TRUE;
            newval = xml_val_new_cstring(YANG_K_MANDATORY,
                                         ncx_id,
                                         (obj->flags & OBJ_FL_MANDATORY)
                                         ? NCX_EL_TRUE : NCX_EL_FALSE);
            if (!newval) {
                val_free_value(appval);
            } else {
                val_add_child(newval, appval);
            }
        }
        break;
    case OBJ_TYP_CASE:
        break;
    case OBJ_TYP_USES:
        break;
    case OBJ_TYP_AUGMENT:
        break;
    case OBJ_TYP_RPC:
        if (outputobj) {
            needed = TRUE;
            buff = xsd_make_rpc_output_typename(obj);
            newval = xml_val_new_string(NCX_EL_RPC_OUTPUT,
                                        ncx_id, buff);
            if (!newval) {
                m__free(buff);
                val_free_value(appval);
            } else {
                val_add_child(newval, appval);
            }
        }
        break;
    case OBJ_TYP_RPCIO:
        break;
    case OBJ_TYP_NOTIF:
        break;
    default:
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        val_free_value(appval);
        return NULL;
    }

    /* check if reference needed */
    ref = obj_get_reference(obj);
    if (ref != NULL) {
        needed = TRUE;
        refval = make_reference(ref);
        if (!refval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(refval, appval);
        }
    }

    /* add an element for each appinfo record */
    if (appinfoQ && !dlq_empty(appinfoQ)) {
        needed = TRUE;
        *res = add_appinfoQ(appinfoQ, appval);
    }

    if (!needed) {
        val_free_value(appval);
        appval = NULL;
    }

    *res = NO_ERR;
    return appval;

}  /* make_obj_appinfo */


/********************************************************************
* FUNCTION new_element
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
*    forcetyp == typ_template_t to use if this is a parm element
*             == NULL if not a parm element
* RETURNS:
*   new element data struct or NULL if malloc error
*********************************************************************/
static val_value_t *
    new_element (const ncx_module_t *mod,
                 const xmlChar *name,
                 const typ_def_t *typdef,
                 const typ_def_t *parent,
                 boolean hasnodes,
                 boolean hasindex,
                 const typ_template_t *forcetyp)
{
    val_value_t   *val;
    const xmlChar *def;
    uint32         maxrows;
    ncx_iqual_t    iqual;
    ncx_btype_t    btyp;
    status_t       res;
    xmlChar        numbuff[NCX_MAX_NUMLEN];

    btyp = typ_get_basetype(typdef);

    /* create a node named 'element' to hold the result */
    if (hasnodes || hasindex) {
        val = xml_val_new_struct(XSD_ELEMENT, xmlns_xs_id());
    } else {
        val = xml_val_new_flag(XSD_ELEMENT, xmlns_xs_id());
    }
    if (!val) {
        return NULL;
    }

    /* add the name attribute */
    res = xml_val_add_cattr(NCX_EL_NAME, 0, name, val);
    if (res != NO_ERR) {
        val_free_value(val);
        return NULL;
    }

    /* add the type attribute unless this is a complex element */
    if (forcetyp) {
        res = xsd_add_parmtype_attr(mod->nsid, forcetyp, val);
        if (res != NO_ERR) {
            val_free_value(val);
            return NULL;
        }            
    } else if (!hasnodes && btyp != NCX_BT_EMPTY) {
        /* add the type attribute */
        res = xsd_add_type_attr(mod, typdef, val);
        if (res != NO_ERR) {
            val_free_value(val);
            return NULL;
        }
    } /* else the type definition should be part of the element */

    /* check if a default attribute is needed */
    def = typ_get_default(typdef);
    if (def) {
        res = xml_val_add_cattr(NCX_EL_DEFAULT, 0, def, val);
        if (res != NO_ERR) {
            val_free_value(val);
            return NULL;
        }
    }

    /* if this is the child node of a container, then
     * override the iqual values in the child and use
     * the parent container node iqual values instead
     *
     * A container is always allowed to be empty, so
     * it can have a consistent start state validatation check
     */

    if (parent) {
        switch (typ_get_basetype(parent)) {
        case NCX_BT_LIST:      /************/
            iqual = NCX_IQUAL_ZMORE;
            maxrows = typ_get_maxrows(parent);
            break;
        default:
            iqual = typ_get_iqualval_def(typdef);
            maxrows = typ_get_maxrows(typdef);
        }
    } else {
        iqual = typ_get_iqualval_def(typdef);
        maxrows = typ_get_maxrows(typdef);
    }

    /* add the minOccurs and maxOccurs as needed */
    if (maxrows) {
        res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, val);
        if (res == NO_ERR) {
            sprintf((char *)numbuff, "%u", maxrows);
            res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, numbuff, val);
        }
    } else {
        switch (iqual) {
        case NCX_IQUAL_ONE:
            /* do nothing since this is the default */
            break;
        case NCX_IQUAL_1MORE:
            res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, XSD_UNBOUNDED, val);
            break;
        case NCX_IQUAL_OPT:
            res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, val);
            break;
        case NCX_IQUAL_ZMORE:
            res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, val);
            if (res == NO_ERR) {
                res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, XSD_UNBOUNDED, val);
            }
            break;
        case NCX_IQUAL_NONE:
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    if (res != NO_ERR) {
        val_free_value(val);
        return NULL;
    }

    return val;

}   /* new_element */


/********************************************************************
* FUNCTION add_basetype_attr
* 
*   Generate a type or base attribute
*
* INPUTS:
*    istype == TRUE if this is a type attribute
*           == FALSE if this is a base attribute
*    mod == module in progress
*    typdef == typ_def for the typ_template struct to use for 
*              the attribute list source
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->v.childQ has entries added for the attributes in this typdef
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_basetype_attr (boolean istype,
                       const ncx_module_t *mod,
                       const typ_def_t *typdef,
                       val_value_t *val)
{

    xmlChar       *str;
    const xmlChar *name, *attrname;
    status_t       res;
    xmlns_id_t     typ_id, targns;
    ncx_btype_t    btyp;

    targns = mod->nsid;

    /* figure out the correct namespace and name for the typename */
    switch (typdef->tclass) {
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
        typ_id = xmlns_xs_id();
        name = xsd_typename(typ_get_basetype(typdef));
        break;
    case NCX_CL_COMPLEX:
        btyp = typ_get_basetype(typdef);
        switch (btyp) {
        case NCX_BT_ANY:
            typ_id = xmlns_xs_id();
            name = xsd_typename(btyp);            
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_CL_NAMED:
        typ_id = typdef->def.named.typ->nsid;
        if (typ_id) {
            /* namespace assigned means it is a top-level exported def */
            name = typdef->def.named.typ->name;
        } else {
            /* inside a grouping, include submod, etc.
             * this has to be a type in the current [sub]module
             * or it would have a NSID assigned already
             *
             * The design of the local typename to 
             * global XSD typename translation does not
             * put top-level typedefs in the typenameQ.
             * They are already in the registry, so name collision
             * can be checked that way
             */
            typ_id = mod->nsid;
            name = ncx_find_typname(typdef->def.named.typ, 
                                    &mod->typnameQ);
            if (!name) {
                /* this must be a local type name in a grouping.
                 * it could also be a local type inside a node
                 */
                name = typdef->def.named.typ->name;
            }
        }
        break;
    case NCX_CL_REF:
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (istype) {
        attrname = NCX_EL_TYPE;
    } else {
        attrname = XSD_BASE;
    }

    /* set the data type */
    if (typ_id == targns) {
        /* this is in the targetNamespace */
        return xml_val_add_cattr(attrname, 0, name, val);
    } else {
        /* construct the QName for the attribute type */
        str = xml_val_make_qname(typ_id, name);
        if (!str) {
            return ERR_INTERNAL_MEM;
        }

        /* add the base or type attribute */
        res = xml_val_add_attr(attrname, 0, str, val);
        if (res != NO_ERR) {
            m__free(str);
            return res;
        }
    }

    return NO_ERR;

}   /* add_basetype_attr */


/********************************************************************
* FUNCTION make_documentation
* 
*   Create a value struct representing the documentation node
*   This is complete; The val_free_value function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    descr == description clause (may be NULL)
*
* RETURNS:
*   value struct or NULL if malloc error
*********************************************************************/
static val_value_t *
    make_documentation (const xmlChar *descr)
{
    if (!descr) {
        return NULL;
    } else {
        return xml_val_new_cstring(XSD_DOCUMENTATION, xmlns_xs_id(), descr);
    }
}   /* make_documentation */


/********************************************************************
* FUNCTION make_schema_loc
* 
*   Get the schemaLocation string for this module
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* !!! TEMP: Location Hardwired to the following string!!!
*
*   <schemaloc>/<module>.xsd
*
* INPUTS:
*    schemaloc == URL prefix parameter from user (may be NULL)
*    modname == module name
*    modname == module version string (may be NULL)
*    cstr == const URI string to use
*    versionnames == TRUE if version in filename requested (CLI default)
*                 == FALSE for no version names in the filenames
*                 Ignored if modversion is NULL
* RETURNS:
*   malloced string or NULL if malloc error
*********************************************************************/
static xmlChar *
    make_schema_loc (const xmlChar *schemaloc,
                     const xmlChar *modname,
                     const xmlChar *modversion,
                     const xmlChar *cstr,
                     boolean versionnames)
{
    xmlChar       *str, *str2;
    uint32         len, slen, plen;

    slen = (schemaloc) ? xml_strlen(schemaloc) : 0;
    plen = (uint32)((slen && schemaloc[slen-1] != '/') ? 1 : 0);

    len = 9;  /* newline + 4 spaces + file ext */
    len += slen;    
    len += plen;
    len += xml_strlen(cstr);
    len += xml_strlen(modname);
    if (versionnames && modversion) {
        len += (xml_strlen(modversion) + 1);
    }

    str = m__getMem(len+1);
    if (!str) {
        return NULL;
    }

    str2 = str;
    str2 += xml_strcpy(str2, cstr);
    str2 += xml_strcpy(str2, (const xmlChar *)"\n    ");
    if (schemaloc && *schemaloc) {
        str2 += xml_strcpy(str2, schemaloc);
        if (plen) {
            *str2++ = '/';
        }
    }
    str2 += xml_strcpy(str2, modname);
    if (versionnames && modversion) {
        *str2++ = '_';    /***** CHANGE TO DOT *****/
        str2 += xml_strcpy(str2, modversion);
    }
    *str2++ = '.';
    str2 += xml_strcpy(str2, NCX_XSD_EXT);

    return str;

}   /* make_schema_loc */


/********************************************************************
* FUNCTION make_appinfo
* 
*   Go through the appinfoQ and add a child node to the
*   parent struct, which should be named 'appinfo'
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    appinfoQ == queue to check
*    maxacc   == value > none if max access clause should be added
*    condition == condition string (may be NULL)
*    units == units string (may be NULL)
*    ref == reference statement (may be NULL)
*    status == status value (may be NCX_STATUS_NONE)
*              clause not generated for default (current)
*
* RETURNS:
*   malloced value containing all the appinfo or NULL if malloc error
*********************************************************************/
static val_value_t *
    make_appinfo (const dlq_hdr_t  *appinfoQ,
                  ncx_access_t maxacc,
                  const xmlChar *condition,
                  const xmlChar *units,
                  const xmlChar *ref,
                  ncx_status_t status)
{
    val_value_t   *newval, *appval;
    xmlns_id_t     ncx_id;
    status_t       res;

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();

    /* add status clause if needed */
    if (status != NCX_STATUS_NONE && status != NCX_STATUS_CURRENT) {
        newval = xml_val_new_cstring(NCX_EL_STATUS, ncx_id, 
                                 ncx_get_status_string(status));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add units clause if needed */
    if (units) {
        newval = xml_val_new_cstring(NCX_EL_UNITS, ncx_id, units);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add max-access clause if needed */
    if (maxacc != NCX_ACCESS_NONE) {
        newval = xml_val_new_cstring(NCX_EL_MAX_ACCESS, ncx_id, 
                                 ncx_get_access_str(maxacc));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add condition clause if needed */
    if (condition) {
        newval = xml_val_new_cstring(NCX_EL_CONDITION, ncx_id, condition);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add reference clause if needed */
    if (ref) {
        newval = make_reference(ref);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add an element for each appinfo record */
    if (appinfoQ && !dlq_empty(appinfoQ)) {
        res = add_appinfoQ(appinfoQ, appval);
        if (res != NO_ERR) {
            val_free_value(appval);
            return NULL;
        }
    }

    return appval;

}   /* make_appinfo */


/********************************************************************
* FUNCTION make_type_appinfo
* 
*   Go through the appinfoQ and add a child node to the
*   parent struct, which should be named 'appinfo'
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    typ == typ_template_t to check
*    path == path strnig for leafref or NULL if N/A
*
* RETURNS:
*   malloced value containing all the appinfo or NULL if malloc error
*********************************************************************/
static val_value_t *
    make_type_appinfo (const typ_template_t *typ,
                       const xmlChar *path)
{
    val_value_t   *newval, *appval;
    xmlns_id_t     ncx_id;
    status_t       res;

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();

    /* add reference clause if needed */
    if (typ->ref) {
        newval = make_reference(typ->ref);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add status clause if needed */
    if (typ->status != NCX_STATUS_NONE && typ->status != NCX_STATUS_CURRENT) {
        newval = xml_val_new_cstring(NCX_EL_STATUS, ncx_id, 
                                     ncx_get_status_string(typ->status));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add units clause if needed */
    if (typ->units) {
        newval = xml_val_new_cstring(NCX_EL_UNITS, ncx_id, typ->units);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add default clause if needed */
    if (typ->defval) {
        newval = xml_val_new_cstring(NCX_EL_DEFAULT, 
                                     ncx_id, 
                                     typ->defval);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add path for leafref if needed */
    if (path) {
        newval = xml_val_new_cstring(YANG_K_PATH, ncx_id, path);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    /* add an element for each appinfo record */
    if (!dlq_empty(&typ->appinfoQ)) {
        res = add_appinfoQ(&typ->appinfoQ, appval);
        if (res != NO_ERR) {
            val_free_value(appval);
            return NULL;
        }
    }

    return appval;

}   /* make_type_appinfo */


/********************************************************************
* FUNCTION add_one_import
* 
*   Add one import node
*
* INPUTS:
*    pcb == parser control block
*    modname == name of module
*    revision == revision date of module
*    cp == conversion parameters in use
*    val == struct parent to contain child nodes for each import
*
* OUTPUTS:
*    val->childQ has entries added for the imports required
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_one_import (yang_pcb_t *pcb,
                    const xmlChar *modname,
                    const xmlChar *revision,
                    const yangdump_cvtparms_t *cp,
                    val_value_t *val)
{
    val_value_t         *imval;
    ncx_module_t        *immod;
    xmlChar             *str;
    xmlns_id_t           id;
    status_t             res;

    id = xmlns_xs_id();

    immod = ncx_find_module(modname, revision);
    if (!immod) {
        res = ncxmod_load_module(modname, 
                                 revision, 
                                 pcb->savedevQ,
                                 &immod);
        if (!immod) {
            return res;
        }
    }
    
    imval = xml_val_new_flag(NCX_EL_IMPORT, id);
    if (!imval) {
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(imval, val);
    }

    res = xml_val_add_cattr(NCX_EL_NAMESPACE, 0, 
                            xmlns_get_ns_name(immod->nsid), 
                            imval);
    if (res == NO_ERR) {
        str = xsd_make_schema_location(immod, 
                                       cp->schemaloc, 
                                       cp->versionnames);
        if (str) {
            res = xml_val_add_attr(XSD_LOC, 0, str, imval);
            if (res != NO_ERR) {
                m__free(str);
            }
        }
    }

    return res;

}   /* add_one_import */


/************ E X T E R N A L    F U N C T I O N S   ******/


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
const xmlChar *
    xsd_typename (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_ANY:
        return XSD_ANY_TYPE;
    case NCX_BT_BITS:
    case NCX_BT_ENUM:
        return NCX_EL_STRING;
    case NCX_BT_EMPTY:
        return NCX_EL_STRING;
    case NCX_BT_BOOLEAN:
        return NCX_EL_BOOLEAN;
    case NCX_BT_INT8:
        return NCX_EL_BYTE;
    case NCX_BT_INT16:
        return NCX_EL_SHORT;
    case NCX_BT_INT32:
        return NCX_EL_INT;
    case NCX_BT_INT64:
        return NCX_EL_LONG;
    case NCX_BT_UINT8:
        return NCX_EL_UNSIGNED_BYTE;
    case NCX_BT_UINT16:
        return NCX_EL_UNSIGNED_SHORT;
    case NCX_BT_UINT32:
        return NCX_EL_UNSIGNED_INT;
    case NCX_BT_UINT64:
        return NCX_EL_UNSIGNED_LONG;
    case NCX_BT_DECIMAL64:
        return NCX_EL_LONG;
    case NCX_BT_FLOAT64:
        return NCX_EL_DOUBLE;
    case NCX_BT_STRING:
    case NCX_BT_LEAFREF:  /***/
    case NCX_BT_INSTANCE_ID:
        return NCX_EL_STRING;
    case NCX_BT_IDREF:
        return XSD_QNAME;
    case NCX_BT_BINARY:
        return XSD_BASE64;
    case NCX_BT_SLIST:
        return XSD_LIST;
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_LIST:
        return NCX_EL_NONE;   /***/
    case NCX_BT_UNION:
        return NCX_EL_UNION;
    case NCX_BT_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_EL_NONE;   /***/
    }
    /*NOTREACHED*/
}   /* xsd_typename */


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
xmlChar *
    xsd_make_basename (const typ_template_t *typ)
{
    xmlChar          *basename, *str2;

    /* hack: make a different name for the base type
     * that will not be hidden
     */
    basename = m__getMem(xml_strlen(typ->name) 
                         + xml_strlen(XSD_BT) + 1);
    if (!basename) {
        return NULL;
    }
    str2 = basename;
    str2 += xml_strcpy(str2, typ->name);
    str2 += xml_strcpy(str2, XSD_BT);
    return basename;

}   /* xsd_make_basename */


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
val_value_t *
    xsd_make_enum_appinfo (int32 enuval,
                           const dlq_hdr_t  *appinfoQ,
                           ncx_status_t status)
{
    val_value_t         *newval, *appval;
    ncx_num_t            num;
    xmlChar              numbuff[NCX_MAX_NUMLEN];
    xmlns_id_t           ncx_id;
    status_t             res;
    uint32               len;

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();

    /* add status clause if not set to default (current) */
    switch (status) {
    case NCX_STATUS_CURRENT:
    case NCX_STATUS_NONE:
        break;
    case NCX_STATUS_DEPRECATED:
    case NCX_STATUS_OBSOLETE:
        newval = xml_val_new_cstring(NCX_EL_STATUS, ncx_id, 
                                     ncx_get_status_string(status));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* add enum value clause */
    num.i = enuval;
    res = ncx_sprintf_num(numbuff, &num, NCX_BT_INT32, &len);
    if (res != NO_ERR) {
        val_free_value(appval);
        return NULL;
    }
    newval = xml_val_new_cstring(NCX_EL_VALUE, ncx_id, numbuff);
    if (!newval) {
        val_free_value(appval);
        return NULL;
    } else {
        val_add_child(newval, appval);
    }

    /* add an element for each appinfo record */
    if (appinfoQ) {
        res = add_appinfoQ(appinfoQ, appval);
        if (res != NO_ERR) {
            val_free_value(appval);
            return NULL;
        }
    }

    return appval;

}   /* xsd_make_enum_appinfo */


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
val_value_t *
    xsd_make_bit_appinfo (uint32 bitpos,
                          const dlq_hdr_t  *appinfoQ,
                          ncx_status_t status)
{
    val_value_t         *newval, *appval;
    ncx_num_t            num;
    xmlChar              numbuff[NCX_MAX_NUMLEN];
    xmlns_id_t           ncx_id;
    status_t             res;
    uint32               len;

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();

    /* add status clause if not set to default (current) */
    switch (status) {
    case NCX_STATUS_CURRENT:
    case NCX_STATUS_NONE:
        break;
    case NCX_STATUS_DEPRECATED:
    case NCX_STATUS_OBSOLETE:
        newval = xml_val_new_cstring(NCX_EL_STATUS, ncx_id, 
                                     ncx_get_status_string(status));
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* add bit position clause */
    num.u = bitpos;
    res = ncx_sprintf_num(numbuff, &num, NCX_BT_UINT32, &len);
    if (res != NO_ERR) {
        val_free_value(appval);
        return NULL;
    }
    newval = xml_val_new_cstring(NCX_EL_POSITION, ncx_id, numbuff);
    if (!newval) {
        val_free_value(appval);
        return NULL;
    } else {
        val_add_child(newval, appval);
    }

    /* add an element for each appinfo record */
    if (appinfoQ) {
        res = add_appinfoQ(appinfoQ, appval);
        if (res != NO_ERR) {
            val_free_value(appval);
            return NULL;
        }
    }

    return appval;

}   /* xsd_make_bit_appinfo */


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
val_value_t *
    xsd_make_err_appinfo (const xmlChar *ref,
                          const xmlChar *errmsg,
                          const xmlChar *errtag)
{
    val_value_t   *appval, *newval;
    xmlns_id_t     ncx_id;

#ifdef DEBUG
    if (!(ref || errmsg || errtag)) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    appval = xml_val_new_struct(NCX_EL_APPINFO, xmlns_xs_id());
    if (!appval) {
        return NULL;
    }

    ncx_id = xmlns_ncx_id();

    if (ref) {
        newval = make_reference(ref);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    if (errtag) {
        newval = xml_val_new_cstring(NCX_EL_ERROR_APP_TAG,
                                     ncx_id, errtag);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    if (errmsg) {
        newval = xml_val_new_cstring(NCX_EL_ERROR_MESSAGE,
                                     ncx_id, errmsg);
        if (!newval) {
            val_free_value(appval);
            return NULL;
        } else {
            val_add_child(newval, appval);
        }
    }

    return appval;

}   /* xsd_make_err_appinfo */


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
xmlChar *
    xsd_make_schema_location (const ncx_module_t *mod,
                              const xmlChar *schemaloc,
                              boolean versionnames)
{
    const xmlChar *cstr;

    if (mod->nsid) {
        cstr = xmlns_get_ns_name(mod->nsid);
    } else {
        cstr = EMPTY_STRING;
    }
    if (!cstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }

    return make_schema_loc(schemaloc, mod->name, 
                           mod->version, cstr, versionnames);

}   /* xsd_make_schema_location */


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
xmlChar *
    xsd_make_output_filename (const ncx_module_t *mod,
                              const yangdump_cvtparms_t *cp)
{
    xmlChar        *buff, *p;
    const xmlChar  *ext;
    uint32          len;

    switch (cp->format) {
    case NCX_CVTTYP_SQL:
    case NCX_CVTTYP_SQLDB:
        ext = (const xmlChar *)".sql";
        break;
    case NCX_CVTTYP_HTML:
        if (cp->html_div) {
            ext = (const xmlChar *)".div";
        } else {
            ext = (const xmlChar *)".html";
        }
        break;
    case NCX_CVTTYP_XSD:
        ext = (const xmlChar *)".xsd";
        break;
    case NCX_CVTTYP_H:
    case NCX_CVTTYP_UH:
    case NCX_CVTTYP_YH:
        ext = (const xmlChar *)".h";
        break;
    case NCX_CVTTYP_C:
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_YC:
        ext = (const xmlChar *)".c";
        break;
    case NCX_CVTTYP_CPP_TEST:
        ext = (const xmlChar *)".cpp";
        break;
    case NCX_CVTTYP_YANG:
    case NCX_CVTTYP_COPY:
        ext = (const xmlChar *)".yang";
        break;
    case NCX_CVTTYP_YIN:
        ext = (const xmlChar *)".yin";
        break;
    case NCX_CVTTYP_TG2:
        /*** check cp for submode of TG2 ***/
        ext = (const xmlChar *)".py";
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (cp->output && *cp->output) {
        len = xml_strlen(cp->full_output);
        if (cp->output_isdir) {
            if (cp->full_output[len-1] != NCXMOD_PSCHAR) {
                len++;
            }
            if (!cp->versionnames) {
                len += xml_strlen(mod->name) + xml_strlen(ext); 
            } else if (mod->version) {
                len += xml_strlen(mod->name) + xml_strlen(mod->version) + 1 +
                    xml_strlen(ext); 
            } else {
                len += xml_strlen(mod->name) + xml_strlen(ext); 
            }
        }
    } else {
        if (!cp->versionnames) {
            len = xml_strlen(mod->name) + xml_strlen(ext); 
        } else if (mod->version) {
            len = xml_strlen(mod->name) + xml_strlen(mod->version) + 1 +
                xml_strlen(ext); 
        } else {
            len = xml_strlen(mod->name) + xml_strlen(ext); 
        }
    }

    switch (cp->format) {
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_UH:
        len += xml_strlen(NCX_USER_SIL_PREFIX);
        break;
    case NCX_CVTTYP_YC:
    case NCX_CVTTYP_YH:
        len += xml_strlen(NCX_YUMA_SIL_PREFIX);
        break;
    default:
        ;
    }

    buff = m__getMem(len+1);
    if (!buff) {
        return NULL;
    }

    p = buff;

    if (cp->output && *cp->output) {
        if (cp->output_isdir) {
            p += xml_strcpy(p, (const xmlChar *)cp->full_output);
            if (*(p-1) != NCXMOD_PSCHAR) {
                *p++ = NCXMOD_PSCHAR;
            }
        } else {
            /* !!! y_ and u_ not supported here !!!
             * user has specified the full file name
             */
            xml_strcpy(p, (const xmlChar *)cp->full_output);
            return buff;
        }
    }

    switch (cp->format) {
    case NCX_CVTTYP_UC:
    case NCX_CVTTYP_UH:
        p += xml_strcpy(p, NCX_USER_SIL_PREFIX);
        break;
    case NCX_CVTTYP_YC:
    case NCX_CVTTYP_YH:
        p += xml_strcpy(p, NCX_YUMA_SIL_PREFIX);
        break;
    default:
        ;
    }

    p += xml_strcpy(p, mod->name);
    if (cp->versionnames && mod->version) {
        *p++ = YANG_FILE_SEPCHAR;
        p += xml_strcpy(p, mod->version);
    }
    xml_strcpy(p, ext); 

    return buff;

}   /* xsd_make_output_filename */


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
status_t
    xsd_do_documentation (const xmlChar *descr,
                          val_value_t *val)
{
    val_value_t *chnode;

    if (descr) {
        chnode = make_documentation(descr);
        if (!chnode) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(chnode, val);
        }
    }

    return NO_ERR;

}   /* xsd_do_documentation */


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
status_t
    xsd_do_reference (const xmlChar *ref,
                      val_value_t *val)
{
    val_value_t *chnode;

    if (ref) {
        chnode = make_reference(ref);
        if (!chnode) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(chnode, val);
        }
    }

    return NO_ERR;

}   /* xsd_do_reference */


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
status_t
    xsd_add_mod_documentation (const ncx_module_t *mod,
                               val_value_t *annot)
{
    val_value_t      *doc, *appinfo, *appval;
    const xmlChar    *banner0, *banner1, *banner1b;
    xmlChar          *str, *str2;
    uint32            len, versionlen;
    xmlns_id_t        xsd_id, ncx_id;
    status_t          res;
    xmlChar           versionbuffer[NCX_VERSION_BUFFSIZE];

    xsd_id = xmlns_xs_id();
    ncx_id = xmlns_ncx_id();

    res = ncx_get_version(versionbuffer, NCX_VERSION_BUFFSIZE);
    if (res != NO_ERR) {
        SET_ERROR(res);
        xml_strcpy(versionbuffer, (const xmlChar *)"unknown");
    }
    versionlen = xml_strlen(versionbuffer);

    /* create first <documentation> element */
    banner0 = XSD_BANNER_0Y;
    banner1 = (mod->ismod) ? XSD_BANNER_1 : XSD_BANNER_1S;
    banner1b = (mod->ismod) ? NULL : XSD_BANNER_1B;

    /* figure out the buffer length first */
    len = xml_strlen(banner0) +
        xml_strlen(mod->sourcefn) +
        xml_strlen(XSD_BANNER_0END) +
        versionlen + 
        xml_strlen(banner1) +           /* (Sub)Module */
        xml_strlen(XSD_BANNER_3);            /* Version */

    if (mod->organization) {
        len += xml_strlen(XSD_BANNER_2) +     /* organization */
            xml_strlen(mod->organization);
    }

    if (banner1b) {
        len += xml_strlen(XSD_BANNER_1B) + xml_strlen(mod->belongs);
    }

    len += xml_strlen(mod->name);

    if (mod->version) {
        len += xml_strlen(mod->version);
    } else {
        len += xml_strlen(NCX_EL_NONE);
    }

    if (mod->contact_info) {
        len += (xml_strlen(XSD_BANNER_5) + xml_strlen(mod->contact_info));
    }

    str = m__getMem(len+1);
    if (!str) {
        return ERR_INTERNAL_MEM;
    }

    /* build the module documentation in one buffer */
    str2 = str;
    str2 += xml_strcpy(str2, banner0);
    str2 += xml_strcpy(str2, mod->sourcefn);
    str2 += xml_strcpy(str2, XSD_BANNER_0END);
    str2 += xml_strcpy(str2, versionbuffer);
    str2 += xml_strcpy(str2, banner1);
    str2 += xml_strcpy(str2, mod->name);

    if (banner1b) {
        str2 += xml_strcpy(str2, XSD_BANNER_1B);
        str2 += xml_strcpy(str2, mod->belongs);
    }

    if (mod->organization) {
        str2 += xml_strcpy(str2, XSD_BANNER_2);
        str2 += xml_strcpy(str2, mod->organization);
    }

    str2 += xml_strcpy(str2, XSD_BANNER_3);
    if (mod->version) {
        str2 += xml_strcpy(str2, mod->version);
    } else {
        str2 += xml_strcpy(str2, NCX_EL_NONE);
    }

    if (mod->contact_info) {
        str2 += xml_strcpy(str2, XSD_BANNER_5);
        str2 += xml_strcpy(str2, mod->contact_info);
    }

    doc = xml_val_new_string(XSD_DOCUMENTATION, xsd_id, str);
    if (!doc) {
        m__free(str);
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(doc, annot);
    }

    if (mod->descr) {
        doc = make_documentation(mod->descr);
        if (!doc) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(doc, annot);
        }
    }

    if (mod->ref || 
        mod->source ||
        mod->belongs || 
        mod->organization || 
        mod->contact_info) {

        appinfo = xml_val_new_struct(NCX_EL_APPINFO, xsd_id);
        if (!appinfo) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(appinfo, annot);
        }

        if (mod->ref) {
            appval = make_reference(mod->ref);
            if (!appval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(appval, appinfo);
            }
        }

        if (mod->source) {
            appval = xml_val_new_cstring(NCX_EL_SOURCE, ncx_id, mod->source);
            if (!appval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(appval, appinfo);
            }
        }

        if (mod->belongs) {
            appval = xml_val_new_cstring(YANG_K_BELONGS_TO, ncx_id, 
                                         mod->belongs);
            if (!appval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(appval, appinfo);
            }
        }

        if (mod->organization) {
            appval = xml_val_new_cstring(YANG_K_ORGANIZATION, ncx_id, 
                                         mod->organization);
            if (!appval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(appval, appinfo);
            }
        }

        if (mod->contact_info) {
            appval = xml_val_new_cstring(YANG_K_CONTACT, ncx_id,
                                         mod->contact_info);
            if (!appval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(appval, appinfo);
            }
        }
    }

    if (!dlq_empty(&mod->revhistQ)) {
        appinfo = xml_val_new_struct(NCX_EL_APPINFO, xsd_id);
        if (!appinfo) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(appinfo, annot);
        }
        res = add_revhist(mod, appinfo);
        if (res != NO_ERR) {
            return res;
        }
    }

    if (!dlq_empty(&mod->appinfoQ)) {
        appval = make_appinfo(&mod->appinfoQ,
                              NCX_ACCESS_NONE, 
                              NULL, 
                              NULL, 
                              NULL,
                              NCX_STATUS_CURRENT);
        if (!appval) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(appval, annot);
        }
    }        

    return NO_ERR;

}   /* xsd_add_mod_documentation */


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
status_t
    xsd_add_imports (yang_pcb_t *pcb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     val_value_t *val)
{
    val_value_t              *imval;
    const ncx_import_t       *import;
    const yang_import_ptr_t  *impptr;
    xmlChar                  *str;
    xmlns_id_t                id;
    status_t                  res;
    boolean                   use_xsi;

    use_xsi = FALSE;
    id = xmlns_xs_id();

    if (use_xsi) {
        /* add <import> element to val for the XML namespace XSD */
        imval = xml_val_new_flag(NCX_EL_IMPORT, id);
        if (!imval) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(imval, val);
        }

        res = xml_val_add_cattr(NCX_EL_NAMESPACE, 0, NCX_XML_URN, imval);
        if (res == NO_ERR) {
            res = xml_val_add_cattr(XSD_LOC, 0, NCX_XML_SLOC, imval);
        }
        if (res != NO_ERR) {
            return res;
        }
    }

    /* add import for NCX extensions used in the XSD,
     * but not imported into the data model module
     */
    imval = xml_val_new_flag(NCX_EL_IMPORT, id);
    if (!imval) {
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(imval, val);
    }
    res = xml_val_add_cattr(NCX_EL_NAMESPACE, 0, NCX_URN, imval);
    if (res != NO_ERR) {
        return res;
    }
    str = make_schema_loc(cp->schemaloc, 
                          NCX_MOD, 
                          (cp->versionnames) ? mod->version : NULL,
                          NCX_URN,
                          cp->versionnames);
    if (str) {
        res = xml_val_add_attr(XSD_LOC, 0, str, imval);
        if (res != NO_ERR) {
            m__free(str);
            return res;
        }
    } else {
        return ERR_INTERNAL_MEM;
    }

    /* add XSD import for each YANG import */
    if (cp->unified && mod->ismod) {
        for (impptr = (const yang_import_ptr_t *)dlq_firstEntry(&mod->saveimpQ);
             impptr != NULL;
             impptr = (const yang_import_ptr_t *)dlq_nextEntry(impptr)) {
            res = add_one_import(pcb,
                                 impptr->modname, 
                                 impptr->revision,
                                 cp, val);
            if (res != NO_ERR) {
                return res;
            }
        }
    } else {
        for (import = (const ncx_import_t *)dlq_firstEntry(&mod->importQ);
             import != NULL;
             import = (const ncx_import_t *)dlq_nextEntry(import)) {
            res = add_one_import(pcb,
                                 import->module, 
                                 import->revision,
                                 cp, val);
            if (res != NO_ERR) {
                return res;
            }
        }
    }

    return NO_ERR;

}   /* xsd_add_imports */


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
status_t
    xsd_add_includes (const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      val_value_t *val)
{
    val_value_t         *incval;
    const ncx_include_t *inc;
    xmlChar             *str;
    xmlns_id_t           id;
    status_t             res;

    id = xmlns_xs_id();

    for (inc = (const ncx_include_t *)dlq_firstEntry(&mod->includeQ);
         inc != NO_ERR;
         inc = (const ncx_include_t *)dlq_nextEntry(inc)) {

        incval = xml_val_new_flag(NCX_EL_INCLUDE, id);
        if (!incval) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(incval, val);   /* add early */
        }

        str = make_include_location(inc->submodule, cp->schemaloc);
        if (!str) {
            return ERR_INTERNAL_MEM;
        }

        res = xml_val_add_attr(XSD_LOC, 0, str, incval);
        if (res != NO_ERR) {
            m__free(str);
            return res;
        }
    }

    return NO_ERR;

}   /* xsd_add_includes */


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
val_value_t *
    xsd_new_element (const ncx_module_t *mod,
                     const xmlChar *name,
                     const typ_def_t *typdef,
                     const typ_def_t *parent,
                     boolean hasnodes,
                     boolean hasindex)
{
    return new_element(mod, name, typdef, parent, 
                       hasnodes, hasindex, NULL);
}   /* xsd_new_element */


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
val_value_t *
    xsd_new_leaf_element (const ncx_module_t *mod,
                          const obj_template_t *obj,
                          boolean hasnodes,
                          boolean addtype,
                          boolean iskey)

{
    val_value_t      *elem;
    const typ_def_t  *typdef;
    const xmlChar    *def, *leafdef;
    xmlChar           numbuff[NCX_MAX_NUMLEN];
    ncx_btype_t       btyp;
    boolean           isleaf;
    status_t          res;

    if (obj->objtype == OBJ_TYP_LEAF) {
        isleaf = TRUE;
        typdef = obj->def.leaf->typdef;
        leafdef = obj->def.leaf->defval;
        def = typ_get_default(typdef);
    } else {
        isleaf = FALSE;
        typdef = obj->def.leaflist->typdef;
        leafdef = NULL;
        def = NULL;
    }

    if (hasnodes) {
        elem = xml_val_new_struct(XSD_ELEMENT, xmlns_xs_id());
    } else {
        elem = xml_val_new_flag(XSD_ELEMENT, xmlns_xs_id());
    }
    if (!elem) {
        return NULL;
    }

    /* add the name attribute */
    res = xml_val_add_cattr(NCX_EL_NAME,
                            0,
                            obj_get_name(obj), 
                            elem);
    if (res != NO_ERR) {
        val_free_value(elem);
        return NULL;
    }

    if (addtype) {
        btyp = typ_get_basetype(typdef);
        if (btyp != NCX_BT_EMPTY) {
            /* add the type attribute */
            res = xsd_add_type_attr(mod, typdef, elem);
            if (res != NO_ERR) {
                val_free_value(elem);
                return NULL;
            }
        }
    }

    /* check if a default attribute is needed */
    if (!iskey) {
        if (leafdef) {
            res = xml_val_add_cattr(NCX_EL_DEFAULT, 0, leafdef, elem);
            if (res != NO_ERR) {
                val_free_value(elem);
                return NULL;
            }
        } else if (def) {
            res = xml_val_add_cattr(NCX_EL_DEFAULT, 0, def, elem);
            if (res != NO_ERR) {
                val_free_value(elem);
                return NULL;
            }
        }
    }

    if (isleaf) {
        if (!iskey) {
            /* add the attributes for a leaf */
            if (!(obj->flags & OBJ_FL_MANDATORY)) {
                res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, elem);
            }
        }
    } else {
        /* add the attributes for a leaf-list */
        if (obj->def.leaflist->minset) {
            sprintf((char *)numbuff, "%u", obj->def.leaflist->minelems);
            res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, numbuff, elem);
        } else {
            res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, elem);
        }
        if (res != NO_ERR) {
            val_free_value(elem);
            return NULL;
        }

        if (obj->def.leaflist->maxset) {
            if (obj->def.leaflist->maxelems) {
                sprintf((char *)numbuff, "%u", obj->def.leaflist->maxelems);
            } else {
                sprintf((char *)numbuff, "%s", XSD_UNBOUNDED);
            }
            res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, numbuff, elem);
        } else {
            res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, XSD_UNBOUNDED, elem);
        }
        if (res != NO_ERR) {
            val_free_value(elem);
            return NULL;
        }
    }

    return elem;

}   /* xsd_new_leaf_element */


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
status_t
    xsd_add_parmtype_attr (xmlns_id_t targns,
                           const typ_template_t *typ,
                           val_value_t *val)
{

    xmlChar       *str;
    status_t       res;

    /* set the data type */
    if (typ->nsid == targns) {
        /* this is in the targetNamespace */
        return xml_val_add_cattr(NCX_EL_TYPE, 0, typ->name, val);
    } else {
        /* construct the QName for the attribute type */
        str = xml_val_make_qname(typ->nsid, typ->name);
        if (!str) {
            return ERR_INTERNAL_MEM;
        }

        /* add the base or type attribute */
        res = xml_val_add_attr(NCX_EL_TYPE, 0, str, val);
        if (res != NO_ERR) {
            m__free(str);
            return res;
        }
    }

    return NO_ERR;

}   /* xsd_add_parmtype_attr */

#if 0  /* complex types not supported in YANG */
/********************************************************************
* FUNCTION xsd_add_key
* 
*   Generate the 'key' node for a table or container element
*
* INPUTS:
*    name == name of node that contains the key
*    typch == typ_def_t for the child node to use to gen a key
*    val == struct parent to contain child nodes for each type
*
* OUTPUTS:
*    val->v.childQ has entries added for the key
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xsd_add_key (const xmlChar *name,
                 const typ_def_t *typdef,
                 val_value_t *val)
{
    val_value_t        *node, *chnode;
    const typ_child_t  *ch;
    const typ_index_t  *in;
    const typ_def_t    *realtypdef;
    const xmlChar      *cstr;
    xmlChar            *buff, *str;
    xmlns_id_t          xsd_id;
    ncx_btype_t         btyp;
    status_t            res;

    xsd_id = xmlns_xs_id();

    /* create a 'key' node */
    node = xml_val_new_struct(XSD_KEY, xsd_id);
    if (!node) {
        return ERR_INTERNAL_MEM;
    }

    /* generate a key name */
    buff = m__getMem(xml_strlen(name) + 
                     xml_strlen(XSD_KEY_SUFFIX) + 1);
    if (!buff) {
        val_free_value(node);
        return ERR_INTERNAL_MEM;
    }
    str = buff;
    str += xml_strcpy(str, name);
    str += xml_strcpy(str, XSD_KEY_SUFFIX);

    /* add the name attribute to the key node */
    res = xml_val_add_attr(NCX_EL_NAME, 0, buff, node);
    if (res != NO_ERR) {
        m__free(buff);
        val_free_value(node);
        return ERR_INTERNAL_MEM;
    }

    /* add the selector child node */
    chnode = xml_val_new_flag(XSD_SELECTOR, xsd_id);
    if (!chnode) {
        val_free_value(node);
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(chnode, node);   /* add early */
    }

    realtypdef = typ_get_cbase_typdef(typdef);
    btyp = typ_get_basetype(typdef);

    /* get the selector xpath value */
    if (btyp == NCX_BT_LIST) {
        /* selector is the current node */
        cstr = (const xmlChar *)".";
    } else {
        /* type is NCX_BT_XCONTAINER and the selector is
         * the one and only child node
         */
        ch = typ_first_child(&realtypdef->def.complex);
        if (!ch) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            cstr = ch->name;
        }
    }

    /* add the xpath attribute to the selector node */
    if (res == NO_ERR) {
        res = xml_val_add_cattr(NCX_EL_XPATH, 0, cstr, chnode);
    }
    if (res != NO_ERR) {
        val_free_value(node);
        return ERR_INTERNAL_MEM;
    }

    /* add all the field child nodes */
    /* go through all the index nodes as field nodes */
    for (in = typ_first_index(&realtypdef->def.complex);
         in != NULL;
         in = typ_next_index(in)) {
        /* add the selector child node */
        chnode = xml_val_new_flag(XSD_FIELD, xsd_id);
        if (!chnode) {
            val_free_value(node);
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(chnode, node);   /* add early */
        }

        /* get the field string value based on the index type */
        switch (in->ityp) {
        case NCX_IT_INLINE:
        case NCX_IT_NAMED:
        case NCX_IT_LOCAL:
        case NCX_IT_REMOTE:
            str = xml_strdup(in->typch.name);
            break;
        case NCX_IT_SLOCAL:
        case NCX_IT_SREMOTE:
            if (in->sname) {
                str = NULL;
                str = xml_strdup(in->sname);
                if (str) {
                    /* convert the dot chars to forward slashes */
                    buff = str;
                    while (*buff) {
                        if (*buff==NCX_SCOPE_CH) {
                            *buff = (xmlChar)'/';
                        }
                        buff++;
                    }
                }
            } else {
                str = xml_strdup(in->typch.name);
            }
            break;
        default:
            val_free_value(node);
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (!str) {
            val_free_value(node);
            return ERR_INTERNAL_MEM;
        }
                
        /* add the xpath attribute to the field node */
        res = xml_val_add_attr(NCX_EL_XPATH, 0, str, chnode);
        if (res != NO_ERR) {
            m__free(str);
            val_free_value(node);
            return ERR_INTERNAL_MEM;
        }
    }

    val_add_child(node, val);
    return NO_ERR;

}   /* xsd_add_key */
#endif


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
status_t
    xsd_do_annotation (const xmlChar *descr,
                       const xmlChar *ref,
                       const xmlChar *condition,
                       const xmlChar *units,
                       ncx_access_t maxacc,
                       ncx_status_t status,
                       const dlq_hdr_t *appinfoQ,
                       val_value_t  *val)
{
    val_value_t  *annot, *chval;

    /* add an annotation node if there are any 
     * description, condition, or appinfo clauses present
     */
    if (descr || 
        ref || 
        condition || 
        units || 
        (appinfoQ && !dlq_empty(appinfoQ)) ||
        maxacc != NCX_ACCESS_NONE || 
        (status != NCX_STATUS_NONE && status != NCX_STATUS_CURRENT)) {

        annot = xml_val_new_struct(XSD_ANNOTATION, xmlns_xs_id());
        if (!annot) {
            return ERR_INTERNAL_MEM;
        }

        if (descr) {
            chval = make_documentation(descr);
            if (!chval) {
                val_free_value(annot);
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chval, annot);
            }
        }

        if ((appinfoQ && !dlq_empty(appinfoQ)) ||
            maxacc != NCX_ACCESS_NONE || 
            condition || 
            units ||
            ref ||
            (status != NCX_STATUS_NONE && status != NCX_STATUS_CURRENT)) {
            chval = make_appinfo(appinfoQ, 
                                 maxacc,
                                 condition,
                                 units, 
                                 ref,
                                 status);
            if (!chval) {
                val_free_value(annot);
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chval, annot);
            }
        }
                
        val_add_child(annot, val);
    }
    return NO_ERR;

} /* xsd_do_annotation */


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
val_value_t *
    xsd_make_obj_annotation (obj_template_t *obj,
                             status_t  *res)
{
    val_value_t    *annot, *doc, *appinfo;
    const xmlChar  *descr;

#ifdef DEBUG
    if (!obj || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    annot = NULL;
    doc = NULL;
    appinfo = NULL;

    descr = obj_get_description(obj);
    if (descr) {
        doc = make_documentation(descr);
        if (!doc) {
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
    }

    appinfo = make_obj_appinfo(obj, res);
    if (*res != NO_ERR) {
        if (doc) {
            val_free_value(doc);
        }
        return NULL;
    }

    if (doc != NULL || appinfo != NULL) {
        annot = xml_val_new_struct(XSD_ANNOTATION, xmlns_xs_id());
        if (!annot) {
            if (appinfo) {
                val_free_value(appinfo);
            }
            if (doc) {
                val_free_value(doc);
            }
            *res = ERR_INTERNAL_MEM;
            return NULL;
        } else {
            if (doc) {
                val_add_child(doc, annot);
            }
            if (appinfo) {
                val_add_child(appinfo, annot);
            }
        }
    }

    *res = NO_ERR;
    return annot;

} /* xsd_make_obj_annotation */


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
status_t
    xsd_do_type_annotation (const typ_template_t *typ,
                            val_value_t *val)
{
    val_value_t    *annot, *chval;
    const xmlChar  *path;
    xmlns_id_t      xsd_id;
    status_t        res;


#ifdef DEBUG
    if (!typ || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    path = NULL;
    annot = NULL;
    res = NO_ERR;

    if (typ->typdef.tclass == NCX_CL_SIMPLE &&
        typ_get_basetype(&typ->typdef)==NCX_BT_LEAFREF) {
        path = typ_get_leafref_path(&typ->typdef);
    }

    if (typ->descr || typ->ref || 
        typ->defval || typ->units || path ||
        !dlq_empty(&typ->appinfoQ) ||
        (typ->status != NCX_STATUS_NONE && 
         typ->status != NCX_STATUS_CURRENT)) {
        
        xsd_id = xmlns_xs_id();

        annot = xml_val_new_struct(XSD_ANNOTATION, xsd_id);
        if (!annot) {
            return ERR_INTERNAL_MEM;
        } else {
            val_add_child(annot, val);
        }

        if (typ->descr) {
            chval = make_documentation(typ->descr);
            if (!chval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chval, annot);
            }
        }

        if (!dlq_empty(&typ->appinfoQ) || typ->ref ||
            typ->units || typ->defval || path ||
            (typ->status != NCX_STATUS_NONE && 
             typ->status != NCX_STATUS_CURRENT)) {

            chval = make_type_appinfo(typ, path);
            if (!chval) {
                return ERR_INTERNAL_MEM;
            } else {
                val_add_child(chval, annot);
            }
        }
    }

    return res;

} /* xsd_make_type_annotation */


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
val_value_t *
    xsd_make_group_annotation (const grp_template_t *grp,
                               status_t  *res)
{
    val_value_t  *annot, *chval;
    xmlns_id_t    xsd_id;


#ifdef DEBUG
    if (!grp || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    annot = NULL;

    if (grp->descr || grp->ref || !dlq_empty(&grp->appinfoQ) ||
        (grp->status != NCX_STATUS_NONE && 
         grp->status != NCX_STATUS_CURRENT)) {
        
        xsd_id = xmlns_xs_id();

        annot = xml_val_new_struct(XSD_ANNOTATION, xsd_id);
        if (!annot) {
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }

        if (grp->descr) {
            chval = make_documentation(grp->descr);
            if (!chval) {
                val_free_value(annot);
                *res = ERR_INTERNAL_MEM;
                return NULL;
            } else {
                val_add_child(chval, annot);
            }
        }

        if (grp->ref ||
            !dlq_empty(&grp->appinfoQ) ||
            (grp->status != NCX_STATUS_NONE && 
             grp->status != NCX_STATUS_CURRENT)) {
            chval = make_appinfo(&grp->appinfoQ, 
                                 NCX_ACCESS_NONE,
                                 NULL, 
                                 NULL,
                                 grp->ref,
                                 grp->status);
            if (!chval) {
                val_free_value(annot);
                *res = ERR_INTERNAL_MEM;
                return NULL;
            } else {
                val_add_child(chval, annot);
            }
        }
    }

    *res = NO_ERR;
    return annot;

} /* xsd_make_group_annotation */


/********************************************************************
* FUNCTION xsd_add_aughook
* 
*   Add an abstract element to set a substitutionGroup that
*   can be used by an augment clause to create new nodes
*   within an existing structure.  This allows any namespace,
*   not just external namespaces like xsd_add_any
*
*   augment /foo/bar  -->   substitutionGroup='__.foo.bar.A__'
*
*   This function is called for /foo and /foo/bar, which
*   creates abstract elements /foo/__.foo.A__ and /foo/bar/__.foo.bar.A__
*
*   The YANG name length restrictions, and internal NcxName
*   restrictions do not matter since these constructed element names
*   are only used within the XSD
*  
* INPUTS:
*    val == val_value_t struct to add new last child leaf 
*    obj == object to use as the template for the new last leaf
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xsd_add_aughook (val_value_t *val)
{
    val_value_t   *aug;
    status_t       res;
    
    aug = xml_val_new_flag(XSD_ANY, xmlns_xs_id());
    if (!aug) {
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(aug, val);  /* add early */
    }

    res = xml_val_add_cattr(XSD_MIN_OCCURS, 0, XSD_ZERO, aug);
    if (res != NO_ERR) {
        return res;
    }

    res = xml_val_add_cattr(XSD_MAX_OCCURS, 0, XSD_UNBOUNDED, aug);
    if (res != NO_ERR) {
        return res;
    }

    res = xml_val_add_cattr(XSD_NAMESPACE, 0, XSD_OTHER, aug);
    if (res != NO_ERR) {
        return res;
    }

    res = xml_val_add_cattr(XSD_PROC_CONTENTS, 0, XSD_LAX, aug);
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

}   /* xsd_add_aughook */


/********************************************************************
* FUNCTION xsd_make_rpc_output_typename
* 
*   Create a type name for the RPC function output data structure
*
* INPUTS:
*    obj == object to make an RPC name from
*    
* RETURNS:
*   malloced buffer with the typename
*********************************************************************/
xmlChar *
    xsd_make_rpc_output_typename (const obj_template_t *obj)
{
    const xmlChar  *name;
    xmlChar        *buff, *p;
    uint32          len;

    name = obj_get_name(obj);
    len = xml_strlen(name) + xml_strlen(XSD_OUTPUT_TYPEEXT);

    buff = m__getMem(len+1);
    if (!buff) {
        return NULL;
    }

    p = buff;
    p += xml_strcpy(p, name);
    p += xml_strcpy(p, XSD_OUTPUT_TYPEEXT);

    /*** ADD NAME COLLISION DETECTION LATER !!! ***/

    return buff;

}   /* xsd_make_rpc_output_typename */


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
status_t
    xsd_add_type_attr (const ncx_module_t *mod,
                       const typ_def_t *typdef,
                       val_value_t *val)
{
    return add_basetype_attr(TRUE, mod, typdef, val);

}   /* xsd_add_type_attr */


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
status_t
    test_basetype_attr (const ncx_module_t *mod,
                        const typ_def_t *typdef)
{
    status_t       res;
    xmlns_id_t     typ_id;
    ncx_btype_t    btyp;
    const xmlChar *test;

#ifdef DEBUG
    if (mod == NULL || typdef == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    /* figure out the correct namespace and name for the typename */
    switch (typdef->tclass) {
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
        break;
    case NCX_CL_COMPLEX:
        btyp = typ_get_basetype(typdef);
        switch (btyp) {
        case NCX_BT_ANY:
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_CL_NAMED:
        typ_id = typdef->def.named.typ->nsid;
        if (typ_id == 0) {
            /* inside a grouping, include submod, etc.
             * this has to be a type in the current [sub]module
             * or it would have a NSID assigned already
             *
             * The design of the local typename to 
             * global XSD typename translation does not
             * put top-level typedefs in the typenameQ.
             * They are already in the registry, so name collision
             * can be checked that way
             */
            test = ncx_find_typname(typdef->def.named.typ, 
                                    &mod->typnameQ);
            if (test == NULL) {
                res = ERR_NCX_SKIPPED;
            }
        }
        break;
    case NCX_CL_REF:
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}   /* test_basetype_attr */


/********************************************************************
* FUNCTION get_next_seqnum
* 
*  Get a unique integer
*
* RETURNS:
*   next seqnum
*********************************************************************/
uint32
    get_next_seqnum (void)
{
    return seqnum++;

} /* get_next_seqnum */

/* END file xsd_util.c */
