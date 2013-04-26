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
#ifndef _H_xml_util
#define _H_xml_util
/*  FILE: xml_util.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    General Utilities that simplify usage of the libxml2 functions

    - xml_node_t allocation
      - xml_new_node
      - xml_init_node
      - xml_free_node
      - xml_clean_node

    - XmlReader utilities
      - xml_get_reader_from_filespec  (parse debug test documents)
      - xml_get_reader_for_session
      - xml_reset_reader_for_session
      - xml_free_reader
      - xml_get_node_name   (xmlparser enumeration for node type)
      - xml_advance_reader
      - xml_consume_node
      - xml_consume_start_node
      - xml_consume_end_node
      - xml_node_match
      - xml_endnode_match
      - xml_docdone
      - xml_dump_node (debug printf)

    - XML Attribute utilities
      - xml_init_attrs
      - xml_add_attr
      - xml_first_attr
      - xml_next_attr
      - xml_find_attr
      - xml_clean_attrs

    - XmlChar string utilites
      - xml_strlen
      - xml_strcpy
      - xml_strncpy
      - xml_strdup
      - xml_strcat
      - xml_strncat
      - xml_strndup
      - xml_ch_strndup
      - xml_strcmp
      - xml_strncmp
      - xml_isspace
      - xml_isspace_str
      - xml_copy_clean_string
      - xml_convert_char_entity

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
14-oct-05    abb      begun
2-jan-06     abb      rewrite xml_consume_* API to use simpler 
                      xml_node_t
*/


/* From /usr/include/libxml2/libxml/ */
#include <xmlreader.h>
#include <xmlstring.h>

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define MAX_CHAR_ENT 8

#define XML_START_MSG ((const xmlChar *)\
		       "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")

#define XML_START_MSG_SIZE   38

#define XML_READER_OPTIONS    XML_PARSE_RECOVER+XML_PARSE_NOERROR+\
    XML_PARSE_NOWARNING+XML_PARSE_NOBLANKS+XML_PARSE_NONET+       \
    XML_PARSE_XINCLUDE

#define XML_SES_URL "netconf://pdu"

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* queue of xml_attr_t */
typedef dlq_hdr_t  xml_attrs_t;


/* represents one attribute */
typedef struct xml_attr_t_ {
    dlq_hdr_t      attr_qhdr;
    xmlns_id_t     attr_ns;
    xmlns_id_t     attr_xmlns_ns;
    const xmlChar *attr_qname;
    const xmlChar *attr_name;
    xmlChar       *attr_dname;   /* full qualified name if any */
    xmlChar       *attr_val;
    struct xpath_pcb_t_ *attr_xpcb;
} xml_attr_t;


/* only 4 types of nodes returned */
typedef enum xml_nodetyp_t_ {
    XML_NT_NONE,
    XML_NT_EMPTY,                           /* standalone empty node */
    XML_NT_START,                         /* start-tag of an element */
    XML_NT_END,                             /* end-tag of an element */
    XML_NT_STRING                             /* string content node */
} xml_nodetyp_t;

    
/* gather node data into a simple struct 
 * If a simple value is present, the the simval pointer will
 * be non-NULL and point at the value string after it has
 * been trimmed and any character entities translated
 */
typedef struct xml_node_t_ {
    xml_nodetyp_t  nodetyp;
    xmlns_id_t     nsid;
    xmlns_id_t     contentnsid;
    const xmlChar *module;
    xmlChar       *qname;
    const xmlChar *elname;
    const xmlChar *simval;               /* may be cleaned val */
    uint32         simlen;
    xmlChar       *simfree;     /* non-NULL if simval is freed */
    int            depth;
    xml_attrs_t    attrs;
} xml_node_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION xml_new_node
* 
* Malloc and init a new xml_node_t struct
*
* RETURNS:
*   pointer to new node or NULL if malloc error
*********************************************************************/
extern xml_node_t *
    xml_new_node (void);


/********************************************************************
* FUNCTION xml_init_node
* 
* Init an xml_node_t struct
*
* INPUTS:
*   node == pointer to node to init
*********************************************************************/
extern void
    xml_init_node (xml_node_t *node);


/********************************************************************
* FUNCTION xml_free_node
* 
* Free an xml_node_t struct
*
* INPUTS:
*   node == pointer to node to free
*********************************************************************/
extern void
    xml_free_node (xml_node_t *node);


/********************************************************************
* FUNCTION xml_clean_node
* 
* Clean an xml_node_t struct
*
* INPUTS:
*   node == pointer to node to clean
*********************************************************************/
extern void
    xml_clean_node (xml_node_t *node);


/********************************************************************
* FUNCTION xml_get_reader_from_filespec
* 
* Get a new xmlTextReader for parsing a debug test file
*
* INPUTS:
*   filespec == full filename including path of the 
*               XML instance document to parse
* OUTPUTS:
*   *reader == pointer to new reader or NULL if some error
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    xml_get_reader_from_filespec (const char *filespec,
				  xmlTextReaderPtr  *reader);


/********************************************************************
* FUNCTION xml_get_reader_for_session
* 
* Get a new xmlTextReader for parsing the input of a NETCONF session
*
* INPUTS:
*   readfn == IO read function to use for this xmlTextReader
*   closefn == IO close fn to use for this xmlTextReader
*   context == the ses_cb_t pointer passes as a void *
*              this will be passed to the read and close functions
* OUTPUTS:
*   *reader == pointer to new reader or NULL if some error
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    xml_get_reader_for_session (xmlInputReadCallback readfn,
				xmlInputCloseCallback closefn,
				void *context,
				xmlTextReaderPtr  *reader);


/********************************************************************
* FUNCTION xml_reset_reader_for_session
* 
* Reset the xmlTextReader for parsing the input of a NETCONF session
*
* INPUTS:
*   readfn == IO read function to use for this xmlTextReader
*   closefn == IO close fn to use for this xmlTextReader
*   context == the ses_cb_t pointer passes as a void *
*              this will be passed to the read and close functions
* OUTPUTS:
*   *reader == pointer to new reader or NULL if some error
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t
    xml_reset_reader_for_session (xmlInputReadCallback readfn,
				  xmlInputCloseCallback closefn,
				  void *context,
				  xmlTextReaderPtr  reader);


/********************************************************************
* FUNCTION xml_free_reader
* 
* Free the previously allocated xmlTextReader
*
* INPUTS:
*   reader == xmlTextReader to close and deallocate
* RETURNS:
*   none
*********************************************************************/
extern void
    xml_free_reader (xmlTextReaderPtr reader);


/********************************************************************
* FUNCTION xml_get_node_name
* 
* get the node type according to the xmlElementType enum list
* in /usr/include/libxml/libxml/tree.h
*
* INPUTS:
*   nodeval == integer node type from system
* RETURNS:
*   string corresponding to the integer value
*********************************************************************/
extern const char * 
    xml_get_node_name (int nodeval);


/********************************************************************
* FUNCTION xml_advance_reader
* 
* Advance to the next node in the specified reader
*
* INPUTS:
*   reader == XmlReader already initialized from File, Memory,
*             or whatever
* RETURNS:
*   FALSE if OEF seen, or TRUE if normal
*********************************************************************/
extern boolean 
    xml_advance_reader (xmlTextReaderPtr reader);


/********************************************************************
* FUNCTION xml_node_match
*
* check if a specific node is the proper owner, name, and type
*
* INPUTS:
*    node == node to match against
*    nsid == namespace ID to match (0 == match any)
*    elname == element name to match (NULL == match any)
*    nodetyp == node type to match (XML_NT_NONE == match any)
* RETURNS:
*    status
*********************************************************************/
extern status_t 
    xml_node_match (const xml_node_t *node,
		    xmlns_id_t nsid,
		    const xmlChar *elname,
		    xml_nodetyp_t  nodetyp);


/********************************************************************
* FUNCTION xml_endnode_match
*
* check if a specific node is the proper endnode match
* for a given startnode
*
* INPUTS:
*    startnode == start node to match against
*    endnode == potential end node to test
* RETURNS:
*    status,
*********************************************************************/
extern status_t
    xml_endnode_match (const xml_node_t *startnode,
		       const xml_node_t *endnode);


/********************************************************************
* FUNCTION xml_docdone
*
* check if the input is completed for a given PDU
*
* INPUTS:
*    reader == xml text reader
* RETURNS:
*    TRUE if document is done
*    FALSE if document is not done or some error
*    If a node is read, the reader will be pointing to that node
*********************************************************************/
extern boolean 
    xml_docdone (xmlTextReaderPtr reader);


/********************************************************************
* FUNCTION xml_dump_node
*
* Debug function to printf xml_node_t contents
*
* INPUTS:
*    node == node to dump
*********************************************************************/
extern void
    xml_dump_node (const xml_node_t *node);


/********************************************************************
* FUNCTION xml_init_attrs
*
* initialize an xml_attrs_t variable
*
* INPUTS:
*    attrs == attribute queue to init
* RETURNS:
*    none
*********************************************************************/
extern void
    xml_init_attrs (xml_attrs_t *attrs);


/********************************************************************
* FUNCTION xml_new_attr
*
* malloc and init an attribute struct
*
* INPUTS:
*    none
* RETURNS:
*    pointer to new xml_attr_t or NULL if malloc error
*********************************************************************/
extern xml_attr_t *
    xml_new_attr (void);


/********************************************************************
* FUNCTION xml_free_attr
*
* free an attribute 
*
* INPUTS:
*    attr == xml_attr_t to free
*********************************************************************/
extern void
    xml_free_attr (xml_attr_t *attr);


/********************************************************************
* FUNCTION xml_add_attr
*
* add an attribute to an attribute list
*
* INPUTS:
*    attrs == attribute queue to init
*    ns_id == namespace ID (use XMLNS_NONE if no prefix desired)
*    attr_name == attribute name string
*    attr_val == attribute value string
* RETURNS:
*    NO_ERR if all okay
*********************************************************************/
extern status_t
    xml_add_attr (xml_attrs_t *attrs, 
		  xmlns_id_t  ns_id,
		  const xmlChar *attr_name,
		  const xmlChar *attr_val);


/********************************************************************
* FUNCTION xml_add_qattr
*
* add a qualified attribute to an attribute list with a prefix
*
* INPUTS:
*    attrs == attribute queue to init
*    ns_id == namespace ID (use XMLNS_NONE if no prefix desired)
*    attr_qname == qualified attribute name string
*    plen == attribute prefix length
*    attr_val == attribute value string
*    res == address of return status
*
* OUTPUTS:
*   *res == return status
* RETURNS:
*    pointer to attr that was added, NULL if none added
*********************************************************************/
extern xml_attr_t *
    xml_add_qattr (xml_attrs_t *attrs, 
		   xmlns_id_t  ns_id,
		   const xmlChar *attr_qname,
		   uint32  plen,
		   const xmlChar *attr_val,
		   status_t *res);


/********************************************************************
* FUNCTION xml_add_xmlns_attr
*
* add an xmlns decl to the attribute Queue
*
* INPUTS:
*    attrs == attribute queue to add to
*    ns_id == namespace ID of the xmlns target
*    pfix == namespace prefix string assigned
*         == NULL for default namespace
*
* RETURNS:
*    NO_ERR if all okay
*********************************************************************/
extern status_t
    xml_add_xmlns_attr (xml_attrs_t *attrs, 
			xmlns_id_t  ns_id,
			const xmlChar *pfix);


/********************************************************************
* FUNCTION xml_add_xmlns_attr_string
*
* add an xmlns decl to the attribute Queue
*
* INPUTS:
*    attrs == attribute queue to add to
*    ns == namespace URI string of the xmlns target
*    pfix == namespace prefix string assigned
*         == NULL for default namespace
*
* RETURNS:
*    NO_ERR if all okay
*********************************************************************/
extern status_t
    xml_add_xmlns_attr_string (xml_attrs_t *attrs, 
			       const xmlChar *ns,
			       const xmlChar *pfix);


/********************************************************************
* FUNCTION xml_add_inv_xmlns_attr
*
* add an xmlns decl to the attribute Queue
* for an INVALID namespace.  This is needed for the
* error-info element within the rpc-error report
*
* INPUTS:
*    attrs == attribute queue to add to
*    ns_id == namespace ID of the xmlns target
*    pfix == namespace prefix string assigned
*         == NULL for default namespace
*    nsval == namespace URI value of invalid namespace
*
* RETURNS:
*    NO_ERR if all okay
*********************************************************************/
extern status_t
    xml_add_inv_xmlns_attr (xml_attrs_t *attrs, 
			    xmlns_id_t  ns_id,
			    const xmlChar *pfix,
			    const xmlChar *nsval);


/********************************************************************
* FUNCTION xml_first_attr
*
* get the first attribute in the list
*
* INPUTS:
*    attrs == attribute queue to get from
* RETURNS:
*    pointer to first entry or NULL if none
*********************************************************************/
extern xml_attr_t *
    xml_first_attr (xml_attrs_t  *attrs);


/********************************************************************
* FUNCTION xml_get_first_attr
*
* get the first attribute in the attrs list, from an xml_node_t param
*
* INPUTS:
*    node == node with the attrQ to use
* RETURNS:
*    pointer to first entry or NULL if none
*********************************************************************/
extern xml_attr_t *
    xml_get_first_attr (const xml_node_t  *node);


/********************************************************************
* FUNCTION xml_next_attr
*
* get the next attribute in the list
*
* INPUTS:
*    attr == attribute entry to get next for
* RETURNS:
*    pointer to the next entry or NULL if none
*********************************************************************/
extern xml_attr_t *
    xml_next_attr (xml_attr_t  *attr);


/********************************************************************
* FUNCTION xml_clean_attrs
*
* clean an xml_attrs_t variable
*
* INPUTS:
*    attrs == attribute queue to clean
* RETURNS:
*    none
*********************************************************************/
extern void
    xml_clean_attrs (xml_attrs_t  *attrs);



/********************************************************************
* FUNCTION xml_find_attr
* 
*  Must be called after xxx_xml_consume_node
*  Go looking for the specified attribute node
* INPUTS:
*   node == xml_node_t to check
*   nsid == namespace ID of attribute to find
*   attrname == attribute name to find
* RETURNS:
*   pointer to found xml_attr_t or NULL if not found
*********************************************************************/
extern xml_attr_t *
    xml_find_attr (xml_node_t   *node,
		   xmlns_id_t    nsid,                  
		   const xmlChar *attrname);


/********************************************************************
* FUNCTION xml_find_attr_q
* 
*  Must be called after xxx_xml_consume_node
*  Go looking for the specified attribute node
* INPUTS:
*   node == xml_node_t to check
*   nsid == namespace ID of attribute to find
*   attrname == attribute name to find
* RETURNS:
*   pointer to found xml_attr_t or NULL if not found
*********************************************************************/
extern xml_attr_t *
    xml_find_attr_q (xml_attrs_t *attrs,
		     xmlns_id_t        nsid,
		     const xmlChar    *attrname);


/********************************************************************
* FUNCTION xml_find_ro_attr
* 
*  Must be called after xxx_xml_consume_node
*  Go looking for the specified attribute node
* INPUTS:
*   node == xml_node_t to check
*   nsid == namespace ID of attribute to find
*   attrname == attribute name to find
* RETURNS:
*   pointer to found xml_attr_t or NULL if not found
*********************************************************************/
extern const xml_attr_t *
    xml_find_ro_attr (const xml_node_t *node,
                      xmlns_id_t        nsid,
		      const xmlChar    *attrname);


/********************************************************************
* FUNCTION xml_strlen
* 
* String len for xmlChar -- does not check for buffer overflow
* INPUTS:
*   str == buffer to check
* RETURNS:
*   number of xmlChars before null terminator found
*********************************************************************/
extern uint32
    xml_strlen (const xmlChar *str);


/********************************************************************
* FUNCTION xml_strlen_sp
* 
* get length and check if any whitespace at the same time
* String len for xmlChar -- does not check for buffer overflow
* Check for any whitespace in the string as well
*
* INPUTS:
*   str == buffer to check
*   sp == address of any-spaces-test output
*
* OUTPUTS:
*   *sp == TRUE if any whitespace found in the string
*
* RETURNS:
*   number of xmlChars before null terminator found
*********************************************************************/
extern uint32 
    xml_strlen_sp (const xmlChar *str,
		   boolean *sp);


/********************************************************************
* FUNCTION xml_strcpy
* 
* String copy for xmlChar -- does not check for buffer overflow
*
* Even if return value is zero, the EOS char is copied
*
* INPUTS:
*   copyTo == buffer to copy into
*   copyFrom == zero-terminated xmlChar string to copy from
* RETURNS:
*   number of bytes copied to the result buffer, not including EOS
*********************************************************************/
extern uint32
    xml_strcpy (xmlChar *copyTo, 
		const xmlChar *copyFrom);


/********************************************************************
* FUNCTION xml_strncpy
* 
* String copy for xmlChar -- checks for buffer overflow
* INPUTS:
*   copyTo == buffer to copy into
*   copyFrom == zero-terminated xmlChar string to copy from
*   maxLen == max number of xmlChars to copy, but does include 
*             the terminating zero that is added if this max is reached
* RETURNS:
*   number of bytes copied to the result buffer
*********************************************************************/
extern uint32
    xml_strncpy (xmlChar *copyTo, 
		 const xmlChar *copyFrom,
		 uint32  maxlen);


/********************************************************************
* FUNCTION xml_strdup
* 
* String duplicate for xmlChar
* INPUTS:
*   copyFrom == zero-terminated xmlChar string to copy from
* RETURNS:
*   pointer to new string or NULL if some error
*********************************************************************/
extern xmlChar * 
    xml_strdup (const xmlChar *copyFrom);


/********************************************************************
* FUNCTION xml_strcat
* 
* String concatenate for xmlChar
* INPUTS:
*   appendTo == zero-terminated xmlChar string to append to
*   appendFrom == zero-terminated xmlChar string to append from
* RETURNS:
*   appendTo if no error or NULL if some error
*********************************************************************/
extern xmlChar * 
    xml_strcat (xmlChar *appendTo, 
		const xmlChar *appendFrom);


/********************************************************************
* FUNCTION xml_strncat
* 
* String concatenate for at most maxlen xmlChars
* INPUTS:
*   appendTo == zero-terminated xmlChar string to append to
*   appendFrom == zero-terminated xmlChar string to append from
*   maxlen == max number of chars to append
* RETURNS:
*   appendTo if no error or NULL if some error
*********************************************************************/
extern xmlChar * 
    xml_strncat (xmlChar *appendTo,
		 const xmlChar *appendFrom,
		 uint32 maxlen);


/********************************************************************
* FUNCTION xml_strndup
* 
* String duplicate for max N xmlChars
* INPUTS:
*   copyFrom == zero-terminated xmlChar string to copy from
*   maxlen == max number of non-zero chars to copy
* RETURNS:
*   pointer to new string or NULL if some error
*********************************************************************/
extern xmlChar * 
    xml_strndup (const xmlChar *copyFrom, 
		 uint32 maxlen);


/********************************************************************
* FUNCTION xml_ch_strndup
* 
* String duplicate for max N chars
* INPUTS:
*   copyFrom == zero-terminated char string to copy from
*   maxlen == max number of non-zero chars to copy
* RETURNS:
*   pointer to new string or NULL if some error
*********************************************************************/
extern char * 
    xml_ch_strndup (const char *copyFrom, 
		    uint32 maxlen);


/********************************************************************
* FUNCTION xml_strcmp
* 
* String compare for xmlChar
* INPUTS:
*   s1 == zero-terminated xmlChar string to compare
*   s2 == zero-terminated xmlChar string to compare
*
* RETURNS:
*   == -1 : string 1 is less than string 2
*   == 0  : strings are equal
*   == 1  : string 1 is greater than string 2
*********************************************************************/
extern int 
    xml_strcmp (const xmlChar *s1, 
		const xmlChar *s2);


/********************************************************************
* FUNCTION xml_stricmp
* 
* Case insensitive string compare for xmlChar
* INPUTS:
*   s1 == zero-terminated xmlChar string to compare
*   s2 == zero-terminated xmlChar string to compare
*
* RETURNS:
*   == -1 : string 1 is less than string 2
*   == 0  : strings are equal
*   == 1  : string 1 is greater than string 2
*********************************************************************/
extern int 
    xml_stricmp (const xmlChar *s1, 
                 const xmlChar *s2);


/********************************************************************
* FUNCTION xml_strncmp
* 
* String compare for xmlChar for at most 'maxlen' xmlChars
* INPUTS:
*   s1 == zero-terminated xmlChar string to compare
*   s2 == zero-terminated xmlChar string to compare
*   maxlen == max number of xmlChars to compare
*
* RETURNS:
*   == -1 : string 1 is less than string 2
*   == 0  : strings are equal
*   == 1  : string 1 is greater than string 2
*********************************************************************/
extern int 
    xml_strncmp (const xmlChar *s1, 
		 const xmlChar *s2,
		 uint32 maxlen);


/********************************************************************
* FUNCTION xml_strnicmp
* 
* Case insensitive string compare for xmlChar for at 
* most 'maxlen' xmlChars
*
* INPUTS:
*   s1 == zero-terminated xmlChar string to compare
*   s2 == zero-terminated xmlChar string to compare
*   maxlen == max number of xmlChars to compare
*
* RETURNS:
*   == -1 : string 1 is less than string 2
*   == 0  : strings are equal
*   == 1  : string 1 is greater than string 2
*********************************************************************/
extern int 
    xml_strnicmp (const xmlChar *s1, 
                  const xmlChar *s2,
                  uint32 maxlen);


/********************************************************************
* FUNCTION xml_isspace
* 
* Check if an xmlChar is a space char
* INPUTS:
*   ch == xmlChar to check
* RETURNS:
*   TRUE if a space, FALSE otherwise
*********************************************************************/
extern boolean
    xml_isspace (uint32 ch);


/********************************************************************
* FUNCTION xml_isspace_str
* 
* Check if an xmlChar string is all whitespace chars
* INPUTS:
*   str == xmlChar string to check
* RETURNS:
*   TRUE if all whitespace, FALSE otherwise
*********************************************************************/
extern boolean
    xml_isspace_str (const xmlChar *str);


/********************************************************************
* FUNCTION xml_strcmp_nosp
* 
* String compare for xmlChar for 2 strings, but ignoring
* whitespace differences.  All consecutive whitespace is 
* treated as one space char for comparison purposes
*
* Needed by yangdiff to compare description clauses which
* have been reformated
*
* INPUTS:
*   s1 == zero-terminated xmlChar string to compare
*   s2 == zero-terminated xmlChar string to compare
*
* RETURNS:
*   == -1 : string 1 is less than string 2
*   == 0  : strings are equal
*   == 1  : string 1 is greater than string 2
*********************************************************************/
extern int 
    xml_strcmp_nosp (const xmlChar *s1, 
		     const xmlChar *s2);


/********************************************************************
* FUNCTION xml_copy_clean_string
* 
* Get a malloced string contained the converted string
* from the input
* Get rid of the leading and trailing whilespace
*
* Character entities have already been removed by the xmlTextReader
*
* INPUTS:
*   str == xmlChar string to check
* RETURNS:
*   pointer to new malloced string
*********************************************************************/
extern xmlChar *
    xml_copy_clean_string (const xmlChar *str);


/********************************************************************
* FUNCTION xml_convert_char_entity
* 
* Convert an XML character entity into a single xmlChar
*
* INPUTS:
*    str == string pointing to start of char entity
* OUTPUTS:
*    *used == number of chars consumed 
* RETURNS:
*   converted xmlChar
*********************************************************************/
extern xmlChar
    xml_convert_char_entity (const xmlChar *str, 
			     uint32 *used);


/********************************************************************
* FUNCTION xml_check_ns
*
* INPUTS:
*   reader == XmlReader already initialized from File, Memory,
*             or whatever
*   pfs->ns_id == element namespace to check
*   elname == element name to check
*
* OUTPUTS:
*   *id == namespace ID found or 0 if none
*   *pfix_len == filled in > 0 if one found
*             real element name will start at pfix_len+1
*             if pfix is non-NULL
*   *badns == pointer to unknown namespace if error returned
* RETURNS:
*   status; could be error if namespace specified but not supported
*********************************************************************/
extern status_t
    xml_check_ns (xmlTextReaderPtr reader,
		  const xmlChar   *elname,
		  xmlns_id_t       *id,
		  uint32           *pfix_len,
		  const xmlChar   **badns);


/********************************************************************
* FUNCTION xml_check_qname_content
* 
* Check if the string node content is a likely QName
* If so, then get the namespace URI for the prefix,
* look it up in def_reg, and store the NSID in the node
*
* INPUTS:
*    reader == reader to use
*    node == current string node in progress
*
* OUTPUTS:
*    
*********************************************************************/
extern void
    xml_check_qname_content (xmlTextReaderPtr reader,
			     xml_node_t      *node);


/********************************************************************
* FUNCTION xml_get_namespace_id
* 
* Get the namespace for the specified prefix (may be NULL)
* Use the current XML reader context to resolve the prefix
*
* INPUTS:
*    reader == XML reader to use
*    prefix == prefix string to use (NULL == default namespace)
*    prefixlen == N if not a Z-terminated string
*              == 0 if it is a Z-terminated string
*    retnsid == address of return namespace ID
*
* OUTPUTS:
*    *retnsid == XMLNS ID for the namespace, 0 if none found
*               INVALID ID if unknown
* RETURNS:
*   status
*********************************************************************/
extern status_t
    xml_get_namespace_id (xmlTextReaderPtr reader,
			  const xmlChar *prefix,
			  uint32 prefixlen,
			  xmlns_id_t  *retnsid);


/********************************************************************
* FUNCTION xml_consume_node
* 
* parse function for YIN input
*
* INPUTS:
*    reader == xmlTextReader to use
*    node == address of node pointer to use
*            MUST be an initialized node
*            with xml_new_node or xml_init_node
*    nserr == TRUE if bad namespace should be checked
*          == FALSE if not
*    adv == TRUE if advance reader
*        == FALSE if no advance (reget current node)
*
* OUTPUTS:
*    *xmlnode == filled in or malloced and filled-in xml node
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    xml_consume_node (xmlTextReaderPtr reader,
                      xml_node_t  *xmlnode,
                      boolean nserr,
                      boolean adv);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xml_util */
