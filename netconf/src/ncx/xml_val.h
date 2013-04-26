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
#ifndef _H_xml_val
#define _H_xml_val

/*  FILE: xml_val.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Utility functions for creating value structs
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-nov-06    abb      Begun; split from xsd.c
16-jan-07    abb      Moved from ncxdump/xml_val_util.h
*/

#include <xmlstring.h>

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xml_val_make_qname
* 
*   Build output value: add child node to a struct node
*   Malloc a string buffer and create a QName string
*   This is complete; The m__free function must be called
*   with the return value if it is non-NULL;
*
* INPUTS:
*    nsid == namespace ID to use
*    name == condition clause (may be NULL)
*
* RETURNS:
*   malloced value string or NULL if malloc error
*********************************************************************/
extern xmlChar *
    xml_val_make_qname (xmlns_id_t  nsid,
			const xmlChar *name);


/********************************************************************
* FUNCTION xml_val_qname_len
* 
*   Determine the length of the qname string that would be generated
*   with the xml_val_make_qname function
*
* INPUTS:
*    nsid == namespace ID to use
*    name == condition clause (may be NULL)
*
* RETURNS:
*   length of string needed for this QName
*********************************************************************/
extern uint32
    xml_val_qname_len (xmlns_id_t  nsid,
		       const xmlChar *name);


/********************************************************************
* FUNCTION xml_val_sprintf_qname
* 
*   construct a QName into a buffer
*
* INPUTS:
*    buff == buffer
*    bufflen == size of buffer
*    nsid == namespace ID to use
*    name == condition clause (may be NULL)
*
* RETURNS:
*   number of bytes written to the buffer
*********************************************************************/
extern uint32
    xml_val_sprintf_qname (xmlChar *buff,
			   uint32 bufflen,
			   xmlns_id_t  nsid,
			   const xmlChar *name);


/********************************************************************
* FUNCTION xml_val_add_attr
* 
*   Set up a new attr val and add it to the specified val
*   hand off a malloced attribute string
*
* INPUTS:
*    name == attr name
*    nsid == namespace ID of attr
*    attrval  == attr val to add (do not use strdup)
*    val == parent val struct to hold the new attr
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_val_add_attr (const xmlChar *name,
		      xmlns_id_t nsid,
		      xmlChar *attrval,
		      val_value_t *val);


/********************************************************************
* FUNCTION xml_val_add_cattr
* 
*   Set up a new const attr val and add it to the specified val
*   copy a const attribute string
*
* INPUTS:
*    name == attr name
*    nsid == namespace ID of attr
*    cattrval  == const attr val to add (use strdup)
*    val == parent val struct to hold the new attr
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_val_add_cattr (const xmlChar *name,
		       xmlns_id_t nsid,
		       const xmlChar *cattrval,
		       val_value_t *val);


/********************************************************************
* FUNCTION xml_val_new_struct
* 
*   Set up a new struct
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
* 
* RETURNS:
*   new struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_struct (const xmlChar *name,
			xmlns_id_t     nsid);


/********************************************************************
* FUNCTION xml_val_new_string
* 
*  Set up a new string element; reuse the value instead of copying it
*  hand off a malloced string
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
*    strval == malloced string value that will be freed later
* 
* RETURNS:
*   new string or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_string (const xmlChar *name,
			xmlns_id_t     nsid,
			xmlChar *strval);


/********************************************************************
* FUNCTION xml_val_new_cstring
* 
*   Set up a new string from a const string
*   copy a const string
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
*    strval == const string value that will strduped first
* 
* RETURNS:
*   new string or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_cstring (const xmlChar *name,
			 xmlns_id_t     nsid,
			 const xmlChar *strval);


/********************************************************************
* FUNCTION xml_val_new_flag
* 
*   Set up a new flag
*   This is not complete; more nodes will be added
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
* 
* RETURNS:
*   new struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_flag (const xmlChar *name,
		      xmlns_id_t     nsid);


/********************************************************************
* FUNCTION xml_val_new_boolean
* 
*   Set up a new boolean
*   This is not complete; more nodes will be added
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
*    boo == boolean value to set
*
* RETURNS:
*   new struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_boolean (const xmlChar *name,
                         xmlns_id_t     nsid,
                         boolean boo);


/********************************************************************
* FUNCTION xml_val_new_number
* 
*   Set up a new number
*
* INPUTS:
*    name == element name
*    nsid == namespace ID of name
*    num == number value to set
*    btyp == base type of 'num'
*
* RETURNS:
*   new struct or NULL if malloc error
*********************************************************************/
extern val_value_t *
    xml_val_new_number (const xmlChar *name,
                        xmlns_id_t     nsid,
                        ncx_num_t *num,
                        ncx_btype_t btyp);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xml_val */
