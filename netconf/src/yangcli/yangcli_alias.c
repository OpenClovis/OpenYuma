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
/*  FILE: yangcli_alias.c

   NETCONF YANG-based CLI Tool

   alias command

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
30-sep-11    abb      begun;

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "procdefs.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "status.h"
#include "xml_util.h"
#include "yangcli.h"
#include "yangcli_alias.h"
#include "yangcli_cmd.h"
#include "yangcli_util.h"


/********************************************************************
 * FUNCTION get_first_alias
 * 
 * Get the first alias record
 *
 * RETURNS:
 *   pointer to alias record or NULL if none
 *********************************************************************/
static alias_cb_t *
    get_first_alias (void)
{
    dlq_hdr_t  *aliasQ = get_aliasQ();

    if (aliasQ) {
        return (alias_cb_t *)dlq_firstEntry(aliasQ);
    }
    return NULL;

}  /* get_first_alias */


/********************************************************************
 * FUNCTION get_next_alias
 * 
 * Get the next alias record
 *
 * RETURNS:
 *   pointer to alias record or NULL if none
 *********************************************************************/
static alias_cb_t *
    get_next_alias (alias_cb_t *curalias)
{
    if (curalias) {
        return (alias_cb_t *)dlq_nextEntry(curalias);
    }
    return NULL;

}  /* get_next_alias */


/********************************************************************
 * FUNCTION free_alias
 * 
 * Free the alias record
 *
 * INPUTS:
 *   alias == pointer to alias record to free
 *********************************************************************/
static void
    free_alias (alias_cb_t *alias)
{
    if (alias == NULL) {
        return;
    }
    if (alias->name != NULL) {
        m__free(alias->name);
    }
    if (alias->value != NULL) {
        m__free(alias->value);
    }
    m__free(alias);

}  /* free_alias */


/********************************************************************
 * FUNCTION new_alias
 * 
 * Malloc and fill in an alias record
 *
 * INPUTS:
 *   name == alias name (not z-terminated)
 *   namelen == length of name string
 *
 * RETURNS:
 *   pointer to alias record or NULL if none
 *********************************************************************/
static alias_cb_t *
    new_alias (const xmlChar *name,
               uint32 namelen)
{
    alias_cb_t  *alias;

    if (namelen == 0) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    alias = m__getObj(alias_cb_t);
    if (alias == NULL) {
        return NULL;
    }
    memset(alias, 0x0, sizeof(alias_cb_t));
    alias->name = xml_strndup(name, namelen);
    if (alias->name == NULL) {
        free_alias(alias);
        return NULL;
    }
    return alias;

}  /* new_alias */


/********************************************************************
 * FUNCTION add_alias
 * 
 * Add the alias record
 *
 * INPUTS:
 *   alias == alias to add
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    add_alias (alias_cb_t *alias)
{
    dlq_hdr_t  *aliasQ = get_aliasQ();
    alias_cb_t *curalias;
    int         ret;

    if (aliasQ == NULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        free_alias(alias);
        return ERR_INTERNAL_VAL;
    }

    for (curalias = (alias_cb_t *)dlq_firstEntry(aliasQ);
         curalias != NULL;
         curalias = (alias_cb_t *)dlq_nextEntry(curalias)) {

        ret = xml_strcmp(curalias->name, alias->name);
        if (ret == 0) {
            SET_ERROR(ERR_NCX_DUP_ENTRY);
            free_alias(alias);
            return ERR_NCX_DUP_ENTRY;
        } else if (ret > 0) {
            dlq_insertAhead(alias, curalias);
            return NO_ERR;
        }
    }

    /* new last entry */
    dlq_enque(alias, aliasQ);
    return NO_ERR;

}  /* add_alias */


/********************************************************************
 * FUNCTION find_alias
 * 
 * Find the alias record
 *
 * INPUTS:
 *   name == alias name to find (not assumed to be z-terminated
 *   namelen == length of name string
 * RETURNS:
 *   alias record; NULL if not found
 *********************************************************************/
static alias_cb_t *
    find_alias (const xmlChar *name,
                uint32 namelen)
{
    dlq_hdr_t  *aliasQ = get_aliasQ();
    alias_cb_t *curalias;

    if (aliasQ == NULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    for (curalias = (alias_cb_t *)dlq_firstEntry(aliasQ);
         curalias != NULL;
         curalias = (alias_cb_t *)dlq_nextEntry(curalias)) {

        uint32 curlen = xml_strlen(curalias->name);
        int ret = xml_strncmp(curalias->name, name, namelen);

        if (ret == 0) {
            if (curlen == namelen) {
                return curalias;
            }
        } else if (ret > 0) {
            /* already past where this name should be sorted */
            return NULL;
        }
    }

    return NULL;

}  /* find_alias */


/********************************************************************
 * FUNCTION show_alias_ptr
 * 
 * Output 1 alias record
 *
 * INPUTS:
 *    alias == alias record to show
 *********************************************************************/
static void
    show_alias_ptr (const alias_cb_t *alias)
{
    const xmlChar *qchar = NULL;
    switch (alias->quotes) {
    case 0:
        qchar = EMPTY_STRING;
        break;
    case 1:
        qchar = (const xmlChar *)"'";
        break;
    case 2:
        qchar = (const xmlChar *)"\"";
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    log_write("\n%s=%s%s%s", alias->name, qchar,
              (alias->value) ? alias->value : EMPTY_STRING, qchar);

}  /* show_alias_ptr */


/********************************************************************
 * FUNCTION write_alias
 * 
 * Output 1 alias record to a file
 *
 * INPUTS:
 *    fp == FILE pointer to use for writing at current pos
 *    alias == alias record to write
 *********************************************************************/
static void
    write_alias (FILE *fp,
                 const alias_cb_t *alias)
{
    const xmlChar *qchar = NULL;
    switch (alias->quotes) {
    case 0:
        qchar = EMPTY_STRING;
        break;
    case 1:
        qchar = (const xmlChar *)"'";
        break;
    case 2:
        qchar = (const xmlChar *)"\"";
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    fprintf(fp, "%s=%s%s%s\n", alias->name, qchar,
            (alias->value) ? alias->value : EMPTY_STRING, qchar);

}  /* write_alias */


/********************************************************************
 * FUNCTION show_alias_name
 * 
 * Output 1 alias by name
 *
 * INPUTS:
 *    name == name of alias to show (not z-terminated)
 *    namelen == length of name
 *********************************************************************/
static void
    show_alias_name (const xmlChar *name,
                     uint32 namelen)
{
    alias_cb_t *alias = find_alias(name, namelen);

    if (alias) {
        show_alias_ptr(alias);
        log_write("\n");
    } else {
        uint32 i;
        log_error("\nError: alias '");
        for (i = 0; i < namelen; i++) {
            log_error("%c", name[i]);
        }
        log_error("' not found\n");
    }

}  /* show_alias_name */


/********************************************************************
 * FUNCTION parse_alias
 * 
 * Parse a string into its components
 *
 * INPUTS:
 *    parmstr == parameter string to parse
 *    namelen == address of return name length
 *    valstr == address of return value string pointer
 *
 * OUTPUTS:
 *    *namelen == return name length
 *    *valstr == return value string pointer (NULL if no value)
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    parse_alias (const xmlChar *parmstr,
                 uint32 *namelen,
                 const xmlChar **valstr)
{
    const xmlChar *p;
    boolean done;

    *namelen = 0;
    *valstr = NULL;

    /* check string for OK chars */
    p = parmstr;
    while (*p) {
        if (*p == '\n' || *p == '\t') {
            ;  // just skip this char; TBD: config param for this
        } else if (!isprint((int)*p)) {
            return ERR_NCX_INVALID_VALUE;
        }
        p++;
    }

    /* look for end of name */
    p = parmstr;
    if (!ncx_valid_fname_ch(*p++)) {
        return ERR_NCX_INVALID_NAME;
    }
    done = FALSE;
    while (*p && !done) {
        if (*p == '=') {
            done = TRUE;
        } else if (!ncx_valid_name_ch(*p)) {
            return ERR_NCX_INVALID_NAME;
        } else {
            p++;
        }
    }

    /* set output parms */
    *namelen = (uint32)(p - parmstr);
    if (done) {
        /* stopped at equals sign; check next char OK */
        if (p[1] == 0 || xml_isspace(p[1])) {
            return ERR_NCX_INVALID_VALUE;
        }
        *valstr = &p[1];
    } /* else stopped at end of valid name string */

    return NO_ERR;

}  /* parse_alias */


/********************************************************************
 * FUNCTION set_alias
 * 
 * Set an alias value field
 *
 * INPUTS:
 *    alias == alias record to use
 *    valstr == value string to use
 *
 * RETURNS:
 *    status
 *********************************************************************/
static status_t
    set_alias (alias_cb_t *alias,
               const xmlChar *valstr)
{
    status_t  res = NO_ERR;

    if (alias->value) {
        m__free(alias->value);
        alias->value = NULL;
    }
    alias->quotes = 0;
    if (valstr) {
        if (*valstr == '"' || *valstr == '\'') {
            /* preseve quotes used */
            alias->quotes = (*valstr == '"') ? 2 : 1;

            /* check trim quoted string */
            uint32 len = xml_strlen(valstr);
            if (len > 2) {
                const xmlChar *endstr = &valstr[len-1];
                if (*endstr == *valstr) {
                    /* remove paired quotes */
                    alias->value = xml_strndup(valstr+1, len-2);
                    if (alias->value == NULL) {
                        res = ERR_INTERNAL_MEM;
                    }
                } else {
                    /* improper string; unmatched quotes */
                    res = ERR_NCX_INVALID_VALUE;
                }
            } else {
                /* else got just a quote char */
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            alias->value = xml_strdup(valstr);
            if (alias->value == NULL) {
                res = ERR_INTERNAL_MEM;
            }
        }
    } /* else cleared value if not already */

    return res;

}  /* set_alias */


/********************************************************************
 * FUNCTION handle_alias_parm
 * 
 * alias def
 * alias def=def-value
 *
 * Handle the alias command, based on the parameter
 *
 * INPUTS:
 *    varstr == alias command line
 *    setonly == TRUE if expecting set version only; ignore show alias
 *               FALSE if expecting show alias or set alias
 *    loginfo == TRUE if log-level=info should be used
 *               FALSE if log-level=debug2 should be used
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    handle_alias_parm (const xmlChar *varstr,
                       boolean setonly,
                       boolean loginfo)
{
    const xmlChar *valptr = NULL;    
    uint32    nlen = 0;
    status_t  res;

    res = parse_alias(varstr, &nlen, &valptr);
    if (res == NO_ERR) {
        if (valptr) {
            /* setting an alias */
            alias_cb_t *alias = find_alias(varstr, nlen);
            if (alias) {
                if (LOGDEBUG3) {
                    log_debug3("\nAbout to replace alias '%s'"
                               "\n  old value: '%s'"
                               "\n  new value: '%s'",
                               alias->name, 
                               alias->value ? alias->value : EMPTY_STRING,
                               valptr);
                }
                /* modify an existing alias */
                res = set_alias(alias, valptr);
                if (res == NO_ERR) {
                    if (loginfo) {
                        log_info("\nUpdated alias '%s'\n", alias->name);
                    } else {
                        log_debug2("\nUpdated alias '%s'", alias->name);
                    }
                } else {
                    log_error("\nError: invalid alias value '%s'\n",
                              valptr);
                }
            } else {
                /* create a new alias */
                alias = new_alias(varstr, nlen);
                if (alias == NULL) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    res = set_alias(alias, valptr);
                    if (res == NO_ERR) {
                        res = add_alias(alias);
                        if (res == NO_ERR) {
                            if (loginfo) {
                                log_info("\nAdded alias '%s'\n", alias->name);
                            } else {
                                log_debug2("\nAdded alias '%s'", alias->name);
                            }
                        } else {
                            log_error("\nError: alias was not added '%s'\n",
                                      get_error_string(res));
                        }
                    } else {
                        log_error("\nError: invalid alias value '%s'\n",
                                  valptr);
                        free_alias(alias);
                    }
                }
            }
        } else if (!setonly) {
            /* just provided a name; show alias */
            show_alias_name(varstr, nlen);
        } else if (LOGDEBUG) {
            log_debug("\nSkipping alias '%s' because no value set", varstr);
        }
    } else if (res == ERR_NCX_INVALID_NAME) {
        log_error("\nError: invalid alias (%s)", get_error_string(res));
    } else {
        log_error("\nError: invalid alias '%s' (%s)", varstr,
                  get_error_string(res));
    }
    return res;

}  /* handle_alias_parm */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
 * FUNCTION do_alias (local RPC)
 * 
 * alias
 * alias def
 * alias def=def-value
 *
 * Handle the alias command, based on the parameter
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the alias command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_alias (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len)
{
    val_value_t        *valset, *parm;
    status_t            res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (res == NO_ERR && valset) {
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_VAR);
        if (parm) {
            const xmlChar *varstr = VAL_STR(parm);
            res = handle_alias_parm(varstr, FALSE, TRUE);
        } else {
            /* no parameter; show all aliases */
            show_aliases();
        }
    }

    if (valset) {
        val_free_value(valset);
    }

    return res;

}  /* do_alias */


/********************************************************************
 * FUNCTION do_aliases (local RPC)
 * 
 * aliases
 * aliases clear
 * aliases show
 * aliases load[=filespec]
 * aliases save[=filespec]
 *
 * Handle the aliases command, based on the parameter
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the aliases command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    do_aliases (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len)
{
    val_value_t   *valset;
    status_t       res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (res == NO_ERR && valset) {
        /* get the 1 of N 'alias-action' choice */
        val_value_t *parm;
        const xmlChar *parmval = NULL;
        boolean done = FALSE;
        
        /* aliases show */
        parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_SHOW);
        if (parm) {
            show_aliases();
            done = TRUE;
        }

        /* aliases clear */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_CLEAR);
            if (parm) {
                dlq_hdr_t *aliasQ = get_aliasQ();
                if (!dlq_empty(aliasQ)) {
                    free_aliases();
                    log_info("\nDeleted all aliases from memory\n");
                }else {
                    log_info("\nNo aliases found\n");
                }
                done = TRUE;
            }
        }

        /* aliases load */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_LOAD);
            if (parm) {
                if (xml_strlen(VAL_STR(parm))) {
                    parmval = VAL_STR(parm);
                } else {
                    parmval = get_aliases_file();
                }
                res = load_aliases(parmval);
                if (res == NO_ERR) {
                    log_info("\nLoaded aliases OK from '%s'\n", parmval);
                } else {
                    log_error("\nLoad aliases from '%s' failed (%s)\n", 
                              parmval, get_error_string(res));
                }
                done = TRUE;
            }
        }

        /* aliases save */
        if (!done) {
            parm = val_find_child(valset, YANGCLI_MOD, YANGCLI_SAVE);
            if (parm) {
                if (xml_strlen(VAL_STR(parm))) {
                    parmval = VAL_STR(parm);
                } else {
                    parmval = get_aliases_file();
                }
                res = save_aliases(parmval);
                if (res == NO_ERR) {
                    log_info("\nSaved aliases OK to '%s'\n", parmval);
                } else {
                    log_error("\nSave aliases to '%s' failed (%s)\n", 
                              parmval, get_error_string(res));
                }
                done = TRUE;
            }
        }

        if (!done) {
            /* no parameters; show all aliases */
            show_aliases();
        }
    }

    if (valset) {
        val_free_value(valset);
    }

    return res;

}  /* do_aliases */


/********************************************************************
 * FUNCTION do_unset (local RPC)
 * 
 * unset def
 *
 * Handle the unset command; remove the specified alias
 *
 * INPUTS:
 *    server_cb == server control block to use
 *    rpc == RPC method for the unset command
 *    line == CLI input in progress
 *    len == offset into line buffer to start parsing
 *
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t
    do_unset (server_cb_t *server_cb,
              obj_template_t *rpc,
              const xmlChar *line,
              uint32  len)
{
    val_value_t        *valset, *parm;
    status_t            res = NO_ERR;

    valset = get_valset(server_cb, rpc, &line[len], &res);

    if (res == NO_ERR && valset) {
        parm = val_find_child(valset, YANGCLI_MOD, NCX_EL_NAME);
        if (parm) {
            const xmlChar *varstr = VAL_STR(parm);
            alias_cb_t *alias = find_alias(varstr, xml_strlen(varstr));
            if (alias) {
                dlq_remove(alias);
                free_alias(alias);
                log_info("\nDeleted alias '%s'\n", varstr);
            } else {
                res = ERR_NCX_INVALID_VALUE;
                log_error("\nError: unknown alias '%s'\n", varstr);
            }
        }  /* else missing parameter already reported */
    } /* else no valset already reported */

    if (valset) {
        val_free_value(valset);
    }

    return res;

}  /* do_unset */


/********************************************************************
 * FUNCTION show_aliases
 * 
 * Output all the alias values
 *
 *********************************************************************/
void
    show_aliases (void)
{
    alias_cb_t  *alias;
    boolean      anyout = FALSE;

    for (alias = get_first_alias();
         alias != NULL;
         alias = get_next_alias(alias)) {
        show_alias_ptr(alias);
        anyout = TRUE;
    }
    if (anyout) {
        log_write("\n");
    }

}  /* show_aliases */


/********************************************************************
 * FUNCTION show_alias
 * 
 * Output 1 alias by name
 *
 * INPUTS:
 *    name == name of alias to show
 *********************************************************************/
void
    show_alias (const xmlChar *name)
{
    show_alias_name(name, xml_strlen(name));

}  /* show_alias */


/********************************************************************
 * FUNCTION free_aliases
 * 
 * Free all the command aliases
 *
 *********************************************************************/
void
    free_aliases (void)
{
    dlq_hdr_t   *aliasQ = get_aliasQ();
    alias_cb_t  *alias;

    while (!dlq_empty(aliasQ)) {
        alias = (alias_cb_t *)dlq_deque(aliasQ);
        free_alias(alias);
    }

}  /* free_aliases */


/********************************************************************
 * FUNCTION load_aliases
 * 
 * Load the aliases from the specified filespec
 *
 * INPUT:
 *   fspec == input filespec to use (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    load_aliases (const xmlChar *fspec)
{
    FILE      *fp;
    xmlChar   *fullspec, *buffer;
    status_t   res = NO_ERR;
    
    buffer = m__getMem(NCX_MAX_LINELEN+1);
    if (buffer == NULL) {
        log_error("\nError: malloc failed\n");
        return ERR_INTERNAL_MEM;
    }

    if (fspec == NULL) {
        fspec = get_aliases_file();
    }

    fullspec = ncx_get_source(fspec, &res);
    if (res == NO_ERR && fullspec) {
        fp = fopen((const char *)fullspec, "r");
        if (fp) {
#define MAX_ALIAS_ERRORS 5
            boolean done = FALSE;
            uint32 errorcnt = 0;
            while (!done) {
                char *retstr = fgets((char *)buffer, NCX_MAX_LINELEN+1, fp);
                if (retstr) {
                    uint32 len = xml_strlen(buffer);
                    if (len) {
                        if (*buffer != '#' && *buffer != '\n') {
                            if (buffer[len-1] == '\n') {
                                buffer[len-1] = 0;
                            } 
                            res = handle_alias_parm(buffer, TRUE, FALSE);
                            if (res != NO_ERR) {
                                if (++errorcnt == MAX_ALIAS_ERRORS) {
                                    log_error("\nError: skipping aliases; "
                                              "too many errors\n");
                                    done = TRUE;
                                }
                            }
                        }  // else skip a newline or comment line
                    } // else skip empty line
                } else {
                    done = TRUE;
                }
            }
            fclose(fp);
        } else if (LOGDEBUG) {
            log_debug("\nAliases file '%s' could not be opened\n", fullspec);
        }
    } else {
        log_error("\nError: Expand source '%s' failed (%s)",
                  fspec, get_error_string(res));
    }

    if (fullspec) {
        m__free(fullspec);
    }

    if (buffer) {
        m__free(buffer);
    }
    return res;

}  /* load_aliases */


/********************************************************************
 * FUNCTION save_aliases
 * 
 * Save the aliases to the specified filespec
 *
 * INPUT:
 *   fspec == output filespec to use  (NULL == default)
 *
 * RETURNS:
 *   status
 *********************************************************************/
status_t
    save_aliases (const xmlChar *fspec)
{
    xmlChar     *fullspec;
    FILE        *fp = NULL;
    status_t     res = NO_ERR;

    if (fspec == NULL) {
        fspec = get_aliases_file();
    }

    fullspec = ncx_get_source(fspec, &res);
    if (res == NO_ERR && fullspec) {
        fp = fopen((const char *)fullspec, "w");
        if (fp) {
            /* this will truncate the file if there are no aliases */
            alias_cb_t  *alias;
            for (alias = get_first_alias();
                 alias != NULL;
                 alias = get_next_alias(alias)) {
                write_alias(fp, alias);
            }
            fclose(fp);
        } else {
            res = errno_to_status();
            log_error("\nError: Open aliases file '%s' failed (%s)\n",
                      fullspec, get_error_string(res));
        }
    } else {
        log_error("\nError: Expand source '%s' failed (%s)\n",
                  fspec, get_error_string(res));
    }

    if (fullspec) {
        m__free(fullspec);
    }

    return res;

}  /* save_aliases */


/********************************************************************
 * FUNCTION expand_alias
 * 
 * Check if the first token is an alias name
 * If so, construct a new command line with the alias contents
 *
 * INPUT:
 *   line == command line to check and possibly convert
 *   res == address of return status
 *
 * OUTPUTS:
 *   *res == return status (ERR_NCX_SKIPPED if nothing done)
 *
 * RETURNS:
 *   pointer to malloced command string if *res==NO_ERR
 *   NULL if *res==ERR_NCX_SKIPPED or some real error
 *********************************************************************/
xmlChar *
    expand_alias (xmlChar *line,
                  status_t *res)
{
    xmlChar  *start, *p = line, *newline;
    alias_cb_t *alias;
    uint32    namelen, newlen;
    boolean   done = FALSE;

    /* skip any leading whitespace; not expected from yangcli */
    while (*p && xml_isspace(*p)) {
        p++;
    }
    if (*p == 0) {
        *res = ERR_NCX_SKIPPED;
        return NULL;
    }

    /* look for end of name */
    start = p;
    if (!ncx_valid_fname_ch(*p++)) {
        *res = ERR_NCX_SKIPPED;
        return NULL;
    }
    while (*p && !done) {
        if (xml_isspace(*p)) {
            done = TRUE;
        } else if (!ncx_valid_name_ch(*p)) {
            *res = ERR_NCX_SKIPPED;
            return NULL;
        } else {
            p++;
        }
    }

    /* look for the alias */
    namelen = (uint32)(p - start);
    alias = find_alias(start, namelen);
    if (alias == NULL) {
        *res = ERR_NCX_SKIPPED;
        return NULL;
    }

    /* replace the alias name with its contents and make a new string */
    if (alias->value) {
        newlen = xml_strlen(p) + xml_strlen(alias->value);
    } else {
        newlen = xml_strlen(p);
    }
    newline = m__getMem(newlen + 1);
    if (newline == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    start = newline;
    if (alias->value) {
        start += xml_strcpy(start, alias->value);
    }
    xml_strcpy(start, p);

    if (LOGDEBUG2) {
        log_debug2("\nExpanded alias '%s'; new line: '%s'",
                   alias->name, newline);
    }

    *res = NO_ERR;
    return newline;

}  /* expand_alias */


/* END yangcli_alias.c */
