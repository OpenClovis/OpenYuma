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
/*  FILE: status.c
 *
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
22-dec-01    abb      begin
22-apr-05    abb      updated for netconf project

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <xmlstring.h>

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_log
#include  "log.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define MAX_ERR_FILENAME     64
#define MAX_ERR_MSG_LEN      256
#define MAX_ERR_LEVEL        15

/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/

/* the array of form IDs and form handlers */
typedef struct error_snapshot_t_ {
    int              linenum;
    int              sqlError;
    status_t         status;
    char             filename[MAX_ERR_FILENAME];
    const char      *msg;
} error_snapshot_t;

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/* used for error_stack, when debug_level == NONE */
static int error_level;
static error_snapshot_t error_stack[MAX_ERR_LEVEL];
static uint32 error_count;


/********************************************************************
* FUNCTION set_error 
*
*  Generate an error stack entry or if log_error is enabled,
*  then log the error report right now.
*
* Used to flag internal code errors
*
* DO NOT USE THIS FUNCTION DIRECTLY!
* USE THE SET_ERROR MACRO INSTEAD!
*
* INPUTS:
*    filename == C filename that caused the error
*    linenum -- line number in the C file that caused the error
*    status == internal error code
*    sqlError = mySQL error code (deprecated)
*
* RETURNS
*    the 'status' parameter will be returned
*********************************************************************/
status_t set_error (const char *filename, 
                    int linenum, 
                    status_t status, 
                    int sqlError)
{
    
    if (log_get_debug_level() > LOG_DEBUG_NONE) {
        log_error("\nE0:\n   %s:%d\n   Error %d: %s\n",
                  filename,
                  linenum,
                  status,
                  get_error_string(status));
    } else if (error_level < MAX_ERR_LEVEL) {
        error_stack[error_level].linenum = linenum;
        error_stack[error_level].sqlError = sqlError;
        error_stack[error_level].status = status;
        strncpy(error_stack[error_level].filename, 
                filename, MAX_ERR_FILENAME-1);
        error_stack[error_level].filename[MAX_ERR_FILENAME-1] = 0;      
        error_stack[error_level].msg = get_error_string(status);
        error_level++;
    }

    error_count++;

    return status;

}   /* set_error */


/********************************************************************
* FUNCTION print_errors
*
* Dump any entries stored in the error_stack.
*
*********************************************************************/
void print_errors (void)
{
    int  i;

    for (i=0; i < error_level; i++) {
        log_error("\nE%d:\n   %s:%d\n   Error %d: %s", i,
                  error_stack[i].filename,
                  error_stack[i].linenum,
                  error_stack[i].status,
                  (error_stack[i].msg) ? error_stack[i].msg : "");
        if (i==error_level-1) {
            log_error("\n");
        }
    }

}   /* print_errors */


/********************************************************************
* FUNCTION clear_errors
*
* Clear the error_stack if it has any errors stored in it
*
*********************************************************************/
void clear_errors (void)
{
    int  i;

    for (i=0; i < error_level; i++) {
        error_stack[i].linenum = 0;
        error_stack[i].sqlError = 0;
        error_stack[i].status = NO_ERR;
        error_stack[i].filename[0] = 0;
        error_stack[i].msg = NULL;
    }
    error_level = 0;

}   /* clear_errors */


/********************************************************************
* FUNCTION get_errtyp
*
* Get the error classification for the result code
*
* INPUTS:
*   res == result code
*
* RETURNS:
*     error type for 'res' parameter
*********************************************************************/
errtyp_t 
    get_errtyp (status_t  res)
{
    if (res == NO_ERR) {
        return ERR_TYP_NONE;
    } else if (res < ERR_LAST_INT_ERR) {
        return ERR_TYP_INTERNAL;
    } else if (res < ERR_LAST_SYS_ERR) {
        return ERR_TYP_SYSTEM;
    } else if (res < ERR_LAST_USR_ERR) {
        return ERR_TYP_USER;
    } else if (res < ERR_LAST_WARN) {
        return ERR_TYP_WARN;
    } else {
        return ERR_TYP_INFO;
    }
    /*NOTREACHED*/

}   /* get_errtyp */


/********************************************************************
* FUNCTION get_error_string
*
* Get the error message for a specific internal error
*
* INPUTS:
*   res == internal status_t error code
*
* RETURNS:
*   const pointer to error message
*********************************************************************/
const char *
    get_error_string (status_t res)
{
    switch (res) {
    case NO_ERR:
        return "ok";
    case ERR_END_OF_FILE:
        return "EOF reached";
    case ERR_INTERNAL_PTR:
        return "NULL pointer";
    case ERR_INTERNAL_MEM:
        return "malloc failed";
    case ERR_INTERNAL_VAL:
        return "invalid internal value";
    case ERR_INTERNAL_BUFF:
        return "internal buffering failed";
    case ERR_INTERNAL_QDEL:
        return "invalid queue deletion";
    case ERR_INTERNAL_INIT_SEQ:
        return "wrong init sequence";
    case ERR_QNODE_NOT_HDR:
        return "queue node not header";
    case ERR_QNODE_NOT_DATA:
        return "queue node not data";
    case ERR_BAD_QLINK:
        return "invalid queue header";
    case ERR_Q_ALREADY:
        return "entry already queued";
    case ERR_TOO_MANY_ENTRIES:
        return "too many entries";
    case ERR_XML2_FAILED:
        return "libxml2 operation failed";
    case ERR_FIL_OPEN:
        return "cannot open file";
    case ERR_FIL_READ:
        return "cannot read file";
    case ERR_FIL_CLOSE:
        return "cannot close file";
    case ERR_FIL_WRITE:
        return "cannot write file";
    case ERR_FIL_CHDIR:
        return "cannot change directory";
    case ERR_FIL_STAT:
        return "cannot stat file";
    case ERR_BUFF_OVFL:
        return "buffer overflow error";
    case ERR_FIL_DELETE:
        return "cannot delete file";
    case ERR_FIL_SETPOS:
        return "cannot access file";
    case ERR_DB_CONNECT_FAILED:
        return "db connect failed";
    case ERR_DB_ENTRY_EXISTS:
        return "db entry exists";
    case ERR_DB_NOT_FOUND:
        return "db not found";
    case ERR_DB_QUERY_FAILED:
        return "db query failed";
    case ERR_DB_DELETE_FAILED:
        return "db delete failed";
    case ERR_DB_WRONG_CKSUM:
        return "wrong checksum";
    case ERR_DB_WRONG_TAGTYPE:
        return "wrong tag type";
    case ERR_DB_READ_FAILED:
        return "db read failed";
    case ERR_DB_WRITE_FAILED:
        return "db write failed";
    case ERR_DB_INIT_FAILED:
        return "db init failed";
    case ERR_TR_BEEP_INIT:
        return "beep init failed";
    case ERR_TR_BEEP_NC_INIT:
        return "beep init nc failed";
    case ERR_XML_READER_INTERNAL:
        return "xml reader internal";
    case ERR_OPEN_DIR_FAILED:
        return "open directory failed";
    case ERR_READ_DIR_FAILED:
        return "read directory failed";

    case ERR_NO_CFGFILE:
        return "no config file";
    case ERR_NO_SRCFILE:
        return "no source file";
    case ERR_PARSPOST_RD_INPUT:
        return "POST read input";
    case ERR_FIL_BAD_DRIVE:
        return "bad drive";
    case ERR_FIL_BAD_PATH:
        return "bad path";
    case ERR_FIL_BAD_FILENAME:
        return "bad filename";
    case ERR_DUP_VALPAIR:
        return "duplicate value pair";
    case ERR_PAGE_NOT_HANDLED:
        return "page not handled";
    case ERR_PAGE_ACCESS_DENIED:
        return "page access denied";
    case ERR_MISSING_FORM_PARAMS:
        return "missing form params";
    case ERR_FORM_STATE:
        return "invalid form state";
    case ERR_DUP_NS:
        return "duplicate namespace";
    case ERR_XML_READER_START_FAILED:
        return "xml reader start failed";
    case ERR_XML_READER_READ:
        return "xml reader read failed";
    case ERR_XML_READER_NODETYP:
        return "wrong XML node type";
    case ERR_XML_READER_NULLNAME:
        return "xml reader null name";
    case ERR_XML_READER_NULLVAL:
        return "xml reader null value";
    case ERR_XML_READER_WRONGNAME:
        return "xml reader wrong name";
    case ERR_XML_READER_WRONGVAL:
        return "xml reader wrong value";
    case ERR_XML_READER_WRONGEL:
        return "xml reader wrong element";
    case ERR_XML_READER_EXTRANODES:
        return "xml reader extra nodes";
    case ERR_XML_READER_EOF:
        return "xml reader EOF";
    case ERR_NCX_WRONG_LEN:
        return "wrong length";
    case ERR_NCX_ENTRY_EXISTS:
        return "entry exists";
    case ERR_NCX_DUP_ENTRY:
        return "duplicate entry";
    case ERR_NCX_NOT_FOUND:
        return "not found";
    case ERR_NCX_MISSING_FILE:
        return "missing file";
    case ERR_NCX_UNKNOWN_PARM:
        return "unknown parameter";
    case ERR_NCX_INVALID_NAME:
        return "invalid name";
    case ERR_NCX_UNKNOWN_NS:
        return "unknown namespace";
    case ERR_NCX_WRONG_NS:
        return "wrong namespace";
    case ERR_NCX_WRONG_TYPE:
        return "wrong data type";
    case ERR_NCX_WRONG_VAL:
        return "wrong value";
    case ERR_NCX_MISSING_PARM:
        return "missing parameter";
    case ERR_NCX_EXTRA_PARM:
        return "extra parameter";
    case ERR_NCX_EMPTY_VAL:
        return "empty value";
    case ERR_NCX_MOD_NOT_FOUND:
        return "module not found";
    case ERR_NCX_LEN_EXCEEDED:
        return "max length exceeded";
    case ERR_NCX_INVALID_TOKEN:
        return "invalid token";
    case ERR_NCX_UNENDED_QSTRING:
        return "unended quoted string";
    case ERR_NCX_READ_FAILED:
        return "read failed";
    case ERR_NCX_INVALID_NUM:
        return "invalid number";
    case ERR_NCX_INVALID_HEXNUM:
        return "invalid hex number";
    case ERR_NCX_INVALID_REALNUM:
        return "invalid real number";
    case ERR_NCX_EOF:
        return "EOF reached";
    case ERR_NCX_WRONG_TKTYPE:
        return "wrong token type";
    case ERR_NCX_WRONG_TKVAL:
        return "wrong token value";
    case ERR_NCX_BUFF_SHORT:
        return "buffer overflow";
    case ERR_NCX_INVALID_RANGE:
        return "invalid range";
    case ERR_NCX_OVERLAP_RANGE:
        return "overlapping range";
    case ERR_NCX_DEF_NOT_FOUND:
        return "definition not found";
    case ERR_NCX_DEFSEG_NOT_FOUND:
        return "definition segment not found";
    case ERR_NCX_TYPE_NOT_INDEX:
        return "type not allowed in index";
    case ERR_NCX_INDEX_TYPE_NOT_FOUND:
        return "index type not found";
    case ERR_NCX_TYPE_NOT_MDATA:
        return "type not mdata";
    case ERR_NCX_MDATA_NOT_ALLOWED:
        return "meta-data not allowed";
    case ERR_NCX_TOP_NOT_FOUND:
        return "top not found";

    /*** match netconf errors here ***/
    case ERR_NCX_IN_USE:
        return "resource in use";
    case ERR_NCX_INVALID_VALUE:
        return "invalid value";
    case ERR_NCX_TOO_BIG:
        return "too big";
    case ERR_NCX_MISSING_ATTRIBUTE:
        return "missing attribute";
    case ERR_NCX_BAD_ATTRIBUTE:
        return "bad attribute";
    case ERR_NCX_UNKNOWN_ATTRIBUTE:
        return "unknown attribute";
    case ERR_NCX_MISSING_ELEMENT:
        return "missing element";
    case ERR_NCX_BAD_ELEMENT:
        return "bad element";
    case ERR_NCX_UNKNOWN_ELEMENT:
        return "unknown element";
    case ERR_NCX_UNKNOWN_NAMESPACE:
        return "unknown namespace";
    case ERR_NCX_ACCESS_DENIED:
        return "access denied";
    case ERR_NCX_LOCK_DENIED:
        return "lock denied";
    case ERR_NCX_RESOURCE_DENIED:
        return "resource denied";
    case ERR_NCX_ROLLBACK_FAILED:
        return "rollback failed";
    case ERR_NCX_DATA_EXISTS:
        return "data exists";
    case ERR_NCX_DATA_MISSING:
        return "data missing";
    case ERR_NCX_OPERATION_NOT_SUPPORTED:
        return "operation not supported";
    case ERR_NCX_OPERATION_FAILED:
        return "operation failed";
    case ERR_NCX_PARTIAL_OPERATION:
        return "partial operation";

    /* netconf error extensions */
    case ERR_NCX_WRONG_NAMESPACE:
        return "wrong namespace";
    case ERR_NCX_WRONG_NODEDEPTH:
        return "wrong node depth";
    case ERR_NCX_WRONG_OWNER:
        return "wrong owner";
    case ERR_NCX_WRONG_ELEMENT:
        return "wrong element";
    case ERR_NCX_WRONG_ORDER:
        return "wrong order";
    case ERR_NCX_EXTRA_NODE:
        return "extra node";
    case ERR_NCX_WRONG_NODETYP:
        return "wrong node type";
    case ERR_NCX_WRONG_NODETYP_SIM:
        return "expecting complex node type";
    case ERR_NCX_WRONG_NODETYP_CPX:
        return "expecting string node type";
    case ERR_NCX_WRONG_DATATYP:
        return "wrong data type"; 
    case ERR_NCX_WRONG_DATAVAL:
        return "wrong data value"; 
    case ERR_NCX_NUMLEN_TOOBIG:
        return "invalid number length"; 
    case ERR_NCX_NOT_IN_RANGE:
        return "value not in range"; 
    case ERR_NCX_WRONG_NUMTYP:
        return "wrong number type"; 
    case ERR_NCX_EXTRA_ENUMCH:
        return "invalid enum value"; 
    case ERR_NCX_VAL_NOTINSET:
        return "value not in set"; 
    case ERR_NCX_EXTRA_LISTSTR:
        return "extra list string found"; 
    case ERR_NCX_UNKNOWN_OBJECT:
        return "unknown object"; 
    case ERR_NCX_EXTRA_PARMINST:
        return "extra parameter instance"; 
    case ERR_NCX_EXTRA_CHOICE:
        return "extra case in choice"; 
    case ERR_NCX_MISSING_CHOICE:
        return "missing mandatory choice"; 
    case ERR_NCX_CFG_STATE:
        return "wrong config state"; 
    case ERR_NCX_UNKNOWN_APP:
        return "unknown application"; 
    case ERR_NCX_UNKNOWN_TYPE:
        return "unknown data type"; 
    case ERR_NCX_NO_ACCESS_ACL:
        return "access control violation";
    case ERR_NCX_NO_ACCESS_LOCK:
        return "config locked";
    case ERR_NCX_NO_ACCESS_STATE:
        return "wrong config state";
    case ERR_NCX_NO_ACCESS_MAX:
        return "max-access exceeded";
    case ERR_NCX_WRONG_INDEX_TYPE:
        return "wrong index type";
    case ERR_NCX_WRONG_INSTANCE_TYPE:
        return "wrong instance type";
    case ERR_NCX_MISSING_INDEX:
        return "missing index component";
    case ERR_NCX_CFG_NOT_FOUND:
        return "config not found";
    case ERR_NCX_EXTRA_ATTR:
        return "extra attribute instance(s) found";
    case ERR_NCX_MISSING_ATTR:
        return "required attribute not found";
    case ERR_NCX_MISSING_VAL_INST:
        return "required value instance not found";
    case ERR_NCX_EXTRA_VAL_INST:
        return "extra value instance(s) found";
    case ERR_NCX_NOT_WRITABLE:
        return "target is read only";
    case ERR_NCX_INVALID_PATTERN:
        return "invalid pattern";
    case ERR_NCX_WRONG_VERSION:
        return "wrong version";
    case ERR_NCX_CONNECT_FAILED:
        return "connect failed";
    case ERR_NCX_UNKNOWN_HOST:
        return "unknown host";
    case ERR_NCX_SESSION_FAILED:
        return "session failed";
    case ERR_NCX_AUTH_FAILED:
        return "authentication failed";
    case ERR_NCX_UNENDED_COMMENT:
        return "end of comment not found";
    case ERR_NCX_INVALID_CONCAT:
        return "invalid string concatenation";
    case ERR_NCX_IMP_NOT_FOUND:
        return "import not found";
    case ERR_NCX_MISSING_TYPE:
        return "missing typedef sub-section";
    case ERR_NCX_RESTRICT_NOT_ALLOWED:
        return "restriction not allowed for this type";
    case ERR_NCX_REFINE_NOT_ALLOWED:
        return "specified refinement not allowed";
    case ERR_NCX_DEF_LOOP:
        return "definition loop detected";
    case ERR_NCX_DEFCHOICE_NOT_OPTIONAL:
        return "default case contains mandatory object(s)";
    case ERR_NCX_IMPORT_LOOP:
        return "import loop";
    case ERR_NCX_INCLUDE_LOOP:
        return "include loop";
    case ERR_NCX_EXP_MODULE:
        return "expecting module";
    case ERR_NCX_EXP_SUBMODULE:
        return "expecting submodule";
    case ERR_NCX_PREFIX_NOT_FOUND:
        return "undefined prefix";
    case ERR_NCX_IMPORT_ERRORS:
        return "imported module has errors";
    case ERR_NCX_PATTERN_FAILED:
        return "pattern match failed";
    case ERR_NCX_INVALID_TYPE_CHANGE:
        return "invalid data type change";
    case ERR_NCX_MANDATORY_NOT_ALLOWED:
        return "mandatory object not allowed";
    case ERR_NCX_UNIQUE_TEST_FAILED:
        return "unique-stmt test failed";
    case ERR_NCX_MAX_ELEMS_VIOLATION:
        return "max-elements exceeded";
    case ERR_NCX_MIN_ELEMS_VIOLATION:
        return "min-elements not reached";
    case ERR_NCX_MUST_TEST_FAILED:
        return "must-stmt test failed";
    case ERR_NCX_DATA_REST_VIOLATION:
        return "data restriction violation";
    case ERR_NCX_INSERT_MISSING_INSTANCE:
        return "missing instance for insert operation";
    case ERR_NCX_NOT_CONFIG:
        return "object not config";
    case ERR_NCX_INVALID_CONDITIONAL:
        return "invalid conditional object";
    case ERR_NCX_USING_OBSOLETE:
        return "using obsolete definition";
    case ERR_NCX_INVALID_AUGTARGET:
        return "invalid augment target";
    case ERR_NCX_DUP_REFINE_STMT:
        return "duplicate refine sub-clause";
    case ERR_NCX_INVALID_DEV_STMT:
        return "invalid deviate sub-clause";
    case ERR_NCX_INVALID_XPATH_EXPR:
        return "invalid XPath expression syntax";
    case ERR_NCX_INVALID_INSTANCEID:
        return "invalid instance-identifier syntax";
    case ERR_NCX_MISSING_INSTANCE:
        return "require-instance test failed";
    case ERR_NCX_UNEXPECTED_INSERT_ATTRS:
        return "key or select attribute not allowed";
    case ERR_NCX_INVALID_UNIQUE_NODE:
        return "invalid unique-stmt node";
    case ERR_NCX_INVALID_DUP_IMPORT:
        return "invalid duplicate import-stmt";
    case ERR_NCX_INVALID_DUP_INCLUDE:
        return "invalid duplicate include-stmt";
    case ERR_NCX_AMBIGUOUS_CMD:
        return "ambiguous command";
    case ERR_NCX_UNKNOWN_MODULE:
        return "unknown module";
    case ERR_NCX_UNKNOWN_VERSION:
        return "unknown version";
    case ERR_NCX_VALUE_NOT_SUPPORTED:
        return "value not supported";
    case ERR_NCX_LEAFREF_LOOP:
        return "leafref path loop";
    case ERR_NCX_VAR_NOT_FOUND:
        return "variable not found";
    case ERR_NCX_VAR_READ_ONLY:
        return "variable is read-only";
    case ERR_NCX_DEC64_BASEOVFL:
        return "decimal64 base number overflow";
    case ERR_NCX_DEC64_FRACOVFL:
        return "decimal64 fraction precision overflow";
    case ERR_NCX_RPC_WHEN_FAILED:
        return "when-stmt tested false";
    case ERR_NCX_NO_MATCHES:
        return "no matches found";
    case ERR_NCX_MISSING_REFTARGET:
        return "missing refine target";
    case ERR_NCX_CANDIDATE_DIRTY:
        return "candidate cannot be locked, discard-changes needed";
    case ERR_NCX_TIMEOUT:
        return "timeout occurred";
    case ERR_NCX_GET_SCHEMA_DUPLICATES:
        return "multiple module revisions exist";
    case ERR_NCX_XPATH_NOT_NODESET:
        return "XPath result not a nodeset";
    case ERR_NCX_XPATH_NODESET_EMPTY:
        return "XPath node-set result is empty";
    case ERR_NCX_IN_USE_LOCKED:
        return "node is protected by a partial lock";
    case ERR_NCX_IN_USE_COMMIT:
        return "cannot perform the operation with confirmed-commit pending";
    case ERR_NCX_SUBMOD_NOT_LOADED:
        return "cannot directly load a submodule";
    case ERR_NCX_ACCESS_READ_ONLY:
        return "cannot write to a read-only object";
    case ERR_NCX_CONFIG_NOT_TARGET:
        return "cannot write to this configuration directly";
    case ERR_NCX_MISSING_RBRACE:
        return "YANG file missing right brace";
    case ERR_NCX_INVALID_FRAMING:
        return "invalid protocol framing characters received";
    case ERR_NCX_PROTO11_NOT_ENABLED:
        return "base:1.1 protocol not enabled";
    case ERR_NCX_CC_NOT_ACTIVE:
        return "persistent confirmed commit not active";
    case ERR_NCX_MULTIPLE_MATCHES:
        return "multiple matches found";
    case ERR_NCX_NO_DEFAULT:
        return "no schema default for this node";
    case ERR_NCX_MISSING_KEY:
        return "expected key leaf in list";
    case ERR_NCX_TOP_LEVEL_MANDATORY_FAILED:
        return "top-level mandatory objects are not allowed";

    /* user warnings start at 400 */
    case ERR_MAKFILE_DUP_SRC:
        return "duplicate source";
    case ERR_INC_NOT_FOUND:
        return "include file not found";
    case ERR_CMDLINE_VAL:
        return "invalid command line value";
    case ERR_CMDLINE_OPT:
        return "invalid command line option";
    case ERR_CMDLINE_OPT_UNKNOWN:
        return "command line option unknown";
    case ERR_CMDLINE_SYNTAX:
        return "invalid command line syntax";
    case ERR_CMDLINE_VAL_REQUIRED:
        return "missing command line value";
    case ERR_FORM_INPUT:
        return "invalid form input";
    case ERR_FORM_UNKNOWN:
        return "invalid form";
    case ERR_NCX_NO_INSTANCE:
        return "no instance found";
    case ERR_NCX_SESSION_CLOSED:
        return "session closed by remote peer";
    case ERR_NCX_DUP_IMPORT:
        return "duplicate import";
    case ERR_NCX_PREFIX_DUP_IMPORT:
        return "duplicate import with different prefix value";
    case ERR_NCX_TYPDEF_NOT_USED:
        return "local typedef not used";
    case ERR_NCX_GRPDEF_NOT_USED:
        return "local grouping not used";
    case ERR_NCX_IMPORT_NOT_USED:
        return "import not used";
    case ERR_NCX_DUP_UNIQUE_COMP:
        return "duplicate unique-stmt argument";
    case ERR_NCX_STMT_IGNORED:
        return "statement ignored";
    case ERR_NCX_DUP_INCLUDE:
        return "duplicate include";
    case ERR_NCX_INCLUDE_NOT_USED:
        return "include not used";
    case ERR_NCX_DATE_PAST:
        return "revision date before 1970";
    case ERR_NCX_DATE_FUTURE:
        return "revision date in the future";
    case ERR_NCX_ENUM_VAL_ORDER:
        return "enum value order";
    case ERR_NCX_BIT_POS_ORDER:
        return "bit position order";
    case ERR_NCX_INVALID_STATUS:
        return "invalid status for child node";
    case ERR_NCX_DUP_AUGNODE:
        return "duplicate sibling node name from external augment";
    case ERR_NCX_DUP_IF_FEATURE:
        return "duplicate if-feature statement";
    case ERR_NCX_USING_DEPRECATED:
        return "using deprecated definition";
    case ERR_NCX_MAX_KEY_CHECK:
        return "XPath object predicate check limit reached";
    case ERR_NCX_EMPTY_XPATH_RESULT:
        return "empty XPath result in must or when expr";
    case ERR_NCX_NO_XPATH_ANCESTOR:
        return "no ancestor node available";
    case ERR_NCX_NO_XPATH_PARENT:
        return "no parent node available";
    case ERR_NCX_NO_XPATH_CHILD:
        return "no child node available";
    case ERR_NCX_NO_XPATH_DESCENDANT:
        return "no descendant node available";
    case ERR_NCX_NO_XPATH_NODES:
        return "no nodes available";
    case ERR_NCX_BAD_REV_ORDER:
        return "bad revision-stmt order";
    case ERR_NCX_DUP_PREFIX:
        return "duplicate prefix";
    case ERR_NCX_IDLEN_EXCEEDED:
        return "identifier length exceeded";
    case ERR_NCX_LINELEN_EXCEEDED:
        return "display line length exceeded";
    case ERR_NCX_RCV_UNKNOWN_CAP:
        return "received unknown capability";
    case ERR_NCX_RCV_INVALID_MODCAP:
        return "invalid module capability URI";
    case ERR_NCX_USING_ANYXML:
        return "unknown child node, using anyxml";
    case ERR_NCX_USING_BADDATA:
        return "invalid value used for parm";
    case ERR_NCX_USING_STRING:
        return "changing object type to string";
    case ERR_NCX_USING_RESERVED_NAME:
        return "using a reserved name";
    case ERR_NCX_CONF_PARM_EXISTS:
        return "conf file parm already exists";
    case ERR_NCX_NO_REVISION:
        return "no valid revision statements found";
    case ERR_NCX_DEPENDENCY_ERRORS:
        return "dependency file has errors";
    case ERR_NCX_TOP_LEVEL_MANDATORY:
        return "top-level object is mandatory";
    case ERR_NCX_FILE_MOD_MISMATCH:
        return "file name does not match [sub]module name";
    case ERR_NCX_UNIQUE_CONDITIONAL_MISMATCH:
        return "unique-stmt component conditions do not match parent list";

    /* system info codes start at 500 */
    case ERR_PARS_SECDONE:
        return "invalid command line value";
    case ERR_NCX_SKIPPED:
        return "operation skipped"; 
    case ERR_NCX_CANCELED:
        return "operation canceled"; 

    default:
        return "--";
    }

    /*NOTREACHED*/

} /* get_error_string */


/********************************************************************
* FUNCTION errno_to_status
*
* Get the errno variable and convert it to a status_t
*
* INPUTS:
*   none; must be called just after error occurred to prevent
*        the errno variable from being overwritten by a new operation
*
* RETURNS:
*     status_t for the errno enum
*********************************************************************/
status_t
    errno_to_status (void)
{
    switch (errno) {
    case EACCES:
        return ERR_NCX_ACCESS_DENIED;
    case EAFNOSUPPORT:
    case EPROTONOSUPPORT:
        return ERR_NCX_OPERATION_NOT_SUPPORTED;
    case EINVAL:
        return ERR_NCX_INVALID_VALUE;
    case EMFILE:
        return ERR_NCX_OPERATION_FAILED;
    case ENFILE:
        return ERR_NCX_RESOURCE_DENIED;
    case ENOBUFS:
    case ENOMEM:
        return ERR_INTERNAL_MEM;
    default:
        return ERR_NCX_OPERATION_FAILED;
    }

} /* errno_to_status */


/********************************************************************
* FUNCTION status_init
*
* Init this module
*
*********************************************************************/
void
    status_init (void)
{
    error_level = 0;
    memset(&error_stack, 0x0, sizeof(error_stack));
    error_count = 0;

} /* status_init */


/********************************************************************
* FUNCTION status_cleanup
*
* Cleanup this module
*
*********************************************************************/
void
    status_cleanup (void)
{
    clear_errors();

} /* status_cleanup */


/********************************************************************
* FUNCTION print_error_count
*
* Print the error_count field, if it is non-zero
* to STDOUT or the logfile
* Clears out the error_count afterwards so
* the count will start over after printing!!!
*
*********************************************************************/
void
    print_error_count (void)
{
    if (error_count) {
        log_error("\n\n*** Total Internal Errors: %u ***\n", 
                  error_count);
        error_count = 0;
    }

} /* print_error_count */


/********************************************************************
* FUNCTION print_error_messages
*
* Print the error number and error message for each error
* to STDOUT or the logfile
*
*********************************************************************/
void
    print_error_messages (void)
{
    status_t   res;

    /* internal errors */
    for (res = NO_ERR; res < ERR_LAST_INT_ERR; res++) {
        log_write("\n%3d\t%s",
                  res,
                  get_error_string(res));
    }

    /* system errors */
    for (res = ERR_SYS_BASE; res < ERR_LAST_SYS_ERR; res++) {
        log_write("\n%3d\t%s",
                  res,
                  get_error_string(res));
    }

    /* user errors */
    for (res = ERR_USR_BASE; res < ERR_LAST_USR_ERR; res++) {
        log_write("\n%3d\t%s",
                  res,
                  get_error_string(res));
    }

    /* warnings */
    for (res = ERR_WARN_BASE; res < ERR_LAST_WARN; res++) {
        log_write("\n%3d\t%s",
                  res,
                  get_error_string(res));
    }

    log_write("\n");


} /* print_error_messages */



/* END file status.c */
