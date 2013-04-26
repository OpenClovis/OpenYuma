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
#ifndef _H_op
#define _H_op
/*  FILE: op.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

    NETCONF protocol operations

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
06-apr-05    abb      Begun.
*/

#include <xmlstring.h>

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/



/********************************************************************
*                                                                   *
*                                    T Y P E S                      *
*                                                                   *
*********************************************************************/

/* NETCONF protocol operation enumeration is actually
 * an RPC method in the NECONF namespace
 */
typedef enum op_method_t_ {
    OP_NO_METHOD,
    OP_GET_CONFIG,            /* base protocol operations */
    OP_EDIT_CONFIG,
    OP_COPY_CONFIG,
    OP_DELETE_CONFIG,
    OP_LOCK,
    OP_UNLOCK,
    OP_GET,
    OP_CLOSE_SESSION,
    OP_KILL_SESSION,
    OP_COMMIT,                   /* #candidate capability */
    OP_DISCARD_CHANGES,          /* #candidate capability */
    OP_VALIDATE,                  /* #validate capability */
    OP_CANCEL_COMMIT            /* base:1.1 + conf-commit */
} op_method_t;


/* NETCONF protocol operation PDU source types */
typedef enum op_srctyp_t_ {
    OP_SOURCE_NONE,
    OP_SOURCE_CONFIG,  
    OP_SOURCE_INLINE,
    OP_SOURCE_URL
} op_srctyp_t;


/* NETCONF protocol operation PDU target types */
typedef enum op_targtyp_t_ {
    OP_TARGET_NONE,
    OP_TARGET_CONFIG,  
    OP_TARGET_URL
} op_targtyp_t;


/* NETCONF protocol default edit-config operation types */
typedef enum op_defop_t_ {
    OP_DEFOP_NOT_SET,
    OP_DEFOP_NONE,
    OP_DEFOP_MERGE,
    OP_DEFOP_REPLACE,
    OP_DEFOP_NOT_USED
} op_defop_t;


/* NETCONF protocol operation PDU filter types */
typedef enum op_filtertyp_t_ {
    OP_FILTER_NONE,
    OP_FILTER_SUBTREE,
    OP_FILTER_XPATH
} op_filtertyp_t;


/* NETCONF edit-config operation types */
typedef enum op_editop_t_ {
    OP_EDITOP_NONE,
    OP_EDITOP_MERGE,
    OP_EDITOP_REPLACE,
    OP_EDITOP_CREATE,
    OP_EDITOP_DELETE,
    OP_EDITOP_LOAD,         /* internal enumeration */
    OP_EDITOP_COMMIT,
    OP_EDITOP_REMOVE               /* base:1.1 only */
} op_editop_t;


/* YANG insert operation types */
typedef enum op_insertop_t_ {
    OP_INSOP_NONE,
    OP_INSOP_FIRST,
    OP_INSOP_LAST,
    OP_INSOP_BEFORE,
    OP_INSOP_AFTER
} op_insertop_t;


/* NETCONF full operation list for access control */
typedef enum op_t_ {
    OP_NONE,
    OP_MERGE,
    OP_REPLACE,
    OP_CREATE,
    OP_DELETE,
    OP_LOAD,
    OP_NOTIFY,
    OP_READ
} op_t;


/* NETCONF edit-config test-option types */
typedef enum op_testop_t_ {
    OP_TESTOP_NONE,
    OP_TESTOP_TESTTHENSET,
    OP_TESTOP_SET,
    OP_TESTOP_TESTONLY
} op_testop_t;


/* NETCONF edit-config error-option types */
typedef enum op_errop_t_ {
    OP_ERROP_NONE,
    OP_ERROP_STOP,
    OP_ERROP_CONTINUE,
    OP_ERROP_ROLLBACK
} op_errop_t;


/* NETCONF protocol operation filter spec */
typedef struct op_filter_t_ {
    op_filtertyp_t       op_filtyp;    
    struct val_value_t_ *op_filter;  /* back-ptr to the filter value */
} op_filter_t;

 
/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION op_method_name
*
* Get the keyword for the specified STD RPC method
*
* INPUTS:  
*    op_id == proto op ID
* RETURNS:
*    string for the operation, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_method_name (op_method_t op_id);


/********************************************************************
* FUNCTION op_editop_name
*
* Get the keyword for the specified op_editop_t enumeration
*
* INPUTS:  
*    ed_id == edit operation ID
* RETURNS:
*    string for the edit operation type, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_editop_name (op_editop_t ed_id);


/********************************************************************
* FUNCTION op_editop_id
*
* Get the ID for the editop from its keyword
*
* INPUTS:  
*    opstr == string for the edit operation type
* RETURNS:
*    the op_editop_t enumeration value for the string
*********************************************************************/
extern op_editop_t 
    op_editop_id (const xmlChar *opstr);


/********************************************************************
* FUNCTION op_insertop_name
*
* Get the keyword for the specified op_insertop_t enumeration
*
* INPUTS:  
*    ed_id == insert operation ID
*
* RETURNS:
*    string for the insert operation type, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_insertop_name (op_insertop_t ins_id);


/********************************************************************
* FUNCTION op_insertop_id
*
* Get the ID for the insert operation from its keyword
*
* INPUTS:  
*    opstr == string for the insert operation type
* RETURNS:
*    the op_insertop_t enumeration value for the string
*********************************************************************/
extern op_insertop_t 
    op_insertop_id (const xmlChar *opstr);


/********************************************************************
* FUNCTION op_filtertyp_id
*
* Get the ID for the filter type from its keyword
*
* INPUTS:  
*    filstr == string for the filter type
* RETURNS:
*    the op_filtertyp_t enumeration value for the string
*********************************************************************/
extern op_filtertyp_t 
    op_filtertyp_id (const xmlChar *filstr);


/********************************************************************
* FUNCTION op_defop_name
*
* Get the keyword for the specified op_defop_t enumeration
*
* INPUTS:  
*    def_id == default operation ID
* RETURNS:
*    string for the default operation type, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_defop_name (op_defop_t def_id);


/********************************************************************
* FUNCTION op_defop_id
*
* Get the ID for the default-operation from its keyword
*
* INPUTS:  
*    defstr == string for the default operation
* RETURNS:
*    the op_editop_t enumeration value for the string
*********************************************************************/
extern op_editop_t 
    op_defop_id (const xmlChar *defstr);


/********************************************************************
* FUNCTION op_testop_name
*
* Get the keyword for the specified op_testop_t enumeration
*
* INPUTS:  
*    test_id == test operation ID
* RETURNS:
*    string for the test operation type, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_testop_name (op_testop_t test_id);


/********************************************************************
* FUNCTION op_testop_enum
*
* Get the enum for the specified op_testop_t string
*
* INPUTS:  
*    teststr == string for the test operation type
* RETURNS:
*    test_id == test operation ID
*    
*********************************************************************/
extern op_testop_t
    op_testop_enum (const xmlChar *teststr);


/********************************************************************
* FUNCTION op_errop_name
*
* Get the keyword for the specified op_errop_t enumeration
*
* INPUTS:  
*    err_id == error operation ID
* RETURNS:
*    string for the error operation type, or "none" or "illegal"
*********************************************************************/
extern const xmlChar * 
    op_errop_name (op_errop_t err_id);


/********************************************************************
* FUNCTION op_errop_id
*
* Get the ID for the error-option from its keyword
*
* INPUTS:  
*    errstr == string for the error option
* RETURNS:
*    the op_errop_t enumeration value for the string
*********************************************************************/
extern op_errop_t 
    op_errop_id (const xmlChar *errstr);


/********************************************************************
* FUNCTION op_defop_id2
*
* Get the ID for the default-operation from its keyword
* Return the op_defop_t, not the op_editop_t conversion
*
* INPUTS:  
*    defstr == string for the default operation
* RETURNS:
*    the op_defop_t enumeration value for the string
*********************************************************************/
extern op_defop_t 
    op_defop_id2 (const xmlChar *defstr);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_op */
