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
/*  FILE: mgr_top.c

  NCX Manager Top Element Handler

  This module uses a simple queue of top-level entries
  because there are not likely to be very many of them.

  Each top-level node is keyed by the owner name and 
  the element name.  
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11feb07      abb      begun; start from agt/agt_top.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_mgr
#include "mgr.h"
#endif

#ifndef _H_mgr_top
#include "mgr_top.h"
#endif

#ifndef _H_mgr_xml
#include "mgr_xml.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_top
#include "top.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define MGR_TOP_DEBUG 1

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION mgr_top_dispatch_msg
* 
* Find the appropriate top node handler and call it
*
* INPUTS:
*   scb == session control block containing the xmlreader
*          set at the start of an incoming message.
*
* RETURNS:
*  none
*********************************************************************/
void
    mgr_top_dispatch_msg (ses_cb_t  *scb)
{
    xml_node_t     top;
    status_t       res;
    top_handler_t  handler;

#ifdef DEBUG
    if (!scb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    xml_init_node(&top);

    /* get the first node */
    res = mgr_xml_consume_node(scb->reader, &top);
    if (res != NO_ERR) {
        log_info("\nmgr_top: get node failed (%s); session dropped", 
                 get_error_string(res));
        xml_clean_node(&top);
        scb->state = SES_ST_SHUTDOWN_REQ;
        return;
    }

#ifdef MGR_TOP_DEBUG
    if (LOGDEBUG3) {
        log_debug3("\nmgr_top: got node");
        xml_dump_node(&top);
    }
#endif

    /* check node type and if handler exists, then call it */
    if (top.nodetyp==XML_NT_START || top.nodetyp==XML_NT_EMPTY) {
        /* find the module, elname tuple in the topQ */
        handler = top_find_handler(top.module, top.elname);
        if (handler) {
            /* call the handler */
            (*handler)(scb, &top);
        } else {
            res = ERR_NCX_DEF_NOT_FOUND;
        }
    } else {
        res = ERR_NCX_WRONG_NODETYP;
    }

    /* check any error trying to invoke the top handler */
    if (res != NO_ERR) {
        log_error("\nError: agt_top skipped msg for session %d (%s)",
                  scb->sid, get_error_string(res));
    }

    xml_clean_node(&top);

} /* mgr_top_dispatch_msg */


/* END file mgr_top.c */
