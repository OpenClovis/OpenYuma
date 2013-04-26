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
/*  FILE: mgr_hello.c

    Handle the NETCONF <hello> (top-level) element.

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
15jan07      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_cap
#include "cap.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_cap
#include "mgr_cap.h"
#endif

#ifndef _H_mgr_hello
#include "mgr_hello.h"
#endif

#ifndef _H_mgr_ses
#include "mgr_ses.h"
#endif

#ifndef _H_mgr_val_parse
#include "mgr_val_parse.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_top
#include "top.h"
#endif

#ifndef _H_val
#include  "val.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_xml_wr
#include  "xml_wr.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define MGR_HELLO_DEBUG 1
#endif

#define MGR_SERVER_HELLO_OBJ ((const xmlChar *)"server-hello")


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
static boolean mgr_hello_init_done = FALSE;


/********************************************************************
* FUNCTION process_server_hello
*
* Process the NETCONF server <hello> contents
*
*  1) Protocol capabilities
*  2) Module capabilities
*  3) Unrecognized capabilities
*
* INPUTS:
*    scb == session control block to set
*    hello == value struct for the hello message to check
*
* OUTPUTS:
*    server caps in the scb->mgrcb is set
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    process_server_hello (ses_cb_t *scb,
                         val_value_t *hello)
{

    val_value_t  *caps, *sidval, *cap;
    mgr_scb_t    *mscb;
    boolean       c1, c2;
    status_t      res;

    mscb = mgr_ses_get_mscb(scb);

    /* make sure the capabilities element is present
     * This should not fail, since already parsed this far 
     */
    caps = val_find_child(hello, NC_MODULE, NCX_EL_CAPABILITIES);
    if (!caps || caps->res != NO_ERR) {
        log_error("\nError: no <capabilities> found in server <hello>");
        return ERR_NCX_MISSING_VAL_INST;
    }   

    /* make sure the session-id element is present
     * This should not fail, since already parsed this far 
     */
    sidval = val_find_child(hello, NC_MODULE, NCX_EL_SESSION_ID);
    if (!sidval || sidval->res != NO_ERR) {
        log_error("\nError: no <session-id> found in server <hello>");
        return ERR_NCX_MISSING_VAL_INST;
    } else {
        mscb->agtsid = VAL_UINT(sidval);
    }

    /* go through the capability nodes and construct a caplist */
    for (cap = val_find_child(caps, NC_MODULE, NCX_EL_CAPABILITY);
         cap != NULL;
         cap = val_find_next_child(caps, 
                                   NC_MODULE, 
                                   NCX_EL_CAPABILITY, 
                                   cap)) {

        if (cap->res != NO_ERR) {
            continue;
        }
        
        res = cap_add_std_string(&mscb->caplist, VAL_STR(cap));
        if (res == ERR_NCX_SKIPPED) {
            res = cap_add_module_string(&mscb->caplist, VAL_STR(cap));
            if (res == ERR_NCX_SKIPPED) {
                /* 
                 * if (ncx_warning_enabled(ERR_NCX_RCV_UNKNOWN_CAP)) {
                 *    log_warn("\nWarning: received unknown capability '%s'",
                 *             VAL_STR(cap));
                 * }
                 */
                if (LOGDEBUG2) {
                    log_debug2("\nmgr: Got enterprise capability %s", 
                               VAL_STR(cap));
                }

                /* hack: check for juniper 1.0 server
                 * change the useprefix mode to TRUE to get
                 * <rpc> operations to work with this server
                 */
                if (!xml_strcmp(VAL_STR(cap), CAP_JUNOS)) {
                    if (LOGDEBUG) {
                        log_debug("\nUsing XML prefixes to work "
                                  "with Junos 1.0 server\n");
                    }
                    ncx_set_useprefix(TRUE);
                }

                res = cap_add_ent(&mscb->caplist, VAL_STR(cap));
                if (res != NO_ERR) {
                    return res;
                }
            }
        }
    }

    /* check if the mandatory base protocol capability was set */
    res = NO_ERR;
    c1 = cap_std_set(&mscb->caplist, CAP_STDID_V1);
    c2 = cap_std_set(&mscb->caplist, CAP_STDID_V11);

    if (c1 && c2) {
        if (LOGDEBUG2) {
            log_debug2("\nmgr_hello: server supports "
                       "base:1.0 and base:1.1");
        }
        if (ses_protocol_requested(scb, NCX_PROTO_NETCONF11)) {
            if (LOGDEBUG2) {
                log_debug2("\nmgr_hello: set protocol to base:1.1 "
                           "for session '%d'",
                           scb->sid);
            }
            ses_set_protocol(scb, NCX_PROTO_NETCONF11);
        } else if (ses_protocol_requested(scb, NCX_PROTO_NETCONF10)) {
            if (LOGDEBUG2) {
                log_debug2("\nmgr_hello: set protocol to base:1.0 "
                           "for session '%d'",
                           scb->sid);
            }
            ses_set_protocol(scb, NCX_PROTO_NETCONF10);
        } else {
            log_error("\nError: Internal: no protocols requested, "
                      "dropping session '%d'",
                      scb->sid);
            res = ERR_NCX_MISSING_VAL_INST;
        }
    } else if (c1) {
        if (LOGDEBUG2) {
            log_debug2("\nmgr_hello: server supports "
                       "base:1.0 only");
        }
        if (ses_protocol_requested(scb, NCX_PROTO_NETCONF10)) {
            if (LOGDEBUG2) {
                log_debug2("\nmgr_hello: set protocol to base:1.0 "
                           "for session '%d'",
                           scb->sid);
            }
            ses_set_protocol(scb, NCX_PROTO_NETCONF10);
        } else {
            log_error("\nError: Server supports base:1.0 only;"
                     "\n  Protocol 'netconf1.0' not enabled, "
                      "dropping session '%d'",
                      scb->sid);
            res = ERR_NCX_MISSING_VAL_INST;
        }
    } else if (c2) {
        if (LOGDEBUG2) {
            log_debug2("\nmgr_hello: server supports "
                       "base:1.1 only");
        }
        if (ses_protocol_requested(scb, NCX_PROTO_NETCONF11)) {
            if (LOGDEBUG2) {
                log_debug2("\nmgr_hello: set protocol to base:1.1 "
                           "for session '%d'",
                           scb->sid);
            }
            ses_set_protocol(scb, NCX_PROTO_NETCONF11);
        } else {
            log_error("\nError: Server supports base:1.1 only;"
                     "\n  Protocol 'netconf1.1' not enabled, "
                      "dropping session '%d'",
                      scb->sid);
            res = ERR_NCX_MISSING_VAL_INST;
        }
    } else {
        log_error("\nError: no support for base:1.0 "
                  "or base:1.1 found in server <hello>;"
                  "\n   dropping session '%d'",
                  scb->sid);
        return ERR_NCX_MISSING_VAL_INST;
    }

    /* set target type var in the manager session control block */
    c1 = cap_std_set(&mscb->caplist, CAP_STDID_WRITE_RUNNING);
    c2 = cap_std_set(&mscb->caplist, CAP_STDID_CANDIDATE);

    if (c1 && c2) {
        mscb->targtyp = NCX_AGT_TARG_CAND_RUNNING;
    } else if (c1) {
        mscb->targtyp = NCX_AGT_TARG_RUNNING;
    } else if (c2) {
        mscb->targtyp = NCX_AGT_TARG_CANDIDATE;
    } else {
        mscb->targtyp = NCX_AGT_TARG_NONE;
        if (LOGINFO) {
            log_info("\nmgr_hello: no writable target found for"
                     " session %u (a:%u)", 
                     scb->sid,
                     mscb->agtsid);
        }
    }

    /* set the startup type in the mscb */
    if (cap_std_set(&mscb->caplist, CAP_STDID_STARTUP)) {
        mscb->starttyp = NCX_AGT_START_DISTINCT;
    } else {
        mscb->starttyp = NCX_AGT_START_MIRROR;
    }

    return NO_ERR;

} /* process_server_hello */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION mgr_hello_init
*
* Initialize the mgr_hello module
* Adds the mgr_hello_dispatch function as the handler
* for the NETCONF <hello> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
status_t 
    mgr_hello_init (void)
{
    status_t  res;

    if (!mgr_hello_init_done) {
        res = top_register_node(NC_MODULE, NCX_EL_HELLO, 
                                mgr_hello_dispatch);
        if (res != NO_ERR) {
            return res;
        }
        mgr_hello_init_done = TRUE;
    }
    return NO_ERR;

} /* mgr_hello_init */


/********************************************************************
* FUNCTION mgr_hello_cleanup
*
* Cleanup the mgr_hello module.
* Unregister the top-level NETCONF <hello> element
*
*********************************************************************/
void 
    mgr_hello_cleanup (void)
{
    if (mgr_hello_init_done) {
        top_unregister_node(NC_MODULE, NCX_EL_HELLO);
        mgr_hello_init_done = FALSE;
    }

} /* mgr_hello_cleanup */


/********************************************************************
* FUNCTION mgr_hello_dispatch
*
* Handle an incoming <hello> message from the client
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
void 
    mgr_hello_dispatch (ses_cb_t *scb,
                        xml_node_t *top)
{
    val_value_t           *val;
    ncx_module_t          *mod;
    obj_template_t        *obj;
    mgr_scb_t             *mscb;
    xml_msg_hdr_t          msg;
    status_t               res;

#ifdef DEBUG
    if (!scb || !top) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

#ifdef MGR_HELLO_DEBUG
    if (LOGDEBUG) {
        log_debug("\nmgr_hello got node");
    }
    if (LOGDEBUG2) {
        xml_dump_node(top);
    }
#endif

    mscb = mgr_ses_get_mscb(scb);

    /* only process this message in hello wait state */
    if (scb->state != SES_ST_HELLO_WAIT) {
        /* TBD: stats update */
        if (LOGINFO) {
            log_info("\nmgr_hello dropped, wrong state for session %d",
                     scb->sid);
        }
        return;
    }

    /* init local vars */
    res = NO_ERR;
    val = NULL;
    obj = NULL;
    xml_msg_init_hdr(&msg);

    /* get a value struct to hold the server hello msg */
    val = val_new_value();
    if (!val) {
        res = ERR_INTERNAL_MEM;
    }

    /* get the type definition from the registry */
    if (res == NO_ERR) {
        mod = ncx_find_module(NC_MODULE, NULL);
        if (mod) {
            obj = ncx_find_object(mod, MGR_SERVER_HELLO_OBJ);
        }
        if (!obj) {
            /* netconf module should have loaded this definition */
            res = SET_ERROR(ERR_INTERNAL_PTR);
        }
    }

    /* parse an server hello message */
    if (res == NO_ERR) {
        res = mgr_val_parse(scb, obj, top, val);
    }
    
    /* examine the server capability list
     * and it matches the server protocol version
     */
    if (res == NO_ERR) {
        res = process_server_hello(scb, val);
    }

    /* report first error and close session */
    if (res != NO_ERR) {
        if (LOGINFO) {
            log_info("\nmgr_connect error (%s)\n  dropping session %u (a:%u)",
                     get_error_string(res), 
                     scb->sid, 
                     mscb->agtsid,
                     res);
        }
    } else {
        scb->state = SES_ST_IDLE;
        if (LOGDEBUG) {
            log_debug("\nmgr_hello manager hello ok");
        }
    }
    if (val) {
        val_free_value(val);
    }

} /* mgr_hello_dispatch */


/********************************************************************
* FUNCTION mgr_hello_send
*
* Send the manager <hello> message to the server on the 
* specified session
*
* INPUTS:
*   scb == session control block
*
* RETURNS:
*   status
*********************************************************************/
status_t
    mgr_hello_send (ses_cb_t *scb)
{
    val_value_t  *mycaps;
    xml_msg_hdr_t msg;
    status_t      res;
    xml_attrs_t   attrs;
    boolean       anyout;
    xmlns_id_t    nc_id;

#ifdef DEBUG
    if (!scb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef MGR_HELLO_DEBUG
    if (LOGDEBUG2) {
        log_debug2("\nmgr sending hello on session %d", scb->sid);
    }
#endif

    res = NO_ERR;
    anyout = FALSE;
    xml_msg_init_hdr(&msg);
    xml_init_attrs(&attrs);
    nc_id = xmlns_nc_id();

    /* get my client caps, custom made for this session */
    mycaps = mgr_cap_get_ses_capsval(scb);
    if (!mycaps) {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* setup the prefix map with the NETCONF namespace */
    if (res == NO_ERR) {
        res = xml_msg_build_prefix_map(&msg, &attrs, TRUE, FALSE);
    }

    /* send the <?xml?> directive */
    if (res == NO_ERR) {
        res = ses_start_msg(scb);
    }

    /* start the hello element */
    if (res == NO_ERR) {
        anyout = TRUE;
        xml_wr_begin_elem_ex(scb, 
                             &msg, 
                             0, 
                             nc_id, 
                             NCX_EL_HELLO, 
                             &attrs, 
                             ATTRQ, 
                             0, 
                             START);
    }
    
    /* send the capabilities list */
    if (res == NO_ERR) {
        xml_wr_full_val(scb, &msg, mycaps, NCX_DEF_INDENT);
    }

    /* finish the hello element */
    if (res == NO_ERR) {
        xml_wr_end_elem(scb, &msg, nc_id, NCX_EL_HELLO, 0);
    }

    /* finish the message */
    if (anyout) {
        ses_finish_msg(scb);
    }

    xml_clean_attrs(&attrs);
    xml_msg_clean_hdr(&msg);
    if (mycaps != NULL) {
        val_free_value(mycaps);
    }
    return res;

} /* mgr_hello_send */


/* END file mgr_hello.c */


