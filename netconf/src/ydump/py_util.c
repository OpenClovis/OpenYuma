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
/*  FILE: py_util.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11-mar-10    abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_c_util
#include  "c_util.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_ncx
#include  "ncx.h"
#endif

#ifndef _H_ncx_str
#include  "ncx_str.h"
#endif

#ifndef _H_ncxmod
#include  "ncxmod.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_py_util
#include  "py_util.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif

#ifndef _H_yangdump_util
#include "yangdump_util.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION get_sa_datatype
* 
* Get the SQLAlchemy data type for the associated NCX internal type
*
* INPUTS:
*   btyp == NCX base type
*
* RETURNS:
*   SA type enumeration string
*********************************************************************/
const xmlChar *
    get_sa_datatype (ncx_btype_t btyp)
{
    switch (btyp) {
    case NCX_BT_NONE:
        return NCX_EL_NONE;
    case NCX_BT_ANY:
        return SA_PICKLE_TYPE;
    case NCX_BT_BITS:
    case NCX_BT_ENUM:
    case NCX_BT_EMPTY:
        return SA_STRING;
    case NCX_BT_BOOLEAN:
        return SA_BOOLEAN;
    case NCX_BT_INT8:
    case NCX_BT_INT16:
        return SA_INTEGER;
    case NCX_BT_INT32:
    case NCX_BT_INT64:
        return SA_INTEGER;
    case NCX_BT_UINT8:
    case NCX_BT_UINT16:
    case NCX_BT_UINT32:
    case NCX_BT_UINT64:
        return SA_INTEGER;
    case NCX_BT_DECIMAL64:
        return SA_NUMERIC;
    case NCX_BT_FLOAT64:
        return SA_FLOAT;
    case NCX_BT_STRING:
        return SA_STRING;
    case NCX_BT_BINARY:
        return SA_BINARY;
    case NCX_BT_INSTANCE_ID:
    case NCX_BT_UNION:
    case NCX_BT_LEAFREF:
    case NCX_BT_IDREF:
    case NCX_BT_SLIST:
        return SA_STRING;
    case NCX_BT_CONTAINER:
    case NCX_BT_CHOICE:
    case NCX_BT_CASE:
    case NCX_BT_LIST:
    case NCX_BT_EXTERN:
        return SA_PICKLE_TYPE;
    case NCX_BT_INTERN:
        return SA_STRING;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_EL_NONE;
    }
    /*NOTREACHED*/

} /* get_sa_datatype */


/********************************************************************
* FUNCTION write_py_header
* 
* Write the Python file header
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
void
    write_py_header (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp)
{
    int32                     indent;

    indent = cp->indent;

    if (indent <= 0) {
        indent = NCX_DEF_INDENT;
    }

    /* utf-8 decl */
    ses_putstr(scb, PY_CODING);

    /* start comment */
    ses_putchar(scb, '\n');
    ses_putstr(scb, PY_COMMENT);    

    /* banner comments */
    ses_putstr(scb, COPYRIGHT_HEADER);

    /* generater tag */
    write_banner_session_ex(scb, FALSE);

    /* module name */
    if (mod->ismod) {
        ses_putstr_indent(scb, YANG_K_MODULE, indent);
    } else {
        ses_putstr_indent(scb, YANG_K_SUBMODULE, indent);
    }
    ses_putchar(scb, ' ');
    ses_putstr(scb, mod->name);

    /* version */
    if (mod->version) {
        write_c_simple_str(scb, 
                           YANG_K_REVISION,
                           mod->version, 
                           indent,
                           0);
        ses_putchar(scb, '\n');
    }

    /* namespace or belongs-to */
    if (mod->ismod) {
        write_c_simple_str(scb, 
                           YANG_K_NAMESPACE, 
                           mod->ns,
                           indent,
                           0);
    } else {
        write_c_simple_str(scb, 
                           YANG_K_BELONGS_TO, 
                           mod->belongs,
                           indent,
                           0);
    }

    /* organization */
    if (mod->organization) {
        write_c_simple_str(scb, 
                           YANG_K_ORGANIZATION,
                           mod->organization, 
                           indent,
                           0);
    }

    ses_putchar(scb, '\n');

    /* end comment */
    ses_putchar(scb, '\n');
    ses_putstr(scb, PY_COMMENT);
    ses_putchar(scb, '\n');

} /* write_py_header */


/********************************************************************
* FUNCTION write_py_footer
* 
* Write the Python file footer
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*
*********************************************************************/
void
    write_py_footer (ses_cb_t *scb,
                    const ncx_module_t *mod)
{
    ses_putchar(scb, '\n');
    ses_putstr(scb, PY_COMMENT);
    ses_putstr(scb, (const xmlChar *)" END ");
    write_c_safe_str(scb, ncx_get_modname(mod));
    ses_putstr(scb, (const xmlChar *)".py ");
    ses_putstr(scb, PY_COMMENT);
    ses_putchar(scb, '\n');    

} /* write_py_footer */


/* END py_util.c */



