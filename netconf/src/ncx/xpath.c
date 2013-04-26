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
/*  FILE: xpath.c

    Schema and data model Xpath search support
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
31dec07      abb      begun

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
#include "ncx_num.h"
#include "obj.h"
#include "tk.h"
#include "typ.h"
#include "var.h"
#include "xpath.h"
#include "xpath1.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
#define MAX_FILL  255


/********************************************************************
* FUNCTION do_errmsg
* 
* Generate the errormsg
*
* INPUTS:
*    tkc == token chain in progress (may be NULL)
*    mod == module in progress
*    tkerr == error struct to use
*    res == error enumeration
*
*********************************************************************/
static void
    do_errmsg (tk_chain_t *tkc,
               ncx_module_t *mod,
               ncx_error_t *tkerr,
               status_t  res)
{
    if (tkc) {
        tkc->curerr = tkerr;
    }
    ncx_print_errormsg(tkc, mod, res);

}  /* do_errmsg */


/********************************************************************
* FUNCTION next_nodeid
* 
* Get the next Name of QName segment of an Xpath schema-nodeid
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    tkc == token chain in progress  (may be NULL)
*    mod == module in progress
*    obj == object calling this fn (for error purposes)
*    target == Xpath expression string to evaluate
*    prefix == address for return buffer to store 
*          prefix portion of QName (if any)
*    name == address for return buffer to store 
*          name portion of segment
*    len == address of return byte count
*
* OUTPUTS:
*   *prefix == malloced buffer with prefix portion of QName
*   *name == malloced name name portion of QName
*   *cnt == number of bytes used in target
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    next_nodeid (tk_chain_t *tkc,
                 ncx_module_t *mod,
                 obj_template_t *obj,
                 const xmlChar *target,
                 xmlChar **prefix,
                 xmlChar **name,
                 uint32 *len)
{
    const xmlChar *p, *q;
    uint32         cnt;
    status_t       res;

    *prefix = NULL;
    *name = NULL;
    *len = 0;

    /* find the EOS or a separator */
    p = target;
    while (*p && *p != ':' && *p != '/') {
        p++;
    }
    cnt = (uint32)(p-target);

    if (!ncx_valid_name(target, cnt)) {
        log_error("\nError: invalid name string (%s)", 
                  target);
        res = ERR_NCX_INVALID_NAME;
        do_errmsg(tkc, mod, &obj->tkerr, res);
        return res;
    }

    if (*p==':') {
        /* copy prefix, then get name portion */
        *prefix = m__getMem(cnt+1);
        if (!*prefix) {
            log_error("\nError: malloc failed");
            res = ERR_INTERNAL_MEM;
            do_errmsg(tkc, mod, &obj->tkerr, res);
            return res;
        }
        xml_strncpy(*prefix, target, cnt);

        q = ++p;
        while (*q && *q != '/') {
            q++;
        }
        cnt = (uint32)(q-p);

        if (!ncx_valid_name(p, cnt)) {
            log_error("\nError: invalid name string (%s)", 
                      target);
            res = ERR_NCX_INVALID_NAME;
            do_errmsg(tkc, mod, &obj->tkerr, res);
            if (*prefix) {
                m__free(*prefix);
                *prefix = NULL;
            }
            return res;
        }

        *name = m__getMem(cnt+1);
        if (!*name) {
            log_error("\nError: malloc failed");
            res = ERR_INTERNAL_MEM;
            do_errmsg(tkc, mod, &obj->tkerr, res);
            if (*prefix) {
                m__free(*prefix);
                *prefix = NULL;
            }
            return res;
        }

        xml_strncpy(*name, p, cnt);
        *len = (uint32)(q-target);
    } else  {
        /* found EOS or pathsep, got just one 'name' string */
        *name = m__getMem(cnt+1);
        if (!*name) {
            log_error("\nError: malloc failed");
            res = ERR_INTERNAL_MEM;
            do_errmsg(tkc, mod, &obj->tkerr, res);
            return res;
        }
        xml_strncpy(*name, target, cnt);
        *len = cnt;
    }
    return NO_ERR;

}  /* next_nodeid */


/********************************************************************
* FUNCTION next_nodeid_noerr
* 
* Get the next Name of QName segment of an Xpath schema-nodeid
*
* Error messages are not printed by this function!!
*
* INPUTS:
*    target == Xpath expression string in progress to evaluate
*    prefix == address for return buffer to store 
*          prefix portion of QName (if any)
*    name == address for return buffer to store 
*          name portion of segment
*    len == address of return byte count
*
* OUTPUTS:
*   *prefix == malloced buffer with prefix portion of QName
*   *name == malloced name name portion of QName
*   *cnt == number of bytes used in target
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    next_nodeid_noerr (const xmlChar *target,
                       xmlChar **prefix,
                       xmlChar **name,
                       uint32 *len)
{
    const xmlChar *p, *q;
    uint32         cnt;

    *prefix = NULL;
    *name = NULL;
    *len = 0;

    /* find the EOS or a separator */
    p = target;
    while (*p && *p != ':' && *p != '/') {
        p++;
    }
    cnt = (uint32)(p-target);

    if (!ncx_valid_name(target, cnt)) {
        return ERR_NCX_INVALID_NAME;
    }

    if (*p==':') {
        *prefix = m__getMem(cnt+1);
        if (!*prefix) {
            return ERR_INTERNAL_MEM;
        }
        /* copy prefix, then get name portion */
        xml_strncpy(*prefix, target, cnt);

        q = ++p;
        while (*q && *q != '/') {
            q++;
        }
        cnt = (uint32)(q-p);

        if (!ncx_valid_name(p, cnt)) {
            if (*prefix) {
                m__free(*prefix);
                *prefix = NULL;
            }
            return ERR_NCX_INVALID_NAME;
        }

        *name = m__getMem(cnt+1);
        if (!*name) {
            if (*prefix) {
                m__free(*prefix);
                *prefix = NULL;
            }
            return ERR_INTERNAL_MEM;
        }
            
        xml_strncpy(*name, p, cnt);
        *len = (uint32)(q-target);
    } else  {
        /* found EOS or pathsep, got just one 'name' string */
        *name = m__getMem(cnt+1);
        if (!*name) {
            return ERR_INTERNAL_MEM;
        }
        xml_strncpy(*name, target, cnt);
        *len = cnt;
    }
    return NO_ERR;

}  /* next_nodeid_noerr */


/********************************************************************
* FUNCTION next_val_nodeid
* 
* Get the next Name of QName segment of an Xpath schema-nodeid
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    target == Xpath expression string to evaluate
*    logerrors = TRUE to use log_error, FALSE to skip it
*    prefix == address for return buffer to store 
*          prefix portion of QName (if any)
*    name == address for return buffer to store 
*          name portion of segment
*    len == address of return byte count
*
* OUTPUTS:
*   *prefix == malloced buffer with prefix portion of QName
*   *name == malloced name name portion of QName
*   *cnt == number of bytes used in target
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    next_val_nodeid (const xmlChar *target,
                     boolean logerrors,
                     xmlChar **prefix,
                     xmlChar **name,
                     uint32 *len)
{
    const xmlChar *p, *q;
    uint32         cnt;

    *prefix = NULL;
    *name = NULL;
    *len = 0;

    /* find the EOS or a separator */
    p = target;
    while (*p && *p != ':' && *p != '/') {
        p++;
    }
    cnt = (uint32)(p-target);

    if (!ncx_valid_name(target, cnt)) {
        if (logerrors) {
            log_error("\nError: invalid name string (%s)", 
                      target);
        }
        return ERR_NCX_INVALID_NAME;
    }

    if (*p==':') {
        *prefix = m__getMem(cnt+1);
        if (!*prefix) {
            return ERR_INTERNAL_MEM;
        }
        /* copy prefix, then get name portion */
        xml_strncpy(*prefix, target, cnt);

        q = ++p;
        while (*q && *q != '/') {
            q++;
        }
        cnt = (uint32)(q-p);

        if (!ncx_valid_name(p, cnt)) {
            if (logerrors) {
                log_error("\nError: invalid name string (%s)", 
                          target);
            }
            m__free(*prefix);
            *prefix = NULL;
            return ERR_NCX_INVALID_NAME;
        }

        *name = m__getMem(cnt+1);
        if (!*name) {
            if (*prefix) {
                m__free(*prefix);
                *prefix = NULL;
            }
            return ERR_INTERNAL_MEM;
        }
        xml_strncpy(*name, p, cnt);
        *len = (uint32)(q-target);
    } else  {
        *name = m__getMem(cnt+1);
        if (!*name) {
            return ERR_INTERNAL_MEM;
        }
        /* found EOS or pathsep, got just one 'name' string */
        xml_strncpy(*name, target, cnt);
        *len = cnt;
    }
    return NO_ERR;

}  /* next_val_nodeid */


/********************************************************************
* FUNCTION find_schema_node
* 
* Follow the absolute-path or descendant node path expression
* and return the obj_template_t that it indicates, and the
* que that the object is in
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parswer control block to use
*    tkc == token chain in progress
*    mod == module in progress
*    obj == object calling this fn (for error purposes)
*    datadefQ == Q of obj_template_t containing 'obj'
*    target == Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*    targQ == address of return target queue (may be NULL)
*    tkerr == error struct to use
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*      *targQ == datadefQ header for targobj
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    find_schema_node (yang_pcb_t *pcb,
                      tk_chain_t *tkc,
                      ncx_module_t *mod,
                      obj_template_t *obj,
                      dlq_hdr_t *datadefQ,
                      const xmlChar *target,
                      obj_template_t **targobj,
                      dlq_hdr_t **targQ,
                      ncx_error_t *tkerr)
{
    ncx_import_t   *imp;
    obj_template_t *curobj, *nextobj;
    dlq_hdr_t      *curQ;
    const xmlChar  *str;
    uint32          len;
    status_t        res;
    ncx_node_t      dtyp;
    xmlChar        *prefix;
    xmlChar        *name;

    imp = NULL;
    dtyp = NCX_NT_OBJ;
    curQ = NULL;
    prefix = NULL;
    name = NULL;

    /* skip the first fwd slash, if any */
    if (*target == '/') {
        str = target+1;
    } else {
        str = target;
    }

    /* get the first QName (prefix, name) */
    res = next_nodeid(tkc, mod, obj, str, &prefix, &name, &len);
    if (res != NO_ERR) {
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        str += len;
    }

    /* get the import if there is a real prefix entered */
    if (prefix && xml_strcmp(prefix, mod->prefix)) {
        imp = ncx_find_pre_import(mod, prefix);
        if (!imp) {
            log_error("\nError: prefix '%s' not found in module imports"
                      " in Xpath target %s", 
                      prefix, 
                      target);
            res = ERR_NCX_INVALID_NAME;
            do_errmsg(tkc, mod, tkerr, res);
            m__free(prefix);
            if (name) {
                m__free(name);
            }
            return res;
        }
    }

    /* get the first object template */
    if (imp) {
        curobj = ncx_locate_modqual_import(pcb,
                                           imp, 
                                           name, 
                                           &dtyp);
    } else if (*target == '/') {
        curobj = obj_find_template_top(mod,
                                       ncx_get_modname(mod),
                                       name);
    } else {
        curobj = obj_find_template(datadefQ, 
                                   ncx_get_modname(mod), 
                                   name);
    }

    if (!curobj) {
        if (ncx_valid_name2(name)) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
        log_error("\nError: object '%s' not found in module %s"
                  " in Xpath target %s",
                  name, (imp && imp->mod) ? imp->mod->name : mod->name,
                  target);
        do_errmsg(tkc, mod, tkerr, res);
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        curQ = datadefQ;
    }

    if (obj_is_augclone(curobj)) {
        res = ERR_NCX_INVALID_VALUE;
        log_error("\nError: augment is external: node '%s'"
                  " from module %s, line %u in Xpath target %s",
                  (name) ? name : NCX_EL_NONE,
                  curobj->tkerr.mod->name,
                  curobj->tkerr.linenum, 
                  target);
        do_errmsg(tkc, mod, tkerr, res);
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    }

    if (prefix) {
        m__free(prefix);
        prefix = NULL;
    }
    if (name) {
        m__free(name);
        name = NULL;
    }

    /* got the first object; keep parsing node IDs
     * until the Xpath expression is done or an error occurs
     */
    while (*str == '/') {
        str++;

        /* check for trailing '/' */
        if (*str == '\0') {
            continue;
        }

        /* get the next QName (prefix, name) */
        res = next_nodeid(tkc, mod, obj, str, &prefix, &name, &len);
        if (res != NO_ERR) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        } else {
            str += len;
        }

        /* make sure the prefix is valid, if present */
        if (prefix && xml_strcmp(prefix, mod->prefix)) {
            imp = ncx_find_pre_import(mod, prefix);
            if (!imp) {
                log_error("\nError: prefix '%s' not found in module"
                          " imports in Xpath target '%s'",
                          prefix, target);
                res = ERR_NCX_INVALID_NAME;
                do_errmsg(tkc, mod, tkerr, res);
                m__free(prefix);
                if (name) {
                    m__free(name);
                }
                return res;
            }
        } else {
            imp = NULL;
        }

        /* make sure the name is a valid name string */
        if (name && !ncx_valid_name2(name)) {
            log_error("\nError: object name '%s' not a valid "
                      "identifier in Xpath target '%s'",
                      name, target);
            res = ERR_NCX_INVALID_NAME;
            do_errmsg(tkc, mod, tkerr, res);
            if (prefix) {
                m__free(prefix);
            }
            m__free(name);
            return res;
        }

        /* determine 'nextval' based on [curval, prefix, name] */
        curQ = obj_get_datadefQ(curobj);

        if (name && curQ) {
            nextobj = obj_find_template(curQ,
                                        (imp && imp->mod) ? imp->mod->name : 
                                        ncx_get_modname(mod), 
                                        name);
        } else {
            res = ERR_NCX_DEFSEG_NOT_FOUND;
            log_error("\nError: '%s' in Xpath target '%s' invalid: "
                      "%s on line %u is a %s",
                      name, 
                      target, 
                      obj_get_name(curobj),
                      curobj->tkerr.linenum, 
                      obj_get_typestr(curobj));
            do_errmsg(tkc, mod, tkerr, res);
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        }

        if (nextobj) {
            curobj = nextobj;
        } else {
            res = ERR_NCX_DEFSEG_NOT_FOUND;
            log_error("\nError: object '%s' not found in module %s",
                      name, 
                      (imp && imp->mod) ? imp->mod->name : mod->name);
            do_errmsg(tkc, mod, tkerr, res);
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        }

        if (prefix) {
            m__free(prefix);
            prefix = NULL;
        }
        if (name) {
            m__free(name);
            name = NULL;
        }
    }

    if (targobj) {
        *targobj = curobj;
    }
    if (targQ) {
        *targQ = curQ;
    }

    if (prefix) {
        m__free(prefix);
    }
    if (name) {
        m__free(name);
    }

    return NO_ERR;

}  /* find_schema_node */


/********************************************************************
* FUNCTION find_schema_node_int
* 
* Follow the absolute-path expression
* and return the obj_template_t that it indicates
* A missing prefix means check any namespace for the symbol
*
* !!! Internal version !!!
* !!! Error messages are not printed by this function!!
*
* INPUTS:
*    target == Absolute Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    find_schema_node_int (const xmlChar *target,
                          obj_template_t **targobj)
{
    obj_template_t *curobj, *nextobj;
    dlq_hdr_t      *curQ;
    ncx_module_t   *mod;
    const xmlChar  *str;
    xmlChar        *prefix;
    xmlChar        *name;
    uint32          len;
    status_t        res;

    prefix = NULL;
    name = NULL;

    /* skip the first fwd slash, if any
     * the target must be from the config root
     * so if the first fwd-slash is missing then
     * just keep going and assume the config root anyway
     */
    if (*target == '/') {
        str = ++target;
    } else {
        str = target;
    }

    /* get the first QName (prefix, name) */
    res = next_nodeid_noerr(str, &prefix, &name, &len);
    if (res != NO_ERR) {
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        str += len;
    }

    /* get the import if there is a real prefix entered */
    if (prefix) {
        mod = (ncx_module_t *)xmlns_get_modptr(xmlns_find_ns_by_prefix(prefix));
        if (!mod) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_INVALID_NAME;
        }
        /* get the first object template */
        curobj = obj_find_template_top(mod, ncx_get_modname(mod), name);
    } else {
        /* no prefix given, check all top-level objects */
        curobj = ncx_find_any_object(name);
    }

    /* check if first level object found */
    if (!curobj) {
        if (ncx_valid_name2(name)) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    }

    if (prefix) {
        m__free(prefix);
        prefix = NULL;
    }
    if (name) {
        m__free(name);
        name = NULL;
    }

    if (obj_is_augclone(curobj)) {
        return ERR_NCX_INVALID_VALUE;
    }

    /* got the first object; keep parsing node IDs
     * until the Xpath expression is done or an error occurs
     */
    while (*str == '/') {
        str++;
        /* get the next QName (prefix, name) */
        res = next_nodeid_noerr(str, &prefix, &name, &len);
        if (res != NO_ERR) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        } else {
            str += len;
        }

        /* make sure the name is a valid name string */
        if (!name || !ncx_valid_name2(name)) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_INVALID_NAME;
        }

        /* determine 'nextval' based on [curval, prefix, name] */
        curQ = obj_get_datadefQ(curobj);
        if (!curQ) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_DEFSEG_NOT_FOUND;
        }

        /* make sure the prefix is valid, if present */
        if (prefix) {
            mod = (ncx_module_t *)xmlns_get_modptr
                (xmlns_find_ns_by_prefix(prefix));
            if (!mod) {
                m__free(prefix);
                m__free(name);
                return ERR_NCX_INVALID_NAME;
            }
            nextobj = obj_find_template(curQ, ncx_get_modname(mod), name);
        } else {
            /* no prefix given; try current module first */
            nextobj = obj_find_template(curQ, obj_get_mod_name(curobj), name); 
            if (!nextobj) {
                nextobj = obj_find_template(curQ, NULL, name); 
            }
        }

        if (prefix) {
            m__free(prefix);
            prefix = NULL;
        }
        if (name) {
            m__free(name);
            name = NULL;
        }

        /* setup next loop or error exit because last node not found */
        if (nextobj) {
            curobj = nextobj;
        } else {
            return ERR_NCX_DEFSEG_NOT_FOUND;
        }
    }

    if (targobj) {
        *targobj = curobj;
    }

    return NO_ERR;

}  /* find_schema_node_int */


/********************************************************************
* FUNCTION find_val_node
* 
* Follow the Xpath expression
* and return the val_value_t that it indicates, if any
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    startval == val_value_t node to start search
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == Xpath expression string to evaluate
*    targval == address of return value  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targval == target value node
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    find_val_node (val_value_t *startval,
                   ncx_module_t *mod,
                   const xmlChar *target,
                   val_value_t **targval)
{
    val_value_t    *curval;
    ncx_module_t   *usemod;
    const xmlChar  *str;
    xmlChar        *prefix;
    xmlChar        *name;
    uint32          len;
    status_t        res;

    prefix = NULL;
    name = NULL;

    /* check absolute path starting with root val */
    if (*target == '/' && !obj_is_root(startval->obj)) {
        log_error("\nError: Absolute path given but startval is not "
                  "root in Xpath target %s", target);
        return ERR_NCX_DEF_NOT_FOUND;
    }

    /* check '/' corner-case */
    if (!xml_strcmp(target, (const xmlChar *)"/")) {
        if (targval) {
            *targval = startval;
        }
        return NO_ERR;
    }

    /* skip the first fwd slash, if any */
    if (*target == '/') {
        str = ++target;
    } else {
        str = target;
    }

    /* get the first QName (prefix, name) */
    res = next_val_nodeid(str, TRUE, &prefix, &name, &len);
    if (res != NO_ERR) {
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        str += len;
    }

    res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
    if (res != NO_ERR) {
        if (prefix) {
            log_error("\nError: module not found for prefix %s"
                      " in Xpath target %s",
                      prefix, target);
            m__free(prefix);
        } else {
            log_error("\nError: no module prefix specified"
                          " in Xpath target %s", target);
        }
        if (name) {
            m__free(name);
        }
        return ERR_NCX_MOD_NOT_FOUND;
    }

    /* get the first value node */
    curval = val_find_child(startval, usemod->name, name);
    if (!curval) {
        if (ncx_valid_name2(name)) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
        log_error("\nError: value node '%s' not found for module %s"
                  " in Xpath target %s",
                  name, usemod->name, target);
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    }

    if (prefix) {
        m__free(prefix);
        prefix = NULL;
    }
    if (name) {
        m__free(name);
        name = NULL;
    }

    /* got the first object; keep parsing node IDs
     * until the Xpath expression is done or an error occurs
     */
    while (*str == '/') {
        str++;
        /* get the next QName (prefix, name) */
        res = next_val_nodeid(str, TRUE, &prefix, &name, &len);
        if (res != NO_ERR) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        } else {
            str += len;
        }

        res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
        if (res != NO_ERR) {
            if (prefix) {
                log_error("\nError: module not found for prefix %s"
                          " in Xpath target %s",
                          prefix, target);
                m__free(prefix);
            } else {
                log_error("\nError: no module prefix specified"
                          " in Xpath target %s", target);
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_MOD_NOT_FOUND;
        }

        /* determine 'nextval' based on [curval, prefix, name] */
        switch (curval->obj->objtype) {
        case OBJ_TYP_CONTAINER:
        case OBJ_TYP_LIST:
        case OBJ_TYP_CHOICE:
        case OBJ_TYP_CASE:
        case OBJ_TYP_RPC:
        case OBJ_TYP_RPCIO:
        case OBJ_TYP_NOTIF:
            curval = val_find_child(curval, usemod->name, name);
            if (!curval) {
                if (ncx_valid_name2(name)) {
                    res = ERR_NCX_DEF_NOT_FOUND;
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
                log_error("\nError: value node '%s' not found for module %s"
                          " in Xpath target %s",
                          name, usemod->name, target);
                if (prefix) {
                    m__free(prefix);
                }
                if (name) {
                    m__free(name);
                }
                return res;
            }
            break;
        case OBJ_TYP_LEAF:
        case OBJ_TYP_LEAF_LIST:
            res = ERR_NCX_DEFSEG_NOT_FOUND;
            log_error("\nError: '%s' in Xpath target '%s' invalid: "
                      "%s is a %s",
                      name, target, curval->name,
                      obj_get_typestr(curval->obj));
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            do_errmsg(NULL, mod, NULL, res);
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        }
    }


    if (prefix) {
        m__free(prefix);
    }
    if (name) {
        m__free(name);
    }

    if (targval) {
        *targval = curval;
    }
    return NO_ERR;

}  /* find_val_node */


#if 0  // NOT USED SINCE UNIQUE ALLOWS NODE-SETS
/********************************************************************
* FUNCTION find_val_node_unique
* 
* Follow the relative-path schema-nodeid expression
* and return the val_value_t that it indicates, if any
* Used in the YANG unique-stmt component
*
*  [/] foo/bar/baz
*
* No predicates are expected, but choice and case nodes
* are expected and will be accounted for if present
*
* XML mode - unique-smmt processing
* check first before logging errors;
*
* If logerrors:
*   Error messages are printed by this function!!
*   Do not duplicate error messages upon error return
*
* INPUTS:
*    startval == val_value_t node to start search
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == relative path expr to evaluate
*    logerrors == TRUE to log_error, FALSE to skip
*    targval == address of return value  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targval == target value node
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    find_val_node_unique (val_value_t *startval,
                          ncx_module_t *mod,
                          const xmlChar *target,
                          boolean logerrors,
                          val_value_t **targval)
{
    val_value_t    *curval;
    ncx_module_t   *usemod;
    const xmlChar  *str;
    xmlChar        *prefix;
    xmlChar        *name;
    uint32          len;
    status_t        res;

    prefix = NULL;
    name = NULL;

    /* skip the first fwd slash, if any */
    if (*target == '/') {
        str = ++target;
    } else {
        str = target;
    }

    /* get the first QName (prefix, name) */
    res = next_val_nodeid(str, logerrors, &prefix, &name, &len);
    if (res != NO_ERR) {
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        str += len;
    }

    res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
    if (res != NO_ERR) {
        if (prefix) {
            if (logerrors) {
                log_error("\nError: module not found for prefix %s"
                          " in Xpath target %s",
                          prefix, target);
            }
            m__free(prefix);
        } else {
            if (logerrors) {
                log_error("\nError: no module prefix specified"
                          " in Xpath target %s", target);
            }
        }
        if (name) {
            m__free(name);
        }
        return ERR_NCX_MOD_NOT_FOUND;
    }

    /* get the first value node */
    curval = val_find_child(startval, usemod->name, name);
    if (!curval) {
        if (ncx_valid_name2(name)) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
        if (logerrors) {
            log_error("\nError: value node '%s' not found for module %s"
                      " in Xpath target %s",
                      name, usemod->name, target);
        }
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    }

    if (prefix) {
        m__free(prefix);
        prefix = NULL;
    }
    if (name) {
        m__free(name);
        name = NULL;
    }
    
    /* got the first object; keep parsing node IDs
     * until the Xpath expression is done or an error occurs
     */
    while (*str == '/') {
        str++;
        /* get the next QName (prefix, name) */
        res = next_val_nodeid(str, logerrors, &prefix, &name, &len);
        if (res != NO_ERR) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        } else {
            str += len;
        }

        res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
        if (res != NO_ERR) {
            if (prefix) {
                if (logerrors) {
                    log_error("\nError: module not found for prefix %s"
                              " in Xpath target %s",
                              prefix, target);
                }
                m__free(prefix);
            } else {
                if (logerrors) {
                    log_error("\nError: no module prefix specified"
                              " in Xpath target %s", target);
                }
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_MOD_NOT_FOUND;
        }

        /* determine 'nextval' based on [curval, prefix, name] */
        switch (curval->obj->objtype) {
        case OBJ_TYP_CONTAINER:
        case OBJ_TYP_LIST:
        case OBJ_TYP_CHOICE:
        case OBJ_TYP_CASE:
        case OBJ_TYP_RPC:
        case OBJ_TYP_RPCIO:
        case OBJ_TYP_NOTIF:
            curval = val_find_child(curval, usemod->name, name);
            if (!curval) {
                if (ncx_valid_name2(name)) {
                    res = ERR_NCX_DEF_NOT_FOUND;
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
                if (logerrors) {
                    log_error("\nError: value node '%s' not found for module %s"
                              " in Xpath target %s",
                              (name) ? name : NCX_EL_NONE, 
                              usemod->name, 
                              target);
                }
                if (prefix) {
                    m__free(prefix);
                }
                if (name) {
                    m__free(name);
                }
                return res;
            }
            break;
        case OBJ_TYP_LEAF:
        case OBJ_TYP_LEAF_LIST:
            res = ERR_NCX_DEFSEG_NOT_FOUND;
            if (logerrors) {
                log_error("\nError: '%s' in Xpath target '%s' invalid: "
                          "%s is a %s",
                          (name) ? name : NCX_EL_NONE, 
                          target, 
                          curval->name,
                          obj_get_typestr(curval->obj));
            }
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            if (logerrors) {
                do_errmsg(NULL, mod, NULL, res);
            }
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        }

        if (prefix) {
            m__free(prefix);
            prefix = NULL;
        }
        if (name) {
            m__free(name);
            name = NULL;
        }
    }

    if (prefix) {
        m__free(prefix);
    }
    if (name) {
        m__free(name);
    }

    if (targval) {
        *targval = curval;
    }
    return NO_ERR;

}  /* find_val_node_unique */
#endif


/********************************************************************
* FUNCTION find_obj_node_unique
* 
* Follow the relative-path schema-nodeid expression
* and return the object node that it indicates, if any
* Used in the YANG unique-stmt component
*
*  [/] foo/bar/baz
*
* No predicates are expected, but choice and case nodes
* are expected and will be accounted for if present
*
* XML mode - unique-smmt processing
* check first before logging errors;
*
* If logerrors:
*   Error messages are printed by this function!!
*   Do not duplicate error messages upon error return
*
* INPUTS:
*    startobj == object node to start search
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == relative path expr to evaluate
*    logerrors == TRUE to log_error, FALSE to skip
*    targobj == address of return value  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object node
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    find_obj_node_unique (obj_template_t *startobj,
                          ncx_module_t *mod,
                          const xmlChar *target,
                          boolean logerrors,
                          obj_template_t **targobj)
{
    ncx_module_t *usemod = NULL;
    const xmlChar *str = NULL;
    xmlChar *prefix = NULL;
    xmlChar *name = NULL;
    uint32 len = 0;

    /* skip the first fwd slash, if any */
    if (*target == '/') {
        str = ++target;
    } else {
        str = target;
    }

    /* get the first QName (prefix, name) */
    status_t res = next_val_nodeid(str, logerrors, &prefix, &name, &len);
    if (res != NO_ERR) {
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    } else {
        str += len;
    }

    res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
    if (res != NO_ERR) {
        if (prefix) {
            if (logerrors) {
                log_error("\nError: module not found for prefix %s"
                          " in Xpath target %s",
                          prefix, target);
            }
            m__free(prefix);
        } else {
            if (logerrors) {
                log_error("\nError: no module prefix specified"
                          " in Xpath target %s", target);
            }
        }
        if (name) {
            m__free(name);
        }
        return ERR_NCX_MOD_NOT_FOUND;
    }

    /* get the first value node */
    obj_template_t *curobj = obj_find_child(startobj, usemod->name, name);
    if (!curobj) {
        if (ncx_valid_name2(name)) {
            res = ERR_NCX_DEF_NOT_FOUND;
        } else {
            res = ERR_NCX_INVALID_NAME;
        }
        if (logerrors) {
            log_error("\nError: value node '%s' not found for module %s"
                      " in Xpath target %s",
                      name, usemod->name, target);
        }
        if (prefix) {
            m__free(prefix);
        }
        if (name) {
            m__free(name);
        }
        return res;
    }

    if (prefix) {
        m__free(prefix);
        prefix = NULL;
    }
    if (name) {
        m__free(name);
        name = NULL;
    }
    
    /* got the first object; keep parsing node IDs
     * until the Xpath expression is done or an error occurs
     */
    while (*str == '/') {
        str++;
        /* get the next QName (prefix, name) */
        res = next_val_nodeid(str, logerrors, &prefix, &name, &len);
        if (res != NO_ERR) {
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        } else {
            str += len;
        }

        res = xpath_get_curmod_from_prefix(prefix, mod, &usemod);
        if (res != NO_ERR) {
            if (prefix) {
                if (logerrors) {
                    log_error("\nError: module not found for prefix %s"
                              " in Xpath target %s",
                              prefix, target);
                }
                m__free(prefix);
            } else {
                if (logerrors) {
                    log_error("\nError: no module prefix specified"
                              " in Xpath target %s", target);
                }
            }
            if (name) {
                m__free(name);
            }
            return ERR_NCX_MOD_NOT_FOUND;
        }

        /* determine 'nextval' based on [curval, prefix, name] */
        switch (curobj->objtype) {
        case OBJ_TYP_CONTAINER:
        case OBJ_TYP_LIST:
        case OBJ_TYP_CHOICE:
        case OBJ_TYP_CASE:
        case OBJ_TYP_RPC:
        case OBJ_TYP_RPCIO:
        case OBJ_TYP_NOTIF:
            curobj = obj_find_child(curobj, usemod->name, name);
            if (!curobj) {
                if (ncx_valid_name2(name)) {
                    res = ERR_NCX_DEF_NOT_FOUND;
                } else {
                    res = ERR_NCX_INVALID_NAME;
                }
                if (logerrors) {
                    log_error("\nError: value node '%s' not found for module %s"
                              " in Xpath target %s",
                              (name) ? name : NCX_EL_NONE, 
                              usemod->name, 
                              target);
                }
                if (prefix) {
                    m__free(prefix);
                }
                if (name) {
                    m__free(name);
                }
                return res;
            }
            break;
        case OBJ_TYP_LEAF:
        case OBJ_TYP_LEAF_LIST:
            res = ERR_NCX_DEFSEG_NOT_FOUND;
            if (logerrors) {
                log_error("\nError: '%s' in Xpath target '%s' invalid: "
                          "%s is a %s",
                          (name) ? name : NCX_EL_NONE, 
                          target, 
                          obj_get_name(curobj),
                          obj_get_typestr(curobj));
            }
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            if (logerrors) {
                do_errmsg(NULL, mod, NULL, res);
            }
            if (prefix) {
                m__free(prefix);
            }
            if (name) {
                m__free(name);
            }
            return res;
        }

        if (prefix) {
            m__free(prefix);
            prefix = NULL;
        }
        if (name) {
            m__free(name);
            name = NULL;
        }
    }

    if (prefix) {
        m__free(prefix);
    }
    if (name) {
        m__free(name);
    }

    if (targobj) {
        *targobj = curobj;
    }
    return NO_ERR;

}  /* find_obj_node_unique */


/********************************************************************
* FUNCTION decode_url_string
* 
* Fill buffer with a plain string from a URL string
*
* INPUTS:
*    urlstr == string to convert
*    urlstrlen == number of bytes to check
*    buffer == buffer to fill; may be NULL to get count
*    cnt == address of return cnt
*
* OUTPUTS:
*   if non-NULL inputs:
*      *cnt = number bytes required
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    decode_url_string (const xmlChar *urlstr,
                       uint32 urlstrlen,
                       xmlChar *buffer,
                       uint32 *cnt)
{
    const xmlChar   *instr;
    xmlChar         *writeptr, ch, numbuff[4];
    uint32           inlen, outlen;
    boolean          done;
    ncx_num_t        num;
    status_t         res;

    *cnt = 0;
    instr = urlstr;
    writeptr = buffer;
    inlen = 0;
    outlen = 0;
    ncx_init_num(&num);

    /* normal case, there is some top-level node next */
    done = FALSE;
    while (!done) {
        /* check end of input string */
        if (inlen == urlstrlen) {
            if (writeptr != NULL) {
                *writeptr = 0;
            }
            done = TRUE;
            continue;
        }
        ch = 0;
        if (*instr == '%') {
            /* hex encoded char */
            instr++;
            inlen++;
            if (inlen + 2 <= urlstrlen) {
                numbuff[0] = instr[0];
                numbuff[1] = instr[1];
                numbuff[2] = 0;
                res = ncx_convert_num(numbuff,
                                      NCX_NF_HEX,
                                      NCX_BT_UINT8,
                                      &num);
                if (res != NO_ERR) {
                    ncx_clean_num(NCX_BT_UINT8, &num);
                    return res;
                }
                ch = (xmlChar)num.u;
                ncx_clean_num(NCX_BT_UINT8, &num);
                instr += 2;
                inlen += 2;
            } else {
                /* invalid escaped char found */
                return ERR_NCX_INVALID_VALUE;
            }
        } else {
            /* normal char */
            ch = *instr++;
            inlen++;
        }

        if (ch == 0) {
            return ERR_NCX_INVALID_VALUE;
        } else {
            outlen++;
            if (writeptr != NULL) {
                *writeptr++ = ch;
            }
        }
    }
    
    *cnt = outlen;
    return NO_ERR;

}  /* decode_url_string */


/********************************************************************
* FUNCTION decode_url_esc_string
* 
* Fill buffer with a plain string from a
* yangcli escaped URL content string
*
* INPUTS:
*    urlstr == string to convert, must be 0-terminated
*    buffer == buffer to fill; may be NULL to get count
*    incnt == address of return cnt of bytes used in urlstr
*    outcnt == address of return cnt of bytes written to buffer
*
* OUTPUTS:
*   if non-NULL inputs:
*      *incnt = number bytes used in urlstr
*      *outcnt = number bytes required for buffer write
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    decode_url_esc_string (const xmlChar *urlstr,
                           xmlChar *buffer,
                           uint32 *incnt,
                           uint32 *outcnt)
{
    const xmlChar   *instr;
    xmlChar         *writeptr, ch, numbuff[4];
    uint32           outlen;
    boolean          done;
    ncx_num_t        num;
    status_t         res;

    *incnt = 0;
    *outcnt = 0;
    instr = urlstr;
    writeptr = buffer;
    outlen = 0;
    done = FALSE;
    ncx_init_num(&num);
    res = NO_ERR;

    /* normal case, there is some top-level node next */
    while (!done) {
        if (*instr == 0) {
            done = TRUE;
            continue;
        }

        ch = 0;

        /* check end of input string */
        if (*instr == '/' && instr[1] != '/') {
            /* reached the end of the content string */
            if (writeptr != NULL) {
                *writeptr = 0;
            }
            done = TRUE;
            continue;
        } else if (*instr == '/' && instr[1] == '/') {
            /* found an escaped forward slash */
            ch = '/';
            instr += 2;
        } else if (*instr == '%') {
            /* hex encoded char */
            instr++;
            if (instr[0] && instr[1]) {
                numbuff[0] = instr[0];
                numbuff[1] = instr[1];
                numbuff[2] = 0;
                res = ncx_convert_num(numbuff,
                                      NCX_NF_HEX,
                                      NCX_BT_UINT8,
                                      &num);
                if (res != NO_ERR) {
                    ncx_clean_num(NCX_BT_UINT8, &num);
                    return res;
                }
                ch = (xmlChar)num.u;
                ncx_clean_num(NCX_BT_UINT8, &num);
                instr += 2;
            } else {
                /* invalid escaped char found */
                return ERR_NCX_INVALID_VALUE;
            }
        } else {
            /* normal char */
            ch = *instr++;
        }

        if (ch == 0) {
            return ERR_NCX_INVALID_VALUE;
        } else {
            outlen++;
            if (writeptr != NULL) {
                *writeptr++ = ch;
            }
        }
    }

    *incnt = (uint32)(instr - urlstr);
    *outcnt = outlen;
    return NO_ERR;

}  /* decode_url_esc_string */


/********************************************************************
* FUNCTION fill_xpath_string
* 
* Fill buffer with an XPath string from a URL string
*
* INPUTS:
*    urlpath == string to convert
*    buff == buffer to fill; may be NULL to get count
*    match_names == enum for selected match names mode
*    alt_naming == TRUE if alt-name and cli-drop-node-name
*      containers should be checked
*    wildcards == TRUE if 'skip key wildcard' allowed
*                 FALSE if not allowed
*    withkeys == TRUE if keys are expected
*                FALSE if just nodes are expected
*    cnt == address of return cnt
*
* OUTPUTS:
*   if non-NULL inputs:
*      *cnt = number bytes required
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    fill_xpath_string (const xmlChar *urlpath,
                       xmlChar *buffer,
                       ncx_name_match_t match_names,
                       boolean alt_naming,
                       boolean wildcards,
                       boolean withkeys,
                       uint32 *cnt)
{
    const xmlChar   *startstr, *p;
    obj_template_t  *targobj;
    obj_key_t       *objkey;
    xmlChar         *writeptr, *tempbuff, *usebuff;
    uint32           inlen, outlen, outtotal;
    boolean          done, expectnode;
    status_t         res;
    xmlChar          namebuff[MAX_FILL+1];

    *cnt = 0;

    if (*urlpath != '/') {
        return ERR_NCX_INVALID_VALUE;
    }

    startstr = urlpath+1;

    /* check special case; path is just docroot */
    if (*startstr == 0) {
        if (buffer != NULL) {
            buffer[0] = '/';
            buffer[1] = 0;
        }
        *cnt = 1;
        return NO_ERR;
    }

    res = NO_ERR;
    targobj = NULL;
    expectnode = TRUE;
    inlen = 0;
    outlen = 0;
    objkey = NULL;
    tempbuff = NULL;
    usebuff = NULL;

    /* account for output of starting '/' */
    outtotal = 1;
    writeptr = buffer;
    if (writeptr != NULL) {
        *writeptr++ = '/';
    }

    /* check corner-case: URL starts with // - not allowed */
    if (*startstr == '/') {
        return ERR_NCX_INVALID_VALUE;
    }

    /* keep getting nodes until string ends or error out */
    done = FALSE;
    while (!done) {
        /* get the next string token to process */
        if (expectnode) {
            /* get the chars that represent the content
             * in this mode just skip to the end or the next '/'
             */
            p = startstr;
            while (*p != 0 && *p != '/') {
                p++;
            }
            inlen = (uint32)(p - startstr);
            if (inlen == 0) {
                /* invalid URL probably out of synch
                 * empty identifier string
                 */
                return ERR_NCX_INVALID_VALUE;
            }

            /* check corner-case wildcard in wrong place */
            if (inlen == 1 && *startstr == XP_URL_ESC_WILDCARD) {
                return ERR_NCX_INVALID_VALUE;
            }

            /* got some string content; convert to plain string */
            if (inlen >= MAX_FILL) {
                tempbuff = m__getMem(inlen + 1);
                if (tempbuff == NULL) {
                    return ERR_INTERNAL_MEM;
                }
                usebuff = tempbuff;
            } else {
                usebuff = namebuff;
            }
            
            /* get the identifier into the name buffer first */
            outlen = 0;
            res = decode_url_string(startstr,
                                    inlen,
                                    usebuff,
                                    &outlen);
            if (res == NO_ERR) {
                const xmlChar *testname;

                /* get the target database object to use */
                if (targobj == NULL) {
                    /* find a top-level data node object */
                    targobj = ncx_match_any_object(usebuff, 
                                                   match_names,
                                                   alt_naming,
                                                   &res);
                } else {
                    targobj = obj_find_child_ex(targobj, 
                                                NULL,   /* match any module */
                                                usebuff,
                                                match_names,
                                                alt_naming,
                                                TRUE,  /* dataonly */
                                                &res);
                }
                if (targobj != NULL) {
                    testname = obj_get_name(targobj);
                    if (writeptr != NULL) {
                        writeptr += xml_strcpy(writeptr, testname);
                    }
                    outtotal += xml_strlen(testname);
                }
            }

            /* cleanup temp buff */
            if (tempbuff != NULL) {
                m__free(tempbuff);
                tempbuff = NULL;
            }

            /* check error exit */
            if (targobj == NULL || res != NO_ERR) {
                if (res == NO_ERR) {
                    return ERR_NCX_INVALID_VALUE;
                } else {
                    return res;
                }
            }

            /* skip to the rpc input node if needed
             * this code does not yet support
             * custom RPC operations; just data tree access
             * this code will not be reached for a custom RPC
             * if dataonly is set to TRUE above
             */
            if (targobj->objtype == OBJ_TYP_RPC) {
                targobj = obj_find_child(targobj, 
                                         NULL, 
                                         YANG_K_INPUT);
                if (targobj == NULL) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                }
            }

            objkey = NULL;
            if (withkeys) {
                /* check list keys to follow */
                objkey = obj_first_key(targobj);
                if (objkey != NULL) {
                    expectnode = FALSE;
                }
            }
            if (objkey == NULL) {
                if (*p == '/') {
                    outtotal++;
                    if (writeptr != NULL) {
                        *writeptr++ = '/';
                    }
                }
            }
        } else {
            /* expecting a key value */
            const xmlChar *namestr = obj_get_name(objkey->keyobj);

            if (namestr == NULL) {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }

            /* check if this key is being skipped */
            if (startstr[0] == XP_URL_ESC_WILDCARD &&
                startstr[1] == '/' &&
                startstr[2] != '/') {
                if (wildcards) {
                    /* skip this key in the output */
                    p = &startstr[1];
                } else {
                    return ERR_NCX_INVALID_VALUE;
                }
            } else {
                /* treat whatever string that is present
                 * as a key content node 
                 * account for [foo='val'] predicate
                 * first the [foo=' part
                 */
                outtotal += (1 + xml_strlen(namestr) + 2);
                if (writeptr != NULL) {
                    *writeptr++ = '[';
                    writeptr += xml_strcpy(writeptr, namestr);
                    *writeptr++ = '=';
                    *writeptr++ = '\'';
                }

                /* get the decoded value into the real buffer if used */
                inlen = 0;
                outlen = 0;
                res = decode_url_esc_string(startstr,
                                            writeptr,
                                            &inlen,
                                            &outlen);
                if (res != NO_ERR) {
                    return res;
                }
                outtotal += outlen;
                if (writeptr != NULL) {
                    writeptr += outlen;
                }

                /* account for the '] part */
                outtotal += 2;
                if (writeptr != NULL) {
                    *writeptr++ = '\'';
                    *writeptr++ = ']';
                }
                p = &startstr[inlen];
            }

            /* set up next key if any 
             * account for next '/' if needed
             */
            objkey = obj_next_key(objkey);
            if (objkey == NULL) {
                expectnode = TRUE;

                if (*p == '/') {
                    outtotal++;
                    if (writeptr != NULL) {
                        *writeptr++ = '/';
                    }
                }
            }
        }

        /* input string is either done or pointing at next '/' char */
        if (*p == 0) {
            done = TRUE;
        } else {
            /* skip '/' input char */
            startstr = ++p;
        }
    }

    if (writeptr != NULL) {
        *writeptr = 0;
    }

    *cnt = outtotal;
    return NO_ERR;

}  /* fill_xpath_string */


/************    E X T E R N A L   F U N C T I O N S    ************/


/*********    S C H E M A  N O D E  I D    S U P P O R T  ***********/


/********************************************************************
* FUNCTION xpath_find_schema_target
* 
* find target, save in *targobj
* Follow the absolute-path or descendant-node path expression
* and return the obj_template_t that it indicates, and the
* que that the object is in
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block to use
*    tkc == token chain in progress  (may be NULL: errmsg only)
*    mod == module in progress
*    obj == augment object initiating search, NULL to start at top
*    datadefQ == Q of obj_template_t containing 'obj'
*    target == Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*    targQ == address of return target queue (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*      *targQ == datadefQ Q header which contains targobj
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_find_schema_target (yang_pcb_t *pcb,
                              tk_chain_t *tkc,
                              ncx_module_t *mod,
                              obj_template_t *obj,
                              dlq_hdr_t  *datadefQ,
                              const xmlChar *target,
                              obj_template_t **targobj,
                              dlq_hdr_t **targQ)
{
    return xpath_find_schema_target_err(pcb,
                                        tkc, 
                                        mod, 
                                        obj, 
                                        datadefQ, 
                                        target, 
                                        targobj, 
                                        targQ, 
                                        NULL);

}  /* xpath_find_schema_target */


/********************************************************************
* FUNCTION xpath_find_schema_target_err
* 
* find target, save in *targobj, use the errtk if error
* Same as xpath_find_schema_target except a token struct
* is provided to use for the error token, instead of 'obj'
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    tkc == token chain in progress (may be NULL: errmsg only)
*    mod == module in progress
*    obj == augment object initiating search, NULL to start at top
*    datadefQ == Q of obj_template_t containing 'obj'
*    target == Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*    targQ == address of return target queue (may be NULL)
*    tkerr == error struct to use if any messages generated
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*      *targQ == datadefQ header for targobj
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_find_schema_target_err (yang_pcb_t *pcb,
                                  tk_chain_t *tkc,
                                  ncx_module_t *mod,
                                  obj_template_t *obj,
                                  dlq_hdr_t  *datadefQ,
                                  const xmlChar *target,
                                  obj_template_t **targobj,
                                  dlq_hdr_t **targQ,
                                  ncx_error_t *tkerr)
{
    status_t    res;

#ifdef DEBUG
    if (!mod || !datadefQ || !target) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (*target == '/') {
        /* check error: if nested object is using an abs. Xpath */
        if (obj && obj->parent && !obj_is_root(obj->parent)) {
            log_error("\nError: Absolute Xpath expression not "
                      "allowed here (%s)", target);
            res = ERR_NCX_INVALID_VALUE;
            do_errmsg(tkc, mod, tkerr ? tkerr : &obj->tkerr, res);
            return res;
        }
    }

    res = find_schema_node(pcb,
                           tkc, 
                           mod, 
                           obj, 
                           datadefQ,
                           target, 
                           targobj,
                           targQ, 
                           tkerr ? tkerr : (obj ? &obj->tkerr : NULL));
    return res;

}  /* xpath_find_schema_target_err */


/********************************************************************
* FUNCTION xpath_find_schema_target_int
* 
* internal find target, without any error reporting
* Follow the absolute-path expression
* and return the obj_template_t that it indicates
*
* Internal access version
* Error messages are not printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    target == absolute Xpath expression string to evaluate
*    targobj == address of return object  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs:
*      *targobj == target object
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_find_schema_target_int (const xmlChar *target,
                                  obj_template_t **targobj)
{
#ifdef DEBUG
    if (!target) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return find_schema_node_int(target, targobj);

}  /* xpath_find_schema_target_int */


/********************************************************************
* FUNCTION xpath_find_val_target
* 
* used by cfg.c to find parms in the value struct for
* a config file (ncx:cli)
*
* Follow the absolute-path Xpath expression as used
* internally to identify a config DB node
* and return the val_value_t that it indicates
*
* Expression must be the node-path from root for
* the desired node.
*
* Error messages are logged by this function
*
* INPUTS:
*    startval == top-level start element to search
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == Xpath expression string to evaluate
*    targval == address of return value  (may be NULL)
*
* OUTPUTS:
*   if non-NULL inputs and value node found:
*      *targval == target value node
*   If non-NULL targval and error exit:
*      *targval == last good node visited in expression (if any)
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_find_val_target (val_value_t *startval,
                           ncx_module_t *mod,
                           const xmlChar *target,
                           val_value_t **targval)
{

#ifdef DEBUG
    if (!startval || !target) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return find_val_node(startval, mod, target, targval);

}  /* xpath_find_val_target */


/********************************************************************
* FUNCTION xpath_find_val_unique
* 
* called by server to find a descendant value node
* based  on a relative-path sub-clause of a unique-stmt
*
* Follow the relative-path Xpath expression as used
* internally to identify a config DB node
* and return the val_value_t that it indicates
*
* Error messages are logged by this function
* only if logerrors is TRUE
*
* INPUTS:
*    startval == starting context node (contains unique-stmt)
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    target == Xpath expression string to evaluate
*    root = XPath docroot to use
*    logerrors == TRUE to use log_error, FALSE to skip it
*    retpcb == address of return value
*
* OUTPUTS:
*   if value node found:
*      *retpcb == malloced XPath PCB with result
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_find_val_unique (val_value_t *startval,
                           ncx_module_t *mod,
                           const xmlChar *target,
                           val_value_t *root,
                           boolean logerrors,
                           xpath_pcb_t **retpcb)
{

    assert( startval && "startval is NULL!" );
    assert( target && "target is NULL!" );
    assert( root && "root is NULL!" );
    assert( retpcb && "retpcb is NULL!" );

    *retpcb = NULL;

    /* normalize the target by finding the object node and
     * generating an object ID string; the prefixes in
     * the node-identifier are relative to the module
     * containing the unique-stmt
     */
    obj_template_t *targobj = NULL;
    status_t res = find_obj_node_unique(startval->obj, mod, target,
                                        logerrors, &targobj);
    if (res != NO_ERR) {
        return res;
    }
    if (targobj == NULL) {
        return ERR_NCX_OPERATION_FAILED;
    }

    xpath_pcb_t *pcb = xpath_new_pcb(NULL, NULL);
    if (pcb == NULL) {
        return ERR_INTERNAL_MEM;
    }

    res = obj_gen_object_id_unique(targobj, startval->obj, &pcb->exprstr);
    if (res != NO_ERR) {
        xpath_free_pcb(pcb);
        return res;
    }

    pcb->result = xpath1_eval_expr(pcb, startval, root, logerrors, FALSE, &res);
    if (pcb->result == NULL || res != NO_ERR) {
        xpath_free_pcb(pcb);
        if (res == NO_ERR) {
            res = ERR_NCX_OPERATION_FAILED;
        }
    } else {
        *retpcb = pcb;
    }
    return res;

}  /* xpath_find_val_unique */


/*******    X P A T H   and   K E Y R E F    S U P P O R T   *******/


/********************************************************************
* FUNCTION xpath_new_pcb
* 
* malloc a new XPath parser control block
* xpathstr is allowed to be NULL, otherwise
* a strdup will be made and exprstr will be set
*
* Create and initialize an XPath parser control block
*
* INPUTS:
*   xpathstr == XPath expression string to save (a copy will be made)
*            == NULL if this step should be skipped
*   getvar_fn == callback function to retirieve an XPath
*                variable binding
*                NULL if no variables are used
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
xpath_pcb_t *
    xpath_new_pcb (const xmlChar *xpathstr,
                   xpath_getvar_fn_t  getvar_fn)
{

    return xpath_new_pcb_ex(xpathstr, getvar_fn, NULL);

}  /* xpath_new_pcb */


/********************************************************************
* FUNCTION xpath_new_pcb_ex
* 
* malloc a new XPath parser control block
* xpathstr is allowed to be NULL, otherwise
* a strdup will be made and exprstr will be set
*
* Create and initialize an XPath parser control block
*
* INPUTS:
*   xpathstr == XPath expression string to save (a copy will be made)
*            == NULL if this step should be skipped
*   getvar_fn == callback function to retirieve an XPath
*                variable binding
*                NULL if no variables are used
*   cookie == runstack context pointer to use, cast as a cookie
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
xpath_pcb_t *
    xpath_new_pcb_ex (const xmlChar *xpathstr,
                      xpath_getvar_fn_t  getvar_fn,
                      void *cookie)
{
    xpath_pcb_t *pcb;

    pcb = m__getObj(xpath_pcb_t);
    if (!pcb) {
        return NULL;
    }

    memset(pcb, 0x0, sizeof(xpath_pcb_t));

    if (xpathstr) {
        pcb->exprstr = xml_strdup(xpathstr);
        if (!pcb->exprstr) {
            m__free(pcb);
            return NULL;
        }
    }

    ncx_init_errinfo(&pcb->errinfo);

    pcb->functions = xpath1_get_functions_ptr();
    pcb->getvar_fn = getvar_fn;
    pcb->cookie = cookie;
    pcb->missing_errors = TRUE;

    dlq_createSQue(&pcb->result_cacheQ);
    dlq_createSQue(&pcb->resnode_cacheQ);
    dlq_createSQue(&pcb->varbindQ);

    return pcb;

}  /* xpath_new_pcb_ex */


/********************************************************************
* FUNCTION xpath_clone_pcb
* 
* Clone an XPath PCB for a  must clause copy
* copy from typdef to object for leafref
* of object to value for NETCONF PDU processing
*
* INPUTS:
*    srcpcb == struct with starting contents
*
* RETURNS:
*   new xpatyh_pcb_t clone of the srcmust, NULL if malloc error
*   It will not be processed or parsed.  Only the starter
*   data will be set
*********************************************************************/
xpath_pcb_t *
    xpath_clone_pcb (const xpath_pcb_t *srcpcb)
{
    xpath_pcb_t *newpcb;
    status_t     res;

#ifdef DEBUG
    if (!srcpcb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    newpcb = xpath_new_pcb(srcpcb->exprstr, srcpcb->getvar_fn);
    if (!newpcb) {
        return NULL;
    }

    if (srcpcb->tkc) {
        newpcb->tkc = tk_clone_chain(srcpcb->tkc);
        if (!newpcb->tkc) {
            xpath_free_pcb(newpcb);
            return NULL;
        }
    }
    ncx_set_error(&newpcb->tkerr,
                  srcpcb->tkerr.mod,
                  srcpcb->tkerr.linenum,
                  srcpcb->tkerr.linepos);
    newpcb->reader = srcpcb->reader;
    newpcb->source = srcpcb->source;

    res = ncx_copy_errinfo(&srcpcb->errinfo, &newpcb->errinfo);
    if (res != NO_ERR) {
        xpath_free_pcb(newpcb);
        return NULL;
    }
    newpcb->logerrors = srcpcb->logerrors;
    newpcb->targobj = srcpcb->targobj;
    newpcb->altobj = srcpcb->altobj;
    newpcb->varobj = srcpcb->varobj;
    newpcb->curmode = srcpcb->curmode;
    newpcb->obj = srcpcb->obj;
    newpcb->objmod = srcpcb->objmod;
    newpcb->docroot = srcpcb->docroot;
    newpcb->doctype = srcpcb->doctype;
    newpcb->val = srcpcb->val;
    newpcb->val_docroot = srcpcb->val_docroot;
    newpcb->flags = srcpcb->flags;
    /*** skip copying the scratch result ***/
    /*** ??? context ??? ***/
    newpcb->functions = srcpcb->functions;
    /* result_cacheQ not copied */
    /* resnode_cacheQ not copied */
    /* result_count not copied */
    /* resnode_count not copied */
    newpcb->parseres = srcpcb->parseres;
    newpcb->validateres = srcpcb->validateres;
    newpcb->valueres = srcpcb->valueres;
    newpcb->seen = srcpcb->seen;

    /*** does the varbindQ need to be cloned?  ***/

    return newpcb;

}  /* xpath_clone_pcb */


/********************************************************************
* FUNCTION xpath_find_pcb
* 
* Find an XPath PCB
* find by exact match of the expressions string
*
* INPUTS:
*    pcbQ == Q of xpath_pcb_t structs to check
*    exprstr == XPath expression string to find
*
* RETURNS:
*   pointer to found xpath_pcb_t or NULL if not found
*********************************************************************/
xpath_pcb_t *
    xpath_find_pcb (dlq_hdr_t *pcbQ,
                    const xmlChar *exprstr)
{
    xpath_pcb_t *pcb;

#ifdef DEBUG
    if (!pcbQ || !exprstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (pcb = (xpath_pcb_t *)dlq_firstEntry(pcbQ);
         pcb != NULL;
         pcb = (xpath_pcb_t *)dlq_nextEntry(pcb)) {

        if (pcb->exprstr && 
            !xml_strcmp(exprstr, pcb->exprstr)) {
            return pcb;
        }
    }
    return NULL;

}  /* xpath_find_pcb */


/********************************************************************
 * Free a malloced XPath parser control block
 *
 * \param pcb pointer to parser control block to free
 *********************************************************************/
void xpath_free_pcb (xpath_pcb_t *pcb)
{
    xpath_result_t   *result;
    xpath_resnode_t  *resnode;

    if (!pcb) {
        return;
    }

    if (pcb->tkc) {
        tk_free_chain(pcb->tkc);
    }

    if (pcb->exprstr) {
        m__free(pcb->exprstr);
    }

    if (pcb->result) {
        xpath_free_result(pcb->result);
    }

    ncx_clean_errinfo(&pcb->errinfo);

    while (!dlq_empty(&pcb->result_cacheQ)) {
        result = (xpath_result_t *)
            dlq_deque(&pcb->result_cacheQ);
        xpath_free_result(result);
    }

    while (!dlq_empty(&pcb->resnode_cacheQ)) {
        resnode = (xpath_resnode_t *)
            dlq_deque(&pcb->resnode_cacheQ);
        xpath_free_resnode(resnode);
    }

    var_clean_varQ(&pcb->varbindQ);

    m__free(pcb);

}  /* xpath_free_pcb */


/********************************************************************
* FUNCTION xpath_new_result
* 
* malloc an XPath result
* Create and initialize an XPath result struct
*
* INPUTS:
*   restype == the desired result type
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
xpath_result_t *
    xpath_new_result (xpath_restype_t  restype)
{
    xpath_result_t *result;

    result = m__getObj(xpath_result_t);
    if (!result) {
        return NULL;
    }
    xpath_init_result(result, restype);
    return result;

}  /* xpath_new_result */


/********************************************************************
* FUNCTION xpath_init_result
* 
* Initialize an XPath result struct
* malloc an XPath result node
*
* INPUTS:
*   result == pointer to result struct to initialize
*   restype == the desired result type
*********************************************************************/
void 
    xpath_init_result (xpath_result_t *result,
                       xpath_restype_t  restype)
{
#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(result, 0x0, sizeof(xpath_result_t));
    result->restype = restype;

    switch (restype) {
    case XP_RT_NODESET:
        dlq_createSQue(&result->r.nodeQ);
        break;
    case XP_RT_NUMBER:
        ncx_init_num(&result->r.num);
        ncx_set_num_zero(&result->r.num, NCX_BT_FLOAT64);
        result->isval = TRUE;
        break;
    case XP_RT_STRING:
    case XP_RT_BOOLEAN:
        result->isval = TRUE;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* xpath_init_result */


/********************************************************************
* FUNCTION xpath_free_result
* 
* Free a malloced XPath result struct
*
* INPUTS:
*   result == pointer to result struct to free
*********************************************************************/
void xpath_free_result (xpath_result_t *result)
{
    if (!result) {
        return;
    }

    xpath_clean_result(result);
    m__free(result);

}  /* xpath_free_result */


/********************************************************************
* FUNCTION xpath_clean_result
* 
* Clean an XPath result struct
*
* INPUTS:
*   result == pointer to result struct to clean
*********************************************************************/
void
    xpath_clean_result (xpath_result_t *result)
{
    xpath_resnode_t *resnode;

#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (result->restype) {
    case XP_RT_NODESET:
        while (!dlq_empty(&result->r.nodeQ)) {
            resnode = (xpath_resnode_t *)dlq_deque(&result->r.nodeQ);
            xpath_free_resnode(resnode);
        }
        break;
    case XP_RT_NUMBER:
        ncx_clean_num(NCX_BT_FLOAT64, &result->r.num);
        break;
    case XP_RT_STRING:
        if (result->r.str) {
            m__free(result->r.str);
            result->r.str = NULL;
        }
        break;
    case XP_RT_BOOLEAN:
        result->r.boo = FALSE;
        break;
    case XP_RT_NONE:
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    result->restype = XP_RT_NONE;
    result->res = NO_ERR;

}  /* xpath_clean_result */


/********************************************************************
* FUNCTION xpath_new_resnode
* 
* Create and initialize an XPath result node struct
*
* INPUTS:
*   restype == the desired result type
*
* RETURNS:
*   pointer to malloced struct, NULL if malloc error
*********************************************************************/
xpath_resnode_t *
    xpath_new_resnode (void)
{
    xpath_resnode_t *resnode;

    resnode = m__getObj(xpath_resnode_t);
    if (!resnode) {
        return NULL;
    }
    
    xpath_init_resnode(resnode);
    return resnode;

}  /* xpath_new_resnode */


/********************************************************************
* FUNCTION xpath_init_resnode
* 
* Initialize an XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to initialize
*********************************************************************/
void 
    xpath_init_resnode (xpath_resnode_t *resnode)
{
#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(resnode, 0x0, sizeof(xpath_resnode_t));

}  /* xpath_init_resnode */


/********************************************************************
* FUNCTION xpath_free_resnode
* 
* Free a malloced XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to free
*********************************************************************/
void
    xpath_free_resnode (xpath_resnode_t *resnode)
{
#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    xpath_clean_resnode(resnode);
    m__free(resnode);

}  /* xpath_free_resnode */


/********************************************************************
* FUNCTION xpath_delete_resnode
* 
* Delete and free a malloced XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to free
*********************************************************************/
void
    xpath_delete_resnode (xpath_resnode_t *resnode)
{
#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    dlq_remove(resnode);
    xpath_clean_resnode(resnode);
    m__free(resnode);

}  /* xpath_delete_resnode */


/********************************************************************
* FUNCTION xpath_clean_resnode
* 
* Clean an XPath result node struct
*
* INPUTS:
*   resnode == pointer to result node struct to clean
*********************************************************************/
void
    xpath_clean_resnode (xpath_resnode_t *resnode)
{

#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    memset(resnode, 0x0, sizeof(xpath_resnode_t));

}  /* xpath_clean_resnode */


/********************************************************************
* FUNCTION xpath_get_curmod_from_prefix
* 
* Get the correct module to use for a given prefix
*
* INPUTS:
*    prefix == string to check
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    targmod == address of return module
*
* OUTPUTS:
*    *targmod == target moduke to use
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_get_curmod_from_prefix (const xmlChar *prefix,
                                  ncx_module_t *mod,
                                  ncx_module_t **targmod)
{
    ncx_import_t   *imp;
    status_t        res;
    
#ifdef DEBUG
    if (!targmod) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    /* get the import if there is a real prefix entered */
    if (prefix && *prefix) {
        if (!mod) {
            *targmod = (ncx_module_t *)xmlns_get_modptr
                (xmlns_find_ns_by_prefix(prefix));
            if (!*targmod) {
                res = ERR_NCX_MOD_NOT_FOUND;
            }
        } else if (xml_strcmp(prefix, mod->prefix)) {
            imp = ncx_find_pre_import(mod, prefix);
            if (!imp) {
                res = ERR_NCX_INVALID_NAME;
            } else {
                if (imp->mod) {
                    *targmod = imp->mod;
                } else {
                    *targmod = ncx_find_module(imp->module, imp->revision);
                }
                if (!*targmod) {
                    res = ERR_NCX_MOD_NOT_FOUND;
                } else {
                    imp->mod = *targmod;
                }
            }
        } else {
            *targmod = mod;
            res = NO_ERR;
        }
    } else if (mod) {
        *targmod = mod;
        res = NO_ERR;
    } else {
        *targmod = NULL;
        res = ERR_NCX_DATA_MISSING;
    }
    return res;

}   /* xpath_get_curmod_from_prefix */


/********************************************************************
* FUNCTION xpath_get_curmod_from_prefix_str
* 
* Get the correct module to use for a given prefix
* Unended string version
*
* INPUTS:
*    prefix == string to check
*    prefixlen == length of prefix
*    mod == module to use for the default context
*           and prefixes will be relative to this module's
*           import statements.
*        == NULL and the default registered prefixes
*           will be used
*    targmod == address of return module
*
* OUTPUTS:
*    *targmod == target moduke to use
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_get_curmod_from_prefix_str (const xmlChar *prefix,
                                      uint32 prefixlen,
                                      ncx_module_t *mod,
                                      ncx_module_t **targmod)
{
    xmlChar        *buff;
    status_t        res;

#ifdef DEBUG
    if (!targmod) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (prefix && prefixlen) {
        buff = m__getMem(prefixlen+1);
        if (!buff) {
            return ERR_INTERNAL_MEM;
        }
        xml_strncpy(buff, prefix, prefixlen);

        res = xpath_get_curmod_from_prefix(buff, mod, targmod);
        
        m__free(buff);

        return res;
    } else {
        return xpath_get_curmod_from_prefix(NULL, mod, targmod);
    }
    /*NOTREACHED*/

}   /* xpath_get_curmod_from_prefix_str */


/********************************************************************
* FUNCTION xpath_parse_token
* 
* Parse the XPath token sequence for a specific token type
* It has already been tokenized
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*    pcb == parser control block in progress
*    tktyp == expected token type
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_parse_token (xpath_pcb_t *pcb,
                       tk_type_t  tktype)
{
    status_t     res;

#ifdef DEBUG
    if (!pcb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* get the next token */
    res = TK_ADV(pcb->tkc);
    if (res != NO_ERR) {
        if (pcb->logerrors) {
            ncx_print_errormsg(pcb->tkc, pcb->tkerr.mod, res);
        }
        return res;
    }

    if (TK_CUR_TYP(pcb->tkc) != tktype) {
        res = ERR_NCX_WRONG_TKTYPE;
        if (pcb->logerrors) {
            ncx_mod_exp_err(pcb->tkc, 
                            pcb->tkerr.mod, 
                            res,
                            tk_get_token_name(tktype));
        }
        return res;
    }

    return NO_ERR;

}  /* xpath_parse_token */


/********************************************************************
* FUNCTION xpath_cvt_boolean
* 
* Convert an XPath result to a boolean answer
*
* INPUTS:
*    result == result struct to convert to boolean
*
* RETURNS:
*   TRUE or FALSE depending on conversion
*********************************************************************/
boolean
    xpath_cvt_boolean (const xpath_result_t *result)
{
#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (result->restype) {
    case XP_RT_NONE:
        return FALSE;
    case XP_RT_NODESET:
        return (dlq_empty(&result->r.nodeQ)) ? FALSE : TRUE;
    case XP_RT_NUMBER:
        return (ncx_num_zero(&result->r.num, NCX_BT_FLOAT64)) ?
            FALSE : TRUE;
    case XP_RT_STRING:
        return (result->r.str && xml_strlen(result->r.str)) ?
            TRUE : FALSE;
    case XP_RT_BOOLEAN:
        return result->r.boo;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

}  /* xpath_cvt_boolean */


/********************************************************************
* FUNCTION xpath_cvt_number
* 
* Convert an XPath result to a number answer
*
* INPUTS:
*    result == result struct to convert to a number
*    num == pointer to ncx_num_t to hold the conversion result
*
* OUTPUTS:
*   *num == numeric result from conversion
*
*********************************************************************/
void
    xpath_cvt_number (const xpath_result_t *result,
                      ncx_num_t *num)
{
    const xpath_resnode_t   *resnode;
    val_value_t             *val;
    status_t                 res;
    ncx_num_t                testnum;
    ncx_numfmt_t             numformat;

#ifdef DEBUG
    if (!result || !num) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    res = NO_ERR;

    switch (result->restype) {
    case XP_RT_NONE:
        ncx_set_num_nan(num, NCX_BT_FLOAT64);
        break;
    case XP_RT_NODESET:
        if (dlq_empty(&result->r.nodeQ)) {
            ncx_set_num_nan(num, NCX_BT_FLOAT64);
        } else {
            if (result->isval) {
                resnode = (const xpath_resnode_t *)
                    dlq_firstEntry(&result->r.nodeQ);
                val = val_get_first_leaf(resnode->node.valptr);
                if (val && typ_is_number(val->btyp)) {
                    res = ncx_cast_num(&val->v.num,
                                       val->btyp,
                                       num,
                                       NCX_BT_FLOAT64);
                    if (res != NO_ERR) {
                        ncx_set_num_nan(num, NCX_BT_FLOAT64);
                    }
                } else if (val && val->btyp == NCX_BT_STRING) {
                    ncx_init_num(&testnum);
                    res = ncx_convert_num(result->r.str,
                                          NCX_NF_NONE,
                                          NCX_BT_FLOAT64,
                                          &testnum);
                    if (res == NO_ERR) {
                        (void)ncx_copy_num(&testnum, 
                                           num, 
                                           NCX_BT_FLOAT64);
                    } else {
                        ncx_set_num_nan(num, NCX_BT_FLOAT64);
                    }
                    ncx_clean_num(NCX_BT_FLOAT64, &testnum);
                } else {
                    ncx_set_num_nan(num, NCX_BT_FLOAT64);
                }
            } else {
                /* does not matter */
                ncx_set_num_zero(num, NCX_BT_FLOAT64);
            }
        }
        break;
    case XP_RT_NUMBER:
        ncx_copy_num(&result->r.num, num, NCX_BT_FLOAT64);
        break;
    case XP_RT_STRING:
        if (result->r.str) {
            numformat = ncx_get_numfmt(result->r.str);
            switch (numformat) {
            case NCX_NF_DEC:
            case NCX_NF_REAL:
                break;
            case NCX_NF_NONE:
            case NCX_NF_OCTAL:
            case NCX_NF_HEX:
                res = ERR_NCX_WRONG_NUMTYP;
                break;
            default:
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }

            if (res == NO_ERR) {
                ncx_init_num(&testnum);
                res = ncx_convert_num(result->r.str,
                                      numformat,
                                      NCX_BT_FLOAT64,
                                      &testnum);
                if (res == NO_ERR) {
                    (void)ncx_copy_num(&testnum, num, NCX_BT_FLOAT64);
                } else {
                    ncx_set_num_nan(num, NCX_BT_FLOAT64);
                }
                ncx_clean_num(NCX_BT_FLOAT64, &testnum);
            } else {
                ncx_set_num_nan(num, NCX_BT_FLOAT64);
            }
        } else {
            ncx_set_num_nan(num, NCX_BT_FLOAT64);
        }
        break;
    case XP_RT_BOOLEAN:
        if (result->r.boo) {
            ncx_set_num_one(num, NCX_BT_FLOAT64);
        } else {
            ncx_set_num_zero(num, NCX_BT_FLOAT64);
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* xpath_cvt_number */


/********************************************************************
* FUNCTION xpath_cvt_string
* 
* Convert an XPath result to a string answer
*
* INPUTS:
*    pcb == parser control block to use
*    result == result struct to convert to a number
*    str == pointer to xmlChar * to hold the conversion result
*
* OUTPUTS:
*   *str == pointer to malloced string from conversion
*
* RETURNS:
*   status; could get an ERR_INTERNAL_MEM error or NO_RER
*********************************************************************/
status_t
    xpath_cvt_string (xpath_pcb_t *pcb,
                      const xpath_result_t *result,
                      xmlChar **str)
{
    status_t                 res;
    uint32                   len;

#ifdef DEBUG
    if (!result || !str) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    *str = NULL;

    switch (result->restype) {
    case XP_RT_NONE:
        *str = xml_strdup(EMPTY_STRING);
        break;
    case XP_RT_NODESET:
        if (dlq_empty(&result->r.nodeQ)) {
            *str = xml_strdup(EMPTY_STRING);
        } else {
            if (result->isval) {
                res = xpath1_stringify_nodeset(pcb, result, str);
            } else {
                *str = xml_strdup(EMPTY_STRING);
            }
        }
        break;
    case XP_RT_NUMBER:
        res = ncx_sprintf_num(NULL, 
                              &result->r.num,
                              NCX_BT_FLOAT64,
                              &len);
        if (res != NO_ERR) {
            return res;
        }

        *str = m__getMem(len+1);
        if (*str) {
            res = ncx_sprintf_num(*str,
                                  &result->r.num,
                                  NCX_BT_FLOAT64,
                                  &len);
            if (res != NO_ERR) {
                m__free(*str);
                *str = NULL;
                return res;
            }
        }
        break;
    case XP_RT_STRING:
        if (result->r.str) {
            *str = xml_strdup(result->r.str);
        } else {
            *str = xml_strdup(EMPTY_STRING);
        }
        break;
    case XP_RT_BOOLEAN:
        if (result->r.boo) {
            *str = xml_strdup(NCX_EL_TRUE);
        } else {
            *str = xml_strdup(NCX_EL_FALSE);
        }
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (!*str) {
        res = ERR_INTERNAL_MEM;
    }
    return res;

}  /* xpath_cvt_string */


/********************************************************************
* FUNCTION xpath_get_resnodeQ
* 
* Get the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnodeQ or NULL if some error
*********************************************************************/
dlq_hdr_t *
    xpath_get_resnodeQ (xpath_result_t *result)
{
#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (result->restype != XP_RT_NODESET) {
        return NULL;
    }
    return &result->r.nodeQ;

}  /* xpath_get_resnodeQ */


/********************************************************************
* FUNCTION xpath_get_first_resnode
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
xpath_resnode_t *
    xpath_get_first_resnode (xpath_result_t *result)
{
#ifdef DEBUG
    if (!result) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (result->restype != XP_RT_NODESET) {
        return NULL;
    }

    return (xpath_resnode_t *)
        dlq_firstEntry(&result->r.nodeQ);

}  /* xpath_get_first_resnode */


/********************************************************************
* FUNCTION xpath_get_next_resnode
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
xpath_resnode_t *
    xpath_get_next_resnode (xpath_resnode_t *resnode)
{
#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (xpath_resnode_t *)dlq_nextEntry(resnode);

}  /* xpath_get_next_resnode */


/********************************************************************
* FUNCTION xpath_get_resnode_valptr
* 
* Get the first result in the renodeQ from a result struct
*
* INPUTS:
*    result == result struct to check
*
* RETURNS:
*   pointer to resnode or NULL if some error
*********************************************************************/
val_value_t *
    xpath_get_resnode_valptr (xpath_resnode_t *resnode)
{
#ifdef DEBUG
    if (!resnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return resnode->node.valptr;

}  /* xpath_get_resnode_valptr */


/********************************************************************
* FUNCTION xpath_get_varbindQ
* 
* Get the varbindQ from a parser control block struct
*
* INPUTS:
*    pcb == parser control block to use
*
* RETURNS:
*   pointer to varbindQ or NULL if some error
*********************************************************************/
dlq_hdr_t *
    xpath_get_varbindQ (xpath_pcb_t *pcb)
{
#ifdef DEBUG
    if (!pcb) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return &pcb->varbindQ;

}  /* xpath_get_varbindQ */


/********************************************************************
* FUNCTION xpath_move_nodeset
* 
* Move the nodes from a nodeset reult into the
* target nodeset result.
* This is needed to support partial lock
* because multiple select expressions are allowed
* for the same partial lock
*
* INPUTS:
*    srcresult == XPath result nodeset source
*    destresult == XPath result nodeset target
*
* OUTPUTS:
*    srcresult nodes will be moved to the target result
*********************************************************************/
void
    xpath_move_nodeset (xpath_result_t *srcresult,
                        xpath_result_t *destresult)
{
#ifdef DEBUG
    if (srcresult == NULL || destresult == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (srcresult->restype != XP_RT_NODESET) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    if (destresult->restype != XP_RT_NODESET) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    dlq_block_enque(&srcresult->r.nodeQ,
                    &destresult->r.nodeQ);

}  /* xpath_move_nodeset */


/********************************************************************
* FUNCTION xpath_nodeset_empty
* 
* Check if the result is an empty nodeset
*
* INPUTS:
*    result == XPath result to check
*
* RETURNS:
*    TRUE if this is an empty nodeset
*    FALSE if not empty or not a nodeset
*********************************************************************/
boolean
    xpath_nodeset_empty (const xpath_result_t *result)
{
#ifdef DEBUG
    if (result == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (result->restype != XP_RT_NODESET) {
        return FALSE;
    }

    return dlq_empty(&result->r.nodeQ) ? TRUE : FALSE;

}  /* xpath_nodeset_empty */


/********************************************************************
* FUNCTION xpath_nodeset_swap_valptr
* 
* Check if the result has the oldval ptr and if so,
* replace it with the newval ptr
*
* INPUTS:
*    result == result struct to check
*    oldval == value ptr to find
*    newval == new value replace it with if oldval is found
*
*********************************************************************/
void
    xpath_nodeset_swap_valptr (xpath_result_t *result,
                               val_value_t *oldval,
                               val_value_t *newval)
{
    xpath_resnode_t *resnode;

#ifdef DEBUG
    if (result == NULL || oldval == NULL || newval == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (result->restype != XP_RT_NODESET) {
        SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    for (resnode = xpath_get_first_resnode(result);
         resnode != NULL;
         resnode = xpath_get_next_resnode(resnode)) {
        if (resnode->node.valptr == oldval) {
            resnode->node.valptr = newval;
        }            
    }

}  /* xpath_nodeset_swap_valptr */


/********************************************************************
* FUNCTION xpath_nodeset_delete_valptr
* 
* Check if the result has the oldval ptr and if so,
* delete it
*
* INPUTS:
*    result == result struct to check
*    oldval == value ptr to find
*
*********************************************************************/
void
    xpath_nodeset_delete_valptr (xpath_result_t *result,
                                 val_value_t *oldval)
{
    xpath_resnode_t *resnode, *nextnode;
    val_value_t     *valptr;
    boolean          done;

#ifdef DEBUG
    if (result == NULL || oldval == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (result->restype != XP_RT_NODESET) {
        SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    for (resnode = xpath_get_first_resnode(result);
         resnode != NULL;
         resnode = nextnode) {

        nextnode = xpath_get_next_resnode(resnode);

        /* check if the oldval is an ancestor-or-self
         * node for this valptr in the result node-set
         */
        valptr = resnode->node.valptr;
        done = FALSE;
        while (!done) {
            if (valptr == oldval) {
                dlq_remove(resnode);
                xpath_free_resnode(resnode);
                done = TRUE;
            } else if (valptr->parent != NULL &&
                       !obj_is_root(valptr->parent->obj)) {
                valptr = valptr->parent;
            } else {
                done = TRUE;
            }
        }
    }

}  /* xpath_nodeset_delete_valptr */


/********************************************************************
* FUNCTION xpath_convert_url_to_path
* 
* Convert a URL format path to XPath format path
*
* INPUTS:
*    urlpath == URL path string to convert to XPath
*    match_names == enum for selected match names mode
*    alt_naming == TRUE if alt-name and cli-drop-node-name
*      containers should be checked
*    wildcards == TRUE if wildcards allowed instead of key values
*                 FALSE if the '-' wildcard mechanism not allowed
*    withkeys == TRUE if keys are expected
*                FALSE if just nodes are expected
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced string containing XPath expression from conversion
*   NULL if some error
*********************************************************************/
xmlChar *
    xpath_convert_url_to_path (const xmlChar *urlpath,
                               ncx_name_match_t match_names,
                               boolean alt_naming,
                               boolean wildcards,
                               boolean withkeys,
                               status_t *res)
{
    xmlChar *buff;
    uint32   cnt;

#ifdef DEBUG
    if (urlpath == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    buff = NULL;
    *res = fill_xpath_string(urlpath, 
                             NULL, 
                             match_names, 
                             alt_naming, 
                             wildcards,
                             withkeys,
                             &cnt);
    if (*res == NO_ERR) {
        buff = m__getMem(cnt+1);
        if (buff == NULL) {
            *res = ERR_INTERNAL_MEM;
            return NULL;
        }
        *res = fill_xpath_string(urlpath, 
                                 buff, 
                                 match_names,
                                 alt_naming,
                                 wildcards,
                                 withkeys,
                                 &cnt);
        if (*res != NO_ERR) {
            m__free(buff);
            buff = NULL;
        } else if (LOGDEBUG2) {
            log_debug2("\nConverted urlstring '%s' to XPath '%s'",
                       urlpath, 
                       buff);
        }
    }
    return buff;

} /* xpath_convert_url_to_path */


/* END xpath.c */
