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
/*  FILE: agt_connect.c

    Handle the <ncx-connect> (top-level) element.
    This message is used for thin clients to connect
    to the ncxserver. 

   Client --> SSH2 --> OpenSSH.subsystem(netconf) -->
 
      ncxserver_connect --> AF_LOCAL/ncxserver.sock -->

      ncxserver.listen --> top_dispatch -> ncx_connect_handler

                
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

#include "procdefs.h"
#include "agt.h"
#include "agt_connect.h"
#include "agt_hello.h"
#include "agt_rpcerr.h"
#include "agt_ses.h"
#include "agt_state.h"
#include "agt_sys.h"
#include "agt_util.h"
#include "cap.h"
#include "cfg.h"
#include "log.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ses.h"
#include "status.h"
#include "top.h"
#include "val.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
static boolean agt_connect_init_done = FALSE;

/********************************************************************
* FUNCTION agt_connect_init
*
* Initialize the agt_connect module
* Adds the agt_connect_dispatch function as the handler
* for the NCX <ncx-connect> top-level element.
*
* INPUTS:
*   none
* RETURNS:
*   NO_ERR if all okay, the minimum spare requests will be malloced
*********************************************************************/
status_t 
    agt_connect_init (void)
{
    status_t  res;

    if (!agt_connect_init_done) {
        res = top_register_node(NCX_MODULE, 
                                NCX_EL_NCXCONNECT, 
                                agt_connect_dispatch);
        if (res != NO_ERR) {
            return res;
        }
        agt_connect_init_done = TRUE;
    }
    return NO_ERR;

} /* agt_connect_init */


/********************************************************************
* FUNCTION agt_connect_cleanup
*
* Cleanup the agt_connect module.
* Unregister the top-level NCX <ncx-connect> element
*
*********************************************************************/
void 
    agt_connect_cleanup (void)
{
    if (agt_connect_init_done) {
        top_unregister_node(NCX_MODULE, NCX_EL_NCXCONNECT);
        agt_connect_init_done = FALSE;
    }

} /* agt_connect_cleanup */


/********************************************************************
* FUNCTION agt_connect_dispatch
*
* Handle an incoming <ncx-connect> request
*
* INPUTS:
*   scb == session control block
*   top == top element descriptor
*********************************************************************/
void 
    agt_connect_dispatch (ses_cb_t *scb,
                          xml_node_t *top)
{
    xml_attr_t      *attr;
    status_t         res;
    ncx_num_t        num;

#ifdef DEBUG
    if (!scb || !top) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    log_debug("\nagt_connect got node");

    res = NO_ERR;

    /* make sure 'top' is the right kind of node */
    if (top->nodetyp != XML_NT_EMPTY) {
        res = ERR_NCX_WRONG_NODETYP;
        /* TBD: stats update */
    }

    /* only process this message in session init state */
    if (res==NO_ERR && scb->state != SES_ST_INIT) {
        /* TBD: stats update */
        res = ERR_NCX_NO_ACCESS_STATE;
    } else {
        scb->state = SES_ST_IN_MSG;
    }

    /* check the ncxserver version */
    if (res == NO_ERR) {
        attr = xml_find_attr(top, 0, NCX_EL_VERSION);
        if (attr && attr->attr_val) {
            res = ncx_convert_num(attr->attr_val, 
                                  NCX_NF_DEC,
                                  NCX_BT_UINT32, 
                                  &num);
            if (res == NO_ERR) {
                if (num.u != NCX_SERVER_VERSION) {
                    res = ERR_NCX_WRONG_VERSION;
                }
            }
        } else {
            res = ERR_NCX_MISSING_ATTR;
        }
    }

    /* check the magic password string */
    if (res == NO_ERR) {
        attr = xml_find_attr(top, 0, NCX_EL_MAGIC);
        if (attr && attr->attr_val) {
            if (xml_strcmp(attr->attr_val, 
                           (const xmlChar *)NCX_SERVER_MAGIC)) {
                res = ERR_NCX_ACCESS_DENIED;
            }
        } else {
            res = ERR_NCX_MISSING_ATTR;
        }
    }

    /* check the transport */
    if (res == NO_ERR) {
        attr = xml_find_attr(top, 0, NCX_EL_TRANSPORT);
        if (attr && attr->attr_val) {
            if ( 0 == xml_strcmp(attr->attr_val, 
                           (const xmlChar *)NCX_SERVER_TRANSPORT)) {
                /* transport indicates an external connection over
                 * ssh, check the ncxserver port number */
                attr = xml_find_attr(top, 0, NCX_EL_PORT);
                if (attr && attr->attr_val) {
                    res = ncx_convert_num(attr->attr_val, 
                                          NCX_NF_DEC,
                                          NCX_BT_UINT16, 
                                          &num);
                    if (res == NO_ERR) {
                        if (!agt_ses_ssh_port_allowed((uint16)num.u)) {
                            res = ERR_NCX_ACCESS_DENIED;
                        }
                    }
                } else {
                    res = ERR_NCX_MISSING_ATTR;
                }
            }
            else if ( xml_strcmp(attr->attr_val, 
                           (const xmlChar *)NCX_SERVER_TRANSPORT_LOCAL)) {
                /* transport is unsupported (i.e. not SSH or 'local' ) */
                res = ERR_NCX_ACCESS_DENIED;
            }
        } else {
            res = ERR_NCX_MISSING_ATTR;
        }
    }

    /* get the username */
    if (res == NO_ERR) {
        attr = xml_find_attr(top, 0, NCX_EL_USER);
        if (attr && attr->attr_val) {
            scb->username = xml_strdup(attr->attr_val);
            if (!scb->username) {
                res = ERR_INTERNAL_MEM;
            }
        } else {
            res = ERR_NCX_MISSING_ATTR;
        }
    }

    /* get the client address */
    if (res == NO_ERR) {
        attr = xml_find_attr(top, 0, NCX_EL_ADDRESS);
        if (attr && attr->attr_val) {
            scb->peeraddr = xml_strdup(attr->attr_val);
            if (!scb->peeraddr) {
                res = ERR_INTERNAL_MEM;
            }
        } else {
            res = ERR_NCX_MISSING_ATTR;
        }
    }

    if (res == NO_ERR) {
        /* add the session to the netconf-state DM */
        res = agt_state_add_session(scb);

        /* bump the session state and send the agent hello message */
        if (res == NO_ERR) {
            res = agt_hello_send(scb);
            if (res != NO_ERR) {
                agt_state_remove_session(scb->sid);
            }
        }

        if (res == NO_ERR) {
            scb->state = SES_ST_HELLO_WAIT;
        }
    }

    /* report first error and close session */
    if (res != NO_ERR) {
        agt_ses_request_close(scb, scb->sid, SES_TR_BAD_START);
        if (LOGINFO) {
            log_info("\nagt_connect error (%s)\n"
                     "  dropping session %d",
                     get_error_string(res), 
                     scb->sid);
        }
    } else {
        log_debug("\nagt_connect msg ok");
        agt_sys_send_sysSessionStart(scb);
    }
    
} /* agt_connect_dispatch */


/* END file agt_connect.c */


