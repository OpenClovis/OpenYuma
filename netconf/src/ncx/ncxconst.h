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
#ifndef _H_ncxconst
#define _H_ncxconst

/*  FILE: ncxconst.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Contains NCX constants separated to prevent H file include loops
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-nov-05    abb      Begun; split from ncx.h
04-feb-06    abb      Move base/nc.h constants into this file
10-nov-07    abb      Moved types typ ncxtypes.h
*/

#include <math.h>
#include <xmlstring.h>

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* NETCONF Base URN */
#define NC_VER          "1.0"
#define NC_PREFIX       (const xmlChar *)"nc"

#define NC_MODULE       (const xmlChar *)"yuma-netconf"

#define NCN_MODULE       (const xmlChar *)"notifications"

#define NCX_MODULE       (const xmlChar *)"yuma-ncx"
#define NCX_PREFIX       (const xmlChar *)"ncx"

/* source of secure and very-secure extensions! */
#define NACM_PREFIX      (const xmlChar *)"nacm"

#define NC_OK_REPLY  (const xmlChar *)"RpcOkReplyType"

/* NETCONF namespace used for base:10 and base:1.1 */
#define NC_URN  (const xmlChar *)"urn:ietf:params:xml:ns:netconf:base:1.0"

/* NETCONF SSH End of Message Marker */
#define NC_SSH_END "]]>]]>"
#define NC_SSH_END_LEN 6

/* NETCONF SSH End of Chunks Marker */
#define NC_SSH_END_CHUNKS "\n##\n"
#define NC_SSH_END_CHUNKS_LEN 4

/* NETCONF Module Owner */
#define NC_OWNER        (const xmlChar *)"ietf"

/* NETCONF edit-config operation attribute name */
#define NC_OPERATION_ATTR_NAME (const xmlChar *)"operation"

#define NC_RPC_REPLY_TYPE (const xmlChar *)"rpc-reply"

#define NCX_SSH_PORT    22
#define NCX_NCSSH_PORT  830

#define INVALID_URN    (const xmlChar *)"INVALID"

/* base:1.1 subtree wildcard URN */
#define WILDCARD_URN   (const xmlChar *)""
    
/* max number len to use for static buffer allocation only */
#define NCX_MAX_NUMLEN   47

 /* all name fields in YANG can be 1 to N bytes
  * set a limit based on likely malloc failure
  */
#define NCX_MAX_NLEN  0xffffffe

#define NCX_MAX_USERNAME_LEN   127

/* ncxserver server transport */
#define NCX_SERVER_TRANSPORT "ssh"

/* ncxserver server transport for local connections*/
#define NCX_SERVER_TRANSPORT_LOCAL "local"

/* ncxserver version */
#define NCX_SERVER_VERSION   1


/* URN for NETCONF standard modules */
#define NC_URN1         (const xmlChar *)"urn:netconf:params:xml:ns:"


/* URN for with-defaults 'default' XML attribute */
#define NC_WD_ATTR_URN (const xmlChar *)\
    "urn:ietf:params:xml:ns:netconf:default:1.0"
#define NC_WD_ATTR_PREFIX (const xmlChar *)"wda"

/* URN for NCX extensions */
#define NCX_URN    (const xmlChar *)"http://netconfcentral.org/ns/yuma-ncx"
#define NCX_MOD    (const xmlChar *)"yuma-ncx"


/* URN for XSD */
#define XSD_URN         (const xmlChar *)"http://www.w3.org/2001/XMLSchema"
#define XSD_PREFIX      (const xmlChar *)"xs"

/* URN for XSI */
#define XSI_URN         (const xmlChar *)\
			 "http://www.w3.org/2001/XMLSchema-instance"
#define XSI_PREFIX      (const xmlChar *)"xsi"


/* URN for XML */
#define XML_URN         (const xmlChar *)\
			 "http://www.w3.org/XML/1998/namespace"
#define XML_PREFIX      (const xmlChar *)"xml"


/* URN for NETCONF Notifications */
#define NCN_URN         (const xmlChar *)\
                         "urn:ietf:params:xml:ns:netconf:notification:1.0"
#define NCN_PREFIX      (const xmlChar *)"ncn"


/* schemaLocation string */
#define NCX_SCHEMA_LOC  (const xmlChar *)"http://www.netconfcentral.org/xsd/"

#define NCX_XSD_EXT     (const xmlChar *)"xsd"

#define NCX_URNP1       (const xmlChar *)"http://"


/* default prefix to use for NCX namespace */
#define NCX_PREFIX      (const xmlChar *)"ncx"
#define NCX_PREFIX_LEN  3

/* default prefix for the xmlns namespace (not really used */
#define NS_PREFIX      (const xmlChar *)"namespace"
#define NS_PREFIX_LEN  2
#define NS_URN     (const xmlChar *)"http://www.w3.org/2000/xmlns/"

/* needed for xml:lang attribute */
#define NCX_XML_URN  (const xmlChar *)"http://www.w3.org/XML/1998/namespace"
#define NCX_XML_SLOC (const xmlChar *)\
		      "http://www.w3.org/XML/1998/namespace"\
		      "\n    http://www.w3.org/2001/xml.xsd"

#define EMPTY_STRING  (const xmlChar *)""

#define NCX_DEF_MODULE  (const xmlChar *)"yuma-netconf"

#define NCX_DEF_LANG    (const xmlChar *)"en"

#define NCX_DEF_MERGETYPE NCX_MERGE_LAST

#define NCX_DEF_FRACTION_DIGITS 2

#define NCX_DEF_LINELEN     72

#define NCX_MAX_LINELEN     4095

#define NCX_DEF_WARN_IDLEN    64

#define NCX_DEF_WARN_LINELEN  72

#define NCX_CONF_SUFFIX     (const xmlChar *)"conf"

#define NCX_CLI_START_CH   '-'

#define NCX_SQUOTE_CH  '\''
#define NCX_QUOTE_CH   '\"'
#define NCX_VAR_CH     '$'

/* inline XML entered as [<foo>...</foo>] */
#define NCX_XML_CH     '<'
#define NCX_XML1a_CH   '['
#define NCX_XML1b_CH   '<'
#define NCX_XML2a_CH   '>'
#define NCX_XML2b_CH   ']'

#define NCX_ASSIGN_CH  '='
#define NCX_AT_CH      '@'

#define NCX_TABSIZE     8

#define NCX_VERSION_BUFFSIZE  24

/* start of comment token in all NCX text */
#define NCX_COMMENT_CH   '#'

/* scoped identifier field separator token */
#define NCX_SCOPE_CH     '/'

/* filespec identifier field separator token */
#define NCX_PATHSEP_CH     '/'

/* prefix scoped identifier field separator token */
#define NCX_MODSCOPE_CH     ':'

/* start of a double quoted string in NCX text */
#define NCX_QSTRING_CH   '"'

/* start of a single quoted string in NCX text */
#define NCX_SQSTRING_CH   '\''

/* start of an NCX or XPath varbind */
#define NCX_VARBIND_CH    '$'

/* Standard 0x0 syntax to indicate a HEX number is specified */
#define NCX_IS_HEX_CH(c) ((c)=='x' || (c)=='X')

/* max hex digits in the value part of a hex number */
#define NCX_MAX_HEXCHAR 16

/* max attributes that will be printed per line */
#define NCX_ATTR_PER_LINE    3
#define NCX_BIG_ATTR_SIZE    24

/* max decimal digits in a plain int */
#define NCX_MAX_DCHAR   20

/* max digits in a real number */
#define NCX_MAX_RCHAR   255

/* Maximum string length (2^^31 - 1) */
#define NCX_MAX_STRLEN  0x7fffffff

/* Maximum string of a single quoted string (64K - 1) */
#define NCX_MAX_Q_STRLEN  0xffff

/* max number of cached ncx_filptr_t records */
#define NCX_DEF_FILPTR_CACHESIZE  300

/* default indent amount for nesting XML output */
#define NCX_DEF_INDENT  2

/* default virtual value cache timeout value in seconds
 * use 0 to disable the cache and refresh a virtual value
 * every time is is retrieved (2 second default)
 */
#define NCX_DEF_VTIMEOUT  2

/* Default startup config transaction ID file name */
#define NCX_DEF_STARTUP_TXID_FILE  (const xmlChar *)"startup-cfg-txid.txt"

/* Default startup config data file name */
#define NCX_DEF_STARTUP_FILE  (const xmlChar *)"startup-cfg.xml"

#define NCX_YUMA_HOME_STARTUP_FILE  (const xmlChar *)\
    "$YUMA_HOME/data/startup-cfg.xml"

#define NCX_YUMA_HOME_STARTUP_DIR  (const xmlChar *)\
    "$YUMA_HOME/data/"

#define NCX_DOT_YUMA_STARTUP_FILE  (const xmlChar *)\
    "~/.yuma/startup-cfg.xml"

#define NCX_DOT_YUMA_STARTUP_DIR  (const xmlChar *)"~/.yuma/"

#define NCX_YUMA_INSTALL_STARTUP_FILE  (const xmlChar *)\
    "$YUMA_INSTALL/data/startup-cfg.xml"

#define NCX_DEF_INSTALL_STARTUP_FILE  (const xmlChar *)\
    "/etc/yuma/startup-cfg.xml"

/* Default backup config data file name */
#define NCX_DEF_BACKUP_FILE  (const xmlChar *)"backup-cfg.xml"

/* default conrm-tmieout value in seconds */
#define NCX_DEF_CONFIRM_TIMEOUT  600

/* default value for the with-defaults option */
#define NCX_DEF_WITHDEF   NCX_WITHDEF_EXPLICIT

/* default value for the with-metadata option */
#define NCX_DEF_WITHMETA  FALSE

/* default value for the agent superuser */
#define NCX_DEF_SUPERUSER  NCX_EL_SUPERUSER

#define NCX_DEF_STREAM_NAME  (const xmlChar *)"NETCONF"

#define NCX_DEF_STREAM_DESCR (const xmlChar *)\
    "default NETCONF event stream"

/* String start char:
 * if the XML string starts with a double quote
 * then it will be interpreted as whitespace-allowed
 */
#define NCX_STR_START   (xmlChar)'"'
#define NCX_STR_END     (xmlChar)'"'


/* Enumeration number start and end chars */
#define NCX_ENU_START   (xmlChar)'('
#define NCX_ENU_END     (xmlChar)')'


/* String names matching psd_pstype_t enums */
#define NCX_PSTYP_CLI        (const xmlChar *)"cli"
#define NCX_PSTYP_DATA       (const xmlChar *)"data"
#define NCX_PSTYP_RPC        (const xmlChar *)"rpc"

/* file name prefix to use for split SIL files */
#define NCX_USER_SIL_PREFIX (const xmlChar *)"u_"
#define NCX_YUMA_SIL_PREFIX (const xmlChar *)"y_"

/* Min and max int */
#define NCX_MIN_INT   INT_MIN
#define NCX_MAX_INT   INT_MAX

#define NCX_MIN_INT8    -128
#define NCX_MAX_INT8    127

#define NCX_MIN_INT16   -32768
#define NCX_MAX_INT16   32767

/* Min and max long */
#define NCX_MAX_LONG   9223372036854775807LL
#define NCX_MIN_LONG   (-NCX_MAX_LONG - 1LL)


/* Min and max uint */
#define NCX_MIN_UINT   0
#define NCX_MAX_UINT   UINT_MAX

#define NCX_MAX_UINT8  255
#define NCX_MAX_UINT16 65535

/* Min and max uint64 */
#define NCX_MIN_ULONG   0
#define NCX_MAX_ULONG   18446744073709551615ULL


/* Min and max float */
#define NCX_MIN_FLOAT   "-inf"
#define NCX_MAX_FLOAT   "inf"

/* Min and max double */
#define NCX_MIN_DOUBLE  "-inf"
#define NCX_MAX_DOUBLE   "inf"


/* NETCONF built-in config names */
#define NCX_CFG_RUNNING      (const xmlChar *)"running"
#define NCX_CFG_CANDIDATE    (const xmlChar *)"candidate"
#define NCX_CFG_STARTUP      (const xmlChar *)"startup"

#define NCX_NUM_CFGS         3

/* NCX Extension configuration names */
#define NCX_CFG_ROLLBACK     (const xmlChar *)"rollback"


/* NCX and NETCONF element and attribute names
 *
 * NETCONF common parameter names 
 * NETCONF built-in error-info element names 
 * name of Xpath index for position-only table index 
 * NCX token and element names 
 * NCX token names of the builtin types used in the <syntax> clause 
 *
 */

#define NCX_EL_ACCESS_RC       (const xmlChar *)"read-create"
#define NCX_EL_ACCESS_RO       (const xmlChar *)"read-only"
#define NCX_EL_ACCESS_RW       (const xmlChar *)"read-write"
#define NCX_EL_AUDIT_LOG       (const xmlChar *)"audit-log"
#define NCX_EL_AUDIT_LOG_APPEND (const xmlChar *)"audit-log-append"
#define NCX_AUGHOOK_START      (const xmlChar *)"__"
#define NCX_AUGHOOK_END        (const xmlChar *)".A__"

#define NCX_EL_ABSTRACT        (const xmlChar *)"abstract"
#define NCX_EL_ACCESS_CONTROL  (const xmlChar *)"access-control"
#define NCX_EL_ADDRESS         (const xmlChar *)"address"
#define NCX_EL_ALT_NAME        (const xmlChar *)"alt-name"
#define NCX_EL_ANY             (const xmlChar *)"any"
#define NCX_EL_ANYXML          (const xmlChar *)"anyxml"
#define NCX_EL_APPINFO         (const xmlChar *)"appinfo"
#define NCX_EL_APPLICATION     (const xmlChar *)"application"
#define NCX_EL_BAD_ATTRIBUTE   (const xmlChar *)"bad-attribute"
#define NCX_EL_BAD_ELEMENT     (const xmlChar *)"bad-element"
#define NCX_EL_BAD_NAMESPACE   (const xmlChar *)"bad-namespace"
#define NCX_EL_BAD_VALUE       (const xmlChar *)"bad-value"
#define NCX_EL_BINARY          (const xmlChar *)"binary"
#define NCX_EL_BITS            (const xmlChar *)"bits"
#define NCX_EL_BOOLEAN         (const xmlChar *)"boolean"
#define NCX_EL_BRIEF           (const xmlChar *)"brief"
#define NCX_EL_BYTE            (const xmlChar *)"byte"
#define NCX_EL_C               (const xmlChar *)"c"
#define NCX_EL_CPP_TEST        (const xmlChar *)"cpp_test"
#define NCX_EL_CANCEL          (const xmlChar *)"cancel"
#define NCX_EL_CANCEL_COMMIT   (const xmlChar *)"cancel-commit"
#define NCX_EL_CANDIDATE       (const xmlChar *)"candidate"
#define NCX_EL_CAPABILITIES    (const xmlChar *)"capabilities"
#define NCX_EL_CAPABILITY      (const xmlChar *)"capability"
#define NCX_EL_CASE            (const xmlChar *)"case"
#define NCX_EL_CASE_NAME       (const xmlChar *)"case-name"
#define NCX_EL_CHOICE          (const xmlChar *)"choice"
#define NCX_EL_CHOICE_NAME     (const xmlChar *)"choice-name"
#define NCX_EL_CLASS           (const xmlChar *)"class"
#define NCX_EL_CLI             (const xmlChar *)"cli"
#define NCX_EL_CLOSE_SESSION   (const xmlChar *)"close-session"
#define NCX_EL_COMMIT          (const xmlChar *)"commit"
#define NCX_EL_COMPLETE        (const xmlChar *)"complete"
#define NCX_EL_CONDITION       (const xmlChar *)"condition"
#define NCX_EL_CONFIG          (const xmlChar *)"config"
#define NCX_EL_CONFIRMED       (const xmlChar *)"confirmed"
#define NCX_EL_CONFIRMED_COMMIT  (const xmlChar *)"confirmed-commit"
#define NCX_EL_CONFIRM_TIMEOUT (const xmlChar *)"confirm-timeout"
#define NCX_EL_CONTAINER       (const xmlChar *)"container"
#define NCX_EL_CONTACT_INFO    (const xmlChar *)"contact-info"
#define NCX_EL_CONTINUE_ON_ERROR (const xmlChar *)"continue-on-error"
#define NCX_EL_COPY            (const xmlChar *)"copy"
#define NCX_EL_COPYRIGHT       (const xmlChar *)"copyright"
#define NCX_EL_COPY_CONFIG     (const xmlChar *)"copy-config"
#define NCX_EL_CREATE          (const xmlChar *)"create"
#define NCX_EL_CURRENT         (const xmlChar *)"current"
#define NCX_EL_DATA            (const xmlChar *)"data"
#define NCX_EL_DATAPATH        (const xmlChar *)"datapath"
#define NCX_EL_DATA_CLASS      (const xmlChar *)"data-class"
#define NCX_EL_DEBUG           (const xmlChar *)"debug"
#define NCX_EL_DECIMAL64       (const xmlChar *)"decimal64"
#define NCX_EL_DEF             (const xmlChar *)"def"
#define NCX_EL_DEFAULT         (const xmlChar *)"default"
#define NCX_EL_DEFAULT_OPERATION (const xmlChar *)"default-operation"
#define NCX_EL_DEFAULT_PARM    (const xmlChar *)"default-parm"
#define NCX_EL_DEFAULT_PARM_EQUALS_OK (const xmlChar *)"default-parm-equals-ok"
#define NCX_EL_DEFAULT_STYLE   (const xmlChar *)"default-style"
#define NCX_EL_DEFINITIONS     (const xmlChar *)"definitions"
#define NCX_EL_DEFOP           (const xmlChar *)"default-operation"
#define NCX_EL_DELETE          (const xmlChar *)"delete"
#define NCX_EL_DELETE_CONFIG   (const xmlChar *)"delete-config"
#define NCX_EL_DEPRECATED      (const xmlChar *)"deprecated"
#define NCX_EL_DESCRIPTION     (const xmlChar *)"description"
#define NCX_EL_DEVIATION       (const xmlChar *)"deviation"
#define NCX_EL_DISABLED        (const xmlChar *)"disabled"
#define NCX_EL_DISCARD_CHANGES (const xmlChar *)"discard-changes"
#define NCX_EL_DOUBLE          (const xmlChar *)"double"
#define NCX_EL_DYNAMIC         (const xmlChar *)"dynamic"
#define NCX_EL_EDIT_CONFIG     (const xmlChar *)"edit-config"
#define NCX_EL_EMPTY           (const xmlChar *)"empty"
#define NCX_EL_ENFORCING       (const xmlChar *)"enforcing"
#define NCX_EL_ENUM            (const xmlChar *)"enum"
#define NCX_EL_ENUMERATION     (const xmlChar *)"enumeration"
#define NCX_EL_ERROR           (const xmlChar *)"error"
#define NCX_EL_ERROR_APP_TAG   (const xmlChar *)"error-app-tag"
#define NCX_EL_ERROR_INFO      (const xmlChar *)"error-info"
#define NCX_EL_ERROR_LEVEL     (const xmlChar *)"error-level"
#define NCX_EL_ERROR_NUMBER    (const xmlChar *)"error-number"
#define NCX_EL_ERROR_OPTION    (const xmlChar *)"error-option"
#define NCX_EL_ERROR_MESSAGE   (const xmlChar *)"error-message"
#define NCX_EL_ERROR_PATH      (const xmlChar *)"error-path"
#define NCX_EL_ERROR_SEVERITY  (const xmlChar *)"error-severity"
#define NCX_EL_ERROR_TAG       (const xmlChar *)"error-tag"
#define NCX_EL_ERROR_TYPE      (const xmlChar *)"error-type"
#define NCX_EL_EVENTLOG_SIZE   (const xmlChar *)"eventlog-size"
#define NCX_EL_EVENTTIME       (const xmlChar *)"eventTime"
#define NCX_EL_EXACT           (const xmlChar *)"exact"
#define NCX_EL_EXACT_NOCASE    (const xmlChar *)"exact-nocase"
#define NCX_EL_EXEC            (const xmlChar *)"exec"
#define NCX_EL_EXPLICIT        (const xmlChar *)"explicit"
#define NCX_EL_EXTEND          (const xmlChar *)"extend"
#define NCX_EL_EXTERN          (const xmlChar *)"extern"
#define NCX_EL_FALSE           (const xmlChar *)"false"
#define NCX_EL_FEATURE_CODE_DEFAULT \
    (const xmlChar *)"feature-code-default"
#define NCX_EL_FEATURE_ENABLE_DEFAULT \
    (const xmlChar *)"feature-enable-default"
#define NCX_EL_FEATURE_STATIC (const xmlChar *)"feature-static"
#define NCX_EL_FEATURE_DYNAMIC (const xmlChar *)"feature-dynamic"
#define NCX_EL_FEATURE_ENABLE (const xmlChar *)"feature-enable"
#define NCX_EL_FEATURE_DISABLE (const xmlChar *)"feature-disable"
#define NCX_EL_FILTER          (const xmlChar *)"filter"
#define NCX_EL_FIRST           (const xmlChar *)"first"
#define NCX_EL_FIRST_NOCASE    (const xmlChar *)"first-nocase"
#define NCX_EL_FLAG            (const xmlChar *)"flag"
#define NCX_EL_FLOAT           (const xmlChar *)"float"
#define NCX_EL_FLOAT64         (const xmlChar *)"float64"
#define NCX_EL_FORMAT          (const xmlChar *)"format"
#define NCX_EL_FULL            (const xmlChar *)"full"
#define NCX_EL_GET             (const xmlChar *)"get"
#define NCX_EL_GET_CONFIG      (const xmlChar *)"get-config"
#define NCX_EL_GET_SCHEMA      (const xmlChar *)"get-schema"
#define NCX_EL_GROUP_ID        (const xmlChar *)"group-id"
#define NCX_EL_H               (const xmlChar *)"h"
#define NCX_EL_HEADER          (const xmlChar *)"header"
#define NCX_EL_HELLO           (const xmlChar *)"hello"
#define NCX_EL_HELLO_TIMEOUT   (const xmlChar *)"hello-timeout"
#define NCX_EL_HELP            (const xmlChar *)"help"
#define NCX_EL_HIDDEN          (const xmlChar *)"hidden"
#define NCX_EL_HOME            (const xmlChar *)"home"
#define NCX_EL_HTML            (const xmlChar *)"html"
#define NCX_EL_IDENTIFIER      (const xmlChar *)"identifier"
#define NCX_EL_IDENTITYREF     (const xmlChar *)"identityref"
#define NCX_EL_IDLE_TIMEOUT    (const xmlChar *)"idle-timeout"
#define NCX_EL_ILLEGAL         (const xmlChar *)"illegal"
#define NCX_EL_IMPORT          (const xmlChar *)"import"
#define NCX_EL_IMPORTS         (const xmlChar *)"imports"
#define NCX_EL_INCLUDE         (const xmlChar *)"include"
#define NCX_EL_INDENT          (const xmlChar *)"indent"
#define NCX_EL_INFO            (const xmlChar *)"info"
#define NCX_EL_INPUT           (const xmlChar *)"input"
#define NCX_EL_INSTANCE_IDENTIFIER \
                               (const xmlChar *)"instance-identifier"
#define NCX_EL_INT             (const xmlChar *)"int"
#define NCX_EL_INT8            (const xmlChar *)"int8"
#define NCX_EL_INT16           (const xmlChar *)"int16"
#define NCX_EL_INT32           (const xmlChar *)"int32"
#define NCX_EL_INT64           (const xmlChar *)"int64"
#define NCX_EL_JSON            (const xmlChar *)"json"
#define NCX_EL_KEY             (const xmlChar *)"key"
#define NCX_EL_KILL_SESSION    (const xmlChar *)"kill-session"
#define NCX_EL_LANG            (const xmlChar *)"xml:lang"
#define NCX_EL_LAST_MODIFIED   (const xmlChar *)"last-modified"
#define NCX_EL_LEAFREF         (const xmlChar *)"leafref"
#define NCX_EL_LINESIZE        (const xmlChar *)"linesize"
#define NCX_EL_LIST            (const xmlChar *)"list"
#define NCX_EL_LOAD            (const xmlChar *)"load"
#define NCX_EL_LOAD_CONFIG     (const xmlChar *)"load-config"
#define NCX_EL_LOCK            (const xmlChar *)"lock"
#define NCX_EL_LOCK_SOURCE     (const xmlChar *)"lock-source"
#define NCX_EL_LOG             (const xmlChar *)"log"
#define NCX_EL_LOGAPPEND       (const xmlChar *)"log-append"
#define NCX_EL_LOGLEVEL        (const xmlChar *)"log-level"
#define NCX_EL_LONG            (const xmlChar *)"long"
#define NCX_EL_MAGIC           (const xmlChar *)"magic"
#define NCX_EL_MAX_ACCESS      (const xmlChar *)"max-access"
#define NCX_EL_MAX_BURST       (const xmlChar *)"max-burst"
#define NCX_EL_MERGE           (const xmlChar *)"merge"
#define NCX_EL_MESSAGE_ID      (const xmlChar *)"message-id"
#define NCX_EL_METADATA        (const xmlChar *)"metadata"
#define NCX_EL_MISSING_CHOICE  (const xmlChar *)"missing-choice"
#define NCX_EL_MODPATH         (const xmlChar *)"modpath"
#define NCX_EL_MODULE          (const xmlChar *)"module"
#define NCX_EL_MODULES         (const xmlChar *)"modules"
#define NCX_EL_MOD_REVISION    (const xmlChar *)"mod-revision"
#define NCX_EL_MONITOR         (const xmlChar *)"monitor"
#define NCX_EL_NAME            (const xmlChar *)"name"
#define NCX_EL_NCX             (const xmlChar *)"ncx"
#define NCX_EL_NCXCONNECT      (const xmlChar *)"ncx-connect"
#define NCX_EL_NAMESPACE       (const xmlChar *)"namespace"
#define NCX_EL_NETCONF         (const xmlChar *)"netconf"
#define NCX_EL_NETCONF10       (const xmlChar *)"netconf1.0"
#define NCX_EL_NETCONF11       (const xmlChar *)"netconf1.1"
#define NCX_EL_NO              (const xmlChar *)"no"
#define NCX_EL_NODEFAULT       (const xmlChar *)"no default"
#define NCX_EL_NODUPLICATES    (const xmlChar *)"no-duplicates"
#define NCX_EL_NONE            (const xmlChar *)"none"
#define NCX_EL_NON_UNIQUE      (const xmlChar *)"non-unique"
#define NCX_EL_NO_OP           (const xmlChar *)"no-op"
#define NCX_EL_NOOP_ELEMENT    (const xmlChar *)"noop-element"
#define NCX_EL_NOTIFICATION    (const xmlChar *)"notification"
#define NCX_EL_NOT_USED        (const xmlChar *)"not-used"
#define NCX_EL_NOT_SET         (const xmlChar *)"not-set"
#define NCX_EL_NULL            (const xmlChar *)"null"
#define NCX_EL_OBJECT          (const xmlChar *)"object"
#define NCX_EL_OBJECTS         (const xmlChar *)"objects"
#define NCX_EL_OBSOLETE        (const xmlChar *)"obsolete"
#define NCX_EL_OFF             (const xmlChar *)"off"
#define NCX_EL_OK              (const xmlChar *)"ok"
#define NCX_EL_OK_ELEMENT      (const xmlChar *)"ok-element"
#define NCX_EL_ONE             (const xmlChar *)"one"
#define NCX_EL_ONE_NOCASE      (const xmlChar *)"one-nocase"
#define NCX_EL_ORDER           (const xmlChar *)"order"
#define NCX_EL_ORDER_L         (const xmlChar *)"loose"
#define NCX_EL_ORDER_S         (const xmlChar *)"strict"
#define NCX_EL_OTHER           (const xmlChar *)"other"
#define NCX_EL_OUTPUT          (const xmlChar *)"output"
#define NCX_EL_OWNER           (const xmlChar *)"owner"
#define NCX_EL_PARM            (const xmlChar *)"parm"
#define NCX_EL_PARMSET         (const xmlChar *)"parmset"
#define NCX_EL_PARMS           (const xmlChar *)"parms"
#define NCX_EL_PASSWORD        (const xmlChar *)"password"
#define NCX_EL_PATH            (const xmlChar *)"path"
#define NCX_EL_PATTERN         (const xmlChar *)"pattern"
#define NCX_EL_PERMISSIVE      (const xmlChar *)"permissive"
#define NCX_EL_PERSIST         (const xmlChar *)"persist"
#define NCX_EL_PERSIST_ID      (const xmlChar *)"persist-id"
#define NCX_EL_PLAIN           (const xmlChar *)"plain"
#define NCX_EL_PORT            (const xmlChar *)"port"
#define NCX_EL_POS             (const xmlChar *)"pos"
#define NCX_EL_POSITION        (const xmlChar *)"position"
#define NCX_EL_PREFIX          (const xmlChar *)"prefix"
#define NCX_EL_PROTOCOL        (const xmlChar *)"protocol"
#define NCX_EL_PROTOCOLS       (const xmlChar *)"protocols"
#define NCX_EL_QNAME           (const xmlChar *)"qname"
#define NCX_EL_REMOVE          (const xmlChar *)"remove"
#define NCX_EL_REPLACE         (const xmlChar *)"replace"
#define NCX_EL_REPORT_ALL      (const xmlChar *)"report-all"
#define NCX_EL_REPORT_ALL_TAGGED (const xmlChar *)"report-all-tagged"
#define NCX_EL_RESTART         (const xmlChar *)"restart"
#define NCX_EL_REVISION        (const xmlChar *)"revision"
#define NCX_EL_REVISION_HISTORY (const xmlChar *)"revision-history"
#define NCX_EL_ROLLBACK_ON_ERROR (const xmlChar *)"rollback-on-error"
#define NCX_EL_ROOT            (const xmlChar *)"root"
#define NCX_EL_RPC             (const xmlChar *)"rpc"
#define NCX_EL_RPC_ERROR       (const xmlChar *)"rpc-error"
#define NCX_EL_RPC_OUTPUT      (const xmlChar *)"rpc-output"
#define NCX_EL_RPC_REPLY       (const xmlChar *)"rpc-reply"
#define NCX_EL_RPC_TYPE        (const xmlChar *)"rpc-type"
#define NCX_EL_RUNNING         (const xmlChar *)"running"
#define NCX_EL_RUNPATH         (const xmlChar *)"runpath"
#define NCX_EL_SCHEMA_INSTANCE (const xmlChar *)"schema-instance"
#define NCX_EL_SCRIPT          (const xmlChar *)"script"
#define NCX_EL_SECURE          (const xmlChar *)"secure"
#define NCX_EL_SELECT          (const xmlChar *)"select"
#define NCX_EL_SEQUENCE_ID     (const xmlChar *)"sequence-id"
#define NCX_EL_SERVER          (const xmlChar *)"server"
#define NCX_EL_SESSION_ID      (const xmlChar *)"session-id"
#define NCX_EL_SET             (const xmlChar *)"set"
#define NCX_EL_SHORT           (const xmlChar *)"short"
#define NCX_EL_SHOW_ERRORS     (const xmlChar *)"show-errors"
#define NCX_EL_SHUTDOWN        (const xmlChar *)"shutdown"
#define NCX_EL_SIL_DELETE_CHILDREN_FIRST (const xmlChar *)\
    "sil-delete-children-first"
#define NCX_EL_SLIST           (const xmlChar *)"slist"
#define NCX_EL_SOURCE          (const xmlChar *)"source"
#define NCX_EL_SQL             (const xmlChar *)"sql"
#define NCX_EL_SQLDB           (const xmlChar *)"sqldb"
#define NCX_EL_START           (const xmlChar *)"start"
#define NCX_EL_STARTUP         (const xmlChar *)"startup"
#define NCX_EL_STATE           (const xmlChar *)"state"
#define NCX_EL_STATIC          (const xmlChar *)"static"
#define NCX_EL_STATUS          (const xmlChar *)"status"
#define NCX_EL_STOP_ON_ERROR   (const xmlChar *)"stop-on-error"
#define NCX_EL_STRING          (const xmlChar *)"string"
#define NCX_EL_STRUCT          (const xmlChar *)"struct"
#define NCX_EL_SUBDIRS         (const xmlChar *)"subdirs"
#define NCX_EL_SUBMODULE       (const xmlChar *)"submodule"
#define NCX_EL_SUBTREE         (const xmlChar *)"subtree"
#define NCX_EL_SUPERUSER       (const xmlChar *)"superuser"
#define NCX_EL_SYNTAX          (const xmlChar *)"syntax"
#define NCX_EL_SYSTEM_SORTED   (const xmlChar *)"system-sorted"
#define NCX_EL_TABLE           (const xmlChar *)"table"
#define NCX_EL_TARGET          (const xmlChar *)"target"
#define NCX_EL_TESTONLY        (const xmlChar *)"test-only"
#define NCX_EL_TEST_OPTION     (const xmlChar *)"test-option"
#define NCX_EL_TESTTHENSET     (const xmlChar *)"test-then-set"
#define NCX_EL_TEXT            (const xmlChar *)"text"
#define NCX_EL_TG2             (const xmlChar *)"tg2"
#define NCX_EL_TIMEOUT         (const xmlChar *)"timeout"
#define NCX_EL_TXT             (const xmlChar *)"txt"
#define NCX_EL_TRANSPORT       (const xmlChar *)"transport"
#define NCX_EL_TRIM            (const xmlChar *)"trim"
#define NCX_EL_TRUE            (const xmlChar *)"true"
#define NCX_EL_TYPE            (const xmlChar *)"type"
#define NCX_EL_UC              (const xmlChar *)"uc"
#define NCX_EL_UH              (const xmlChar *)"uh"
#define NCX_EL_UINT8           (const xmlChar *)"uint8"
#define NCX_EL_UINT16          (const xmlChar *)"uint16"
#define NCX_EL_UINT32          (const xmlChar *)"uint32"
#define NCX_EL_UINT64          (const xmlChar *)"uint64"
#define NCX_EL_UNION           (const xmlChar *)"union"
#define NCX_EL_UNITS           (const xmlChar *)"units"
#define NCX_EL_UNLOCK          (const xmlChar *)"unlock"
#define NCX_EL_UNSIGNED_BYTE   (const xmlChar *)"unsignedByte"
#define NCX_EL_UNSIGNED_INT    (const xmlChar *)"unsignedInt"
#define NCX_EL_UNSIGNED_LONG   (const xmlChar *)"unsignedLong"
#define NCX_EL_UNSIGNED_SHORT  (const xmlChar *)"unsignedShort"
#define NCX_EL_ULONG           (const xmlChar *)"ulong"
#define NCX_EL_UPDATE          (const xmlChar *)"update"
#define NCX_EL_URL             (const xmlChar *)"url"
#define NCX_EL_URLTARGET       (const xmlChar *)"urltarget"
#define NCX_EL_USAGE           (const xmlChar *)"usage"
#define NCX_EL_USAGE_C         (const xmlChar *)"conditional"
#define NCX_EL_USAGE_M         (const xmlChar *)"mandatory"
#define NCX_EL_USAGE_O         (const xmlChar *)"optional"
#define NCX_EL_USER            (const xmlChar *)"user"
#define NCX_EL_USER_WRITE      (const xmlChar *)"user-write"
#define NCX_EL_USEXMLORDER     (const xmlChar *)"usexmlorder"
#define NCX_EL_USTRING         (const xmlChar *)"ustring"
#define NCX_EL_VALIDATE        (const xmlChar *)"validate"
#define NCX_EL_VALUE           (const xmlChar *)"value"
#define NCX_EL_VAR             (const xmlChar *)"var"
#define NCX_EL_VARS            (const xmlChar *)"vars"
#define NCX_EL_VERSION         (const xmlChar *)"version"
#define NCX_EL_VERY_SECURE     (const xmlChar *)"very-secure"
#define NCX_EL_WARNING         (const xmlChar *)"warning"
#define NCX_EL_WARN_IDLEN      (const xmlChar *)"warn-idlen"
#define NCX_EL_WARN_LINELEN    (const xmlChar *)"warn-linelen"
#define NCX_EL_WARN_OFF        (const xmlChar *)"warn-off"
#define NCX_EL_WHEN            (const xmlChar *)"when"
#define NCX_EL_WITH_DEFAULTS   (const xmlChar *)"with-defaults"
#define NCX_EL_WITH_METADATA   (const xmlChar *)"with-metadata"
#define NCX_EL_WITH_STARTUP    (const xmlChar *)"with-startup"
#define NCX_EL_WITH_URL        (const xmlChar *)"with-url"
#define NCX_EL_WITH_VALIDATE   (const xmlChar *)"with-validate"
#define NCX_EL_WRITABLE_RUNNING (const xmlChar *)"writable-running"
#define NCX_EL_XCONTAINER      (const xmlChar *)"xcontainer"
#define NCX_EL_XLIST           (const xmlChar *)"xlist"
#define NCX_EL_XML             (const xmlChar *)"xml"
#define NCX_EL_XML_NONS        (const xmlChar *)"xml-nons"
#define NCX_EL_XPATH           (const xmlChar *)"xpath"
#define NCX_EL_XSD             (const xmlChar *)"xsd"
#define NCX_EL_XSDLIST         (const xmlChar *)"xsdlist"
#define NCX_EL_YANG            (const xmlChar *)"yang"
#define NCX_EL_YC              (const xmlChar *)"yc"
#define NCX_EL_YES             (const xmlChar *)"yes"
#define NCX_EL_YH              (const xmlChar *)"yh"
#define NCX_EL_YIN             (const xmlChar *)"yin"
#define NCX_EL_YUMA_HOME       (const xmlChar *)"yuma-home"

/* bit definitions for ncx_lstr_t flags field */
#define NCX_FL_RANGE_ERR   bit0
#define NCX_FL_VALUE_ERR   bit1

/* bit definitions for NETCONF session protocol versions */
#define NCX_FL_PROTO_NETCONF10  bit0
#define NCX_FL_PROTO_NETCONF11  bit1


/* textual parameter tags for various NCX functions
 * that use boolean parameters
 */
#define NCX_SAVESTR    TRUE
#define NCX_NO_SAVESTR FALSE

#define NCX_LMEM_ENUM(L)   ((L)->val.enu)
#define NCX_LMEM_STR(L)    ((L)->val.str)
#define NCX_LMEM_STRVAL(L) ((L)->val.str)
#define NCX_LMEM_NUM(L)    ((L)->val.num)

/* constants for isglobal function parameter */
#define ISGLOBAL  TRUE
#define ISLOCAL   FALSE

/* bad-data enumeration values */
#define E_BAD_DATA_IGNORE (const xmlChar *)"ignore"
#define E_BAD_DATA_WARN   (const xmlChar *)"warn"
#define E_BAD_DATA_CHECK  (const xmlChar *)"check"
#define E_BAD_DATA_ERROR  (const xmlChar *)"error"

#define COPYRIGHT_STRING \
    "Copyright (c) 2008-2012, Andy Bierman, All Rights Reserved.\n"


#define Y_PREFIX         (const xmlChar *)"y_"
#define U_PREFIX         (const xmlChar *)"u_"
#define EDIT_SUFFIX      (const xmlChar *)"_edit"
#define GET_SUFFIX       (const xmlChar *)"_get"
#define MRO_SUFFIX       (const xmlChar *)"_mro"
#define INIT_SUFFIX      (const xmlChar *)"_init"
#define INIT2_SUFFIX     (const xmlChar *)"_init2"
#define CLEANUP_SUFFIX   (const xmlChar *)"_cleanup"

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_ncxconst */
