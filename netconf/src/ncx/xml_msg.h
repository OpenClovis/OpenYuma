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
#ifndef _H_xml_msg
#define _H_xml_msg
/*  FILE: xml_msg.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   XML Message send and receive

   Deals with generic namespace and xmlns optimization
   and tries to keep changing the default namespace so
   most nested elements do not have prefixes

   Deals with the NETCONF requirement that the attributes
   in <rpc> are returned in <rpc-reply> unchanged.  Although
   XML allows the xnmlns prefixes to change, the same prefixes
   are used in the <rpc-reply> that the NMS provided in the <rpc>.

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
14-jan-07    abb      Begun; split from agt_rpc.h
*/

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                         T Y P E S                                 *
*                                                                   *
*********************************************************************/

/* Common XML Message Header */
typedef struct xml_msg_hdr_t_ {
    /* incoming: 
     * All the namespace decls that were in the <rpc>
     * request are used in the <rpc-reply>, so the same prefixes
     * will be used, and the XML on the wire will be easier to debug
     * by examining packet traces
     *
     * if useprefix=TRUE then prefixes will be used for
     * element start and end tags. FALSE then default NS
     * will be used so no prefixes will be needed except
     * for XML content
     */
    boolean           useprefix;
    ncx_withdefaults_t withdef;       /* with-defaults value */
    dlq_hdr_t       prefixQ;             /* Q of xmlns_pmap_t */
    dlq_hdr_t       errQ;               /* Q of rpc_err_rec_t */

    /* agent access control for database reads and writes;
     * !!! shadow pointer to per-session cache, not malloced
     */
    struct agt_acm_cache_t_ *acm_cache;

    /* agent access control read authorization 
     * callback function: xml_msg_authfn_t
     */
    void                    *acm_cbfn;

} xml_msg_hdr_t;


/* read authorization callback template */
typedef boolean (*xml_msg_authfn_t) (xml_msg_hdr_t *msg,
				     const xmlChar *username,
				     const val_value_t *val);


				      
/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION xml_msg_init_hdr
*
* Initialize a new xml_msg_hdr_t struct
*
* INPUTS:
*   msg == xml_msg_hdr_t memory to initialize
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_msg_init_hdr (xml_msg_hdr_t *msg);


/********************************************************************
* FUNCTION xml_msg_clean_hdr
*
* Clean all the memory used by the specified xml_msg_hdr_t
* but do not free the struct itself
*
* INPUTS:
*   msg == xml_msg_hdr_t to clean
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_msg_clean_hdr (xml_msg_hdr_t *msg);


/********************************************************************
* FUNCTION xml_msg_get_prefix
*
* Find the namespace prefix for the specified namespace ID
* If it is not there then create one
*
* INPUTS:
*    msg  == message to search
*    parent_nsid == parent namespace ID
*    nsid == namespace ID to find
*    curelem == value node for current element if available
*    xneeded == pointer to xmlns needed flag output value
*
* OUTPUTS:
*   *xneeded == TRUE if the prefix is new and an xmlns
*               decl is needed in the element being generated
*
* RETURNS:
*   pointer to prefix if found, else NULL if not found
*********************************************************************/
extern const xmlChar *
    xml_msg_get_prefix (xml_msg_hdr_t *msg,
			xmlns_id_t parent_nsid,
			xmlns_id_t nsid,
			val_value_t *curelem,
			boolean  *xneeded);


/********************************************************************
* FUNCTION xml_msg_get_prefix_xpath
*
* Find the namespace prefix for the specified namespace ID
* If it is not there then create one in the msg prefix map
* Always returns a prefix, instead of using a default
*
* creates a new pfixmap if needed
*
* !!! MUST BE CALLED BEFORE THE <rpc-reply> XML OUTPUT
* !!! HAS BEGUN.  CANNOT BE CALLED BY OUTPUT FUNCTIONS
* !!! DURING THE <get> OR <get-config> OUTPUT GENERATION
*
* INPUTS:
*    msg  == message to search
*    nsid == namespace ID to find
*
* RETURNS:
*   pointer to prefix if found, else NULL if not found
*********************************************************************/
extern const xmlChar *
    xml_msg_get_prefix_xpath (xml_msg_hdr_t *msg,
			      xmlns_id_t nsid);


/********************************************************************
* FUNCTION xml_msg_get_prefix_start_tag
*
* Find the namespace prefix for the specified namespace ID
* DO NOT CREATE A NEW PREFIX MAP IF IT IS NOT THERE
* does not create any pfixmap, just returns NULL if not found
*
* INPUTS:
*    msg  == message to search
*    nsid == namespace ID to find
*
* RETURNS:
*   pointer to prefix if found, else NULL if not found
*********************************************************************/
extern const xmlChar *
    xml_msg_get_prefix_start_tag (xml_msg_hdr_t *msg,
				  xmlns_id_t nsid);


/********************************************************************
* FUNCTION xml_msg_gen_new_prefix
*
* Generate a new namespace prefix
* 
* INPUTS:
*    msg  == message to search and generate a prefix for
*    nsid == namespace ID to generate prefix for
*    retbuff == address of return buffer
*    buffsize == buffer size
* OUTPUTS:
*   if *retbuff is NULL it will be created
*   else *retbuff is filled in with the new prefix if NO_ERR
*
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    xml_msg_gen_new_prefix (xml_msg_hdr_t *msg,
			    xmlns_id_t  nsid,
			    xmlChar **retbuff,
			    uint32 buffsize);


/********************************************************************
* FUNCTION xml_msg_build_prefix_map
*
* Build a queue of xmlns_pmap_t records for the current message
* 
* INPUTS:
*    msg == message in progrss
*    attrs == the top-level attrs list (e;g, rpc_in_attrs)
*    addncid == TRUE if a prefix entry for the NC namespace
*                should be added
*            == FALSE if the NC nsid should not be added
*    addncxid == TRUE if a prefix entry for the NCX namespace
*                should be added
*             == FALSE if the NCX nsid should not be added
* OUTPUTS:
*   msg->prefixQ will be populated as needed,
*   could be partially populated if some error returned
*
*   XMLNS Entries for NETCONF and NCX will be added if they 
*   are not present
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_msg_build_prefix_map (xml_msg_hdr_t *msg,
			      xml_attrs_t *attrs,
			      boolean addncid,
			      boolean addncxid);


/********************************************************************
* FUNCTION xml_msg_finish_prefix_map
*
* Finish the queue of xmlns_pmap_t records for the current message
* 
* INPUTS:
*    msg == message in progrss
*    attrs == the top-level attrs list (e;g, rpc_in_attrs)
* OUTPUTS:
*   msg->prefixQ will be populated as needed,
*   could be partially populated if some error returned
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_msg_finish_prefix_map (xml_msg_hdr_t *msg,
                               xml_attrs_t *attrs);


/********************************************************************
* FUNCTION xml_msg_check_xmlns_attr
*
* Check the default NS and the prefix map in the msg;
* 
* INPUTS:
*    msg == message in progress
*    nsid == namespace ID to check
*    badns == namespace URI of the bad namespace
*             used if the nsid is the INVALID marker
*    attrs == Q to hold the xml_attr_t, if generated
*
* OUTPUTS:
*   msg->prefixQ will be populated as needed,
*   could be partially populated if some error returned
*  
*   XMLNS attr entry may be added to the attrs Q
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_msg_check_xmlns_attr (xml_msg_hdr_t *msg, 
			      xmlns_id_t nsid,
			      const xmlChar *badns,
			      xml_attrs_t  *attrs);


/********************************************************************
* FUNCTION xml_msg_gen_xmlns_attrs
*
* Generate any xmlns directives in the top-level
* attribute Q
*
* INPUTS:
*    msg == message in progress
*    attrs == xmlns_attrs_t Q to process
*    addncx == TRUE if an xmlns for the NCX prefix (for errors)
*              should be added to the <rpc-reply> element
*              FALSE if not
*
* OUTPUTS:
*   *attrs will be populated as needed,
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_msg_gen_xmlns_attrs (xml_msg_hdr_t *msg, 
			     xml_attrs_t *attrs,
                             boolean addncx);


/********************************************************************
* FUNCTION xml_msg_clean_defns_attr
*
* Get rid of an xmlns=foo default attribute
*
* INPUTS:
*    attrs == xmlns_attrs_t Q to process
*
* OUTPUTS:
*   *attrs will be cleaned as needed,
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_msg_clean_defns_attr (xml_attrs_t *attrs);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_xml_msg */
