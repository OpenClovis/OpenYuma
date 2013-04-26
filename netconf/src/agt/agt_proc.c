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
/*  FILE: agt_proc.c

   /proc file system monitoring module

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
18jul09      abb      begun

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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_cb.h"
#include "agt_proc.h"
#include "agt_rpc.h"
#include "agt_util.h"
#include "cfg.h"
#include "getcb.h"
#include "log.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "rpc.h"
#include "ses.h"
#include "ses_msg.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xml_util.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define proc_MOD        (const xmlChar *)"yuma-proc"

#define proc_MOD_REV    NULL

#define proc_N_proc      (const xmlChar *)"proc"

#define proc_N_cpuinfo   (const xmlChar *)"cpuinfo"

#define proc_N_cpu       (const xmlChar *)"cpu"

#define proc_N_meminfo   (const xmlChar *)"meminfo"

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

static boolean              agt_proc_init_done = FALSE;

static ncx_module_t         *procmod;

static val_value_t          *myprocval;

static obj_template_t       *myprocobj;


/********************************************************************
* FUNCTION is_proc_supported
*
* Check if at least /proc/meminfo and /proc/cpuinfo exist
*
* RETURNS:
*   TRUE if minimum /proc support found
*   FALSE if minimum /proc support not found
*********************************************************************/
static boolean
    is_proc_supported (void)
{
    struct stat      statbuf;
    int              ret;

    memset(&statbuf, 0x0, sizeof(statbuf));
    ret = stat("/proc/meminfo", &statbuf);
    if (ret == 0 && S_ISREG(statbuf.st_mode)) {
        memset(&statbuf, 0x0, sizeof(statbuf));

        ret = stat("/proc/cpuinfo", &statbuf);
        if (ret == 0 && S_ISREG(statbuf.st_mode)) {
            return TRUE;
        }
    }

    return FALSE;

} /* is_proc_supported */


/********************************************************************
* FUNCTION make_proc_varname
*
* Make a proc var name string
*
* RETURNS:
*   malloced and filled in string
*********************************************************************/
static xmlChar *
    make_proc_varname (const char *varname,
                       int varnamelen)
{
    const char *str;
    xmlChar    *buff, *retbuff;
    int         count;

    retbuff = (xmlChar *)m__getMem(varnamelen+1);
    if (retbuff == NULL) {
        return NULL;
    }

    count = 0;
    str = varname;
    buff = retbuff;

    while (*str && count < varnamelen) {
      if (isalnum((xmlChar)*str)) {
            *buff++ = (xmlChar)*str++;
        } else {
            *buff++ = (xmlChar)'_';
            str++;
        }
        count++;
    }
    *buff = (xmlChar)'\0';

    return retbuff;
    
} /* make_proc_varname */


/********************************************************************
* FUNCTION make_proc_value
*
* Make a proc value string
*
* INPUTS:
*   line == pointer to input line to process
*
* RETURNS:
*   malloced and filled in string
*********************************************************************/
static xmlChar *
    make_proc_value (const char *line)

{
    const char *str, *start;
    uint32      valuelen;

    str = line;

    while (isspace((xmlChar)*str) && *str != '\n' && *str != '\0') {
        str++;
    }

    if (*str == '\n' || *str == '\0') {
        start = line;
        valuelen = 0;
    } else {
        start = str++;
        while (*str != '\n' && *str != '\0') {
            str++;
        }
        valuelen = (uint32)(str - start);
    }
        
    return xml_strndup((const xmlChar *)start, valuelen);
    
} /* make_proc_value */


/********************************************************************
* FUNCTION make_proc_leaf
*
* make a val_value_t struct for the leaf found on the
* current buffer line
*
* INPUTS:
*   buffer == input line buffer to check
*   parentobj == parent object for this leaf
*   res == address of return status
* 
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   malloced and filled in leaf 
*********************************************************************/
static val_value_t *
    make_proc_leaf (char *buffer,
                    obj_template_t *parentobj,
                    status_t *res)
{
    obj_template_t        *parmobj;
    val_value_t           *parmval;
    xmlChar               *parmname, *parmvalstr;
    char                  *colonchar, *str;
    int                    parmnamelen;

    *res = NO_ERR;

    colonchar = strchr(buffer, ':');
    if (colonchar == NULL) {
        /* skip this line, no colon char found */
        if (LOGDEBUG) {
            log_debug("\nagt_proc: skipping line '%s'", buffer);
        }
        return NULL;
    }

    /* look backward until the first non-whitespace */
    str = colonchar - 1;
    while (str >= buffer && isspace((xmlChar)*str)) {
        str--;
    }

    if (str == buffer) {
        /* no keyword found */
        if (LOGDEBUG) {
            log_debug("\nagt_proc: skipping line '%s'", buffer);
        }
        return NULL;
    }

    /* get the converted parm name */
    parmnamelen = str-buffer+1;
    parmname = make_proc_varname(buffer, parmnamelen);
    if (parmname == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* find the leaf for this parm name */
    parmobj = obj_find_child(parentobj, proc_MOD, parmname);

    m__free(parmname);

    if (parmobj == NULL) {
        /* no parameter to match this line */
        if (LOGDEBUG) {
            log_debug("\nagt_proc: skipping <%s> line '%s'", 
                      obj_get_name(parentobj),
                      buffer);
        }
        return NULL;
    }

    /* found the leaf, get the trimmed value string */
    parmvalstr = make_proc_value(colonchar+1);
    if (parmvalstr == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* make the leaf value */
    parmval = val_make_simval_obj(parmobj, parmvalstr, res);

    m__free(parmvalstr);

    if (parmval == NULL) {
        log_error("\nError: make /proc leaf <%s> failed",
                          obj_get_name(parmobj));
    }

    return parmval;

}  /* make_proc_leaf */


/********************************************************************
* FUNCTION add_cpuinfo
*
* make a val_value_t struct for the /proc/cpuinfo file
* and add it as a child to the specified container value
*
INPUTS:
*   procval == parent value struct to add the cpuinfo as a child
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_cpuinfo (val_value_t *procval)
{
    FILE                  *cpuinfofile;
    obj_template_t        *cpuinfoobj, *cpuobj;
    val_value_t           *cpuinfoval, *cpuval, *parmval;
    char                  *buffer, *readtest;
    boolean                done;
    status_t               res;

    /* find the cpuinfo object */
    cpuinfoobj = obj_find_child(myprocobj,
                                proc_MOD,
                                proc_N_cpuinfo);
    if (cpuinfoobj == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* find the cpu object */
    cpuobj = obj_find_child(cpuinfoobj,
                            proc_MOD,
                            proc_N_cpu);
    if (cpuobj == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* open the /proc/cpuinfo file for reading */
    cpuinfofile = fopen("/proc/cpuinfo", "r");
    if (cpuinfofile == NULL) {
        return errno_to_status();
    }

    /* get a file read line buffer */
    buffer = m__getMem(NCX_MAX_LINELEN);
    if (buffer == NULL) {
        fclose(cpuinfofile);
        return ERR_INTERNAL_MEM;
    }

    /* create cpuinfo container */
    cpuinfoval = val_new_value();
    if (cpuinfoval == NULL) {
        m__free(buffer);
        fclose(cpuinfofile);
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(cpuinfoval, cpuinfoobj);

    /* hand off cpuinfoval memory here */
    val_add_child(cpuinfoval, procval);

    /* loop through the file until done */
    res = NO_ERR;
    cpuval = NULL;
    done = FALSE;
    while (!done) {
        readtest = fgets(buffer, NCX_MAX_LINELEN, cpuinfofile);
        if (readtest == NULL) {
            done = TRUE;
            continue;
        }

        if (cpuval == NULL) {
            cpuval = val_new_value();
            if (cpuval == NULL) {
                res = ERR_INTERNAL_MEM;
                done = TRUE;
                continue;
            } else {
                val_init_from_template(cpuval, cpuobj);
                /* hand off cpuval memory here */
                val_add_child(cpuval, cpuinfoval);
            }
        } /* else already have an active 'cpu' entry */

        if (strlen(buffer) == 1 && *buffer == '\n') {
            /* force a new CPU entry */
            if (cpuval) {
                res = val_gen_index_chain(cpuobj, cpuval);
                if (res == NO_ERR) {
                    cpuval = NULL;
                } else {
                    log_error("\nError:val_gen_index failed (%s)",
                              get_error_string(res));
                }
                /* else empty line in cpu entry, e.g. arm */
            }
        } else {
            res = NO_ERR;
            parmval = make_proc_leaf(buffer, cpuobj, &res);
            if (parmval) {
                val_add_child(parmval, cpuval);
            }
        }
    }

    if (res == NO_ERR && cpuval) {
        if (val_get_first_index(cpuval) == NULL) {
            val_gen_index_chain(cpuobj, cpuval);
        }
    }
            
    fclose(cpuinfofile);
    m__free(buffer);

    return res;

}  /* add_cpuinfo */


/********************************************************************
* FUNCTION get_meminfo
*
* <get> operation handler for the meminfo NP container
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_meminfo (ses_cb_t *scb,
                 getcb_mode_t cbmode,
                 val_value_t *virval,
                 val_value_t  *dstval)
{
    FILE                  *meminfofile;
    obj_template_t        *meminfoobj;
    val_value_t           *parmval;
    char                  *buffer, *readtest;
    boolean                done;
    status_t               res;

    (void)scb;
    res = NO_ERR;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    meminfoobj = virval->obj;

    /* open the /proc/meminfo file for reading */
    meminfofile = fopen("/proc/meminfo", "r");
    if (meminfofile == NULL) {
        return errno_to_status();
    }

    /* get a file read line buffer */
    buffer = m__getMem(NCX_MAX_LINELEN);
    if (buffer == NULL) {
        fclose(meminfofile);
        return ERR_INTERNAL_MEM;
    }

    /* loop through the file until done */
    res = NO_ERR;
    done = FALSE;
    while (!done) {
        readtest = fgets(buffer, NCX_MAX_LINELEN, meminfofile);
        if (readtest == NULL) {
            done = TRUE;
            continue;
        }

        if (strlen(buffer) == 1 && *buffer == '\n') {
            ;
        } else {
            res = NO_ERR;
            parmval = make_proc_leaf(buffer, meminfoobj, &res);
            if (parmval) {
                val_add_child(parmval, dstval);
            }
        }
    }

    fclose(meminfofile);
    m__free(buffer);

    return res;

} /* get_meminfo */


/********************************************************************
* FUNCTION add_meminfo
*
* make a val_value_t struct for the /proc/meminfo file
* and add it as a child to the specified container value
*
INPUTS:
*   procval == parent value struct to add the cpuinfo as a child
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_meminfo (val_value_t *procval)
{
    obj_template_t        *meminfoobj;
    val_value_t           *meminfoval;

    /* find the meminfo object */
    meminfoobj = obj_find_child(myprocobj,
                                proc_MOD,
                                proc_N_meminfo);
    if (meminfoobj == NULL) {
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* create meminfo virtual NP container */
    meminfoval = val_new_value();
    if (meminfoval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    val_init_virtual(meminfoval, get_meminfo, meminfoobj);

    /* hand off meminfoval memory here */
    val_add_child(meminfoval, procval);

    return NO_ERR;

}  /* add_meminfo */


/************* E X T E R N A L    F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_proc_init
*
* INIT 1:
*   Initialize the proc monitor module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_proc_init (void)
{
    agt_profile_t   *agt_profile;
    status_t         res;

    if (agt_proc_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    log_debug2("\nagt: Loading proc module");

    agt_profile = agt_get_profile();
    procmod = NULL;
    myprocval = NULL;
    myprocobj = NULL;
    agt_proc_init_done = TRUE;

    /* load the netconf-state module */
    res = ncxmod_load_module(proc_MOD,
                             proc_MOD_REV,
                             &agt_profile->agt_savedevQ,
                             &procmod);

    return res;

}  /* agt_proc_init */


/********************************************************************
* FUNCTION agt_proc_init2
*
* INIT 2:
*   Initialize the monitoring data structures
*   This must be done after the <running> config is loaded
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_proc_init2 (void)
{
    cfg_template_t        *runningcfg;
    status_t               res;

    if (!agt_proc_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    res = NO_ERR;

    /* check if /proc file system supported */
    if (!is_proc_supported()) {
        if (LOGDEBUG) {
            log_debug("\nagt_proc: no /proc support found");
        }
        return NO_ERR;
    }

    runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* get all the object nodes first */
    myprocobj = obj_find_template_top(procmod, 
                                      proc_MOD,
                                      proc_N_proc);
    if (!myprocobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* add /proc */
    myprocval = val_new_value();
    if (!myprocval) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(myprocval, myprocobj);

    /* handing off the malloced memory here */
    val_add_child_sorted(myprocval, runningcfg->root);

    res = add_cpuinfo(myprocval);

    if (res == NO_ERR) {
        res = add_meminfo(myprocval);
    }

    return res;

}  /* agt_proc_init2 */


/********************************************************************
* FUNCTION agt_proc_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
void 
    agt_proc_cleanup (void)
{
    if (agt_proc_init_done) {
        procmod = NULL;
        myprocval = NULL;
        myprocval = NULL;
        agt_proc_init_done = FALSE;
    }
}  /* agt_proc_cleanup */


/* END file agt_proc.c */
