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
/*  FILE: yangcli_save.c

   NETCONF YANG-based CLI Tool

   save command

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
13-aug-09    abb      begun; started from yangcli_cmd.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "libtecla.h"

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_ses
#include "mgr_ses.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
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

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_var
#include "var.h"
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

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yangcli
#include "yangcli.h"
#endif

#ifndef _H_yangcli_cmd
#include "yangcli_cmd.h"
#endif

#ifndef _H_yangcli_save
#include "yangcli_save.h"
#endif

#ifndef _H_yangcli_util
#include "yangcli_util.h"
#endif


/********************************************************************
* FUNCTION send_copy_config_to_server
* 
* Send a <copy-config> operation to the server to support
* the save operation
*
* INPUTS:
*    server_cb == server control block to use
*
* OUTPUTS:
*    state may be changed or other action taken
*    config_content is consumed -- freed or transfered
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    send_copy_config_to_server (server_cb_t *server_cb)
{
    obj_template_t        *rpc, *input, *child;
    mgr_rpc_req_t         *req;
    val_value_t           *reqdata, *parm, *target, *source;
    ses_cb_t              *scb;
    status_t               res;

    req = NULL;
    reqdata = NULL;
    res = NO_ERR;
    rpc = NULL;

    /* get the <copy-config> template */
    rpc = ncx_find_object(get_netconf_mod(server_cb), 
                          NCX_EL_COPY_CONFIG);
    if (!rpc) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* get the 'input' section container */
    input = obj_find_child(rpc, NULL, YANG_K_INPUT);
    if (!input) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* construct a method + parameter tree */
    reqdata = xml_val_new_struct(obj_get_name(rpc), 
                                 obj_get_nsid(rpc));
    if (!reqdata) {
        log_error("\nError allocating a new RPC request");
        return ERR_INTERNAL_MEM;
    }

    /* set the edit-config/input/target node to the default_target */
    child = obj_find_child(input, NC_MODULE, NCX_EL_TARGET);
    parm = val_new_value();
    if (!parm) {
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(parm, child);
    val_add_child(parm, reqdata);

    target = xml_val_new_flag((const xmlChar *)"startup",
                              obj_get_nsid(child));
    if (!target) {
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    }
    val_add_child(target, parm);

    /* set the edit-config/input/default-operation node to 'none' */
    child = obj_find_child(input, NC_MODULE, NCX_EL_SOURCE);
    parm = val_new_value();
    if (!parm) {
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(parm, child);
    val_add_child(parm, reqdata);

    source = xml_val_new_flag((const xmlChar *)"running",
                              obj_get_nsid(child));
    if (!source) {
        val_free_value(reqdata);
        return ERR_INTERNAL_MEM;
    }
    val_add_child(source, parm);

    /* allocate an RPC request and send it */
    scb = mgr_ses_get_scb(server_cb->mysid);
    if (!scb) {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    } else {
        req = mgr_rpc_new_request(scb);
        if (!req) {
            res = ERR_INTERNAL_MEM;
            log_error("\nError allocating a new RPC request");
        } else {
            req->data = reqdata;
            req->rpc = rpc;
            req->timeout = server_cb->timeout;
        }
    }
        
    if (res == NO_ERR) {
        if (LOGDEBUG2) {
            log_debug2("\nabout to send RPC request with reqdata:");
            val_dump_value_max(reqdata, 
                               0,
                               server_cb->defindent,
                               DUMP_VAL_LOG,
                               server_cb->display_mode,
                               FALSE,
                               FALSE);
        }

        /* the request will be stored if this returns NO_ERR */
        res = mgr_rpc_send_request(scb, req, yangcli_reply_handler);
    }

    if (res != NO_ERR) {
        if (req) {
            mgr_rpc_free_request(req);
        } else if (reqdata) {
            val_free_value(reqdata);
        }
    } else {
        server_cb->state = MGR_IO_ST_CONN_RPYWAIT;
    }

    return res;

} /* send_copy_config_to_server */


/********************************************************************
 * FUNCTION do_save
 * 
 * INPUTS:
 *    server_cb == server control block to use
 *
 * OUTPUTS:
 *   copy-config and/or commit operation will be sent to server
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_save (server_cb_t *server_cb)
{
    const ses_cb_t   *scb;
    const mgr_scb_t  *mscb;
    xmlChar          *line;
    status_t          res;

    res = NO_ERR;

    /* get the session info */
    scb = mgr_ses_get_scb(server_cb->mysid);
    if (!scb) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    mscb = (const mgr_scb_t *)scb->mgrcb;

    log_info("\nSaving configuration to non-volative storage");

    /* determine which commands to send */
    switch (mscb->targtyp) {
    case NCX_AGT_TARG_NONE:
        log_stdout("\nWarning: No writable targets supported on this server");
        break;
    case NCX_AGT_TARG_CAND_RUNNING:
        if (!xml_strcmp(server_cb->default_target, NCX_EL_CANDIDATE)) {
            line = xml_strdup(NCX_EL_COMMIT);
            if (line) {
                res = conn_command(server_cb, line);
                m__free(line);
            } else {
                res = ERR_INTERNAL_MEM;
                log_stdout("\nError: Malloc failed");
            }
            if (res == NO_ERR &&
                mscb->starttyp == NCX_AGT_START_DISTINCT) {
                /* need 2 operations so set the command mode and the 
                 * reply handler will initiate the 2nd command
                 * if the first one worked
                 */
                server_cb->command_mode = CMD_MODE_SAVE;
            }
        } else {
            if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
                res = send_copy_config_to_server(server_cb);
                if (res != NO_ERR) {
                    log_stdout("\nError: send copy-config failed (%s)",
                               get_error_string(res));
                }
            } else {
                log_stdout("\nWarning: No distinct save operation needed "
                           "for this server");
            }
        }
        break;
    case NCX_AGT_TARG_CANDIDATE:
        line = xml_strdup(NCX_EL_COMMIT);
        if (line) {
            res = conn_command(server_cb, line);
            m__free(line);
        } else {
            res = ERR_INTERNAL_MEM;
            log_stdout("\nError: Malloc failed");
        }
        if (res == NO_ERR &&
            mscb->starttyp == NCX_AGT_START_DISTINCT) {
            /* need 2 operations so set the command mode and the 
             * reply handler will initiate the 2nd command
             * if the first one worked
             */
            server_cb->command_mode = CMD_MODE_SAVE;
        }
        break;
    case NCX_AGT_TARG_RUNNING:
        if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
            res = send_copy_config_to_server(server_cb);
            if (res != NO_ERR) {
                log_stdout("\nError: send copy-config failed (%s)",
                           get_error_string(res));
            }
        } else {
            log_stdout("\nWarning: No distinct save operation needed "
                       "for this server");
        }
        break;
    case NCX_AGT_TARG_LOCAL:
        log_stdout("Error: Local URL target not supported");
        break;
    case NCX_AGT_TARG_REMOTE:
        log_stdout("Error: Local URL target not supported");
        break;
    default:
        log_stdout("Error: Internal target not set");
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

}  /* do_save */


/********************************************************************
 * FUNCTION finish_save
 * 
 * INPUTS:
 *    server_cb == server control block to use
 *
 * OUTPUTS:
 *   copy-config will be sent to server
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    finish_save (server_cb_t *server_cb)
{
    const ses_cb_t   *scb;
    const mgr_scb_t  *mscb;
    status_t          res;

    res = NO_ERR;

    /* get the session info */
    scb = mgr_ses_get_scb(server_cb->mysid);
    if (!scb) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    mscb = (const mgr_scb_t *)scb->mgrcb;

    log_info("\nFinal step saving configuration to non-volative storage");

    if (mscb->starttyp == NCX_AGT_START_DISTINCT) {
        res = send_copy_config_to_server(server_cb);
        if (res != NO_ERR) {
            log_stdout("\nError: send copy-config failed (%s)",
                       get_error_string(res));
        }
    } else {
        log_stdout("\nWarning: No distinct save operation needed "
                   "for this server");
    }

    return res;

}  /* finish_save */


/* END yangcli_save.c */
