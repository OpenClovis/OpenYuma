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
/*  FILE: obj.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
09dec07      abb      begun
21jul08      abb      start obj-based rewrite

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <xmlstring.h>

#include "procdefs.h"
#include "dlq.h"
#include "grp.h"
#include "ncxconst.h"
#include "ncx.h"
#include "ncx_appinfo.h"
#include "ncx_feature.h"
#include "ncx_list.h"
#include "obj.h"
#include "tk.h"
#include "typ.h"
#include "xml_util.h"
#include "xmlns.h"
#include "xpath.h"
#include "yang.h"
#include "yangconst.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
/* #define OBJ_CLONE_DEBUG 1 */
/* #define OBJ_MEM_DEBUG 1 */
#endif

/********************************************************************
*                                                                   *
*                         V A R I A B L E S                         *
*                                                                   *
*********************************************************************/
static obj_case_t * new_case (boolean isreal); 
static void free_case (obj_case_t *cas);
static obj_template_t* find_template( dlq_hdr_t*que, const xmlChar *modname, 
                                      const xmlChar *objname, boolean lookdeep,
                                      boolean partialmatch, boolean usecase, 
                                      boolean altnames, boolean dataonly, 
                                      uint32 *matchcount );

/********************************************************************
* FUNCTION find_type_in_grpchain
* 
* Search for a defined typedef in the typeQ of each grouping
* in the chain
*
* INPUTS:
*   obj == blank template to free
*
* RETURNS:
*   pointer to found type template, NULL if not found
*********************************************************************/
static typ_template_t *
    find_type_in_grpchain (grp_template_t *grp,
                           const xmlChar *typname)
{
    typ_template_t  *typ;
    grp_template_t  *testgrp;

    typ = ncx_find_type_que(&grp->typedefQ, typname);
    if (typ) {
        return typ;
    }

    testgrp = grp->parentgrp;
    while (testgrp) {
        typ = ncx_find_type_que(&testgrp->typedefQ, typname);
        if (typ) {
            return typ;
        }
        testgrp = testgrp->parentgrp;
    }
    return NULL;

}  /* find_type_in_grpchain */



/********************************************************************
* FUNCTION init_template
* 
* Initialize the fields in an obj_template_t
*
* INPUTS:
*   obj == template to init
*********************************************************************/
static void init_template (obj_template_t *obj)
{
    (void)memset(obj, 0x0, sizeof(obj_template_t));
    dlq_createSQue(&obj->metadataQ);
    dlq_createSQue(&obj->appinfoQ);
    dlq_createSQue(&obj->iffeatureQ);
    dlq_createSQue(&obj->inherited_iffeatureQ);
    dlq_createSQue(&obj->inherited_whenQ);

}  /* init_template */


/********************************************************************
* FUNCTION new_blank_template
* 
* Malloc and initialize the fields in a an obj_template_t
* Do not malloc or initialize any of the def union pointers
*
*
* RETURNS:
*   pointer to the malloced and partially initialized struct;
*   NULL if an error
*********************************************************************/
static obj_template_t * 
    new_blank_template (void)
{
    obj_template_t  *obj;

    obj = m__getObj(obj_template_t);
    if (!obj) {
        return NULL;
    }
    init_template(obj);
    return obj;

}  /* new_blank_template */


/********************************************************************
 * Clean a Q of xpath_pcb_t entries
 *
 * \param mustQ Q of ncx_errinfo_t to delete
 *
 *********************************************************************/
static void clean_mustQ (dlq_hdr_t *mustQ)
{
    if ( !mustQ ) {
        return;
    }

    while (!dlq_empty(mustQ)) {
        xpath_pcb_t *must = (xpath_pcb_t *)dlq_deque(mustQ);
        xpath_free_pcb(must);
    }
}  /* clean_mustQ */


/********************************************************************
* FUNCTION clean_metadataQ
* 
* Clean a Q of obj_metadata_t
*
* INPUTS:
*   metadataQ == Q of obj_metadata_t to delete
*
*********************************************************************/
static void clean_metadataQ (dlq_hdr_t *metadataQ)
{
    obj_metadata_t *meta;

    while (!dlq_empty(metadataQ)) {
        meta = (obj_metadata_t *)dlq_deque(metadataQ);
        obj_free_metadata(meta);
    }

}  /* clean_metadataQ */


/********************************************************************
* FUNCTION clone_datadefQ
* 
* Clone a Q of obj_template_t
*
* INPUTS:
*    mod == module owner of the cloned data
*    newQ == Q of obj_template_t getting new contents
*    srcQ == Q of obj_template_t with starting contents
*    mobjQ == Q of OBJ_TYP_REFINE obj_template_t to 
*            merge into the clone, as refinements
*            (May be NULL)
*   parent == parent object containing the srcQ (may be NULL)
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    clone_datadefQ (ncx_module_t *mod,
                    dlq_hdr_t *newQ,
                    dlq_hdr_t *srcQ,
                    dlq_hdr_t *mobjQ,
                    obj_template_t *parent)
{
    obj_template_t  *newobj, *srcobj, *testobj;
    status_t         res;

    res = NO_ERR;

    for (srcobj = (obj_template_t *)dlq_firstEntry(srcQ);
         srcobj != NULL;
         srcobj = (obj_template_t *)dlq_nextEntry(srcobj)) {

        if (!obj_has_name(srcobj)) {
            continue;
        }

        newobj = obj_clone_template(mod, srcobj, mobjQ);
        if (!newobj) {
            log_error("\nError: clone of object %s failed",
                      obj_get_name(srcobj));
            return ERR_INTERNAL_MEM;
        } else {
            testobj = obj_find_template(newQ, 
                                        obj_get_mod_name(newobj),
                                        obj_get_name(newobj));
            if (testobj) {
                log_error("\nError: Object %s on line %s "
                          "already defined at line %u",
                          obj_get_name(newobj),
                          srcobj->tkerr.linenum,
                          testobj->tkerr.linenum);
                obj_free_template(newobj);
            } else {
                newobj->parent = parent;
                dlq_enque(newobj, newQ);
            }
        }
    }

    return res;

}  /* clone_datadefQ */


/********************************************************************
* FUNCTION clone_appinfoQ
* 
* Copy the contents of the src appinfoQ to the new appinfoQ
* Also add in any merge appinfo that are present
*
* INPUTS:
*    newQ == Q of ncx_appinfo_t getting new contents
*    srcQ == Q of ncx_appinfo_t with starting contents
*    merQ == Q of ncx_appinfo_t to merge into the clone,
*            as additions    (May be NULL)
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    clone_appinfoQ (dlq_hdr_t *newQ,
                    dlq_hdr_t *srcQ,
                    dlq_hdr_t *merQ)
{
    ncx_appinfo_t  *newapp, *srcapp;

    for (srcapp = (ncx_appinfo_t *)dlq_firstEntry(srcQ);
         srcapp != NULL;
         srcapp = (ncx_appinfo_t *)dlq_nextEntry(srcapp)) {

        newapp = ncx_clone_appinfo(srcapp);
        if (!newapp) {
            log_error("\nError: clone of appinfo failed");
            return ERR_INTERNAL_MEM;
        } else {
            dlq_enque(newapp, newQ);
        }
    }

    if (merQ) {
        for (srcapp = (ncx_appinfo_t *)dlq_firstEntry(merQ);
             srcapp != NULL;
             srcapp = (ncx_appinfo_t *)dlq_nextEntry(srcapp)) {

            newapp = ncx_clone_appinfo(srcapp);
            if (!newapp) {
                log_error("\nError: clone of appinfo failed");
                return ERR_INTERNAL_MEM;
            } else {
                dlq_enque(newapp, newQ);
            }
        }
    }

    return NO_ERR;

}  /* clone_appinfoQ */


/********************************************************************
* FUNCTION clone_iffeatureQ
* 
* Copy the contents of the src iffeatureQ to the new iffeatureQ
* Also add in any merge if-features that are present
*
* INPUTS:
*    newQ == Q of ncx_iffeature_t getting new contents
*    srcQ == Q of ncx_iffeature_t with starting contents
*    merQ == Q of ncx_iffeature_t to merge into the clone,
*            as additions    (May be NULL)
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    clone_iffeatureQ (dlq_hdr_t *newQ,
                      dlq_hdr_t *srcQ,
                      dlq_hdr_t *merQ)
{
    ncx_iffeature_t *newif, *srcif;

    for (srcif = (ncx_iffeature_t *)dlq_firstEntry(srcQ);
         srcif != NULL;
         srcif = (ncx_iffeature_t *)dlq_nextEntry(srcif)) {

        newif = ncx_clone_iffeature(srcif);
        if (!newif) {
            log_error("\nError: clone of iffeature failed");
            return ERR_INTERNAL_MEM;
        } else {
            dlq_enque(newif, newQ);
        }
    }

    if (merQ) {
        for (srcif = (ncx_iffeature_t *)dlq_firstEntry(merQ);
             srcif != NULL;
             srcif = (ncx_iffeature_t *)dlq_nextEntry(srcif)) {

            newif = ncx_clone_iffeature(srcif);
            if (!newif) {
                log_error("\nError: clone of iffeature failed");
                return ERR_INTERNAL_MEM;
            } else {
                dlq_enque(newif, newQ);
            }
        }
    }

    return NO_ERR;

}  /* clone_iffeatureQ */


/********************************************************************
* FUNCTION clone_case
* 
* Clone a case struct
*
* INPUTS:
*    mod == module owner of the cloned data
*    cas == obj_case_t data structure to clone
*    mcas == obj_refine_t data structure to merge
*            into the clone, as refinements.  Only
*            legal case refinements will be checked
*            (May be NULL)
*    obj == new object template getting this cloned case
*    mobjQ == Q containing OBJ_TYP_REFINE objects to check
*
* RETURNS:
*   pointer to malloced case clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_case_t *
    clone_case (ncx_module_t *mod,
                obj_case_t *cas,
                obj_refine_t *mcas,
                obj_template_t *obj,
                dlq_hdr_t  *mobjQ)
{
    obj_case_t     *newcas;
    status_t        res;

    res = NO_ERR;

    newcas = new_case(TRUE);  /*** need a real datadefQ ***/
    if (!newcas) {
        return NULL;
    }

    /* set the fields that cannot be refined */
    newcas->name = cas->name;
    newcas->nameclone = TRUE;
    newcas->status = cas->status;

    if (mcas && mcas->descr) {
        newcas->descr = xml_strdup(mcas->descr);
        if (!newcas->descr) {
            free_case(newcas);
            return NULL;
        }
    } else if (cas->descr) {
        newcas->descr = xml_strdup(cas->descr);
        if (!newcas->descr) {
            free_case(newcas);
            return NULL;
        }
    }

    if (mcas && mcas->ref) {
        newcas->ref = xml_strdup(mcas->ref);
        if (!newcas->ref) {
            free_case(newcas);
            return NULL;
        }
    } else if (cas->ref) {
        newcas->ref = xml_strdup(cas->ref);
        if (!newcas->ref) {
            free_case(newcas);
            return NULL;
        }
    }

    res = clone_datadefQ(mod, 
                         newcas->datadefQ, 
                         cas->datadefQ,
                         mobjQ, 
                         obj);
    if (res != NO_ERR) {
        free_case(newcas);
        return NULL;
    }

    return newcas;

}  /* clone_case */


/********************************************************************
* FUNCTION clone_mustQ
* 
* Clone a Q of must clauses (ncx_errinfo_t structs)
*
* INPUTS:
*    newQ == Q getting new structs
*    srcQ == Q with starting contents
*    mergeQ == Q of refinements (additional must statements)
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    clone_mustQ (dlq_hdr_t *newQ,
                 dlq_hdr_t *srcQ,
                 dlq_hdr_t *mergeQ)
{

    xpath_pcb_t *srcmust, *newmust;

    for (srcmust = (xpath_pcb_t *)dlq_firstEntry(srcQ);
         srcmust != NULL;
         srcmust = (xpath_pcb_t *)dlq_nextEntry(srcmust)) {

        newmust = xpath_clone_pcb(srcmust);
        if (!newmust) {
            return ERR_INTERNAL_MEM;
        } else {
            dlq_enque(newmust, newQ);
        }
    }

    if (mergeQ) {
        for (srcmust = (xpath_pcb_t *)dlq_firstEntry(mergeQ);
             srcmust != NULL;
             srcmust = (xpath_pcb_t *)dlq_nextEntry(srcmust)) {

            newmust = xpath_clone_pcb(srcmust);
            if (!newmust) {
                return ERR_INTERNAL_MEM;
            } else {
                dlq_enque(newmust, newQ);
            }
        }
    }

    return NO_ERR;

}  /* clone_mustQ */


/********************************************************************
* Malloc and initialize the fields in a an obj_container_t
*
* \param isreal TRUE for a real object, FALSE for a cloned object
* \return pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_container_t * 
    new_container (boolean isreal)
{
    obj_container_t  *con;

    con = m__getObj(obj_container_t);
    if (!con) {
        return NULL;
    }
    (void)memset(con, 0x0, sizeof(obj_container_t));

    con->datadefQ = dlq_createQue();
    if (!con->datadefQ) {
        m__free(con);
        return NULL;
    }

    if (isreal) {
        con->typedefQ = dlq_createQue();
        if (!con->typedefQ) {
            dlq_destroyQue(con->datadefQ);
            m__free(con);
            return NULL;
        }

        con->groupingQ = dlq_createQue();
        if (!con->groupingQ) {
            dlq_destroyQue(con->datadefQ);
            dlq_destroyQue(con->typedefQ);
            m__free(con);
            return NULL;
        }

        con->status = NCX_STATUS_CURRENT;
    }

    dlq_createSQue(&con->mustQ);

    return con;

}  /* new_container */


/********************************************************************
* Scrub the memory in a obj_container_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* \param con obj_container_t data structure to free
* \param flags flags field from object freeing this container
*********************************************************************/
static void free_container ( obj_container_t *con, uint32 flags )
{
    if ( !con ) {
        return;
    }

    boolean notclone = (flags & OBJ_FL_CLONE) ? FALSE : TRUE;

    if ( notclone ) {
        m__free(con->name);
    }

    m__free(con->descr);
    m__free(con->ref);
    m__free(con->presence);

    if (notclone) {
        typ_clean_typeQ(con->typedefQ);
        dlq_destroyQue(con->typedefQ);
        grp_clean_groupingQ(con->groupingQ);
        dlq_destroyQue(con->groupingQ);
    }

    if (!con->datadefclone) {
        obj_clean_datadefQ(con->datadefQ);
        dlq_destroyQue(con->datadefQ);
    }

    clean_mustQ(&con->mustQ);

    m__free(con);
}  /* free_container */


/********************************************************************
* FUNCTION clone_container
* 
* Clone a container struct
*
* INPUTS:
*    mod == module owner of the cloned data
*    parent == new object containing 'con'
*    con == obj_container_t data structure to clone
*    mcon == obj_refine_t data structure to merge
*            into the clone, as refinements.  Only
*            legal container refinements will be checked
*            (May be NULL)
*    mobjQ == starting merge object Q, passed through
*
* RETURNS:
*   pointer to malloced object clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_container_t *
    clone_container (ncx_module_t *mod,
                     obj_template_t  *parent,
                     obj_container_t *con,
                     obj_refine_t *mcon,
                     dlq_hdr_t  *mobjQ)
{
    obj_container_t *newcon;
    status_t         res;

    newcon = new_container(FALSE);
    if (!newcon) {
        return NULL;
    }

    /* set the fields that cannot be refined */
    newcon->name = con->name;

    newcon->typedefQ = con->typedefQ;
    newcon->groupingQ = con->groupingQ;
    newcon->status = con->status;


    if (mcon && mcon->descr) {
        newcon->descr = xml_strdup(mcon->descr);
        if (!newcon->descr) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (con->descr) {
        newcon->descr = xml_strdup(con->descr);
        if (!newcon->descr) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mcon && mcon->ref) {
        newcon->ref = xml_strdup(mcon->ref);
        if (!newcon->ref) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (con->ref) {
        newcon->ref = xml_strdup(con->ref);
        if (!newcon->ref) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mcon && mcon->presence) {
        newcon->presence = xml_strdup(mcon->presence);
        if (!newcon->presence) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (con->presence) {
        newcon->presence = xml_strdup(con->presence);
        if (!newcon->presence) {
            free_container(newcon, OBJ_FL_CLONE);
            return NULL;
        }
    }

    res = clone_mustQ(&newcon->mustQ, 
                      &con->mustQ,
                      (mcon) ? &mcon->mustQ : NULL);
    if (res != NO_ERR) {
        free_container(newcon, OBJ_FL_CLONE);
        return NULL;
    }

    res = clone_datadefQ(mod, 
                         newcon->datadefQ, 
                         con->datadefQ, 
                         mobjQ, 
                         parent);
    if (res != NO_ERR) {
        free_container(newcon, OBJ_FL_CLONE);
        return NULL;
    }

    return newcon;

}  /* clone_container */


/********************************************************************
* FUNCTION new_leaf
* 
* Malloc and initialize the fields in a an obj_leaf_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_leaf_t * new_leaf (boolean isreal)
{
    obj_leaf_t  *leaf;

    leaf = m__getObj(obj_leaf_t);
    if (!leaf) {
        return NULL;
    }

    (void)memset(leaf, 0x0, sizeof(obj_leaf_t));

    if (isreal) {
        leaf->typdef = typ_new_typdef();
        if (!leaf->typdef) {
            m__free(leaf);
            return NULL;
        }
        leaf->status = NCX_STATUS_CURRENT;
    }

    dlq_createSQue(&leaf->mustQ);

    return leaf;

}  /* new_leaf */


/********************************************************************
* Scrub the memory in a obj_leaf_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* \param leaf obj_leaf_t data structure to free
* \param flags flags field from object freeing this leaf
*********************************************************************/
static void free_leaf (obj_leaf_t *leaf, uint32 flags)
{
    if ( !leaf ) {
        return;
    }

    boolean notclone = (flags & OBJ_FL_CLONE) ? FALSE : TRUE;

    m__free(leaf->defval);
    m__free(leaf->descr);
    m__free(leaf->ref);

    if (notclone ) {
        m__free(leaf->name);
        m__free(leaf->units);
        
        if ( leaf->typdef && (leaf->typdef->tclass != NCX_CL_BASE)) {
            typ_free_typdef(leaf->typdef);
        }
    }

    clean_mustQ(&leaf->mustQ);
    m__free(leaf);
}  /* free_leaf */


/********************************************************************
* FUNCTION clone_leaf
* 
* Clone a leaf struct
*
* INPUTS:
*    leaf == obj_leaf_t data structure to clone
*    mleaf == obj_refine_t data structure to merge
*            into the clone, as refinements.  Only
*            legal leaf refinements will be checked
*            (May be NULL)
*
* RETURNS:
*   pointer to malloced object clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_leaf_t *
    clone_leaf (obj_leaf_t *leaf,
                obj_refine_t *mleaf)
{
    obj_leaf_t      *newleaf;
    status_t         res;

    newleaf = new_leaf(FALSE);
    if (!newleaf) {
        return NULL;
    }

    /* set the fields that cannot be refined */
    newleaf->name = leaf->name;
    newleaf->units = leaf->units;
    newleaf->typdef = leaf->typdef;
    newleaf->status = leaf->status;

    if (mleaf && mleaf->def) {
        newleaf->defval = xml_strdup(mleaf->def);
        if (!newleaf->defval) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (leaf->defval) {
        newleaf->defval = xml_strdup(leaf->defval);
        if (!newleaf->defval) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mleaf && mleaf->descr) {
        newleaf->descr = xml_strdup(mleaf->descr);
        if (!newleaf->descr) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (leaf->descr) {
        newleaf->descr = xml_strdup(leaf->descr);
        if (!newleaf->descr) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mleaf && mleaf->ref) {
        newleaf->ref = xml_strdup(mleaf->ref);
        if (!newleaf->ref) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (leaf->ref) {
        newleaf->ref = xml_strdup(leaf->ref);
        if (!newleaf->ref) {
            free_leaf(newleaf, OBJ_FL_CLONE);
            return NULL;
        }
    }

    res = clone_mustQ(&newleaf->mustQ, 
                      &leaf->mustQ,
                      (mleaf) ? &mleaf->mustQ : NULL);
    if (res != NO_ERR) {
        free_leaf(newleaf, OBJ_FL_CLONE);
        return NULL;
    }

    return newleaf;

}  /* clone_leaf */


/********************************************************************
* FUNCTION new_leaflist
* 
* Malloc and initialize the fields in a an obj_leaflist_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_leaflist_t * 
    new_leaflist (boolean isreal)
{
    obj_leaflist_t  *leaflist;

    leaflist = m__getObj(obj_leaflist_t);
    if (!leaflist) {
        return NULL;
    }

    (void)memset(leaflist, 0x0, sizeof(obj_leaflist_t));

    if (isreal) {
        leaflist->typdef = typ_new_typdef();
        if (!leaflist->typdef) {
            m__free(leaflist);
            return NULL;
        }
        leaflist->status = NCX_STATUS_CURRENT;
        leaflist->ordersys = TRUE;
    }

    dlq_createSQue(&leaflist->mustQ);

    return leaflist;

}  /* new_leaflist */


/********************************************************************
 * Scrub the memory in a obj_leaflist_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param leaflist obj_leaflist_t data structure to free
 * \param flags flags field from object freeing this leaf-list
 *********************************************************************/
static void free_leaflist (obj_leaflist_t *leaflist, uint32 flags)
{
    if ( !leaflist ) {
        return;
    }

    boolean notclone = (flags & OBJ_FL_CLONE) ? FALSE : TRUE;

    if (notclone ) {
        m__free(leaflist->name);
        m__free(leaflist->units);
        typ_free_typdef(leaflist->typdef);
    }

    m__free(leaflist->descr);
    m__free(leaflist->ref);

    clean_mustQ(&leaflist->mustQ);

    m__free(leaflist);

}  /* free_leaflist */


/********************************************************************
* FUNCTION clone_leaflist
* 
* Clone a leaf-list struct
*
* INPUTS:
*    leaflist == obj_leaflist_t data structure to clone
*    mleaflist == obj_refine_t data structure to merge
*            into the clone, as refinements.  Only
*            legal leaf-list refinements will be checked
*            (May be NULL)
*
* RETURNS:
*   pointer to malloced object clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_leaflist_t *
    clone_leaflist (obj_leaflist_t *leaflist,
                    obj_refine_t *mleaflist)
{
    obj_leaflist_t      *newleaflist;
    status_t             res;

    newleaflist = new_leaflist(FALSE);
    if (!newleaflist) {
        return NULL;
    }

    /* set the fields that cannot be refined */
    newleaflist->name = leaflist->name;
    newleaflist->units = leaflist->units;
    newleaflist->typdef = leaflist->typdef;
    newleaflist->ordersys = leaflist->ordersys;
    newleaflist->status = leaflist->status;

    if (mleaflist && mleaflist->descr) {
        newleaflist->descr = xml_strdup(mleaflist->descr);
        if (!newleaflist->descr) {
            free_leaflist(newleaflist, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (leaflist->descr) {
        newleaflist->descr = xml_strdup(leaflist->descr);
        if (!newleaflist->descr) {
            free_leaflist(newleaflist, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mleaflist && mleaflist->ref) {
        newleaflist->ref = xml_strdup(mleaflist->ref);
        if (!newleaflist->ref) {
            free_leaflist(newleaflist, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (leaflist->ref) {
        newleaflist->ref = xml_strdup(leaflist->ref);
        if (!newleaflist->ref) {
            free_leaflist(newleaflist, OBJ_FL_CLONE);
            return NULL;
        }
    }

    res = clone_mustQ(&newleaflist->mustQ, &leaflist->mustQ,
                     (mleaflist) ? &mleaflist->mustQ : NULL);
    if (res != NO_ERR) {
        free_leaflist(newleaflist, OBJ_FL_CLONE);
        return NULL;
    }

    if (mleaflist && mleaflist->minelems_tkerr.mod) {
        newleaflist->minelems = mleaflist->minelems;
        newleaflist->minset = TRUE;
    } else {
        newleaflist->minelems = leaflist->minelems;
        newleaflist->minset = leaflist->minset;
    }

    if (mleaflist && mleaflist->maxelems_tkerr.mod) {
        newleaflist->maxelems = mleaflist->maxelems;
        newleaflist->maxset = TRUE;
    } else {
        newleaflist->maxelems = leaflist->maxelems;
        newleaflist->maxset = leaflist->maxset;
    }

    return newleaflist;

}  /* clone_leaflist */

/********************************************************************
 * Free a key Q.
 *
 * \param keyQ the queue to free.
 ********************************************************************/
static void free_keyQ( dlq_hdr_t* keyQ )
{
    if ( !keyQ ) {
        return;
    }

    while (!dlq_empty(keyQ)) {
        obj_key_t *key = (obj_key_t *)dlq_deque(keyQ);
        obj_free_key(key);
    }
}

/********************************************************************
 * Free a unique Q.
 *
 * \param keyQ the queue to free.
 ********************************************************************/
static void free_uniqueQ( dlq_hdr_t* uniqueQ )
{
    if ( !uniqueQ ) {
        return;
    }

    while (!dlq_empty(uniqueQ)) {
        obj_unique_t *uni = (obj_unique_t *)dlq_deque(uniqueQ);
        obj_free_unique(uni);
    }
}

/********************************************************************
 * Scrub the memory in a obj_list_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param list obj_list_t data structure to free
 * \param flags flags field from object freeing this list
 *********************************************************************/
static void free_list( obj_list_t *list, uint32 flags) 
{ 
    if ( !list ) {
        return;
    }

    boolean notclone = (flags & OBJ_FL_CLONE) ? FALSE : TRUE;

    m__free(list->descr);
    m__free(list->ref);
    free_keyQ( &list->keyQ );
    free_uniqueQ( &list->uniqueQ );

    if (notclone) {
        m__free(list->name);
        m__free(list->keystr);
        typ_clean_typeQ(list->typedefQ);
        dlq_destroyQue(list->typedefQ);

        grp_clean_groupingQ(list->groupingQ);
        dlq_destroyQue(list->groupingQ);
    }

    if ( !list->datadefclone ) {
        obj_clean_datadefQ( list->datadefQ );
        dlq_destroyQue( list->datadefQ );
    }

    clean_mustQ(&list->mustQ);
    m__free(list);
}  /* free_list */


/********************************************************************
* FUNCTION new_list
* 
* Malloc and initialize the fields in a an obj_list_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_list_t * 
    new_list (boolean isreal)
{
    obj_list_t  *list;

    list = m__getObj(obj_list_t);
    if (!list) {
        return NULL;
    }
    (void)memset(list, 0x0, sizeof(obj_list_t));


    dlq_createSQue(&list->keyQ);
    dlq_createSQue(&list->uniqueQ);

    list->status = NCX_STATUS_CURRENT;
    list->ordersys = TRUE;

    if (isreal) {
        list->typedefQ = dlq_createQue();
        if (!list->typedefQ) {
            m__free(list);
            return NULL;
        }

        list->groupingQ = dlq_createQue();
        if (!list->groupingQ) {
            dlq_destroyQue(list->typedefQ);
            m__free(list);
            return NULL;
        }

    }

    dlq_createSQue(&list->mustQ);

    list->datadefQ = dlq_createQue();
    if (!list->datadefQ) {
        free_list(list, (isreal) ? 0U : OBJ_FL_CLONE);
        return NULL;
    }

    return list;

}  /* new_list */


/********************************************************************
* FUNCTION clone_list
* 
* Clone a leaf-list struct
*
* INPUTS:
*    mod == module owner of the cloned data
*    newparent == new obj containing 'list'
*    srclist == obj_template_t data structure to clone
*    mlist == obj_refine_t data structure to merge
*            into the clone, as refinements.  Only
*            legal list refinements will be checked
*            (May be NULL)
*    mobjQ == Q of objects with OBJ_TYP_REFINE to apply
*
* RETURNS:
*   pointer to malloced object clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_list_t *
    clone_list (ncx_module_t *mod,
                obj_template_t *newparent,
                obj_template_t *srclist,
                obj_refine_t *mlist,
                dlq_hdr_t *mobjQ)
{
    obj_list_t      *list, *newlist;
    status_t         res;

    newlist = new_list(FALSE);
    if (!newlist) {
        return NULL;
    }

    list = srclist->def.list;

    /* set the fields that cannot be refined */
    newlist->name = list->name;
    newlist->keystr = list->keystr;
    newlist->typedefQ = list->typedefQ;
    newlist->groupingQ = list->groupingQ;
    newlist->ordersys = list->ordersys;
    newlist->status = list->status;

    if (mlist && mlist->descr) {
        newlist->descr = xml_strdup(mlist->descr);
        if (!newlist->descr) {
            free_list(newlist, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (list->descr) {
        newlist->descr = xml_strdup(list->descr);
        if (!newlist->descr) {
            free_list(newlist, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mlist && mlist->ref) {
        newlist->ref = xml_strdup(mlist->ref);
        if (!newlist->ref) {
            free_list(newlist, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (list->ref) {
        newlist->ref = xml_strdup(list->ref);
        if (!newlist->ref) {
            free_list(newlist, OBJ_FL_CLONE);
            return NULL;
        }
    }

    res = clone_mustQ(&newlist->mustQ, 
                      &list->mustQ,
                      (mlist) ? &mlist->mustQ : NULL);
    if (res != NO_ERR) {
        free_list(newlist, OBJ_FL_CLONE);
        return NULL;
    }

    if (mlist && mlist->minelems_tkerr.mod) {
        newlist->minelems = mlist->minelems;
        newlist->minset = TRUE;
    } else {
        newlist->minelems = list->minelems;
        newlist->minset = list->minset;
    }

    if (mlist && mlist->maxelems_tkerr.mod) {
        newlist->maxelems = mlist->maxelems;
        newlist->maxset = TRUE;
    } else {
        newlist->maxelems = list->maxelems;
        newlist->maxset = list->maxset;
    }

    res = clone_datadefQ(mod, 
                         newlist->datadefQ, 
                         list->datadefQ, 
                         mobjQ, 
                         newparent);
    if (res != NO_ERR) {
        free_list(newlist, OBJ_FL_CLONE);
        return NULL;
    }

    /* newlist->keyQ is still empty
     * newlist->uniqueQ is still empty
     * these are filled in by the yang_obj_resolve_final function
     */
    return newlist;

}  /* clone_list */


/********************************************************************
* FUNCTION new_case
* 
* Malloc and initialize the fields in a an obj_case_t
*
* INPUTS:
*    isreal == TRUE if this is for a real object
*            == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_case_t * 
    new_case (boolean isreal)
{
    obj_case_t  *cas;

    cas = m__getObj(obj_case_t);
    if (!cas) {
        return NULL;
    }
    (void)memset(cas, 0x0, sizeof(obj_case_t));

    cas->status = NCX_STATUS_CURRENT;

    if (isreal) {
        cas->datadefQ = dlq_createQue();
        if (!cas->datadefQ) {
            m__free(cas);
            return NULL;
        }
    }

    return cas;

}  /* new_case */


/********************************************************************
* Clean and free the fields in a an obj_case_t, then free 
* the case struct
*
* \param cas the case struct to free
*********************************************************************/
static void free_case (obj_case_t *cas)
{
    if ( !cas ) {
        return;
    }

    if (!cas->nameclone ) {
        m__free(cas->name);
    }

    m__free(cas->descr);
    m__free(cas->ref);

    if (!cas->datadefclone) {
        obj_clean_datadefQ(cas->datadefQ);
        dlq_destroyQue(cas->datadefQ);
    }
    m__free(cas);
}  /* free_case */

/********************************************************************
* FUNCTION new_choice
* 
* Malloc and initialize the fields in a an obj_choice_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_choice_t * 
    new_choice (boolean isreal)
{
    obj_choice_t  *ch;

    ch = m__getObj(obj_choice_t);
    if (!ch) {
        return NULL;
    }
    (void)memset(ch, 0x0, sizeof(obj_choice_t));

    ch->caseQ = dlq_createQue();
    if (!ch->caseQ) {
        m__free(ch);
        return NULL;
    }

    if (isreal) {
        ch->status = NCX_STATUS_CURRENT;
    }

    return ch;

}  /* new_choice */


/********************************************************************
 * Scrub the memory in a obj_choice_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param choic obj_choice_t data structure to free
 * \param flags flags field from object freeing this choice
 *********************************************************************/
static void free_choice (obj_choice_t *choic, uint32 flags)
{
    if ( !choic ) {
        return;
    }

    boolean notclone = (flags & OBJ_FL_CLONE) ? FALSE : TRUE;

    if (notclone) {
        m__free(choic->name);
    }

    m__free(choic->defval);
    m__free(choic->descr);
    m__free(choic->ref);

    if ( !choic->caseQclone ) {
        obj_clean_datadefQ(choic->caseQ);
        dlq_destroyQue(choic->caseQ);
    }

    m__free(choic);
}  /* free_choice */


/********************************************************************
* FUNCTION clone_choice
* 
* Clone a choice struct
*
* INPUTS:
*    mod == module owner of the cloned data
*    choic == obj_choice_t data structure to clone
*    mchoic == obj_choice_t data structure to merge
*            into the clone, as refinements.  Only
*            legal choice refinements will be checked
*            (May be NULL)
*    obj == parent object containing 'choic'
*    mobjQ == Q with OBJ_TYP_REFINE nodes to check
*
* RETURNS:
*   pointer to malloced object clone
*   NULL if  malloc error or internal error
*********************************************************************/
static obj_choice_t *
    clone_choice (ncx_module_t *mod,
                  obj_choice_t *choic,
                  obj_refine_t *mchoic,
                  obj_template_t *obj,
                  dlq_hdr_t  *mobjQ)
{
    obj_choice_t    *newchoic;
    status_t         res;

    newchoic = new_choice(FALSE);
    if (!newchoic) {
        return NULL;
    }

    /* set the fields that cannot be refined */
    newchoic->name = choic->name;
    newchoic->status = choic->status;

    if (mchoic && mchoic->def) {
        newchoic->defval = xml_strdup(mchoic->def);
        if (!newchoic->defval) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (choic->defval) {
        newchoic->defval = xml_strdup(choic->defval);
        if (!newchoic->defval) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mchoic && mchoic->descr) {
        newchoic->descr = xml_strdup(mchoic->descr);
        if (!newchoic->descr) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (choic->descr) {
        newchoic->descr = xml_strdup(choic->descr);
        if (!newchoic->descr) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    }

    if (mchoic && mchoic->ref) {
        newchoic->ref = xml_strdup(mchoic->ref);
        if (!newchoic->ref) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    } else if (choic->ref) {
        newchoic->ref = xml_strdup(choic->ref);
        if (!newchoic->ref) {
            free_choice(newchoic, OBJ_FL_CLONE);
            return NULL;
        }
    }

    res = clone_datadefQ(mod, 
                         newchoic->caseQ,
                         choic->caseQ, 
                         mobjQ, 
                         obj);
    if (res != NO_ERR) {
        free_choice(newchoic, OBJ_FL_CLONE);
        return NULL;
    }

    return newchoic;

}  /* clone_choice */


/********************************************************************
* FUNCTION new_uses
* 
* Malloc and initialize the fields in a obj_uses_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_uses_t * 
    new_uses (boolean isreal)
{
    obj_uses_t  *us;

    us = m__getObj(obj_uses_t);
    if (!us) {
        return NULL;
    }
    (void)memset(us, 0x0, sizeof(obj_uses_t));

    if (isreal) {
        us->status = NCX_STATUS_CURRENT;   /* default */
    }

    us->datadefQ = dlq_createQue();
    if (!us->datadefQ) {
        m__free(us);
        return NULL;
    }

    return us;

}  /* new_uses */


/********************************************************************
 * Scrub the memory in a obj_uses_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param us the obj_uses_t data structure to free
 *********************************************************************/
static void free_uses (obj_uses_t *us)
{
    if ( !us ) {
        return;
    }

    m__free(us->prefix);
    m__free(us->name);
    m__free(us->descr);
    m__free(us->ref);

    obj_clean_datadefQ(us->datadefQ);
    dlq_destroyQue(us->datadefQ);

    m__free(us);
}  /* free_uses */


/********************************************************************
* FUNCTION new_refine
* 
* Malloc and initialize the fields in a obj_refine_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_refine_t * 
    new_refine (void)
{
    obj_refine_t  *refi;

    refi = m__getObj(obj_refine_t);
    if (!refi) {
        return NULL;
    }
    (void)memset(refi, 0x0, sizeof(obj_refine_t));

    dlq_createSQue(&refi->mustQ);

    return refi;

}  /* new_refine */


/********************************************************************
 * Scrub the memory in a obj_refine_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param refi == obj_refine_t data structure to free
 *********************************************************************/
static void free_refine (obj_refine_t *refi)
{    
    if ( !refi ) {
        return;
    }

    m__free(refi->target);
    m__free(refi->descr);
    m__free(refi->ref);
    m__free(refi->presence);
    m__free(refi->def);

    clean_mustQ(&refi->mustQ);

    m__free(refi);

}  /* free_refine */


/********************************************************************
* FUNCTION new_augment
* 
* Malloc and initialize the fields in a obj_augment_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_augment_t * 
    new_augment (boolean isreal)
{
    obj_augment_t  *aug;

    aug = m__getObj(obj_augment_t);
    if (!aug) {
        return NULL;
    }
    (void)memset(aug, 0x0, sizeof(obj_augment_t));

    dlq_createSQue(&aug->datadefQ);

    if (isreal) {
        aug->status = NCX_STATUS_CURRENT;
    }

    return aug;

}  /* new_augment */


/********************************************************************
 * Scrub the memory in a obj_augment_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param aug the obj_augment_t data structure to free
 *********************************************************************/
static void free_augment (obj_augment_t *aug)
{
    if ( !aug ) {
        return;
    }

    m__free(aug->target);
    m__free(aug->descr);
    m__free(aug->ref);
    obj_clean_datadefQ(&aug->datadefQ);

    m__free(aug);
}  /* free_augment */


/********************************************************************
* FUNCTION new_rpc
* 
* Malloc and initialize the fields in a an obj_rpc_t
*
* RETURNS:
*    pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_rpc_t * 
    new_rpc (void)
{
    obj_rpc_t  *rpc;

    rpc = m__getObj(obj_rpc_t);
    if (!rpc) {
        return NULL;
    }
    (void)memset(rpc, 0x0, sizeof(obj_rpc_t));

    dlq_createSQue(&rpc->typedefQ);
    dlq_createSQue(&rpc->groupingQ);
    dlq_createSQue(&rpc->datadefQ);
    rpc->status = NCX_STATUS_CURRENT;

    /* by default: set supported to true for
     * agent simulation mode; there will not be any
     * callbacks to load, but RPC message
     * processing based on the template will be done
     */
    rpc->supported = TRUE;

    return rpc;

}  /* new_rpc */


/********************************************************************
 * Clean and free the fields in a an obj_rpc_t, then free 
 * the RPC struct
 *
 * \param rpc the RPC struct to free
 *********************************************************************/
static void free_rpc (obj_rpc_t *rpc)
{
    if ( !rpc ) {
        return;
    }

    m__free(rpc->name);
    m__free(rpc->descr);
    m__free(rpc->ref);

    typ_clean_typeQ(&rpc->typedefQ);
    grp_clean_groupingQ(&rpc->groupingQ);
    obj_clean_datadefQ(&rpc->datadefQ);

    m__free(rpc);
}  /* free_rpc */


/********************************************************************
* FUNCTION new_rpcio
* 
* Malloc and initialize the fields in a an obj_rpcio_t
* Fields are setup within the new obj_template_t, based
* on the values in rpcobj
*
* RETURNS:
*    pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_rpcio_t * 
    new_rpcio (void)
{
    obj_rpcio_t  *rpcio;

    rpcio = m__getObj(obj_rpcio_t);
    if (!rpcio) {
        return NULL;
    }
    (void)memset(rpcio, 0x0, sizeof(obj_rpcio_t));

    dlq_createSQue(&rpcio->typedefQ);
    dlq_createSQue(&rpcio->groupingQ);
    dlq_createSQue(&rpcio->datadefQ);
    return rpcio;

}  /* new_rpcio */


/********************************************************************
 * Clean and free the fields in a an obj_rpcio_t, then free 
 * the RPC IO struct
 *
 * \param rpcio the RPC IO struct to free
 *********************************************************************/
static void free_rpcio (obj_rpcio_t *rpcio)
{
    if ( !rpcio ) {
        return;
    }

    m__free(rpcio->name);
    typ_clean_typeQ(&rpcio->typedefQ);
    grp_clean_groupingQ(&rpcio->groupingQ);
    obj_clean_datadefQ(&rpcio->datadefQ);
    m__free(rpcio);
}  /* free_rpcio */


/********************************************************************
* FUNCTION new_notif
* 
* Malloc and initialize the fields in a an obj_notif_t
*
* RETURNS:
*    pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
static obj_notif_t * 
    new_notif (void)
{
    obj_notif_t  *notif;

    notif = m__getObj(obj_notif_t);
    if (!notif) {
        return NULL;
    }
    (void)memset(notif, 0x0, sizeof(obj_notif_t));

    dlq_createSQue(&notif->typedefQ);
    dlq_createSQue(&notif->groupingQ);
    dlq_createSQue(&notif->datadefQ);
    notif->status = NCX_STATUS_CURRENT;
    return notif;

}  /* new_notif */


/********************************************************************
 * Clean and free the fields in a an obj_notif_t, then free 
 * the notification struct
 *
 * \param notif == notification struct to free
 *********************************************************************/
static void free_notif (obj_notif_t *notif)
{
    if ( !notif ) {
        return;
    }

    m__free(notif->name);
    m__free(notif->descr);
    m__free(notif->ref);
    typ_clean_typeQ(&notif->typedefQ);
    grp_clean_groupingQ(&notif->groupingQ);
    obj_clean_datadefQ(&notif->datadefQ);
    m__free(notif);
}  /* free_notif */

/********************************************************************/
/** 
 * utility function to perform full or partial case sensitive /
 * insenstive string comparison.
 *
 * \param objname object name to find
 * \param curname object name being matched
 * \param partialmatch flag idicating if a partial match is allowed.
 * \param usecase TRUE if case-sensitive FALSE if case-insensitive 
 * \param len_objname the length of the object name.
 * \return true if the names match
 */
static bool compare_names( const xmlChar *objname,
                           const xmlChar *curname,
                           boolean partialmatch,
                           boolean usecase,
                           uint32 len_objname )
{
    int ret = 0;

    if (partialmatch) {
        if (usecase) {
            ret = xml_strncmp(objname, curname, len_objname );
        } else {
            ret = xml_strnicmp(objname, curname, len_objname );
        }
    } else {
        if (usecase) {
            ret = xml_strcmp(objname, curname);
        } else {
            ret = xml_stricmp(objname, curname);
        }
    }

    return ( ret == 0 );
} /* compare_names */

/********************************************************************/
/** 
 * utility function to search a case object
 *
 * \param obj the parent object
 * \param modname module name that defines the obj_template_t
 *                ( NULL and first match will be done, and the
   *               module ignored (Name instead of QName )
 * \param objname object name to find
 * \param lookdeep TRUE to check objects inside choices/cases and match these 
 *                 nodes before matching the choice or case
 * \param partialmatch TRUE if a strncmp (partial match ) is reqquried.
 * \param usecase == TRUE if case-sensitive
 *               FALSE if case-insensitive 
 * \param altnames == TRUE if altnames allowed
 *                FALSE if normal names only
 * \param dataonly == TRUE to check just data nodes
 *               FALSE to check all nodes
 * \param namematch flag indicating if the current objects name is a match
 * \param matchcount == address of return parameter match count
 * \return pointer to obj_template_t or NULL if not found in 'que'
 */
static obj_template_t* search_case( obj_template_t* obj,
        const xmlChar *modname,
        const xmlChar *objname,
        boolean lookdeep,
        boolean partialmatch,
        boolean usecase,
        boolean altnames,
        boolean dataonly,
        boolean nameMatch,
        uint32 *matchcount,
        obj_template_t** partialMatchedObj )
{
    obj_case_t* cas = obj->def.cas;
    obj_template_t* chObj  = find_template( cas->datadefQ, modname, objname,
                              lookdeep, partialmatch, usecase, altnames, 
                              dataonly, matchcount );
    if ( chObj ) {
        if (partialmatch) {
            if ( !(*partialMatchedObj ) ) {
                *partialMatchedObj = chObj;
            }
        } else {
            return chObj;
        }
    }

    /* last try: the choice name itself */
    if ( nameMatch && !partialmatch ) {
        return obj;
    }

    return NULL;
} /* search_case */

/********************************************************************/
/** 
 * utility function to search a choice object
 
 * \param obj the parent object
 * \param modname module name that defines the obj_template_t
 *                ( NULL and first match will be done, and the
   *               module ignored (Name instead of QName )
 * \param objname object name to find
 * \param lookdeep TRUE to check objects inside choices/cases and match 
 *                 these nodes before matching the choice or case
 * \param partialmatch TRUE if a strncmp (partial match ) is reqquried.
 * \param usecase == TRUE if case-sensitive
 *               FALSE if case-insensitive 
 * \param altnames == TRUE if altnames allowed
 *                FALSE if normal names only
 * \param dataonly == TRUE to check just data nodes
 *               FALSE to check all nodes
 * \param namematch flag indicating if the current objects name is a match
 * \param matchcount == address of return parameter match count
 * \return pointer to obj_template_t or NULL if not found in 'que'
 */
static obj_template_t* search_choice( obj_template_t* obj,
        const xmlChar *modname,
        const xmlChar *objname,
        boolean lookdeep,
        boolean partialmatch,
        boolean usecase,
        boolean altnames,
        boolean dataonly,
        boolean nameMatch,
        uint32 *matchcount,
        obj_template_t** partialMatchedObj )
{
    obj_template_t* casobj = 
        (obj_template_t*) dlq_firstEntry(obj->def.choic->caseQ);
    obj_template_t* chObj;

    for ( ; casobj; casobj = (obj_template_t *)dlq_nextEntry(casobj)) {
        chObj = search_case( casobj, modname, objname, lookdeep, partialmatch,
               usecase, altnames, dataonly, nameMatch, matchcount, 
               partialMatchedObj );
        if ( chObj ) {
            if ( partialmatch ) {
                if ( !(*partialMatchedObj) ) {
                    *partialMatchedObj = chObj;
                }
            } 
            else {
                return chObj;
            }
        }
    }

    /* last try: the choice name itself */
    if ( nameMatch && !partialmatch ) {
        return obj;
    }

    return NULL;
} /* search choice */

/********************************************************************
 * Check if an object is a CHOICE or CASE.
 *
 * \param obj the object to check
 * \return true of the object is a CHOICE or CASE.
 */
static boolean  obj_is_choice_or_case( obj_template_t* obj )
{
    return ( obj->objtype == OBJ_TYP_CHOICE || obj->objtype == OBJ_TYP_CASE );
}

/********************************************************************/
/** 
 * Utility function for managing partial matches. Increment the count
 * of partial matches and set the partial match object if it is NULL, 
 * this ensures that the first partial match is used.
 *
 * \param obj the object being matched
 * \param nameMatch flag indicating if the name matched
 * \param partialmatch flag indicating if a partial match is allowed
 * \param lookdeep flag indicating if look deep is set
 * \param matchcount address of return parameter match count
 * \param partialMatchedObj the partial matched object 
 * \return a matching object or NULL,
 */
static obj_template_t* handle_partial_match_found( 
        obj_template_t* obj, 
        boolean nameMatch,
        boolean partialmatch,
        boolean lookdeep,
        uint32 *matchcount, 
        obj_template_t** partialMatchedObj )
{
    if ( nameMatch ) {
        if ( !partialmatch ) {
            return obj;
         }
         else {
             if ( lookdeep || !obj_is_choice_or_case( obj ) ) {
                 ++(*matchcount);
                 if ( !(*partialMatchedObj) ) {
                     *partialMatchedObj = obj;
                 }
             }
         }
    }
    return NULL;
}

/********************************************************************/
/** 
 * Find an object with the specified name
 *
 * \param que Q of obj_template_t to search
 * \param modname module name that defines the obj_template_t
 *                ( NULL and first match will be done, and the
   *               module ignored (Name instead of QName )
 * \param objname object name to find
 * \param lookdeep TRUE to check objects inside choices/cases and match 
 *                 these nodes before matching the choice or case
 * \param partialmatch TRUE if a strncmp (partial match ) is reqquried.
 * \param usecase == TRUE if case-sensitive
 *               FALSE if case-insensitive 
 * \param altnames == TRUE if altnames allowed
 *                FALSE if normal names only
 * \param dataonly == TRUE to check just data nodes
 *               FALSE to check all nodes
 * \param matchcount == address of return parameter match count
 * \return pointer to obj_template_t or NULL if not found in 'que'
 *********************************************************************/
static obj_template_t* find_template (dlq_hdr_t  *que,
                   const xmlChar *modname,
                   const xmlChar *objname,
                   boolean lookdeep,
                   boolean partialmatch,
                   boolean usecase,
                   boolean altnames,
                   boolean dataonly,
                   uint32 *matchcount  )
{
    obj_template_t *partialMatchedObj = NULL;
    obj_template_t *matchedObj = NULL;
    const xmlChar *curname;         // the name of the current obj being checked
    const xmlChar *curmodulename;   // the name of the current mod being checked
    bool nameMatch;

    uint32 len_objname = ( partialmatch ?  xml_strlen(objname) : 0 );
    *matchcount = 0;

    /* check all the objects in this datadefQ */
    obj_template_t* obj = (obj_template_t *)dlq_firstEntry(que);
    for( ; obj; obj = (obj_template_t *)dlq_nextEntry(obj) ) {
        /* skip augment and uses */
        if (!obj_has_name(obj) || !obj_is_enabled(obj)) {
            continue;
        }

        /* skip rpc and notifications for dataonly matching */
        if (dataonly && (obj_is_rpc(obj) || obj_is_notif(obj)) ) { 
            continue;
        }
        
        /* skip objects with no name / altname */
        curname = (altnames ?  obj_get_altname(obj) : obj_get_name(obj) );
        if ( !curname ) {
            continue;
        }

        curmodulename = obj_get_mod_name(obj);
        nameMatch = compare_names( objname, curname, partialmatch, usecase, 
                                   len_objname );

        if (!lookdeep) {
            /* if lookdeep == FALSE then check the choice name of an 
             * OBJ_TYP_CHOICE or OBJ_TYP_CASE object. */

            /* if the module does not match the current module name, skip it */
            if ( modname && xml_strcmp(modname, curmodulename) ) {
                continue;
            }

            matchedObj = handle_partial_match_found( obj, nameMatch, 
                    partialmatch, lookdeep, matchcount, &partialMatchedObj );
            if ( matchedObj ) {
                return matchedObj;
            }
        }

        switch ( obj->objtype ) {
        case OBJ_TYP_CHOICE:
            /* since the choice and case layers disappear, need to check if any 
             * real node names would clash will also check later that all choice
             * nodes within the same sibling set do not clash either */
            matchedObj = search_choice( obj, modname, objname, lookdeep, 
                    partialmatch, usecase, altnames, dataonly, nameMatch, 
                    matchcount, &partialMatchedObj );
            if ( matchedObj ) {
                return matchedObj;
            }
            break;

        case OBJ_TYP_CASE:
            matchedObj = search_case( obj, modname, objname, lookdeep, 
                    partialmatch, usecase, altnames, dataonly, nameMatch, 
                    matchcount, &partialMatchedObj );
            if ( matchedObj ) {
                return matchedObj;
            }
            break;

        default:
            /* check if a specific module name requested,
             * and if so, skip any object not from that module
             */
            if (lookdeep) {
                if (modname && xml_strcmp(modname, curmodulename)) {
                    continue;
                }
                matchedObj = handle_partial_match_found( obj, nameMatch, 
                     partialmatch, lookdeep, matchcount, &partialMatchedObj );
                if ( matchedObj ) {
                    return matchedObj;
                }
            }
        }
    }

    if (partialmatch) {
        return partialMatchedObj;
    } 

    return NULL;
}  /* find_template */


/********************************************************************
* FUNCTION get_config_flag
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*   setflag == address of return config-stmt set flag
*
* OUTPUTS:
*   *setflag == TRUE if the config-stmt is set in this
*               node, or if it is a top-level object
*            == FALSE if the config-stmt is inherited from its parent
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
static boolean
    get_config_flag (const obj_template_t *obj,
                     boolean *setflag)
{
    switch (obj->objtype) {
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
        if (obj_is_root(obj)) {
            *setflag = TRUE;
            return TRUE;
        } else if ((obj->parent && 
                    !obj_is_root(obj->parent)) || obj->grp) {
            *setflag = (obj->flags & OBJ_FL_CONFSET) 
                ? TRUE : FALSE;
        } else {
            *setflag = TRUE;
        }
        return (obj->flags & OBJ_FL_CONFIG) ? TRUE : FALSE;
    case OBJ_TYP_CASE:
        *setflag = FALSE;
        if (obj->parent) {
            return (obj->parent->flags & OBJ_FL_CONFIG)
                ? TRUE : FALSE;
        } else {
            /* should not happen */
            return FALSE;
        }
        /*NOTREACHED*/
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        /* no real setting -- not applicable */
        *setflag = FALSE;
        return FALSE;
    case OBJ_TYP_RPC:
        /* no real setting for this, but has to be true
         * to allow rpc/input to be true
         */
        *setflag = FALSE;
        return TRUE;
    case OBJ_TYP_RPCIO:
        *setflag = FALSE;
        if (!xml_strcmp(obj->def.rpcio->name, YANG_K_INPUT)) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;
    case OBJ_TYP_NOTIF:
        *setflag = FALSE;
        return FALSE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    /*NOTREACHED*/

}   /* get_config_flag */


/********************************************************************
* FUNCTION get_object_string
* 
* Generate the object identifier string
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   stopobj == ancestor node to treat as root (may be NULL)
*   buff == buffer to use (may be NULL)
*   bufflen == length of buffer to use (0 to ignore check)
*   normalmode == TRUE for a real Xpath OID
*              == FALSE to generate a big element name to use
*                 for augment substitutionGroup name generation
*   mod == module in progress for C code generation
*       == NULL for other purposes
*   retlen == address of return length
*   withmodname == TRUE if force modname in string
*   forcexpath == TRUE to generate a value tree absolute path expr
*                 FALSE to use normalnode var to determine mode
* OUTPUTS
*   *retlen == length of object identifier string
*   if buff non-NULL:
*      *buff filled in with string
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    get_object_string (const obj_template_t *obj,
                       const obj_template_t *stopobj,
                       xmlChar  *buff,
                       uint32 bufflen,
                       boolean normalmode,
                       ncx_module_t *mod,
                       uint32 *retlen,
                       boolean withmodname,
                       boolean forcexpath)
{
    *retlen = 0;

    boolean addmodname = withmodname || forcexpath;
    boolean topnode = FALSE;

    if (obj->parent && 
        obj->parent != stopobj && 
        !obj_is_root(obj->parent)) {
        status_t res = get_object_string(obj->parent, stopobj, buff, bufflen, 
                                         normalmode, mod, retlen, 
                                         withmodname, forcexpath);
        if (res != NO_ERR) {
            return res;
        }
    } else {
        topnode = TRUE;
    }

    if (!obj_has_name(obj)) {
        /* should not enounter a uses or augment!! */
        return NO_ERR;
    }

    if (forcexpath && (obj->objtype == OBJ_TYP_CHOICE ||
                       obj->objtype == OBJ_TYP_CASE)) {
        return NO_ERR;
    }

    const xmlChar *modname = NULL;
    uint32 modnamelen = 0;

    if (forcexpath) {
        modname = xmlns_get_ns_prefix(obj_get_nsid(obj));
        if (!modname) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        modnamelen = xml_strlen(modname);
    } else {
        modname = obj_get_mod_name(obj);
        modnamelen = xml_strlen(modname);
    }

    if (!addmodname && mod != NULL &&
        (xml_strcmp(modname, ncx_get_modname(mod)))) {
        addmodname = TRUE;
    }

    /* get the name and check the added length */
    const xmlChar *name = obj_get_name(obj);
    uint32 namelen = xml_strlen(name), seplen = 1;

    if (addmodname) {
        if (bufflen &&
            ((*retlen + namelen + modnamelen + 2) >= bufflen)) {
            return ERR_BUFF_OVFL;
        }
    } else {
        if (bufflen && ((*retlen + namelen + 1) >= bufflen)) {
            return ERR_BUFF_OVFL;
        }
    }

    if (topnode && stopobj) {
        seplen = 0;
    }

    /* copy the name string recusively, letting the stack
     * keep track of the next child node to write 
     */
    if (buff) {
        /* node separator char */
        if (topnode && stopobj) {
            ;
        } else if (normalmode) {
            buff[*retlen] = '/';
        } else {
            buff[*retlen] = '.';
        }

        if (addmodname) {
            xml_strcpy(&buff[*retlen + seplen], modname);
            buff[*retlen + modnamelen + seplen] = 
                (forcexpath || withmodname) ? ':' : '_';
            xml_strcpy(&buff[*retlen + modnamelen + seplen + 1], name);
        } else {
            xml_strcpy(&buff[*retlen + seplen], name);
        }
    }
    if (addmodname) {
        *retlen += (namelen + modnamelen + seplen + 1);
    } else {
        *retlen += (namelen + seplen);
    }
    return NO_ERR;

}  /* get_object_string */


/********************************************************************
 * FUNCTION find_next_child
 * 
 * Check the instance qualifiers and see if the specified node
 * is a valid (subsequent) child node.
 *
 * Example:
 *  
 *  container foo { 
 *    leaf a { type int32; }
 *    leaf b { type int32; }
 *    leaf-list c { type int32; }
 *
 * Since a, b, and c are all optional, all of them have to be
 * checked, even while node 'a' is expected
 * The caller will save the current child in case the pointer
 * needs to be backed up.
 *
 * INPUTS:
 *   chobj == current child object template
 *   chnode == xml_node_t of start element to match
 *
 * RETURNS:
 *   pointer to child that matched or NULL if no valid next child
 *********************************************************************/
static obj_template_t *
    find_next_child (obj_template_t *chobj,
                     const xml_node_t *chnode)
{

    obj_template_t *chnext, *foundobj;
    status_t        res;

    chnext = chobj;

    for (;;) {
        switch (obj_get_iqualval(chnext)) {
        case NCX_IQUAL_ONE:
        case NCX_IQUAL_1MORE:
            /* the current child is mandatory; this is an error */
            return NULL;
            /* else fall through to next case */
        case NCX_IQUAL_OPT:
        case NCX_IQUAL_ZMORE:
            /* the current child is optional; keep trying
             * try to get the next child in the complex type 
             */
            chnext = obj_next_child(chnext);
            if (!chnext) {
                return NULL;
            } else {
                if ( obj_is_choice_or_case( chnext ) ) {
                    foundobj = obj_find_child(chnext,
                                              xmlns_get_module(chnode->nsid),
                                              chnode->elname);
                    if (foundobj && obj_is_choice_or_case( foundobj ) ) {
                        foundobj = NULL;
                    }
                    if (foundobj) {
                        return chnext;  /* not the nested foundobj! */
                    }
                } else {
                    res = xml_node_match(chnode,
                                         obj_get_nsid(chnext), 
                                         obj_get_name(chnext), 
                                         XML_NT_NONE);
                    if (res == NO_ERR) {
                        return chnext;
                    }
                }
            }
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
    }
    /*NOTREACHED*/

} /* find_next_child */


/********************************************************************
* FUNCTION process_one_walker_child
* 
* Process one child object node 
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    obj == object to process
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*              == NULL to match any child name
*    configonly = TRUE for config=true only
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    fncalled == address of return function called flag
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
static boolean
    process_one_walker_child (obj_walker_fn_t walkerfn,
                              void *cookie1,
                              void *cookie2,
                              obj_template_t  *obj,
                              const xmlChar *modname,
                              const xmlChar *childname,
                              boolean configonly,
                              boolean textmode,
                              boolean *fncalled)
                              
{
    boolean         fnresult;

    *fncalled = FALSE;
    if (!obj_has_name(obj)) {
        return TRUE;
    }

    if (configonly && !childname && 
        !obj_is_config(obj)) {
        return TRUE;
    }

    fnresult = TRUE;
    if (textmode) {
        if (obj_is_leafy(obj)) {
            fnresult = (*walkerfn)(obj, cookie1, cookie2);
            *fncalled = TRUE;
        }
    } else if (modname && childname) {
        if (!xml_strcmp(modname, 
                        obj_get_mod_name(obj)) &&
            !xml_strcmp(childname, obj_get_name(obj))) {

            fnresult = (*walkerfn)(obj, cookie1, cookie2);
            *fncalled = TRUE;
        }
    } else if (modname) {
        if (!xml_strcmp(modname, obj_get_mod_name(obj))) {
            fnresult = (*walkerfn)(obj, cookie1, cookie2);
            *fncalled = TRUE;
        }
    } else if (childname) {
        if (!xml_strcmp(childname, obj_get_name(obj))) {
            fnresult = (*walkerfn)(obj, cookie1, cookie2);
            *fncalled = TRUE;
        }
    } else {
        fnresult = (*walkerfn)(obj, cookie1, cookie2);
        *fncalled = TRUE;
    }

    return fnresult;

}  /* process_one_walker_child */


/********************************************************************
* FUNCTION test_one_child
* 
* The the specified node
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing the XPath object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    obj == node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of node to find
*              == NULL to match any name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
static boolean
    test_one_child (ncx_module_t *exprmod,
                    obj_walker_fn_t walkerfn,
                    void *cookie1,
                    void *cookie2,
                    obj_template_t *obj,
                    const xmlChar *modname,
                    const xmlChar *name,
                    boolean configonly,
                    boolean textmode)
{
    boolean               fnresult, fncalled;

    if ( obj_is_choice_or_case( obj ) ) {
        fnresult = obj_find_all_children(exprmod,
                                         walkerfn,
                                         cookie1,
                                         cookie2,
                                         obj,
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         FALSE);
    } else {
        fnresult = process_one_walker_child(walkerfn,
                                            cookie1,
                                            cookie2,
                                            obj,
                                            modname, 
                                            name,
                                            configonly,
                                            textmode,
                                            &fncalled);
    }

    if (!fnresult) {
        return FALSE;
    }

    return TRUE;

}  /* test_one_child */


/********************************************************************
* FUNCTION test_one_ancestor
* 
* The the specified node
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    obj == node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of node to find
*              == NULL to match any name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if axis ancestor-or-self
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
static boolean
    test_one_ancestor (ncx_module_t *exprmod,
                       obj_walker_fn_t walkerfn,
                       void *cookie1,
                       void *cookie2,
                       obj_template_t *obj,
                       const xmlChar *modname,
                       const xmlChar *name,
                       boolean configonly,
                       boolean textmode,
                       boolean orself,
                       boolean *fncalled)
{
    boolean               fnresult;

    if ( obj_is_choice_or_case( obj ) ) {
        fnresult = obj_find_all_ancestors(exprmod,
                                          walkerfn,
                                          cookie1,
                                          cookie2,
                                          obj,
                                          modname,
                                          name,
                                          configonly,
                                          textmode,
                                          FALSE,
                                          orself,
                                          fncalled);
    } else {
        fnresult = process_one_walker_child(walkerfn,
                                            cookie1,
                                            cookie2,
                                            obj,
                                            modname, 
                                            name,
                                            configonly,
                                            textmode,
                                            fncalled);
    }

    if (!fnresult) {
        return FALSE;
    }

    return TRUE;

}  /* test_one_ancestor */


/********************************************************************
* FUNCTION test_one_descendant
* 
* The the specified node
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startobj == node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of node to find
*              == NULL to match any name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if descendant-or-self test
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called

* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
static boolean
    test_one_descendant (ncx_module_t *exprmod,
                         obj_walker_fn_t walkerfn,
                         void *cookie1,
                         void *cookie2,
                         obj_template_t *startobj,
                         const xmlChar *modname,
                         const xmlChar *name,
                         boolean configonly,
                         boolean textmode,
                         boolean orself,
                         boolean *fncalled)
{
    obj_template_t *obj;
    dlq_hdr_t      *datadefQ;
    boolean         fnresult;

    if (orself) {
        fnresult = process_one_walker_child(walkerfn,
                                            cookie1,
                                            cookie2,
                                            startobj,
                                            modname, 
                                            name,
                                            configonly,
                                            textmode,
                                            fncalled);
        if (!fnresult) {
            return FALSE;
        }
    }

    datadefQ = obj_get_datadefQ(startobj);
    if (!datadefQ) {
        return TRUE;
    }

    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if ( obj_is_choice_or_case( obj ) ) {
            fnresult = obj_find_all_descendants(exprmod,
                                                walkerfn,
                                                cookie1,
                                                cookie2,
                                                obj,
                                                modname,
                                                name,
                                                configonly,
                                                textmode,
                                                FALSE,
                                                orself,
                                                fncalled);
        } else {
            fnresult = process_one_walker_child(walkerfn,
                                                cookie1,
                                                cookie2,
                                                obj,
                                                modname, 
                                                name,
                                                configonly,
                                                textmode,
                                                fncalled);
            if (fnresult && !*fncalled) {
                fnresult = obj_find_all_descendants(exprmod,
                                                    walkerfn,
                                                    cookie1,
                                                    cookie2,
                                                    obj,
                                                    modname,
                                                    name,
                                                    configonly,
                                                    textmode,
                                                    FALSE,
                                                    orself,
                                                    fncalled);
            }
        }
        if (!fnresult) {
            return FALSE;
        }
    }

    return TRUE;

}  /* test_one_descendant */


/********************************************************************
* FUNCTION test_one_pfnode
* 
* The the specified node
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    obj == node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    childname == name of child node to find
*              == NULL to match any child name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 1 level is checked
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    forward == TRUE: forward axis test
*               FALSE: reverse axis test
*    axis == axis enum to use
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
static boolean
    test_one_pfnode (ncx_module_t *exprmod,
                     obj_walker_fn_t walkerfn,
                     void *cookie1,
                     void *cookie2,
                     obj_template_t *obj,
                     const xmlChar *modname,
                     const xmlChar *name,
                     boolean configonly,
                     boolean dblslash,
                     boolean textmode,
                     boolean forward,
                     ncx_xpath_axis_t axis,
                     boolean *fncalled)
{
    obj_template_t *child;
    boolean         fnresult, needcont;

    /* for objects, need to let same node match, because
     * if there are multiple instances of the object,
     * it would match
     */
    if (obj->objtype == OBJ_TYP_LIST ||
        obj->objtype == OBJ_TYP_LEAF_LIST) {
        ;
    } else if (forward) {
        obj = (obj_template_t *)dlq_nextEntry(obj);
    } else {
        obj = (obj_template_t *)dlq_prevEntry(obj);
    }

    while (obj) {
        needcont = FALSE;

        if (!obj_has_name(obj)) {
            needcont = TRUE;
        }

        if (configonly && !name && !obj_is_config(obj)) {
            needcont = TRUE;
        }

        if (needcont) {
            /* get the next node to process */
            if (forward) {
                obj = (obj_template_t *)dlq_nextEntry(obj);
            } else {
                obj = (obj_template_t *)dlq_prevEntry(obj);
            }
            continue;
        }

        if ( obj_is_choice_or_case( obj ) ) {
            for (child = (forward) ? obj_first_child(obj) :
                     obj_last_child(obj);
                 child != NULL;
                 child = (forward) ? obj_next_child(child) :
                     obj_previous_child(child)) {

                fnresult = obj_find_all_pfaxis(exprmod,
                                               walkerfn,
                                               cookie1,
                                               cookie2,
                                               child,
                                               modname,
                                               name,
                                               configonly,
                                               dblslash,
                                               textmode,
                                               FALSE,
                                               axis,
                                               fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        } else {
            fnresult = process_one_walker_child(walkerfn,
                                                cookie1,
                                                cookie2,
                                                obj,
                                                modname,
                                                name,
                                                configonly,
                                                textmode,
                                                fncalled);
            if (!fnresult) {
                return FALSE;
            }

            if (!*fncalled && dblslash) {
                /* if /foo did not get added, than 
                 * try /foo/bar, /foo/baz, etc.
                 * check all the child nodes even if
                 * one of them matches, because all
                 * matches are needed with the '//' operator
                 */
                for (child = (forward) ?
                         obj_first_child(obj) :
                         obj_last_child(obj);
                     child != NULL;
                     child = (forward) ?
                         obj_next_child(child) :
                         obj_previous_child(child)) {

                    fnresult = 
                        obj_find_all_pfaxis(exprmod,
                                            walkerfn, 
                                            cookie1, 
                                            cookie2,
                                            child, 
                                            modname, 
                                            name, 
                                            configonly,
                                            dblslash,
                                            textmode,
                                            FALSE,
                                            axis,
                                            fncalled);
                    if (!fnresult) {
                        return FALSE;
                    }
                }
            }
        }

        /* get the next node to process */
        if (forward) {
            obj = (obj_template_t *)dlq_nextEntry(obj);
        } else {
            obj = (obj_template_t *)dlq_prevEntry(obj);
        }

    }
    return TRUE;

}  /* test_one_pfnode */


/********************************************************************/
/**
 * Check if an obj_template_t in the mod->datadefQ or any
 * of the include files visible to this module
 *
 * Top-level access is not tracked, so the 'lookdeep' variable
 * is hard-wired to FALSE
 *
 * \param mod ncx_module to check
 * \param modname module name for the object (needed for augments) 
 *                (may be NULL to match any 'objname' instance)
 * \param objname name of the object name to find
 * \param match TRUE to partial-length match node names 
 *              FALSE to find exact-length match only
 * \param altnames TRUE if alt-name should be checked
 * \param usecase TRUE if case-sensitive, FALSE if allow case-insensitive match
 * \param onematch TRUE if only 1 match allowed FALSE if first match of 
 *                 N allowed
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \return pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* find_template_top_in_submodules_includes( 
        ncx_module_t *mod,
        const xmlChar *modname,
        const xmlChar *objname,
        boolean match,
        boolean altnames,
        boolean usecase,
        boolean dataonly,
        uint32 *matchcount ) 
{
    obj_template_t *obj;
    
    /* Q of all includes this [sub]module has seen */
    dlq_hdr_t *que = ncx_get_allincQ(mod);
    ncx_include_t  *inc;
    yang_node_t    *node;

    /* check all the submodules, visible to this module or submodule */
    for (inc = (ncx_include_t *)dlq_firstEntry(&mod->includeQ);
         inc;
         inc = (ncx_include_t *)dlq_nextEntry(inc)) {

        /* get the real submodule struct */
        if ( !inc->submod ) {
            node = yang_find_node(que, inc->submodule, inc->revision);

            if( !node || !node->submod ) {
                /* include not found, skip this one */
                continue;
            }
            inc->submod = node->submod;
        }

        /* check the type Q in this submodule */
        obj = find_template( &inc->submod->datadefQ, modname, objname, FALSE,
                             match, usecase, altnames, dataonly, matchcount );
        if (obj) {
            return obj;
        }
    }
    return NULL;
}


/********************************************************************/
/**
 * Handle flags for multiple matches, setting the return status and
 * return value accordingly.
 *
 * If the find request specified single full length match and multiple
 * matches were found, set the return status to
 * ERR_NCX_MULTIPLE_MATCHES and return NULL, otherwise return the
 * supplied object_template.
 */
static obj_template_t* rationalise_multiple_matches( obj_template_t* obj,
                                                     boolean match,
                                                     boolean onematch,
                                                     uint32 matchcount,
                                                     status_t *retres )
{
    if (match && onematch && matchcount > 1) {
        *retres = ERR_NCX_MULTIPLE_MATCHES;
        return NULL;
    }
    return obj;
}

/********************************************************************/
/**
 * Check if an obj_template_t in the mod->datadefQ or any
 * of the include files visible to this module
 *
 * Top-level access is not tracked, so the 'lookdeep' variable
 * is hard-wired to FALSE
 *
 * \param mod ncx_module to check
 * \param modname module name for the object (needed for augments) 
 *                (may be NULL to match any 'objname' instance)
 * \param objname name of the object name to find
 * \param match TRUE to partial-length match node names 
 *              FALSE to find exact-length match only
 * \param altnames TRUE if alt-name should be checked
 * \param usecase TRUE if case-sensitive, FALSE if allow case-insensitive match
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \return pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* find_template_top_in_paraent_module( 
        ncx_module_t *mod,
        const xmlChar *modname,
        const xmlChar *objname,
        boolean match,
        boolean altnames,
        boolean usecase,
        boolean dataonly,
        uint32 *matchcount ) 
{
    if ( !mod->ismod ) {
        ncx_module_t  *mainmod = ncx_get_parent_mod(mod);
        if ( mainmod ) {
            /* check the [sub]module datadefQ */
            return find_template( &mainmod->datadefQ, modname, objname, FALSE, 
                  match, usecase, altnames, dataonly, matchcount );
        }
    }
    return NULL;
}

/********************************************************************/
/**
 * FUNCTION find_template_top
 *
 * Check if an obj_template_t in the mod->datadefQ or any
 * of the include files visible to this module
 *
 * Top-level access is not tracked, so the 'lookdeep' variable
 * is hard-wired to FALSE
 *
 * \param mod ncx_module to check
 * \param modname module name for the object (needed for augments) 
 *                (may be NULL to match any 'objname' instance)
 * \param objname name of the object name to find
 * \param match TRUE to partial-length match node names 
 *              FALSE to find exact-length match only
 * \param altnames TRUE if alt-name should be checked
 * \param usecase TRUE if case-sensitive, FALSE if allow case-insensitive match
 * \param onematch TRUE if only 1 match allowed FALSE if first match of 
 *                 N allowed
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param retres output return status
 * \return pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* find_template_top( ncx_module_t *mod,
                                          const xmlChar *modname,
                                          const xmlChar *objname,
                                          boolean match,
                                          boolean altnames,
                                          boolean usecase,
                                          boolean onematch,
                                          boolean dataonly,
                                          status_t *retres )
{
    obj_template_t *obj;
    uint32 matchcount = 0;

    *retres = NO_ERR;

    /* check the [sub]module datadefQ */
    obj = find_template( &mod->datadefQ, modname, objname, FALSE, 
                         match, usecase, altnames, dataonly, &matchcount );
    if ( obj ) { 
        return rationalise_multiple_matches( obj, match, onematch, matchcount, 
              retres );
    }

    /* Check Submodule Includes */
    obj = find_template_top_in_submodules_includes( mod, modname, objname, 
          match, altnames, usecase, dataonly, &matchcount );
    if ( obj ) { 
        return rationalise_multiple_matches( obj, match, onematch, matchcount, 
              retres );
    }

    /* if this is a submodule, then still need to check
     * the datadefQ of the main module
     */
    obj = find_template_top_in_paraent_module(  mod, modname, objname, match,
            altnames, usecase, dataonly, &matchcount );
    if ( obj ) { 
        return rationalise_multiple_matches( obj, match, onematch, matchcount, 
                                             retres );
    }

    *retres = ERR_NCX_DEF_NOT_FOUND;
    return NULL;

}   /* find_template_top */

/********************************************************************
* FUNCTION count_keys
* 
* Count the keys; walker function
*
* INPUTS:
*   obj == the key object
*   cookie1 == the running count
*   cookie2 = not used
* RETURNS:
*   TRUE to keep walking
*********************************************************************/
static boolean
    count_keys (obj_template_t *obj,
                void *cookie1,
                void *cookie2)
{
    uint32 *count = (uint32 *)cookie1;
    (void)obj;
    (void)cookie2;
    (*count)++;
    return TRUE;
} /* count_keys */

/********************************************************************/
/**
 * Try to find an object using an exact match.
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_exact_match ( ncx_module_t *mod,
                                         const xmlChar *modname,
                                         const xmlChar *objname,
                                         boolean dataonly,
                                         status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              FALSE,    /* match */
                              FALSE,    /* altnames */
                              TRUE,     /* usecase */
                              FALSE,    /* onematch */
                              dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using a case insensitive exact length match
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_insensitive_exact_length ( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              FALSE,    /* match */
                              FALSE,    /* altnames */
                              FALSE,     /* usecase */
                              FALSE,    /* onematch */
                              dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using a case sensitive partial name match
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param onematch TRUE if only 1 match allowed
*                  FALSE if first match of N allowed
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_sensitive_partial_match ( ncx_module_t *mod, 
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      boolean onematch,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              TRUE,     /* match */
                              FALSE,    /* altnames */
                              TRUE,     /* usecase */
                              onematch, dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using a case insensitive partial name match
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param onematch TRUE if only 1 match allowed
*                  FALSE if first match of N allowed
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_insensitive_partial_match( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      boolean onematch,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              TRUE,     /* match */
                              FALSE,    /* altnames */
                              FALSE,     /* usecase */
                              onematch, dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using an exact match alt name.
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_exact_match_alt_name ( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              FALSE,     /* match */
                              TRUE,      /* altnames */
                              TRUE,      /* usecase */
                              FALSE,     /* onematch */
                              dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using a case insensitive exact match alt name.
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_insensitive_exact_alt_name ( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              FALSE,     /* match */
                              TRUE,      /* altnames */
                              FALSE,      /* usecase */
                              FALSE,     /* onematch */
                              dataonly, retres );
}

/********************************************************************/
/**
 * Try to find an object using a case sensitive partial match alt name.
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param onematch TRUE if only 1 match allowed
 *                 FALSE if first match of N allowed
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_sensitive_partial_alt_name ( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      boolean onematch,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              TRUE,      /* match */
                              TRUE,      /* altnames */
                              TRUE,      /* usecase */
                              onematch,  /* onematch */
                              dataonly, retres );
}

/********************************************************************
 * Try to find an object using a case insensitive partial match alt name.
 *
 * \param mod ncx_module to check
 * \param modname the module name for the object (needed for augments)
 *                (may be NULL to match any 'objname' instance)
 * \param objname the object name to find
 * \param dataonly TRUE to check just data nodes FALSE to check all nodes
 * \param onematch TRUE if only 1 match allowed
 *                 FALSE if first match of N allowed
 * \param retres the return status
 * \return *  pointer to struct if present, NULL otherwise
 *********************************************************************/
static obj_template_t* try_case_insensitive_partial_alt_name( ncx_module_t *mod,
      const xmlChar *modname,
      const xmlChar *objname,
      boolean dataonly,
      boolean onematch,
      status_t *retres )
{
    return find_template_top( mod, modname, objname, 
                              TRUE,      /* match */
                              TRUE,      /* altnames */
                              FALSE,     /* usecase */
                              onematch,  /* onematch */
                              dataonly, retres );
}

/********************************************************************
 * Check if the RPC object has any real input children
 *
 * \param obj the obj_template to check
 * \param chkParam the name of the parameter to check for
 * \return TRUE if there are any input children
 *********************************************************************/
static boolean obj_rpc_has_input_or_output( obj_template_t *obj, 
                                            const xmlChar* chkParam )
{
    assert(obj && "obj is NULL" );
    if (obj->objtype != OBJ_TYP_RPC) {
        return FALSE;
    }

    obj_template_t* childobj = obj_find_child( obj, obj_get_mod_name(obj),
                                               chkParam );
    if (childobj) {
        return obj_has_children(childobj);
    } else {
        return FALSE;
    }

}   /* obj_rpc_has_input_or_output */


/********************************************************************
 * Try to find the module for an object using its nsid.
 *
 * \param node the node to use.
 * \param force_modQ the Q of ncx_module_t to check
 * \return pointer to the ncx_module or NULL.
 *********************************************************************/
static ncx_module_t* find_module_from_nsid( const xml_node_t* node,
                                            dlq_hdr_t *force_modQ )
{
    ncx_module_t* foundmod = NULL;

    if (node->nsid) {
        if (node->nsid == xmlns_nc_id() || !force_modQ ) {
            foundmod = xmlns_get_modptr(node->nsid);
        } else if (force_modQ) {
            /* try a session module Q */
            foundmod = ncx_find_module_que_nsid( force_modQ, node->nsid);
            if ( !foundmod ) {
                /* try a client-loaded module */
                foundmod = xmlns_get_modptr(node->nsid);
            }
        }
    }

    return foundmod;
}

/********************************************************************
 * Try to find the module name for an object using its nsid.
 *
 * \param node the node to use.
 * \param force_modQ the Q of ncx_module_t to check
 * \return pointer to the modulename
 *********************************************************************/
static const xmlChar* get_module_name_from_nsid( const xml_node_t* node,
                                                 dlq_hdr_t *force_modQ )
{
    ncx_module_t *foundmod = find_module_from_nsid( node, force_modQ );
    return ( foundmod ? ncx_get_modname(foundmod) : NULL );
}

/********************************************************************
 * Utility function for validating the type of a founc child node.
 * Check that the child object is a valid type. If the child node is
 * of a valid type this function simply returns it, otherwise it
 * returns NULL.
 *
 * \param obj the parent objext template
 * \param foundobj the child node
 * 
 *********************************************************************/
static obj_template_t* validated_child_object( obj_template_t* obj, 
                                              obj_template_t* foundobj )
{
    if ( foundobj )
    {
        if ( !obj_is_data_db(foundobj) || obj_is_abstract(foundobj) ||
                obj_is_cli(foundobj) || obj_is_choice_or_case( obj ) ) {
                return NULL;
        }
    }

    return foundobj;
}

/********************************************************************
 * Check if an object is a <notification> element
 *
 * \param obj the parent objext template
 * \return True if the NSID of the object is the NCNID and the
 * object's name is NCX_EL_NOTIFICATION
 * 
 * @TODO: How does this test differ from calling obj_is_notif
 *********************************************************************/
static bool obj_is_notification_parent( obj_template_t* obj )
{
    return ( obj_get_nsid(obj) == xmlns_ncn_id() &&
            !xml_strcmp( obj_get_name( obj ), NCX_EL_NOTIFICATION ) );
}

/********************************************************************
 * Get the child node for the supplied root node.
 *
 * \param obj the parent object template
 * \param curnode the current XML start or empty node to check
 * \param force_modQ the Q of ncx_module_t to check
 * \return the found object or NULL;
 *********************************************************************/
static obj_template_t* get_child_node_from_root( obj_template_t* obj, 
                                                 const xml_node_t* curnode, 
                                                 dlq_hdr_t *force_modQ )
{
    ncx_module_t *foundmod = find_module_from_nsid( curnode, force_modQ );;
    const xmlChar *foundmodname = ( foundmod ? ncx_get_modname(foundmod) 
                                             : NULL );

    obj_template_t* foundobj = NULL;
    /* the child node can be any top-level object in the configuration */
    if (foundmodname) {
        /* get the name from 1 module */
        foundobj =  ncx_find_object( foundmod, curnode->elname );
    } else if (force_modQ) {
        /* check this Q of modules for a top-level match */
        foundobj = ncx_find_any_object_que(force_modQ, curnode->elname);
        if ( !foundobj && curnode->nsid == 0) {
            foundobj = ncx_find_any_object(curnode->elname);
        }                
    } else {
        /* NSID not set, get the name from any module */
        foundobj = ncx_find_any_object(curnode->elname);
   }

    return validated_child_object( obj, foundobj );
}

/********************************************************************
 * Get the child node for the supplied notification node.
 *
 * hack: special case handling of the <notification> element the child 
 * node can be <eventTime> or any top-level OBJ_TYP_NOTIF node 
 *
 * \param obj the parent object template
 * \param curnode the current XML start or empty node to check
 * \param force_modQ the Q of ncx_module_t to check, if set to NULL and the 
 *                   xmlns registry of module pointers will be used instead 
 * \return the found object or NULL;
 *********************************************************************/
static obj_template_t* get_child_node_for_notif( obj_template_t* obj, 
                                                 const xml_node_t* curnode, 
                                                 dlq_hdr_t *force_modQ )
{
    ncx_module_t *foundmod = find_module_from_nsid( curnode, force_modQ );;
    const xmlChar *foundmodname = ( foundmod ? ncx_get_modname(foundmod) 
                                             : NULL );

    obj_template_t* foundobj = NULL;
    if (foundmodname) {
        /* try a child of <notification> */
        foundobj = obj_find_child(obj, foundmodname, curnode->elname);

        if (!foundobj) {
            /* try to find an <eventType> */
            foundobj =  ncx_find_object( foundmod, curnode->elname);

             // check the foundobj is a notification type
            if ( foundobj && !obj_is_notif(foundobj) ) {
                /* object is the wrong type */
                foundobj = NULL;
            }
        }
    } else {
        /* no namespace ID used try to find any eventType object */
        if (force_modQ) {
            foundobj = ncx_find_any_object_que(force_modQ, curnode->elname);
        } else {
            foundobj = ncx_find_any_object(curnode->elname);
        }

        if (foundobj) {
             // check the foundobj is a notification type
            if ( obj_is_notif( foundobj ) ) {
                foundobj = NULL;
            }
        } else {
            /* try a child of obj (eventTime), no need to check type,
             * since any child is a direct descendent of the
             * notification object */
            foundobj = obj_find_child(obj, NULL, curnode->elname );
        }
    }

    return foundobj;
}

/********************************************************************
 * Get the child node for the curnode based on xml order.
 *
 * \param curnode the current XML start or empty node to check
 * \param chobj the current child node.
 * \param force_modQ the Q of ncx_module_t to check
 * \param rettop the address of return topchild object
 * \param topdone flag indicating if rettop was set
 * \return the found object or NULL;
 *********************************************************************/
static obj_template_t* get_xml_ordered_child_node( const xml_node_t* curnode, 
                                                   obj_template_t* chobj, 
                                                   dlq_hdr_t *force_modQ,
                                                   obj_template_t **rettop,
                                                   boolean* topdone )
{
    /* the current node or one of the subsequent child nodes must match */
    if (!chobj) {
        return NULL;
    }

    const xmlChar *foundmodname = get_module_name_from_nsid( curnode, 
                                                             force_modQ );
    obj_template_t* foundobj = NULL;

    if ( obj_is_choice_or_case( chobj ) ) {
        /* these nodes are not really in the XML so check all the child nodes 
         * of the cases. When found, need to remember the current child node 
         * at the choice or case level, so when the lower level child pointer 
         * runs out, the search can continue at the next sibling of 'rettop' */
        foundobj = obj_find_child(chobj, foundmodname, curnode->elname);
        if (foundobj) {
            /* make sure this matched a real node instead */
            if ( obj_is_choice_or_case( foundobj ) ) {
                foundobj = NULL;
             } else {
                 *rettop = chobj;
                 *topdone = TRUE;
             }
        }                   
   }
   else {
       /* the YANG node and XML node line up, ergo compare them directly */
       if ( NO_ERR == xml_node_match( curnode, obj_get_nsid(chobj), 
               obj_get_name(chobj), XML_NT_NONE ) ) {
           foundobj = chobj;
        } else {
            foundobj = NULL;
        }
    }

     if (!foundobj) {
        /* check if there are other child nodes that could match, due to 
         * instance qualifiers */
        obj_template_t *nextchobj = find_next_child(chobj, curnode);
        if (nextchobj) {
            foundobj = nextchobj;
        } else if (*rettop) {
            // @TODO: Why is rettop being used as an input parameter here? it is
            // only set of we have found an object by obj_get_child_node!
            nextchobj = find_next_child(*rettop, curnode);
            if (nextchobj) {
                foundobj = nextchobj;
            }
        }
    }

    return foundobj;
}

/********************************************************************
 * Get any matching node within the current parent,  regardless of xml order.
 *
 * \param obj the parent object template
 * \param curnode the current XML start or empty node to check
 * \param force_modQ the Q of ncx_module_t to check
 * \return the found object or NULL;
 *********************************************************************/
static obj_template_t* get_first_matching_child_node( obj_template_t* obj,
                                                const xml_node_t* curnode, 
                                                dlq_hdr_t *force_modQ )
{
    const xmlChar *foundmodname = get_module_name_from_nsid( curnode,
                                                             force_modQ );
    obj_template_t* foundobj = obj_find_child( obj, foundmodname, 
                                               curnode->elname );

    return (foundobj && obj_is_choice_or_case( foundobj ) ) ? NULL
                                                            : foundobj; 
}

/********************************************************************
 * Search for a child of the current XML node. 
 *
 * \param obj the parent object template
 * \param chobj the current child node 
 * \param xmlorder TRUE if should follow strict XML element order,
 * \param curnode the current XML start or empty node to check
 * \param force_modQ the Q of ncx_module_t to check
 * \param rettop the address of return topchild object
 * \param topdone flag indicating if rettop was set
 * \return the found object or NULL;
 *********************************************************************/
static obj_template_t* search_for_child_node( obj_template_t *obj,
                                              obj_template_t *chobj,
                                              const xml_node_t *curnode,
                                              boolean xmlorder,
                                              dlq_hdr_t *force_modQ,
                                              obj_template_t **rettop,
                                              boolean *topdone )
{
    if (obj_is_root(obj)) {
        return get_child_node_from_root( obj, curnode, force_modQ );
    } else if ( obj_is_notification_parent( obj ) ) {
        return get_child_node_for_notif( obj, curnode, force_modQ );
    } else if (xmlorder) {
        return get_xml_ordered_child_node( curnode, chobj, force_modQ, 
                rettop, topdone );
    } else {
        return get_first_matching_child_node( obj, curnode, force_modQ );
    }
}


/********************************************************************
 * Clean the inherited iffeature queue
 *
 * \param ptrQ the Q of obj_iffeature_ptr_t to clean
 *********************************************************************/
static void clean_inherited_iffeatureQ( dlq_hdr_t *ptrQ )
{
    obj_iffeature_ptr_t *iffptr;
    while (!dlq_empty(ptrQ)) {
        iffptr = (obj_iffeature_ptr_t *)dlq_deque(ptrQ);
        obj_free_iffeature_ptr(iffptr);
    }
}

/********************************************************************
 * Clean the inherited when-stmt queue
 *
 * \param ptrQ the Q of obj_xpath_ptr_t to clean
 *********************************************************************/
static void clean_inherited_whenQ( dlq_hdr_t *ptrQ )
{
    obj_xpath_ptr_t *xptr;
    while (!dlq_empty(ptrQ)) {
        xptr = (obj_xpath_ptr_t *)dlq_deque(ptrQ);
        obj_free_xpath_ptr(xptr);
    }
}

    
/**************** E X T E R N A L    F U N C T I O N S    **********/

/********************************************************************
* Malloc and initialize the fields in a an object template
*
* \param objtype the specific object type to create
* \return pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
obj_template_t* obj_new_template( obj_type_t objtype )
{
    obj_template_t  *obj;

    if ( objtype == OBJ_TYP_NONE || objtype > OBJ_TYP_NOTIF )
    {
        SET_ERROR( ERR_INTERNAL_VAL );
        return NULL;
    }

    obj = m__getObj(obj_template_t);
    if (!obj) {
        return NULL;
    }
    init_template(obj);
    obj->objtype = objtype;
    
    switch (objtype) {
    case OBJ_TYP_CONTAINER:
        obj->def.container = new_container(TRUE);
        break;

    case OBJ_TYP_LEAF:
    case OBJ_TYP_ANYXML:
        obj->def.leaf = new_leaf(TRUE);
        break;

    case OBJ_TYP_LEAF_LIST:
        obj->def.leaflist = new_leaflist(TRUE);
        break;

    case OBJ_TYP_LIST:
        obj->def.list = new_list(TRUE);
        break;

    case OBJ_TYP_CHOICE:
        obj->def.choic = new_choice(TRUE);
        break;

    case OBJ_TYP_CASE:
        obj->def.cas = new_case(TRUE);
        break;

    case OBJ_TYP_USES:
        obj->def.uses = new_uses(TRUE);
        break;

    case OBJ_TYP_REFINE:
        obj->def.refine = new_refine();
        break;

    case OBJ_TYP_AUGMENT:
        obj->def.augment = new_augment(TRUE);
        break;

    case OBJ_TYP_RPC:
        obj->def.rpc = new_rpc();
        break;

    case OBJ_TYP_RPCIO:
        obj->def.rpcio = new_rpcio();
        break;

    case OBJ_TYP_NOTIF:
        obj->def.notif = new_notif();
        break;

    default:
        break;
    }

    // any member of the union def can be used to check that the object was 
    // allocated correctly
    if ( !obj->def.container ) {
        m__free( obj );
        obj = NULL;
    }

    return obj;
}  /* obj_new_template */


/********************************************************************
 * Scrub the memory in a obj_template_t by freeing all
 * the sub-fields and then freeing the entire struct itself 
 * The struct must be removed from any queue it is in before
 * this function is called.
 *
 * \param obj obj_template_t data structure to free
 *********************************************************************/
void obj_free_template( obj_template_t *obj )
{
    if (!obj) {
        return;
    }

#ifdef OBJ_MEM_DEBUG
    if (obj_is_cloned(obj)) {
        log_debug4("\nobj_free: %p (cloned)", obj);
    } else {
        log_debug4("\nobj_free: %p (%s)", obj, obj_get_name(obj));
    }
#endif

    clean_metadataQ(&obj->metadataQ);
    ncx_clean_appinfoQ(&obj->appinfoQ);
    ncx_clean_iffeatureQ(&obj->iffeatureQ);
    clean_inherited_iffeatureQ(&obj->inherited_iffeatureQ);
    clean_inherited_whenQ(&obj->inherited_whenQ);
    xpath_free_pcb(obj->when);

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        free_container(obj->def.container, obj->flags);
        break;

    case OBJ_TYP_LEAF:
    case OBJ_TYP_ANYXML:
        free_leaf(obj->def.leaf, obj->flags);
        break;

    case OBJ_TYP_LEAF_LIST:
        free_leaflist(obj->def.leaflist, obj->flags);
        break;

    case OBJ_TYP_LIST:
        free_list(obj->def.list, obj->flags);
        break;

    case OBJ_TYP_CHOICE:
        free_choice(obj->def.choic, obj->flags);
        break;

    case OBJ_TYP_CASE:
        free_case(obj->def.cas);
        break;

    case OBJ_TYP_USES:
        free_uses(obj->def.uses);
        break;

    case OBJ_TYP_REFINE:
        free_refine(obj->def.refine);
        break;

    case OBJ_TYP_AUGMENT:
        free_augment(obj->def.augment);
        break;

    case OBJ_TYP_RPC:
        free_rpc(obj->def.rpc);
        break;

    case OBJ_TYP_RPCIO:
        free_rpcio(obj->def.rpcio);
        break;

    case OBJ_TYP_NOTIF:
        free_notif(obj->def.notif);
        break;

    case OBJ_TYP_NONE:
        // no object type specific fields set yet
        break;

    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    m__free(obj);

}  /* obj_free_template */


/********************************************************************
* FUNCTION obj_find_template
* 
* Find an object with the specified name
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
obj_template_t * obj_find_template ( dlq_hdr_t  *que,
                                     const xmlChar *modname,
                                     const xmlChar *objname )
{
    assert( que && "que is NULL" );
    assert( objname && "objname is NULL" );

    uint32 matchcount=0;
    return find_template( que, modname, objname, 
                          FALSE, /* lookdeep */
                          FALSE, /* match */
                          TRUE,  /* usecase */
                          FALSE, /* altnames */
                          FALSE, /* dataonly */
                          &matchcount );
}  /* obj_find_template */


/********************************************************************
* FUNCTION obj_find_template_con
* 
* Find an object with the specified name
* Return a const pointer; used by yangdump
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
const obj_template_t *
    obj_find_template_con (dlq_hdr_t  *que,
                           const xmlChar *modname,
                           const xmlChar *objname)
{
    assert( que && "que is NULL" );
    assert( objname && "objname is NULL" );

    uint32 matchcount=0;
    return find_template( que, modname, objname, 
                          FALSE, /* lookdeep */
                          FALSE, /* match */
                          TRUE,  /* usecase */
                          FALSE, /* altnames */
                          FALSE, /* dataonly */
                          &matchcount );
}  /* obj_find_template_con */

/********************************************************************
* FUNCTION obj_find_template_test
* 
* Find an object with the specified name
*
* INPUTS:
*    que == Q of obj_template_t to search
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found in 'que'
*********************************************************************/
obj_template_t *
    obj_find_template_test (dlq_hdr_t  *que,
                            const xmlChar *modname,
                            const xmlChar *objname)
{
    assert( que && "que is NULL" );
    assert( objname && "objname is NULL" );

    uint32 matchcount=0;
    return find_template( que, modname, objname,
                          TRUE, /* lookdeep */
                          FALSE, /* match */
                          TRUE,  /* usecase */
                          FALSE, /* altnames */
                          FALSE, /* dataonly */
                          &matchcount );

}  /* obj_find_template_test */


/********************************************************************
* FUNCTION obj_find_template_top
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files visible to this module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
obj_template_t *
    obj_find_template_top (ncx_module_t *mod,
                           const xmlChar *modname,
                           const xmlChar *objname)
{
    status_t  res;
    return obj_find_template_top_ex(mod,
                                    modname,
                                    objname,
                                    NCX_MATCH_EXACT,
                                    FALSE,
                                    FALSE,
                                    &res);

}   /* obj_find_template_top */


/********************************************************************
* FUNCTION obj_find_template_top_ex
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files visible to this module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*   match_names == enum for selected match names mode
*   alt_names == TRUE if alt-name should be checked in addition
*                to the YANG node name
*             == FALSE to check YANG names only
*   dataonly == TRUE to check just data nodes
*               FALSE to check all nodes
*   retres == address of return status
*
* OUTPUTS:
*   *retres set to return status
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
obj_template_t * obj_find_template_top_ex ( ncx_module_t *mod,
                                            const xmlChar *modname,
                                            const xmlChar *objname,
                                            ncx_name_match_t match_names,
                                            boolean alt_names,
                                            boolean dataonly,
                                            status_t *retres )
{
    obj_template_t  *obj;
    boolean          multmatches = FALSE;

    // note: modname may be NULL
    assert( mod &&  " mod param is NULL" );
    assert( objname && " objname param is NULL" );
    assert( retres && " retres param is NULL" );

    /* 1) try an exact match */
    obj = try_exact_match( mod, modname, objname, dataonly, retres );
    if (obj) {
        return obj;
    }

    /* 2) try an case-insensitive exact-length match */
    if ( match_names >= NCX_MATCH_EXACT_NOCASE ) {
        obj = try_case_insensitive_exact_length( mod, modname, objname, 
                                                 dataonly, retres );
        if (obj) {
            return obj;
        }
    } else if (!alt_names) { /* NCX_MATCH_EXACT mode */
        return NULL; 
    }

    /* 3) try an case-sensitive partial-name match */
    if (match_names >= NCX_MATCH_ONE) {
        obj = try_case_sensitive_partial_match( mod, modname, objname, 
                (match_names < NCX_MATCH_FIRST), dataonly, retres);
        if (obj) {
            return obj;
        }
        multmatches = (*retres == ERR_NCX_MULTIPLE_MATCHES );
    } else if (!alt_names) { /* NCX_MATCH_EXACT_NOCASE mode */
        return NULL; 
    } 

    /* 4) try an case-insensitive partial-name match */
    if (match_names == NCX_MATCH_ONE_NOCASE || 
        match_names == NCX_MATCH_FIRST_NOCASE) {
        obj = try_case_insensitive_partial_match( mod, modname, objname, 
                (match_names < NCX_MATCH_FIRST), dataonly, retres );
        if (obj) {
            return obj;
        }
        multmatches = (*retres == ERR_NCX_MULTIPLE_MATCHES );
    } else if (!alt_names) { /* NCX_MATCH_ONE mode or NCX_MATCH_FIRST mode */
        if (multmatches) {
            *retres = ERR_NCX_MULTIPLE_MATCHES;
        }
        return NULL;
    }

    /* 5) try an exact match on alt-name */
    obj = try_exact_match_alt_name( mod, modname, objname, dataonly, retres );
    if (obj) {
        *retres = NO_ERR;  // FIXME: Why is retres being ignored?
        return obj;
    }

    /* 6) try an case-insensitive exact-length match on alt-name */
    if (match_names >= NCX_MATCH_EXACT_NOCASE) {
        obj = try_case_insensitive_exact_alt_name( mod, modname, objname, 
                                                   dataonly, retres );
        if (obj) {
            return obj;
        }
    } else { /* NCX_MATCH_EXACT mode + alt_names */
        return NULL;    // note: multmatches cannot be ture on this path!
    }

    /* 7) try an case-sensitive partial-name match on alt-name */
    if (match_names >= NCX_MATCH_ONE) {
        obj = try_case_sensitive_partial_alt_name( mod, modname, objname, 
                (match_names < NCX_MATCH_FIRST), dataonly, retres );
        if (obj) {
            return obj;
        }
    } else {
        /* NCX_MATCH_EXACT_NOCASE mode + alt_names */
        return NULL;
    }

    /* 8) try an case-insensitive partial-name match on alt-name */
    if (match_names == NCX_MATCH_ONE_NOCASE || 
        match_names == NCX_MATCH_FIRST_NOCASE) {
        obj = try_case_insensitive_partial_alt_name( mod, modname, objname, 
                (match_names < NCX_MATCH_FIRST), dataonly, retres );
        if (obj) {
            return obj;
        }
    } 

    if (multmatches) {
        *retres = ERR_NCX_MULTIPLE_MATCHES;
    }
    return NULL;

}   /* obj_find_template_top_ex */

/********************************************************************
* FUNCTION obj_find_template_all
*
* Check if an obj_template_t in the mod->datadefQ or any
* of the include files used within the entire main module
*
* Top-level access is not tracked, so the 'test' variable
* is hard-wired to FALSE
*
* INPUTS:
*   mod == ncx_module to check
*   modname == module name for the object (needed for augments)
*              (may be NULL to match any 'objname' instance)
*   objname == object name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
obj_template_t *
    obj_find_template_all (ncx_module_t *mod,
                           const xmlChar *modname,
                           const xmlChar *objname)
{
    assert( mod && "modname is NULL" );
    assert( objname && "objname is NULL" );

    obj_template_t *obj;
    yang_node_t    *node;
    dlq_hdr_t      *que;

    /* check the main module */
    uint32 matchcount=0;
    obj = find_template( &mod->datadefQ, modname, objname, 
                         FALSE, /* lookdeep */
                         FALSE,    /* match */
                         TRUE,   /* usecase */
                         FALSE, /* altnames */
                         FALSE, /* dataonly */
                         &matchcount );
    if (obj) {
        return obj;
    }

    que = ncx_get_allincQ(mod);

    /* check all the submodules, but only the ones visible
     * to this module or submodule, YANG only
     */
    for (node = (yang_node_t *)dlq_firstEntry(que);
         node != NULL;
         node = (yang_node_t *)dlq_nextEntry(node)) {

        if (node->submod) {
            /* check the object Q in this submodule */
            obj = find_template(&node->submod->datadefQ, modname, objname,
                                FALSE,  /* lookdeep */
                                FALSE,     /* match */
                                TRUE,    /* usecase */
                                FALSE,  /* altnames */
                                FALSE, /* dataonly */
                                &matchcount );
            if (obj) {
                return obj;
            }
        }
    }

    return NULL;

}   /* obj_find_template_all */


/********************************************************************
 * Find a child object with the specified Qname
 *
 * !!! This function checks for accessible names only!!!
 * !!! That means child nodes of choice->case will be
 * !!! present instead of the choice name or case name
 *
 * \param obj the obj_template_t to check
 * \param modname the module name that defines the obj_template_t
 *                if it is NULL and first match will be done, and the
 *                module ignored (Name instead of QName)
 * \param objname the object name to find
 * \return pointer to obj_template_t or NULL if not found
 *********************************************************************/
obj_template_t * obj_find_child ( obj_template_t *obj,
                                  const xmlChar *modname,
                                  const xmlChar *objname)
{
    assert( obj && "que is NULL" );
    assert( objname && "objname is NULL" );
    dlq_hdr_t  *que;

    que = obj_get_datadefQ(obj);
    if (que != NULL) {
        uint32 matchcount=0;
        return find_template( que, modname, objname, 
                              TRUE,  /* lookdeep */
                              FALSE,    /* match */
                              TRUE,   /* usecase */
                              FALSE, /* altnames */
                              FALSE, /* dataonly */
                              &matchcount );
    }

    return NULL;
}  /* obj_find_child */


/********************************************************************
* FUNCTION obj_find_child_ex
* 
* Find a child object with the specified Qname
* extended match modes
*
* !!! This function checks for accessible names only!!!
* !!! That means child nodes of choice->case will be
* !!! present instead of the choice name or case name
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find
*    match_names == enum for selected match names mode
*    alt_names == TRUE if alt-name should be checked in addition
*                to the YANG node name
*             == FALSE to check YANG names only
*    dataonly == TRUE to check just data nodes
*                FALSE to check all nodes
*    retres == address of return status
*
* OUTPUTS:
*   if retres not NULL, *retres set to return status
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
obj_template_t *
    obj_find_child_ex (obj_template_t  *obj,
                       const xmlChar *modname,
                       const xmlChar *objname,
                       ncx_name_match_t match_names,
                       boolean alt_names,
                       boolean dataonly,
                       status_t *retres)
{
    assert( obj && "obj is NULL" );
    assert( objname && "objname is NULL" );

    dlq_hdr_t  *que;
    uint32      matchcount;


    if (retres != NULL) {
        *retres = NO_ERR;
    }

    que = obj_get_datadefQ(obj);
    if ( !que ) {
        return NULL;
    }

    /* 1) try an exact match */
    obj = find_template(que,
                        modname, 
                        objname, 
                        TRUE,     /* loopdeep */
                        FALSE,       /* match */
                        TRUE,      /* usecase */
                        FALSE,   /* alt_names */
                        dataonly,
                        &matchcount);
    if (obj != NULL) {
        return obj;
    }

    /* 2) try an case-insensitive exact-length match */
    if (match_names >= NCX_MATCH_EXACT_NOCASE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,      /* loopdeep */
                            FALSE,        /* match */
                            FALSE,      /* usecase */
                            FALSE,     /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            return obj;
        }
    } else if (!alt_names) {
        /* NCX_MATCH_EXACT mode */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    /* 3) try an case-sensitive partial-name match */
    if (match_names >= NCX_MATCH_ONE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,     /* loopdeep */
                            TRUE,        /* match */
                            TRUE,      /* usecase */
                            FALSE,    /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            if (match_names <= NCX_MATCH_ONE_NOCASE &&
                matchcount > 1) {
                if (retres != NULL) {
                    *retres = ERR_NCX_MULTIPLE_MATCHES;
                }
                return NULL;
            }
            return obj;
        }
    } else if (!alt_names) {
        /* NCX_MATCH_EXACT_NOCASE mode */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    /* 4) try an case-insensitive partial-name match */
    if (match_names == NCX_MATCH_ONE_NOCASE ||
        match_names == NCX_MATCH_FIRST_NOCASE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,     /* loopdeep */
                            TRUE,        /* match */
                            FALSE,     /* usecase */
                            FALSE,    /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            if (match_names <= NCX_MATCH_ONE_NOCASE &&
                matchcount > 1) {
                if (retres != NULL) {
                    *retres = ERR_NCX_MULTIPLE_MATCHES;
                }
                return NULL;
            }
            return obj;
        }
    } else if (!alt_names) {
        /* NCX_MATCH_ONE mode or NCX_MATCH_FIRST mode */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    if (!alt_names) {
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    /* 5) try an exact match on alt-name */
    obj = find_template(que,
                        modname, 
                        objname, 
                        TRUE,      /* loopdeep */
                        FALSE,        /* match */
                        TRUE,       /* usecase */
                        TRUE,      /* altnames */
                        dataonly,
                        &matchcount);
    if (obj) {
        return obj;
    }

    /* 6) try an case-insensitive exact-length match on alt-name */
    if (match_names >= NCX_MATCH_EXACT_NOCASE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,       /* loopdeep */
                            FALSE,         /* match */
                            FALSE,       /* usecase */
                            TRUE,       /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            if (match_names <= NCX_MATCH_ONE_NOCASE &&
                matchcount > 1) {
                if (retres != NULL) {
                    *retres = ERR_NCX_MULTIPLE_MATCHES;
                }
                return NULL;
            }
            return obj;
        }
    } else {
        /* NCX_MATCH_EXACT mode + alt_names */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    /* 7) try an case-sensitive partial-name match on alt-name */
    if (match_names >= NCX_MATCH_ONE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,       /* loopdeep */
                            TRUE,          /* match */
                            TRUE,        /* usecase */
                            TRUE,       /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            if (match_names <= NCX_MATCH_ONE_NOCASE &&
                matchcount > 1) {
                if (retres != NULL) {
                    *retres = ERR_NCX_MULTIPLE_MATCHES;
                }
                return NULL;
            }
            return obj;
        }
    } else {
        /* NCX_MATCH_EXACT_NOCASE mode + alt_names */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    /* 8) try an case-insensitive partial-name match on alt-name */
    if (match_names == NCX_MATCH_ONE_NOCASE ||
        match_names == NCX_MATCH_FIRST_NOCASE) {
        obj = find_template(que,
                            modname, 
                            objname, 
                            TRUE,       /* loopdeep */
                            TRUE,          /* match */
                            FALSE,       /* usecase */
                            TRUE,       /* altnames */
                            dataonly,
                            &matchcount);
        if (obj) {
            if (match_names <= NCX_MATCH_ONE_NOCASE &&
                matchcount > 1) {
                return NULL;
            }
            return obj;
        }
    } else {
        /* NCX_MATCH_ONE mode or NCX_MATCH_FIRST mode */
        if (retres != NULL) {
            *retres = ERR_NCX_DEF_NOT_FOUND;
        }
        return NULL;
    }

    if (retres != NULL) {
        *retres = ERR_NCX_DEF_NOT_FOUND;
    }
    return NULL;

}  /* obj_find_child_ex */


/********************************************************************
* FUNCTION obj_find_child_str
* 
* Find a child object with the specified Qname
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find, not Z-terminated
*    objnamelen == length of objname string
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
obj_template_t *
    obj_find_child_str (obj_template_t *obj,
                        const xmlChar *modname,
                        const xmlChar *objname,
                        uint32 objnamelen)
{
    assert( obj && "obj is NULL" );
    assert( objname && "objname is NULL" );

    obj_template_t *template;
    dlq_hdr_t      *que;
    xmlChar              *buff;

    if (objnamelen > NCX_MAX_NLEN) {
        return NULL;
    }
    
    que = obj_get_datadefQ(obj);
    if (que) {
        buff = m__getMem(objnamelen+1);
        if (buff) {
            uint32 matchcount=0;
            xml_strncpy(buff, objname, objnamelen);
            template = find_template( que, modname, buff, 
                                      TRUE,   /* lookdeep */
                                      FALSE,     /* match */
                                      TRUE,    /* usecase */
                                      FALSE,  /* altnames */
                                      FALSE,  /* dataonly */
                                      &matchcount );
            m__free(buff);
            return template;
        }
    }

    return NULL;

}  /* obj_find_child_str */


/********************************************************************
* FUNCTION obj_match_child_str
* 
* Match a child object with the specified Qname
* Find first command that matches all N chars of objname
*
* !!! This function checks for accessible names only!!!
* !!! That means child nodes of choice->case will be
* !!! present instead of the choice name or case name
*
* INPUTS:
*    obj == obj_template_t to check
*    modname == module name that defines the obj_template_t
*            == NULL and first match will be done, and the
*               module ignored (Name instead of QName)
*    objname == object name to find, not Z-terminated
*    objnamelen == length of objname string
*    matchcount == address of return parameter match count
*                  (may be NULL)
* OUTPUTS:
*   if non-NULL:
*    *matchcount == number of parameters that matched
*                   only the first match will be returned
*
* RETURNS:
*    pointer to obj_template_t or NULL if not found
*********************************************************************/
obj_template_t *
    obj_match_child_str (obj_template_t *obj,
                         const xmlChar *modname,
                         const xmlChar *objname,
                         uint32 objnamelen,
                         uint32 *matchcount)
{
    assert( obj && "obj is NULL" );
    assert( objname && "objname is NULL" );

    obj_template_t  *template;
    dlq_hdr_t       *que;
    xmlChar               *buff;

    if (objnamelen > NCX_MAX_NLEN) {
        return NULL;
    }
    
    que = obj_get_datadefQ(obj);
    if (que) {
        buff = m__getMem(objnamelen+1);
        if (buff) {
            xml_strncpy(buff, objname, objnamelen);
            template = find_template(que, 
                                     modname, 
                                     buff, 
                                     TRUE,   /* lookdeep */
                                     TRUE,      /* match */
                                     TRUE,    /* usecase */
                                     FALSE,  /* altnames */
                                     FALSE,  /* dataonly */
                                     matchcount);
            m__free(buff);
            return template;
        }
    }

    return NULL;

}  /* obj_match_child_str */


/********************************************************************
* FUNCTION obj_first_child
* 
* Get the first child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_first_child (obj_template_t *obj)
{
    dlq_hdr_t       *que;
    obj_template_t  *chobj;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    que = obj_get_datadefQ(obj);
    if (que != NULL) {
        for (chobj = (obj_template_t *)dlq_firstEntry(que);
             chobj != NULL;
             chobj = (obj_template_t *)dlq_nextEntry(chobj)) {
            if (obj_has_name(chobj) && obj_is_enabled(chobj)) {
                return chobj;
            }
        }
    }

    return NULL;

}  /* obj_first_child */


/********************************************************************
* FUNCTION obj_last_child
* 
* Get the last child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_last_child (obj_template_t *obj)
{
    dlq_hdr_t       *que;
    obj_template_t  *chobj;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    que = obj_get_datadefQ(obj);
    if (que) {
        for (chobj = (obj_template_t *)dlq_lastEntry(que);
             chobj != NULL;
             chobj = (obj_template_t *)dlq_prevEntry(chobj)) {
            if (obj_has_name(chobj) && obj_is_enabled(chobj)) {
                return chobj;
            }
        }
    }

    return NULL;

}  /* obj_last_child */


/********************************************************************
* FUNCTION obj_next_child
* 
* Get the next child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_next_child (obj_template_t *obj)
{
    obj_template_t  *next;
    boolean          done;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    next = obj;
    done = FALSE;
    while (!done) {
        next = (obj_template_t *)dlq_nextEntry(next);
        if (!next) {
            done = TRUE;
        } else if (obj_has_name(next) && obj_is_enabled(next)) {
            return next;
        }
    }
    return NULL;

}  /* obj_next_child */


/********************************************************************
* FUNCTION obj_previous_child
* 
* Get the previous child object if the specified object
* has any children
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_previous_child (obj_template_t *obj)
{
    obj_template_t  *prev;
    boolean          done;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    prev = obj;
    done = FALSE;
    while (!done) {
        prev = (obj_template_t *)dlq_prevEntry(prev);
        if (!prev) {
            done = TRUE;
        } else if (obj_has_name(prev) && obj_is_enabled(prev)) {
            return prev;
        }
    }
    return NULL;

}  /* obj_previous_child */


/********************************************************************
* FUNCTION obj_first_child_deep
* 
* Get the first child object if the specified object
* has any children.  Look past choices and cases to
* the real nodes within them
*
*  !!!! SKIPS OVER AUGMENT AND USES AND CHOICES AND CASES !!!!
*
* INPUTS:
*    obj == obj_template_t to check

* RETURNS:
*    pointer to first child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_first_child_deep (obj_template_t *obj)
{
    dlq_hdr_t       *que;
    obj_template_t  *chobj;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* go through the child nodes of this object looking
     * for the first data object; skip over all meta-objects
     */
    que = obj_get_datadefQ(obj);
    if (que) {
        for (chobj = (obj_template_t *)dlq_firstEntry(que);
             chobj != NULL;
             chobj = (obj_template_t *)dlq_nextEntry(chobj)) {

            if (obj_has_name(chobj) && obj_is_enabled(chobj)) {
                if ( obj_is_choice_or_case( chobj ) ) {
                    return (obj_first_child_deep(chobj));
                } else {
                    return chobj;
                }
            }
        }
    }

    return NULL;

}  /* obj_first_child_deep */


/********************************************************************
* FUNCTION obj_next_child_deep
* 
* Get the next child object if the specified object
* has any children.  Look past choice and case nodes
* to the real nodes within them
*
*  !!!! SKIPS OVER AUGMENT AND USES !!!!
*
* INPUTS:
*    obj == obj_template_t to check
*
* RETURNS:
*    pointer to next child obj_template_t or 
*    NULL if not found 
*********************************************************************/
obj_template_t *
    obj_next_child_deep (obj_template_t *obj)
{
    obj_template_t  *cas, *next, *last, *child;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* start the loop at the current object to set the
     * 'last' object correctly
     */
    next = obj;
    while (next) {
        last = next;

        /* try next sibling */
        next = obj_next_child(next);
        if (next) {
            switch (next->objtype) {
            case OBJ_TYP_CHOICE:
                /* dive into each case to find a first object
                 * this should return the first object in the 
                 * first case, but it checks the entire choice
                 * to support empty case arms
                 */
                for (cas = obj_first_child(next);
                     cas != NULL;
                     cas = obj_next_child(next)) {
                    child = obj_first_child(cas);
                    if (child) {
                        return child;
                    }
                }
                continue;
            case OBJ_TYP_CASE:
                child = obj_first_child(next);
                if (child) {
                    return child;
                }
                continue;
            default:
                return next;
            }
        }

        /* was last sibling, try parent if this is a case */
        if (last->parent && 
            (last->parent->objtype==OBJ_TYP_CASE)) {

            cas = (obj_template_t *)
                dlq_nextEntry(last->parent);
            if (!cas) {
                /* no next case, try next object after choice */
                return obj_next_child_deep(last->parent->parent);
            } else {
                /* keep trying the next case until one with
                 * a child node is found
                 */
                while (1) {
                    next = obj_first_child(cas);
                    if (next) {
                        return next;
                    } else {
                        cas = (obj_template_t *)
                            dlq_nextEntry(cas);
                        if (!cas) {
                            /* no next case, ret. object after choice */
                            return 
                                obj_next_child_deep(last->parent->parent);
                        }
                    }
                }
                /*NOTREACHED*/
            }
        }
    }
    return NULL;

}  /* obj_next_child_deep */


/********************************************************************
* FUNCTION obj_find_all_children
* 
* Find all occurances of the specified node(s)
* within the children of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath expression
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    childname == name of child node to find
*              == NULL to match any child name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if childname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    obj_find_all_children (ncx_module_t *exprmod,
                           obj_walker_fn_t walkerfn,
                           void *cookie1,
                           void *cookie2,
                           obj_template_t *startnode,
                           const xmlChar *modname,
                           const xmlChar *childname,
                           boolean configonly,
                           boolean textmode,
                           boolean useroot)
{
    dlq_hdr_t         *datadefQ;
    obj_template_t    *obj;
    ncx_module_t      *mod;
    boolean            fnresult;

#ifdef DEBUG
    if (!exprmod || !walkerfn || !startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (exprmod && exprmod->parent) {
        /* look in the parent module, not a submodule */
        exprmod = exprmod->parent;
    }

    if (obj_is_root(startnode) && !useroot) {

        for (obj = ncx_get_first_data_object(exprmod);
             obj != NULL;
             obj = ncx_get_next_data_object(exprmod, obj)) {

            fnresult = test_one_child(exprmod,
                                      walkerfn,
                                      cookie1,
                                      cookie2,
                                      obj,
                                      modname,
                                      childname,
                                      configonly,
                                      textmode);
            if (!fnresult) {
                return FALSE;
            }
        }

        for (mod = ncx_get_first_module();
             mod != NULL;
             mod = ncx_get_next_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_child(exprmod,
                                          walkerfn,
                                          cookie1,
                                          cookie2,
                                          obj,
                                          modname,
                                          childname,
                                          configonly,
                                          textmode);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        for (mod = ncx_get_first_session_module();
             mod != NULL;
             mod = ncx_get_next_session_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_child(exprmod,
                                          walkerfn,
                                          cookie1,
                                          cookie2,
                                          obj,
                                          modname,
                                          childname,
                                          configonly,
                                          textmode);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }
    } else {

        datadefQ = obj_get_datadefQ(startnode);
        if (!datadefQ) {
            return TRUE;
        }

        for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
             obj != NULL;
             obj = (obj_template_t *)dlq_nextEntry(obj)) {

            fnresult = test_one_child(exprmod,
                                      walkerfn,
                                      cookie1,
                                      cookie2,
                                      obj,
                                      modname,
                                      childname,
                                      configonly,
                                      textmode);
            if (!fnresult) {
                return FALSE;
            }
        }
    }

    return TRUE;

}  /* obj_find_all_children */


/********************************************************************
* FUNCTION obj_find_all_ancestors
* 
* Find all occurances of the specified node(s)
* within the ancestors of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of ancestor node to find
*              == NULL to match any ancestor name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    obj_find_all_ancestors (ncx_module_t *exprmod,
                            obj_walker_fn_t walkerfn,
                            void *cookie1,
                            void *cookie2,
                            obj_template_t *startnode,
                            const xmlChar *modname,
                            const xmlChar *name,
                            boolean configonly,
                            boolean textmode,
                            boolean useroot,
                            boolean orself,
                            boolean *fncalled)
{
    obj_template_t       *obj;
    ncx_module_t         *mod;
    boolean               fnresult;

#ifdef DEBUG
    if (!exprmod || !walkerfn || !startnode || !fncalled) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    *fncalled = FALSE;

    if (orself) {
        obj = startnode;
    } else {
        obj = startnode->parent;
    }

    if (exprmod && exprmod->parent) {
        /* look in the parent module, not a submodule */
        exprmod = exprmod->parent;
    }

    if (obj && obj_is_root(obj) && !useroot) {

        for (obj = ncx_get_first_data_object(exprmod);
             obj != NULL;
             obj = ncx_get_next_data_object(exprmod, obj)) {

            fnresult = test_one_ancestor(exprmod,
                                         walkerfn,
                                         cookie1,
                                         cookie2,
                                         obj,
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         orself,
                                         fncalled);
            if (!fnresult) {
                return FALSE;
            }
        }

        for (mod = ncx_get_first_module();
             mod != NULL;
             mod = ncx_get_next_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_ancestor(exprmod,
                                             walkerfn,
                                             cookie1,
                                             cookie2,
                                             obj,
                                             modname,
                                             name,
                                             configonly,
                                             textmode,
                                             orself,
                                             fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        for (mod = ncx_get_first_session_module();
             mod != NULL;
             mod = ncx_get_next_session_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_ancestor(exprmod,
                                             walkerfn,
                                             cookie1,
                                             cookie2,
                                             obj,
                                             modname,
                                             name,
                                             configonly,
                                             textmode,
                                             orself,
                                             fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }
    } else {
        while (obj) {
            if ( obj_is_choice_or_case( obj ) ) {
                fnresult = TRUE;
            } else {
                fnresult = process_one_walker_child(walkerfn,
                                                    cookie1,
                                                    cookie2,
                                                    obj,
                                                    modname, 
                                                    name,
                                                    configonly,
                                                    textmode,
                                                    fncalled);
            }
            if (!fnresult) {
                return FALSE;
            }
            obj = obj->parent;
        }
    }

    return TRUE;

}  /* obj_find_all_ancestors */


/********************************************************************
* FUNCTION obj_find_all_descendants
* 
* Find all occurances of the specified node(s)
* within the descendants of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing XPath expression
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == start node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*    name == name of descendant node to find
*              == NULL to match any descendant name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    useroot == TRUE is it is safe to use the toproot
*               FALSE if not, use all moduleQ search instead
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    obj_find_all_descendants (ncx_module_t *exprmod,
                              obj_walker_fn_t walkerfn,
                              void *cookie1,
                              void *cookie2,
                              obj_template_t *startnode,
                              const xmlChar *modname,
                              const xmlChar *name,
                              boolean configonly,
                              boolean textmode,
                              boolean useroot,
                              boolean orself,
                              boolean *fncalled)
{
    obj_template_t       *obj;
    ncx_module_t         *mod;
    boolean               fnresult;

#ifdef DEBUG
    if (!exprmod || !walkerfn || !startnode || !fncalled) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    *fncalled = FALSE;

    if (exprmod && exprmod->parent) {
        /* look in the parent module, not a submodule */
        exprmod = exprmod->parent;
    }

    if (obj_is_root(startnode) && !useroot) {

        for (obj = ncx_get_first_data_object(exprmod);
             obj != NULL;
             obj = ncx_get_next_data_object(exprmod, obj)) {

            fnresult = test_one_descendant(exprmod,
                                           walkerfn,
                                           cookie1,
                                           cookie2,
                                           obj,
                                           modname,
                                           name,
                                           configonly,
                                           textmode,
                                           orself,
                                           fncalled);
            if (!fnresult) {
                return FALSE;
            }
        }

        for (mod = ncx_get_first_module();
             mod != NULL;
             mod = ncx_get_next_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_descendant(exprmod,
                                               walkerfn,
                                               cookie1,
                                               cookie2,
                                               obj,
                                               modname,
                                               name,
                                               configonly,
                                               textmode,
                                               orself,
                                               fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        for (mod = ncx_get_first_session_module();
             mod != NULL;
             mod = ncx_get_next_session_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_descendant(exprmod,
                                               walkerfn,
                                               cookie1,
                                               cookie2,
                                               obj,
                                               modname,
                                               name,
                                               configonly,
                                               textmode,
                                               orself,
                                               fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }
    } else {
        fnresult = test_one_descendant(exprmod,
                                       walkerfn,
                                       cookie1,
                                       cookie2,
                                       startnode,
                                       modname,
                                       name,
                                       configonly,
                                       textmode,
                                       orself,
                                       fncalled);
        if (!fnresult) {
            return FALSE;
        }
    }
    return TRUE;

}  /* obj_find_all_descendants */


/********************************************************************
* FUNCTION obj_find_all_pfaxis
* 
* Find all occurances of the specified preceding
* or following node(s).  Could also be
* within the descendants of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* This function skips choice and case nodes and
* only processes real data nodes
*
* INPUTS:
*    exprmod == module containing object
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == starting sibling node to check
*    modname == module name; 
*                only matches in this module namespace
*                will be returned
*            == NULL:
*                 namespace matching will be skipped
*
*    name == name of preceding or following node to find
*         == NULL to match any name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 1 level is checked
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*    fncalled == address of return function called flag
*
* OUTPUTS:
*   *fncalled set to TRUE if a callback function was called
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    obj_find_all_pfaxis (ncx_module_t *exprmod,
                         obj_walker_fn_t walkerfn,
                         void *cookie1,
                         void *cookie2,
                         obj_template_t *startnode,
                         const xmlChar *modname,
                         const xmlChar *name,
                         boolean configonly,
                         boolean dblslash,
                         boolean textmode,
                         boolean useroot,
                         ncx_xpath_axis_t axis,
                         boolean *fncalled)
{
    obj_template_t       *obj;
    ncx_module_t         *mod;
    boolean               fnresult, forward;

#ifdef DEBUG
    if (!exprmod || !walkerfn || !startnode || !fncalled) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    *fncalled = FALSE;

    if (exprmod && exprmod->parent) {
        /* look in the parent module, not a submodule */
        exprmod = exprmod->parent;
    }

    /* check the Q containing the startnode
     * for preceding or following nodes;
     * could be sibling node check or any node check
     */
    switch (axis) {
    case XP_AX_PRECEDING:
        dblslash = TRUE;
        /* fall through */
    case XP_AX_PRECEDING_SIBLING:
        /* execute the callback for all preceding nodes
         * that match the filter criteria 
         */
        forward = FALSE;
        break;
    case XP_AX_FOLLOWING:
        dblslash = TRUE;
        /* fall through */
    case XP_AX_FOLLOWING_SIBLING:
        forward = TRUE;
        break;
    case XP_AX_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    if (obj_is_root(startnode) && !dblslash) {
        return TRUE;
    }

    if (obj_is_root(startnode) && !useroot) {

        for (obj = ncx_get_first_data_object(exprmod);
             obj != NULL;
             obj = ncx_get_next_data_object(exprmod, obj)) {

            fnresult = test_one_pfnode(exprmod,
                                       walkerfn,
                                       cookie1,
                                       cookie2,
                                       obj,
                                       modname,
                                       name,
                                       configonly,
                                       dblslash,
                                       textmode,
                                       forward,
                                       axis,
                                       fncalled);
            if (!fnresult) {
                return FALSE;
            }
        }

        for (mod = ncx_get_first_module();
             mod != NULL;
             mod = ncx_get_next_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_pfnode(exprmod,
                                           walkerfn,
                                           cookie1,
                                           cookie2,
                                           obj,
                                           modname,
                                           name,
                                           configonly,
                                           dblslash,
                                           textmode,
                                           forward,
                                           axis,
                                           fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        for (mod = ncx_get_first_session_module();
             mod != NULL;
             mod = ncx_get_next_session_module(mod)) {

            for (obj = ncx_get_first_data_object(mod);
                 obj != NULL;
                 obj = ncx_get_next_data_object(mod, obj)) {

                fnresult = test_one_pfnode(exprmod,
                                           walkerfn,
                                           cookie1,
                                           cookie2,
                                           obj,
                                           modname,
                                           name,
                                           configonly,
                                           dblslash,
                                           textmode,
                                           forward,
                                           axis,
                                           fncalled);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }
    } else {
        fnresult = test_one_pfnode(exprmod,
                                   walkerfn,
                                   cookie1,
                                   cookie2,
                                   startnode,
                                   modname,
                                   name,
                                   configonly,
                                   dblslash,
                                   textmode,
                                   forward,
                                   axis,
                                   fncalled);
        if (!fnresult) {
            return FALSE;
        }
    }

    return TRUE;

}  /* obj_find_all_pfaxis */


/********************************************************************
* FUNCTION obj_find_case
* 
* Find a specified case arm by name
*
* INPUTS:
*    choic == choice struct to check
*    modname == name of the module that added this case (may be NULL)
*    casname == name of the case to find
*
* RETURNS:
*    pointer to obj_case_t for requested case, NULL if not found
*********************************************************************/
obj_case_t *
    obj_find_case (obj_choice_t *choic,
                   const xmlChar *modname,
                   const xmlChar *casname)
{
    obj_template_t *casobj;
    obj_case_t     *cas;

#ifdef DEBUG
    if (!choic || !casname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (casobj = (obj_template_t *)dlq_firstEntry(choic->caseQ);
         casobj != NULL;
         casobj = (obj_template_t *)dlq_nextEntry(casobj)) {

        cas = casobj->def.cas;
        if (modname && xml_strcmp(obj_get_mod_name(casobj), modname)) {
            continue;
        }

        if (!xml_strcmp(casname, cas->name)) {
            return cas;
        }
    }
    return NULL;

}  /* obj_find_case */


/********************************************************************
* FUNCTION obj_new_rpcio
* 
* Malloc and initialize the fields in a an obj_rpcio_t
* Fields are setup within the new obj_template_t, based
* on the values in rpcobj
*
* INPUTS:
*   rpcobj == parent OBJ_TYP_RPC template
*   name == name string of the node (input or output)
*
* RETURNS:
*    pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
obj_template_t * 
    obj_new_rpcio (obj_template_t *rpcobj,
                   const xmlChar *name)
{
    obj_template_t  *rpcio;

#ifdef DEBUG
    if (!rpcobj || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    rpcio = obj_new_template(OBJ_TYP_RPCIO);
    if (!rpcio) {
        return NULL;
    }
    rpcio->def.rpcio->name = xml_strdup(name);
    if (!rpcio->def.rpcio->name) {
        obj_free_template(rpcio);
        return NULL;
    }
    ncx_set_error(&rpcio->tkerr,
                  rpcobj->tkerr.mod,
                  rpcobj->tkerr.linenum,
                  rpcobj->tkerr.linepos);
    rpcio->parent = rpcobj;

    return rpcio;

}  /* obj_new_rpcio */


/********************************************************************
 * Clean and free all the obj_template_t structs in the specified Q
 *
 * \param datadefQ Q of obj_template_t to clean
 *********************************************************************/
void obj_clean_datadefQ (dlq_hdr_t *que)
{
    if (!que) {
        return;
    }

    while (!dlq_empty(que)) {
        obj_template_t *obj = (obj_template_t *)dlq_deque(que);
        obj_free_template(obj);
    }

}  /* obj_clean_datadefQ */

/********************************************************************
* FUNCTION obj_find_type
*
* Check if a typ_template_t in the obj typedefQ hierarchy
*
* INPUTS:
*   obj == obj_template using the typedef
*   typname == type name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
typ_template_t *
    obj_find_type (obj_template_t *obj,
                   const xmlChar *typname)
{
    dlq_hdr_t      *que;
    typ_template_t *typ;
    grp_template_t *testgrp;

#ifdef DEBUG
    if (!obj || !typname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* if this is a direct child of a grouping, try the tryedefQ
     * in the grouping first
     */
    if (obj->grp) {
        que = &obj->grp->typedefQ;
        typ = ncx_find_type_que(que, typname);
        if (typ) {
            return typ;
        }

        testgrp = obj->grp->parentgrp;
        while (testgrp) {
            typ = ncx_find_type_que(&testgrp->typedefQ, typname);
            if (typ) {
                return typ;
            }
            testgrp = testgrp->parentgrp;
        }
    }

    /* object not in directly in a group or nothing found
     * check if this object has a typedefQ
     */
    que = NULL;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        que = obj->def.container->typedefQ;
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        break;
    case OBJ_TYP_LIST:
        que = obj->def.list->typedefQ;
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_USES:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_AUGMENT:
        break;
    case OBJ_TYP_RPC:
        que = &obj->def.rpc->typedefQ;
        break;
    case OBJ_TYP_RPCIO:
        que = &obj->def.rpcio->typedefQ;
        break;
    case OBJ_TYP_NOTIF:
        que = &obj->def.notif->typedefQ;
        break;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (que) {
        typ = ncx_find_type_que(que, typname);
        if (typ) {
            return typ;
        }
    }

    if (obj->parent && !obj_is_root(obj->parent)) {
        return obj_find_type(obj->parent, typname);
    } else {
        return NULL;
    }

}   /* obj_find_type */


/********************************************************************
* FUNCTION obj_first_typedef
*
* Get the first local typedef for this object, if any
*
* INPUTS:
*   obj == obj_template to use
*
* RETURNS:
*  pointer to first typ_template_t struct if present, NULL otherwise
*********************************************************************/
typ_template_t *
    obj_first_typedef (obj_template_t *obj)
{
    dlq_hdr_t      *que;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    que = NULL;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        que = obj->def.container->typedefQ;
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        break;
    case OBJ_TYP_LIST:
        que = obj->def.list->typedefQ;
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_USES:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_AUGMENT:
        break;
    case OBJ_TYP_RPC:
        que = &obj->def.rpc->typedefQ;
        break;
    case OBJ_TYP_RPCIO:
        que = &obj->def.rpcio->typedefQ;
        break;
    case OBJ_TYP_NOTIF:
        que = &obj->def.notif->typedefQ;
        break;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (que) {
        return (typ_template_t *)dlq_firstEntry(que);
    }
    return NULL;

}   /* obj_first_typedef */


/********************************************************************
* FUNCTION obj_find_grouping
*
* Check if a grp_template_t in the obj groupingQ hierarchy
*
* INPUTS:
*   obj == obj_template using the grouping
*   grpname == grouping name to find
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
grp_template_t *
    obj_find_grouping (obj_template_t *obj,
                       const xmlChar *grpname)
{
    dlq_hdr_t      *que;
    grp_template_t *grp, *testgrp;

#ifdef DEBUG
    if (!obj || !grpname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* check direct nesting within a grouping chain */
    if (obj->grp) {
        grp = ncx_find_grouping_que(&obj->grp->groupingQ, grpname);
        if (grp) {
            return grp;
        }

        testgrp = obj->grp->parentgrp;
        while (testgrp) {
            if (!xml_strcmp(testgrp->name, grpname)) {
                return testgrp;
            } else {
                grp = ncx_find_grouping_que(&testgrp->groupingQ, grpname);
                if (grp) {
                    return grp;
                }
            }
            testgrp = testgrp->parentgrp;
        }
    }

    /* check the object has a groupingQ within the object chain */
    que = NULL;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        que = obj->def.container->groupingQ;
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        break;
    case OBJ_TYP_LIST:
        que = obj->def.list->groupingQ;
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_USES:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_AUGMENT:
        break;
    case OBJ_TYP_RPC:
        que = &obj->def.rpc->groupingQ;
        break;
    case OBJ_TYP_RPCIO:
        que = &obj->def.rpcio->groupingQ;
        break;
    case OBJ_TYP_NOTIF:
        que = &obj->def.notif->groupingQ;
        break;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (que) {
        grp = ncx_find_grouping_que(que, grpname);
        if (grp) {
            return grp;
        }
    }

    if (obj->parent && !obj_is_root(obj->parent)) {
        return obj_find_grouping(obj->parent, grpname);
    } else {
        return NULL;
    }

}   /* obj_find_grouping */


/********************************************************************
* FUNCTION obj_first_grouping
*
* Get the first local grouping if any
*
* INPUTS:
*   obj == obj_template to use
*
* RETURNS:
*  pointer to struct if present, NULL otherwise
*********************************************************************/
grp_template_t *
    obj_first_grouping (obj_template_t *obj)
{
    dlq_hdr_t      *que;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    que = NULL;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        que = obj->def.container->groupingQ;
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        break;
    case OBJ_TYP_LIST:
        que = obj->def.list->groupingQ;
        break;
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_USES:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_AUGMENT:
        break;
    case OBJ_TYP_RPC:
        que = &obj->def.rpc->groupingQ;
        break;
    case OBJ_TYP_RPCIO:
        que = &obj->def.rpcio->groupingQ;
        break;
    case OBJ_TYP_NOTIF:
        que = &obj->def.notif->groupingQ;
        break;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (que) {
        return (grp_template_t *)dlq_firstEntry(que);
    }
    return NULL;

}   /* obj_first_grouping */


/********************************************************************
* FUNCTION obj_set_named_type
* 
* Resolve type test 
* Called during phase 2 of module parsing
*
* INPUTS:
*   tkc == token chain
*   mod == module in progress
*   typname == name field from typ->name  (may be NULL)
*   typdef == typdef in progress
*   parent == obj_template containing this typedef
*          == NULL if this is the top-level, use mod->typeQ
*   grp == grp_template containing this typedef
*          == NULL if the typedef is not contained in a grouping
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    obj_set_named_type (tk_chain_t *tkc,
                        ncx_module_t *mod,
                        const xmlChar *typname,
                        typ_def_t *typdef,
                        obj_template_t *parent,
                        grp_template_t *grp)
{
    typ_template_t *testtyp;

    if (typdef->tclass == NCX_CL_NAMED &&
        typdef->def.named.typ==NULL) {

        /* assumed to be a named type from this module
         * because any named type from another module
         * would get resolved OK, or fail due to syntax
         * or dependency loop
         */
        if (typname && !xml_strcmp(typname, typdef->typenamestr)) {
            log_error("\nError: typedef '%s' cannot use type '%s'",
                      typname, typname);
            tkc->curerr = &typdef->tkerr;
            return ERR_NCX_DEF_LOOP;
        }

        testtyp = NULL;

        /* find the type within the specified typedef Q */
        if (typdef->typenamestr) {
            if (grp) {
                testtyp = find_type_in_grpchain(grp, 
                                                typdef->typenamestr);
            }

            if (!testtyp && parent) {
                testtyp = obj_find_type(parent, 
                                        typdef->typenamestr);
            }

            if (!testtyp) {
                testtyp = ncx_find_type(mod, 
                                        typdef->typenamestr,
                                        FALSE);
            }
        }

        if (!testtyp) {
            log_error("\nError: type '%s' not found", 
                      typdef->typenamestr);
            tkc->curerr = &typdef->tkerr;
            return ERR_NCX_UNKNOWN_TYPE;
        } else {
            typdef->def.named.typ = testtyp;
            typdef->linenum = testtyp->tkerr.linenum;
            testtyp->used = TRUE;
        }
    }
    return NO_ERR;

}   /* obj_set_named_type */


/********************************************************************
* FUNCTION obj_clone_template
*
* Clone an obj_template_t
* Copy the pointers from the srcobj into the new obj
*
* If the mobj is non-NULL, then the non-NULL revisable
* fields in the mobj struct will be merged into the new object
*
* INPUTS:
*   mod == module struct that is defining the new cloned data
*          this may be different than the module that will
*          contain the cloned data (except top-level objects)
*   srcobj == obj_template to clone
*             !!! This struct MUST NOT be deleted!!!
*             !!! Unless all of its clones are also deleted !!!
*   mobjQ == merge object Q (may be NULL)
*           datadefQ to check for OBJ_TYP_REFINE nodes
*           If the target of the refine node matches the
*           srcobj (e.g., from same grouping), then the
*           sub-clauses in that refinement-stmt that
*           are allowed to be revised will be checked
*
* RETURNS:
*   pointer to malloced clone obj_template_t
*   NULL if malloc failer error or internal error
*********************************************************************/
obj_template_t *
    obj_clone_template (ncx_module_t *mod,
                        obj_template_t *srcobj,
                        dlq_hdr_t *mobjQ)
{
    obj_template_t     *newobj, *mobj, *testobj;
    status_t            res;

#ifdef DEBUG
    if (!srcobj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (srcobj->objtype == OBJ_TYP_NONE ||
        srcobj->objtype > OBJ_TYP_AUGMENT) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

#ifdef OBJ_CLONE_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\nobj_clone: '%s' in mod '%s' on line %u",
                   obj_get_name(srcobj),
                   obj_get_mod_name(srcobj),
                   srcobj->tkerr.linenum);
    }
#endif

    newobj = new_blank_template();
    if (!newobj) {
        return NULL;
    }

    /* set most of the common fields but leave some blank 
     * since the uses or augment calling this fn is going to
     * re-parent the cloned node under a different part of the tree
     * maintain struct definition order!
     */
    mobj = NULL;
    newobj->objtype = srcobj->objtype;
    newobj->flags = (srcobj->flags | OBJ_FL_CLONE);

    if (mobjQ) {
        /* this code assumes all the refine-stmts have been normalized
         * and there is only one refine-stmt per object at this point  */
        for (testobj = (obj_template_t *)dlq_firstEntry(mobjQ);
             testobj != NULL && mobj == NULL;
             testobj = (obj_template_t *)dlq_nextEntry(testobj)) {

            if (testobj->objtype != OBJ_TYP_REFINE) {
                continue;
            }

            if (testobj->def.refine->targobj == srcobj) {
                mobj = testobj;
            }
        }
    }

    if (mobj) {
        newobj->flags |= mobj->flags;

        /* check if special flags need to be cleared */
        if ((mobj->flags & OBJ_FL_MANDSET) &&
            !(mobj->flags & OBJ_FL_MANDATORY)) {
            newobj->flags &= ~OBJ_FL_MANDATORY;
        }
        if ((mobj->flags & OBJ_FL_CONFSET) &&
            !(mobj->flags & OBJ_FL_CONFIG)) {
            newobj->flags &= ~OBJ_FL_CONFIG;
        }
    }

    ncx_set_error(&newobj->tkerr,
                  srcobj->tkerr.mod,
                  srcobj->tkerr.linenum,
                  srcobj->tkerr.linepos);

    //newobj->grp not set
    //newobj->parent not set
    //newobj->usesobj not set
    //newobj->augobj not set

    if (srcobj->when) {
        newobj->when = xpath_clone_pcb(srcobj->when);
        if (newobj->when == NULL) {
            obj_free_template(newobj);
            return NULL;
        }
    }

    //newobj->metadataQ not set

    res = clone_appinfoQ(&newobj->appinfoQ, &srcobj->appinfoQ,
                         (mobj) ? &mobj->appinfoQ : NULL);
    if (res != NO_ERR) {
        obj_free_template(newobj);
        return NULL;
    }

    res = clone_iffeatureQ(&newobj->iffeatureQ, &srcobj->iffeatureQ,
                           (mobj) ? &mobj->iffeatureQ : NULL);
    if (res != NO_ERR) {
        obj_free_template(newobj);
        return NULL;
    }
    
    //newobj->cbset not set

    newobj->mod = mod;
    newobj->nsid = mod->nsid;



    /* do not set the group in a clone */
    /* newobj->grp = srcobj->grp; */


    /* set the specific object definition type */
    switch (srcobj->objtype) {
    case OBJ_TYP_CONTAINER:
        newobj->def.container = 
            clone_container(mod, newobj, srcobj->def.container,
                            (mobj) ? mobj->def.refine : NULL, mobjQ);
        if (!newobj->def.container) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        newobj->def.leaf = 
            clone_leaf(srcobj->def.leaf,
                       (mobj) ? mobj->def.refine : NULL);
        if (!newobj->def.leaf) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_LEAF_LIST:
        newobj->def.leaflist = 
            clone_leaflist(srcobj->def.leaflist,
                           (mobj) ? mobj->def.refine : NULL);
        if (!newobj->def.leaflist) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_LIST:
        newobj->def.list = 
            clone_list(mod, newobj, srcobj,
                       (mobj) ? mobj->def.refine : NULL, mobjQ);
        if (!newobj->def.list) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_CHOICE:
        newobj->def.choic = 
            clone_choice(mod, srcobj->def.choic,
                         (mobj) ? mobj->def.refine : NULL, newobj, mobjQ);
        if (!newobj->def.choic) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_CASE:
        newobj->def.cas = 
            clone_case(mod, srcobj->def.cas,
                       (mobj) ? mobj->def.refine : NULL, newobj, mobjQ);
        if (!newobj->def.cas) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case OBJ_TYP_USES:
        if (mobj) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            /* set back pointer to uses! do not clone! */
            newobj->def.uses = srcobj->def.uses;
            newobj->flags |= OBJ_FL_DEFCLONE;
        }
        break;
    case OBJ_TYP_AUGMENT:
        if (mobj) {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            /* set back pointer to augment! do not clone! */
            newobj->def.augment = srcobj->def.augment;
            newobj->flags |= OBJ_FL_DEFCLONE;
        }
        break;
    case OBJ_TYP_NONE:
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
        obj_free_template(newobj);
        return NULL;
    } else {
        return newobj;
    }

}   /* obj_clone_template */


/********************************************************************
* FUNCTION obj_clone_template_case
*
* Clone an obj_template_t but make sure it is wrapped
* in a OBJ_TYP_CASE layer
*
* Copy the pointers from the srcobj into the new obj
*
* Create an OBJ_TYP_CASE wrapper if needed,
* for a short-case-stmt data def 
*
* If the mobj is non-NULL, then the non-NULL revisable
* fields in the mobj struct will be merged into the new object
*
* INPUTS:
*   mod == module struct that is defining the new cloned data
*          this may be different than the module that will
*          contain the cloned data (except top-level objects)
*   srcobj == obj_template to clone
*             !!! This struct MUST NOT be deleted!!!
*             !!! Unless all of its clones are also deleted !!!
*   mobjQ == Q of obj_refine_t objects to merge (may be NULL)
*           only fields allowed to be revised will be checked
*           even if other fields are set in this struct
*
* RETURNS:
*   pointer to malloced clone obj_template_t
*   NULL if malloc failer error or internal error
*********************************************************************/
obj_template_t *
    obj_clone_template_case (ncx_module_t *mod,
                             obj_template_t *srcobj,
                             dlq_hdr_t *mobjQ)
{
    obj_template_t     *casobj, *newobj;

#ifdef DEBUG
    if (!srcobj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (srcobj->objtype == OBJ_TYP_NONE ||
        srcobj->objtype > OBJ_TYP_AUGMENT) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    if (srcobj->objtype == OBJ_TYP_CASE) {
        return obj_clone_template(mod, srcobj, mobjQ);
    }

    casobj = new_blank_template();
    if (!casobj) {
        return NULL;
    }

    /* set most of the common fields but leave the mod and parent NULL
     * since the uses or augment calling this fn is going to
     * re-prent the cloned node under a different part of the tree
     */
    casobj->objtype = OBJ_TYP_CASE;
    ncx_set_error(&casobj->tkerr,
                  srcobj->tkerr.mod,
                  srcobj->tkerr.linenum,
                  srcobj->tkerr.linepos);
    casobj->flags = OBJ_FL_CLONE;
    casobj->def.cas = new_case(TRUE);
    if (!casobj->def.cas) {
        obj_free_template(casobj);
        return NULL;
    }
    casobj->def.cas->name = xml_strdup(obj_get_name(srcobj));
    if (!casobj->def.cas->name) {
        obj_free_template(casobj);
        return NULL;
    }
    casobj->def.cas->status = obj_get_status(srcobj);

    newobj = obj_clone_template(mod, srcobj, mobjQ);
    if (!newobj) {
        obj_free_template(casobj);
        return NULL;
    }

    newobj->parent = casobj;
    dlq_enque(newobj, casobj->def.cas->datadefQ);
    return casobj;

}   /* obj_clone_template_case */


/********************************************************************
* FUNCTION obj_new_unique
* 
* Alloc and Init a obj_unique_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
obj_unique_t *
    obj_new_unique (void)
{
    obj_unique_t  *un;

    un = m__getObj(obj_unique_t);
    if (!un) {
        return NULL;
    }
    obj_init_unique(un);
    return un;

}  /* obj_new_unique */


/********************************************************************
* FUNCTION obj_init_unique
* 
* Init a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to init
*********************************************************************/
void
    obj_init_unique (obj_unique_t *un)
{
#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    memset(un, 0, sizeof(obj_unique_t));
    dlq_createSQue(&un->compQ);

}  /* obj_init_unique */


/********************************************************************
* FUNCTION obj_free_unique
* 
* Free a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to free
*********************************************************************/
void
    obj_free_unique (obj_unique_t *un)
{
#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    obj_clean_unique(un);
    m__free(un);

}  /* obj_free_unique */


/********************************************************************
* FUNCTION obj_clean_unique
* 
* Clean a obj_unique_t struct
*
* INPUTS:
*   un == obj_unique_t struct to clean
*********************************************************************/
void
    obj_clean_unique (obj_unique_t *un)
{
    obj_unique_comp_t *unc;

#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (un->xpath) {
        m__free(un->xpath);
        un->xpath = NULL;
    }

    while (!dlq_empty(&un->compQ)) {
        unc = (obj_unique_comp_t *)dlq_deque(&un->compQ);
        obj_free_unique_comp(unc);
    }

}  /* obj_clean_unique */


/********************************************************************
* FUNCTION obj_new_unique_comp
* 
* Alloc and Init a obj_unique_comp_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
obj_unique_comp_t *
    obj_new_unique_comp (void)
{
    obj_unique_comp_t  *unc;

    unc = m__getObj(obj_unique_comp_t);
    if (!unc) {
        return NULL;
    }
    memset(unc, 0x0, sizeof(obj_unique_comp_t));
    return unc;

}  /* obj_new_unique_comp */


/********************************************************************
* FUNCTION obj_free_unique_comp
* 
* Free a obj_unique_comp_t struct
*
* INPUTS:
*   unc == obj_unique_comp_t struct to free
*********************************************************************/
void
    obj_free_unique_comp (obj_unique_comp_t *unc)
{
#ifdef DEBUG
    if (!unc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (unc->xpath) {
        m__free(unc->xpath);
    }
    m__free(unc);

}  /* obj_free_unique_comp */


/********************************************************************
* FUNCTION obj_find_unique
* 
* Find a specific unique-stmt
*
* INPUTS:
*    que == queue of obj_unique_t to check
*    xpath == relative path expression for the
*             unique node to find
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
obj_unique_t *
    obj_find_unique (dlq_hdr_t *que,
                     const xmlChar *xpath)
{
    obj_unique_t  *un;

#ifdef DEBUG
    if (!que || !xpath) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (un = (obj_unique_t *)dlq_firstEntry(que);
         un != NULL;
         un = (obj_unique_t *)dlq_nextEntry(un)) {
        if (!xml_strcmp(un->xpath, xpath)) {
            return un;
        }
    }
    return NULL;

}  /* obj_find_unique */


/********************************************************************
* FUNCTION obj_first_unique
* 
* Get the first unique-stmt for a list
*
* INPUTS:
*   listobj == (list) object to check for unique structs
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
obj_unique_t *
    obj_first_unique (obj_template_t *listobj)
{

#ifdef DEBUG
    if (!listobj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (listobj->objtype != OBJ_TYP_LIST) {
        return NULL;
    }

    return (obj_unique_t *)
        dlq_firstEntry(&listobj->def.list->uniqueQ);

}  /* obj_first_unique */


/********************************************************************
* FUNCTION obj_next_unique
* 
* Get the next unique-stmt for a list
*
* INPUTS:
*  un == current unique node
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
obj_unique_t *
    obj_next_unique (obj_unique_t *un)
{
#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_unique_t *)dlq_nextEntry(un);

}  /* obj_next_unique */


/********************************************************************
* FUNCTION obj_first_unique_comp
* 
* Get the first identifier in a unique-stmt for a list
*
* INPUTS:
*   un == unique struct to check
*
* RETURNS:
*   pointer to found entry or NULL if not found
*********************************************************************/
obj_unique_comp_t *
    obj_first_unique_comp (obj_unique_t *un)
{

#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_unique_comp_t *)dlq_firstEntry(&un->compQ);

}  /* obj_first_unique_comp */


/********************************************************************
* FUNCTION obj_next_unique_comp
* 
* Get the next unique-stmt component for a list
*
* INPUTS:
*  uncomp == current unique component node
*
* RETURNS:
*   pointer to next entry or NULL if none
*********************************************************************/
obj_unique_comp_t *
    obj_next_unique_comp (obj_unique_comp_t *uncomp)
{
#ifdef DEBUG
    if (!uncomp) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_unique_comp_t *)dlq_nextEntry(uncomp);

}  /* obj_next_unique_comp */


/********************************************************************
* FUNCTION obj_new_key
* 
* Alloc and Init a obj_key_t struct
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
obj_key_t *
    obj_new_key (void)
{
    obj_key_t  *key;

    key = m__getObj(obj_key_t);
    if (!key) {
        return NULL;
    }
    memset(key, 0x0, sizeof(obj_key_t));
    return key;

}  /* obj_new_key */


/********************************************************************
* FUNCTION obj_free_key
* 
* Free a obj_key_t struct
*
* INPUTS:
*   key == obj_key_t struct to free
*********************************************************************/
void
    obj_free_key (obj_key_t *key)
{
#ifdef DEBUG
    if (!key) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    m__free(key);

}  /* obj_free_key */


/********************************************************************
* FUNCTION obj_find_key
* 
* Find a specific key component by key leaf identifier name
* Assumes deep keys are not supported!!!
*
* INPUTS:
*   que == Q of obj_key_t to check
*   keycompname == key component name to find
*
* RETURNS:
*   pointer to found key component or NULL if not found
*********************************************************************/
obj_key_t *
    obj_find_key (dlq_hdr_t *que,
                  const xmlChar *keycompname)
{
    obj_key_t  *key;

#ifdef DEBUG
    if (!que || !keycompname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (key = (obj_key_t *)dlq_firstEntry(que);
         key != NULL;
         key = (obj_key_t *)dlq_nextEntry(key)) {
        if (!xml_strcmp(obj_get_name(key->keyobj), keycompname)) {
            return key;
        }
    }
    return NULL;

}  /* obj_find_key */


/********************************************************************
* FUNCTION obj_find_key2
* 
* Find a specific key component, check for a specific node
* in case deep keys are supported, and to check for duplicates
*
* INPUTS:
*   que == Q of obj_key_t to check
*   keyobj == key component object to find
*
* RETURNS:
*   pointer to found key component or NULL if not found
*********************************************************************/
obj_key_t *
    obj_find_key2 (dlq_hdr_t *que,
                   obj_template_t *keyobj)
{
    obj_key_t  *key;

#ifdef DEBUG
    if (!que || !keyobj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (key = (obj_key_t *)dlq_firstEntry(que);
         key != NULL;
         key = (obj_key_t *)dlq_nextEntry(key)) {
        if (keyobj == key->keyobj) {
            return key;
        }
    }
    return NULL;

}  /* obj_find_key2 */


/********************************************************************
* FUNCTION obj_first_key
* 
* Get the first key record
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   pointer to first key component or NULL if not found
*********************************************************************/
obj_key_t *
    obj_first_key (obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    if (obj->objtype != OBJ_TYP_LIST) {
        return NULL;
    }

    return (obj_key_t *)dlq_firstEntry(&obj->def.list->keyQ);

}  /* obj_first_key */


/********************************************************************
* FUNCTION obj_first_ckey
* 
* Get the first key record: Const version
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   pointer to first key component or NULL if not found
*********************************************************************/
const obj_key_t *
    obj_first_ckey (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    if (obj->objtype != OBJ_TYP_LIST) {
        return NULL;
    }

    return (const obj_key_t *)dlq_firstEntry(&obj->def.list->keyQ);

}  /* obj_first_ckey */


/********************************************************************
* FUNCTION obj_next_key
* 
* Get the next key record
*
* INPUTS:
*   objkey == current key record
*
* RETURNS:
*   pointer to next key component or NULL if not found
*********************************************************************/
obj_key_t *
    obj_next_key (obj_key_t *objkey)
{
#ifdef DEBUG
    if (!objkey) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_key_t *)dlq_nextEntry(objkey);

}  /* obj_next_key */


/********************************************************************
* FUNCTION obj_next_ckey
* 
* Get the next key record: Const version
*
* INPUTS:
*   objkey == current key record
*
* RETURNS:
*   pointer to next key component or NULL if not found
*********************************************************************/
const obj_key_t *
    obj_next_ckey (const obj_key_t *objkey)
{
#ifdef DEBUG
    if (!objkey) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (const obj_key_t *)dlq_nextEntry(objkey);

}  /* obj_next_ckey */


/********************************************************************
* FUNCTION obj_key_count
* 
* Get the number of keys for this object
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*   number of keys in the obj_key_t Q
*********************************************************************/
uint32
    obj_key_count (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (obj->objtype != OBJ_TYP_LIST) {
        return 0;
    }

    return dlq_count(&obj->def.list->keyQ);

}  /* obj_key_count */


/********************************************************************
* FUNCTION obj_key_count_to_root
* 
* Check ancestor-or-self nodes until root reached
* Find all lists; Count the number of keys
*
* INPUTS:
*   obj == object to start check from
* RETURNS:
*   number of keys in ancestor-or-self nodes
*********************************************************************/
uint32
    obj_key_count_to_root (obj_template_t *obj)
{
    uint32 count = 0;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (obj_is_root(obj)) {
        return 0;
    }

    obj_traverse_keys(obj, (void *)&count, NULL, count_keys);

    return count;
    
}  /* obj_key_count_to_root */


/********************************************************************
* FUNCTION obj_traverse_keys
* 
* Check ancestor-or-self nodes until root reached
* Find all lists; For each list, starting with the
* closest to root, invoke the callback function
* for each of the key objects in order
*
* INPUTS:
*   obj == object to start check from
*   cookie1 == cookie1 to pass to the callback function
*   cookie2 == cookie2 to pass to the callback function
*   walkerfn == walker callback function
*           returns FALSE to terminate traversal
*
*********************************************************************/
void
    obj_traverse_keys (obj_template_t *obj,
                       void *cookie1,
                       void *cookie2,
                       obj_walker_fn_t walkerfn)
{
    obj_key_t *objkey;

#ifdef DEBUG
    if (!obj || !walkerfn) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (obj_is_root(obj)) {
        return;
    }

    if (obj->parent != NULL) {
        obj_traverse_keys(obj->parent, cookie1, cookie2, walkerfn);
    }

    if (obj->objtype != OBJ_TYP_LIST) {
        return;
    }

    for (objkey = obj_first_key(obj);
         objkey != NULL;
         objkey = obj_next_key(objkey)) {

        if (objkey->keyobj) {
            boolean ret = (*walkerfn)(objkey->keyobj, cookie1, cookie2);
            if (!ret) {
                return;
            }
        } // else some error; skip this key!!!
    }
    
}  /* obj_traverse_keys */


/********************************************************************
* FUNCTION obj_any_rpcs
* 
* Check if there are any RPC methods in the datadefQ
*
* INPUTS:
*   que == Q of obj_template_t to check
*
* RETURNS:
*   TRUE if any OBJ_TYP_RPC found, FALSE if not
*********************************************************************/
boolean
    obj_any_rpcs (const dlq_hdr_t *datadefQ)
{
    const obj_template_t  *obj;

#ifdef DEBUG
    if (!datadefQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    for (obj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {
        if (obj->objtype == OBJ_TYP_RPC) {
            return TRUE;
        }
    }
    return FALSE;

}  /* obj_any_rpcs */


/********************************************************************
* FUNCTION obj_any_notifs
* 
* Check if there are any notifications in the datadefQ
*
* INPUTS:
*   que == Q of obj_template_t to check
*
* RETURNS:
*   TRUE if any OBJ_TYP_NOTIF found, FALSE if not
*********************************************************************/
boolean
    obj_any_notifs (const dlq_hdr_t *datadefQ)
{
    const obj_template_t  *obj;

#ifdef DEBUG
    if (!datadefQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    for (obj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {
        if (obj->objtype == OBJ_TYP_NOTIF) {
            return TRUE;
        }
    }
    return FALSE;

}  /* obj_any_notifs */


/********************************************************************
* FUNCTION obj_new_deviate
* 
* Malloc and initialize the fields in a an object deviate statement
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
obj_deviate_t * 
    obj_new_deviate (void)
{
    obj_deviate_t  *deviate;

    deviate = m__getObj(obj_deviate_t);
    if (!deviate) {
        return NULL;
    }

    memset(deviate, 0x0, sizeof(obj_deviate_t));

    dlq_createSQue(&deviate->mustQ);
    dlq_createSQue(&deviate->uniqueQ);
    dlq_createSQue(&deviate->appinfoQ);

    return deviate;

} /* obj_new_deviate */


/********************************************************************
* Clean and free an object deviate statement
* \param deviate the pointer to the struct to clean and free
*********************************************************************/
void obj_free_deviate (obj_deviate_t *deviate)
{
    if (!deviate) {
        return;
    }

    typ_free_typdef(deviate->typdef);
    m__free(deviate->units);
    m__free(deviate->defval);

    clean_mustQ(&deviate->mustQ);

    free_uniqueQ( &deviate->uniqueQ );
    ncx_clean_appinfoQ(&deviate->appinfoQ);

    m__free(deviate);

} /* obj_free_deviate */


/********************************************************************
* FUNCTION obj_get_deviate_arg
* 
* Get the deviate-arg string from its enumeration
*
* INPUTS:
*   devarg == enumeration to convert
* RETURNS:
*   const string version of the enum
*********************************************************************/
const xmlChar *
    obj_get_deviate_arg (obj_deviate_arg_t devarg)
{
    switch (devarg) {
    case OBJ_DARG_NONE:
        return NCX_EL_NONE;
    case OBJ_DARG_ADD:
        return YANG_K_ADD;
    case OBJ_DARG_DELETE:
        return YANG_K_DELETE;
    case OBJ_DARG_REPLACE:
        return YANG_K_REPLACE;
    case OBJ_DARG_NOT_SUPPORTED:
        return YANG_K_NOT_SUPPORTED;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return (const xmlChar *)"--";
    }

} /* obj_get_deviate_arg */


/********************************************************************
* FUNCTION obj_new_deviation
* 
* Malloc and initialize the fields in a an object deviation statement
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
obj_deviation_t * 
    obj_new_deviation (void)
{
    obj_deviation_t  *deviation;

    deviation = m__getObj(obj_deviation_t);
    if (!deviation) {
        return NULL;
    }

    memset(deviation, 0x0, sizeof(obj_deviation_t));

    dlq_createSQue(&deviation->deviateQ);
    dlq_createSQue(&deviation->appinfoQ);

    return deviation;

} /* obj_new_deviation */


/********************************************************************
* Clean and free an object deviation statement

* \param deviation the pointer to the struct to clean and free
*********************************************************************/
void obj_free_deviation (obj_deviation_t *deviation)
{
    if (!deviation) {
        return;
    }

    m__free(deviation->target);
    m__free(deviation->targmodname);
    m__free(deviation->descr);
    m__free(deviation->ref);
    m__free(deviation->devmodname);

    while (!dlq_empty(&deviation->deviateQ)) {
        obj_deviate_t *deviate = 
            (obj_deviate_t *) dlq_deque(&deviation->deviateQ);
        obj_free_deviate(deviate);
    }

    ncx_clean_appinfoQ(&deviation->appinfoQ);
    m__free(deviation);
} /* obj_free_deviation */


/********************************************************************
 * Clean and free an Q of object deviation statements
 *
 * \param deviationQ the pointer to Q of the structs to clean and free
 *********************************************************************/
void obj_clean_deviationQ (dlq_hdr_t *deviationQ)
{
    if (!deviationQ) {
        return;
    }

    while (!dlq_empty(deviationQ)) {
        obj_deviation_t *deviation = (obj_deviation_t *)dlq_deque(deviationQ);
        obj_free_deviation(deviation);
    }

} /* obj_clean_deviationQ */


/********************************************************************
 * Malloc and Generate the object ID for an object node
 * 
 * \param obj the node to generate the instance ID for
 * \param buff the pointer to address of buffer to use
 * \return status
 *********************************************************************/
status_t obj_gen_object_id (const obj_template_t *obj, xmlChar  **buff)
{
    uint32    len;
    status_t  res;

#ifdef DEBUG 
    if (!obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *buff = NULL;

    /* figure out the length of the object ID */
    res = get_object_string(obj, NULL, NULL, 0, TRUE, NULL, &len, 
                            FALSE, FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    }

    /* get the object ID for real this time */
    res = get_object_string(obj, NULL, *buff, len+1, TRUE, NULL, &len, 
                            FALSE, FALSE);
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
        return SET_ERROR(res);
    }

    return NO_ERR;

}  /* obj_gen_object_id */


/********************************************************************
 * Malloc and Generate the object ID for an object node
 * Remove all conceptual OBJ_TYP_CHOICE and OBJ_TYP_CASE nodes
 * so the resulting string will represent the structure of the
 * value tree for XPath searching
 *
 * \param obj the node to generate the instance ID for
 * \param buff the pointer to address of buffer to use
 * \return status
 *********************************************************************/
status_t obj_gen_object_id_xpath (const obj_template_t *obj, xmlChar  **buff)
{
    uint32    len;
    status_t  res;

#ifdef DEBUG 
    if (!obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *buff = NULL;

    /* figure out the length of the object ID */
    res = get_object_string(obj, NULL, NULL, 0, TRUE, NULL, &len, FALSE, TRUE);
    if (res != NO_ERR) {
        return res;
    }

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    }

    /* get the object ID for real this time */
    res = get_object_string(obj, NULL, *buff, len+1, TRUE, NULL, &len, 
                            FALSE, TRUE);
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
        return SET_ERROR(res);
    }

    return NO_ERR;

}  /* obj_gen_object_id_xpath */


/********************************************************************
 * Malloc and Generate the object ID for a unique-stmt test
 *
 * \param obj the node to generate the instance ID for
 * \param stopobj the ancestor node to stop at
 * \param buff the pointer to address of buffer to use
 * \return status
 *********************************************************************/
status_t obj_gen_object_id_unique (const obj_template_t *obj, 
                                   const obj_template_t *stopobj,
                                   xmlChar  **buff)
{
    uint32    len;
    status_t  res;

    assert( obj && "obj is NULL!" );
    assert( stopobj && "stopobj is NULL!" );
    assert( buff && "buff is NULL!" );

    *buff = NULL;

    /* figure out the length of the object ID */
    res = get_object_string(obj, stopobj, NULL, 0, TRUE, NULL, &len, 
                            FALSE, TRUE);
    if (res != NO_ERR) {
        return res;
    }

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    }

    /* get the object ID for real this time */
    res = get_object_string(obj, stopobj, *buff, len+1, TRUE, NULL, &len, 
                            FALSE, TRUE);
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
        return SET_ERROR(res);
    }

    return NO_ERR;

}  /* obj_gen_object_id_unique */


/********************************************************************
* FUNCTION obj_gen_object_id_code
* 
* Malloc and Generate the object ID for an object node
* for C code usage
* generate a unique name for C code; handles augments
*
* INPUTS:
*   mod == current module in progress
*   obj == node to generate the instance ID for
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
status_t
    obj_gen_object_id_code (ncx_module_t *mod,
                            const obj_template_t *obj,
                            xmlChar  **buff)
{
    uint32    len;
    status_t  res;

#ifdef DEBUG 
    if (!mod || !obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *buff = NULL;

    /* figure out the length of the object ID */
    res = get_object_string(obj, NULL, NULL, 0, TRUE, mod, &len, FALSE, FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    }

    /* get the object ID for real this time */
    res = get_object_string(obj, NULL, *buff, len+1, TRUE, mod, &len, 
                            FALSE, FALSE);
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
        return SET_ERROR(res);
    }

    return NO_ERR;

}  /* obj_gen_object_id_code */


/********************************************************************
* FUNCTION obj_copy_object_id
* 
* Generate the object ID for an object node and copy to the buffer
* copy an object ID to a buffer
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   buff == buffer to use
*   bufflen == size of buff
*   reallen == address of return length of actual identifier 
*               (may be NULL)
*
* OUTPUTS
*   buff == filled in with the object ID
*  if reallen not NULL:
*     *reallen == length of identifier, even if error occurred
*  
* RETURNS:
*   status
*********************************************************************/
status_t
    obj_copy_object_id (const obj_template_t *obj,
                        xmlChar  *buff,
                        uint32 bufflen,
                        uint32 *reallen)
{
#ifdef DEBUG 
    if (!obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    return get_object_string(obj, NULL, buff, bufflen, TRUE, NULL, reallen, 
                             FALSE, FALSE);

}  /* obj_copy_object_id */


/********************************************************************
* FUNCTION obj_copy_object_id_mod
* 
* Generate the object ID for an object node and copy to the buffer
* copy an object ID to a buffer; Use modname in object identifier
* 
* INPUTS:
*   obj == node to generate the instance ID for
*   buff == buffer to use
*   bufflen == size of buff
*   reallen == address of return length of actual identifier 
*               (may be NULL)
*
* OUTPUTS
*   buff == filled in with the object ID
*  if reallen not NULL:
*     *reallen == length of identifier, even if error occurred
*  
* RETURNS:
*   status
*********************************************************************/
status_t
    obj_copy_object_id_mod (const obj_template_t *obj,
                            xmlChar  *buff,
                            uint32 bufflen,
                            uint32 *reallen)
{
#ifdef DEBUG 
    if (!obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    return get_object_string(obj, NULL, buff, bufflen, TRUE, NULL, reallen, 
                             TRUE, FALSE);

}  /* obj_copy_object_id_mod */


/********************************************************************
* FUNCTION obj_gen_aughook_id
* 
* Malloc and Generate the augment hook element name for
* the specified object. This will be a child node of the
* specified object.
* 
* INPUTS:
*   obj == node to generate the augment hook ID for
*   buff == pointer to address of buffer to use
*
* OUTPUTS
*   *buff == malloced buffer with the instance ID
*
* RETURNS:
*   status
*********************************************************************/
status_t
    obj_gen_aughook_id (const obj_template_t *obj,
                        xmlChar  **buff)
{
    xmlChar  *p;
    uint32    len, extra;
    status_t  res;

#ifdef DEBUG 
    if (!obj || !buff) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    *buff = NULL;

    /* figure out the length of the aughook ID */
    res = get_object_string(obj, NULL, NULL, 0, FALSE, NULL, &len, 
                            FALSE, FALSE);
    if (res != NO_ERR) {
        return res;
    }

    /* get the length for the aughook prefix and suffix */
    extra = (xml_strlen(NCX_AUGHOOK_START) + xml_strlen(NCX_AUGHOOK_END));

    /* get a buffer to fit the instance ID string */
    *buff = (xmlChar *)m__getMem(len+extra+1);
    if (!*buff) {
        return ERR_INTERNAL_MEM;
    }

    /* put prefix in buffer */
    p = *buff;
    p += xml_strcpy(p, NCX_AUGHOOK_START);
    
    /* add the aughook ID to the buffer */
    res = get_object_string(obj, NULL, p, len+1, FALSE, NULL, &len, 
                            FALSE, FALSE);
    if (res != NO_ERR) {
        m__free(*buff);
        *buff = NULL;
        return SET_ERROR(res);
    }

    /* add suffix to the buffer */
    p += len;
    xml_strcpy(p, NCX_AUGHOOK_END);

    return NO_ERR;

}  /* obj_gen_aughook_id */


/********************************************************************
* FUNCTION obj_get_name
* 
* Get the name field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the name field, NULL if some error or unnamed
*********************************************************************/
const xmlChar * 
    obj_get_name (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return (const xmlChar *)"<none>";
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->name;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return obj->def.leaf->name;
    case OBJ_TYP_LEAF_LIST:
        return obj->def.leaflist->name;
    case OBJ_TYP_LIST:
        return obj->def.list->name;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->name;
    case OBJ_TYP_CASE:
        return obj->def.cas->name;
    case OBJ_TYP_USES:
        return YANG_K_USES;
    case OBJ_TYP_AUGMENT:
        return YANG_K_AUGMENT;
    case OBJ_TYP_REFINE:
        return YANG_K_REFINE;
    case OBJ_TYP_RPC:
        return obj->def.rpc->name;
    case OBJ_TYP_RPCIO:
        return obj->def.rpcio->name;
    case OBJ_TYP_NOTIF:
        return obj->def.notif->name;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_EL_NONE;
    }
    /*NOTREACHED*/

}  /* obj_get_name */


/********************************************************************
* FUNCTION obj_set_name
* 
* Set the name field for this obj
*
* INPUTS:
*   obj == the specific object to set or change the name
*   objname == new name string to use
*
* RETURNS:
*   status
*********************************************************************/
status_t
    obj_set_name (obj_template_t *obj,
                  const xmlChar *objname)
{
    xmlChar  **namevar, *newname;
    boolean   *nameclone, defnameclone;

#ifdef DEBUG
    if (obj == NULL || objname == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    namevar = NULL;
    defnameclone = FALSE;
    nameclone = &defnameclone;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        namevar = &obj->def.container->name;
        break;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        namevar = &obj->def.leaf->name;
        break;
    case OBJ_TYP_LEAF_LIST:
        namevar = &obj->def.leaflist->name;
        break;
    case OBJ_TYP_LIST:
        namevar = &obj->def.list->name;
        break;
    case OBJ_TYP_CHOICE:
        namevar = &obj->def.choic->name;
        break;
    case OBJ_TYP_CASE:
        namevar = &obj->def.cas->name;
        nameclone = &obj->def.cas->nameclone;
        break;
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        return ERR_NCX_SKIPPED;
    case OBJ_TYP_RPC:
        namevar = &obj->def.rpc->name;
        break;
    case OBJ_TYP_RPCIO:
        namevar = &obj->def.rpcio->name;
        break;
    case OBJ_TYP_NOTIF:
        namevar = &obj->def.notif->name;
        break;
    case OBJ_TYP_NONE:
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    newname = xml_strdup(objname);
    if (newname == NULL) {
        return ERR_INTERNAL_MEM;
    }

    if (*namevar != NULL && !*nameclone) {
        m__free(*namevar);
        *namevar = NULL;
    }

    *namevar = newname;
    *nameclone = TRUE;

    return NO_ERR;

}  /* obj_set_name */


/********************************************************************
* FUNCTION obj_has_name
* 
* Check if the specified object type has a name
*
* this function is used throughout the code to 
* filter out uses and augment nodes from the
* real nodes.  Those are the only YANG nodes that
* do not have a name assigned to them
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   TRUE if obj has a name
*   FALSE otherwise
*********************************************************************/
boolean
    obj_has_name (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
        return TRUE;
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        return FALSE;
    case OBJ_TYP_RPC:
    case OBJ_TYP_RPCIO:
    case OBJ_TYP_NOTIF:
        return TRUE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}  /* obj_has_name */


/********************************************************************
* FUNCTION obj_has_text_content
* 
* Check if the specified object type has a text content
* for XPath purposes
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   TRUE if obj has text content
*   FALSE otherwise
*********************************************************************/
boolean
    obj_has_text_content (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
        return TRUE;
    default:
        return FALSE;
    }
    /*NOTREACHED*/

}  /* obj_has_text_content */


/********************************************************************
* FUNCTION obj_get_status
* 
* Get the status field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG status clause for this object
*********************************************************************/
ncx_status_t
    obj_get_status (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_STATUS_NONE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->status;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return obj->def.leaf->status;
    case OBJ_TYP_LEAF_LIST:
        return obj->def.leaflist->status;
    case OBJ_TYP_LIST:
        return obj->def.list->status;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->status;
    case OBJ_TYP_CASE:
    case OBJ_TYP_REFINE:
        return (obj->parent) ?
            obj_get_status(obj->parent) : NCX_STATUS_CURRENT;
    case OBJ_TYP_USES:
        return obj->def.uses->status;
    case OBJ_TYP_AUGMENT:
        return obj->def.augment->status;
    case OBJ_TYP_RPC:
        return obj->def.rpc->status;
    case OBJ_TYP_RPCIO:
        return (obj->parent) ?
            obj_get_status(obj->parent) : NCX_STATUS_CURRENT;
    case OBJ_TYP_NOTIF:
        return obj->def.notif->status;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_STATUS_NONE;
    }
    /*NOTREACHED*/

}  /* obj_get_status */


/********************************************************************
* FUNCTION obj_get_description
* 
* Get the description field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
const xmlChar *
    obj_get_description (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->descr;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return obj->def.leaf->descr;
    case OBJ_TYP_LEAF_LIST:
        return obj->def.leaflist->descr;
    case OBJ_TYP_LIST:
        return obj->def.list->descr;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->descr;
    case OBJ_TYP_CASE:
        return obj->def.cas->descr;
    case OBJ_TYP_USES:
        return obj->def.uses->descr;
    case OBJ_TYP_REFINE:
        return obj->def.refine->descr;
    case OBJ_TYP_AUGMENT:
        return obj->def.augment->descr;
    case OBJ_TYP_RPC:
        return obj->def.rpc->descr;
    case OBJ_TYP_RPCIO:
        return NULL;
    case OBJ_TYP_NOTIF:
        return obj->def.notif->descr;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_description */


/********************************************************************
* FUNCTION obj_get_alt_description
* 
* Get the alternate description field for this obj
* Check if any 'info', then 'help' appinfo nodes present
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
const xmlChar *
    obj_get_alt_description (const obj_template_t *obj)
{
    const ncx_appinfo_t *appinfo;
    const xmlChar *altdescr;

#ifdef DEBUG 
    if (obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    altdescr = NULL;
    appinfo = ncx_find_const_appinfo(&obj->appinfoQ,
                                     NULL, /* any module */
                                     NCX_EL_INFO);
    if (appinfo != NULL) {
        altdescr = ncx_get_appinfo_value(appinfo);
    }

    if (altdescr != NULL) {
        return altdescr;
    }

    appinfo = ncx_find_const_appinfo(&obj->appinfoQ,
                                     NULL, /* any module */
                                     NCX_EL_HELP);
    if (appinfo != NULL) {
        altdescr = ncx_get_appinfo_value(appinfo);
    }

    return altdescr;

}  /* obj_get_alt_description */


/********************************************************************
* FUNCTION obj_get_description_addr
* 
* Get the address of the description field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG description string for this object
*********************************************************************/
const void *
    obj_get_description_addr (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return &obj->def.container->descr;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return &obj->def.leaf->descr;
    case OBJ_TYP_LEAF_LIST:
        return &obj->def.leaflist->descr;
    case OBJ_TYP_LIST:
        return &obj->def.list->descr;
    case OBJ_TYP_CHOICE:
        return &obj->def.choic->descr;
    case OBJ_TYP_CASE:
        return &obj->def.cas->descr;
    case OBJ_TYP_USES:
        return &obj->def.uses->descr;
    case OBJ_TYP_REFINE:
        return &obj->def.refine->descr;
    case OBJ_TYP_AUGMENT:
        return &obj->def.augment->descr;
    case OBJ_TYP_RPC:
        return &obj->def.rpc->descr;
    case OBJ_TYP_RPCIO:
        return NULL;
    case OBJ_TYP_NOTIF:
        return &obj->def.notif->descr;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_description_addr */


/********************************************************************
* FUNCTION obj_get_reference
* 
* Get the reference field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG reference string for this object
*********************************************************************/
const xmlChar *
    obj_get_reference (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->ref;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return obj->def.leaf->ref;
    case OBJ_TYP_LEAF_LIST:
        return obj->def.leaflist->ref;
    case OBJ_TYP_LIST:
        return obj->def.list->ref;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->ref;
    case OBJ_TYP_CASE:
        return obj->def.cas->ref;
    case OBJ_TYP_USES:
        return obj->def.uses->ref;
    case OBJ_TYP_REFINE:
        return obj->def.refine->ref;
    case OBJ_TYP_AUGMENT:
        return obj->def.augment->ref;
    case OBJ_TYP_RPC:
        return obj->def.rpc->ref;
    case OBJ_TYP_RPCIO:
        return NULL;
    case OBJ_TYP_NOTIF:
        return obj->def.notif->ref;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_reference */


/********************************************************************
* FUNCTION obj_get_reference_addr
* 
* Get the reference field for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   YANG reference string for this object
*********************************************************************/
const void *
    obj_get_reference_addr (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return &obj->def.container->ref;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return &obj->def.leaf->ref;
    case OBJ_TYP_LEAF_LIST:
        return &obj->def.leaflist->ref;
    case OBJ_TYP_LIST:
        return &obj->def.list->ref;
    case OBJ_TYP_CHOICE:
        return &obj->def.choic->ref;
    case OBJ_TYP_CASE:
        return &obj->def.cas->ref;
    case OBJ_TYP_USES:
        return &obj->def.uses->ref;
    case OBJ_TYP_REFINE:
        return &obj->def.refine->ref;
    case OBJ_TYP_AUGMENT:
        return &obj->def.augment->ref;
    case OBJ_TYP_RPC:
        return &obj->def.rpc->ref;
    case OBJ_TYP_RPCIO:
        return NULL;
    case OBJ_TYP_NOTIF:
        return &obj->def.notif->ref;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_reference_addr */


/********************************************************************
* FUNCTION obj_get_config_flag
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
boolean
    obj_get_config_flag (const obj_template_t *obj)
{
    boolean retval;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    retval = obj_get_config_flag_deep(obj);
    return retval;

}   /* obj_get_config_flag */


/********************************************************************
* FUNCTION obj_get_config_flag2
*
* Get the config flag for an obj_template_t 
* Return the explicit value or the inherited value
* Also return if the config-stmt is really set or not
*
* INPUTS:
*   obj == obj_template to check
*   setflag == address of return config-stmt set flag
*
* OUTPUTS:
*   *setflag == TRUE if the config-stmt is set in this
*               node, or if it is a top-level object
*            == FALSE if the config-stmt is inherited from its parent
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   
*********************************************************************/
boolean
    obj_get_config_flag2 (const obj_template_t *obj,
                         boolean *setflag)
{
#ifdef DEBUG
    if (!obj || !setflag) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif
    return get_config_flag(obj, setflag);

}   /* obj_get_config_flag2 */


/********************************************************************
* FUNCTION obj_get_max_access
*
* Get the NCX max-access enum for an obj_template_t 
* Return the explicit value or the inherited value
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   ncx_access_t enumeration
*********************************************************************/
ncx_access_t
    obj_get_max_access (const obj_template_t *obj)
{
    boolean      retval, setflag, done;

#ifdef DEBUG
    if (obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_ACCESS_NONE;
    }
#endif

    done = FALSE;
    while (!done) {
        setflag = FALSE;
        retval = get_config_flag(obj, &setflag);
        if (setflag) {
            done = TRUE;
        } else {
            obj = obj->parent;
            if (obj == NULL || obj_is_root(obj)) {
                done = TRUE;
            }
        }
    }
    if (setflag) {
        return (retval) ? NCX_ACCESS_RC : NCX_ACCESS_RO;
    } else {
        /* top-level not set defaults to config */
        return NCX_ACCESS_RC;
    }

}   /* obj_get_max_access */


/********************************************************************
* FUNCTION obj_get_appinfoQ
* 
* Get the appinfoQ for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the appinfoQ for this object
*********************************************************************/
dlq_hdr_t *
    obj_get_appinfoQ (obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return &obj->appinfoQ;

}  /* obj_get_appinfoQ */


/********************************************************************
* FUNCTION obj_get_mustQ
* 
* Get the mustQ for this obj
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the mustQ for this object
*********************************************************************/
dlq_hdr_t *
    obj_get_mustQ (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return &obj->def.container->mustQ;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
        return &obj->def.leaf->mustQ;
    case OBJ_TYP_LEAF_LIST:
        return &obj->def.leaflist->mustQ;
    case OBJ_TYP_LIST:
        return &obj->def.list->mustQ;
    case OBJ_TYP_REFINE:
        return &obj->def.refine->mustQ;
    default:
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_mustQ */


/********************************************************************
* FUNCTION obj_get_typestr
*
* Get the name of the object type
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   name string for this object type
*********************************************************************/
const xmlChar *
    obj_get_typestr (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_EL_NONE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return YANG_K_CONTAINER;
    case OBJ_TYP_ANYXML:
        return YANG_K_ANYXML;
    case OBJ_TYP_LEAF:
        return YANG_K_LEAF;
    case OBJ_TYP_LEAF_LIST:
        return YANG_K_LEAF_LIST;
    case OBJ_TYP_LIST:
        return YANG_K_LIST;
    case OBJ_TYP_CHOICE:
        return YANG_K_CHOICE;
    case OBJ_TYP_CASE:
        return YANG_K_CASE;
    case OBJ_TYP_USES:
        return YANG_K_USES;
    case OBJ_TYP_REFINE:
        return YANG_K_REFINE;
    case OBJ_TYP_AUGMENT:
        return YANG_K_AUGMENT;
    case OBJ_TYP_RPC:
        return YANG_K_RPC;
    case OBJ_TYP_RPCIO:
        return YANG_K_CONTAINER;
    case OBJ_TYP_NOTIF:
        return YANG_K_NOTIFICATION;
    case OBJ_TYP_NONE:
        return NCX_EL_NONE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_EL_NONE;
    }
    /*NOTREACHED*/

}   /* obj_get_typestr */


/********************************************************************
* FUNCTION obj_get_datadefQ
*
* Get the datadefQ (or caseQ) if this object has one
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*    pointer to Q of obj_template, or NULL if none
*********************************************************************/
dlq_hdr_t *
    obj_get_datadefQ (obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->datadefQ;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_REFINE:
        return NULL;
    case OBJ_TYP_LIST:
        return obj->def.list->datadefQ;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->caseQ;
    case OBJ_TYP_CASE:
        return obj->def.cas->datadefQ;
    case OBJ_TYP_USES:
        return obj->def.uses->datadefQ;
    case OBJ_TYP_AUGMENT:
        return &obj->def.augment->datadefQ;
    case OBJ_TYP_RPC:
        return &obj->def.rpc->datadefQ;
    case OBJ_TYP_RPCIO:
        return &obj->def.rpcio->datadefQ;
    case OBJ_TYP_NOTIF:
        return &obj->def.notif->datadefQ;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}   /* obj_get_datadefQ */


/********************************************************************
* FUNCTION obj_get_cdatadefQ
*
* Get a const pointer to the datadefQ (or caseQ) if this object has one
*
* INPUTS:
*   obj == object to check
*
* RETURNS:
*    pointer to Q of obj_template, or NULL if none
*********************************************************************/
const dlq_hdr_t *
    obj_get_cdatadefQ (const obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->datadefQ;
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_REFINE:
        return NULL;
    case OBJ_TYP_LIST:
        return obj->def.list->datadefQ;
    case OBJ_TYP_CHOICE:
        return obj->def.choic->caseQ;
    case OBJ_TYP_CASE:
        return obj->def.cas->datadefQ;
    case OBJ_TYP_USES:
        return obj->def.uses->datadefQ;
    case OBJ_TYP_AUGMENT:
        return &obj->def.augment->datadefQ;
    case OBJ_TYP_RPC:
        return &obj->def.rpc->datadefQ;
    case OBJ_TYP_RPCIO:
        return &obj->def.rpcio->datadefQ;
    case OBJ_TYP_NOTIF:
        return &obj->def.notif->datadefQ;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}   /* obj_get_cdatadefQ */


/********************************************************************
* FUNCTION obj_get_default
* 
* Get the default value for the specified object
* Only OBJ_TYP_LEAF objtype is supported
* If the leaf has nodefault, then the type is checked
* Choice defaults are ignored.
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   pointer to default value string or NULL if none
*********************************************************************/
const xmlChar *
    obj_get_default (const obj_template_t *obj)
{
#ifdef DEBUG 
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (obj->objtype != OBJ_TYP_LEAF) {
        return NULL;
    }
    if (obj->def.leaf->defval) {
        return obj->def.leaf->defval;
    }
    return typ_get_default(obj->def.leaf->typdef);

}  /* obj_get_default */


/********************************************************************
* FUNCTION obj_get_default_case
* 
* Get the default case for the specified OBJ_TYP_CHOICE object
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   pointer to default case object template OBJ_TYP_CASE
*********************************************************************/
obj_template_t *
    obj_get_default_case (obj_template_t *obj)
{
#ifdef DEBUG 
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (obj->objtype != OBJ_TYP_CHOICE) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    if (obj->def.choic->defval) {
        return obj_find_child(obj, 
                              obj_get_mod_name(obj),
                              obj->def.choic->defval);
    }
    return NULL;

}  /* obj_get_default_case */


/********************************************************************
* FUNCTION obj_get_level
* 
* Get the nest level for the specified object
* Top-level is '1'
* Does not count groupings as a level
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   level that this object is located, by checking the parent chain
*********************************************************************/
uint32
    obj_get_level (const obj_template_t *obj)
{
    const obj_template_t  *parent;
    uint32           level;

#ifdef DEBUG 
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    level = 1;
    parent = obj->parent;
    while (parent && !obj_is_root(parent)) {
        level++;
        parent = parent->parent;
    }
    return level;

}  /* obj_get_level */


/********************************************************************
* FUNCTION obj_has_typedefs
* 
* Check if the object has any nested typedefs in it
* This will obly be called if the object is defined in a
* grouping.
*
* INPUTS:
*   obj == object to check
*  
* RETURNS:
*   TRUE if any nested typedefs, FALSE otherwise
*********************************************************************/
boolean
    obj_has_typedefs (const obj_template_t *obj)
{
    const obj_template_t *chobj;
    const grp_template_t *grp;
    const dlq_hdr_t      *typedefQ, *groupingQ, *datadefQ;

#ifdef DEBUG 
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        typedefQ = obj->def.container->typedefQ;
        groupingQ = obj->def.container->groupingQ;
        datadefQ = obj->def.container->datadefQ;
        break;
    case OBJ_TYP_LIST:
        typedefQ = obj->def.list->typedefQ;
        groupingQ = obj->def.list->groupingQ;
        datadefQ = obj->def.list->datadefQ;
        break;
    case OBJ_TYP_RPC:
        typedefQ = &obj->def.rpc->typedefQ;
        groupingQ = &obj->def.rpc->groupingQ;
        datadefQ = &obj->def.rpc->datadefQ;
        break;
    case OBJ_TYP_RPCIO:
        typedefQ = &obj->def.rpcio->typedefQ;
        groupingQ = &obj->def.rpcio->groupingQ;
        datadefQ = &obj->def.rpcio->datadefQ;
        break;
    case OBJ_TYP_NOTIF:
        typedefQ = &obj->def.notif->typedefQ;
        groupingQ = &obj->def.notif->groupingQ;
        datadefQ = &obj->def.notif->datadefQ;
        break;
    default:
        return FALSE;
    }


    if (!dlq_empty(typedefQ)) {
        return TRUE;
    }
        
    for (grp = (const grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (const grp_template_t *)dlq_nextEntry(grp)) {
        if (grp_has_typedefs(grp)) {
            return TRUE;
        }
    }

    for (chobj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         chobj != NULL;
         chobj = (const obj_template_t *)dlq_nextEntry(chobj)) {
        if (obj_has_typedefs(chobj)) {
            return TRUE;
        }
    }

    return FALSE;

}  /* obj_has_typedefs */


/********************************************************************
* FUNCTION obj_get_typdef
* 
* Get the typdef for the leaf or leaf-list
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the typdef or NULL if this object type does not
*    have a typdef
*********************************************************************/
typ_def_t *
    obj_get_typdef (obj_template_t  *obj)
{
    if (obj->objtype == OBJ_TYP_LEAF ||
        obj->objtype == OBJ_TYP_ANYXML) {
        return obj->def.leaf->typdef;
    } else if (obj->objtype == OBJ_TYP_LEAF_LIST) {
        return obj->def.leaflist->typdef;
    } else {
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_typdef */


/********************************************************************
* FUNCTION obj_get_ctypdef
* 
* Get the typdef for the leaf or leaf-list : Const version
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the typdef or NULL if this object type does not
*    have a typdef
*********************************************************************/
const typ_def_t *
    obj_get_ctypdef (const obj_template_t  *obj)
{
    if (obj->objtype == OBJ_TYP_LEAF ||
        obj->objtype == OBJ_TYP_ANYXML) {
        return obj->def.leaf->typdef;
    } else if (obj->objtype == OBJ_TYP_LEAF_LIST) {
        return obj->def.leaflist->typdef;
    } else {
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_ctypdef */


/********************************************************************
* FUNCTION obj_get_basetype
* 
* Get the NCX base type enum for the object type
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    base type enumeration
*********************************************************************/
ncx_btype_t
    obj_get_basetype (const obj_template_t  *obj)
{
    switch (obj->objtype) {
    case OBJ_TYP_LEAF:
        return typ_get_basetype(obj->def.leaf->typdef);
    case OBJ_TYP_LEAF_LIST:
        return typ_get_basetype(obj->def.leaflist->typdef);
    case OBJ_TYP_CONTAINER:
        return NCX_BT_CONTAINER;
    case OBJ_TYP_LIST:
        return NCX_BT_LIST;
    case OBJ_TYP_CHOICE:
        return NCX_BT_CHOICE;
    case OBJ_TYP_CASE:
        return NCX_BT_CASE;
    case OBJ_TYP_RPC:
        return NCX_BT_CONTAINER;
    case OBJ_TYP_RPCIO:
        return NCX_BT_CONTAINER;
    case OBJ_TYP_NOTIF:
        return NCX_BT_CONTAINER;
    case OBJ_TYP_ANYXML:
        return NCX_BT_ANY;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_BT_NONE;
    }
    /*NOTREACHED*/

}  /* obj_get_basetype */


/********************************************************************
* FUNCTION obj_get_mod_prefix
* 
* Get the module prefix for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod prefix
*********************************************************************/
const xmlChar *
    obj_get_mod_prefix (const obj_template_t  *obj)
{

    return ncx_get_mod_prefix(obj->tkerr.mod);

}  /* obj_get_mod_prefix */


/********************************************************************
* FUNCTION obj_get_mod_xmlprefix
* 
* Get the module prefix for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod XML prefix
*********************************************************************/
const xmlChar *
    obj_get_mod_xmlprefix (const obj_template_t  *obj)
{

    return ncx_get_mod_xmlprefix(obj->tkerr.mod);

}  /* obj_get_mod_xmlprefix */


/********************************************************************
* FUNCTION obj_get_mod_name
* 
* Get the module name for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod prefix
*********************************************************************/
const xmlChar *
    obj_get_mod_name (const obj_template_t  *obj)
{
    ncx_module_t  *usemod;

#ifdef DEBUG
    if (obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (obj->mod != NULL) {
        usemod = obj->mod;
    } else if (obj->tkerr.mod != NULL) {
        usemod = obj->tkerr.mod;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (usemod->ismod) {
        return usemod->name;
    } else {
        return usemod->belongs;
    }

}  /* obj_get_mod_name */


/********************************************************************
* FUNCTION obj_get_mod
* 
* Get the module pointer for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to module
*********************************************************************/
ncx_module_t *
    obj_get_mod (obj_template_t  *obj)
{
    ncx_module_t  *usemod;

#ifdef DEBUG
    if (obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (obj->mod != NULL) {
        usemod = obj->mod;
    } else if (obj->tkerr.mod != NULL) {
        usemod = obj->tkerr.mod;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    return usemod;

}  /* obj_get_mod */


/********************************************************************
* FUNCTION obj_get_mod_version
* 
* Get the module version for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to mod version or NULL if none
*********************************************************************/
const xmlChar *
    obj_get_mod_version (const obj_template_t  *obj)
{
#ifdef DEBUG
    if (!obj || !obj->tkerr.mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return obj->tkerr.mod->version;

}  /* obj_get_mod_version */


/********************************************************************
* FUNCTION obj_get_type_name
* 
* Get the typename for an object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    const pointer to type name string
*********************************************************************/
const xmlChar *
    obj_get_type_name (const obj_template_t  *obj)
{
    const typ_def_t *typdef;



#ifdef DEBUG
    if (!obj || !obj->tkerr.mod) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    typdef = obj_get_ctypdef(obj);
    if (typdef) {
        if (typdef->typenamestr) {
            return typdef->typenamestr;
        } else {
            return (const xmlChar *)
                tk_get_btype_sym(obj_get_basetype(obj));
        }
    } else {
        return obj_get_typestr(obj);
    }

}  /* obj_get_type_name */


/********************************************************************
* FUNCTION obj_get_nsid
* 
* Get the namespace ID for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    namespace ID
*********************************************************************/
xmlns_id_t
    obj_get_nsid (const obj_template_t  *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (obj->nsid != 0) {
        return obj->nsid;
    } else if (obj->tkerr.mod) {
        return ncx_get_mod_nsid(obj->tkerr.mod);
    } else {
        return 0;
    }

}  /* obj_get_nsid */


/********************************************************************
* FUNCTION obj_get_iqualval
* 
* Get the instance qualifier for this object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    instance qualifier enumeration
*********************************************************************/
ncx_iqual_t
    obj_get_iqualval (obj_template_t  *obj)
{
    boolean      required;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_IQUAL_NONE;
    }
#endif

    required = obj_is_mandatory(obj);
    return obj_get_iqualval_ex(obj, required);
    
}  /* obj_get_iqualval */


/********************************************************************
* FUNCTION obj_get_iqualval_ex
* 
* Get the instance qualifier for this object
*
* INPUTS:
*    obj  == object to check
*    required == value to use for 'is_mandatory()' logic
*
* RETURNS:
*    instance qualifier enumeration
*********************************************************************/
ncx_iqual_t
    obj_get_iqualval_ex (obj_template_t  *obj,
                         boolean required)
{
    ncx_iqual_t  ret;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_IQUAL_NONE;
    }
#endif

    ret = NCX_IQUAL_NONE;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPCIO:
        ret = (required) ? NCX_IQUAL_ONE : NCX_IQUAL_OPT;
        break;
    case OBJ_TYP_LEAF_LIST:
        if (obj->def.leaflist->minset) {
            if (obj->def.leaflist->maxset && 
                obj->def.leaflist->maxelems==1) {
                ret = NCX_IQUAL_ONE;
            } else {
                ret = NCX_IQUAL_1MORE;
            }
        } else {
            if (obj->def.leaflist->maxset && 
                obj->def.leaflist->maxelems==1) {
                ret = NCX_IQUAL_OPT;
            } else {
                ret = NCX_IQUAL_ZMORE;
            }
        }
        break;
    case OBJ_TYP_LIST:
        if (obj->def.list->minset) {
            if (obj->def.list->maxset && obj->def.list->maxelems==1) {
                ret = NCX_IQUAL_ONE;
            } else {
                ret = NCX_IQUAL_1MORE;
            }
        } else {
            if (obj->def.list->maxset && obj->def.list->maxelems==1) {
                ret = NCX_IQUAL_OPT;
            } else {
                ret = NCX_IQUAL_ZMORE;
            }
        }
        break;
    case OBJ_TYP_REFINE:
        ret = NCX_IQUAL_ZMORE;
        break;
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        ret = NCX_IQUAL_ONE;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return ret;

}  /* obj_get_iqualval_ex */


/********************************************************************
* FUNCTION obj_get_min_elements
* 
* Get the min-elements clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*    minelems == address of return min-elements value
*
* OUTPUTS:
*   *minelems == min-elements value if it is set for this object
*   
* RETURNS:
*    TRUE if min-elements is set, FALSE if not or N/A
*********************************************************************/
boolean
    obj_get_min_elements (obj_template_t  *obj,
                          uint32 *minelems)
{

#ifdef DEBUG
    if (!obj || !minelems) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_LEAF_LIST:
        *minelems = obj->def.leaflist->minelems;
        return obj->def.leaflist->minset;
    case OBJ_TYP_LIST:
        *minelems = obj->def.list->minelems;
        return obj->def.list->minset;
    case OBJ_TYP_REFINE:
        *minelems = obj->def.refine->minelems;
        return (obj->def.refine->minelems_tkerr.mod) ? TRUE : FALSE;
    default:
        return FALSE;
    }
    /*NOTREACHED*/

}  /* obj_get_min_elements */


/********************************************************************
* FUNCTION obj_get_max_elements
* 
* Get the max-elements clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*    maxelems == address of return max-elements value
*
* OUTPUTS:
*   *maxelems == max-elements value if it is set for this object
*
* RETURNS:
*    TRUE if max-elements is set, FALSE if not or N/A
*********************************************************************/
boolean
    obj_get_max_elements (obj_template_t  *obj,
                          uint32 *maxelems)
{

#ifdef DEBUG
    if (!obj || !maxelems) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_LEAF_LIST:
        *maxelems = obj->def.leaflist->maxelems;
        return obj->def.leaflist->maxset;
    case OBJ_TYP_LIST:
        *maxelems = obj->def.list->maxelems;
        return obj->def.list->maxset;
    case OBJ_TYP_REFINE:
        *maxelems = obj->def.refine->maxelems;
        return (obj->def.refine->maxelems_tkerr.mod) ? TRUE : FALSE;
    default:
        return FALSE;
    }
    /*NOTREACHED*/

}  /* obj_get_max_elements */


/********************************************************************
* FUNCTION obj_get_units
* 
* Get the units clause for this object, if any
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to units clause, or NULL if none
*********************************************************************/
const xmlChar *
    obj_get_units (obj_template_t  *obj)
{
    const xmlChar    *units;
    const typ_def_t  *typdef;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    units = NULL;

    switch (obj->objtype) {
    case OBJ_TYP_LEAF:
        units = obj->def.leaf->units;
        break;
    case OBJ_TYP_LEAF_LIST:
        units = obj->def.leaflist->units;
        break;
    default:
        return NULL;
    }

    if (!units) {
        typdef = obj_get_ctypdef(obj);
        if (typdef) {
            units = typ_get_units_from_typdef(typdef);
        }
    }
    return units;

}  /* obj_get_units */


/********************************************************************
* FUNCTION obj_get_parent
* 
* Get the parent of the current object
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
obj_template_t *
    obj_get_parent (obj_template_t  *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return obj->parent;

}  /* obj_get_parent */


/********************************************************************
* FUNCTION obj_get_cparent
* 
* Get the parent of the current object
* CONST POINTER VERSION
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
const obj_template_t *
    obj_get_cparent (const obj_template_t  *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return obj->parent;

}  /* obj_get_cparent */


/********************************************************************
* FUNCTION obj_get_real_parent
* 
* Get the parent of the current object;
* skip OBJ_TYP_CHOICE and OBJ_TYP_CASE
*
* INPUTS:
*    obj  == object to check
*
* RETURNS:
*    pointer to the parent of this object or NULL if none
*********************************************************************/
obj_template_t *
    obj_get_real_parent (obj_template_t  *obj)
{
#ifdef DEBUG
    if (obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    obj = obj->parent;
    if (obj != NULL) {
        switch (obj->objtype) {
        case OBJ_TYP_CHOICE:
        case OBJ_TYP_CASE:
            return obj_get_real_parent(obj);
        default:
            return obj;
        }
    }
    return NULL;

}  /* obj_get_real_parent */


/********************************************************************
* FUNCTION obj_get_presence_string
*
* Get the presence-stmt value, if any
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to string
*   NULL if none
*********************************************************************/
const xmlChar *
    obj_get_presence_string (const obj_template_t *obj)
{

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (obj->objtype != OBJ_TYP_CONTAINER) {
        return NULL;
    }

    return obj->def.container->presence;

}  /* obj_get_presence_string */


/********************************************************************
* FUNCTION obj_get_presence_string_field
*
* Get the address ot the presence-stmt value, if any
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to address of presence string
*   NULL if none
*********************************************************************/
void* obj_get_presence_string_field (const obj_template_t *obj)
{
    assert( obj && "obj is NULL" );

    if (obj->objtype != OBJ_TYP_CONTAINER) {
        return NULL;
    }

    return &obj->def.container->presence;
}  /* obj_get_presence_string_field */


/********************************************************************
 * Get the correct child node for the specified parent and
 * current XML node. This function finds the right module namespace
 * and child node, given the current context
 * !! Will ignore choice and case nodes !!
 * !! This function called by agt_val_parse and mgr_val_parse
 * !! Only YANG data nodes are expected 
 *
 * \param obj the parent object template
 * \param chobj the current child node 
 *              (may be NULL if the xmlorder param is FALSE).
 * \param xmlorder TRUE if should follow strict XML element order,
 *                 FALSE if sibling node order errors should be 
 *                 ignored; find child nodes out of order
 *                 and check too-many-instances later
 * \param curnode the current XML start or empty node to check
 * \param force_modQ the Q of ncx_module_t to check, if set to NULL and the 
 *                   xmlns registry of module pointers will be used instead 
 *                   (except netconf.yang)
 * \param rettop the address of return topchild object
 * \param retobj the address of return object to use
 * \return *   status
 *********************************************************************/
status_t obj_get_child_node ( obj_template_t *obj,
                              obj_template_t *chobj,
                              const xml_node_t *curnode,
                              boolean xmlorder,
                              dlq_hdr_t *force_modQ,
                              obj_template_t **rettop,
                              obj_template_t **retobj )
{
    assert ( obj && "obj is NULL" );
    assert ( curnode && "curnode is NULL" );
    assert ( rettop && "rettop is NULL" );
    assert ( retobj && "retobj is NULL" );

    boolean topdone = FALSE;
    obj_template_t *foundobj = search_for_child_node( obj, chobj, curnode,
            xmlorder, force_modQ, rettop, &topdone );

    if (foundobj) {
        if (foundobj->objtype == OBJ_TYP_CHOICE) {
            log_debug("\n***CHOICE %s \n", obj_get_name(foundobj));
        }
        if (foundobj->objtype == OBJ_TYP_CASE) {
            log_debug("\n***CASE %s \n", obj_get_name(foundobj));
        }

        *retobj = foundobj;
        if (!topdone) {
            *rettop = foundobj;
        }
        return NO_ERR;
    }

    return ERR_NCX_DEF_NOT_FOUND;
}  /* obj_get_child_node */

/********************************************************************
* FUNCTION obj_get_child_count
*
* Get the number of child nodes the object has
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   number of child nodes
*********************************************************************/
uint32 obj_get_child_count (const obj_template_t *obj)
{
    assert( obj && "obj is NULL" );

    const dlq_hdr_t   *datadefQ = obj_get_cdatadefQ(obj);
    if (datadefQ) {
        return dlq_count(datadefQ);
    } else {
        return 0;
    }

}   /* obj_get_child_count */


/********************************************************************
* FUNCTION obj_get_default_parm
* 
* Get the ncx:default-parm object for this object
* Only supported for OBJ_TYP_CONTAINER and OBJ_TYP_RPCIO (input)
*
* INPUTS:
*   obj == the specific object to check
*
* RETURNS:
*   pointer to the name field, NULL if some error or unnamed
*********************************************************************/
obj_template_t * 
    obj_get_default_parm (obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        return obj->def.container->defaultparm;
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
    case OBJ_TYP_CASE:
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_RPC:
    case OBJ_TYP_ANYXML:
        return NULL;
    case OBJ_TYP_RPCIO:
        if (obj->def.rpcio->defaultparm != NULL) {
            return obj->def.rpcio->defaultparm;
        }
        if (!xml_strcmp(obj_get_name(obj), YANG_K_INPUT)) {
            if (obj_get_child_count(obj) == 1) {
                obj_template_t  *childobj = obj_first_child(obj);
                if (childobj != NULL && obj_is_leafy(childobj)) {
                    return childobj;
                }
            }
        }
        return NULL;
    case OBJ_TYP_NOTIF:
        return NULL;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* obj_get_default_parm */


/********************************************************************
* FUNCTION obj_get_config_flag_deep
*
* get config flag during augment expand
* Get the config flag for an obj_template_t 
* Go all the way up the tree until an explicit
* set node or the root is found
*
* Used by get_list_key because the config flag
* of the parent is not set yet when a key leaf is expanded
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*********************************************************************/
boolean
    obj_get_config_flag_deep (const obj_template_t *obj)
{
    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
        if (obj_is_root(obj)) {
            return TRUE;   
        }
        /* check if this normal object has a config-stmt */
        if (obj->flags & OBJ_FL_CONFSET) {
            return (obj->flags & OBJ_FL_CONFIG) ? TRUE : FALSE;
        }

        if (obj->parent) {
            return obj_get_config_flag_deep(obj->parent);
        }

        /* this should be an object in a grouping */
        if (obj->grp) {
            return TRUE;  // !!! this is the old default !!!
            // return FALSE;
        } else {
            return TRUE;
        }
    case OBJ_TYP_CASE:
        if (obj->parent) {
            return obj_get_config_flag_deep(obj->parent);
        } else {
            /* should not happen */
            return FALSE;
        }
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        /* no real setting -- not applicable */
        return FALSE;
    case OBJ_TYP_RPC:
        /* no real setting for this, but has to be true
         * to allow rpc/input to be true
         */
        return TRUE;
    case OBJ_TYP_RPCIO:
        if (!xml_strcmp(obj->def.rpcio->name, YANG_K_INPUT)) {
            return TRUE;
        } else {
            return FALSE;
        }
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}   /* obj_get_config_flag_deep */


/********************************************************************
* FUNCTION obj_get_config_flag_check
*
* get config flag during YANG module checking
* Used by yang_obj.c to make sure ncx:root objects
* are not treated as 'config', like obj_get_config_deep
*
* INPUTS:
*   obj == obj_template to check
*   ingrp == address if in grouping flag
*
* OUTPUTS:
*   *ingrp == TRUE if hit grouping top without finding
*             a definitive answer
* RETURNS:
*   TRUE if config set to TRUE
*   FALSE if config set to FALSE
*   !!! ignore if *ingrp == TRUE
*********************************************************************/
boolean
    obj_get_config_flag_check (const obj_template_t *obj,
                               boolean *ingrp)
{
#ifdef DEBUG
    if (obj == NULL || ingrp == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    *ingrp = FALSE;

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_LEAF:
    case OBJ_TYP_LEAF_LIST:
    case OBJ_TYP_LIST:
    case OBJ_TYP_CHOICE:
        /* check if this normal object has a config-stmt */
        if (obj->flags & OBJ_FL_CONFSET) {
            return (obj->flags & OBJ_FL_CONFIG) ? TRUE : FALSE;
        }

        if (obj->parent) {
            return obj_get_config_flag_check(obj->parent, ingrp);
        }

        /* this should be an object in a grouping */
        if (obj->grp) {
            *ingrp = TRUE;
            return FALSE;
        } else {
            return TRUE;
        }
    case OBJ_TYP_CASE:
        if (obj->parent) {
            return obj_get_config_flag_check(obj->parent, ingrp);
        } else {
            /* should not happen */
            return FALSE;
        }
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
        /* no real setting -- not applicable */
        return FALSE;
    case OBJ_TYP_RPC:
        /* no real setting for this, but has to be true
         * to allow rpc/input to be true
         */
        return TRUE;
    case OBJ_TYP_RPCIO:
        if (!xml_strcmp(obj->def.rpcio->name, YANG_K_INPUT)) {
            return TRUE;
        } else {
            return FALSE;
        }
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}   /* obj_get_config_flag_check */


/********************************************************************
* FUNCTION obj_get_fraction_digits
* 
* Get the fraction-digits field from the object typdef
*
* INPUTS:
*     obj == object template to  check
*
* RETURNS:
*     number of fixed decimal digits expected (1..18)
*     0 if some error
*********************************************************************/
uint8
    obj_get_fraction_digits (const obj_template_t  *obj)
{
    const typ_def_t  *typdef;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    typdef = obj_get_ctypdef(obj);
    if (typdef) {
        return typ_get_fraction_digits(typdef);
    } else {
        return 0;
    }

}  /* obj_get_fraction_digits */


/********************************************************************
* FUNCTION obj_get_first_iffeature
* 
* Get the first if-feature clause (if any) for the specified object
*
* INPUTS:
*     obj == object template to  check
*
* RETURNS:
*     pointer to first if-feature struct
*     NULL if none available
*********************************************************************/
const ncx_iffeature_t *
    obj_get_first_iffeature (const obj_template_t  *obj)
{

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (const ncx_iffeature_t *)
        dlq_firstEntry(&obj->iffeatureQ);

}  /* obj_get_first_iffeature */


/********************************************************************
* FUNCTION obj_get_next_iffeature
* 
* Get the next if-feature clause (if any)
*
* INPUTS:
*     iffeature == current iffeature struct
*
* RETURNS:
*     pointer to next if-feature struct
*     NULL if none available
*********************************************************************/
const ncx_iffeature_t *
    obj_get_next_iffeature (const ncx_iffeature_t  *iffeature)
{

#ifdef DEBUG
    if (!iffeature) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (const ncx_iffeature_t *)dlq_nextEntry(iffeature);

}  /* obj_get_next_iffeature */


/********************************************************************
 * Check if object is a proper leaf
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a leaf
 *********************************************************************/
boolean obj_is_leaf (const obj_template_t  *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->objtype == OBJ_TYP_LEAF); 
}  /* obj_is_leaf */


/********************************************************************
 * Check if object is a proper leaf or leaflist 
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a leaf or leaflist
 *********************************************************************/
boolean obj_is_leafy (const obj_template_t  *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->objtype == OBJ_TYP_LEAF || obj->objtype == OBJ_TYP_LEAF_LIST); 
}  /* obj_is_leafy */


/********************************************************************
 * Figure out if the obj is YANG mandatory or not
 *
 * \param obj the obj_template to check
 * \return TRUE if object is not mandatory
 *********************************************************************/
boolean obj_is_mandatory (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        if (obj->def.container->presence) {
            return FALSE;
        }
        /* else drop through and check children */
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPCIO:
        {
            obj_template_t *chobj = obj_first_child(obj);
            for ( ; chobj; chobj = obj_next_child(chobj)) {
                if (obj_is_mandatory(chobj)) {
                    return TRUE;
                }
            }
        }
        return FALSE;

    case OBJ_TYP_LEAF:
        if (obj_is_key(obj)) {
            return TRUE;
        } 
        /* else fall through */
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_CHOICE:
        return (obj->flags & OBJ_FL_MANDATORY) ? TRUE : FALSE;
    case OBJ_TYP_LEAF_LIST:
        return (obj->def.leaflist->minelems) ? TRUE : FALSE;
    case OBJ_TYP_LIST:
        return (obj->def.list->minelems) ? TRUE : FALSE;
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
}   /* obj_is_mandatory */


/********************************************************************
 * Figure out if the obj is YANG mandatory or not
 * Check the when-stmts, not just mandatory-stmt
 *
 * \param obj the obj_template to check
 * \param config_only  flag indicating weather to only check the config.
 * \return TRUE if object is mandatory
*********************************************************************/
boolean obj_is_mandatory_when_ex (obj_template_t *obj, boolean config_only)
{
    assert(obj && "obj is NULL" );

    if (config_only && !obj_is_config(obj)) {
        return FALSE;
    }

    switch (obj->objtype) {
    case OBJ_TYP_CONTAINER:
        if (obj->def.container->presence) {
            return FALSE;
        }
        /* else drop through and check children */
    case OBJ_TYP_CASE:
    case OBJ_TYP_RPCIO:
        {
            obj_template_t *chobj = obj_first_child(obj);
            for ( ; chobj; chobj = obj_next_child(chobj)) {
                if (obj_is_mandatory_when_ex(chobj, config_only)) {
                    return TRUE;
                }
            }
        }
        return FALSE;

    case OBJ_TYP_LEAF:
        if (obj_is_key(obj)) {
            return TRUE;
        } 
        /* else fall through */
    case OBJ_TYP_ANYXML:
    case OBJ_TYP_CHOICE:
        if (obj_has_when_stmts(obj)) {
            return FALSE;
        }
        return (obj->flags & OBJ_FL_MANDATORY) ? TRUE : FALSE;
    case OBJ_TYP_LEAF_LIST:
        if (obj_has_when_stmts(obj)) {
            return FALSE;
        }
        return (obj->def.leaflist->minelems) ? TRUE : FALSE;
    case OBJ_TYP_LIST:
        if (obj_has_when_stmts(obj)) {
            return FALSE;
        }
        return (obj->def.list->minelems) ? TRUE : FALSE;
    case OBJ_TYP_USES:
    case OBJ_TYP_AUGMENT:
    case OBJ_TYP_REFINE:
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

}   /* obj_is_mandatory_when_ex */


/********************************************************************
 * Figure out if the obj is YANG mandatory or not
 * Check the when-stmts, not just mandatory-stmt
 *
 * \param obj the obj_template to check
 * \return TRUE if object is mandatory
 *********************************************************************/
boolean obj_is_mandatory_when (obj_template_t *obj)
{
    return obj_is_mandatory_when_ex(obj, FALSE);
}   /* obj_is_mandatory_when */


/********************************************************************
 * Figure out if the obj is a cloned object, inserted via uses
 * or augment statements
 *
 * \param obj the obj_template to check
 * \return TRUE if object is cloned
 *********************************************************************/
boolean obj_is_cloned (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_CLONE) ? TRUE : FALSE;
}   /* obj_is_cloned */


/********************************************************************
 * Figure out if the obj is a cloned object, inserted via an
 * augment statement
 *
 * \param obj the obj_template to check
 * \return TRUE if object is sourced from an augment
*********************************************************************/
boolean obj_is_augclone (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_AUGCLONE) ? TRUE : FALSE;
}   /* obj_is_augclone */


/********************************************************************
 * Figure out if the obj is a refinement object, within a uses-stmt
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a refinement
 *********************************************************************/
boolean obj_is_refine (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->objtype == OBJ_TYP_REFINE) ? TRUE : FALSE;
}   /* obj_is_refine */

/********************************************************************
 * Check if the object is defined within data or within a
 * notification or RPC instead
 * 
 * \param obj the obj_template to check
 * \return TRUE if data object (could be in a grouping or real data)
 *********************************************************************/
boolean obj_is_data (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    switch (obj->objtype) {
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_RPCIO:
        return TRUE;  /* hack for yangdump HTML output */
    case OBJ_TYP_REFINE:
        return FALSE;
    default:
        if (obj->parent && !obj_is_root(obj->parent)) {
            return obj_is_data(obj->parent);
        } else {
            return TRUE;
        }
    }
}  /* obj_is_data */


/********************************************************************
 * Check if the object is some sort of data Constrained to only check 
 * the config DB objects, not any notification or RPC objects
 *
 * \param obj the obj_template to check
 * \return TRUE if data object (could be in a grouping or real data)
 *         FALSE if defined within notification or RPC (or some error)
 *********************************************************************/
boolean obj_is_data_db (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj_is_abstract(obj) || obj_is_cli(obj)) {
        return FALSE;
    }

    switch (obj->objtype) {
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_RPCIO:
        return FALSE;
    case OBJ_TYP_REFINE:
        return FALSE;
    default:
        if (obj_is_root(obj)) {
            return TRUE;
        } else if (obj->parent && !obj_is_root(obj->parent)) {
            return obj_is_data_db(obj->parent);
        } else {
            return TRUE;
        }
    }
    /*NOTREACHED*/

}  /* obj_is_data_db */


/********************************************************************
* Check if the object is in an rpc/input section
*
 * \param obj the obj_template to check
 * \return TRUE if /rpc/input object
*********************************************************************/
boolean obj_in_rpc (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    switch (obj->objtype) {
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_RPCIO:
        return (!xml_strcmp(obj_get_name(obj), YANG_K_INPUT)) ?
            TRUE : FALSE;
    case OBJ_TYP_REFINE:
        return FALSE;
    default:
        if (obj->parent && !obj_is_root(obj->parent)) {
            return obj_in_rpc(obj->parent);
        } else {
            return FALSE;
        }
    }
    /*NOTREACHED*/

}  /* obj_in_rpc */


/********************************************************************
 * Check if the object is in an rpc-reply/output section
 *
 * \param obj the obj_template to check
 * \return TRUE if /rpc-reply/output object
 *********************************************************************/
boolean obj_in_rpc_reply (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    switch (obj->objtype) {
    case OBJ_TYP_RPC:
    case OBJ_TYP_NOTIF:
        return FALSE;
    case OBJ_TYP_RPCIO:
        return (!xml_strcmp(obj_get_name(obj), YANG_K_OUTPUT)) ?
            TRUE : FALSE;
    case OBJ_TYP_REFINE:
        return FALSE;
    default:
        if (obj->parent && !obj_is_root(obj->parent)) {
            return obj_in_rpc_reply(obj->parent);
        } else {
            return FALSE;
        }
    }
    /*NOTREACHED*/

}  /* obj_in_rpc_reply */


/********************************************************************
 * FUNCTION obj_in_notif
 * 
 * Check if the object is in a notification
 *
 * \param obj the obj_template to check
 * \return TRUE if /notification object
 *********************************************************************/
boolean obj_in_notif (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    switch (obj->objtype) {
    case OBJ_TYP_RPC:
        return FALSE;
    case OBJ_TYP_NOTIF:
        return TRUE;
    case OBJ_TYP_RPCIO:
        return FALSE;
    case OBJ_TYP_REFINE:
        return FALSE;
    default:
        if (obj->parent && !obj_is_root(obj->parent)) {
            return obj_in_notif(obj->parent);
        } else {
            return FALSE;
        }
    }
    /*NOTREACHED*/
}  /* obj_in_notif */


/********************************************************************
 * Check if the object is an RPC method
 * 
 * \param obj the obj_template to check
 * \return TRUE if RPC method
 *********************************************************************/
boolean obj_is_rpc (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->objtype == OBJ_TYP_RPC) ? TRUE : FALSE;
}  /* obj_is_rpc */


/********************************************************************
 * Check if the object is a notification
 * 
 * \param obj the obj_template to check
 * \return TRUE if notification
 *********************************************************************/
boolean obj_is_notif (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->objtype == OBJ_TYP_NOTIF);
}  /* obj_is_notif */


/********************************************************************
 * Check if object was entered in empty fashion:
 *   list foo;
 *   uses grpx;
 *
 * \param obj the obj_template to check
 * \return TRUE if object is empty of subclauses
 *   FALSE if object is not empty of subclauses
 *********************************************************************/
boolean obj_is_empty (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_EMPTY) ? TRUE : FALSE;
}   /* obj_is_empty */


/********************************************************************
 * Check if one object is a match in identity with another one
 * Only used by yangdiff to compare objects.
 *
 * \param obj the first obj_template in the match
 * \param obj the second obj_template in the match
 * \return TRUE is a match, FALSE otherwise
 *********************************************************************/
boolean obj_is_match ( const obj_template_t  *obj1,
                       const obj_template_t *obj2 )
{

    if (!xmlns_ids_equal(obj_get_nsid(obj1),
                         obj_get_nsid(obj2))) {
        return FALSE;
    }

    if (obj_has_name(obj1) && obj_has_name(obj2)) {
        return xml_strcmp(obj_get_name(obj1), 
                          obj_get_name(obj2)) ? FALSE : TRUE;
    } else {
        return FALSE;
    }

}  /* obj_is_match */


/********************************************************************
 * Check if object is marked as a hidden object
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as OBJ_FL_HIDDEN 
 *********************************************************************/
boolean obj_is_hidden (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_HIDDEN) ? TRUE : FALSE;
}   /* obj_is_hidden */


/********************************************************************
 * Check if object is marked as a root object
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as OBJ_FL_ROOT
 *********************************************************************/
boolean obj_is_root (const obj_template_t *obj)
{
    //assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_ROOT) ? TRUE : FALSE;
}   /* obj_is_root */


/********************************************************************
 * Check if object is marked as a password object
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as OBJ_FL_PASSWD
 *********************************************************************/
boolean obj_is_password (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_PASSWD) ? TRUE : FALSE;
}   /* obj_is_password */

/********************************************************************
 * Check if object is marked as an XSD list
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as OBJ_FL_XSDLIST
 *********************************************************************/
boolean obj_is_xsdlist (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_XSDLIST) ? TRUE : FALSE;
}   /* obj_is_xsdlist */

/********************************************************************
 * Check if object is marked as a CLI object
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:cli
 *********************************************************************/
boolean obj_is_cli (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj->flags & OBJ_FL_CLI) {
        return TRUE;
    } else if (obj->parent) {
        return obj_is_cli(obj->parent);
    } else {
        return FALSE;
    }
}   /* obj_is_cli */


/********************************************************************
 * Check if object is being used as a key leaf within a list
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a key leaf
 *********************************************************************/
boolean obj_is_key (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_KEY) ? TRUE : FALSE;
}   /* obj_is_key */


/********************************************************************
 * Check if object is being used as an object identifier or error-info
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:abstract
 *********************************************************************/
boolean obj_is_abstract (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_ABSTRACT) ? TRUE : FALSE;
}   /* obj_is_abstract */


/********************************************************************
 * Check if object is an XPath string
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:xpath
 *********************************************************************/
boolean obj_is_xpath_string (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    boolean retval = ( (obj->flags & (OBJ_FL_XPATH | OBJ_FL_SCHEMAINST)) ||
                        obj_get_basetype(obj)==NCX_BT_INSTANCE_ID) ? TRUE 
                                                                   : FALSE;

    if ( !retval ) {
        const typ_def_t *typdef = obj_get_ctypdef(obj);
        if (typdef) {
            return typ_is_xpath_string(typdef);
        }
    }

    return retval;

}   /* obj_is_xpath_string */


/********************************************************************
 * Check if object is a schema-instance string
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:schema-instance
 *********************************************************************/
boolean obj_is_schema_instance_string (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj_get_basetype(obj) != NCX_BT_STRING) {
        return FALSE;
    }

    return (obj->flags & OBJ_FL_SCHEMAINST) ? TRUE : FALSE;
}   /* obj_is_schema_instance_string */


/********************************************************************
 * Check if object is tagged ncx:secure
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:secure
 *********************************************************************/
boolean obj_is_secure (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_SECURE) ? TRUE : FALSE;
}   /* obj_is_secure */


/********************************************************************
 * Check if object is tagged ncx:very-secure
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:very-secure
 *********************************************************************/
boolean obj_is_very_secure (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_VERY_SECURE) ? TRUE : FALSE;
}   /* obj_is_very_secure */


/********************************************************************
 * Check if the object is system or user-ordered
 *
 * \param obj the obj_template to check
 * \return TRUE if object is system ordered
 *********************************************************************/
boolean obj_is_system_ordered (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    switch (obj->objtype) {
    case OBJ_TYP_LEAF_LIST:
        return obj->def.leaflist->ordersys;
    case OBJ_TYP_LIST:
        return obj->def.list->ordersys;
    default:
        return TRUE;
    }
    /*NOTREACHED*/
}  /* obj_is_system_ordered */


/********************************************************************
 * Check if the object is an NP-container
 *
 * \param obj the obj_template to check
 * \return TRUE if object is an NP-container
 *********************************************************************/
boolean
    obj_is_np_container (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj->objtype != OBJ_TYP_CONTAINER) {
        return FALSE;
    }

    return (obj->def.container->presence) ? FALSE : TRUE;

}  /* obj_is_np_container */


/********************************************************************
 * Check any if-feature statement that may
 * cause the specified object to be invisible
 *
 * \param obj the obj_template to check
 * \return TRUE if object is enabled
 *********************************************************************/
boolean obj_is_enabled (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    const ncx_iffeature_t *iffeature = obj_get_first_iffeature(obj);
    for ( ; iffeature ; iffeature = obj_get_next_iffeature(iffeature)) {
        if (!iffeature->feature || !ncx_feature_enabled(iffeature->feature)) {
            return FALSE;
        }
    }

    const obj_iffeature_ptr_t *iffptr = (obj_iffeature_ptr_t *)
        dlq_firstEntry(&obj->inherited_iffeatureQ);
    for ( ; iffptr ; iffptr = (obj_iffeature_ptr_t *)dlq_nextEntry(iffptr) ) {
        if (!iffptr->iffeature->feature || 
            !ncx_feature_enabled(iffptr->iffeature->feature)) {
            return FALSE;
        }
    }

    obj_template_t *testobj = obj->parent;
    boolean done = FALSE;
    while (!done) {
        if (testobj &&
            (testobj->objtype == OBJ_TYP_CHOICE ||
             testobj->objtype == OBJ_TYP_CASE)) {

            iffeature = obj_get_first_iffeature(testobj);
            for ( ; iffeature ; 
                  iffeature = obj_get_next_iffeature(iffeature)) {
                if (!iffeature->feature || 
                    !ncx_feature_enabled(iffeature->feature)) {
                    return FALSE;
                }
            }

            iffptr = (obj_iffeature_ptr_t *)
                dlq_firstEntry(&testobj->inherited_iffeatureQ);
            for ( ; iffptr ; iffptr = (obj_iffeature_ptr_t *)
                      dlq_nextEntry(iffptr) ) {
                if (!iffptr->iffeature->feature || 
                    !ncx_feature_enabled(iffptr->iffeature->feature)) {
                    return FALSE;
                }
            }

            testobj = testobj->parent;
        } else {
            done = TRUE;
        }
    }

    return TRUE;

}  /* obj_is_enabled */


/********************************************************************
 * Check if the object is a single instance of if it
 * allows multiple instances; check all of the
 * ancestors if needed
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a single instance object
 *********************************************************************/
boolean obj_is_single_instance (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    while (obj != NULL) {
        ncx_iqual_t iqual = obj_get_iqualval(obj);
        switch (iqual) {
        case NCX_IQUAL_ZMORE:
        case NCX_IQUAL_1MORE:
            return FALSE;
        default:
            /* don't bother checking the root and don't go past the root into
             * the RPC parameters */
            obj = obj->parent;
            if ( obj && obj_is_root(obj)) {
                 obj = NULL;
            }
        }
    }
    return TRUE;
}  /* obj_is_single_instance */


/********************************************************************
 * Check if the object is a short case statement
 *
 * \param obj the obj_template to check
 * \return TRUE if object is a 1 object case statement
 *********************************************************************/
boolean obj_is_short_case (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    const obj_case_t   *cas;
    if (obj->objtype != OBJ_TYP_CASE) {
        return FALSE;
    }

    cas = obj->def.cas;

    if (dlq_count(cas->datadefQ) != 1) {
        return FALSE;
    }

    if (obj->when && obj->when->exprstr) {
        return FALSE;
    }

    if (obj_get_first_iffeature(obj) != NULL) {
        return FALSE;
    }

    if (obj_get_status(obj) != NCX_STATUS_CURRENT) {
        return FALSE;
    }

    if (obj_get_description(obj) != NULL) {
        return FALSE;
    }

    if (obj_get_reference(obj) != NULL) {
        return FALSE;
    }

    if (dlq_count(obj_get_appinfoQ(obj)) > 0) {
        return FALSE;
    }

    return TRUE;
}  /* obj_is_short_case */


/********************************************************************
 * Check if the object is top-level object within
 * the YANG module that defines it
 *
 * \param obj the obj_template to check
 * \return TRUE if obj is a top-level object
 *********************************************************************/
boolean obj_is_top (const obj_template_t *obj)
{
    assert(obj && "obj is NULL" );
    return (obj->flags & OBJ_FL_TOP) ? TRUE : FALSE;
}  /* obj_is_top */


/********************************************************************
 * Figure out if the obj is OK for current CLI implementation
 * Top object must be a container. Child objects must be only choices of leafs,
 * plain leafs, or leaf lists are allowed
 *
 * \param obj the obj_template to check
 * \return TRUE if object is OK for CLI
 *********************************************************************/
boolean obj_ok_for_cli (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj->objtype != OBJ_TYP_CONTAINER) {
        return FALSE;
    }

    obj_template_t *chobj = obj_first_child(obj);
    for (; chobj; chobj = obj_next_child(chobj)) {

        switch (chobj->objtype) {
        case OBJ_TYP_ANYXML:
            return TRUE;   /**** was FALSE ****/
        case OBJ_TYP_LEAF:
        case OBJ_TYP_LEAF_LIST:
            break;
        case OBJ_TYP_CHOICE:
            {
                obj_template_t* casobj = obj_first_child(chobj);
                for ( ; casobj; casobj = obj_next_child(casobj)) {
                    obj_template_t* caschild = obj_first_child(casobj);
                    for ( ; caschild ; caschild = obj_next_child(caschild)) {
                        switch (caschild->objtype) {
                        case OBJ_TYP_ANYXML:
                            return FALSE;
                        case OBJ_TYP_LEAF:
                        case OBJ_TYP_LEAF_LIST:
                            break;
                        default:
                            return FALSE;
                        }
                    }
                }
            }
            break;
        default:
            return FALSE;
        }
    }

    return TRUE;

}   /* obj_ok_for_cli */


/********************************************************************
 * Check if there are any accessible nodes within the object
 *
 * \param obj the obj_template to check
 * \return TRUE if there are any accessible children
 *********************************************************************/
boolean obj_has_children (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if ( obj_first_child_deep(obj) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}   /* obj_has_children */


/********************************************************************
 * Check if there are any accessible read-only nodes within the object
 *
 * \param obj the obj_template to check
 * \return TRUE if there are any accessible read-only children
 *********************************************************************/
boolean obj_has_ro_children (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    obj_template_t *childobj = obj_first_child(obj);
    for ( ; childobj ; childobj = obj_next_child(childobj)) {

        if ( obj_has_name(childobj) && obj_is_enabled(childobj) && 
            !obj_is_abstract(childobj)) {

            if (!obj_get_config_flag(childobj)) {
                return TRUE;
            }
        }
    }

    return FALSE;

}   /* obj_has_ro_children */


/********************************************************************
 * Check if the RPC object has any real input children
 *
 * \param obj the obj_template to check
 * \return TRUE if there are any input children
 *********************************************************************/
boolean obj_rpc_has_input (obj_template_t *obj)
{
    return obj_rpc_has_input_or_output( obj, YANG_K_INPUT );
}   /* obj_rpc_has_input */

/********************************************************************
 * Check if the RPC object has any real output children
 *
 * \param obj the obj_template to check
 * \return TRUE if there are any output children
 *********************************************************************/
boolean obj_rpc_has_output (obj_template_t *obj)
{
    return obj_rpc_has_input_or_output( obj, YANG_K_OUTPUT );
}   /* obj_rpc_has_output */


/********************************************************************
 * Check if any when-stmts apply to this object
 * Does not check if they are true, just any when-stmts present
 *
 * \param obj the obj_template to check
 * \return TRUE if object has any when-stmts associated with it
 *********************************************************************/
boolean obj_has_when_stmts (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    if (obj->when || !dlq_empty(&obj->inherited_whenQ)) {
        return TRUE;
    }

    obj_template_t *testobj = obj->parent;
    boolean done = FALSE;
    while (!done) {
        if (testobj &&
            (testobj->objtype == OBJ_TYP_CHOICE ||
             testobj->objtype == OBJ_TYP_CASE)) {

            if (testobj->when || !dlq_empty(&testobj->inherited_whenQ)) {
                return TRUE;
            }

            testobj = testobj->parent;
        } else {
            done = TRUE;
        }
    }

    return FALSE;

}  /* obj_has_when_stmts */


/********************************************************************
* FUNCTION obj_new_metadata
* 
* Malloc and initialize the fields in a an obj_metadata_t
*
* INPUTS:
*  isreal == TRUE if this is for a real object
*          == FALSE if this is a cloned object
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
obj_metadata_t * obj_new_metadata (void)
{
    obj_metadata_t  *meta;

    meta = m__getObj(obj_metadata_t);
    if (!meta) {
        return NULL;
    }

    (void)memset(meta, 0x0, sizeof(obj_metadata_t));

    meta->typdef = typ_new_typdef();
    if (!meta->typdef) {
        m__free(meta);
        return NULL;
    }

    return meta;

}  /* obj_new_metadata */


/********************************************************************
* FUNCTION obj_free_metadata
* 
* Scrub the memory in a obj_metadata_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    meta == obj_metadata_t data structure to free
*********************************************************************/
void obj_free_metadata (obj_metadata_t *meta)
{
    if (!meta) {
        return;
    }

    if (meta->name) {
        m__free(meta->name);
    }
    if (meta->typdef) {
        typ_free_typdef(meta->typdef);
    }
    m__free(meta);

}  /* obj_free_metadata */


/********************************************************************
* FUNCTION obj_add_metadata
* 
* Add the filled out object metadata definition to the object
*
* INPUTS:
*    meta == obj_metadata_t data structure to add
*    obj == object template to add meta to
*
* RETURNS:
*    status
*********************************************************************/
status_t
    obj_add_metadata (obj_metadata_t *meta,
                      obj_template_t *obj)
{
    obj_metadata_t *testmeta;

#ifdef DEBUG
    if (!meta || !obj) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    testmeta = obj_find_metadata(obj, meta->name);
    if (testmeta) {
        return ERR_NCX_ENTRY_EXISTS;
    }

    meta->parent = obj;
    meta->nsid = obj_get_nsid(obj);
    dlq_enque(meta, &obj->metadataQ);
    return NO_ERR;

}  /* obj_add_metadata */


/********************************************************************
* FUNCTION obj_find_metadata
* 
* Find the object metadata definition in the object
*
* INPUTS:
*    obj == object template to check
*    name == name of obj_metadata_t data structure to find
*
* RETURNS:
*    pointer to found entry, NULL if not found
*********************************************************************/
obj_metadata_t *
    obj_find_metadata (const obj_template_t *obj,
                       const xmlChar *name)
{
    obj_metadata_t *testmeta;

#ifdef DEBUG
    if (!obj || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (testmeta = (obj_metadata_t *)
             dlq_firstEntry(&obj->metadataQ);
         testmeta != NULL;
         testmeta = (obj_metadata_t *)
             dlq_nextEntry(testmeta)) {

        if (!xml_strcmp(testmeta->name, name)) {
            return testmeta;
        }
    }

    return NULL;

}  /* obj_find_metadata */


/********************************************************************
* FUNCTION obj_first_metadata
* 
* Get the first object metadata definition in the object
*
* INPUTS:
*    obj == object template to check
*
* RETURNS:
*    pointer to first entry, NULL if none
*********************************************************************/
obj_metadata_t *
    obj_first_metadata (const obj_template_t *obj)
{

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_metadata_t *)
        dlq_firstEntry(&obj->metadataQ);

}  /* obj_first_metadata */


/********************************************************************
* FUNCTION obj_next_metadata
* 
* Get the next object metadata definition in the object
*
* INPUTS:
*    meta == current meta object template
*
* RETURNS:
*    pointer to next entry, NULL if none
*********************************************************************/
obj_metadata_t *
    obj_next_metadata (const obj_metadata_t *meta)
{

#ifdef DEBUG
    if (!meta) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (obj_metadata_t *)dlq_nextEntry(meta);

}  /* obj_next_metadata */


/********************************************************************
 * FUNCTION obj_sort_children
 * 
 * Check all the child nodes of the specified object
 * and rearrange them into alphabetical order,
 * based on the element local-name.
 *
 * ONLY SAFE TO USE FOR ncx:cli CONTAINERS
 * YANG DATA CONTENT ORDER NEEDS TO BE PRESERVED
 *
 * INPUTS:
 *    obj == object template to reorder
 *********************************************************************/
void
    obj_sort_children (obj_template_t *obj)
{
    obj_template_t    *newchild, *curchild;
    dlq_hdr_t         *datadefQ, sortQ;
    boolean            done;
    int                retval;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    datadefQ = obj_get_datadefQ(obj);
    if (datadefQ == NULL) {
        return;
    } 

    dlq_createSQue(&sortQ);
    newchild = (obj_template_t *)dlq_deque(datadefQ);
    while (newchild != NULL) {
        if (!obj_has_name(newchild)) {
            dlq_enque(newchild, &sortQ);
        } else {
            obj_sort_children(newchild);

            done = FALSE;
            for (curchild = (obj_template_t *)
                     dlq_firstEntry(&sortQ);
                 curchild != NULL && !done;
                 curchild = (obj_template_t *)
                     dlq_nextEntry(curchild)) {
            
                if (!obj_has_name(curchild)) {
                    continue;
                }

                retval = xml_strcmp(obj_get_name(newchild),
                                    obj_get_name(curchild));
                if (retval == 0) {        
                   if (obj_get_nsid(newchild) 
                        < obj_get_nsid(curchild)) {
                        dlq_insertAhead(newchild, curchild);
                    } else {
                        dlq_insertAfter(newchild, curchild);                    
                    }
                   done = TRUE;
                } else if (retval < 0) {
                    dlq_insertAhead(newchild, curchild);
                    done = TRUE;
                }
            }
            
            if (!done) {
                dlq_enque(newchild, &sortQ);
            }
        }
        newchild = (obj_template_t *)dlq_deque(datadefQ);
    }

    dlq_block_enque(&sortQ, datadefQ);

}  /* obj_sort_children */


/********************************************************************
* FUNCTION obj_set_ncx_flags
*
* Check the NCX appinfo extensions and set flags as needed
*
** INPUTS:
*   obj == obj_template to check
*
* OUTPUTS:
*   may set additional bits in the obj->flags field
*
*********************************************************************/
void
    obj_set_ncx_flags (obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );

    const dlq_hdr_t *appinfoQ = obj_get_appinfoQ(obj);

    if (obj_is_leafy(obj)) {
        if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_PASSWORD)) {
            obj->flags |= OBJ_FL_PASSWD;
        }
    }

    if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_HIDDEN)) {
        obj->flags |= OBJ_FL_HIDDEN;
    }

    if (obj_is_leafy(obj)) {
        if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_XSDLIST)) {
            obj->flags |= OBJ_FL_XSDLIST;
        }
    }

    if (obj->objtype == OBJ_TYP_CONTAINER) {
        if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_ROOT)) {
            obj->flags |= OBJ_FL_ROOT;
        }
    }

    if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_CLI)) {
        obj->flags |= OBJ_FL_CLI;
    }

    if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_ABSTRACT)) {
        obj->flags |= OBJ_FL_ABSTRACT;
    }

    if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, 
                               NCX_EL_DEFAULT_PARM_EQUALS_OK)) {
        obj->flags |= OBJ_FL_CLI_EQUALS_OK;
    }

    if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, 
                               NCX_EL_SIL_DELETE_CHILDREN_FIRST)) {
        obj->flags |= OBJ_FL_SIL_DELETE_CHILDREN_FIRST;
    }

    if (ncx_find_const_appinfo(appinfoQ, NACM_PREFIX, NCX_EL_SECURE)) {
        obj->flags |= OBJ_FL_SECURE;
    }

    if (ncx_find_const_appinfo(appinfoQ, NACM_PREFIX, NCX_EL_VERY_SECURE)) {
        obj->flags |= OBJ_FL_VERY_SECURE;
    }

    if (obj_is_config(obj)) {
        const ncx_appinfo_t *appinfo = 
            ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, NCX_EL_USER_WRITE);
        if (appinfo) {
            const xmlChar *str = ncx_get_appinfo_value(appinfo);
            if (str) {
                ncx_list_t mylist;
                ncx_init_list(&mylist, NCX_BT_STRING);
                status_t res = ncx_set_list(NCX_BT_STRING, str, &mylist);
                if (res != NO_ERR) {
                    /* not setting any user-write flags! */
                    log_error("\nError: invalid ncx:user-write value '%s' (%s)",
                              str, get_error_string(res));
                } else {
                    /* not checking if the list has extra bogus strings!! */
                    if (!ncx_string_in_list(NCX_EL_CREATE, &mylist)) {
                        obj->flags |= OBJ_FL_BLOCK_CREATE;
                    }
                    if (!ncx_string_in_list(NCX_EL_UPDATE, &mylist)) {
                        obj->flags |= OBJ_FL_BLOCK_UPDATE;
                    }
                    if (!ncx_string_in_list(NCX_EL_DELETE, &mylist)) {
                        obj->flags |= OBJ_FL_BLOCK_DELETE;
                    }
                }
                ncx_clean_list(&mylist);                
            } else {
                /* treat no value the same as an empty string,
                 * which means no user access at all;
                 * YANG parser should complain if the extension usage
                 * has no value proveded    */
                obj->flags |= OBJ_FL_BLOCK_CREATE;
                obj->flags |= OBJ_FL_BLOCK_UPDATE;
                obj->flags |= OBJ_FL_BLOCK_DELETE;
            }
        }
    }

    if (obj_is_leafy(obj)) {
        const typ_def_t *typdef = obj_get_ctypdef(obj);

        /* ncx:xpath extension */
        if (typ_is_xpath_string(typdef)) {
            obj->flags |= OBJ_FL_XPATH;
        } else if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, 
                                          NCX_EL_XPATH)) {
            obj->flags |= OBJ_FL_XPATH;
        }

        /* ncx:qname extension */
        if (typ_is_qname_string(typdef)) {
            obj->flags |= OBJ_FL_QNAME;
        } else if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, 
                                          NCX_EL_XPATH)) {
            obj->flags |= OBJ_FL_QNAME;
        }

        /* ncx:schema-instance extension */
        if (typ_is_schema_instance_string(typdef)) {
            obj->flags |= OBJ_FL_SCHEMAINST;
        } else if (ncx_find_const_appinfo(appinfoQ, NCX_PREFIX, 
                                          NCX_EL_SCHEMA_INSTANCE)) {
            obj->flags |= OBJ_FL_SCHEMAINST;
        }
    }

}   /* obj_set_ncx_flags */


/********************************************************************
* FUNCTION obj_enabled_child_count
*
* Get the count of the number of enabled child nodes
* for the object template
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   number of enabled child nodes
*********************************************************************/
uint32
    obj_enabled_child_count (obj_template_t *obj)
{
    dlq_hdr_t       *childQ;
    obj_template_t  *chobj;
    uint32           count;

#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    childQ = obj_get_datadefQ(obj);
    if (childQ == NULL) {
        return 0;
    }

    count = 0;

    for (chobj = (obj_template_t *)dlq_firstEntry(childQ);
         chobj != NULL;
         chobj = (obj_template_t *)dlq_nextEntry(chobj)) {
        if (!obj_has_name(chobj)) {
            continue;
        }
        if (obj_is_enabled(chobj)) {
            count++;
        }
    }
    return count;

}  /* obj_enabled_child_count */


/********************************************************************
* FUNCTION obj_dump_child_list
*
* Dump the object names in a datadefQ -- just child level
* uses log_write for output
*
* INPUTS:
*   datadefQ == Q of obj_template_t to dump
*   startindent == start-indent columns
*   indent == indent amount
*********************************************************************/
void
    obj_dump_child_list (dlq_hdr_t *datadefQ,
                         uint32  startindent,
                         uint32 indent)
{
    obj_template_t  *obj;
    dlq_hdr_t       *child_datadefQ;
    uint32           i;

#ifdef DEBUG
    if (!datadefQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        log_write("\n");
        for (i=0; i < startindent; i++) {
            log_write(" ");
        }

        log_write("%s", obj_get_typestr(obj));

        if (obj_has_name(obj)) {
            log_write(" %s", obj_get_name(obj));
        }

        child_datadefQ = obj_get_datadefQ(obj);
        if (child_datadefQ != NULL) {
            obj_dump_child_list(child_datadefQ,
                                startindent+indent,
                                indent);
        }
    }

}  /* obj_dump_child_list */


/********************************************************************
* FUNCTION obj_get_keystr
*
* Get the key string for this list object
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   pointer to key string or NULL if none or not a list
*********************************************************************/
const xmlChar *
    obj_get_keystr (obj_template_t *obj)
{
#ifdef DEBUG
    if (!obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (obj->objtype != OBJ_TYP_LIST) {
        return NULL;
    }

    return obj->def.list->keystr;

}  /* obj_get_keystr */


/********************************************************************
* FUNCTION obj_delete_obsolete
*
* Delete any obsolete child nodes within the specified object subtree
*
* INPUTS:
*   objQ == Q of obj_template to check
*
*********************************************************************/
void
    obj_delete_obsolete (dlq_hdr_t  *objQ)
{
    obj_template_t  *childobj, *nextobj;
    dlq_hdr_t       *childdatadefQ;

#ifdef DEBUG
    if (objQ == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    for (childobj = (obj_template_t *)dlq_firstEntry(objQ);
         childobj != NULL;
         childobj = nextobj) {

        nextobj = (obj_template_t *)dlq_nextEntry(childobj);
        if (obj_has_name(childobj) &&
            obj_get_status(childobj) == NCX_STATUS_OBSOLETE) {
            if (LOGDEBUG) {
                ncx_module_t  *testmod = obj_get_mod(childobj);
                log_debug("\nDeleting obsolete node '%s' "
                          "from %smodule '%s'",
                          obj_get_name(childobj),
                          (testmod && !testmod->ismod) ? "sub" : "",
                          (testmod) ? testmod->name : EMPTY_STRING);
            }
            dlq_remove(childobj);
            obj_free_template(childobj);
        } else {
            childdatadefQ = obj_get_datadefQ(childobj);
            if (childdatadefQ != NULL) {
                obj_delete_obsolete(childdatadefQ);
            }
        }
    }

}  /* obj_delete_obsolete */

/********************************************************************
 * Get the alt-name for this object, if any
 *
 * \param obj the obj_template to check
 * \return pointer to alt-name of NULL if none
 *********************************************************************/
const xmlChar* obj_get_altname (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );

    const xmlChar *altname = NULL;

    const ncx_appinfo_t* appinfo = ncx_find_const_appinfo( &obj->appinfoQ, 
            NULL, /* any module */
            NCX_EL_ALT_NAME );

    if ( appinfo ) {
        altname = ncx_get_appinfo_value(appinfo);
    }

    return altname;

}   /* obj_get_altname */

/********************************************************************
 * Get the target object for a leafref leaf or leaf-list
 * \param obj the object to check
 * \return pointer to the target object or NULL if this object type does not 
 *      have a leafref target object
 *********************************************************************/
obj_template_t *
    obj_get_leafref_targobj (obj_template_t  *obj)
{
    assert( obj && "obj is NULL!" );

    if (obj->objtype == OBJ_TYP_LEAF) {
        return obj->def.leaf->leafrefobj;
    } else if (obj->objtype == OBJ_TYP_LEAF_LIST) {
        return obj->def.leaflist->leafrefobj;
    } 

    return NULL;
}  /* obj_get_leafref_targobj */


/********************************************************************
 * Get the target object for an augments object
 * \param obj the object to check
 * \return pointer to the augment context target object
 *   or NULL if this object type does not have an augment target object
 *********************************************************************/
obj_template_t *
    obj_get_augment_targobj (obj_template_t  *obj)
{
    assert( obj && "obj is NULL!" );

    if (obj->augobj && obj->augobj->objtype == OBJ_TYP_AUGMENT) {
        return obj->augobj->def.augment->targobj;
    } 

    return NULL;
}  /* obj_get_augment_targobj */

/********************************************************************
 * Check if object is marked as ncx:default-parm-equals-ok
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:default-parm-equals-ok
 *********************************************************************/
boolean obj_is_cli_equals_ok (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );
    return (obj->flags & OBJ_FL_CLI_EQUALS_OK) ? TRUE : FALSE;
}   /* obj_is_cli_equals_ok */

/********************************************************************
 * Check if object is marked as ncx:sil-delete-children-first
 *
 * \param obj the obj_template to check
 * \return TRUE if object is marked as ncx:sil-delete-children-first.
 *********************************************************************/
boolean obj_is_sil_delete_children_first (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );
    return (obj->flags & OBJ_FL_SIL_DELETE_CHILDREN_FIRST) ? TRUE : FALSE;
}   /* obj_is_sil_delete_children_first */

/********************************************************************
 * Add a child object to the specified complex node
 *
 * \param child the obj_template to add
 * \param parent the obj_template of the parent
 *********************************************************************/
void obj_add_child (obj_template_t *child, obj_template_t *parent)
{
    assert( child && "child is NULL!" );
    assert( parent && "parent is NULL!" );

    dlq_hdr_t *que = obj_get_datadefQ(parent);
    if (que) {
        dlq_enque(child, que);
    }
    child->parent = parent;

}   /* obj_add_child */


/********************************************************************
* FUNCTION obj_is_block_user_create
*
* Check if object is marked as ncx:user-write with create 
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user create access
*   FALSE if not
*********************************************************************/
boolean
    obj_is_block_user_create (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );
    return (obj->flags & OBJ_FL_BLOCK_CREATE) ? TRUE : FALSE;
}


/********************************************************************
* FUNCTION obj_is_block_user_update
*
* Check if object is marked as ncx:user-write with update
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user update access
*   FALSE if not
*********************************************************************/
boolean
    obj_is_block_user_update (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );
    return (obj->flags & OBJ_FL_BLOCK_UPDATE) ? TRUE : FALSE;
}


/********************************************************************
* FUNCTION obj_is_block_user_delete
*
* Check if object is marked as ncx:user-write with delete
* access disabled
*
* INPUTS:
*   obj == obj_template to check
*
* RETURNS:
*   TRUE if object is marked to block user delete access
*   FALSE if not
*********************************************************************/
boolean
    obj_is_block_user_delete (const obj_template_t *obj)
{
    assert( obj && "obj is NULL!" );
    return (obj->flags & OBJ_FL_BLOCK_DELETE) ? TRUE : FALSE;
}


/********************************************************************
* FUNCTION obj_new_iffeature_ptr
*
* Malloc and initialize a new obj_iffeature_ptr_t struct
*
* INPUTS:
*  iff == iffeature to point at
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
obj_iffeature_ptr_t *
    obj_new_iffeature_ptr (ncx_iffeature_t *iff)
{
    obj_iffeature_ptr_t *iffptr = m__getObj(obj_iffeature_ptr_t);
    if (iffptr == NULL) {
        return NULL;
    }
    memset(iffptr, 0x0, sizeof(obj_iffeature_ptr_t));
    iffptr->iffeature = iff;
    return iffptr;
}


/********************************************************************
* FUNCTION obj_free_iffeature_ptr
*
* Free an obj_iffeature_ptr_t struct
*
* INPUTS:
*   iffptr == struct to free
*********************************************************************/
void obj_free_iffeature_ptr (obj_iffeature_ptr_t *iffptr)
{
    if (iffptr == NULL) {
        return;
    }
    m__free(iffptr);
}


/********************************************************************
 * Get first if-feature pointer
 *
 * \param obj the obj_template to check
 * \return pointer to first entry or NULL if none
 *********************************************************************/
obj_iffeature_ptr_t *
    obj_first_iffeature_ptr (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    obj_iffeature_ptr_t *iffptr = (obj_iffeature_ptr_t *)
        dlq_firstEntry(&obj->inherited_iffeatureQ);
    return iffptr;

}  /* obj_first_iffeature_ptr */


/********************************************************************
 * Get the next if-feature pointer
 *
 * \param iffptr the current iffeature ptr struct
 * \return pointer to next entry or NULL if none
 *********************************************************************/
obj_iffeature_ptr_t *
    obj_next_iffeature_ptr (obj_iffeature_ptr_t *iffptr)
{
    assert(iffptr && "iffptr is NULL" );

    obj_iffeature_ptr_t *nextptr = (obj_iffeature_ptr_t *)
        dlq_nextEntry(iffptr);
    return nextptr;

}  /* obj_next_iffeature_ptr */


/********************************************************************
* FUNCTION obj_new_xpath_ptr
*
* Malloc and initialize a new obj_xpath_ptr_t struct
*
* INPUTS:
*   xpath == Xpath PCB to point at
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
obj_xpath_ptr_t *
    obj_new_xpath_ptr (struct xpath_pcb_t_ *xpath)
{
    obj_xpath_ptr_t *xptr = m__getObj(obj_xpath_ptr_t);
    if (xptr == NULL) {
        return NULL;
    }
    memset(xptr, 0x0, sizeof(obj_xpath_ptr_t));
    xptr->xpath = xpath;
    return xptr;
}


/********************************************************************
* FUNCTION obj_free_xpath_ptr
*
* Free an obj_xpath_ptr_t struct
*
* INPUTS:
*   xptr == struct to free
*********************************************************************/
void obj_free_xpath_ptr (obj_xpath_ptr_t *xptr)
{
    if (xptr == NULL) {
        return;
    }
    m__free(xptr);
}


/********************************************************************
 * Get first xpath pointer struct
 *
 * \param obj the obj_template to check
 * \return pointer to first entry or NULL if none
 *********************************************************************/
obj_xpath_ptr_t *
    obj_first_xpath_ptr (obj_template_t *obj)
{
    assert(obj && "obj is NULL" );

    obj_xpath_ptr_t *xptr = (obj_xpath_ptr_t *)
        dlq_firstEntry(&obj->inherited_whenQ);
    return xptr;

}  /* obj_first_xpath_ptr */


/********************************************************************
 * Get the next xpath pointer struct
 *
 * \param xptr the current xpath ptr struct
 * \return pointer to next entry or NULL if none
 *********************************************************************/
obj_xpath_ptr_t *
    obj_next_xpath_ptr (obj_xpath_ptr_t *xptr)
{
    assert(xptr && "xptr is NULL" );
    obj_xpath_ptr_t *nextptr = (obj_xpath_ptr_t *)dlq_nextEntry(xptr);
    return nextptr;

}  /* obj_next_iffeature_ptr */



/* END obj.c */
