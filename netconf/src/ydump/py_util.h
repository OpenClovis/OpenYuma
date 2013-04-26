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
#ifndef _H_py_util
#define _H_py_util

/*  FILE: py_util.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  YANGDUMP Python code generation utilities
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
11-mar-10    abb      Begun

*/

#include <xmlstring.h>

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

#define PY_COMMENT (const xmlChar *)"\"\"\" "

/* SQL Alchemy 0.5.6 generic data types */
#define SA_STRING  (const xmlChar *)"String"
#define SA_UNICODE (const xmlChar *)"Unicode"
#define SA_TEXT    (const xmlChar *)"Text"
#define SA_UNICODE_TEXT  (const xmlChar *)"UnicodeText"
#define SA_INTEGER (const xmlChar *)"Integer"
#define SA_SMALL_INTEGER (const xmlChar *)"SmallInteger"
#define SA_NUMERIC (const xmlChar *)"Numeric"
#define SA_INTEGER (const xmlChar *)"Integer"
#define SA_FLOAT   (const xmlChar *)"DateTime"
#define SA_DATE_TIME (const xmlChar *)"DateTime"
#define SA_DATE (const xmlChar *)"Date"
#define SA_TIME (const xmlChar *)"Time"
#define SA_INTERVAL (const xmlChar *)"Interval"
#define SA_BOOLEAN (const xmlChar *)"Boolean"
#define SA_BINARY (const xmlChar *)"Binary"
#define SA_PICKLE_TYPE (const xmlChar *)"PickleType"


/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                         F U N C T I O N S                         *
*                                                                   *
*********************************************************************/


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
extern const xmlChar *
    get_sa_datatype (ncx_btype_t btyp);


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
extern void
    write_py_header (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp);


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
extern void
    write_py_footer (ses_cb_t *scb,
                     const ncx_module_t *mod);

#if 0
/*******************************************************************
* FUNCTION write_py_objtype
* 
* Generate the Python data type for the YANG data type
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object template to check
*
**********************************************************************/
extern void
    write_py_objtype (ses_cb_t *scb,
                      const obj_template_t *obj);

#endif

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_py_util */
