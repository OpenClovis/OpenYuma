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
/*  FILE: val.c

   support for the val_value_t data structure

   Note: NCX_BT_LEAFREF code marked with a **** comment
   is not completed yet.  The string value version of the
   leafref is always returned, instead of the canonical form
   of the data type of the leafref path target
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
19dec05      abb      begun
21jul08      abb      start obj-based rewrite
28dec11      abb      add editvars only if XML attrs present

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <xmlstring.h>

#include "procdefs.h"
#include "b64.h"
#include "cfg.h"
#include "dlq.h"
#include "getcb.h"
#include "json_wr.h"
#include "log.h"
#include "ncx.h"
#include "ncx_list.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncxconst.h"
#include "obj.h"
#include "ses.h"
#include "tk.h"
#include "typ.h"
#include "val.h"
#include "val_util.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "xpath.h"
#include "xpath1.h"
#include "xpath_yang.h"
#include "yangconst.h"
#include "uptime.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define VAL_DEBUG  1 */
/* #define VAL_EDITVARS_DEBUG */
/* #define VAL_FREE_DEBUG 1 */

/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/

typedef struct finderparms_t_ {
    val_value_t  *findval;
    val_value_t  *foundval;
    int64         findpos;
    int64         foundpos;
} finderparms_t;

/* pick a log output function for dump_value */
typedef void (*dumpfn_t) (const char *fstr, ...);

/* pick a log indent function for dump_value */
typedef void (*indentfn_t) (int32 indentcnt);

#ifdef VAL_EDITVARS_DEBUG
static uint32 editvars_malloc = 0;
static uint32 editvars_free = 0;
#endif


/********************************************************************
* FUNCTION stdout_num
* 
* Printf the specified ncx_num_t to stdout
*
* INPUTS:
*    btyp == base type of the number
*    num == number to printf
*
*********************************************************************/
static void
    stdout_num (ncx_btype_t btyp,
                const ncx_num_t *num)
{
    xmlChar    numbuff[VAL_MAX_NUMLEN];
    uint32     len;
    status_t   res;

    res = ncx_sprintf_num(numbuff, num, btyp, &len);
    if (res != NO_ERR) {
        log_stdout("invalid num '%s'", get_error_string(res));
    } else {
        log_stdout("%s", numbuff);
    }

} /* stdout_num */


/********************************************************************
* FUNCTION dump_extern
* 
* Printf the specified external file
*
* INPUTS:
*    fspec == filespec to printf
*
*********************************************************************/
static void
    dump_extern (const xmlChar *fname)
{
    FILE               *fil;
    boolean             done;
    int                 ch;

    if (!fname) {
        log_error("\nval: No extern fname");
        return;
    }

    fil = fopen((const char *)fname, "r");
    if (!fil) {
        log_error("\nError: Open extern failed (%s)", fname);
        return;
    } 

    done = FALSE;
    while (!done) {
        ch = fgetc(fil);
        if (ch == EOF) {
            fclose(fil);
            done = TRUE;
        } else {
            log_write("%c", ch);
        }
    }

} /* dump_extern */


/********************************************************************
* FUNCTION dump_alt_extern
* 
* Printf the specified external file to the alternate logfile
*
* INPUTS:
*    fspec == filespec to printf
*
*********************************************************************/
static void
    dump_alt_extern (const xmlChar *fname)
{
    FILE               *fil;
    boolean             done;
    int                 ch;

    if (!fname) {
        log_error("\nval: No extern fname");
        return;
    }

    fil = fopen((const char *)fname, "r");
    if (!fil) {
        log_error("\nval: Open extern failed (%s)", fname);
        return;
    } 

    done = FALSE;
    while (!done) {
        ch = fgetc(fil);
        if (ch == EOF) {
            fclose(fil);
            done = TRUE;
        } else {
            log_alt_write("%c", ch);
        }
    }

} /* dump_alt_extern */


/********************************************************************
* FUNCTION stdout_extern
* 
* Printf the specified external file to stdout
*
* INPUTS:
*    fspec == filespec to printf
*
*********************************************************************/
static void
    stdout_extern (const xmlChar *fname)
{
    FILE               *fil;
    boolean             done;
    int                 ch;

    if (!fname) {
        log_stdout("\nval: No extern fname");
        return;
    }

    fil = fopen((const char *)fname, "r");
    if (!fil) {
        log_stdout("\nval: Open extern failed (%s)", fname);
        return;
    } 

    done = FALSE;
    while (!done) {
        ch = fgetc(fil);
        if (ch == EOF) {
            fclose(fil);
            done = TRUE;
        } else {
            log_stdout("%c", ch);
        }
    }

} /* stdout_extern */


/********************************************************************
* FUNCTION dump_intern
* 
* Printf the specified internal XML buffer
*
* INPUTS:
*    intbuff == internal buffer to printf
*
*********************************************************************/
static void
    dump_intern (const xmlChar *intbuff)
{
    const xmlChar      *ch;

    if (!intbuff) {
        log_error("\nval: No internal buffer");
        return;
    }

    ch = intbuff;
    while (*ch) {
        log_write("%c", *ch++);
    }

} /* dump_intern */


/********************************************************************
* FUNCTION dump_alt_intern
* 
* Printf the specified internal XML buffer to the alternate logfile
*
* INPUTS:
*    intbuff == internal buffer to printf
*
*********************************************************************/
static void
    dump_alt_intern (const xmlChar *intbuff)
{
    const xmlChar      *ch;

    if (!intbuff) {
        log_error("\nval: No internal buffer");
        return;
    }

    ch = intbuff;
    while (*ch) {
        log_alt_write("%c", *ch++);
    }

} /* dump_alt_intern */


/********************************************************************
* FUNCTION stdout_intern
* 
* Printf the specified internal XML buffer to stdout
*
* INPUTS:
*    intbuff == internal buffer to printf
*
*********************************************************************/
static void
    stdout_intern (const xmlChar *intbuff)
{
    const xmlChar      *ch;

    if (!intbuff) {
        log_stdout("\nval: No internal buffer");
        return;
    }

    ch = intbuff;
    while (*ch) {
        log_stdout("%c", *ch++);
    }

} /* stdout_intern */


/********************************************************************
* FUNCTION pattern_match
* 
* Check the specified string against the specified pattern
*
* INPUTS:
*    pattern == compiled pattern to use 
*    strval == string to check
*
* RETURNS:
*    TRUE is string matches pattern; FALSE otherwise
*********************************************************************/
static boolean
    pattern_match (const xmlRegexpPtr pattern,
                   const xmlChar *strval)
{
    int ret;

    ret = xmlRegexpExec(pattern, strval);
    if (ret==1) {
        return TRUE;
    } else if (ret==0) {
        return FALSE;
    } else if (ret < 0) {
        /* pattern match execution error, but compiled ok */
        SET_ERROR(ERR_NCX_INVALID_PATTERN);
        return FALSE;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/

} /* pattern_match */


/********************************************************************
* FUNCTION check_svalQ_enum
* 
* Check all the ncx_enum_t structs in a queue of typ_sval_t 
*
* INPUTS:
*    name == enum string
*    checkQ == pointer to Q of typ_sval_t to check
*    reten == address of return typ_enum_t
* 
* OUTPUTS:
*    *reten == typ_enum_t found if matched (res == NO_ERR)
* RETURNS:
*    status, NO_ERR if string value is in the set
*********************************************************************/
static status_t
    check_svalQ_enum (const xmlChar *name,
                      dlq_hdr_t *checkQ,
                      typ_enum_t  **reten)
{
    typ_enum_t  *en;

    /* check this typdef for restrictions */
    for (en = (typ_enum_t *)dlq_firstEntry(checkQ);
         en != NULL;
         en = (typ_enum_t *)dlq_nextEntry(en)) {

        if (!xml_strcmp(name, en->name)) {
            *reten = en;
            return NO_ERR;
        }
    }
    return ERR_NCX_NOT_FOUND;

} /* check_svalQ_enum */



/********************************************************************
* FUNCTION free_editvars
* 
* Clean and free the val->editvars field
*
* INPUTS:
*    val == val_value_t data structure to use
*
* OUTPUTS:
*    val->editvars is cleaned, freed, and set to NULL
*********************************************************************/
static void
    free_editvars (val_value_t *val)
{
    if (val->editvars) {
#ifdef VAL_EDITVARS_DEBUG
        log_debug3("\n\nfree_editvars: %s: %d = %p\n",
                   (val->name) ? val->name : NCX_EL_NONE,
                   ++editvars_free,
                   val->editvars);
#endif

        if (val->editvars->insertstr) {
            m__free(val->editvars->insertstr);
        }

        if (val->editvars->insertxpcb) {
            xpath_free_pcb(val->editvars->insertxpcb);
        }

        m__free(val->editvars);
        val->editvars = NULL;
    } else {
#ifdef VAL_EDITVARS_DEBUG
        log_debug3("\nval_free_editvars skipped (%u, %s)",
                   editvars_free, 
                   val->name);
#endif
    }

}  /* free_editvars */


/********************************************************************
* FUNCTION clean_value
* 
* Scrub the memory in a ncx_value_t by freeing all
* the sub-fields. DOES NOT free the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    val == val_value_t data structure to clean
*    full == TRUE if full clean
*            FALSE if value only
*********************************************************************/
static void 
    clean_value (val_value_t *val,
                 boolean full)
{
    val_value_t   *cur;
    val_index_t   *in;
    ncx_btype_t    btyp;

    if (full && val->virtualval) {
        /* check if any cached entry of self needs to be cleared */
        val_free_value(val->virtualval);
        val->virtualval = NULL;
    }

    btyp = val->btyp;

    if (full && val->editvars) {
        free_editvars(val);
    }

    /* clean the val->v union, depending on base type */
    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        ncx_clean_num(btyp, &val->v.num);
        break;
    case NCX_BT_ENUM:
        ncx_clean_enum(&val->v.enu);
        break;
    case NCX_BT_BINARY:
        ncx_clean_binary(&val->v.binary);
        break;
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        ncx_clean_str(&val->v.str);
        break;
    case NCX_BT_IDREF:
        if (val->v.idref.name) {
            m__free(val->v.idref.name);
            val->v.idref.name = NULL;
        }
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        ncx_clean_list(&val->v.list);
        break;
    case NCX_BT_LIST:
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        while (!dlq_empty(&val->v.childQ)) {
            cur = (val_value_t *)dlq_deque(&val->v.childQ);
            val_free_value(cur);
        }
        break;
    case NCX_BT_EXTERN:
        if (val->v.fname) { 
            m__free(val->v.fname);
            val->v.fname = NULL;
        }
        break;
    case NCX_BT_INTERN:
        if (val->v.intbuff) { 
            m__free(val->v.intbuff);
            val->v.intbuff = NULL;
        }
        break;
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
    case NCX_BT_NONE:
    case NCX_BT_UNION:
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (full) {
        if (val->dname) {
            m__free(val->dname);
            val->dname = NULL;
        }
        val->nsid = 0;
        while (!dlq_empty(&val->metaQ)) {
            cur = (val_value_t *)dlq_deque(&val->metaQ);
            val_free_value(cur);
        }
    }

    while (!dlq_empty(&val->indexQ)) {
        in = (val_index_t *)dlq_deque(&val->indexQ);
        m__free(in);
    }

    if (val->xpathpcb) {
        xpath_free_pcb(val->xpathpcb);
        val->xpathpcb = NULL;
    }

}  /* clean_value */


/********************************************************************
* FUNCTION merge_simple
* 
* Merge simple src val into dest val (! MUST be same type !)
* Instance qualifiers have already been checked,
* and only zero or 1 instance is allowed, so replace
* the current dest value
*
* INPUTS:
*    src == val to merge from
*    dest == val to merge into
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    merge_simple (ncx_btype_t btyp,
                  const val_value_t *src,
                  val_value_t *dest)
{
    status_t res = NO_ERR;
    xmlChar *val_copy = NULL;

    /* need to replace the current value or merge a list, etc. */
    switch (btyp) {
    case NCX_BT_ENUM:
        dest->v.enu.name = src->v.enu.name;
        dest->v.enu.val = src->v.enu.val;
        break;
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
        dest->v.boo = src->v.boo;
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
        dest->v.num.i = src->v.num.i;
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
        dest->v.num.u = src->v.num.u;
        break;
    case NCX_BT_INT64:
        dest->v.num.l = src->v.num.l;
        break;
    case NCX_BT_UINT64:
        dest->v.num.ul = src->v.num.ul;
        break;
    case NCX_BT_DECIMAL64:
        ncx_clean_num(btyp, &dest->v.num);
        dest->v.num.dec.val = src->v.num.dec.val;
        dest->v.num.dec.digits = src->v.num.dec.digits;
        dest->v.num.dec.zeroes = src->v.num.dec.zeroes;
        break;
    case NCX_BT_FLOAT64:
        ncx_clean_num(btyp, &dest->v.num);
        dest->v.num.d = src->v.num.d;
        break;
    case NCX_BT_BINARY:
        val_copy = m__getMem( src->v.binary.ustrlen );
        if (val_copy) {
            ncx_clean_binary(&dest->v.binary);
            dest->v.binary.ubufflen = src->v.binary.ubufflen;
            dest->v.binary.ustrlen = src->v.binary.ustrlen;
            dest->v.binary.ustr = val_copy;
            memcpy( dest->v.binary.ustr, src->v.binary.ustr, 
                    src->v.binary.ustrlen );
        } else {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:   /*** !!! not sure LEAFREF will ever get here */
        val_copy = xml_strdup( src->v.str );
        if (val_copy) {
            ncx_clean_str(&dest->v.str);
            dest->v.str = val_copy;
        } else {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case NCX_BT_IDREF:
        val_copy = xml_strdup( src->v.idref.name );
        if (val_copy) {
            dest->v.idref.nsid = src->v.idref.nsid; 
            dest->v.idref.identity = src->v.idref.identity;
            if (dest->v.idref.name) {
                m__free(dest->v.idref.name);
            }
            dest->v.idref.name = val_copy;
        } else {
            res = ERR_INTERNAL_MEM;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }
    return res;

}  /* merge_simple */


/********************************************************************
* FUNCTION index_match
* 
* Check 2 val_value structs for the same instance ID
* 
* The node data types must match, and must be
*    NCX_BT_LIST
*
* All index components must exactly match.
* 
* INPUTS:
*    val1 == first value to index match
*    val2 == second value to index match
*
* RETURNS:
*  -2 if some error
*   -1 if val1 < val2 index
*   0 if val1 == val2 index
*   1 if val1 > val2 index
*********************************************************************/
static int32
    index_match (const val_value_t *val1,
                 const val_value_t *val2)
{
    const val_index_t *c1, *c2;
    int32              cmp;
    status_t           res;

    /* only lists have index chains */
    if (val1->obj->objtype != OBJ_TYP_LIST) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return -2;
    }

    /* object templates must exactly match */
    if (val1->obj != val2->obj) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return -2;
    }

    /* get the first pair of index nodes to check */
    c1 = (val_index_t *)dlq_firstEntry(&val1->indexQ);
    c2 = (val_index_t *)dlq_firstEntry(&val2->indexQ);

    /* match index values, 1 for 1, left to right */
    for (;;) {

        /* check if 1 table has index values but not the other */
        if (!c1 && !c2) {
            return 0;
        } else if (!c1) {
            return -1;
        } else if (!c2) {
            return 1;
        }

        if (xml_strcmp(c1->val->name, c2->val->name)) {
            /* could be an invalid key not checked yet
             * not calling SET_ERROR()
             */
            return -2;
        }

        res = NO_ERR;

        /* same name in the same namespace */
        if (c1->val->btyp == c2->val->btyp) {
            cmp = val_compare(c1->val, c2->val);
        } else if (typ_is_string(c1->val->btyp)) {
            /* this might be anyxml parse */
            cmp = val_compare_to_string(c2->val, VAL_STR(c1->val), &res);
            cmp *= -1;
        } else if (typ_is_string(c2->val->btyp)) {
            /* this might be anyxml parse */
            cmp = val_compare_to_string(c1->val, VAL_STR(c2->val), &res);
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
            return -2;
        }

        if (res != NO_ERR) {
            SET_ERROR(res);     
            return -2;
        }

        if (cmp) {
            return cmp;
        }

        /* node matched, get next node */
        c1 = (val_index_t *)dlq_nextEntry(c1);
        c2 = (val_index_t *)dlq_nextEntry(c2);
    }
    /*NOTREACHED*/

}  /* index_match */


/********************************************************************
* FUNCTION init_from_template
* 
* Initialize a value node from its object template
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the malloced struct to initialize
*   obj == object template to use
*   btyp == builtin type to use
*********************************************************************/
static void
    init_from_template (val_value_t *val,
                        obj_template_t *obj,
                        ncx_btype_t  btyp)
{
    typ_template_t  *listtyp;
    ncx_btype_t            listbtyp;

    val->obj = obj;
    val->typdef = obj_get_typdef(obj);
    val->btyp = btyp;
    val->nsid = obj_get_nsid(obj);

    /* the agent new_child_val function may have already
     * set the name field in agt_val_parse.c
     */
    if (!val->name) {
        val->name = obj_get_name(obj);
    }

    /* set the case object field if this is a node from a
     * choice.  This will be used to validate and manipulate
     * the choice that is not actually taking up a node
     * in the value tree
     */
    val->dataclass = obj_get_config_flag(obj) ?
        NCX_DC_CONFIG : NCX_DC_STATE;
    if (obj->parent && obj->parent->objtype==OBJ_TYP_CASE) {
        val->casobj = obj->parent;
    }
    if (!typ_is_simple(val->btyp)) {
        val_init_complex(val, btyp);
    } else if (val->btyp == NCX_BT_SLIST) {
        listtyp = typ_get_listtyp(val->typdef);
        if (!listtyp) {
            SET_ERROR(ERR_INTERNAL_VAL);
            listbtyp = NCX_BT_STRING;
        } else {
            listbtyp = typ_get_basetype(&listtyp->typdef);
        }
        ncx_init_list(&val->v.list, listbtyp);
    } else if (val->btyp == NCX_BT_BITS) {
        ncx_init_list(&val->v.list, NCX_BT_BITS);
    } else if (val->btyp == NCX_BT_EMPTY) {
        val->v.boo = TRUE;
    }

}  /* init_from_template */


/********************************************************************
* FUNCTION check_rangeQ
* 
* Check all the typ_rangedef_t structs in the queue
* For search qualifier NCX_SQUAL_RANGE
*
* INPUTS:
*    btyp == number base type
*    num == number to check
*    checkQ == pointer to Q of typ_rangedef_t to check
*
* RETURNS:
*    status, NO_ERR if string value is in the set
*********************************************************************/
static status_t
    check_rangeQ (ncx_btype_t  btyp,
                  const ncx_num_t *num,
                  dlq_hdr_t *checkQ)
{
    typ_rangedef_t  *rv;
    int32            cmp;
    boolean          lbok, ubok;

    for (rv = (typ_rangedef_t *)dlq_firstEntry(checkQ);
         rv != NULL;
         rv = (typ_rangedef_t *)dlq_nextEntry(rv)) {

        lbok = FALSE;
        ubok = FALSE;

        /* make sure the range numbers are really this type */
        if (rv->btyp != btyp) {
            return SET_ERROR(ERR_NCX_WRONG_NUMTYP);
        }

        /* check lower bound first, only if there is one */
        if (!(rv->flags & TYP_FL_LBINF)) {
            cmp = ncx_compare_nums(num, &rv->lb, btyp);
            if (cmp >= 0) {
                lbok = TRUE;
            }
        } else {
            /* LB == -INF, always passes the test */
            lbok = TRUE;
        } 

        /* check upper bound last, only if there is one */
        if (!(rv->flags & TYP_FL_UBINF)) {
            cmp = ncx_compare_nums(num, &rv->ub, btyp);
            if (cmp <= 0) {
                ubok = TRUE;
            }
        } else {
            /* UB == INF, always passes the test */
            ubok = TRUE;
        }

        if (lbok && ubok) {
            /* num is >= LB and <= UB */
            return NO_ERR;
        }
    }

    /* ran out of rangedef segments to check */
    return ERR_NCX_NOT_IN_RANGE;

} /* check_rangeQ */



/********************************************************************
* FUNCTION position_walker
* 
* Position finder val_walker_fn for XPath support
* Follows val_walker_fn_t template
*
* INPUTS:
*   val == value node being processed in the tree walk
*   cookie1 == cookie1 value passed to start fn
*   cookie2 == cookie2 value passed to start fn
*
* RETURNS:
*   TRUE if walk should continue, FALSE if not
*********************************************************************/
static boolean
    position_walker (val_value_t *val,
                     void *cookie1,
                     void *cookie2)
{
    finderparms_t *finderparms;

    finderparms = (finderparms_t *)cookie1;
    (void)cookie2;

    finderparms->foundval = val;
    finderparms->foundpos++;

    if (finderparms->findval && 
        (finderparms->findval == val)) {
        return FALSE;
    }

    if (finderparms->findpos && 
        (finderparms->findpos == finderparms->foundpos)) {
        return FALSE;
    }

    return TRUE;

}  /* position_walker */


/********************************************************************
* FUNCTION process_one_valwalker
* 
* Process one child object node for
* the obj_find_all_* functions
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
*    name == name of node to filter
*              == NULL to match any node name
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
    process_one_valwalker (val_walker_fn_t walkerfn,
                           void *cookie1,
                           void *cookie2,
                           val_value_t *val,
                           const xmlChar *modname,
                           const xmlChar *name,
                           boolean configonly,
                           boolean textmode,
                           boolean *fncalled)
{
    boolean         fnresult;

    *fncalled = FALSE;
    if (configonly && !obj_is_config(val->obj)) {
        return TRUE;
    }

    fnresult = TRUE;
    if (textmode) {
        if (obj_is_leafy(val->obj)) {
            if (walkerfn) {
                fnresult = (*walkerfn)(val, cookie1, cookie2);
            }
            *fncalled = TRUE;
        }
    } else if (modname && name) {
        if (!xml_strcmp(modname, 
                        val_get_mod_name(val)) &&
            !xml_strcmp(name, val->name)) {

            if (walkerfn) {
                fnresult = (*walkerfn)(val, cookie1, cookie2);
            }
            *fncalled = TRUE;
        }
    } else if (modname) {
        if (!xml_strcmp(modname, val_get_mod_name(val))) {
            if (walkerfn) {
                fnresult = (*walkerfn)(val, cookie1, cookie2);
            }
            *fncalled = TRUE;
        }
    } else if (name) {
        if (!xml_strcmp(name, val->name)) {
            if (walkerfn) {
                fnresult = (*walkerfn)(val, cookie1, cookie2);
            }
            *fncalled = TRUE;
        }
    } else {
        if (walkerfn) {
            fnresult = (*walkerfn)(val, cookie1, cookie2);
        }
        *fncalled = TRUE;
    }

    return fnresult;

}  /* process_one_valwalker */


/********************************************************************
* FUNCTION setup_virtual_retval
* 
* Set an initialized val_value_t as a virtual return val
*
* INPUTS:
*    virval == virtual value to set from
*    realval == value to set to
*
* OUTPUTS:
*    *realval is setup for return if NO_ERR
*********************************************************************/
static void
    setup_virtual_retval (const val_value_t  *virval,
                          val_value_t *realval)
{
    typ_template_t  *listtyp;
    ncx_btype_t      btyp;

    realval->name = virval->name;
    realval->nsid = virval->nsid;
    realval->obj = virval->obj;
    realval->typdef = virval->typdef;
    realval->flags = virval->flags;
    realval->btyp = virval->btyp;
    realval->dataclass = virval->dataclass;
    realval->parent = virval->parent;

    if (!typ_is_simple(virval->btyp)) {
        val_init_complex(realval, virval->btyp);
    } else if (virval->btyp == NCX_BT_SLIST ||
               virval->btyp == NCX_BT_BITS) {
        listtyp = typ_get_listtyp(realval->typdef);
        btyp = typ_get_basetype(&listtyp->typdef);
        ncx_init_list(&realval->v.list, btyp);
    }

}  /* setup_virtual_retval */




/********************************************************************
* FUNCTION copy_editvars
* 
* Copy the editvars struct contents
*
* INPUTS:
*    val == value to copy from
*    copy == value to copy to
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    copy_editvars (const val_value_t *val,
                   val_value_t *copy)
{
    status_t   res;

    res = NO_ERR;

    /* set the copy->editvars */
    if (val->editvars) {
        if (!copy->editvars) {
            res = val_new_editvars(copy);
            if (res != NO_ERR || !copy->editvars ) {
                if ( NO_ERR == res ) {
                    res = ERR_INTERNAL_PTR;
                }

                if ( copy->editvars ) {
                    free_editvars( copy );
                }
                return res;
            }
        }
        copy->editvars->curparent = val->editvars->curparent;
        copy->editop = val->editop;
        copy->editvars->insertop = val->editvars->insertop;
        copy->editvars->iskey = val->editvars->iskey;
        copy->editvars->operset = val->editvars->operset;

        if (val->editvars->insertstr) {
            copy->editvars->insertstr = 
                xml_strdup(val->editvars->insertstr);
            if (!copy->editvars->insertstr) {
                res = ERR_INTERNAL_MEM;
            }
        }

        if (val->editvars->insertxpcb) {
            copy->editvars->insertxpcb = 
                xpath_clone_pcb(val->editvars->insertxpcb);
            if (!copy->editvars->insertxpcb) {
                res = ERR_INTERNAL_MEM;
            }
        }

        copy->editvars->insertval = val->editvars->insertval;
    }

    return res;

}  /* copy_editvars */


/********************************************************************
* FUNCTION cache_virtual_value
* 
* get + cache as val->virtualval; DO NOT FREE the return val
* Get the value of a value node and store the malloced
* pointer in the virtualval cache 
* 
* If the val->getcb is NULL, then an error will be returned
*
* Caller should check for *res == ERR_NCX_SKIPPED
* This will be returned if virtual value has no
* instance at this time.
*
* INPUTS:
*   scb == session control block getting the virtual value
*          the scb->cache_timeout value will be used
*          id scb is not NULL
*   val == virtual value to get value for
*   res == pointer to output function return status value
*
* OUTPUTS:
*    val->virtualval set to the malloced val; will be cleared
*        if already set
*    val->cachetime set to the current time if the getcb is used
*    *res == the function return status
*
* RETURNS:
*   A pointer to the malloced val;
*   This pointer can not be stored; it is free as part of virtualval
*
*********************************************************************/
static val_value_t *
    cache_virtual_value (ses_cb_t *scb,
                         val_value_t *val,
                         status_t *res)
{
    val_value_t *retval;
    getcb_fn_t   getcb;
    time_t       timenow;
    double       timediff, timerval;
    uint32       deftimeout;
    boolean      disable_cache;

    if (!val->getcb) {
        *res = ERR_NCX_OPERATION_FAILED;
        return NULL;
    }

    getcb = (getcb_fn_t)val->getcb;

    if (val->virtualval != NULL) {
        /* already have a value; check if it is fresh enough */
        (void)uptime(&timenow);
        timediff = difftime(timenow, val->cachetime);

        disable_cache = FALSE;
        if (scb != NULL) {
            timerval = (double)scb->cache_timeout;
            if (scb->cache_timeout == 0) {
                disable_cache = TRUE;
            }
        } else {
            deftimeout = ncx_get_vtimeout_value();
            timerval = (double)deftimeout;
        }

        if (LOGDEBUG4) {
            log_debug4("\nval: virtual val timer %e", timediff);
        }

        if (disable_cache || timediff > timerval) {
            if (LOGDEBUG4) {
                log_debug4("\nval: refresh virtual val %s",
                           val->name);
            }
            val_free_value(val->virtualval);
            val->virtualval = NULL;
        } else {
            return val->virtualval;
        }
    }

    /* first get or stale and need a refresh */
    retval = val_new_value();
    if (!retval) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    setup_virtual_retval(val, retval);
    (void)uptime(&val->cachetime);

    *res = (*getcb)(NULL, GETCB_GET_VALUE, val, retval);
    if (*res != NO_ERR) {
        val_free_value(retval);
        retval = NULL;
    } else {
        val->virtualval = retval;
        val->virtualval->parent = val->parent;
    }
    return retval;

}  /* cache_virtual_value */


/********************************************************************
* FUNCTION clone_test
* 
* Clone a specified val_value_t struct and sub-trees
* Only clone the nodes that pass the test function callback
*
* INPUTS:
*    val == value to clone
*    testcb  == filter test callback function to use
*               NULL means no filter
*    with_editvars == TRUE if the new editvars should be cloned;
*                     FALSE if the new editvars should be NULL
*    res == address of return status
*
* OUTPUTS:
*    *res == resturn status
*
* RETURNS:
*   clone of val, or NULL if a malloc failure or entire node filtered
*********************************************************************/
static val_value_t *
    clone_test (const val_value_t *val,
                val_test_fn_t  testfn,
                boolean with_editvars,
                status_t *res)
{
    const val_value_t *ch;
    val_value_t       *copy, *copych;
    boolean            testres;
    uint32             i;

#ifdef DEBUG
    if (!val || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (testfn) {
        testres = (*testfn)(val);
        if (!testres) {
            *res = ERR_NCX_SKIPPED;
            return NULL;
        }
    }

    if (val->res != NO_ERR) {
        *res = val->res;
        return NULL;
    }

    copy = val_new_value();
    if (!copy) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* copy all the fields */
    copy->obj = val->obj;
    copy->typdef = val->typdef;

    if (val->dname) {
        copy->dname = xml_strdup(val->dname);
        if (!copy->dname) {
            *res = ERR_INTERNAL_MEM;
            val_free_value(copy);
            return NULL;
        }
        copy->name = copy->dname;
    } else {
        copy->dname = NULL;
        copy->name = val->name;
    }

    copy->parent = val->parent;
    copy->nsid = val->nsid;
    copy->btyp = val->btyp;
    copy->flags = val->flags;
    copy->dataclass = val->dataclass;

    /* copy any active partial locks;
     * this should be empty for candidate or PDU source
     * vals, but in case a copy of running is made, this
     * array of partial locks needs to be transferred
     */
    for (i=0; i<VAL_MAX_PLOCKS; i++) {
        copy->plock[i] = val->plock[i];
    }

    /* copy meta-data */
    for (ch = (const val_value_t *)dlq_firstEntry(&val->metaQ);
         ch != NULL;
         ch = (const val_value_t *)dlq_nextEntry(ch)) {
        copych = clone_test(ch, testfn, with_editvars, res);
        if (!copych) {
            if (*res == ERR_NCX_SKIPPED) {
                *res = NO_ERR;
            } else {
                val_free_value(copy);
                return NULL;
            }
        } else {
            dlq_enque(copych, &copy->metaQ);
        }
    }

    /* set the copy->editvars */
    if (with_editvars) {
        *res = copy_editvars(val, copy);
        if (*res != NO_ERR) {
            val_free_value(copy);
            return NULL;
        }
    }

    copy->res = val->res;
    copy->getcb = val->getcb;

    /* clone the XPath control block if there is one */
    if (val->xpathpcb) {
        copy->xpathpcb = xpath_clone_pcb(val->xpathpcb);
        if (copy->xpathpcb == NULL) {
            *res = ERR_INTERNAL_MEM;
            val_free_value(copy);
            return NULL;
        }
    }

    /* DO NOT COPY copy->index = val->index; */
    /* set copy->indexQ after cloning child nodes is done */

    copy->casobj = val->casobj;

    /* assume OK return for now */
    *res = NO_ERR;

    /* v_ union: copy the actual value or children for complex types */
    switch (val->btyp) {
    case NCX_BT_ENUM:
        if (val->v.enu.dname != NULL) {
            /* make sure clone does not point at malloced name
             * that will get deleted, so this pointer will
             * point at garbage
             */
            copy->v.enu.dname = xml_strdup(val->v.enu.dname);
            copy->v.enu.name = copy->v.enu.dname;
            if (copy->v.enu.dname == NULL) {
                *res = ERR_INTERNAL_MEM;
            }
        } else {
            copy->v.enu.name = val->v.enu.name;
        }
        VAL_ENUM(copy) = VAL_ENUM(val);
        break;
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
        copy->v.boo = val->v.boo;
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        *res = ncx_copy_num(&val->v.num, &copy->v.num, val->btyp);
        break;
    case NCX_BT_BINARY:
        ncx_init_binary(&copy->v.binary);
        if (val->v.binary.ustr) {
            copy->v.binary.ustr = m__getMem(val->v.binary.ustrlen);
            if (!copy->v.binary.ustr) {
                *res = ERR_INTERNAL_MEM;
            } else {
                memcpy(copy->v.binary.ustr, 
                       val->v.binary.ustr, 
                       val->v.binary.ustrlen);
                copy->v.binary.ustrlen = val->v.binary.ustrlen;
                copy->v.binary.ubufflen = val->v.binary.ustrlen;
            }
        }
        break;
    case NCX_BT_STRING: 
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        *res = ncx_copy_str(&val->v.str, &copy->v.str, val->btyp);
        break;
    case NCX_BT_IDREF:
        copy->v.idref.name = xml_strdup(val->v.idref.name);
        if (!copy->v.idref.name) {
            *res = ERR_INTERNAL_MEM;
        } else {
            *res = NO_ERR;
        }
        copy->v.idref.nsid = val->v.idref.nsid;
        copy->v.idref.identity = val->v.idref.identity;
        break;
    case NCX_BT_BITS:
    case NCX_BT_SLIST:
        *res = ncx_copy_list(&val->v.list, &copy->v.list);
        break;
    case NCX_BT_ANY:
    case NCX_BT_LIST:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        val_init_complex(copy, val->btyp);
        for (ch = (const val_value_t *)dlq_firstEntry(&val->v.childQ);
             ch != NULL && *res==NO_ERR;
             ch = (const val_value_t *)dlq_nextEntry(ch)) {
            if (ch->res == NO_ERR) {
                copych = clone_test(ch, testfn, with_editvars, res);
                if (!copych) {
                    if (*res == ERR_NCX_SKIPPED) {
                        *res = NO_ERR;
                    }
                } else {
                    copych->parent = copy;
                    dlq_enque(copych, &copy->v.childQ);
                }
            } else {
                log_warn("\nWarning: Skipping invalid value node '%s' (%s)",
                         ch->name, get_error_string(ch->res));
            }
        }
        break;
    case NCX_BT_EXTERN:
        if (val->v.fname) {
            copy->v.fname = xml_strdup(val->v.fname);
            if (!copy->v.fname) {
                *res = ERR_INTERNAL_MEM;
            }
        }
        break;
    case NCX_BT_INTERN:
        if (val->v.intbuff) {
            copy->v.intbuff = xml_strdup(val->v.intbuff);
            if (!copy->v.intbuff) {
                *res = ERR_INTERNAL_MEM;
            }
        }
        break;
    default:
        *res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* reconstruct index records if needed */
    if (*res==NO_ERR && !dlq_empty(&val->indexQ)) {
        *res = val_gen_index_chain(val->obj, copy);
    }

    if (*res != NO_ERR) {
        val_free_value(copy);
        copy = NULL;
    }

    return copy;

}  /* clone_test */


/*************** E X T E R N A L    F U N C T I O N S  *************/


/********************************************************************
* FUNCTION val_new_value
* 
* Malloc and initialize the fields in a val_value_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
val_value_t * 
    val_new_value (void)
{
    val_value_t *val = m__getObj(val_value_t);
    if (!val) {
        return NULL;
    }

    (void)memset(val, 0x0, sizeof(val_value_t));
    dlq_createSQue(&val->metaQ);
    dlq_createSQue(&val->indexQ);

    return val;

}  /* val_new_value */


/********************************************************************
* FUNCTION val_init_complex
* 
* Initialize the fields in a complex val_value_t
* this is deprecated and should only be called 
* by val_init_from_template
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the malloced struct to initialize
*********************************************************************/
void
    val_init_complex (val_value_t *val, 
                      ncx_btype_t btyp)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val->btyp = btyp;
    dlq_createSQue(&val->v.childQ);

}  /* val_init_complex */


/********************************************************************
* FUNCTION val_init_virtual
* 
* Special function to initialize a virtual value node
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the malloced struct to initialize
*   cbfn == get callback function to use
*   obj == object template to use
*********************************************************************/
void
    val_init_virtual (val_value_t *val,
                      void  *cbfn,
                      obj_template_t *obj)
{
#ifdef DEBUG
    if (!val || !cbfn || !obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_init_from_template(val, obj);
    val->getcb = cbfn;

}  /* val_init_virtual */


/********************************************************************
* FUNCTION val_init_from_template
* 
* Initialize a value node from its object template
*
* MUST CALL val_new_value FIRST
*
* INPUTS:
*   val == pointer to the initialized value struct to bind
*   obj == object template to use
*********************************************************************/
void
    val_init_from_template (val_value_t *val,
                            obj_template_t *obj)
{
#ifdef DEBUG
    if (!val || !obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    init_from_template(val, obj, obj_get_basetype(obj));

}  /* val_init_from_template */


/********************************************************************
* FUNCTION val_free_value
* 
* Scrub the memory in a val_value_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    val == val_value_t to delete
*********************************************************************/
void val_free_value (val_value_t *val)
{
    if (!val) {
        return;
    }

#ifdef VAL_FREE_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\nval_free_value '%s' %p", 
                   val->dname ? val->dname : NCX_EL_NONE, val);
    }
#endif

    clean_value(val, TRUE);
    m__free(val);
}  /* val_free_value */


/********************************************************************
* FUNCTION val_set_name
* 
* Set (or reset) the name of a value struct
*
* INPUTS:
*    val == val_value_t data structure to check
*    name == name string to set
*    namelen == length of name string
*********************************************************************/
void 
    val_set_name (val_value_t *val,
                  const xmlChar *name,
                  uint32 namelen)
{
#ifdef DEBUG
    if (!val || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;  
    }
#endif

    /* check no change to name */
    if (val->name && (xml_strlen(val->name) == namelen) &&
        !xml_strncmp(val->name, name, namelen)) {
        return;
    }

    /* replace the name field */
    if (val->dname) {
        m__free(val->dname);
    }
    val->dname = xml_strndup(name, namelen);
    if (!val->dname) {
        SET_ERROR(ERR_INTERNAL_MEM);
    } 
    val->name = val->dname;

}  /* val_set_name */


/********************************************************************
* FUNCTION val_force_dname
* 
* Set (or reset) the name of a value struct
* Set all descendant nodes as well
* Force dname to be used, not object name backptr
*
* INPUTS:
*    val == val_value_t data structure to check
*    name == name string to set
*    namelen == length of name string
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_force_dname (val_value_t *val)
{
    val_value_t  *chval;
    status_t      res = NO_ERR;

#ifdef DEBUG
    if (val == NULL || val->name == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val->dname == NULL) {
        val->dname = xml_strdup(val->name);
        if (val->dname == NULL) {
            return ERR_INTERNAL_MEM;
        }
        val->name = val->dname;
    }

    for (chval = val_get_first_child(val);
         chval != NULL;
         chval = val_get_next_child(chval)) {
        res = val_force_dname(chval);
        if (res != NO_ERR) {
            return res;
        }
    }
    return res;

}  /* val_force_dname */


/********************************************************************
* FUNCTION val_set_qname
* 
* Set (or reset) the name and namespace ID of a value struct
*
* INPUTS:
*    val == val_value_t data structure to check
*    nsid == namespace ID to set
*    name == name string to set
*    namelen == length of name string
*********************************************************************/
void 
    val_set_qname (val_value_t *val,
                   xmlns_id_t   nsid,
                   const xmlChar *name,
                   uint32 namelen)
{
#ifdef DEBUG
    if (!val || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;  
    }
#endif

    val->nsid = nsid;

    /* check no change to name */
    if (val->name && (xml_strlen(val->name) == namelen) &&
        !xml_strncmp(val->name, name, namelen)) {
        return;
    }

    /* replace the name field */
    if (val->dname) {
        m__free(val->dname);
    }
    val->dname = xml_strndup(name, namelen);
    if (!val->dname) {
        SET_ERROR(ERR_INTERNAL_MEM);
    } 
    val->name = val->dname;

}  /* val_set_qname */


/********************************************************************
* FUNCTION val_string_ok
* 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_string_ok (typ_def_t *typdef,
                   ncx_btype_t btyp,
                   const xmlChar *strval)
{
    return val_string_ok_ex(typdef, btyp, strval, NULL, TRUE);

} /* val_string_ok */


/********************************************************************
* FUNCTION val_string_ok_errinfo
* 
* retrieve the YANG custom error info if any 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
* Retrieve the configured error info struct if any error
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*    errinfo == address of return errinfo block (may be NULL)
*
* OUTPUTS:
*   if non-NULL: 
*       *errinfo == error record to use if return error
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_string_ok_errinfo (typ_def_t *typdef,
                           ncx_btype_t btyp,
                           const xmlChar *strval,
                           ncx_errinfo_t **errinfo)
{
    return val_string_ok_ex(typdef, btyp, strval, errinfo, TRUE);
}


/********************************************************************
* FUNCTION val_string_ok_ex
* 
* retrieve the YANG custom error info if any 
* Check a string to make sure the value is valid based
* on the restrictions in the specified typdef
* Retrieve the configured error info struct if any error
*
* INPUTS:
*    typdef == typ_def_t for the designated string type
*    btyp == basetype of the string
*    strval == string value to check
*    errinfo == address of return errinfo block (may be NULL)
*    logerrors == TRUE to log errors
*              == FALSE to not log errors (use for NCX_BT_UNION)
* OUTPUTS:
*   if non-NULL: 
*       *errinfo == error record to use if return error
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_string_ok_ex (typ_def_t *typdef,
                      ncx_btype_t btyp,
                      const xmlChar *strval,
                      ncx_errinfo_t **errinfo,
                      boolean logerrors)
{
    xpath_pcb_t      *xpathpcb;
    obj_template_t   *leafobj, *objroot;
    xpath_source_t    xpath_source;
    status_t          res;
    ncx_num_t         len;

#ifdef DEBUG
    if (!typdef || !strval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    xpath_source = XP_SRC_INSTANCEID;

    if (errinfo) {
        *errinfo = NULL;
    }

    /* make sure the data type is correct */
    switch (btyp) {
    case NCX_BT_BINARY:
        /* just get the length of the decoded binary string */
        len.u = b64_get_decoded_str_len( strval, xml_strlen(strval) );
        res = val_range_ok_errinfo( typdef, NCX_BT_UINT32, &len, errinfo);
        break;
    case NCX_BT_STRING:
        if (!(typ_is_xpath_string(typdef) ||
              typ_is_schema_instance_string(typdef))) {

            len.u = xml_strlen(strval);
            res = val_range_ok_errinfo(typdef, NCX_BT_UINT32, &len, errinfo);
            if (res == NO_ERR) {
                res = val_pattern_ok_errinfo(typdef, strval, errinfo);
            }
            break;
        }
        /* else fall through and treat XPath string */
    case NCX_BT_INSTANCE_ID:
        /* instance-identifier is handled with xpath.c xpath1.c */
        if (typ_is_schema_instance_string(typdef)) {
            xpath_source = XP_SRC_SCHEMA_INSTANCEID;
        }

        xpathpcb = xpath_new_pcb(strval, NULL);
        if (xpathpcb == NULL) {
            res = ERR_INTERNAL_MEM;
        } else if (btyp == NCX_BT_INSTANCE_ID ||
                   typ_is_schema_instance_string(typdef)) {
            /* do a first pass parsing to resolve all
             * the prefixes and check well-formed XPath
             */
            res = xpath_yang_parse_path_ex(NULL, NULL, xpath_source,
                                           xpathpcb, logerrors);
            if (res == NO_ERR) {
                leafobj = NULL;
                res = xpath_yang_validate_path_ex(NULL, 
                                                  ncx_get_gen_root(), 
                                                  xpathpcb, 
                                                  (xpath_source == 
                                                   XP_SRC_INSTANCEID) 
                                                  ? FALSE : TRUE,
                                                  &leafobj, logerrors);
            }
            xpath_free_pcb(xpathpcb);
        } else {
            xpathpcb->logerrors = logerrors;
            res = xpath1_parse_expr(NULL, NULL, xpathpcb, XP_SRC_YANG);
            if (res == NO_ERR) {
                objroot = ncx_get_gen_root();
                res = xpath1_validate_expr(objroot->tkerr.mod, 
                                           objroot, xpathpcb);
            }
            xpath_free_pcb(xpathpcb);
        }
        break;
    case NCX_BT_LEAFREF:
        /*** FIXME: MISSING LEAFREF VALIDATION ***/
        return NO_ERR;
    default:
        return ERR_NCX_WRONG_DATATYP;
    }

    return res;

} /* val_string_ok_errinfo */


/********************************************************************
* FUNCTION val_list_ok
* 
* Check a list to make sure the all the strings are valid based
* on the specified typdef
*
* validate all the ncx_lmem_t entries in the list
* against the specified typdef.  Mark any errors
* in the ncx_lmem_t flags field of each member
* in the list with an error
*
* INPUTS:
*    typdef == typ_def_t for the designated list type
*    btyp == base type (NCX_BT_SLIST or NCX_BT_BITS)
*    list == list struct with ncx_lmem_t structs to check
*
* OUTPUTS:
*   If return other than NO_ERR:
*     each list->lmem.flags field may contain bits set
*     for errors:
*        NCX_FL_RANGE_ERR: size out of range
*        NCX_FL_VALUE_ERR  value not permitted by value set, 
*                          or pattern
* RETURNS:
*    status
*********************************************************************/
status_t
    val_list_ok (typ_def_t *typdef,
                 ncx_btype_t  btyp,
                 ncx_list_t *list)
{

#ifdef DEBUG
    if (!typdef || !list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_list_ok_errinfo(typdef, btyp, list, NULL);

} /* val_list_ok */


/********************************************************************
* FUNCTION val_list_ok_errinfo
* 
* Check a list to make sure the all the strings are valid based
* on the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated list type
*    btyp == base type (NCX_BT_SLIST or NCX_BT_BITS)
*    list == list struct with ncx_lmem_t structs to check
*    errinfo == address of return rpc-error info struct
*
* OUTPUTS:
*   If return other than NO_ERR:
*     *errinfo contains the YANG specified error info, if any*   
*     each list->lmem.flags field may contain bits set
*     for errors:
*        NCX_FL_RANGE_ERR: size out of range
*        NCX_FL_VALUE_ERR  value not permitted by value set, 
*                          or pattern
* RETURNS:
*    status
*********************************************************************/
status_t
    val_list_ok_errinfo (typ_def_t *typdef,
                         ncx_btype_t  btyp,
                         ncx_list_t *list,
                         ncx_errinfo_t **errinfo)
{
    typ_template_t *listtyp;
    typ_def_t      *listdef;
    ncx_lmem_t     *lmem;
    status_t              res;

#ifdef DEBUG
    if (!typdef || !list) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    listdef = NULL;

    if (errinfo) {
        *errinfo = NULL;
    }

    /* listtyp is for the list members, not the list itself */
    if (btyp == NCX_BT_SLIST) {
        listtyp = typ_get_listtyp(typdef);
        listdef = &listtyp->typdef;
    }

    /* go through all the list members and check them */
    for (lmem = (ncx_lmem_t *)dlq_firstEntry(&list->memQ);
         lmem != NULL;
         lmem = (ncx_lmem_t *)dlq_nextEntry(lmem)) {

        /* stop on first error -- if 1 list member is invalid
         * then the entire list is invalid
         */
        if (btyp == NCX_BT_SLIST) {
            res = val_simval_ok_errinfo(listdef, lmem->val.str, errinfo);
        } else {
            res = val_bit_ok(typdef, lmem->val.str, NULL);
        }
        if (res != NO_ERR) {
            return res;
        }
    }

    return NO_ERR;

} /* val_list_ok_errinfo */

  
/********************************************************************
* FUNCTION val_enum_ok
* 
* Check an enumerated integer string to make sure the value 
* is valid based on the specified typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    enumval == enum string value to check
*    retval == pointer to return integer variable
*    retstr == pointer to return string name variable
* 
* OUTPUTS:
*    *retval == integer value of enum
*    *retstr == pointer to return string name variable 
* 
* RETURNS:
*    status
*********************************************************************/
status_t
    val_enum_ok (typ_def_t *typdef,
                 const xmlChar *enumval,
                 int32 *retval,
                 const xmlChar **retstr)
{
    dlq_hdr_t     *checkQ;
    typ_enum_t    *en;
    status_t       res;
    boolean        last;
    ncx_btype_t    btyp;

#ifdef DEBUG
    if (!typdef || !enumval ||!retval ||!retstr) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* make sure the data type is correct */
    btyp = typ_get_basetype(typdef);
    if (btyp != NCX_BT_ENUM) {
        return ERR_NCX_WRONG_DATATYP;
    }

    /* check which string Q to use for further processing */
    switch (typdef->tclass) {
    case NCX_CL_SIMPLE:
        checkQ = &typdef->def.simple.valQ;
        last = TRUE;
        break;
    case NCX_CL_NAMED:
        /* the restrictions in the newtyp override anything
         * in the parent typedef(s)
         */
        last = FALSE;
        if (typdef->def.named.newtyp) {
            checkQ = &typdef->def.named.newtyp->def.simple.valQ;
        } else {
            checkQ = NULL;
        }
        break;
    default:
        return ERR_NCX_WRONG_DATATYP;  /* should not happen */
    }

    /* check typdefs until the final one in the chain is reached */
    for (;;) {
        if (checkQ) {
            res = check_svalQ_enum(enumval, checkQ, &en);
            if (res == NO_ERR) {
                /* return both the name and number of the found enum */
                *retval = en->val;
                *retstr = en->name;
                return NO_ERR;
            }
        }

        /* check if any more typdefs to search */
        if (last) {
            return ERR_NCX_VAL_NOTINSET;
        }

        /* setup the typdef and checkQ for the next loop */
        typdef = typ_get_next_typdef(&typdef->def.named.typ->typdef);
        if (!typdef) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        switch (typdef->tclass) {
        case NCX_CL_SIMPLE:
            checkQ = &typdef->def.simple.valQ;
            last = TRUE;
            break;
        case NCX_CL_NAMED:
            if (typdef->def.named.newtyp) {
                checkQ = &typdef->def.named.newtyp->def.simple.valQ;
            } else {
                checkQ = NULL;
            }
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    /*NOTREACHED*/

} /* val_enum_ok */


/********************************************************************
* FUNCTION val_bit_ok
* 
* Check a bit name is valid for the typedef
*
* INPUTS:
*    typdef == typ_def_t for the designated bits type
*    bitname == bit name value to check
*    position == address of return bit struct position value
*
* OUTPUTS:
*  if non-NULL:
*     *position == bit position value
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_bit_ok (typ_def_t *typdef,
                const xmlChar *bitname,
                uint32 *position)
{
    dlq_hdr_t  *checkQ;
    status_t       res;
    boolean        last;
    typ_enum_t    *en;

#ifdef DEBUG
    if (!typdef || !bitname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* check which string Q to use for further processing */
    switch (typdef->tclass) {
    case NCX_CL_SIMPLE:
        checkQ = &typdef->def.simple.valQ;
        last = TRUE;
        break;
    case NCX_CL_NAMED:
        /* the restrictions in the newtyp override anything
         * in the parent typedef(s)
         */
        last = FALSE;
        if (typdef->def.named.newtyp) {
            checkQ = &typdef->def.named.newtyp->def.simple.valQ;
        } else {
            checkQ = NULL;
        }
        break;
    default:
        return ERR_NCX_WRONG_DATATYP;  /* should not happen */
    }

    /* check typdefs until the final one in the chain is reached */
    for (;;) {
        if (checkQ) {
            res = check_svalQ_enum(bitname, checkQ, &en);
            if (res == NO_ERR) {
                if (position) {
                    *position = en->pos;
                }
                return NO_ERR;
            }
        }

        /* check if any more typdefs to search */     
        if (last) {
            return ERR_NCX_VAL_NOTINSET;
        }

        /* setup the typdef and checkQ for the next loop */
        typdef = typ_get_next_typdef(&typdef->def.named.typ->typdef);
        if (!typdef) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        switch (typdef->tclass) {
        case NCX_CL_SIMPLE:
            checkQ = &typdef->def.simple.valQ;
            last = TRUE;
            break;
        case NCX_CL_NAMED:
            if (typdef->def.named.newtyp) {
                checkQ = &typdef->def.named.newtyp->def.simple.valQ;
            } else {
                checkQ = NULL;
            }
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    }
    /*NOTREACHED*/

} /* val_bit_ok */


/********************************************************************
* FUNCTION val_idref_ok
* 
* Check if an identityref QName is valid for the typedef
* The QName must match an identity that has the same base
* as specified in the typdef
*
* INPUTS:
*    typdef == typ_def_t for the designated identityref type
*    qname == QName or local-name string to check
*    nsid == namespace ID from XML node for NS of QName
*            this NSID will be used and not the prefix in qname
*            it was parsed in the XML node and is not a module prefix
*    name == address of return local name part of QName
*    id == address of return identity, if found
*
* OUTPUTS:
*  if non-NULL:
*     *name == pointer into the qname string at the start of
*              the local name part
*     *id == pointer to ncx_identity_t found
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_idref_ok (typ_def_t *typdef,
                  const xmlChar *qname,
                  xmlns_id_t nsid,
                  const xmlChar **name,
                  const ncx_identity_t **id)
{
    const typ_idref_t     *idref;
    const xmlChar         *str, *modname;
    ncx_module_t          *mod;
    const ncx_identity_t  *identity;
    boolean                found;

#ifdef DEBUG
    if (!typdef || !qname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (typ_get_basetype(typdef) != NCX_BT_IDREF) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    found = FALSE;

    idref = typ_get_cidref(typdef);
    if (!idref) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    
    /* find the local-name in the prefix:local-name combo */
    str = qname;
    while (*str && *str != ':') {
        str++;
    }
    if (*str == ':') {
        str++;
    } else {
        str = qname;
    }

    if (nsid) {
        /* str should point to the local name now */
        modname = xmlns_get_module(nsid);
        if (!modname) {
            return ERR_NCX_INVALID_VALUE;
        }
        mod = ncx_find_module(modname, NULL);
        if (!mod) {
            return ERR_NCX_INVALID_VALUE;
        }

        /* the namespace produced a module which should have
         * the identity-stmt with this name
         */
        identity = ncx_find_identity(mod, str, FALSE);
    } else {
        /* no default namespace, so be liberal and
         * try to find any matching value
         */
        identity = NULL;
        for (mod = ncx_get_first_module();
             mod != NULL && identity == NULL;
             mod = ncx_get_next_module(mod)) {
            identity = ncx_find_identity(mod, str, FALSE); 
        }
    }

    if (!identity) {
        return ERR_NCX_INVALID_VALUE;
    }

    /* got some identity match; make sure this identity
     * has an ancestor-or-self node that is the same base
     * as the base specified in the typdef
     */
    while (identity && !found) {
        if (!xml_strcmp(ncx_get_modname(identity->tkerr.mod), 
                        idref->modname) &&
            !xml_strcmp(identity->name, idref->basename)) {
            found = TRUE;
        } else {
            identity = identity->base;
        }
    }

    if (found) {
        if (name) {
            *name = str;
        }
        if (id) {
            *id = identity;
        }
        return NO_ERR;
    }

    return ERR_NCX_INVALID_VALUE;

} /* val_idref_ok */


/********************************************************************
* FUNCTION val_parse_idref
* 
* Parse a CLI BASED identityref QName into its various parts
*
* INPUTS:
*    mod == module containing the default-stmt (or NULL if N/A)
*    qname == QName or local-name string to parse
*    nsid == address of return namespace ID of the module
*            indicated by the prefix. If mod==NULL then
*            a prefix MUST be present
*    name == address of return local name part of QName
*    id == address of return identity, if found
*
* OUTPUTS:
*  if non-NULL:
*     *nsid == namespace ID for the prefix part of the QName
*     *name == pointer into the qname string at the start of
*              the local name part
*     *id == pointer to ncx_identity_t found (if any, not an error)
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_parse_idref (ncx_module_t *mod,
                     const xmlChar *qname,
                     xmlns_id_t  *nsid,
                     const xmlChar **name,
                     const ncx_identity_t **id)
{
    const xmlChar         *str, *modname;
    ncx_module_t          *impmod;
    const ncx_identity_t  *identity;
    const ncx_import_t    *import;
    xmlChar               *prefixbuff;
    status_t               res;
    uint32                 prefixlen;
    xmlns_id_t             prefixnsid;

#ifdef DEBUG
    if (!qname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    
    res = NO_ERR;
    impmod = NULL;
    identity = NULL;
    prefixnsid = 0;

    if (nsid) {
        *nsid = 0;
    }
    if (name) {
        *name = NULL;
    }

    /* find the local-name in the prefix:local-name combo */
    str = qname;
    while (*str && *str != ':') {
        str++;
    }

    /* figure out the prefix namespace ID */
    if (*str == ':') {
        /* prefix found */
        prefixnsid = 0;
        prefixlen = (uint32)(str++ - qname);
        prefixbuff = m__getMem(prefixlen+1);
        if (!prefixbuff) {
            return ERR_INTERNAL_MEM;
        } else {
            xml_strncpy(prefixbuff, qname, prefixlen);
        }

        if (mod) {
            if (xml_strcmp(mod->prefix, prefixbuff)) {
                import = ncx_find_pre_import(mod, prefixbuff);
                if (import) {
                    impmod = ncx_find_module(import->module,
                                             import->revision);
                    if (impmod) {
                        prefixnsid = impmod->nsid;
                        identity = ncx_find_identity(impmod, str, FALSE);
                    } else {
                        res = ERR_NCX_MOD_NOT_FOUND;
                    }
                } else {
                    res = ERR_NCX_IMP_NOT_FOUND;
                }
            } else {
                prefixnsid = mod->nsid;
                identity = ncx_find_identity(mod, str, FALSE);
            }
        } else {
            /* look up the default prefix for a module */
            prefixnsid = xmlns_find_ns_by_prefix(prefixbuff);
            if (prefixnsid) {
                modname = xmlns_get_module(prefixnsid);
                if (modname) {
                    impmod = ncx_find_module(modname, NULL);
                    if (impmod) {
                        prefixnsid = impmod->nsid;
                        identity = ncx_find_identity(impmod, str, FALSE);
                    } else {
                        res = ERR_NCX_MOD_NOT_FOUND;
                    }
                } else {
                    res = ERR_NCX_MOD_NOT_FOUND;
                }
            } else {
                res = ERR_NCX_PREFIX_NOT_FOUND;
            }
        }
        m__free(prefixbuff);
        if (name) {
            *name = str;
        }
    } else {
        /* no prefix entered */
        if (name) {
            *name = qname;
        }
        if (mod) {
            prefixnsid = mod->nsid;
            identity = ncx_find_identity(mod, qname, FALSE);
        } else {
            identity = NULL;
            /* check all session modules first */
            for (mod = ncx_get_first_session_module();
                 mod != NULL && identity == NULL;
                 mod = ncx_get_next_session_module(mod)) {
                identity = ncx_find_identity(mod, qname, FALSE);
                if (identity) {
                    prefixnsid = mod->nsid;                 
                }
            }

            /* check all normal modules last */
            for (mod = ncx_get_first_module();
                 mod != NULL && identity == NULL;
                 mod = ncx_get_next_module(mod)) {
                identity = ncx_find_identity(mod, qname, FALSE);
                if (identity) {
                    prefixnsid = mod->nsid;                 
                }
            }
            if (!identity) {
                res = ERR_NCX_INVALID_VALUE;
            }
        }
    }

    if (id) {
        *id = identity;
    }
    if (nsid) {
        *nsid = prefixnsid;
    }

    return res;

}  /* val_parse_idref */


/********************************************************************
* FUNCTION val_range_ok
* 
* Check a number to see if it is in range or not
* Could be a number or size range
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    btyp == base type of num
*    num == number to check
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_range_ok (typ_def_t *typdef,
                  ncx_btype_t  btyp,
                  const ncx_num_t *num)
{
#ifdef DEBUG
    if (!typdef || !num) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_range_ok_errinfo(typdef, btyp, num, NULL);

} /* val_range_ok */


/********************************************************************
* FUNCTION val_range_ok_errinfo
* 
* Check a number to see if it is in range or not
* Could be a number or size range
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    btyp == base type of num
*    num == number to check
*    errinfo == address of return error struct
*
* OUTPUTS:
*   if non-NULL:
*     *errinfo == errinfo record on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_range_ok_errinfo (typ_def_t *typdef,
                          ncx_btype_t  btyp,
                          const ncx_num_t *num,
                          ncx_errinfo_t **errinfo)
{
    typ_def_t       *testdef;
    dlq_hdr_t       *checkQ;
    ncx_errinfo_t   *range_errinfo;
    status_t          res;

#ifdef DEBUG
    if (!typdef || !num) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (errinfo) {
        *errinfo = NULL;
    }

    /* find the real typdef to check */
    testdef = typ_get_qual_typdef(typdef, NCX_SQUAL_RANGE);
    if (!testdef) {
        /* assume this means no range specified and
         * not an internal PTR or VAL error
         */
        return NO_ERR;   
    }

    /* there can only be one active range, which is
     * the most derived typdef that declares a range
     */
    range_errinfo = typ_get_range_errinfo(testdef);
    checkQ = typ_get_rangeQ_con(testdef);

    res = check_rangeQ(btyp, num, checkQ);
    if (res != NO_ERR && errinfo && range_errinfo &&
        ncx_errinfo_set(range_errinfo)) {
        *errinfo = range_errinfo;
    }
    return res;

} /* val_range_ok_errinfo */


/********************************************************************
* FUNCTION val_pattern_ok
* 
* Check a string against all the patterns in a big AND expression
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    strval == string value to check
* 
* RETURNS:
*    NO_ERR if pattern OK or no patterns found to check; error otherwise
*********************************************************************/
status_t
    val_pattern_ok (typ_def_t *typdef,
                    const xmlChar *strval)
{
#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_pattern_ok_errinfo(typdef, strval, NULL);

}  /* val_pattern_ok */


/********************************************************************
* FUNCTION val_pattern_ok_errinfo
* 
* Check a string against all the patterns in a big AND expression
*
* INPUTS:
*    typdef == typ_def_t for the designated enum type
*    strval == string value to check
*    errinfo == address of return errinfo struct for err-pattern
*
* OUTPUTS:
*   *errinfo set to error info struct if any, and if error exit
*
* RETURNS:
*    NO_ERR if pattern OK or no patterns found to check; 
*    error otherwise, and *errinfo will be set if the pattern
*    that failed has any errinfo defined in it
*********************************************************************/
status_t
    val_pattern_ok_errinfo (typ_def_t *typdef,
                            const xmlChar *strval,
                            ncx_errinfo_t **errinfo)
{
    typ_pattern_t   *pat;

#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (!strval) {
        strval = EMPTY_STRING;
    }

    if (typ_get_basetype(typdef) != NCX_BT_STRING) {
        return ERR_NCX_WRONG_DATATYP;
    }

    if (errinfo) {
        *errinfo = NULL;
    }

    while (typdef) {
        for (pat = typ_get_first_pattern(typdef);
             pat != NULL;
             pat = typ_get_next_pattern(pat)) {

            if (!pattern_match(pat->pattern, strval)) {
                if (errinfo && 
                    ncx_errinfo_set(&pat->pat_errinfo)) {
                    *errinfo = &pat->pat_errinfo;
                }
                return ERR_NCX_PATTERN_FAILED;
            } /* else matched -- keep trying more patterns */
        }

        typdef = typ_get_parent_typdef(typdef);
    }

    return NO_ERR;

} /* val_pattern_ok_errinfo */


/********************************************************************
* FUNCTION val_simval_ok
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_simval_ok (typ_def_t *typdef,
                   const xmlChar *simval)
{
#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_simval_ok_max(typdef, simval, NULL, NULL, TRUE);

} /* val_simval_ok */


/********************************************************************
* FUNCTION val_simval_ok_errinfo
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_simval_ok_errinfo (typ_def_t *typdef,
                           const xmlChar *simval,
                           ncx_errinfo_t **errinfo)
{
    return val_simval_ok_max(typdef, simval, errinfo, NULL, TRUE);

} /* val_simval_ok_errinfo */


/********************************************************************
* FUNCTION val_simval_ok_ex
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*    mod == module in progress to use for idref and other
*           strings with prefixes in them
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_simval_ok_ex (typ_def_t *typdef,
                      const xmlChar *simval,
                      ncx_errinfo_t **errinfo,
                      ncx_module_t *mod)
{
    return val_simval_ok_max(typdef, simval, errinfo, mod, TRUE);

}  /* val_simval_ok_ex */


/********************************************************************
* FUNCTION val_simval_ok_max
* 
* check any simple type to see if it is valid,
* but do not retrieve the value; used to check the
* default parameter for example
*
* INPUTS:
*    typdef == typ_def_t for the simple type to check
*    simval == value string to check (NULL means empty string)
*    errinfo == address of return error struct
*    mod == module in progress to use for idref and other
*           strings with prefixes in them
*    logerrors == TRUE to log errors; FALSE to not log errors
*                (use FALSE for NCX_BT_UNION)
* OUTPUTS:
*   if non-NULL:
*      *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_simval_ok_max (typ_def_t *typdef,
                       const xmlChar *simval,
                       ncx_errinfo_t **errinfo,
                       ncx_module_t *mod,
                       boolean logerrors)
{
    const xmlChar          *retstr, *name;
    val_value_t            *unval;
    typ_template_t         *listtyp;
    typ_def_t              *realtypdef;
    const ncx_identity_t   *identity;
    ncx_num_t               num;
    ncx_list_t              list;
    status_t                res;
    ncx_btype_t             btyp, listbtyp;
    int32                   retval;
    boolean                 forced;
    xmlns_id_t              nsid;

#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (errinfo) {
        *errinfo = NULL;
    }

    if (!simval) {
        forced = TRUE;
        simval = EMPTY_STRING;
    } else {
        forced = FALSE;
    }

    btyp = typ_get_basetype(typdef);

    switch (btyp) {
    case NCX_BT_NONE:
        res = ERR_NCX_DEF_NOT_FOUND;
        break;
    case NCX_BT_ANY:
        res = ERR_NCX_INVALID_VALUE;
        break;
    case NCX_BT_ENUM:
        res = val_enum_ok(typdef, simval, &retval, &retstr);
        break;
    case NCX_BT_EMPTY:
        if (forced || !*simval) {
            res = NO_ERR;
        } else {
            res = ERR_NCX_INVALID_VALUE;
        }
        break;
    case NCX_BT_BOOLEAN:
        if (ncx_is_true(simval) || ncx_is_false(simval)) {
            res = NO_ERR;
        } else {
            res = ERR_NCX_INVALID_VALUE;
        }
        break;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
        if (*simval == '-') {
            res = ERR_NCX_INVALID_VALUE;
            break;
        }
        /* fall through */
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_FLOAT64:
        res = ncx_decode_num(simval, btyp, &num);
        if (res == NO_ERR) {
            res = val_range_ok_errinfo(typdef, btyp, &num, errinfo);
        }
        ncx_clean_num(btyp, &num);
        break;
    case NCX_BT_DECIMAL64:
        res = ncx_decode_dec64(simval, typ_get_fraction_digits(typdef), &num);
        if (res == NO_ERR) {
            res = val_range_ok(typdef, btyp, &num);
        }
        ncx_clean_num(btyp, &num);
        break;
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
        res = val_string_ok_ex(typdef, btyp, simval, errinfo, logerrors);
        break;
    case NCX_BT_INSTANCE_ID:
        res = val_string_ok_ex(typdef, btyp, simval, errinfo, logerrors);
        break;
    case NCX_BT_UNION:
        unval = val_new_value();
        if (!unval) {
            res = ERR_INTERNAL_MEM;
        } else {
            unval->btyp = NCX_BT_UNION;
            unval->typdef = typdef;
            res = val_union_ok_ex(typdef, simval, unval, errinfo, mod);
            unval->btyp = NCX_BT_NONE;            
            val_free_value(unval);
        }
        break;
    case NCX_BT_LEAFREF:
        /* cannot check instances for default or
         * manager-side set function, so just check
         * if the pointed-at typedef validates correctly
         */
        realtypdef = typ_get_xref_typdef(typdef);
        if (realtypdef) {
            res = val_simval_ok_max(realtypdef, simval, errinfo, mod,
                                    logerrors);
        } else {
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_IDREF:
        identity = NULL;
        res = val_parse_idref(mod, simval, &nsid, &name, &identity);
        if (res == NO_ERR) {
            if (identity == NULL) {
                res = val_idref_ok(typdef, simval, nsid, &name, &identity);
            }
        }
        break;
    case NCX_BT_BITS:
        ncx_init_list(&list, NCX_BT_BITS);
        res = ncx_set_list(NCX_BT_BITS, simval, &list);
        if (res == NO_ERR) {
            res = ncx_finish_list(typdef, &list);
            if (res == NO_ERR) {
                res = val_list_ok_errinfo(typdef, btyp, &list, errinfo);
            }
        }
        ncx_clean_list(&list);
        break;
    case NCX_BT_SLIST:
        listtyp = typ_get_listtyp(typdef);
        listbtyp = typ_get_basetype(&listtyp->typdef);
        ncx_init_list(&list, listbtyp);
        res = ncx_set_list(listbtyp, simval, &list);
        if (res == NO_ERR) {
            res = ncx_finish_list(&listtyp->typdef, &list);
            if (res == NO_ERR) {
                res = val_list_ok_errinfo(typdef, btyp, &list, errinfo);
            }
        }
        ncx_clean_list(&list);
        break;
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_LIST:
    case NCX_BT_EXTERN:
    case NCX_BT_INTERN:
        res = ERR_NCX_INVALID_VALUE;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* val_simval_ok_max */


/********************************************************************
* FUNCTION val_union_ok
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_union_ok (typ_def_t *typdef,
                  const xmlChar *strval,
                  val_value_t *retval)
{
#ifdef DEBUG
    if (!typdef || !retval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return val_union_ok_ex(typdef, strval, retval, NULL, NULL);

} /* val_union_ok */


/********************************************************************
* FUNCTION val_union_ok_errinfo
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*    errinfo == address of error struct
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*   *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_union_ok_errinfo (typ_def_t *typdef,
                          const xmlChar *strval,
                          val_value_t *retval,
                          ncx_errinfo_t **errinfo)
{
    return val_union_ok_ex(typdef, strval, retval, errinfo, NULL);

} /* val_union_ok_errinfo */


/********************************************************************
* FUNCTION val_union_ok_ex
* 
* Check a union to make sure the string is valid based
* on the specified typdef, and convert the string to
* an NCX internal format
*
* INPUTS:
*    typdef == typ_def_t for the designated union type
*    strval == the value to check against the member typ defs
*    retval == pointer to output struct for converted value
*    errinfo == address of error struct
*    mod == module in progress, if any
*
* OUTPUTS:
*   If return NO_ERR:
*   retval->str or retval->num will be set with the converted value
*   *errinfo == error struct on error exit
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_union_ok_ex (typ_def_t *typdef,
                     const xmlChar *strval,
                     val_value_t *retval,
                     ncx_errinfo_t **errinfo,
                     ncx_module_t *mod)
{
    typ_def_t        *undef;
    typ_unionnode_t  *un;
    status_t                res;
    boolean                 done;
    ncx_btype_t             testbtyp;

#ifdef DEBUG
    if (!typdef || !retval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    if (errinfo) {
        *errinfo = NULL;
    }

    /* get the first union member type */
    un = typ_first_unionnode(typdef);

#ifdef DEBUG
    if (!un || (!un->typ && !un->typdef)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif

    done = FALSE;

    /* go through all the union member typdefs until
     * the first match (decodes as a valid value for that typdef)
     */
    while (!done) {

        /* get type info for this member typedef */
        if (un->typ) {
            undef = &un->typ->typdef;
        } else if (un->typdef) {
            undef = un->typdef;
        } else {
            res = SET_ERROR(ERR_INTERNAL_VAL);
            done = TRUE;
            continue;
        }

        testbtyp = typ_get_basetype(undef);
        if (testbtyp == NCX_BT_UNION) {
            res = val_union_ok_ex(undef, strval, retval, errinfo, mod);
        } else {
            res = val_simval_ok_max(undef, strval, errinfo, mod, FALSE);
        }
        if (res == NO_ERR) {
            /* the un->typ field may be NULL for YANG unions in progress
             * When the default is checked this ptr may be NULL, but the
             * retval is not used by that fn.  After the module is
             * registered, the un->typ field should be set
             */
            if (testbtyp != NCX_BT_UNION) {
                retval->btyp = typ_get_basetype(undef);
            }
            done = TRUE;
        } else if (res != ERR_INTERNAL_MEM) {
            un = (typ_unionnode_t *)dlq_nextEntry(un);
            if (!un) {
                res = ERR_NCX_WRONG_NODETYP;
                done = TRUE;
            }
        } else {
            done = TRUE;
        }
    }

    return res;

} /* val_union_ok_ex */


/********************************************************************
* FUNCTION val_get_metaQ
* 
* Get the meta Q header for the value
* 
* INPUTS:
*    val == value node to check
*
* RETURNS:
*   pointer to the metaQ for this value
*********************************************************************/
dlq_hdr_t *
    val_get_metaQ (val_value_t  *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (val->getcb) {
        /* the virtual value will not have any attributes
         * present; only the PDU value nodes will have
         * any XML attributes present
         */
        return NULL;
    } else {
        return &val->metaQ;
    }

}  /* val_get_metaQ */


/********************************************************************
* FUNCTION val_get_first_meta
* 
* Get the first metaQ entry from the specified Queue
* 
* INPUTS:
*    queue == queue of meta-vals to check
*
* RETURNS:
*   pointer to the first meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
val_value_t *
    val_get_first_meta (dlq_hdr_t *queue)
{
#ifdef DEBUG
    if (!queue) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_value_t *)dlq_firstEntry(queue);

}  /* val_get_first_meta */


/********************************************************************
* FUNCTION val_get_first_meta_val
* 
* Get the first metaQ entry from the specified Queue
* 
* INPUTS:
*    value node to get the metaQ from
*
* RETURNS:
*   pointer to the first meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
val_value_t *
    val_get_first_meta_val (val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_value_t *)dlq_firstEntry(&val->metaQ);

}  /* val_get_first_meta_val */


/********************************************************************
* FUNCTION val_get_next_meta
* 
* Get the next metaQ entry from the specified entry
* 
* INPUTS:
*    curnode == current meta-var node
*
* RETURNS:
*   pointer to the next meta-var in the Queue if found, 
*   or NULL if none
*********************************************************************/
val_value_t *
    val_get_next_meta (val_value_t *curnode)
{
#ifdef DEBUG
    if (!curnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_value_t *)dlq_nextEntry(curnode);

}  /* val_get_next_meta */


/********************************************************************
* FUNCTION val_meta_empty
* 
* Check if the metaQ is empty for the value node
*
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the metaQ for the value is empty
*   FALSE otherwise
*********************************************************************/
boolean
    val_meta_empty (val_value_t *val)
{

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TRUE;
    }
#endif

    if (val->getcb) {
        /* only the real values (not virtual values) will
         * have any XML attributes present
         */
        return TRUE;
    } else {
        return dlq_empty(&val->metaQ);
    }

}  /* val_meta_empty */


/********************************************************************
* FUNCTION val_find_meta
* 
* Get the corresponding meta data node 
* 
* INPUTS:
*    val == value to check for metadata
*    nsid == namespace ID of 'name'; 0 == don't use
*    name == name of metadata variable name
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_find_meta (val_value_t  *val,
                   xmlns_id_t    nsid,
                   const xmlChar *name)
{
    val_value_t *metaval;

#ifdef DEBUG
    if (!val || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
        
    for (metaval = (val_value_t *)dlq_firstEntry(&val->metaQ);
         metaval != NULL;
         metaval = (val_value_t *)dlq_nextEntry(metaval)) {

        /* check the node if the name matches and
         * check for position instance match 
         */
        if (!xml_strcmp(metaval->name, name)) {
            if (xmlns_ids_equal(nsid, metaval->nsid)) {
                return metaval;
            }
        }
    }

    return NULL;

}  /* val_find_meta */


/********************************************************************
* FUNCTION val_meta_match
* 
* Return true if the corresponding attribute exists and has
* the same value 
* 
* INPUTS:
*    val == value to check for metadata
*    metaval == value to match in the val->metaQ 
*
* RETURNS:
*   TRUE if the specified attr if found and has the same value
*   FALSE otherwise
*********************************************************************/
boolean
    val_meta_match (val_value_t *val,
                    val_value_t *metaval)
{
    val_value_t *m1;
    dlq_hdr_t   *queue;
    boolean            ret, done;

#ifdef DEBUG
    if (!val || !metaval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    queue = val_get_metaQ(val);
    if (!queue) {
        return FALSE;
    }

    ret = FALSE;
    done = FALSE;

    /* check all the metavars in the val->metaQ until
     * the specified entry is found or list ends
     */
    for (m1 = val_get_first_meta(queue);
         m1 != NULL && !done;
         m1 = val_get_next_meta(m1)) {

        /* check the node if the name matches and
         * then if the namespace matches
         */
        if (!xml_strcmp(metaval->name, m1->name)) {
            if (!xmlns_ids_equal(metaval->nsid, m1->nsid)) {
                continue;
            }
            ret = (val_compare(metaval, m1)) ? FALSE : TRUE;
            done = TRUE;
        }
    }

    return ret;

}  /* val_meta_match */


/********************************************************************
* FUNCTION val_metadata_inst_count
* 
* Get the number of instances of the specified attribute
*
* INPUTS:
*     val == value to check for metadata instance count
*     nsid == namespace ID of the meta data variable
*     name == name of the meta data variable
*
* RETURNS:
*   number of instances found in val->metaQ
*********************************************************************/
uint32
    val_metadata_inst_count (val_value_t  *val,
                             xmlns_id_t nsid,
                             const xmlChar *name)
{
    val_value_t *metaval;
    uint32       cnt;

#ifdef DEBUG
    if (!val || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    cnt = 0;

    for (metaval = (val_value_t *)dlq_firstEntry(&val->metaQ);
         metaval != NULL;
         metaval = (val_value_t *)dlq_nextEntry(metaval)) {
        if (xml_strcmp(metaval->name, name)) {
            continue;
        }
        if (nsid && metaval->nsid) {
            if (metaval->nsid == nsid) {
                cnt++;
            }
        } else {
            cnt++;
        }
    }
    return cnt;

} /* val_metadata_inst_count */


/********************************************************************
* FUNCTION val_dump_value
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
void
    val_dump_value (val_value_t *val,
                    int32 startindent)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_dump_value_max(val, 
                       startindent, 
                       NCX_DEF_INDENT,
                       DUMP_VAL_LOG, 
                       ncx_get_display_mode(),
                       FALSE,
                       FALSE);

} /* val_dump_value */


/********************************************************************
* FUNCTION val_dump_value_ex
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*    display_mode == display mode to use
*********************************************************************/
void
    val_dump_value_ex (val_value_t *val,
                       int32 startindent,
                       ncx_display_mode_t display_mode)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_dump_value_max(val, 
                       startindent, 
                       NCX_DEF_INDENT,
                       DUMP_VAL_LOG,
                       display_mode,
                       FALSE,
                       FALSE);

} /* val_dump_value_ex */


/********************************************************************
* FUNCTION val_dump_alt_value
* 
* Printf the specified val_value_t struct to 
* the alternate logfile
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
void
    val_dump_alt_value (val_value_t *val,
                        int32 startindent)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_dump_value_max(val, 
                       startindent, 
                       NCX_DEF_INDENT,
                       DUMP_VAL_ALT_LOG,
                       ncx_get_display_mode(),
                       FALSE,
                       FALSE);

} /* val_dump_alt_value */


/********************************************************************
* FUNCTION val_stdout_value
* 
* Printf the specified val_value_t struct to stdout
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*
*********************************************************************/
void
    val_stdout_value (val_value_t *val,
                      int32 startindent)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_dump_value_max(val, 
                       startindent, 
                       NCX_DEF_INDENT,
                       DUMP_VAL_STDOUT,
                       ncx_get_display_mode(),
                       FALSE,
                       FALSE);

} /* val_stdout_value */


/********************************************************************
* FUNCTION val_stdout_value_ex
* 
* Printf the specified val_value_t struct to stdout
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to printf
*    startindent == start indent char count
*    display_mode == display mode to use
*********************************************************************/
void
    val_stdout_value_ex (val_value_t *val,
                         int32 startindent,
                         ncx_display_mode_t display_mode)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val_dump_value_max(val, 
                       startindent, 
                       NCX_DEF_INDENT,
                       DUMP_VAL_STDOUT,
                       display_mode,
                       FALSE,
                       FALSE);

} /* val_stdout_value_ex */


/********************************************************************
* FUNCTION val_dump_value_max
* 
* Printf the specified val_value_t struct to 
* the logfile, or stdout if none set
* Uses conf file format (see ncx/conf.h)
*
* INPUTS:
*    val == value to dump
*    startindent == start indent char count
*    indent_amount == number of spaces for each indent
*    dumpmode == logging mode to use
*    display_mode == formatting mode for display
*    with_meta == TRUE if metaQ should be printed
*                 FALSE to skip meta data
*    configonly == TRUE if config only nodes should be displayed
*                  FALSE if all nodes should be displayed
*********************************************************************/
void
    val_dump_value_max (val_value_t *val,
                        int32 startindent,
                        int32 indent_amount,
                        val_dumpvalue_mode_t dumpmode,
                        ncx_display_mode_t display_mode,
                        boolean with_meta,
                        boolean configonly)
{
    dumpfn_t            dumpfn, errorfn;
    indentfn_t          indentfn;
    val_value_t        *chval;
    ncx_lmem_t         *listmem;
    val_idref_t        *idref;
    const xmlChar      *prefix;
    xmlChar            *buff;
    FILE               *outputfile;
    dlq_hdr_t          *metaQ;
    val_value_t        *metaval;
    ncx_btype_t         btyp, lbtyp;
    uint32              len;
    status_t            res;
    boolean             quotes;
    int32               bump_amount;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    bump_amount = max(0, indent_amount);

    if (configonly && !obj_is_config(val->obj)) {
        return;
    }

    if (display_mode == NCX_DISPLAY_MODE_XML ||
        display_mode == NCX_DISPLAY_MODE_XML_NONS) {

        outputfile = log_get_logfile();
        if (!outputfile) {
            outputfile = stdout;
        }
        res = xml_wr_check_open_file(outputfile,
                                     val,
                                     NULL,
                                     FALSE,
                                     FALSE,
                                     (display_mode == NCX_DISPLAY_MODE_XML),
                                     startindent,
                                     indent_amount,
                                     NULL);
        if (res != NO_ERR) {
            log_error("\nError: dump value '%s' to XML file failed (%s)",
                      val->name, get_error_string(res));
        }
        return;
    } else if (display_mode == NCX_DISPLAY_MODE_JSON) {
        outputfile = log_get_logfile();
        if (!outputfile) {
            outputfile = stdout;
        }
        res = json_wr_check_open_file(outputfile,
                                      val,
                                      startindent,
                                      indent_amount,
                                      NULL);
        if (res != NO_ERR) {
            log_error("\nError: dump value '%s' to JSON file failed (%s)",
                      val->name, get_error_string(res));
        }
        return;
    }

    switch (dumpmode) {
    case DUMP_VAL_NONE:
        return;
    case DUMP_VAL_STDOUT:
        dumpfn = log_stdout;
        errorfn = log_stdout;
        indentfn = log_stdout_indent;
        break;
    case DUMP_VAL_LOG:
        dumpfn = log_write;
        errorfn = log_error;
        indentfn = log_indent;
        break;
    case DUMP_VAL_ALT_LOG:
        dumpfn = log_alt_write;
        errorfn = log_error;
        indentfn = log_alt_indent;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    /* indent and print the val name */
    (*indentfn)(startindent);
    if (display_mode == NCX_DISPLAY_MODE_PLAIN) {
        if (val->btyp == NCX_BT_EXTERN) {
            (*dumpfn)("%s (extern=%s) ", 
                      (val->name) ? (const char *)val->name : "--",
                      (val->v.fname) ? (const char *)val->v.fname : "--");
        } else if (val->btyp == NCX_BT_INTERN) {
            (*dumpfn)("%s (intern) ",
                      (val->name) ? (const char *)val->name : "--");
        } else if (dumpmode == DUMP_VAL_ALT_LOG &&
                   !xml_strcmp(val->name, NCX_EL_DATA)) {
            ;  /* skip the name */
        } else {
            (*dumpfn)("%s ", (val->name) ? (const char *)val->name : "--");
        }
    } else {
        if (display_mode == NCX_DISPLAY_MODE_PREFIX) {
            prefix = xmlns_get_ns_prefix(val_get_nsid(val));
        } else {
            /* assume the mode is NCX_DISPLAY_MODE_MODULE */
            prefix = val_get_mod_name(val);
        }
        if (!prefix) {
            prefix = (const xmlChar *)"invalid";
        }
        if (val->btyp == NCX_BT_EXTERN) {
            (*dumpfn)("%s:%s (extern=%s) ", 
                      prefix,
                      (val->name) ? (const char *)val->name : "--",
                      (val->v.fname) ? (const char *)val->v.fname : "--");
        } else if (val->btyp == NCX_BT_INTERN) {
            (*dumpfn)("%s:%s (intern) ",
                      prefix,
                      (val->name) ? (const char *)val->name : "--");
        } else if (dumpmode == DUMP_VAL_ALT_LOG &&
                   !xml_strcmp(val->name, NCX_EL_DATA)) {
            ;  /* skip the name */
        } else {
            (*dumpfn)("%s:%s ", 
                      prefix,
                      (val->name) ? (const char *)val->name : "--");
        }
    }

    btyp = val->btyp;

    /* check if an index clause needs to be printed next */
    if (!dlq_empty(&val->indexQ)) {
        res = val_get_index_string(NULL, 
                                   NCX_IFMT_CLI, 
                                   val, 
                                   NULL, 
                                   &len);
        if (res == NO_ERR) {
            buff = m__getMem(len+1);
            if (buff) {
                res = val_get_index_string(NULL, 
                                           NCX_IFMT_CLI, 
                                           val, 
                                           buff, 
                                           &len);
                if (res == NO_ERR) {
                    (dumpfn)("%s ", buff);
                } else {
                    SET_ERROR(res);
                }
                m__free(buff);
            } else {
                (*errorfn)("\nval: malloc failed for %u bytes", len+1);
            }
        }
    }

    /* dump the value, depending on the base type */
    switch (btyp) {
    case NCX_BT_NONE:
        SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case NCX_BT_ANY:
        (*dumpfn)("(any)");
        break;
    case NCX_BT_ENUM:
        if (val->v.enu.name) {
            if (val_need_quotes(val->v.enu.name)) {
                (*dumpfn)("\'%s\'", (const char *)val->v.enu.name);
            } else {
                (*dumpfn)("%s", (const char *)val->v.enu.name);
            }
        }
        break;
    case NCX_BT_EMPTY:
        if (!val->v.boo) {
            (*dumpfn)("(not set)");   /* should not happen */
        }
        break;
    case NCX_BT_BOOLEAN:
        if (val->v.boo) {
            (*dumpfn)("true");
        } else {
            (*dumpfn)("false");
        }
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        switch (dumpmode) {
        case DUMP_VAL_STDOUT:
            stdout_num(btyp, &val->v.num);
            break;
        case DUMP_VAL_LOG:
            ncx_printf_num(&val->v.num, btyp);
            break;
        case DUMP_VAL_ALT_LOG:
            ncx_alt_printf_num(&val->v.num, btyp);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case NCX_BT_BINARY:
        (*dumpfn)("binary string, length '%u'",
                  val->v.binary.ustrlen);
        break;
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        /* leafref is not dumped in canonical form */
        if (VAL_STR(val)) {
            quotes = val_need_quotes(VAL_STR(val));

            if (dumpmode == DUMP_VAL_ALT_LOG &&
                !xml_strcmp(val->name, NCX_EL_DATA)) {
                quotes = FALSE;
            }

            if (quotes) {
                (*dumpfn)("%c", VAL_QUOTE_CH);
            }
            if (val->obj && obj_is_password(val->obj)) {
                (*dumpfn)("%s", VAL_PASSWORD_STRING);
            } else {
                (*dumpfn)("%s", (const char *)VAL_STR(val));
            }
            if (quotes) {
                (*dumpfn)("%c", VAL_QUOTE_CH);
            }
        }
        break;
    case NCX_BT_IDREF:
        idref = VAL_IDREF(val);
        if (idref->nsid && idref->name) {
            (*dumpfn)("%s:%s",
                      xmlns_get_ns_prefix(idref->nsid),
                      idref->name);
        } else if (idref->name) {
            (*dumpfn)("%s", idref->name);
        }
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        if (dlq_empty(&val->v.list.memQ)) {
            (*dumpfn)("{ }");
        } else {
            lbtyp = val->v.list.btyp;
            (*dumpfn)("{");
            for (listmem = (ncx_lmem_t *)
                     dlq_firstEntry(&val->v.list.memQ);
                 listmem != NULL;
                 listmem = (ncx_lmem_t *)dlq_nextEntry(listmem)) {

                if (startindent >= 0) {
                    (*indentfn)(startindent+bump_amount);
                }

                if (typ_is_string(lbtyp)) {
                    if (listmem->val.str) {
                        quotes = val_need_quotes(listmem->val.str);
                        if (quotes) {
                            (*dumpfn)("%c", VAL_QUOTE_CH);
                        }
                        (*dumpfn)("%s ", (const char *)listmem->val.str);
                        if (quotes) {
                            (*dumpfn)("%c", VAL_QUOTE_CH);
                        }
                    }
                } else if (typ_is_number(lbtyp)) {
                    switch (dumpmode) {
                    case DUMP_VAL_STDOUT:
                        stdout_num(lbtyp, &listmem->val.num);
                        break;
                    case DUMP_VAL_LOG:
                        ncx_printf_num(&listmem->val.num, lbtyp);
                        break;
                    case DUMP_VAL_ALT_LOG:
                        ncx_alt_printf_num(&listmem->val.num, lbtyp);
                        break;
                    default:
                        SET_ERROR(ERR_INTERNAL_VAL);
                    }
                    (*dumpfn)(" ");
                } else {
                    switch (lbtyp) {
                    case NCX_BT_ENUM:
                        if (listmem->val.enu.name) {
                            (*dumpfn)("%s ",
                                      (const char *)listmem->val.enu.name);
                        }
                        break;
                    case NCX_BT_BITS:
                        (*dumpfn)("%s ", (const char *)listmem->val.str);
                        break;
                    case NCX_BT_BOOLEAN:
                        (*dumpfn)("%s ",
                                  (listmem->val.boo) ? 
                                  NCX_EL_TRUE : NCX_EL_FALSE);
                        break;
                    default:
                        SET_ERROR(ERR_INTERNAL_VAL);
                    }
                }
            }
            (*indentfn)(startindent);
            (*dumpfn)("}");
        }
        break;
    case NCX_BT_LIST:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:   // should not happen
    case NCX_BT_CASE:    // should not happen
        (*dumpfn)("{");
        for (chval = (val_value_t *)dlq_firstEntry(&val->v.childQ);
             chval != NULL;
             chval = (val_value_t *)dlq_nextEntry(chval)) {
            val_dump_value_max(chval, 
                               startindent+bump_amount,
                               indent_amount,
                               dumpmode,
                               display_mode,
                               with_meta,
                               configonly);
        }
        (*indentfn)(startindent);
        (*dumpfn)("}");
        break;
    case NCX_BT_EXTERN:
        (*dumpfn)("{");
        (*indentfn)(startindent);

        switch (dumpmode) {
        case DUMP_VAL_STDOUT:
            stdout_extern(val->v.fname);
            break;
        case DUMP_VAL_LOG:
            dump_extern(val->v.fname);
            break;
        case DUMP_VAL_ALT_LOG:
            dump_alt_extern(val->v.fname);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        
        (*indentfn)(startindent);
        (*dumpfn)("}");
        break;
    case NCX_BT_INTERN:
        (*dumpfn)("{");
        (*indentfn)(startindent);

        switch (dumpmode) {
        case DUMP_VAL_STDOUT:
            stdout_intern(val->v.intbuff);
            break;
        case DUMP_VAL_LOG:
            dump_intern(val->v.intbuff);
            break;
        case DUMP_VAL_ALT_LOG:
            dump_alt_intern(val->v.intbuff);
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        (*indentfn)(startindent);
        (*dumpfn)("}");
        break;
    default:
        (*errorfn)("\nval: illegal btype (%d)", btyp);
    }    

    /* dump the metadata queue if non-empty */
    if (with_meta) {
        metaQ = val_get_metaQ(val);
        if (metaQ && !dlq_empty(metaQ)) {
            if (startindent >= 0) {
                (*indentfn)(startindent+bump_amount);
            }

            (*dumpfn)("%s.metaQ ", val->name);
            for (metaval = val_get_first_meta(metaQ);
                 metaval != NULL;
                 metaval = val_get_next_meta(metaval)) {

                val_dump_value_max(metaval, 
                                   startindent+(2*bump_amount),
                                   indent_amount,
                                   dumpmode,
                                   display_mode,
                                   with_meta,
                                   configonly);
            }
        }
    }

}   /* val_dump_value_max */


/********************************************************************
* FUNCTION val_set_string
* 
* set a generic string using the builtin string typdef
* Set an initialized val_value_t as a simple type
* namespace set to 0 !!!
* use after calling val_new_value
*
* INPUTS:
*    val == value to set
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
status_t 
    val_set_string (val_value_t  *val,
                    const xmlChar *valname,
                    const xmlChar *valstr)
{
#ifdef DEBUG
    if (!val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (valname) {
        return val_set_simval_str(val, 
                                  typ_get_basetype_typdef(NCX_BT_STRING),
                                  0, 
                                  valname, 
                                  xml_strlen(valname), 
                                  valstr);
    } else {
        return val_set_simval_str(val, 
                                  typ_get_basetype_typdef(NCX_BT_STRING),
                                  0, 
                                  NULL, 
                                  0, 
                                  valstr);

    }

}  /* val_set_string */


/********************************************************************
* FUNCTION val_set_string2
* 
* set a string with any typdef
* Set an initialized val_value_t as a simple type
*
* Will check if the string is OK for the typdef!
*
* INPUTS:
*    val == value to set
*    valname == name of simple value
*    typdef == parm typdef (may be NULL)
*    valstr == simple value encoded as a string
*    valstrlen == length of valstr to use
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
*
* RETURNS:
*    status
*********************************************************************/
status_t 
    val_set_string2 (val_value_t  *val,
                     const xmlChar *valname,
                     typ_def_t *typdef,
                     const xmlChar *valstr,
                     uint32 valstrlen)
{
    status_t  res;
    xmlChar  *temp;

#ifdef DEBUG
    if (!val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (typdef) {
        val->typdef = typdef;
        val->btyp = typ_get_basetype(typdef);
    } else {
        val->typdef = typ_get_basetype_typdef(NCX_BT_STRING);
        val->btyp = NCX_BT_STRING;
    }
    //val->nsid = 0;

    switch (val->btyp) {
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
        if (valname && !val->name) {
            if (val->dname) {
                SET_ERROR(ERR_INTERNAL_VAL);
                m__free(val->dname);
            }
            val->dname = xml_strdup(valname);
            if (!val->dname) {
                return ERR_INTERNAL_MEM;
            }
            val->name = val->dname;
        }

        if (valstr) {
            VAL_STR(val) = xml_strndup(valstr, valstrlen);
        } else {
            VAL_STR(val) = xml_strdup(EMPTY_STRING);
        }

        if (!VAL_STR(val)) {
            res = ERR_INTERNAL_MEM;
        } else {
            res = NO_ERR;
        }
        break;
    case NCX_BT_LEAFREF:
    default:
        if (valstr) {
            temp = xml_strndup(valstr, valstrlen);
        } else {
            temp = xml_strdup(EMPTY_STRING);
        }
        if (temp) {
            res = val_set_simval(val, typdef, val->nsid, NULL, temp);
            m__free(temp);
        } else {
            res = ERR_INTERNAL_MEM;
        }
    }
    return res;

}  /* val_set_string2 */


/********************************************************************
* FUNCTION val_reset_empty
* 
* Recast an already initialized value as an NCX_BT_EMPTY
* clean a value and set it to empty type
* used by yangcli to delete leafs
*
* INPUTS:
*    val == value to set
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
status_t 
    val_reset_empty (val_value_t  *val)
{
#ifdef DEBUG
    if (!val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    
    return val_set_simval(val,
                          typ_get_basetype_typdef(NCX_BT_EMPTY),
                          val_get_nsid(val),
                          val->name,
                          NULL);

}  /* val_reset_empty */


/********************************************************************
* FUNCTION val_set_simval
* 
* set any simple value with any typdef
* Set an initialized val_value_t as a simple type
*
* INPUTS:
*    val == value to set
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
status_t 
    val_set_simval (val_value_t  *val,
                    typ_def_t    *typdef,
                    xmlns_id_t    nsid,
                    const xmlChar *valname,
                    const xmlChar *valstr)
{
#ifdef DEBUG
    if (!val || !typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (valname) {
        return val_set_simval_str(val, 
                                  typdef, 
                                  nsid, 
                                  valname, 
                                  xml_strlen(valname), 
                                  valstr);
    } else {
        return val_set_simval_str(val, 
                                  typdef, 
                                  nsid, 
                                  NULL, 
                                  0, 
                                  valstr);
    }

}  /* val_set_simval */


/********************************************************************
* FUNCTION val_set_simval_str
* 
* set any simple value with any typdef, and a counted string
* Set an initialized val_value_t as a simple type
*
* The string value will be converted to a value
* struct format and checked against the provided typedef
*
* Handles the following data types:
* 
*  NCX_BT_INT8
*  NCX_BT_INT16
*  NCX_BT_INT32
*  NCX_BT_INT64
*  NCX_BT_UINT8
*  NCX_BT_UINT16
*  NCX_BT_UINT32
*  NCX_BT_UINT64
*  NCX_BT_DECIMAL64
*  NCX_BT_FLOAT64
*  NCX_BT_BINARY
*  NCX_BT_STRING
*  NCX_BT_INSTANCE_ID
*  NCX_BT_ENUM
*  NCX_BT_EMPTY
*  NCX_BT_BOOLEAN
*  NCX_BT_SLIST
*  NCX_BT_BITS
*  NCX_BT_UNION
*  NCX_BT_LEAFREF
*  NCX_BT_IDREF
*
* INPUTS:
*    val == value to set
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valnamelen == length of name string to compare
*    valstr == simple value encoded as a string
*
* OUTPUTS:
*    *val is filled in if return NO_ERR
* RETURNS:
*  status
*********************************************************************/
status_t 
    val_set_simval_str (val_value_t  *val,
                        typ_def_t *typdef,
                        xmlns_id_t    nsid,
                        const xmlChar *valname,
                        uint32 valnamelen,
                        const xmlChar *valstr)
{
    const xmlChar        *localname;
    const ncx_identity_t *identity;
    obj_template_t       *leafobj, *objroot;
    xpath_pcb_t          *xpathpcb;
    status_t              res;
    uint32                ulen;
    xmlns_id_t            qname_nsid;
    boolean               startsimple;
    xpath_source_t        xpath_source;

#ifdef DEBUG
    if (!val || !typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val->btyp != NCX_BT_NONE) {
        startsimple = typ_is_simple(val->btyp);
    } else {
        startsimple = TRUE;
    }

    /* clean the old value even if it was ANY */
    clean_value(val, FALSE);

    res = NO_ERR;
    val->btyp = typ_get_basetype(typdef);

    if (!startsimple) {
        /* clear out the childQ so it does not appear
         * to be a bogus string that needs to be cleaned
         */
        memset(&val->v.childQ, 0x0, sizeof(dlq_hdr_t));
    }

    if (!(val->btyp == NCX_BT_INSTANCE_ID ||
          typ_is_schema_instance_string(typdef) ||
          typ_is_xpath_string(typdef))) {
          
        res = val_simval_ok(typdef, valstr);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* only set name if it is not already set */
    if (!val->name && valname) {
        val->dname = xml_strndup(valname, valnamelen);
        if (!val->dname) {
            return ERR_INTERNAL_MEM;
        }
        val->name = val->dname;
    }

    /* set the object to the generic string if not set */
    if (!val->obj) {
        val->obj = ncx_get_gen_string();
    }

    /* set these fields even if already set by 
     * val_init_from_template
     */
    val->nsid = nsid;
    val->typdef = typdef;
    xpath_source = XP_SRC_INSTANCEID;

    /* convert the value string, if any */
    switch (val->btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_FLOAT64:
        if (valstr && *valstr) {
            res = ncx_convert_num(valstr, 
                                  NCX_NF_NONE, 
                                  val->btyp, 
                                  &val->v.num);
        } else {
            res = ERR_NCX_EMPTY_VAL;
        }
        break;
    case NCX_BT_DECIMAL64:
        if (valstr && *valstr) {
            res = ncx_convert_dec64(valstr, 
                                    NCX_NF_NONE, 
                                    typ_get_fraction_digits(val->typdef), 
                                    &val->v.num);
        } else {
            res = ERR_NCX_EMPTY_VAL;
        }
        break;
    case NCX_BT_BINARY:
        ulen = 0;
        if (valstr && *valstr) {
            ulen = xml_strlen(valstr);
        }
        if (!ulen) {
            break;
        }

        /* allocate a binary string as big as the input
         * text string, even though it will be about 25% too big
         */
        val->v.binary.ustr = m__getMem(ulen+1);
        if (!val->v.binary.ustr) {
            res = ERR_INTERNAL_MEM;
        } else {
            /* really a test tool only for binary strings
             * entered at the CLI; use @foo.jpg for
             * raw input of binary files in var_get_script_val
             */
#if 0
            memcpy(val->v.binary.ustr, valstr, ulen);
            val->v.binary.ubufflen = ulen+1;
            val->v.binary.ustrlen = ulen;
#else
          val->v.binary.ubufflen = ulen+1;
          res = b64_decode(valstr, ulen,
                          val->v.binary.ustr, val->v.binary.ubufflen,
                          &val->v.binary.ustrlen);
#endif
        }
        break;
    case NCX_BT_ANY:
        val->btyp = NCX_BT_STRING;
        val->typdef = typ_get_basetype_typdef(NCX_BT_STRING);
        if (valstr) {
            VAL_STR(val) = xml_strdup(valstr);
        } else {
            VAL_STR(val) = xml_strdup(EMPTY_STRING);
        }
        if (!VAL_STR(val)) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case NCX_BT_LEAFREF:
        if (valstr) {
            VAL_STR(val) = xml_strdup(valstr);
        } else {
            VAL_STR(val) = xml_strdup(EMPTY_STRING);
        }
        if (!VAL_STR(val)) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    case NCX_BT_STRING:
        if (val->obj && obj_is_schema_instance_string(val->obj)) {
            xpath_source = XP_SRC_SCHEMA_INSTANCEID;
        } else {
            if (valstr) {
                VAL_STR(val) = xml_strdup(valstr);
            } else {
                VAL_STR(val) = xml_strdup(EMPTY_STRING);
            }
            if (!VAL_STR(val)) {
                res = ERR_INTERNAL_MEM;
            } else if (typ_is_xpath_string(val->typdef) ||
                       (val->obj && obj_is_xpath_string(val->obj))) {

                xpathpcb = xpath_new_pcb(VAL_STR(val), NULL);
                if (!xpathpcb) {
                    res = ERR_INTERNAL_MEM;
                } else {
                    res = xpath1_parse_expr(NULL, 
                                            NULL,
                                            xpathpcb,
                                            XP_SRC_YANG);
                    if (res == NO_ERR) {
                        objroot = ncx_get_gen_root();
                        res = xpath1_validate_expr(objroot->tkerr.mod, 
                                                   objroot, 
                                                   xpathpcb);
                    }
                    if (val->xpathpcb) {
                        xpath_free_pcb(val->xpathpcb);
                    }
                    val->xpathpcb = xpathpcb;
                }
            }
            break;
        }
        /* fall through if this is an ncx:schema-instance string */
    case NCX_BT_INSTANCE_ID:
        if (valstr) {
            VAL_STR(val) = xml_strdup(valstr);
        } else {
            VAL_STR(val) = xml_strdup(EMPTY_STRING);
        }
        if (!VAL_STR(val)) {
            res = ERR_INTERNAL_MEM;
        } else {
            xpathpcb = xpath_new_pcb(VAL_STR(val), NULL);
            if (!xpathpcb) {
                res = ERR_INTERNAL_MEM;
            } else {
                res = xpath_yang_parse_path(NULL, 
                                            NULL,
                                            xpath_source,
                                            xpathpcb);
                if (res == NO_ERR) {
                    leafobj = NULL;
                    res = xpath_yang_validate_path(NULL, 
                                                   ncx_get_gen_root(), 
                                                   xpathpcb, 
                                                   (xpath_source 
                                                    == XP_SRC_INSTANCEID) 
                                                   ? FALSE : TRUE,
                                                   &leafobj);
                }
                if (val->xpathpcb) {
                    xpath_free_pcb(val->xpathpcb);
                }
                val->xpathpcb = xpathpcb;
            }
        }
        break;
    case NCX_BT_IDREF:
        res = val_parse_idref(NULL, 
                              valstr, 
                              &qname_nsid, 
                              &localname, 
                              &identity);
        if (res == NO_ERR) {
            res = val_idref_ok(typdef, 
                               valstr, 
                               qname_nsid, 
                               &localname, 
                               &identity);
            if (res == NO_ERR) {
                val->v.idref.nsid = qname_nsid;
                val->v.idref.identity = identity;
                val->v.idref.name = xml_strdup(localname);
                if (!val->v.idref.name) {
                    res = ERR_INTERNAL_MEM;
                }
            }
        }
        break;
    case NCX_BT_ENUM:
        res = ncx_set_enum(valstr, &val->v.enu);
        break;
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
        /* if supplied, match the flag name against the supplied value */
        if (valstr) {
            if (!xml_strcmp(NCX_EL_TRUE, valstr)) {
                val->v.boo = TRUE;
            } else if (!xml_strcmp(NCX_EL_FALSE, valstr)) {
                val->v.boo = FALSE;
            } else if (!xml_strncmp(valstr, valname, valnamelen)) {
                val->v.boo = TRUE;
            } else {
                res = ERR_NCX_INVALID_VALUE;
            }
        } else {
            /* name indicates presence, so set val to TRUE */
            val->v.boo = TRUE;
        }           
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        res = ncx_set_strlist(valstr, &val->v.list);
        break;
    case NCX_BT_UNION:
        /* set as generic string -- parser as other end will
         * match against actual union types
         */
        val->btyp = NCX_BT_STRING;
        VAL_STR(val) = xml_strdup(valstr);
        if (!VAL_STR(val)) {
            res = ERR_INTERNAL_MEM;
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
        val->btyp = NCX_BT_NONE;
    }

    return res;

}  /* val_set_simval_str */


/********************************************************************
* FUNCTION val_make_simval
* 
* Create and set a val_value_t as a simple type
* same as val_set_simval, but malloc the value first
*
* INPUTS:
*    typdef == typdef of expected type
*    nsid == namespace ID of this field
*    valname == name of simple value
*    valstr == simple value encoded as a string
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*    pointer to malloced and filled in val_value_t struct
*    NULL if some error
*********************************************************************/
val_value_t *
    val_make_simval (typ_def_t    *typdef,
                     xmlns_id_t    nsid,
                     const xmlChar *valname,
                     const xmlChar *valstr,
                     status_t  *res)
{
    val_value_t  *val;

    if (!typdef || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }

    val = val_new_value();
    if (!val) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    if (valname) {
        *res = val_set_simval_str(val, 
                                  typdef, 
                                  nsid, 
                                  valname, 
                                  xml_strlen(valname), 
                                  valstr);
    } else {
        *res = val_set_simval_str(val, 
                                  typdef, 
                                  nsid, 
                                  NULL, 
                                  0, 
                                  valstr);
    }
    return val;

}  /* val_make_simval */


/********************************************************************
* FUNCTION val_make_string
* 
* Malloc and set a val_value_t as a generic NCX_BT_STRING
* namespace set to 0 !!!
*
* INPUTS:
*    nsid == namespace ID to use
*    valname == name of simple value
*    valstr == simple value encoded as a string
*
* RETURNS:
*   malloced val struct filled in; NULL if malloc or strdup failed
*********************************************************************/
val_value_t *
    val_make_string (xmlns_id_t nsid,
                     const xmlChar *valname,
                     const xmlChar *valstr)
{
    val_value_t *val;
    status_t     res;

    val = val_new_value();
    if (!val) {
        return NULL;
    }
    res = val_set_string(val, valname, valstr);
    if (res != NO_ERR) {
        val_free_value(val);
        val = NULL;
    } else {
        val->nsid = nsid;
    }
    return val;

}  /* val_make_string */


/********************************************************************
* FUNCTION val_merge
* 
* Merge src val into dest val (! MUST be same type !)
* Any meta vars in src are NOT merged into dest!!!
*
* This function is not used to merge complex objects
* !!! For typ_is_simple() only !!!
*
* The source may need to be copied in order to merge, for some basetypes
* The dest is not altered unless the source value can be merged completed.
*
* INPUTS:
*    src == val to merge from
*    dest == val to merge into  !!! old value is not saved !!!
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_merge (const val_value_t *src,
               val_value_t *dest)
{
#ifdef DEBUG
    if (!src || !dest) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (!typ_is_simple(src->btyp) || 
        !typ_is_simple(dest->btyp)) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }   
#endif

    status_t res = NO_ERR;
    ncx_btype_t btyp = dest->btyp;
    ncx_iqual_t iqual = typ_get_iqualval_def(dest->typdef);

    /* the mergetype is only set to NCX_MERGE_SORT for type bits
     * it is NCX_MERGE_NONE for all other data types
     * the value NCX_MERGE_FIRST is never used
     */
    ncx_merge_t mergetyp = typ_get_mergetype(dest->typdef);
    if (mergetyp == NCX_MERGE_NONE) {
        mergetyp = NCX_MERGE_LAST;
    }

    boolean dupsok = FALSE;
    ncx_list_t *list_copy = NULL;
    switch (iqual) {
    case NCX_IQUAL_1MORE:
    case NCX_IQUAL_ZMORE:
        /* duplicates not allowed in leaf lists 
         * leave the current value in place
         */
        res = SET_ERROR(ERR_NCX_DUP_ENTRY);
        break;
    case NCX_IQUAL_ONE:
    case NCX_IQUAL_OPT:
        /* need to replace the current value or merge a list, etc. */
        switch (btyp) {
        case NCX_BT_ENUM:
        case NCX_BT_EMPTY:
        case NCX_BT_BOOLEAN:
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_INT64:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
        case NCX_BT_UINT64:
        case NCX_BT_DECIMAL64:
        case NCX_BT_FLOAT64:
        case NCX_BT_STRING:
        case NCX_BT_BINARY:
        case NCX_BT_INSTANCE_ID:
        case NCX_BT_LEAFREF:
            res = merge_simple(btyp, src, dest);
            break;
        case NCX_BT_UNION:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            break;
        case NCX_BT_SLIST:
        case NCX_BT_BITS:
            if (btyp == NCX_BT_SLIST) {
                dupsok = val_duplicates_allowed(dest);
            }
            list_copy = ncx_new_list(dest->btyp);
            if (list_copy) {
                res = ncx_copy_list(&src->v.list, list_copy);
                if (res != NO_ERR) {
                    ncx_free_list(list_copy);
                }
            } else {
                res = ERR_INTERNAL_MEM;
            }
            if (res == NO_ERR) {
                ncx_merge_list(list_copy, &dest->v.list, mergetyp, dupsok);
                ncx_free_list(list_copy);
            }
            break;
        case NCX_BT_ANY:
        case NCX_BT_CONTAINER:
        case NCX_BT_LIST:
        case NCX_BT_CHOICE:
        case NCX_BT_CASE:
            res = SET_ERROR(ERR_INTERNAL_VAL);
            break;
        default:
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }

        /* copy the editvars struct to the leaf */
        /*** REMOVING SINCE SOURCE IS SAVED ***/
        //if (res == NO_ERR) {
        //   res = copy_editvars(src, dest);
        //}
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* set or clear the set-by-default flag;
     * normalize VAL_FL_WITHDEF by converting to VAL_FL_DEFSET here */
    if (res == NO_ERR) {
        if (val_set_by_default(src)) {
            dest->flags |= VAL_FL_DEFSET;
        } else {
            dest->flags &= ~VAL_FL_DEFSET;
        }
        dest->flags &= ~VAL_FL_WITHDEF;
    }

    return res;

}  /* val_merge */


/********************************************************************
* FUNCTION val_clone
* 
* Clone a specified val_value_t struct and sub-trees
*
* INPUTS:
*    val == value to clone
*
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
val_value_t *
    val_clone (const val_value_t *val)
{
    status_t res;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return clone_test(val, NULL, TRUE, &res);

}  /* val_clone */


/********************************************************************
* FUNCTION val_clone2
* 
* Clone a specified val_value_t struct and sub-trees
* but not the editvars
*
* INPUTS:
*    val == value to clone
*   
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
val_value_t *
    val_clone2 (const val_value_t *val)
{
    status_t res;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return clone_test(val, NULL, FALSE, &res);

}  /* val_clone2 */


/********************************************************************
* FUNCTION val_clone_config_data
* 
* Clone a specified val_value_t struct and sub-trees
* Filter with the val_is_config_data callback function
* pass in a config node, such as <config> root
* will call clone_test with the val_is_config_data
* callbacck function
*
* DOES NOT CLONE THE EDITVARS!!!!
*
* INPUTS:
*    val == config data value to clone
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   clone of val, or NULL if a malloc failure
*********************************************************************/
val_value_t *
    val_clone_config_data (const val_value_t *val,
                           status_t *res)
{
#ifdef DEBUG
    if (!val || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return clone_test(val, val_is_config_data, FALSE, res);

}  /* val_clone_config_data */


/********************************************************************
* FUNCTION val_replace
* 
* Replace a specified val_value_t struct and sub-trees
* !!! this can be destructive to the source 'val' parameter !!!!
*
* INPUTS:
*    val == value to clone from
*    copy == address of value to replace
*
* OUTPUTS:
*   *copy has been deleted and reforms with the contents of 'val'
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_replace (val_value_t *val,
                 val_value_t *copy)
{
    xmlChar    *buffer;
    uint32      len;
    status_t    res;

#ifdef DEBUG
    if (!val || !copy) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    if (val->btyp == NCX_BT_EXTERN) {
        clean_value(copy, TRUE);
        val_init_from_template(copy, val->obj);
        copy->btyp = NCX_BT_EXTERN;
        copy->v.fname = val->v.fname;
        val->v.fname = NULL;
        res = copy_editvars(val, copy);
        return res;
    } else if (!typ_is_simple(val->btyp)) {
        clean_value(copy, TRUE);
        val_init_from_template(copy, val->obj);
        val_move_children(val, copy);
        val_set_name(copy, val->name, xml_strlen(val->name));
        copy->nsid = val->nsid;
        if (copy->btyp == NCX_BT_LIST) {
            res = val_gen_index_chain(copy->obj, copy);
        }
        if (res == NO_ERR) {
            res = copy_editvars(val, copy);
        }
        return res;
    }

    buffer = NULL;

    if (typ_is_string(val->btyp) && typ_is_string(copy->btyp)) {
        if (copy->v.str) {
            ncx_clean_str(&copy->v.str);
        }
        copy->v.str = val->v.str;
        val->v.str = NULL;
        copy->btyp = val->btyp;
    } else {
        res = val_sprintf_simval_nc(NULL, val, &len);
        if (res == NO_ERR) {
            buffer = m__getMem(len+1);
            if (!buffer) {
                return ERR_INTERNAL_MEM;
            }
            res = val_sprintf_simval_nc(buffer, val, &len);
            if (res == NO_ERR) {
                res = val_set_simval(copy,
                                     val->typdef,
                                     val->nsid,
                                     val->name,
                                     buffer);
            }
        }
        if (buffer) {
            m__free(buffer);
        }
    }

    if (res == NO_ERR) {
        res = copy_editvars(val, copy);
    }

    copy->flags &= ~VAL_FL_DEFSET;

    return res;
    
}  /* val_replace */


/********************************************************************
* FUNCTION val_replace_str
* 
* Replace a specified val_value_t struct with a string type
*
* INPUTS:
*    str == value to clone from; may be NULL
*    stringlen == number of chars to use from str, if not NULL
*    copy == address of value to replace
*
* OUTPUTS:
*   *copy has been deleted and reforms with the contents of 'val'
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_replace_str (const xmlChar *str,
                     uint32 stringlen,
                     val_value_t *copy)
{
    val_value_t *strval;
    xmlChar     *dumstr;
    status_t     res;

#ifdef DEBUG
    if (copy == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    strval = val_make_string(copy->nsid, 
                             copy->name, 
                             (const xmlChar *)"dummy");
    if (strval == NULL) {
        return ERR_INTERNAL_MEM;
    }
    
    dumstr = xml_strndup(str, stringlen);
    if (dumstr == NULL) {
        val_free_value(strval);
        return ERR_INTERNAL_MEM;
    }
    m__free(strval->v.str);
    strval->v.str = dumstr;

    res = val_replace(strval, copy);
    val_free_value(strval);
    return res;

}  /* val_replace_str */



/********************************************************************
* FUNCTION val_add_meta
*
*   Add a meta value node to a parent value node
*   Simply makes a new last meta!!!
*
* INPUTS:
*    child == node to store in the parent
*    parent == complex value node with a childQ
*
*********************************************************************/
void
    val_add_meta (val_value_t *meta,
                   val_value_t *parent)
{
#ifdef DEBUG
    if (meta == NULL || parent == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    dlq_enque(meta, &parent->metaQ);

}   /* val_add_meta */

/********************************************************************
* FUNCTION val_add_child
* 
*   Add a child value node to a parent value node
*   Simply makes a new last child!!!
*   Does not check siblings!!!  
*   Relies on val_set_canonical_order
*
*   To modify existing extries, use val_add_child_sorted instead!!
*
* INPUTS:
*    child == node to store in the parent
*    parent == complex value node with a childQ
*
*********************************************************************/
void
    val_add_child (val_value_t *child,
                   val_value_t *parent)
{
#ifdef DEBUG
    if (child == NULL || parent == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    child->parent = parent;
    dlq_enque(child, &parent->v.childQ);

}   /* val_add_child */


/********************************************************************
* FUNCTION val_add_child_sorted
* 
*   Add a child value node to a parent value node
*   in the proper place
*
* INPUTS:
*    child == node to store in the parent
*    parent == complex value node with a childQ
*
*********************************************************************/
void
    val_add_child_sorted (val_value_t *child,
                          val_value_t *parent)
{
    assert( child && "child is NULL!" );
    assert( parent && "parent is NULL!" );

    child->parent = parent;
    dlq_hdr_t *childQ = &parent->v.childQ;

    /* check new first entry */
    if (dlq_empty(childQ)) {
        dlq_enque(child, childQ);
        return;
    }

    val_value_t *curval = NULL;
    obj_template_t *newobj = child->obj;
    xmlns_id_t parentid = val_get_nsid(parent);
    xmlns_id_t childid = val_get_nsid(child);
    boolean sysorder = obj_is_system_ordered(newobj);
    int ret = 0;

    /* The current set of sibling nodes needs to
     * be searched to determine where to insert this child
     */
    if (obj_is_root(parent->obj)) {
        /* adding objects to the root is different; need
         * to use alphabetical order, not schema order
         * since submodules blur the top-level object
         * order within a module namespace
         */
        for (curval = val_get_first_child(parent);
             curval != NULL;
             curval = val_get_next_child(curval)) {

            /* check same type of sibling cornercase
             * should only happen if the child is
             * type list or leaf-list
             */
            if (newobj == curval->obj) {
                /* make a new sorted or last one of these entries */
                boolean syssorted = ncx_get_system_sorted();
                boolean done = FALSE;

                while (!done) {
                    if (sysorder && syssorted) {
                        if (newobj->objtype == OBJ_TYP_LIST) {
                            ret = val_index_compare(child, curval);
                        } else {
                            ret = val_compare(child, curval);
                        }
                        if (ret < 0) {
                            dlq_insertAhead(child, curval);
                            return;
                        }
                    }
                    val_value_t *nextchild = val_get_next_child(curval);
                    if (nextchild == NULL || nextchild->obj != child->obj) {
                        done = TRUE;
                    } else {
                        curval = nextchild;
                    }
                }
                dlq_insertAfter(child, curval);
                return;
            }

            ret = xml_strcmp(child->name, curval->name);
            if (ret < 0) {
                dlq_insertAhead(child, curval);
                return;
            } else if (ret == 0) {
                ret = xml_strcmp(val_get_mod_name(child),
                                 val_get_mod_name(curval));
                if (ret < 0) {
                    dlq_insertAhead(child, curval);
                    return;
                    /* same name, insert in module alphabetical order */
                }
            }
        }

        /* make new last entry */
        dlq_enque(child, childQ);
    } else if (parent->obj->objtype == OBJ_TYP_ANYXML) {
        /* there is no schema order to check, so see if this
         * child already exists
         */
        curval = val_find_child(parent, val_get_mod_name(child), child->name);
        if (curval != NULL) {
            /* make new last instance of this child node */
            val_value_t *saveval = NULL;
            while (curval != NULL) {
                saveval = curval;
                curval = val_find_next_child(parent, val_get_mod_name(child),
                                             child->name, curval);
            }
            dlq_insertAfter(child, saveval);
        } else {
            /* make new last child; first one of these */
            dlq_enque(child, childQ);
        }
    } else {
        /* normal container or list */
        for (curval = val_get_first_child(parent);
             curval != NULL;
             curval = val_get_next_child(curval)) {

            /* check same type of sibling cornercase
             * should only happen if the child is
             * type list or leaf-list
             */
            if (newobj == curval->obj) {
                /* make a new last one of these entries */
                boolean syssorted = ncx_get_system_sorted();
                boolean done = FALSE;

                while (!done) {
                    if (sysorder && syssorted) {
                        if (newobj->objtype == OBJ_TYP_LIST) {
                            ret = val_index_compare(child, curval);
                        } else {
                            ret = val_compare(child, curval);
                        }
                        if (ret < 0) {
                            dlq_insertAhead(child, curval);
                            return;
                        }
                    }

                    val_value_t *nextchild = val_get_next_child(curval);
                    if (nextchild == NULL || nextchild->obj != child->obj) {
                        done = TRUE;
                    } else {
                        curval = nextchild;
                    }              
                }

                /* make a new last instance of this node type */
                dlq_insertAfter(child, curval);
                return;
            }

            /* simple test; since native children
             * will be before external augmented children;
             * any native node will insert ahead of such 
             * an augment node
             */
            if (val_get_nsid(curval) != parentid && childid == parentid) {
                dlq_insertAhead(child, curval);
                return;
            }

            /* new node and current node are different so
             * check if the current object is after the
             * new object in the schema order.  If so,
             * then insert ahead of this node
             *
             * the object siblings are not numbered, so
             * a linear search is used here
             */
            obj_template_t *testobj = obj_next_child_deep(child->obj);
            for (; testobj != NULL; testobj = obj_next_child_deep(testobj)) {
                if (testobj == curval->obj) {
                    /* insert child ahead of this node
                     * which occurs after it in schema order
                     */
                    dlq_insertAhead(child, curval);
                    return;
                }
            }
        }

        /* make a new last entry */
        dlq_enque(child, childQ);
    }

}   /* val_add_child_sorted */


/********************************************************************
* FUNCTION val_insert_child
* 
*   Insert a child value node to a parent value node
*
* INPUTS:
*    child == node to store in the parent
*    current == current node to insert after; 
*               NULL to make new first entry
*    parent == complex value node with a childQ
*
*********************************************************************/
void
    val_insert_child (val_value_t *child,
                      val_value_t *current,
                      val_value_t *parent)
{
#ifdef DEBUG
    if (child == NULL || parent == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    child->parent = parent;
    if (current) {
        dlq_insertAfter(child, current);
    } else {
        val_add_child_sorted(child, parent);
    }

}   /* val_insert_child */


/********************************************************************
* FUNCTION val_remove_child
* 
*   Remove a child value node from its parent value node
*
* INPUTS:
*    child == node to store in the parent
*
*********************************************************************/
void
    val_remove_child (val_value_t *child)
{
#ifdef DEBUG
    if (!child) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    dlq_remove(child);
    child->parent = NULL;

}   /* val_remove_child */


/********************************************************************
* FUNCTION val_swap_child
* 
*   Swap a child value node with a current value node
*
* INPUTS:
*    newchild == node to store in the place of the curchild
*    curchild == node to remove from the parent
*
*********************************************************************/
void
    val_swap_child (val_value_t *newchild,
                    val_value_t *curchild)
{
#ifdef DEBUG
    if (!newchild || !curchild) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    newchild->parent = curchild->parent;
    newchild->getcb = curchild->getcb;

    dlq_swap(newchild, curchild);

    curchild->parent = NULL;

}   /* val_swap_child */


/********************************************************************
* FUNCTION val_first_child_match
* 
* Get the first instance of the corresponding child node 
* 
* INPUTS:
*    parent == parent value to check
*    child == child value to find (e.g., from a NETCONF PDU) 
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_first_child_match (val_value_t  *parent,
                           val_value_t *child)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !child) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        /* check the node if the QName matches */
        if (val->nsid == child->nsid &&
            !xml_strcmp(val->name, child->name)) {

            if (val->btyp == NCX_BT_LIST) {
                /* match the instance identifiers, if any */
                if (val_index_match(child, val)) {
                    return val;
                }
            } else if (val->obj->objtype == OBJ_TYP_LEAF_LIST) {
                if (val->btyp == child->btyp) {
                    /* find the leaf-list with the same value */
                    if (!val_compare(val, child)) {
                        return val;
                    }
                } else {
                    /* match any value; if this is a subtree
                     * filter test, it is not for a content match
                     * node
                     */
                    return val;
                }
            } else {
                /* can only be this one instance */
                return val;
            }
        }
    }

    return NULL;

}  /* val_first_child_match */


/********************************************************************
* FUNCTION val_next_child_match
* 
* Get the next instance of the corresponding child node 
* 
* INPUTS:
*    parent == parent value to check
*    child == child value to find (e.g., from a NETCONF PDU) 
*    curmatch == current child of parent that matched
*                
* RETURNS:
*   pointer to the next child match if found or NULL if not found
*********************************************************************/
val_value_t *
    val_next_child_match (val_value_t *parent,
                          val_value_t *child,
                          val_value_t *curmatch)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !child || !curmatch) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    for (val = (val_value_t *)dlq_nextEntry(curmatch);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        /* check the node if the QName matches */
        if (val->nsid == child->nsid &&
            !xml_strcmp(val->name, child->name)) {

            if (val->btyp == NCX_BT_LIST) {
                /* match the instance identifiers, if any */
                if (val_index_match(child, val)) {
                    return val;
                }
            } else if (val->obj->objtype == OBJ_TYP_LEAF_LIST) {
                if (val->btyp == child->btyp) {
                    /* find the leaf-list with the same value */
                    if (!val_compare(val, child)) {
                        return val;
                    }
                } else {
                    /* match any value; if this is a subtree
                     * filter test, it is not for a content match
                     * node
                     */
                    return val;
                }
            } else {
                /* can only be this one instance */
                return val;
            }
        }
    }

    return NULL;

}  /* val_next_child_match */


/********************************************************************
* FUNCTION val_get_first_child
* 
* Get the child node
* 
* INPUTS:
*    parent == parent complex type to check
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_get_first_child (const val_value_t  *parent)
{
#ifdef DEBUG
    if (!parent) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    val_value_t *val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
    for (; val != NULL; val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        return val;
    }

    return NULL;

}  /* val_get_first_child */


/********************************************************************
* FUNCTION val_get_next_child
* 
* Get the next child node
* 
* INPUTS:
*    curchild == current child node
*
* RETURNS:
*   pointer to the next child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_get_next_child (const val_value_t  *curchild)
{
#ifdef DEBUG
    if (!curchild) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    val_value_t *val = (val_value_t *)dlq_nextEntry(curchild);
    for (; val != NULL; val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        return val;
    }

    return NULL;

}  /* val_get_next_child */


/********************************************************************
* FUNCTION val_find_child
* 
* Find the first instance of the specified child node
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_find_child (const val_value_t  *parent,
                    const xmlChar *modname,
                    const xmlChar *childname)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !childname) {
      log_error("Parameter error: modname [%s], childname [%s]", (modname ? (const char*) modname:"NULL"),(childname ? (const char*)childname : "NULL"));
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        if (modname && 
            xml_strcmp(modname, 
                       val_get_mod_name(val))) {
            continue;
        }
        if (!xml_strcmp(val->name, childname)) {
            return val;
        }
    }
    return NULL;

}  /* val_find_child */


/********************************************************************
* FUNCTION val_find_child_que
* 
* Find the first instance of the specified child node in the
* specified child Q
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_find_child_que (const dlq_hdr_t *childQ,
                        const xmlChar *modname,
                        const xmlChar *childname)
{
    val_value_t *val;

#ifdef DEBUG
    if (!childQ || !childname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (val = (val_value_t *)dlq_firstEntry(childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        if (modname && 
            xml_strcmp(modname, 
                       val_get_mod_name(val))) {
            continue;
        }
        if (!xml_strcmp(val->name, childname)) {
            return val;
        }
    }
    return NULL;

}  /* val_find_child_que */


/********************************************************************
* FUNCTION val_match_child
* 
* Match the first instance of the specified child node
*
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_match_child (const val_value_t  *parent,
                     const xmlChar *modname,
                     const xmlChar *childname)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !childname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        if (modname && 
            xml_strcmp(modname, val_get_mod_name(val))) {
            continue;
        }
        if (!xml_strncmp(val->name, childname,
                         xml_strlen(childname))) {
            return val;
        }
    }
    return NULL;

}  /* val_match_child */


/********************************************************************
* FUNCTION val_find_next_child
* 
* Find the next instance of the specified child node
* 
* INPUTS:
*    parent == parent complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    childname == name of child node to find
*    curchild == current child of this object type to start search
*
* RETURNS:
*   pointer to the child if found or NULL if not found
*********************************************************************/
val_value_t *
    val_find_next_child (const val_value_t  *parent,
                         const xmlChar *modname,
                         const xmlChar *childname,
                         const val_value_t *curchild)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !childname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }

    for (val = (val_value_t *)dlq_nextEntry(curchild);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {
        if (VAL_IS_DELETED(val)) {
            continue;
        }
        if (modname && 
            xml_strcmp(modname, 
                       val_get_mod_name(val))) {
            continue;
        }
        if (!xml_strcmp(val->name, childname)) {
            return val;
        }
    }
    return NULL;

}  /* val_find_next_child */


/********************************************************************
* FUNCTION val_first_child_name
* 
* Get the first corresponding child node instance, by name
* find first -- really for resolve index function
* 
* INPUTS:
*    parent == parent complex type to check
*    name == child name to find
*
* RETURNS:
*   pointer to the FIRST match if found, or NULL if not found
*********************************************************************/
val_value_t *
    val_first_child_name (val_value_t  *parent,
                          const xmlChar *name)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }
        
    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, name)) {
            return val;
        }
    }

    return NULL;

}  /* val_first_child_name */


/********************************************************************
* FUNCTION val_first_child_qname
* 
* Get the first corresponding child node instance, by QName
* 
* INPUTS:
*    parent == parent complex type to check
*    nsid == namespace ID to use, 0 for any
*    name == child name to find
*
* RETURNS:
*   pointer to the first match if found, or NULL if not found
*********************************************************************/
val_value_t *
    val_first_child_qname (val_value_t *parent,
                           xmlns_id_t   nsid,
                           const xmlChar *name)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }
        
    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        if (!xmlns_ids_equal(nsid, val->nsid)) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, name)) {
            return val;
        }
    }

    return NULL;

}  /* val_first_child_qname */


/********************************************************************
* FUNCTION val_next_child_qname
* 
* Get the next corresponding child node instance, by QName
* 
* INPUTS:
*    parent == parent complex type to check
*    nsid == namespace ID to use, 0 for any
*    name == child name to find
*    curchild == current child match to start from
*
* RETURNS:
*   pointer to the next match if found, or NULL if not found
*********************************************************************/
val_value_t *
    val_next_child_qname (val_value_t *parent,
                          xmlns_id_t   nsid,
                          const xmlChar *name,
                          val_value_t *curchild)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !name || !curchild) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }
        
    for (val = (val_value_t *)dlq_nextEntry(curchild);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        if (!xmlns_ids_equal(nsid, val->nsid)) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, name)) {
            return val;
        }
    }

    return NULL;

}  /* val_next_child_qname */


/********************************************************************
* FUNCTION val_first_child_string
* 
* find first name value pair
* Get the first corresponding child node instance, by name
* and by string value.
* Child node must be a base type of 
*   NCX_BT_STRING
*   NCX_BT_INSTANCE_ID
*   NCX_BT_LEAFREF
*
* INPUTS:
*    parent == parent complex type to check
*    name == child name to find
*    strval == string value to find 
* RETURNS:
*   pointer to the FIRST match if found, or NULL if not found
*********************************************************************/
val_value_t *
    val_first_child_string (val_value_t  *parent,
                            const xmlChar *name,
                            const xmlChar *strval)
{
    val_value_t *val;

#ifdef DEBUG
    if (!parent || !name || !strval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return NULL;
    }
        
    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, name)) {
            if (typ_is_string(val->btyp)) {
                if (!xml_strcmp(val->v.str, strval)) {
                    return val;
                }
            } else {
                /* requested child node is wrong type */
                return NULL;
            }
        }
    }

    return NULL;

}  /* val_first_child_string */


/********************************************************************
* FUNCTION val_child_cnt
* 
* Get the number of child nodes present
* get number of child nodes present -- for choice checking
* 
* INPUTS:
*    parent == parent complex type to check
*
* RETURNS:
*   the number of child nodes found
*********************************************************************/
uint32
    val_child_cnt (val_value_t  *parent)
{
    val_value_t *val;
    uint32       cnt;

#ifdef DEBUG
    if (!parent) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return 0;
    }
        
    cnt = 0;
    for (val = (val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        cnt++;
    }
    return cnt;

}  /* val_child_cnt */


/********************************************************************
* FUNCTION val_child_inst_cnt
* 
* Get the corresponding child instance count by name
* get instance count -- for instance qualifer checking
* 
* INPUTS:
*    parent == parent complex type to check
*    modname == module name defining the child (may be NULL)
*    name == child name to find and count
*
* RETURNS:
*   the instance count
*********************************************************************/
uint32
    val_child_inst_cnt (const val_value_t  *parent,
                        const xmlChar *modname,
                        const xmlChar *name)
{
    const val_value_t *val;
    uint32       cnt;

#ifdef DEBUG
    if (!parent || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (!typ_has_children(parent->btyp)) {
        return 0;
    }
        
    cnt = 0;
    for (val = (const val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (const val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        if (modname &&
            xml_strcmp(modname, val_get_mod_name(val))) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, name)) {
            cnt++;
        } 
    }

    return cnt;

}  /* val_child_inst_cnt */


/********************************************************************
* FUNCTION val_find_all_children
* 
* Find all occurances of the specified node(s)
* within the children of the current node. 
* The walker fn will be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == node to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of child node to find
*              == NULL to match any child name
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
boolean
    val_find_all_children (val_walker_fn_t walkerfn,
                           void *cookie1,
                           void *cookie2,
                           val_value_t *startnode,
                           const xmlChar *modname,
                           const xmlChar *name,
                           boolean configonly,
                           boolean textmode)
{
    val_value_t *val, *useval;
    boolean      fnresult, fncalled;
    status_t     res;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!typ_has_children(startnode->btyp)) {
        return FALSE;
    }

    if (val_is_virtual(startnode)) {
        res = NO_ERR;
        useval = cache_virtual_value(NULL, startnode, &res);
        if (useval == NULL) {
            return FALSE;
        }
    } else {
        useval = startnode;
    }

    for (val = (val_value_t *)dlq_firstEntry(&useval->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         val, 
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }
    }
    return TRUE;

}  /* val_find_all_children */


/********************************************************************
* FUNCTION val_find_all_ancestors
* 
* Find all the ancestor instances of the specified node
* within the path to root from the current node;
* use the filter criteria provided
*
* INPUTS:
*
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == node to start search at
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of ancestor node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if name == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    val_find_all_ancestors (val_walker_fn_t  walkerfn,
                            void *cookie1,
                            void *cookie2,
                            val_value_t *startnode,
                            const xmlChar *modname,
                            const xmlChar *name,
                            boolean configonly,
                            boolean textmode,
                            boolean orself)
{
    val_value_t *val;
    boolean      fnresult, fncalled;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (orself) {
        val = startnode;
    } else {
        val = startnode->parent;
    }

    while (val) {
        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         val, 
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }
        val = val->parent;
    }
    return TRUE;

}  /* val_find_all_ancestors */


/********************************************************************
* FUNCTION val_find_all_descendants
* 
* Find all occurances of the specified node
* within the current subtree. The walker fn will
* be called for each match.  
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == startnode complex type to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of descendant node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    orself == TRUE if axis is really ancestor-or-self
*              FALSE if axis is ancestor
*    forceall == TRUE to invoke the descendant callbacks
*                even if fncalled was true from the current
*               (parent) node;  FALSE to skip descendants
*               if fncalled was TRUE
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    val_find_all_descendants (val_walker_fn_t walkerfn,
                              void *cookie1,
                              void *cookie2,
                              val_value_t *startnode,
                              const xmlChar *modname,
                              const xmlChar *name,
                              boolean configonly,
                              boolean textmode,
                              boolean orself,
                              boolean forceall)
{
    val_value_t *val, *useval;
    boolean      fncalled, fnresult;
    status_t     res;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (val_is_virtual(startnode)) {
        res = NO_ERR;
        useval = cache_virtual_value(NULL, startnode, &res);
        if (useval == NULL) {
            return res;
        }
    } else {
        useval = startnode;
    }

    if (orself) {
        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         useval,
                                         modname, 
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }
    }

    if (!typ_has_children(startnode->btyp)) {
        return TRUE;
    }

    for (val = (val_value_t *)dlq_firstEntry(&useval->v.childQ);
         val != NULL;
         val = (val_value_t *)dlq_nextEntry(val)) {

        if (VAL_IS_DELETED(val)) {
            continue;
        }

        fncalled = FALSE;
        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         val, 
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }
        if (!fncalled || forceall) {
            fnresult = val_find_all_descendants(walkerfn,
                                                cookie1,
                                                cookie2,
                                                val,
                                                modname,
                                                name, 
                                                configonly,
                                                textmode,
                                                FALSE,
                                                forceall);
            if (!fnresult) {
                return FALSE;
            }
        }
    }
    return TRUE;

}  /* val_find_all_descendants */


/********************************************************************
* FUNCTION val_find_all_pfaxis
* 
* Find all occurances of the specified node
* for the specified preceding or following axis
*
*   preceding::*
*   following::*
*
* within the current subtree. The walker fn will
* be called for each match.  Because the callbacks
* will be done in sequential order, starting from
* the 
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    topnode == topnode used as the relative root for
*               calculating node position
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 1 level is checked, not their descendants
*    textmode == TRUE if just testing for text() nodes
*                name modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    val_find_all_pfaxis (val_walker_fn_t  walkerfn,
                         void *cookie1,
                         void *cookie2,
                         val_value_t  *startnode,
                         const xmlChar *modname,
                         const xmlChar *name,
                         boolean configonly,
                         boolean dblslash,
                         boolean textmode,
                         ncx_xpath_axis_t axis)
{
    val_value_t   *val, *child, *useval;
    boolean        fncalled, fnresult, forward;
    status_t       res;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* check the Q containing the startnode
     * for preceding or following nodes;
     * could be sibling node check or any node check
     */
    switch (axis) {
    case XP_AX_PRECEDING:
        forward = FALSE;
        val = (val_value_t *)dlq_prevEntry(startnode);
        break;
    case XP_AX_FOLLOWING:
        forward = TRUE;
        val = (val_value_t *)dlq_nextEntry(startnode);
        break;
    case XP_AX_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    /* for virtual nodes, it is assumed that the set of 
     * siblings represents real values, but each 
     * value node checked and expanded if a virtual node
     */
    while (val) {

        if (VAL_IS_DELETED(val) ||
            (configonly && !obj_is_config(val->obj))) {
            /* skip this entry */
            if (forward) {
                val = (val_value_t *)dlq_nextEntry(val);
            } else {
                val = (val_value_t *)dlq_prevEntry(val);
            }
            continue;
        }

        if (val_is_virtual(val)) {
            res = NO_ERR;
            useval = cache_virtual_value(NULL, val, &res);
            if (useval == NULL) {
                return res;
            }
        } else {
            useval = val;
        }

        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         useval, 
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }

        if (!fncalled && dblslash) {
            /* if /foo did not get added, than 
             * try /foo/bar, /foo/baz, etc.
             * check all the child nodes even if
             * one of them matches, because all
             * matches are needed with the '//' operator
             */
            for (child = val_get_first_child(useval);
                 child != NULL;
                 child = val_get_next_child(child)) {

                fnresult = 
                    val_find_all_pfaxis(walkerfn, 
                                        cookie1, 
                                        cookie2,
                                        child, 
                                        modname, 
                                        name, 
                                        configonly,
                                        dblslash,
                                        textmode,
                                        axis);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        if (forward) {
            val = (val_value_t *)dlq_nextEntry(val);
        } else {
            val = (val_value_t *)dlq_prevEntry(val);
        }
    }
    return TRUE;

}  /* val_find_all_pfaxis */


/********************************************************************
* FUNCTION val_find_all_pfsibling_axis
* 
* Find all occurances of the specified node
* for the specified axis
*
*   preceding-sibling::*
*   following-sibling::*
*
* within the current subtree. The walker fn will
* be called for each match.  Because the callbacks
* will be done in sequential order, starting from
* the 
*
* If the walker function returns TRUE, then the 
* walk will continue; If FALSE it will terminate right away
*
* INPUTS:
*    walkerfn == callback function to use
*    cookie1 == cookie1 value to pass to walker fn
*    cookie2 == cookie2 value to pass to walker fn
*    startnode == starting sibling node to check
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*
* RETURNS:
*   TRUE if normal termination occurred
*   FALSE if walker fn requested early termination
*********************************************************************/
boolean
    val_find_all_pfsibling_axis (val_walker_fn_t  walkerfn,
                                 void *cookie1,
                                 void *cookie2,
                                 val_value_t  *startnode,
                                 const xmlChar *modname,
                                 const xmlChar *name,
                                 boolean configonly,
                                 boolean dblslash,
                                 boolean textmode,
                                 ncx_xpath_axis_t axis)
{
    val_value_t *val, *useval, *child;
    boolean      fncalled, fnresult, forward;
    status_t     res;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* check the Q containing the startnode
     * for preceding or following nodes;
     */
    switch (axis) {
    case XP_AX_PRECEDING_SIBLING:
        /* execute the callback for all preceding nodes
         * that match the filter criteria 
         */
        val = (val_value_t *)dlq_prevEntry(startnode);
        forward = FALSE;
        break;
    case XP_AX_FOLLOWING_SIBLING:
        val = (val_value_t *)dlq_nextEntry(startnode);
        forward = TRUE;
        break;
    case XP_AX_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    while (val) {
        if (VAL_IS_DELETED(val) ||
            (configonly && !obj_is_config(val->obj))) {
            if (forward) {
                val = (val_value_t *)dlq_nextEntry(val);
            } else {
                val = (val_value_t *)dlq_prevEntry(val);
            }
            continue;
        }

        if (val_is_virtual(val)) {
            res = NO_ERR;
            useval = cache_virtual_value(NULL, val, &res);
            if (useval == NULL) {
                return FALSE;
            }
        } else {
            useval = val;
        }


        fnresult = process_one_valwalker(walkerfn,
                                         cookie1,
                                         cookie2,
                                         useval, 
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (!fnresult) {
            return FALSE;
        }

        if (!fncalled && dblslash) {
            /* if /foo did not get added, than 
             * try /foo/bar, /foo/baz, etc.
             * check all the child nodes even if
             * one of them matches, because all
             * matches are needed with the '//' operator
             */
            for (child = val_get_first_child(useval);
                 child != NULL;
                 child = val_get_next_child(child)) {

                fnresult = 
                    val_find_all_pfsibling_axis(walkerfn, 
                                                cookie1, 
                                                cookie2,
                                                child, 
                                                modname, 
                                                name, 
                                                configonly,
                                                dblslash,
                                                textmode,
                                                axis);
                if (!fnresult) {
                    return FALSE;
                }
            }
        }

        if (forward) {
            val = (val_value_t *)dlq_nextEntry(val);
        } else {
            val = (val_value_t *)dlq_prevEntry(val);
        }
    }
    return TRUE;

}  /* val_find_all_pfsibling_axis */


/********************************************************************
* FUNCTION val_get_axisnode
* 
* Find the specified node based on the context node,
* a context position and an axis
*
*   ancestor::*
*   ancestor-or-self::*
*   child::*
*   descendant::*
*   descendant-or-self::*
*   following::*
*   following-sibling::*
*   preceding::*
*   preceding-sibling::*
*   self::*
*
* INPUTS:
*    startnode == context node to run tests from
*    modname == module name; 
*                the first match in this module namespace
*                will be returned
*            == NULL:
*                 the first match in any namespace will
*                 be  returned;
*    name == name of preceding or following node to find
*              == NULL to match any node name
*    configonly == TRUE to skip over non-config nodes
*                  FALSE to check all nodes
*                  Only used if decname == NULL
*    dblslash == TRUE if all decendents of the preceding
*                 or following nodes should be checked
*                FALSE only 
*    textmode == TRUE if just testing for text() nodes
*                name and modname will be ignored in this mode
*                FALSE if using name and modname to filter
*    axis == axis enum to use
*    position == position to find in the specified axis
*
* RETURNS:
*   pointer to found value or NULL if none
*********************************************************************/
val_value_t *
    val_get_axisnode (val_value_t *startnode,
                      const xmlChar *modname,
                      const xmlChar *name,
                      boolean configonly,
                      boolean dblslash,
                      boolean textmode,
                      ncx_xpath_axis_t axis,
                      int64 position)
{
    finderparms_t    finderparms;
    boolean          fnresult, fncalled, orself;

#ifdef DEBUG
    if (!startnode) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (position <= 0) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    orself = FALSE;
    memset(&finderparms, 0x0, sizeof(finderparms_t));
    finderparms.findpos = position;

    /* check the Q containing the startnode
     * for preceding or following nodes;
     * could be sibling node check or any node check
     */
    switch (axis) {
    case XP_AX_ANCESTOR_OR_SELF:
        orself = TRUE;
        /* fall through */
    case XP_AX_ANCESTOR:
        fnresult = val_find_all_ancestors(position_walker,
                                          &finderparms,
                                          NULL,
                                          startnode,
                                          modname,
                                          name,
                                          configonly,
                                          textmode,
                                          orself);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_ATTRIBUTE:
        /* TBD: attributes not supported */
        return NULL;
    case XP_AX_CHILD:
        fnresult = val_find_all_children(position_walker,
                                         &finderparms,
                                         NULL,
                                         startnode,
                                         modname,
                                         name,
                                         configonly,
                                         textmode);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_DESCENDANT_OR_SELF:
        orself = TRUE;
        /* fall through */
    case XP_AX_DESCENDANT:
        fnresult = val_find_all_descendants(position_walker,
                                            &finderparms,
                                            NULL,
                                            startnode,
                                            modname,
                                            name,
                                            configonly,
                                            textmode,
                                            orself,
                                            FALSE);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_PRECEDING:
    case XP_AX_FOLLOWING:
        fnresult = val_find_all_pfaxis(position_walker,
                                       &finderparms,
                                       NULL,
                                       startnode,
                                       modname,
                                       name,
                                       configonly,
                                       dblslash,
                                       textmode,
                                       axis);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_PRECEDING_SIBLING:
    case XP_AX_FOLLOWING_SIBLING:
        fnresult = val_find_all_pfsibling_axis(position_walker,
                                               &finderparms,
                                               NULL,
                                               startnode,
                                               modname,
                                               name,
                                               configonly,
                                               dblslash,
                                               textmode,
                                               axis);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_NAMESPACE:
        return NULL;
    case XP_AX_PARENT:
        if (!startnode->parent) {
            return NULL;
        }
        /* there can only be one node in this axis,
         * so if the startnode isn't it, then return NULL
         */
        fnresult = process_one_valwalker(position_walker,
                                         &finderparms,
                                         NULL,
                                         startnode->parent,
                                         modname,
                                         name,
                                         configonly,
                                         textmode,
                                         &fncalled);
        if (fnresult) {
            return NULL;
        } else {
            return finderparms.foundval;
        }
    case XP_AX_NONE:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* val_get_axisnode */


/********************************************************************
* FUNCTION val_get_child_inst_id
* 
* Get the instance ID for this child node within the parent context
* 
* INPUTS:
*    parent == parent complex type to check
*    child == child node to find ID for
*
* RETURNS:
*   the instance ID num (1 .. N), or 0 if some error
*********************************************************************/
uint32
    val_get_child_inst_id (const val_value_t  *parent,
                           const val_value_t *child)
{
    const val_value_t *val;
    uint32             cnt;

#ifdef DEBUG
    if (!parent || !child) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
    if (!typ_has_children(parent->btyp)) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
#endif

    cnt = 0;
    for (val = (const val_value_t *)dlq_firstEntry(&parent->v.childQ);
         val != NULL;
         val = (const val_value_t *)dlq_nextEntry(val)) {

        /* do not skip over deleted nodes in case the reason
         * this function is called is to print the instance ID
         * of the node being deleted
         */

        if (xml_strcmp(val_get_mod_name(child),
                       val_get_mod_name(val))) {
            continue;
        }

        /* check the node if the name matches */
        if (!xml_strcmp(val->name, child->name)) {
            cnt++;
            if (val == child) {
                return cnt;
            }
        }
    }

    SET_ERROR(ERR_INTERNAL_VAL);
    return 0;

}  /* val_get_child_inst_id */


/********************************************************************
* FUNCTION val_liststr_count
* 
* Get the number of strings in the list type
* 
* INPUTS:
*    val == value to check
*
* RETURNS:
*  number of list entries; also zero for error
*********************************************************************/
uint32
    val_liststr_count (const val_value_t  *val)
{
    uint32             cnt;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    cnt = 0;
    switch (val->btyp) {
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        cnt = ncx_list_cnt(&val->v.list);
        break;
    default:
        SET_ERROR(ERR_NCX_WRONG_TYPE);
    }

    return cnt;

}  /* val_liststr_count */


/********************************************************************
* FUNCTION val_index_match
* 
* Check 2 val_value structs for the same instance ID
* 
* The node data types must match, and must be
*    NCX_BT_LIST
*
* All index components must exactly match.
* 
* INPUTS:
*    val1 == first value to index match
*    val2 == second value to index match
*
* RETURNS:
*   TRUE if the index chains match
*********************************************************************/
boolean
    val_index_match (const val_value_t *val1,
                     const val_value_t *val2)
{
    assert(val1 && "val1 is NULL!" );
    assert(val2 && "val2 is NULL!" );

    int32 ret = index_match(val1, val2);
    return (ret) ? FALSE : TRUE;

}  /* val_index_match */


/********************************************************************
* FUNCTION val_index_compare
* 
* Check 2 val_value structs for the same instance ID
* 
* The node data types must match, and must be
*    NCX_BT_LIST
*
* INPUTS:
*    val1 == first value to index match
*    val2 == second value to index match
*
* RETURNS:
*   -1 , - or 1 for compare value
*********************************************************************/
int
    val_index_compare (const val_value_t *val1,
                       const val_value_t *val2)
{
    assert(val1 && "val1 is NULL!" );
    assert(val2 && "val2 is NULL!" );

    int32 ret = index_match(val1, val2);
    return ret;

}  /* val_index_compare */


/********************************************************************
* FUNCTION val_compare_max
* 
* Compare 2 val_value_t struct value contents
* Check all or config only
* Check just child nodes or all descendant nodes
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*    configonly == TRUE to compare config=true nodes only
*                  FALSE to compare all nodes
*    childonly == TRUE to look just 1 level for comparison
*                 FALSE to compare all descendant nodes of complex types
*    editing == TRUE to compare for editing
*               FALSE to compare just the values, so a set by
*               default and value=default are the same value
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
int32
    val_compare_max (const val_value_t *val1,
                     const val_value_t *val2,
                     boolean configonly,
                     boolean childonly,
                     boolean editing)
{
    ncx_btype_t  btyp;
    const val_value_t *ch1, *ch2;
    int32        ret;
    xmlns_id_t   nsid1, nsid2;

    assert( val1 && "val1 is NULL!");
    assert( val2 && "val2 is NULL!");

    if (val1->btyp != val2->btyp) {
        /* this might happen if a new config tree
         * has a delete node with no value
         */
        return -1;
    }

    /* normally ignore all meta-data, except when checking
     * for nested operations
     */
    if (configonly && editing) {
        /* if there was an nc:operation or YANG attribute in the
         * node, then do not treat the nodes as equal   */
        if (val1->editvars && val1->editvars->operset) {
            return -1;
        }
        if (val2->editvars && val2->editvars->operset) {
            return 1;
        }

        /* if the set-by-default property is different than
         * do not treat the values as equal   */
        if (val_set_by_default(val1) != val_set_by_default(val2)) {
            return 1;
        }
    }

    btyp = val1->btyp;

    switch (btyp) {
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
        if (val1->v.boo == val2->v.boo) {
            ret = 0;
        } else if (val1->v.boo) {
            ret = 1;
        } else {
            ret = -1;
        }
        break;
    case NCX_BT_ENUM:
        if (VAL_ENUM(val1) == VAL_ENUM(val2)) {
            ret = 0;
        } else if (VAL_ENUM(val1) < VAL_ENUM(val2)) {
            ret = -1;
        } else {
            ret = 1;
        }
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_DECIMAL64:
    case NCX_BT_FLOAT64:
        ret = ncx_compare_nums(&val1->v.num, &val2->v.num, btyp);
        break;
    case NCX_BT_BINARY:
        if (!val1->v.binary.ustr) {
            ret = -1;
        } else if (!val2->v.binary.ustr) {
            ret = 1;
        } else if (val1->v.binary.ustrlen <
                   val2->v.binary.ustrlen) {
            ret = -1;
        } else if (val1->v.binary.ustrlen >
                   val2->v.binary.ustrlen) {
            ret = 1;
        } else {
            ret = memcmp(val1->v.binary.ustr,
                         val2->v.binary.ustr,
                         val1->v.binary.ustrlen);
        }
        break;
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        ret = ncx_compare_strs(&val1->v.str, &val2->v.str, btyp);
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        ret = ncx_compare_lists(&val1->v.list, &val2->v.list);
        break;
    case NCX_BT_IDREF:
        /* note that this sort order based on NSID number may not
         * be stable across reboots, so the order of system-ordered
         * identityref leaf-lists can change
         */
        if (val1->v.idref.nsid == val2->v.idref.nsid) {
            if (val1->v.idref.name == NULL) {
                ret = 1;
            } else if (val2->v.idref.name == NULL) {
                ret = -1;
            } else {
                ret = xml_strcmp(val1->v.idref.name, val2->v.idref.name);
            }
        } else if (val1->v.idref.nsid < val2->v.idref.nsid) {
            ret = -1;
        } else {
           ret = 1;
        }
        break;
    case NCX_BT_LIST:
        ret = index_match(val1, val2);
        if (ret) {
            break;
        } /* else drop though and check values */
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        ch1 = (val_value_t *)dlq_firstEntry(&val1->v.childQ);
        ch2 = (val_value_t *)dlq_firstEntry(&val2->v.childQ);
        for (;;) {
            if ((ch1 && VAL_IS_DELETED(ch1)) || 
                (ch2 && VAL_IS_DELETED(ch2)) || 
                configonly) {
                while (ch1 && (VAL_IS_DELETED(ch1) ||
                               !obj_get_config_flag(ch1->obj))) {
                    ch1 = (val_value_t *)dlq_nextEntry(ch1);
                }
                while (ch2 && (VAL_IS_DELETED(ch2) ||
                               !obj_get_config_flag(ch2->obj))) {
                    ch2 = (val_value_t *)dlq_nextEntry(ch2);
                }
            }

            /* check if both child nodes exist */
            if (!ch1 && !ch2) {
                return 0;
            } else if (!ch1) {
                return -1;
            } else if (!ch2) {
                return 1;
            }

            /* check if the namespaces are the same */
            nsid1 = val_get_nsid(ch1);
            nsid2 = val_get_nsid(ch1);

            if (nsid1 < nsid2) {
                return -1;
            } else if (nsid1 > nsid2) {
                return 1;
            }

            /* check if both child nodes have the same name */
            ret = xml_strcmp(ch1->name, ch2->name);
            if (ret) {
                return ret;
            }

            if (!childonly || typ_is_simple(ch1->btyp)) {
                /* check if they have same value */
                ret = val_compare_max(ch1, ch2, configonly, childonly, editing);
                if (ret) {
                    return ret;
                }
            } // else childonly and complex node; treat as same

            /* get the next pair of child nodes to check */
            ch1 = (val_value_t *)dlq_nextEntry(ch1);
            ch2 = (val_value_t *)dlq_nextEntry(ch2);
        }
        /*NOTREACHED*/
    case NCX_BT_EXTERN:
        SET_ERROR(ERR_INTERNAL_VAL);
        ret = -1;
        break;
    case NCX_BT_INTERN:
        SET_ERROR(ERR_INTERNAL_VAL);
        ret = -1;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        ret = -1;
    }
    return ret;

}  /* val_compare_max */


/********************************************************************
* FUNCTION val_compare_ex
* 
* Compare 2 val_value_t struct value contents
* Check all or config only
*
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*    configonly == TRUE to compare config=true nodes only
*                  FALSE to compare all nodes
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
int32
    val_compare_ex (const val_value_t *val1,
                    const val_value_t *val2,
                    boolean configonly)
{
    return val_compare_max(val1, val2, configonly, FALSE, TRUE);
}


/********************************************************************
* FUNCTION val_compare
* 
* Compare 2 val_value_t struct value contents
*
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* Handle NCX_CL_COMPLEX by checking the index if needed
* and then checking all the child nodes recursively
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    val2 == second value to check
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
int32
    val_compare (const val_value_t *val1,
                 const val_value_t *val2)
{
    return val_compare_max(val1, val2, FALSE, FALSE, TRUE);
}  /* val_compare */


/********************************************************************
* FUNCTION val_compare_to_string
* 
* Compare a val_value_t struct value contents to a string
* 
* Handles NCX_CL_BASE and NCX_CL_SIMPLE data classes
* by comparing the simple value.
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == first value to check
*    strval2 == second value to check 
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
int32
    val_compare_to_string (const val_value_t *val1,
                           const xmlChar *strval2,
                           status_t *res)
{
#define MYBUFFSIZE  64

    assert( val1 && "val1 is NULL!");
    assert( strval2 && "strval2 is NULL!");
    assert( res && "res is NULL!");

    xmlChar      buff[MYBUFFSIZE];
    xmlChar     *mbuff = NULL;
    uint32       len = 0;
    int32        retval = 0;

    status_t myres = val_sprintf_simval_nc(NULL, val1, &len);
    if (myres != NO_ERR) {
        *res = myres;
        return -2;
    }
    if (len < MYBUFFSIZE) {
        myres = val_sprintf_simval_nc(buff, val1, &len);
    } else {
        mbuff =m__getMem(len+1);
        if (!mbuff) {
            *res = ERR_INTERNAL_MEM;
            return -2;
        }
        myres = val_sprintf_simval_nc(mbuff, val1, &len);
    }

    if (myres != NO_ERR) {
        *res = myres;
        retval = -2;
    } else if (mbuff) {
        retval = xml_strcmp(mbuff, strval2);
    } else {
        retval = xml_strcmp(buff, strval2);
    }

    if ( mbuff ) {
        m__free(mbuff);
    }

    *res = NO_ERR;
    return retval;

}  /* val_compare_to_string */


/********************************************************************
* FUNCTION val_compare_for_replace
* 
* Compare 2 val_value_t struct value contents
* for the nc:operation=replace procedures
* Only check the child nodes to see if the
* config nodes are the same
*
* !!!! Meta-value contents are ignored for this test !!!!
* 
* INPUTS:
*    val1 == new value to print
*    val2 == current value to check
*
* RETURNS:
*   compare result
*     -1: val1 is less than val2 (if complex just different or error)
*      0: val1 is the same as val2 
*      1: val1 is greater than val2
*********************************************************************/
int32
    val_compare_for_replace (const val_value_t *val1,
                             const val_value_t *val2)
{
    assert( val1 && "val1 is NULL!");
    assert( val2 && "val2 is NULL!");

    const val_value_t *ch1, *ch2;
    int32        ret = 0;
    xmlns_id_t   nsid1, nsid2;

    switch (val1->btyp) {
    case NCX_BT_LIST:
        ret = index_match(val1, val2);
        if (ret) {
            break;
        } /* else drop though and check values */
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        ch1 = (val_value_t *)dlq_firstEntry(&val1->v.childQ);
        ch2 = (val_value_t *)dlq_firstEntry(&val2->v.childQ);
        for (;;) {
            while (ch1 && (VAL_IS_DELETED(ch1) ||
                           !obj_get_config_flag(ch1->obj))) {
                ch1 = (val_value_t *)dlq_nextEntry(ch1);
            }
            while (ch2 && (VAL_IS_DELETED(ch2) ||
                           !obj_get_config_flag(ch2->obj))) {
                ch2 = (val_value_t *)dlq_nextEntry(ch2);
            }

            /* check if both child nodes exist */
            if (!ch1 && !ch2) {
                return 0;
            } else if (!ch1) {
                return -1;
            } else if (!ch2) {
                return 1;
            }

            /* check if the namespaces are the same */
            nsid1 = val_get_nsid(ch1);
            nsid2 = val_get_nsid(ch1);

            if (nsid1 < nsid2) {
                return -1;
            } else if (nsid1 > nsid2) {
                return 1;
            }

            /* check if both child nodes have the same name */
            ret = xml_strcmp(ch1->name, ch2->name);
            if (ret) {
                return ret;
            }

            /* check if they have same value
             * need to compare complex types as well
             * as leafs because the replace operation
             * could already be set in the parent
             * so skipping a complex node can result
             * in the merge of child nodes, instead of replace
             */
            ret = val_compare_ex(ch1, ch2, TRUE);
            if (ret) {
                return ret;
            }

            /* get the next pair of child nodes to check */
            ch1 = (val_value_t *)dlq_nextEntry(ch1);
            ch2 = (val_value_t *)dlq_nextEntry(ch2);
        }
        break;
    default:
        ret = val_compare_ex(val1, val2, TRUE);
    }

    return ret;

}  /* val_compare_for_replace */


/********************************************************************
* FUNCTION val_sprintf_simval_nc
* 
* Sprintf the xmlChar string NETCONF representation of a simple value
*
* buff is allowed to be NULL; if so, then this fn will
* just return the length of the string (w/o EOS ch) 
*
* USAGE:
*  call 1st time with a NULL buffer to get the length
*  call the 2nd time with a buffer of the returned length
*
* !!!! DOES NOT CHECK BUFF OVERRUN IF buff is non-NULL !!!!
*
* INPUTS:
*    buff == buffer to write (NULL means get length only)
*    val == value to check
*    len == address of return length
*
* OUTPUTS:
*   *len == number of bytes written (or just length if buff == NULL)
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_sprintf_simval_nc (xmlChar *buff,
                           const val_value_t *val,
                           uint32 *len)
{
    const ncx_lmem_t  *lmem, *nextlmem;
    const xmlChar     *s, *prefix;
    xmlChar           *str;
    ncx_btype_t        btyp;
    status_t           res;
    int32              icnt;
    uint32             mylen;
    char               numbuff[VAL_MAX_NUMLEN];

#ifdef DEBUG
    if (!val || !len) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    btyp = val->btyp;

    switch (btyp) {
    case NCX_BT_EMPTY:
        /* flag is element name : <foo/>  */
        if (val->v.boo) {
            if (buff) {
                icnt = sprintf((char *)buff, "<%s/>", val->name);
                if (icnt < 0) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    *len = (uint32)icnt;
                }
            } else {
                *len = xml_strlen(val->name) + 3;
            }
        } else {
            if (buff) {
                *buff = 0;
            }
            *len = 0;
        }
        break;
    case NCX_BT_BOOLEAN:
        if (val->v.boo) {
            if (buff) {
                sprintf((char *)buff, "true");
            }
            *len = 4;
        } else {
            if (buff) {
                sprintf((char *)buff, "false");
            }
            *len = 5;
        }
        break;
    case NCX_BT_ENUM:
        if (buff) {
            if (val->v.enu.name) {
                icnt = sprintf((char *)buff, "%s", val->v.enu.name);
                if (icnt < 0 ) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    *len = (uint32)icnt;
                }
            } else {
                *len = 0;
            }
        } else if (val->v.enu.name) {
            *len = xml_strlen(val->v.enu.name);
        } else {
            *len = 0;
        }
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_FLOAT64:
        res = ncx_sprintf_num(buff, &val->v.num, btyp, len);
        if (res != NO_ERR) {
            return SET_ERROR(res);
        }
        break;
    case NCX_BT_DECIMAL64:
        if (val->v.num.dec.val == 0) {
            if (buff) {
                *len = xml_strcpy(buff, (const xmlChar *)"0.0");
            } else {
                *len = xml_strlen((const xmlChar *)"0.0");
            }
        } else {
            /* need to generate the value string */
            res = ncx_sprintf_num(buff, &val->v.num, btyp, len);
            if (res != NO_ERR) {
                return SET_ERROR(res);
            }
        }
        break;
    case NCX_BT_STRING: 
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        if (val->obj && obj_is_password(val->obj)) {
            s = VAL_PASSWORD_STRING;
        } else {
            s = VAL_STR(val);
        }
        if (buff) {
            if (s) {
                *len = xml_strcpy(buff, s);
            } else {
                *len = 0;
            }
        } else {
            *len = (s) ? xml_strlen(s) : 0;
        }
        break;
    case NCX_BT_BINARY:
        s = val->v.binary.ustr;
        if (buff) {
            if (s) {
                /* !!! do not know the real buffer length
                 * !!! to send; assume call to this fn
                 * !!! to retrieve the length was done OK
                 */

                res = b64_encode(s, val->v.binary.ustrlen, buff, NCX_MAX_UINT,
                                 0 /*no newline split*/, len);
            } else {
                *len = 0;
            }
        } else if (s) {
            *len = b64_get_encoded_str_len( val->v.binary.ustrlen, 
                                            0/*no newline split*/ );
        } else {
            *len = 0;
        }
        break;
    case NCX_BT_BITS:
        *len = 0;
        for (lmem = (const ncx_lmem_t *)dlq_firstEntry(&val->v.list.memQ);
             lmem != NULL;
             lmem = nextlmem) {

            nextlmem = (const ncx_lmem_t *)dlq_nextEntry(lmem);

            s = lmem->val.str;
            if (buff) {
                /* hardwire double quotes to wrapper list strings */
                icnt = sprintf((char *)buff,
                               "%s", 
                               (s) ? (const char *)s : "");
                if (icnt < 0) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    buff += icnt;
                    *len += (uint32)icnt;
                }
                if (nextlmem) {
                    *buff++ = ' ';
                    *len += 1;
                }
            } else {
                *len += ((s) ? xml_strlen(s) : 0);
                if (nextlmem) {
                    *len += 1;
                }
            }
        }
        break;
    case NCX_BT_SLIST:
        *len = 0;
        for (lmem = (const ncx_lmem_t *)dlq_firstEntry(&val->v.list.memQ);
             lmem != NULL;
             lmem = (const ncx_lmem_t *)dlq_nextEntry(lmem)) {

            if (typ_is_string(val->v.list.btyp)) {
                s = lmem->val.str;
            } else if (typ_is_number(val->v.list.btyp)) {
                res = ncx_sprintf_num((xmlChar *)numbuff, 
                                      &lmem->val.num, 
                                      val->v.list.btyp, 
                                      len);
                if (res != NO_ERR) {
                    return SET_ERROR(res);
                }
                s = (const xmlChar *)numbuff;
            } else {
                switch (val->v.list.btyp) {
                case NCX_BT_ENUM:
                    s = VAL_ENUM_NAME(val);
                    break;
                case NCX_BT_BOOLEAN:
                    if (val->v.boo) {
                        s = NCX_EL_TRUE;
                    } else {
                        s = NCX_EL_FALSE;
                    }
                    break;
                default:
                    SET_ERROR(ERR_INTERNAL_VAL);
                    s = NULL;
                }
            }

            if (buff) {
                icnt = sprintf((char *)buff, "%s ", 
                               (s) ? (const char *)s : "");
                if (icnt < 0) {
                    return SET_ERROR(ERR_INTERNAL_VAL);
                } else {
                    buff += icnt;
                    *len += (uint32)icnt;
                }
            } else {
                *len += (1 + ((s) ? xml_strlen(s) : 0));
            }
        }
        break;
    case NCX_BT_IDREF:
        /* use the xmlprefix assigned to the NS ID for the
         * identityref, plus ':', plus the identity name
         */
        prefix = NULL;
        if (val->v.idref.nsid) {
            prefix = xmlns_get_ns_prefix(val->v.idref.nsid);
        }

        mylen = 0;
        if (buff) {
            str = buff;
            if (prefix) {
                mylen = xml_strcpy(str, prefix);
                str += mylen;
                mylen++;
                *str++ = ':';
            }
            if(val->v.idref.name) {
                mylen += xml_strcpy(str, val->v.idref.name);
            } else {
                mylen += xml_strcpy(str, NCX_EL_NONE);
            }
        } else {
            mylen = (prefix) ? xml_strlen(prefix)+1 : 0;
            if(val->v.idref.name) {
                mylen += xml_strlen(val->v.idref.name);
            } else {
                mylen += xml_strlen(NCX_EL_NONE);
            }
        }
        *len = mylen;
        break;
    case NCX_BT_LIST:
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        return SET_ERROR(ERR_NCX_OPERATION_NOT_SUPPORTED);
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    return NO_ERR;

}  /* val_sprintf_simval_nc */


/********************************************************************
* FUNCTION val_make_sprintf_string
*
* Malloc a buffer and then sprintf the xmlChar string 
* NETCONF representation of a simple value
*
* INPUTS:
*    val == value to print
*
* RETURNS:
*   malloced buffer with string represetation of the
*     'val' value node
*   NULL if some error
*********************************************************************/
xmlChar *
    val_make_sprintf_string (const val_value_t *val)
{
    xmlChar   *buff;
    uint32     len;
    status_t   res;
    
    len = 0;
    res = val_sprintf_simval_nc(NULL, val, &len);
    if (res != NO_ERR) {
        return NULL;
    }
    buff = m__getMem(len+1);
    if (buff == NULL) {
        return NULL;
    }
    res = val_sprintf_simval_nc(buff, val, &len);
    if (res != NO_ERR) {
        m__free(buff);
        return NULL;
    }

    return buff;

}  /* val_make_sprintf_string */


/********************************************************************
* FUNCTION val_resolve_scoped_name
* 
* Find the scoped identifier in the specified complex value
* 
* E.g.: foo.bar.baz
*
* INPUTS:
*    val == complex type to check
*    name == scoped name string of a nested node to find
*    chval == address of return child val
*
* OUTPUTS:
*    *chval is set to the value of the found local scoped 
*      child member, if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    val_resolve_scoped_name (val_value_t *val,
                             const xmlChar *name,
                             val_value_t **chval)
{
#define BUFFLEN 0xfffe

    xmlChar        *buff;
    const xmlChar  *next;
    val_value_t    *ch, *nextch;

#ifdef DEBUG
    if (!val || !name || !chval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    buff = m__getMem(BUFFLEN+1);
    if (!buff) {
        return SET_ERROR(ERR_INTERNAL_MEM);
    }

    /* get the top-level definition name and look for it
     * in the child queue.  This is going to work because
     * the token was already parsed as a scoped token string
     */
    next = ncx_get_name_segment(name, buff, BUFFLEN);

    /* the first segment is the start value */
    if (!next || xml_strcmp(buff, val->name)) {
        m__free(buff);
        return SET_ERROR(ERR_NCX_NOT_FOUND);
    }

    /* Each time get_name_segment is called, the next pointer
     * will be a dot or end of string.
     *
     * Keep looping until there are no more name segments left
     * or an error occurs.  The first time the loop is entered
     * the *next char should be non-zero.
     */
    ch = val;
    while (next && *next) {
        /* there is a next child, this better be a complex value */

        nextch = NULL;
        if (typ_has_children(ch->btyp)) {
            next = ncx_get_name_segment(++next, buff, BUFFLEN);     
            nextch = val_first_child_name(ch, buff);
        }
        if (!nextch) {
            m__free(buff);
            return SET_ERROR(ERR_NCX_DEFSEG_NOT_FOUND);
        }
        ch = nextch;
    }

    m__free(buff);
    *chval = ch;
    return NO_ERR;

} /* val_resolve_scoped_name */


/********************************************************************
* FUNCTION val_get_iqualval
* 
* Get the effective instance qualifier value for this value
* 
* INPUTS:
*    val == value construct to check
*
* RETURNS:
*   iqual value
*********************************************************************/
ncx_iqual_t 
    val_get_iqualval (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_IQUAL_NONE;
    }
#endif

    return obj_get_iqualval(val->obj);

} /* val_get_iqualval */


/********************************************************************
* FUNCTION val_duplicates_allowed
* 
* Determine if duplicates are allowed for the given val type
* The entire definition chain is checked to see if a 'no-duplicates'
*
* The default is config, so some sort of named type or parameter
* must be declared to create a non-config data element
*
* Fishing order:
*  1) typdef chain
*  2) parm definition
*  3) parmset definition
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value is classified as configuration
*     FALSE if the value is not classified as configuration
*********************************************************************/
boolean
    val_duplicates_allowed (val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* see if info already cached */
    if (val->flags & VAL_FL_DUPDONE) {
        return (val->flags & VAL_FL_DUPOK) ? TRUE : FALSE;
    }

    /* check for no-duplicates in the type appinfo */
    if (val->typdef) {
        if (typ_find_appinfo(val->typdef, 
                             NCX_PREFIX, 
                             NCX_EL_NODUPLICATES)) {
            val->flags |= VAL_FL_DUPDONE;
            return FALSE;
        }
    } else {
        val->flags |= VAL_FL_DUPDONE;
        return FALSE;
    }

    /* default is to allow duplicates */
    val->flags |= (VAL_FL_DUPDONE | VAL_FL_DUPOK);
    return TRUE;

}  /* val_duplicates_allowed */


/********************************************************************
* FUNCTION val_has_content
* 
* Determine if there is a value or any child nodes for this val
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value has some content
*     FALSE if the value does not have any content
*********************************************************************/
boolean
    val_has_content (const val_value_t *val)
{
    ncx_btype_t  btyp;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!val_is_real(val)) {
        return TRUE;
    }

    btyp = val->btyp;

    if (typ_has_children(btyp)) {
        return !dlq_empty(&val->v.childQ);
    } else if (btyp == NCX_BT_EMPTY) {
        return FALSE;
    } else if ((btyp == NCX_BT_SLIST || btyp==NCX_BT_BITS)
               && ncx_list_empty(&val->v.list)) {
        return FALSE;
    } else if (typ_is_string(btyp)) {
        return (VAL_STR(val) && *(VAL_STR(val))) ? TRUE : FALSE;
    } else {
        return TRUE;
    }

}  /* val_has_content */


/********************************************************************
* FUNCTION val_has_index
* 
* Determine if this value has an index
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if the value has an index
*     FALSE if the value does not have an index
*********************************************************************/
boolean
    val_has_index (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (dlq_empty(&val->indexQ)) ? FALSE : TRUE;

}  /* val_has_index */


/********************************************************************
* FUNCTION val_get_first_index
* 
* Get the first index entry, if any for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*    pointer to first val_index_t node,  NULL if none
*********************************************************************/
val_index_t *
    val_get_first_index (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_index_t *)dlq_firstEntry(&val->indexQ);

}  /* val_get_first_index */


/********************************************************************
* FUNCTION val_get_next_index
* 
* Get the next index entry, if any for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*    pointer to next val_index_t node,  NULL if none
*********************************************************************/
val_index_t *
    val_get_next_index (const val_index_t *valindex)
{
#ifdef DEBUG
    if (!valindex) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_index_t *)dlq_nextEntry(valindex);

}  /* val_get_next_index */


/********************************************************************
* FUNCTION val_parse_meta
* 
* Parse the metadata descriptor against the typdef
* Check only that the value is ok, not instance count
*
* INPUTS:
*     typdef == typdef to check
*     attr == XML attribute to check
*     retval == initialized val_value_t to fill in
*
* OUTPUTS:
*   *retval == filled in if return is NO_ERR
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    val_parse_meta (typ_def_t *typdef,
                    xml_attr_t *attr,
                    val_value_t *retval)
{
    const xmlChar   *enustr, *attrval;
    ncx_btype_t      btyp;
    int32            enuval;
    status_t         res;

#ifdef DEBUG
    if (!typdef || !attr || !retval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    btyp = typ_get_basetype(typdef);
    attrval = attr->attr_val;

    /* setup the return value */
    retval->flags |= VAL_FL_META;  /* obj field is NULL in meta */
    retval->btyp = btyp;
    retval->typdef = typdef;
    retval->dname = xml_strdup(attr->attr_name);
    if (!retval->dname) {
        return ERR_INTERNAL_MEM;
    }
    retval->name = retval->dname;
    retval->nsid = attr->attr_ns;
    retval->xpathpcb = attr->attr_xpcb;
    attr->attr_xpcb = NULL;

    /* handle the attr string according to its base type */
    switch (btyp) {
    case NCX_BT_BOOLEAN:
        if (attrval && !xml_strcmp(attrval, NCX_EL_TRUE)) {
            retval->v.boo = TRUE;
        } else if (attrval && 
                   !xml_strcmp(attrval,
                               (const xmlChar *)"1")) {
            retval->v.boo = TRUE;
        } else if (attrval && !xml_strcmp(attrval, NCX_EL_FALSE)) {
            retval->v.boo = FALSE;
        } else if (attrval && 
                   !xml_strcmp(attrval,
                               (const xmlChar *)"0")) {
            retval->v.boo = FALSE;
        } else {
            res = ERR_NCX_INVALID_VALUE;
        }
        break;
    case NCX_BT_ENUM:
        res = val_enum_ok(typdef, attrval, &enuval, &enustr);
        if (res == NO_ERR) {
            retval->v.enu.name = enustr;
            retval->v.enu.val = enuval;
        }
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_FLOAT64:
        res = ncx_decode_num(attrval, btyp, &retval->v.num);
        if (res == NO_ERR) {
            res = val_range_ok(typdef, btyp, &retval->v.num);
        }
        break;
    case NCX_BT_DECIMAL64:
        res = ncx_decode_dec64(attrval, 
                               typ_get_fraction_digits(typdef),
                               &retval->v.num);
        if (res == NO_ERR) {
            res = val_range_ok(typdef, btyp, &retval->v.num);
        }
        break;
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
    case NCX_BT_INSTANCE_ID:
        res = val_string_ok(typdef, btyp, attrval);
        if (res == NO_ERR) {
            retval->v.str = xml_strdup(attrval);
            if (!retval->v.str) {
                res = ERR_INTERNAL_MEM;
            }
        }
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    return res;

} /* val_parse_meta */


/********************************************************************
* FUNCTION val_set_extern
* 
* Setup an NCX_BT_EXTERN value
*
* INPUTS:
*     val == value to setup
*     fname == filespec string to set as the value
*********************************************************************/
void
    val_set_extern (val_value_t  *val,
                    xmlChar *fname)
{
#ifdef DEBUG
    if (!val || !fname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val->btyp = NCX_BT_EXTERN;
    val->v.fname = fname;

} /* val_set_extern */


/********************************************************************
* FUNCTION val_set_intern
* 
* Setup an NCX_BT_INTERN value
*
* INPUTS:
*     val == value to setup
*     intbuff == internal buffer to set as the value
*********************************************************************/
void
    val_set_intern (val_value_t  *val,
                    xmlChar *intbuff)
{
#ifdef DEBUG
    if (!val || !intbuff) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val->btyp = NCX_BT_INTERN;
    val->v.intbuff = intbuff;

} /* val_set_intern */


/********************************************************************
* FUNCTION val_fit_oneline
* 
* Check if the XML encoding for the specified val_value_t
* should take one line or more than one line
*
* Simple types should not use more than one line or introduce
* any extra whitespace in any simple content element
* 
* !!!The calculation includes the XML start and end tags!!!
*
*  totalsize: <foo:node>value</foo:node>  == 26
*
* INPUTS:
*   val == value to check
*   linelen == length of line to check against
*   
* RETURNS:
*   TRUE if the val is a type that should or must fit on one line
*   FALSE otherwise
*********************************************************************/
boolean
    val_fit_oneline (const val_value_t *val,
                     uint32 linesize)
{
    ncx_btype_t       btyp;
    const xmlChar    *str;
    uint32            cnt, valsize, valnamesize, totalsize;
    xmlns_id_t        nsid;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TRUE;
    }
#endif

    btyp = val->btyp;

    if (btyp == NCX_BT_EMPTY) {
        return TRUE;
    }

    valsize = 0;
    switch (btyp) {
    case NCX_BT_ENUM:
        valsize = (VAL_ENUM_NAME(val)) ?
            xml_strlen(VAL_ENUM_NAME(val)) : 0;
        break;
    case NCX_BT_BOOLEAN:
        valsize = (VAL_BOOL(val)) ? 4 : 5;
        break;
    case NCX_BT_INT8:
        valsize = 4;
        break;
    case NCX_BT_INT16:
        valsize = 6;
        break;
    case NCX_BT_INT32:
        valsize = 11;
        break;
    case NCX_BT_UINT8:
        valsize = 3;
        break;
    case NCX_BT_UINT16:
        valsize = 5;
        break;
    case NCX_BT_UINT32:
        valsize = 10;
        break;
    case NCX_BT_UINT64:
    case NCX_BT_INT64:
    case NCX_BT_DECIMAL64:
        valsize = 21;
        break;
    case NCX_BT_FLOAT64:
        /* guess an average amount, since most numbers are 
         * never going to be this many digits
         */
        valsize = 32;
        break;
    case NCX_BT_BINARY:
        valsize = val->v.binary.ustrlen;
        break;
    case NCX_BT_INSTANCE_ID:
        /*** TEMP !!! fall through !!!! ****/
        /*** TBD: XPath check ***/
        /* return FALSE; */
    case NCX_BT_STRING:
    case NCX_BT_LEAFREF:
        if (VAL_STR(val)) {
            valsize = xml_strlen(VAL_STR(val));

            /* check if multiple new-lines are entered */
            str = VAL_STR(val);
            cnt = 0;
            while (*str && cnt < 2) {
                if (*str++ == '\n') {
                    cnt++;
                }
            }
            if (cnt >= 2) {
                return FALSE;
            }
        }
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        /* these are printed 1 per line right now */
        return TRUE; 
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_LIST:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
        return dlq_empty(&val->v.childQ);
    case NCX_BT_EXTERN:
    case NCX_BT_INTERN:
        /* just put these on a new line; rare usage at this time */
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return TRUE;
    }

    if (valsize >= linesize) {
        return FALSE;
    }

    valnamesize = xml_strlen(val->name);

    nsid = val_get_nsid(val);
    if (val->nsid) {
        /* account for 'foo:' */
        valnamesize += 
            (xml_strlen(xmlns_get_ns_prefix(nsid)) + 1);
    }

    /* <foo:valname>val</foo:valname> */
    totalsize = valsize + 5 + (2 * valnamesize);

    return (totalsize <= linesize) ? TRUE : FALSE;

}  /* val_fit_oneline */


/********************************************************************
* FUNCTION val_create_allowed
* 
* Check if the specified value is allowed to have a
* create edit-config operation attribute
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is allowed to have the edit-op
*   FALSE otherwise
*********************************************************************/
boolean
    val_create_allowed (const val_value_t *val)
{

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->index) ? FALSE : TRUE;

}  /* val_create_allowed */


/********************************************************************
* FUNCTION val_delete_allowed
* 
* Check if the specified value is allowed to have a
* delete edit-config operation attribute
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is allowed to have the edit-op
*   FALSE otherwise
*********************************************************************/
boolean
    val_delete_allowed (const val_value_t *val)
{

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->index) ? FALSE : TRUE;

}  /* val_delete_allowed */


/********************************************************************
* FUNCTION val_is_config_data
* 
* Check if the specified value is a config DB object instance
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a config DB object instance
*   FALSE otherwise
*********************************************************************/
boolean
    val_is_config_data (const val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL || val->obj == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (obj_is_root(val->obj)) {
        return TRUE;
    } else if (obj_is_data_db(val->obj) && 
        obj_get_config_flag(val->obj)) {
        return TRUE;
    } else {
        return FALSE;
    }

}  /* val_is_config_data */


/********************************************************************
* FUNCTION val_is_virtual
* 
* Check if the specified value is a virtual value
* such that a 'get' callback function is required
* to access the real value contents
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a virtual value
*   FALSE otherwise
*********************************************************************/
boolean
    val_is_virtual (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->getcb) ? TRUE : FALSE;

}  /* val_is_virtual */


/********************************************************************
* FUNCTION val_get_virtual_value
* 
* Get the value of a value node
* The top-level value is provided by the caller
* and must be malloced with val_new_value
* before calling this function
* 
* must free the return val; not cached
*
* If the val->getcb is NULL, then an error will be returned
*
* Caller should check for *res == ERR_NCX_SKIPPED
* This will be returned if virtual value has no
* instance at this time.
*
*  !!! DO NOT SAVE THE RETURN VALUE LONGER THAN THE 
*  !!! VIRTUAL VALUE CACHE TIMEOUT VALUE
*
* INPUTS:
*   session == session CB ptr cast as void *
*              that is getting the virtual value
*   val == virtual value to get value for
*   res == pointer to output function return status value
*
* OUTPUTS:
*    val->virtualval will be set with the cached return value
*    *res == the function return status
*
* RETURNS:
*   A malloced and filled in val_value_t struct
*   This value is cached in the val->virtualval pointer
*   and will be freed when the cache is replaced or when
*   val is freed
*********************************************************************/
val_value_t *
    val_get_virtual_value (void *session,
                           val_value_t *val,
                           status_t *res)
{
    ses_cb_t    *scb;
    val_value_t *retval;

#ifdef DEBUG
    if (!val || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (!val->getcb) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    scb = (ses_cb_t *)session;
    retval = cache_virtual_value(scb, val, res);
    return retval;

}  /* val_get_virtual_value */


/********************************************************************
* FUNCTION val_is_default
* 
* Check if the specified value is set to the YANG default value
* 
* INPUTS:
*   val == value to check
*
* SIDE EFFECTS:
*   val->flags may be adjusted
*         VAL_FL_DEFVALSET will be set if not set already
*         VAL_FL_DEFVAL will be set or cleared if 
*            VAL_FL_DEFSETVAL is not already set,
*            after determining if the value == its default
*
* RETURNS:
*   TRUE if the val is set to the default value
*   FALSE otherwise
*********************************************************************/
boolean
    val_is_default (val_value_t *val)
{
    const xmlChar *def;
    xmlChar       *binbuff;
    val_value_t   *testval;
    ncx_enum_t     enu;
    ncx_num_t      num;
    boolean        ret;
    ncx_btype_t    btyp;
    status_t       res;
    uint32         len, deflen;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* complex types do not have defaults */
    if (val->typdef == NULL) {
        return FALSE;
    }

    /* check added by default */
    if (val->flags & VAL_FL_DEFSET) {
        return TRUE;
    }

    /* check the cached response for a normal leaf */
    if (val->flags & VAL_FL_DEFVALSET) {
        return (val->flags & VAL_FL_DEFVAL) ? TRUE : FALSE;
    }

    /* check general corner-case: if the value is part of
     * an index, then return FALSE, even if the data type
     * for the index node has a default
     */
    if (val->index) {
        val->flags |= VAL_FL_DEFVALSET;
        val->flags &= ~VAL_FL_DEFVAL;
        return FALSE;
    }


    /* check if the data type has a default */
    def = obj_get_default(val->obj);
    if (def == NULL) {
        val->flags |= VAL_FL_DEFVALSET;
        val->flags &= ~VAL_FL_DEFVAL;
        return FALSE;
    }

    /* check if this is a virtual value, return FALSE instead
     * of retrieving the value!!! Used for monitoring only!!!
     */
    if (val->getcb != NULL) {
        val->flags |= VAL_FL_DEFVALSET;
        val->flags &= ~VAL_FL_DEFVAL;
        return FALSE;
    }

    ret = FALSE;

    btyp = val->btyp;

    switch (btyp) {
    case NCX_BT_EMPTY:
    case NCX_BT_BOOLEAN:
        if (ncx_is_true(def)) {
            /* default is true */
            if (val->v.boo) {
                ret = TRUE;
            }
        } else if (ncx_is_false(def)) {
            /* default is false */
            if (!val->v.boo) {
                ret = TRUE;
            }
        }
        break;
    case NCX_BT_ENUM:
        ncx_init_enum(&enu);
        res = ncx_set_enum(def, &enu);
        if (res == NO_ERR && !ncx_compare_enums(&enu, &val->v.enu)) {
            ret = TRUE;
        }
        ncx_clean_enum(&enu);
        break;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
    case NCX_BT_FLOAT64:
        ncx_init_num(&num);
        res = ncx_decode_num(def, btyp, &num);
        if (res == NO_ERR && 
            !ncx_compare_nums(&num, &val->v.num, btyp)) {
            ret = TRUE;
        }
        ncx_clean_num(btyp, &num);
        break;
    case NCX_BT_DECIMAL64:
        ncx_init_num(&num);
        res = ncx_decode_dec64(def,
                               typ_get_fraction_digits(val->typdef),
                               &num);
        if (res == NO_ERR && 
            !ncx_compare_nums(&num, &val->v.num, btyp)) {
            ret = TRUE;
        }
        ncx_clean_num(btyp, &num);
        break;
    case NCX_BT_BINARY:
        deflen = xml_strlen(def);
        len = b64_get_decoded_str_len( def, deflen );
        if ( len ) {
           binbuff = m__getMem(len);
            if (!binbuff) {
                SET_ERROR(ERR_INTERNAL_MEM);
                return FALSE;
            } 
            res = b64_decode(def, deflen, binbuff, len, &len);
            if (res == NO_ERR) {
                ret = memcmp(binbuff, val->v.binary.ustr, len) 
                    ? FALSE : TRUE;
            }
            m__free(binbuff);
        }
        break;
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_LEAFREF:
        if (!xml_strcmp(def, val->v.str)) {
            ret = TRUE;
        }
        break;
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
    case NCX_BT_IDREF:
        // treating possible malloc failure as if the value is
        // not set to its YANG default value
        testval = val_make_simval(val->typdef, val->nsid, val->name, def, &res);
        if (testval && res == NO_ERR) {
            ret = val_compare(val, testval);
        }
        val_free_value(testval);
        break;
    case NCX_BT_LIST:
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_EXTERN:
    case NCX_BT_INTERN:
        /* not supported for default value */
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    val->flags |= VAL_FL_DEFVALSET;
    if (ret) {
        val->flags |= VAL_FL_DEFVAL;
    } else {
        val->flags &= ~VAL_FL_DEFVAL;
    }

    return ret;
    
}  /* val_is_default */


/********************************************************************
* FUNCTION val_is_real
* 
* Check if the specified value is a real value
*
*  return TRUE if not virtual or NCX_BT_EXTERN or NCX_BT_INTERN)
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   TRUE if the val is a real value
*   FALSE otherwise
*********************************************************************/
boolean
    val_is_real (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->getcb || val->btyp==NCX_BT_EXTERN ||
            val->btyp==NCX_BT_INTERN) ? FALSE : TRUE;

}  /* val_is_real */


/********************************************************************
* FUNCTION val_get_parent_nsid
* 
*    Try to get the parent namespace ID
* 
* INPUTS:
*   val == value to check
*   
* RETURNS:
*   namespace ID of parent, or 0 if not found or not a value parent
*********************************************************************/
xmlns_id_t
    val_get_parent_nsid (const val_value_t *val)
{
    const val_value_t  *v;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (!val->parent) {
        return 0;
    }

    v = (const val_value_t *)val->parent;
    return v->nsid;

}  /* val_get_parent_nsid */


/********************************************************************
* FUNCTION val_instance_count
* 
* Count the number of instances of the specified object name
* in the parent value struct.  This only checks the first
* level under the parent, not the entire subtree
*
*
* INPUTS:
*   val == value to check
*   modname == name of module which defines the object to count
*              NULL (do not check module names)
*   objname == name of object to count
*
* RETURNS:
*   number of instances found
*********************************************************************/
uint32
    val_instance_count (val_value_t  *val,
                        const xmlChar *modname,
                        const xmlChar *objname)
{
    val_value_t *chval;
    uint32       cnt;

#ifdef DEBUG
    if (!val || !objname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    cnt = 0;

    for (chval = val_get_first_child(val);
         chval != NULL;
         chval = val_get_next_child(chval)) {

        if (modname && 
            xml_strcmp(modname,
                       val_get_mod_name(chval))) {
            continue;
        }

        if (!xml_strcmp(objname, chval->name)) {
            cnt++;
        }
    }
    return cnt;
    
}  /* val_instance_count */


/********************************************************************
* FUNCTION val_set_extra_instance_errors
* 
* mark ERR_NCX_EXTRA_VAL_INST errors for nodes > 'maxelems'
* Count the number of instances of the specified object name
* in the parent value struct.  This only checks the first
* level under the parent, not the entire subtree
* Set the val-res status for all instances beyond the 
* specified 'maxelems' count to ERR_NCX_EXTRA_VAL_INST
*
* INPUTS:
*   val == value to check
*   modname == name of module which defines the object to count
*              NULL (do not check module names)
*   objname == name of object to count
*   maxelems == number of allowed instances
*
*********************************************************************/
void
    val_set_extra_instance_errors (val_value_t  *val,
                                   const xmlChar *modname,
                                   const xmlChar *objname,
                                   uint32 maxelems)
{
    val_value_t *chval;
    uint32       cnt;

#ifdef DEBUG
    if (!val || !objname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (maxelems == 0) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    cnt = 0;

    for (chval = val_get_first_child(val);
         chval != NULL;
         chval = val_get_next_child(chval)) {

        if (modname && 
            xml_strcmp(modname,
                       val_get_mod_name(chval))) {
            continue;
        }

        if (!xml_strcmp(objname, chval->name)) {
            if (++cnt > maxelems) {
                chval->res = ERR_NCX_EXTRA_VAL_INST;
            }
        }
    }
    
}  /* val_set_extra_instance_errors */


/********************************************************************
* FUNCTION val_need_quotes
* 
* Check if a string needs to be quoted to be output
* within a conf file or ncxcli stdout output
*
* INPUTS:
*    str == string to check
*
* RETURNS:
*    TRUE if double quoted string is needed
*    FALSE if not needed
*********************************************************************/
boolean
    val_need_quotes (const xmlChar *str)
{
#ifdef DEBUG
    if (!str) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* any whitespace or newline needs quotes */
    while (*str) {
        if (isspace(*str) || *str == '\n') {
            return TRUE;
        }
        str++;
    }
    return FALSE;

}  /* val_need_quotes */


/********************************************************************
* FUNCTION val_all_whitespace
* 
* Check if a string is all whitespace
*
* INPUTS:
*    str == string to check
*
* RETURNS:
*    TRUE if string is all whitespace or empty length
*    FALSE if non-whitespace char found
*********************************************************************/
boolean
    val_all_whitespace (const xmlChar *str)
{
#ifdef DEBUG
    if (!str) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    /* any whitespace or newline needs quotes */
    while (*str) {
        if (!(isspace(*str) || *str == '\n')) {
            return FALSE;
        }
        str++;
    }
    return TRUE;

}  /* val_all_whitespace */


/********************************************************************
* FUNCTION val_match_metaval
* 
* Match the specific attribute value and namespace ID
*
* INPUTS:
*     attr == attr to check
*     nsid == mamespace ID to match against
*     name == attribute name to match against
*
* RETURNS:
*     TRUE if attr is a match; FALSE otherwise
*********************************************************************/
boolean
    val_match_metaval (const xml_attr_t *attr,
                       xmlns_id_t  nsid,
                       const xmlChar *name)
{
#ifdef DEBUG
    if (!attr || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (xml_strcmp(attr->attr_name, name)) {
        return FALSE;
    }
    if (attr->attr_ns) {
        return (attr->attr_ns==nsid);
    } else {
        /* unqualified match */
        return TRUE;
    }
} /* val_match_metaval */


/********************************************************************
* FUNCTION val_get_dirty_flag
* 
* Get the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
boolean
    val_get_dirty_flag (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
#endif

    return (val->flags & VAL_FL_DIRTY) ? TRUE : FALSE;

} /* val_get_dirty_flag */


/********************************************************************
* FUNCTION val_get_subtree_dirty_flag
* 
* Get the subtree dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is subtree dirty, false otherwise
*********************************************************************/
boolean
    val_get_subtree_dirty_flag (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
#endif

    return (val->flags & VAL_FL_SUBTREE_DIRTY) ? TRUE : FALSE;

} /* val_get_subtree_dirty_flag */


/********************************************************************
* FUNCTION val_set_dirty_flag
* 
* Set the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
void
    val_set_dirty_flag (val_value_t *val)
{
    if (!val) {
        return;
    }

    val->flags |= VAL_FL_DIRTY;

    val_value_t *parent = val->parent;
    while (parent && !obj_is_root(parent->obj)) {
        parent->flags |= VAL_FL_SUBTREE_DIRTY;
        parent = parent->parent;
    }

} /* val_set_dirty_flag */


/********************************************************************
* FUNCTION val_clear_dirty_flag
* 
* Clear the dirty flag for this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty, false otherwise
*********************************************************************/
void
    val_clear_dirty_flag (val_value_t *val)
{
    if (!val) {
        return;
    }

    val->flags &= ~VAL_FL_DIRTY;

    val_value_t *parent = val->parent;
    while (parent && !obj_is_root(parent->obj)) {
        parent->flags &= ~VAL_FL_SUBTREE_DIRTY;
        parent = parent->parent;
    }

} /* val_clear_dirty_flag */


/********************************************************************
* FUNCTION val_dirty_subtree
* 
* Check the dirty or subtree_dirty flag
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     TRUE if value is dirty or any subtree may be dirty, false otherwise
*********************************************************************/
boolean
    val_dirty_subtree (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
#endif

    return (val->flags & (VAL_FL_DIRTY | VAL_FL_SUBTREE_DIRTY)) ? TRUE : FALSE;

} /* val_dirty_subtree */



/********************************************************************
 * FUNCTION val_clean_tree
 * 
 * Clear the dirty flag and the operation for all
 * nodes within a value struct
 *
 * INPUTS:
 *   val == value node to clean
 *
 * OUTPUTS:
 *   val and all its child nodes (if any) are cleaned
 *     val->flags: VAL_FL_DIRTY bit cleared to 0
 *     val->editvars deleted
 *     val->curparent: cleared to NULL
 *********************************************************************/
void
    val_clean_tree (val_value_t *val)
{
    val_value_t           *chval;

#ifdef DEBUG
    if (!val || !val->obj) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (obj_is_data_db(val->obj)) {
        for (chval = val_get_first_child(val);
             chval != NULL;
             chval = val_get_next_child(chval)) {
            val_clean_tree(chval);
        }
        val->flags &= ~ (VAL_FL_DIRTY | VAL_FL_SUBTREE_DIRTY);
        val->editop = OP_EDITOP_NONE;
        free_editvars(val);
    }

}  /* val_clean_tree */


/********************************************************************
* FUNCTION val_get_nest_level
* 
* Get the next level of the value
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     nest level from the root
*********************************************************************/
uint32
    val_get_nest_level (val_value_t *val)
{
    uint32 level;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
#endif

    level = 1;
    while (val->parent) {
        level++;
        val = val->parent;
    }
    return level;

} /* val_get_nest_level */


/********************************************************************
* FUNCTION val_get_first_leaf
* 
* Get the first leaf or leaflist node in the 
* specified value tree
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     pointer to first leaf found within this val struct
*    pointer to val if val is a leaf
*********************************************************************/
val_value_t *
    val_get_first_leaf (val_value_t *val)
{
    val_value_t  *child, *found;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    if (obj_is_leafy(val->obj)) {
        return val;
    } else if (typ_has_children(val->btyp)) {
        for (child = (val_value_t *)
                 dlq_firstEntry(&val->v.childQ);
             child != NULL;
             child = (val_value_t *)dlq_nextEntry(val)) {

            found = val_get_first_leaf(child);
            if (found) {
                return found;
            }
        }
    }

    return NULL;

}  /* val_get_first_leaf */


/********************************************************************
* FUNCTION val_get_mod_name
* 
* Get the module name associated with this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
const xmlChar *
    val_get_mod_name (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    if (val->nsid) {
        return xmlns_get_module(val->nsid);
    } else if (val->obj) {
        return obj_get_mod_name(val->obj);
    } else {
        return NULL;
    }
}  /* val_get_mod_name */


/********************************************************************
* FUNCTION val_get_mod_prefix
* 
* Get the module prefix associated with this value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
const xmlChar *
    val_get_mod_prefix (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    if (val->nsid) {
        return xmlns_get_ns_prefix(val->nsid);
    } else if (val->obj) {
        return obj_get_mod_prefix(val->obj);
    } else {
        return NULL;
    }
}  /* val_get_mod_prefix */


/********************************************************************
* FUNCTION val_get_nsid
* 
* Get the namespace ID for the specified value node
*
* INPUTS:
*     val == value node to check
*
* RETURNS:
*     const pointer to module name string
*     NULL if not found
*********************************************************************/
xmlns_id_t
    val_get_nsid (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
#endif

    if (val->nsid) {
        return val->nsid;
    } else if (val->obj) {
        return obj_get_nsid(val->obj);
    } else {
        return 0;
    }

}  /* val_get_nsid */


/********************************************************************
* FUNCTION val_change_nsid
* 
* Change the namespace ID fora value node and all its descendants
*
* INPUTS:
*     val == value node to change
*    nsid == new namespace ID to use
*
*********************************************************************/
void
    val_change_nsid (val_value_t *val,
                     xmlns_id_t nsid)
{
    val_value_t  *child;

#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    val->nsid = nsid;

    for (child = val_get_first_child(val);
         child != NULL;
         child = val_get_next_child(child)) {
        val_change_nsid(child, nsid);
    }

}  /* val_change_nsid */


/********************************************************************
* FUNCTION val_make_from_insertxpcb
* 
* Make a val_value_t for a list, with the
* child nodes for key leafs, specified in the
* key attribute string given to the insert operation
*
* INPUTS:
*   sourceval == list val_value_t from the PDU with the insertxpcb
*               to process 
*   status == address of return status (may be NULL, ignored)
*
* OUTPUTS:
*   if non-NULL:
*      *status == return status
*
* RETURNS:
*   malloced list val_value_t struct with converted value
*********************************************************************/
val_value_t *
    val_make_from_insertxpcb (val_value_t *sourceval,
                              status_t *res)
{
    val_value_t     *listval, *keyval;
    xpath_pcb_t     *xpcb;
    const xmlChar   *keyname, *keystring;
    boolean          done;
    status_t         myres;

#ifdef DEBUG
    if (!sourceval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    myres = NO_ERR;
    if (res) {
        *res = NO_ERR;
    }

    listval = val_new_value();
    if (!listval) {
        if (res) {
            *res = ERR_INTERNAL_MEM;
        }
        return NULL;
    }
    val_init_from_template(listval, sourceval->obj);

    myres = val_new_editvars(sourceval);
    if (myres != NO_ERR) {
        val_free_value(listval);
        if (res) {
            *res = myres;
        }
        return NULL;
    }

    xpcb = sourceval->editvars->insertxpcb;
    if (!xpcb || !xpcb->tkc || xpcb->validateres != NO_ERR) {
        if (res) {
            *res = SET_ERROR(ERR_INTERNAL_VAL);
        }
        val_free_value(listval);
        return NULL;
    }

    tk_reset_chain(xpcb->tkc);

    done = FALSE;
    while (!done && myres == NO_ERR) {
        keyname = NULL;
        keystring = NULL;
        keyval = NULL;

        myres = xpath_parse_token(xpcb, TK_TT_LBRACK);
        if (myres != NO_ERR) {
            continue;
        }

        myres = TK_ADV(xpcb->tkc);
        if (myres != NO_ERR) {
            continue;
        }
        keyname = TK_CUR_VAL(xpcb->tkc);

        myres = xpath_parse_token(xpcb, TK_TT_EQUAL);
        if (myres != NO_ERR) {
            continue;
        }
        
        myres = TK_ADV(xpcb->tkc);
        if (myres != NO_ERR) {
            continue;
        }
        keystring = TK_CUR_VAL(xpcb->tkc);

        myres = xpath_parse_token(xpcb, TK_TT_RBRACK);
        if (myres != NO_ERR) {
            continue;
        }
        
        if (!keyname || !keystring) {
            myres = SET_ERROR(ERR_INTERNAL_VAL);
            continue;
        }

        keyval = val_make_string(val_get_nsid(sourceval),
                                 keyname, 
                                 keystring);
        if (!keyval) {
            myres = ERR_INTERNAL_MEM;
            continue;
        } else {
            val_add_child(keyval, listval);
        }

        if (tk_next_typ(xpcb->tkc) != TK_TT_LBRACK) {
            done = TRUE;
        }
    }

    if (myres == NO_ERR) {
        myres = val_gen_index_chain(listval->obj, listval);
    }

    if (res) {
        *res = myres;
    }
                                    
    if (myres != NO_ERR) {
        val_free_value(listval);
        listval = NULL;
    }
        
    return listval;
        
}  /* val_make_from_insertxpcb */


/********************************************************************
* FUNCTION val_new_unique
* 
* Malloc and initialize the fields in a val_unique_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
val_unique_t * 
    val_new_unique (void)
{
    val_unique_t  *valuni;

    valuni = m__getObj(val_unique_t);
    if (!valuni) {
        return NULL;
    }

    (void)memset(valuni, 0x0, sizeof(val_unique_t));
    return valuni;

}  /* val_new_unique */


/********************************************************************
* FUNCTION val_free_unique
* 
* CLean and free a val_unique_t struct
*
* INPUTS:
*    valuni == val_unique struct to free
*********************************************************************/
void
    val_free_unique (val_unique_t *valuni)
{
    if (!valuni) {
        return;
    }
    if (valuni->pcb) {
        xpath_free_pcb(valuni->pcb);
    }
    m__free(valuni);

}  /* val_free_unique */


/********************************************************************
* FUNCTION val_get_typdef
* 
* Get the typdef field for a value struct
*
* INPUTS:
*    val == val_value_t struct to use
*
* RETURNS:
*   pointer to the typdef or NULL if none
*********************************************************************/
const typ_def_t *
    val_get_typdef (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return val->typdef;

}  /* val_get_typdef */


/********************************************************************
* FUNCTION val_set_by_default
* 
* Check if the value was set by val_add_defaults
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if set by default
*   FALSE if set explicitly by some user or the startup config
*********************************************************************/
boolean
    val_set_by_default (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->flags & (VAL_FL_DEFSET | VAL_FL_WITHDEF))
        ? TRUE : FALSE;

}  /* val_set_by_default */


/********************************************************************
* FUNCTION val_has_withdef_default
* 
* Check if the value contained the wd:default attribute
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if wd:default was set to true
*   FALSE if wd:default attribute was not set to true
*********************************************************************/
boolean
    val_has_withdef_default (const val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->flags & VAL_FL_WITHDEF) ? TRUE : FALSE;

}  /* val_has_withdef_default */


/********************************************************************
* FUNCTION val_set_withdef_default
* 
* Set the value flags as having the wd:default attribute
*
* INPUTS:
*    val == val_value_t struct to set
*
*********************************************************************/
void
    val_set_withdef_default (val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    val->flags |= VAL_FL_WITHDEF;

}  /* val_set_withdef_default */


/********************************************************************
* FUNCTION val_is_metaval
* 
* Check if the value is a meta-val (XML attribute)
*
* INPUTS:
*    val == val_value_t struct to check
*
* RETURNS:
*   TRUE if val is a meta-val
*   FALSE if val is not a meta-val
*********************************************************************/
boolean
    val_is_metaval (const val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    return (val->flags & VAL_FL_META) ? TRUE : FALSE;

}  /* val_is_metaval */


/********************************************************************
* FUNCTION val_move_chidren
* 
* Move all the child nodes from src to dest
* Source and dest must both be containers!
*
* INPUTS:
*    srcval == source val_value_t struct to move
*    destval == destination value struct ot use
*
*********************************************************************/
void
    val_move_children (val_value_t *srcval,
                       val_value_t *destval)
{
    val_value_t   *childval;

#ifdef DEBUG
    if (!srcval || !destval) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (typ_is_simple(srcval->btyp) || typ_is_simple(destval->btyp)) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* set new parent */
    for (childval = (val_value_t *)
             dlq_firstEntry(&srcval->v.childQ);
         childval != NULL;
         childval = (val_value_t *)dlq_nextEntry(childval)) {
        childval->parent = destval;
    }

    /* move all the entries at once */
    dlq_block_enque(&srcval->v.childQ, &destval->v.childQ);

}  /* val_move_children */


/********************************************************************
* FUNCTION val_cvt_generic
* 
* Convert all the database object pointers to
* generic object pointers to decouple a user
* variable in yangcli from the server-specific
* object definition (which goes away when the
* session is terminated)
*
* !!! Need to assume the val->obj pointer is already
* !!! invalid.  This can happen to yangcli when a 
* !!! session is dropped and there are vars that
* !!! reference YANG objects from the session
*
* INPUTS:
*    val == val_value_t struct to convert to generic
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_cvt_generic (val_value_t *val)
{
    val_value_t   *childval;
    val_index_t   *in;
    xmlChar       *buffer;
    status_t       res;
    uint32         len;
    boolean        haschildren;

#ifdef DEBUG
    if (val == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val->obj == NULL) {
        /* leave this object-less value alone */
        return NO_ERR;
    }

    res = NO_ERR;
    haschildren = typ_has_children(val->btyp);

    if (typ_is_string(val->btyp)) {
        val->obj = ncx_get_gen_string();
        if (val->xpathpcb) {
            xpath_free_pcb(val->xpathpcb);
            val->xpathpcb = NULL;
        }
    } else if (val->btyp == NCX_BT_ANY) {
        /* !!! this should not happen if agt/mgr_val_parse used
         * !!! parse_any will set the val->btyp to container
         */
        val->obj = ncx_get_gen_anyxml();
    } else {
        switch (val->btyp) {
        case NCX_BT_BINARY:
            val->obj = ncx_get_gen_binary();
            break;
        case NCX_BT_EMPTY:
            val->obj = ncx_get_gen_empty();
            break;
        case NCX_BT_CONTAINER:
            val->obj = ncx_get_gen_container();
            break;
        case NCX_BT_LIST:
            while (!dlq_empty(&val->indexQ)) {
                in = (val_index_t *)dlq_deque(&val->indexQ);
                m__free(in);
            }
            val->obj = ncx_get_gen_container();
            break;
        default:
            len = 0;
            res = val_sprintf_simval_nc(NULL, val, &len);
            if (res != NO_ERR) {
                return res;
            }

            buffer = m__getMem(len+1);
            if (!buffer) {
                return ERR_INTERNAL_MEM;
            }

            res = val_sprintf_simval_nc(buffer, val, &len);
            if (res == NO_ERR) {
                clean_value(val, FALSE);
                val->v.str = buffer;
                val->btyp = NCX_BT_STRING;
                val->obj = ncx_get_gen_string();
            } else {
                m__free(buffer);
                return res;
            }
        }
    }

    /* dive down into all the child nodest */
    if (haschildren) {
        for (childval = (val_value_t *)
                 dlq_firstEntry(&val->v.childQ);
             childval != NULL;
             childval = (val_value_t *)dlq_nextEntry(childval)) {

            res = val_cvt_generic(childval);
        }
    }

    return res;

}  /* val_cvt_generic */


/********************************************************************
* FUNCTION val_set_pcookie
* 
* Set the SIL pointer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*    pcookie == pointer cookie value to set
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_set_pcookie (val_value_t *val,
                     void *pcookie)
{
#ifdef DEBUG
    if (val == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val->editvars == NULL) {
        status_t res = val_new_editvars(val);
        if (res != NO_ERR) {
            return res;
        }
    }

    val->editvars->pcookie = pcookie;
    return NO_ERR;

} /* val_set_pcookie */


/********************************************************************
* FUNCTION val_set_icookie
* 
* Set the SIL integer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*    icookie == integer cookie value to set
*
* RETURNS:
*   status
*********************************************************************/
status_t
    val_set_icookie (val_value_t *val,
                     int icookie)
{
#ifdef DEBUG
    if (val == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (val->editvars == NULL) {
        status_t res = val_new_editvars(val);
        if (res != NO_ERR) {
            return res;
        }
    }

    val->editvars->icookie = icookie;
    return NO_ERR;

} /* val_set_icookie */


/********************************************************************
* FUNCTION val_get_pcookie
* 
* Get the SIL pointer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*
* RETURNS:
*    pointer cookie value or NULL if none
*********************************************************************/
void *
    val_get_pcookie (val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (val->editvars == NULL) {
        return NULL;
    }

    return val->editvars->pcookie;

} /* val_get_pcookie */


/********************************************************************
* FUNCTION val_get_icookie
* 
* Get the SIL integer cookie in the editvars for
* the specified value node
*
* INPUTS:
*    val == val_value_t struct to set
*
* RETURNS:
*    integer cookie value or 0 if none
*********************************************************************/
int
    val_get_icookie (val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (val->editvars == NULL) {
        return 0;
    }

    return val->editvars->icookie;

} /* val_get_icookie */


/********************************************************************
* FUNCTION val_delete_default_leaf
* 
* Do the internal work to convert a leaf to its YANG default value
*
* INPUTS:
*    val == val_value_t struct to use
*
* RETURNS:
*    status
*********************************************************************/
status_t
    val_delete_default_leaf (val_value_t *val)
{
    if ( !val || !val->obj ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    const xmlChar *defval = obj_get_default(val->obj);

    if ( !defval ) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    clean_value(val, FALSE);

    status_t res = val_set_simval_str(val, val->typdef, val->nsid, val->name,
                                      xml_strlen(val->name), defval);

    val->flags |= VAL_FL_DEFSET;

    return res;

} /* val_delete_default_leaf */


/********************************************************************
* FUNCTION val_force_empty
* 
* Convert a simple node to an empty type
*
* INPUTS:
*    val == val_value_t struct to use
*
*********************************************************************/
void
    val_force_empty (val_value_t *val)
{
#ifdef DEBUG
    if (val == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!typ_is_simple(val->btyp)) {
        SET_ERROR(ERR_NCX_WRONG_TYPE);
        return;
    }

    clean_value(val, FALSE);
    val->btyp = NCX_BT_EMPTY;
    val->v.boo = TRUE;

} /* val_force_empty */


/********************************************************************
* FUNCTION val_move_fields_for_xml
* 
* Move or copy the internal fields from one val to another
* for xml_wr purposes
*
* INPUTS:
*    srcval == source val_value_t struct to move from
*    destval == destination val to move to
*    movemeta == TRUE if metaQ should be transferred
*********************************************************************/
void
    val_move_fields_for_xml (val_value_t *srcval,
                             val_value_t *destval,
                             boolean movemeta)
{
#ifdef DEBUG
    if (srcval == NULL || destval == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    destval->parent = srcval->parent;
    destval->dataclass = srcval->dataclass;
    if (movemeta) {
        dlq_block_enque(&srcval->metaQ, &destval->metaQ);
    }

} /* val_move_fields_for_xml */


/********************************************************************
* FUNCTION val_get_first_key
* 
* Get the first key record if this is a list with a key-stmt
*
* INPUTS:
*   val == value node to check
*
*********************************************************************/
val_index_t *
    val_get_first_key (val_value_t *val)
{
#ifdef DEBUG
    if (!val) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    if (val->btyp != NCX_BT_LIST) {
        return NULL;
    }

    return (val_index_t *)dlq_firstEntry(&val->indexQ);

} /* val_get_first_key */


/********************************************************************
* FUNCTION val_get_next_key
* 
* Get the next key record if this is a list with a key-stmt
*
* INPUTS:
*   curkey == current key node
*
*********************************************************************/
val_index_t *
    val_get_next_key (val_index_t *curkey)
{
#ifdef DEBUG
    if (!curkey) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (val_index_t *)dlq_nextEntry(curkey);

} /* val_get_next_key */


/********************************************************************
* FUNCTION val_remove_key
* 
* Remove a key pointer because the key is invalid
* Free the key pointer
*
* INPUTS:
*   keyval == value node to find, remove and free
*
*********************************************************************/
void
    val_remove_key (val_value_t *keyval)
{
    assert(keyval && "keyval is NULL!" );

    val_value_t *parent = keyval->parent;
    val_index_t *valin = (val_index_t *)dlq_firstEntry(&parent->indexQ);
    val_index_t *nextvalin = NULL;
    for (; valin != NULL; valin = nextvalin) {
        nextvalin = (val_index_t *)dlq_nextEntry(valin);

        if (valin->val == keyval) {
            dlq_remove(valin);
            m__free(valin);
            return;
        }
    }
} /* val_remove_key */


/********************************************************************
* FUNCTION val_new_deleted_value
* 
* Malloc and initialize the fields in a val_value_t to be used
* as a deleted node marker
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
val_value_t * 
    val_new_deleted_value (void)
{
    val_value_t  *val;

    val = m__getObj(val_value_t);
    if (!val) {
        return NULL;
    }

    (void)memset(val, 0x0, sizeof(val_value_t));
    dlq_createSQue(&val->metaQ);
    dlq_createSQue(&val->indexQ);
    val->flags |= VAL_FL_DELETED;
    return val;

}  /* val_new_deleted_value */


/********************************************************************
* FUNCTION val_new_editvars
* 
* Malloc and initialize the val->editvars field
*
* INPUTS:
*    val == val_value_t data structure to use
*
* OUTPUTS:
*    val->editvars is malloced and initialized
* 
* RETURNS:
*   status
*********************************************************************/
status_t
    val_new_editvars (val_value_t *val)
{
    val_editvars_t  *editvars;

    if (val->editvars) {
        return SET_ERROR(ERR_NCX_DATA_EXISTS);
    }

    editvars = m__getObj(val_editvars_t);
    if (!editvars) {
        return ERR_INTERNAL_MEM;
    }
    memset(editvars, 0x0, sizeof(val_editvars_t));

    val->editvars = editvars;

#ifdef VAL_EDITVARS_DEBUG
    log_debug3("\n\nval_new_editvars: %u = %p\n",
               ++editvars_malloc,
               editvars);
#endif

    return NO_ERR;

}  /* val_new_editvars */


/********************************************************************
* FUNCTION val_free_editvars
* 
* Free the editing variables for the value node
*
* INPUTS:
*    val == val_value_t data structure to use
*
* OUTPUTS:
*    val->editvars is freed if set
*    val->editop set to OP_EDITOP_NONE
*********************************************************************/
void
    val_free_editvars (val_value_t *val)
{
    if (val == NULL) {
        return;
    }

    free_editvars(val);
    val->editop = OP_EDITOP_NONE;

}  /* val_free_editvars */


/* END file val.c */
