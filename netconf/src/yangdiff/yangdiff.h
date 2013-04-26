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
#ifndef _H_yangdiff
#define _H_yangdiff

/*  FILE: yangdiff.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Exports yangdiff.ncx conversion CLI parameter struct
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
01-mar-08    abb      Begun; moved from ncx/ncxtypes.h

*/

#include <xmlstring.h>

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/
#define YANGDIFF_PROGNAME   (const xmlChar *)"yangdiff"

#define YANGDIFF_BUFFSIZE          0x7fff
#define YANGDIFF_DEF_MAXSIZE       20

#define YANGDIFF_DIFFTYPE_TERSE    (const xmlChar *)"terse"
#define YANGDIFF_DIFFTYPE_NORMAL   (const xmlChar *)"normal"
#define YANGDIFF_DIFFTYPE_REVISION (const xmlChar *)"revision"

#define YANGDIFF_DEF_OUTPUT        (const xmlChar *)"stdout"
#define YANGDIFF_DEF_DIFFTYPE      (const xmlChar *)"summary"
#define YANGDIFF_DEF_DT            YANGDIFF_DT_NORMAL
#define YANGDIFF_DEF_CONFIG        (const xmlChar *)"/etc/yuma/yangdiff.conf"
#define YANGDIFF_DEF_FILENAME      (const xmlChar *)"yangdiff.log"

#define YANGDIFF_MOD               (const xmlChar *)"yangdiff"
#define YANGDIFF_CONTAINER         (const xmlChar *)"yangdiff"

#define YANGDIFF_PARM_CONFIG        (const xmlChar *)"config"
#define YANGDIFF_PARM_NEW           (const xmlChar *)"new"
#define YANGDIFF_PARM_NEWPATH       (const xmlChar *)"newpath"
#define YANGDIFF_PARM_OLD           (const xmlChar *)"old"
#define YANGDIFF_PARM_OLDPATH       (const xmlChar *)"oldpath"

#define YANGDIFF_PARM_DIFFTYPE      (const xmlChar *)"difftype"
#define YANGDIFF_PARM_OUTPUT        (const xmlChar *)"output"
#define YANGDIFF_PARM_INDENT        (const xmlChar *)"indent"
#define YANGDIFF_PARM_HEADER        (const xmlChar *)"header"
#define YANGDIFF_LINE (const xmlChar *)\
"\n==================================================================="

#define FROM_STR (const xmlChar *)"from "
#define TO_STR (const xmlChar *)"to "
#define A_STR (const xmlChar *)"A "
#define D_STR (const xmlChar *)"D "
#define M_STR (const xmlChar *)"M "
#define ADD_STR (const xmlChar *)"- Added "
#define DEL_STR (const xmlChar *)"- Removed "
#define MOD_STR (const xmlChar *)"- Changed "



/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/
typedef enum yangdiff_difftype_t_ {
    YANGDIFF_DT_NONE,
    YANGDIFF_DT_TERSE,
    YANGDIFF_DT_NORMAL,
    YANGDIFF_DT_REVISION
} yangdiff_difftype_t;

/* struct of yangdiff conversion parameters */
typedef struct yangdiff_diffparms_t_ {
    /* external parameters */
    xmlChar        *old;        /* not malloced */
    xmlChar        *new;        /* not malloced */
    const xmlChar  *oldpath;
    const xmlChar  *modpath;
    const xmlChar  *newpath;
    const xmlChar  *output;
    const xmlChar  *difftype;
    const xmlChar  *logfilename;
    const xmlChar  *config;
    boolean         helpmode;
    help_mode_t     helpsubmode;
    int32           indent;
    boolean         logappend;
    log_debug_t     log_level;
    boolean         header;
    boolean         subdirs;
    boolean         versionmode;
    uint32          maxlen;

    /* internal vars */
    dlq_hdr_t       oldmodQ;
    dlq_hdr_t       newmodQ;
    ses_cb_t       *scb;
    xmlChar        *curold;
    xmlChar        *curnew;
    xmlChar        *buff;
    ncx_module_t   *oldmod;
    ncx_module_t   *newmod;
    xmlChar        *full_old;
    xmlChar        *full_new;
    xmlChar        *full_output;
    xmlChar        *full_logfilename;

    uint32          bufflen;
    uint32          modbufflen;
    boolean         firstdone;
    boolean         old_isdir;
    boolean         new_isdir;
    boolean         output_isdir;

    yangdiff_difftype_t edifftype;
    int32           curindent;
} yangdiff_diffparms_t;

/* change description block */
typedef struct yangdiff_cdb_t_ {
    const xmlChar *fieldname;
    const xmlChar *chtyp;
    const xmlChar *oldval;
    const xmlChar *newval;
    const xmlChar *useval;
    boolean        changed;
} yangdiff_cdb_t;

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangdiff */
