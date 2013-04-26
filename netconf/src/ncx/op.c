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
/*  FILE: op.c

   NETCONF Protocol Operations
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
02may05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <memory.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_op
#include  "op.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_xmlns
#include  "xmlns.h"
#endif

#ifndef _H_xml_util
#include  "xml_util.h"
#endif

#ifndef _H_yangconst
#include  "yangconst.h"
#endif


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
const xmlChar * 
    op_method_name (op_method_t op_id) 
{
    switch (op_id) {
    case OP_NO_METHOD:
        return NCX_EL_NONE;
    case OP_GET_CONFIG:
        return NCX_EL_GET_CONFIG;
    case OP_EDIT_CONFIG:
        return NCX_EL_EDIT_CONFIG;
    case OP_COPY_CONFIG:
        return NCX_EL_COPY_CONFIG;
    case OP_DELETE_CONFIG:
        return NCX_EL_DELETE_CONFIG;
    case OP_LOCK:
        return NCX_EL_LOCK;
    case OP_UNLOCK:
        return NCX_EL_UNLOCK;
    case OP_GET:
        return NCX_EL_GET;
    case OP_CLOSE_SESSION:
        return NCX_EL_CLOSE_SESSION;
    case OP_KILL_SESSION:
        return NCX_EL_KILL_SESSION;
    case OP_COMMIT:
        return NCX_EL_COMMIT;
    case OP_DISCARD_CHANGES:
        return NCX_EL_DISCARD_CHANGES;
    case OP_VALIDATE:
        return NCX_EL_VALIDATE;
    case OP_CANCEL_COMMIT:
        return NCX_EL_CANCEL_COMMIT;
    default:
        return NCX_EL_ILLEGAL;
    }
}  /* op_method_name */


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
const xmlChar * 
    op_editop_name (op_editop_t ed_id) 
{
    switch (ed_id) {
    case OP_EDITOP_NONE:
        return NCX_EL_NONE;
    case OP_EDITOP_MERGE:
        return NCX_EL_MERGE;
    case OP_EDITOP_REPLACE:
        return NCX_EL_REPLACE;
    case OP_EDITOP_CREATE:
        return NCX_EL_CREATE;
    case OP_EDITOP_DELETE:
        return NCX_EL_DELETE;
    case OP_EDITOP_LOAD:
        return NCX_EL_LOAD;
    case OP_EDITOP_COMMIT:
        return NCX_EL_COMMIT;
    case OP_EDITOP_REMOVE:
        return NCX_EL_REMOVE;
    default:
        return (const xmlChar *) "illegal";
    }
} /* op_editop_name */


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
op_editop_t 
    op_editop_id (const xmlChar *opstr)
{
#ifdef DEBUG
    if (!opstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_EDITOP_NONE;
    }
#endif

    if (!xml_strcmp(opstr, NCX_EL_MERGE)) {
        return OP_EDITOP_MERGE;
    }
    if (!xml_strcmp(opstr, NCX_EL_REPLACE)) {
        return OP_EDITOP_REPLACE;
    }
    if (!xml_strcmp(opstr, NCX_EL_CREATE)) {
        return OP_EDITOP_CREATE;
    }
    if (!xml_strcmp(opstr, NCX_EL_DELETE)) {
        return OP_EDITOP_DELETE;
    }
    if (!xml_strcmp(opstr, NCX_EL_REMOVE)) {
        return OP_EDITOP_REMOVE;
    }
    if (!xml_strcmp(opstr, NCX_EL_NONE)) {
        return OP_EDITOP_NONE;
    }

    /* internal extensions, should not be used in NETCONF PDU */
    if (!xml_strcmp(opstr, NCX_EL_LOAD)) {
        return OP_EDITOP_LOAD;
    }
    if (!xml_strcmp(opstr, NCX_EL_COMMIT)) {
        return OP_EDITOP_COMMIT;
    }

    return OP_EDITOP_NONE;

} /* op_editop_id */


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
const xmlChar * 
    op_insertop_name (op_insertop_t ins_id) 
{
    switch (ins_id) {
    case OP_INSOP_NONE:
        return NCX_EL_NONE;
    case OP_INSOP_FIRST:
        return YANG_K_FIRST;
    case OP_INSOP_LAST:
        return YANG_K_LAST;
    case OP_INSOP_BEFORE:
        return YANG_K_BEFORE;
    case OP_INSOP_AFTER:
        return YANG_K_AFTER;
    default:
        return (const xmlChar *) "illegal";
    }
} /* op_insertop_name */


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
op_insertop_t 
    op_insertop_id (const xmlChar *opstr)
{
#ifdef DEBUG
    if (!opstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_INSOP_NONE;
    }
#endif

    if (!xml_strcmp(opstr, YANG_K_FIRST)) {
        return OP_INSOP_FIRST;
    }
    if (!xml_strcmp(opstr, YANG_K_LAST)) {
        return OP_INSOP_LAST;
    }
    if (!xml_strcmp(opstr, YANG_K_BEFORE)) {
        return OP_INSOP_BEFORE;
    }
    if (!xml_strcmp(opstr, YANG_K_AFTER)) {
        return OP_INSOP_AFTER;
    }
    return OP_INSOP_NONE;

} /* op_insertop_id */


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
op_filtertyp_t 
    op_filtertyp_id (const xmlChar *filstr)
{
#ifdef DEBUG
    if (!filstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_FILTER_NONE;
    }
#endif

    if (!xml_strcmp(filstr, NCX_EL_SUBTREE)) {
        return OP_FILTER_SUBTREE;
    }
    if (!xml_strcmp(filstr, NCX_EL_XPATH)) {
        return OP_FILTER_XPATH;
    }

    return OP_FILTER_NONE;

} /* op_filtertyp_id */


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
const xmlChar * 
    op_defop_name (op_defop_t def_id) 
{
    switch (def_id) {
    case OP_DEFOP_NOT_SET:
        return NCX_EL_NOT_SET;
    case OP_DEFOP_NONE:
        return NCX_EL_NONE;
    case OP_DEFOP_MERGE:
        return NCX_EL_MERGE;
    case OP_DEFOP_REPLACE:
        return NCX_EL_REPLACE;
    case OP_DEFOP_NOT_USED:
        return NCX_EL_NOT_USED;
    default:
        return NCX_EL_ILLEGAL;
    }
} /* op_defop_name */


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
op_editop_t 
    op_defop_id (const xmlChar *defstr)
{
#ifdef DEBUG
    if (!defstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_EDITOP_NONE;
    }
#endif

    if (!xml_strcmp(defstr, NCX_EL_MERGE)) {
        return OP_EDITOP_MERGE;
    }
    if (!xml_strcmp(defstr, NCX_EL_REPLACE)) {
        return OP_EDITOP_REPLACE;
    }
    if (!xml_strcmp(defstr, NCX_EL_NONE)) {
        return OP_EDITOP_NONE;
    }
    return OP_EDITOP_NONE;

} /* op_defop_id */


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
const xmlChar * 
    op_testop_name (op_testop_t test_id) 
{
    switch (test_id) {
    case OP_TESTOP_NONE:
        return NCX_EL_NONE;
    case OP_TESTOP_TESTTHENSET:
        return NCX_EL_TESTTHENSET;
    case OP_TESTOP_SET:
        return NCX_EL_SET;
    case OP_TESTOP_TESTONLY:
        return NCX_EL_TESTONLY;
    default:
        return NCX_EL_ILLEGAL;
    }
} /* op_testop_name */


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
op_testop_t
    op_testop_enum (const xmlChar *teststr)
{
    if (!xml_strcmp(teststr, NCX_EL_TESTONLY)) {
        return OP_TESTOP_TESTONLY;
    } else if (!xml_strcmp(teststr, NCX_EL_TESTTHENSET)) {
        return OP_TESTOP_TESTTHENSET;
    } else if (!xml_strcmp(teststr, NCX_EL_SET)) {
        return OP_TESTOP_SET;
    } else {
        return OP_TESTOP_NONE;
    }
} /* op_testop_enum */


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
const xmlChar * 
    op_errop_name (op_errop_t err_id) 
{
    switch (err_id) {
    case OP_ERROP_NONE:
        return NCX_EL_NONE;
    case OP_ERROP_STOP:
        return NCX_EL_STOP_ON_ERROR;
    case OP_ERROP_CONTINUE:
        return NCX_EL_CONTINUE_ON_ERROR;
    case OP_ERROP_ROLLBACK:
        return NCX_EL_ROLLBACK_ON_ERROR;
    default:
        return NCX_EL_ILLEGAL;
    }
} /* op_errop_name */


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
op_errop_t 
    op_errop_id (const xmlChar *errstr)
{
#ifdef DEBUG
    if (!errstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_ERROP_NONE;
    }
#endif

    if (!xml_strcmp(errstr, NCX_EL_STOP_ON_ERROR)) {
        return OP_ERROP_STOP;
    }
    if (!xml_strcmp(errstr, NCX_EL_CONTINUE_ON_ERROR)) {
        return OP_ERROP_CONTINUE;
    }
    if (!xml_strcmp(errstr, NCX_EL_ROLLBACK_ON_ERROR)) {
        return OP_ERROP_ROLLBACK;
    }
    return OP_ERROP_NONE;

} /* op_errop_id */


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
op_defop_t 
    op_defop_id2 (const xmlChar *defstr)
{
#ifdef DEBUG
    if (!defstr) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return OP_DEFOP_NOT_SET;
    }
#endif

    if (!xml_strcmp(defstr, NCX_EL_NONE)) {
        return OP_DEFOP_NONE;
    }
    if (!xml_strcmp(defstr, NCX_EL_MERGE)) {
        return OP_DEFOP_MERGE;
    }
    if (!xml_strcmp(defstr, NCX_EL_REPLACE)) {
        return OP_DEFOP_REPLACE;
    }
    if (!xml_strcmp(defstr, NCX_EL_NOT_USED)) {
        return OP_DEFOP_NOT_USED;
    }
    return OP_DEFOP_NOT_SET;

} /* op_defop_id2 */


/* END file op.c */
