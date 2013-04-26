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
#ifndef _H_json_wr
#define _H_json_wr

/*  FILE: json_wr.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    JSON Write functions

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
01-sep-11    abb      Begun;

*/

#include <stdio.h>
#include <xmlstring.h>

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION json_wr_full_check_val
* 
* generate entire val_value_t *w/filter)
* Write an entire val_value_t out as XML, including the top level
* Using an optional testfn to filter output
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write
*   startindent == start indent amount if indent enabled
*   testcb == callback function to use, NULL if not used
*   
* RETURNS:
*   status
*********************************************************************/
extern status_t
    json_wr_full_check_val (ses_cb_t *scb,
                            xml_msg_hdr_t *msg,
                            val_value_t *val,
                            int32  startindent,
                            val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION json_wr_check_open_file
* 
* Write the specified value to an open FILE in JSON format
*
* INPUTS:
*    fp == open FILE control block
*    val == value for output
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*    testfn == callback test function to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    json_wr_check_open_file (FILE *fp, 
                             val_value_t *val,
                             int32 startindent,
                             int32  indent,
                             val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION json_wr_check_file
* 
* Write the specified value to a FILE in JSON format
*
* INPUTS:
*    filespec == exact path of filename to open
*    val == value for output
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*    testfn == callback test function to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    json_wr_check_file (const xmlChar *filespec, 
                        val_value_t *val,
                        int32 startindent,
                        int32  indent,
                        val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION json_wr_file
* 
* Write the specified value to a FILE in JSON format
*
* INPUTS:
*    filespec == exact path of filename to open
*    val == value for output
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    json_wr_file (const xmlChar *filespec,
                  val_value_t *val,
                  int32 startindent,
                  int32 indent);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_json_wr */

