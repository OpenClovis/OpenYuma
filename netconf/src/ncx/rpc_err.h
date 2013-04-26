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
#ifndef _H_rpc_err
#define _H_rpc_err
/*  FILE: rpc_err.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol standard error definitions

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
06-apr-05    abb      Begun.
*/

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

/***

   From draft-ietf-netconf-prot-12.txt:

   Tag:         in-use
   Error-type:  protocol, application
   Severity:    error
   Error-info:  none
   Description: The request requires a resource that already in use.

   Tag:         invalid-value
   Error-type:  protocol, application
   Severity:    error
   Error-info:  none
   Description: The request specifies an unacceptable value for one
                or more parameters.

   Tag:         too-big
   Error-type:  transport, rpc, protocol, application
   Severity:    error
   Error-info:  none
   Description: The request or response (that would be generated) is too
                large for the implementation to handle.

   Tag:         missing-attribute
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-attribute> : name of the missing attribute
                <bad-element> : name of the element that should
                contain the missing attribute
   Description: An expected attribute is missing

   Tag:         bad-attribute
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-attribute> : name of the attribute w/ bad value
                <bad-element> : name of the element that contains
                the attribute with the bad value
   Description: An attribute value is not correct; e.g., wrong type,
                out of range, pattern mismatch

   Tag:         unknown-attribute
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-attribute> : name of the unexpected attribute
                <bad-element> : name of the element that contains
                the unexpected attribute
   Description: An unexpected attribute is present

   Tag:         missing-element
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-element> : name of the missing element
   Description: An expected element is missing

   Tag:         bad-element
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-element> : name of the element w/ bad value
   Description: An element value is not correct; e.g., wrong type,
                out of range, pattern mismatch

   Tag:         unknown-element
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-element> : name of the unexpected element
   Description: An unexpected element is present

   Tag:         unknown-namespace
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  <bad-element> : name of the element that contains
                the unexpected namespace
                <bad-namespace> : name of the unexpected namespace
   Description: An unexpected namespace is present

   Tag:         access-denied
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  none
   Description: Access to the requested RPC, protocol operation,
                or data model is denied because authorization failed

   Tag:         lock-denied
   Error-type:  protocol
   Severity:    error
   Error-info:  <session-id> : session ID of session holding the
                requested lock, or zero to indicate a non-NETCONF
                entity holds the lock
   Description: Access to the requested lock is denied because the
                lock is currently held by another entity

   Tag:         resource-denied
   Error-type:  transport, rpc, protocol, application
   Severity:    error
   Error-info:  none
   Description: Request could not be completed because of insufficient
                resources

   Tag:         rollback-failed
   Error-type:  protocol, application
   Severity:    error
   Error-info:  none
   Description: Request to rollback some configuration change (via
                rollback-on-error or discard-changes operations) was
                not completed for some reason.

   Tag:         data-exists
   Error-type:  application
   Severity:    error
   Error-info:  none
   Description: Request could not be completed because the relevant
                data model content already exists. For example,
                a 'create' operation was attempted on data which
                already exists.

   Tag:         data-missing
   Error-type:  application
   Severity:    error
   Error-info:  none
   Description: Request could not be completed because the relevant
                data model content does not exist.  For example,
                a 'replace' or 'delete' operation was attempted on
                data which does not exist.

   Tag:         operation-not-supported
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  none
   Description: Request could not be completed because the requested
                operation is not supported by this implementation.

   Tag:         operation-failed
   Error-type:  rpc, protocol, application
   Severity:    error
   Error-info:  none
   Description: Request could not be completed because the requested
                operation failed for some reason not covered by
                any other error condition.

   Tag:         partial-operation
   Error-type:  application
   Severity:    error
   Error-info:  <ok-element> : identifies an element in the data model
                for which the requested operation has been completed
                for that node and all its child nodes.  This element
                can appear zero or more times in the <error-info>
                container.

                <err-element> : identifies an element in the data model
                for which the requested operation has failed for that
                node and all its child nodes. This element
                can appear zero or more times in the <error-info>
                container.

                <noop-element> : identifies an element in the data model
                for which the requested operation was not attempted for
                that node and all its child nodes. This element
                can appear zero or more times in the <error-info>
                container.

   Description: Some part of the requested operation failed or was
                not attempted for some reason.  Full cleanup has
                not been performed (e.g., rollback not supported)
                by the server.  The error-info container is used
                to identify which portions of the application
                data model content for which the requested operation
                has succeeded (<ok-element>), failed (<bad-element>),
                or not attempted (<noop-element>).

   [New in NETCONF base:1.1; do not send to NETCONF:base:1.0 sessions]

    error-tag:      malformed-message
    error-type:     rpc
    error-severity: error
    error-info:     none
    Description:    A message could not be handled because it failed to
                 be parsed correctly. For example, the message is not
                 well-formed XML or it uses an invalid character set.

                 This error-tag is new in :base:1.1 and MUST NOT be
                 sent to old clients.



***/

#ifndef _H_dlq
#include "dlq.h"
#endif

/********************************************************************
*                                                                   *
*                        C O N S T A N T S                          *
*                                                                   *
*********************************************************************/

/*** YANG standard error-app-tag values ***/
/* 13.1 */
#define RPC_ERR_APPTAG_UNIQUE_FAILED    (const xmlChar *)"data-not-unique"

/* 13.2 */
#define RPC_ERR_APPTAG_MAX_ELEMS       (const xmlChar *)"too-many-elements"

/* 13.3 */
#define RPC_ERR_APPTAG_MIN_ELEMS       (const xmlChar *)"too-few-elements"

/* 13.4 */
#define RPC_ERR_APPTAG_MUST            (const xmlChar *)"must-violation"

/* 13.5 */
#define RPC_ERR_APPTAG_INSTANCE_REQ    (const xmlChar *)"instance-required"

/* 13.6 */
#define RPC_ERR_APPTAG_CHOICE          (const xmlChar *)"missing-choice"

/* 13.7 */
#define RPC_ERR_APPTAG_INSERT          (const xmlChar *)"missing-instance"


#define RPC_ERR_APPTAG_RANGE           (const xmlChar *)"not-in-range"

#define RPC_ERR_APPTAG_VALUE_SET       (const xmlChar *)"not-in-value-set"

#define RPC_ERR_APPTAG_PATTERN         (const xmlChar *)"pattern-test-failed"

#define RPC_ERR_APPTAG_DATA_REST   \
    (const xmlChar *)"data-restriction-violation"

#define RPC_ERR_APPTAG_DATA_NOT_UNIQUE \
    (const xmlChar *)"data-not-unique"

/* ietf-netconf-state:get-schema */
/* ietf-netconf-partial-lock:partial-lock */
#define RPC_ERR_APPTAG_NO_MATCHES     (const xmlChar *)"no-matches"

/* ietf-netconf-partial-lock:partial-lock */
#define RPC_ERR_APPTAG_NOT_NODESET      (const xmlChar *)"not-a-node-set"

/* ietf-netconf-partial-lock:partial-lock */
#define RPC_ERR_APPTAG_LOCKED           (const xmlChar *)"locked"

/* ietf-netconf-partial-lock:partial-lock */
#define RPC_ERR_APPTAG_COMMIT           (const xmlChar *)\
    "outstanding-confirmed-commit"


/*** netconfcentral errror-app-tag values ***/
#define RPC_ERR_APPTAG_GEN_WARNING      (const xmlChar *)"general-warning"
#define RPC_ERR_APPTAG_GEN_ERROR        (const xmlChar *)"general-error"
#define RPC_ERR_APPTAG_INT_ERROR        (const xmlChar *)"internal-error"
#define RPC_ERR_APPTAG_IO_ERROR         (const xmlChar *)"io-error"
#define RPC_ERR_APPTAG_MALLOC_ERROR     (const xmlChar *)"malloc-error"
#define RPC_ERR_APPTAG_LIMIT_REACHED    (const xmlChar *)"limit-reached"
#define RPC_ERR_APPTAG_LIBXML2_ERROR    (const xmlChar *)"libxml2-error"
#define RPC_ERR_APPTAG_SQL_ERROR        (const xmlChar *)"sql-error"
#define RPC_ERR_APPTAG_SSH_ERROR        (const xmlChar *)"ssh-error"
#define RPC_ERR_APPTAG_BEEP_ERROR       (const xmlChar *)"beep-error"
#define RPC_ERR_APPTAG_HTML_ERROR       (const xmlChar *)"html-error"
#define RPC_ERR_APPTAG_DATA_INCOMPLETE  (const xmlChar *)"data-incomplete"
#define RPC_ERR_APPTAG_DATA_INVALID     (const xmlChar *)"data-invalid"
#define RPC_ERR_APPTAG_DUPLICATE_ERROR  (const xmlChar *)"duplicate-error"
#define RPC_ERR_APPTAG_RESOURCE_IN_USE  (const xmlChar *)"resource-in-use"
#define RPC_ERR_APPTAG_NO_ACCESS        (const xmlChar *)"no-access"
#define RPC_ERR_APPTAG_RECOVER_FAILED   (const xmlChar *)"recover-failed"
#define RPC_ERR_APPTAG_NO_SUPPORT       (const xmlChar *)"no-support"

#define RPC_ERR_LAST_ERROR RPC_ERR_MALFORMED_MESSAGE

/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/

/* enumerations for NETCONF standard errors */
typedef enum rpc_err_t_ {
    RPC_ERR_NONE,
    RPC_ERR_IN_USE,
    RPC_ERR_INVALID_VALUE,
    RPC_ERR_TOO_BIG,
    RPC_ERR_MISSING_ATTRIBUTE,
    RPC_ERR_BAD_ATTRIBUTE,
    RPC_ERR_UNKNOWN_ATTRIBUTE,
    RPC_ERR_MISSING_ELEMENT,
    RPC_ERR_BAD_ELEMENT,
    RPC_ERR_UNKNOWN_ELEMENT,
    RPC_ERR_UNKNOWN_NAMESPACE,
    RPC_ERR_ACCESS_DENIED,
    RPC_ERR_LOCK_DENIED,
    RPC_ERR_RESOURCE_DENIED,
    RPC_ERR_ROLLBACK_FAILED,
    RPC_ERR_DATA_EXISTS,
    RPC_ERR_DATA_MISSING,
    RPC_ERR_OPERATION_NOT_SUPPORTED,
    RPC_ERR_OPERATION_FAILED,
    RPC_ERR_PARTIAL_OPERATION,    /* deprecated; not used */
    RPC_ERR_MALFORMED_MESSAGE
} rpc_err_t;


/* enumerations for NETCONF standard error severities */
typedef enum rpc_err_sev_t_ {
    RPC_ERR_SEV_NONE,
    RPC_ERR_SEV_WARNING,
    RPC_ERR_SEV_ERROR
} rpc_err_sev_t;


/* one error-info sub-element */
typedef struct rpc_err_info_ {
    dlq_hdr_t          qhdr;
    xmlns_id_t         name_nsid;
    const xmlChar     *name;
    xmlChar           *dname;
    boolean            isqname;     /* val_nsid + v.strval == QName */
    ncx_btype_t        val_btype;
    uint8              val_digits;       /* used for decimal64 only */
    xmlns_id_t         val_nsid;
    xmlChar           *badns;      /* if val_nsid INVALID namespace */
    xmlChar           *dval;
    union {
	const xmlChar     *strval;     /* for string error content */
	ncx_num_t          numval;     /* for number error content */
	void              *cpxval;     /* val_value_t */
    } v;
} rpc_err_info_t;


typedef struct rpc_err_rec_t_ {
    dlq_hdr_t          error_qhdr;
    status_t           error_res;
    ncx_layer_t        error_type;
    rpc_err_t          error_id;
    rpc_err_sev_t      error_severity;
    const xmlChar     *error_tag;
    const xmlChar     *error_app_tag;
    xmlChar           *error_path;
    xmlChar           *error_message;
    const xmlChar     *error_message_lang;
    dlq_hdr_t          error_info;     /* Q of rpc_err_info_t */
} rpc_err_rec_t;


/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION rpc_err_get_errtag
*
* Get the RPC error-tag for an rpc_err_t enumeration
*
* INPUTS:
*   errid == rpc error enum to convert to a string
*
* RETURNS:
*   string for the specified error-tag enum
*********************************************************************/
extern const xmlChar *
    rpc_err_get_errtag (rpc_err_t errid);


/********************************************************************
* FUNCTION rpc_err_get_errtag_enum
*
* Get the RPC error-tag enum for an error-tag string
*
* INPUTS:
*   errtag == error-tag string to check
*
* RETURNS:
*   enum for this error-tag
*********************************************************************/
extern rpc_err_t
    rpc_err_get_errtag_enum (const xmlChar *errtag);


/********************************************************************
* FUNCTION rpc_err_new_record
*
* Malloc and init an rpc_err_rec_t struct
*
* RETURNS:
*   malloced error record or NULL if memory error
*********************************************************************/
extern rpc_err_rec_t *
    rpc_err_new_record (void);


/********************************************************************
* FUNCTION rpc_err_init_record
*
* Init an rpc_err_rec_t struct
*
* INPUTS:
*   err == rpc_err_rec_t struct to init
* RETURNS:
*   none
*********************************************************************/
extern void
    rpc_err_init_record (rpc_err_rec_t *err);


/********************************************************************
* FUNCTION rpc_err_free_record
*
* Clean and free an rpc_err_rec_t struct
*
* INPUTS:
*   err == rpc_err_rec_t struct to clean and free
* RETURNS:
*   none
*********************************************************************/
extern void
    rpc_err_free_record (rpc_err_rec_t *err);


/********************************************************************
* FUNCTION rpc_err_clean_record
*
* Clean an rpc_err_rec_t struct
*
* INPUTS:
*   err == rpc_err_rec_t struct to clean
* RETURNS:
*   none
*********************************************************************/
extern void
    rpc_err_clean_record (rpc_err_rec_t *err);


/********************************************************************
* FUNCTION rpc_err_new_info
*
* Malloc and init an rpc_err_info_t struct
*
* RETURNS:
*   malloced error-info record, or NULL if memory error
*********************************************************************/
extern rpc_err_info_t *
    rpc_err_new_info (void);


/********************************************************************
* FUNCTION rpc_err_free_info
*
* Clean and free an rpc_err_info_t struct
*
* INPUTS:
*   errinfo == rpc_err_info_t struct to clean and free
* RETURNS:
*   none
*********************************************************************/
extern void
    rpc_err_free_info (rpc_err_info_t *errinfo);


/********************************************************************
* FUNCTION rpc_err_dump_errors
*
* Dump the error messages in the RPC message error Q
*
* INPUTS:
*   msg == rpc_msg_t struct to check for errors
*
*********************************************************************/
extern void
    rpc_err_dump_errors (const rpc_msg_t  *msg);


/********************************************************************
* FUNCTION rpc_err_get_severity
*
* Translate an rpc_err_sev_t to a string 
*
* INPUTS:
*   sev == rpc_err_sev_t enum to translate
*
* RETURNS:
* const pointer to the enum string
*********************************************************************/
extern const xmlChar *
    rpc_err_get_severity (rpc_err_sev_t  sev);


/********************************************************************
* FUNCTION rpc_err_clean_errQ
*
* Clean all the entries from a Q of rpc_err_rec_t
*
* INPUTS:
*   errQ == Q of rpc_err_rec_t to clean
* RETURNS:
*   none
*********************************************************************/
extern void 
    rpc_err_clean_errQ (dlq_hdr_t *errQ);


/********************************************************************
* FUNCTION rpc_err_any_errors
*
* Check if there are any errors in the RPC message error Q
*
* INPUTS:
*   msg == rpc_msg_t struct to check for errors
*
* RETURNS:
*   TRUE if any errors recorded; FALSE if none
*********************************************************************/
extern boolean
    rpc_err_any_errors (const rpc_msg_t  *msg);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_rpc_err */
