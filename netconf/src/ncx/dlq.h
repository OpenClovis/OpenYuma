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
#ifndef _H_dlq
#define _H_dlq
/*  FILE: dlq.h
*********************************************************************
*                                    *
*             P U R P O S E                    *
*                                    *
*********************************************************************

    dlq provides general double-linked list and queue support:
   
    API Functions
    =============

       QUEUE initialization/cleanup
       * dlq_createQue - create and initialize dynamic queue hdr
       * dlq_destroyQue - destroy previously created dynamic queue
       * dlq_createSQue - initialize static queue hdr, no destroy needed

       FIFO queue operations
       * dlq_enque - add node to end of list
       * dlq_block_enque - add N nodes to end of list
       * dlq_deque - return first node, remove from list

       TEST queue operations
       * dlq_empty - return TRUE if queue is empty, FALSE otherwise

       LINEAR search (linked list)
       * dlq_nextEntry - return node AFTER param node - leave in list
       * dlq_prevEntry - return node BEFORE param node - leave in list
       * dlq_firstEntry - return first node in the Q
       * dlq_lastEntry - return last node in the Q

       Q INSERTION operations
       * dlq_insertAhead - add node in list ahead of param node
       * dlq_insertAfter - add node in list after param node
       * dlq_block_insertAhead - add N nodes in list ahead of param node
       * dlq_block_insertAfter - add N nodes in list after param node
       * dlq_block_move - move contents of srcQ to the destintaion Q

       Q DELETION operations
       * dlq_remove - remove a node from a linked list

       Q DEBUG operations
       * dlq_dumpHdr - printf Q header info  (CPP_DEBUG required)

   CPP Macros Used:
   ================

       CPP_DEBUG - enables debug code

       CPP_NO_MACROS - forces function calls instead of macros for
           some functions.  Should not be used except in some debug modes

       CPP_ICHK - this will force function calls instead of macros and
           enable lots of parameter checking (internal checks). Should
           only be used for unit test debugging

       ENTER_CRIT - this macro hook can be set to call an enter critical
           section function for thread-safe queueing
       EXIT_CRIT - this macro hook can be set to call an exit critical
           section function for thread-safe queueing

*********************************************************************
*                                    *
*           C H A N G E     H I S T O R Y                *
*                                    *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
06-jan-89    abb      Begun.
18-jan-91    abb      adapted for depend project
12-mar-91    abb      adapted for DAVID sbee project
14-jun-91    abb      changed que.h to dlq.h
27-apr-05    abb      update docs and use for netconf project
15-feb-06    abb      make DLQ module const compatible
                      get rid of dlq_hdrPT, as this doesn't work 
                      for const pointers.
15-sep-06    abb      added dlq_swap function
26-jan-07    abb      added dlq_hdr_t alias for NCX naming conventions
12-oct-07    abb      add dlq_count
*/

#include "procdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*
*                             C O N S T A N T S
*
*********************************************************************/

/* que header types */
#define        DLQ_NULL_NODE    1313
#define        DLQ_SHDR_NODE    2727
#define        DLQ_DHDR_NODE    3434
#define        DLQ_DATA_NODE    5757
#define        DLQ_DEL_NODE     8686
#define        DLQ_DEL_DHDR     9696

#define dlq_hdr_t  dlq_hdrT


/********************************************************************
*
*                           T Y P E S
*                                    
*********************************************************************/

typedef struct TAGdlq_hdrT 
{
     unsigned short        hdr_typ;
     struct TAGdlq_hdrT    *prev;
     struct TAGdlq_hdrT    *next;
} dlq_hdrT;


/* shorthand macros */
#define _hdr_node(P) (((const dlq_hdrT *)(P))->hdr_typ==DLQ_SHDR_NODE \
    || ((const dlq_hdrT *)(P))->hdr_typ==DLQ_DHDR_NODE)

#define _data_node(P) (((const dlq_hdrT *)(P))->hdr_typ==DLQ_DATA_NODE)

/********************************************************************
*
*                          F U N C T I O N S
*
*********************************************************************/

#ifdef CPP_DEBUG
/********************************************************************
* FUNCTION dlq_dumpHdr
* 
* use log_debug to dump a Queue header contents
*
* INPUTS:
*   nodeP == Q header cast as a void *
*********************************************************************/
extern void dlq_dumpHdr (const void *nodeP);
#endif


/********************************************************************
* FUNCTION dlq_createQue
* 
* create a dynamic queue header
* 
* RETURNS:
*   pointer to malloced queue header
*   NULL if memory error
*********************************************************************/
extern dlq_hdrT * dlq_createQue (void);


/********************************************************************
* FUNCTION dlq_createSQue
* 
* create a static queue header
* 
* INPUTS:
*   queAddr == pointer to malloced queue header to initialize
*********************************************************************/
extern void dlq_createSQue (dlq_hdrT * queAddr);


/********************************************************************
* FUNCTION dlq_destroyQue
* 
* free a dynamic queue header previously allocated
* with dlq_createQue
*
* INPUTS:
*   listP == pointer to malloced queue header to free
*********************************************************************/
extern void dlq_destroyQue (dlq_hdrT * listP);


/********************************************************************
* FUNCTION dlq_enque
* 
* add a queue node to the end of a queue list
*
* INPUTS:
*   newP == pointer to queue entry to add
*   listP == pointer to queue list to put newP
*********************************************************************/
extern void dlq_enque (REG void *newP, REG dlq_hdrT * listP);


/********************************************************************
* FUNCTION dlq_deque
* 
* remove the first queue node from the queue list
*
* INPUTS:
*   listP == pointer to queue list remove the first entry
*
* RETURNS:
*   pointer to removed first entry
*   NULL if the queue was empty
*********************************************************************/
extern void *dlq_deque (dlq_hdrT * listP);


/********************************************************************
* FUNCTION dlq_nextEntry
* 
* get the next queue entry after the current entry
*
* INPUTS:
*   nodeP == pointer to current queue entry to use
*
* RETURNS:
*   pointer to next queue entry
*   NULL if the no next entry was found
*********************************************************************/
#if defined(CPP_NO_MACROS)
extern void *dlq_nextEntry (const void *nodeP);
#else
#define dlq_nextEntry(P)  (_data_node(((const dlq_hdrT *) (P))->next) ? \
      ((const dlq_hdrT *) (P))->next : NULL)
#endif        /* END CPP_NO_MACROS */


/********************************************************************
* FUNCTION dlq_prevEntry
* 
* get the previous queue entry before the current entry
*
* INPUTS:
*   nodeP == pointer to current queue entry to use
*
* RETURNS:
*   pointer to previous queue entry
*   NULL if the no previous entry was found
*********************************************************************/
#if defined(CPP_NO_MACROS)
extern void *dlq_prevEntry (const void *nodeP);
#else
#define dlq_prevEntry(P) (_data_node(((const dlq_hdrT *) (P))->prev ) ? \
      ((const dlq_hdrT *) (P))->prev : NULL)
#endif    /* CPP_NO_MACROS */


/********************************************************************
* FUNCTION dlq_insertAhead
* 
* insert the new queue entry before the current entry
*
* INPUTS:
*   newP == pointer to new queue entry to insert ahead of nodeP
*   nodeP == pointer to current queue entry to insert ahead
*********************************************************************/
extern void dlq_insertAhead (void *newP, void *nodeP);


/********************************************************************
* FUNCTION dlq_insertAfter
* 
* insert the new queue entry after the current entry
*
* INPUTS:
*   newP == pointer to new queue entry to insert after nodeP
*   nodeP == pointer to current queue entry to insert after
*********************************************************************/
extern void dlq_insertAfter (void *newP, void *nodeP);


/********************************************************************
* FUNCTION dlq_remove
* 
* remove the queue entry from its queue list
* entry MUST have been enqueued somehow before
* this function is called
*
* INPUTS:
*   nodeP == pointer to queue entry to remove from queue
*********************************************************************/
extern void dlq_remove (void *nodeP);


/********************************************************************
* FUNCTION dlq_swap
* 
* remove the cur_node queue entry from its queue list
* and replace it with the new_node
* cur_node entry MUST have been enqueued somehow before
* this function is called. 
* new_node MUST NOT already be in a queue
*
* INPUTS:
*   new_node == pointer to new queue entry to put into queue
*   cur_node == pointer to current queue entry to remove from queue
*********************************************************************/
extern void dlq_swap (void *new_node, void *cur_node);


/********************************************************************
* FUNCTION dlq_firstEntry
* 
* get the first entry in the queue list
*
* INPUTS:
*   listP == pointer to queue list to get the first entry from
*
* RETURNS:
*   pointer to first queue entry
*   NULL if the queue is empty
*********************************************************************/
#if defined(CPP_NO_MACROS)
extern void *dlq_firstEntry (const dlq_hdrT * listP);
#else
#define dlq_firstEntry(P) ((P) != ((const dlq_hdrT *)(P))->next ? \
        ((const dlq_hdrT *)(P))->next : NULL)
#endif        /* CPP_NO_MACROS */


/********************************************************************
* FUNCTION dlq_lastEntry
* 
* get the last entry in the queue list
*
* INPUTS:
*   listP == pointer to queue list to get the last entry from
*
* RETURNS:
*   pointer to last queue entry
*   NULL if the queue is empty
*********************************************************************/
#if defined(CPP_NO_MACROS)
extern void *dlq_lastEntry (const dlq_hdrT * listP);
#else
#define dlq_lastEntry(P) ((P) != ((const dlq_hdrT *)(P))->next ? \
        ((const dlq_hdrT *)(P))->prev : NULL)
#endif        /* CPP_NO_MACROS */


/********************************************************************
* FUNCTION dlq_empty
* 
* check if queue list is empty
*
* INPUTS:
*   listP == pointer to queue list to check
*
* RETURNS:
*   TRUE if queue is empty
*   FALSE if queue is not empty
*********************************************************************/
#if defined(CPP_NO_MACROS)
extern boolean dlq_empty (const dlq_hdrT * listP);
#else
#define dlq_empty(P) (boolean)((P)==((const dlq_hdrT *)(P))->next)
#endif        /* CPP_NO_MACROS */


/********************************************************************
* FUNCTION dlq_block_enque
* 
* add all the queue entries in the srcP queue list to the
* end of the dstP queue list
*
* INPUTS:
*   srcP == pointer to queue list entry to add end of dstP list
*   dstP == pointer to queue list to add all newP entries
*********************************************************************/
extern void dlq_block_enque (dlq_hdrT * srcP, dlq_hdrT * dstP);


/********************************************************************
* FUNCTION dlq_block_insertAhead
* 
* insert all the entries in the srcP queue list before 
* the dstP queue entry
*
* INPUTS:
*   srcP == pointer to new queue list to insert all entries
*           ahead of dstP
*   dstP == pointer to current queue entry to insert ahead
*********************************************************************/
extern void dlq_block_insertAhead (dlq_hdrT *srcP, void *dstP);


/********************************************************************
* FUNCTION dlq_block_insertAfter
* 
* insert all the entries in the srcP queue list after
* the dstP queue entry
*
* INPUTS:
*   srcP == pointer to new queue list to insert all entries
*           after dstP
*   dstP == pointer to current queue entry to insert after
*********************************************************************/
extern void dlq_block_insertAfter (dlq_hdrT *srcP, void *dstP);


/********************************************************************
* FUNCTION dlq_block_move
* 
* enque from [srcP ..  end of srcQ list] to the dstQ
* insert all the entries in the srcP queue list after
* the dstP queue entry
*
* INPUTS:
*   srcQ == pointer to source queue list to move entries from
*   srcP == pointer to source queue entry in the srcQ
*           move this entry and all entries to the end of
*           the srcQ to the end of dstQ
*   dstQ == pointer to destination queue list to move the
*           entries from the srcQ to the end of this queue
*********************************************************************/
extern void dlq_block_move (dlq_hdrT *srcQ, void *srcP, dlq_hdrT * dstQ);


/********************************************************************
* FUNCTION dlq_count
* 
* get the number of queue entries in the listP queue list
*
* INPUTS:
*   listP == pointer to queue list to check
*
* RETURNS:
*   number of queue entries found in listP queue
*********************************************************************/
extern unsigned int dlq_count (const dlq_hdrT *listP);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif    /* _H_dlq */
