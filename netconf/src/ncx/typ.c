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
/*  FILE: typ.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
12nov05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>

#include <xmlstring.h>

#include  "procdefs.h"
#include "dlq.h"
#include "ncxconst.h"
#include "ncx.h"
#include "ncx_appinfo.h"
#include "ncx_num.h"
#include "tk.h"
#include "typ.h"
#include "xml_util.h"
#include "xpath.h"
#include "yangconst.h"

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
static typ_template_t  *basetypes[NCX_NUM_BASETYPES+1];

static boolean          typ_init_done = FALSE;




/********************************************************************
* FUNCTION clean_simple
* 
* Clean a simple type struct contents, but do not delete it
*
* INPUTS:
*     sim == pointer to the typ_simple_t  struct to clean
*********************************************************************/
static void
    clean_simple (typ_simple_t  *sim)
{
    typ_enum_t      *en;
    typ_unionnode_t *un;
    typ_rangedef_t  *rv;
    typ_sval_t      *sv;
    typ_pattern_t   *pat;
    ncx_btype_t     rtyp;    /* range base type */

#ifdef DEBUG
    if (!sim) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (sim->range.rangestr) {
        m__free(sim->range.rangestr);
        sim->range.rangestr = NULL;
    }
    memset(&sim->range.tkerr, 0x0, sizeof(ncx_error_t));

    if (sim->idref.baseprefix) {
        m__free(sim->idref.baseprefix);
        sim->idref.baseprefix = NULL;
    }

    if (sim->idref.basename) {
        m__free(sim->idref.basename);
        sim->idref.basename = NULL;
    }
        
    sim->idref.base = NULL;

    /* clean the rangeQ only if it is used */
    if (!dlq_empty(&sim->range.rangeQ)) {
        rtyp = typ_get_range_type(sim->btyp);
        while (!dlq_empty(&sim->range.rangeQ)) {
            rv = (typ_rangedef_t *)dlq_deque(&sim->range.rangeQ);
            typ_free_rangedef(rv, rtyp);
        }
    }
    ncx_clean_errinfo(&sim->range.range_errinfo);

    /* clean the patternQ */
    while (!dlq_empty(&sim->patternQ)) {
        pat = (typ_pattern_t *)dlq_deque(&sim->patternQ);
        typ_free_pattern(pat);
    }

    
    /* the Qs are used for differnt items, based on the type */
    switch (sim->btyp) {
    case NCX_BT_BITS:
    case NCX_BT_ENUM:
        while (!dlq_empty(&sim->valQ)) {
            en = (typ_enum_t *)dlq_deque(&sim->valQ);
            typ_free_enum(en);
        }
        break;
    case NCX_BT_UNION:
        while (!dlq_empty(&sim->unionQ)) {
            un = (typ_unionnode_t *)dlq_deque(&sim->unionQ);
            typ_free_unionnode(un);
        }
        break;
    case NCX_BT_SLIST:
        break;
    default:
        /* this will be non-empty for enums and strings */
        while (!dlq_empty(&sim->valQ)) {
            sv = (typ_sval_t *)dlq_deque(&sim->valQ);
            typ_free_sval(sv);
        }
    }

    if (sim->xleafref) {
        xpath_free_pcb(sim->xleafref);
        sim->xleafref = NULL;
    }

    sim->btyp = NCX_BT_NONE;
    sim->strrest = NCX_SR_NONE;
    sim->listtyp = NULL;

}  /* clean_simple */


/********************************************************************
* FUNCTION clean_named
* 
* Clean a named type struct contents, but do not delete it
*
* INPUTS:
*     nam == pointer to the typ_named_t struct to clean
*********************************************************************/
static void
    clean_named (typ_named_t  *nam)
{
#ifdef DEBUG
    if (!nam) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    nam->typ = NULL;
    nam->flags = 0;
    if (nam->newtyp) {
        typ_free_typdef(nam->newtyp);
        nam->newtyp = NULL;
    }

}  /* clean_named */


/************* E X T E R N A L    F U N C T I O N S  *****************/


/********************************************************************
* FUNCTION typ_load_basetypes
* 
* Create typ_template_t structs for the base types
* Must be called before any modules are loaded
* load the typ_template_t structs for the ncx_btype_t types
* MUST be called during ncx_init startup
*
* RETURNS:
*     status
*********************************************************************/
status_t
    typ_load_basetypes (void)
{
    typ_template_t  *typ;
    ncx_btype_t      btyp;
    xmlns_id_t       xsd_id;

    if (typ_init_done) {
        return NO_ERR;
    }

    xsd_id = xmlns_xs_id();
    basetypes[NCX_BT_NONE] = NULL;

    for (btyp=NCX_FIRST_DATATYPE; btyp<=NCX_LAST_DATATYPE; btyp++) {
        basetypes[btyp] = NULL;
    }

    for (btyp=NCX_FIRST_DATATYPE; btyp<=NCX_LAST_DATATYPE; btyp++) {
        /* create a typ_template_t struct */
        typ = typ_new_template();
        if (!typ) {
            return SET_ERROR(ERR_INTERNAL_MEM);
        }

        /* fill in the essential fields */
        typ->name = xml_strdup((const xmlChar *)tk_get_btype_sym(btyp));
        if (!typ->name) {
            m__free(typ);
            return SET_ERROR(ERR_INTERNAL_MEM);
        }

        typ->typdef.iqual = NCX_IQUAL_ONE;
        typ->typdef.tclass = NCX_CL_BASE;
        typ->typdef.maxaccess = NCX_ACCESS_NONE;
        typ->typdef.def.base = btyp;
        typ->nsid = xsd_id;

        /* save the struct in the basetype queue */
        basetypes[btyp] = typ;
    }

    typ_init_done = TRUE;
    return NO_ERR;

}  /* typ_load_basetypes */


/********************************************************************
* FUNCTION typ_unload_basetypes
* 
* Unload and destroy the typ_template_t structs for the base types
* unload the typ_template_t structs for the ncx_btype_t types
* SHOULD be called during ncx_cleanup
*
*********************************************************************/
void
    typ_unload_basetypes (void)
{
    typ_template_t  *typ;
    ncx_btype_t      btyp;

    if (!typ_init_done) {
        return;
    }

    for (btyp = NCX_FIRST_DATATYPE; btyp <= NCX_LAST_DATATYPE; btyp++) {
        typ = basetypes[btyp];
        typ_free_template(typ);
        basetypes[btyp] = NULL;
    }
    typ_init_done = FALSE;

}  /* typ_unload_basetypes */


/********************************************************************
* FUNCTION typ_new_template
* 
* Malloc and initialize the fields in a typ_template_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
typ_template_t * 
    typ_new_template (void)
{
    typ_template_t  *typ;

    typ = m__getObj(typ_template_t);
    if (!typ) {
        return NULL;
    }
    (void)memset(typ, 0x0, sizeof(typ_template_t));
    typ_init_typdef(&typ->typdef);
    typ->status = NCX_STATUS_CURRENT;
    dlq_createSQue(&typ->appinfoQ);
    return typ;

}  /* typ_new_template */


/********************************************************************
* FUNCTION typ_free_template
* 
* Scrub the memory in a typ_template_t by freeing all
* the sub-fields and then freeing the entire struct itself 
* The struct must be removed from any queue it is in before
* this function is called.
*
* INPUTS:
*    typ == typ_template_t to delete
*********************************************************************/
void 
    typ_free_template (typ_template_t *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* clean the typ_def_t struct */
    typ_clean_typdef(&typ->typdef);

    if (typ->name) {
        m__free(typ->name);
    }
    if (typ->descr) {
        m__free(typ->descr);
    }
    if (typ->ref) {
        m__free(typ->ref);
    }
    if (typ->defval) {
        m__free(typ->defval);
    }
    if (typ->units) {
        m__free(typ->units);
    }

    ncx_clean_appinfoQ(&typ->appinfoQ);


    m__free(typ);

}  /* typ_free_template */


/********************************************************************
* FUNCTION typ_new_typdef
* 
* Malloc and initialize the fields in a typ_def_t
*
* RETURNS:
*   pointer to the malloced and initialized struct or NULL if an error
*********************************************************************/
typ_def_t * 
    typ_new_typdef (void)
{
    typ_def_t  *typdef;

    typdef = m__getObj(typ_def_t);
    if (!typdef) {
        return NULL;
    }
    typ_init_typdef(typdef);
    return typdef;

}  /* typ_new_typdef */


/********************************************************************
* FUNCTION typ_init_typdef
* 
* init a pre-allocated typdef (done first)
* Initialize the fields in a typ_def_t
* !! Still need to call typ_init_simple
* !! when the actual builting type is determined
*
* INPUTS:
*   typdef == pointer to the struct to initialize
*********************************************************************/
void
    typ_init_typdef (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    (void)memset(typdef, 0x0, sizeof(typ_def_t));
    typdef->iqual = NCX_IQUAL_ONE;
    dlq_createSQue(&typdef->appinfoQ);

}  /* typ_init_typdef */


/********************************************************************
* FUNCTION typ_init_simple
* 
* Init a typ_simple_t struct inside a typ_def_t
* init a simple data type after typ_init_typdef
*
* INPUTS:
*     typdef == pointer to the typ_def_t  struct to init
*     as a NCX_CL_SIMPLE variant
*********************************************************************/
void
    typ_init_simple (typ_def_t  *tdef, 
                     ncx_btype_t btyp)
{
    typ_simple_t  *sim;

#ifdef DEBUG
    if (!tdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tdef->iqual = NCX_IQUAL_ONE;
    tdef->tclass = NCX_CL_SIMPLE;

    if (btyp == NCX_BT_BITS) {
        tdef->mergetype = NCX_MERGE_SORT;
    }

    sim = &tdef->def.simple;
    sim->btyp = btyp;
    dlq_createSQue(&sim->range.rangeQ);
    ncx_init_errinfo(&sim->range.range_errinfo);
    dlq_createSQue(&sim->valQ);
    dlq_createSQue(&sim->metaQ);
    dlq_createSQue(&sim->unionQ);
    dlq_createSQue(&sim->patternQ);
    tdef->def.simple.strrest = NCX_SR_NONE;
    tdef->def.simple.flags = 0;

}  /* typ_init_simple */


/********************************************************************
* FUNCTION typ_init_named
* 
* Init a typ_named_t struct inside a typ_def_t
* init a named data type after typ_init_typdef
*
* INPUTS:
*     typdef == pointer to the typ_def_t  struct to init
*     as a NCX_CL_SIMPLE variant
*********************************************************************/
void
    typ_init_named (typ_def_t  *tdef)
{

#ifdef DEBUG
    if (!tdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tdef->tclass = NCX_CL_NAMED;
    tdef->def.named.typ = NULL;
    tdef->def.named.newtyp = NULL;

}  /* typ_init_named */


/********************************************************************
 * Scrub the memory in a typ_def_t by freeing all Then free the typdef itself
 *
 * \param typdef typ_def_t to delete
 *********************************************************************/
void typ_free_typdef (typ_def_t *typdef)
{
    if (!typdef) {
        return;
    }

    typ_clean_typdef(typdef);
    m__free(typdef);

}  /* typ_free_typdef */


/********************************************************************
* FUNCTION typ_clean_typdef
* 
* Clean a typ_def_t struct, but do not delete it
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to clean
*********************************************************************/
void
    typ_clean_typdef (typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    if (typdef->prefix) {
        m__free(typdef->prefix);
        typdef->prefix = NULL;
    }
    if (typdef->typenamestr) {
        m__free(typdef->typenamestr);
        typdef->typenamestr = NULL;
    }

    ncx_clean_appinfoQ(&typdef->appinfoQ);

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_REF:
        break;
    case NCX_CL_SIMPLE:
        clean_simple(&typdef->def.simple);
        break;
    case NCX_CL_NAMED:
        clean_named(&typdef->def.named);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* typ_clean_typdef */


/********************************************************************
* FUNCTION typ_set_named_typdef
* 
* Set the fields in a named typedef (used by YANG parser)
*
* INPUTS:
*   typdef == type def struct to set
*   imptyp == named type to set within 'typ.typdef'
*
*********************************************************************/
void
    typ_set_named_typdef (typ_def_t *typdef,
                          typ_template_t *imptyp)
{
#ifdef DEBUG
    if (!typdef || !imptyp) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    typdef->tclass = NCX_CL_NAMED;
    typdef->def.named.typ = imptyp;
    typdef->linenum = imptyp->tkerr.linenum;

}  /* typ_set_named_typdef */


/********************************************************************
* FUNCTION typ_get_named_typename
* 
* Get the type name of the named typ
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to type name, NULL if some error
*********************************************************************/
const xmlChar *
    typ_get_named_typename (const typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass != NCX_CL_NAMED) {
        return NULL;
    }
    return (typdef->def.named.typ) ? typdef->def.named.typ->name : NULL;

}  /* typ_get_named_typename */


/********************************************************************
* FUNCTION typ_get_named_type_linenum
* 
* Get the line number of the type template of the named type
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to type name, NULL if some error
*********************************************************************/
uint32
    typ_get_named_type_linenum (const typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (typdef->tclass != NCX_CL_NAMED) {
        return 0;
    }
    return (typdef->def.named.typ) ? 
        typ_get_typ_linenum(typdef->def.named.typ) : 0;

}  /* typ_get_named_type_linenum */


/********************************************************************
* FUNCTION typ_set_new_named
* 
* Create a new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to setup
*     btyp == builtin type (NCX_BT_NONE if local type extension)
*
* RETURNS:
*    status
*********************************************************************/
status_t
    typ_set_new_named (typ_def_t  *typdef, 
                       ncx_btype_t btyp)
{
    typ_def_t *tdef;

#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    tdef = typdef->def.named.newtyp = typ_new_typdef();
    if (!tdef) {
        return ERR_INTERNAL_MEM;
    }

    /* initialize the new typdef with the parent base type */
    typ_init_simple(tdef, btyp);

    return NO_ERR;

}  /* typ_set_new_named */


/********************************************************************
* FUNCTION typ_get_new_named
* 
* Access the new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to new typ_def_t or NULL if none
*********************************************************************/
typ_def_t *
    typ_get_new_named (typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass != NCX_CL_NAMED) {
        return NULL;
    }
    return typdef->def.named.newtyp;

}  /* typ_get_new_named */


/********************************************************************
* FUNCTION typ_cget_new_named
* 
* Access the new typdef inside a typ_named_t struct inside a typ_def_t
*
* INPUTS:
*     typdef == pointer to the typ_def_t struct to check
*
* RETURNS:
*    pointer to new typ_def_t or NULL if none
*********************************************************************/
const typ_def_t *
    typ_cget_new_named (const typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass != NCX_CL_NAMED) {
        return NULL;
    }
    return typdef->def.named.newtyp;

}  /* typ_cget_new_named */


/********************************************************************
* FUNCTION typ_set_simple_typdef
* 
* Set the fields in a simple typedef (used by YANG parser)
*
* INPUTS:
*   typ == type template to set
*   btyp == builtin type to set within 'typ.typdef'
*
*********************************************************************/
void
    typ_set_simple_typdef (typ_template_t *typ,
                           ncx_btype_t btyp)
{
#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    typ->typdef.tclass = NCX_CL_SIMPLE;
    typ->typdef.def.simple.btyp = btyp;

}  /* typ_set_simple_typdef */


/********************************************************************
* FUNCTION typ_new_enum
* 
* Alloc and Init a typ_enum_t struct
* malloc and init an enumeration descriptor, strdup name ptr
*
* INPUTS:
*   name == name string for the enumeration
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*   Note that the enum integer value is initialized to zero
*********************************************************************/
typ_enum_t *
    typ_new_enum (const xmlChar *name)
{
    typ_enum_t  *ev;

#ifdef DEBUG
    if (!name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    ev = m__getObj(typ_enum_t);
    if (!ev) {
        return NULL;
    }
    memset(ev, 0, sizeof(typ_enum_t));
    ev->name = xml_strdup(name);
    if (!ev->name) {
        m__free(ev);
        return NULL;
    }
    dlq_createSQue(&ev->appinfoQ);
    return ev;

}  /* typ_new_enum */


/********************************************************************
* FUNCTION typ_new_enum2
* 
* Alloc and Init a typ_enum_t struct
* Use the string value as-=is, instead of mallocing a new one
* malloc and init an enumeration descriptor, pass off name ptr
* 
* INPUTS:
*   name == name string for the enumeration (will get free-ed later!!)
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*   Note that the enum integer value is initialized to zero
*********************************************************************/
typ_enum_t *
    typ_new_enum2 (xmlChar *name)
{
    typ_enum_t  *ev;

#ifdef DEBUG
    if (!name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    ev = m__getObj(typ_enum_t);
    if (!ev) {
        return NULL;
    }
    memset(ev, 0, sizeof(typ_enum_t));
    ev->name = name;
    dlq_createSQue(&ev->appinfoQ);
    return ev;

}  /* typ_new_enum2 */


/********************************************************************
* FUNCTION typ_free_enum
* 
* Free a typ_enum_t struct
* free an enumeration descriptor
*
* INPUTS:
*   en == enum struct to free
*********************************************************************/
void
    typ_free_enum (typ_enum_t *en)
{
#ifdef DEBUG
    if (!en) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (en->name) {
        m__free(en->name);
    }
    if (en->descr) {
        m__free(en->descr);
    }
    if (en->ref) {
        m__free(en->ref);
    }

    ncx_clean_appinfoQ(&en->appinfoQ);

    m__free(en);

}  /* typ_free_enum */


/********************************************************************
* FUNCTION typ_new_rangedef
* 
* Alloc and Init a typ_rangedef_t struct (range-stmt)
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
typ_rangedef_t *
    typ_new_rangedef (void)
{
    typ_rangedef_t  *rv;

    rv = m__getObj(typ_rangedef_t);
    if (!rv) {
        return NULL;
    }
    memset(rv, 0, sizeof(typ_rangedef_t));
    return rv;
 }  /* typ_new_rangedef */


/********************************************************************
* FUNCTION typ_free_rangedef
* 
* Free a typ_rangedef_t struct (range-stmt)
* 
* INPUTS:
*   rv == rangeval struct to delete
*   btyp == base type of range (float and double have malloced strings)
*********************************************************************/
void
    typ_free_rangedef (typ_rangedef_t *rv, 
                       ncx_btype_t  btyp)
{
#ifdef DEBUG
    if (!rv) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (rv->rangestr) {
        m__free(rv->rangestr);
    }

    ncx_clean_num(btyp, &rv->lb);
    ncx_clean_num(btyp, &rv->ub);

    if (rv->lbstr) {
        m__free(rv->lbstr);
    }
    if (rv->ubstr) {
        m__free(rv->ubstr);
    }

    m__free(rv);

 }  /* typ_free_rangedef */


/********************************************************************
* FUNCTION typ_normalize_rangeQ
* 
* Start with a valid rangedef chain
* Combine any consecutive range definitions like
*   1..4|5|6|7..9  would break replaced with 1..9
* concat consecutive rangedef sections for integral numbers
*
* Not done for NCX_BT_FLOAT64 data types
*
* INPUTS:
*   rangeQ == Q of typ_rangeval_t structs to normalize
*   btyp == base type of range
*********************************************************************/
void
    typ_normalize_rangeQ (dlq_hdr_t *rangeQ,
                          ncx_btype_t  btyp)
{
    typ_rangedef_t *rv1, *rv2;
    boolean  concat;

#ifdef DEBUG
    if (!rangeQ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    concat = FALSE;

    /* check if this range type can be normalized at all */
    switch (btyp) {
    case NCX_BT_INT8:
    case NCX_BT_INT16:
    case NCX_BT_INT32:
    case NCX_BT_INT64:
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
        /* range type OK, continue on */
        break;
    case NCX_BT_DECIMAL64:
        /***********/
        break;
    case NCX_BT_FLOAT64:
        /* cannot concat real numbers (by definition ;-) */
        return;
    default:
        /* not a number type, internal error */
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    /* check empty rangeQ */
    rv1 = (typ_rangedef_t *)dlq_firstEntry(rangeQ);
    if (!rv1) {
        return;
    }

    /* get next entry to check against first */
    rv2 = (typ_rangedef_t *)dlq_nextEntry(rv1);
    while (rv2) {
        switch (btyp) {
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
            if (rv1->ub.i+1 == rv2->lb.i) {
                rv1->ub.i = rv2->ub.i;
                concat = TRUE;
            }
            break;
        case NCX_BT_INT64:
            if (rv1->ub.l+1 == rv2->lb.l) {
                rv1->ub.l = rv2->ub.l;
                concat = TRUE;
            }
            break;
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
            if (rv1->ub.u+1 == rv2->lb.u) {
                rv1->ub.u = rv2->ub.u;
                concat = TRUE;
            }
            break;
        case NCX_BT_UINT64:
            if (rv1->ub.ul+1 == rv2->lb.ul) {
                rv1->ub.ul = rv2->ub.ul;
                concat = TRUE;
            }
            break;
        default:
            ;
        }

        if (concat) {
            /* keep rv1 as the first rangedef to test */
            dlq_remove(rv2);
            typ_free_rangedef(rv2, btyp);
            rv2 = (typ_rangedef_t *)dlq_nextEntry(rv1);
            concat = FALSE;
        } else {
            /* move along both rangedef pointers */
            rv1 = rv2;
            rv2 = (typ_rangedef_t *)dlq_nextEntry(rv2);
        }
    }

 }  /* typ_normalize_rangeQ */


/********************************************************************
* FUNCTION typ_get_rangeQ
*
* Return the rangeQ for the given typdef
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
dlq_hdr_t *
    typ_get_rangeQ (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range.rangeQ;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp &&
            !dlq_empty(&typdef->def.named.newtyp->def.simple.range.rangeQ)) {
            return &typdef->def.named.newtyp->def.simple.range.rangeQ;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_rangeQ(&typdef->def.named.typ->typdef) : NULL;
        }
    case NCX_CL_REF:
        return (typdef->def.ref.typdef) ?
            typ_get_rangeQ(typdef->def.ref.typdef) : NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_rangeQ */


/********************************************************************
* FUNCTION typ_get_rangeQ_con
*
* Return the rangeQ for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the rangeQ from this typdef, or NULL if none
*********************************************************************/
dlq_hdr_t *
    typ_get_rangeQ_con (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range.rangeQ;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return &typdef->def.named.newtyp->def.simple.range.rangeQ;
        } else {
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_rangeQ_con */


/********************************************************************
* FUNCTION typ_get_crangeQ
*
* Return the rangeQ for the given typdef
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
const dlq_hdr_t *
    typ_get_crangeQ (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range.rangeQ;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp &&
            !dlq_empty(&typdef->def.named.newtyp->def.simple.range.rangeQ)) {
            return &typdef->def.named.newtyp->def.simple.range.rangeQ;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_crangeQ(&typdef->def.named.typ->typdef) : NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_REF:
        return (typdef->def.ref.typdef) ? 
            typ_get_crangeQ(typdef->def.ref.typdef) : NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_crangeQ */


/********************************************************************
* FUNCTION typ_get_rangeQ_con
*
* Return the rangeQ for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the rangeQ from this typdef, or NULL if none
*********************************************************************/
const dlq_hdr_t *
    typ_get_crangeQ_con (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range.rangeQ;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return &typdef->def.named.newtyp->def.simple.range.rangeQ;
        } else {
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_crangeQ_con */


/********************************************************************
* FUNCTION typ_get_range_con
*
* Return the range struct for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range struct for this typdef
*********************************************************************/
typ_range_t *
    typ_get_range_con (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return &typdef->def.named.newtyp->def.simple.range;
        } else {
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_range_con */


/********************************************************************
* FUNCTION typ_get_crange_con
*
* Return the range struct for the given typdef
* Do not follow typdef chains
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range struct for this typdef
*********************************************************************/
const typ_range_t *
    typ_get_crange_con (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return &typdef->def.named.newtyp->def.simple.range;
        } else {
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_crange_con */


/********************************************************************
* FUNCTION typ_get_rangestr
*
* Return the range string for the given typdef chain
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the range string for this typdef, NULL if none
*********************************************************************/
const xmlChar *
    typ_get_rangestr (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return typdef->def.simple.range.rangestr;
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp &&
            typdef->def.named.newtyp->def.simple.range.rangestr) {
            return typdef->def.named.newtyp->def.simple.range.rangestr;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_rangestr(&typdef->def.named.typ->typdef) : NULL;
        }
    case NCX_CL_REF:
        return (typdef->def.ref.typdef) ?
            typ_get_rangestr(typdef->def.ref.typdef) : NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_get_rangestr */


/********************************************************************
* FUNCTION typ_first_rangedef
*
* Return the lower bound range definition struct
* Follow typdef chains if needed until first range found
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
const typ_rangedef_t *
    typ_first_rangedef (const typ_def_t *typdef)
{
    const dlq_hdr_t *rangeQ;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    rangeQ = typ_get_crangeQ(typdef);
    if (rangeQ) {
        return (const typ_rangedef_t *)dlq_firstEntry(rangeQ);
    } else {
        return NULL;
    }
}  /* typ_first_rangedef */


/********************************************************************
* FUNCTION typ_first_rangedef_con
*
* Return the lower bound range definition struct
* Constain search to this typdef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first rangedef struct or NULL if none
*********************************************************************/
const typ_rangedef_t *
    typ_first_rangedef_con (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return NULL;
    case NCX_CL_SIMPLE:
        return (const typ_rangedef_t *)
            dlq_firstEntry(&typdef->def.simple.range.rangeQ);
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return (const typ_rangedef_t *)
                dlq_firstEntry(
                        &typdef->def.named.newtyp->def.simple.range.rangeQ);
        } else {
            return NULL;
        }
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
}  /* typ_first_rangedef_con */


/********************************************************************
* FUNCTION typ_get_rangebounds_con
*
* Return the lower and upper bound range number
* Constain search to this typdef
* deprecated -- does not support multi-part ranges
*
* INPUTS:
*    typ_def == typ def struct to check
*    btyp == pointer to output range number type
*    lb == pointer to output lower bound number
*    ub == pointer to output upper bound number
*
* OUTPUTS:
*   *btyp == the type of number in the return type, if non-NULL
*   *lb == lower bound number
*   *ub == upper bound number
*
* RETURNS:
*   status, NO_ERR == something found
*********************************************************************/
status_t
    typ_get_rangebounds_con (const typ_def_t *typdef,
                             ncx_btype_t *btyp,
                             const ncx_num_t **lb,
                             const ncx_num_t **ub)
{
    const typ_rangedef_t *rdef;
    status_t res;

#ifdef DEBUG
    if (!typdef || !btyp || !lb || !ub) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    
    res = NO_ERR;
    switch (typdef->tclass) {
    case NCX_CL_BASE:
        res = ERR_NCX_SKIPPED;
        break;
    case NCX_CL_SIMPLE:
        /* get lower bound */
        rdef = (const typ_rangedef_t *)
            dlq_firstEntry(&typdef->def.simple.range.rangeQ);
        if (rdef ) {
            *btyp = rdef->btyp;
            *lb = &rdef->lb;

            /* get upper bound */
            rdef = (const typ_rangedef_t *)
                dlq_lastEntry(&typdef->def.simple.range.rangeQ);
            if (rdef) {
                *ub = &rdef->ub;
            } else {
                res = SET_ERROR(ERR_INTERNAL_PTR);
            }
        } else {
            res = ERR_NCX_NOT_FOUND;
        }
        break;
    case NCX_CL_COMPLEX:
        res = ERR_NCX_WRONG_DATATYP;
        break;
    case NCX_CL_NAMED:
        /* same check as simple type if newtyp exists */
        if (typdef->def.named.newtyp) {
            rdef = (const typ_rangedef_t *)dlq_firstEntry
                (&typdef->def.named.newtyp->def.simple.range.rangeQ);
            if (rdef ) {
                *btyp = rdef->btyp;
                *lb = &rdef->lb;

                rdef = (const typ_rangedef_t *)dlq_lastEntry
                    (&typdef->def.named.newtyp->def.simple.range.rangeQ);
                if (rdef ) {
                    *ub = &rdef->ub;
                } else {
                    res = SET_ERROR(ERR_INTERNAL_PTR);
                }
            } else {
                res = ERR_NCX_NOT_FOUND;
            }
        } else {
            res = ERR_NCX_NOT_FOUND;
        }
        break;
    case NCX_CL_REF:
        res = ERR_NCX_NOT_FOUND;
        break;
    default:
        res = SET_ERROR(ERR_INTERNAL_VAL);
    }
    return res;
    
}  /* typ_get_rangebounds_con */


/********************************************************************
* FUNCTION typ_get_strrest
* 
* Get the string restrinvtion type set for this typdef
*
* INPUTS:
*   typdef == typdef to check
*
* RETURNS:
*   string restrinction enumeration value
*********************************************************************/
ncx_strrest_t 
    typ_get_strrest (const typ_def_t *typdef)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_SR_NONE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
        return NCX_SR_NONE;
    case NCX_CL_SIMPLE:
        return typdef->def.simple.strrest;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return typdef->def.named.newtyp->def.simple.strrest;
        } else {
            return NCX_SR_NONE;
        }
    case NCX_CL_REF:
        if (typdef->def.ref.typdef) {
            return typ_get_strrest(typdef->def.ref.typdef);
        } else {
            return NCX_SR_NONE;
        }
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_SR_NONE;
    }
    /*NOTREACHED*/

} /* typ_get_strrest */


/********************************************************************
* FUNCTION typ_set_strrest
* 
* Set the string restrinvtion type set for this typdef
*
* INPUTS:
*   typdef == typdef to check
*   strrest == string restriction enum value to set
*
*********************************************************************/
void
    typ_set_strrest (typ_def_t *typdef,
                     ncx_strrest_t strrest)
{
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
        SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case NCX_CL_SIMPLE:
        typdef->def.simple.strrest = strrest;
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            typdef->def.named.newtyp->def.simple.strrest = strrest;
        }
        break;
    case NCX_CL_REF:
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

} /* typ_set_strrest */


/********************************************************************
* FUNCTION typ_new_sval
* 
* Alloc and Init a typ_sval_t struct
* malloc and init a string descriptor
*
* INPUTS:
*   str == string value inside token to copy
*   btyp == type of string (NCX_BT_STRING/LIST, OSTRING/OLIST)
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
typ_sval_t *
    typ_new_sval (const xmlChar *str,
                  ncx_btype_t  btyp)
{
    typ_sval_t  *sv;

#ifdef DEBUG
    if (!str) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (!typ_is_string(btyp)) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    sv = m__getObj(typ_sval_t);
    if (!sv) {
        return NULL;
    }
    memset(sv, 0, sizeof(typ_sval_t));
    sv->val = xml_strdup(str);
    if (!sv->val) {
        m__free(sv);
        return NULL;
    }
    return sv;

}  /* typ_new_sval */


/********************************************************************
* FUNCTION typ_free_sval
* 
* Free a typ_sval_t struct
* free a string descriptor
*
* INPUTS:
*   sv == typ_sval_t struct to free
*********************************************************************/
void
    typ_free_sval (typ_sval_t *sv)
{
#ifdef DEBUG
    if (!sv) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (sv->val) {
        m__free(sv->val);
    }
    m__free(sv);

}  /* typ_free_sval */



/********************************************************************
* FUNCTION typ_new_listval
* 
* Alloc and Init a typ_listval_t struct
* malloc and init a list descriptor
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
typ_listval_t *
    typ_new_listval (void)
{
    typ_listval_t  *lv;

    lv = m__getObj(typ_listval_t);
    if (!lv) {
        return NULL;
    }
    memset(lv, 0, sizeof(typ_listval_t));
    dlq_createSQue(&lv->strQ);
    return lv;

}  /* typ_new_listval */


/********************************************************************
* FUNCTION typ_free_listval
* 
* Free a typ_listval_t struct
* free a list descriptor
*
* INPUTS:
*   lv == typ_listval_t struct to free
*********************************************************************/
void
    typ_free_listval (typ_listval_t *lv)
{
    typ_sval_t *sv;

#ifdef DEBUG
    if (!lv) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(&lv->strQ)) {
        sv = (typ_sval_t *)dlq_deque(&lv->strQ);
        typ_free_sval(sv);
    }
    m__free(lv);
}  /* typ_free_listval */


/********************************************************************
* FUNCTION typ_get_range_type
* 
* Get the correct typ_rangedef_t data type for the
* indicated base type
* get the proper range base type to use for a given base type
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     base type enum of the range data type
*********************************************************************/
ncx_btype_t 
    typ_get_range_type (ncx_btype_t btyp)
{
    /* figure out what type of number is in the rangeval Q */
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
        return btyp;
    case NCX_BT_STRING:
    case NCX_BT_BINARY:
    case NCX_BT_SLIST:
        /* length ranges have to be non-negative uint32 */
        return NCX_BT_UINT32;
    default:
        /* the rest of the data types don't have ranges, but to simplify
         * cleanup, a value is returned, even though the rangeQ
         * will be empty during cleanup
         * Do not flag an error, but do not return a valid type either
         */
        return NCX_BT_NONE;
    }
    /*NOTREACHED*/

}  /* typ_get_range_type */


/********************************************************************
* FUNCTION typ_get_basetype
* 
* Get the final base type of the specified typ_def_t
* Follow any typdef links and get the actual base type of 
* the specified typedef 
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base type of final typ_def_t
*********************************************************************/
ncx_btype_t
    typ_get_basetype (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_BT_NONE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
        return NCX_BT_NONE;
    case NCX_CL_BASE:
        return typdef->def.base;
    case NCX_CL_SIMPLE:
        return typdef->def.simple.btyp; 
    case NCX_CL_NAMED:
        if (typdef->def.named.typ) {
            return typ_get_basetype(&typdef->def.named.typ->typdef);
        } else {
            return NCX_BT_NONE;
        }
    case NCX_CL_REF:
        if (typdef->def.ref.typdef) {
            return typ_get_basetype(typdef->def.ref.typdef);
        } else {
            return NCX_BT_NONE;
        }
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_BT_NONE;
    }
    /*NOTREACHED*/
}  /* typ_get_basetype */


/********************************************************************
* FUNCTION typ_get_name
* 
* Get the name for the specified typdef
*
* INPUTS:
*     typdef == type definition to  check
*
* RETURNS:
*     type name or empty string if some error
*********************************************************************/
const xmlChar *
    typ_get_name (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
        SET_ERROR(ERR_INTERNAL_VAL);
        return EMPTY_STRING;
    case NCX_CL_BASE:
        return (const xmlChar *)tk_get_btype_sym(typdef->def.base);
    case NCX_CL_SIMPLE:
        return (const xmlChar *)
            tk_get_btype_sym(typdef->def.simple.btyp);
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ? 
            typdef->def.named.typ->name : NULL;
    case NCX_CL_REF:
        return (typdef->def.ref.typdef) ?
            typ_get_name(typdef->def.ref.typdef) : NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return EMPTY_STRING;
    }
    /*NOTREACHED*/

}  /* typ_get_name */


/********************************************************************
* FUNCTION typ_get_basetype_name
* 
* Get the name of the final base type of the specified typ_template_t
*
* INPUTS:
*     typ == template containing the typdef to  check
*
* RETURNS:
*     base type name of final embedded typ_def_t
*********************************************************************/
const xmlChar *
    typ_get_basetype_name (const typ_template_t  *typ)
{
    ncx_btype_t  btyp;

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    btyp = typ_get_basetype(&typ->typdef);
    if (btyp != NCX_BT_NONE) {
        return (const xmlChar *)tk_get_btype_sym(btyp);
    } else {
        return EMPTY_STRING;
    }
    /*NOTREACHED*/

}  /* typ_get_basetype_name */


/********************************************************************
* FUNCTION typ_get_parenttype_name
* 
* Get the final base type of the specified typ_def_t
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base type of final typ_def_t
*********************************************************************/
const xmlChar *
    typ_get_parenttype_name (const typ_template_t  *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ->typdef.tclass == NCX_CL_NAMED) {
        return (typ->typdef.def.named.typ) ?
            typ->typdef.def.named.typ->name : EMPTY_STRING;
    } else {
        return EMPTY_STRING;
    }
    /*NOTREACHED*/

}  /* typ_get_parenttype_name */


/********************************************************************
* FUNCTION typ_get_base_class
* 
* Follow any typdef links and get the class of the base typdef
* for the specified typedef 
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     base class of final typ_def_t
*********************************************************************/
ncx_tclass_t
    typ_get_base_class (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_CL_NONE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
        return NCX_CL_NONE;
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return typdef->tclass;
    case NCX_CL_NAMED:
        if (typdef->def.named.typ) {
            return typ_get_base_class(&typdef->def.named.typ->typdef);
        } else {
            return NCX_CL_NAMED;
        }
    case NCX_CL_REF:
        return typ_get_base_class(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_CL_NONE;
    }
    /*NOTREACHED*/
}  /* typ_get_base_class */


/********************************************************************
* FUNCTION typ_get_basetype_typ
* 
* Get the default typ_template_t for the specified base type
* Get the default type template for the specified base type
*
* INPUTS:
*     btyp == base type to get
* RETURNS:
*     pointer to the type template for the specified basetype
*********************************************************************/
typ_template_t *
    typ_get_basetype_typ (ncx_btype_t  btyp)
{
    if (!typ_init_done) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return NULL;
    }
    if (btyp<NCX_FIRST_DATATYPE || btyp>NCX_LAST_DATATYPE) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    return basetypes[btyp];

}  /* typ_get_basetype_typ */


/********************************************************************
* FUNCTION typ_get_basetype_typdef
* 
* Get the default typdef for the specified base type
*
* INPUTS:
*     btyp == base type to get
* RETURNS:
*     pointer to the typdef for the specified basetype
*********************************************************************/
typ_def_t *
    typ_get_basetype_typdef (ncx_btype_t  btyp)
{
    if (!typ_init_done) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return NULL;
    }
    if (btyp<NCX_FIRST_DATATYPE || btyp>NCX_LAST_DATATYPE) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    return &basetypes[btyp]->typdef;

}  /* typ_get_basetype_typdef */


/********************************************************************
* FUNCTION typ_get_parent_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Ignores current named type even if if has new restrictions
* Get the parent typdef for NCX_CL_NAMED and NCX_CL_REF
* Returns NULL for all other classes
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
typ_def_t *
    typ_get_parent_typdef (typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        if (typdef->def.named.typ) {
            return &typdef->def.named.typ->typdef;
        } else {
            return NULL;
        }
    case NCX_CL_REF:
        return typdef->def.ref.typdef;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
}  /* typ_get_parent_typdef */


/********************************************************************
* FUNCTION typ_get_parent_type
* 
* Get the next typ_template_t in a chain -- for NCX_CL_NAMED only
*
* INPUTS:
*     typ == type template to check
* RETURNS:
*     pointer to next non-empty typ_template_t for a named type
*********************************************************************/
const typ_template_t *
    typ_get_parent_type (const typ_template_t  *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typ->typdef.tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        return typ->typdef.def.named.typ;
    case NCX_CL_REF:
        return NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
}  /* typ_get_parent_type */


/********************************************************************
* FUNCTION typ_get_cparent_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Ignores current named type even if if has new restrictions
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
const typ_def_t *
    typ_get_cparent_typdef (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return NULL;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            &typdef->def.named.typ->typdef : NULL;
    case NCX_CL_REF:
        return typdef->def.ref.typdef;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
}  /* typ_get_cparent_typdef */


/********************************************************************
* FUNCTION typ_get_next_typdef
* 
* Get the next typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* Get the next typdef in the chain for NCX_CL_NAMED or NCX_CL_REF
* Returns the input typdef for all other typdef classes
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     pointer to next non-empty typ_def_t
*********************************************************************/
typ_def_t *
    typ_get_next_typdef (typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return typdef;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return typdef;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_next_typdef(&typdef->def.named.typ->typdef) : NULL;
        }
    case NCX_CL_REF:
        return typdef->def.ref.typdef;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return typdef;
    }
    /*NOTREACHED*/
}  /* typ_get_next_typdef */


/********************************************************************
* FUNCTION typ_get_base_typdef
* 
* Get the base typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
* get the real typdef that describes the type, if the
* input is one of the 'pointer' typdef classes. Otherwise,
* just return the input typdef
*
* INPUTS:
*     typdef == typdef to check
* RETURNS:
*     pointer to base typ_def_t
*********************************************************************/
typ_def_t *
    typ_get_base_typdef (typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return typdef;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_base_typdef(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        return typdef->def.ref.typdef;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return typdef;
    }
    /*NOTREACHED*/
}  /* typ_get_base_typdef */


/********************************************************************
* FUNCTION typ_get_cbase_typdef
* 
* Get the base typ_def_t in a chain -- for NCX_CL_NAMED chained typed
* Also NCX_CL_REF pointer typdefs
*
* INPUTS:
*     typdef == typdef to check
* RETURNS:
*     pointer to base typ_def_t
*********************************************************************/
const typ_def_t *
    typ_get_cbase_typdef (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return typdef;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_cbase_typdef(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        return typdef->def.ref.typdef;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return typdef;
    }
    /*NOTREACHED*/
}  /* typ_get_cbase_typdef */


/********************************************************************
* FUNCTION typ_get_qual_typdef
* 
* Get the final typ_def_t of the specified typ_def_t
* based on the qualifier
* Get the next typdef in the chain for NCX_CL_NAMED or NCX_CL_REF
* Skip any named types without the specific restriction defined
*
* Returns the input typdef for simple typdef classes
*
* INPUTS:
*     typdef == typdef to check
*     squal == type of search qualifier desired
*          NCX_SQUAL_NONE == get first non-empty typdef
*          NCX_SQUAL_RANGE == find the first w/ range definition
*          NCX_SQUAL_VAL == find the first w/ stringval/pattern def
*          NCX_SQUAL_META == find the first typdef w/ meta-data def
*          
* RETURNS:
*     pointer to found typ_def_t or NULL if none found
*********************************************************************/
typ_def_t *
    typ_get_qual_typdef (typ_def_t  *typdef,
                         ncx_squal_t  squal)
{
    typ_def_t *ntypdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
        return NULL;
    case NCX_CL_BASE:
        if (squal==NCX_SQUAL_NONE) {
            return typdef;
        } else {
            return NULL;
        }
    case NCX_CL_SIMPLE:
        switch (squal) {
        case NCX_SQUAL_NONE:
            return typdef;
        case NCX_SQUAL_RANGE:
            return (dlq_empty(&typdef->def.simple.range.rangeQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_VAL:
            return (dlq_empty(&typdef->def.simple.valQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_META:
            return (dlq_empty(&typdef->def.simple.metaQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_APPINFO:
            return (dlq_empty(&typdef->appinfoQ)) ? NULL : typdef;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_NAMED:
        ntypdef = typdef->def.named.newtyp;
        if (!ntypdef) {
            return (typdef->def.named.typ) ?
                typ_get_qual_typdef(&typdef->def.named.typ->typdef, 
                                    squal) : NULL;
        }
        switch (squal) {
        case NCX_SQUAL_NONE:
            return typdef;
        case NCX_SQUAL_RANGE:
            if (!dlq_empty(&ntypdef->def.simple.range.rangeQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_VAL:
            if (!dlq_empty(&ntypdef->def.simple.valQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_META:
            if (!dlq_empty(&ntypdef->def.simple.metaQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_APPINFO:
            if (!dlq_empty(&typdef->appinfoQ)) {
                return typdef;
            }
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
        return (typdef->def.named.typ) ?
            typ_get_qual_typdef(&typdef->def.named.typ->typdef,
                                squal) : NULL;
    case NCX_CL_REF:
        return typ_get_qual_typdef(typdef->def.ref.typdef, squal);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
}  /* typ_get_qual_typdef */


/********************************************************************
* FUNCTION typ_get_cqual_typdef
* 
* Get the final typ_def_t of the specified typ_def_t
* based on the qualifier
* INPUTS:
*     typdef == typdef to check
*     squal == type of search qualifier desired
*          NCX_SQUAL_NONE == get first non-empty typdef
*          NCX_SQUAL_RANGE == find the first w/ range definition
*          NCX_SQUAL_VAL == find the first w/ stringval/pattern def
*          NCX_SQUAL_META == find the first typdef w/ meta-data def
*          
* RETURNS:
*     pointer to found typ_def_t or NULL if none found
*********************************************************************/
const typ_def_t *
    typ_get_cqual_typdef (const typ_def_t  *typdef,
                          ncx_squal_t  squal)
{
    const typ_def_t *ntypdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
        return NULL;
    case NCX_CL_BASE:
        if (squal==NCX_SQUAL_NONE) {
            return typdef;
        } else {
            return NULL;
        }
    case NCX_CL_SIMPLE:
        switch (squal) {
        case NCX_SQUAL_NONE:
            return typdef;
        case NCX_SQUAL_RANGE:
            return (dlq_empty(&typdef->def.simple.range.rangeQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_VAL:
            return (dlq_empty(&typdef->def.simple.valQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_META:
            return (dlq_empty(&typdef->def.simple.metaQ)) ? 
                NULL : typdef;
        case NCX_SQUAL_APPINFO:
            return (dlq_empty(&typdef->appinfoQ)) ? NULL : typdef;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
        /*NOTREACHED*/
    case NCX_CL_NAMED:
        ntypdef = typdef->def.named.newtyp;
        if (!ntypdef) {
            return (typdef->def.named.typ) ?
                typ_get_cqual_typdef(&typdef->def.named.typ->typdef, 
                                     squal) : NULL;
        }
        switch (squal) {
        case NCX_SQUAL_NONE:
            return typdef;
        case NCX_SQUAL_RANGE:
            if (!dlq_empty(&ntypdef->def.simple.range.rangeQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_VAL:
            if (!dlq_empty(&ntypdef->def.simple.valQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_META:
            if (!dlq_empty(&ntypdef->def.simple.metaQ)) {
                return typdef;
            }
            break;
        case NCX_SQUAL_APPINFO:
            if (!dlq_empty(&typdef->appinfoQ)) {
                return typdef;
            }
            break;
        default:
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        }
        return (typdef->def.named.typ) ?
            typ_get_cqual_typdef(&typdef->def.named.typ->typdef,
                                 squal) : NULL;
    case NCX_CL_REF:
        return typ_get_cqual_typdef(typdef->def.ref.typdef, squal);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
}  /* typ_get_cqual_typdef */


/********************************************************************
* FUNCTION typ_find_appinfo
*
* Find the specified appinfo variable by its prefix and name
*
* INPUTS:
*  typdef ==  typedef to check
*  prefix == module prefix (may be NULL)
*  name == name of the appinfo var to find
*
* RETURNS:
*   pointer to found appinfo struct or NULL if not found
*********************************************************************/
const ncx_appinfo_t *
    typ_find_appinfo (const typ_def_t *typdef,
                      const xmlChar *prefix,
                      const xmlChar *name)                    
{
    const typ_def_t        *appdef;
    const ncx_appinfo_t    *appinfo;
    boolean                 done;

#ifdef DEBUG
    if (!typdef || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    done = FALSE;
    appinfo = NULL;

    while (!done) {
        appdef = typ_get_cqual_typdef(typdef, NCX_SQUAL_APPINFO);
        if (appdef) {
            appinfo = ncx_find_const_appinfo(&appdef->appinfoQ, 
                                             prefix, 
                                             name);
            if (appinfo) {
                done = TRUE;
            } else if (appdef->tclass == NCX_CL_NAMED
                       && appdef->def.named.typ) {
                typdef = &appdef->def.named.typ->typdef;
            } else {
                done = TRUE;
            }
        } else {
            done = TRUE;
        }
    }

    return appinfo;

}  /* typ_find_appinfo */


/********************************************************************
* FUNCTION typ_find_appinfo_con
*
* Find the specified appinfo name, constrained to the current typdef
*
* INPUTS:
*  typdef ==  typedef to check
*  prefix == appinfo module prefix (may be NULL)
*  name == name of the appinfo var to find
*
* RETURNS:
*   pointer to found appinfo struct or NULL if not found
*********************************************************************/
const ncx_appinfo_t *
    typ_find_appinfo_con (const typ_def_t *typdef,
                          const xmlChar *prefix,
                          const xmlChar *name)                
{

#ifdef DEBUG
    if (!typdef || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return ncx_find_const_appinfo(&typdef->appinfoQ, 
                                  prefix, 
                                  name);

}  /* typ_find_appinfo_con */


/********************************************************************
* FUNCTION typ_is_xpath_string
*
* Find the ncx:xpath extension within the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:xpath extension found
*   FALSE otherwise
*********************************************************************/
boolean
    typ_is_xpath_string (const typ_def_t *typdef)
{
    const typ_template_t  *test_typ;
#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (typ_get_basetype(typdef) == NCX_BT_INSTANCE_ID) {
        return TRUE;
    }

    if (ncx_find_const_appinfo(&typdef->appinfoQ,
                               NCX_PREFIX, 
                               NCX_EL_XPATH)) {
        return TRUE;
    }

    if (typdef->tclass == NCX_CL_NAMED) {
        if (typdef->def.named.newtyp &&
            ncx_find_const_appinfo(&typdef->def.named.newtyp->appinfoQ,
                                   NCX_PREFIX, 
                                   NCX_EL_XPATH)) {
            return TRUE;
        }
        if (typdef->def.named.typ) {
            test_typ = typdef->def.named.typ;

            /* hardwire the YANG xpath1.0 type */
            if (!xml_strcmp(test_typ->tkerr.mod->name,
                            (const xmlChar *)"ietf-yang-types") &&
                !xml_strcmp(test_typ->name,
                            (const xmlChar *)"xpath1.0")) {
                return TRUE;
            }
            return typ_is_xpath_string(&test_typ->typdef);
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

}  /* typ_is_xpath_string */


/********************************************************************
* FUNCTION typ_is_qname_string
*
* Find the ncx:qname extension within the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:qname extension found
*   FALSE otherwise
*********************************************************************/
boolean
    typ_is_qname_string (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (ncx_find_const_appinfo(&typdef->appinfoQ,
                               NCX_PREFIX, 
                               NCX_EL_QNAME)) {
        return TRUE;
    }

    if (typdef->tclass == NCX_CL_NAMED) {
        if (typdef->def.named.newtyp &&
            ncx_find_const_appinfo(&typdef->def.named.newtyp->appinfoQ,
                                   NCX_PREFIX, 
                                   NCX_EL_QNAME)) {
            return TRUE;
        }
        if (typdef->def.named.typ) {
            return typ_is_qname_string(&typdef->def.named.typ->typdef);
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

}  /* typ_is_qname_string */


/********************************************************************
* FUNCTION typ_is_schema_instance_string
*
* Find the ncx:schema-instance extension within 
* the specified typdef chain
*
* INPUTS:
*  typdef == start of typ_def_t chain to check
*
* RETURNS:
*   TRUE if ncx:schema-instance extension found
*   FALSE otherwise
*********************************************************************/
boolean
    typ_is_schema_instance_string (const typ_def_t *typdef)
{
    const typ_template_t  *test_typ;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (typ_get_basetype(typdef) != NCX_BT_STRING) {
        return FALSE;
    }

    if (ncx_find_const_appinfo(&typdef->appinfoQ,
                               NCX_PREFIX, 
                               NCX_EL_SCHEMA_INSTANCE)) {
        return TRUE;
    }

    if (typdef->tclass == NCX_CL_NAMED) {
        if (typdef->def.named.newtyp &&
            ncx_find_const_appinfo(&typdef->def.named.newtyp->appinfoQ,
                                   NCX_PREFIX, 
                                   NCX_EL_SCHEMA_INSTANCE)) {
            return TRUE;
        }
        if (typdef->def.named.typ) {
            test_typ = typdef->def.named.typ;

            /* hardwire the nacm schema-instance-identifier type */
            if (!xml_strcmp(test_typ->name, YANG_SII_STRING)) {
                return TRUE;
            }
            return typ_is_schema_instance_string(&test_typ->typdef);
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

}  /* typ_is_schema_instance_string */


/********************************************************************
* FUNCTION typ_get_defval
*
* Find the default value string for the specified type template
* get default from template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   pointer to found defval string or NULL if none
*********************************************************************/
const xmlChar * 
    typ_get_defval (const typ_template_t *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ->defval) {
        return typ->defval;
    }

    if (typ->typdef.tclass == NCX_CL_NAMED) {
        return (typ->typdef.def.named.typ) ?
            typ_get_defval(typ->typdef.def.named.typ) : NULL;
    } else {
        /* no check for NCX_CL_REF because only type templates
         * have default values, not embedded typdefs
         */
        return NULL;
    }
}  /* typ_get_defval */


/********************************************************************
* FUNCTION typ_get_default
*
* Check if this typdef has a default value defined
* get default from typdef
*
* INPUTS:
*   typdef == typ_def_t struct to check
* RETURNS:
*   pointer to default or NULL if there is none
*********************************************************************/
const xmlChar *
    typ_get_default (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass == NCX_CL_NAMED) {
        return (typdef->def.named.typ) ?
            typ_get_defval(typdef->def.named.typ) : NULL;
    } else {
        /* Unless an embedded data node is a named type 
         * with a simple base type, it cannot have a default
         * !!! Not sure this always applies to NCX_CL_REF !!!
         */
        return NULL;   
    }

    
}  /* typ_get_default */


/********************************************************************
* FUNCTION typ_get_iqualval
*
* Find the instance qualifier value enum for the specified type template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   iqual value enum
*********************************************************************/
ncx_iqual_t
    typ_get_iqualval (const typ_template_t *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_IQUAL_NONE;
    }
#endif

    return typ_get_iqualval_def(&typ->typdef);
    
}  /* typ_get_iqualval */


/********************************************************************
* FUNCTION typ_get_iqualval_def
*
* Find the instance qualifier value enum for the specified type template
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   iqual value enum
*********************************************************************/
ncx_iqual_t
    typ_get_iqualval_def (const typ_def_t *typdef)
{
    ncx_btype_t       btyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_IQUAL_NONE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        btyp = typ_get_basetype(typdef);
        if (btyp == NCX_BT_LIST && typdef->iqual==NCX_IQUAL_ONE) {
            return NCX_IQUAL_ZMORE;
        } else {
            return typdef->iqual;
        }
    case NCX_CL_NAMED:
        if (typdef->iqual != NCX_IQUAL_ONE) {
            return typdef->iqual;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_iqualval(typdef->def.named.typ) : NCX_IQUAL_NONE;
        }
    case NCX_CL_REF:
        if (typdef->iqual != NCX_IQUAL_ONE) {
            return typdef->iqual;
        } else {
            return typdef->def.ref.typdef->iqual;
        }
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_IQUAL_NONE;
    }
    
}  /* typ_get_iqualval_def */


/********************************************************************
* FUNCTION typ_get_units
*
* Find the units string for the specified type template
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   pointer to found units string or NULL if none
*********************************************************************/
const xmlChar * 
    typ_get_units (const typ_template_t *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ->units) {
        return typ->units;
    }

    return typ_get_units_from_typdef(&typ->typdef);

}  /* typ_get_units */


/********************************************************************
* FUNCTION typ_get_units_from_typdef
*
* Find the units string for the specified typdef template
* Follow any NCX_CL_NAMED typdefs and check for a units
* clause in the the nearest ancestor typdef
* get units from named type if any
*
* INPUTS:
*  typdef == typ_def_t struct to check
*
* RETURNS:
*   pointer to found units string or NULL if none
*********************************************************************/
const xmlChar * 
    typ_get_units_from_typdef (const typ_def_t *typdef)
{
    const typ_template_t *typ;
    boolean               done;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass != NCX_CL_NAMED) {
        return NULL;
    }

    done = FALSE;
    while (!done) {
        typ = typdef->def.named.typ;
        if (typ->units) {
            return typ->units;
        }

        typdef = &typ->typdef;
        
        if (typdef->tclass != NCX_CL_NAMED) {
            done = TRUE;
        }
    }
    return NULL;

}  /* typ_get_units_from_typdef */


/********************************************************************
* FUNCTION typ_has_children
*
* Check if this is a data type that uses the val.v.childQ
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if the childQ is used, FALSE otherwise
*********************************************************************/
boolean
    typ_has_children (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_LIST:
    case NCX_BT_ANY:
        return TRUE;
    default:
        return FALSE;
    }
}  /* typ_has_children */


/********************************************************************
* FUNCTION typ_has_index
*
* Check if this is a data type that has an index
*
* INPUTS:
*   btyp == base type enum
*
* RETURNS:
*   TRUE if the indexQ is used, FALSE otherwise
*********************************************************************/
boolean
    typ_has_index (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_LIST:
        return TRUE;
    default:
        return FALSE;
    }
}  /* typ_has_index */


/********************************************************************
* FUNCTION typ_is_simple
*
* Check if this is a simple data type
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if this is a simple data type, FALSE otherwise
*********************************************************************/
boolean
    typ_is_simple (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_ANY:
        return FALSE;
    case NCX_BT_BITS:
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
    case NCX_BT_UNION:
    case NCX_BT_LEAFREF:
    case NCX_BT_IDREF:
    case NCX_BT_SLIST:
        return TRUE;
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_LIST:
    case NCX_BT_EXTERN:
    case NCX_BT_INTERN:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/
}  /* typ_is_simple */


/********************************************************************
* FUNCTION typ_is_xsd_simple
*
* Check if this is a simple data type in XSD encoding
*
* INPUTS:
*  btyp == base type enum
*
* RETURNS:
*   TRUE if this is a simple data type, FALSE otherwise
*********************************************************************/
boolean
    typ_is_xsd_simple (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_ANY:
    case NCX_BT_EMPTY:
        return FALSE;
    case NCX_BT_BITS:
    case NCX_BT_BOOLEAN:
    case NCX_BT_ENUM:
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
    case NCX_BT_UNION:
    case NCX_BT_SLIST:
    case NCX_BT_LEAFREF:
    case NCX_BT_IDREF:
    case NCX_BT_INSTANCE_ID:
        return TRUE;
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_LIST:
    case NCX_BT_EXTERN:
    case NCX_BT_INTERN:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
    /*NOTREACHED*/
}  /* typ_is_xsd_simple */


/********************************************************************
* FUNCTION typ_first_enumdef
*
* Get the first enum def struct
* looks past named typedefs to base typedef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
typ_enum_t *
    typ_first_enumdef (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    if (typdef->tclass != NCX_CL_SIMPLE) {
        return NULL;
    }

    return (typ_enum_t *)
        dlq_firstEntry(&typdef->def.simple.valQ);

}  /* typ_first_enumdef */


/********************************************************************
* FUNCTION typ_next_enumdef
*
* Get the next enum def struct
*
* INPUTS:
*    enumdef == typ enum struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
typ_enum_t *
    typ_next_enumdef (typ_enum_t *enumdef)
{

#ifdef DEBUG
    if (!enumdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (typ_enum_t *)dlq_nextEntry(enumdef);

}  /* typ_next_enumdef */


/********************************************************************
* FUNCTION typ_first_enumdef2
*
* Get the first enum def struct
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
typ_enum_t *
    typ_first_enumdef2 (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typdef->tclass != NCX_CL_SIMPLE) {
        return NULL;
    }

    return (typ_enum_t *)
        dlq_firstEntry(&typdef->def.simple.valQ);

}  /* typ_first_enumdef2 */


/********************************************************************
* FUNCTION typ_first_con_enumdef
*
* Get the first enum def struct
* constrained to this typdef
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first enum def of NULL if none
*********************************************************************/
const typ_enum_t *
    typ_first_con_enumdef (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    
    switch (typdef->tclass) {
    case NCX_CL_SIMPLE:
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            typdef = typdef->def.named.newtyp;
        } else {
            return NULL;
        }
        break;
    default:
        return NULL;
    }

    return (const typ_enum_t *)
        dlq_firstEntry(&typdef->def.simple.valQ);

}  /* typ_first_con_enumdef */


/********************************************************************
* FUNCTION typ_find_enumdef
*
* Get the specified enum def struct
*
* INPUTS:
*    ebQ == enum/bits Q to check
*    name == name of the enum to find
* RETURNS:
*   pointer to the specified enum def of NULL if none
*********************************************************************/
typ_enum_t *
    typ_find_enumdef (dlq_hdr_t *ebQ,
                      const xmlChar *name)
{
    typ_enum_t *en;

#ifdef DEBUG
    if (!ebQ || !name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (en = (typ_enum_t *)dlq_firstEntry(ebQ);
         en != NULL;
         en = (typ_enum_t *)dlq_nextEntry(en)) {
        if (!xml_strcmp(en->name, name)) {
            return en;
        }
    }
    return NULL;
    
}  /* typ_find_enumdef */


/********************************************************************
* FUNCTION typ_enumdef_count
*
* Get the number of typ_enum_t Q entries
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   number of entries
*********************************************************************/
uint32
    typ_enumdef_count (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (typdef->tclass != NCX_CL_SIMPLE) {
        return 0;
    }

    return dlq_count(&typdef->def.simple.valQ);

}  /* typ_enumdef_count */


/********************************************************************
* FUNCTION typ_first_strdef
*
* Get the first string def struct
*
* INPUTS:
*    typdef == typ def struct to check
*
* RETURNS:
*   pointer to the first string def of NULL if none
*********************************************************************/
const typ_sval_t *
    typ_first_strdef (const typ_def_t *typdef)
{
    ncx_btype_t btyp;
    const typ_sval_t  *retval;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    retval = NULL;

    btyp = typ_get_basetype(typdef);
    if (!typ_is_string(btyp)) {
        return NULL;
    }

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
        break;
    case NCX_CL_SIMPLE:
        retval = (const typ_sval_t *)
            dlq_firstEntry(&typdef->def.simple.valQ);
        break;
    case NCX_CL_COMPLEX:
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            retval = typ_first_strdef(typdef->def.named.newtyp);
        }
        break;
    case NCX_CL_REF:
        /**** !!! SHOULD THIS BE NULL INSTEAD !!!  ****/
        retval = typ_first_strdef(typdef->def.ref.typdef);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

    return retval;

}  /* typ_first_strdef */


/********************************************************************
* FUNCTION typ_get_maxrows
* 
* Get the maxrows value if it exists or zero if not
*
* INPUTS:
*     typdef == typdef to  check
* RETURNS:
*     max number of rows or zero if not applicable
*********************************************************************/
uint32
    typ_get_maxrows (const typ_def_t  *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
        return 0;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_maxrows(&typdef->def.named.typ->typdef) : 0;
    case NCX_CL_REF:
        return typ_get_maxrows(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return 0;
    }
    /*NOTREACHED*/
}  /* typ_get_maxrows */


/********************************************************************
* FUNCTION typ_get_maxaccess
*
* Find the max-access value for the specified typdef
* Follow named types to see if any parent typdef has a 
* maxaccess clause, if none found in the parameter
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   maxaccess enumeration
*********************************************************************/
ncx_access_t
    typ_get_maxaccess (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_ACCESS_NONE;
    }
#endif

    if (typdef->maxaccess != NCX_ACCESS_NONE) {
        return typdef->maxaccess;
    }

    switch (typdef->tclass) {
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return NCX_ACCESS_NONE;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_maxaccess(&typdef->def.named.typ->typdef)
            : NCX_ACCESS_NONE;
    case NCX_CL_REF:
        return typ_get_maxaccess(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_ACCESS_NONE;
    }
    /*NOTREACHED*/
    
}  /* typ_get_maxaccess */


/********************************************************************
* FUNCTION typ_get_dataclass
*
* Find the data-class value for the specified typdef
* Follow named types to see if any parent typdef has a 
* data-class clause, if none found in the parameter
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   data class enumeration
*********************************************************************/
ncx_data_class_t
    typ_get_dataclass (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_ACCESS_NONE;
    }
#endif

    if (typdef->dataclass != NCX_DC_NONE) {
        return typdef->dataclass;
    }

    switch (typdef->tclass) {
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return NCX_DC_NONE;
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_dataclass(&typdef->def.named.typ->typdef) : NCX_DC_NONE;
    case NCX_CL_REF:
        return typ_get_dataclass(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_DC_NONE;
    }
    /*NOTREACHED*/
    
}  /* typ_get_dataclass */


/********************************************************************
* FUNCTION typ_get_mergetype
*
*  Get the merge type for a specified type def
*
* INPUTS:
*  typdef == typ_def_t struct to check
* RETURNS:
*   merge type enumeration
*********************************************************************/
ncx_merge_t
    typ_get_mergetype (const typ_def_t *typdef)
{
    ncx_merge_t  mtyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_MERGE_NONE;
    }
#endif

    mtyp = NCX_MERGE_NONE;

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        break;
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        mtyp = typdef->mergetype;
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp &&
            typdef->def.named.newtyp->mergetype != NCX_MERGE_NONE) {
            mtyp = typdef->def.named.newtyp->mergetype;
        } else {
            return (typdef->def.named.typ) ?
                typ_get_mergetype(&typdef->def.named.typ->typdef)
                : NCX_MERGE_NONE;
        }
        break;
    case NCX_CL_REF:
        return typ_get_mergetype(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_MERGE_NONE;
    }
    if (mtyp == NCX_MERGE_NONE) {
        return NCX_DEF_MERGETYPE;
    } else {
        return mtyp;
    }
    /*NOTREACHED*/
    
}  /* typ_get_mergetype */


/********************************************************************
* FUNCTION typ_get_nsid
*
* Return the namespace ID
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
xmlns_id_t
    typ_get_nsid (const typ_template_t *typ)
{

#ifdef DEBUG
    if (!typ) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    return typ->nsid;
    
}  /* typ_get_nsid */


/********************************************************************
* FUNCTION typ_get_listtyp
*
* Return the typ_template for the list type, if the supplied
* typ_template contains a list typ_def, or named type chain
*    leads to a NCX_BT_SLIST or NCX_BT_BITS typdef
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
typ_template_t *
    typ_get_listtyp (typ_def_t *typdef)
{
    typ_def_t *ltypdef;
    ncx_btype_t      btyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_listtyp(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        ltypdef = typdef->def.ref.typdef;
        break;
    default:
        ltypdef = typdef;
    }

    btyp = typ_get_basetype(ltypdef);

    switch (btyp) {
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        return ltypdef->def.simple.listtyp;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/
    
}  /* typ_get_listtyp */


/********************************************************************
* FUNCTION typ_get_clisttyp
*
* Return the typ_template for the list type, if the supplied
* typ_template contains a list typ_def, or named type chain
*    leads to a NCX_BT_SLIST typdef
*
* INPUTS:
*  typ == typ_template_t struct to check
* RETURNS:
*   namespace ID of the type
*********************************************************************/
const typ_template_t *
    typ_get_clisttyp (const typ_def_t *typdef)
{
    const typ_def_t *ltypdef;
    ncx_btype_t      btyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_get_clisttyp(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        ltypdef = typdef->def.ref.typdef;
        break;
    default:
        ltypdef = typdef;
    }

    btyp = typ_get_basetype(ltypdef);
    switch (btyp) {
    case NCX_BT_SLIST:
    case NCX_BT_BITS:
        return ltypdef->def.simple.listtyp;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    
}  /* typ_get_clisttyp */


/********************************************************************
* FUNCTION typ_new_unionnode
* 
* Alloc and Init a typ_unionnode_t struct
*
* INPUTS:
*   typ == pointer to type template for this union node
*
* RETURNS:
*   pointer to malloced struct or NULL if memory error
*********************************************************************/
typ_unionnode_t *
    typ_new_unionnode (typ_template_t *typ)
{
    typ_unionnode_t  *un;

    /* typ is allowed to be NULL */

    un = m__getObj(typ_unionnode_t);
    if (!un) {
        return NULL;
    }
    memset(un, 0, sizeof(typ_unionnode_t));
    un->typ = typ;
    return un;
}  /* typ_new_unionnode */


/********************************************************************
* FUNCTION typ_free_unionnode
* 
* Free a typ_unionnode_t struct
*
* INPUTS:
*   un == union node to free
*********************************************************************/
void
    typ_free_unionnode (typ_unionnode_t *un)
{

#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    if (un->typdef) {
        typ_free_typdef(un->typdef);
    }
    m__free(un);

}  /* typ_free_unionnode */


/********************************************************************
* FUNCTION typ_get_unionnode_ptr
* 
* Get the proper typdef pointer from a unionnode
*
* INPUTS:
*   un == union node to check
*
* RETURNS:
*   pointer to the typ_def_t inside
*********************************************************************/
typ_def_t *
    typ_get_unionnode_ptr (typ_unionnode_t *un)
{

#ifdef DEBUG
    if (!un) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif
    if (un->typdef) {
        return un->typdef;
    } else if (un->typ) {
        return &un->typ->typdef;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

}  /* typ_get_unionnode_ptr */

/********************************************************************
* FUNCTION typ_first_unionnode
* 
* Get the first union node in the queue for a given typdef
*
* INPUTS:
*   typdef == pointer to type definition for the union node
*
* RETURNS:
*   pointer to first typ_unionnode struct or NULL if none
*********************************************************************/
typ_unionnode_t *
    typ_first_unionnode (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_SIMPLE:
        if (typdef->def.simple.btyp != NCX_BT_UNION) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        } else {
            return (typ_unionnode_t *)
                dlq_firstEntry(&typdef->def.simple.unionQ);
        }
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_first_unionnode(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        return typ_first_unionnode(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* typ_first_unionnode */



/********************************************************************
* FUNCTION typ_first_con_unionnode
* 
* Get the first union node in the queue for a given typdef
*
* INPUTS:
*   typdef == pointer to type definition for the union node
*
* RETURNS:
*   pointer to first typ_unionnode struct or NULL if none
*********************************************************************/
const typ_unionnode_t *
    typ_first_con_unionnode (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_SIMPLE:
        if (typdef->def.simple.btyp != NCX_BT_UNION) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return NULL;
        } else {
            return (const typ_unionnode_t *)
                dlq_firstEntry(&typdef->def.simple.unionQ);
        }
    case NCX_CL_NAMED:
        return (typdef->def.named.typ) ?
            typ_first_con_unionnode(&typdef->def.named.typ->typdef) : NULL;
    case NCX_CL_REF:
        return (typdef->def.ref.typdef) ?
            typ_first_con_unionnode(typdef->def.ref.typdef) : NULL;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* typ_first_con_unionnode */


/********************************************************************
* FUNCTION typ_is_number
* 
* Check if the base type is numeric
*
* INPUTS:
*    btype == basetype enum to check
*
* RETURNS:
*    TRUE if base type is numeric
*    FALSE if some other type
*********************************************************************/
boolean
    typ_is_number (ncx_btype_t btyp)
{
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
        return TRUE;
    default:
        return FALSE;
    }

}  /* typ_is_number */


/********************************************************************
* FUNCTION typ_is_string
* 
* Check if the base type is a simple string (not list)
*
* INPUTS:
*    btyp == base type enum to check
*
* RETURNS:
*    TRUE if base type is textual
*    FALSE if some other type
*********************************************************************/
boolean
    typ_is_string (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_STRING:
    case NCX_BT_INSTANCE_ID:
        return TRUE;
    case NCX_BT_LEAFREF:   /***/
        return TRUE;
    default:
        return FALSE;
    }

}  /* typ_is_string */


/********************************************************************
* FUNCTION typ_is_enum
* 
* Check if the base type is an enumeration
*
* INPUTS:
*    btyp == base type enum to check
*
* RETURNS:
*    TRUE if base type is an enumeration
*    FALSE if some other type
*********************************************************************/
boolean
    typ_is_enum (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_ENUM:
        return TRUE;
    default:
        return FALSE;
    }

}  /* typ_is_enum */


/********************************************************************
* FUNCTION typ_new_pattern
* 
*   Malloc and init a pattern struct
*
* INPUTS:
*  pat_str == pattern string to copy and save
*
* RETURNS:
*   malloced struct or NULL if memory error
*********************************************************************/
typ_pattern_t *
    typ_new_pattern (const xmlChar *pat_str)
{
    typ_pattern_t  *pat;

#ifdef DEBUG
    if (!pat_str) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pat = m__getObj(typ_pattern_t);
    if (!pat) {
        return NULL;
    }
    memset(pat, 0x0, sizeof(typ_pattern_t));

    pat->pat_str = xml_strdup(pat_str);
    if (!pat->pat_str) {
        m__free(pat);
        return NULL;
    }
    
    ncx_init_errinfo(&pat->pat_errinfo);

    return pat;

} /* typ_new_pattern */


/********************************************************************
* FUNCTION typ_free_pattern
* 
*   Free a pattern struct
*   Must be freed from any Q before calling this function
*
* INPUTS:
*    pat == typ_pattern_t struct to free
*
*********************************************************************/
void
    typ_free_pattern (typ_pattern_t *pat)
{
#ifdef DEBUG
    if (!pat) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (pat->pattern) {
        xmlRegFreeRegexp(pat->pattern);
    }
    if (pat->pat_str) {
        m__free(pat->pat_str);
    }
    ncx_clean_errinfo(&pat->pat_errinfo);

    m__free(pat);

} /* typ_free_pattern */


/********************************************************************
* FUNCTION typ_compile_pattern
* 
* Compile a pattern as into a regex_t struct
*
* INPUTS:
*     btyp == base type of the string
*     sv == ncx_sval_t holding the pattern to compile
*
* OUTPUTS:
*     pat->pattern is set if NO_ERR
*
* RETURNS:
*     status
*********************************************************************/
status_t
    typ_compile_pattern (typ_pattern_t *pat)
{

#ifdef DEBUG
    if (!pat || !pat->pat_str) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    pat->pattern = xmlRegexpCompile(pat->pat_str);
    if (!pat->pattern) {
        return ERR_NCX_INVALID_PATTERN;
    } else {
        return NO_ERR;
    }

}  /* typ_compile_pattern */


/********************************************************************
* FUNCTION typ_get_first_pattern
* 
* Get the first pattern struct for a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
typ_pattern_t *
    typ_get_first_pattern (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_REF:
        return NULL;
    case NCX_CL_SIMPLE:
        return (typ_pattern_t *)
            dlq_firstEntry(&typdef->def.simple.patternQ);
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return typ_get_first_pattern(typdef->def.named.newtyp);
        } else {
            /* constrained -- do not go to next typdef */
            return NULL;
        }
        /*NOTREACHED*/
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* typ_get_first_pattern */


/********************************************************************
* FUNCTION typ_get_next_pattern
* 
* Get the next pattern struct for a typdef
*
* INPUTS:
*   curpat  == current typ_pattern_t to check
*
* RETURNS:
*   pointer to next pattern struct or NULL if none
*********************************************************************/
typ_pattern_t *
    typ_get_next_pattern (typ_pattern_t *curpat)
{
#ifdef DEBUG
    if (!curpat) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (typ_pattern_t *)dlq_nextEntry(curpat);

}  /* typ_get_next_pattern */


/********************************************************************
* FUNCTION typ_get_first_cpattern
* 
* Get the first pattern struct for a typdef
* Const version
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
const typ_pattern_t *
    typ_get_first_cpattern (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NONE:
    case NCX_CL_BASE:
    case NCX_CL_REF:
        return NULL;
    case NCX_CL_SIMPLE:
        return (const typ_pattern_t *)
            dlq_firstEntry(&typdef->def.simple.patternQ);
        break;
    case NCX_CL_NAMED: 
        if (typdef->def.named.newtyp) {
            return typ_get_first_cpattern(typdef->def.named.newtyp);
        } else {
            /* constrained -- do not go to next typdef */
            return NULL;
        }
        /*NOTREACHED*/
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
    /*NOTREACHED*/

}  /* typ_get_first_cpattern */


/********************************************************************
* FUNCTION typ_get_next_cpattern
* 
* Get the next pattern struct for a typdef
* Const version
*
* INPUTS:
*   curpat  == current typ_pattern_t to check
*
* RETURNS:
*   pointer to next pattern struct or NULL if none
*********************************************************************/
const typ_pattern_t *
    typ_get_next_cpattern (const typ_pattern_t *curpat)
{

#ifdef DEBUG
    if (!curpat) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (typ_pattern_t *)dlq_nextEntry(curpat);

}  /* typ_get_next_cpattern */


/********************************************************************
* FUNCTION typ_get_pattern_count
* 
* Get the number of pattern structs in a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   count of the typ_pattern_t structs found
*********************************************************************/
uint32
    typ_get_pattern_count (const typ_def_t *typdef)
{
    const typ_pattern_t *pat;
    uint32               cnt;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    cnt = 1;    
    pat = typ_get_first_cpattern(typdef);
    if (!pat) {
        return 0;
    }
    pat = typ_get_next_cpattern(pat);
    while (pat) {
        cnt++;
        pat = typ_get_next_cpattern(pat);
    }
    return cnt;

}  /* typ_get_pattern_count */


/********************************************************************
* FUNCTION typ_get_range_errinfo
* 
* Get the range errinfo for a typdef
*
* INPUTS:
*   typdef == typ_def_t to check
*
* RETURNS:
*   pointer to pattern string or NULL if none
*********************************************************************/
ncx_errinfo_t *
    typ_get_range_errinfo (typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            return &typdef->def.named.newtyp->def.simple.range.range_errinfo;
        }
        break;
    case NCX_CL_SIMPLE:
        return &typdef->def.simple.range.range_errinfo;
    case NCX_CL_REF:
    default:
        ;
    }
    return NULL;

}  /* typ_get_range_errinfo */


/********************************************************************
 * Clean a queue of typ_template_t structs
 *
 * \param que Q of typ_template_t to clean
 *
 *********************************************************************/
void typ_clean_typeQ (dlq_hdr_t *que)
{
    if ( !que ) {
        return;
    }

    while (!dlq_empty(que)) {
        typ_template_t *typ = (typ_template_t *)dlq_deque(que);
        typ_free_template(typ);
    }
}  /* typ_clean_typeQ */


/********************************************************************
* FUNCTION typ_ok_for_inline_index
* 
* Check if the base type is okay to use in an inline index decl
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
boolean
    typ_ok_for_inline_index (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_BITS:
    case NCX_BT_ENUM:
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
    case NCX_BT_IDREF:
    case NCX_BT_SLIST:
    case NCX_BT_UNION:
        return TRUE;
    case NCX_BT_EMPTY:
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_NONE:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
}  /* typ_ok_for_inline_index */


/********************************************************************
* FUNCTION typ_ok_for_metadata
* 
* Check if the base type is okay to use in an XML attribute
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
boolean typ_ok_for_metadata (ncx_btype_t btyp)
{
    return typ_ok_for_inline_index(btyp);
}  /* typ_ok_for_metadata */


/********************************************************************
* FUNCTION typ_ok_for_index
* 
* Check if the base type is okay to use in an index decl
*
* INPUTS:
*     typdef == type def struct to check
*
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
boolean
    typ_ok_for_index (const typ_def_t  *typdef)
{
    ncx_btype_t            btyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        btyp = typ_get_basetype(typdef);
        return typ_ok_for_inline_index(btyp);
    case NCX_CL_SIMPLE:
        btyp = typ_get_basetype(typdef);
        return typ_ok_for_inline_index(btyp);
    case NCX_CL_NAMED:
        if (typdef->def.named.typ) {
            return typ_ok_for_index(&typdef->def.named.typ->typdef);
        } else {
            return FALSE;
        }
    case NCX_CL_REF:
        return typ_ok_for_index(typdef->def.ref.typdef);
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

    /*NOTREACHED*/

}  /* typ_ok_for_index */


/********************************************************************
* FUNCTION typ_ok_for_union
* 
* Check if the base type is okay to use in an union decl
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
boolean
    typ_ok_for_union (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_BITS:
    case NCX_BT_ENUM:
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
    case NCX_BT_UNION:
    case NCX_BT_IDREF:
        return TRUE;
    case NCX_BT_EMPTY:
    case NCX_BT_LEAFREF:
    case NCX_BT_ANY:
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_NONE:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
}  /* typ_ok_for_union */


/********************************************************************
* FUNCTION typ_ok
* 
* Check if the typdef chain has any errors
* Checks the named types in the typdef chain to
* see if they were already flagged as invalid
*
* INPUTS:
*     typdef == starting typdef to check
* RETURNS:
*     TRUE if okay, FALSE if any errors so far
*********************************************************************/
boolean
    typ_ok (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (typdef->tclass) {
    case NCX_CL_BASE:
    case NCX_CL_SIMPLE:
    case NCX_CL_COMPLEX:
        return TRUE;
    case NCX_CL_NAMED:
        if (typdef->def.named.typ) {
            if (typdef->def.named.typ->res != NO_ERR) {
                return FALSE;
            } else {
                return typ_ok(&typdef->def.named.typ->typdef);
            }
        } else {
            return FALSE;
        }
    case NCX_CL_REF:
        if (typdef->def.ref.typdef) {
            return typ_ok(typdef->def.ref.typdef);
        } else {
            return FALSE;
        }
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
}  /* typ_ok */


/********************************************************************
* FUNCTION typ_ok_for_xsdlist
* 
* Check if the base type is okay to use in an ncx:xsdlist typedef
*
* INPUTS:
*     btyp == base type enum
* RETURNS:
*     TRUE if okay, FALSE if not
*********************************************************************/
boolean
    typ_ok_for_xsdlist (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_BITS:
        return FALSE;
    case NCX_BT_ENUM:
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
    case NCX_BT_STRING:   /*** really NMTOKEN, not string ***/
        return TRUE;
    case NCX_BT_BINARY:
    case NCX_BT_INSTANCE_ID:
        return FALSE;
    case NCX_BT_UNION:
    case NCX_BT_LEAFREF:
    case NCX_BT_EMPTY:
        return FALSE;
    case NCX_BT_ANY:
    case NCX_BT_NONE:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }
}  /* typ_ok_for_xsdlist */


/********************************************************************
* FUNCTION typ_get_leafref_path
* 
*   Get the path argument for the leafref data type
*
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the path argument or NULL if some error
*********************************************************************/
const xmlChar *
    typ_get_leafref_path (const typ_def_t *typdef)
{
    const xmlChar          *pathstr;
    const typ_def_t        *tdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pathstr = NULL;

    if (typ_get_basetype(typdef) != NCX_BT_LEAFREF) {
        return NULL;
    }

    tdef = typ_get_cbase_typdef(typdef);
    if (tdef && tdef->def.simple.xleafref) {
        pathstr = tdef->def.simple.xleafref->exprstr;
    }
    return pathstr;

}   /* typ_get_leafref_path */


/********************************************************************
* FUNCTION typ_get_leafref_path_addr
* 
*   Get the address of the path argument for the leafref data type
*
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the path argument or NULL if some error
*********************************************************************/
const void *
    typ_get_leafref_path_addr (const typ_def_t *typdef)
{
    const void         *pathstr;
    const typ_def_t    *tdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pathstr = NULL;

    if (typ_get_basetype(typdef) != NCX_BT_LEAFREF) {
        return NULL;
    }

    tdef = typ_get_cbase_typdef(typdef);
    if (tdef && tdef->def.simple.xleafref) {
        pathstr = &tdef->def.simple.xleafref->exprstr;
    }
    return pathstr;

}   /* typ_get_leafref_path_addr */


/********************************************************************
* FUNCTION typ_get_leafref_pcb
* 
*   Get the XPath parser control block for the leafref data type
*   returns xpath_pcb_t but cannot import due to H file loop
* INPUTS:
*    typdef == typdef for the the leafref
*
* RETURNS:
*    pointer to the PCB struct or NULL if some error
*********************************************************************/
struct xpath_pcb_t_ *
    typ_get_leafref_pcb (typ_def_t *typdef)
{
    typ_def_t        *tdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ_get_basetype(typdef) != NCX_BT_LEAFREF) {
        return NULL;
    }

    tdef = typ_get_base_typdef(typdef);
    if (tdef && tdef->def.simple.xleafref) {
        return tdef->def.simple.xleafref;
    } else {
        return NULL;
    }
    /*NOTREACHED*/

}   /* typ_get_leafref_pcb */


/********************************************************************
* FUNCTION typ_get_constrained
* 
*   Get the constrained true/false field for the data type
*   leafref or instance-identifier constrained flag
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
* RETURNS:
*    TRUE if constrained; FALSE if not
*********************************************************************/
boolean
    typ_get_constrained (const typ_def_t *typdef)
{
    const typ_def_t        *tdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    tdef = typ_get_cbase_typdef(typdef);
    if (tdef) {
        return tdef->def.simple.constrained;
    } else {
        return FALSE;
    }
    /*NOTREACHED*/

}   /* typ_get_constrained */


/********************************************************************
* FUNCTION typ_set_xref_typdef
* 
*   Set the target typdef for a leafref or instance-identifier
*   NCX_BT_LEAFREF or NCX_BT_INSTANCE_ID
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
*********************************************************************/
void
    typ_set_xref_typdef (typ_def_t *typdef,
                         typ_def_t *target)
{
    typ_def_t        *tdef;
    ncx_btype_t       btyp;

#ifdef DEBUG
    if (!typdef || !target) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    btyp = typ_get_basetype(typdef);
    
    if (!(btyp == NCX_BT_LEAFREF || btyp == NCX_BT_INSTANCE_ID)) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    tdef = typ_get_base_typdef(typdef);
    if (tdef && tdef->tclass == NCX_CL_SIMPLE) {
        tdef->def.simple.xrefdef = target;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}   /* typ_set_xref_typdef */


/********************************************************************
* FUNCTION typ_get_xref_typdef
* 
*   Get the xrefdef target typdef from a leafref 
*   or instance-identifier
*   NCX_BT_LEAFREF or NCX_BT_INSTANCE_ID
*
* INPUTS:
*    typdef == typdef for the the leafref or instance-identifier
*
* RETURNS:
*    pointer to the PCB struct or NULL if some error
*********************************************************************/
typ_def_t *
    typ_get_xref_typdef (typ_def_t *typdef)
{
    typ_def_t        *tdef;
    ncx_btype_t       btyp;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    btyp = typ_get_basetype(typdef);
    
    if (!(btyp == NCX_BT_LEAFREF || btyp == NCX_BT_INSTANCE_ID)) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    tdef = typ_get_base_typdef(typdef);
    if (tdef && tdef->tclass == NCX_CL_SIMPLE) {
        btyp = typ_get_basetype(tdef->def.simple.xrefdef);
        if (btyp == NCX_BT_LEAFREF) {
            return typ_get_xref_typdef(tdef->def.simple.xrefdef);
        } else {
            return tdef->def.simple.xrefdef;
        }
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

}   /* typ_get_xref_typdef */


/********************************************************************
* FUNCTION typ_has_subclauses
* 
*   Check if the specified typdef has any sub-clauses
*   Used by yangdump to reverse-engineer the YANG from the typdef
*   If any appinfo clauses present, then the result will be TRUE
*
* INPUTS:
*    typdef == typdef to check
*
* RETURNS:
*    TRUE if any sub-clauses, FALSE otherwise
*********************************************************************/
boolean
    typ_has_subclauses (const typ_def_t *typdef)
{

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    if (!dlq_empty(&typdef->appinfoQ)) {
        return TRUE;
    }

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        return FALSE;
    case NCX_CL_SIMPLE:
        switch (typdef->def.simple.btyp) {
        case NCX_BT_UNION:
        case NCX_BT_BITS:
        case NCX_BT_ENUM:
            return TRUE;
        case NCX_BT_EMPTY:
        case NCX_BT_BOOLEAN:
        case NCX_BT_INSTANCE_ID:
            return FALSE;
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_INT64:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
        case NCX_BT_UINT64:
        case NCX_BT_FLOAT64:
            return !dlq_empty(&typdef->def.simple.range.rangeQ);
        case NCX_BT_DECIMAL64:
            return TRUE;
        case NCX_BT_STRING:
        case NCX_BT_BINARY:
            if (!dlq_empty(&typdef->def.simple.range.rangeQ)) {
                return TRUE;
            }
            if (!dlq_empty(&typdef->def.simple.patternQ)) {
                return TRUE;
            }
            return !dlq_empty(&typdef->def.simple.valQ);
        case NCX_BT_SLIST:
        case NCX_BT_LEAFREF:
        case NCX_BT_IDREF:
            return TRUE;
        case NCX_BT_NONE:
        default:
            return FALSE;
        }
    case NCX_CL_COMPLEX:
        return TRUE;
    case NCX_CL_NAMED:
        return (typdef->def.named.newtyp) ? TRUE : FALSE;
    case NCX_CL_REF:
        return FALSE;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return FALSE;
    }

}   /* typ_has_subclauses */


/********************************************************************
* FUNCTION typ_get_idref
* 
* Get the idref field if this is an NCX_BT_IDREF typdef
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     pointer to idref field or NULL if wrong type
*********************************************************************/
typ_idref_t *
    typ_get_idref (typ_def_t  *typdef)
{
   typ_def_t  *basetypdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ_get_basetype(typdef) != NCX_BT_IDREF) {
        return NULL;
    }

    basetypdef = typ_get_base_typdef(typdef);
    return &basetypdef->def.simple.idref;

}  /* typ_get_idref */


/********************************************************************
* FUNCTION typ_get_cidref
* 
* Get the idref field if this is an NCX_BT_IDREF typdef
* Const version
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     pointer to idref field or NULL if wrong type
*********************************************************************/
const typ_idref_t *
    typ_get_cidref (const typ_def_t  *typdef)
{
    const typ_def_t  *basetypdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    if (typ_get_basetype(typdef) != NCX_BT_IDREF) {
        return NULL;
    }

    basetypdef = typ_get_cbase_typdef(typdef);
    return &basetypdef->def.simple.idref;

}  /* typ_get_cidref */


/********************************************************************
* FUNCTION typ_get_fraction_digits
* 
* Get the fraction-digits field from the typdef chain
* typdef must be an NCX_BT_DECIMAL64 or 0 will be returned
* valid values are 1..18
*
* INPUTS:
*     typdef == typdef to  check
*
* RETURNS:
*     number of fixed decimal digits expected (1..18)
*     0 if some error
*********************************************************************/
uint8
    typ_get_fraction_digits (const typ_def_t  *typdef)
{
    const typ_def_t  *basetypdef;

#ifdef DEBUG
    if (!typdef) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    if (typ_get_basetype(typdef) != NCX_BT_DECIMAL64) {
        return 0;
    }

    basetypdef = typ_get_cbase_typdef(typdef);
    return basetypdef->def.simple.digits;

}  /* typ_get_fraction_digits */


/********************************************************************
* FUNCTION typ_set_fraction_digits
* 
* Set the fraction-digits field from the typdef chain
*
* INPUTS:
*   typdef == typdef to set (must be TYP_CL_SIMPLE)
*   digits == digits value to set
*
* RETURNS:
*     status
*********************************************************************/
status_t
    typ_set_fraction_digits (typ_def_t  *typdef,
                             uint8 digits)
{
#ifdef DEBUG
    if (!typdef) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
    if (typdef->tclass != NCX_CL_SIMPLE) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
#endif
    
    if (digits < TYP_DEC64_MIN_DIGITS ||
        digits > TYP_DEC64_MAX_DIGITS) {
        return ERR_NCX_INVALID_VALUE;
    }

    typdef->def.simple.digits = digits;

    return NO_ERR;

}  /* typ_set_fraction_digits */


/********************************************************************
* FUNCTION typ_get_typ_linenum
* 
* Get the line number for the typ_template_t
*
* INPUTS:
*   typ == type template to check

* RETURNS:
*   line number
*********************************************************************/
uint32
    typ_get_typ_linenum (const typ_template_t  *typ)
{
#ifdef DEBUG
    if (!typ) { 
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    return typ->tkerr.linenum;

}  /* typ_get_typ_linenum */


/********************************************************************
* FUNCTION typ_get_typdef_linenum
* 
* Get the line number for the typ_def_t
*
* INPUTS:
*   typ == typdef to check

* RETURNS:
*   line number
*********************************************************************/
uint32
    typ_get_typdef_linenum (const typ_def_t  *typdef)
{
#ifdef DEBUG
    if (!typdef) { 
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    return typdef->tkerr.linenum;

}  /* typ_get_typdef_linenum */


/* END typ.c */
