/*
    module ietf-interfaces
    namespace urn:ietf:params:xml:ns:yang:ietf-interfaces
 */
 
#include <xmlstring.h>
#include "procdefs.h"
#include "agt.h"
#include "agt_cb.h"
#include "agt_timer.h"
#include "agt_util.h"
#include "agt_not.h"
#include "agt_rpc.h"
#include "dlq.h"
#include "ncx.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "status.h"
#include "rpc.h"

#include <string.h>
#include <assert.h>

/* module static variables */
static ncx_module_t *ietf_interfaces_mod;
static obj_template_t* interfaces_state_obj;

/*

Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo: 4307500   49739    0    0    0     0          0         0  4307500   49739    0    0    0     0       0          0
 wlan0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
  eth0: 604474858 23680030    0    0    0     0          0    357073 701580994 6935958    0    0    0     0       1          0
 */
 
static status_t
    add_interfaces_state_entry(char* buf, val_value_t* interfaces_state_val)
{
    /*objs*/
    obj_template_t* interface_obj;
    obj_template_t* name_obj;
    obj_template_t* statistics_obj;
    obj_template_t* obj;

    /*vals*/
    val_value_t* interface_val;
    val_value_t* name_val;
    val_value_t* statistics_val;
    val_value_t* val;

    status_t res=NO_ERR;
    boolean done;
    char* name;
    char* str;
    char* endptr;
    unsigned int i;
    uint64_t counter;
    int ret;

    char* counter_names_array[] = {
        "in-octets",
        "in-unicast-pkts",
        "in-errors",
        "in-discards",
        NULL/*"in-fifo"*/,
        NULL/*"in-frames"*/,
        NULL/*in-compressed*/,
        "in-multicast-pkts",
        "out-octets",
        "out-unicast-pkts",
        "out-errors",
        "out-discards",
        NULL/*"out-fifo"*/,
        NULL/*out-collisions*/,
        NULL/*out-carrier*/,
        NULL/*out-compressed*/
    };

    /* get the start of the interface name */
    str = buf;
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

    if (*str != ':') {
        /* expected e.g. eth0: ...*/
        return ERR_NCX_SKIPPED;
    } else {
    	*str=0;
    	str++;
    }

    /* /interfaces-state/interface */
    interface_obj = obj_find_child(interfaces_state_val->obj,
                                   "ietf-interfaces",
                                   "interface");
    assert(interface_obj != NULL);

    interface_val = val_new_value();
    if (interface_val == NULL) {
        return ERR_INTERNAL_MEM;
    }

    val_init_from_template(interface_val, interface_obj);

    val_add_child(interface_val, interfaces_state_val);

    /* /interfaces-state/interface/name */
    name_obj = obj_find_child(interface_obj,
                              "ietf-interfaces",
                              "name");
    assert(name_obj != NULL);


    name_val = val_new_value();
    if (name_val == NULL) {
                return ERR_INTERNAL_MEM;
    }       
    
    val_init_from_template(name_val, name_obj);

    res = val_set_simval_obj(name_val, name_obj, name);

    val_add_child(name_val, interface_val);

    /* /interfaces-state/interface/statistics */
    statistics_obj = obj_find_child(interface_obj,
                                   "ietf-interfaces",
                                   "statistics");
    assert(statistics_obj != NULL);
    statistics_val = val_new_value();
    if (statistics_val == NULL) {
        return ERR_INTERNAL_MEM;
    }

    val_init_from_template(statistics_val, statistics_obj);

    val_add_child(statistics_val, interface_val);

    done = FALSE;
    for(i=0;i<(sizeof(counter_names_array)/sizeof(char*));i++) {
        endptr = NULL;
        counter = strtoull((const char *)str, &endptr, 10);
        if (counter == 0 && str == endptr) {
            /* number conversion failed */
            log_error("Error: /proc/net/dev number conversion failed.");
            return ERR_NCX_OPERATION_FAILED;
        }

        if(counter_names_array[i]!=NULL) {
            obj = obj_find_child(statistics_obj,
                                 "ietf-interfaces",
                                 counter_names_array[i]);
    	    assert(obj != NULL);

            val = val_new_value();
            if (val == NULL) {
                return ERR_INTERNAL_MEM;
            }
            val_init_from_template(val, obj);
            VAL_UINT64(val) = counter;
            val_add_child(val, statistics_val);
        }

        str = (xmlChar *)endptr;
        if (*str == '\0' || *str == '\n') {
            break;
        }
    }
    return res;
}

/* Registered callback functions: get_interfaces_state */

static status_t
    get_interfaces_state(ses_cb_t *scb,
                         getcb_mode_t cbmode,
                         val_value_t *vir_val,
                         val_value_t *dst_val)
{
    FILE* f;
    status_t res;
    res = NO_ERR;
    boolean done;
    char* buf;
    unsigned int line;

    /* open /proc/net/dev for reading */
    f = fopen("/proc/net/dev", "r");
    if (f == NULL) {
        return ERR_INTERNAL_VAL;
    }

    /* get a file read line buffer */
    buf = (char*)malloc(NCX_MAX_LINELEN);
    if (buf == NULL) {
        fclose(f);
        return ERR_INTERNAL_MEM;
    }

    done = FALSE;
    line = 0;

    while (!done) {
        if (NULL == fgets((char *)buf, NCX_MAX_LINELEN, f)) {
            done = TRUE;
            continue;
        } else {
            line++;
        }

        if (line < 3) {
            /* skip the first 2 lines */
            continue;
        }

        res = add_interfaces_state_entry(buf, dst_val);
        if (res != NO_ERR) {
             done = TRUE;
        }
    }

    fclose(f);
    free(buf);

    return res;
}

/* The 3 mandatory callback functions: y_ietf_interfaces_init, y_ietf_interfaces_init2, y_ietf_interfaces_cleanup */

status_t
    y_ietf_interfaces_init (
        const xmlChar *modname,
        const xmlChar *revision)
{
    agt_profile_t *agt_profile;
    status_t res;

    agt_profile = agt_get_profile();

    res = ncxmod_load_module(
        "ietf-interfaces",
        NULL,
        &agt_profile->agt_savedevQ,
        &ietf_interfaces_mod);
    if (res != NO_ERR) {
        return res;
    }

    interfaces_state_obj = ncx_find_object(
        ietf_interfaces_mod,
        "interfaces-state");
    if (interfaces_state_obj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }
    
    return res;
}

status_t y_ietf_interfaces_init2(void)
{
    status_t res;
    cfg_template_t* runningcfg;
    val_value_t* interfaces_state_val;

    res = NO_ERR;

    runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    interfaces_state_val = val_find_child(runningcfg->root,
                                          "ietf-interfaces",
                                          "interfaces-state");
    /* Can not coexist with other implementation
     * of ietf-interfaces.
     */
    if(interfaces_state_val!=NULL) {
        log_error("\nError: /interfaces-state already present!");
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    interfaces_state_val = val_new_value();
    if (interfaces_state_val == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    val_init_from_template(interfaces_state_val,
                           interfaces_state_obj);

    val_init_virtual(interfaces_state_val,
                     get_interfaces_state,
                     interfaces_state_val->obj);

    val_add_child(interfaces_state_val, runningcfg->root);

    return res;
}

void y_ietf_interfaces_cleanup (void)
{
}
