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
/*  FILE: mgr_cap.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
03feb06      abb      begun; split out from base/cap.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_mgr_cap
#include "mgr_cap.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xml_val
#include  "xml_val.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define MGR_CAP_DEBUG 1 */


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static val_value_t   *mgr_caps = NULL;
static cap_list_t    *my_mgr_caps = NULL;


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION mgr_cap_cleanup
*
* Clean the NETCONF manager capabilities
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
void 
    mgr_cap_cleanup (void)
{
    if (mgr_caps) {
        val_free_value(mgr_caps);
        mgr_caps = NULL;
    }

    if (my_mgr_caps) {
        cap_clean_caplist(my_mgr_caps);
        m__free(my_mgr_caps);
        my_mgr_caps = NULL;
    }

} /* mgr_cap_cleanup */


/********************************************************************
* FUNCTION mgr_cap_set_caps
*
* Initialize the NETCONF manager capabilities
*
* TEMP !!! JUST SET EVERYTHING, WILL REALLY GET FROM MGR STARTUP
*
* INPUTS:
*    none
* RETURNS:
*    NO_ERR if all goes well
*********************************************************************/
status_t 
    mgr_cap_set_caps (void)
{
    val_value_t *oldcaps, *newcaps;
    cap_list_t *oldmycaps,*newmycaps;
    xmlns_id_t  nc_id;
    status_t  res;

    res = NO_ERR;
    newcaps = NULL;
    nc_id = xmlns_nc_id();
    oldcaps = mgr_caps;
    oldmycaps = my_mgr_caps;

    /* get a new cap_list */
    newmycaps = cap_new_caplist();
    if (!newmycaps) {
        res = ERR_INTERNAL_MEM;
    }

    /* get a new val_value_t cap list for manager <hello> messages */
    if (res == NO_ERR) {
        newcaps = xml_val_new_struct(NCX_EL_CAPABILITIES, nc_id);
        if (!newcaps) {
            res = ERR_INTERNAL_MEM;
        }
    }

    /* add capability for NETCONF version 1.0 support */
    if (res == NO_ERR) {
        res = cap_add_std(newmycaps, CAP_STDID_V1);
        if (res == NO_ERR) {
            res = cap_add_stdval(newcaps, CAP_STDID_V1);
        }
    }

    /* check the return value */
    if (res != NO_ERR) {
        /* toss the new, put back the old */
        cap_free_caplist(newmycaps);
        val_free_value(newcaps);
        my_mgr_caps = oldmycaps;
        mgr_caps = oldcaps;
    } else {
        /* toss the old, install the new */
        if (oldmycaps) {
            cap_free_caplist(oldmycaps);
        }
        if (oldcaps) {
            val_free_value(oldcaps);
        }
        my_mgr_caps = newmycaps;
        mgr_caps = newcaps;
    }   

    return res;

} /* mgr_cap_set_caps */


/********************************************************************
* FUNCTION mgr_cap_get_caps
*
* Get the NETCONF manager capabilities
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the manager caps list
*********************************************************************/
cap_list_t * 
    mgr_cap_get_caps (void)
{
    return my_mgr_caps;
} /* mgr_cap_get_caps */


/********************************************************************
* FUNCTION mgr_cap_get_capsval
*
* Get the NETCONF manager capabilities ain val_value_t format
*
* INPUTS:
*    none
* RETURNS:
*    pointer to the manager caps list
*********************************************************************/
val_value_t * 
    mgr_cap_get_capsval (void)
{
    return mgr_caps;
} /* mgr_cap_get_capsval */


/********************************************************************
* FUNCTION mgr_cap_get_ses_capsval
*
* Get the NETCONF manager capabilities ain val_value_t format
* for a specific session, v2 supports base1.0 and/or base1.1
* INPUTS:
*    scb == session control block to use
* RETURNS:
*    MALLOCED pointer to the manager caps list to use
*    and then discard with val_free_value
*********************************************************************/
val_value_t * 
    mgr_cap_get_ses_capsval (ses_cb_t *scb)
{
    val_value_t *newcaps;
    xmlns_id_t  nc_id;
    status_t    res;

    nc_id = xmlns_nc_id();
    res = NO_ERR;

    /* get a new val_value_t cap list for manager <hello> messages */
    newcaps = xml_val_new_struct(NCX_EL_CAPABILITIES, nc_id);
    if (newcaps == NULL) {
        return NULL;
    }

    /* add capability for NETCONF version 1.0 support */
    if (ses_protocol_requested(scb, NCX_PROTO_NETCONF10)) {
        res = cap_add_stdval(newcaps, CAP_STDID_V1);
    }
    /* add capability for NETCONF version 1.1 support */
    if (res == NO_ERR &&
        ses_protocol_requested(scb, NCX_PROTO_NETCONF11)) {
        res = cap_add_stdval(newcaps, CAP_STDID_V11);
    }

    /* check the return value */
    if (res != NO_ERR) {
        val_free_value(newcaps);
        newcaps = NULL;
    }   

    return newcaps;;


} /* mgr_cap_get_ses_capsval */



/* END file mgr_cap.c */
