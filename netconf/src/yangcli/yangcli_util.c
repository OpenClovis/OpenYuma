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
/*  FILE: yangcli_util.c

   Utilities for NETCONF YANG-based CLI Tool

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
01-jun-08    abb      begun; started from ncxcli.c
27-mar-09    abb      split out from yangcli.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "libtecla.h"

#include "procdefs.h"
#include "log.h"
#include "mgr.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "runstack.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "var.h"
#include "xmlns.h"
#include "xml_util.h"
#include "xml_val.h"
#include "xpath.h"
#include "xpath_yang.h"
#include "yangconst.h"
#include "yangcli.h"
#include "yangcli_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION is_top_command
* 
* Check if command name is a top command
* Must be full name
*
* INPUTS:
*   rpcname == command name to check
*
* RETURNS:
*   TRUE if this is a top command
*   FALSE if not
*********************************************************************/
boolean
    is_top_command (const xmlChar *rpcname)
{
#ifdef DEBUG
    if (!rpcname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!xml_strcmp(rpcname, YANGCLI_ALIAS)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_ALIASES)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_CD)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_CONNECT)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_EVAL)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_EVENTLOG)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_ELIF)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_ELSE)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_END)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_FILL)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_HELP)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_IF)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_HISTORY)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_LIST)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_LOG_ERROR)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_LOG_WARN)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_LOG_INFO)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_LOG_DEBUG)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_MGRLOAD)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_PWD)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_QUIT)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_RECALL)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_RUN)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_SHOW)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_WHILE)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_UNSET)) {
        ;
    } else if (!xml_strcmp(rpcname, YANGCLI_USERVARS)) {
        ;
    } else {
        return FALSE;
    }
    return TRUE;

}  /* is_top_command */


/********************************************************************
* FUNCTION new_modptr
* 
*  Malloc and init a new module pointer block
* 
* INPUTS:
*    mod == module to cache in this struct
*    malloced == TRUE if mod is malloced
*                FALSE if mod is a back-ptr
*    feature_list == feature list from capability
*    deviation_list = deviations list from capability
*
* RETURNS:
*   malloced modptr_t struct or NULL of malloc failed
*********************************************************************/
modptr_t *
    new_modptr (ncx_module_t *mod,
                ncx_list_t *feature_list,
                ncx_list_t *deviation_list)
{
    modptr_t  *modptr;

#ifdef DEBUG
    if (!mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    modptr = m__getObj(modptr_t);
    if (!modptr) {
        return NULL;
    }
    memset(modptr, 0x0, sizeof(modptr_t));
    modptr->mod = mod;
    modptr->feature_list = feature_list;
    modptr->deviation_list = deviation_list;

    return modptr;

}  /* new_modptr */


/********************************************************************
* FUNCTION free_modptr
* 
*  Clean and free a module pointer block
* 
* INPUTS:
*    modptr == mod pointer block to free
*              MUST BE REMOVED FROM ANY Q FIRST
*
*********************************************************************/
void
    free_modptr (modptr_t *modptr)
{
#ifdef DEBUG
    if (!modptr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    m__free(modptr);

}  /* free_modptr */


/********************************************************************
* FUNCTION find_modptr
* 
*  Find a specified module name
* 
* INPUTS:
*    modptrQ == Q of modptr_t to check
*    modname == module name to find
*    revision == module revision (may be NULL)
*
* RETURNS:
*   pointer to found entry or NULL if not found
*
*********************************************************************/
modptr_t *
    find_modptr (dlq_hdr_t *modptrQ,
                 const xmlChar *modname,
                 const xmlChar *revision)
{
    modptr_t  *modptr;

#ifdef DEBUG
    if (!modptrQ || !modname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (modptr = (modptr_t *)dlq_firstEntry(modptrQ);
         modptr != NULL;
         modptr = (modptr_t *)dlq_nextEntry(modptr)) {

        if (xml_strcmp(modptr->mod->name, modname)) {
            continue;
        }

        if (revision && 
            modptr->mod->version &&
            !xml_strcmp(modptr->mod->version, revision)) {
            return modptr;
        }
        if (revision == NULL) {
            return modptr;
        }
    }
    return NULL;
    
}  /* find_modptr */


/********************************************************************
* FUNCTION clear_server_cb_session
* 
*  Clean the current session data from an server control block
* 
* INPUTS:
*    server_cb == control block to use for clearing
*                the session data
*********************************************************************/
void
    clear_server_cb_session (server_cb_t *server_cb)
{
    modptr_t  *modptr;

#ifdef DEBUG
    if (!server_cb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* get rid of the val->obj pointers that reference
     * server-specific object trees that have been freed
     * already by mgr_ses_free_session
     */
    runstack_session_cleanup(server_cb->runstack_context);

    while (!dlq_empty(&server_cb->modptrQ)) {
        modptr = (modptr_t *)dlq_deque(&server_cb->modptrQ);
        free_modptr(modptr);
    }
    server_cb->mysid = 0;
    server_cb->state = MGR_IO_ST_IDLE;

    if (server_cb->connect_valset) {
        val_free_value(server_cb->connect_valset);
        server_cb->connect_valset = NULL;
    }

}  /* clear_server_cb_session */


/********************************************************************
* FUNCTION is_top
* 
* Check the state and determine if the top or conn
* mode is active
* 
* INPUTS:
*   server state to use
*
* RETURNS:
*  TRUE if this is TOP mode
*  FALSE if this is CONN mode (or associated states)
*********************************************************************/
boolean
    is_top (mgr_io_state_t state)
{
    switch (state) {
    case MGR_IO_ST_INIT:
    case MGR_IO_ST_IDLE:
        return TRUE;
    case MGR_IO_ST_CONNECT:
    case MGR_IO_ST_CONN_START:
    case MGR_IO_ST_SHUT:
    case MGR_IO_ST_CONN_IDLE:
    case MGR_IO_ST_CONN_RPYWAIT:
    case MGR_IO_ST_CONN_CANCELWAIT:
    case MGR_IO_ST_CONN_CLOSEWAIT:
    case MGR_IO_ST_CONN_SHUT:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

}  /* is_top */


/********************************************************************
* FUNCTION use_servercb
* 
* Check if the server_cb should be used for modules right now
*
* INPUTS:
*   server_cb == server control block to check
*
* RETURNS:
*   TRUE to use server_cb
*   FALSE if not
*********************************************************************/
boolean
    use_servercb (server_cb_t *server_cb)
{
    if (!server_cb || is_top(server_cb->state)) {
        return FALSE;
    } else if (dlq_empty(&server_cb->modptrQ)) {
        return FALSE;
    }
    return TRUE;
}  /* use_servercb */


/********************************************************************
* FUNCTION find_module
* 
*  Check the server_cb for the specified module; if not found
*  then try ncx_find_module
* 
* INPUTS:
*    server_cb == control block to free
*    modname == module name
*
* RETURNS:
*   pointer to the requested module
*      using the registered 'current' version
*   NULL if not found
*********************************************************************/
ncx_module_t *
    find_module (server_cb_t *server_cb,
                 const xmlChar *modname)
{
    modptr_t      *modptr;
    ncx_module_t  *mod;

#ifdef DEBUG
    if (!modname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (use_servercb(server_cb)) {
        for (modptr = (modptr_t *)dlq_firstEntry(&server_cb->modptrQ);
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            if (!xml_strcmp(modptr->mod->name, modname)) {
                return modptr->mod;
            }
        }
    }

    mod = ncx_find_module(modname, NULL);

    return mod;

}  /* find_module */


/********************************************************************
* FUNCTION get_strparm
* 
* Get the specified string parm from the parmset and then
* make a strdup of the value
*
* INPUTS:
*   valset == value set to check if not NULL
*   modname == module defining parmname
*   parmname  == name of parm to get
*
* RETURNS:
*   pointer to string !!! THIS IS A MALLOCED COPY !!!
*********************************************************************/
xmlChar *
    get_strparm (val_value_t *valset,
                 const xmlChar *modname,
                 const xmlChar *parmname)
{
    val_value_t    *parm;
    xmlChar        *str;

#ifdef DEBUG
    if (!valset || !parmname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    
    str = NULL;
    parm = findparm(valset, modname, parmname);
    if (parm) {
        str = xml_strdup(VAL_STR(parm));
        if (!str) {
            log_error("\nyangcli: Out of Memory error");
        }
    }
    return str;

}  /* get_strparm */


/********************************************************************
* FUNCTION findparm
* 
* Get the specified string parm from the parmset and then
* make a strdup of the value
*
* INPUTS:
*   valset == value set to search
*   modname == optional module name defining the parameter to find
*   parmname  == name of parm to get, or partial name to get
*
* RETURNS:
*   pointer to val_value_t if found
*********************************************************************/
val_value_t *
    findparm (val_value_t *valset,
              const xmlChar *modname,
              const xmlChar *parmname)
{
    val_value_t *parm;

#ifdef DEBUG
    if (!parmname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!valset) {
        return NULL;
    }

    parm = val_find_child(valset, modname, parmname);
    if (!parm && get_autocomp()) {
        parm = val_match_child(valset, modname, parmname);
    }
    return parm;

}  /* findparm */


/********************************************************************
* FUNCTION add_clone_parm
* 
*  Create a parm 
* 
* INPUTS:
*   val == value to clone and add
*   valset == value set to add parm into
*
* RETURNS:
*    status
*********************************************************************/
status_t
    add_clone_parm (const val_value_t *val,
                    val_value_t *valset)
{
    val_value_t    *parm;

#ifdef DEBUG
    if (!val || !valset) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    parm = val_clone(val);
    if (!parm) {
        log_error("\nyangcli: val_clone failed");
        return ERR_INTERNAL_MEM;
    } else {
        val_add_child(parm, valset);
    }
    return NO_ERR;

}  /* add_clone_parm */


/********************************************************************
* FUNCTION is_yangcli_ns
* 
*  Check the namespace and make sure this is an YANGCLI command
* 
* INPUTS:
*   ns == namespace ID to check
*
* RETURNS:
*  TRUE if this is the YANGCLI namespace ID
*********************************************************************/
boolean
    is_yangcli_ns (xmlns_id_t ns)
{
    const xmlChar *modname;

    modname = xmlns_get_module(ns);
    if (modname && !xml_strcmp(modname, YANGCLI_MOD)) {
        return TRUE;
    } else {
        return FALSE;
    }

}  /* is_yangcli_ns */


/********************************************************************
 * FUNCTION clear_result
 * 
 * clear out the pending result info
 *
 * INPUTS:
 *   server_cb == server control block to use
 *
 *********************************************************************/
void
    clear_result (server_cb_t *server_cb)

{
#ifdef DEBUG
    if (!server_cb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (server_cb->local_result) {
        val_free_value(server_cb->local_result);
        server_cb->local_result = NULL;
    }
    if (server_cb->result_name) {
        m__free(server_cb->result_name);
        server_cb->result_name = NULL;
    }
    if (server_cb->result_filename) {
        m__free(server_cb->result_filename);
        server_cb->result_filename = NULL;
    }
    server_cb->result_vartype = VAR_TYP_NONE;
    server_cb->result_format = RF_NONE;

}  /* clear_result */


/********************************************************************
* FUNCTION check_filespec
* 
* Check the filespec string for a file assignment statement
* Save it if it si good
*
* INPUTS:
*    server_cb == server control block to use
*    filespec == string to check
*    varname == variable name to use in log_error
*              if this is complex form
*
* OUTPUTS:
*    server_cb->result_filename will get set if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
status_t
    check_filespec (server_cb_t *server_cb,
                    const xmlChar *filespec,
                    const xmlChar *varname)
{
    xmlChar       *newstr;
    const xmlChar *teststr;
    status_t       res;
#ifdef DEBUG
    if (!server_cb || !filespec) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (!*filespec) {
        if (varname) {
            log_error("\nError: file assignment variable '%s' "
                      "is empty string", varname);
        } else {
            log_error("\nError: file assignment filespec "
                      "is empty string");
        }
        return ERR_NCX_INVALID_VALUE;
    }

    /* variable must be a string with only
     * valid filespec chars in it; no spaces
     * are allowed; too many security holes
     * if arbitrary strings are allowed here
     */
    if (val_need_quotes(filespec)) {
        if (varname) {
            log_error("\nError: file assignment variable '%s' "
                      "contains whitespace (%s)", 
                      varname, filespec);
        } else {
            log_error("\nError: file assignment filespec '%s' "
                      "contains whitespace", filespec);
        }
        return ERR_NCX_INVALID_VALUE;
    }

    /* check for acceptable chars */
    res = NO_ERR;
    newstr = ncx_get_source_ex(filespec, FALSE, &res);
    if (newstr == NULL || res != NO_ERR) {
        log_error("\nError: get source for '%s' failed (%s)",
                  filespec, res);
        if (newstr != NULL) {
            m__free(newstr);
        }
        return res;
    }

    teststr = newstr;
    while (*teststr) {
        if (*teststr == NCXMOD_PSCHAR ||
            *teststr == '.' ||
#ifdef WINDOWS
            *teststr == ':' ||
#endif
            ncx_valid_name_ch(*teststr)) {
            teststr++;
        } else {
            if (varname) {
                log_error("\nError: file assignment variable '%s' "
                          "contains invalid filespec (%s)", 
                          varname, filespec);
            } else {
                log_error("\nError: file assignment filespec '%s' "
                          "contains invalid filespec", filespec);
            }
            m__free(newstr);
            return ERR_NCX_INVALID_VALUE;
        }
    }

    /* toss out the old value, if any */
    if (server_cb->result_filename) {
        m__free(server_cb->result_filename);
    }

    /* save the filename, may still be an invalid fspec
     * pass off newstr memory here
     */
    server_cb->result_filename = newstr;
    if (!server_cb->result_filename) {
        return ERR_INTERNAL_MEM;
    }
    return NO_ERR;

}  /* check_filespec */


/********************************************************************
 * FUNCTION get_instanceid_parm
 * 
 * Validate an instance identifier parameter
 * Return the target object
 * Return a value struct from root containing
 * all the predicate assignments in the stance identifier
 *
 * INPUTS:
 *    target == XPath expression for the instance-identifier
 *    schemainst == TRUE if ncx:schema-instance string
 *                  FALSE if instance-identifier
 *    configonly == TRUE if there should be an error given
 *                  if the target does not point to a config node
 *                  FALSE if the target can be config false
 *    targobj == address of return target object for this expr
 *    targval == address of return pointer to target value
 *               node within the value subtree returned
 *    retres == address of return status
 *
 * OUTPUTS:
 *    *targobj == the object template for the target
 *    *targval == the target node within the returned subtree
 *                from root
 *    *retres == return status for the operation
 *
 * RETURNS:
 *   If NO_ERR:
 *     malloced value node representing the instance-identifier
 *     from root to the targobj
 *  else:
 *    NULL, check *retres
 *********************************************************************/
val_value_t *
    get_instanceid_parm (const xmlChar *target,
                         boolean schemainst,
                         boolean configonly,
                         obj_template_t **targobj,
                         val_value_t **targval,
                         status_t *retres)
{
    xpath_pcb_t           *xpathpcb;
    val_value_t           *retval;
    status_t               res;

#ifdef DEBUG
    if (!target || !targobj || !targval || !retres) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *targobj = NULL;
    *targval = NULL;
    *retres = NO_ERR;

    /* get a parser block for the instance-id */
    xpathpcb = xpath_new_pcb(target, NULL);
    if (!xpathpcb) {
        log_error("\nError: malloc failed");
        *retres = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* initial parse into a token chain
     * this is only for parsing leafref paths! 
     */
    res = xpath_yang_parse_path(NULL, 
                                NULL, 
                                (schemainst) ?
                                XP_SRC_SCHEMA_INSTANCEID :
                                XP_SRC_INSTANCEID,
                                xpathpcb);
    if (res != NO_ERR) {
        log_error("\nError: parse XPath target '%s' failed",
                  xpathpcb->exprstr);
        xpath_free_pcb(xpathpcb);
        *retres = res;
        return NULL;
    }

    /* validate against the object tree */
    res = xpath_yang_validate_path(NULL, 
                                   ncx_get_gen_root(),
                                   xpathpcb,
                                   schemainst,
                                   targobj);
    if (res != NO_ERR) {
        log_error("\nError: validate XPath target '%s' failed",
                  xpathpcb->exprstr);
        xpath_free_pcb(xpathpcb);
        *retres = res;
        return NULL;
    }

    /* check if the target is a config node or not
     * TBD: check the baddata parm and possibly allow
     * the target of a write to be a config=false for
     * server testing purposes
     */
    if (configonly && !obj_get_config_flag(*targobj)) {
        log_error("\nError: XPath target '%s' is not a config=true node",
                  xpathpcb->exprstr);
        xpath_free_pcb(xpathpcb);
        *retres = ERR_NCX_ACCESS_READ_ONLY;
        return NULL;
    }
    
    /* have a valid target object, so follow the
     * parser chain and build a value subtree
     * from the XPath expression
     */
    retval = xpath_yang_make_instanceid_val(xpathpcb, 
                                            &res,
                                            targval);

    xpath_free_pcb(xpathpcb);
    *retres = res;

    return retval;

} /* get_instanceid_parm */


/********************************************************************
* FUNCTION get_file_result_format
* 
* Check the filespec string for a file assignment statement
* to see if it is text, XML, or JSON
*
* INPUTS:
*    filespec == string to check
*
* RETURNS:
*   result format enumeration; RF_NONE if some error
*********************************************************************/
result_format_t
    get_file_result_format (const xmlChar *filespec)
{
    const xmlChar *teststr;
    uint32         len;

#ifdef DEBUG
    if (!filespec) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return RF_NONE;
    }
#endif

    len = xml_strlen(filespec);
    if (len < 5) {
        return RF_TEXT;
    }

    teststr = &filespec[len-1];

    while (teststr > filespec && *teststr != '.') {
        teststr--;
    }

    if (teststr == filespec) {
        return RF_TEXT;
    }

    teststr++;

    if (!xml_strcmp(teststr, NCX_EL_XML)) {
        return RF_XML;
    }

    if (!xml_strcmp(teststr, NCX_EL_JSON)) {
        return RF_JSON;
    }

    if (!xml_strcmp(teststr, NCX_EL_YANG)) {
        return RF_TEXT;
    }

    if (!xml_strcmp(teststr, NCX_EL_TXT)) {
        return RF_TEXT;
    }

    if (!xml_strcmp(teststr, NCX_EL_TEXT)) {
        return RF_TEXT;
    }

    if (!xml_strcmp(teststr, NCX_EL_LOG)) {
        return RF_TEXT;
    }

    return RF_TEXT;  // default to text instead of error!

}  /* get_file_result_format */


/********************************************************************
* FUNCTION interactive_mode
* 
*  Check if the program is in interactive mode
* 
* RETURNS:
*   TRUE if insteractive mode, FALSE if batch mode
*********************************************************************/
boolean
    interactive_mode (void)
{
    return get_batchmode() ? FALSE : TRUE;

}  /* interactive_mode */


/********************************************************************
 * FUNCTION init_completion_state
 * 
 * init the completion_state struct for a new command
 *
 * INPUTS:
 *    completion_state == record to initialize
 *    server_cb == server control block to use
 *    cmdstate ==initial  calling state
 *********************************************************************/
void
    init_completion_state (completion_state_t *completion_state,
                           server_cb_t *server_cb,
                           command_state_t  cmdstate)
{
#ifdef DEBUG
    if (!completion_state) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(completion_state, 
           0x0, 
           sizeof(completion_state_t));
    completion_state->server_cb = server_cb;
    completion_state->cmdstate = cmdstate;

}  /* init_completion_state */


/********************************************************************
 * FUNCTION set_completion_state
 * 
 * set the completion_state struct for a new mode or sub-command
 *
 * INPUTS:
 *    completion_state == record to set
 *    rpc == rpc operation in progress (may be NULL)
 *    parm == parameter being filled in
 *    cmdstate ==current calling state
 *********************************************************************/
void
    set_completion_state (completion_state_t *completion_state,
                          obj_template_t *rpc,
                          obj_template_t *parm,
                          command_state_t  cmdstate)
{
#ifdef DEBUG
    if (!completion_state) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    completion_state->cmdstate = cmdstate;
    completion_state->cmdobj = rpc;
    if (rpc) {
        completion_state->cmdinput =
            obj_find_child(rpc, NULL, YANG_K_INPUT);
    } else {
        completion_state->cmdinput = NULL;
    }
    completion_state->cmdcurparm = parm;

}  /* set_completion_state */


/********************************************************************
 * FUNCTION set_completion_state_curparm
 * 
 * set the current parameter in the completion_state struct
 *
 * INPUTS:
 *    completion_state == record to set
 *    parm == parameter being filled in
 *********************************************************************/
void
    set_completion_state_curparm (completion_state_t *completion_state,
                                  obj_template_t *parm)
{
#ifdef DEBUG
    if (!completion_state) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    completion_state->cmdcurparm = parm;

}  /* set_completion_state_curparm */



/********************************************************************
* FUNCTION xpath_getvar_fn
 *
 * see ncx/xpath.h -- matches xpath_getvar_fn_t template
 *
 * Callback function for retrieval of a variable binding
 * 
 * INPUTS:
 *   pcb   == XPath parser control block in use
 *   varname == variable name requested
 *   res == address of return status
 *
 * OUTPUTS:
 *  *res == return status
 *
 * RETURNS:
 *    pointer to the ncx_var_t data structure
 *    for the specified varbind
*********************************************************************/
ncx_var_t *
    xpath_getvar_fn (struct xpath_pcb_t_ *pcb,
                     const xmlChar *varname,
                     status_t *res)
{
    ncx_var_t           *retvar;
    runstack_context_t  *rcxt;

#ifdef DEBUG
    if (varname == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* if the runstack context is not set then the default
     * context will be used
     */
    rcxt = (runstack_context_t *)pcb->cookie;
    retvar = var_find(rcxt, varname, 0);
    if (retvar == NULL) {
        *res = ERR_NCX_DEF_NOT_FOUND;
    } else {
        *res = NO_ERR;
    }

    return retvar;

}  /* xpath_getvar_fn */


/********************************************************************
* FUNCTION get_netconf_mod
* 
*  Get the netconf module
*
* INPUTS:
*   server_cb == server control block to use
* 
* RETURNS:
*    netconf module
*********************************************************************/
ncx_module_t *
    get_netconf_mod (server_cb_t *server_cb)
{
    ncx_module_t  *mod;

#ifdef DEBUG
    if (server_cb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    mod = find_module(server_cb, NCXMOD_YUMA_NETCONF);
    return mod;

}  /* get_netconf_mod */


/********************************************************************
* FUNCTION clone_old_parm
* 
*  Clone a parameter value from the 'old' value set
*  if it exists there, and add it to the 'new' value set
*  only if the new value set does not have this parm
*
* The old and new pvalue sets must be complex types 
*  NCX_BT_LIST, NCX_BT_CONTAINER, or NCX_BT_ANYXML
*
* INPUTS:
*   oldvalset == value set to copy from
*   newvalset == value set to copy into
*   parm == object template to find and copy
*
* RETURNS:
*  status
*********************************************************************/
status_t
    clone_old_parm (val_value_t *oldvalset,
                    val_value_t *newvalset,
                    obj_template_t *parm)
{
    val_value_t  *findval, *newval;

#ifdef DEBUG
    if (oldvalset == NULL || newvalset == NULL || parm == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!typ_has_children(oldvalset->btyp)) {
        return ERR_NCX_INVALID_VALUE;
    }
    if (!typ_has_children(newvalset->btyp)) {
        return ERR_NCX_INVALID_VALUE;
    }
#endif

    findval = val_find_child(newvalset,
                             obj_get_mod_name(parm),
                             obj_get_name(parm));
    if (findval != NULL) {
        return NO_ERR;
    }

    findval = val_find_child(oldvalset,
                             obj_get_mod_name(parm),
                             obj_get_name(parm));
    if (findval == NULL) {
        return NO_ERR;
    }

    newval = val_clone(findval);
    if (newval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    val_add_child(newval, newvalset);
    return NO_ERR;

}  /* clone_old_parm */


/* END yangcli_util.c */
