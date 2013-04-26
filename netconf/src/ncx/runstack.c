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
/*  FILE: runstack.c

    Simple stack of script execution contexts
    to support nested 'run' commands within scripts

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
22-aug-07    abb      begun; split from ncxcli.c


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "procdefs.h"
#include "log.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "runstack.h"
#include "status.h"
#include "val.h"
#include "var.h"
#include "xml_util.h"
#include "xpath.h"
#include "xpath1.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#ifdef DEBUG
#define RUNSTACK_DEBUG   1
#endif


#define RUNSTACK_BUFFLEN  32000

/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean            runstack_init_done = FALSE;
static runstack_context_t defcxt;


static dlq_hdr_t* select_useQ( runstack_context_t* rcxt )
{
    runstack_entry_t *se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    // TODO reverse this logic...
    if ( se == NULL) {
        return &rcxt->zero_condcbQ;
    } else {
        return &se->condcbQ;
    }
}


/********************************************************************
* FUNCTION make_parmvar
* 
* 
*  Still need to add the script parameters (if any)
*  with the runstack_setparm
*
* INPUTS:
*   source == file source
*   fp == file pointer
*
* RETURNS:
*   pointer to new val struct, NULL if an error
*********************************************************************/
static val_value_t *
    make_parmval (const xmlChar *name,
                  const xmlChar *value)
{
    val_value_t    *val;
    status_t        res;

    /* create the parameter */
    val = val_new_value();
    if (!val) {
        return NULL;
    }

    /* set the string value */
    res = val_set_string(val, name, value);
    if (res != NO_ERR) {
        val_free_value(val);
        return NULL;
    }
    return val;

}  /* make_parmval */


/********************************************************************
* FUNCTION get_parent_cond_state
* 
* Get the parent conditional state
*
* INPUTS:
*   condcb == current conditional control block
*
* RETURNS:
*   TRUE if parent conditiona; state is TRUE
*   FALSE otherwise
*********************************************************************/
static boolean
    get_parent_cond_state (runstack_condcb_t *condcb)

{
    runstack_condcb_t *testcb;

    testcb = (runstack_condcb_t *)dlq_prevEntry(condcb);
    if (testcb == NULL) {
        return TRUE;
    } else if (testcb->cond_type == RUNSTACK_COND_IF) {
        return testcb->u.ifcb.curcond;
    } else {
        return testcb->u.loopcb.startcond;     
    }

}  /* get_parent_cond_state */


/********************************************************************
* FUNCTION reset_cond_state
* 
* Reset the runstack context condition state
*
* INPUTS:
*   rcxt == runstack context to use
*
*********************************************************************/
static void
    reset_cond_state (runstack_context_t *rcxt)

{
    runstack_entry_t  *se;
    dlq_hdr_t         *useQ;
    runstack_condcb_t *condcb;

    se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    if (se == NULL) {
        useQ = &rcxt->zero_condcbQ;
    } else {
        useQ = &se->condcbQ;
    }

    condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if (condcb == NULL) {
        rcxt->cond_state = TRUE;
    } else if (condcb->cond_type == RUNSTACK_COND_IF) {
        rcxt->cond_state = condcb->u.ifcb.curcond;
    } else {
        rcxt->cond_state = condcb->u.loopcb.startcond;     
    }

}  /* reset_cond_state */


/********************************************************************
* FUNCTION free_line_entry
* 
* Clean and free a runstack line entry
*
* INPUTS:
*   le == line entry to free
*
*********************************************************************/
static void
    free_line_entry (runstack_line_t *le)

{
    if (le->line) {
        m__free(le->line);
    }
    m__free(le);

}  /* free_line_entry */


/********************************************************************
* FUNCTION new_line_entry
* 
* Malloc and init a new runstack line entry
*
* INPUTS:
*   line == line to save, a copy will be made
*
* RETURNS:
*   pointer to new runstack line entry, NULL if an error
*********************************************************************/
static runstack_line_t *
    new_line_entry (const xmlChar *line)
{
    runstack_line_t *le;

    le = m__getObj(runstack_line_t);
    if (le == NULL) {
        return NULL;
    }

    memset(le, 0x0, sizeof(runstack_line_t));

    le->line = xml_strdup(line);
    if (le->line == NULL) {
        m__free(le);
        return NULL;
    }
    return le;

}  /* new_line_entry */


/********************************************************************
* FUNCTION get_loopcb
* 
* Get the most recent loop control block
* 
* INPUTS:
*   rcxt == runstack contect to use
*
* RETURNS:
*   pointer to last cond block with loopcb or NULL if none
*********************************************************************/
static runstack_condcb_t *get_loopcb (runstack_context_t *rcxt)
{
    dlq_hdr_t *useQ = select_useQ( rcxt );

    runstack_condcb_t *condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    for (; condcb; condcb = (runstack_condcb_t *)dlq_prevEntry(condcb)) {
        if (condcb->cond_type == RUNSTACK_COND_LOOP) {
            return condcb;
        }
    }

    return NULL;
}  /* get_loopcb */


/********************************************************************
* FUNCTION get_first_loopcb
* 
* Get the first loop, which has the lineQ
*
* INPUTS:
*   rcxt == runstack contect to use
*
* RETURNS:
*   pointer to first cond block with loopcb or NULL if none
*********************************************************************/
static runstack_condcb_t *get_first_loopcb (runstack_context_t *rcxt)
{
    dlq_hdr_t *useQ = select_useQ( rcxt );

    runstack_condcb_t *condcb = (runstack_condcb_t *)dlq_firstEntry(useQ);
    for (; condcb; condcb = (runstack_condcb_t *)dlq_nextEntry(condcb)) {
        if (condcb->cond_type == RUNSTACK_COND_LOOP) {
            return condcb;
        }
    }

    return NULL;
}  /* get_first_loopcb */


/********************************************************************
* FUNCTION free_condcb
* 
* Clean and free a runstack cond. control block
*
* INPUTS:
*   condcb == conditional control block to clean and free
*
*********************************************************************/
static void
    free_condcb (runstack_condcb_t *condcb)
{
    runstack_line_t    *le;
    runstack_loopcb_t  *loopcb;

    switch (condcb->cond_type) {
    case RUNSTACK_COND_NONE:
        SET_ERROR(ERR_INTERNAL_PTR);
        break;
    case RUNSTACK_COND_IF:
        break;
    case RUNSTACK_COND_LOOP:
        loopcb = &condcb->u.loopcb;
        if (loopcb->xpathpcb != NULL) {
            xpath_free_pcb(loopcb->xpathpcb);
        }
        if (loopcb->docroot != NULL) {
            val_free_value(loopcb->docroot);
        }
        while (!dlq_empty(&loopcb->lineQ)) {
            le = (runstack_line_t *)dlq_deque(&loopcb->lineQ);
            free_line_entry(le);
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_PTR);
    }
    m__free(condcb);

}  /* free_condcb */


/********************************************************************
* FUNCTION new_condcb
* 
* Malloc and init a new runstack cond control block
*
* INPUTS:
*   cond_type == condition control block type
*
* RETURNS:
*   pointer to new runstack if control block entry, NULL if an error
*********************************************************************/
static runstack_condcb_t *
    new_condcb (runstack_condtype_t cond_type)
{
    runstack_condcb_t *condcb;

    condcb = m__getObj(runstack_condcb_t);
    if (condcb == NULL) {
        return NULL;
    }

    memset(condcb, 0x0, sizeof(runstack_condcb_t));
    condcb->cond_type = cond_type;
    if (cond_type == RUNSTACK_COND_LOOP) {
        dlq_createSQue(&condcb->u.loopcb.lineQ);
    }
    return condcb;

}  /* new_condcb */


/********************************************************************
* FUNCTION free_stack_entry
* 
* Clean and free a runstack entry
*
* INPUTS:
*   se == stack entry to free
*
* INPUTS:
*    se == stack entry to free
*********************************************************************/
static void
    free_stack_entry (runstack_entry_t *se)
{
    ncx_var_t          *var;
    runstack_condcb_t  *condcb;

    if (se->buff) {
        m__free(se->buff);
    }
    if (se->fp) {
        fclose(se->fp);
    }
    if (se->source) {
        m__free(se->source);
    }
    while (!dlq_empty(&se->parmQ)) {
        var = (ncx_var_t *)dlq_deque(&se->parmQ);
        var_free(var);
    }
    while (!dlq_empty(&se->varQ)) {
        var = (ncx_var_t *)dlq_deque(&se->varQ);
        var_free(var);
    }
    while (!dlq_empty(&se->condcbQ)) {
        condcb = (runstack_condcb_t *)dlq_deque(&se->condcbQ);
        free_condcb(condcb);
    }
    m__free(se);

}  /* free_stack_entry */


/********************************************************************
* FUNCTION new_stack_entry
* 
* Malloc and init a new runstack entry
*
* INPUTS:
*   source == file source
*
* RETURNS:
*   pointer to new val struct, NULL if an error
*********************************************************************/
static runstack_entry_t *
    new_stack_entry (const xmlChar *source)
{
    runstack_entry_t *se;

    se = m__getObj(runstack_entry_t);
    if (se == NULL) {
        return NULL;
    }

    memset(se, 0x0, sizeof(runstack_entry_t));

    dlq_createSQue(&se->parmQ);
    dlq_createSQue(&se->varQ);
    dlq_createSQue(&se->condcbQ);

    /* get a new line input buffer */
    se->buff = m__getMem(RUNSTACK_BUFFLEN);
    if (!se->buff) {
        free_stack_entry(se);
        return NULL;
    }

    /* set the script source */
    se->source = xml_strdup(source);
    if (!se->source) {
        free_stack_entry(se);
        return NULL;
    }

    return se;

}  /* new_stack_entry */

/********************************************************************
 * Utility function for removing the last and deleting and 
 * entry from a queue.
 *
 * \param rcxt the runstack context
 * \param condcd the runstack control sequence
 * \return NO_ERR;
 *
 ********************************************************************/
static status_t free_condcb_end( runstack_context_t* rcxt,
                                 runstack_condcb_t* condcb,
                                 dlq_hdr_t* useQ )
{
    /* this 'end' completes this queue, remove it from the stack and delete it*/
    dlq_remove(condcb);
    free_condcb(condcb);

    /* figure out the new conditional state */
    condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if (condcb == NULL) {
        /* no conditionals at all in this frame now */
        rcxt->cond_state = TRUE;
    } else if (condcb->cond_type == RUNSTACK_COND_IF) {
        rcxt->cond_state = condcb->u.ifcb.curcond;
    } else {
        rcxt->cond_state = condcb->u.loopcb.startcond;
    }

    return NO_ERR;
}

/********************************************************************
 * Utility function for selecting the line end entry.
 *
 * \param rcxt the runstack context
 * \param loopcb the loop cb entry
 * \param useQ the useQ (thius will be updated)
 * \return the line entry to use.
 *
 ********************************************************************/
static runstack_line_t* select_end_entry( runstack_context_t* rcxt,
                                          runstack_loopcb_t* loopcb,
                                          dlq_hdr_t** useQ )
{            
    /* get the 'end' entry in the lineq */
    if (loopcb->collector) {
        if (rcxt->cur_src == RUNSTACK_SRC_LOOP) {
            /* looping in the outside loop and collecting in the inside loop
            * the 'end' command may not be last */
            return loopcb->collector->cur_line;
        } else {
            /* collecting so the 'end' command is last */
            *useQ = &loopcb->collector->lineQ;
            return (runstack_line_t *)dlq_lastEntry(*useQ);
        }
    } else {
            /* only loop collecting so the 'end' command is last */
            *useQ = &loopcb->lineQ;
            return(runstack_line_t *)dlq_lastEntry(*useQ);
    }
}

/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION runstack_level
* 
*  Get the current stack level

* INPUTS:
*    rcxt == runstack context to use
*
* RETURNS:
*   current stack level; 0 --> not in any script
*********************************************************************/
extern uint32
    runstack_level (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }
    return rcxt->script_level;

}  /* runstack_level */


/********************************************************************
* FUNCTION runstack_push
* 
*  Add a script nest level context to the stack
*  Call just after the file is opened and before
*  the first line has been read
* 
*  Still need to add the script parameters (if any)
*  with the runstack_setparm
*
* INPUTS:
*   rcxt == runstack context to use
*   source == file source
*   fp == file pointer
*
* RETURNS:
*   status
*********************************************************************/
status_t
    runstack_push (runstack_context_t *rcxt,
                   const xmlChar *source,
                   FILE *fp)
{
    runstack_entry_t  *se;
    val_value_t       *val;
    status_t           res;

#ifdef DEBUG
    if (source == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    /* check if this would overflow the runstack */
    if (rcxt->script_level+1 == rcxt->max_script_level) {
        return ERR_NCX_RESOURCE_DENIED;
    }

    se = new_stack_entry(source);
    if (se == NULL) {
        return ERR_INTERNAL_MEM;
    }

    se->fp = fp;
    se->bufflen = RUNSTACK_BUFFLEN;

    /* create the P0 parameter */
    val = make_parmval((const xmlChar *)"0", source);
    if (!val) {
        free_stack_entry(se);
        return ERR_INTERNAL_MEM;
    }

    /* need to increment script level now so var_set_move
     * will work, and the correct script parameter queue
     * will be used for the P0 parameter
     */
    dlq_enque(se, &rcxt->runstackQ);
    rcxt->script_level++;
    rcxt->cur_src = RUNSTACK_SRC_SCRIPT;

    /* create a new var entry and add it to the runstack que
     * pass off 'val' memory here
     */
    res = var_set_move(rcxt,
                       (const xmlChar *)"0", 
                       1, 
                       VAR_TYP_LOCAL, 
                       val);
    if (res != NO_ERR) {
        dlq_remove(se);
        free_stack_entry(se);
        rcxt->script_level--;
        return res;
    }

    if (LOGDEBUG) {
        log_debug("\nrunstack: Starting level %u script %s",
                  rcxt->script_level, 
                  source);
    }

    return NO_ERR;

}  /* runstack_push */


/********************************************************************
* FUNCTION runstack_pop
* 
*  Remove a script nest level context from the stack
*  Call just after script is completed
* 
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
void
    runstack_pop (runstack_context_t *rcxt)
{
    runstack_entry_t  *se;
    runstack_condcb_t *condcb;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->script_level == 0) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    if (se == NULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    dlq_remove(se);

    if (se->source && LOGDEBUG) {
        log_debug("\nrunstack: Ending level %u script %s",
                  rcxt->script_level, 
                  se->source);
    }
    
    free_stack_entry(se);

    rcxt->script_level--;

    /* reset the current input source */
    condcb = get_loopcb(rcxt);
    if (condcb == NULL) {
        rcxt->cur_src = (rcxt->script_level) 
            ? RUNSTACK_SRC_SCRIPT : RUNSTACK_SRC_USER;
    } else if (condcb->u.loopcb.loop_state == RUNSTACK_LOOP_COLLECTING) {
        rcxt->cur_src = (rcxt->script_level) 
            ? RUNSTACK_SRC_SCRIPT : RUNSTACK_SRC_USER;
    } else {
        rcxt->cur_src = RUNSTACK_SRC_LOOP;
    }
    
}  /* runstack_pop */


/********************************************************************
* FUNCTION runstack_get_cmd
* 
*  Read the current runstack context and construct
*  a command string for processing by do_run_script.
*    - Comment lines will be skipped.
*    - Extended lines will be concatenated in the
*      buffer.  If a buffer overflow occurs due to this
*      concatenation, an error will be returned
* 
* INPUTS:
*   rcxt == runstack context to use
*   res == address of status result
*
* OUTPUTS:
*   *res == function result status
*
* RETURNS:
*   pointer to the command line to process (should treat as CONST !!!)
*   NULL if some error
*********************************************************************/
xmlChar *
    runstack_get_cmd (runstack_context_t *rcxt,
                      status_t *res)
{
    runstack_entry_t  *se;
    xmlChar           *retstr, *start, *str;
    boolean            done;
    int                len, total, errint;

#ifdef DEBUG
    if (res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->script_level == 0) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    /* init locals */
    se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    if (se == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    retstr = NULL;  /* start of return string */
    start = se->buff;
    total = 0;
    done = FALSE;

    if (rcxt->script_cancel) {
        if (LOGINFO) {
            log_info("\nScript '%s' canceled", se->source);
        }
        done = TRUE;
        *res = ERR_NCX_CANCELED;
        runstack_clear_cancel(rcxt);
    }

    /* get a command line, handling comment and continuation lines */
    while (!done) {

        /* check overflow error */
        if (total==se->bufflen) {
            *res = ERR_BUFF_OVFL;
            /* do not allow truncated command to execute */
            retstr = NULL;
            done = TRUE;
            continue;
        }

        /* read the next line from the file */
        if (!fgets((char *)start, se->bufflen-total, se->fp)) {
            /* check if the file ended and that is why
             * no line was retrieved
             */
            errint = feof(se->fp);

            if (retstr) {
                log_warn("\nWarning: script possibly truncated."
                         "\n   Ended on a continuation line");
                *res = NO_ERR;
            } else if (errint) {
                *res = ERR_NCX_EOF;
            } else {
                *res = ERR_NCX_READ_FAILED;
            }
            done = TRUE;
            continue;
        }

        se->linenum++;  /* used for error messages */

        len = (int)xml_strlen(start);
        
        /* get rid of EOLN if present */
        if (len && start[len-1]=='\n') {
            start[--len] = 0;
        }

        /* check blank line */
        str = start;
        while (*str && xml_isspace(*str)) {
            str++;
        }
        if (!*str) {
            if (retstr) {
                /* return the string we have so far */
                *res = NO_ERR;
                done = TRUE;
            } else {
                /* try again */
                continue;
            }
        }
                
        /* check first line or line continuation in progress */
        if (retstr == NULL) {
            /* retstr not set yet, allowed to have a comment line here */
            if (*str == '#') {
                /* got a comment, try for another line */
                *start = 0;
                continue;
            } else {
                /* start the return string and keep going */
                str = start;
                retstr = start;
            }
        }

        /* check line continuation */

        if (len && start[len-1]=='\\') {
            /* get rid of the final backslash */
            total += len-1;
            start[len-1] = 0;
            start += len-1;
            /* get another line full */
        } else {
            *res = NO_ERR;
            done = TRUE;
        }
    }

    if (retstr == NULL) {
        runstack_pop(rcxt);
    } else if (LOGDEBUG) {
        log_debug("\nrunstack: run line %u, %s\n cmd: %s",
                  se->linenum, 
                  se->source, 
                  retstr);
    }

    return retstr;

}  /* runstack_get_cmd */


/********************************************************************
* FUNCTION runstack_cancel
* 
*  Cancel all running scripts
*
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
void
    runstack_cancel (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }
    if (rcxt->script_level) {
        rcxt->script_cancel = TRUE;
    }
}  /* runstack_cancel */


/********************************************************************
* FUNCTION runstack_clear_cancel
* 
*  Clear the cancel flags 
*
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
void
    runstack_clear_cancel (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->script_level == 0 && rcxt->script_cancel) {
        rcxt->cur_src = RUNSTACK_SRC_USER;
        rcxt->script_cancel = FALSE;
    }

}  /* runstack_clear_cancel */


/********************************************************************
* FUNCTION runstack_get_que
* 
*  Read the current runstack context and figure
*  out which queue to get
*
* INPUTS:
*   rcxt == runstack context to use
*   isglobal == TRUE if global queue desired
*            == FALSE if the runstack var que is desired
*
* RETURNS:
*   pointer to the requested que
*********************************************************************/
dlq_hdr_t *
    runstack_get_que (runstack_context_t *rcxt,
                      boolean isglobal)
{
    runstack_entry_t  *se;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    /* check global que */
    if (isglobal) {
        return &rcxt->globalQ;
    }

    /* check level zero local que */
    if (rcxt->script_level == 0) {
        return &rcxt->zeroQ;
    }

    /* get the current slot */
    se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    if (se == NULL) {
        return NULL;
    }
    return &se->varQ;

}  /* runstack_get_que */


/********************************************************************
* FUNCTION runstack_get_parm_que
* 
*  Get the parameter queue for the current stack level
*
* INPUTS:
*     rcxt == runstack context to use
*
* RETURNS:
*   que for current stack level, NULL if an error
*********************************************************************/
dlq_hdr_t *
    runstack_get_parm_que (runstack_context_t *rcxt)
{
    runstack_entry_t  *se;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->script_level == 0) {
        return NULL;
    }

    /* get the current slot */
    se = (runstack_entry_t *)dlq_lastEntry(&rcxt->runstackQ);
    if (se == NULL) {
        return NULL;
    }

    return &se->parmQ;

}  /* runstack_get_parm_que */


/********************************************************************
* FUNCTION runstack_init
* 
*  Must Init this module before using it!!!
*
*********************************************************************/
void
    runstack_init (void)
{
    if (!runstack_init_done) {
        runstack_init_context(&defcxt);
        runstack_init_done = TRUE;
    }
} /* runstack_init */                 


/********************************************************************
* FUNCTION runstack_cleanup
* 
*  Must cleanup this module after using it!!!
*
*********************************************************************/
void
    runstack_cleanup (void)
{
    if (runstack_init_done) {
        runstack_clean_context(&defcxt);
        runstack_init_done = FALSE;
    }
} /* runstack_cleanup */                      


/********************************************************************
* FUNCTION runstack_clean_context
* 
*  INPUTS:
*     rcxt == runstack context to clean, but not free
*
*********************************************************************/
void
    runstack_clean_context (runstack_context_t *rcxt)
{
    ncx_var_t          *var;
    runstack_condcb_t  *condcb;

#ifdef DEBUG
    if (rcxt == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (rcxt->script_level > 0) {
        runstack_pop(rcxt);
    }

    while (!dlq_empty(&rcxt->globalQ)) {
        var = (ncx_var_t *)dlq_deque(&rcxt->globalQ);
        var_free(var);
    }

    while (!dlq_empty(&rcxt->zeroQ)) {
        var = (ncx_var_t *)dlq_deque(&rcxt->zeroQ);
        var_free(var);
    }

    while (!dlq_empty(&rcxt->zero_condcbQ)) {
        condcb= (runstack_condcb_t *)dlq_deque(&rcxt->zero_condcbQ);
        free_condcb(condcb);
    }

} /* runstack_clean_context */ 


/********************************************************************
* FUNCTION runstack_free_context
* 
*  INPUTS:
*     rcxt == runstack context to free
*
*********************************************************************/
void
    runstack_free_context (runstack_context_t *rcxt)
{
#ifdef DEBUG
    if (rcxt == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    runstack_clean_context(rcxt);
    m__free(rcxt);

} /* runstack_free_context */ 


/********************************************************************
* FUNCTION runstack_init_context
* 
* Initialize a pre-malloced runstack context
*
*  INPUTS:
*     rcxt == runstack context to free
*
*********************************************************************/
void
    runstack_init_context (runstack_context_t *rcxt)
{
#ifdef DEBUG
    if (rcxt == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(rcxt, 0x0, sizeof(runstack_context_t));
    dlq_createSQue(&rcxt->globalQ);
    dlq_createSQue(&rcxt->zeroQ);
    dlq_createSQue(&rcxt->runstackQ);
    dlq_createSQue(&rcxt->zero_condcbQ);
    rcxt->max_script_level = RUNSTACK_MAX_NEST;
    rcxt->cond_state = TRUE;
    rcxt->cur_src = RUNSTACK_SRC_USER;

} /* runstack_init_context */ 


/********************************************************************
* FUNCTION runstack_new_context
* 
*  Malloc a new runstack context
*
*  RETURNS:
*     malloced and initialized runstack context
*
*********************************************************************/
runstack_context_t *
    runstack_new_context (void)
{
    runstack_context_t *rcxt;

    rcxt = m__getObj(runstack_context_t);
    if (rcxt == NULL) {
        return NULL;
    }
    runstack_init_context(rcxt);
    return rcxt;

} /* runstack_new_context */ 


/********************************************************************
* FUNCTION runstack_session_cleanup
* 
* Cleanup after a yangcli session has ended
*
*  INPUTS:
*     rcxt == runstack context to use
*
*********************************************************************/
void
    runstack_session_cleanup (runstack_context_t *rcxt)
{
    runstack_entry_t   *se;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    var_cvt_generic(&rcxt->globalQ);
    var_cvt_generic(&rcxt->zeroQ);
    
    for (se = (runstack_entry_t *)dlq_firstEntry(&rcxt->runstackQ);
         se != NULL;
         se = (runstack_entry_t *)dlq_nextEntry(se)) {
        var_cvt_generic(&se->varQ);
    }

} /* runstack_session_cleanup */


/********************************************************************
* FUNCTION runstack_get_source
* 
* Determine which input source is active for
* the specified runstack context
*
*  INPUTS:
*     rcxt == runstack context to use
*
*********************************************************************/
runstack_src_t
    runstack_get_source (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    return rcxt->cur_src;

} /* runstack_get_source */


/********************************************************************
* FUNCTION runstack_save_line
* 
* Save the current line if needed if a loop is active
*
* INPUTS:
*    rcxt == runstack context to use
*    line == line to save, a copy will be made
*
* RETURNS:
*   status
*********************************************************************/
status_t
    runstack_save_line (runstack_context_t *rcxt,
                        const xmlChar *line)
{
    runstack_condcb_t  *condcb;
    runstack_loopcb_t  *loopcb;
    runstack_line_t    *le;

#ifdef DEBUG
    if (line == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->cur_src == RUNSTACK_SRC_LOOP) {
        return NO_ERR;
    }

    condcb = get_loopcb(rcxt);
    if (condcb != NULL) {
        loopcb = &condcb->u.loopcb;
        if (loopcb->loop_state == RUNSTACK_LOOP_COLLECTING) {
            /* save this line */
            le = new_line_entry(line);
            if (le == NULL) {
                return ERR_INTERNAL_MEM;
            }
            if (loopcb->collector != NULL) {
                dlq_enque(le, &loopcb->collector->lineQ);
            } else {
                dlq_enque(le, &loopcb->lineQ);
            }

            /* adjust any loopcb first_line pointers that
             * need to be set
             */

            dlq_hdr_t *useQ = select_useQ( rcxt );
            for (condcb = (runstack_condcb_t *)dlq_firstEntry(useQ);

                 condcb != NULL;
                 condcb = (runstack_condcb_t *)dlq_nextEntry(condcb)) {

                if (condcb->cond_type != RUNSTACK_COND_LOOP) {
                    continue;
                }

                loopcb = &condcb->u.loopcb;
                if (!loopcb->empty_block && loopcb->first_line == NULL) {
                    /* the first line needs to be set */
                    loopcb->first_line = le;
                }
            }
        }
    }
    return NO_ERR;

} /* runstack_save_line */


/********************************************************************
* FUNCTION runstack_get_loop_cmd
* 
* Get the next command during loop processing.
* This function is called after the while loop end
* has been reached, and buffered commands are used
*
* INPUTS:
*   rcxt == runstack context to use
*   res == address of status result
*
* OUTPUTS:
*   *res == function result status
*        == ERR_NCX_LOOP_ENDED if the loop expr was just
*           evaluated to FALSE; In this case the caller must
*           use the runstack_get_source function to determine
*           the source of the next line.  If this occurs,
*           the loopcb (and while context) will be removed from
*           the curent runstack frame
*
* RETURNS:
*   pointer to the command line to process (should treat as CONST !!!)
*   NULL if some error
*********************************************************************/
xmlChar *
    runstack_get_loop_cmd (runstack_context_t *rcxt,
                           status_t *res)
{
    runstack_condcb_t  *condcb;
    runstack_loopcb_t  *loopcb, *test_loopcb, *first_loopcb;
    runstack_line_t    *le;
    xpath_result_t     *result;
    boolean             needremove, needeval, needmax, cond;

#ifdef DEBUG
    if (res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    first_loopcb = NULL;
    loopcb = NULL;
    le = NULL;
    result = NULL;
    needremove = FALSE;
    needeval = FALSE;
    needmax = FALSE;
    cond = FALSE;
    *res = NO_ERR;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    if (rcxt->script_cancel) {
        if (LOGINFO) {
            log_info("\nScript in loop canceled");
        }
        *res = ERR_NCX_CANCELED;
        if (rcxt->script_level == 0) {
            runstack_clear_cancel(rcxt);
        } else {
            runstack_pop(rcxt);
        }
        return NULL;
    }

    condcb = get_loopcb(rcxt);
    if (condcb == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    loopcb = &condcb->u.loopcb;

    if (loopcb->loop_state == RUNSTACK_LOOP_COLLECTING) {
        /* need to get a previous loopcb that is looping */
        first_loopcb = loopcb->collector;
        if (first_loopcb == NULL) {
            *res = SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
        test_loopcb = first_loopcb;
    } else {
        test_loopcb = loopcb;
    }

    if (loopcb->empty_block) {
        /* corner-case, no statements */
        needeval = TRUE;
        needmax = TRUE;
    } else if (test_loopcb->cur_line == NULL) {
        /* the next line is supposed to be the first line */
        le = test_loopcb->cur_line = test_loopcb->first_line;
        if (first_loopcb != NULL) {
            if (loopcb->first_line == NULL) {
                loopcb->first_line = le;
            }
        }
        /* the following test may compare cur_line to a 
         * NULL last_line if the collector is different
         * than the current loop. This is OK; the
         * outside loop is looping and only the inside
         * loop is collecting, so the last_line is not
         * set until runstack_handle_end sets it
         */
    } else if (test_loopcb->cur_line == loopcb->last_line) {
        /* the next line is supposed to be the first line 
         * set the inside loop cur line not the outside
         * loop cur line.  Since the last line is actually
         * set on the inside loop, the state is now LOOPING
         * so the inside loop cur_line pointer will be used
         * until the loopcb is deleted
         */
        le = loopcb->cur_line = loopcb->first_line;
        needeval = TRUE;
        needmax = TRUE;
    } else {
        /* in the list somewhere 1 .. N 
        * the outside loop is looping and the inside
        * loop is collecting; or there is only 1 loop
        * which is collecting or looping
        */
        le = test_loopcb->cur_line = 
            (runstack_line_t *)dlq_nextEntry(test_loopcb->cur_line);
        if (first_loopcb != NULL) {
            if (loopcb->first_line == NULL) {
                loopcb->first_line = le;
            }
        }            
    }

    if (*res == NO_ERR && needmax) {
        loopcb->loop_count++;
        if (loopcb->maxloops) {
            if (loopcb->loop_count == loopcb->maxloops) {
                log_debug("\nrunstack: max loop iterations ('%u') reached",
                          loopcb->maxloops);
                
                /* need to remove the loopcb and change the
                 * script source
                 */
                needremove = TRUE;
                needeval = FALSE;
            }
        }
    }

    if (*res == NO_ERR && needeval) {
        /* get current condition state
         * correct syntax will not allow the current context
         * to be inside an if-stmt already, so checking the
         * current if-stmt status should give the outside 
         * loop context
         */
        cond = runstack_get_cond_state(rcxt);

        if (cond) {
            /* try to parse the XPath expression */
            result = xpath1_eval_expr(loopcb->xpathpcb, 
                                      loopcb->docroot, /* context */
                                      loopcb->docroot,
                                      TRUE,
                                      FALSE,
                                      res);

            if (result != NULL && *res == NO_ERR) {
                /* get new condition state for this loop */
                cond = xpath_cvt_boolean(result);
                xpath_free_result(result);
            } else {
                /* error in the XPath expression */
                if (result != NULL) {
                    xpath_free_result(result);
                }
                return NULL;
            }
        }

        if (!cond) {
            needremove = TRUE;
        }
    }

    if (needremove) {
        /* save a pointer to the last line;
         * since this was not the first loopcb,
         * the last_line pointer will be valid
         * in the previous loopcb on the queue
         */
        le = loopcb->last_line;

        dlq_remove(condcb);
        free_condcb(condcb);
        *res = ERR_NCX_LOOP_ENDED;

        /* reset the runstack context data structures
         * check if there are any more active loops
         */
        condcb = get_loopcb(rcxt);
        if (condcb == NULL) {
            /* done with all looping */
            rcxt->cur_src = (rcxt->script_level) 
                ? RUNSTACK_SRC_SCRIPT : RUNSTACK_SRC_USER;
        } else {
            /* set the cur line pointer to the end of this
             * just deleted loop
             */
            loopcb = &condcb->u.loopcb;
            if (loopcb->loop_state == RUNSTACK_LOOP_LOOPING) {
                rcxt->cur_src = RUNSTACK_SRC_LOOP;
            } else {
                rcxt->cur_src = (rcxt->script_level) 
                    ? RUNSTACK_SRC_SCRIPT : RUNSTACK_SRC_USER;
            }
        }
        reset_cond_state(rcxt);
        le = NULL;
    }

    if (LOGDEBUG2) {
        if (le) {
            log_debug2("\nrunstack: loop cmd '%s'", le->line);
        } else {
            log_debug2("\nrunstack: loop cmd NULL");
        }
    }

    return (le) ? le->line : NULL;

}  /* runstack_get_loop_cmd */


/********************************************************************
* FUNCTION runstack_get_cond_state
* 
* Get the current conditional code state for the context
*
* INPUTS:
*   rcxt == runstack context to use
*
* RETURNS:
*   TRUE if in a TRUE conditional or no conditional
*   FALSE if in a FALSE conditional statement right now
*********************************************************************/
boolean
    runstack_get_cond_state (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }
    return rcxt->cond_state;

} /* runstack_get_cond_state */


/********************************************************************
* FUNCTION runstack_handle_while
* 
* Process the current command, which is a 'while' command.
* This must be called before the next command is retrieved
* during runstack context processing
*
* INPUTS:
*    rcxt == runstack context to use
*    maxloops == max number of loop iterations
*    xpathpcb == XPath control block to save which contains
*                the expression to be processed.
*                !!! this memory is handed off here; it will
*                !!!  be freed as part of the loopcb context
*    docroot == docroot var struct that represents
*               the XML document to run the XPath expr against
*                !!! this memory is handed off here; it will
*                !!!  be freed as part of the loopcb context
*
* RETURNS:
*    status; if status != NO_ERR then the xpathpcb and docroot
*    parameters need to be freed by the caller
*********************************************************************/
status_t
    runstack_handle_while (runstack_context_t *rcxt,
                           uint32 maxloops,
                           xpath_pcb_t *xpathpcb,
                           val_value_t *docroot)
{
    runstack_condcb_t    *condcb, *testloopcb;
    runstack_loopcb_t    *loopcb;
    xpath_result_t       *result;
    status_t              res;

    assert( xpathpcb && "xpathpcb == NULL" );
    assert( docroot && "docroot == NULL ");

    res = NO_ERR;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    condcb = new_condcb(RUNSTACK_COND_LOOP);
    if (condcb == NULL) {
        return ERR_INTERNAL_MEM;
    }
    loopcb = &condcb->u.loopcb;

    loopcb->loop_state = RUNSTACK_LOOP_COLLECTING;
    loopcb->maxloops = maxloops;

    if (rcxt->cond_state) {
        /* figure out if the first loop is enabled or not */
        result = xpath1_eval_expr(xpathpcb, 
                                  docroot, /* context */
                                  docroot,
                                  TRUE,
                                  FALSE,
                                  &res);

        if (result != NULL && res == NO_ERR) {
            /* get new condition state for this loop */
            rcxt->cond_state = loopcb->startcond = xpath_cvt_boolean(result);
        } 
        if (result != NULL) {
            xpath_free_result(result);
        }
        if (res != NO_ERR) {
            free_condcb(condcb);
            return res;
        }
    } else {
        /* do not bother evaluating the loop cond inside
         * a nested false conditional
         */
        loopcb->startcond = FALSE;
    }

    /* check if this is a nested loop or the first loop */
    testloopcb = get_first_loopcb(rcxt);
    if (testloopcb != NULL) {
        loopcb->collector = &testloopcb->u.loopcb;
    }

    dlq_enque(condcb, useQ);

    /* accept xpathpcb and docroot memory here */
    loopcb->xpathpcb = xpathpcb;
    loopcb->docroot = docroot;

    return NO_ERR;

}  /* runstack_handle_while */


/********************************************************************
* FUNCTION runstack_handle_if
* 
* Handle the if command for the specific runstack context
*
* INPUTS:
*   rcxt == runstack context to use
*   startcond == start condition state for this if block
*                may be FALSE because the current conditional
*                state is already FALSE
*
* RETURNS:
*   status
*********************************************************************/
status_t
    runstack_handle_if (runstack_context_t *rcxt,
                        boolean startcond)
{
    runstack_condcb_t    *condcb;
    runstack_ifcb_t      *ifcb;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    condcb = new_condcb(RUNSTACK_COND_IF);
    if (condcb == NULL) {
        return ERR_INTERNAL_MEM;
    }
    ifcb = &condcb->u.ifcb;

    ifcb->ifstate = RUNSTACK_IF_IF;
    ifcb->ifused = ifcb->startcond = startcond;

    dlq_enque(condcb, useQ);

    /* only allow TRUE --> TRUE/FALSE transition
     * because a FALSE conditional is always inherited
     */
    if (rcxt->cond_state) {
        rcxt->cond_state = startcond;
    }

    return NO_ERR;

} /* runstack_handle_if */


/********************************************************************
* FUNCTION runstack_handle_elif
* 
* Handle the elif command for the specific runstack context
*
* INPUTS:
*   rcxt == runstack context to use
*   startcond == start condition state for this elif block
*                may be FALSE because the current conditional
*                block has already used its TRUE block
*
* RETURNS:
*   status
*********************************************************************/
status_t
    runstack_handle_elif (runstack_context_t *rcxt,
                          boolean startcond)
{
    runstack_condcb_t    *condcb;
    runstack_ifcb_t      *ifcb;
    status_t              res;

    res = NO_ERR;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if (condcb == NULL) {
        log_error("\nError: unexpected 'elif' command");
        return ERR_NCX_INVALID_VALUE;
    }

    /* make sure this conditional block is an ifcb */
    if (condcb->cond_type != RUNSTACK_COND_IF) {
        log_error("\nError: unexpected 'elif' command");
        return ERR_NCX_INVALID_VALUE;
    }

    /* make sure the current state is IF or ELIF */
    switch (condcb->u.ifcb.ifstate) {
    case RUNSTACK_IF_NONE:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case RUNSTACK_IF_IF:
    case RUNSTACK_IF_ELIF:
        ifcb = &condcb->u.ifcb;

        /* this is the expected state, update to ELIF state */
        ifcb->ifstate = RUNSTACK_IF_ELIF;

        /* set the elif conditional state depending
         * on if any of the previous if or elif blocks
         * were true or not
         *
         * figure out the new conditional state
         */
        if (get_parent_cond_state(condcb)) {
            if (ifcb->ifused) {
                /* this has to be a false block */
                rcxt->cond_state = FALSE;
                ifcb->curcond = FALSE;
            } else {
                ifcb->ifused = startcond;
                rcxt->cond_state = startcond;
                ifcb->curcond = startcond;
            }
        }
        break;
    case RUNSTACK_IF_ELSE:
        log_error("\nError: unexpected 'elif'; previous 'else' command "
                  "already active");
        res = ERR_NCX_INVALID_VALUE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* runstack_handle_elif */


/********************************************************************
* FUNCTION runstack_handle_else
* 
* Handle the elsecommand for the specific runstack context
*
* INPUTS:
*   rcxt == runstack context to use
*
* RETURNS:
*   status
*********************************************************************/
status_t
    runstack_handle_else (runstack_context_t *rcxt)
{
    runstack_condcb_t    *condcb;
    status_t              res;

    res = NO_ERR;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if (condcb == NULL) {
        log_error("\nError: unexpected 'else' command");
        return ERR_NCX_INVALID_VALUE;
    }

    /* make sure this conditional block is an ifcb */
    if (condcb->cond_type != RUNSTACK_COND_IF) {
        log_error("\nError: unexpected 'else' command");
        return ERR_NCX_INVALID_VALUE;
    }

    /* make sure the current state is IF or ELIF */
    switch (condcb->u.ifcb.ifstate) {
    case RUNSTACK_IF_NONE:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case RUNSTACK_IF_IF:
    case RUNSTACK_IF_ELIF:
        /* this is the expected state, update to ELSE state */
        condcb->u.ifcb.ifstate = RUNSTACK_IF_ELSE;

        /* set the else conditional state depending
         * on if any of the previous if or elif blocks
         * were true or not
         *
         * figure out the new conditional state
         */
        rcxt->cond_state = condcb->u.ifcb.curcond = !condcb->u.ifcb.ifused;
        break;
    case RUNSTACK_IF_ELSE:
        log_error("\nError: unexpected 'else'; previous 'else' command "
                  "already active");
        res = ERR_NCX_INVALID_VALUE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* runstack_handle_else */


/********************************************************************
* FUNCTION runstack_handle_end
* 
* Handle the end command for the specific runstack context
*
* INPUTS:
*   rcxt == runstack context to use
*
* RETURNS:
*   status
*********************************************************************/
status_t runstack_handle_end (runstack_context_t *rcxt)
{
    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    runstack_condcb_t *condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if ( !condcb ) {
        log_error("\nError: unexpected 'end' command");
        return ERR_NCX_INVALID_VALUE;
    }

    if (condcb->cond_type == RUNSTACK_COND_IF) {
        log_debug2("\nrunstack: end for if command");
        return free_condcb_end( rcxt, condcb, useQ );
    } else {
        log_debug2("\nrunstack: end for while command");
        runstack_loopcb_t *loopcb = &condcb->u.loopcb;

        if ( ! loopcb || loopcb->loop_state != RUNSTACK_LOOP_COLLECTING) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        } 

        if ( !loopcb->startcond) {
            return free_condcb_end( rcxt, condcb, useQ );
        }
        
        runstack_line_t *le = select_end_entry( rcxt, loopcb, &useQ );
        if ( !le ) {
                return SET_ERROR(ERR_INTERNAL_VAL);
        } 

        /* check if the first line is this end line */
        if (loopcb->first_line == le) {
            loopcb->first_line = NULL;
        }

        /* get pointer for loopcb.last_line */
        if ( !loopcb->collector ) {
            /* outside loop so need to delete this 'end' line */
            dlq_remove(le);
            free_line_entry(le);

            /* get the new last while loop line */
            le = (runstack_line_t *)dlq_lastEntry(useQ);
        } else {
            /* get the next to last line */
            le = (runstack_line_t *)dlq_prevEntry(le);
        }

        /* set the last_line pointer for this loopcb */
        if ( !le ) {
            /* there are no lines in this while loop */
            loopcb->empty_block = TRUE;
            loopcb->first_line = NULL;
            loopcb->last_line = NULL;
        } else {
            loopcb->last_line = le;
        }

        /* need to go back to the start of the while loop and test the eval 
         * condition again */

        /* loopcb->cur_line = NULL; */
        loopcb->loop_state = RUNSTACK_LOOP_LOOPING;
        loopcb->loop_count = 1;
        rcxt->cur_src = RUNSTACK_SRC_LOOP;
    }

    return NO_ERR;
} /* runstack_handle_end */


/********************************************************************
* FUNCTION runstack_get_if_used
* 
* Check if the run context, which should be inside an if-stmt
* now, has used the 'true' block already
*
* INPUTS:
*   rcxt == runstack context to use
*
* RETURNS:
*   TRUE if TRUE block already used
*   FALSE if FALSE not already used or not in an if-block
*********************************************************************/
boolean
    runstack_get_if_used (runstack_context_t *rcxt)
{
    runstack_condcb_t *condcb;

    if (rcxt == NULL) {
        rcxt = &defcxt;
    }

    dlq_hdr_t *useQ = select_useQ( rcxt );
    condcb = (runstack_condcb_t *)dlq_lastEntry(useQ);
    if (condcb == NULL) {
        /* error -- not in any conditional block */
        return FALSE;
    } else if (condcb->cond_type == RUNSTACK_COND_IF) {
        return condcb->u.ifcb.ifused;
    } else {
        /* error -- in a while loop */
        return FALSE;
    }

} /* runstack_get_if_used */


/* END runstack.c */
