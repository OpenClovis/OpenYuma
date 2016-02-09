/* 

    module helloworld
    namespace http://helloworld.com/ns/helloworld

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
static ncx_module_t *helloworld_mod;

status_t
    y_helloworld_init (
        const xmlChar *modname,
        const xmlChar *revision)
{
    agt_profile_t *agt_profile;
    status_t res;

    agt_profile = agt_get_profile();

    res = ncxmod_load_module(
        "helloworld",
        NULL,
        &agt_profile->agt_savedevQ,
        &helloworld_mod);
    if (res != NO_ERR) {
        return res;
    }

    /* put your module initialization code here */
    
    return res;
} /* y_helloworld_init */


status_t
    y_helloworld_init2 (void)
{
    status_t res;
    cfg_template_t* runningcfg;
    obj_template_t* helloworld_obj;
    obj_template_t* message_obj;    
    val_value_t* helloworld_val;
    val_value_t* message_val;

    res = NO_ERR;

    runningcfg = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!runningcfg || !runningcfg->root) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    helloworld_obj = ncx_find_object(
        helloworld_mod,
        "helloworld");
    if (helloworld_obj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    helloworld_val = val_new_value();
    if (helloworld_val == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    val_init_from_template(helloworld_val,
                 helloworld_obj);

    val_add_child(helloworld_val, runningcfg->root);

    message_obj = obj_find_child(helloworld_obj,
                                 "helloworld",
                                 "message");
    if (message_obj == NULL) {
        return SET_ERROR(ERR_NCX_DEF_NOT_FOUND);
    }

    message_val = val_new_value();
    if (message_val == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    val_init_from_template(message_val,message_obj);
    
    res = val_set_simval_obj(message_val,message_val->obj, "Hello World!");

    val_add_child(message_val, helloworld_val);
    
    return res;
}

void
    y_helloworld_cleanup (void)
{
    /* put your cleanup code here */
    
}
