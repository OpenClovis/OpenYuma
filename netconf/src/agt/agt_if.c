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
/*  FILE: agt_if.c

   interfaces module

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
#include "agt_if.h"
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

#define interfaces_MOD        (const xmlChar *)"yuma-interfaces"

#define interfaces_MOD_REV    NULL

#define interfaces_N_interfaces      (const xmlChar *)"interfaces"

#define interfaces_N_interface       (const xmlChar *)"interface"

#define interfaces_N_name            (const xmlChar *)"name"

#define interfaces_N_counters        (const xmlChar *)"counters"

#define interfaces_OID_counters (const xmlChar *)\
    "/interfaces/interface/counters"

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

static boolean              agt_if_init_done = FALSE;

static boolean              agt_if_not_supported;

static ncx_module_t         *ifmod;


/********************************************************************
* FUNCTION is_interfaces_supported
*
* Check if at least /proc/net/dev exists
*
* RETURNS:
*   TRUE if minimum if support found
*   FALSE if minimum if support not found
*********************************************************************/
static boolean
    is_interfaces_supported (void)
{
    struct stat      statbuf;
    int              ret;

    memset(&statbuf, 0x0, sizeof(statbuf));
    ret = stat("/proc/net/dev", &statbuf);
    if (ret == 0 && S_ISREG(statbuf.st_mode)) {
        return TRUE;
    }

    return FALSE;

} /* is_interfaces_supported */


/********************************************************************
* FUNCTION get_ifname_string
*
* Get the name field from the /proc/net/dev line
*
* INPUTS:
*   buffer == line from the /proc/net/dev file to read
*   nameptr == address of return name string pointer
*   namelen == address of return name string length
*
* OUTPUTS:
*   *nameptr set to the start of the name string
*   *namelen == name string length
*
* RETURNS:
*    status
*    NO_ERR if outputs filled in OK
*    some other error like ERR_INTERNAL_MEM
*********************************************************************/
static status_t 
    get_ifname_string (xmlChar *buffer,
                       xmlChar **nameptr,
                       int *namelen)
{
    xmlChar      *name, *str;

    *nameptr = NULL;
    *namelen = 0;

    /* get the start of the interface name */
    str = buffer;
    while (*str && xml_isspace(*str)) {
        str++;
    }
    if (*str == '\0') {
        /* not expecting a line with just whitespace on it */
        return ERR_NCX_SKIPPED;
    } else {
        name = str++;
        *nameptr = name;
    }

    /* get the end of the interface name */
    while (*str && *str != ':') {
        str++;
    }
    if (*str == '\0') {
        /* not expecting a line with just foo on it */
        return ERR_NCX_SKIPPED;
    }

    *namelen = (str - name);

    return NO_ERR;

} /* get_ifname_string */


/********************************************************************
* FUNCTION find_interface_entry
*
* Get the specified internface entry
*
* INPUTS:
*   interfacesval == parent entry to check 
*   nameptr == name string
*   namelen == name length
*
* RETURNS:
*    pointer to the found entry or NULL if not found
*********************************************************************/
static val_value_t *
    find_interface_entry (val_value_t *interfacesval,
                          const xmlChar *nameptr,
                          int namelen)
{
    val_value_t  *childval, *nameval;


    for (childval = val_get_first_child(interfacesval);
         childval != NULL;
         childval = val_get_next_child(childval)) {

        nameval = val_find_child(childval,
                                 interfaces_MOD,
                                 interfaces_N_name);
        if (nameval == NULL) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }

        if (!xml_strncmp(VAL_STR(nameval),
                         nameptr,
                         (uint32)namelen) &&
            xml_strlen(VAL_STR(nameval)) == (uint32)namelen) {
            return childval;
        }
    }

    return NULL;

} /* find_interface_entry */


/********************************************************************
* FUNCTION make_interface_entry
*
* Make the starter interface entry for the specified name
*
* INPUTS:
*   interfaceobj == object template to use
*   nameptr == name string, zero-terminated
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*    pointer to the new entry or NULL if malloc failed
*********************************************************************/
static val_value_t *
    make_interface_entry (obj_template_t *interfaceobj,
                          xmlChar *nameptr,
                          status_t *res)
{
    obj_template_t     *nameobj;
    val_value_t        *interfaceval, *nameval;

    *res = NO_ERR;

    nameobj = obj_find_child(interfaceobj,
                             interfaces_MOD,
                             interfaces_N_name);
    if (nameobj == NULL) {
        *res = SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
        return NULL;
    }
                             
    interfaceval = val_new_value();
    if (interfaceval == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    val_init_from_template(interfaceval, interfaceobj);

    nameval = val_make_simval_obj(nameobj, nameptr, res);
    if (nameval == NULL) {
        val_free_value(interfaceval);
        return NULL;
    }

    val_add_child(nameval, interfaceval);

    *res = val_gen_index_chain(interfaceobj, interfaceval);

    if (*res != NO_ERR) {
        val_free_value(interfaceval);
        return NULL;
    }

    return interfaceval;

} /* make_interface_entry */


/********************************************************************
* FUNCTION fill_if_counters
*
* <get> operation handler for the interfaces/interface/counters node
*
* INPUTS:
*   countersobj == object template with all the child node to use
*   nameval == value node for the <name> key that is desired
*   buffer == line from the /proc/net/dev file to read
*   dstval == destination value to fill in
*
* OUTPUTS:
*   child nodes added to dstval->v.childQ if NO_ERR returned
*
* RETURNS:
*    status
*    NO_ERR if this is the right line for the <name> key
*      and everything filled in OK
*    ERR_NCX_SKIPPED if this is the wrong line for this <name>
*    some other error like ERR_INTERNAL_MEM
*********************************************************************/
static status_t 
    fill_if_counters (obj_template_t *countersobj,
                      val_value_t *nameval,
                      xmlChar *buffer,
                      val_value_t  *dstval)
{
    obj_template_t        *childobj;
    val_value_t           *childval;
    xmlChar               *str, *name;
    char                  *endptr;
    status_t               res;
    uint32                 leafcount;
    uint64                 counter;
    boolean                done;

    res = NO_ERR;
    leafcount = 0;
    counter = 0;

    /* get the start of the interface name */
    str = buffer;
    while (*str && isspace(*str)) {
        str++;
    }
    if (*str == '\0') {
        /* not expecting a line with just whitespace on it */
        return ERR_NCX_SKIPPED;
    } else {
        name = str++;
    }

    /* get the end of the interface name */
    while (*str && *str != ':') {
        str++;
    }
    if (*str == '\0') {
        /* not expecting a line with just foo on it */
        return ERR_NCX_SKIPPED;
    }

    /* is this the requested interface line? */
    if (xml_strncmp((const xmlChar *)name,
                    VAL_STR(nameval),
                    (uint32)(str - name))) {
        /* not the right interface name */
        return ERR_NCX_SKIPPED;
    }

    /* get the str pointed at the first byte of the
     * 16 ordered counter values
     */
    str++;

    /* get the first counter object ready */
    childobj = obj_first_child(countersobj);
    if (childobj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* keep getting counters until the line runs out */
    done = FALSE;
    while (!done) {
        endptr = NULL;
        counter = strtoull((const char *)str, &endptr, 10);
        if (counter == 0 && str == (xmlChar *)endptr) {
            /* number conversion failed */
            log_error("\nError: /proc/net/dev number conversion failed");
            return ERR_NCX_OPERATION_FAILED;
        }

        childval = val_new_value();
        if (childval == NULL) {
            return ERR_INTERNAL_MEM;
        }
        val_init_from_template(childval, childobj);
        VAL_ULONG(childval) = counter;
        val_add_child(childval, dstval);

        leafcount++;

        str = (xmlChar *)endptr;
        if (*str == '\0' || *str == '\n') {
            done = TRUE;
        } else {
            childobj = obj_next_child(childobj);
            if (childobj == NULL) {
                done = TRUE;
            }
        }
    }

    if (LOGDEBUG2) {
        log_debug2("\nagt_if: filled %u of 16 counters for '%s'",
                   leafcount,
                   VAL_STR(nameval));
    }

    return res;

} /* fill_if_counters */


/********************************************************************
* FUNCTION get_if_counters
*
* <get> operation handler for the interfaces/interface/counters node
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_if_counters (ses_cb_t *scb,
                     getcb_mode_t cbmode,
                     val_value_t *virval,
                     val_value_t  *dstval)
{
    FILE                  *countersfile;
    obj_template_t        *countersobj;
    val_value_t           *parentval, *nameval;
    xmlChar               *buffer, *readtest;
    boolean                done;
    status_t               res;
    uint32                 linecount;

    (void)scb;
    res = NO_ERR;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    countersobj = virval->obj;

    /* get the interface parent entry for this counters entry */
    parentval = virval->parent;
    if (parentval == NULL) {
        /* the parent should be valid */
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* get the name key leaf from the parent */
    nameval = val_find_child(parentval,
                             interfaces_MOD,
                             interfaces_N_name);
    if (nameval == NULL) {
        /* there should be a key present */
        return SET_ERROR(ERR_INTERNAL_VAL);
    }        

    /* open the /proc/net/dev file for reading */
    countersfile = fopen("/proc/net/dev", "r");
    if (countersfile == NULL) {
        return errno_to_status();
    }

    /* get a file read line buffer */
    buffer = m__getMem(NCX_MAX_LINELEN);
    if (buffer == NULL) {
        fclose(countersfile);
        return ERR_INTERNAL_MEM;
    }

    /* loop through the file until done */
    res = NO_ERR;
    done = FALSE;
    linecount = 0;

    while (!done) {
      readtest = (xmlChar *)
	fgets((char *)buffer, NCX_MAX_LINELEN, countersfile);
        if (readtest == NULL) {
            done = TRUE;
            continue;
        } else {
            linecount++;
        }

        if (linecount < 3) {
            /* skip the header junk on the first 2 lines */
            continue;
        } 

        res = fill_if_counters(countersobj,
                               nameval, 
                               buffer, 
                               dstval);
        if (res == ERR_NCX_SKIPPED) {
            res = NO_ERR;
        } else {
            done = TRUE;
        }
    }

    fclose(countersfile);
    m__free(buffer);

    return res;

} /* get_if_counters */


/********************************************************************
* FUNCTION add_interface_entries
*
* make a val_value_t struct for each interface line found
* in the /proc/net/dev file
* and add it as a child to the specified container value
*
INPUTS:
*   interfaacesval == parent value struct to add each
*   interface list entry
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_interface_entries (val_value_t *interfacesval)
{

    FILE                  *countersfile;
    obj_template_t        *interfaceobj, *countersobj;
    val_value_t           *interfaceval, *countersval;
    xmlChar               *buffer, *readtest, *ifname;
    boolean                done;
    status_t               res;
    uint32                 linecount;
    int                    ifnamelen;

    res = NO_ERR;

    interfaceobj = obj_find_child(interfacesval->obj,
                                  interfaces_MOD,
                                  interfaces_N_interface);
    if (interfaceobj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    countersobj = obj_find_child(interfaceobj,
                                 interfaces_MOD,
                                 interfaces_N_counters);
    if (countersobj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    /* open the /proc/net/dev file for reading */
    countersfile = fopen("/proc/net/dev", "r");
    if (countersfile == NULL) {
        return errno_to_status();
    }

    /* get a file read line buffer */
    buffer = m__getMem(NCX_MAX_LINELEN);
    if (buffer == NULL) {
        fclose(countersfile);
        return ERR_INTERNAL_MEM;
    }

    /* loop through the file until done */
    res = NO_ERR;
    done = FALSE;
    linecount = 0;

    while (!done) {
      readtest = (xmlChar *)
	fgets((char *)buffer, NCX_MAX_LINELEN, countersfile);
        if (readtest == NULL) {
            done = TRUE;
            continue;
        } else {
            linecount++;
        }

        if (linecount < 3) {
            /* skip the header junk on the first 2 lines */
            continue;
        } 

        ifname = NULL;
        ifnamelen = 0;
        res = get_ifname_string(buffer, &ifname, &ifnamelen);
        if (res != NO_ERR) {
            done = TRUE;
        } else {
            /* got the name string
             * see if this entry is already present
             */
            interfaceval = find_interface_entry(interfacesval,
                                                ifname,
                                                ifnamelen);
            if (interfaceval == NULL) {
                /* create a new entry */
                res = NO_ERR;
                ifname[ifnamelen] = 0;
                interfaceval = make_interface_entry(interfaceobj,
                                                    ifname,
                                                    &res);
                if (interfaceval == NULL) {
                    done = TRUE;
                    continue;
                } else {
                    val_add_child(interfaceval, interfacesval);
                }
            }

            /* add the counters virtual node to the entry */
            countersval = val_new_value();
            if (countersval == NULL) {
                res = ERR_INTERNAL_MEM;
                done = TRUE;
                continue;
            } else {
                val_init_virtual(countersval,
                                 get_if_counters,
                                 countersobj);
                val_add_child(countersval, interfaceval);
            }
        }
    }

    fclose(countersfile);
    m__free(buffer);

    return res;

}  /* add_interface_entries */


/************* E X T E R N A L    F U N C T I O N S ***************/


/********************************************************************
* FUNCTION agt_if_init
*
* INIT 1:
*   Initialize the interfaces monitor module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_if_init (void)
{
    status_t       res;
    agt_profile_t *agt_profile;

    if (agt_if_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    log_debug2("\nagt: Loading interfaces module");

    ifmod = NULL;
    agt_if_not_supported = FALSE;
    agt_if_init_done = TRUE;
    agt_profile = agt_get_profile();

    /* load the yuma-interfaces module */
    res = ncxmod_load_module(interfaces_MOD,
                             interfaces_MOD_REV,
                             &agt_profile->agt_savedevQ,
                             &ifmod);

    /* check if /interfaces file system supported */
    if (!is_interfaces_supported()) {
        if (LOGDEBUG) {
            log_debug("\nagt_interfaces: no /interfaces support found");
        }
        agt_if_not_supported = TRUE;
    }

    return res;

}  /* agt_if_init */


/********************************************************************
* FUNCTION agt_if_init2
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
    agt_if_init2 (void)
{
    cfg_template_t        *runningcfg;
    obj_template_t       *interfacesobj;
    val_value_t           *interfacesval;
    status_t               res;

    res = NO_ERR;

    if (!agt_if_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    interfacesobj = obj_find_template_top(ifmod, 
                                          interfaces_MOD,
                                          interfaces_N_interfaces);
    if (!interfacesobj) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    if (!agt_if_not_supported) {
	/* check to see if the /interfaces subtree already exists */
	interfacesval = val_find_child(runningcfg->root,
				       interfaces_MOD,
				       interfaces_N_interfaces);
	if (interfacesval == NULL) {
	    /* need to create the /interfaces root */
	    interfacesval = val_new_value();
	    if (interfacesval == NULL) {
		return ERR_INTERNAL_MEM;
	    }
	    val_init_from_template(interfacesval, interfacesobj);

	    /* handing off the malloced memory here */
	    val_add_child_sorted(interfacesval, runningcfg->root);
	}

        res = add_interface_entries(interfacesval);
    }

    return res;

}  /* agt_if_init2 */


/********************************************************************
* FUNCTION agt_if_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
void 
    agt_if_cleanup (void)
{
    if (agt_if_init_done) {
        ifmod = NULL;
        agt_if_init_done = FALSE;
    }

}  /* agt_if_cleanup */


/* END file agt_if.c */
