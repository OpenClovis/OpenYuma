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
#ifndef _H_cli
#define _H_cli

/*  FILE: cli.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    command line interpreter parsing to internal val_value_t format

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
21-oct-05    abb      Begun
09-feb-06    abb      Change from xmlTextReader to rpc_agt callback
                      API format
10-feb-07    abb      Split out common functions from agt_ps_parse.h
29-jul-08    abb      change to cli.h; remove all by CLI parsing;
                      conversion from NCX parmset to YANG object
07-feb-09    abb      Add cli_rawparm_t and cli_parse_raw
                      for bootstrap CLI support

*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_runstack
#include "runstack.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* value only of full test values for the valonly parameter */
#define  VALONLY   TRUE
#define  FULLTEST  FALSE


/* plain or script values for the climode parameter */
#define  SCRIPTMODE    TRUE
#define  PLAINMODE     FALSE

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* CLI parsing modes */
typedef enum cli_mode_t_ {
    CLI_MODE_NONE,
    CLI_MODE_PROGRAM,    /* real argc, argv */
    CLI_MODE_COMMAND     /* called from yangcli command parser */
} cli_mode_t;



/* used for bootstrap CLI parms only, no validation */
typedef struct cli_rawparm_t_ {
    dlq_hdr_t   qhdr;
    const char *name;
    char       *value;
    boolean     hasvalue;
    int32       count;
} cli_rawparm_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION cli_new_rawparm
* 
* bootstrap CLI support
* Malloc and init a raw parm entry
*
* INPUTS:
*   name == name of parm (static const string)
*
* RETURNS:
*   new parm entry, NULL if malloc failed
*********************************************************************/
extern cli_rawparm_t *
    cli_new_rawparm (const xmlChar *name);


/********************************************************************
* FUNCTION cli_new_empty_rawparm
* 
* Malloc and init a raw parm entry that has no value (NCX_BT_EMPTY)
*
* INPUTS:
*   name == name of parm (static const string)
*
* RETURNS:
*   new parm entry, NULL if malloc failed
*********************************************************************/
extern cli_rawparm_t *
    cli_new_empty_rawparm (const xmlChar *name);


/********************************************************************
* FUNCTION cli_free_rawparm
* 
* Clean and free a raw parm entry
*
* INPUTS:
*   parm == raw parm entry to free
*********************************************************************/
extern void
    cli_free_rawparm (cli_rawparm_t *parm);


/********************************************************************
* FUNCTION cli_clean_rawparmQ
* 
* Clean and free a Q of raw parm entries
*
* INPUTS:
*   parmQ == Q of raw parm entry to free
*********************************************************************/
extern void
    cli_clean_rawparmQ (dlq_hdr_t *parmQ);


/********************************************************************
* FUNCTION cli_find_rawparm
* 
* Find the specified raw parm entry
*
* INPUTS:
*   name == object name to really use
*   parmQ == Q of cli_rawparm_t 
*
* RETURNS:
*   raw parm entry if found, NULL if not
*********************************************************************/
extern cli_rawparm_t *
    cli_find_rawparm (const xmlChar *name,
		      dlq_hdr_t *parmQ);


/********************************************************************
* FUNCTION cli_parse_raw
* 
* Generate N sets of variable/value pairs for the
* specified boot-strap CLI parameters
* 
* There are no modules loaded yet, and nothing
* has been initialized, not even logging
* This function is almost the first thing done
* by the application
*
* CLI Syntax Supported
*
* [prefix] parmname
*
* [prefix] parmname=value
*
*    prefix ==  0 to 2 dashes    foo  -foo --foo
*    parmname == any valid NCX identifier string
*    value == string
*
* No spaces are allowed after 'parmname' if 'value' is entered
* Each parameter is allowed to occur zero or one times.
* If multiple instances of a parameter are entered, then
* the last one entered will win.
* The 'autocomp' parameter is set to TRUE
*
* The 'value' string cannot be split across multiple argv segments.
* Use quotation chars within the CLI shell to pass a string
* containing whitespace to the CLI parser:
*
*   --foo="quoted string if whitespace needed"
*   --foo="quoted string if setting a variable \
*         as a top-level assignment"
*   --foo=unquoted-string-without-whitespace
*
*  - There are no 1-char aliases for CLI parameters.
*  - Position-dependent, unnamed parameters are not supported
*
* INPUTS:
*   argc == number of strings passed in 'argv'
*   argv == array of command line argument strings
*   parmQ == Q of cli_rawparm_t entries
*            that should be used to validate the CLI input
*            and store the results
*
* OUTPUTS:
*    *rawparm (within parmQ):
*       - 'value' will be recorded if it is present
*       - count will be set to the number of times this 
*         parameter was entered (set even if no value)
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    cli_parse_raw (int argc, 
		   char *argv[],
		   dlq_hdr_t *rawparmQ);


/********************************************************************
* FUNCTION cli_parse
* 
* schema based CLI support
* Generate 1 val_value_t struct from a Unix Command Line,
* which should conform to the specified obj_template_t definition.
*
* For CLI interfaces, only one container object can be specified
* at this time.
*
* CLI Syntax Supported
*
* [prefix] parmname [separator] [value]
*
*    prefix ==  0 to 2 dashes    foo  -foo --foo
*    parmname == any valid NCX identifier string
*    separator ==  - equals char (=)
*                  - whitespace (sp, ht)
*    value == any NCX number or NCX string
*          == 'enum' data type
*          == not present (only for 'flag' data type)
*          == extended
*    This value: string will converted to appropriate 
*    simple type in val_value_t format.
*
* The format "--foo=bar" must be processed within one argv segment.
* If the separator is a whitespace char, then the next string
* in the argv array must contain the expected value.
* Whitespace can be preserved using single or double quotes
*
* DESIGN NOTES:
*
*   1) parse the (argc, argv) input against the specified object
*   2) add any missing mandatory and condition-met parameters
*      to the container.
*   3) Add defaults if valonly is false
*   4) Validate any 'choice' constructs within the parmset
*   5) Validate the proper number of instances (missing or extra)
*      (unless valonly is TRUE)
*
* The 'value' string cannot be split across multiple argv segments.
* Use quotation chars within the CLI shell to pass a string
* containing whitespace to the CLI parser:
*
*   --foo="quoted string if whitespace needed"
*   --foo="quoted string if setting a variable \
*         as a top-level assignment"
*   --foo=unquoted-string-without-whitespace
*
* The input is parsed against the specified obj_template_t struct.
* A val_value_t tree is built as the input is read.
* Each parameter is syntax checked as is is parsed.
*
* If possible, the parser will skip to next parmameter in the parmset,
* in order to support 'continue-on-error' type of operations.
*
* Any agent callback functions that might be registered for
* the specified container (or its sub-nodes) are not called
* by this function.  This must be done after this function
* has been called, and returns NO_ERR.
* 
* INPUTS:
*   rcxt == runstack context to use
*   argc == number of strings passed in 'argv'
*   argv == array of command line argument strings
*   obj == obj_template_t of the container 
*          that should be used to validate the input
*          against the child nodes of this container
*   valonly == TRUE if only the values presented should
*             be checked, no defaults, missing parms (Step 1 & 2 only)
*           == FALSE if all the tests and procedures should be done
*   autocomp == TRUE if parameter auto-completion should be
*               tried if any specified parameters are not matches
*               for the specified parmset
*            == FALSE if exact match only is desired
*   mode == CLI_MODE_PROGRAM if calling with real (argc, argv)
*           parameters; these may need some prep work
*        == CLI_MODE_COMMAND if calling from yangcli or
*           some other internal command parser.
*           These strings will not be preped at all
*
*   status == pointer to status_t to get the return value
*
* OUTPUTS:
*   *status == the final function return status
*
* Just as the NETCONF parser does, the CLI parser will not add a parameter
* to the val_value_t if any errors occur related to the initial parsing.
*
* RETURNS:
*   pointer to the malloced and filled in val_value_t
*********************************************************************/
extern val_value_t *
    cli_parse (runstack_context_t *rcxt,
               int argc, 
	       char *argv[],
	       obj_template_t *obj,
	       boolean valonly,
	       boolean script,
	       boolean autocomp,
               cli_mode_t  mode,
	       status_t  *status);


/********************************************************************
* FUNCTION cli_parse_parm
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the parent container value
*
* ONLY CALLED FROM CLI PARSING FUNCTIONS IN ncxcli.c
* ALLOWS SCRIPT EXTENSIONS TO BE PRESENT
*
* INPUTS:
*   rcxt == runstack context to use
*   val == parent value struct to adjust
*   parm == obj_template_t descriptor for the missing parm
*   strval == string representation of the parm value
*             (may be NULL if parm btype is NCX_BT_EMPTY
*   script == TRUE if CLI script mode
*          == FALSE if CLI plain mode
* 
* OUTPUTS:
*   A new val_value_t will be inserted in the val->v.childQ 
*   as required to fill in the parm.
*
* RETURNS:
*   status 
*********************************************************************/
extern status_t
    cli_parse_parm (runstack_context_t *rcxt,
                    val_value_t *val,
		    obj_template_t *obj,
		    const xmlChar *strval,
		    boolean script);


/********************************************************************
* FUNCTION cli_parse_parm_ex
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the parent container value
* Allow different bad data error handling vioa parameter
*
* ONLY CALLED FROM CLI PARSING FUNCTIONS IN ncxcli.c
* ALLOWS SCRIPT EXTENSIONS TO BE PRESENT
*
* INPUTS:
*   rcxt == runstack context to use
*   val == parent value struct to adjust
*   parm == obj_template_t descriptor for the missing parm
*   strval == string representation of the parm value
*             (may be NULL if parm btype is NCX_BT_EMPTY
*   script == TRUE if CLI script mode
*          == FALSE if CLI plain mode
*   bad_data == enum defining how bad data should be handled
*
* OUTPUTS:
*   A new val_value_t will be inserted in the val->v.childQ 
*   as required to fill in the parm.
*
* RETURNS:
*   status 
*********************************************************************/
extern status_t
    cli_parse_parm_ex (runstack_context_t *rcxt,
                       val_value_t *val,
		       obj_template_t *obj,
		       const xmlChar *strval,
		       boolean script,
		       ncx_bad_data_t  bad_data);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_cli */
