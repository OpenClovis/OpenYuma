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
/*  FILE: agt_acm.c

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
04feb06      abb      begun
01aug08      abb      convert from NCX PSD to YANG OBJ design
20feb10      abb      add enable-nacm leaf and notification-rules
                      change indexing to user-ordered rule-name
                      instead of allowed-rights bits field

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <assert.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include "procdefs.h"
#include "agt.h"
#include "agt_acm.h"
#include "agt_cb.h"
#include "agt_not.h"
#include "agt_ses.h"
#include "agt_util.h"
#include "agt_val.h"
#include "def_reg.h"
#include "dlq.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ncx_list.h"
#include "ncxconst.h"
#include "ncxmod.h"
#include "obj.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "xmlns.h"
#include "xpath.h"
#include "xpath1.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define AGT_ACM_MODULE     (const xmlChar *)"ietf-netconf-acm"

#define nacm_N_groups (const xmlChar *)"groups"
#define nacm_N_group (const xmlChar *)"group"
#define nacm_N_name (const xmlChar *)"name"
#define nacm_N_userName (const xmlChar *)"user-name"

#define nacm_N_ruleList (const xmlChar *)"rule-list"
#define nacm_N_rule (const xmlChar *)"rule"

#define nacm_N_accessOperations (const xmlChar *)"access-operations"
#define nacm_N_comment (const xmlChar *)"comment"
#define nacm_N_enableNacm (const xmlChar *)"enable-nacm"
#define nacm_N_group (const xmlChar *)"group"
#define nacm_N_name (const xmlChar *)"name"
#define nacm_N_groups (const xmlChar *)"groups"
#define nacm_N_moduleName (const xmlChar *)"module-name"
#define nacm_N_moduleRule (const xmlChar *)"module-rule"
#define nacm_N_nacm (const xmlChar *)"nacm"
#define nacm_N_rule_name (const xmlChar *)"rule-name"
#define nacm_N_notificationName \
    (const xmlChar *)"notification-name"
#define nacm_N_readDefault (const xmlChar *)"read-default"
#define nacm_N_writeDefault (const xmlChar *)"write-default"
#define nacm_N_execDefault (const xmlChar *)"exec-default"
#define nacm_N_path (const xmlChar *)"path"
#define nacm_N_rpcName (const xmlChar *)"rpc-name"
#define nacm_N_notificationName (const xmlChar *)"notification-name"

#define nacm_N_denied_operations (const xmlChar *)"denied-operations"
#define nacm_N_deniedDataWrites (const xmlChar *)"denied-data-writes"
#define nacm_N_deniedNotifications (const xmlChar *)"denied-notifications"

#define nacm_OID_nacm (const xmlChar *)"/nacm"
#define nacm_OID_nacm_enable_nacm (const xmlChar *)"/nacm/enable-nacm"

#define nacm_E_noRuleDefault_permit (const xmlChar *)"permit"
#define nacm_E_noRuleDefault_deny   (const xmlChar *)"deny"

#define nacm_E_allowedRights_read  (const xmlChar *)"read"
#define nacm_E_allowedRights_create  (const xmlChar *)"create"
#define nacm_E_allowedRights_update  (const xmlChar *)"update"
#define nacm_E_allowedRights_delete  (const xmlChar *)"delete"
#define nacm_E_allowedRights_exec  (const xmlChar *)"exec"


/********************************************************************
*                                                                    *
*                             T Y P E S                              *
*                                                                    *
*********************************************************************/



/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
static boolean agt_acm_init_done = FALSE;

static ncx_module_t  *nacmmod;

static const xmlChar *superuser;

static agt_acmode_t   acmode;

static uint32         denied_operations_count;

static uint32         denied_data_writes_count;

static agt_acm_cache_t  *notif_cache;

static boolean log_reads;

static boolean log_writes;

/********************************************************************
* FUNCTION is_superuser
*
* Check if the specified user name is the superuser
*
* INPUTS:
*   username == username to check
*
* RETURNS:
*   TRUE if username is the superuser
*   FALSE if username is not the superuser
*********************************************************************/
static boolean
    is_superuser (const xmlChar *username)
{
    if (!superuser || !*superuser) {
        return FALSE;
    }
    if (!username || !*username) {
        return FALSE;
    }
    return (xml_strcmp(superuser, username)) ? FALSE : TRUE;

}  /* is_superuser */


/********************************************************************
* FUNCTION check_mode
*
* Check the access-control mode being used
* 
* INPUTS:
*   access == requested access mode
*   obj == object template to check
*
* RETURNS:
*   TRUE if access granted
*   FALSE to check the rules and find out
*********************************************************************/
static boolean 
    check_mode (const xmlChar *access,
                const obj_template_t *obj)
{
    boolean      isread;

    /* check if this is a read or a write */
    if (!xml_strcmp(access, nacm_E_allowedRights_create)) {
        isread = FALSE;
    } else if (!xml_strcmp(access, nacm_E_allowedRights_update)) {
        isread = FALSE;
    } else if (!xml_strcmp(access, nacm_E_allowedRights_delete)) {
        isread = FALSE;
    } else if (!xml_strcmp(access, nacm_E_allowedRights_exec)) {
        isread = FALSE;
    } else {
        isread = TRUE;
    }

    switch (acmode) {
    case AGT_ACMOD_ENFORCING:
        break;
    case AGT_ACMOD_PERMISSIVE:
        if (isread && !obj_is_very_secure(obj)) {
            return TRUE;
        }
        break;
    case AGT_ACMOD_DISABLED:
        if (isread) {
            if (!obj_is_very_secure(obj)) {
                return TRUE;
            }
        } else {
            if (!(obj_is_secure(obj) || obj_is_very_secure(obj))) {
                return TRUE;
            }
        }
        break;
    case AGT_ACMOD_OFF:
        return TRUE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return FALSE;

}  /* check_mode */


/********************************************************************
* FUNCTION new_modrule
*
* create a moduleRule cache entry
*
* INPUTS:
*   nsid == module namspace ID
*   rule == back-ptr to moduleRule entry
*
* RETURNS:
*   filled in, malloced struct or NULL if malloc error
*********************************************************************/
static agt_acm_modrule_t *
    new_modrule (xmlns_id_t nsid,
                 val_value_t *rule)
{
    agt_acm_modrule_t *modrule;

    modrule = m__getObj(agt_acm_modrule_t);
    if (!modrule) {
        return NULL;
    }
    memset(modrule, 0x0, sizeof(agt_acm_modrule_t));

    modrule->nsid = nsid;
    modrule->modrule = rule;
    return modrule;

}  /* new_modrule */


/********************************************************************
* FUNCTION free_modrule
*
* free a moduleRule cache entry
*
* INPUTS:
*   modrule == entry to free
*
*********************************************************************/
static void
    free_modrule (agt_acm_modrule_t *modrule)
{
    m__free(modrule);

}  /* free_modrule */


/********************************************************************
* FUNCTION new_datarule
*
* create a data rule cache entry
*
* INPUTS:
*   pcb == parser control block to cache  (live memory freed later)
*   result == XPath result to cache   (live memory freed later)
*   rule == back-ptr to dataRule entry
*
* RETURNS:
*   filled in, malloced struct or NULL if malloc error
*********************************************************************/
static agt_acm_datarule_t *
    new_datarule (xpath_pcb_t *pcb,
                  xpath_result_t *result,
                  val_value_t *rule)
{
    agt_acm_datarule_t *datarule;

    datarule = m__getObj(agt_acm_datarule_t);
    if (!datarule) {
        return NULL;
    }
    memset(datarule, 0x0, sizeof(agt_acm_datarule_t));

    datarule->pcb = pcb;
    datarule->result = result;
    datarule->datarule = rule;
    return datarule;

}  /* new_datarule */


/********************************************************************
* FUNCTION free_datarule
*
* free a dataRule cache entry
*
* INPUTS:
*   datarule == entry to free
*
*********************************************************************/
static void
    free_datarule (agt_acm_datarule_t *datarule)
{
    xpath_free_result(datarule->result);
    xpath_free_pcb(datarule->pcb);
    m__free(datarule);
}  /* free_datarule */


/********************************************************************
* FUNCTION new_group_ptr
*
* create a group pointer
*
* INPUTS:
*   name == group identity name to use
*
* RETURNS:
*   filled in, malloced struct or NULL if malloc error
*********************************************************************/
static agt_acm_group_t *
    new_group_ptr (const xmlChar *groupname)
{
    agt_acm_group_t *grptr;

    grptr = m__getObj(agt_acm_group_t);
    if (!grptr) {
        return NULL;
    }
    memset(grptr, 0x0, sizeof(agt_acm_group_t));
    grptr->groupname = groupname;
    return grptr;

}  /* new_group_ptr */


/********************************************************************
* FUNCTION free_group_ptr
*
* free a group pointer
*
* INPUTS:
*   grptr == group to free
*
* RETURNS:
*   filled in, malloced struct or NULL if malloc error
*********************************************************************/
static void
    free_group_ptr (agt_acm_group_t *grptr)
{
    m__free(grptr);

}  /* free_group_ptr */


/********************************************************************
* FUNCTION find_group_ptr
*
* find a group pointer in a user group record
*
* INPUTS:
*   usergroups == user-to-groups struct to check
*   name == group identity name to find
*
* RETURNS:
*   pointer to found record or NULL if not found
*********************************************************************/
static agt_acm_group_t *
    find_group_ptr (agt_acm_usergroups_t *usergroups,
                    const xmlChar *name)
{
    agt_acm_group_t   *grptr;

    for (grptr = (agt_acm_group_t *)
             dlq_firstEntry(&usergroups->groupQ);
         grptr != NULL;
         grptr = (agt_acm_group_t *)dlq_nextEntry(grptr)) {

        if (!xml_strcmp(grptr->groupname, name)) {
            return grptr;
        }
    }
    return NULL;

}  /* find_group_ptr */


/********************************************************************
* FUNCTION add_group_ptr
*
* add a group pointer in a user group record
* if it is not already there
*
* INPUTS:
*   usergroups == user-to-groups struct to use
*   name == group identity name to add
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_group_ptr (agt_acm_usergroups_t *usergroups,
                   const xmlChar *name)
{
    agt_acm_group_t   *grptr;

    grptr = find_group_ptr(usergroups, name);
    if (grptr) {
        return NO_ERR;
    }

    grptr = new_group_ptr(name);
    if (!grptr) {
        return ERR_INTERNAL_MEM;
    }

    dlq_enque(grptr, &usergroups->groupQ);
    return NO_ERR;

}  /* add_group_ptr */


/********************************************************************
* FUNCTION new_usergroups
*
* create a user-to-groups struct
*
* INPUTS:
*   username == name of user to use
*
* RETURNS:
*   filled in, malloced struct or NULL if malloc error
*********************************************************************/
static agt_acm_usergroups_t * new_usergroups (const xmlChar *username)
{
    agt_acm_usergroups_t *usergroups;

    usergroups = m__getObj(agt_acm_usergroups_t);
    if (!usergroups) {
        return NULL;
    }

    memset(usergroups, 0x0, sizeof(agt_acm_usergroups_t));
    dlq_createSQue(&usergroups->groupQ);
    usergroups->username = xml_strdup(username);
    if ( !usergroups->username ) {
        m__free(usergroups);
        return NULL;
    }

    return usergroups;
}  /* new_usergroups */


/********************************************************************
* FUNCTION free_usergroups
*
* free a user-to-groups struct
*
* INPUTS:
*   usergroups == struct to free
*
*********************************************************************/
static void
    free_usergroups (agt_acm_usergroups_t *usergroups)
{
    agt_acm_group_t *grptr;

    while (!dlq_empty(&usergroups->groupQ)) {
        grptr = (agt_acm_group_t *)
            dlq_deque(&usergroups->groupQ);
        free_group_ptr(grptr);
    }
    if (usergroups->username) {
        m__free(usergroups->username);
    }
    m__free(usergroups);

}  /* free_usergroups */


/********************************************************************
* FUNCTION get_usergroups_entry
*
* create (TBD: cache) a user-to-groups entry
* for the specified username, based on the /nacm/groups
* contents at this time
*
* INPUTS:
*   nacmroot == root of the nacm tree, already fetched
*   username == user name to create mapping for
*   groupcount == address of return group count field
*
* OUTPUTS:
*   *groupcount == number of groups that the specified
*                  user is part of (i.e., number of 
*                  agt_acm_group_t structs in the groups Q
*
* RETURNS:
*  malloced usergroups entry for the specified user
*********************************************************************/
static agt_acm_usergroups_t *
    get_usergroups_entry (val_value_t *nacmroot,
                          const xmlChar *username,
                          uint32 *groupcount)
{
    agt_acm_usergroups_t  *usergroups;
    val_value_t           *groupsval, *groupval, *group_name_val, *userval;
    boolean                done;
    status_t               res;

    *groupcount = 0;
    res = NO_ERR;

    /*** no cache yet -- just create entry each time for now ***/

    usergroups = new_usergroups(username);
    if (!usergroups) {
        return NULL;
    }

    /* get /nacm/groups node */
    groupsval = val_find_child(nacmroot,
                               AGT_ACM_MODULE,
                               nacm_N_groups);
    if (!groupsval) {
        return usergroups;
    }

    /* check each /nacm/groups/group node */
    for (groupval = val_get_first_child(groupsval);
         groupval != NULL && res == NO_ERR;
         groupval = val_get_next_child(groupval)) {

        done = FALSE;
        group_name_val = val_find_child(groupval,
                                     AGT_ACM_MODULE,
                                     nacm_N_name);
        assert(group_name_val!=NULL);
        /* check each /nacm/groups/group/user-name node */
        for (userval = val_find_child(groupval,
                                      AGT_ACM_MODULE,
                                      nacm_N_userName);
             userval != NULL && !done;
             userval = val_find_next_child(groupval,
                                           AGT_ACM_MODULE,
                                           nacm_N_userName,
                                           userval)) {

            if (!xml_strcmp(username, VAL_STR(userval))) {
                /* user is a member of this group
                 * get the groupIdentity key leaf
                 */
                done = TRUE;
                res = add_group_ptr(usergroups,
                                    VAL_STRING(group_name_val));
                (*groupcount)++;
            }
        }
    }

    if (res != NO_ERR) {
        log_error("\nError: agt_acm add user2group entry failed");
    }

    return usergroups;
    
}  /* get_usergroups_entry */


/********************************************************************
* FUNCTION new_acm_cache
*
* Malloc and initialize an agt_acm_cache_t stuct
*
* RETURNS:
*   malloced msg cache or NULL if error
*********************************************************************/
static agt_acm_cache_t  *
    new_acm_cache (void)
{
    agt_acm_cache_t  *acm_cache;
    int i;
    acm_cache = m__getObj(agt_acm_cache_t);
    if (!acm_cache) {
        return NULL;
    }
    memset(acm_cache, 0x0, sizeof(agt_acm_cache_t));
    dlq_createSQue(&acm_cache->modruleQ);
    for(i=0;i<DATA_RULE_QUEUE_NUM;i++) {
        dlq_createSQue(&acm_cache->dataruleQ[i]);
    }
    acm_cache->mode = acmode;
    acm_cache->flags = FL_ACM_CACHE_VALID;
    return acm_cache;

} /* new_acm_cache */


/********************************************************************
* FUNCTION free_acm_cache
*
* Clean and free a agt_acm_cache_t struct
*
* INPUTS:
*   acm_cache == cache struct to free
*********************************************************************/
static void
    free_acm_cache (agt_acm_cache_t  *acm_cache)
{
    agt_acm_modrule_t    *modrule;
    agt_acm_datarule_t   *datarule;
    int i;

    while (!dlq_empty(&acm_cache->modruleQ)) {
        modrule = (agt_acm_modrule_t *)
            dlq_deque(&acm_cache->modruleQ);
        free_modrule(modrule);
    }

    for(i=0;i<DATA_RULE_QUEUE_NUM;i++) {
        while (!dlq_empty(&acm_cache->dataruleQ[i])) {
            datarule = (agt_acm_datarule_t *)
            dlq_deque(&acm_cache->dataruleQ[i]);
            free_datarule(datarule);
        }
    }

    if (acm_cache->usergroups) {
        free_usergroups(acm_cache->usergroups);
    }

    m__free(acm_cache);

} /* free_acm_cache */


/********************************************************************
* FUNCTION check_access_bit
*
*
* INPUTS:
*    ruleval == /nacm/rule-list/rule node, pre-fetched
*    access-operation == string (enum name) for the requested access
*              (read, create, update, delete, exec)
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* OUTPUTS:
*    *done == TRUE if a matching group was found, 
*             so return value is the final answer
*          == FALSE if no matching group was found
*
* RETURNS:
*    only valid if *done == TRUE:
*      TRUE if requested access right found 
*      FALSE if requested access right not found
*********************************************************************/
static boolean
    check_access_bit (val_value_t *ruleval,
                      const xmlChar *access,
                      agt_acm_usergroups_t *usergroups,
                      boolean *done)
{
    val_value_t      *group, *rights;
    boolean           granted;

    *done = FALSE;
    granted = FALSE;

    for (group = val_find_child(ruleval->parent,
                                AGT_ACM_MODULE,
                                nacm_N_group);
         group != NULL && !*done;
         group = val_find_next_child(ruleval->parent,
                                      AGT_ACM_MODULE,
                                      nacm_N_name,
                                      group)) 
        {
        if ((0==strcmp((char*)VAL_STRING(group),"*")) || find_group_ptr(usergroups,VAL_STRING(group))) 
            {
            /* this group is a match */
            *done = TRUE;

            /* get the allowedRights leaf
             * and check if the exec bit is set
             */
            rights = val_find_child(ruleval,
                                    AGT_ACM_MODULE,
                                    nacm_N_accessOperations);
            assert(rights);
            if (0==strcmp((char*)VAL_STRING(rights),"*")) 
            {
                granted=TRUE;
            } else {
#if 0
                ncx_list_t       *bits;
                bits = &VAL_BITS(rights);
                granted = ncx_string_in_list(access, bits);
#else
                if(strstr((char*)VAL_STRING(rights), (const char*)access) != NULL) {
                    granted = TRUE;
                }
#endif
            }
        }
    }
                                   
    return granted;

} /* check_access_bit */

/********************************************************************
* FUNCTION check_rule_group
*
*
* INPUTS:
*    ruleval == /nacm/rule-list/rule node, pre-fetched
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* RETURNS:
*      TRUE if group was found
*      FALSE if group was not found
*********************************************************************/
static boolean
    check_rule_group (val_value_t *ruleval,
                      agt_acm_usergroups_t *usergroups)
{
    val_value_t      *group;

    for (group = val_find_child(ruleval->parent,
                                AGT_ACM_MODULE,
                                nacm_N_group);
         group != NULL;
         group = val_find_next_child(ruleval->parent,
                                      AGT_ACM_MODULE,
                                      nacm_N_name,
                                      group)) {
        if ((0==strcmp((char*)VAL_STRING(group),"*")) || find_group_ptr(usergroups,
                           VAL_STRING(group))) {
            return TRUE;
        }
    }

    return FALSE;

} /* check_rule_group */


/********************************************************************
* FUNCTION get_nacm_root
*
* get the /nacm root object
*
* RETURNS:
*   pointer to root or NULL if none
*********************************************************************/
static val_value_t *
    get_nacm_root (void)
{
    cfg_template_t        *runningcfg;
    val_value_t           *nacmval;

    /* make sure the running config root is set */
    runningcfg = cfg_get_config(NCX_EL_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return NULL;
    }
    
    nacmval = val_find_child(runningcfg->root,
                             AGT_ACM_MODULE,
                             nacm_N_nacm);

    return nacmval;

}  /* get_nacm_root */


/********************************************************************
* FUNCTION get_default_rpc_response
*
* get the default response for the specified RPC object
* there are no rules that match any groups with this user
*
*  INPUTS:
*    cache == agt_acm cache to check
*    nacmroot == pre-fectched NACM root
*    rpcobj == RPC template for this request
*    
* RETURNS:
*   TRUE if access granted
*   FALSE if access denied
*********************************************************************/
static boolean
    get_default_rpc_response (agt_acm_cache_t *cache,
                              val_value_t *nacmroot,
                              const obj_template_t *rpcobj)
{
    val_value_t  *noRule;
    boolean       retval;

    /* check if the RPC method is tagged as 
     * ncx:secure or ncx:very-secure and
     * deny access if so
     */
    if (obj_is_secure(rpcobj) ||
        obj_is_very_secure(rpcobj)) {
        return FALSE;
    }

    /* check the noDefaultRule setting on this agent */
    if (cache->flags & FL_ACM_DEFEXEC_SET) {
        return (cache->flags & FL_ACM_DEFEXEC_OK) ? 
            TRUE : FALSE;
    }

    noRule = val_find_child(nacmroot,
                            AGT_ACM_MODULE,
                            nacm_N_execDefault);
    if (!noRule) {
        cache->flags |= (FL_ACM_DEFEXEC_SET | FL_ACM_DEFEXEC_OK);
        return TRUE;  /* default is TRUE */
    } 

    if (!xml_strcmp(VAL_ENUM_NAME(noRule),
                    nacm_E_noRuleDefault_permit)) {
        retval = TRUE;
    } else {
        retval = FALSE;
    }
    cache->flags |= FL_ACM_DEFEXEC_SET;
    if (retval) {
        cache->flags |= FL_ACM_DEFEXEC_OK;
    }

    return retval;

}  /* get_default_rpc_response */


/********************************************************************
* FUNCTION check_rpc_rules
*
* Check the configured /nacm/rule-list/rule lists to see if the
* user is allowed to invoke the specified RPC method
*
* INPUTS:
*    nacmrootval == /nacm node, pre-fetched
*    rpcobj == RPC template requested
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* OUTPUTS:
*    *done == TRUE if a rule was found, so return value is
*             the final answer
*          == FALSE if no rpcRule was found to match
*
* RETURNS:
*    only valid if *done == TRUE:
*      TRUE if authorization to invoke the RPC op is granted
*      FALSE if authorization to invoke the RPC op is not granted
*********************************************************************/
static boolean
    check_rpc_rules (val_value_t *nacmroot,
                     const obj_template_t *rpcobj,
                     agt_acm_usergroups_t *usergroups,
                     boolean *done)
{
    val_value_t      *rule_list, *rule, *modname, *rpcname;
    boolean           granted, done2;
    status_t          res;

    *done = FALSE;
    granted = FALSE;
    res = NO_ERR;

    /* check all the entries */
for (rule_list = val_find_child(nacmroot,
                                    AGT_ACM_MODULE,
                                    nacm_N_ruleList);
     rule_list != NULL && res == NO_ERR && !*done;
     rule_list = val_find_next_child(nacmroot,
                                   AGT_ACM_MODULE,
                                   nacm_N_ruleList,
                                   rule_list)) {
    for (rule = val_find_child(rule_list, 
                                  AGT_ACM_MODULE, 
                                  nacm_N_rule);
         rule != NULL && res == NO_ERR && !*done;
         rule = val_find_next_child(rule_list,
                                       AGT_ACM_MODULE,
                                       nacm_N_rule,
                                       rule)) {

        /* get the rpc operation name key */
        rpcname = val_find_child(rule,
                                 AGT_ACM_MODULE,
                                 nacm_N_rpcName);
        if (!rpcname) {
            continue;
        }

        /* get the module name key */
        modname = val_find_child(rule,
                                 AGT_ACM_MODULE,
                                 nacm_N_moduleName);


        /* check if this is the right module */
        if ((modname != NULL) &&
            xml_strcmp((const xmlChar*)"*", (const xmlChar*)VAL_STR(modname)) &&
            xml_strcmp(obj_get_mod_name(rpcobj), VAL_STR(modname)) &&
            (xml_strcmp(obj_get_mod_name(rpcobj), (const xmlChar*)"yuma-netconf") && xml_strcmp((const xmlChar*)"ietf-netconf", VAL_STR(modname)))
           ) {
            continue;
        }

        /* check if this is the right RPC operation */
        if (xml_strcmp(obj_get_name(rpcobj),
                       VAL_STR(rpcname))) {
            continue;
        }

        /* this rpc rule is for the specified RPC operation
         * check if exec access is allowed
         */
        done2 = FALSE;
        granted = check_access_bit(rule,
                                   nacm_E_allowedRights_exec,
                                   usergroups,
                                   &done2);
        if (done2) {
            *done = TRUE;
        } else {
            granted = FALSE;
        }
    }
}
    if (res != NO_ERR) {
        granted = FALSE;
        *done = TRUE;
    }

    return granted;

} /* check_rpc_rules */


/********************************************************************
* FUNCTION check_module_rules
*
* Check the configured /nacm/rules/moduleRule list to see if the
* user is allowed access the specified module
*
* INPUTS:
*    cache == cache to use
*    rulesval == /nacm/rules node, pre-fetched
*    obj == RPC or data template requested
*    access == string (enum name) for the requested access
*              (read, write, exec)
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* OUTPUTS:
*    *done == TRUE if a rule was found, so return value is
*             the final answer
*          == FALSE if no rpcRule was found to match
*
* RETURNS:
*    only valid if *done == TRUE:
*      TRUE if authorization to invoke the RPC op is granted
*      FALSE if authorization to invoke the RPC op is not granted
*********************************************************************/
static boolean
    check_module_rules (agt_acm_cache_t *cache,
                        val_value_t *nacmroot,
                        const obj_template_t *obj,
                        const xmlChar *access,
                        agt_acm_usergroups_t *usergroups,
                        boolean *done)
{
    val_value_t        *rule_list, *modrule, *modname;
    agt_acm_modrule_t  *modrule_cache;
    boolean             granted;
    xmlns_id_t          nsid;
    status_t            res;

    *done = FALSE;
    granted = FALSE;
    res = NO_ERR;

    return FALSE;
    /*TODO: Not ready. */
    rule_list = val_find_child(nacmroot,
                             AGT_ACM_MODULE,
                             nacm_N_ruleList);

    if (!(cache->flags & FL_ACM_MODRULES_SET)) {
        cache->flags |= FL_ACM_MODRULES_SET;

        /* check all the moduleRule entries */
        for (modrule = val_find_child(rule_list,
                                      AGT_ACM_MODULE,
                                      nacm_N_moduleRule);
             modrule != NULL && res == NO_ERR;
             modrule = val_find_next_child(rule_list,
                                           AGT_ACM_MODULE,
                                           nacm_N_moduleRule,
                                           modrule)) {

            /* get the module name key */
            modname = val_find_child(modrule,
                                     AGT_ACM_MODULE,
                                     nacm_N_moduleName);
            if (!modname) {
                res = SET_ERROR(ERR_INTERNAL_VAL);
                continue;
            }

            nsid = xmlns_find_ns_by_module(VAL_STR(modname));
            if (nsid) {
                modrule_cache = new_modrule(nsid, modrule);
                if (!modrule_cache) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    dlq_enque(modrule_cache, &cache->modruleQ);
                }
            } else {
                /* this rule is for a module that is not
                 * loaded into the system at this time
                 * just skip this entry;
                 */
                ;
            }
        }
    }

    /* get the namespace ID to check against */
    nsid = obj_get_nsid(obj);

    /* go through the cache and exit if any matches are found */
    for (modrule_cache = (agt_acm_modrule_t *)
             dlq_firstEntry(&cache->modruleQ);
         modrule_cache != NULL && res == NO_ERR && !*done;
         modrule_cache = (agt_acm_modrule_t *)
             dlq_nextEntry(modrule_cache)) {

            /* check if this is the right module */
        if (nsid != modrule_cache->nsid) {
            continue;
        }

        /* this module rule is for the specified module
         * check if the requested access is allowed
         */

        granted = check_access_bit(modrule_cache->modrule,
                                   access,
                                   usergroups,
                                   done);
    }

    if (res != NO_ERR) {
        granted = FALSE;
        *done = TRUE;
    }

    return granted;

} /* check_module_rules */


/********************************************************************
* FUNCTION get_default_data_response
*
* get the default response for the specified data object
* there are no rules that match any groups with this user
*
*  INPUTS:
*    cache == agt_acm cache to use
*    nacmroot == pre-fectched NACM root
*    obj == data object template for this request
*    iswrite == TRUE for write access
*               FALSE for read access
*
* RETURNS:
*   TRUE if access granted
*   FALSE if access denied
*********************************************************************/
static boolean
    get_default_data_response (agt_acm_cache_t *cache,
                               val_value_t *nacmroot,
                               const val_value_t *val,
                               boolean iswrite)
{
    const obj_template_t  *testobj;
    val_value_t           *noRule;
    boolean                retval;

    /* check if the data node is tagged as 
     * ncx:secure or ncx:very-secure and
     * deny access if so
     */
    testobj = val->obj;

    /* special case -- there are no ACM rules for the
     * config root, so allow all writes on this
     * container and start checking at the top-level
     * YANG nodes instead
     */
    if (iswrite && obj_is_root(val->obj)) {
        return TRUE;
    }

    /* make sure this is not an nested object within a
     * object tagged as ncx:secure or ncx:very-secure
     */
    while (testobj) {
        if (iswrite) {
            /* reject any ncx:secure or ncx:very-secure object */
            if (obj_is_secure(testobj) ||
                obj_is_very_secure(testobj)) {
                return FALSE;
            }
        } else {
            /* allow ncx:secure to be read; reject ncx:very-secure */
            if (obj_is_very_secure(testobj)) {
                return FALSE;
            }
        }

        /* stop at root */
        if (obj_is_root(testobj)) {
            testobj = NULL;
        } else {
            testobj = testobj->parent;
        }

        /* no need to check further if the parent was the root
         * need to make sure not to go past the
         * config parameter into the rpc input
         * then the secret <load-config> rpc
         */
        if (testobj && obj_is_root(testobj)) {
            testobj = NULL;
        }
    }

    /* check the noDefaultRule setting on this agent */
    if (iswrite) {
        if (cache->flags & FL_ACM_DEFWRITE_SET) {
            return (cache->flags & FL_ACM_DEFWRITE_OK) ?
                TRUE : FALSE;
        }
        noRule = val_find_child(nacmroot,
                                AGT_ACM_MODULE,
                                nacm_N_writeDefault);
        if (!noRule) {
            cache->flags |= FL_ACM_DEFWRITE_SET;
            return FALSE;  /* default is FALSE */
        }

        if (!xml_strcmp(VAL_ENUM_NAME(noRule),
                        nacm_E_noRuleDefault_permit)) {
            retval = TRUE;
        } else {
            retval = FALSE;
        }

        cache->flags |= FL_ACM_DEFWRITE_SET;
        if (retval) {
            cache->flags |= FL_ACM_DEFWRITE_OK;
        }
    } else {
        if (cache->flags & FL_ACM_DEFREAD_SET) {
            return (cache->flags & FL_ACM_DEFREAD_OK) ?
                TRUE : FALSE;
        }
        noRule = val_find_child(nacmroot,
                                AGT_ACM_MODULE,
                                nacm_N_readDefault);
        if (!noRule) {
            cache->flags |= (FL_ACM_DEFREAD_SET | FL_ACM_DEFREAD_OK);
            return TRUE;  /* default is TRUE */
        }

        if (!xml_strcmp(VAL_ENUM_NAME(noRule),
                        nacm_E_noRuleDefault_permit)) {
            retval = TRUE;
        } else {
            retval = FALSE;
        }

        cache->flags |= FL_ACM_DEFREAD_SET;
        if (retval) {
            cache->flags |= FL_ACM_DEFREAD_OK;
        }
    }

    return retval;

}  /* get_default_data_response */

static const char* get_rule_queue_access_str(unsigned int access_id)
{
    if(access_id==DATA_RULE_QUEUE_READ) {
        return "read";
    } else if(access_id==DATA_RULE_QUEUE_UPDATE) {
        return "update";
    } else if(access_id==DATA_RULE_QUEUE_CREATE) {
        return "create";
    } else if(access_id==DATA_RULE_QUEUE_DELETE) {
        return "delete";
    } else {
        assert(0);
    }
}

static unsigned int get_rule_queue_access_id(const char* access_str)
{
    if(0==strcmp("read", access_str)) {
        return DATA_RULE_QUEUE_READ;
    } else if(0==strcmp("update", access_str)) {
        return DATA_RULE_QUEUE_UPDATE;
    } else if(0==strcmp("create", access_str)) {
        return DATA_RULE_QUEUE_CREATE;
    } else if(0==strcmp("delete", access_str)) {
        return DATA_RULE_QUEUE_DELETE;
    } else {
        assert(0);
    }
}

/********************************************************************
* FUNCTION cache_data_rules
*
* Cache the data rules.
* -- If there is a failure partway through caching the items
*    all the new datarules will be deleted
* -- Do not set the flag unless the entire operation succeeds
* -- On failure remove all added datarules
*
* INPUTS:
*    cache == agt_acm cache to use
*    nacmroot == /nacm node
*
* OUTPUTS:
*    *cache is modified with the cached datarules items.
*
* RETURNS:
*    NO_ERR on success or an error if the operation failed.
*
*********************************************************************/
static status_t
    cache_data_rules( agt_acm_cache_t *cache,
                      val_value_t *nacmroot)
{
    status_t            res = NO_ERR;
    val_value_t         *rule, *rule_list;
    val_value_t         *valroot;
    int                 i;

    if (cache->flags & FL_ACM_DATARULES_SET) {
        // datarules have already been already cached, return no error
        //return NO_ERR;


        /* regenerate all cached datarules since no mechanism for updating config=false rules is in place yet */
        agt_acm_datarule_t  *freerule;
        for(i=0;i<DATA_RULE_QUEUE_NUM;i++) {
            while (!dlq_empty(&cache->dataruleQ[i])) {
                freerule = (agt_acm_datarule_t *)dlq_deque(&cache->dataruleQ[i]);
                if (freerule) {
                    free_datarule(freerule);
                } else {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                }
            }
        }
    }

    /* the /nacm node is supposed to be a child of <config> */
    valroot = nacmroot->parent;
    if (!valroot || !obj_is_root(valroot->obj)) 
    {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

for ( rule_list = val_find_child( nacmroot, AGT_ACM_MODULE, 
                                     nacm_N_ruleList );
          rule_list != NULL;
          rule_list = val_find_next_child( rule_list, AGT_ACM_MODULE,
                                          nacm_N_ruleList, rule_list ) ) {
    /* check all the rule entries */
    for ( rule = val_find_child( rule_list, AGT_ACM_MODULE, 
                                     nacm_N_rule );
          rule != NULL;
          rule = val_find_next_child( rule_list, AGT_ACM_MODULE,
                                          nacm_N_rule, rule ) ) 
    {
        val_value_t         *path;
        val_value_t         *access_operations;
        xpath_pcb_t         *pcb;
        xpath_result_t      *result;


        /* get the XPath expression leaf */
        path = val_find_child( rule, AGT_ACM_MODULE, nacm_N_path );
        if ( !path || !path->xpathpcb ) {
            continue;
        }

        access_operations = val_find_child( rule, AGT_ACM_MODULE, nacm_N_accessOperations );
        if ( !access_operations ) {
            continue;
        }

        for(i=0;i<DATA_RULE_QUEUE_NUM;i++) {

            /* make sure the source is not XML so the defunct reader
             * does not get accessed; the clone should save the
             * NSID bindings in all the tokens
             */
          if((0!=strcmp("*",(const char*)VAL_STRING(access_operations))) && (NULL==strstr(get_rule_queue_access_str(i),(const char*)VAL_STRING(access_operations)))) {
                continue;
            }

            pcb = xpath_clone_pcb( path->xpathpcb );
            if(!pcb) {
                res = ERR_INTERNAL_MEM;
                break;
            }

            pcb->source = XP_SRC_YANG;
            result = xpath1_eval_expr( pcb, valroot, valroot, FALSE,
                                       (i==DATA_RULE_QUEUE_READ)?FALSE:TRUE /*configonly*/, &res );
            if ( !result ) {
                res = ERR_INTERNAL_MEM;
                xpath_free_pcb(pcb);
                break;
            }

            if ( res == NO_ERR) {
                agt_acm_datarule_t  *datarule_cache;
                datarule_cache = new_datarule(pcb, result, rule);
                if ( datarule_cache ) {
                    /* pass off 'pcb' and 'result' memory here */
                    dlq_enque( datarule_cache, &cache->dataruleQ[i] );
                } else {
                    res = ERR_INTERNAL_MEM;
                }
            }

            if ( res != NO_ERR ) {
                xpath_free_pcb( pcb );
                xpath_free_result( result );
                break;
            }
        }
    }
}
    if (res == NO_ERR) {
        cache->flags |= FL_ACM_DATARULES_SET;
    } else {
        for(i=0;i<DATA_RULE_QUEUE_NUM;i++) {

            agt_acm_datarule_t  *freerule;
            while (!dlq_empty(&cache->dataruleQ[i])) {
                freerule = (agt_acm_datarule_t *)dlq_deque(&cache->dataruleQ[i]);
                if (freerule) {
                    free_datarule(freerule);
                } else {
                    SET_ERROR(ERR_INTERNAL_VAL);
                }
            }
            log_error("\nError: cache NACM data rules failed! (%s)",
                      get_error_string(res));
        }
    }

    return res;
}


/********************************************************************
* FUNCTION check_data_rules
*
* Check the configured /nacm/rules/dataRule list to see if the
* user is allowed access the specified data object
*
* INPUTS:
*    cache == agt_acm cache to use
*    nacmroot == /nacm node prefetched
*    val == value node requested
*    access == string (enum name) for the requested access
*              (read, write)
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* OUTPUTS:
*    *done == TRUE if a rule was found, so return value is
*             the final answer
*          == FALSE if no dataRule was found to match
*
* RETURNS:
*    only valid if *done == TRUE:
*      TRUE if authorization to access data is granted
*      FALSE if authorization to access data is not granted
*********************************************************************/
static boolean
    check_data_rules (agt_acm_cache_t *cache,
                      val_value_t *nacmroot,
                      const val_value_t *val,
                      const xmlChar *access,
                      agt_acm_usergroups_t *usergroups,
                      boolean *done)
{
    dlq_hdr_t           *resnodeQ;
    agt_acm_datarule_t  *datarule_cache;
    boolean              granted = FALSE;
    status_t             res = NO_ERR;
    int                  access_id;
    *done = FALSE;

    /* fill the dataruleQ in the cache if needed */
#if 0
    if (!(cache->flags & FL_ACM_DATARULES_SET)) 
#else
    if (1)
#endif
    {
        res = cache_data_rules( cache, nacmroot);
    }

    if ( res != NO_ERR )
    {
        *done = FALSE;
        return FALSE;
    }

    access_id = get_rule_queue_access_id((const char*)access);
    {

        /* go through the cache and exit if any matches are found */
        for ( datarule_cache = (agt_acm_datarule_t *) 
                  dlq_firstEntry(&cache->dataruleQ[access_id]);
              datarule_cache != NULL && !*done;
              datarule_cache = (agt_acm_datarule_t *) 
                  dlq_nextEntry(datarule_cache)) 
        {
            if(!check_rule_group (datarule_cache->datarule, usergroups)) {
                continue;
            }

            resnodeQ = xpath_get_resnodeQ(datarule_cache->result);
            assert(resnodeQ);

            if ( ((access_id==DATA_RULE_QUEUE_READ) && xpath1_check_node_child_exists_slow( datarule_cache->pcb,resnodeQ, val )) ||
                 ((access_id==DATA_RULE_QUEUE_UPDATE) && !obj_is_leaf(val->obj)) ||
                 xpath1_check_node_exists_slow( datarule_cache->pcb,
                                                resnodeQ, val ))
            {
                *done = TRUE;
                granted = TRUE;
            }
        }
    }

    if ( res != NO_ERR )
    {
        *done = TRUE;
    } 
    return granted;

} /* check_data_rules */


/********************************************************************
* FUNCTION valnode_access_allowed
*
* Check if the specified user is allowed to access a value node
* The val->obj template will be checked against the val->editop
* requested ac
cess and the user's configured max-access
* 
* INPUTS:
*   cache == cache for this session/message
*   user == user name string
*   val  == val_value_t in progress to check
*   newval  == newval val_value_t in progress to check (write only)
*   curval  == curval val_value_t in progress to check (write only)
*   editop == edit operation if this is a write; ignored otherwise
* RETURNS:
*   TRUE if user allowed this level of access to the value node
*********************************************************************/
static boolean 
    valnode_access_allowed (agt_acm_cache_t *cache,
                            const xmlChar *user,
                            const val_value_t *val,
                            const val_value_t *newval,
                            const val_value_t *curval,
                            op_editop_t editop)
{
    const xmlChar* access;
    val_value_t *nacmroot = NULL;
    boolean      iswrite;
    logfn_t      logfn;

    /* check if this is a read or a write */
    if ((newval!=NULL) || (curval!=NULL)) {
        iswrite = TRUE;
        logfn = (log_writes) ? log_debug2 : log_noop;
    } else {
        iswrite = FALSE;
        logfn = (log_reads) ? log_debug4 : log_noop;
    }

    /* make sure object is not the special node <config> */
    if (obj_is_root(val->obj)) {
        (*logfn)("\nagt_acm: PERMIT (root-node)");
        return TRUE;
    }

    /* ncx:user-write blocking has highest priority */
    if(iswrite) {
        switch (editop) {
        case OP_EDITOP_CREATE:
          access = (const xmlChar*) "create";
            if (obj_is_block_user_create(val->obj)) {
                (*logfn)("\nagt_acm: DENY (block-user-create)");
                return FALSE;
            }
            break;
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            access = (const xmlChar*)"delete";
            if (obj_is_block_user_delete(val->obj)) {
                (*logfn)("\nagt_acm: DENY (block-user-delete)");
                return FALSE;
            }
            break;
        default:
            /* This is a merge or replace.  If blocked,
             * first make sure this requested operation is
             * also the effective operation; otherwise
             * the user will never be able to update a sub-node
             * of a list or container with update access blocked  */
            access = (const xmlChar*)"update";
            if (obj_is_block_user_update(val->obj)) {
                if (agt_apply_this_node(editop, newval, curval)) {
                    (*logfn)("\nagt_acm: DENY (block-user-update)");
                    return FALSE;
                }
            }
        }
    } else {
        access = (const xmlChar*)"read";
    }

    /* super user is allowed to access anything except user-write blocked */
    if (is_superuser(user)) {
        (*logfn)("\nagt_acm: PERMIT (superuser)");
        return TRUE;
    }

    if (cache->mode == AGT_ACMOD_DISABLED) {
        (*logfn)("\nagt_acm: PERMIT (NACM disabled)");
        return TRUE;
    }

    /* check if access granted without any rules */
    if (check_mode((const xmlChar*)access, val->obj)) {
        (*logfn)("\nagt_acm: PERMIT (permissive mode)");
        return TRUE;
    }


    /* get the NACM root to decide any more */
    if (cache->nacmroot) {
        nacmroot = cache->nacmroot;
    } else {
        nacmroot = get_nacm_root();
        if (!nacmroot) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return FALSE;
        } else {
            cache->nacmroot = nacmroot;
        }
    }

    uint32 groupcnt = 0;
    agt_acm_usergroups_t *usergroups = NULL;
    if (cache->usergroups) {
        usergroups = cache->usergroups;
        groupcnt = cache->groupcnt;
    } else {
        usergroups = get_usergroups_entry(nacmroot, user, &groupcnt);
        if (!usergroups) {
            /* out of memory! deny all access! */
            return FALSE;
        } else {
            cache->usergroups = usergroups;
            cache->groupcnt = groupcnt;
        }
    }

    /* usergroups malloced at this point */
    boolean retval = FALSE;
    boolean done = FALSE;

    const xmlChar *substr = iswrite ? (const xmlChar *)"write-default" :
        (const xmlChar *)"read-default";

    if (groupcnt == 0) {
        /* just check the default for this RPC operation */
        retval = get_default_data_response(cache, nacmroot, val, iswrite);
        // substr set already
    } else {

        /* there is a rules node so check the dataRule list */
        if (!done) {
          retval = check_data_rules(cache, nacmroot, val, (const xmlChar*)access,
                                      usergroups, &done);
            if (done) {
                substr = (const xmlChar *)"data-rule";
            } else {
                /* no data rule found; try a module namespace rule */
                retval = check_module_rules(cache, nacmroot, val->obj,  (const xmlChar*)access,
                                            usergroups, &done);
                if (done) {
                    substr = (const xmlChar *)"module-rule";
                } else {
                    /* no module rule so use the default */
                    retval = get_default_data_response(cache, nacmroot, val,
                                                       iswrite);
                    // substr already set
                }
            }
        }
    }

    if (iswrite) {
        (*logfn)("\nagt_acm: %s write (%s)", retval ? "PERMIT" : "DENY",
                 substr ? substr : NCX_EL_NONE);
    } else {
        (*logfn)("\nagt_acm: %s read (%s)", retval ? "PERMIT" : "DENY",
                 substr ? substr : NCX_EL_NONE);
    }

    return retval;

}   /* valnode_access_allowed */


/********************************************************************
* FUNCTION check_notif_rules
*
* Check the configured /nacm/rules/notification-rule list to see if the
* user is allowed to invoke the specified RPC method
*
* INPUTS:
*    nacmrootval == /nacm node, pre-fetched
*    notifobj == notification template requested
*    usergroups == user-to-group mapping for access processing
*    done == address of return done processing flag
*
* OUTPUTS:
*    *done == TRUE if a rule was found, so return value is
*             the final answer
*          == FALSE if no rpcRule was found to match
*
* RETURNS:
*    only valid if *done == TRUE:
*      TRUE if authorization to receive the notification is granted
*      FALSE if authorization to receive the notification is not granted
*********************************************************************/
static boolean
    check_notif_rules (val_value_t *nacmrootval,
                       const obj_template_t *notifobj,
                       agt_acm_usergroups_t *usergroups,
                       boolean *done)
{
    val_value_t      *modname, *notifname, *rule_list, *rule;
    boolean           granted, done2;
    status_t          res;

    *done = FALSE;
    granted = FALSE;
    res = NO_ERR;
    
    /* check all the notification rule entries */
for (rule_list = val_find_child(nacmrootval, 
                                AGT_ACM_MODULE, 
                                nacm_N_ruleList);
     rule_list != NULL && res == NO_ERR && !*done;
     rule_list = val_find_next_child(nacmrootval,
                                AGT_ACM_MODULE,
                                nacm_N_ruleList,
                                rule_list)) {

    for (rule = val_find_child(rule_list, 
                               AGT_ACM_MODULE, 
                               nacm_N_rule);
         rule != NULL && res == NO_ERR && !*done;
         rule = val_find_next_child(rule_list,
                                         AGT_ACM_MODULE,
                                         nacm_N_rule,
                                         rule)) {


        /* get the module name key */
        modname = val_find_child(rule,
                                 AGT_ACM_MODULE,
                                 nacm_N_moduleName);

        /* get the notification name key */
        notifname = val_find_child(rule,
                                   AGT_ACM_MODULE,
                                   nacm_N_notificationName);
        if (!notifname) {
            continue;
        }

        /* check if this is the right module */

        if ((modname!=NULL) &&
            xml_strcmp((const xmlChar*)"*", VAL_STR(modname)) &&
            xml_strcmp(obj_get_mod_name(notifobj), VAL_STR(modname)) &&
            (xml_strcmp(obj_get_mod_name(notifobj), (const xmlChar*)"yuma-netconf") && xml_strcmp((const xmlChar*)"ietf-netconf", VAL_STR(modname)))
           ) {
            continue;
        }

        /* check if this is the right notification event */
        if (xml_strcmp(obj_get_name(notifobj),
                       VAL_STR(notifname))) {
            continue;
        }

        /* this notification-rule is for the specified event
         * check if any of the groups in the usergroups
         * list for this user match any of the groups in
         * the allowed-group leaf-list
         */
        done2 = FALSE;
        granted = check_access_bit(rule,
                                   nacm_E_allowedRights_read,
                                   usergroups,
                                   &done2);
        if (done2) {
            *done = TRUE;
        } else {
            granted = FALSE;
        }
    }
}
    return granted;

} /* check_notif_rules */


/********************************************************************
* FUNCTION get_default_notif_response
*
* get the default response for the specified notification object
* there are no rules that match any groups with this user
*
*  INPUTS:
*    cache == agt_acm cache to check
*    nacmroot == pre-fectched NACM root
*    notifobj == notification template for this request
*    
* RETURNS:
*   TRUE if access granted
*   FALSE if access denied
*********************************************************************/
static boolean
    get_default_notif_response (agt_acm_cache_t *cache,
                                val_value_t *nacmroot,
                                const obj_template_t *notifobj)
{
    val_value_t  *noRule;
    boolean       retval;

    /* check if the notification event is tagged as 
     * nacm:secure or nacm:very-secure and
     * deny access if so
     */
    if (obj_is_secure(notifobj) ||
        obj_is_very_secure(notifobj)) {
        return FALSE;
    }

    /* check the read-default setting */
    if (cache->flags & FL_ACM_DEFREAD_SET) {
        return (cache->flags & FL_ACM_DEFREAD_OK) ? 
            TRUE : FALSE;
    }

    noRule = val_find_child(nacmroot,
                            AGT_ACM_MODULE,
                            nacm_N_readDefault);
    if (!noRule) {
        cache->flags |= (FL_ACM_DEFREAD_SET | FL_ACM_DEFREAD_OK);
        return TRUE;  /* default is TRUE */
    } 

    if (!xml_strcmp(VAL_ENUM_NAME(noRule),
                    nacm_E_noRuleDefault_permit)) {
        retval = TRUE;
    } else {
        retval = FALSE;
    }
    cache->flags |= FL_ACM_DEFREAD_SET;
    if (retval) {
        cache->flags |= FL_ACM_DEFREAD_OK;
    }

    return retval;

}  /* get_default_notif_response */


/********************************************************************
* FUNCTION get_denied_perations
*
* <get> operation handler for the nacm/denied_operations counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_denied_operations (ses_cb_t *scb,
                    getcb_mode_t cbmode,
                    const val_value_t *virval,
                    val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    VAL_UINT(dstval) = denied_operations_count;
    return NO_ERR;

} /* get_denied_operations */


/********************************************************************
* FUNCTION get_deniedDataWrites
*
* <get> operation handler for the nacm/deniedDataWrites counter
*
* INPUTS:
*    see ncx/getcb.h getcb_fn_t for details
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_deniedDataWrites (ses_cb_t *scb,
                          getcb_mode_t cbmode,
                          const val_value_t *virval,
                          val_value_t  *dstval)
{
    (void)scb;
    (void)virval;

    if (cbmode != GETCB_GET_VALUE) {
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    VAL_UINT(dstval) = denied_data_writes_count;
    return NO_ERR;

} /* get_deniedDataWrites */


/********************************************************************
* FUNCTION nacm_callback
*
* top-level nacm callback function
*
* INPUTS:
*    see agt/agt_cb.h  (agt_cb_pscb_t)
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    nacm_callback (ses_cb_t  *scb,
                   rpc_msg_t  *msg,
                   agt_cbtyp_t cbtyp,
                   op_editop_t  editop,
                   val_value_t  *newval,
                   val_value_t  *curval)
{
    status_t   res;
    boolean    clear_cache;
    val_value_t *useval = NULL;

    (void)scb;
    (void)msg;

    if (newval != NULL) {
        useval = newval;
    } else if (curval != NULL) {
        useval = curval;
    }

    if (LOGDEBUG2) {
        log_debug2("\nServer %s callback: t: %s:%s, op:%s\n", 
                   agt_cbtype_name(cbtyp),
                   (useval) ? val_get_mod_name(useval) : NCX_EL_NONE,
                   (useval) ? useval->name : NCX_EL_NONE,
                   op_editop_name(editop));
    }

    res = NO_ERR;
    clear_cache = FALSE;

    switch (cbtyp) {
    case AGT_CB_VALIDATE:
        break;
    case AGT_CB_APPLY:
        break;
    case AGT_CB_COMMIT:
        switch (editop) {
        case OP_EDITOP_LOAD:
            break;
        case OP_EDITOP_MERGE:
        case OP_EDITOP_REPLACE:
        case OP_EDITOP_CREATE:
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            clear_cache = TRUE;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case AGT_CB_ROLLBACK:
        clear_cache = TRUE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (clear_cache) {
        if (notif_cache != NULL) {
            free_acm_cache(notif_cache);
            notif_cache = NULL;
        }
        agt_ses_invalidate_session_acm_caches();
    }

    return res;

} /* nacm_callback */


/********************************************************************
* FUNCTION nacm_enable_nacm_callback
*
* /nacm/enable-nacm callback function
*
* INPUTS:
*    see agt/agt_cb.h  (agt_cb_pscb_t)
*
* RETURNS:
*    status
*********************************************************************/
static status_t 
    nacm_enable_nacm_callback (ses_cb_t  *scb,
                               rpc_msg_t  *msg,
                               agt_cbtyp_t cbtyp,
                               op_editop_t  editop,
                               val_value_t  *newval,
                               val_value_t  *curval)
{
    status_t   res;

    (void)scb;
    (void)msg;

    if (LOGDEBUG2) {
        val_value_t *useval = NULL;
        if (newval != NULL) {
            useval = newval;
        } else if (curval != NULL) {
            useval = curval;
        }

        log_debug2("\nServer %s callback: t: %s:%s, op:%s\n", 
                   agt_cbtype_name(cbtyp),
                   (useval != NULL) ? 
                   val_get_mod_name(useval) : NCX_EL_NONE,
                   (useval != NULL) ? useval->name : NCX_EL_NONE,
                   op_editop_name(editop));
    }

    res = NO_ERR;

    switch (cbtyp) {
    case AGT_CB_VALIDATE:
        break;
    case AGT_CB_APPLY:
        break;
    case AGT_CB_COMMIT:
        switch (editop) {
        case OP_EDITOP_LOAD:
        case OP_EDITOP_MERGE:
        case OP_EDITOP_REPLACE:
        case OP_EDITOP_CREATE:
            if (newval != NULL && VAL_BOOL(newval)) {
                if (acmode != AGT_ACMOD_ENFORCING) {
                    log_info("\nEnabling NACM Enforcing mode");
                }
                acmode = AGT_ACMOD_ENFORCING;
            } else {
                log_warn("\nWarning: Disabling NACM");
                acmode = AGT_ACMOD_DISABLED;
            }
            break;
        case OP_EDITOP_DELETE:
        case OP_EDITOP_REMOVE:
            log_warn("\nWarning: Disabling NACM");
            acmode = AGT_ACMOD_DISABLED;
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case AGT_CB_ROLLBACK:
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }
    
    return res;

} /* nacm_enable_nacm_callback */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_acm_init
* 
* Initialize the NCX Agent access control module
* 
* INPUTS:
*   none
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
status_t 
    agt_acm_init (void)
{
    status_t  res;
    agt_profile_t  *agt_profile;

    if (agt_acm_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    log_debug2("\nagt: Loading NETCONF Access Control module");

    agt_profile = agt_get_profile();

    nacmmod = NULL;
    notif_cache = NULL;

    /* load in the access control parameters */
    res = ncxmod_load_module(AGT_ACM_MODULE, NULL, &agt_profile->agt_savedevQ,
                             &nacmmod);
    if (res != NO_ERR) {
        return res;
    }

    superuser = NULL;
    acmode = AGT_ACMOD_ENFORCING;
    denied_operations_count = 0;
    denied_data_writes_count = 0;
    agt_acm_init_done = TRUE;
    log_reads = FALSE;
    log_writes = TRUE;

    res = agt_cb_register_callback(AGT_ACM_MODULE, nacm_OID_nacm, NULL,
                                   nacm_callback);
    if (res != NO_ERR) {
        return res;
    }

    res = agt_cb_register_callback(AGT_ACM_MODULE, nacm_OID_nacm_enable_nacm,
                                   NULL, nacm_enable_nacm_callback);
    if (res != NO_ERR) {
        return res;
    }

    return NO_ERR;

}  /* agt_acm_init */


/********************************************************************
* FUNCTION agt_acm_init2
* 
* Phase 2:
*   Initialize the nacm.yang configuration data structures
* 
* INPUTS:
*   none
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
status_t 
    agt_acm_init2 (void)
{
    const agt_profile_t   *profile;
    val_value_t           *nacmval, *childval;
    status_t               res = NO_ERR;
    boolean                added = FALSE;

    if (!agt_acm_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }

    profile = agt_get_profile();
    superuser = profile->agt_superuser;

    if (profile->agt_accesscontrol_enum != AGT_ACMOD_NONE) {
        acmode = profile->agt_accesscontrol_enum;
    }

    log_reads = profile->agt_log_acm_reads;
    log_writes = profile->agt_log_acm_writes;
    // no controls for RPC; notification treated as a read

    nacmval = agt_add_top_node_if_missing(nacmmod, nacm_N_nacm, &added, &res);
    if (res != NO_ERR || nacmval == NULL) {
        return res;
    }
    if (added) {

        /* add following leafs:

           leaf /nacm/enable-nacm
           leaf /nacm/read-default
           leaf /nacm/write-default
           leaf /nacm/exec-default

           These values are saved in NV-store, even if the CLI config
           has disabled NACM.  TBD: what to do about operator thinking
           NACM is enabled but it is really turned off!!    */

        res = val_add_defaults(nacmval, NULL, NULL, FALSE);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* add read-only virtual leafs to the nacm value node
     * create /nacm/denied-operations
     */
    childval = agt_make_virtual_leaf(nacmval->obj, nacm_N_denied_operations,
                                     get_denied_operations, &res);
    if (childval != NULL) {
        val_add_child_sorted(childval, nacmval);
    }

    /* create /nacm/deniedDataWrites */
    if (res == NO_ERR) {
        childval = agt_make_virtual_leaf(nacmval->obj, nacm_N_deniedDataWrites,
                                         get_deniedDataWrites, &res);
        if (childval != NULL) {
            val_add_child_sorted(childval, nacmval);
        }
    }

    notif_cache = new_acm_cache();
    if (notif_cache == NULL) {
        res = ERR_INTERNAL_MEM;
    }

    return res;

}  /* agt_acm_init2 */


/********************************************************************
* FUNCTION agt_acm_cleanup
*
* Cleanup the NCX Agent access control module
* 
*********************************************************************/
void
    agt_acm_cleanup (void)
{
    if (!agt_acm_init_done) {
        return;
    }

    agt_cb_unregister_callbacks(AGT_ACM_MODULE, nacm_OID_nacm);
    agt_cb_unregister_callbacks(AGT_ACM_MODULE, 
                                nacm_OID_nacm_enable_nacm);
    nacmmod = NULL;
    if (notif_cache != NULL) {
        free_acm_cache(notif_cache);
    }
    agt_acm_init_done = FALSE;

}   /* agt_acm_cleanup */


/********************************************************************
* FUNCTION agt_acm_rpc_allowedg
*
* Check if the specified user is allowed to invoke an RPC
* 
* INPUTS:
*   msg == XML header in incoming message in progress
*   user == user name string
*   rpcobj == obj_template_t for the RPC method to check
*
* RETURNS:
*   TRUE if user allowed invoke this RPC; FALSE otherwise
*********************************************************************/
boolean 
    agt_acm_rpc_allowed (xml_msg_hdr_t *msg,
                         const xmlChar *user,
                         const obj_template_t *rpcobj)
{
    val_value_t             *nacmroot, *rulesval;
    agt_acm_usergroups_t    *usergroups;
    agt_acm_cache_t         *cache;
    uint32                   groupcnt;
    boolean                  retval, done;

    assert( msg && "msg is NULL!" );
    assert( user && "user is NULL!" );
    assert( rpcobj && "rpcobj is NULL!" );

    log_debug2("\nagt_acm: check <%s> RPC allowed for user '%s'",
               obj_get_name(rpcobj), user);

    if (acmode == AGT_ACMOD_DISABLED) {
        log_debug2("\nagt_acm: PERMIT (NACM disabled)");
        return TRUE;
    }

    /* super user is allowed to access anything */
    if (is_superuser(user)) {
        log_debug2("\nagt_acm: PERMIT (superuser)");
        return TRUE;
    }

    /* everybody is allowed to close their own session */
    if (obj_get_nsid(rpcobj) == xmlns_nc_id() &&
        !xml_strcmp(obj_get_name(rpcobj), NCX_EL_CLOSE_SESSION)) {
        log_debug2("\nagt_acm: PERMIT (close-session)");
        return TRUE;
    }

    /* check if access granted without any rules */
    if (check_mode(nacm_E_allowedRights_exec, rpcobj)) {
        log_debug2("\nagt_acm: PERMIT (permissive mode)");
        return TRUE;
    }

    cache = msg->acm_cache;

    /* get the NACM root to decide any more */
    if (cache->nacmroot) {
        nacmroot = cache->nacmroot;
    } else {
        nacmroot = get_nacm_root();
        if (!nacmroot) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return FALSE;
        } else {
            cache->nacmroot = nacmroot;
        }
    }

    groupcnt = 0;

    if (cache->usergroups) {
        usergroups = cache->usergroups;
        groupcnt = cache->groupcnt;
    } else {
        usergroups = get_usergroups_entry(nacmroot, user, &groupcnt);
        if (!usergroups) {
            /* out of memory! deny all access! */
            log_debug2("\nagt_acm: DENY (out of memory)");
            return FALSE;
        } else {
            cache->usergroups = usergroups;
            cache->groupcnt = groupcnt;
        }
    }

    /* usergroups malloced at this point */
    retval = FALSE;

    const xmlChar *substr = NULL;
    if (groupcnt == 0) {
        /* just check the default for this RPC operation */
        retval = get_default_rpc_response(cache, nacmroot, rpcobj);
        substr = (const xmlChar *)"exec-default";
    } else {
        {
            /* there is a rules node so check the rpcRules */
            done = FALSE;
            retval = check_rpc_rules(nacmroot, rpcobj, usergroups, &done);
            if (done) {
                substr = (const xmlChar *)"rpc-rule";
            } else {
                /* no RPC rule found; try a module namespace rule */
                retval = check_module_rules(cache, rulesval, rpcobj,
                                            nacm_E_allowedRights_exec,
                                            usergroups, &done);
                if (done) {
                    substr = (const xmlChar *)"module-rule";
                } else {
                    /* no module rule so use the default */
                    retval = get_default_rpc_response(cache, nacmroot, rpcobj);
                    substr = (const xmlChar *)"exec-default";
                }
            }
        }
    }

    if (!retval) {
        denied_operations_count++;
    }

    log_debug2("\nagt_acm: %s (%s)", retval ? "PERMIT" : "DENY",
               substr ? substr : NCX_EL_NONE);

    return retval;

}   /* agt_acm_rpc_allowed */


/********************************************************************
* FUNCTION agt_acm_notif_allowed
*
* Check if the specified user is allowed to receive
* a notification event
* 
* INPUTS:
*   user == user name string
*   notifobj == obj_template_t for the notification event to check
*
* RETURNS:
*   TRUE if user allowed receive this notification event;
*   FALSE otherwise
*********************************************************************/
boolean 
    agt_acm_notif_allowed (const xmlChar *user,
                           const obj_template_t *notifobj)
{
    val_value_t             *nacmroot, *rulesval;
    agt_acm_usergroups_t    *usergroups;
    uint32                   groupcnt;
    boolean                  retval, done;
    logfn_t                  logfn;

    assert( user && "user is NULL!" );
    assert( notifobj && "notifobj is NULL!" );

    logfn = (log_reads) ? log_debug2 : log_noop;

    (*logfn)("\nagt_acm: check <%s> Notification allowed for user '%s'",
             obj_get_name(notifobj), user);
    if (acmode == AGT_ACMOD_DISABLED) {
        (*logfn)("\nagt_acm: PERMIT (NACM disabled)");
        return TRUE;
    }

    /* super user is allowed to access anything */
    if (is_superuser(user)) {
        (*logfn)("\nagt_acm: PERMIT (superuser)");
        return TRUE;
    }

    /* do not block a replayComplete or notificationComplete event */
    if (agt_not_is_replay_event(notifobj)) {
        (*logfn)("\nagt_acm: PERMIT (not a replay event)");
        return TRUE;
    }

    /* check if access granted without any rules */
    if (check_mode(nacm_E_allowedRights_read, notifobj)) {
        (*logfn)("\nagt_acm: PERMIT (permissive mode)");
        return TRUE;
    }

    /* get the notification acm cache */
    if (notif_cache == NULL) {
        notif_cache = new_acm_cache();
        if (notif_cache == NULL) {
            log_error("\nagt_acm: malloc failed");
            return FALSE;
        }
    }

    /* get the NACM root to decide any more */
    if (notif_cache->nacmroot) {
        nacmroot = notif_cache->nacmroot;
    } else {
        nacmroot = get_nacm_root();
        if (!nacmroot) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return FALSE;
        } else {
            notif_cache->nacmroot = nacmroot;
        }
    }

    groupcnt = 0;

    if (notif_cache->usergroups &&
        !xml_strcmp(notif_cache->usergroups->username, user)) {
        usergroups = notif_cache->usergroups;
        groupcnt = notif_cache->groupcnt;
    } else {
        usergroups = get_usergroups_entry(nacmroot, user, &groupcnt);
        if (!usergroups) {
            /* out of memory! deny all access! */
            (*logfn)("\nagt_acm: DENY (out of memory)");
            return FALSE;
        } else {
            if (notif_cache->usergroups) {
                free_usergroups(notif_cache->usergroups);
            }
            notif_cache->usergroups = usergroups;
            notif_cache->groupcnt = groupcnt;
        }
    }

    /* usergroups malloced at this point */
    retval = FALSE;

    const xmlChar *substr = NULL;
    if (groupcnt == 0) {
        /* just check the default for this notification */
        retval = get_default_notif_response(notif_cache, nacmroot, notifobj);
        substr = (const xmlChar *)"read-default";
    } else {
            done = FALSE;
            retval = check_notif_rules(nacmroot, notifobj, usergroups, &done);
            if (done) {
                substr = (const xmlChar *)"notification-rule";
            } else             {
                /* no notification rule found;
                 * try a module namespace rule
                 */
                retval = check_module_rules(notif_cache, rulesval, notifobj,
                                            nacm_E_allowedRights_read,
                                            usergroups, &done);
                if (done) {
                    substr = (const xmlChar *)"module-rule";
                } else {
                    /* no module rule so use the default */
                    retval = get_default_notif_response(notif_cache, nacmroot, 
                                                        notifobj);
                    substr = (const xmlChar *)"read-default";
                }
            }
    }

    (*logfn)("\nagt_acm: %s (%s)", retval ? "PERMIT" : "DENY",
             substr ? substr : NCX_EL_NONE);

    return retval;

}   /* agt_acm_notif_allowed */


/********************************************************************
* FUNCTION agt_acm_val_write_allowed
*
* Check if the specified user is allowed to access a value node
* The val->obj template will be checked against the val->editop
* requested access and the user's configured max-access
* 
* INPUTS:
*   msg == XML header from incoming message in progress
*   newval  == val_value_t in progress to check
*                (may be NULL, if curval set)
*   curval  == val_value_t in progress to check
*                (may be NULL, if newval set)
*   editop == requested CRUD operation
*
* RETURNS:
*   TRUE if user allowed this level of access to the value node
*********************************************************************/
boolean 
    agt_acm_val_write_allowed (xml_msg_hdr_t *msg,
			       const xmlChar *user,
			       const val_value_t *newval,
			       const val_value_t *curval,
                               op_editop_t editop)
{
    boolean  retval;
    const val_value_t *val = (newval) ? newval : curval;
    logfn_t logfn = (log_writes) ? log_debug2 : log_noop;

    (*logfn)("\nagt_acm: check write <%s> allowed for user '%s'",
             val->name, user);

    /* do not check writes during the bootup process
     * cannot compare 'superuser' name in case it is
     * disabled or changed from the default
     */
    if (editop == OP_EDITOP_LOAD) {
        (*logfn)("\nagt_acm: PERMIT (OP_EDITOP_LOAD)");
        return TRUE;
    }

    /* defer check if no edit op requested on this node */
    if (editop == OP_EDITOP_NONE) {
        (*logfn)("\nagt_acm: PERMIT (OP_EDITOP_NONE)");
        return TRUE;
    }

    assert( msg && "msg is NULL!" );
    assert( user && "user is NULL!" );
    assert( val && "val is NULL!" );

    if (msg->acm_cache == NULL) {
        /* this is a rollback operation so just allow it */
        (*logfn)("\nagt_acm: PERMIT (rollback)");
        return TRUE;
    }

    retval = valnode_access_allowed(msg->acm_cache, user, val, newval, curval, editop);

    if (!retval) {
        denied_data_writes_count++;
    }

    return retval;

}   /* agt_acm_val_write_allowed */


/********************************************************************
* FUNCTION agt_acm_val_read_allowed
*
* Check if the specified user is allowed to read a value node
* 
* INPUTS:
*   msg == XML header from incoming message in progress
*   user == user name string
*   val  == val_value_t in progress to check
*
* RETURNS:
*   TRUE if user allowed read access to the value node
*********************************************************************/
boolean 
    agt_acm_val_read_allowed (xml_msg_hdr_t *msg,
                              const xmlChar *user,
                              const val_value_t *val)
{
    assert( msg && "msg is NULL!" );
    assert( msg->acm_cache && "cache is NULL!" );
    assert( user && "user is NULL!" );
    assert( val && "val is NULL!" );

    if (log_reads) {
        log_debug4("\nagt_acm: check read on <%s> allowed for user '%s'",
                   val->name, user);
    }

    return valnode_access_allowed(msg->acm_cache, user, val, NULL, NULL,  OP_EDITOP_NONE);

}   /* agt_acm_val_read_allowed */


/********************************************************************
* FUNCTION agt_acm_init_msg_cache
*
* Malloc and initialize an agt_acm_cache_t struct
* and attach it to the incoming message
*
* INPUTS:
*   scb == session control block to use
*   msg == message to use
*
* OUTPUTS:
*   scb->acm_cache pointer may be set, if it was NULL
*   msg->acm_cache pointer set
*
* RETURNS:
*   status
*********************************************************************/
status_t
    agt_acm_init_msg_cache (ses_cb_t *scb,
                            xml_msg_hdr_t *msg)
{
    assert( scb && "scb is NULL!" );
    assert( msg && "msg is NULL!" );

    if (msg->acm_cache) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        agt_acm_clear_msg_cache(msg);
    }

    msg->acm_cbfn = agt_acm_val_read_allowed;

    if (agt_acm_session_cache_valid(scb)) {
        msg->acm_cache = scb->acm_cache;
    } else {
        if (scb->acm_cache != NULL) {
            free_acm_cache(scb->acm_cache);
        }
        scb->acm_cache = new_acm_cache();
        msg->acm_cache = scb->acm_cache;
    }

    if (msg->acm_cache == NULL) {
        return ERR_INTERNAL_MEM;
    } else {
        return NO_ERR;
    }

} /* agt_acm_init_msg_cache */


/********************************************************************
* FUNCTION agt_acm_clear_msg_cache
*
* Clear an agt_acm_cache_t struct
* attached to the specified message
*
* INPUTS:
*   msg == message to use
*
* OUTPUTS:
*   msg->acm_cache pointer is freed and set to NULL
*
*********************************************************************/
void agt_acm_clear_msg_cache (xml_msg_hdr_t *msg)
{
    assert( msg && "msg is NULL!" );
    msg->acm_cbfn = NULL;
    msg->acm_cache = NULL;

} /* agt_acm_clear_msg_cache */


/********************************************************************
* FUNCTION agt_acm_clear_session_cache
*
* Clear an agt_acm_cache_t struct in a session control block
*
* INPUTS:
*   scb == sesion control block to use
*
* OUTPUTS:
*   scb->acm_cache pointer is freed and set to NULL
*
*********************************************************************/
void agt_acm_clear_session_cache (ses_cb_t *scb)
{
    assert( scb && "scb is NULL!" );
    if (scb->acm_cache != NULL) {
        free_acm_cache(scb->acm_cache);
        scb->acm_cache = NULL;
    }

} /* agt_acm_clear_session_cache */


/********************************************************************
* FUNCTION agt_acm_invalidate_session_cache
*
* Mark an agt_acm_cache_t struct in a session control block
* as invalid so it will be refreshed next use
*
* INPUTS:
*   scb == sesion control block to use
*
*********************************************************************/
void agt_acm_invalidate_session_cache (ses_cb_t *scb)
{
    assert( scb && "scb is NULL!" );
    if (scb->acm_cache != NULL) {
        scb->acm_cache->flags &= ~FL_ACM_CACHE_VALID; 
    }

} /* agt_acm_invalidate_session_cache */


/********************************************************************
* FUNCTION agt_acm_session_cache_valid
*
* Check if a session ACM cache is valid
*
* INPUTS:
*   scb == sesion control block to check
*
* RETURNS:
*   TRUE if cache calid
*   FALSE if cache invalid or NULL
*********************************************************************/
boolean agt_acm_session_cache_valid (const ses_cb_t *scb)
{
    assert( scb && "scb is NULL!" );
    if (scb->acm_cache != NULL) {
        return (scb->acm_cache->flags 
                & FL_ACM_CACHE_VALID) ? TRUE : FALSE;
    } else {
        return FALSE;
    }
} /* agt_acm_session_cache_valid */


/********************************************************************
* FUNCTION agt_acm_session_is_superuser
*
* Check if the specified session is the superuser
*
* INPUTS:
*   scb == session to check
*
* RETURNS:
*   TRUE if session is for the superuser
*   FALSE if session is not for the superuser
*********************************************************************/
boolean
    agt_acm_session_is_superuser (const ses_cb_t *scb)
{
    assert( scb && "scb is NULL!" );
    return is_superuser(scb->username);

}  /* agt_acm_session_is_superuser */


/* END file agt_acm.c */
