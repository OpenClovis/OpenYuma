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
/*  FILE: def_reg.c

   Definition Lookup Registry

   Stores Pointers to Data Structures but does not malloc or free
   those structures.  That must be done outside this registry,
   which justs provides hash table storage of pointers

   Registry node types

     DEF_NT_NSNODE  : Namespace to module pointer

     DEF_NT_FDNODE  : File Descriptor to session control block

     DEF_NT_MODNAME : ncx_module_t lookup

     DEF_NT_OWNNODE   Parent node of module-specific applications

     DEF_NT_DEFNODE : Child node: Module-specific definition

     DEF_NT_CFGNODE : Child node: Configuration data pointer

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
17oct05      abb      begun
11-feb-06    abb      remove application layer; too complicated
18-aug-07    abb      add user variable support for ncxcli

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_bobhash
#include "bobhash.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
/* 256 row, chained entry top hash table */
#define DR_TOP_HASH_SIZE   (hashsize(8))
#define DR_TOP_HASH_MASK   (hashmask(8))

/* random number to seed the hash function */
#define DR_HASH_INIT       0x7e456289


/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/
/* top-level registry node type */
typedef enum def_nodetyp_t_ {
    DEF_NT_NONE,
    DEF_NT_NSNODE,          /* topht namespace URI */
    DEF_NT_FDNODE           /* topht file descriptor integer */
} def_nodetyp_t;


/* The def_reg module header that must be the first field in
 * any record stored in any level hash table
 */
typedef struct def_hdr_t_ {
    dlq_hdr_t       qhdr;
    def_nodetyp_t   nodetyp; 
    const xmlChar  *key;
} def_hdr_t;


/* DEF_NT_NSNODE
 * Top tier: Entries that don't have any children 
 */
typedef struct def_topnode_t_ {
    def_hdr_t      hdr;
    void          *dptr;
} def_topnode_t;


/* DEF_NT_FDNODE
 *
 * Top Tier: File Descriptor Mapping Content Struct
 */
typedef struct def_fdmap_t_ {
    xmlChar    num[NCX_MAX_NUMLEN];
    int        fd;
    ses_cb_t  *scb;
} def_fdmap_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/* first tier: module header hash table */
static dlq_hdr_t   topht[DR_TOP_HASH_SIZE];

/* module init flag */
static boolean     def_reg_init_done = FALSE;


/********************************************************************
* FUNCTION find_top_node_h
* 
* Find a top level node by its type and key
* Retrieve the hash table index in case the search fails
*
* INPUTS:
*    nodetyp == def_nodetyp_t enum value
*    key == top level node key
* OUTPUTS:
*    *h == hash table index
* RETURNS:
*    void *to the top level entry or NULL if not found
*********************************************************************/
static void * 
    find_top_node_h (def_nodetyp_t nodetyp,
                     const xmlChar *key,
                     uint32 *h)
{
    uint32 len;
    def_hdr_t *hdr;

    len = xml_strlen(key);
    if (!len) {
        return NULL;
    }

    /* get the hash value */
    *h = bobhash(key, len, DR_HASH_INIT);

    /* clear bits to fit the topht array size */
    *h &= DR_TOP_HASH_MASK;

    for (hdr = (def_hdr_t *)dlq_firstEntry(&topht[*h]);
         hdr != NULL;
         hdr = (def_hdr_t *)dlq_nextEntry(hdr)) {
        if (hdr->nodetyp==nodetyp && !xml_strcmp(key, hdr->key)) {
            return (void *)hdr;
        }
    }
    return NULL;

}  /* find_top_node_h */


/********************************************************************
* FUNCTION find_top_node
* 
* Find a top level node by its type and key
*
* INPUTS:
*    nodetyp == def_nodetyp_t enum value
*    key == top level node key
* RETURNS:
*    void *to the top level entry or NULL if not found
*********************************************************************/
static void * 
    find_top_node (def_nodetyp_t nodetyp,
                   const xmlChar *key)
{
    uint32  h;
    return find_top_node_h(nodetyp, key, &h);

}  /* find_top_node */


/********************************************************************
* FUNCTION add_top_node
*   
* add one top-level node to the registry
* !!! NOT FOR OWNNODE ENTRIES !!!
*
* INPUTS:
*    nodetyp == internal node type enum
*    key == address of key string inside 'ptr'
*    ptr == struct pointer to store in the registry
* RETURNS:
*    status of the operation
*********************************************************************/
static status_t 
    add_top_node (def_nodetyp_t  nodetyp,
                  const xmlChar *key, 
                  void *ptr)
{
    uint32 h;
    def_topnode_t *top;

    h = 0;

    /* check if the entry already exists */
    top = (def_topnode_t *)find_top_node_h(nodetyp, key, &h);
    if (top) {
        return ERR_NCX_DUP_ENTRY;
    }

    /* create a new def_topnode_t struct and initialize it */
    top = m__getObj(def_topnode_t);
    if (top == NULL) {
        return ERR_INTERNAL_MEM;
    }
    (void)memset(top, 0x0, sizeof(def_topnode_t));
    top->hdr.nodetyp = nodetyp;
    top->hdr.key = key;
    top->dptr = ptr;

    /* add the topnode to the topht */
    dlq_enque(top, &topht[h]);
    return NO_ERR;

} /* add_top_node */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION def_reg_init
* 
* Initialize the def_reg module
*
* RETURNS:
*    none
*********************************************************************/
void 
    def_reg_init (void)
{
    uint32 i;

    if (!def_reg_init_done) {
        /* initialize the application hash table */
        for (i=0; i<DR_TOP_HASH_SIZE; i++) {
            dlq_createSQue(&topht[i]);
        }
        def_reg_init_done = TRUE;
    }  /* else already done */

}  /* def_reg_init */


/********************************************************************
* FUNCTION def_reg_cleanup
* 
* Cleanup all the malloced memory in this module
* and return the module to an uninitialized state
* After this fn, the def_reg_init fn could be called again
*
* INPUTS:
*    none
* RETURNS:
*    none
*********************************************************************/
void 
    def_reg_cleanup (void)
{
    uint32 i;
    def_hdr_t     *hdr;
    def_topnode_t *topnode;

    if (!def_reg_init_done) {
        return;
    }

    /* cleanup the top hash table */
    for (i=0; i<DR_TOP_HASH_SIZE; i++) {
        while (!dlq_empty(&topht[i])) {
            hdr = (def_hdr_t *)dlq_deque(&topht[i]);
            switch (hdr->nodetyp) {
            case DEF_NT_NSNODE:
                m__free(hdr);
                break;
            case DEF_NT_FDNODE:
                /* free the def_fdnode_t struct first */
                topnode = (def_topnode_t *)hdr;
                m__free(topnode->dptr);
                m__free(topnode);
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
                m__free(hdr);  /* free it anyway */
            }
        }
    }
    (void)memset(topht, 0x0, sizeof(dlq_hdr_t)*DR_TOP_HASH_SIZE);

    def_reg_init_done = FALSE;

}  /* def_reg_cleanup */


/********************************************************************
* FUNCTION def_reg_add_ns
*   
* add one xmlns_t to the registry
*
* INPUTS:
*    ns == namespace record to add
* RETURNS:
*    status of the operation
*********************************************************************/
status_t 
    def_reg_add_ns (xmlns_t *ns)
{
#ifdef DEBUG
    if (!ns) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return add_top_node(DEF_NT_NSNODE, ns->ns_name, ns);

} /* def_reg_add_ns */


/********************************************************************
* FUNCTION def_reg_find_ns
*   
* find one xmlns_t in the registry
* find a xmlns_t by its value (name)
*
* INPUTS:
*    nsname == namespace ID to find
* RETURNS:
*    pointer to xmlns_t or NULL if not found
*********************************************************************/
xmlns_t * 
    def_reg_find_ns (const xmlChar *nsname)
{
    def_topnode_t *nsdef;

#ifdef DEBUG
    if (!nsname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    nsdef = find_top_node(DEF_NT_NSNODE, nsname);
    return (nsdef) ? (xmlns_t *)nsdef->dptr : NULL;

} /* def_reg_find_ns */


/********************************************************************
* FUNCTION def_reg_del_ns
*
* unregister a xmlns_t
* delete one ncx_module from the registry
*
* INPUTS:
*    nsname == namespace name to delete
* RETURNS:
*    none
*********************************************************************/
void
    def_reg_del_ns (const xmlChar *nsname)
{
    def_topnode_t *nsdef;

#ifdef DEBUG
    if (!nsname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    nsdef = find_top_node(DEF_NT_NSNODE, nsname);
    if (nsdef) {
        dlq_remove(nsdef);
        m__free(nsdef);
    }
} /* def_reg_del_ns */


/********************************************************************
* FUNCTION def_reg_add_scb
*
* add one FD to SCB mapping to the registry
*
* INPUTS:
*    fd == file descriptor to add
*    session == ses_cb_t for the session
* RETURNS:
*    status of the operation
*********************************************************************/
status_t 
    def_reg_add_scb (int fd,
                     ses_cb_t *scb)
{
    def_fdmap_t *fdmap;
    int          ret;
    status_t     res;

#ifdef DEBUG
    if (!scb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* create an FD-to-SCB mapping */
    fdmap = m__getObj(def_fdmap_t);
    if (!fdmap) {
        return ERR_INTERNAL_MEM;
    }
    memset(fdmap, 0x0, sizeof(def_fdmap_t));

    /* get a string key */
    ret = snprintf((char *)fdmap->num, sizeof(fdmap->num), "%d", fd);
    if (ret <= 0) {
        m__free(fdmap);
        return ERR_NCX_INVALID_NUM;
    }

    /* set the mapping */
    fdmap->fd = fd;
    fdmap->scb = scb;
    
    /* save the string-keyed mapping entry */
    res = add_top_node(DEF_NT_FDNODE, fdmap->num, fdmap);
    if (res != NO_ERR) {
        m__free(fdmap);
    }
    return res;

} /* def_reg_add_scb */


/********************************************************************
* FUNCTION def_reg_find_scb
*   
* find one FD-to-SCB mapping in the registry
*
* INPUTS:
*    fd == file descriptor ID to find
* RETURNS:
*    pointer to ses_cb_t or NULL if not found
*********************************************************************/
ses_cb_t * 
    def_reg_find_scb (int fd)
{
    def_topnode_t *fddef;
    def_fdmap_t   *fdmap;
    int            ret;
    xmlChar        buff[NCX_MAX_NUMLEN];

    ret = snprintf((char *)buff, sizeof(buff), "%d", fd);
    if (ret <= 0) {
        return NULL;
    }
    
    fddef = find_top_node(DEF_NT_FDNODE, buff);
    if (!fddef) {
        return NULL;
    }
    fdmap = fddef->dptr;
    return fdmap->scb;

} /* def_reg_find_scb */


/********************************************************************
* FUNCTION def_reg_del_scb
*   
* delete one FD to SCB mapping from the registry
*
* INPUTS:
*    fd == file descriptor index to delete
* RETURNS:
*    none
*********************************************************************/
void
    def_reg_del_scb (int fd)
{
    def_topnode_t *fddef;
    int            ret;
    xmlChar        buff[NCX_MAX_NUMLEN];

    ret = snprintf((char *)buff, sizeof(buff), "%d", fd);
    if (ret <= 0) {
        return;
    }

    fddef = find_top_node(DEF_NT_FDNODE, buff);
    if (fddef) {
        dlq_remove(fddef);
        m__free(fddef->dptr);  /* free the def_fdmap_t */
        m__free(fddef);
    }
} /* def_reg_del_scb */


/* END file def_reg.c */
