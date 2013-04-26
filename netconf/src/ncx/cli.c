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
/*  FILE: cli.c

  Parse an XML representation of a parameter set instance,
  and create a ps_parmset_t struct that contains the data found.

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
21oct05      abb      begun
19dec05      abb      restart after refining NCX
10feb06      abb      change raw xmlTextReader interface to use
                      ses_cbt_t instead
10feb07      abb      split common routines from agt_ps_parse
                      so mgr code can use it
21jul08      abb      converted to cli.c; removed non-CLI functions

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <xmlstring.h>

#include  "procdefs.h"
#include "cfg.h"
#include "cli.h"
#include "dlq.h"
#include "log.h"
#include "obj.h"
#include "runstack.h"
#include  "status.h"
#include "typ.h"
#include "val.h"
#include "val_util.h"
#include "var.h"
#include "xml_util.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define CLI_DEBUG 1

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION parse_parm_cmn
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the value set
*
* INPUTS:
*   rcxt == runstack context to use
*   new_parm == complex val_value_t to add the parsed parm into
*   strval == string representation of the object value
*             (may be NULL if obj btype is NCX_BT_EMPTY
*   script == TRUE if parsing a script (in the manager)
*          == FALSE if parsing XML or CLI
*
* OUTPUTS:
*   If the specified parm is mandatory w/defval defined, then a 
*   new val_value_t will be inserted in the vnew_parm->v.childQ 
*   as required to fill in the value set.
*
* RETURNS:
*   status 
*********************************************************************/
static status_t
    parse_parm_cmn (runstack_context_t *rcxt,
                    val_value_t *new_parm,
                    const xmlChar *strval,
                    boolean script)
{
    val_value_t          *newchild;
    typ_def_t            *typdef;
    obj_template_t       *obj, *choiceobj, *targobj;
    ncx_btype_t           btyp;
    status_t              res;

    res = NO_ERR;
    obj = new_parm->obj;

    /* check special case where the parm is a container
     * with 1 child -- a choice of empty; for this
     * common NETCONF mechanism, try to match the
     * strval to a child name
     */
    if (obj_is_root(obj)) {
        if (script) {
            (void)var_get_script_val(rcxt, obj, new_parm, strval, ISPARM, &res);
        } else {
            log_error("\nError: ncx:root object only "
                      "supported in script mode");
            res = ERR_NCX_INVALID_VALUE;
        }
    } else if (obj->objtype == OBJ_TYP_CONTAINER) {
        /* check if the only child is an OBJ_TYP_CHOICE */
        choiceobj = obj_first_child(obj);
        if (choiceobj == NULL) {
            log_error("\nError: container %s does not have any child nodes", 
                      obj_get_name(obj));
            return ERR_NCX_INVALID_VALUE;
        }

        /* check that empty string was not entered, for a default
         * there is no default for a container   */
        if (strval == NULL || *strval == 0) {
            log_error("\nError: 'missing value string");
            return ERR_NCX_INVALID_VALUE;
        }

        /* figure out how to create the child node (newchild)
         * that will be added to the new_parm container
         */
        if (*strval == '$' || *strval == '@') {
            /* this is a file or var reference */
            if (script) {
                newchild = var_get_script_val_ex(rcxt, obj, choiceobj, NULL,
                                                 strval, ISPARM, NULL, &res);
                if (newchild != NULL) {
                    if (newchild->obj == obj) {
                        val_replace(newchild, new_parm);
                        val_free_value(newchild);
                    } else {
                        val_add_child(newchild, new_parm);
                    }
                }
            } else {
                log_error("\nError: var or file reference only "
                          "supported in script mode");
                return ERR_NCX_INVALID_VALUE;
            }
        } else {
            if (choiceobj->objtype != OBJ_TYP_CHOICE) {
                log_error("\nError: child %s in container %s must be "
                          "a 'choice' object",
                          obj_get_name(choiceobj),
                          obj_get_name(obj));
                return ERR_NCX_INVALID_VALUE;
            }

            /* check if a child of any case is named 'strval'
             * this check will look deep and find child nodes
             * within a choice or case with the same name
             * as a member of the choice or case node
             */
            targobj = obj_find_child(choiceobj, obj_get_mod_name(choiceobj),
                                     strval);
            if (targobj == NULL) {
                log_error("\nError: choice %s in container %s"
                          " does not have any child nodes named '%s'",
                          obj_get_name(choiceobj), obj_get_name(obj),
                          strval);
                return ERR_NCX_INVALID_VALUE;
            }

            if (targobj->objtype != OBJ_TYP_LEAF) {
                log_error("\nError: case %s in choice %s in container %s"
                          " is not a leaf node",
                          obj_get_name(targobj), obj_get_name(choiceobj),
                          obj_get_name(obj));
                return ERR_NCX_INVALID_VALUE;
            }

            if (obj_get_basetype(targobj) != NCX_BT_EMPTY) {
                log_error("\nError: leaf %s in choice %s in container %s"
                          " is not type 'empty'",
                          obj_get_name(targobj),
                          obj_get_name(choiceobj),
                          obj_get_name(obj));
                return ERR_NCX_INVALID_VALUE;
            }

            /* found a match so create a value node */
            newchild = val_new_value();
            if (!newchild) {
                res = ERR_INTERNAL_MEM;
            } else {
                val_init_from_template(newchild, targobj);
                val_add_child(newchild, new_parm);
            }
        }
    } else if (obj->objtype == OBJ_TYP_CHOICE && strval != NULL) {
        if (*strval == '$' || *strval == '@') {
            if (script) {
                newchild = var_get_script_val(rcxt, obj, NULL, strval, 
                                              ISPARM, &res);
                if (newchild != NULL) {
                    val_replace(newchild, new_parm);
                    val_free_value(newchild);
                }
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            /* check if a child of any case is named 'strval' */
            targobj = obj_find_child(obj, obj_get_mod_name(obj), strval);
            if (targobj && 
                obj_get_basetype(targobj) == NCX_BT_EMPTY) {
                /* found a match so set the value node to type empty */
                val_init_from_template(new_parm, targobj);
                val_set_name(new_parm, obj_get_name(targobj),
                             xml_strlen(obj_get_name(targobj)));
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
        }
    } else {
        if (script) {
            (void)var_get_script_val(rcxt, obj, new_parm, strval, ISPARM, &res);
        } else {
            /* get the base type value */
            btyp = obj_get_basetype(obj);
            if (btyp == NCX_BT_ANY || typ_is_simple(btyp)) {
                typdef = obj_get_typdef(obj);
                if (btyp != NCX_BT_ANY) {
                    res = val_simval_ok(typdef, strval);
                }
                if (res == NO_ERR) {
                    res = val_set_simval(new_parm, typdef, obj_get_nsid(obj),
                                         obj_get_name(obj), strval);
                }
            } else {
                res = ERR_NCX_WRONG_DATATYP;
            }
        }
    }

    return res;

}  /* parse_parm_cmn */


/********************************************************************
* FUNCTION parse_cli_parm
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the value set
*
* INPUTS:
*   rcxt == runstack context to use
*   val == complex val_value_t to add the parsed parm into
*   obj == obj_template_t descriptor for the missing parm
*   strval == string representation of the object value
*             (may be NULL if obj btype is NCX_BT_EMPTY
*   script == TRUE if parsing a script (in the manager)
*          == FALSE if parsing XML or CLI
*
* OUTPUTS:
*   If the specified parm is mandatory w/defval defined, then a 
*   new val_value_t will be inserted in the val->v.childQ as required
*   to fill in the value set.
*
* RETURNS:
*   status 
*********************************************************************/
static status_t
    parse_cli_parm (runstack_context_t *rcxt,
                    val_value_t *val,
                    obj_template_t *obj,
                    const xmlChar *strval,
                    boolean script)
{
    val_value_t          *new_parm;
    status_t              res;

    /* create a new parm and fill it in */
    new_parm = val_new_value();
    if (!new_parm) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(new_parm, obj);

    res = parse_parm_cmn(rcxt, new_parm, strval, script);

    /* save or free the new child node */
    if (res != NO_ERR) {
        val_free_value(new_parm);
    } else {
        val_add_child_sorted(new_parm, val);
    }

    return res;

}  /* parse_cli_parm */


/********************************************************************
* FUNCTION parse_parm_ex
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the value set (extended)
*
* INPUTS:
*   rcxt == runstack context to use
*   val == complex val_value_t to add the parsed parm into
*   obj == obj_template_t descriptor for the missing parm
*   nsid == namespace ID to really use
*   name == object name to really use
*   strval == string representation of the object value
*             (may be NULL if obj btype is NCX_BT_EMPTY
*   script == TRUE if parsing a script (in the manager)
*          == FALSE if parsing XML or CLI
*
* OUTPUTS:
*   If the specified parm is mandatory w/defval defined, then a 
*   new val_value_t will be inserted in the val->v.childQ as required
*   to fill in the value set.
*
* RETURNS:
*   status 
*********************************************************************/
static status_t
    parse_parm_ex (runstack_context_t *rcxt,
                   val_value_t *val,
                   obj_template_t *obj,
                   xmlns_id_t  nsid,
                   const xmlChar *name,
                   const xmlChar *strval,
                   boolean script)
{
    val_value_t       *new_parm;
    status_t           res;
    
    /* create a new parm and fill it in */
    new_parm = val_new_value();
    if (!new_parm) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(new_parm, obj);

    /* adjust namespace ID and name */
    val_set_name(new_parm, name, xml_strlen(name));
    new_parm->nsid = nsid;

    res = parse_parm_cmn(rcxt, new_parm, strval, script);

    if (res != NO_ERR) {
        val_free_value(new_parm);
    } else {
        val_add_child(new_parm, val);
    }
    return res;

}  /* parse_parm_ex */


/********************************************************************
* FUNCTION find_rawparm
* 
* Find the specified raw parm entry
*
* INPUTS:
*   parmQ == Q of cli_rawparm_t 
*   name == object name to really use
*   namelen == length of 'name'
*
* RETURNS:
*   raw parm entry if found, NULL if not
*********************************************************************/
static cli_rawparm_t *
    find_rawparm (dlq_hdr_t *parmQ,
                  const char *name,
                  int32 namelen)
{
    cli_rawparm_t  *parm;

    if (!namelen) {
        return NULL;
    }

    /* check exact match only */
    for (parm = (cli_rawparm_t *)dlq_firstEntry(parmQ);
         parm != NULL;
         parm = (cli_rawparm_t *)dlq_nextEntry(parm)) {

        if (strlen(parm->name) == (size_t)namelen &&
            !strncmp(parm->name, name, namelen)) {
            return parm;
        }
    }

    return NULL;

} /* find_rawparm */


/********************************************************************
* FUNCTION copy_argv
* 
*  Check the argv string and copy it to the CLI buffer
*  Add quotes as needed to restore command to original form
*
* INPUTS:
*   buffer == buffer spot to copy string into
*   parmstr == parameter string to use
*
* RETURNS:
*   number of chars written to the buffer
*********************************************************************/
static int32
    copy_argv (char *buffer,
               const char *parmstr)
{
    const char   *str;
    char         *str2;

    str = NULL;
    str2 = NULL;

    /* look for any whitespace in the string at all */
    str = parmstr;
    while (*str && !isspace((int)*str)) {
        str++;
    }
    if (*str == '\0') {
        /* no whitespace, so nothing to worry about */
        return (int32)xml_strcpy((xmlChar *)buffer, 
                                 (const xmlChar *)parmstr);
    } 

    /* check leading whitespace */
    str = parmstr;
    while (*str && isspace((int)*str)) {
        str++;
    }
    if (*str == '\0') {
        /* the string is all whitespace, just copy it
         * and it will be skipped later
         */
        return  (int32)xml_strcpy((xmlChar *)buffer, 
                                  (const xmlChar *)parmstr);
    } else if (str != parmstr) {
        /* there was starting whitespace
         * just guess that the whole string was
         * quoted, since it is a rare case
         */
        str2 = buffer;
        *str2++ = '"';
        str2 += (int32)xml_strcpy((xmlChar *)str2,
                                  (const xmlChar *)parmstr);
        *str2++ = '"';
        *str2 = '\0';
        return (str2 - buffer);
    }

    /* started with some non-whitespace; go until the
     * equals sign is seen or the next whitespace
     */
    while (*str && !isspace((int)*str) && (*str != '=')) {
        str++;
    }

    /* check where the search stopped */
    if (*str == '=') {
        str++;   /* str == first char to be inside dquotes */
    } else if (isspace((int)*str)) {
        /* see if the equals sign is still out there */
      while (*str && isspace((int)*str)) {
            str++;
        }
        if (*str == '=') {
            str++;  /* str == first char to be inside dquotes */
        } else {
            /* hit end of string of the start of some new token */
        }
    } else {
        /* should not have hit end of string */
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
     
    /* have some split point in the command string to
     * guess where the original quotes were located
     */
    str2 = buffer;
    str2 += (int32)xml_strncpy((xmlChar *)str2, 
                               (const xmlChar *)parmstr,
                               (uint32)(str - parmstr));
    *str2++ = '"';
    str2 += (int32)xml_strcpy((xmlChar *)str2, 
                              (const xmlChar *)str);
    *str2++ = '"';
    *str2 = '\0';

    return (str2 - buffer);

} /* copy_argv */


/********************************************************************
* FUNCTION copy_argv_to_buffer
* 
*  Check the argv string and copy it to the CLI buffer
*  Add quotes as needed to restore command to original form
*
* INPUTS:
*   argc == number of strings passed in 'argv'
*   argv == array of command line argument strings
*   mode == CLI parsing mode in progress 
*           (CLI_MODE_PROGRAM or CLI_MODE_COMMAND)
*   bufflen == address of return buffer length
*   res == address of return status
*
* OUTPUTS:
*   *bufflen contains the number of chars to use in the 
*       return buffer.
*     Note that the malloced size of the return
*     buffer may be slightly larger than this.
*   *res == return status
*
* RETURNS:
*   malloced buffer with the argv[] strings copied
*   as if it were 1 long command line
*********************************************************************/
static char *
    copy_argv_to_buffer (int argc,
                         char *argv[],
                         cli_mode_t mode,
                         int32 *bufflen,
                         status_t *res)
{
    char     *buff;
    int32     padamount;
    int32     buffpos;
    int       parmnum;

    *bufflen = 0;
    *res = NO_ERR;

    /* gather all the argv strings into one buffer for easier parsing
     * need to copy because the buffer is written during parsing
     * and the argv parameter is always a 'const char **' data type
     *
     * Unfortunately, glibc will remove any 'outside' quotes
     *  that were found while processing the input
     *
     *  program --foo --bar=1 2  --baz="1 2 3" "--goo='a b c'"
     *
     *  argc = 6
     *  argv[0] = <absolute path to program>
     *  argv[1] = '--foo'
     *  argv[2] = '--bar=1'
     *  argv[3] = '2'
     *  argv[4] = '--baz=1 2 3'
     *  argv[5] = "--goo='a b c'"
     *
     * Note:
     *   argv[2] is an error anytime, no quotes will be added
     *   argv[4] needs quotes put back into the string
     *   argv[5] needs to be left alone and not get any new quotes
     *
     * first determine the buffer length
     *
     * mode == CLI_MODE_COMMAND:
     *   padamount = 1 == for the space char added
     *
     * mode == CLI_MODE_PROGRAM:
     *   padamount = 3 
     *      1 for the space char added
     *      2 for the 2 quote chars that may be needed
     */
    switch (mode) {
    case CLI_MODE_PROGRAM:
        padamount = 3;
        break;
    case CLI_MODE_COMMAND:
        padamount = 1;
        break;
    default:
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    for (parmnum=1; parmnum < argc; parmnum++) {
        *bufflen += (strlen(argv[parmnum]) + padamount);
    }

    buff = m__getMem(*bufflen + 1);
    if (!buff) {
        /* non-recoverable error */
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* copy the argv strings into the buffer */
    buffpos = 0;

    for (parmnum=1; parmnum < argc; parmnum++) {
        if (mode == CLI_MODE_PROGRAM) {
            buffpos += copy_argv(&buff[buffpos], argv[parmnum]);
        } else {
            buffpos += (int32)xml_strcpy((xmlChar *)&buff[buffpos], 
                                         (const xmlChar *)argv[parmnum]);
        }
        if (parmnum+1 < argc) {
            buff[buffpos++] = ' ';
        }
    }
    buff[buffpos] = 0;

    /* need to shorten the buffer because the malloc guessed
     * 2 chars bigger than needed in most cases, on each
     * separate command line
     */
    *bufflen = buffpos;

    return buff;

} /* copy_argv_to_buffer */


/**************    E X T E R N A L   F U N C T I O N S **********/


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
cli_rawparm_t *
    cli_new_rawparm (const xmlChar *name)
{
    cli_rawparm_t  *parm;

#ifdef DEBUG
    if (!name) {
        return NULL;
    }
#endif

    parm = m__getObj(cli_rawparm_t);
    if (parm) {
        memset(parm, 0x0, sizeof(cli_rawparm_t));
        parm->name = (const char *)name;
        parm->hasvalue = TRUE;
    }
    return parm;

} /* cli_new_rawparm */


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
cli_rawparm_t *
    cli_new_empty_rawparm (const xmlChar *name)
{
    cli_rawparm_t  *parm;

#ifdef DEBUG
    if (!name) {
        return NULL;
    }
#endif

    parm = m__getObj(cli_rawparm_t);
    if (parm) {
        memset(parm, 0x0, sizeof(cli_rawparm_t));
        parm->name = (const char *)name;
    }
    return parm;

} /* cli_new_empty_rawparm */


/********************************************************************
* FUNCTION cli_free_rawparm
* 
* Clean and free a raw parm entry
*
* INPUTS:
*   parm == raw parm entry to free
*********************************************************************/
void
    cli_free_rawparm (cli_rawparm_t *parm)
{
#ifdef DEBUG
    if (!parm) {
        return;
    }
#endif

    if (parm->value) {
        m__free(parm->value);
    }
    m__free(parm);

} /* cli_free_rawparm */


/********************************************************************
* FUNCTION cli_clean_rawparmQ
* 
* Clean and free a Q of raw parm entries
*
* INPUTS:
*   parmQ == Q of raw parm entry to free
*********************************************************************/
void
    cli_clean_rawparmQ (dlq_hdr_t *parmQ)
{
    cli_rawparm_t  *parm;

#ifdef DEBUG
    if (!parmQ) {
        return;
    }
#endif

    while (!dlq_empty(parmQ)) {
        parm = (cli_rawparm_t *)dlq_deque(parmQ);
        cli_free_rawparm(parm);
    }

} /* cli_clean_rawparmQ */


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
cli_rawparm_t *
    cli_find_rawparm (const xmlChar *name,
                      dlq_hdr_t *parmQ)
{
#ifdef DEBUG
    if (!name || !parmQ) {
        return NULL;
    }
#endif

    return find_rawparm(parmQ, 
                        (const char *)name, 
                        strlen((const char *)name));

} /* cli_find_rawparm */


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
status_t
    cli_parse_raw (int argc, 
                   char *argv[],
                   dlq_hdr_t *rawparmQ)
{
    cli_rawparm_t  *rawparm;
    char           *parmname, *parmval, *str, *buff;
    int32           parmnamelen, buffpos, bufflen;
    int             i;
    status_t        res;

#ifdef DEBUG
    if (!argv || !rawparmQ) {
        return ERR_INTERNAL_PTR;
    }
    if (dlq_empty(rawparmQ)) {
        return ERR_INTERNAL_VAL;
    }
#endif

    /* check if there are any parameters at all to parse */
    if (argc < 2) {
        return NO_ERR;
    }

    if (LOGDEBUG2) {
        log_debug("\nCLI bootstrap: input parameters:");
        for (i=0; i<argc; i++) {
            log_debug("\n   arg%d: '%s'", i, argv[i]);
        }
    }

    res = NO_ERR;
    bufflen = 0;
    buff = copy_argv_to_buffer(argc, argv, CLI_MODE_PROGRAM, &bufflen, &res);
    if (!buff) {
        return res;
    }

    /* setup parm loop */
    buffpos = 0;
    res = NO_ERR;

    /* go through all the command line strings
     * setup parmname and parmval based on strings found
     * and the rawparmQ for these parameters
     * save each parm value in the parmQ entry
     */
    while (buffpos < bufflen && res == NO_ERR) {

        /* first skip starting whitespace */
      while (buff[buffpos] && isspace((int)buff[buffpos])) {
            buffpos++;
        }

        /* check start of parameter name conventions 
         * allow zero, one, or two dashes before parmname
         *          foo   -foo   --foo
         */
        if (!buff[buffpos]) {
            res = ERR_NCX_WRONG_LEN;
        } else if (buff[buffpos] == NCX_CLI_START_CH) {
            if (!buff[buffpos+1]) {
                res = ERR_NCX_WRONG_LEN;
            } else if (buff[buffpos+1] == NCX_CLI_START_CH) {
                if (!buff[buffpos+2]) {
                    res = ERR_NCX_WRONG_LEN;
                } else {
                    buffpos += 2;  /* skip past 2 dashes */
                }
            } else {
                buffpos++;    /* skip past 1 dash */
            }
        } /* else no dashes, leave parmname pointer alone */

        /* check for the end of the parm name, wsp or equal sign
         * get the parm template, and get ready to parse it
         */
        if (res != NO_ERR) {
            continue;
        }

        /* check the parmname string for a terminating char */
        parmname = &buff[buffpos];
        str = &parmname[1];
        while (*str && !isspace((int)*str) && *str != '=') {
            str++;
        }

        parmnamelen = str - parmname;
        buffpos += parmnamelen;

        parmval = NULL;

        /* check if this parameter name is in the parmset def */
        rawparm = find_rawparm(rawparmQ, parmname, parmnamelen);
        if (rawparm) {
            rawparm->count++;
            if (!rawparm->hasvalue) {
                /* start over, since no value is expected
                 * for this known empty parm
                 */
                continue;
            }
        }

        if ((buffpos < bufflen) &&
            ((buff[buffpos] == '=') || isspace((int)buff[buffpos]))) {

            /* assume that the parm followed by a 
             * space is the termination, try again on
             * the next keyword match
             */
            if (buff[buffpos] != '=' && !rawparm) {
                continue;
            }

            buffpos++;  /* skip past '=' or whitespace */

            while (buff[buffpos] && isspace((int)buff[buffpos])) {
                buffpos++;
            }

            /* if any chars left in buffer, get the parmval */
            if (buffpos < bufflen)  {
                if (buff[buffpos] == NCX_QUOTE_CH) {
                    /* set the start after quote */
                    parmval = &buff[++buffpos];
                        
                    /* find the end of the quoted string */
                    str = &parmval[1];
                    while (*str && *str != NCX_QUOTE_CH) {
                        str++;
                    }
                } else {
                    /* set the start of the parmval */
                    parmval = &buff[buffpos];
                    
                    /* find the end of the unquoted string */
                    str = &parmval[1];
                    while (*str && !isspace((int)*str)) {
                        str++;
                    }
                }

                /* terminate string */
                *str = 0;

                /* skip buffpos past eo-string */
                buffpos += (int32)((str - parmval) + 1);  
            }
        }

        if (rawparm) {
            if (rawparm->value) {
                m__free(rawparm->value);
            }
            rawparm->value = (char *)
                xml_strdup((const xmlChar *)parmval);
            if (!rawparm->value) {
                res = ERR_INTERNAL_MEM;
            }
        }
    }

    m__free(buff);

    return res;

}  /* cli_parse_raw */


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
val_value_t *
    cli_parse (runstack_context_t *rcxt,
               int argc, 
               char *argv[],
               obj_template_t *obj,
               boolean valonly,
               boolean script,
               boolean autocomp,
               cli_mode_t mode,
               status_t  *status)
{
#define ERRLEN  127

    val_value_t    *val;
    obj_template_t *chobj;
    const char     *msg;
    char           *parmname, *parmval;
    char           *str = NULL, *buff, testch;
    int32           buffpos, bufflen;
    uint32          parmnamelen, copylen, matchcount;
    ncx_btype_t     btyp;
    status_t        res;
    xmlChar         errbuff[ERRLEN+1], savechar;
    boolean         gotdashes, gotmatch, gotdefaultparm, isdefaultparm;
    int             i;

#ifdef DEBUG
    if (!status) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (!argv || !obj) {
        *status = SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    res = NO_ERR;
    *status = NO_ERR;

    /* check if CLI parmset really OK to use
     * Must have only choices, leafs, and leaflists
     * Since the CLI parameters come from an external YANG file
     * This is a quick check to make sure the CLI code will work
     * with the provided object template.
     */
    if (!script && !obj_ok_for_cli(obj)) {
        *status = SET_ERROR(ERR_NCX_OPERATION_FAILED);
        return NULL;
    }

    /* check if there are any parameters at all to parse */
    if (argc < 2 && valonly) {
        *status = NO_ERR;
        return NULL;
    }

    if (LOGDEBUG3) {
        log_debug3("\nCLI: input parameters:");
        for (i=0; i<argc; i++) {
            log_debug3("\n   arg%d: '%s'", i, argv[i]);
        }
    }

    /* allocate a new container value to hold the output */
    val = val_new_value();
    if (!val) {
        *status = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(val, obj);

    if (script) {
        /* set the parmset name to the application pathname */
        val->dname = xml_strdup((const xmlChar *)argv[0]);
        val->name = val->dname;
    } else {
        /* set the parmset name to the PSD static name */
        val->name = obj_get_name(obj);
    }

    /* check for a malloc error */
    if (!val->name) {
        *status = ERR_INTERNAL_MEM;
        val_free_value(val);
        return NULL;
    }

    bufflen = 0;
    buff = NULL;

    /* handle special case -- may just need default CLI params
     * and validation
     */
    if (argc < 2) {
        /* 2) add any defaults for optional parms that are not set */
        if (!valonly) {
            res = val_add_defaults(val, NULL, NULL, script);
        }

        /* 3) CLI Instance Check
         * Go through all the parameters in the object and check
         * to see if each child node is present with the 
         * correct number of instances
         * The CLI object node can be a choice or a simple parameter 
         * The choice test is also performed by this function call
         */
        if (res == NO_ERR && !valonly) {
            res = val_instance_check(val, val->obj);
        }
        *status = res;
        return val;
    }

    /* need to parse some CLI parms
     * get 1 normalized buffer
     */
    buff = copy_argv_to_buffer(argc, argv, mode, &bufflen, status);
    if (!buff) {
        val_free_value(val);
        return NULL;
    }

    /* setup parm loop */
    res = NO_ERR;
    buffpos = 0;
    gotdefaultparm = FALSE;

    /* 1) go through all the command line strings
     * setup parmname and parmval based on strings found
     * and the PSD for these parameters
     * save each parm value in a ps_parm_t struct
     */
    while (buffpos < bufflen && res == NO_ERR) {
        boolean finish_equals_ok = FALSE;

        gotdashes = FALSE;
        gotmatch = FALSE;
        isdefaultparm = FALSE;
        chobj = NULL;
        parmval = NULL;
        parmname = NULL;
        parmnamelen = 0;

        /* first skip starting whitespace */
        while (buff[buffpos] && isspace((int)buff[buffpos])) {
            buffpos++;
        }

        /* check start of parameter name conventions 
         * allow zero, one, or two dashes before parmname
         *          foo   -foo   --foo
         */
        if (!buff[buffpos]) {
            /* got some extra whitespace at the EOLN */
            continue;
        } else if (buff[buffpos] == NCX_CLI_START_CH) {
            if (!buff[buffpos+1]) {
                res = ERR_NCX_INVALID_VALUE;
            } else if (buff[buffpos+1] == NCX_CLI_START_CH) {
                testch = buff[buffpos+2];
                if (testch == '\0' ||
                    isspace((int)testch) ||
                    testch == NCX_CLI_START_CH) {
                    res = ERR_NCX_INVALID_VALUE;
                } else {
                    buffpos += 2;  /* skip past 2 dashes */
                    gotdashes = TRUE;
                }
            } else {
                testch = buff[buffpos+1];
                if (testch == '\0' ||
                    isspace((int)testch)) {
                    res = ERR_NCX_INVALID_VALUE;
                } else {
                    buffpos++;    /* skip past 1 dash */
                    gotdashes = TRUE;
                }
            }
        } /* else no dashes, leave parmname pointer alone */

        /* should be pointing at start of the parm name
         * check for the end of the parm name, wsp or equal sign
         * get the parm template, and get ready to parse it
         */
        if (res == NO_ERR) {
            /* check the parmname string for a terminating char */
            parmname = &buff[buffpos];
            parmnamelen = 0;

            if (ncx_valid_fname_ch(*parmname)) {
                str = &parmname[1];
                while (*str && ncx_valid_name_ch(*str)) {
                    str++;
                }
                parmnamelen = (uint32)(str - parmname);
                copylen = parmnamelen;

                /* check if this parameter name is in the parmset def */
                chobj = obj_find_child_str(obj, NULL, 
                                           (const xmlChar *)parmname,
                                           parmnamelen);

                /* check if parm was found, try partial name if not */
                if (!chobj && autocomp) {
                    matchcount = 0;
                    chobj = obj_match_child_str(obj, NULL,
                                                (const xmlChar *)parmname,
                                                parmnamelen,
                                                &matchcount);
                    if (chobj) {
                        gotmatch = TRUE;
                        if (matchcount > 1) {
                            res = ERR_NCX_AMBIGUOUS_CMD;
                        }
                    } else {
                        copylen = 0;
                    }
                }

                /* advance the buffer pointer */
                buffpos += copylen;

            }  /* else it could be a default-parm value */


            if (res != NO_ERR) {
                if (res == ERR_NCX_INVALID_VALUE) {
                    log_error("\nError: invalid CLI parameter prefix");
                } else {
                    log_error("\nError: invalid CLI syntax (%s)",
                              get_error_string(res));
                }
                continue;
            }

            if (!chobj && !gotdashes) {
                if (parmnamelen) {
                    int32 idx = buffpos + parmnamelen;

                    /* check if next char is an equals sign and
                     * the default-parm-equals_ok flag is set
                     */
                    if (obj_is_cli_equals_ok(obj) && buff[idx] == '=') {
                        finish_equals_ok = TRUE;
                    } else {
                        /* check for whitespace following value */
                        while (isspace((int)buff[idx]) && idx < bufflen) {
                            idx++;
                        }

                        /* check for equals sign, indicating an unknown
                         * parameter name, not a value string for the
                         * default parameter
                         */
                        if (buff[idx] == '=') {
                            /* will prevent chobj from getting set */
                            res = ERR_NCX_UNKNOWN_PARM;
                        }
                    }
                }

                if (res == NO_ERR) {
                    /* try the default parameter
                     * if any defined, then use the unknown parm
                     * name as a parameter value for the default parm
                     */
                    chobj = obj_get_default_parm(obj);
                    if (chobj) {
                        if (chobj->objtype != OBJ_TYP_LEAF_LIST &&
                            gotdefaultparm) {
                            log_error("\nError: default parm '%s' "
                                      "already entered",
                                      obj_get_name(chobj));
                            res = ERR_NCX_DUP_ENTRY;
                            continue;
                        }
                        gotdefaultparm = TRUE;
                        isdefaultparm = TRUE;
                    }
                }
            }

            if (chobj == NULL) {
                res = ERR_NCX_UNKNOWN_PARM;
            } else {
                /* do not check parameter order for CLI */
                btyp = obj_get_basetype(chobj);
                parmval = NULL;

                /* skip past any whitespace after the parm name */
                if (!isdefaultparm) {
                    while (isspace((int)buff[buffpos]) && buffpos < bufflen) {
                        buffpos++;
                    }
                }

                /* check if ended on space of EOLN */
                if (btyp == NCX_BT_EMPTY) {
                    if (buff[buffpos] == '=') {
                        log_error("\nError: cannot assign value "
                                  "to empty leaf '%s'", 
                                  obj_get_name(obj));
                        res = ERR_NCX_INVALID_VALUE;
                    }
                } else if (buffpos < bufflen) {
                    if (!isdefaultparm) {
                        if (buff[buffpos] == '=') {
                            buffpos++;

                            /* skip any whitespace */
                            while (buff[buffpos] && buffpos < bufflen &&
                                   isspace((int)buff[buffpos])) {
                                buffpos++;
                            }
                        } /* else whitespace already skipped */
                    }

                    /* if any chars left in buffer, get the parmval */
                    if (buffpos < bufflen) {
                        if (finish_equals_ok) {
                            /* treating the entire string as the 
                             * parm value; expecting a string to follow
                             */
                            int32  j, testidx = 0;

                            parmval = &buff[buffpos];

                            while ((buffpos+testidx) < bufflen &&
                                   buff[buffpos+testidx] != '=') {
                                testidx++;
                            }

                            j = buffpos+testidx+1;
                            if (buff[buffpos+testidx] != '=') {
                                res = SET_ERROR(ERR_INTERNAL_VAL);
                            } else {
                                if (j < bufflen) {
                                    if (buff[j] == NCX_QUOTE_CH ||
                                        buff[j] == NCX_SQUOTE_CH) {

                                        savechar = buff[j++];
                                        while (j < bufflen && buff[j] && 
                                               buff[j] != savechar) {
                                            j++;
                                        }
                                        if (buff[j]) {
                                            if (j < bufflen) {
                                                /* OK exit */
                                                str = &buff[j+1];
                                            } else {
                                                res = SET_ERROR
                                                    (ERR_INTERNAL_VAL);
                                            }
                                        } else {
                                            str = &buff[j];
                                        }
                                    } else {
                                        /* not a quoted string */
                                        while (j < bufflen && buff[j] &&
                                               !isspace((int)buff[j])) {
                                            j++;
                                        }
                                        str = &buff[j];  // OK exit
                                    }
                                } else {
                                    str = &buff[j];
                                }
                            }
                        } else if (buff[buffpos] == NCX_QUOTE_CH ||
                                   buff[buffpos] == NCX_SQUOTE_CH) {

                            savechar = buff[buffpos];

                            if (script) {
                                /* set the start at quote */
                                parmval = &buff[buffpos];
                            } else {
                                /* set the start after quote */
                                parmval = &buff[++buffpos];
                            }

                            /* find the end of the quoted string */
                            str = &parmval[1];
                            while (*str && *str != savechar) {
                                str++;
                            }

                            if (*str == savechar) {
                                /* if script mode keep ending quote */
                                if (script) {
                                    str++;
                                }
                            } else {
                                log_error("\nError: unfinished string "
                                          "found '%s'", parmval);
                                res = ERR_NCX_UNENDED_QSTRING;
                            }
                        } else if (script && (buffpos+1 < bufflen) &&
                                   (buff[buffpos] == NCX_XML1a_CH) &&
                                   (buff[buffpos+1] == NCX_XML1b_CH)) {
                            /* set the start of the XML parmval to the [ */
                            parmval = &buff[buffpos];
                            
                            /* find the end of the inline XML */
                            str = parmval+1;
                            while (*str && !((*str==NCX_XML2a_CH) 
                                             && (str[1]==NCX_XML2b_CH))) {
                                str++;
                            }

                            if (!*str) {
                                /* did not find end of XML string */
                                res = ERR_NCX_DATA_MISSING;
                            } else {
                                /* setup after the ] char to be zeroed */
                                str += 2;
                            }
                        } else {
                            /* set the start of the parmval */
                            parmval = &buff[buffpos];

                            /* find the end of the unquoted string */
                            str = parmval+1;
                            while (*str && !isspace((int)*str)) {
                                str++;
                            }
                        }

                        /* terminate string */
                        if (str) {
                            *str = 0;

                            /* skip buffpos past eo-string */
                            buffpos += (uint32)((str - parmval) + 1);  
                        } else {
                            res = SET_ERROR(ERR_INTERNAL_VAL);
                        }
                    }
                }


                /* make sure value entered if expected
                 * NCX_BT_EMPTY and NCX_BT_STRING 
                 * (if zero-length strings allowed)
                 */
                if (res==NO_ERR && !parmval && btyp != NCX_BT_EMPTY) {
                    if (!(typ_is_string(btyp) &&
                        (val_simval_ok(obj_get_typdef(chobj),
                                       EMPTY_STRING) == NO_ERR))) {
                        res = ERR_NCX_EMPTY_VAL;
                    }
                }
            }
        }

        /* create a new val_value struct and set the value */
        if (res == NO_ERR) {
            res = parse_cli_parm(rcxt, val, chobj, 
                                 (const xmlChar *)parmval, script);
        } else if (res == ERR_NCX_EMPTY_VAL &&
                   gotmatch && !gotdashes) {
            /* matched parm did not work out so
             * check if this is intended to be a
             * parameter value for the default-parm
             */
            chobj = obj_get_default_parm(obj);
            if (chobj) {
                savechar = parmname[parmnamelen];
                parmname[parmnamelen] = 0;
                res = parse_cli_parm(rcxt, val, chobj, 
                                     (const xmlChar *)parmname, script);
                parmname[parmnamelen] = savechar;
            }
        }

        /* check any errors in the parm name or value */
        if (res != NO_ERR) {
            msg = get_error_string(res);
            errbuff[0] = 0;
            if (parmname != NULL) {
                xml_strncpy(errbuff, (const xmlChar *)parmname, 
                            min(parmnamelen, ERRLEN));
            }
            switch (res) {
            case ERR_NCX_UNKNOWN_PARM:
                log_error("\nError: Unknown parameter (%s)", errbuff);
                break;
            case ERR_NCX_AMBIGUOUS_CMD:
                parmname[parmnamelen] = 0;
                log_error("\nError: multiple matches for '%s'",
                          parmname);
                break;
            default:
                if (*errbuff) {
                    if (parmval != NULL) {
                        log_error("\nError: %s (%s = %s)", 
                                  msg, 
                                  errbuff, 
                                  parmval);
                    } else if (buffpos < bufflen) {
                        log_error("\nError: %s (%s = %s)", 
                                  msg, 
                                  errbuff, 
                                  &buff[buffpos]);
                    } else {
                        log_error("\nError: %s (%s)", 
                                  msg, 
                                  errbuff);
                    }
                } else {
                    log_error("\nError: %s", msg);
                }
            }
            m__free(buff);
            *status = res;
            return val;
        }
    }

    /* cleanup after loop */
    m__free(buff);
    buff = NULL;

    /* 2) add any defaults for optional parms that are not set */
    if (res == NO_ERR && !valonly) {
        res = val_add_defaults(val, NULL, NULL, script);
    }

    /* 3) CLI Instance Check
     * Go through all the parameters in the object and check
     * to see if each child node is present with the 
     * correct number of instances
     * The CLI object node can be a choice or a simple parameter 
     * The choice test is also performed by this function call
     */
    if (res == NO_ERR && !valonly) {
        res = val_instance_check(val, val->obj);
    }

    *status = res;
    return val;

}  /* cli_parse */


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
status_t
    cli_parse_parm (runstack_context_t *rcxt,
                    val_value_t *val,
                    obj_template_t *obj,
                    const xmlChar *strval,
                    boolean script)
{
    return parse_cli_parm(rcxt, val, obj, strval, script);

}  /* cli_parse_parm */


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
status_t
    cli_parse_parm_ex (runstack_context_t *rcxt,
                       val_value_t *val,
                       obj_template_t *obj,
                       const xmlChar *strval,
                       boolean script,
                       ncx_bad_data_t  bad_data)
{
    obj_template_t  *genstr;
    status_t         res;

    res = parse_cli_parm(rcxt, val, obj, strval, script);
    if (res == NO_ERR || NEED_EXIT(res)) {
        return res;
    }

    switch (bad_data) {
    case NCX_BAD_DATA_WARN:
        if (ncx_warning_enabled(ERR_NCX_USING_BADDATA)) {
            log_warn("\nWarning: invalid value "
                     "'%s' used for parm '%s'",
                     (strval) ? strval : EMPTY_STRING,
                     obj_get_name(obj));
        }
        /* drop through */
    case NCX_BAD_DATA_IGNORE:
        genstr = ncx_get_gen_string();
        res = parse_parm_ex(rcxt,
                            val, 
                            genstr, 
                            obj_get_nsid(obj),
                            obj_get_name(obj),
                            strval, 
                            script);
        return res;
    case NCX_BAD_DATA_CHECK:
    case NCX_BAD_DATA_ERROR:
        return res;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    /*NOTREACHED*/

}  /* cli_parse_parm_ex */


/* END file cli.c */
