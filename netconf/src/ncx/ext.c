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
/*  FILE: ext.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
05jan08      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncx_appinfo
#include "ncx_appinfo.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                         V A R I A B L E S                         *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION ext_new_template
* 
* Malloc and initialize the fields in a ext_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
ext_template_t * 
    ext_new_template (void)
{
    ext_template_t  *ext;

    ext = m__getObj(ext_template_t);
    if (!ext) {
        return NULL;
    }
    (void)memset(ext, 0x0, sizeof(ext_template_t));
    dlq_createSQue(&ext->appinfoQ);
    ext->status = NCX_STATUS_CURRENT;   /* default */
    return ext;

}  /* ext_new_template */


/********************************************************************
* FUNCTION ext_free_template
* 
* Scrub the memory in a ext_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    ext == ext_template_t data structure to free
*********************************************************************/
void 
    ext_free_template (ext_template_t *ext)
{
#ifdef DEBUG
    if (!ext) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (ext->name) {
        m__free(ext->name);
    }
    if (ext->descr) {
        m__free(ext->descr);
    }
    if (ext->ref) {
        m__free(ext->ref);
    }
    if (ext->arg) {
        m__free(ext->arg);
    }

    ncx_clean_appinfoQ(&ext->appinfoQ);

    m__free(ext);

}  /* ext_free_template */


/********************************************************************
* FUNCTION ext_clean_extensionQ
* 
* Clean a queue of ext_template_t structs
*
* INPUTS:
*    que == Q of ext_template_t data structures to free
*********************************************************************/
void 
    ext_clean_extensionQ (dlq_hdr_t *que)
{
    ext_template_t *ext;

#ifdef DEBUG
    if (!que) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(que)) {
        ext = (ext_template_t *)dlq_deque(que);
        ext_free_template(ext);
    }

}  /* ext_clean_groupingQ */


/********************************************************************
* FUNCTION ext_find_extension
* 
* Search a queue of ext_template_t structs for a given name
*
* INPUTS:
*    que == Q of ext_template_t data structures to search
*    name == name string to find
*
* RETURNS:
*   pointer to found entry, or NULL if not found
*********************************************************************/
ext_template_t *
    ext_find_extension (ncx_module_t *mod,
                        const xmlChar *name)
{
    ext_template_t  *extension;
    dlq_hdr_t       *que;
    yang_node_t     *node;
    ncx_include_t   *inc;

#ifdef DEBUG
    if (!mod || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    extension = ext_find_extension_que(&mod->extensionQ, name);
    if (extension) {
        return extension;
    }

    que = ncx_get_allincQ(mod);

    /* check all the submodules, but only the ones visible
     * to this module or submodule
     */
    for (inc = (ncx_include_t *)dlq_firstEntry(&mod->includeQ);
         inc != NULL;
         inc = (ncx_include_t *)dlq_nextEntry(inc)) {

        /* get the real submodule struct */
        if (!inc->submod) {
            node = yang_find_node(que, 
                                  inc->submodule,
                                  inc->revision);
            if (node) {
                inc->submod = node->submod;
            }
            if (!inc->submod) {
                /* include not found or errors found in it */
                continue;
            }
        }

        /* check the extension Q in this submodule */
        extension = ext_find_extension_que(&inc->submod->extensionQ, name);
        if (extension) {
            return extension;
        }
    }

    return NULL;

}  /* ext_find_extension */


/********************************************************************
* FUNCTION ext_find_extension_que
* 
* Find an ext_template_t struct in the specified Q
*
* INPUTS:
*    extensionQ == Q of ext_template_t to search
*    name == extension name to find
*
* RETURNS:
*    pointer to found extension or NULL if not found
*********************************************************************/
ext_template_t *
    ext_find_extension_que (dlq_hdr_t *extensionQ,
                            const xmlChar *name)
{
    ext_template_t *extension;

#ifdef DEBUG
    if (!extensionQ || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (extension = (ext_template_t *)dlq_firstEntry(extensionQ);
         extension != NULL;
         extension = (ext_template_t *)dlq_nextEntry(extension)) {

        if (!xml_strcmp(extension->name, name)) {
            return extension;
        }
    }
    return NULL;
         
} /* ext_find_extension_que */


/********************************************************************
* FUNCTION ext_find_extension_all
* 
* Search a module of ext_template_t structs for a given name
* Check all submodules as well
*
* INPUTS:
*    mod == module to check
*    name == name string to find
*
* RETURNS:
*   pointer to found entry, or NULL if not found
*********************************************************************/
ext_template_t *
    ext_find_extension_all (ncx_module_t *mod,
                            const xmlChar *name)
{
    ext_template_t  *extension;
    dlq_hdr_t       *que;
    yang_node_t     *node;

#ifdef DEBUG
    if (!mod || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    extension = ext_find_extension_que(&mod->extensionQ, name);
    if (extension) {
        return extension;
    }

    que = ncx_get_allincQ(mod);

    /* check all the submodules, but only the ones visible
     * to this module or submodule
     */
    for (node = (yang_node_t *)dlq_firstEntry(que);
         node != NULL;
         node = (yang_node_t *)dlq_nextEntry(node)) {

        if (node->submod) {
            /* check the extension Q in this submodule */
            extension = ext_find_extension_que(&node->submod->extensionQ, 
                                               name);
            if (extension) {
                return extension;
            }
        }
    }

    return NULL;

}  /* ext_find_extension_all */



/* END ext.c */
