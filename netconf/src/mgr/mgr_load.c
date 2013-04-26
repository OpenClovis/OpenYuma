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
/*  FILE: mgr_load.c

   Load Data File into an object template

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
27jun11      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/

#include <xmlstring.h>

#include "procdefs.h"
#include "log.h"
#include "mgr.h"
#include "mgr_load.h"
#include "mgr_ses.h"
#include "mgr_val_parse.h"
#include "ses.h"
#include "status.h"
#include "xml_util.h"


/********************************************************************
* FUNCTION mgr_load_extern_file
*
* Read an external file and convert it into a real data structure
*
* INPUTS:
*   filespec == XML filespec to load
*   targetobj == template to use to validate XML and encode as this object
*                !! must not be ncx:root!!
*             == NULL to use OBJ_TYP_ANYXML as the target object template
*   res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*     malloced val_value_t struct filled in with the contents
*********************************************************************/
val_value_t *
    mgr_load_extern_file (const xmlChar *filespec,
                          obj_template_t *targetobj,
                          status_t *res)
{
    ses_cb_t        *scb;
    val_value_t     *topval;
    boolean          isany;

    *res = NO_ERR;
    isany = FALSE;
    topval = NULL;

    /* only supporting parsing of XML files for now */
    if (!yang_fileext_is_xml(filespec)) {
        *res = ERR_NCX_OPERATION_NOT_SUPPORTED;
        return NULL;
    }

    /* create a dummy session control block */
    scb = mgr_ses_new_dummy_session();
    if (scb == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (targetobj == NULL) {
        isany = TRUE;
        targetobj = ncx_get_gen_anyxml();
    }

    if (LOGDEBUG2) {
        log_debug2("\nLoading extern XML file as '%s'",
                   filespec,
                   (isany) ? NCX_EL_ANYXML : obj_get_name(targetobj));
    }

    /* setup the config file as the xmlTextReader input */
    *res = xml_get_reader_from_filespec((const char *)filespec, 
                                        &scb->reader);
    if (*res == NO_ERR) {
        topval = val_new_value();
        if (topval == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            /* do not initialize the new value! */
            *res = mgr_val_parse(scb, targetobj, NULL, topval);
        }
    }

    mgr_ses_free_dummy_session(scb);

    return topval;

} /* mgr_load_extern_file */


/* END file mgr_load.c */
