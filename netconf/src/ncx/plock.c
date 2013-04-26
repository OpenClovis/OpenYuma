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
/*  FILE: plock.c

   Partial lock control block
   Support for RFC 5717 operations

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
21jun10      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_cfg
#include  "cfg.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_plock
#include  "plock.h"
#endif

#ifndef _H_plock_cb
#include  "plock_cb.h"
#endif

#ifndef _H_ses
#include  "ses.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tstamp
#include  "tstamp.h"
#endif

#ifndef _H_val
#include  "val.h"
#endif

#ifndef _H_xmlns
#include  "xmlns.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_xpath
#include  "xpath.h"
#endif

#ifndef _H_xpath1
#include  "xpath1.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#ifdef DEBUG
#define PLOCK_DEBUG 1
#endif

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


/********************************************************************
* FUNCTION plock_get_id
*
* Get the lock ID for this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   the lock ID for this lock
*********************************************************************/
plock_id_t
    plock_get_id (plock_cb_t *plcb)
{
#ifdef DEBUG
    if (plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    return plcb->plock_id;

}  /* plock_get_id */


/********************************************************************
* FUNCTION plock_get_sid
*
* Get the session ID holding this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   session ID that owns this lock
*********************************************************************/
uint32
    plock_get_sid (plock_cb_t *plcb)
{
#ifdef DEBUG
    if (plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    return plcb->plock_sesid;

}  /* plock_get_sid */



/********************************************************************
* FUNCTION plock_get_timestamp
*
* Get the timestamp of the lock start time
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   timestamp in date-time format
*********************************************************************/
const xmlChar *
    plock_get_timestamp (plock_cb_t *plcb)
{
#ifdef DEBUG
    if (plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return plcb->plock_time;

}  /* plock_get_timestamp */


/********************************************************************
* FUNCTION plock_get_final_result
*
* Get the final result for this partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   pointer to final result struct
*********************************************************************/
xpath_result_t *
    plock_get_final_result (plock_cb_t *plcb)
{
#ifdef DEBUG
    if (plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return plcb->plock_final_result;

}  /* plock_get_final_result */


/********************************************************************
* FUNCTION plock_get_first_select
*
* Get the first select XPath control block for the partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*   pointer to first xpath_pcb_t for the lock
*********************************************************************/
xpath_pcb_t *
    plock_get_first_select (plock_cb_t *plcb)
{
#ifdef DEBUG
    if (plcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (xpath_pcb_t *)dlq_firstEntry(&plcb->plock_xpathpcbQ);

}  /* plock_get_first_select */


/********************************************************************
* FUNCTION plock_get_next_select
*
* Get the next select XPath control block for the partial lock
*
* INPUTS:
*   xpathpcb == current select block to use
*
* RETURNS:
*   pointer to first xpath_pcb_t for the lock
*********************************************************************/
xpath_pcb_t *
    plock_get_next_select (xpath_pcb_t *xpathpcb)
{
#ifdef DEBUG
    if (xpathpcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (xpath_pcb_t *)dlq_nextEntry(xpathpcb);

}  /* plock_get_next_select */


/********************************************************************
* FUNCTION plock_add_select
*
* Add a select XPath control block to the partial lock
*
* INPUTS:
*   plcb == partial lock control block to use
*   xpathpcb == xpath select block to add
*   result == result struct to add
*
*********************************************************************/
void
    plock_add_select (plock_cb_t *plcb,
                      xpath_pcb_t *xpathpcb,
                      xpath_result_t *result)
{
#ifdef DEBUG
    if (plcb == NULL || xpathpcb == NULL || result == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    dlq_enque(xpathpcb, &plcb->plock_xpathpcbQ);
    dlq_enque(result, &plcb->plock_resultQ);

}  /* plock_add_select */


/********************************************************************
* FUNCTION plock_make_final_result
*
* Create a final XPath result for all the partial results
*
* This does not add the partial lock to the target config!
* This is an intermediate step!
*
* INPUTS:
*   plcb == partial lock control block to use
*
* RETURNS:
*    status; NCX_ERR_INVALID_VALUE if the final nodeset is empty
*********************************************************************/
status_t
    plock_make_final_result (plock_cb_t *plcb)
{
    xpath_result_t *result;
    xpath_pcb_t    *xpathpcb;

#ifdef DEBUG
    if (plcb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    xpathpcb = (xpath_pcb_t *)
        dlq_firstEntry(&plcb->plock_xpathpcbQ);
    if (xpathpcb == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    for (result = (xpath_result_t *)
             dlq_firstEntry(&plcb->plock_resultQ);
         result != NULL;
         result = (xpath_result_t *)dlq_nextEntry(result)) {
        xpath_move_nodeset(result, plcb->plock_final_result);
    }

    xpath1_prune_nodeset(xpathpcb, plcb->plock_final_result);

    if (xpath_nodeset_empty(plcb->plock_final_result)) {
        return ERR_NCX_XPATH_NODESET_EMPTY;
    } else {
        return NO_ERR;
    }

}  /* plock_make_final_result */


/* END file plock.c */
