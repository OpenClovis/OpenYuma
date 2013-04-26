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
/*  FILE: agt_ncx.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
04feb06      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include  "procdefs.h"
#include "agt.h"
#include "agt_cap.h"
#include "agt_cb.h"
#include "agt_cfg.h"
#include "agt_cli.h"
#include "agt_ncx.h"
#include "agt_rpc.h"
#include "agt_rpcerr.h"
#include "agt_ses.h"
#include "agt_sys.h"
#include "agt_state.h"
#include "agt_time_filter.h"
#include "agt_util.h"
#include "agt_val.h"
#include "cap.h"
#include "cfg.h"
#include "ncxmod.h"
#include "obj.h"
#include "op.h"
#include "rpc.h"
#include "rpc_err.h"
#include "ses.h"
#include  "status.h"
#include  "tstamp.h"
#include  "val.h"
#include  "xml_wr.h"
#include  "yangconst.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define IF_MODIFIED_SINCE (const xmlChar *)"if-modified-since"


/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/


/* candidate commit control block struct */
typedef struct commit_cb_t_ {
    xmlChar     *cc_backup_source;   /* malloced */
    xmlChar     *cc_persist_id;      /* malloced */
    time_t       cc_start_time;
    uint32       cc_cancel_timeout;
    ses_id_t     cc_ses_id;
    boolean      cc_active;
} commit_cb_t;

/* copy-config parm-block passed from validate to invoke callback */
typedef struct copy_parms_t_ {
    cfg_template_t  *srccfg;
    cfg_template_t  *destcfg;
    val_value_t     *srcval;
    val_value_t     *srcurlval;       /* malloced */
    xmlChar         *srcfile;         /* malloced */
    xmlChar         *destfile;        /* malloced */
    xmlChar         *desturlspec;     /* malloced */
} copy_parms_t;


/* edit-config parm-block passed from validate to invoke callback */
typedef struct edit_parms_t_ {
    cfg_template_t  *target;
    val_value_t     *srcval;
    val_value_t     *urlval;          /* malloced */
    op_editop_t      defop;
    op_testop_t      testop;
} edit_parms_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static boolean          agt_ncx_init_done = FALSE;

static commit_cb_t      commit_cb;


/********************************************************************
* FUNCTION new_copyparms
*
* malloc a copy parms struct
*
* RETURNS:
*    malloced struct or NULL if no memory error
*********************************************************************/
static copy_parms_t *
    new_copyparms (void)
{
    copy_parms_t   *copyparms;

    copyparms = m__getObj(copy_parms_t);
    if (copyparms == NULL) {
        return NULL;
    }

    memset(copyparms, 0x0, sizeof(copy_parms_t));

    return copyparms;

}  /* new_copyparms */


/********************************************************************
* FUNCTION free_copyparms
*
* clean and free a copy parms struct
*
* INPUTS:
*    copyparms == struct to free
*********************************************************************/
static void
    free_copyparms ( copy_parms_t *copyparms)
{
    if (copyparms->srcurlval != NULL) {
        val_free_value(copyparms->srcurlval);
    }
    if (copyparms->srcfile != NULL) {
        m__free(copyparms->srcfile);
    }
    if (copyparms->destfile != NULL) {
        m__free(copyparms->destfile);
    }
    if (copyparms->desturlspec != NULL) {
        m__free(copyparms->desturlspec);
    }
    m__free(copyparms);

}  /* free_copyparms */


/********************************************************************
* FUNCTION cfg_save_inline
*
* Save the specified cfg to the its startup source, which should
* be stored in the cfg struct
*
* INPUTS:
*    target_url == filespec where to save newroot 
*    newroot == value root to save
*    forstartup == TRUE if this is the startup config being saved
*                  FALSE if this is a URL file being saved
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    cfg_save_inline (const xmlChar *target_url,
                     val_value_t *newroot,
                     boolean forstartup)
{
    agt_profile_t     *profile;
    cfg_template_t    *startup;
    val_value_t       *copystartup;
    status_t           res;
    xml_attrs_t        attrs;

    startup = NULL;
    copystartup = NULL;
    res = NO_ERR;
    profile = agt_get_profile();

    if (forstartup) {
        startup = cfg_get_config_id(NCX_CFGID_STARTUP);
        if (startup != NULL) {
            copystartup = val_clone_config_data(newroot, &res);
            if (copystartup == NULL) {
                return res;
            }
        }
    }

    if (res == NO_ERR) {
        /* write the new startup config */
        xml_init_attrs(&attrs);

        /* output to the specified file or STDOUT */
        res = xml_wr_check_file(target_url,
                                newroot,
                                &attrs,
                                XMLMODE,
                                WITHHDR,
                                TRUE,
                                0,
                                profile->agt_indent,
                                agt_check_save);

        xml_clean_attrs(&attrs);

        if (res == NO_ERR && forstartup && copystartup != NULL) {
            /* toss the old startup and save the new one */
            if (startup->root) {
                val_free_value(startup->root);
            }
            startup->root = copystartup;
            copystartup = NULL;
        }
    }

    if (copystartup) {
        val_free_value(copystartup);
    }

    return res;

} /* cfg_save_inline */


/********************************************************************
* FUNCTION get_validate
*
* get : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_validate (ses_cb_t *scb,
                  rpc_msg_t *msg,
                  xml_node_t *methnode)
{
    cfg_template_t     *source;
    val_value_t        *testval;
    status_t            res, res2;
    boolean             empty_callback = FALSE;

    /* check if the <running> config is ready to read */
    source = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (!source) {
        res = ERR_NCX_OPERATION_FAILED;
    } else {
        res = cfg_ok_to_read(source);
    }
    if (res != NO_ERR) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
        return res;
    }

    /* check if the optional filter parameter is ok */
    res = agt_validate_filter(scb, msg);

    /* check the with-defaults parameter */
    res2 = agt_set_with_defaults(scb, msg, methnode);

    if (res != NO_ERR) {
        return res;   /* error already recorded */
    }

    if (res2 != NO_ERR) {
        return res2;   /* error already recorded */
    }

    testval = val_find_child(msg->rpc_input,
                             y_yuma_time_filter_M_yuma_time_filter,
                             IF_MODIFIED_SINCE);
    if (testval != NULL && testval->res == NO_ERR) {
        boolean isneg = FALSE;
        xmlChar *utcstr;
        int ret;
        utcstr = tstamp_convert_to_utctime(VAL_STR(testval), &isneg, &res);
        if (res != NO_ERR || isneg) {
            if (utcstr) {
                m__free(utcstr);
            }
            if (isneg) {
                res = ERR_NCX_INVALID_VALUE;
            }

            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_VAL, 
                             testval, 
                             NCX_NT_VAL, 
                             testval);
            return res;
        }
        ret = xml_strcmp(source->last_ch_time, utcstr);
        if (ret <= 0) {
            empty_callback = TRUE;
        }
        m__free(utcstr);
    }

    /* cache the 2 parameters and the data output callback function 
     * There is no invoke function -- it is handled automatically
     * by the agt_rpc module
     */
    msg->rpc_user1 = source;
    msg->rpc_data_type = RPC_DATA_STD;
    if (empty_callback) {
        msg->rpc_datacb = agt_output_empty;
    } else {
        msg->rpc_datacb = agt_output_filter;
    }

    return NO_ERR;

} /* get_validate */


/********************************************************************
* FUNCTION get_config_validate
*
* get-config : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    get_config_validate (ses_cb_t *scb,
                         rpc_msg_t *msg,
                         xml_node_t *methnode)
{
    cfg_template_t *source;
    val_value_t    *testval;
    boolean         empty_callback = FALSE;
    status_t        res, res2;

    /* check if the source config database exists */
    res = agt_get_cfg_from_parm(NCX_EL_SOURCE, 
                                msg, 
                                methnode, 
                                &source);
    if (res != NO_ERR) {
        return res;  /* error already recorded */
    } 

    /* check if this config can be read right now */
    res = cfg_ok_to_read(source);
    if (res != NO_ERR) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
        return res;
    }

    /* check if the optional filter parameter is ok */
    res = agt_validate_filter(scb, msg);

    /* check the with-defaults parameter */
    res2 = agt_set_with_defaults(scb, msg, methnode);

    if (res != NO_ERR) {
        return res;   /* error already recorded */
    }

    if (res2 != NO_ERR) {
        return res2;   /* error already recorded */
    }

    testval = val_find_child(msg->rpc_input,
                             y_yuma_time_filter_M_yuma_time_filter,
                             IF_MODIFIED_SINCE);
    if (testval != NULL && testval->res == NO_ERR) {
        boolean isneg = FALSE;
        xmlChar *utcstr;
        int ret;
        utcstr = tstamp_convert_to_utctime(VAL_STR(testval), &isneg, &res);
        if (res != NO_ERR || isneg) {
            if (utcstr) {
                m__free(utcstr);
            }
            if (isneg) {
                res = ERR_NCX_INVALID_VALUE;
            }

            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_VAL, 
                             testval, 
                             NCX_NT_VAL, 
                             testval);
            return res;
        }
        ret = xml_strcmp(source->last_ch_time, utcstr);
        if (ret <= 0) {
            empty_callback = TRUE;
        }
        m__free(utcstr);
    }

    /* cache the 2 parameters and the data output callback function 
     * There is no invoke function -- it is handled automatically
     * by the agt_rpc module
     */
    msg->rpc_user1 = source;
    msg->rpc_data_type = RPC_DATA_STD;
    if (empty_callback) {
        msg->rpc_datacb = agt_output_empty;
    } else {
        msg->rpc_datacb = agt_output_filter;
    }

    return NO_ERR;

} /* get_config_validate */


/********************************************************************
* FUNCTION edit_config_validate
*
* edit-config : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    edit_config_validate (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    cfg_template_t       *target = NULL;
    val_value_t          *val = NULL, *urlval = NULL;
    const agt_profile_t  *agt_profile = agt_get_profile();
    const xmlChar        *urlstr = NULL;
    xmlChar              *urlspec = NULL;
    edit_parms_t         *editparms = NULL;
    op_editop_t           defop = OP_EDITOP_MERGE;
    op_errop_t            errop = OP_ERROP_STOP;
    op_testop_t           testop = OP_TESTOP_NONE;
    boolean               rootcheck = FALSE;

    /* check if the target config database exists */
    status_t res = agt_get_cfg_from_parm(NCX_EL_TARGET, msg, methnode, &target);
    if (res != NO_ERR) {
        return res;  /* error already recorded */
    } 

    /* check if the server supports this config as a target */
    if (target->cfg_id == NCX_CFGID_STARTUP ||
        (target->cfg_id == NCX_CFGID_RUNNING &&
         !(agt_profile->agt_targ == NCX_AGT_TARG_RUNNING ||
           agt_profile->agt_targ == NCX_AGT_TARG_CAND_RUNNING))) {

        /* trying to edit running but this is not allowed */
        res = ERR_NCX_CONFIG_NOT_TARGET;
        val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_TARGET);
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_STRING, 
                         target->name, 
                         (val != NULL) ? NCX_NT_VAL : NCX_NT_NONE, 
                         val);
        return res;
    }

    /* get the default-operation parameter */
    val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_DEFAULT_OPERATION);
    if (!val || val->res != NO_ERR) {
        /* set to the default if any error */
        defop = OP_EDITOP_MERGE;
    } else {
        defop = op_defop_id(VAL_ENUM_NAME(val));
    }

    /* get the error-option parameter */
    val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_ERROR_OPTION);
    if (!val || val->res != NO_ERR) {
        /* set to the default if any error */
        errop = OP_ERROP_STOP;
    } else {
        errop = op_errop_id(VAL_ENUM_NAME(val));
    }

    /* the internal processing needs to know if rollback is
     * requested to optimize the undo-prep and cleanup code path
     * !!! This field is obsolete; rollback always done now !!!
     */
    msg->rpc_err_option = errop;

    /* Get the test-option parameter:
     *
     * This implementation always runs the validation tests in 
     * the same order, even if the value 'set' is used.
     * The validation stage is never bypassed, even if 'set'
     * is used instead of 'test-then-set'.
     *
     * Get the value to check for the test-only extension
     */
    val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_TEST_OPTION);
    if (val == NULL) {
        /* set to the default if not present */
        if (agt_profile->agt_usevalidate) {
            testop = OP_TESTOP_TESTTHENSET;
        } else {
            testop = OP_TESTOP_SET;
        }
    } else if (val->res != NO_ERR) {
        res = val->res;
    } else if (!agt_profile->agt_usevalidate) {
        res = ERR_NCX_UNKNOWN_ELEMENT;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_VAL, 
                         val, 
                         NCX_NT_VAL, 
                         val);
        return res;
    } else {
        testop = op_testop_enum(VAL_ENUM_NAME(val));
    }

    /* try to get the config parameter */
    val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_CONFIG);
    if (val == NULL) {
        val_value_t *errval = NULL;

        /* try to get the config parameter */
        res = agt_get_url_from_parm(NCX_EL_URL, msg, methnode, 
                                    &urlstr, &errval);
        if (res == NO_ERR) {
            /* get the filespec out of the URL */
            urlspec = agt_get_filespec_from_url(urlstr, &res);

            if (urlspec != NULL && res == NO_ERR) {
                /* get the external file loaded into a value struct
                 * for the <nc:config> object node
                 */           
                val = agt_rpc_get_config_file(urlspec,
                                              target,
                                              SES_MY_SID(scb), 
                                              RPC_ERR_QUEUE(msg), 
                                              &res);
                urlval = val;
            } else {
                /* convert file name to full filespec failed */
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res,
                                 methnode, 
                                 NCX_NT_VAL, 
                                 errval, 
                                 NCX_NT_VAL, 
                                 errval);
            }
        } /* else error done */
    }

    /* if this is an edit-config on running or a test-then-set edit 
     * on candidate then force an agt_val_root_check after the
     * apply phase; rollback will be done if any test fails     */
    if (res==NO_ERR &&
        ((target->cfg_id == NCX_CFGID_RUNNING) ||
         (target->cfg_id == NCX_CFGID_CANDIDATE &&
          testop == OP_TESTOP_TESTTHENSET))) {
        /* !!! defer this test; just apply for real and
         * !!! rollback the edits if root check fails
         * !!! force a rootcheck in the apply phase   */
        rootcheck = TRUE;
    }

    if (res == NO_ERR) {
        /* allocate a new transaction control block */
        msg->rpc_txcb = 
            agt_cfg_new_transaction(target->cfg_id, AGT_CFG_EDIT_TYPE_PARTIAL,
                                    rootcheck, FALSE, &res);
        if (msg->rpc_txcb == NULL || res != NO_ERR) {
            if (res == NO_ERR) {
                res = ERR_NCX_OPERATION_FAILED;
            }
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE, 
                             NULL, 
                             NCX_NT_NONE, 
                             NULL);
        }
    }

    if (res == NO_ERR && val != NULL && val->res == NO_ERR) {
        /* first set the config node to canonical order */
        val_set_canonical_order(val);

        /* validate the <config> element (wrt/ embedded operation
         * attributes) against the existing data model.
         * <rpc-error> records will be added as needed 
         */
        res = agt_val_validate_write(scb, msg, target, val, defop);

    } else if (res == NO_ERR) {
        if (!val) {
            /* this is reported in agt_val_parse phase */
            res = ERR_NCX_DATA_MISSING;
        } else {
            res = val->res;
        }
    }

    /* save the edit options in 'user1' */
    if (res == NO_ERR) {
        editparms = m__getObj(edit_parms_t);
        if (editparms == NULL) {
            res = ERR_INTERNAL_MEM;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE,
                             NULL,
                             NCX_NT_NONE, 
                             NULL);
        } else {
            editparms->target = target;
            editparms->srcval = val;
            editparms->urlval = urlval;  /* transfer memory here */
            editparms->defop = defop;
            editparms->testop = testop;
            msg->rpc_user1 = (void *)editparms;            
        }
    } 

    if (res != NO_ERR && urlval) {
        val_free_value(urlval);
    }

    if (urlspec != NULL) {
        m__free(urlspec);
    }

    return res;

} /* edit_config_validate */


/********************************************************************
* FUNCTION edit_config_invoke
*
* edit-config : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    edit_config_invoke (ses_cb_t *scb,
                        rpc_msg_t *msg,
                        xml_node_t *methnode)
{
    cfg_template_t *target;
    val_value_t    *srcval, *urlval;
    edit_parms_t   *editparms;
    agt_profile_t  *profile;
    op_editop_t     defop;
    op_testop_t     testop;
    status_t        res;

    (void)methnode;

    /* get the cached options */
    editparms = (edit_parms_t *)msg->rpc_user1;
    defop = editparms->defop;
    testop = editparms->testop;
    target = editparms->target;
    urlval = editparms->urlval;
    srcval = editparms->srcval;
    profile = agt_get_profile();
    
    /* quick exit if this is a test-only request */
    if (testop == OP_TESTOP_TESTONLY) {
        return NO_ERR;
    }

    /* apply the <config> into the target config */
    res = agt_val_apply_write(scb, msg, target, srcval, defop);

    /* check if the NV-storage needs to be updated after each
     * successful edit-config 
     */
    if (res == NO_ERR &&
        profile->agt_targ == NCX_AGT_TARG_RUNNING &&
        profile->agt_has_startup == FALSE) {

        res = agt_ncx_cfg_save(target, FALSE);
        if (res != NO_ERR) {
            log_error("\nError: Save <running> to NV-storage failed (%s)",
                      get_error_string(res));
        }
    }

    /* cleanup the urlval and editparms
     * allocated in validate callback 
     */
    if (urlval != NULL) {
        val_free_value(urlval);
    }
    m__free(editparms);

    return res;

} /* edit_config_invoke */


/********************************************************************
* FUNCTION validate_copy_source
*
* Muster and validate the parameters for the source of the copy operation.
*
* INPUTS:
*    scb - the session control block
*    msg - the message that triggered the copy_config_validate operation
*    methnode - the xml method node
*
* OUTPUTS:
*    srccfg - the source configuration
*    srcurl - the url of the source
*    srcfile - the source file 
*    
* RETURNS:
*    status
*********************************************************************/
static status_t
    validate_copy_source( ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode,
                          cfg_template_t   **srccfg,
                          xmlChar          **srcfile,
                          xmlChar          **srcurlspec,
                          val_value_t      **srcval )
{
    status_t res = NO_ERR;
    const void* errPtr = 0;
    ncx_node_t errNodeType = NCX_NT_NONE;
    const cap_list_t   *mycaps = agt_cap_get_caps();
    val_value_t *errval2 = NULL;

    if ( !mycaps ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    res = agt_get_cfg_from_parm( NCX_EL_SOURCE, msg, methnode, srccfg );
    if ( NO_ERR == res ) {
        if ( *srccfg ) {
        switch ( (*srccfg)->cfg_id ) {
            case NCX_CFGID_CANDIDATE:
                if ( !cap_std_set( mycaps, CAP_STDID_CANDIDATE ) ) {
                     res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                }
                break;
            case NCX_CFGID_RUNNING:
                break;
            case NCX_CFGID_STARTUP:
                if ( !cap_std_set( mycaps, CAP_STDID_STARTUP ) ) {
                     res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                }
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        errPtr = srccfg;
        errNodeType = NCX_NT_CFG;
    } else if ( ERR_NCX_FOUND_INLINE == res ) {
        res = agt_get_inline_cfg_from_parm( NCX_EL_SOURCE, msg, 
                                               methnode, srcval );
    } else if ( ERR_NCX_FOUND_URL == res  ) {
        const xmlChar *srcurl = NULL;
        
        res = agt_get_url_from_parm( NCX_EL_SOURCE, msg, methnode, 
                                     &srcurl, &errval2 );
        if ( NO_ERR == res ) {
            if ( srcurl )
            {
                /* get the pointer to the filespec part */ 
                *srcurlspec = agt_get_filespec_from_url( srcurl, &res );

                /* check the URL parameter to see if it is valid */
                if ( *srcurlspec ) {
                    if ( NO_ERR == res) {
                        status_t ignoreRes;
                        /* allowed to be a not-found error, which is ignored */
                        *srcfile = ncxmod_find_data_file( *srcurlspec, FALSE, 
                                                          &ignoreRes );
                    }
                } else {
                    res = SET_ERROR(ERR_INTERNAL_PTR);
                }

                errPtr = srcurl;
                errNodeType = NCX_NT_STRING; 
            }
        }
    }

    if (res != NO_ERR) {
        /* set the errval to the destval for now, just in case
         * look for more errors, even if src already invalid
         */
        val_value_t* errval;
        if (errval2) {
            errval = errval2;
        } else {
            errval = val_find_child( msg->rpc_input, NC_MODULE, 
                                     NCX_EL_SOURCE );
        }

        /* the sourcs is not found */
        agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res, 
                          methnode, errNodeType, errPtr, NCX_NT_VAL, errval);
    }

    return res;
}

/********************************************************************
* FUNCTION validate_copy_dest
*
* Muster and validate the parameters for the dest of the copy operation.
*
* INPUTS:
*    scb - the session control block
*    msg - the message that triggered the copy_config_validate operation
*    methnode - the xml method node
*
* OUTPUTS:
*    destcfg - the dest configuration
*    desturl - the url of the dest
*    destfile - the dest file 
*    
* RETURNS:
*    status
*********************************************************************/
static status_t
    validate_copy_dest( ses_cb_t *scb,
                        rpc_msg_t *msg,
                        xml_node_t *methnode,
                        cfg_template_t   **destcfg,
                        xmlChar          **destfile,
                        xmlChar          **desturlspec )
{
    status_t res = NO_ERR;
    const void* errPtr = 0;
    ncx_node_t errNodeType = NCX_NT_NONE;
    const cap_list_t   *mycaps = agt_cap_get_caps();
    val_value_t *errval2 = NULL;

    if ( !mycaps ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* get the config to copy to */
    res = agt_get_cfg_from_parm(NCX_EL_TARGET, msg, methnode, destcfg);
    if (res == NO_ERR ) {
        if ( *destcfg ) {
        switch ( (*destcfg)->cfg_id ) {
            case NCX_CFGID_CANDIDATE:
                if ( !cap_std_set(mycaps, CAP_STDID_CANDIDATE) ) {
                    res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                }
                break;
            case NCX_CFGID_RUNNING:
                /* a server may choose not to support this so it is not
                 * supported use edit-config instead */
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                break;
            case NCX_CFGID_STARTUP:
                if (!cap_std_set(mycaps, CAP_STDID_STARTUP)) {
                    res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                }
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
            errPtr = destcfg;
            errNodeType = NCX_NT_CFG;
        }
    } else if ( ERR_NCX_FOUND_INLINE == res ) {
        /* got some inline <config> as the <target> parameter
         * error not recorded yet; only OK for <source>
         */
        res =  ERR_NCX_OPERATION_NOT_SUPPORTED;
    } else if ( ERR_NCX_FOUND_URL == res ) {
        const xmlChar *desturl = NULL;
        res = agt_get_url_from_parm( NCX_EL_TARGET, msg, methnode, 
                                     &desturl, &errval2 );
        if ( NO_ERR == res ) {
            if ( desturl ) {
                /* check the destination URL */
                *desturlspec = agt_get_filespec_from_url( desturl, &res );

                /* check the URL parameter to see if it is valid */
                if ( *desturlspec ) {
                    if ( NO_ERR == res ) {
                        status_t ignoreRes;
                        /* allowed to be a not-found error, which is ignored */
                        *destfile = ncxmod_find_data_file( *desturlspec, FALSE, 
                                                           &ignoreRes);
                    }
                } else {
                    res = ERR_INTERNAL_PTR;
                }

                errPtr = desturl;
                errNodeType = NCX_NT_STRING;
            }
        }
    }

    if (res != NO_ERR) {
        /* set the errval to the destval for now, just in case
         * look for more errors, even if src already invalid
         */
        val_value_t* errval;
        if (errval2) {
            errval = errval2;
        } else {
            errval = val_find_child( msg->rpc_input, NC_MODULE, 
                                     NCX_EL_TARGET );
        }

        /* cannot use this configuration datastore */
        agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res, 
                          methnode, errNodeType, errPtr, NCX_NT_VAL, errval);
    }
    return res;
}


/********************************************************************
* FUNCTION copy_config_validate
*
* copy-config : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    copy_config_validate (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    status_t            res;            /// Function return value

    /// Source configuration parameters
    cfg_template_t     *srccfg = NULL;
    xmlChar            *srcfile = NULL;
    xmlChar            *srcurlspec = NULL;
    val_value_t        *srcval = NULL;
    val_value_t        *srcurlval = NULL;

    /// Destination configuration parameters
    cfg_template_t     *destcfg = NULL;
    xmlChar            *destfile = NULL;
    xmlChar            *desturlspec = NULL;

    val_value_t        *errval;
    copy_parms_t       *copyparms;

    /* check the source config, URL, or inline (copy source) */
   res = validate_copy_source( scb, msg, methnode, &srccfg, 
                               &srcfile, &srcurlspec, &srcval );

    /* NOTE: The original version of the function simply ignored the
     * value of res set by the switch statement that validated
     * srccfg->cfg_id and continued on regardless. It is assumed that
     * this was not the intended behaviour, ergo the refactored
     * version now checks the result. */
    if ( NO_ERR == res ) {
        res = validate_copy_dest( scb, msg, methnode, &destcfg, 
                                  &destfile, &desturlspec );
    }

    errval = val_find_child( msg->rpc_input, NC_MODULE, NCX_EL_TARGET );

    if ( NO_ERR == res ) {
        /* check a corner-case: URL to database */
        if ( srcfile  && destcfg ) {
            /* get the URL contents as a value struct */
            srcurlval = agt_rpc_get_config_file( srcfile, destcfg, 
                                                 SES_MY_SID(scb),
                                                 RPC_ERR_QUEUE(msg), &res );
        }
        /* check a corner-case: URL to URL
         * this is optional-to-support and is not allowed by this server */
        /* NOTE: Invariant: it is not pssible to have srcfile and  srcurl */
        else if ( srcurlspec  && desturlspec ) {
            const xmlChar *desturl = NULL;
            res = agt_get_url_from_parm( NCX_EL_TARGET, msg, 
                                         methnode, &desturl, &errval );

            res = ERR_NCX_OPERATION_NOT_SUPPORTED;

            agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                              methnode, NCX_NT_STRING, (const void *)desturl, 
                              NCX_NT_VAL, errval);
        }
    }

    /* check the with-defaults parameter, but only if the
     * target is an <url>; otherwise basic mode will be used
     */
    if ( res == NO_ERR && desturlspec  ) {
        res = agt_set_with_defaults(scb, msg, methnode);
    }

    /* check source config == dest config */
    if ( res == NO_ERR && srccfg && srccfg == destcfg) {
        /* invalid operation */
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;

        agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                          methnode, NCX_NT_CFG, (const void *)destcfg, 
                          NCX_NT_VAL, errval );
    }

    /* get the config state; check if database already locked */
    if ( NO_ERR == res && destcfg ) {
        res = cfg_ok_to_write(destcfg, SES_MY_SID(scb));
        /* make sure no transaction in progress (should not happen) */
        if (res == NO_ERR && agt_cfg_txid_in_progress(destcfg->cfg_id)) {
            res = ERR_NCX_NO_ACCESS_STATE;
        }
        if (res != NO_ERR) {
            agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                              methnode, NCX_NT_CFG, (const void *)destcfg, 
                              NCX_NT_NONE, NULL );
        }
    }

    if ( NO_ERR == res && srcval && destcfg && 
         destcfg->cfg_id != NCX_CFGID_STARTUP) {

        /* allocate a transaction control block */
        msg->rpc_txcb = 
            agt_cfg_new_transaction(destcfg->cfg_id, AGT_CFG_EDIT_TYPE_FULL,
                                    FALSE, FALSE, &res);
        if (msg->rpc_txcb == NULL || res != NO_ERR) {
            if (res == NO_ERR) {
                res = ERR_NCX_OPERATION_FAILED;
            }
            agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                              methnode, NCX_NT_NONE, NULL, NCX_NT_NONE, NULL );
        }
        if (res == NO_ERR) {
            /* validate the <config> element (wrt/ embedded operation
             * attributes) against the existing data model.
             * <rpc-error> records will be added as needed 
             */
            msg->rpc_top_editop = OP_EDITOP_REPLACE;
            res = agt_val_validate_write( scb, msg, destcfg, srcval, 
                                          OP_EDITOP_REPLACE );

            /* since the running config is not supported as a target,
             * there is no need to run an agt_val_root_check on
             * the result of the full-root replace operation  */
        }
    }

    /* setup the edit parms to save for the invoke phase */
    if ( NO_ERR == res ) {
        copyparms = new_copyparms();
        if ( copyparms ) {
            copyparms->srccfg = srccfg;
            copyparms->destcfg = destcfg;
            copyparms->srcval = srcval;
            copyparms->srcurlval = srcurlval;
            copyparms->srcfile = srcfile;
            copyparms->destfile = destfile;
            copyparms->desturlspec = desturlspec;
            msg->rpc_user1 = copyparms;
        } else {
            res = ERR_INTERNAL_MEM;
            agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                              methnode, NCX_NT_NONE, NULL, NCX_NT_NONE, NULL );
        }
    }

    if ( NO_ERR != res ) {
        if (srcurlval ) {
            val_free_value(srcurlval);
        }
        if (desturlspec ) {
            m__free(desturlspec);
        }
        if (srcfile ) {
            m__free(srcfile);
        }
        if (destfile ) {
            m__free(destfile);
        }
    }

    if ( srcurlspec ) {
            m__free( srcurlspec );
    }

    return res;
} /* copy_config_validate */


/********************************************************************
* FUNCTION copy_config_invoke
*
* copy-config : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    copy_config_invoke (ses_cb_t *scb,
                        rpc_msg_t *msg,
                        xml_node_t *methnode)
{
    copy_parms_t   *copyparms = (copy_parms_t *)msg->rpc_user1;
    val_value_t    *sourceval = NULL;
    status_t        res = NO_ERR;

    if (copyparms->srcval != NULL ||
        copyparms->srcurlval != NULL) {

        /* set the sourceval; only 1 of these 2 parms
         * should be set by the validate function
         */
        if (copyparms->srcval != NULL) {
            sourceval = copyparms->srcval;
        } else {
            sourceval = copyparms->srcurlval;
        }

        /* copy from inline data or URL data to a database or URL */
        if (copyparms->destcfg != NULL) {
            switch (copyparms->destcfg->cfg_id) {
            case NCX_CFGID_STARTUP:
                /* figure out which URL to use for startup */
                res = NO_ERR;
                copyparms->destfile = agt_get_startup_filespec(&res);
                if (copyparms->destfile != NULL && res == NO_ERR) {
                    res = cfg_save_inline(copyparms->destfile, sourceval, TRUE);
                }
                break;
            case NCX_CFGID_CANDIDATE:
                /* do not just blindly apply the replace write
                 * instead, use the same API as for edit-config
                 * except the edit_type is set differently  */
                /* res = cfg_fill_candidate_from_inline(sourceval); */
                msg->rpc_top_editop = OP_EDITOP_REPLACE;
                res = agt_val_apply_write(scb, msg, copyparms->destcfg,
                                          sourceval, OP_EDITOP_REPLACE);
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        } else if (copyparms->desturlspec != NULL) {
            /* copy from inline data to an URL */
            if (copyparms->destfile == NULL) {
                /* creating a new file; get the destfile to use */
                copyparms->destfile = 
                    agt_get_target_filespec(copyparms->desturlspec, &res);
            } 
            if (res == NO_ERR) {
                res = cfg_save_inline(copyparms->destfile, sourceval, FALSE);
            }
        } else {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    } else if (copyparms->srccfg != NULL) {
        if (copyparms->destcfg != NULL) {
            /* copy from one config to config or URL */
            switch (copyparms->destcfg->cfg_id) {
            case NCX_CFGID_STARTUP:
                res = agt_ncx_cfg_save(copyparms->srccfg, FALSE);
                break;
            case NCX_CFGID_CANDIDATE:
                switch (copyparms->srccfg->cfg_id) {
                case NCX_CFGID_RUNNING:
                    /* same as discard-changes */
                    res = cfg_fill_candidate_from_running();
                    break;
                case NCX_CFGID_STARTUP:
                    res = cfg_fill_candidate_from_startup();
                    break;
                default:
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                }
                break;
            case NCX_CFGID_RUNNING:
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
        } else if (copyparms->desturlspec != NULL) {
            /* copy from source config to an URL */
            if (copyparms->destfile == NULL) {
                /* creating a new file; get the destfile to use */
                copyparms->destfile = 
                    agt_get_target_filespec(copyparms->desturlspec, &res);
            } 
            if (res == NO_ERR) {
                res = cfg_save_inline(copyparms->destfile, 
                                      copyparms->srccfg->root, FALSE);
            }
        } else {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    } else if (copyparms->srcfile != NULL) {
        /* this is an URL to URL copy;
         * not supported at this time
         */
        res = SET_ERROR(ERR_INTERNAL_VAL);
    } else {
        /* no source parameter is set */
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
        /* config operation failed */
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         (copyparms->destcfg) 
                         ? NCX_NT_CFG : NCX_NT_STRING,
                         (copyparms->destcfg) ?
                         (void *)copyparms->destcfg : 
                         (void *)copyparms->desturlspec,
                         NCX_NT_NONE, 
                         NULL);
    }

    free_copyparms(copyparms);

    return res;

} /* copy_config_invoke */


/********************************************************************
* FUNCTION delete_config_validate
*
* delete-config : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    delete_config_validate (ses_cb_t *scb,
                            rpc_msg_t *msg,
                            xml_node_t *methnode)
{
    const agt_profile_t  *prof;
    cfg_template_t       *target = NULL;
    status_t              res;
    void                 *errval = NULL;
    const xmlChar        *desturl = NULL;
    xmlChar              *desturlspec = NULL, *destfile = NULL;
    char                 *errstr = NULL;
    val_value_t          *urlval = NULL;
    ncx_node_t            errtyp;

    /* get the config to delete */
    res = agt_get_cfg_from_parm(NCX_EL_TARGET, msg, methnode, &target);
    if (res == ERR_NCX_FOUND_URL) {
        res = agt_get_url_from_parm(NCX_EL_TARGET, msg, methnode, 
                                    &desturl, &urlval);
        if (res != NO_ERR) {
            /* errors already recorded */
            return res;
        }
    } else if (res == ERR_NCX_FOUND_INLINE) {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    } else if (res != NO_ERR) {
        return res;  /* error already recorded */
    } 

    /* get the agent profile */
    prof = agt_get_profile();
    if (!prof) {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* check the cfg value provided -- only <startup> and
     * <candidate> databases are supported, plus <url> files
     */
    if (res == NO_ERR) {
        /* check if the startup config is allowed to be deleted
         * and that is the config to be deleted
         */
        if (desturl != NULL) {
            if (!prof->agt_useurl) {
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
            } else {
                desturlspec = agt_get_filespec_from_url(desturl, &res);

                /* check the URL parameter to see if it is valid */
                if (desturlspec != NULL && res == NO_ERR) {
                    destfile = ncxmod_find_data_file(desturlspec, FALSE, &res);
                }
            }
        } else if (target->cfg_id == NCX_CFGID_STARTUP) {
            if (!prof->agt_has_startup) {
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
            }
        } else if (target->cfg_id == NCX_CFGID_CANDIDATE) {
            res = ERR_NCX_OPERATION_NOT_SUPPORTED;
        } else {
            res = ERR_NCX_INVALID_VALUE;
        }
    }

    /* check if okay to delete this config now */
    if (res == NO_ERR && desturl == NULL) {
        res = cfg_ok_to_write(target, SES_MY_SID(scb));
    }

    if (res != NO_ERR) {
        errval = urlval ? urlval :
            val_find_child(msg->rpc_input, val_get_mod_name(msg->rpc_input),
                           NCX_EL_TARGET);
        if (errval) {
            errtyp = NCX_NT_VAL;
        } else {
            errtyp = NCX_NT_NONE;
        }

        errstr = strdup("/nc:rpc/nc:delete-config/nc:target");
        agt_record_error(scb, 
                         &msg->mhdr,
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode, 
                         (errstr) ? NCX_NT_STRING : NCX_NT_NONE,
                         errstr,
                         errtyp, 
                         errval);
        if (errstr != NULL) {
            m__free(errstr);
        }
        if (destfile != NULL) {
            m__free(destfile);
        }
    } else {
        msg->rpc_user1 = target;
        msg->rpc_user2 = destfile;
    }

    if (desturlspec != NULL) {
        m__free(desturlspec);
    }

    return res;

} /* delete_config_validate */


/********************************************************************
* FUNCTION delete_config_invoke
*
* delete-config : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    delete_config_invoke (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    cfg_template_t       *target = (cfg_template_t *)msg->rpc_user1;
    xmlChar              *destfile = (xmlChar *)msg->rpc_user2;  
    status_t              res = NO_ERR;
    
    if ( !destfile ) {
        const agt_profile_t  *profile;
        const xmlChar        *startspec;

        /* else this must be a request to delete the startup */
        profile = agt_get_profile();

        /* use the user-set startup or default filename */
        startspec = ( profile->agt_startup ?  profile->agt_startup
                                        : NCX_DEF_STARTUP_FILE );

        destfile = ncxmod_find_data_file( startspec, FALSE, &res );
        if ( !destfile ) {
            log_error("\nError: cannot find config file '%s' to delete",
                      startspec);
            return res;
        }
    }

    if ( 0 != remove( (const char *)destfile) ) {
        char  *errstr;
        res = errno_to_status();
        errstr = (char *) xml_strdup(
                    (const xmlChar *) "/nc:rpc/nc:delete-config/nc:target" );

         if (errstr) {
             agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, 
                          res, methnode, NCX_NT_STRING, destfile,
                          NCX_NT_STRING, errstr );
             m__free(errstr);
         } else {
             agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, 
                          res, methnode, NCX_NT_STRING, destfile,
                          NCX_NT_NONE, NULL );
         }
    } else if ( !msg->rpc_user2 ) {
        if ( target != NULL && target->root != NULL) {
            val_free_value( target->root );
            target->root = NULL;
        }
    }

    if (destfile != NULL) {
        m__free(destfile);
    }

    return res;
} /* delete_config_invoke */


/********************************************************************
* FUNCTION lock_validate
*
* lock : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    lock_validate (ses_cb_t *scb,
                   rpc_msg_t *msg,
                   xml_node_t *methnode)
{

    status_t         res;
    cfg_template_t  *cfg;

    /* get the config to lock */
    res = agt_get_cfg_from_parm(NCX_EL_TARGET, msg, methnode, &cfg);
    if (res != NO_ERR) {
        return res;
    }

    /* get the config state; check if lock can be granted
     * based on the current config state
     */
    res = cfg_ok_to_lock(cfg);

    /* cannot start a lock when confirmed commit pending */
    if (res == NO_ERR) {
        if ((cfg->cfg_id == NCX_CFGID_RUNNING ||
             cfg->cfg_id == NCX_CFGID_CANDIDATE) &&
            commit_cb.cc_active) {
            res = ERR_NCX_IN_USE_COMMIT;
        }
    }

    if (res != NO_ERR) {
        /* lock probably already held */
        agt_record_error(scb,
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_CFG, 
                         cfg,
                         NCX_NT_NONE,
                         NULL);
    } else {
        /* lock can be granted
         * setup the user1 scratchpad with the cfg to lock 
         */
        msg->rpc_user1 = (void *)cfg;
    }

    return res;

} /* lock_validate */


/********************************************************************
* FUNCTION lock_invoke
*
* lock : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    lock_invoke (ses_cb_t *scb,
                 rpc_msg_t *msg,
                 xml_node_t *methnode)
{
    cfg_template_t   *cfg;
    status_t          res;

    cfg = (cfg_template_t *)msg->rpc_user1;
    res = cfg_lock(cfg, SES_MY_SID(scb), CFG_SRC_NETCONF);
    if (res != NO_ERR) {
        /* config is in a state where locks cannot be granted */
        agt_record_error(scb,
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL,
                         NCX_NT_NONE, 
                         NULL);
    }

    return res;

} /* lock_invoke */


/********************************************************************
* FUNCTION unlock_validate
*
* unlock : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    unlock_validate (ses_cb_t *scb,
                     rpc_msg_t *msg,
                     xml_node_t *methnode)
{
    status_t         res;
    cfg_template_t  *cfg;

    /* get the config to lock */
    res = agt_get_cfg_from_parm(NCX_EL_TARGET, msg, methnode, &cfg);
    if (res != NO_ERR) {
        return res;
    }

    /* get the config state; check if lock is already granted
     * based on the current config state
     */
    res = cfg_ok_to_unlock(cfg, SES_MY_SID(scb));
    if (res == NO_ERR) {
        msg->rpc_user1 = (void *)cfg;
    } else {
        agt_record_error(scb,
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE,
                         NULL);
    }

    return res;

} /* unlock_validate */


/********************************************************************
* FUNCTION unlock_invoke
*
* unlock : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    unlock_invoke (ses_cb_t *scb,
                   rpc_msg_t *msg,
                   xml_node_t *methnode)
{
    cfg_template_t  *cfg;
    status_t         res;

    cfg = (cfg_template_t *)msg->rpc_user1;
    res = cfg_unlock(cfg, SES_MY_SID(scb));
    if (res != NO_ERR) {
        agt_record_error(scb,
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE,
                         NULL);
    }
    return res;

} /* unlock_invoke */


/********************************************************************
* FUNCTION close_session_invoke
*
* close-session : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    close_session_invoke (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{

    (void)msg;
    (void)methnode;
    agt_ses_request_close(scb, 
                          SES_MY_SID(scb),
                          SES_TR_CLOSED);
    return NO_ERR;

} /* close_session_invoke */


/********************************************************************
* FUNCTION kill_session_validate
*
* kill-session : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    kill_session_validate (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode)
{
    status_t         res;
    val_value_t     *val;

    res = NO_ERR;

    /* get the session-id parameter */
    val = val_find_child(msg->rpc_input, 
                         NC_MODULE,
                         NCX_EL_SESSION_ID);
    if (!val || val->res != NO_ERR) {
        /* error already recorded in parse phase */
        if (val) {
            return val->res;
        } else {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    /* make sure the session-id is valid 
     * The RFC forces a kill-session of the current
     * session to be an error, even though agt_ses.c
     * supports this corner-case
     */
    if (VAL_UINT(val) == scb->sid
        || !agt_ses_session_id_valid(VAL_UINT(val))) {
        res = ERR_NCX_INVALID_VALUE;
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
    }

    return res;

}  /* kill_session_validate */


/********************************************************************
* FUNCTION kill_session_invoke
*
* kill-session : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    kill_session_invoke (ses_cb_t *scb,
                         rpc_msg_t *msg,
                         xml_node_t *methnode)
{
    val_value_t *val;
    status_t     res;

    /* get the session-id parameter */
    val = val_find_child(msg->rpc_input, 
                         NC_MODULE,
                         NCX_EL_SESSION_ID);
    if (!val || val->res != NO_ERR) {
        /* error already recorded in parse phase */

        if (val) {
            res = val->res;
        } else {
            res = ERR_NCX_OPERATION_FAILED;
        }

        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res,
                         methnode, 
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
    }
    else
    {
        ses_id_t sid = (ses_id_t)VAL_UINT(val);
        agt_ses_kill_session( agt_ses_get_session_for_id( sid ),
                                  scb->sid,
                                  SES_TR_KILLED );
    }
    return NO_ERR;

} /* kill_session_invoke */


/********************************************************************
* FUNCTION validate_validate
*
* validate : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    validate_validate (ses_cb_t *scb,
                       rpc_msg_t *msg,
                       xml_node_t *methnode)
{
    cfg_template_t *target = NULL;
    val_value_t *val = NULL, *child = NULL, *rootval = NULL, *urlval = NULL;
    const xmlChar *errstr = NULL, *urlstr = NULL;
    const agt_profile_t  *profile = agt_get_profile();
    xmlChar *urlspec = NULL, *urlfilename = NULL;
    status_t res = NO_ERR;
    boolean errdone = FALSE, delneeded = FALSE, needfullcheck = FALSE;

    if (!profile->agt_usevalidate) {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    }

    if (res == NO_ERR) {
        /* attempt to get the source parameter */
        val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_SOURCE);
        if (!val || val->res != NO_ERR) {
            res = val ? val->res : ERR_NCX_OPERATION_FAILED;
            errstr = NCX_EL_SOURCE;
        }
    }

    if (res == NO_ERR) {
        /* attempt to get the child parameter */
        child = val_get_first_child(val);
        if (!child || child->res != NO_ERR) {
           res = child ? child->res : ERR_NCX_MISSING_PARM;
           errstr = child ? child->name : NCX_EL_SOURCE;
        }
    }

    /* determine which variant of the input parameter is present */
    if (res == NO_ERR) {
        if (!xml_strcmp(child->name, NCX_EL_RUNNING)) {
            target = cfg_get_config_id(NCX_CFGID_RUNNING);
        } else if (!xml_strcmp(child->name, NCX_EL_CANDIDATE)) {
            if (profile->agt_targ != NCX_AGT_TARG_CANDIDATE) {
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                errstr = child->name;
            } else {
                target = cfg_get_config_id(NCX_CFGID_CANDIDATE);
                needfullcheck = TRUE;
            }
        } else if (!xml_strcmp(child->name, NCX_EL_STARTUP)) {
            if (!profile->agt_has_startup) {
                res = ERR_NCX_OPERATION_NOT_SUPPORTED;
                errstr = child->name;
            } else {
                target = cfg_get_config_id(NCX_CFGID_STARTUP);
            }
        } else if (!xml_strcmp(child->name, NCX_EL_URL)) {
            urlstr = VAL_STR(child);
            errstr = child->name;

            /* get the filespec out of the URL */
            urlspec = agt_get_filespec_from_url(urlstr, &res);     

            if (urlspec || res == NO_ERR) {
                urlfilename = ncxmod_find_data_file(urlspec, FALSE, &res);

                if (urlfilename && res == NO_ERR) {
                    /* get the external file loaded into a value struct
                     * for the <nc:config> object node
                     */           
                    target = cfg_get_config_id(NCX_CFGID_RUNNING);
                    urlval = agt_rpc_get_config_file(urlfilename, target,
                                 SES_MY_SID(scb), RPC_ERR_QUEUE(msg), &res);
                    if (res == NO_ERR) {
                        rootval = urlval;
                        delneeded = TRUE;
                    }
                }
            }
        } else if (!xml_strcmp(child->name, NCX_EL_CONFIG)) {
            rootval = child;
            delneeded = TRUE;
        }

        if (res == NO_ERR && !rootval) {
            if (!target || !target->root) {
                res = ERR_NCX_OPERATION_FAILED;
                errstr = child->name;
            } else {
                rootval = target->root;
            }
        }
    }

    if (res == NO_ERR) {
        /* set the error parameter to gather the most errors */
        msg->rpc_err_option = OP_ERROP_CONTINUE;

        /* allocate a transaction control block; the target cfg_id will
         * be ignored if this is an inline validate   */
        msg->rpc_txcb = 
            agt_cfg_new_transaction(target ? target->cfg_id : NCX_CFGID_RUNNING,
                                    AGT_CFG_EDIT_TYPE_FULL, FALSE, TRUE, &res);
        if (msg->rpc_txcb == NULL || res != NO_ERR) {
            if (res == NO_ERR) {
                res = ERR_NCX_OPERATION_FAILED;
            }
        } else {
            /* the validate command is just a root check
             * there are no individual edits to check, so the
             * SIL validate callbacks do not need to be called */
            if (delneeded) {
                res = agt_val_delete_dead_nodes(scb, msg, rootval);
                // errors recorded if res != NO_ERR
            }
            if (res == NO_ERR && needfullcheck) {
                /* set target to NULL to suppress checks on any locks */
                op_editop_t useop = (profile->agt_validate_all) ?
                    OP_EDITOP_LOAD : OP_EDITOP_COMMIT;
                
                res = agt_val_validate_write(scb, msg, NULL, rootval, useop);
            }
            if (res == NO_ERR) {
                res = agt_val_root_check(scb, &msg->mhdr, msg->rpc_txcb, 
                                         rootval);
            }
            errdone = TRUE;  // rpc-error already recorded if res != NO_ERR
        }
    }

    if (res != NO_ERR && !errdone) {
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, 
                         methnode, (errstr) ? NCX_NT_STRING : NCX_NT_NONE,
                         (errstr) ? errstr : NULL, NCX_NT_NONE, NULL);
    }

    if (urlval != NULL) {
        val_free_value(urlval);
    }
    if (urlspec != NULL) {
        m__free(urlspec);
    }
    if (urlfilename != NULL) {
        m__free(urlfilename);
    }

    return res;

} /* validate_validate */


/********************************************************************
* FUNCTION commit_validate
*
* commit : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    commit_validate (ses_cb_t *scb,
                     rpc_msg_t *msg,
                     xml_node_t *methnode)
{
    const agt_profile_t  *profile = agt_get_profile();
    val_value_t          *errval = NULL;
    status_t              res = NO_ERR;
    boolean               errdone = FALSE;

    if (profile->agt_targ != NCX_AGT_TARG_CANDIDATE) {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    } else {
        /* get the candidate config */
        cfg_template_t *candidate = cfg_get_config_id(NCX_CFGID_CANDIDATE);
        cfg_template_t *running = cfg_get_config_id(NCX_CFGID_RUNNING);

        assert ( candidate && "candidate is NULL!" );
        assert ( running && "candidate is NULL!" );

        val_value_t *persistval, *persistidval;

        /* make sure base:1.1 params allowed if present */
        persistval = val_find_child(msg->rpc_input,
                                    val_get_mod_name(msg->rpc_input),
                                    NCX_EL_PERSIST);
        persistidval = val_find_child(msg->rpc_input,
                                      val_get_mod_name(msg->rpc_input),
                                      NCX_EL_PERSIST_ID);

        if ((persistval != NULL || persistidval != NULL) &&
            ses_get_protocol(scb) == NCX_PROTO_NETCONF10) {
            res = ERR_NCX_PROTO11_NOT_ENABLED;
            if (persistval != NULL) {
                errval = persistval;
            } else {
                errval = persistidval;
            }
        }

        if (res == NO_ERR && persistidval != NULL) {
            if (!commit_cb.cc_active) {
                res = ERR_NCX_CC_NOT_ACTIVE;
                errval = persistidval;
            } else if (commit_cb.cc_persist_id == NULL ||
                       xml_strcmp(VAL_STR(persistidval),
                                  commit_cb.cc_persist_id)) {
                res = ERR_NCX_INVALID_VALUE;
                errval = persistidval;
            }
        }

        if (res == NO_ERR) {
            /* check if the candidate config can be written */
            res = cfg_ok_to_write(candidate, SES_MY_SID(scb));
        }

        if (res == NO_ERR) {
            /* check if the running config can be written */
            res = cfg_ok_to_write(running, SES_MY_SID(scb));
        }


        if (res == NO_ERR) {
            /* allocate a transaction control block */
            msg->rpc_txcb = 
                agt_cfg_new_transaction(NCX_CFGID_RUNNING, 
                                        AGT_CFG_EDIT_TYPE_FULL, FALSE, 
                                        FALSE, &res);
            if (msg->rpc_txcb == NULL || res != NO_ERR) {
                if (res == NO_ERR) {
                    res = ERR_NCX_OPERATION_FAILED;
                }
                agt_record_error( scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                                  methnode, NCX_NT_NONE, NULL, NCX_NT_NONE,
                                  NULL );
                errdone = TRUE;
            } else {
                /* check if this session allowed to perform all the
                 * edits in the commit request   */
                msg->rpc_txcb->commitcheck = TRUE;
                res = agt_val_check_commit_edits(scb, msg, candidate, running);
                if (res != NO_ERR) {
                    errdone = TRUE;
                }

                /* do not need to delete dead nodes in candidate because
                 * it is already done at the end of every edit */

                if (res == NO_ERR ) {
                    res = agt_val_root_check(scb, &msg->mhdr, msg->rpc_txcb,
                                             candidate->root);
                    if (res != NO_ERR) {
                        errdone = TRUE;
                    }
                }
                msg->rpc_txcb->commitcheck = FALSE;
            }
        }
    }

    if (res != NO_ERR && !errdone) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         (errval != NULL) ? NCX_NT_VAL : NCX_NT_NONE, 
                         errval);
    }

    return res;

} /* commit_validate */


/********************************************************************
* FUNCTION write_config
*
* Write the specified cfg->root to the the default backup source
*
* INPUTS:
*    filespec == complete path for the output file
*    cfg == config template to write to XML file
* 
* RETURNS:
*    status
*********************************************************************/
static status_t
    write_config (const xmlChar *filespec,
                  cfg_template_t *cfg)
{
    agt_profile_t     *profile;
    status_t           res;
    xml_attrs_t        attrs;

    if (cfg->root == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    profile = agt_get_profile();

    /* write the new startup config */
    xml_init_attrs(&attrs);

    /* output to the specified file or STDOUT */
    res = xml_wr_check_file(filespec,
                            cfg->root,
                            &attrs,
                            XMLMODE,
                            WITHHDR,
                            TRUE,
                            0,
                            profile->agt_indent,
                            agt_check_save);

    xml_clean_attrs(&attrs);

    return res;

} /* write_config */


/********************************************************************
* FUNCTION clear_commit_cb
*
* Clear the commit_cb data structure
*
*********************************************************************/
static void
    clear_commit_cb (void)
{
    if (commit_cb.cc_persist_id != NULL) {
        m__free(commit_cb.cc_persist_id);
    }
    if (commit_cb.cc_backup_source != NULL) {
        m__free(commit_cb.cc_backup_source);
    }

    memset(&commit_cb, 0x0, sizeof(commit_cb_t));

} /* clear_commit_cb */


/********************************************************************
* FUNCTION commit_invoke
*
* commit : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    commit_invoke (ses_cb_t *scb,
                   rpc_msg_t *msg,
                   xml_node_t *methnode)
{
    val_value_t    *confirmedval, *timeoutval;
    val_value_t    *persistval, *persistidval, *errval;
    cfg_template_t *candidate, *running;
    xmlChar        *fname;
    status_t        res;
    boolean         save_nvstore, errdone, timeout_extended;

    res = NO_ERR;
    errdone = FALSE;
    timeout_extended = FALSE;
    errval = NULL;

    candidate = cfg_get_config_id(NCX_CFGID_CANDIDATE);
    running = cfg_get_config_id(NCX_CFGID_RUNNING);
    if (candidate == NULL || running == NULL) {
        res = SET_ERROR(ERR_INTERNAL_VAL);
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION,
                         res,
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE,
                         NULL);
        return res;
    }

    save_nvstore = TRUE;

    /* get the confirmed parameter */
    confirmedval = val_find_child(msg->rpc_input,
                                  val_get_mod_name(msg->rpc_input),
                                  NCX_EL_CONFIRMED);

    /* get the confirm-timeout parameter */
    timeoutval = val_find_child(msg->rpc_input,
                                val_get_mod_name(msg->rpc_input),
                                NCX_EL_CONFIRM_TIMEOUT);

    
    /* get the persist parameters only if base:1.1 enabled */
    if (ses_get_protocol(scb) == NCX_PROTO_NETCONF11) {
        persistval = val_find_child(msg->rpc_input,
                                    val_get_mod_name(msg->rpc_input),
                                    NCX_EL_PERSIST);
        persistidval = val_find_child(msg->rpc_input,
                                      val_get_mod_name(msg->rpc_input),
                                      NCX_EL_PERSIST_ID);
    } else {
        persistval = NULL;
        persistidval = NULL;
    }

    /* figure out what to do wrt/ confirmed-commit */
    if (commit_cb.cc_active) {
        /* confirmed-commit already active
         * see if this commit is finishing the
         * confirmed commit or extending the timer
         * and perhaps adding more data to running
         */
        if (confirmedval != NULL) {
            if (persistidval != NULL && 
                commit_cb.cc_persist_id != NULL &&
                !xml_strcmp(VAL_STR(persistidval), 
                            commit_cb.cc_persist_id)) {
                /* this session is allowed to be different than
                 * one that started the conf-commit
                 */
                ;
            } else if (commit_cb.cc_persist_id == NULL) {
                /* check same session that started cc */
                if (commit_cb.cc_ses_id != SES_MY_SID(scb)) {
                    res = ERR_NCX_IN_USE_COMMIT;
                    errval = confirmedval;
                }
            } else {
                res = ERR_NCX_OPERATION_FAILED;
                errval = confirmedval;
            }

            /* set the persist-id if needed */
            if (res == NO_ERR && persistval != NULL) {
                if (commit_cb.cc_persist_id != NULL) {
                    if (LOGDEBUG) {
                        log_debug("\nagt_ncx: confirmed-commit by '%u' "
                                  "changing persist from '%s' to '%s'",
                                  SES_MY_SID(scb),
                                  commit_cb.cc_persist_id,
                                  VAL_STR(persistval));
                    }
                    m__free(commit_cb.cc_persist_id);
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nagt_ncx: confirmed-commit by '%u' "
                                  "setting persist to '%s'",
                                  SES_MY_SID(scb),
                                  VAL_STR(persistval));
                    }
                }
                commit_cb.cc_persist_id = 
                    xml_strdup(VAL_STR(persistval));
                if (commit_cb.cc_persist_id == NULL) {
                    res = ERR_INTERNAL_MEM;
                    errval = persistval;
                }
            }

            /* extend the conf-commit timer and send a notification */
            if (res == NO_ERR) {
                /* perhaps set a new owner session */
                commit_cb.cc_ses_id = SES_MY_SID(scb);

                /* extend the timer */
                (void)time(&commit_cb.cc_start_time);
                if (timeoutval != NULL) {
                    commit_cb.cc_cancel_timeout = VAL_UINT(timeoutval);
                } else {
                    commit_cb.cc_cancel_timeout = NCX_DEF_CONFIRM_TIMEOUT;
                }
                if (LOGDEBUG2) {
                    log_debug2("\nConfirmed commit timer extended "
                               "by %u seconds",
                               commit_cb.cc_cancel_timeout);
                }
                save_nvstore = FALSE;
                timeout_extended = TRUE;
                agt_sys_send_sysConfirmedCommit(scb, NCX_CC_EVENT_EXTEND);
            }
        } else {
            /* confirmedval == NULL; finishing conf-commit */
            if (persistidval == NULL &&
                commit_cb.cc_ses_id != SES_MY_SID(scb)) {
                /* persist-id not present and session ID did not match */
                res = ERR_NCX_IN_USE_COMMIT;
                errval = persistidval;
            } else if (LOGDEBUG2) {
                log_debug2("\nConfirmed commit completed by session %u",
                           SES_MY_SID(scb));
            }

            /* finish the confirmed-commit unless invalid persist-id */
            if (res == NO_ERR) {
                res = agt_ncx_cfg_save(running, FALSE);
            }
            if (res == NO_ERR) {
                agt_sys_send_sysConfirmedCommit(scb, NCX_CC_EVENT_COMPLETE);

                clear_commit_cb();
            }
        }
    } else {
        /* check if a new confirmed commit is starting */
        if (confirmedval != NULL) {
            /* save the session ID that started this conf-commit
             * if persist active and orig session terminated,
             * this will be reset to 0; otherwise keep
             * the starting session the same
             */
            if (commit_cb.cc_ses_id == 0) {
                commit_cb.cc_ses_id = SES_MY_SID(scb);
            }

            if (persistval != NULL) {
                if (commit_cb.cc_persist_id != NULL) {
                    SET_ERROR(ERR_INTERNAL_VAL);
                    if (LOGDEBUG) {
                        log_debug("\nagt_ncx: confirmed-commit by '%u' "
                                  "changing persist from '%s' to '%s'",
                                  SES_MY_SID(scb),
                                  commit_cb.cc_persist_id,
                                  VAL_STR(persistval));
                    }
                    m__free(commit_cb.cc_persist_id);
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nagt_ncx: confirmed-commit by '%u' "
                                  "setting persist to '%s'",
                                  SES_MY_SID(scb),
                                  VAL_STR(persistval));
                    }
                }

                commit_cb.cc_persist_id = 
                    xml_strdup(VAL_STR(persistval));
                if (commit_cb.cc_persist_id == NULL) {
                    res = ERR_INTERNAL_MEM;
                }
            }

            if (res == NO_ERR) {
                /* set the timer */
                (void)time(&commit_cb.cc_start_time);
                if (timeoutval) {
                    commit_cb.cc_cancel_timeout = VAL_UINT(timeoutval);
                } else {
                    commit_cb.cc_cancel_timeout = NCX_DEF_CONFIRM_TIMEOUT;
                }
                commit_cb.cc_active = TRUE;
                save_nvstore = FALSE;

                if (LOGDEBUG2) {
                    log_debug2("\nConfirmed commit started, timeout in "
                               "%u seconds",
                               commit_cb.cc_cancel_timeout);
                }
                agt_sys_send_sysConfirmedCommit(scb, NCX_CC_EVENT_START);
            }
        } else {
            /* no confirmed commit is starting */
            save_nvstore = TRUE;
        }
    }

    /* make a backup of running to make sure
     * that if this step fails, running config MUST not change
     */ 
    if (res == NO_ERR &&
        commit_cb.cc_backup_source == NULL) {
        /* search for the default startup-cfg.xml filename */
        fname = ncxmod_find_data_file(NCX_DEF_BACKUP_FILE, FALSE, &res);
        if (fname) {
            /* rewrite the existing backup file
             * hand off fname malloced memory here 
             */
            commit_cb.cc_backup_source = fname;
        } else if (running->src_url) {
            /* use the same path as the startup config file
             * if it has already been set; it may be different
             * each boot via the --startup CLI parameter
             */
            res = NO_ERR;
            commit_cb.cc_backup_source = 
                ncxmod_make_data_filespec_from_src(running->src_url,
                                                   NCX_DEF_BACKUP_FILE,
                                                   &res);
        } else {
            /* create a new backup file name
             * should really check the res code
             * to make sure the error is some sort
             * of not-found error, but if it was a fatal
             * malloc error, etc. then the following
             * attempt to create a backup will probably 
             * fail as well
             */
            res = NO_ERR;
            commit_cb.cc_backup_source = 
                ncxmod_make_data_filespec(NCX_DEF_BACKUP_FILE, &res);
        }
    }

    /* attempt to save the backup, if a file name is available */
    if (res == NO_ERR && timeout_extended == FALSE) {
        res = write_config(commit_cb.cc_backup_source, running);
    }

    if (res == NO_ERR) {
        res = agt_val_apply_commit(scb, msg, candidate, running, save_nvstore);
        if (res != NO_ERR) {
            errdone = TRUE;

            if (msg->rpc_txcb->rollback_res != NO_ERR) {
                /* close out the current transaction now so another
                 * one can start   */
                agt_cfg_free_transaction(msg->rpc_txcb);
                msg->rpc_txcb = NULL;

                /* restore the config because rollback failed */
                status_t res2 = 
                    agt_ncx_load_backup(commit_cb.cc_backup_source, running,
                                        commit_cb.cc_ses_id);
                if (res2 != NO_ERR) {
                    res = res2;
                    errdone = FALSE;
                }
            }
        } else {
            res = cfg_fill_candidate_from_running();
        }
    }

    if (res != NO_ERR && !errdone) {
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res,
                         methnode, NCX_NT_NONE, NULL, 
                         (errval != NULL) ? NCX_NT_VAL : NCX_NT_NONE,
                         errval);
    }

    return res;

} /* commit_invoke */


/********************************************************************
* FUNCTION cancel_commit_validate
*
* cancel-commit : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    cancel_commit_validate (ses_cb_t *scb,
                            rpc_msg_t *msg,
                            xml_node_t *methnode)
{
    cfg_template_t       *running;
    const agt_profile_t  *profile;
    val_value_t          *persistidval;
    const xmlChar        *cc_persistid;
    status_t              res;

    running = NULL;
    profile = agt_get_profile();
    persistidval = NULL;
    cc_persistid = NULL;
    res = NO_ERR;

    if (ses_get_protocol(scb) != NCX_PROTO_NETCONF11) {
        res = ERR_NCX_UNKNOWN_ELEMENT;
    } else if (profile->agt_targ != NCX_AGT_TARG_CANDIDATE) {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    } else if (!agt_ncx_cc_active()) {
        res = ERR_NCX_OPERATION_FAILED;
    } else {
        /* get the running config */
        running = cfg_get_config_id(NCX_CFGID_RUNNING);
        if (running == NULL) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            /* check if this session is allowed to revert running now */
            res = cfg_ok_to_write(running, SES_MY_SID(scb));
        }
    }

    if (res == NO_ERR) {
        cc_persistid = agt_ncx_cc_persist_id();

        persistidval = val_find_child(msg->rpc_input,
                                      val_get_mod_name(msg->rpc_input),
                                      NCX_EL_PERSIST_ID);
        if (persistidval == NULL && cc_persistid == NULL) {
            ; /* no persist in progress or requested - this is OK */
        } else if (persistidval == NULL && cc_persistid != NULL) {
            /* the persist-id is mandatory now */
            res = ERR_NCX_MISSING_PARM;
        } else if (persistidval != NULL && cc_persistid == NULL) {
            /* no persist cc in progress so cannot match ID */
            res = ERR_NCX_CC_NOT_ACTIVE;
        } else {
            /* try to match the persist ID */
            if (xml_strcmp(VAL_STR(persistidval), cc_persistid)) {
                res = ERR_NCX_INVALID_VALUE;
            }
        }
    }

    if (res == NO_ERR && cc_persistid == NULL) {
        /* the cc session is the only one that can cancel */
        if (SES_MY_SID(scb) != agt_ncx_cc_ses_id()) {
            res = ERR_NCX_OPERATION_FAILED;
        }
    }

    if (res != NO_ERR) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
        return res;
    }

    return res;

} /* cancel_commit_validate */


/********************************************************************
* FUNCTION cancel_commit_invoke
*
* cancel-commit : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    cancel_commit_invoke (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    (void)msg;
    (void)methnode;
    agt_ncx_cancel_confirmed_commit(scb, NCX_CC_EVENT_CANCEL);
    return NO_ERR;

} /* cancel_commit_invoke */


/********************************************************************
* FUNCTION discard_changes_validate
*
* discard-changes : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    discard_changes_validate (ses_cb_t *scb,
                              rpc_msg_t *msg,
                              xml_node_t *methnode)
{
    cfg_template_t       *candidate;
    const agt_profile_t  *profile;
    status_t              res;

    res = NO_ERR;
    profile = agt_get_profile();

    if (profile->agt_targ != NCX_AGT_TARG_CANDIDATE) {
        res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    } else {
        /* get the candidate config */
        candidate = cfg_get_config_id(NCX_CFGID_CANDIDATE);
        if (!candidate) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            /* check if this session is allowed to invoke now */
            res = cfg_ok_to_write(candidate, SES_MY_SID(scb));
        }
    }

    if (res != NO_ERR) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
        return res;
    }

    return res;

} /* discard_changes_validate */


/********************************************************************
* FUNCTION discard_changes_invoke
*
* discard-changes : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    discard_changes_invoke (ses_cb_t *scb,
                           rpc_msg_t *msg,
                           xml_node_t *methnode)
{
    cfg_template_t *candidate;
    status_t        res;

    res = NO_ERR;


    /* get the candidate config */
    candidate = cfg_get_config_id(NCX_CFGID_CANDIDATE);
    if (!candidate) {
        res = SET_ERROR(ERR_INTERNAL_VAL);
    } else if (cfg_get_dirty_flag(candidate)) {
        res = cfg_fill_candidate_from_running();
    }

    if (res != NO_ERR) {
        agt_record_error(scb, 
                         &msg->mhdr, 
                         NCX_LAYER_OPERATION, 
                         res, 
                         methnode,
                         NCX_NT_NONE, 
                         NULL, 
                         NCX_NT_NONE, 
                         NULL);
    }

    return res;

} /* discard_changes_invoke */


/********************************************************************
* FUNCTION load_config_validate
*
* load-config : validate params callback
*
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    load_config_validate (ses_cb_t *scb,
                          rpc_msg_t *msg,
                          xml_node_t *methnode)
{
    status_t res = NO_ERR;

    /* This special callback is used by internal NCX functions
     * to load the initial configuration.  The msg->rpc_user1 parameter 
     * has already been set to the address of the cfg_template_t
     * to fill in.
     *
     * NOTE: HACK DEPENDS ON THE agt_rpc_load_config_file to setup
     * the rpc->rpc_user1 parameter
     *
     * make sure this is a DUMMY session, not a real session
     */
    if (scb->type != SES_TYP_DUMMY) {
        res = ERR_NCX_ACCESS_DENIED;
        agt_record_error(scb, &msg->mhdr, NCX_LAYER_OPERATION, res, methnode, 
                         NCX_NT_NONE, NULL, NCX_NT_NONE, NULL);
        return res;
    }

    cfg_template_t *target = (cfg_template_t *)msg->rpc_user1;
    if (!target) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* get the <nc:config> parameter */
    val_value_t *val = val_find_child(msg->rpc_input, NC_MODULE, NCX_EL_CONFIG);
    if (!val) {
        /* we shouldn't get here if the config param is missing */
        return SET_ERROR(ERR_NCX_OPERATION_FAILED);
    }

    /* check if any error values need to be purged from the load config */
    if (msg->rpc_parse_errors) {
        val_purge_errors_from_root(val);
    }

    /* create a transaction CB */
    msg->rpc_txcb = 
        agt_cfg_new_transaction(NCX_CFGID_RUNNING, AGT_CFG_EDIT_TYPE_FULL,
                                FALSE, FALSE, &res);
    if (msg->rpc_txcb == NULL || res != NO_ERR) {
        if (res == NO_ERR) {
            res = ERR_NCX_OPERATION_FAILED;
        }
    } else {
        /* errors will be added as needed */
        res = agt_val_validate_write(scb, msg, target, val, 
                                     msg->rpc_top_editop);

        /* check what to do if errors occurred */
        agt_profile_t *profile = agt_get_profile();
        if (res != NO_ERR) {
            profile->agt_load_validate_errors = TRUE;
            if (profile->agt_startup_error) {
                /* any startup errors mean server shutdown */
                return res;
            }
            // delay this until end of load_running_config
            // error nodes are marked as deleted
            //val_purge_errors_from_root(val);
        }

        res = agt_val_delete_dead_nodes(scb, msg, val);
        if (res == NO_ERR) {
            res = agt_val_root_check(scb, &msg->mhdr, msg->rpc_txcb, val);
        }
        if (res != NO_ERR) {
            if (profile->agt_load_rootcheck_errors && 
                profile->agt_startup_error) {
                ;
            } else if (profile->agt_load_top_rootcheck_errors &&
                       profile->agt_running_error) {
                ;
            } else {
                // delay this until end of load_running_config
                // error nodes are marked as deleted
                //val_purge_errors_from_root(val);
                res = NO_ERR;
            }
        }
        msg->rpc_user2 = val;
    }

    return res;

} /* load_config_validate */


/********************************************************************
* FUNCTION load_config_invoke
*
* load-config : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    load_config_invoke (ses_cb_t *scb,
                        rpc_msg_t *msg,
                        xml_node_t *methnode)
{
    (void)methnode;
    status_t res = NO_ERR;

    /* This special callback is used by internal NCX functions
     * to load the initial configuration. 
     */
    cfg_template_t *target = (cfg_template_t *)msg->rpc_user1;
    val_value_t *val = (val_value_t *)msg->rpc_user2;

    if (target && val) {
        /* load the <config> into the target config */
        res = agt_val_apply_write(scb, msg, target, val, msg->rpc_top_editop);
        if (res != NO_ERR) {
            agt_profile_t *profile = agt_get_profile();
            profile->agt_load_apply_errors = TRUE;
        }
    } else {
        res = SET_ERROR(ERR_INTERNAL_PTR);
    }

    if (target) {
        val_clean_tree(target->root);
    }

    return res;

} /* load_config_invoke */


/********************************************************************
* FUNCTION load_invoke
*
* load module : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    load_invoke (ses_cb_t *scb,
                 rpc_msg_t *msg,
                 xml_node_t *methnode)
{
    val_value_t           *modval, *revval, *devval, *newval;
    ncx_module_t          *mod, *testmod;
    xmlChar               *moduri;
    agt_profile_t         *agt_profile;
    status_t               res;
    boolean                module_added, errdone, sil_loaded;

    res = NO_ERR;
    newval = NULL;
    mod = NULL;
    errdone = FALSE;
    sil_loaded = FALSE;
    module_added = FALSE;
    agt_profile = agt_get_profile();

    /* mandatory module name */
    modval = val_find_child(msg->rpc_input, AGT_SYS_MODULE, NCX_EL_MODULE);
    if (!modval || modval->res != NO_ERR) {
        /* error already recorded */
        return ERR_NCX_OPERATION_FAILED;
    }

    /* optional revision data string */
    revval = val_find_child(msg->rpc_input, AGT_SYS_MODULE, NCX_EL_REVISION);
    if (revval && revval->res != NO_ERR) {
        /* error already recorded */
        return ERR_NCX_OPERATION_FAILED;
    }

    /* check for any version of this module already loaded */
    mod = ncx_find_module(VAL_STR(modval), NULL);
    if (mod == NULL) {
        /* module not loaded already
         * load all the deviations first
         */
        for (devval = val_find_child(msg->rpc_input, AGT_SYS_MODULE,
                                     NCX_EL_DEVIATION);
             devval != NULL && res == NO_ERR;
             devval = val_find_next_child(msg->rpc_input, AGT_SYS_MODULE,
                                          NCX_EL_DEVIATION, devval)) {

            res = ncxmod_load_deviation(VAL_STR(devval),
                                        &agt_profile->agt_savedevQ);
            if (res != NO_ERR) {
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res,
                                 methnode, 
                                 NCX_NT_NONE, 
                                 NULL, 
                                 NCX_NT_VAL, 
                                 devval);
                errdone = TRUE;
            }
        }

        if (res == NO_ERR) {
#ifdef STATIC_SERVER
            res = ncxmod_load_module(VAL_STR(modval), 
                                     (revval) ? VAL_STR(revval) : NULL, 
                                     &agt_profile->agt_savedevQ,
                                     &mod);
#else
            res = agt_load_sil_code(VAL_STR(modval), 
                                    (revval) ? VAL_STR(revval) : NULL,
                                    TRUE);
            if (res == ERR_NCX_SKIPPED) {
                log_warn("\nWarning: SIL code for module '%s' not found",
                         VAL_STR(modval));
                res = ncxmod_load_module(VAL_STR(modval), 
                                         (revval) ? VAL_STR(revval) : NULL, 
                                         &agt_profile->agt_savedevQ,
                                         &mod);
            } else if (res == NO_ERR) {
                sil_loaded = TRUE;
            }
#endif

            /* reget the module; it should be found if status == NO_ERR */
            if (res == NO_ERR) {
                mod = ncx_find_module(VAL_STR(modval),
                                      (revval) ? VAL_STR(revval) : NULL);
                if (mod == NULL) {
                    res = SET_ERROR(ERR_INTERNAL_VAL);
                }
            }

            if (res == NO_ERR) {
                module_added = TRUE;
            } else {
                agt_record_error(scb, 
                                 &msg->mhdr, 
                                 NCX_LAYER_OPERATION, 
                                 res,
                                 methnode, 
                                 NCX_NT_NONE, 
                                 NULL, 
                                 NCX_NT_VAL, 
                                 modval);
                errdone = TRUE;
            }
        }
    } else if (revval != NULL) {
        /* some version of the module is already loaded
         * try again to get the exact version requested 
         */
        testmod = ncx_find_module(VAL_STR(modval), VAL_STR(revval));
        if (testmod != NULL) {
            mod = testmod;
        } else {
            res = ERR_NCX_WRONG_VERSION;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE, 
                             NULL, 
                             NCX_NT_VAL, 
                             revval);
            errdone = TRUE;
        }
    }

    /* generate the return value */
    if (res == NO_ERR && mod != NULL) {
        newval = val_make_string(val_get_nsid(modval),
                                 NCX_EL_MOD_REVISION,
                                 (mod->version) ? 
                                 mod->version : EMPTY_STRING);
        if (newval == NULL) {
            res = ERR_INTERNAL_MEM;
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE, 
                             NULL, 
                             NCX_NT_NONE, 
                             NULL);
            errdone = TRUE;
        }
    }

    if (res == NO_ERR && mod && module_added && !sil_loaded) {
        /* make sure any top-level defaults are set */
        res = agt_set_mod_defaults(mod);
    }

    if (res == NO_ERR && mod && module_added) {
        /* prune all the obsolete objects */
        ncx_delete_mod_obsolete_objects(mod);

        /* add the module commit tests for this module */
        res = agt_val_add_module_commit_tests(mod);
    }

    if (res == NO_ERR && mod && module_added) {
        /* add the <schema> node in netconf-state module */
        res = agt_state_add_module_schema(mod);
    }

    if (res == NO_ERR && mod && module_added) {
        /* add the <capability> node in <hello> template */
        res = agt_cap_add_module(mod);
    }

    if (res == NO_ERR && mod && module_added) {
        /* send the capability change notification */
        moduri = cap_make_moduri(mod);
        if (!moduri) {
            res = ERR_INTERNAL_MEM;
        } else {
            agt_sys_send_sysCapabilityChange(scb, TRUE, moduri);
            m__free(moduri);
        }
    }

    if (res != NO_ERR) {
        if (!errdone) {
            agt_record_error(scb, 
                             &msg->mhdr, 
                             NCX_LAYER_OPERATION, 
                             res,
                             methnode, 
                             NCX_NT_NONE, 
                             NULL, 
                             NCX_NT_VAL, 
                             modval);
        }
        if (newval != NULL) {
            val_free_value(newval);
        }
    } else {
        /* pass off newval memory here */
        if (newval != NULL) {
            msg->rpc_data_type = RPC_DATA_YANG;
            dlq_enque(newval, &msg->rpc_dataQ);
        }
    }

    return res;

} /* load_invoke */


/********************************************************************
* FUNCTION restart_invoke
*
* restart : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    restart_invoke (ses_cb_t *scb,
                    rpc_msg_t *msg,
                    xml_node_t *methnode)
{
    xmlChar   timebuff[TSTAMP_MIN_SIZE];

    (void)msg;
    (void)methnode;

    tstamp_datetime(timebuff);

    log_write("\n\n**************"
              "\nNotice: restart requested\n   by %s "
              "on session %u at %s\n\n",
              scb->username,
              scb->sid,
              timebuff);

    agt_request_shutdown(NCX_SHUT_RESTART);
    return NO_ERR;

} /* restart_invoke */


/********************************************************************
* FUNCTION shutdown_invoke
*
* shutdown : invoke callback
* 
* INPUTS:
*    see agt/agt_rpc.h
* RETURNS:
*    status
*********************************************************************/
static status_t 
    shutdown_invoke (ses_cb_t *scb,
                     rpc_msg_t *msg,
                     xml_node_t *methnode)
{
    xmlChar   timebuff[TSTAMP_MIN_SIZE];

    (void)msg;
    (void)methnode;

    tstamp_datetime(timebuff);

    log_write("\n\n*****************************"
              "\nNotice: shutdown requested\n    by %s "
              "on session %u at %s\n\n",
              scb->username,
              scb->sid,
              timebuff);

    agt_request_shutdown(NCX_SHUT_EXIT);
    return NO_ERR;

} /* shutdown_invoke */


/********************************************************************
* FUNCTION register_nc_callbacks
*
* Register the agent callback functions for the NETCONF RPC methods 
*
* RETURNS:
*    status, NO_ERR if all registered okay
*********************************************************************/
static status_t 
    register_nc_callbacks (void)
{
    status_t  res;

    /* get */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_GET),
                                  AGT_RPC_PH_VALIDATE,
                                  get_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }


    /* get-config */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_GET_CONFIG),
                                  AGT_RPC_PH_VALIDATE,
                                  get_config_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* edit-config */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_EDIT_CONFIG),
                                  AGT_RPC_PH_VALIDATE,
                                  edit_config_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_EDIT_CONFIG),
                                  AGT_RPC_PH_INVOKE,
                                  edit_config_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }


    /* copy-config */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_COPY_CONFIG),
                                  AGT_RPC_PH_VALIDATE,
                                  copy_config_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_COPY_CONFIG),
                                  AGT_RPC_PH_INVOKE,
                                  copy_config_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* delete-config */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_DELETE_CONFIG),
                                  AGT_RPC_PH_VALIDATE,
                                  delete_config_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_DELETE_CONFIG),
                                  AGT_RPC_PH_INVOKE,
                                  delete_config_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* lock */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_LOCK),
                                  AGT_RPC_PH_VALIDATE,
                                  lock_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_LOCK),
                                  AGT_RPC_PH_INVOKE,
                                  lock_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }


    /* unlock */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_UNLOCK),
                                  AGT_RPC_PH_VALIDATE,
                                  unlock_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_UNLOCK),
                                  AGT_RPC_PH_INVOKE,
                                  unlock_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* close-session
     * no validate for close-session 
     */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_CLOSE_SESSION),
                                  AGT_RPC_PH_INVOKE,
                                  close_session_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* kill-session */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_KILL_SESSION),
                                  AGT_RPC_PH_VALIDATE,
                                  kill_session_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_KILL_SESSION),
                                  AGT_RPC_PH_INVOKE,
                                  kill_session_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* validate :validate capability */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_VALIDATE),
                                  AGT_RPC_PH_VALIDATE,
                                  validate_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* commit :candidate capability */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_COMMIT),
                                  AGT_RPC_PH_VALIDATE,
                                  commit_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_COMMIT),
                                  AGT_RPC_PH_INVOKE,
                                  commit_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* discard-changes :candidate capability */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_DISCARD_CHANGES),
                                  AGT_RPC_PH_VALIDATE,
                                  discard_changes_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_DISCARD_CHANGES),
                                  AGT_RPC_PH_INVOKE,
                                  discard_changes_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* cancel-commit :confirmed-commit + :base:1.1 capability */
    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_CANCEL_COMMIT),
                                  AGT_RPC_PH_VALIDATE,
                                  cancel_commit_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE,
                                  op_method_name(OP_CANCEL_COMMIT),
                                  AGT_RPC_PH_INVOKE,
                                  cancel_commit_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* load-config extension */
    res = agt_rpc_register_method(NC_MODULE, 
                                  NCX_EL_LOAD_CONFIG,
                                  AGT_RPC_PH_VALIDATE, 
                                  load_config_validate);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    res = agt_rpc_register_method(NC_MODULE, 
                                  NCX_EL_LOAD_CONFIG,
                                  AGT_RPC_PH_INVOKE,
                                  load_config_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* load module extension */
    res = agt_rpc_register_method(AGT_SYS_MODULE, 
                                  NCX_EL_LOAD,
                                  AGT_RPC_PH_INVOKE,  
                                  load_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* restart extension */
    res = agt_rpc_register_method(AGT_SYS_MODULE, 
                                  NCX_EL_RESTART,
                                  AGT_RPC_PH_INVOKE,  
                                  restart_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* shutdown extension */
    res = agt_rpc_register_method(AGT_SYS_MODULE, 
                                  NCX_EL_SHUTDOWN,
                                  AGT_RPC_PH_INVOKE,  
                                  shutdown_invoke);
    if (res != NO_ERR) {
        return SET_ERROR(res);
    }

    /* no-op extension */
    agt_rpc_support_method(AGT_SYS_MODULE, NCX_EL_NO_OP);

    return NO_ERR;

} /* register_nc_callbacks */


/********************************************************************
* FUNCTION unregister_nc_callbacks
*
* Unregister the agent callback functions for the NETCONF RPC methods 
*
*********************************************************************/
static void
    unregister_nc_callbacks (void)
{
    /* get */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_GET));

    /* get-config */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_GET_CONFIG));

    /* edit-config */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_EDIT_CONFIG));

    /* copy-config */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_COPY_CONFIG));

    /* delete-config */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_DELETE_CONFIG));

    /* lock */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_LOCK));

    /* unlock */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_UNLOCK));

    /* close-session */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_CLOSE_SESSION));

    /* kill-session */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_KILL_SESSION));

    /* validate */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_VALIDATE));

    /* commit */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_COMMIT));

    /* discard-changes */
    agt_rpc_unregister_method(NC_MODULE, 
                              op_method_name(OP_DISCARD_CHANGES));

    /* cancel-commit (base:1.1 only) */
    agt_rpc_unregister_method(NC_MODULE, NCX_EL_CANCEL_COMMIT);

    /* load-config extension */
    agt_rpc_unregister_method(NC_MODULE, NCX_EL_LOAD_CONFIG);

    /* load module extension */
    agt_rpc_unregister_method(AGT_SYS_MODULE, NCX_EL_LOAD);

    /* restart extension */
    agt_rpc_unregister_method(AGT_SYS_MODULE, NCX_EL_RESTART);

    /* shutdown extension */
    agt_rpc_unregister_method(AGT_SYS_MODULE, NCX_EL_SHUTDOWN);

    /* no-op extension */
    agt_rpc_unregister_method(AGT_SYS_MODULE, NCX_EL_NO_OP);

} /* unregister_nc_callbacks */




/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION agt_ncx_init
* 
* Initialize the NCX Agent standard method routines
* 
* RETURNS:
*   status of the initialization procedure
*********************************************************************/
status_t 
    agt_ncx_init (void)
{
    status_t  res;

    if (!agt_ncx_init_done) {

        res = register_nc_callbacks();
        if (res != NO_ERR) {
            unregister_nc_callbacks();
            return res;
        }

        memset(&commit_cb, 0x0, sizeof(commit_cb_t));

        agt_ncx_init_done = TRUE;
    }
    return NO_ERR;

}  /* agt_ncx_init */


/********************************************************************
* FUNCTION agt_ncx_cleanup
*
* Cleanup the NCX Agent standard method routines
* 
* TBD -- put platform-specific agent cleanup here
*
*********************************************************************/
void
    agt_ncx_cleanup (void)
{
    if (agt_ncx_init_done) {

        unregister_nc_callbacks();

        clear_commit_cb();

        agt_ncx_init_done = FALSE;
    }
}   /* agt_ncx_cleanup */


/********************************************************************
* FUNCTION agt_ncx_cfg_load
*
* Load the specifed config from the indicated source
* Called just once from agt.c at boot or reload time!
*
* This function should only be used to load an empty config
* in CFG_ST_INIT state
*
* INPUTS:
*    cfg = Config template to load data into
*    cfgloc == enum for the config source location
*    cfgparm == string parameter used in different ways 
*               depending on the cfgloc value
*     For cfgloc==CFG_LOC_FILE, this is a system-dependent filespec
* 
* OUTPUTS:
*    errQ contains any rpc_err_rec_t structs (if non-NULL)
*
* RETURNS:
*    overall status; may be the last of multiple error conditions
*********************************************************************/
status_t
    agt_ncx_cfg_load (cfg_template_t *cfg,
                      cfg_location_t cfgloc,
                      const xmlChar *cfgparm)
{
    cfg_template_t  *startup;
    val_value_t     *copystartup;
    status_t         res;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (cfg->cfg_state != CFG_ST_INIT) {
        return SET_ERROR(ERR_NCX_CFG_STATE);
    }
#endif

    startup = NULL;
    copystartup = NULL;

    cfg->cfg_loc = cfgloc;
    if (cfgparm) {
        cfg->src_url = xml_strdup(cfgparm);
        if (!cfg->src_url) {
            return ERR_INTERNAL_MEM;
        }
    }

    res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    switch (cfgloc) {
    case CFG_LOC_INTERNAL:
        break;
    case CFG_LOC_FILE:
        if (!cfg->src_url) {
            res = ERR_INTERNAL_MEM;
        } else {
            /* the cfgparm should be a filespec of an XML config file */
            res = agt_rpc_load_config_file(cfgparm, cfg, TRUE, 0);
            if (res == NO_ERR && 
                cfg->root != NULL &&
                cfg->cfg_id != NCX_CFGID_STARTUP) {
                startup = cfg_get_config_id(NCX_CFGID_STARTUP);
                if (startup != NULL) {
                    copystartup = val_clone(cfg->root);
                    if (copystartup == NULL) {
                        log_error("\nError: create <startup> config failed");
                    } else {
                        if (startup->root != NULL) {
                            val_free_value(startup->root);
                        }
                        startup->root = copystartup;
                        copystartup = NULL;
                    }
                }
            }
        }
        break;
    case CFG_LOC_NAMED:
        break;
    case CFG_LOC_LOCAL_URL:
        break;
    case CFG_LOC_REMOTE_URL:
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* agt_ncx_cfg_load */


/********************************************************************
* FUNCTION agt_ncx_cfg_save
*
* Save the specified cfg to the its startup source, which should
* be stored in the cfg struct
*
* INPUTS:
*    cfg  = Config template to save from
*    bkup = TRUE if the current startup config should
*           be saved before it is overwritten
*         = FALSE to just overwrite the old startup cfg
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_ncx_cfg_save (cfg_template_t *cfg,
                      boolean bkup)
{
    cfg_template_t    *startup;
    val_value_t       *copystartup;
    xmlChar           *filebuffer;
    agt_profile_t     *profile;
    status_t           res;
    xml_attrs_t        attrs;

#ifdef DEBUG
    if (!cfg) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!cfg->root) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    filebuffer = NULL;
    startup = NULL;
    copystartup = NULL;
    res = ERR_NCX_OPERATION_NOT_SUPPORTED;
    profile = agt_get_profile();

    switch (cfg->cfg_loc) {
    case CFG_LOC_INTERNAL:
        break;
    case CFG_LOC_NONE:   /* candidate config */
    case CFG_LOC_FILE:
        if (bkup) {
            /* FIXME: remove any existing backup */
            /****/

            /* FIXME: rename the current startup to the backup */
            /****/
        } 

        /* save the new startup database, if there is one */
        res = NO_ERR;
        startup = cfg_get_config_id(NCX_CFGID_STARTUP);
        if (startup != NULL) {
            copystartup = val_clone_config_data(cfg->root, &res);
            if (copystartup == NULL) {
                return res;
            }
        }

        if (res == NO_ERR) {
            filebuffer = agt_get_startup_filespec(&res);
            if (filebuffer != NULL && res == NO_ERR) {
                if (LOGDEBUG) {
                    log_debug("\nWriting <%s> config to file '%s'",
                              cfg->name,
                              filebuffer);
                }
                /* write the new startup config */
                xml_init_attrs(&attrs);

                /* output to the specified file or STDOUT */
                res = xml_wr_check_file(filebuffer,
                                        cfg->root,
                                        &attrs,
                                        XMLMODE,
                                        WITHHDR,
                                        TRUE,
                                        0,
                                        profile->agt_indent,
                                        agt_check_save);

                xml_clean_attrs(&attrs);

                if (res == NO_ERR && startup != NULL) {
                    /* toss the old startup and save the new one */
                    if (startup->root) {
                        val_free_value(startup->root);
                    }
                    startup->root = copystartup;
                    copystartup = NULL;
                    cfg_update_last_ch_time(startup);
                }
            }
        }
        break;
    case CFG_LOC_NAMED:
        break;
    case CFG_LOC_LOCAL_URL:
        break;
    case CFG_LOC_REMOTE_URL:
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (copystartup) {
        val_free_value(copystartup);
    }

    if (filebuffer) {
        m__free(filebuffer);
    }

    return res;

} /* agt_ncx_cfg_save */


/********************************************************************
* FUNCTION agt_ncx_load_backup
*
* Load a backup config into the specified config template
*
* INPUTS:
*    filespec == complete path for the input file
*    cfg == config template to load
*    use_sid == session ID of user to use
*
* RETURNS:
*    status
*********************************************************************/
status_t
    agt_ncx_load_backup (const xmlChar *filespec,
                         cfg_template_t *cfg,
                         ses_id_t  use_sid)
{
    status_t           res;

    res = agt_rpc_load_config_file(filespec, 
                                   cfg, 
                                   FALSE, /* OP_EDITOP_REPLACE */
                                   use_sid);
    return res;

} /* agt_ncx_load_backup */


/********************************************************************
* FUNCTION agt_ncx_cc_active
*
* Check if a confirmed-commit is active, and the timeout
* may need to be processed
*
* RETURNS:
*    TRUE if confirmed-commit is active
*    FALSE otherwise
*********************************************************************/
boolean
    agt_ncx_cc_active (void)
{

    return commit_cb.cc_active;

} /* agt_ncx_cc_active */


/********************************************************************
* FUNCTION agt_ncx_cc_ses_id
*
* Get the confirmed commit session ID
*
* RETURNS:
*    session ID for the confirmed commit
*********************************************************************/
ses_id_t
    agt_ncx_cc_ses_id (void)
{

    return commit_cb.cc_ses_id;

} /* agt_ncx_cc_ses_id */


/********************************************************************
* FUNCTION agt_ncx_clear_cc_ses_id
*
* Clear the confirmed commit session ID
* This will be called by agt_ses when the current
* session exits during a persistent confirmed-commit
*
*********************************************************************/
void
    agt_ncx_clear_cc_ses_id (void)
{

    commit_cb.cc_ses_id = 0;

} /* agt_ncx_clear_cc_ses_id */


/********************************************************************
* FUNCTION agt_ncx_cc_persist_id
*
* Get the confirmed commit persist ID
*
* RETURNS:
*    session ID for the confirmed commit
*********************************************************************/
const xmlChar *
    agt_ncx_cc_persist_id (void)
{

    return commit_cb.cc_persist_id;

} /* agt_ncx_cc_persist_id */


/********************************************************************
* FUNCTION agt_ncx_check_cc_timeout
*
* Check if a confirmed-commit has timed out, and needs to be canceled
*
*********************************************************************/
void
    agt_ncx_check_cc_timeout (void)
{

    time_t            timenow;
    double            timediff;

    if (!commit_cb.cc_active) {
        return;
    }

    (void)time(&timenow);
    timediff = difftime(timenow, commit_cb.cc_start_time);

    if (timediff >= (double)commit_cb.cc_cancel_timeout) {
        if (LOGDEBUG) {
            log_debug("\nConfirmed-commit timeout");
        }
        agt_ncx_cancel_confirmed_commit(NULL, NCX_CC_EVENT_TIMEOUT);
    }

} /* agt_ncx_check_cc_timeout */


/********************************************************************
* FUNCTION agt_ncx_cancel_confirmed_commit
*
* Cancel the confirmed-commit in progress and rollback
* to the backup-cfg.xml file
*
* INPUTS:
*   scb == session control block making this change, may be NULL
*   event == confirmEvent enumeration value to use
*
*********************************************************************/
void
    agt_ncx_cancel_confirmed_commit (ses_cb_t  *scb,
                                     ncx_confirm_event_t event)
{
    cfg_template_t  *running;
    status_t         res;

    if (!commit_cb.cc_active) {
        return;
    }

    running = cfg_get_config_id(NCX_CFGID_RUNNING);

    if (LOGDEBUG) {
        log_debug("\nConfirmed-commit canceled");
    }

    /* restore the config if needed
     * restore as the system user, not any specific user
     * to make sure that all the rollback edits will succeed  */
    res = agt_ncx_load_backup(commit_cb.cc_backup_source, running, 0);
    if (res != NO_ERR) {
        log_error("\nError: restore running config failed (%s)",
                  get_error_string(res));

    }

    if (res == NO_ERR) {
        res = cfg_fill_candidate_from_running();
        if (res != NO_ERR) {
            log_error("\nError: resynch candidate after restore "
                      "running config failed (%s)",
                      get_error_string(res));
        }
    }

    agt_sys_send_sysConfirmedCommit(scb, event);

    clear_commit_cb();

} /* agt_ncx_cancel_confirmed_commit */



/* END file agt_ncx.c */
