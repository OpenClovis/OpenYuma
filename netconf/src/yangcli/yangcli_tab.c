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
/*  FILE: yangcli_tab.c

   NETCONF YANG-based CLI Tool

   See ./README for details

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
18-apr-09    abb      begun; split out from yangcli.c
25-jan-12    abb      Add xpath tab completion provided by Zesi Cai
 
*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
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
#include "ncx.h"
#include "ncxtypes.h"
#include "obj.h"
#include "rpc.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "var.h"
#include "xml_util.h"
#include "yangcli.h"
#include "yangcli_cmd.h"
#include "yangcli_tab.h"
#include "yangcli_util.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* make sure this is not enabled in the checked in version
 * will mess up STDOUT diplsay with debug messages!!!
 */
/* #define YANGCLI_TAB_DEBUG 1 */
/* #define DEBUG_TRACE 1 */


/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*             F O R W A R D   D E C L A R A T I O N S               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
 * FUNCTION cpl_add_completion
 *
 *  COPIED FROM /usr/local/include/libtecla.h
 *
 *  Used by libtecla to store one completion possibility
 *  for the current command line in progress
 * 
 *  cpl      WordCompletion *  The argument of the same name that was passed
 *                             to the calling CPL_MATCH_FN() callback function.
 *  line         const char *  The input line, as received by the callback
 *                             function.
 *  word_start          int    The index within line[] of the start of the
 *                             word that is being completed. If an empty
 *                             string is being completed, set this to be
 *                             the same as word_end.
 *  word_end            int    The index within line[] of the character which
 *                             follows the incomplete word, as received by the
 *                             callback function.
 *  suffix       const char *  The appropriately quoted string that could
 *                             be appended to the incomplete token to complete
 *                             it. A copy of this string will be allocated
 *                             internally.
 *  type_suffix  const char *  When listing multiple completions, gl_get_line()
 *                             appends this string to the completion to indicate
 *                             its type to the user. If not pertinent pass "".
 *                             Otherwise pass a literal or static string.
 *  cont_suffix  const char *  If this turns out to be the only completion,
 *                             gl_get_line() will append this string as
 *                             a continuation. For example, the builtin
 *                             file-completion callback registers a directory
 *                             separator here for directory matches, and a
 *                             space otherwise. If the match were a function
 *                             name you might want to append an open
 *                             parenthesis, etc.. If not relevant pass "".
 *                             Otherwise pass a literal or static string.
 * Output:
 *  return              int    0 - OK.
 *                             1 - Error.
 */



/********************************************************************
 * FUNCTION fill_one_parm_completion
 * 
 * fill the command struct for one RPC parameter value
 * check all the parameter values that match, if possible
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state to use
 *    line == line passed to callback
 *    parmval == parm value string to check
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    parmlen == length of parameter value already entered
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_parm_completion (WordCompletion *cpl,
                              completion_state_t *comstate,
                              const char *line,
                              const char *parmval,
                              int word_start,
                              int word_end,
                              int parmlen)


{
    const char  *endchar;
    int          retval;

    /* check no partial match so need to skip this one */
    if (parmlen > 0 &&
        strncmp(parmval,
                &line[word_start],
                parmlen)) {
        /* parameter value start is not the same so skip it */
        return NO_ERR;
    }

    if (comstate->cmdstate == CMD_STATE_FULL) {
        endchar = " ";
    } else {
        endchar = "";
    }

    retval = cpl_add_completion(cpl, line, word_start, word_end,
                           (const char *)&parmval[parmlen],
                           (const char *)"", endchar);

    if (retval != 0) {
        return ERR_NCX_OPERATION_FAILED;
    }

    return NO_ERR;

}  /* fill_one_parm_completion */


/********************************************************************
 * FUNCTION fill_parm_completion
 * 
 * fill the command struct for one RPC parameter value
 * check all the parameter values that match, if possible
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    parmobj == RPC input parameter template to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state record to use
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    parmlen == length of parameter name already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_parm_completion (obj_template_t *parmobj,
                          WordCompletion *cpl,
                          completion_state_t *comstate,
                          const char *line,
                          int word_start,
                          int word_end,
                          int parmlen)
{
    const char            *defaultstr;
    typ_enum_t            *typenum;
    typ_def_t             *typdef, *basetypdef;
    ncx_btype_t            btyp;
    status_t               res;

#ifdef YANGCLI_TAB_DEBUG
    log_debug2("\n*** fill parm %s ***\n", obj_get_name(parmobj));
#endif

    typdef = obj_get_typdef(parmobj);
    if (typdef == NULL) {
        return NO_ERR;
    }

    res = NO_ERR;
    basetypdef = typ_get_base_typdef(typdef);
    btyp = obj_get_basetype(parmobj);

    switch (btyp) {
    case NCX_BT_ENUM:
    case NCX_BT_BITS:
        for (typenum = typ_first_enumdef(basetypdef);
             typenum != NULL && res == NO_ERR;
             typenum = typ_next_enumdef(typenum)) {

#ifdef YANGCLI_TAB_DEBUG
            log_debug2("\n*** found enubit  %s ***\n", typenum->name);
#endif

            res = fill_one_parm_completion(cpl, comstate, line,
                                           (const char *)typenum->name,
                                           word_start, word_end, parmlen);
        }
        return res;
    case NCX_BT_BOOLEAN:
        res = fill_one_parm_completion(cpl, comstate, line,
                                       (const char *)NCX_EL_TRUE,
                                       word_start, word_end, parmlen);
        if (res == NO_ERR) {
            res = fill_one_parm_completion(cpl, comstate, line,
                                           (const char *)NCX_EL_FALSE,
                                           word_start, word_end, parmlen);
        }
        return res;
    case NCX_BT_INSTANCE_ID:
        break;
    case NCX_BT_IDREF:
        break;
    default:
        break;
    }

    /* no current values to show;
     * just use the default if any
     */
    defaultstr = (const char *)obj_get_default(parmobj);
    if (defaultstr) {
        res = fill_one_parm_completion(cpl, comstate, line, defaultstr,
                                       word_start, word_end, parmlen);
    }

    return NO_ERR;

}  /* fill_parm_completion */


/********************************************************************
 * FUNCTION fill_xpath_children_completion
 * 
 * fill the command struct for one XPath child node
 *
 * INPUTS:
 *    parentObj == object template of parent to check
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_xpath_children_completion (obj_template_t *parentObj,
                                    WordCompletion *cpl,
                                    const char *line,
                                    int word_start,
                                    int word_end,
                                    int cmdlen)
{
    const xmlChar         *pathname;
    int                    retval;
    int word_iter = word_start + 1;
    // line[word_start] == '/'
    word_start ++;
    cmdlen --;
    while(word_iter <= word_end) {
        if (line[word_iter] == '/') {
            // The second '/' is found
            // find the top level obj and fill its child completion
            char childName[128];
            int child_name_len = word_iter - word_start;
            strncpy (childName, &line[word_start], child_name_len);
            childName[child_name_len] = '\0';
            if (parentObj == NULL)
                return NO_ERR;
            obj_template_t *childObj = 
                obj_find_child(parentObj,
                               obj_get_mod_name(parentObj),
                               (const xmlChar *)childName);
            cmdlen = word_end - word_iter;

            // put the children path with topObj into the recursive 
            // lookup function
            return fill_xpath_children_completion(childObj, 
                                                  cpl,line, word_iter,
                                                  word_end, cmdlen);
        }
        word_iter ++;
    }
    obj_template_t * childObj = obj_first_child_deep(parentObj);
    for(;childObj!=NULL; childObj = obj_next_child_deep(childObj)) {
        pathname = obj_get_name(childObj);
        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)pathname,
                    &line[word_start],
                    cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        if( !obj_is_data_db(childObj)) {
            /* object is either rpc or notification*/
            continue;
        }

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)&pathname[cmdlen], "", "");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }
    return NO_ERR;
}  /* fill_xpath_children_completion */


/********************************************************************
 * FUNCTION check_find_xpath_top_obj
 * 
 * Check a module for a specified object (full or partial name)
 *
 * INPUTS:
 *    mod == module to check
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * RETURNS:
 *   pointer to object if found, NULL if not found
 *********************************************************************/
static obj_template_t * 
    check_find_xpath_top_obj (ncx_module_t *mod,
                              const char *line,
                              int word_start,
                              int cmdlen)
{
    obj_template_t *modObj = ncx_get_first_object(mod);
    for(; modObj!=NULL; 
        modObj = ncx_get_next_object(mod, modObj)) {
        const xmlChar *pathname = obj_get_name(modObj);
        /* check if there is a partial command name */
        if (cmdlen > 0 && !strncmp((const char *)pathname,
                                   &line[word_start],
                                   cmdlen)) {
            // The object is the one looking for
            return modObj;
        }
    }
    return NULL;

}  /* check_find_xpath_top_obj */



/********************************************************************
 * FUNCTION find_xpath_top_obj
 * 
 * Check all modules for top-level data nodes to save
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static obj_template_t *
    find_xpath_top_obj (completion_state_t *comstate,
                        const char *line,
                        int word_start,
                        int word_end)
{
    // line[word_end] == '/'
    int cmdlen = (word_end - 1) - word_start;
    obj_template_t *modObj;

    if (use_servercb(comstate->server_cb)) {
        modptr_t *modptr;
        for (modptr = (modptr_t *)
                 dlq_firstEntry(&comstate->server_cb->modptrQ);
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            modObj = check_find_xpath_top_obj(modptr->mod, line,
                                              word_start, cmdlen);
            if (modObj != NULL) {
                return modObj;
            }
        }

        /* check manager loaded commands next */
        for (modptr = (modptr_t *)dlq_firstEntry(get_mgrloadQ());
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            modObj = check_find_xpath_top_obj(modptr->mod, line,
                                              word_start, cmdlen);
            if (modObj != NULL) {
                return modObj;
            }
        }
    } else {
        ncx_module_t * mod = ncx_get_first_session_module();
        for(;mod!=NULL; mod = ncx_get_next_session_module(mod)) {

            modObj = check_find_xpath_top_obj(mod, line,
                                              word_start, cmdlen);
            if (modObj != NULL) {
                return modObj;
            }
        }
    }
    return NULL;
} /* find_xpath_top_obj */


/********************************************************************
 * FUNCTION check_save_xpath_completion
 * 
 * Check a module for saving top-level data objects
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    mod == module to check
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    check_save_xpath_completion (
        WordCompletion *cpl,
        ncx_module_t *mod,
        const char *line,
        int word_start,
        int word_end,
        int cmdlen)
{
    obj_template_t * modObj = ncx_get_first_object(mod);
    for (; modObj != NULL; 
         modObj = ncx_get_next_object(mod, modObj)) {

        if (!obj_is_data_db(modObj)) {
            /* object is either rpc or notification*/
            continue;
        }

        const xmlChar *pathname = obj_get_name(modObj);
        /* check if there is a partial command name */
        if (cmdlen > 0 && strncmp((const char *)pathname, 
                                  &line[word_start], cmdlen)) {
            continue;
        }

#ifdef DEBUG_TRACE
        log_debug2("\nFilling from module %s, object %s", 
                   mod->name, pathname);
#endif

        int retval = cpl_add_completion(cpl, line, word_start, word_end,
                                        (const char *)&pathname[cmdlen],
                                        "", "");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }
    return NO_ERR;

}  /* check_save_xpath_completion */


/********************************************************************
 * FUNCTION fill_xpath_root_completion
 * 
 * Check all modules for top-level data nodes to save
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_xpath_root_completion (
        WordCompletion *cpl,
        completion_state_t *comstate,
        const char *line,
        int word_start,
        int word_end,
        int cmdlen)
{
    status_t res;

    if (use_servercb(comstate->server_cb)) {
        modptr_t *modptr = (modptr_t *)
            dlq_firstEntry(&comstate->server_cb->modptrQ);
        for (; modptr != NULL; modptr = (modptr_t *)dlq_nextEntry(modptr)) {
            res = check_save_xpath_completion(cpl, modptr->mod, line,
                                              word_start, word_end, cmdlen);
            if (res != NO_ERR) {
                return res;
            }
        }

        /* check manager loaded commands next */
        for (modptr = (modptr_t *)dlq_firstEntry(get_mgrloadQ());
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {
            res = check_save_xpath_completion(cpl, modptr->mod, line,
                                              word_start, word_end, cmdlen);
            if (res != NO_ERR) {
                return res;
            }
        }
    } else {
        ncx_module_t * mod = ncx_get_first_session_module();
        for (;mod!=NULL; mod = ncx_get_next_session_module(mod)) {
            res = check_save_xpath_completion(cpl, mod, line,
                                              word_start, word_end, cmdlen);
            if (res != NO_ERR) {
                return res;
            }
        }
    }
    return NO_ERR;

} /* fill_xpath_root_completion */


/********************************************************************
 * TODO:
 * FUNCTION fill_one_xpath_completion
 *
 * fill the command struct for one RPC operation
 * check all the xpath that match
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    rpc == rpc operation to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of parameter name already entered
 *              this may not be the same as
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_xpath_completion (obj_template_t *rpc,
                               WordCompletion *cpl,
                               completion_state_t *comstate,
                               const char *line,
                               int word_start,
                               int word_end,
                               int cmdlen)
{
    (void)rpc;
    int word_iter = word_start + 1;
    // line[word_start] == '/'
    word_start ++;
    cmdlen --;
    while(word_iter <= word_end) {
//        log_write("%c", line[word_iter]);
        if (line[word_iter] == '/') {
              // The second '/' is found
              // TODO: find the top level obj and fill its child completion
//            log_write("more than 1\n");
              obj_template_t * topObj = find_xpath_top_obj(comstate, line,
                                                           word_start,
                                                           word_iter);

              cmdlen = word_end - word_iter;

              // put the children path with topObj into the recursive 
              // lookup function
              return fill_xpath_children_completion (topObj, cpl, line,
                                                     word_iter, word_end, 
                                                     cmdlen);
        }
        word_iter ++;
    }

    // The second '/' is not found
    return fill_xpath_root_completion(cpl, comstate, line, 
                                      word_start, word_end, cmdlen);

}  /* fill_one_xpath_completion */


/********************************************************************
 * FUNCTION fill_one_rpc_completion_parms
 * 
 * fill the command struct for one RPC operation
 * check all the parameters that match
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    rpc == rpc operation to use
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of parameter name already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_rpc_completion_parms (obj_template_t *rpc,
                                   WordCompletion *cpl,
                                   completion_state_t *comstate,
                                   const char *line,
                                   int word_start,
                                   int word_end,
                                   int cmdlen)
{
    obj_template_t        *inputobj, *obj;
    const xmlChar         *parmname;
    int                    retval;
    ncx_btype_t            btyp;
    boolean                hasval;

    if(line[word_start] == '/') {
        return fill_one_xpath_completion(rpc, cpl, comstate, line, word_start,
                                         word_end, cmdlen);
    }

    inputobj = obj_find_child(rpc, NULL, YANG_K_INPUT);
    if (inputobj == NULL || !obj_has_children(inputobj)) {
        /* no input parameters */
        return NO_ERR;
    }

    for (obj = obj_first_child_deep(inputobj);
         obj != NULL;
         obj = obj_next_child_deep(obj)) {

        parmname = obj_get_name(obj);

        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)parmname, &line[word_start], cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        btyp = obj_get_basetype(obj);
        hasval = (btyp == NCX_BT_EMPTY) ? FALSE : TRUE;

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)&parmname[cmdlen],
                                    "", (hasval) ? "=" : " ");

        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    return NO_ERR;

}  /* fill_one_rpc_completion_parms */


/********************************************************************
 * FUNCTION fill_one_module_completion_commands
 * 
 * fill the command struct for one command string
 * for one module; check all the commands in the module
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    mod == module to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of command already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching parameters found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_module_completion_commands (ncx_module_t *mod,
                                         WordCompletion *cpl,
                                         completion_state_t *comstate,
                                         const char *line,
                                         int word_start,
                                         int word_end,
                                         int cmdlen)
{
    obj_template_t        *obj;
    const xmlChar         *cmdname;
    ncx_module_t          *yangcli_mod;
    int                    retval;
    boolean                toponly;

    yangcli_mod = get_yangcli_mod();
    toponly = FALSE;
    if (mod == yangcli_mod &&
        !use_servercb(comstate->server_cb)) {
        toponly = TRUE;
    }

    /* check all the OBJ_TYP_RPC objects in the module */
    for (obj = ncx_get_first_object(mod);
         obj != NULL;
         obj = ncx_get_next_object(mod, obj)) {

        if (!obj_is_rpc(obj)) {
            continue;
        }

        cmdname = obj_get_name(obj);

        if (toponly && !is_top_command(cmdname)) {
            continue;
        }

        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)cmdname, &line[word_start], cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                               (const char *)&cmdname[cmdlen],
                               (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    return NO_ERR;

}  /* fill_one_module_completion_commands */


/********************************************************************
 * FUNCTION fill_one_completion_commands
 * 
 * fill the command struct for one command string
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completeion state struct in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of command already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_completion_commands (WordCompletion *cpl,
                                  completion_state_t *comstate,
                                  const char *line,
                                  int word_start,
                                  int word_end,
                                  int cmdlen)
{
    modptr_t     *modptr;
    status_t      res;

    res = NO_ERR;

    /* figure out which modules to use */
    if (comstate->cmdmodule) {
        /* if foo:\t entered as the current token, then
         * the comstate->cmdmodule pointer will be set
         * to limit the definitions to that module 
         */
        res = fill_one_module_completion_commands
            (comstate->cmdmodule, cpl, comstate, line, word_start,
             word_end, cmdlen);
    } else {
        if (use_servercb(comstate->server_cb)) {
            /* list server commands first */
            for (modptr = (modptr_t *)
                     dlq_firstEntry(&comstate->server_cb->modptrQ);
                 modptr != NULL && res == NO_ERR;
                 modptr = (modptr_t *)dlq_nextEntry(modptr)) {

#ifdef DEBUG_TRACE
                log_debug("\nFilling from server_cb module %s", 
                          modptr->mod->name);
#endif

                res = fill_one_module_completion_commands
                    (modptr->mod, cpl, comstate, line, word_start,
                     word_end, cmdlen);
            }

            /* list manager loaded commands next */
            for (modptr = (modptr_t *)
                     dlq_firstEntry(get_mgrloadQ());
                 modptr != NULL && res == NO_ERR;
                 modptr = (modptr_t *)dlq_nextEntry(modptr)) {

#ifdef DEBUG_TRACE
                log_debug("\nFilling from mgrloadQ module %s", 
                          modptr->mod->name);
#endif

                res = fill_one_module_completion_commands
                    (modptr->mod, cpl, comstate, line, word_start,
                     word_end, cmdlen);
            }
        }

#ifdef DEBUG_TRACE
        /* use the yangcli top commands every time */
        log_debug("\nFilling from yangcli module");
#endif

        res = fill_one_module_completion_commands
            (get_yangcli_mod(), cpl, comstate, line, word_start,
             word_end, cmdlen);
    }

    return res;

}  /* fill_one_completion_commands */


/********************************************************************
 * FUNCTION is_parm_start
 * 
 * check if current backwards sequence is the start of
 * a parameter or not
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_cur == current internal parser position 
 *
 * RETURN:
 *   TRUE if start of parameter found
 *   FALSE if not found
 *
 *********************************************************************/
static boolean
    is_parm_start (const char *line,
                   int word_start,
                   int word_cur)
{
    const char            *str;

    if (word_start >= word_cur) {
        return FALSE;
    }

    str = &line[word_cur];
    if (*str == '-') {
        if ((word_cur - 2) >= word_start) {
            /* check for previous char before '-' */
            if (line[word_cur - 2] == '-') {
                if ((word_cur - 3) >= word_start) {
                    /* check for previous char before '--' */
		  if (isspace((int)line[word_cur - 3])) {
                        /* this a real '--' command start */
                        return TRUE;
                    } else {
                        /* dash dash part of some value string */
                        return FALSE;
                    }
                } else {
                    /* start of line is '--' */
                    return TRUE;
                }
            } else if (isspace((int)line[word_cur - 2])) {
                /* this is a real '-' command start */
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            /* start of line is '-' */
            return TRUE;
        }
    } else {
        /* previous char is not '-' */
        return FALSE;
    }

} /* is_parm_start */


/********************************************************************
 * FUNCTION find_parm_start
 * 
 * go backwards and figure out the previous token
 * from word_end to word_start
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    inputobj == RPC operation input node
 *                used for parameter context
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *   expectparm == address of return expect--parmaeter flag
 *   emptyexit == address of return empty exit flag
 *   parmobj == address of return parameter in progress
 *   tokenstart == address of return start of the current token
 *   
 *
 * OUTPUTS:
 *   *expectparm == TRUE if expecting a parameter
 *                  FALSE if expecting a value
 *   *emptyexit == TRUE if bailing due to no matches possible
 *                 FALSE if there is something to process
 *   *parmobj == found parameter that was entered
 *            == NULL if no parameter expected
 *            == NULL if expected parameter not found
 *               (error returned)
 *  *tokenstart == index within line that the current
 *                 token starts; will be equal to word_end
 *                 if not in the middle of an assignment
 *                 sequence
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    find_parm_start (obj_template_t *inputobj,
                     const char *line,
                     int word_start,
                     int word_end,
                     boolean *expectparm,
                     boolean *emptyexit,
                     obj_template_t **parmobj,
                     int *tokenstart)

{
    obj_template_t       *childobj;
    const char           *str, *seqstart, *equals;
    uint32                withequals, matchcount;
    boolean               inbetween, gotdashes;

    withequals = 0;
    inbetween = FALSE;
    gotdashes = FALSE;
    equals = NULL;

    *expectparm = FALSE;
    *emptyexit = FALSE;
    *parmobj = NULL;
    *tokenstart = 0;

    /* get the last char entered */
    str = &line[word_end - 1];

    /* check starting in between 2 token sequences */
    if (isspace((int)*str)) {
        inbetween = TRUE;

        /* the tab was in between tokens, not inside a token
         * skip all the whitespace backwards
         */
        while (str >= &line[word_start] && isspace((int)*str)) {
            str--;
        }

        if (isspace((int)*str)) {
            /* only found spaces, so this is the line start */
            *expectparm = TRUE;
            *tokenstart = word_end;
            return NO_ERR;
        } /* else found the end of some text */
    } else if (is_parm_start(line, 
                             word_start, 
                             (int)(str - line))) {
        /* got valid -<tab> or --<tab> */
        *expectparm = TRUE;
        *tokenstart = word_end;
        return NO_ERR;
    } else if (*str =='"' || *str == '\'') {
        /* last char was the start or end of a string
         * no completions possible at this point
         */
        *emptyexit = TRUE;
        return NO_ERR;
    }

    /* str is pointing at the last char in the token sequence
     * that needs to be checked; e.g.:
     *    somecommand parm1=3 --parm2=fred<tab>
     *    somecommand parm1<tab>
     *
     * count the equals signs; hack to guess if
     * this is a complete assignment statement
     */
    while (str >= &line[word_start] && !isspace((int)*str)) {
        if (*str == '=') {
            if (equals == NULL) {
                equals = str;
            }
            withequals++;
        }
        str--;
    }

    /* figure out where the backwards search stopped */
    if (isspace((int)*str)) {
        /* found a space entered so see if
         * the string following it looks like
         * the start of a parameter or a value
         */
        seqstart = ++str;
    } else {
        /* str backed up all the way to word_start
         * start the forward analysis from this char
         */
        seqstart = str;
    }

    /* str is now pointing at the preceding token sequence
     * check if the parameter start sequence is next
     */
    if (*str == '-') {
        /* try to find any matching parameters
         * within the rpc/input section
         */
        seqstart++;
        gotdashes = TRUE;
        if (str+1 < &line[word_end]) {
            /* -<some-text ... <tab>  */
            str++;
            if (*str == '-') {
                if (str+1 < &line[word_end]) {
                    /* --<some-text ... <tab>  */
                    seqstart++;
                    str++;
                } else {
                    /* entire line is --<tab> 
                     * return parameter list
                     */
                    *expectparm = TRUE;
                    *tokenstart = word_end;
                    return NO_ERR;

                }
            } /*else  -<some-text ... <tab>  */
        } else {
            /* entire line is -<tab> 
             * return parameter list
             */
            *expectparm = TRUE;
            *tokenstart = word_end;
            return NO_ERR;
        }
    } /* else token does not start with any dashes */

    /* got a sequence start so figure out what to with it */
    if (inbetween) {
        if (withequals == 1) {
            /* this profile fits 
             *   '[-]-<parmname>=<parmval><spaces><tab>'
             * assume that a new parameter is expected
             * and cause all of them to be listed
             */
            *expectparm = TRUE;
            *tokenstart = word_end;
            return NO_ERR;
        } else {
            /* this profile fits 
             *    '[-]-<parmname><spaces><tab>'
             * this entire token needs to be matched to a
             * parameter in the RPC input section
             *
             * first find the end of the parameter name
             */
            str = seqstart;

            while (str < &line[word_end] && 
                   !isspace((int)*str) &&
                   *str != '=') {
                str++;
            }

            /* try to find this parameter name */
            childobj = obj_find_child_str(inputobj, NULL,
                                          (const xmlChar *)seqstart,
                                          (uint32)(str - seqstart));
            if (childobj == NULL) {
                matchcount = 0;

                /* try to match this parameter name */
                childobj = obj_match_child_str(inputobj, NULL,
                                               (const xmlChar *)seqstart,
                                               (uint32)(str - seqstart),
                                               &matchcount);

                if (childobj && matchcount > 1) {
                    /* ambiguous command error
                     * but just return no completions
                     */
                    *emptyexit = TRUE;
                    return NO_ERR;
                }
            }

            /* check find/match child result */
            if (childobj == NULL) {
                /* do not recognize this parameter,
                 * so just return no matches
                 */
                *emptyexit = TRUE;
                return NO_ERR;
            }

            /* else found one matching child object */
            *tokenstart = word_end;

            /* check if the parameter is an empty, which
             * means another parameter is expected, not a value
             */
            if (obj_get_basetype(childobj) == NCX_BT_EMPTY) {
                *expectparm = TRUE;
            } else {
                /* normal leaf parameter; needs a value */
                *parmobj = childobj;
            }
            return NO_ERR;
        }
    }

    /* not in between 2 tokens so tab is 'stuck' to
     * this preceding token; determine if a parameter
     * or a value is expected
     */
    if (withequals == 1) {
        /* this profile fits 
         *   '[--]<parmname>=<parmval><tab>'
         * assume that a parameter value is expected
         * and cause all of them to be listed that
         * match the partial value string
         */
        *tokenstart = (int)((equals+1) - line);

        /* try to find the parameter name */
        childobj = obj_find_child_str(inputobj, NULL,
                                      (const xmlChar *)seqstart,
                                      (uint32)(equals - seqstart));
        if (childobj == NULL) {
            matchcount = 0;
            
            /* try to match this parameter name */
            childobj = obj_match_child_str(inputobj, NULL,
                                           (const xmlChar *)seqstart,
                                           (uint32)(equals - seqstart),
                                           &matchcount);

            if (childobj && matchcount > 1) {
                /* ambiguous command error
                 * but just return no completions
                 */
                *emptyexit = TRUE;
                return NO_ERR;
            }
        }

        /* check find/match child result */
        if (childobj == NULL) {
            /* do not recognize this parameter,
             * so just return no matches
             */
            *emptyexit = TRUE;
        } else {
            /* match the found parameter with a
             * partial value string
             */
            *parmobj = childobj;
        }
        return NO_ERR;
    } else if (gotdashes) {
        /* this profile fits 
         *   '--<parmname><tab>'
         * assume that a parameter name is expected
         * and cause all of them to be listed that
         * match the partial name string
         */
        *tokenstart = (int)(seqstart - line);
        *expectparm = TRUE;
    } else {
        /* this profile fits 
         *   '[don't know]<parmwhat><tab>'
         * assume that a parameter name is expected
         * and cause all of them to be listed that
         * match the partial name string
         *
         * TBD: should check back another token
         * so see if a value or a name is expected
         */
        *tokenstart = (int)(seqstart - line);
        *expectparm = TRUE;  /****/
    }
       
    return NO_ERR;

} /* find_parm_start */


/********************************************************************
 * FUNCTION parse_backwards_parm
 * 
 * go through the command line backwards
 * from word_end to word_start, figuring
 *
 * command state is CMD_STATE_FULL
 *
 * This function only used when there are allowed
 * to be multiple parm=value pairs on the same line
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *
 * OUTPUTS:
 *   parameter or value completions addeed if found
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    parse_backwards_parm (WordCompletion *cpl,
                          completion_state_t *comstate,
                          const char *line,
                          int word_start,
                          int word_end)

{
    obj_template_t        *rpc, *inputobj, *parmobj;
    status_t               res;
    int                    tokenstart;
    boolean                expectparm, emptyexit;

    if (word_end == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    res = NO_ERR;
    parmobj = NULL;
    tokenstart = 0;
    expectparm = FALSE;
    emptyexit = FALSE;
    rpc = comstate->cmdobj;
    inputobj = comstate->cmdinput;

    res = find_parm_start(inputobj, line, word_start, word_end, &expectparm,
                          &emptyexit, &parmobj, &tokenstart);
    if (res != NO_ERR || emptyexit) {
        return res;
    }

    if (expectparm) {
        /* add all the parameters, even those that
         * might already be entered (oh well)
         * this is OK for leaf-lists but not leafs
         */

#ifdef YANGCLI_TAB_DEBUG
        log_debug2("\n*** fill one RPC %s parms ***\n", obj_get_name(rpc));
#endif

        res = fill_one_rpc_completion_parms(rpc, cpl, comstate, line,
                                            tokenstart, word_end, 
                                            word_end - tokenstart);
    } else if (parmobj) {
        /* have a parameter in progress and the
         * token start is supposed to be the value
         * for this parameter
         */

#ifdef YANGCLI_TAB_DEBUG
        log_debug2("\n*** fill one parm in backwards ***\n");
#endif

        res = fill_parm_completion(parmobj, cpl, comstate, line, tokenstart,
                                   word_end, word_end - tokenstart);
    } /* else nothing to do */

    return res;

} /* parse_backwards_parm */


/********************************************************************
 * FUNCTION fill_parm_values
 * 
 * go through the command line 
 * from word_start to word_end, figuring
 * out which value is entered
 *
 * command state is CMD_STATE_GETVAL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *
 * OUTPUTS:
 *   value completions addeed if found
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_parm_values (WordCompletion *cpl,
                      completion_state_t *comstate,
                      const char *line,
                      int word_start,
                      int word_end)
{
    status_t               res;

    /* skip starting whitespace */
    while (word_start < word_end && isspace((int)line[word_start])) {
        word_start++;
    }

    /*** NEED TO CHECK FOR STRING QUOTES ****/

#ifdef YANGCLI_TAB_DEBUG
    log_debug2("\n*** fill parm values ***\n");
#endif

    res = fill_parm_completion(comstate->cmdcurparm, cpl, comstate, line,
                               word_start, word_end, word_end - word_start);

    return res;

} /* fill_parm_values */


/********************************************************************
 * FUNCTION fill_completion_commands
 * 
 * go through the available commands to see what
 * matches should be returned
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 * OUTPUTS:
 *   comstate
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_completion_commands (WordCompletion *cpl,
                              completion_state_t *comstate,
                              const char *line,
                              int word_end)

{
    obj_template_t        *inputobj;
    const char            *str, *cmdend, *cmdname, *match;
    xmlChar               *buffer;
    ncx_node_t             dtyp;
    status_t               res;
    int                    word_start, cmdlen;
    uint32                 retlen;
    boolean                done, equaldone;

    res = NO_ERR;
    str = line;
    cmdend = NULL;
    word_start = 0;

    /* skip starting whitespace */
    while ((str < &line[word_end]) && isspace((int)*str)) {
        str++;
    }

    /* check if any real characters entered yet */
    if (word_end == 0 || str == &line[word_end]) {
        /* found only spaces so far or
         * nothing entered yet 
         */
        res = fill_one_completion_commands(cpl, comstate, line, word_end,
                                           word_end, 0);
        if (res != NO_ERR) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return res;
    }

    /* else found a non-whitespace char in the buffer that
     * is before the current tab position
     * check what kind of command line is in progress
     */
    if (*str == '@' || *str == '$') {
        /* at-file-assign start sequence OR
         * variable assignment start sequence 
         * may deal with auto-completion for 
         * variables and files later
         * For now, skip until the end of the 
         * assignment is found or word_end hit
         */
        comstate->assignstmt = TRUE;
        equaldone = FALSE;

        while ((str < &line[word_end]) && 
               !isspace((int)*str) && (*str != '=')) {
            str++;
        }

        /* check where the string search stopped */
        if (isspace((int)*str) || *str == '=') {
            /* stopped past the first word
             * so need to skip further
             */
	  if (isspace((int)*str)) {
                /* find equals sign or word_end */
                while ((str < &line[word_end]) && 
                       isspace((int)*str)) {
                    str++;
                }
            }

            if ((*str == '=') && (str < &line[word_end])) {
                equaldone = TRUE;
                str++;  /* skip equals sign */

                /* skip more whitespace */
                while ((str < &line[word_end]) && 
                       isspace((int)*str)) {
                    str++;
                }
            }

            /* got past the '$foo =' part 
             * now looking for a command 
             */
            if (str < &line[word_end]) {
                /* go to next part and look for
                 * the end of this start command
                 */
                word_start = (int)(str - line);
            } else if (equaldone) {
                res = fill_one_completion_commands(cpl, comstate, line,
                                                   word_end, word_end, 0);
                if (res != NO_ERR) {
                    cpl_record_error(cpl, get_error_string(res));
                }
                return res;
            } else {
                /* still inside the file or variable name */
                return res;
            }
        } else {
            /* word_end is still inside the first
             * word, which is an assignment of
             * some sort; not going to show
             * any completions for user vars and files yet
             */
            return res;
        }
    } else {
        /* first word starts with a normal char */
        word_start = (int)(str - line);
    }

    /* the word_start var is set to the first char
     * that is supposed to be start command name
     * the 'str' var points to that char
     * check if it is an entire or partial command name
     */
    cmdend = str;
    while (cmdend < &line[word_end] && !isspace((int)*cmdend)) {
        cmdend++;
    }
    cmdlen = (int)(cmdend - str);

    /* check if still inside the start command */
    if (cmdend == &line[word_end]) {
        /* get all the commands that start with the same
         * characters for length == 'cmdlen'
         */
        res = fill_one_completion_commands(cpl, comstate, line, word_start,
                                           word_end, cmdlen);
        if (res != NO_ERR) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return res;
    }

    /* not inside the start command so get the command that
     * was selected if it exists; at this point the command
     * should be known, so find it
     */
    cmdname = &line[word_start];

    buffer = xml_strndup((const xmlChar *)cmdname, (uint32)cmdlen);

    if (buffer == NULL) {
        if (cpl != NULL) {
            cpl_record_error(cpl, get_error_string(ERR_INTERNAL_MEM));
        }
        return 1;
    }
    retlen = 0;
    dtyp = NCX_NT_OBJ;
    res = NO_ERR;
    comstate->cmdobj = (obj_template_t *)
        parse_def(comstate->server_cb, &dtyp, buffer, &retlen, &res);
    m__free(buffer);

    if (comstate->cmdobj == NULL) {
        if (cpl != NULL) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return 1;
    }

    /* have a command that is valid
     * first skip any whitespace after the
     * command name
     */
    str = cmdend;
    while (str < &line[word_end] && isspace((int)*str)) {
        str++;
    }
    /* set new word_start == word_end */
    word_start = (int)(str - line);

    /* check where E-O-WSP search stopped */
    if (str == &line[word_end]) {
        /* stopped before entering any parameters */
        res = fill_one_rpc_completion_parms(comstate->cmdobj, cpl, 
                                            comstate, line,
                                            word_start, word_end, 0);
        return res;
    }


    /* there is more text entered; check if this rpc
     * really has any input parameters or not
     */
    inputobj = obj_find_child(comstate->cmdobj, NULL, YANG_K_INPUT);
    if (inputobj == NULL ||
        !obj_has_children(inputobj)) {
        /* no input parameters expected */
        return NO_ERR;
    } else {
        comstate->cmdinput = inputobj;
    }

    /* check if any strings entered on the line
     * stopping forward parse at line[word_start]
     */
    cmdend = str;
    while (cmdend < &line[word_end] &&
           *cmdend != '"' && 
           *cmdend != '\'') {
        cmdend++;
    }

    if (cmdend < &line[word_end]) {
        /* found the start of a quoted string
         * look for the end of the last quoted
         * string
         */
        done = FALSE;
        while (!done) {
            /* match == start of quoted string */
            match = cmdend++;
            while (cmdend < &line[word_end] &&
                   *cmdend != *match) {
                cmdend++;
            }
            if (cmdend == &line[word_end]) {
                /* entering a value inside a string
                 * so just return, instead of
                 * guessing the string value
                 */
                return res;
            } else {
                cmdend++;
                while (cmdend < &line[word_end] &&
                       *cmdend != '"' && 
                       *cmdend != '\'') {
                    cmdend++;
                }
                if (cmdend == &line[word_end]) {
                    /* did not find the start of
                     * another string so set the
                     * new word_start and call
                     * the parse_backwards_parm fn
                     */
                    word_start = (int)(cmdend - line);
                    done = TRUE;
                } 

                /* else stopped on another start of 
                 * quoted string so loop again
                 */
            }
        }
    }

    /* got some edited text past the last 
     * quoted string so figure out the
     * parm in progress and check for completions
     */
    res = parse_backwards_parm(cpl, comstate, line, word_start, word_end);

    return res;

} /* fill_completion_commands */


/********************************************************************
 * FUNCTION fill_completion_yesno
 * 
 * go through the available commands to see what
 * matches should be returned
 *
 * command state is CMD_STATE_YESNO
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    line == current line in progress
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 * OUTPUTS:
 *   comstate
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_completion_yesno (WordCompletion *cpl,
                           const char *line,
                           int word_end)
{
    const char            *str;
    int                    word_start, retval;

    str = line;
    word_start = 0;

    /* skip starting whitespace */
    while ((str < &line[word_end]) && isspace((int)*str)) {
        str++;
    }

    /* check if any real characters entered yet */
    if (word_end == 0 || str == &line[word_end]) {
        /* found only spaces so far or
         * nothing entered yet 
         */

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)NCX_EL_YES, 
                                    (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }


        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)NCX_EL_NO,
                                    (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    /*** !!!! ELSE NOT GIVING ANY PARTIAL COMPLETIONS YET !!! ***/

    return NO_ERR;

}  /* fill_completion_yesno */


/*.......................................................................
 *
 * FUNCTION yangcli_tab_callback (word_complete_cb)
 *
 *   libtecla tab-completion callback function
 *
 * Matches the CplMatchFn typedef
 *
 * From /usr/lib/include/libtecla.h:
 * 
 * Callback functions declared and prototyped using the following macro
 * are called upon to return an array of possible completion suffixes
 * for the token that precedes a specified location in the given
 * input line. It is up to this function to figure out where the token
 * starts, and to call cpl_add_completion() to register each possible
 * completion before returning.
 *
 * Input:
 *  cpl  WordCompletion *  An opaque pointer to the object that will
 *                         contain the matches. This should be filled
 *                         via zero or more calls to cpl_add_completion().
 *  data           void *  The anonymous 'data' argument that was
 *                         passed to cpl_complete_word() or
 *                         gl_customize_completion()).
 *  line     const char *  The current input line.
 *  word_end        int    The index of the character in line[] which
 *                         follows the end of the token that is being
 *                         completed.
 * Output
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int
    yangcli_tab_callback (WordCompletion *cpl, 
                          void *data,
                          const char *line, 
                          int word_end)
{
    completion_state_t   *comstate;
    status_t              res;
    int                   retval;

#ifdef DEBUG
    if (!cpl || !data || !line) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 1;
    }
#endif

    retval = 0;

    /* check corner case that never has any completions
     * no matter what state; backslash char only allowed
     * for escaped-EOLN or char-sequence that we can't guess
     */
    if (word_end > 0 && line[word_end] == '\\') {
        return retval;
    }

    comstate = (completion_state_t *)data;

    if (comstate->server_cb && comstate->server_cb->climore) {
        comstate->cmdstate = CMD_STATE_MORE;
    }

    switch (comstate->cmdstate) {
    case CMD_STATE_FULL:
        res = fill_completion_commands(cpl, comstate, line, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_GETVAL:
        res = fill_parm_values(cpl, comstate, line, 0, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_YESNO:
        res = fill_completion_yesno(cpl, line, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_MORE:
        /*** NO SUPPORT FOR MORE MODE YET ***/
        break;
    case CMD_STATE_NONE:
        /* command state not initialized */
        SET_ERROR(ERR_INTERNAL_VAL);
        retval = 1;
        break;
    default:
        /* command state garbage value */
        SET_ERROR(ERR_INTERNAL_VAL);
        retval = 1;
    }

    return retval;

} /* yangcli_tab_callback */


/* END yangcli.c */
