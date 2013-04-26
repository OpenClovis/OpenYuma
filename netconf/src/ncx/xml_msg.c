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
/*  FILE: xml_msg.c

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
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
14jan07      abb      begun; split from agt_rpc.c


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>
#include  <assert.h>

#include  "procdefs.h"
#include  "def_reg.h"
#include  "dlq.h"
#include  "ncx.h"
#include  "ncxconst.h"
#include  "rpc_err.h"
#include  "status.h"
#include  "xmlns.h"
#include  "xml_msg.h"
#include  "xml_util.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define XML_MSG_DEBUG 1 */

#define MAX_PREFIX_TRIES   26


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


/********************************************************************
* FUNCTION add_pmap
*
* Add a prefix mapping entry
* 
* INPUTS:
*    msg  == message to search
*    newpmap == xmlns_pmap_t struct to add
*
* RETURNS:
*   none
*********************************************************************/
static void
    add_pmap (xml_msg_hdr_t *msg,
              xmlns_pmap_t *newpmap)
{
    xmlns_pmap_t  *pmap;

    /* add the new prefix mapping */
    for (pmap = (xmlns_pmap_t *)dlq_firstEntry(&msg->prefixQ);
         pmap != NULL;
         pmap = (xmlns_pmap_t *)dlq_nextEntry(pmap)) {
        if (newpmap->nm_id < pmap->nm_id) {
            dlq_insertAhead(newpmap, pmap);
            return;
        }
    }
    dlq_enque(newpmap, &msg->prefixQ);

}  /* add_pmap */


/********************************************************************
* FUNCTION find_pmap
*
* Find the pmap entry for the specified namespace ID
* 
* INPUTS:
*    msg  == message to search
*    nsid == namespace ID to find
*
* RETURNS:
*   pointer to prefix if found, else NULL if not found
*********************************************************************/
static xmlns_pmap_t *
    find_pmap (xml_msg_hdr_t *msg,
               xmlns_id_t nsid)
{
    xmlns_pmap_t  *pmap;

    for (pmap = (xmlns_pmap_t *)dlq_firstEntry(&msg->prefixQ);
         pmap != NULL;
         pmap = (xmlns_pmap_t *)dlq_nextEntry(pmap)) {

        if (pmap->nm_id == nsid) {
            return pmap;
        } else if (pmap->nm_id > nsid) {
            return NULL;
        }
    }
    return NULL;

}  /* find_pmap */


/********************************************************************
* FUNCTION find_prefix
*
* Find the namespace prefix for the specified namespace ID
* 
* INPUTS:
*    msg  == message to search
*    nsid == namespace ID to find
*
* RETURNS:
*   pointer to prefix if found, else NULL if not found
*********************************************************************/
static const xmlChar *
    find_prefix (xml_msg_hdr_t *msg,
                 xmlns_id_t nsid)
{
    const xmlns_pmap_t  *pmap;

    for (pmap = (const xmlns_pmap_t *)dlq_firstEntry(&msg->prefixQ);
         pmap != NULL;
         pmap = (const xmlns_pmap_t *)dlq_nextEntry(pmap)) {

        if (pmap->nm_id == nsid) {
            return (const xmlChar *)pmap->nm_pfix;
        } else if (pmap->nm_id > nsid) {
            return NULL;
        }
    }
    return NULL;

}  /* find_prefix */


/********************************************************************
* FUNCTION find_prefix_val
*
* Find the namespace prefix for the specified namespace ID
* by the prefix value itself
*
* INPUTS:
*    msg  == message to search
*    pfix == prefix to check for
*
* RETURNS:
*   namespace ID in the mapping or 0 if not found
*********************************************************************/
static xmlns_id_t
    find_prefix_val (xml_msg_hdr_t *msg,
                     const xmlChar *pfix)
{
    const xmlns_pmap_t  *pmap;

    for (pmap = (const xmlns_pmap_t *)dlq_firstEntry(&msg->prefixQ);
         pmap != NULL;
         pmap = (const xmlns_pmap_t *)dlq_nextEntry(pmap)) {

        if (!xml_strcmp(pmap->nm_pfix, pfix)) {
            return pmap->nm_id;
        } 
    }
    return XMLNS_NULL_NS_ID;

}  /* find_prefix_val */


/********************************************************************
* FUNCTION xmlns_needed
*
* Check if an xmlns directive is needed for the specified
* element node
*
* INPUTS:
*    curelem  == value node for the current element
*    nsid == namespace ID to check
*
* RETURNS:
*   TRUE if an xmlns directive is needed
*   FALSE if not (already generated in this XML path to toor)
*********************************************************************/
static boolean
    xmlns_needed (val_value_t *curelem,
                  xmlns_id_t nsid)
{

    val_value_t  *metaval;

    /* check if the XMLNS directive already present
     * in this node due to an attribute using the NS ID
     */
    for (metaval = val_get_first_meta_val(curelem);
         metaval != NULL;
         metaval = val_get_next_meta(metaval)) {
        if (metaval->nsid == nsid) {
            return FALSE;
        }
    }

    /* last chance, check the parent */
    if (curelem->parent) {
        if (curelem->parent->nsid == nsid) {
            return FALSE;
        } else {
            return xmlns_needed(curelem->parent, nsid);
        }
    } else {
        /* the NS ID was not found anywhere in the path to root
         * so an XMLNS directive is needed now
         */
        return TRUE;
    }

}  /* xmlns_needed */

/********************************************************************
 * build a prefix map using the xmplns directives in a message.
 * 
 * \param msg the message in progrss
 * \param attrs the the top-level attrs list (e;g, rpc_in_attrs)
 * \return status
 *********************************************************************/
static status_t build_prefix_map_from_xmlns_directives( xml_msg_hdr_t* msg, 
                                                         xml_attrs_t* attrs )
{
    xmlns_id_t invid = xmlns_inv_id();
    xml_attr_t *attr = (xml_attr_t *)xml_first_attr(attrs);

    for ( ; attr;  attr = (xml_attr_t *)xml_next_attr(attr)) {

        /* make sure this is an XMLNS attribute with or wo a prefix */
        if (xml_strncmp(XMLNS, attr->attr_qname, XMLNS_LEN)) {
            continue;
        }

        /* find the namespace associated with the prefix 
           note: it is not an error to have extra xmlns decls in the <rpc>elem;
           still need to make sure not to reuse the prefix anyway */
        xmlns_t *nsrec = def_reg_find_ns(attr->attr_val);

        /* check if this attribute has a prefix */
        if (attr->attr_qname == attr->attr_name) {
            /* no prefix in the name so this must be the default namespace */
            if ( !nsrec ) {
                /* the default namespace is not one of ours, so it will not be 
                 * used in the reply */
                attr->attr_xmlns_ns = invid;
            } else {
                attr->attr_xmlns_ns = nsrec->ns_id;
            }
            continue;
        }

        /* there is a prefix, so get the prefix len. 
         * The entire prefix was saved as the attr_name */
        uint32 plen = xml_strlen(attr->attr_name);

        /* get a new prefix map */
        xmlns_pmap_t *newpmap = xmlns_new_pmap(plen+1);
        if (!newpmap) {
            return ERR_INTERNAL_MEM;
        }

        /* save the prefix and the xmlns ID */
        xml_strncpy(newpmap->nm_pfix, attr->attr_name, plen);
        if ( !nsrec ) {
            newpmap->nm_id = invid;
            attr->attr_xmlns_ns = invid;
        } else {
            newpmap->nm_id = nsrec->ns_id;
            attr->attr_xmlns_ns = nsrec->ns_id;
        }
        newpmap->nm_topattr = TRUE;
        add_pmap(msg, newpmap);
    }

    return NO_ERR;
}

/********************************************************************
 * Add an ncid or ncxid to a prefix map.
 * 
 * \param msg the message in progrss
 * \param attrs the the top-level attrs list (e;g, rpc_in_attrs)
 * \param ncid the ncid to add.
 * \return status
 *********************************************************************/
static status_t add_ncid_to_prefix_map( xml_msg_hdr_t* msg, 
                                        xml_attrs_t* attrs,
                                        xmlns_id_t ncid )
{
    status_t res = NO_ERR;

    if (!find_prefix(msg, ncid) ) {
        /* add a prefix an xmlns attr for NETCONF */
        xmlChar *buff = NULL;
        res = xml_msg_gen_new_prefix(msg, ncid, &buff, 0);
        if (res != NO_ERR) {
            m__free( buff );
            return res;
        }

        res = xml_add_xmlns_attr(attrs, ncid, buff);
        if (res != NO_ERR) {
            m__free( buff );
            return res;
        }

        /* create a new prefix map */
        xmlns_pmap_t *newpmap = xmlns_new_pmap(0);
        if (!newpmap) {
            m__free( buff );
            return ERR_INTERNAL_MEM;
        } 
        
        newpmap->nm_id = ncid;
        newpmap->nm_pfix = buff;
        newpmap->nm_topattr = TRUE;
        add_pmap(msg, newpmap);
    }

    return res;
}

/************** E X T E R N A L   F U N C T I O N S  ***************/


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
void
    xml_msg_init_hdr (xml_msg_hdr_t *msg)
{
#ifdef DEBUG
    if (!msg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(msg, 0x0, sizeof(xml_msg_hdr_t));
    dlq_createSQue(&msg->prefixQ);
    dlq_createSQue(&msg->errQ);
    msg->withdef = NCX_DEF_WITHDEF;
    msg->useprefix = ncx_get_useprefix();

} /* xml_msg_init_hdr */


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
void 
    xml_msg_clean_hdr (xml_msg_hdr_t *msg)
{
    xmlns_pmap_t   *pmap;

#ifdef DEBUG
    if (!msg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* clean prefix queue */
    while (!dlq_empty(&msg->prefixQ)) {
        pmap = (xmlns_pmap_t *)dlq_deque(&msg->prefixQ);
        xmlns_free_pmap(pmap);
    }

    /* clean error queue */
    rpc_err_clean_errQ(&msg->errQ);
    msg->withdef = NCX_DEF_WITHDEF;

} /* xml_msg_clean_hdr */


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
const xmlChar *
    xml_msg_get_prefix (xml_msg_hdr_t *msg,
                        xmlns_id_t parent_nsid,
                        xmlns_id_t nsid,
                        val_value_t *curelem,
                        boolean  *xneeded)
{
    xmlns_pmap_t   *pmap, *newpmap;
    const xmlChar  *pfix;
    status_t        res;

#ifdef DEBUG
    if (!msg || !xneeded) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    newpmap = NULL;
    *xneeded = FALSE;

    /* check if the default namespace is requested */
    if (nsid == 0) {
        return NULL;
    }

    /* see if a prefix is already present in the rpc-reply element */
    pfix = NULL;
    pmap = find_pmap(msg, nsid);
    if (!pmap) {

        /* need to create a new prefix map and save it for real */
        newpmap = xmlns_new_pmap(0);
        if (!newpmap) {
            SET_ERROR(ERR_INTERNAL_MEM);
            return NULL;
        }

        /* generate a prefix ID */
        newpmap->nm_id = nsid;
        res = xml_msg_gen_new_prefix(msg,
                                     nsid, 
                                     &newpmap->nm_pfix, 
                                     0);
        if (res != NO_ERR) {
            SET_ERROR(res);
            xmlns_free_pmap(newpmap);
            return NULL;
        }

        pfix = newpmap->nm_pfix;

        /* add the new prefix mapping to the prefixQ */
        add_pmap(msg, newpmap);
    } else {
        pfix = pmap->nm_pfix;
    }

    /* xmlns directive will be needed if this is a new prefix */
    if (!pmap) {
        /* this is the first use */
        *xneeded = TRUE;
    } else if (!msg->useprefix && (parent_nsid != nsid)) {
        /* the default prefix needs to be generated again */
        *xneeded = TRUE;
    } else if (!pmap->nm_topattr) {
        /* this is not the first use, and the xmlns directive
         * is not in the top element; need to check if
         * the NSID has already been used in the current 
         * element or any of its ancestors
         */
        if (curelem) {
            /* check the actual value tree */
            *xneeded = xmlns_needed(curelem, nsid);
        } else if (parent_nsid != nsid) {
            /* use hack -- if child and parent not the same,
             * then generate the xmlns directive
             */
            *xneeded = TRUE;
        }
    }

    return pfix;

}  /* xml_msg_get_prefix */


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
const xmlChar *
    xml_msg_get_prefix_xpath (xml_msg_hdr_t *msg,
                              xmlns_id_t nsid)
{
    xmlns_pmap_t   *newpmap;
    const xmlChar  *pfix;
    status_t        res;

#ifdef DEBUG
    if (!msg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (!nsid) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    /* see if a prefix is already present in the rpc-reply element */
    pfix = find_prefix(msg, nsid);
    if (pfix) {
        return pfix;
    }

    /* need to create a new prefix map and save it for real */
    newpmap = xmlns_new_pmap(0);
    if (!newpmap) {
        SET_ERROR(ERR_INTERNAL_MEM);
        return NULL;
    }

    /* generate a prefix ID */
    newpmap->nm_id = nsid;
    res = xml_msg_gen_new_prefix(msg, 
                                 nsid, 
                                 &newpmap->nm_pfix, 
                                 0);
    if (res != NO_ERR) {
        xmlns_free_pmap(newpmap);
        return NULL;
    }

    /* add the new prefix mapping to the prefixQ */
    add_pmap(msg, newpmap);

    return newpmap->nm_pfix;

}  /* xml_msg_get_prefix_xpath */


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
const xmlChar *
    xml_msg_get_prefix_start_tag (xml_msg_hdr_t *msg,
                                  xmlns_id_t nsid)
{
    const xmlChar  *pfix;

#ifdef DEBUG
    if (!msg) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (!nsid) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    /* see if a prefix is already present in the rpc-reply element */
    pfix = find_prefix(msg, nsid);
    if (pfix) {
        return pfix;
    } else {
        return NULL;
    }

}  /* xml_msg_get_prefix_start_tag */


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
status_t 
    xml_msg_gen_new_prefix (xml_msg_hdr_t *msg,
                            xmlns_id_t  nsid,
                            xmlChar **retbuff,
                            uint32 buffsize)
{
    const xmlChar      *defpfix;
    xmlChar             startch;
    int32               nlen, i;
    xmlChar             numbuff[NCX_MAX_NUMLEN], *buff;
    xmlns_id_t          testid;

#ifdef DEBUG
    if (!msg || !retbuff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (*retbuff) {
        buff = *retbuff;
    } else {
        buff = m__getMem(NCX_MAX_NUMLEN+1);
        if (!buff) {
            return ERR_INTERNAL_MEM;
        } else {
            buffsize = NCX_MAX_NUMLEN+1;
            *retbuff = buff;
        }
    }

    /* first see if a default prefix was registered with the namespace
     * and use it if not already in the prefix map
     */
    defpfix = xmlns_get_ns_prefix(nsid);
    if (defpfix && *defpfix) {
        testid = find_prefix_val(msg, defpfix);
        if (testid == 0 || testid==nsid) {
            if (xml_strlen(defpfix) < buffsize) {
                xml_strcpy(buff, defpfix);
                return NO_ERR;
            } else {
                return ERR_BUFF_OVFL;
            }
        }
    }

    /* default already in use for something else so generate a prefix */
    nlen = snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%u", (uint32)nsid);
    if (nlen < 0) {
        return ERR_NCX_INVALID_NUM;
    }

    if ((uint32)(nlen+2) >= buffsize) {
        return ERR_BUFF_OVFL;
    }

    /* copy the number to the prefix buffer w/ trailing zero */
    for (i=0; i<=nlen; i++) {
        buff[i+1] = numbuff[i];
    }

    /* set the start letter in the prefix buffer */
    startch = 'n';

    /* try to generate a unique prefix */
    for (i=0; i<=MAX_PREFIX_TRIES; i++) {
        /* adjust the label value */
        buff[0] = startch++;
        if (startch > 'z') {
            startch = 'a';
        }

        /* check if the prefix is in use */
        if (!find_prefix_val(msg, buff)) {
            return NO_ERR;
        }
    }

    return ERR_NCX_OPERATION_FAILED;

}  /* xml_msg_gen_new_prefix */

/********************************************************************
 * Build a queue of xmlns_pmap_t records for the current message
 * This function will populate msg->prefixQ as needed,
 * XMLNS Entries for NETCONF and NCX will be added if they 
 * are not present
 * 
 * \param msg the message in progrss
 * \param attrs the the top-level attrs list (e;g, rpc_in_attrs)
 * \param addncid flag indicating if prefix entry for the NC namespace
 *                should be added
 * \param addncxid flag indicating if a prefix entry for the NCX namespace
 *                should be added
 * \return status
 *********************************************************************/
status_t xml_msg_build_prefix_map ( xml_msg_hdr_t *msg,
                                    xml_attrs_t *attrs,
                                    boolean addncid,
                                    boolean addncxid )
{
    assert( msg && "msg is NULL" );
    assert( attrs && "attrs is NULL" );

    /* look for any xmlns directives in the attrs list and build a prefix map 
     * entry so they will be reused in the reply. Deal with foreign namespace 
     * decls as well */
    status_t res = build_prefix_map_from_xmlns_directives( msg, attrs );
    if ( res != NO_ERR ) {
        return res;
    }

    /* make sure XMLNS decl for NETCONF is in the map make sure it is not the 
     * default, by forcing a new xmlns with a prefix -- needed by XPath 1.0 */
    if ( addncid ) {
        res = add_ncid_to_prefix_map( msg, attrs, xmlns_nc_id() );
    }

    /* make sure XMLNS decl for NCX is in the map try even if errors in setting
     * up the NETCONF pmap */
    if (addncxid ) {
        status_t retres = add_ncid_to_prefix_map( msg, attrs, xmlns_nc_id() );

        if (retres != NO_ERR && res == NO_ERR ) {
            res = retres ;
        }
    }

    return res;
}  /* xml_msg_build_prefix_map */


/**
 * Finish the queue of xmlns_pmap_t records for the current message
 * 
 * /param msg message in progrss
 * /param attrs the top-level attrs list (e;g, rpc_in_attrs)
 *
 * /return status of operation
*********************************************************************/
status_t xml_msg_finish_prefix_map (xml_msg_hdr_t *msg,
                                    xml_attrs_t *attrs)

{
    xmlns_pmap_t    *newpmap;
    xmlChar         *buff = NULL;
    xmlns_id_t       wdaid;
    status_t         res = NO_ERR;

    assert(msg && "msg is NULL"); 
    assert(attrs && "attrs is NULL"); 

    wdaid = xmlns_wda_id();

    /* make sure XMLNS decl for wd:default is in the map 
     * if the report-all-tagged mode is active
     */
    if (msg->withdef == NCX_WITHDEF_REPORT_ALL_TAGGED && 
        !find_prefix(msg, wdaid)) {

        /* add a prefix an xmlns attr for WD */
        res = xml_msg_gen_new_prefix(msg, wdaid, &buff, 0);
        if (res == NO_ERR) {
            res = xml_add_xmlns_attr(attrs, wdaid, buff);
        }
        if (res == NO_ERR) {
            /* create a new prefix map */
            newpmap = xmlns_new_pmap(0);
            if (newpmap == NULL) {
                res = ERR_INTERNAL_MEM;
            } else {
                newpmap->nm_id = wdaid;
                newpmap->nm_pfix = buff;
                newpmap->nm_topattr = TRUE;
                add_pmap(msg, newpmap);
            }
        }
    }

    if (buff && (res != NO_ERR)) {
        m__free(buff);
    }

    return res;

}  /* xml_msg_finish_prefix_map */


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
status_t
    xml_msg_check_xmlns_attr (xml_msg_hdr_t *msg, 
                              xmlns_id_t nsid,
                              const xmlChar *badns,
                              xml_attrs_t  *attrs)

{                     
    const xmlChar *pfix;
    xmlChar       *buff;
    xml_attr_t    *attr;
    status_t       res;

#ifdef DEBUG
    if (!msg || !attrs) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* no namespace is always ok, and if it is the same as the
     * current default, then nothing to do
     */
    if (!nsid) {
        return NO_ERR;
    }

    /* not the default, see if prefix already set for this NS */
    pfix = find_prefix(msg, nsid);
    if (pfix) {
        return NO_ERR;
    }

    /* check if this namespace ID already covered because the
     * xmlns decl is already present in the attrs list parameter
     */
    for (attr = (xml_attr_t *)dlq_firstEntry(attrs);
         attr != NULL;
         attr = (xml_attr_t *)dlq_nextEntry(attr)) {
        if (attr->attr_xmlns_ns == nsid) {
            return NO_ERR;
        }
    }

    /* not already covered */
    buff = NULL;
    res = xml_msg_gen_new_prefix(msg, nsid, &buff, 0);
    if (res == NO_ERR) {
        if (nsid == xmlns_inv_id()) {
            res = xml_add_inv_xmlns_attr(attrs, nsid, buff, badns);
        } else {
            res = xml_add_xmlns_attr(attrs, nsid, buff);
        }
    }
    if (buff) {
        m__free(buff);
    }

    return res;

}  /* xml_msg_check_xmlns_attr */


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
status_t
    xml_msg_gen_xmlns_attrs (xml_msg_hdr_t *msg, 
                             xml_attrs_t *attrs,
                             boolean addncx)
{
    xmlns_pmap_t *pmap, *newpmap;
    xmlChar      *buff;
    status_t      res, retres;
    boolean       ncxfound;
    xmlns_id_t    ncx_id;

#ifdef DEBUG
    if (!msg || !attrs) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    ncxfound = FALSE;
    retres = NO_ERR;
    ncx_id = xmlns_ncx_id();

    for (pmap = (xmlns_pmap_t *)dlq_firstEntry(&msg->prefixQ);
         pmap != NULL;
         pmap = (xmlns_pmap_t *)dlq_nextEntry(pmap)) {

        if (pmap->nm_id == ncx_id) {
            ncxfound = TRUE;
        }

        if (pmap->nm_topattr) {
            continue;
        }

        buff = NULL;
        res = xml_msg_gen_new_prefix(msg, pmap->nm_id, &buff, 0);
        if (res == NO_ERR) {
            res = xml_add_xmlns_attr(attrs, pmap->nm_id, buff);
        }
        if (buff) {
            m__free(buff);
        }

        if (res != NO_ERR) {
            retres = res;
        } else {
            pmap->nm_topattr = TRUE;
        }
    }

    if (!ncxfound && addncx && retres == NO_ERR) {
        /* need to create a new prefix map and save it */
        newpmap = xmlns_new_pmap(0);
        if (!newpmap) {
            retres = ERR_INTERNAL_MEM;
        } else {
            /* generate a prefix ID */
            newpmap->nm_id = ncx_id;
            newpmap->nm_topattr = TRUE;
            res = xml_msg_gen_new_prefix(msg,
                                         ncx_id, 
                                         &newpmap->nm_pfix, 
                                         0);
            if (res == NO_ERR) {
                /* add the new prefix mapping to the prefixQ */
                res = xml_add_xmlns_attr(attrs, 
                                         newpmap->nm_id, 
                                         newpmap->nm_pfix);
                if (res == NO_ERR) {
                    add_pmap(msg, newpmap);
                } else {
                    xmlns_free_pmap(newpmap);
                    retres = res;
                }
            } else {
                xmlns_free_pmap(newpmap);
                retres = res;
            }
        }
    }

    return retres;

}  /* xml_msg_gen_xmlns_attrs */




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
status_t
    xml_msg_clean_defns_attr (xml_attrs_t *attrs)
{
    xml_attr_t  *attr, *nextattr;
    uint32       len, len2;

#ifdef DEBUG
    if (!attrs) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    len = xml_strlen(XMLNS);

    for (attr = (xml_attr_t *)xml_first_attr(attrs);
         attr != NULL;
         attr = nextattr) {


        nextattr = (xml_attr_t *)xml_next_attr(attr);

        len2 = xml_strlen(attr->attr_qname);
        if (len2 >= len) {
            if (!xml_strncmp(attr->attr_qname, XMLNS, len)) {
                if (len == len2) {
                    /* this xmlns=foo is getting toosed so
                     * the netconf NSID can be the default
                     */
                    dlq_remove(attr);
                    xml_free_attr(attr);
                    return NO_ERR;
                } /* else xmlns:foo=bar found */
            }  /* else not xmlns */
        }  /* else not 'xmlns' */
    }

    return NO_ERR;

}  /* xml_msg_clean_defns_attr */


/* END file xml_msg.c */
