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
#ifndef _H_runstack
#define _H_runstack

/*  FILE: runstack.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************


 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
22-aug-07    abb      Begun; split from ncxcli.c
24-apr-10    abb      Redesign to use dynamic memory,
                      allow recursion, and support multiple 
                      concurrent contexts
26-apr-10    abb      added conditional cmd and loop support
*/

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* this is an arbitrary limit, but it must match the
 * yangcli:run rpc P1 - Pn variables, currently set to 9
 * $1 to $9 parameters passed by yangcli to next script
 */
#define RUNSTACK_MAX_PARMS  9


/* this is an arbitrary max to limit resources
 * and run-away scripts that are called recursively
 */
#define RUNSTACK_MAX_NEST   512

/* this is an arbitrary max to limit 
 * run-away scripts that have bugs
 */
#define RUNSTACK_MAX_LOOP   65535


/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* identify the runstack input source */
typedef enum runstack_src_t_ {
    RUNSTACK_SRC_NONE,
    RUNSTACK_SRC_USER,
    RUNSTACK_SRC_SCRIPT,
    RUNSTACK_SRC_LOOP
} runstack_src_t;


/* identify the runstack conditional control block type */
typedef enum runstack_condtype_t_ {
    RUNSTACK_COND_NONE,
    RUNSTACK_COND_IF,
    RUNSTACK_COND_LOOP
} runstack_condtype_t;


/* keep track of the if,elif,else,end sequence */
typedef enum runstack_ifstate_t_ {
    RUNSTACK_IF_NONE,
    RUNSTACK_IF_IF,
    RUNSTACK_IF_ELIF,
    RUNSTACK_IF_ELSE
} runstack_ifstate_t;


/* keep track of the while,end sequence */
typedef enum runstack_loopstate_t_ {
    RUNSTACK_LOOP_NONE,
    RUNSTACK_LOOP_COLLECTING,
    RUNSTACK_LOOP_LOOPING
} runstack_loopstate_t;
    

/* control the conditional state for 1 if...end sequence */
typedef struct runstack_ifcb_t_ {
    runstack_ifstate_t    ifstate;
    boolean               startcond;
    boolean               curcond;
    boolean               ifused;  /* T already used up */
} runstack_ifcb_t;


/* save 1 line for looping purposes */
typedef struct runstack_line_t_ {
    dlq_hdr_t             qhdr;
    xmlChar              *line;
} runstack_line_t;


/* control the looping for 1 while - end sequence */
typedef struct runstack_loopcb_t_ {
    uint32                     maxloops;
    uint32                     loop_count;
    runstack_loopstate_t       loop_state;
    boolean                    startcond;

    /* condition to evaluate before each loop iteration */
    struct xpath_pcb_t_       *xpathpcb;
    val_value_t               *docroot;

    /* if the collector is non-NULL, then save lines
     * in this outer while loop
     */
    struct runstack_loopcb_t_ *collector;

    /* these may point to entries in this lineQ or
     * the collector's lineQ; only the commands
     * that are part of the loop contents are saved
     */
    runstack_line_t           *first_line;
    runstack_line_t           *cur_line;
    runstack_line_t           *last_line;
    boolean                    empty_block;

    /* this Q will only be used if collector is NULL */
    dlq_hdr_t                  lineQ;   /* Q of runstack_line_t */
} runstack_loopcb_t;


/* control the looping for 1 while - end sequence */
typedef struct runstack_condcb_t_ {
    dlq_hdr_t                  qhdr;
    runstack_condtype_t        cond_type;
    union u_ {
        runstack_loopcb_t      loopcb;
        runstack_ifcb_t        ifcb;
    } u;
} runstack_condcb_t;


/* one script run level context entry
 * each time a 'run script' command is
 * encountered, a new stack context is created,
 * unless max_script_level is reached
 */
typedef struct runstack_entry_t_ {
    dlq_hdr_t    qhdr;
    uint32       level;
    FILE        *fp;
    xmlChar     *source;
    xmlChar     *buff;
    int          bufflen;
    uint32       linenum;
    dlq_hdr_t    parmQ;                /* Q of ncx_var_t */
    dlq_hdr_t    varQ;                 /* Q of ncx_var_t */
    dlq_hdr_t    condcbQ;        /* Q of runstack_condcb_t */
} runstack_entry_t;


typedef struct runstack_context_t_ {
    boolean            cond_state;
    boolean            script_cancel;
    uint32             script_level;
    uint32             max_script_level;
    runstack_src_t     cur_src;
    dlq_hdr_t          runstackQ;    /* Q of runstack_entry_t */
    dlq_hdr_t          globalQ;             /* Q of ncx_var_t */
    dlq_hdr_t          zeroQ;               /* Q of ncx_var_t */
    dlq_hdr_t          zero_condcbQ;  /* Q of runstack_condcb_t */
} runstack_context_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION runstack_level
* 
*  Get the current stack level
*
* INPUTS:
*    rcxt == runstack context to use
*
* RETURNS:
*   current stack level; 0 --> not in any script
*********************************************************************/
extern uint32
    runstack_level (runstack_context_t *rcxt);


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
extern status_t
    runstack_push (runstack_context_t *rcxt,
                   const xmlChar *source,
		   FILE *fp);

/********************************************************************
* FUNCTION runstack_pop
* 
*  Remove a script nest level context from the stack
*  Call just after script is completed
* 
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
extern void
    runstack_pop (runstack_context_t *rcxt);


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
extern xmlChar *
    runstack_get_cmd (runstack_context_t *rcxt,
                      status_t *res);


/********************************************************************
* FUNCTION runstack_cancel
* 
*  Cancel all running scripts
*
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
extern void
    runstack_cancel (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_clear_cancel
* 
*  Clear the cancel flags 
*
* INPUTS:
*     rcxt == runstack context to use
*********************************************************************/
extern void
    runstack_clear_cancel (runstack_context_t *rcxt);


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
extern dlq_hdr_t *
    runstack_get_que (runstack_context_t *rcxt,
                      boolean isglobal);


/********************************************************************
* FUNCTION runstack_get_parm_que
* 
*  Get the parameter queue for the current stack level
*
* INPUTS:
*   rcxt == runstack context to use
*
* RETURNS:
*   que for current stack level, NULL if an error
*********************************************************************/
extern dlq_hdr_t *
    runstack_get_parm_que (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_init
* 
*  Must Init this module before using it!!!
*
*********************************************************************/
extern void
    runstack_init (void);


/********************************************************************
* FUNCTION runstack_cleanup
* 
*  Must cleanup this module after using it!!!
*
*********************************************************************/
extern void
    runstack_cleanup (void);


/********************************************************************
* FUNCTION runstack_clean_context
* 
*  INPUTS:
*     rcxt == runstack context to clean, but not free
*
*********************************************************************/
extern void
    runstack_clean_context (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_free_context
* 
*  INPUTS:
*     rcxt == runstack context to free
*
*********************************************************************/
extern void
    runstack_free_context (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_init_context
* 
* Initialize a pre-malloced runstack context
*
*  INPUTS:
*     rcxt == runstack context to free
*
*********************************************************************/
extern void
    runstack_init_context (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_new_context
* 
*  Malloc a new runstack context
*
*  RETURNS:
*     malloced and initialized runstack context
*
*********************************************************************/
extern runstack_context_t *
    runstack_new_context (void);


/********************************************************************
* FUNCTION runstack_session_cleanup
* 
* Cleanup after a yangcli session has ended
*
*  INPUTS:
*     rcxt == runstack context to use
*
*********************************************************************/
extern void
    runstack_session_cleanup (runstack_context_t *rcxt);


/********************************************************************
* FUNCTION runstack_get_source
* 
* Get the current input source for the runstack context
*
*  INPUTS:
*     rcxt == runstack context to use
*
*********************************************************************/
extern runstack_src_t
    runstack_get_source (runstack_context_t *rcxt);


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
extern status_t
    runstack_save_line (runstack_context_t *rcxt,
                        const xmlChar *line);


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
extern xmlChar *
    runstack_get_loop_cmd (runstack_context_t *rcxt,
                           status_t *res);


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
extern boolean
    runstack_get_cond_state (runstack_context_t *rcxt);


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
extern status_t
    runstack_handle_while (runstack_context_t *rcxt,
                           uint32 maxloops,
                           struct xpath_pcb_t_ *xpathpcb,
                           val_value_t *docroot);


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
extern status_t
    runstack_handle_if (runstack_context_t *rcxt,
                        boolean startcond);


/********************************************************************
* FUNCTION runstack_handle_elif
* 
* Handle the elif command for the specific runstack context
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
extern status_t
    runstack_handle_elif (runstack_context_t *rcxt,
                          boolean startcond);


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
extern status_t
    runstack_handle_else (runstack_context_t *rcxt);


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
extern status_t
    runstack_handle_end (runstack_context_t *rcxt);


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
extern boolean
    runstack_get_if_used (runstack_context_t *rcxt);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_runstack */
