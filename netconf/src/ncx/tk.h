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
#ifndef _H_tk
#define _H_tk

/*  FILE: tk.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    YANG Module Compact Syntax Token Handler

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
12-nov-05    abb      Begun

*/

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/

/* maximum line size allowed in an YANG module */
#define TK_BUFF_SIZE                0xffff


/* macros for quick processing of token chains
 * All these macros take 1 parameter 
 * INPUTS:
 *   T == pointer to the tk_chain_t in progress
 */

/* advance the current token pointer */
#define TK_ADV(T) \
    ((T)->cur ? (((T)->cur = (tk_token_t *)dlq_nextEntry((T)->cur)) \
                 ? NO_ERR : ERR_NCX_EOF) : ERR_NCX_EOF)

/* back-up the current token pointer */
#define TK_BKUP(T) \
    if (!  ((T)->cur = (tk_token_t *)dlq_prevEntry((T)->cur)))  \
        (T)->cur = (tk_token_t *)&((T)->tkQ)

/* return the current token */
#define TK_CUR(T) ((T)->cur)

/* return the current token type */
#define TK_CUR_TYP(T) ((T)->cur->typ)

/* return the current token value */
#define TK_CUR_VAL(T) ((T)->cur->val)

/* return the current token value length */
#define TK_CUR_LEN(T) ((T)->cur->len)

/* return the current token module qualifier value */
#define TK_CUR_MOD(T) ((T)->cur->mod)

/* return the current token module qualifier value length */
#define TK_CUR_MODLEN(T) ((T)->cur->modlen)

/* return TRUE if the current token type is a string */
#define TK_CUR_STR(T) ((T)->cur->typ >= TK_TT_STRING && \
		       (T)->cur->typ <= TK_TT_SQSTRING)

/* return TRUE if the specified token type is a string */
#define TK_TYP_STR(T) ((T) >= TK_TT_STRING && (T) <= TK_TT_SQSTRING)

/* return TRUE if the cur tk type allows a non-whitespace string */
#define TK_CUR_NOWSTR(T) ((T)->cur->typ >= TK_TT_STRING && \
                          (T)->cur->typ <= TK_TT_SQSTRING)

/* return TRUE if the cu token type allows whitespace string */
#define TK_CUR_WSTR(T) ((T)->cur->typ==TK_TT_STRING || \
                        (T)->cur->typ==TK_TT_SSTRING ||  \
			(T)->cur->typ==TK_TT_TSTRING ||	 \
			(T)->cur->typ==TK_TT_QSTRING || \
			(T)->cur->typ==TK_TT_SQSTRING)

/* return TRUE if the current token type is a number */
#define TK_CUR_NUM(T) ((T)->cur->typ==TK_TT_DNUM || \
                       (T)->cur->typ==TK_TT_HNUM ||  \
                       (T)->cur->typ==TK_TT_RNUM)

/* return TRUE if the current token type is an integral number */
#define TK_CUR_INUM(T) ((T)->cur->typ==TK_TT_DNUM || \
                        (T)->cur->typ==TK_TT_HNUM)

/* return TRUE if the current token type is an instance qualifier */
#define TK_CUR_IQUAL(T) ((T)->cur->typ==TK_TT_QMARK || \
                         (T)->cur->typ==TK_TT_STAR ||  \
                         (T)->cur->typ==TK_TT_PLUS)


/* return TRUE if the current token type is an identifier */
#define TK_CUR_ID(T) ((T)->cur->typ==TK_TT_TSTRING || \
                      (T)->cur->typ==TK_TT_MSTRING)

/* return TRUE if the current token type is a scoped identifier */
#define TK_CUR_SID(T) ((T)->cur->typ==TK_TT_SSTRING || \
                       (T)->cur->typ==TK_TT_MSSTRING)

/* return the current line number */
#define TK_CUR_LNUM(T) ((T)->cur->linenum)

/* return the current line position */
#define TK_CUR_LPOS(T) ((T)->cur->linepos)

/* return TRUE if the token is a text or number type, 
 * as opposed to a 1 or 2 char non-text token type
 */
#define TK_CUR_TEXT(T) ((T)->cur->typ >= TK_TT_SSTRING &&\
			(T)->cur->typ <= TK_TT_RNUM)


/* return the namespace ID field in the token */
#define TK_CUR_NSID(T) ((T)->cur->nsid)

/* return non-zero if token preservation docmode */
#define TK_DOCMODE(TKC)  ((TKC)->flags & TK_FL_DOCMODE)

#define TK_HAS_ORIGTK(TK) (((TK)->typ == TK_TT_QSTRING ||   \
                            (TK)->typ == TK_TT_SQSTRING) && \
                           (TK)->origval != NULL)

/* bits for tk_chain_t flags field */

/* == 1: the current parse call is to redo a previously
 *       parsed token
 * == 0: normal parse mode
 */
#define TK_FL_REDO        bit0

/* == 1: the tkc->buff field is malloced by this module
 *       and will be freed in tk_free_chain
 * == 0: the tkc->buff memory was provided externally
 *       and will not be freed in tk_free_chain
 */
#define TK_FL_MALLOC      bit1

/* == 1: DOC mode: the tk->origtkQ field will be used 
 *       to preserve pre-string concat and processing
 * == 0: normal mode: the rk-origtkQ will not be used
 *       and only the result token will be saved
 */
#define TK_FL_DOCMODE     bit2





/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/* different types of tokens parsed during 1st pass */
typedef enum tk_type_t_ {
    TK_TT_NONE,
    /* PUT ALL 1-CHAR TOKENS FIRST */
    TK_TT_LBRACE,               /* left brace '{' */
    TK_TT_RBRACE,               /* right brace '}' */
    TK_TT_SEMICOL,              /* semi-colon ';' */
    TK_TT_LPAREN,               /* left paren '(' */
    TK_TT_RPAREN,               /* right paren ')' */
    TK_TT_LBRACK,               /* left bracket '[' */
    TK_TT_RBRACK,               /* right bracket ']' */
    TK_TT_COMMA,                /* comma ',' */ 
    TK_TT_EQUAL,                /* equal sign '=' */ 
    TK_TT_BAR,                  /* bar '|' */
    TK_TT_STAR,                 /* star '*' */
    TK_TT_ATSIGN,               /* at sign '@' */
    TK_TT_PLUS,                 /* plus mark '+' */
    TK_TT_COLON,                /* colon char ':' */
    TK_TT_PERIOD,               /* period char '.' */
    TK_TT_FSLASH,               /* forward slash char '/' */
    TK_TT_MINUS,                /* minus char '-' */
    TK_TT_LT,                   /* less than char '<' */
    TK_TT_GT,                   /* greater than char '>' */

    /* PUT ALL 2-CHAR TOKENS SECOND */
    TK_TT_RANGESEP,             /* range sep, parent node '..' */
    TK_TT_DBLCOLON,             /* 2 colon chars '::' */
    TK_TT_DBLFSLASH,            /* 2 fwd slashes '//' */
    TK_TT_NOTEQUAL,             /* not equal '!=' */
    TK_TT_LEQUAL,               /* less than or equal '<=' */
    TK_TT_GEQUAL,               /* greater than or equal '>=' */


    /* PUT ALL STRING CLASSIFICATION TOKENS THIRD */
    TK_TT_STRING,               /* unquoted string */
    TK_TT_SSTRING,              /* scoped token string */
    TK_TT_TSTRING,              /* token string */
    TK_TT_MSTRING,              /* module-qualified token string */
    TK_TT_MSSTRING,             /* module-qualified scoped string */
    TK_TT_QSTRING,              /* double quoted string */
    TK_TT_SQSTRING,             /* single quoted string */

    TK_TT_VARBIND,              /* XPath varbind '$NCName' */
    TK_TT_QVARBIND,             /* XPath varbind '$prefix:NCName' */

    TK_TT_NCNAME_STAR,         /* XPath NCName:* sequence */

    /* PUT ALL NUMBER CLASSIFICATION TOKENS FOURTH */
    TK_TT_DNUM,                 /* decimal number */
    TK_TT_HNUM,                 /* hex number */
    TK_TT_RNUM,                 /* real number */

    /* PUT ALL SPECIAL CASE TOKENS LAST */
    TK_TT_NEWLINE               /* \n is significant in conf files */
} tk_type_t;


typedef enum tk_source_t_ {
    TK_SOURCE_CONF,
    TK_SOURCE_YANG,
    TK_SOURCE_XPATH,
    TK_SOURCE_REDO
} tk_source_t;


typedef enum tk_origstr_typ_t_ {
    TK_ORIGSTR_NONE,
    TK_ORIGSTR_DQUOTE,
    TK_ORIGSTR_DQUOTE_NL,
    TK_ORIGSTR_SQUOTE,
    TK_ORIGSTR_SQUOTE_NL
} tk_origstr_typ_t;


/* each entry in the origstrQ is the 2nd through Nth string
 * to be concated.  The first string is in the tk->origval
 *  string for double quote and tk->val for single quote
 */
typedef struct tk_origstr_t_ {
    dlq_hdr_t          qhdr;
    tk_origstr_typ_t   origtyp;
    xmlChar           *str;
} tk_origstr_t;


/* single YANG language token type */
typedef struct tk_token_t_ {
    dlq_hdr_t   qhdr;
    tk_type_t   typ;
    xmlChar    *mod;                  /* only used if prefix found */
    uint32      modlen;          /* length of 'mod'; not Z-string! */
    xmlChar    *val;
    xmlChar    *origval;           /* used in DOCMODE for yangdump */
    uint32      len;
    uint32      linenum;
    uint32      linepos;
    xmlns_id_t  nsid;        /* only used for TK_TT_MSTRING tokens */
    dlq_hdr_t   origstrQ;  /* Q of tk_origstr_t only used in DOCMODE */
} tk_token_t;


/* token backptr to get at original token chain for strings
 * used only by yangdump --format=yang|html
 */
typedef struct tk_token_ptr_t_ {
    dlq_hdr_t       qhdr;
    tk_token_t     *tk;
    const void     *field;
} tk_token_ptr_t;


/* token parsing chain */
typedef struct tk_chain_t_ {
    dlq_hdr_t      qhdr;
    dlq_hdr_t      tkQ;            /* Q of tk_token_t */
    dlq_hdr_t      tkptrQ;     /* Q of tk_token_ptr_t */
    tk_token_t    *cur;
    ncx_error_t   *curerr;
    const xmlChar *filename;
    FILE          *fp;
    xmlChar       *buff;
    xmlChar       *bptr;
    uint32         linenum;
    uint32         linepos;
    uint32         flags;
    tk_source_t    source;
} tk_chain_t;


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION tk_new_chain
* 
* Allocatate a new token parse chain
*
* RETURNS:
*  new parse chain or NULL if memory error
*********************************************************************/
extern tk_chain_t *
    tk_new_chain (void);


/********************************************************************
* FUNCTION tk_setup_chain_conf
* 
* Setup a previously allocated chain for a text config file
*
* INPUTS
*    tkc == token chain to setup
*    fp == open file to use for text source
*    filename == source filespec
*********************************************************************/
extern void
    tk_setup_chain_conf (tk_chain_t *tkc,
			 FILE *fp,
			 const xmlChar *filename);


/********************************************************************
* FUNCTION tk_setup_chain_yang
* 
* Setup a previously allocated chain for a YANG file
*
* INPUTS
*    tkc == token chain to setup
*    fp == open file to use for text source
*    filename == source filespec
*********************************************************************/
extern void
    tk_setup_chain_yang (tk_chain_t *tkc,
			 FILE *fp,
			 const xmlChar *filename);


/********************************************************************
* FUNCTION tk_setup_chain_yin
* 
* Setup a previously allocated chain for a YIN file
*
* INPUTS
*    tkc == token chain to setup
*    filename == source filespec
*********************************************************************/
extern void
    tk_setup_chain_yin (tk_chain_t *tkc,
                        const xmlChar *filename);


/********************************************************************
* FUNCTION tk_setup_chain_docmode
* 
* Setup a previously allocated chain for a yangdump docmode output
*
* INPUTS
*    tkc == token chain to setup
*********************************************************************/
extern void
    tk_setup_chain_docmode (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_free_chain
* 
* Cleanup and deallocate a tk_chain_t 
* INPUTS:
*   tkc == TK chain to delete
* RETURNS:
*  none
*********************************************************************/
extern void
    tk_free_chain (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_get_yang_btype_id
* 
* Check if the specified string is a YANG builtin type name
* checks for valid YANG data type name
*
* INPUTS:
*   buff == token string to check -- NOT ZERO-TERMINATED
*   len == length of string to check
* RETURNS:
*   btype found or NCX_BT_NONE if no match
*********************************************************************/
extern ncx_btype_t 
    tk_get_yang_btype_id (const xmlChar *buff, 
			  uint32 len);


/********************************************************************
* FUNCTION tk_get_token_name
* 
* Get the symbolic token name
*
* INPUTS:
*   ttyp == token type
* RETURNS:
*   const string to the name; will not be NULL
*********************************************************************/
extern const char *
    tk_get_token_name (tk_type_t ttyp);


/********************************************************************
* FUNCTION tk_get_token_sym
* 
* Get the symbolic token symbol
*
* INPUTS:
*   ttyp == token type
* RETURNS:
*   const string to the symbol; will not be NULL
*********************************************************************/
extern const char *
    tk_get_token_sym (tk_type_t ttyp);


/********************************************************************
* FUNCTION tk_get_btype_sym
* 
* Get the symbolic token symbol for one of the base types
*
* INPUTS:
*   btyp == base type
* RETURNS:
*   const string to the symbol; will not be NULL
*********************************************************************/
extern const char *
    tk_get_btype_sym (ncx_btype_t btyp);


/********************************************************************
* FUNCTION tk_next_typ
* 
* Get the token type of the next token
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   token type
*********************************************************************/
extern tk_type_t
    tk_next_typ (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_next_typ2
* 
* Get the token type of the token after the next token
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   token type
*********************************************************************/
extern tk_type_t
    tk_next_typ2 (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_next_val
* 
* Get the token type of the next token
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   token type
*********************************************************************/
extern const xmlChar *
    tk_next_val (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_dump_token
* 
* Debug printf the specified token
* !!! Very verbose !!!
*
* INPUTS:
*   tk == token
*
*********************************************************************/
extern void
    tk_dump_token (const tk_token_t *tk);


/********************************************************************
* FUNCTION tk_dump_chain
* 
* Debug printf the token chain
* !!! Very verbose !!!
*
* INPUTS:
*   tkc == token chain 
*
* RETURNS:
*   none
*********************************************************************/
extern void
    tk_dump_chain (const tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_is_wsp_string
* 
* Check if the current token is a string with whitespace in it
*
* INPUTS:
*   tk == token to check
*
* RETURNS:
*    TRUE if a string with whitespace in it
*    FALSE if not a string or no whitespace in the string
*********************************************************************/
extern boolean
    tk_is_wsp_string (const tk_token_t *tk);


/********************************************************************
* FUNCTION tk_tokenize_input
* 
* Parse the input (FILE or buffer) into tk_token_t structs
*
* The tkc param must be initialized to use the internal
* buffer to read from the specified filespec:
*
*    tkc->filename
*    tkc->flags
*    tkc->fp
*    tkc->source
*
* External buffer mode:
*
* If no filename is provided, the the TK_FL_MALLOC 
* flag will not be set, and the tkc->buff field 
* must be initialized before this function is called.  
* This function will not free the buffer,
* but just read from it until a '0' char is reached.
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* INPUTS:
*   tkc == token chain 
*   mod == module in progress (NULL if not used)
*          !!! Just used for error messages !!!
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    tk_tokenize_input (tk_chain_t *tkc,
		       ncx_module_t *mod);


/********************************************************************
* FUNCTION tk_retokenize_cur_string
* 
* The current token is some sort of a string
* Reparse it according to the full YANG token list, as needed
*
* The current token may be replaced with one or more tokens
*
* INPUTS:
*   tkc == token chain 
*   mod == module in progress (NULL if not used)
*
* RETURNS:
*   status of the operation
*********************************************************************/
extern status_t 
    tk_retokenize_cur_string (tk_chain_t *tkc,
			      ncx_module_t *mod);


/********************************************************************
* FUNCTION tk_tokenize_metadata_string
* 
* The specified ncx:metadata string is parsed into tokens
* convert the ncx:metadata content to 1 or 2 tokens
*
* INPUTS:
*   mod == module in progress for error purposes (may be NULL)
*   str == string to tokenize
*   res == address of return status
*
* OUTPUTS:
*   *res == error status, if return NULL or non-NULL
*
* RETURNS:
*   pointer to malloced and filled in token chain
*   ready to be traversed; always check *res for valid syntax
*********************************************************************/
extern tk_chain_t *
    tk_tokenize_metadata_string (ncx_module_t *mod,
				 xmlChar *str,
				 status_t *res);


/********************************************************************
* FUNCTION tk_tokenize_xpath_string
* 
* The specified XPath string is parsed into tokens
*
* INPUTS:
*   mod == module in progress for error purposes (may be NULL)
*   str == string to tokenize
*   curlinenum == current line number
*   curlinepos == current line position
*   res == address of return status
*
* OUTPUTS:
*   *res == error status, if return NULL or non-NULL
*
* RETURNS:
*   pointer to malloced and filled in token chain
*   ready to be traversed; always check *res for valid syntax
*********************************************************************/
extern tk_chain_t *
    tk_tokenize_xpath_string (ncx_module_t *mod,
			      xmlChar *str,
			      uint32 curlinenum,
			      uint32 curlinepos,
			      status_t *res);


/********************************************************************
* FUNCTION tk_token_count
* 
* Get the number of tokens in the queue
*
* INPUTS:
*   tkc == token chain to check
*
* RETURNS:
*   number of tokens in the queue
*********************************************************************/
extern uint32
    tk_token_count (const tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_reset_chain
* 
* Reset the token chain current pointer to the start
*
* INPUTS:
*   tkc == token chain to reset
*
*********************************************************************/
extern void
    tk_reset_chain (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_clone_chain
* 
* Allocatate and a new token parse chain and fill
* it with the specified token chain contents 
*
* INPUTS:
*   oldtkc == token chain to clone
*
* RETURNS:
*    new cloned parse chain or NULL if memory error
*********************************************************************/
extern tk_chain_t *
    tk_clone_chain (tk_chain_t *oldtkc);


/********************************************************************
* FUNCTION tk_add_id_token
* 
* Allocatate a new ID token and add it to the parse chain
*
* INPUTS:
*   tkc == token chain to use
*   valstr == ID name
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_id_token (tk_chain_t *tkc,
                     const xmlChar *valstr);


/********************************************************************
* FUNCTION tk_add_pid_token
* 
* Allocatate a new prefixed ID token and add it to 
* the parse chain
*
* INPUTS:
*   tkc == token chain to use
*   prefix == ID prefix
*   prefixlen == 'prefix' length in bytes
*   valstr == ID name
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_pid_token (tk_chain_t *tkc,
                      const xmlChar *prefix,
                      uint32 prefixlen,
                      const xmlChar *valstr);


/********************************************************************
* FUNCTION tk_add_string_token
* 
* Allocatate a new string token and add it to the parse chain
*
* INPUTS:
*   tkc == token chain to use
*   valstr == string value to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_string_token (tk_chain_t *tkc,
                         const xmlChar *valstr);


/********************************************************************
* FUNCTION tk_add_lbrace_token
* 
* Allocatate a new left brace token and add it to the parse chain
*
* INPUTS:
*   tkc == token chain to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_lbrace_token (tk_chain_t *tkc);



/********************************************************************
* FUNCTION tk_add_rbrace_token
* 
* Allocatate a new right brace token and add it to the parse chain
*
* INPUTS:
*   tkc == token chain to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_rbrace_token (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_add_semicol_token
* 
* Allocatate a new semi-colon token and add it to the parse chain
*
* INPUTS:
*   tkc == token chain to use
*
* RETURNS:
*    status
*********************************************************************/
extern status_t
    tk_add_semicol_token (tk_chain_t *tkc);


/********************************************************************
* FUNCTION tk_check_save_origstr
* 
* Check the docmode and the specified token;
* Save a tk_origptr_t if needed with the field address
*
* INPUTS:
*   tkc = token chain to use
*   tk  = token to use (usually TK_CUR())
*   field == address of string field to save as the key
*
* RETURNS:
*    status; ERR_INTERNAL_MEM if entry cannot be malloced
*********************************************************************/
extern status_t
    tk_check_save_origstr (tk_chain_t  *tkc,
                           tk_token_t *tk,
                           const void *field);


/********************************************************************
* FUNCTION tk_get_first_origstr
* 
* Get the first original string to use
*
* INPUTS:
*   tkptr  = token pointer to use
*   dquote = address of return double quote flag
*   morestr == addres of return more string fragments flag
* OUTPUTS:
*   *dquote = return double quote flag
*            TRUE == TK_TT_QSTRING
*           FALSE == TK_TTSQSTRING
*   *morestr == addres of return more string fragments flag
*           TRUE == more string fragments after first one
* RETURNS:
*   pointer to the first string fragment
*********************************************************************/
extern const xmlChar *
    tk_get_first_origstr (const tk_token_ptr_t  *tkptr,
                          boolean *dquote,
                          boolean *morestr);


/********************************************************************
* FUNCTION tk_first_origstr_rec
* 
* Get the first tk_origstr_t struct (if any)
*
* INPUTS:
*   tkptr  = token pointer to use
*
* RETURNS:
*   pointer to the first original string record
*********************************************************************/
extern const tk_origstr_t *
    tk_first_origstr_rec (const tk_token_ptr_t  *tkptr);


/********************************************************************
* FUNCTION tk_next_origstr_rec
* 
* Get the next tk_origstr_t struct (if any)
*
* INPUTS:
*   origstr  = origisnal string record pointer to use
*
* RETURNS:
*   pointer to the next original string record
*********************************************************************/
extern const tk_origstr_t *
    tk_next_origstr_rec (const tk_origstr_t  *origstr);


/********************************************************************
* FUNCTION tk_get_origstr_parts
* 
* Get the fields from the original string record
*
* INPUTS:
*   origstr  = original string record pointer to use
*   dquote = address of return double quote flag
*   newline == addres of return need newline flag
* OUTPUTS:
*   *dquote = return double quote flag
*            TRUE == TK_TT_QSTRING
*           FALSE == TK_TTSQSTRING
*   *newline == return need newline flag
*           TRUE == need newline + indent before this string
* RETURNS:
*   pointer to the string fragment
*********************************************************************/
extern const xmlChar *
    tk_get_origstr_parts (const tk_origstr_t  *origstr,
                          boolean *dquote,
                          boolean *newline);


/********************************************************************
* FUNCTION tk_find_tkptr
* 
* Find the specified token pointer record
*
* INPUTS:
*   tkc  = token chain to use
*   field == address of field to use as key to find
*
* RETURNS:
*   pointer to the token pointer record or NULL if not found
*********************************************************************/
extern const tk_token_ptr_t *
    tk_find_tkptr (const tk_chain_t  *tkc,
                   const void *field);


/********************************************************************
* FUNCTION tk_tkptr_quotes
* 
* Get the specified token pointer record token ID type
* Use the first string or only string
*
* INPUTS:
*   tkptr  = token pointer to use
*
* RETURNS:
*   number of quotes used in first string
*********************************************************************/
extern uint32
    tk_tkptr_quotes (const tk_token_ptr_t  *tkptr);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif
                      
#endif	    /* _H_tk */
