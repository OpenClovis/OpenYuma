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
/*  FILE: agt_rpcerr.c

   NETCONF Protocol Operations: <rpc-error> Agent Side Support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
06mar06      abb      begun; split from agt_rpc.c


*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>
#include  <assert.h>

#include  "procdefs.h"
#include  "agt_rpc.h"
#include  "agt_rpcerr.h"
#include  "cfg.h"
#include  "def_reg.h"
#include  "ncx.h"
#include  "ncxconst.h"
#include  "rpc.h"
#include  "rpc_err.h"
#include  "status.h"
#include  "val.h"
#include  "val_util.h"
#include  "xmlns.h"
#include  "xml_util.h"
#include  "xpath.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/
    

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION get_rpcerr
*
* Translate the status_t to a rpc_err_t 
*
* INPUTS:
*   intres == internal status_t error code
*   isel == TRUE if this error is for an element
*        == FALSE if this error is for an attribute
*   errsev == address of return error severity
*   apptag == address of return error-app-tag
*
* OUTPUTS:
*  *errsev == error severity (error or warning)
*  *apptag == pointer to error-app-tag string
*
* RETURNS:
*   rpc error id for the specified internal error id
*********************************************************************/
static rpc_err_t
    get_rpcerr (status_t intres,
                boolean isel,
                rpc_err_sev_t *errsev,
                const xmlChar **apptag)
{

    if (intres >= ERR_WARN_BASE) {
        *errsev = RPC_ERR_SEV_WARNING;
        *apptag = RPC_ERR_APPTAG_GEN_WARNING;
        return RPC_ERR_NONE;
    }
    
    /* setup defaults */
    *errsev = RPC_ERR_SEV_ERROR;
    *apptag = RPC_ERR_APPTAG_GEN_ERROR;

    /* translate the internal NCX error code to a netconf code */
    switch (intres) {
    case NO_ERR:
        *errsev = RPC_ERR_SEV_NONE;
        *apptag = NCX_EL_NONE;
        return RPC_ERR_NONE;
    case ERR_END_OF_FILE:
        *apptag = RPC_ERR_APPTAG_IO_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_INTERNAL_PTR:
        *apptag = RPC_ERR_APPTAG_INT_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_INTERNAL_MEM:
        *apptag = RPC_ERR_APPTAG_MALLOC_ERROR;
        return RPC_ERR_RESOURCE_DENIED;
    case ERR_INTERNAL_VAL:
    case ERR_INTERNAL_BUFF:
    case ERR_INTERNAL_QDEL:
    case ERR_INTERNAL_INIT_SEQ:
    case ERR_QNODE_NOT_HDR:
    case ERR_QNODE_NOT_DATA:
    case ERR_BAD_QLINK:
    case ERR_Q_ALREADY:
        *apptag = RPC_ERR_APPTAG_INT_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_TOO_MANY_ENTRIES:
    case ERR_BUFF_OVFL:
        *apptag = RPC_ERR_APPTAG_LIMIT_REACHED;
        return RPC_ERR_RESOURCE_DENIED;
    case ERR_XML2_FAILED:
    case ERR_XML_READER_INTERNAL:
    case ERR_XML_READER_START_FAILED:
    case ERR_XML_READER_READ:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_FIL_OPEN:
    case ERR_FIL_READ:
    case ERR_FIL_CLOSE:
    case ERR_FIL_WRITE:
    case ERR_FIL_CHDIR:
    case ERR_FIL_STAT:
    case ERR_FIL_DELETE:
    case ERR_FIL_SETPOS:
    case ERR_OPEN_DIR_FAILED:
    case ERR_READ_DIR_FAILED:
    case ERR_PARSPOST_RD_INPUT:
        *apptag = RPC_ERR_APPTAG_IO_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_DB_CONNECT_FAILED:
    case ERR_DB_ENTRY_EXISTS:
    case ERR_DB_NOT_FOUND:
    case ERR_DB_QUERY_FAILED:
    case ERR_DB_DELETE_FAILED:
    case ERR_DB_WRONG_CKSUM:
    case ERR_DB_WRONG_TAGTYPE:
    case ERR_DB_READ_FAILED:
    case ERR_DB_WRITE_FAILED:
    case ERR_DB_INIT_FAILED:
        *apptag = RPC_ERR_APPTAG_SQL_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_TR_BEEP_INIT:
    case ERR_TR_BEEP_NC_INIT:
        *apptag = RPC_ERR_APPTAG_BEEP_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NO_CFGFILE:
    case ERR_NO_SRCFILE:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_FIL_BAD_DRIVE:
    case ERR_FIL_BAD_PATH:
    case ERR_FIL_BAD_FILENAME:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_DUP_VALPAIR:
    case ERR_PAGE_NOT_HANDLED:
    case ERR_PAGE_ACCESS_DENIED:
    case ERR_MISSING_FORM_PARAMS:
    case ERR_FORM_STATE:
    case ERR_DUP_NS:
        *apptag = RPC_ERR_APPTAG_HTML_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_XML_READER_NODETYP:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return (isel)  ? RPC_ERR_UNKNOWN_ELEMENT 
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_XML_READER_NULLNAME:
    case ERR_XML_READER_NULLVAL:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return (isel) ? RPC_ERR_BAD_ELEMENT
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_XML_READER_WRONGNAME:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_XML_READER_WRONGVAL:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_XML_READER_WRONGEL:
    case ERR_XML_READER_EXTRANODES:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return RPC_ERR_UNKNOWN_ELEMENT; 
    case ERR_XML_READER_EOF:
        *apptag = RPC_ERR_APPTAG_LIBXML2_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_WRONG_LEN:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_ENTRY_EXISTS:
    case ERR_NCX_DUP_ENTRY:
        *apptag = RPC_ERR_APPTAG_DUPLICATE_ERROR;
        return RPC_ERR_DATA_EXISTS;
    case ERR_NCX_NOT_FOUND:
    case ERR_NCX_MISSING_FILE:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_UNKNOWN_PARM:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT :
            RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_INVALID_NAME:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_UNKNOWN_NS:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_NAMESPACE;
    case ERR_NCX_WRONG_NS:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_NAMESPACE;
    case ERR_NCX_WRONG_TYPE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_WRONG_VAL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_MISSING_PARM:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_DATA_MISSING;
    case ERR_NCX_EXTRA_PARM:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT :
            RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_EMPTY_VAL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_MOD_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_LEN_EXCEEDED:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_INVALID_TOKEN:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_UNENDED_QSTRING:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_READ_FAILED:
        *apptag = RPC_ERR_APPTAG_IO_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_NUM:
    case ERR_NCX_INVALID_HEXNUM:
    case ERR_NCX_INVALID_REALNUM:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_EOF:
        *apptag = RPC_ERR_APPTAG_IO_ERROR;
        return RPC_ERR_DATA_MISSING;
    case ERR_NCX_WRONG_TKTYPE:
    case ERR_NCX_WRONG_TKVAL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_BUFF_SHORT:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_RANGE:
    case ERR_NCX_OVERLAP_RANGE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_DEF_NOT_FOUND:
    case ERR_NCX_DEFSEG_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_TYPE_NOT_INDEX:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_INDEX_TYPE_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_TYPE_NOT_MDATA:
    case ERR_NCX_MDATA_NOT_ALLOWED:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_TOP_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;

    /*** match netconf errors here ***/
    case ERR_NCX_IN_USE:
        *apptag = RPC_ERR_APPTAG_RESOURCE_IN_USE;
        return RPC_ERR_IN_USE;
    case ERR_NCX_INVALID_VALUE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_TOO_BIG:
        *apptag = RPC_ERR_APPTAG_LIMIT_REACHED;
        return RPC_ERR_TOO_BIG;
    case ERR_NCX_MISSING_ATTRIBUTE:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_MISSING_ATTRIBUTE;
    case ERR_NCX_BAD_ATTRIBUTE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_UNKNOWN_ATTRIBUTE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_MISSING_ELEMENT:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_MISSING_ELEMENT;
    case ERR_NCX_BAD_ELEMENT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_BAD_ELEMENT;
    case ERR_NCX_UNKNOWN_ELEMENT:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_UNKNOWN_ELEMENT;
    case ERR_NCX_UNKNOWN_NAMESPACE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_NAMESPACE;
    case ERR_NCX_ACCESS_DENIED:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_LOCK_DENIED:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_LOCK_DENIED;
    case ERR_NCX_RESOURCE_DENIED:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_RESOURCE_DENIED;
    case ERR_NCX_ROLLBACK_FAILED:
        *apptag = RPC_ERR_APPTAG_RECOVER_FAILED;
        return RPC_ERR_ROLLBACK_FAILED;
    case ERR_NCX_DATA_EXISTS:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_DATA_EXISTS;
    case ERR_NCX_DATA_MISSING:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_DATA_MISSING;
    case ERR_NCX_OPERATION_NOT_SUPPORTED:
        *apptag = RPC_ERR_APPTAG_NO_SUPPORT;
        return RPC_ERR_OPERATION_NOT_SUPPORTED;
    case ERR_NCX_OPERATION_FAILED:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_PARTIAL_OPERATION:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_PARTIAL_OPERATION;

    /* netconf error extensions */
    case ERR_NCX_WRONG_NAMESPACE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_NAMESPACE;
    case ERR_NCX_WRONG_NODEDEPTH:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_BAD_ELEMENT;
    case ERR_NCX_WRONG_OWNER:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_ELEMENT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_ORDER:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_EXTRA_NODE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_NODETYP:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_NODETYP_SIM:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_NODETYP_CPX:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_WRONG_DATATYP:
    case ERR_NCX_WRONG_DATAVAL:
    case ERR_NCX_NUMLEN_TOOBIG:
    case ERR_NCX_WRONG_NUMTYP:
    case ERR_NCX_EXTRA_ENUMCH:
    case ERR_NCX_EXTRA_LISTSTR:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_NOT_IN_RANGE:
        *apptag = RPC_ERR_APPTAG_RANGE;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_VAL_NOTINSET:
        *apptag = RPC_ERR_APPTAG_VALUE_SET;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_UNKNOWN_OBJECT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_ELEMENT;
    case ERR_NCX_EXTRA_PARMINST:
    case ERR_NCX_EXTRA_CHOICE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT 
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_MISSING_CHOICE:
        *apptag = RPC_ERR_APPTAG_CHOICE;
        return RPC_ERR_DATA_MISSING;    /* 13.6 */
    case ERR_NCX_CFG_STATE:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_UNKNOWN_APP:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_ELEMENT;
    case ERR_NCX_UNKNOWN_TYPE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT 
            : RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_NO_ACCESS_ACL:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_NO_ACCESS_LOCK:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_IN_USE;
    case ERR_NCX_NO_ACCESS_STATE:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_NO_ACCESS_MAX:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_WRONG_INDEX_TYPE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_BAD_ELEMENT;
    case ERR_NCX_WRONG_INSTANCE_TYPE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_MISSING_INDEX:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_DATA_MISSING;
    case ERR_NCX_CFG_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_EXTRA_ATTR:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_MISSING_ATTR:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_MISSING_ATTRIBUTE;
    case ERR_NCX_MISSING_VAL_INST:
        *apptag = RPC_ERR_APPTAG_INSTANCE_REQ;
        return RPC_ERR_DATA_MISSING;   /* 13.5 */
    case ERR_NCX_EXTRA_VAL_INST:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT :
            RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_NOT_WRITABLE:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_INVALID_PATTERN:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_WRONG_VERSION:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_BAD_ELEMENT 
            : RPC_ERR_BAD_ATTRIBUTE;
    case ERR_NCX_CONNECT_FAILED:
    case ERR_NCX_SESSION_FAILED:
        *apptag = RPC_ERR_APPTAG_IO_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_UNKNOWN_HOST:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_AUTH_FAILED:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_UNENDED_COMMENT:
    case ERR_NCX_INVALID_CONCAT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_IMP_NOT_FOUND:
    case ERR_NCX_MISSING_TYPE:
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_RESTRICT_NOT_ALLOWED:
    case ERR_NCX_REFINE_NOT_ALLOWED:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_DEF_LOOP:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_DEFCHOICE_NOT_OPTIONAL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_IMPORT_LOOP:
    case ERR_NCX_INCLUDE_LOOP:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_EXP_MODULE:
    case ERR_NCX_EXP_SUBMODULE:
    case ERR_NCX_PREFIX_NOT_FOUND:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_IMPORT_ERRORS:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_PATTERN_FAILED:
        *apptag = RPC_ERR_APPTAG_PATTERN;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_INVALID_TYPE_CHANGE:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_MANDATORY_NOT_ALLOWED:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_UNIQUE_TEST_FAILED:
        *apptag = RPC_ERR_APPTAG_UNIQUE_FAILED;
        return RPC_ERR_OPERATION_FAILED;   /* 13.1 */
    case ERR_NCX_MAX_ELEMS_VIOLATION:
        *apptag = RPC_ERR_APPTAG_MAX_ELEMS;
        return RPC_ERR_OPERATION_FAILED;   /* 13.2 */
    case ERR_NCX_MIN_ELEMS_VIOLATION:
        *apptag = RPC_ERR_APPTAG_MIN_ELEMS;
        return RPC_ERR_OPERATION_FAILED;   /* 13.3 */
    case ERR_NCX_MUST_TEST_FAILED:
        *apptag = RPC_ERR_APPTAG_MUST;
        return RPC_ERR_OPERATION_FAILED;   /* 13.4 */
    case ERR_NCX_DATA_REST_VIOLATION:
        *apptag = RPC_ERR_APPTAG_DATA_REST;
        return RPC_ERR_INVALID_VALUE;     /* obsolete */
    case ERR_NCX_INSERT_MISSING_INSTANCE:
        *apptag = RPC_ERR_APPTAG_INSERT;
        return RPC_ERR_BAD_ATTRIBUTE;      /* 13.7 */
    case ERR_NCX_NOT_CONFIG:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_CONDITIONAL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_USING_OBSOLETE:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_AUGTARGET:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_DUP_REFINE_STMT:
        *apptag = RPC_ERR_APPTAG_DUPLICATE_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_DEV_STMT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_XPATH_EXPR:
    case ERR_NCX_INVALID_INSTANCEID:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_MISSING_INSTANCE:  /* not used */
        *apptag = RPC_ERR_APPTAG_DATA_INCOMPLETE;
        return RPC_ERR_DATA_MISSING;
    case ERR_NCX_UNEXPECTED_INSERT_ATTRS:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_UNIQUE_NODE:
    case ERR_NCX_INVALID_DUP_IMPORT:
    case ERR_NCX_INVALID_DUP_INCLUDE:
    case ERR_NCX_AMBIGUOUS_CMD:
    case ERR_NCX_UNKNOWN_MODULE:
    case ERR_NCX_UNKNOWN_VERSION:
    case ERR_NCX_VALUE_NOT_SUPPORTED:
    case ERR_NCX_LEAFREF_LOOP:
    case ERR_NCX_VAR_NOT_FOUND:
    case ERR_NCX_VAR_READ_ONLY:
    case ERR_NCX_DEC64_BASEOVFL:
    case ERR_NCX_DEC64_FRACOVFL:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_RPC_WHEN_FAILED:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT :
            RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_NO_MATCHES:
        *apptag = RPC_ERR_APPTAG_NO_MATCHES;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_MISSING_REFTARGET:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_CANDIDATE_DIRTY:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_RESOURCE_DENIED;
    case ERR_NCX_TIMEOUT:
        *apptag = RPC_ERR_APPTAG_LIMIT_REACHED;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_GET_SCHEMA_DUPLICATES:
        *apptag = RPC_ERR_APPTAG_DATA_NOT_UNIQUE;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_XPATH_NOT_NODESET:
        *apptag = RPC_ERR_APPTAG_NOT_NODESET;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_XPATH_NODESET_EMPTY:
        *apptag = RPC_ERR_APPTAG_NO_MATCHES;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_IN_USE_LOCKED:
        *apptag = RPC_ERR_APPTAG_LOCKED;
        return RPC_ERR_IN_USE;
    case ERR_NCX_IN_USE_COMMIT:
        *apptag = RPC_ERR_APPTAG_COMMIT;
        return RPC_ERR_IN_USE;
    case ERR_NCX_SUBMOD_NOT_LOADED:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_ACCESS_READ_ONLY:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_ACCESS_DENIED;
    case ERR_NCX_CONFIG_NOT_TARGET:
        *apptag = RPC_ERR_APPTAG_NO_ACCESS;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_MISSING_RBRACE:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_INVALID_FRAMING:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_MALFORMED_MESSAGE;
    case ERR_NCX_PROTO11_NOT_ENABLED:
        *apptag = RPC_ERR_APPTAG_NO_SUPPORT;
        return (isel) ? RPC_ERR_UNKNOWN_ELEMENT :
            RPC_ERR_UNKNOWN_ATTRIBUTE;
    case ERR_NCX_CC_NOT_ACTIVE:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_MULTIPLE_MATCHES:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;
    case ERR_NCX_NO_DEFAULT:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_INVALID_VALUE;
    case ERR_NCX_MISSING_KEY:
        *apptag = RPC_ERR_APPTAG_DATA_INVALID;
        return RPC_ERR_MISSING_ELEMENT;
    case ERR_NCX_TOP_LEVEL_MANDATORY_FAILED:
        *apptag = RPC_ERR_APPTAG_GEN_ERROR;
        return RPC_ERR_OPERATION_FAILED;

    /* user warnings start at 400 and do not need to be listed here */
    default:
        return RPC_ERR_OPERATION_FAILED;        
    }
    /*NOTREACHED*/

} /* get_rpcerr */


/**
 * Create a new error information structure and add it to an error
 *
 * /param err pointer to the error to which the error info should be added
 * /param name_nsid the namespace id
 * /param name the error name
 * /param isqname val_nsid + strval == QName
 * /param val_btype the type of error information content 
 * /param val_nsid the namespace id value
 * /param badns an invalid namespace string (NULL if not required)
 * /param strval the string error content (NULL if not required)
 * /param interr pointer to the number error content (NULL if not required)
 *
 * /return the status of the operation
 ********************************************************************/
static status_t enque_error_info (rpc_err_rec_t* err,
                                  xmlns_id_t name_nsid,
                                  const xmlChar* name,
                                  boolean isqname,
                                  ncx_btype_t val_btype,
                                  xmlns_id_t val_nsid,
                                  const xmlChar* badns,
                                  const xmlChar* strval,
                                  status_t* interr)
{
    /* no value for union */
    if (!strval && !interr) {
        return ERR_INTERNAL_MEM;
    }

    rpc_err_info_t* errinfo = rpc_err_new_info();
    if (!errinfo) {
        return ERR_INTERNAL_MEM;
    }

    /* generate bad-attribute value */
    errinfo->name_nsid = name_nsid;
    errinfo->name = name;
    errinfo->isqname = isqname;
    errinfo->val_btype = val_btype;
    errinfo->val_nsid = val_nsid;
    if (badns) {
        errinfo->badns = xml_strdup(badns);
    }
    if (interr) {
        errinfo->v.numval.u = (uint32)(*interr);
    }
    if (strval) {
        errinfo->dval = xml_strdup(strval);
        errinfo->v.strval = errinfo->dval;
    }
    dlq_enque(errinfo, &err->error_info);
    return NO_ERR;

}  /* enque_error_info */



/********************************************************************
* FUNCTION add_base_vars
*
* Malloc val_value_t structs, fill them in and add them to
* the err->error_info Q, for the basic vars that are
* generated for specific error-tag values.
*
* INPUTS:
*   err == rpc_err_rec_t in progress
*   rpcerr == error-tag internal error code
*   errnode == XML node that caused the error (or NULL)
*   badval == bad_value string (or NULL)
*   badns == bad-namespace string (or NULL)
*   badnsid1 == bad NSID #1
*   badnsid2 == bad NSID #2
*   errparm1 == void * to the 1st extra parameter to use (per errcode)
*           == NULL if not use
*   errparm2 == void * to the 2nd extra parameter to use (per errcode)
*           == NULL if not used
*   !!! errparm3 removed !!!
*   errparm4 == void * to the 4th extra parameter to use (per errcode)
*           == NULL if not used

* RETURNS:
*   status
*********************************************************************/
static status_t add_base_vars (rpc_err_rec_t  *err,
                               rpc_err_t  rpcerr,
                               const xml_node_t *errnode,
                               const xmlChar *badval,
                               const xmlChar *badns,
                               xmlns_id_t  badnsid1,
                               xmlns_id_t  badnsid2,
                               const void *errparm1,
                               const void *errparm2,
                               const void *errparm4)
{
    status_t              res = NO_ERR;
    const xmlChar        *badel;
    const uint32         *numptr;
    ses_id_t              sesid;
    boolean               attrerr = FALSE;
    xmlns_id_t            badid;

    /* figure out the required error-info */
    switch (rpcerr) {
    case RPC_ERR_UNKNOWN_ATTRIBUTE:
    case RPC_ERR_MISSING_ATTRIBUTE:
    case RPC_ERR_BAD_ATTRIBUTE:
        /* badnsid1 == error attribute namespace ID
         * errparm2 == error attribute name
         */
        if (errparm2) {
            res = enque_error_info(err, xmlns_nc_id(), NCX_EL_BAD_ATTRIBUTE, 
                                   TRUE, NCX_BT_STRING, badnsid1,
                                   (badnsid1 == xmlns_inv_id()) ? badns : NULL, 
                                   (const xmlChar *)errparm2, NULL); 
            if (res != NO_ERR) {
                return res;
            }
       
        } /* else internal error already recorded */
        attrerr = TRUE;
        /* fall through */
    case RPC_ERR_UNKNOWN_ELEMENT:
    case RPC_ERR_MISSING_ELEMENT:
    case RPC_ERR_BAD_ELEMENT:   
    case RPC_ERR_UNKNOWN_NAMESPACE:
        /* check mandatory param for this rpcerr */
        if (errnode) {
            badid = errnode->nsid;
            badel = errnode->elname;
        } else if (attrerr) {
            badid = badnsid2;
            badel = (const xmlChar *)errparm4;
        } else {
            badid = badnsid1;
            badel = (const xmlChar *)errparm2;
        }

        /* generate bad-element value */
        if (badel) {
            res = enque_error_info(err, xmlns_nc_id(), NCX_EL_BAD_ELEMENT, 
                                   TRUE, NCX_BT_STRING, badid,
                                   (badid == xmlns_inv_id()) ? badns : NULL, 
                                   badel, NULL); 
            if (res != NO_ERR) {
                return res;
            }
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (rpcerr == RPC_ERR_UNKNOWN_NAMESPACE) {
            if (badns) {
                res = enque_error_info(err, xmlns_nc_id(), NCX_EL_BAD_NAMESPACE,
                                       FALSE, NCX_BT_STRING, 0, NULL, badns, 
                                       NULL); 
                if (res != NO_ERR) {
                    return res;
                }
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case RPC_ERR_LOCK_DENIED:
        /* generate session-id value */
        numptr = (const uint32 *)errparm1;
        if (numptr != NULL) {
            sesid = *numptr;
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
            sesid = 0;
        }

        res = enque_error_info(err, xmlns_nc_id(), NCX_EL_SESSION_ID, 
                               FALSE, NCX_BT_UINT32, 0, NULL, NULL, &sesid); 
        if (res != NO_ERR) {
            return res;
        }
        break;
    case RPC_ERR_DATA_MISSING:
        if (errparm2 && !badval) {
            /* expecting the obj_template_t of the missing choice */
            res = enque_error_info(err, xmlns_yang_id(), 
                                   (const xmlChar *)"missing-choice", FALSE, 
                                   NCX_BT_STRING, 0, NULL, 
                                   (const xmlChar *)errparm2, NULL); 
            if (res != NO_ERR) {
                return res;
            }
        }
        break;
    default:
        ;   /* all other rpc-err_t enums handled elsewhere */
    } 

    /* generate NCX extension bad-value */
    if (badval) {
        res = enque_error_info(err, xmlns_ncx_id(), NCX_EL_BAD_VALUE,
                               FALSE, NCX_BT_STRING, 0, NULL, badval, 
                               NULL); 
        if (res != NO_ERR) {
            return res;
        }
    }

    return NO_ERR;

}  /* add_base_vars */


/********************************************************************
* FUNCTION add_error_number
*
* Malloc a val_value_t struct, fill it in and add it to
* the err->error_info Q, for the n<error-number>
* error info element
*
* INPUTS:
*   err == rpc_err_rec_t in progress
*   interr == internal error number to use
*
* OUTPUTS:
*   err->errinfoQ has a data node added if NO_ERR
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    add_error_number (rpc_err_rec_t  *err,
                      status_t  interr)
{
    status_t res = enque_error_info(err, xmlns_ncx_id(), NCX_EL_ERROR_NUMBER,
                         FALSE, NCX_BT_UINT32, 0, NULL, NULL, &interr); 
    return res;
}  /* add_error_number */


/********************************************************************
* FUNCTION agt_rpc_gen_error
*
* Generate an internal <rpc-error> record for an element
* (or non-attribute) related error for any layer. 
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_error (ncx_layer_t layer,
                          status_t   interr,
                          const xml_node_t *errnode,
                          ncx_node_t  parmtyp,
                          const void *error_parm,
                          xmlChar *error_path)
{
    return agt_rpcerr_gen_error_ex(layer, 
                                   interr, 
                                   errnode, 
                                   parmtyp, 
                                   error_parm, 
                                   error_path, 
                                   NULL,
                                   NCX_NT_NONE,
                                   NULL);

} /* agt_rpcerr_gen_error */


/********************************************************************
* FUNCTION agt_rpc_gen_error_errinfo
*
* Generate an internal <rpc-error> record for an element
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*  errinfo == error info struct to use for whatever fields are set
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_error_errinfo (ncx_layer_t layer,
                                  status_t   interr,
                                  const xml_node_t *errnode,
                                  ncx_node_t  parmtyp,
                                  const void *error_parm,
                                  xmlChar *error_path,
                                  const ncx_errinfo_t *errinfo)
{

    return agt_rpcerr_gen_error_ex(layer, 
                                   interr, 
                                   errnode, 
                                   parmtyp, 
                                   error_parm, 
                                   error_path, 
                                   errinfo,
                                   NCX_NT_NONE,
                                   NULL);

} /* agt_rpcerr_gen_error_errinfo */


/**
 * Set the values of an error record structure
 *
 * /param err pointer to the error to set
 * /param error_res the error result
 * /param error_id the error id
 * /param error_type the error type
 * /param error_severity the error severity
 * /param error_tag the error tag
 * /param error_app_tag the error application tag
 * /param error_path the error path
 * /param error_message the error message (may be NULL)
 ********************************************************************/
static void set_error_record (rpc_err_rec_t* err,
                       status_t error_res,
                       rpc_err_t error_id,
                       ncx_layer_t error_type, 
                       rpc_err_sev_t error_severity, 
                       const xmlChar* error_tag,
                       const xmlChar* error_app_tag,
                       xmlChar* error_path,
                       const xmlChar* error_message)

{
    err->error_res = error_res;
    err->error_id = error_id;
    err->error_type = error_type;
    err->error_severity = error_severity;
    err->error_tag = error_tag;
    err->error_app_tag = error_app_tag;
    err->error_path = error_path;
    err->error_message = (error_message) ? xml_strdup(error_message) : NULL;
    err->error_message_lang = NCX_DEF_LANG;
}  /* set_error_record */


/********************************************************************
* FUNCTION agt_rpc_gen_error_ex
*
* Generate an internal <rpc-error> record for an element
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   parmtyp == type of node contained in error_parm
*   error_parm == pointer to the extra parameter expected for
*                this type of error.  
*
*                == (void *)pointer to session_id for lock-denied errors
*                == (void *) pointer to the bad-value string to use
*                   for some other errors
*
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*  errinfo == error info struct to use for whatever fields are set
*  nodetyp == type of node contained in error_path_raw
*  error_path_raw == pointer to the extra parameter expected for
*                this type of error.  
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_error_ex (ncx_layer_t layer,
                             status_t   interr,
                             const xml_node_t *errnode,
                             ncx_node_t  parmtyp,
                             const void *error_parm,
                             xmlChar *error_path,
                             const ncx_errinfo_t *errinfo,
                             ncx_node_t  nodetyp,
                             const void *error_path_raw)
{
    rpc_err_rec_t            *err;
    const obj_template_t     *parm, *in, *obj;
    const xmlns_qname_t      *qname;
    const val_value_t        *valparm;
    const xmlChar            *badval, *badns, *msg, *apptag;
    const void               *err1, *err2, *err4;
    const cfg_template_t     *badcfg;
    rpc_err_t                 rpcerr;
    rpc_err_sev_t             errsev;
    xmlns_id_t                badnsid1, badnsid2;
    boolean                   just_errnum;

    /* get a new error record */
    err = rpc_err_new_record();
    if (!err) {
        return NULL;
    }
    
    parm = NULL;
    in = NULL;
    obj = NULL;
    qname = NULL;
    valparm = NULL;
    badval = NULL;
    badns = NULL;
    msg = NULL;
    apptag = NULL;
    err1 = NULL;
    err2 = NULL;
    err4 = NULL;
    errsev = RPC_ERR_SEV_NONE;
    badnsid1 = 0;
    badnsid2 = 0;
    just_errnum = FALSE;

    switch (interr) {
    case ERR_NCX_MISSING_PARM:
    case ERR_NCX_EXTRA_CHOICE:
    case ERR_NCX_MISSING_CHOICE:
    case ERR_NCX_MISSING_KEY:
    case ERR_NCX_RPC_WHEN_FAILED:
        if (!error_parm) {
            SET_ERROR(ERR_INTERNAL_PTR);
        } else {
            switch (parmtyp) {
            case NCX_NT_OBJ:
                parm = (const obj_template_t *)error_parm;
                if (parm) {
                    badnsid1 = obj_get_nsid(parm);
                    err2 = (const void *)obj_get_name(parm);
                }
                break;
            case NCX_NT_VAL:
                valparm = (const val_value_t *)error_parm;
                if (valparm) {
                    badnsid1 = val_get_nsid(valparm);
                    err2 = (const void *)valparm->name;
                }
                break;
            case NCX_NT_STRING:
                /* FIXME: setting this to 0 may cause a
                 * SET_ERROR when the XML element is printed
                 * because the error-info 'bad-element'
                 * is supposed to be a QName and nsid must != 0
                 * TBD: redo this whole error API!!!
                 * but first just pass in the NSID of the bad-element
                 */
                badnsid1 = 0;
                err2 = (const void *)error_parm;
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case ERR_NCX_LOCK_DENIED:
        if (!error_parm) {
            SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            if (parmtyp == NCX_NT_CFG) {
                badcfg = (const cfg_template_t *)error_parm;
                if (badcfg != NULL) {
                    err1 = &badcfg->locked_by;
                } else {
                    SET_ERROR(ERR_INTERNAL_VAL);
                }
            } else if (parmtyp == NCX_NT_UINT32_PTR) {
                err1 = error_parm;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
        break;
    case ERR_NCX_MISSING_INDEX:
        if (!error_parm || parmtyp != NCX_NT_OBJ) {
            SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            in = (const obj_template_t *)error_parm;
            if (in) {
                badnsid1 = obj_get_nsid(in);
                err2 = (const void *)obj_get_name(in);
            }
        }
        break;
    case ERR_NCX_DEF_NOT_FOUND:
        if (parmtyp == NCX_NT_STRING && error_parm) {
            err1 = error_parm;
        } else if (parmtyp == NCX_NT_VAL && error_parm) {
            valparm = (const val_value_t *)error_parm;
            if (valparm) {
                badnsid1 = val_get_nsid(valparm);
                err2 = (const void *)valparm->name;
            }
        }
        break;
    case ERR_NCX_MISSING_ATTR:
    case ERR_NCX_EXTRA_ATTR:
    case ERR_NCX_MISSING_VAL_INST:
    case ERR_NCX_EXTRA_VAL_INST:
        /* this hack is needed because this error is generated
         * after the xml_attr_t record is gone
         *
         * First set the bad-attribute NS and name
         */
        if (error_parm) {
            if (parmtyp == NCX_NT_QNAME) {
                qname = (const xmlns_qname_t *)error_parm;
                badnsid1 = qname->nsid;
                err2 = (const void *)qname->name;
            } else if (parmtyp == NCX_NT_OBJ) {
                obj = (const obj_template_t *)error_parm;
                badnsid1 = obj_get_nsid(obj);
                err2 = (const void *)obj_get_name(obj);
            } else if (parmtyp == NCX_NT_STRING) {
                badnsid1 = 0;
                err2 = (const void *)error_parm;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }

        /* hack: borrow the errnode pointer to use as a string 
         * for the bad-element name 
         */
        if (errnode) {
            if (qname) {
                badnsid2 = qname->nsid;
            }
            err4 = (const void *)errnode;
            /* make sure add_base_vars doesn't use as an xml_node_t */
            errnode = NULL;  
        } else if (error_path_raw) {
            if (nodetyp == NCX_NT_VAL) {
                /* element nsid:name for the attribute error */
                valparm = (const val_value_t *)error_path_raw;
                badnsid2 = val_get_nsid(valparm);
                err4 = valparm->name;
            }
        }
        break;
    default:
        break;
    }

    rpcerr = get_rpcerr(interr, TRUE, &errsev, &apptag); 

    /* generate a default error message, and continue anyway
     * if the xml_strdup fails with a malloc error
     */
    if (errinfo && errinfo->error_message) {
        msg = errinfo->error_message;
    } else {
        msg = (const xmlChar *)get_error_string(interr);
    }

    if (errinfo && errinfo->error_app_tag) {
        apptag = errinfo->error_app_tag;
    }

    /* setup the return record */
    set_error_record(err, interr, rpcerr, layer, errsev, 
                     rpc_err_get_errtag(rpcerr), apptag, error_path, msg);

    /* figure out the required error-info 
     * It is possible for an attribute error to be recorded
     * through this function due to errors detected after
     * the xml_node_t and xml_attr_t structs have been discarded
     */
    switch (rpcerr) {
    case RPC_ERR_MISSING_ELEMENT:
    case RPC_ERR_UNKNOWN_ELEMENT:
    case RPC_ERR_LOCK_DENIED:
    case RPC_ERR_MISSING_ATTRIBUTE:
    case RPC_ERR_BAD_ATTRIBUTE:
    case RPC_ERR_UNKNOWN_ATTRIBUTE:
        break;
    case RPC_ERR_BAD_ELEMENT:
        if (parmtyp==NCX_NT_STRING) {
            badval = (const xmlChar *)error_parm;
        }
        break;
    case RPC_ERR_UNKNOWN_NAMESPACE:
        if (parmtyp==NCX_NT_STRING) {
            badns = (const xmlChar *)error_parm;
        }
        break;
    case RPC_ERR_DATA_MISSING:
        if (interr == ERR_NCX_MISSING_CHOICE) {
            badval = NULL;
        } else if (interr == ERR_NCX_MISSING_VAL_INST) {
            if (obj) {
                badval = obj_get_name(obj);
            }
        } else {
            just_errnum = TRUE;
        }
        break;
    default:
        if (error_parm && parmtyp==NCX_NT_STRING) {
            badval = (const xmlChar *)error_parm;
        } else {
            just_errnum = TRUE;
        }
    } 

    if (!just_errnum) {
        /* add the required error-info, call even if err2 is NULL */
        /* ignore result and use err anyway */
        add_base_vars(err, rpcerr, errnode, badval, badns, badnsid1, badnsid2,
                      err1, err2, err4);
    }

    /* ignore result and use err anyway */
    add_error_number(err, interr);

    return err;

} /* agt_rpcerr_gen_error_ex */


/********************************************************************
* FUNCTION agt_rpc_gen_insert_error
*
* Generate an internal <rpc-error> record for an element
* for an insert operation failed error
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errval == pointer to the node with the insert error
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_insert_error (ncx_layer_t layer,
                                 status_t   interr,
                                 const val_value_t *errval,
                                 xmlChar *error_path)
{
    rpc_err_rec_t            *err;
    const xmlChar            *badval = NULL, *msg;
    const void               *err2 = NULL;

    assert(errval && "param errval is NULL");

    /* get a new error record */
    err = rpc_err_new_record();
    if (!err) {
        return NULL;
    }

    msg = (const xmlChar *)get_error_string(interr);

    set_error_record(err, interr, RPC_ERR_BAD_ATTRIBUTE, layer, 
                     RPC_ERR_SEV_ERROR, 
                     rpc_err_get_errtag(RPC_ERR_BAD_ATTRIBUTE), 
                     RPC_ERR_APPTAG_INSERT, error_path, msg);

    if (errval->editvars) {
        badval = errval->editvars->insertstr;
    }

    if (errval->obj->objtype == OBJ_TYP_LIST) {
        err2 = (const void *)NCX_EL_KEY;
    } else {
        err2 = (const void *)NCX_EL_VALUE;
    }

    /* add the required error-info, call even if err2 is NULL */
    /* ignore result and use err anyway */
    add_base_vars(err, RPC_ERR_BAD_ATTRIBUTE, NULL, badval, NULL, 
                  xmlns_yang_id(), val_get_nsid(errval), NULL, err2, 
                  (const void *)errval->name);

    /* ignore result and use err anyway */
    add_error_number(err, interr);

    return err;

} /* agt_rpcerr_gen_insert_error */


/********************************************************************
* FUNCTION agt_rpc_gen_unique_error
*
* Generate an internal <rpc-error> record for an element
* for a unique-stmt failed error (data-not-unique)
*
* INPUTS:
*   msghdr == message header to use for prefix storage
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   errval == pointer to the node with the insert error
*   valuniqueQ == Q of val_unique_t structs to
*                   use for <non-unique> elements
*  error_path == malloced string of the value (or type, etc.) instance
*                ID string in NCX_IFMT_XPATH format; this will be added
*                to the rpc_err_rec_t and freed later
*             == NULL if not available
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ
*   NULL if a record could not be allocated or not enough
*     val;id info in the parameters 
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_unique_error (xml_msg_hdr_t *msghdr,
                                 ncx_layer_t layer,
                                 status_t   interr,
                                 const dlq_hdr_t *valuniqueQ,
                                 xmlChar *error_path)
{
    rpc_err_rec_t            *err;
    xmlChar                  *pathbuff;
    const xmlChar            *msg;
    val_unique_t             *unival;
    status_t                  res;

    /* get a new error record */
    err = rpc_err_new_record();
    if (!err) {
        return NULL;
    }

    msg = (const xmlChar *)get_error_string(interr);

    /* setup the return record */
    set_error_record(err, interr, RPC_ERR_OPERATION_FAILED, layer, 
                     RPC_ERR_SEV_ERROR, 
                     rpc_err_get_errtag(RPC_ERR_OPERATION_FAILED),
                     RPC_ERR_APPTAG_UNIQUE_FAILED, error_path, msg);

    for (unival = (val_unique_t *)dlq_firstEntry(valuniqueQ);
         unival != NULL;
         unival = (val_unique_t *)dlq_nextEntry(unival)) {

        pathbuff = NULL;
        xpath_resnode_t *resnode = xpath_get_first_resnode(unival->pcb->result);
        if (resnode == NULL) {
            // should not happen!
            continue;
        }
        val_value_t *valptr = xpath_get_resnode_valptr(resnode);
        if (valptr == NULL) {
            // should not happen!
            continue;
        }
        res = val_gen_instance_id(msghdr, valptr, NCX_IFMT_XPATH1, 
                                  &pathbuff);
        if (res == NO_ERR) {
            res = enque_error_info(err, xmlns_yang_id(), NCX_EL_NON_UNIQUE, 
                                   FALSE, NCX_BT_INSTANCE_ID, 0, NULL, 
                                   pathbuff, NULL);
        }
        if (pathbuff) {
            m__free(pathbuff);
        }
        if (res != NO_ERR) {
            log_error("\nError: could not add unique-error info");
        }
    }

    /* ignore result and use err anyway */
    add_error_number(err, interr);

    return err;

} /* agt_rpcerr_gen_unique_error */


/********************************************************************
* FUNCTION agt_rpc_gen_attr_error
*
* Generate an internal <rpc-error> record for an attribute
*
* INPUTS:
*   layer == protocol layer where the error occurred
*   interr == internal error code
*             if NO_ERR than use the rpcerr only
*   attr   == attribute that had the error
*   errnode == XML node where error occurred
*           == NULL then there is no valid XML node (maybe the error!)
*   errnodeval == valuse struct for the error node id errnode NULL
*           == NULL if not used
*   badns == URI string of the namespace that is bad (or NULL)
*
* RETURNS:
*   pointer to allocated and filled in rpc_err_rec_t struct
*     ready to add to the msg->rpc_errQ (or add more error-info)
*   NULL if a record could not be allocated or not enough
*     valid info in the parameters to generate an error report
*********************************************************************/
rpc_err_rec_t *
    agt_rpcerr_gen_attr_error (ncx_layer_t layer,
                               status_t   interr,
                               const xml_attr_t *attr,
                               const xml_node_t *errnode,
                               const val_value_t *errnodeval,
                               const xmlChar *badns,
                               xmlChar *error_path)
{
    rpc_err_rec_t  *err;
    rpc_err_sev_t   errsev;
    rpc_err_t       rpcerr;
    xmlChar        *badval = NULL;
    const void     *err2 = NULL, *err4 = NULL;
    const xmlChar  *msg, *apptag = NULL;
    xmlns_id_t      badnsid1 = 0, badnsid2 = 0;

    /* get a new error record */
    err = rpc_err_new_record();
    if (!err) {
        return NULL;
    }

    if (attr) {
        badnsid1 = attr->attr_ns;
        err2 = (const void *)attr->attr_name;
    }

    if (errnode == NULL && errnodeval != NULL) {
        badnsid2 = val_get_nsid(errnodeval);
        err4 = errnodeval->name;
    }

    rpcerr = get_rpcerr(interr, FALSE, &errsev, &apptag); 

    /* check the rpcerr out the required error-info */
    switch (rpcerr) {
    case RPC_ERR_UNKNOWN_ATTRIBUTE:
    case RPC_ERR_MISSING_ATTRIBUTE:
    case RPC_ERR_UNKNOWN_NAMESPACE:
        break;
    case RPC_ERR_INVALID_VALUE:
        rpcerr = RPC_ERR_BAD_ATTRIBUTE;
        /* fall through */
    case RPC_ERR_BAD_ATTRIBUTE:
        if (attr) {
            badval = attr->attr_val;
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    } 

    /* generate a default error message */
    msg = (const xmlChar *)get_error_string(interr);

    set_error_record(err, interr, rpcerr, layer, errsev, 
                     rpc_err_get_errtag(rpcerr), apptag, error_path, msg);

    /* add the required error-info */
    /* ignore result and use err anyway */
    add_base_vars(err, rpcerr, errnode, badval, badns, badnsid1, badnsid2, NULL,
                  err2, err4);

    /* ignore result and use err anyway */
    add_error_number(err, interr);

    return err;

} /* agt_rpcerr_gen_attr_error */


/* END file agt_rpcerr.c */
