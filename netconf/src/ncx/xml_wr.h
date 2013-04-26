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
#ifndef _H_xml_wr
#define _H_xml_wr

/*  FILE: xml_wr.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    XML Write functions

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
12-feb-07    abb      Begun; split out from xml_wr_util.c

*/

#include <stdio.h>
#include <xmlstring.h>

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

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

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* isattrq labels */
#define ATTRQ   TRUE
#define METAQ   FALSE

/* empty labels */
#define START  FALSE
#define EMPTY  TRUE

/* docmode labels */
#define XMLMODE FALSE
#define DOCMODE TRUE

/* xmlhdr labels */
#define NOHDR FALSE
#define WITHHDR TRUE

/* buffer size used in begin_elem_val function
 * this is sort of a hack -- a hard limit in the code:
 * this limits the number of different namespace IDs
 * that can be present in a single XPath expression
 * and the xmlns attributes will be corrected generated
 * in element start tags for the XPath expression
 */
#define XML_WR_MAX_NAMESPACES  512


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xml_wr_buff
*
* Write some xmlChars to the specified session
*
* INPUTS:
*   scb == session control block to start msg 
*   buff == buffer to write
*   bufflen == number of bytes to write, not including any
*              EOS char at the end of the buffer
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_buff (ses_cb_t *scb,
		 const xmlChar *buff,
		 uint32 bufflen);


/********************************************************************
* FUNCTION xml_wr_begin_elem_ex
*
* Write a start or empty XML tag to the specified session
*
* INPUTS:
*   scb == session control block
*   msg == top header from message in progress
*   parent_nsid == namespace ID of the parent element, if known
*   nsid == namespace ID of the element to write
*   elname == unqualified name of element to write
*   attrQ == Q of xml_attr_t or val_value_t records to write in
*            the element; NULL == none
*   isattrq == TRUE if the qQ contains xml_attr_t nodes
*              FALSE if the Q contains val_value_t nodes (metadata)
*   indent == number of chars to indent after a newline
*           == -1 means no newline or indent
*           == 0 means just newline
*   empty == TRUE for empty node
*         == FALSE for start node
*
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_begin_elem_ex (ses_cb_t *scb,
			  xml_msg_hdr_t *msg,
			  xmlns_id_t  parent_nsid,
			  xmlns_id_t  nsid,
			  const xmlChar *elname,
			  const dlq_hdr_t *attrQ,
			  boolean isattrq,
			  int32 indent,
			  boolean empty);


/********************************************************************
* FUNCTION xml_wr_begin_elem
*
* Write a start XML tag to the specified session without attributes
*
* INPUTS:
*   scb == session control block
*   msg == top header from message in progress
*   parent_nsid == namespace ID of the parent element
*   nsid == namespace ID of the element to write
*   elname == unqualified name of element to write
*   indent == number of chars to indent after a newline
*           == -1 means no newline or indent
*           == 0 means just newline
*
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_begin_elem (ses_cb_t *scb,
		       xml_msg_hdr_t *msg,
		       xmlns_id_t  parent_nsid,
		       xmlns_id_t  nsid,
		       const xmlChar *elname,
		       int32 indent);


/********************************************************************
* FUNCTION xml_wr_empty_elem
*
* Write an empty XML tag to the specified session without attributes
*
* INPUTS:
*   scb == session control block
*   msg == top header from message in progress
*   parent_nsid == namespace ID of the parent element
*   nsid == namespace ID of the element to write
*   elname == unqualified name of element to write
*   indent == number of chars to indent after a newline
*           == -1 means no newline or indent
*           == 0 means just newline
*
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_empty_elem (ses_cb_t *scb,
		       xml_msg_hdr_t *msg,
		       xmlns_id_t  parent_nsid,
		       xmlns_id_t  nsid,
		       const xmlChar *elname,
		       int32 indent);


/********************************************************************
* FUNCTION xml_wr_end_elem
*
* Write an end tag to the specified session
*
* INPUTS:
*   scb == session control block to start msg 
*   msg == header from message in progress
*   nsid == namespace ID of the element to write
*        == zero to force no prefix lookup; use default NS
*   elname == unqualified name of element to write
*   indent == number of chars to indent after a newline
*             will be ignored if indent is turned off
*             in the agent profile
*           == -1 means no newline or indent
*           == 0 means just newline
*
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_end_elem (ses_cb_t *scb,
		     xml_msg_hdr_t *msg,
		     xmlns_id_t  nsid,
		     const xmlChar *elname,
		     int32 indent);


/********************************************************************
* FUNCTION xml_wr_string_elem
*
* Write a start tag, simple string content, and an end tag
* to the specified session.  A flag element and
* ename will vary from this format.
*
* Simple content nodes are completed on a single line to
* prevent introduction of extra whitespace
*
* INPUTS:
*   scb == session control block
*   msg == header from message in progress
*   str == simple string to write as element content
*   parent_nsid == namespace ID of the parent element
*   nsid == namespace ID of the element to write
*   elname == unqualified name of element to write
*   attrQ == Q of xml_attr_t records to write in
*            the element; NULL == none
*   isattrq == TRUE for Q of xml_attr_t, FALSE for val_value_t
*   indent == number of chars to indent after a newline
*           == -1 means no newline or indent
*           == 0 means just newline
*  
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_string_elem (ses_cb_t *scb,
			xml_msg_hdr_t *msg,
			const xmlChar *str,
			xmlns_id_t  parent_nsid,
			xmlns_id_t  nsid,
			const xmlChar *elname,
			const dlq_hdr_t *attrQ,
			boolean isattrq,
			int32 indent);


/********************************************************************
* FUNCTION xml_wr_qname_elem
*
* Write a start tag, QName string content, and an end tag
* to the specified session.
*
* The ses_start_msg must be called before this
* function, in order for it to allow any writes
*
* INPUTS:
*   scb == session control block
*   msg == header from message in progres
*   val_nsid == namespace ID of the QName prefix
*   str == local-name part of the QName
*   parent_nsid == namespace ID of the parent element
*   nsid == namespace ID of the element to write
*   elname == unqualified name of element to write
*   attrQ == Q of xml_attr_t records to write in
*            the element; NULL == none
*   isattrq == TRUE for Q of xml_attr_t, FALSE for val_value_t
*   indent == number of chars to indent after a newline
*           == -1 means no newline or indent
*           == 0 means just newline
*   isdefault == TRUE if the XML value node represents a default leaf
*             == FALSE otherwise
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_qname_elem (ses_cb_t *scb,
		       xml_msg_hdr_t *msg,
		       xmlns_id_t val_nsid,
		       const xmlChar *str,
		       xmlns_id_t  parent_nsid,
		       xmlns_id_t  nsid,
		       const xmlChar *elname,
		       const dlq_hdr_t *attrQ,
		       boolean isattrq,
		       int32 indent,
                       boolean isdefault);


/********************************************************************
* FUNCTION xml_wr_check_val
* 
* Write an NCX value in XML encoding
* while checking nodes for suppression of output with
* the supplied test fn
*
* !!! NOTE !!!
* 
* This function generates the contents of the val_value_t
* but not the top node itself.  This function is called
* recursively and this is the intended behavior.
*
* To generate XML for an entire val_value_t, including
* the top-level node, use the xml_wr_full_val fn.
*
* If the acm_cache and acm_cbfn fields are set in
* the msg header then access control will be checked
* If FALSE, then nothing will be written to the output session
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write
*   indent == start indent amount if indent enabled
*   testcb == callback function to use, NULL if not used
*   
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_check_val (ses_cb_t *scb,
		      xml_msg_hdr_t *msg,
		      val_value_t *val,
		      int32  indent,
		      val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION xml_wr_val
* 
* output val_value_t node contents only
* Write an NCX value node in XML encoding
* See xml_wr_check_write for full details of this fn.
* It is the same, except a NULL testfn is supplied.
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write
*   indent == start indent amount if indent enabled
*   
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_val (ses_cb_t *scb,
		xml_msg_hdr_t *msg,
		val_value_t *val,
		int32 indent);


/********************************************************************
* FUNCTION xml_wr_full_check_val
* 
* generate entire val_value_t *w/filter)
* Write an entire val_value_t out as XML, including the top level
* Using an optional testfn to filter output
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write
*   indent == start indent amount if indent enabled
*   testcb == callback function to use, NULL if not used
*   
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_full_check_val (ses_cb_t *scb,
			   xml_msg_hdr_t *msg,
			   val_value_t *val,
			   int32  indent,
			   val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION xml_wr_full_val
* 
* generate entire val_value_t
* Write an entire val_value_t out as XML, including the top level
*
* INPUTS:
*   scb == session control block
*   msg == xml_msg_hdr_t in progress
*   val == value to write
*   indent == start indent amount if indent enabled
*   
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_wr_full_val (ses_cb_t *scb,
		     xml_msg_hdr_t *msg,
		     val_value_t *val,
		     int32  indent);


/********************************************************************
* FUNCTION xml_wr_check_open_file
* 
* Write the specified value to an open FILE in XML format
*
* INPUTS:
*    fp == open FILE control block
*    val == value for output
*    attrs == top-level attributes to generate
*    docmode == TRUE if XML_DOC output mode should be used
*            == FALSE if XML output mode should be used
*    xmlhdr == TRUE if <?xml?> directive should be output
*            == FALSE if not
*    withns == TRUE if xmlns attributes should be used
*              FALSE to leave them out
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*    testfn == callback test function to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    xml_wr_check_open_file (FILE *fp, 
                            val_value_t *val,
                            xml_attrs_t *attrs,
                            boolean docmode,
                            boolean xmlhdr,
                            boolean withns,
                            int32 startindent,
                            int32  indent,
                            val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION xml_wr_check_file
* 
* Write the specified value to a FILE in XML format
*
* INPUTS:
*    filespec == exact path of filename to open
*    val == value for output
*    attrs == top-level attributes to generate
*    docmode == TRUE if XML_DOC output mode should be used
*            == FALSE if XML output mode should be used
*    xmlhdr == TRUE if <?xml?> directive should be output
*            == FALSE if not
*    withns == TRUE if xmlns attributes should be used
*              FALSE to leave them out
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*    testfn == callback test function to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    xml_wr_check_file (const xmlChar *filespec, 
		       val_value_t *val,
		       xml_attrs_t *attrs,
		       boolean docmode,
		       boolean xmlhdr,
                       boolean withns,
                       int32 startindent,
		       int32 indent,
		       val_nodetest_fn_t testfn);


/********************************************************************
* FUNCTION xml_wr_file
* 
* Write the specified value to a FILE in XML format
*
* INPUTS:
*    filespec == exact path of filename to open
*    val == value for output
*    attrs == top-level attributes to generate
*    docmode == TRUE if XML_DOC output mode should be used
*            == FALSE if XML output mode should be used
*    xmlhdr == TRUE if <?xml?> directive should be output
*            == FALSE if not
*    withns == TRUE if xmlns attributes should be used
*              FALSE to leave them out
*    startindent == starting indent point
*    indent == indent amount (0..9 spaces)
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    xml_wr_file (const xmlChar *filespec, 
		 val_value_t *val,
		 xml_attrs_t *attrs,
		 boolean docmode,
		 boolean xmlhdr,
                 boolean withns,
                 int32 startindent,
		 int32 indent);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xml_wr */

