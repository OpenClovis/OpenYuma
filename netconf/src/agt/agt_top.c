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
/*  FILE: agt_top.c

  NCX Agent Top Element Handler

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
30dec05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <xmlstring.h>
#include <xmlreader.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_ses.h"
#include "agt_top.h"
#include "agt_xml.h"
#include "dlq.h"
#include "log.h"
#include "ncxconst.h"
#include "ncx.h"
#include "status.h"
#include "top.h"
#include "xmlns.h"
#include "xml_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                            *
*                                                                   *
*********************************************************************/


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_top_dispatch_msg
* 
* Find the appropriate top node handler and call it
* called by the transport manager (through the session manager)
* when a new message is detected
*
* INPUTS:
*   scb == session control block containing the xmlreader
*          set at the start of an incoming message.
*
* RETURNS:
*  none
*********************************************************************/
void
    agt_top_dispatch_msg (ses_cb_t **ppscb)
{
    ses_total_stats_t  *myagttotals;
    agt_profile_t      *profile;
    xml_node_t          top;
    status_t            res;
    top_handler_t       handler;
    ses_cb_t           *scb = *ppscb;
    
#ifdef DEBUG
    if (!scb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    myagttotals = ses_get_total_stats();
    profile = agt_get_profile();

    xml_init_node(&top);

    /* get the first node */
    res = agt_xml_consume_node(scb, 
                               &top, 
                               NCX_LAYER_TRANSPORT, 
                               NULL);
    if (res != NO_ERR) {
        scb->stats.inBadRpcs++;
        myagttotals->stats.inBadRpcs++;
        myagttotals->droppedSessions++;

        if (LOGINFO) {
            log_info("\nagt_top: bad msg for session %d (%s)",
                     scb->sid, 
                     get_error_string(res));
        }

        xml_clean_node(&top);
        agt_ses_free_session(scb);

        /* set the supplied ptr to ptr to scb to NULL so that the 
         * caller of this function knows that it was deallotcated */
        *ppscb=NULL;
        return;
    }

    log_debug3("\nagt_top: got node");
    if (LOGDEBUG4 && scb->state != SES_ST_INIT) {
        xml_dump_node(&top);
    }

    /* check node type and if handler exists, then call it */
    if (top.nodetyp==XML_NT_START || top.nodetyp==XML_NT_EMPTY) {
        /* find the owner, elname tuple in the topQ */
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
        scb->stats.inBadRpcs++;
        myagttotals->stats.inBadRpcs++;
        myagttotals->droppedSessions++;
        
        if (LOGINFO) {
            log_info("\nagt_top: bad msg for session %d (%s)",
                     scb->sid, 
                     get_error_string(res));
        }

        agt_ses_free_session(scb);

        /* set the supplied ptr to ptr to scb to NULL so that the 
         * caller of this function knows that it was deallotcated */
        *ppscb=NULL;

    } else if (profile->agt_stream_output &&
               scb->state == SES_ST_SHUTDOWN_REQ) {
        /* session was closed */
        agt_ses_kill_session(scb,
                             scb->killedbysid,
                             scb->termreason);
        /* set the supplied ptr to ptr to scb to NULL so that the 
         * caller of this function knows that it was deallotcated */
        *ppscb=NULL;
    }

    xml_clean_node(&top);

} /* agt_top_dispatch_msg */


/* END file agt_top.c */
