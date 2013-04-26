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
/*    File dlq.c

      dlq provides general queue and linked list support:

       QUEUE initialization/cleanup
       * create queue   (create and initialize dynamic queue hdr)
       * destroy queue  (destroy previously created dynamic queue)         
       * create Squeue  (initialize static queue hdr--no destroyS function)

       FIFO queue operators
       * enque       (add node to end of list)
       * deque       (return first node - remove from list)
       * empty       (return TRUE if queue is empty, FALSE otherwise)

       LINEAR search (linked list)
       * nextEntry   (return node AFTER param node - leave in list)
       * prevEntry   (return node BEFORE param node - leave in list)
       * insertAhead (add node in list ahead of param node)
       * insertAfter (add node in list after param node)
       * remove      (remove a node from a linked list)


*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
07-jan-89    abb      Begun.
24-mar-90    abb      Add que_firstEntry routine
18-feb-91    abb      Add que_createSQue function
12-mar-91    abb      Adapt for DAVID sbee project
13-jun-91    abb      change que.c to dlq.c
22-oct-93    abb      added critical section hooks
14-nov-93    abb      moved to this library from \usr\src\gendep\1011
15-jan-07    abb      update for NCX software project 
                      add dlq_swap, dlq_hdr_t
                      change err_msg to use log_error function
12-oct-07    abb      add dlq_count
17-feb-10    abb      fill in function headers
*/

/********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#define     err_msg(S)  log_error("\nerr: %s ",#S)


/* add enter and exit critical section hooks here */
#ifndef ENTER_CRIT
#define ENTER_CRIT
#endif

#ifndef EXIT_CRIT
#define EXIT_CRIT
#endif


#ifdef CPP_DEBUG    
/********************************************************************
* FUNCTION dlq_dumpHdr
* 
* use log_debug to dump a Queue header contents
*
* INPUTS:
*   nodeP == Q header cast as a void *
*********************************************************************/
void dlq_dumpHdr (const void *nodeP)
{
    const dlq_hdrT   *p = (const dlq_hdrT *) nodeP;

    if (p==NULL)
    {
        log_debug("\ndlq: NULL hdr");
        return;
    }
    log_debug("\ndlq: ");
    switch(p->hdr_typ)
    {
    case DLQ_NULL_NODE:
        log_debug("unused        ");
        break;
    case DLQ_SHDR_NODE:
        log_debug("shdr node");
        break;
    case DLQ_DHDR_NODE:
        log_debug("dhdr node");
        break;
    case DLQ_DATA_NODE:
        log_debug("data node");
        break;
    case DLQ_DEL_NODE:
        log_debug("deleted       ");
        break;
    default:
        log_debug("invalid       ");
    }
    
    log_debug("(%p) p (%p) n (%p)", p, p->prev, p->next);

}   /* END dlq_dumpHdr */
#endif      /* CPP_DEBUG */



/********************************************************************
* FUNCTION dlq_createQue
* 
* create a dynamic queue header
* 
* RETURNS:
*   pointer to malloced queue header
*   NULL if memory error
*********************************************************************/
dlq_hdrT * dlq_createQue (void)
{
    REG dlq_hdrT  *retP;

    retP = m__getObj(dlq_hdrT);
    if (retP==NULL) {
        return NULL;
    }

    /* init empty list */
    retP->hdr_typ = DLQ_DHDR_NODE;
    retP->prev = retP->next = retP;
    return retP;

}    /* END function dlq_createQue */



/********************************************************************
* FUNCTION dlq_createSQue
* 
* create a static queue header
* 
* INPUTS:
*   queAddr == pointer to malloced queue header to initialize
*********************************************************************/
void  dlq_createSQue (dlq_hdrT *queAddr)
{
#ifdef CPP_ICHK
    if (queAddr==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
#endif
    
    /* init empty list */
    ENTER_CRIT;
    queAddr->hdr_typ = DLQ_SHDR_NODE;
    queAddr->prev = queAddr->next = queAddr;
    EXIT_CRIT;
    
}    /* END function dlq_createSQue */



/********************************************************************
* FUNCTION dlq_destroyQue
* 
* free a dynamic queue header previously allocated
* with dlq_createQue
*
* INPUTS:
*   listP == pointer to malloced queue header to free
*********************************************************************/
void dlq_destroyQue (dlq_hdrT *listP)
{
    if ( !listP )
    {
        return;
    }

#ifdef CPP_ICHK
    if (!_hdr_node(listP) || !dlq_empty(listP))
    {
        err_msg(ERR_INTERNAL_QDEL);
        return;
    }
#endif

    if (listP->hdr_typ==DLQ_DHDR_NODE)
    {
        listP->hdr_typ = DLQ_DEL_DHDR;
        m__free(listP);
    }
    
}    /* END function dlq_destroyQue */



/********************************************************************
* FUNCTION dlq_enque
* 
* add a queue node to the end of a queue list
*
* INPUTS:
*   newP == pointer to queue entry to add
*   listP == pointer to queue list to put newP
*********************************************************************/
void dlq_enque (REG void *newP, REG dlq_hdrT *listP)
{
#ifdef CPP_ICHK
    if (newP==NULL || listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    else if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }
#endif

    ENTER_CRIT;
    /* code copied directly form dlq_insertAhead to save a little time */
    ((dlq_hdrT *) newP)->hdr_typ = DLQ_DATA_NODE;
    ((dlq_hdrT *) newP)->next = listP;
    ((dlq_hdrT *) newP)->prev = ((dlq_hdrT *) listP)->prev;
    ((dlq_hdrT *) newP)->next->prev = newP;
    ((dlq_hdrT *) newP)->prev->next = newP;
    EXIT_CRIT;
    
}    /* END function dlq_enque */



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
void  *dlq_deque (dlq_hdrT *  listP)
{
    REG void   *nodeP;

#ifdef CPP_ICHK
    if (listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return NULL;
    }
    else if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return NULL;
    }
#endif

    ENTER_CRIT;
    /* check que empty */
    if (listP==listP->next)
    {
        EXIT_CRIT;
        return NULL;
    }
    
    /* 
     * return next que element, after removing it from the que
     * the que link ptrs in 'nodeP' are set to NULL upon return
     * the dlq_hdr in 'nodeP' is also marked as deleted
     */

    nodeP = listP->next;
#ifdef CPP_ICHK
    if (!_data_node(nodeP))
    {
        EXIT_CRIT;
        err_msg(ERR_QNODE_NOT_DATA);
        return NULL;
    }
#endif

    /*** dlq_remove (listP->next); *** copied inline below ***/

    /* relink the queue chain */
    ((dlq_hdrT *) nodeP)->prev->next = ((dlq_hdrT *) nodeP)->next;
    ((dlq_hdrT *) nodeP)->next->prev = ((dlq_hdrT *) nodeP)->prev;

    /* remove the node from the linked list */
    ((dlq_hdrT *) nodeP)->hdr_typ = DLQ_DEL_NODE;
    ((dlq_hdrT *) nodeP)->prev   = NULL;
    ((dlq_hdrT *) nodeP)->next   = NULL;
    EXIT_CRIT;
    
    return nodeP;

}    /* END function dlq_deque */



#if defined(CPP_NO_MACROS)
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
void  *dlq_nextEntry (const void *nodeP)
{
    void    *retP;
    
#ifdef CPP_ICHK 
    if (nodeP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return NULL;    /* error */
    }
#endif

    ENTER_CRIT;
    /* get next entry in list -- maybe  (hdr_node==end of list) */
    nodeP = ((dlq_hdrT *) nodeP)->next;

#ifdef CPP_ICHK
    if (!(_data_node(nodeP) || _hdr_node(nodeP)))
    {
        EXIT_CRIT;
        err_msg(ERR_BAD_QLINK);
        return NULL;
    }
#endif
    
    retP = _data_node( ((dlq_hdrT *) nodeP) ) ? nodeP : NULL;
    EXIT_CRIT;
    
    return retP;

}   /* END function dlq_nextEntry */
#endif      /* CPP_NO_MACROS */


#if defined(CPP_NO_MACROS)
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
void  *dlq_prevEntry (const void *nodeP)
{
    void    *retP;
    
#ifdef CPP_ICHK
    if (nodeP==NULL)   
    {
        err_msg(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    ENTER_CRIT;
    /* get prev entry in list -- maybe  (hdr_node==start of list) */
    nodeP = ((dlq_hdrT *) nodeP)->prev;

#ifdef CPP_ICHK
    if (!(_data_node(nodeP) || _hdr_node(nodeP)))
    {
        EXIT_CRIT;
        err_msg(ERR_BAD_QLINK);
        return NULL;
    }
#endif
    
    retP = _data_node( ((dlq_hdrT *) nodeP) ) ? nodeP : NULL;
    EXIT_CRIT;

    return retP;
    
}   /* END function dlq_prevEntry */
#endif      /* CPP_NO_MACROS */



/********************************************************************
* FUNCTION dlq_insertAhead
* 
* insert the new queue entry before the current entry
*
* INPUTS:
*   newP == pointer to new queue entry to insert ahead of nodeP
*   nodeP == pointer to current queue entry to insert ahead
*********************************************************************/
void dlq_insertAhead (void *newP, void *nodeP)
{
#ifdef CPP_ICHK
    if (nodeP==NULL || newP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (nodeP==newP)
    {
        err_msg(ERR_INTERNAL_VAL);
        return;
    }
#endif
    ENTER_CRIT;
    ((dlq_hdrT *) newP)->hdr_typ            = DLQ_DATA_NODE;
    ((dlq_hdrT *) newP)->next       = nodeP;
    ((dlq_hdrT *) newP)->prev       = ((dlq_hdrT *) nodeP)->prev;
    ((dlq_hdrT *) newP)->next->prev  = newP;
    ((dlq_hdrT *) newP)->prev->next  = newP;
    EXIT_CRIT;
    
}    /* END function dlq_insertAhead */



/********************************************************************
* FUNCTION dlq_insertAfter
* 
* insert the new queue entry after the current entry
*
* INPUTS:
*   newP == pointer to new queue entry to insert after nodeP
*   nodeP == pointer to current queue entry to insert after
*********************************************************************/
void dlq_insertAfter (void *newP, void *nodeP)
{
#ifdef CPP_ICHK
    if (nodeP==NULL || newP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (nodeP==newP)
    {
        err_msg(ERR_INTERNAL_VAL);
        return;
    }
#endif

    ENTER_CRIT;    
    ((dlq_hdrT *) newP)->hdr_typ            = DLQ_DATA_NODE;
    ((dlq_hdrT *) newP)->prev       = nodeP;
    ((dlq_hdrT *) newP)->next       = ((dlq_hdrT *) nodeP)->next;
    ((dlq_hdrT *) newP)->next->prev  = newP;
    ((dlq_hdrT *) newP)->prev->next  = newP;
    EXIT_CRIT;
    
}    /* END function dlq_insertAfter */



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
void dlq_remove (void *nodeP)
{
#ifdef CPP_ICHK
    if (nodeP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    else if (!_data_node( ((dlq_hdrT *) nodeP) ))
    {
        err_msg(ERR_QNODE_NOT_DATA);
        return;
    }
#endif

    ENTER_CRIT;
    /* relink the queue chain */
    ((dlq_hdrT *) nodeP)->prev->next = ((dlq_hdrT *) nodeP)->next;
    ((dlq_hdrT *) nodeP)->next->prev = ((dlq_hdrT *) nodeP)->prev;

    /* remove the node from the linked list */
    ((dlq_hdrT *) nodeP)->hdr_typ = DLQ_DEL_NODE;
    ((dlq_hdrT *) nodeP)->prev   = NULL;
    ((dlq_hdrT *) nodeP)->next   = NULL;
    EXIT_CRIT;
    
}   /* END fuction dlq_remove */



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
void dlq_swap (void *new_node, void *cur_node)
{
#ifdef CPP_ICHK
    if (new_node==NULL || cur_node==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    else if (!_data_node( ((dlq_hdrT *) cur_node) ))
    {
        err_msg(ERR_QNODE_NOT_DATA);
        return;
    }
#endif

    ENTER_CRIT;

    /* replace cur_node with new_node in the queue chain */
    ((dlq_hdrT *) cur_node)->prev->next = new_node;
    ((dlq_hdrT *) new_node)->prev = ((dlq_hdrT *) cur_node)->prev;
    ((dlq_hdrT *) cur_node)->prev = NULL;

    ((dlq_hdrT *) cur_node)->next->prev = new_node;
    ((dlq_hdrT *) new_node)->next = ((dlq_hdrT *) cur_node)->next;
    ((dlq_hdrT *) cur_node)->next = NULL;

    /* mark the new node as being in a Q */
    ((dlq_hdrT *) new_node)->hdr_typ = DLQ_DATA_NODE;

    /* mark the current node as removed from the Q */
    ((dlq_hdrT *) cur_node)->hdr_typ = DLQ_DEL_NODE;

    EXIT_CRIT;
    
}   /* END fuction dlq_swap */



#if defined(CPP_NO_MACROS)
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
void *dlq_firstEntry (const dlq_hdrT *    listP)
{
    void    *retP;
    
#ifdef CPP_ICHK
    if (listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return NULL;
    }
    else if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return NULL;
    }
#endif
    ENTER_CRIT;
    retP = (listP != listP->next) ? listP->next : NULL;
    EXIT_CRIT;

    return retP;

}    /* END function dlq_firstEntry */
#endif      /* CPP_NO_MACROS */



#if defined(CPP_NO_MACROS)
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
void *dlq_lastEntry (const dlq_hdrT *    listP)
{
    void    *retP;
    
#ifdef CPP_ICHK
    if (listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return NULL;
    }
    else if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return NULL;
    }
#endif

    ENTER_CRIT;
    retP = (listP != listP->next) ? listP->prev : NULL;
    EXIT_CRIT;

    return retP;

}    /* END function dlq_lastEntry */
#endif      /* CPP_NO_MACROS */


#if defined(CPP_NO_MACROS)
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
boolean dlq_empty (const dlq_hdrT *  listP)
{
    boolean ret;
    
#ifdef CPP_ICHK
    if (listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return TRUE;
    }
    if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return TRUE;
    }
#endif

    ENTER_CRIT;
    ret = (boolean) (listP==listP->next);
    EXIT_CRIT;

    return ret;

}    /* END function dlq_empty */
#endif



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
void       dlq_block_enque (dlq_hdrT * srcP, dlq_hdrT * dstP)
{
    dlq_hdrT  *sf, *sl, *dl;
    
#ifdef CPP_ICHK
    if (srcP==NULL || dstP==NULL)
        return;
    if (!_hdr_node(srcP) || !_hdr_node(dstP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }
#endif

    /* check simple case first */
    if (dlq_empty(srcP))
        return;         /* nothing to add to dst que */

    /* check next simple case--dst empty */
    if (dlq_empty(dstP))
    {
        ENTER_CRIT;
        /* copy srcP pointers to dstP */
        dstP->next = srcP->next;
        dstP->prev = srcP->prev;
        
        /* relink first and last data nodes */
        dstP->next->prev = dstP;
        dstP->prev->next = dstP;
        
        /* make src que empty */
        srcP->next = srcP->prev = srcP;
        EXIT_CRIT;
        return;
    }

    ENTER_CRIT;    
    /* else neither que is empty...move [sf..sl] after dl */
    sf = srcP->next;        /* source first */
    sl = srcP->prev;        /* source last */
    dl = dstP->prev;        /* dst last */
    
    /* extend the dstQ */
    dl->next = sf;          /* link dl --> sf */
    sf->prev = dl;          /* link dl <-- sf */

    /* relink the new last data node in dstQ */
    dstP->prev = sl;        /* link hdr.prev --> sl */
    sl->next = dstP;        /* link hdr.prev <-- sl */

    /* make the srcQ empty */
    srcP->prev = srcP->next = srcP;
    EXIT_CRIT;
    
}   /* END dlq_block_enque */



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
void       dlq_block_insertAhead (dlq_hdrT * srcP, void *dstP)
{
    
    REG dlq_hdrT    *sf, *sl, *d1, *d2;
    
#ifdef CPP_ICHK
    if (srcP==NULL || dstP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (!_hdr_node(srcP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }

    if (!_data_node( ((dlq_hdrT *)dstP) ))
    {
        err_msg(ERR_QNODE_NOT_DATA);
        return;
    }
#endif

    
    /* check simple case first */
    if (dlq_empty(srcP))
        return;         /* nothing to add to dst que */

    ENTER_CRIT;
    /* source que is empty... */
    sf = srcP->next;            /* source-first */
    sl = srcP->prev;            /* source-last */
    
    d1 = ((dlq_hdrT *) dstP)->prev;         /* dest-begin-insert */
    d2 = (dlq_hdrT *) dstP;                 /* dest-end-insert (dstP) */

    /* link src-list into the dst-list */
    d1->next = sf;
    sf->prev = d1;
    
    d2->prev = sl;
    sl->next = d2;

    /* make srcQ empty */
    srcP->prev = srcP->next = srcP;
    EXIT_CRIT;
    
}   /* END dlq_block_insertAhead */



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
void       dlq_block_insertAfter (dlq_hdrT * srcP, void *dstP)
{
    
    REG dlq_hdrT   *sf, *sl, *d1, *d2;
    
#ifdef CPP_ICHK
    if (srcP==NULL || dstP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (!_hdr_node(srcP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }

    if (!_data_node( ((dlq_hdrT *)dstP) ))
    {
        err_msg(ERR_QNODE_NOT_DATA);
        return;
    }
#endif

    
    /* check simple case first */
    if (dlq_empty(srcP))
        return;         /* nothing to add to dst que */

    ENTER_CRIT;
    /* source que is not empty... */
    sf = srcP->next;            /* source-first */
    sl = srcP->prev;            /* source-last */

    /* make new chain: d1 + sf [ + ... ] + sl + d2 (+...) */
    d1 = (dlq_hdrT *) dstP;                     /* dest-begin-insert */
    d2 = ((dlq_hdrT *) dstP)->next;             /* dest-end-insert (dstP) */

    /* link src-list into the dst-list */
    d1->next = sf;
    sf->prev = d1;
    
    d2->prev = sl;
    sl->next = d2;

    /* make srcQ empty */
    srcP->prev = srcP->next = srcP;
    EXIT_CRIT;
    
}   /* END dlq_block_insertAfter */




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
void       dlq_block_move (dlq_hdrT * srcQ, void *srcP, dlq_hdrT * dstQ)
{

    
    REG dlq_hdrT   *sf, *sl;
    dlq_hdrT        tmpQ;
    
#ifdef CPP_ICHK
    if (srcQ==NULL || srcP==NULL || dstQ==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (!_hdr_node(srcQ) || !_hdr_node(dstQ))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }
    if (!_data_node(srcP))
    {
        err_msg(ERR_QNODE_NOT_DATA);
        return;
    }
#endif  
    /* check simple case first */
    if (dlq_empty(srcQ))
        return;         /* nothing to add to dst que */

    ENTER_CRIT;    
    /* unlink the srcQ list from srcP to the end */
    sf = (dlq_hdrT *) srcP;
    sf->prev->next = srcQ;      
    sl = srcQ->prev;
    srcQ->prev = sf->prev;

    /* insert new chain into tmpQ */
    dlq_createSQue(&tmpQ);  
    tmpQ.next = sf;     
    sf->prev = &tmpQ;
    tmpQ.prev = sl;
    sl->next = &tmpQ;
    EXIT_CRIT;
    
    dlq_block_enque(&tmpQ, dstQ);
    
}   /* END dlq_block_move */



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
unsigned int  dlq_count (const dlq_hdrT *listP)
{
    REG const dlq_hdrT    *p;
    REG unsigned int cnt;
    
#ifdef CPP_ICHK
    if (listP==NULL)
    {
        err_msg(ERR_INTERNAL_PTR);
        return;
    }
    if (!_hdr_node(listP))
    {
        err_msg(ERR_QNODE_NOT_HDR);
        return;
    }
#endif

    cnt = 0;

    for (p = dlq_firstEntry(listP);
         p != NULL;
         p = dlq_nextEntry(p)) {
        cnt++;
    }

    return cnt;
    
}   /* END dlq_count */


/* END file dlq.c */
