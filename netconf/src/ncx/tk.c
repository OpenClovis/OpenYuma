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
/*  FILE: tk.c


    YANG Token Manager : Low level token management functions

     - Tokenization of YANG modules
     - Tokenization of text config files

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
12nov05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <assert.h>

#include <xmlstring.h>

#include  "procdefs.h"
#include "dlq.h"
#include "log.h"
#include "ncx.h"
#include "ncxconst.h"
#include  "status.h"
#include "typ.h"
#include "tk.h"
#include "xml_util.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* #define TK_DEBUG 1 */
/* #define TK_RDLN_DEBUG 1 */

#define FL_YANG   bit0                /* source is TK_SOURCE_YANG */
#define FL_CONF   bit1                /* source is TK_SOURCE_CONF */
#define FL_XPATH  bit2               /* source is TK_SOURCE_XPATH */
#define FL_REDO   bit3                /* source is TK_SOURCE_REDO */

#define FL_NCX    bit4                /* place-holder; deprecated */

#define FL_ALL    (FL_YANG|FL_CONF|FL_XPATH|FL_REDO)

/********************************************************************
*                                                                   *
*                            T Y P E S                              *
*                                                                   *
*********************************************************************/

/* One quick entry token lookup */
typedef struct tk_ent_t_ {
    tk_type_t       ttyp;
    uint32          tlen;
    const char     *tid;
    const char     *tname;
    uint32          flags;
} tk_ent_t;

/* One quick entry built-in type name lookup */
typedef struct tk_btyp_t_ {
    ncx_btype_t     btyp;
    uint32          blen;
    const xmlChar  *bid;
    uint32          flags;
} tk_btyp_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

/* lookup list of all the non-textual/numeric tokens 
 *  !! MAKE SURE THIS TABLE ORDER MATCHES THE ORDER OF tk_type_t 
 * The lengths in this table do not include the terminating zero
 */
static tk_ent_t tlist [] = {
    /* align array with token enumeration values, skip 0 */
    { TK_TT_NONE, 0, NULL, "none", 0 }, 
    
    /* ONE CHAR TOKENS */
    { TK_TT_LBRACE, 1, "{", "left brace", FL_ALL },
    { TK_TT_RBRACE, 1, "}", "right brace", FL_ALL },
    { TK_TT_SEMICOL, 1, ";", "semicolon", FL_YANG },
    { TK_TT_LPAREN, 1, "(", "left paren", FL_XPATH },
    { TK_TT_RPAREN, 1, ")", "right paren", FL_XPATH },
    { TK_TT_LBRACK, 1, "[", "left bracket", FL_XPATH },
    { TK_TT_RBRACK, 1, "]", "right bracket", FL_XPATH },
    { TK_TT_COMMA, 1, ",", "comma", FL_XPATH },
    { TK_TT_EQUAL, 1, "=", "equals sign", FL_XPATH },
    { TK_TT_BAR, 1, "|", "vertical bar", (FL_YANG|FL_XPATH|FL_REDO) },
    { TK_TT_STAR, 1, "*", "asterisk", FL_XPATH },
    { TK_TT_ATSIGN, 1, "@", "at sign", FL_XPATH },
    { TK_TT_PLUS, 1, "+", "plus sign", (FL_YANG|FL_XPATH|FL_REDO) },
    { TK_TT_COLON, 1, ":", "colon", FL_XPATH },
    { TK_TT_PERIOD, 1, ".", "period", FL_XPATH },
    { TK_TT_FSLASH, 1, "/", "forward slash", FL_XPATH },
    { TK_TT_MINUS, 1, "-", "minus", FL_XPATH },
    { TK_TT_LT, 1, "<", "less than", FL_XPATH },
    { TK_TT_GT, 1, ">", "greater than", FL_XPATH },

    /* TWO CHAR TOKENS */
    { TK_TT_RANGESEP, 2, "..", "range seperator", 
      (FL_REDO|FL_XPATH) },
    { TK_TT_DBLCOLON, 2, "::", "double colon", FL_XPATH },
    { TK_TT_DBLFSLASH, 2, "//", "double foward slash", FL_XPATH },
    { TK_TT_NOTEQUAL, 2, "!=", "not equal sign", FL_XPATH },
    { TK_TT_LEQUAL, 2, "<=", "less than or equal", FL_XPATH },
    { TK_TT_GEQUAL, 2, ">=", "greater than or equal", FL_XPATH },

    /* string classification tokens, all here for name and placeholder */
    { TK_TT_STRING, 0, "", "unquoted string", FL_ALL},
    { TK_TT_SSTRING, 0, "", "scoped ID string", FL_ALL},
    { TK_TT_TSTRING, 0, "", "token string", FL_ALL},
    { TK_TT_MSTRING, 0, "", "prefix qualified ID string", FL_ALL},
    { TK_TT_MSSTRING, 0, "", "prefix qualified scoped ID string", FL_ALL},
    { TK_TT_QSTRING, 0, "", "double quoted string", FL_ALL},
    { TK_TT_SQSTRING, 0, "", "single quoted string", FL_ALL},

    /* variable binding '$NCName' or '$QName' */
    { TK_TT_VARBIND, 0, "", "varbind", FL_XPATH },
    { TK_TT_QVARBIND, 0, "", "qvarbind", FL_XPATH },

    /* XPath NameTest, form 2: 'NCName:*' */
    { TK_TT_NCNAME_STAR, 0, "", "NCName:*", FL_XPATH },

    /* number classification tokens */
    { TK_TT_DNUM, 0, "", "decimal number", FL_ALL},
    { TK_TT_HNUM, 0, "", "hex number", FL_ALL},
    { TK_TT_RNUM, 0, "", "real number", FL_ALL},

    /* newline for conf file parsing only */
    { TK_TT_NEWLINE, 0, "", "newline", FL_CONF}, 

    /* EO Array marker */
    { TK_TT_NONE, 0, NULL, "none", 0}
};


/* lookup list of YANG builtin type names
 * also includes NCX extensions xsdlist 
 * and placeholder types for YANG data node types
 *
 *  !!! ORDER IS HARD-WIRED TO PREVENT RUNTIME INIT FUNCTION !!!
 *  !!! NEED TO CHANGE IF YANG DATA TYPE NAMES ARE CHANGED !!!
 *  !!! NEED TO KEEP ARRAY POSITION AND NCX_BT_ VALUE THE SAME !!!
 *
 * hack: string lengths are stored so only string compares
 * with the correct number of chars are actually made
 * when parsing the YANG type statements
 *
 * TBD: change to runtime strlen evaluation 
 * instead of hardwire lengths.
 */
static tk_btyp_t blist [] = {
    { NCX_BT_NONE, 4, (const xmlChar *)"NONE", 0 },
    { NCX_BT_ANY, 6, NCX_EL_ANYXML, 0 },   /* anyxml */
    { NCX_BT_BITS, 4, NCX_EL_BITS, FL_YANG },
    { NCX_BT_ENUM, 11, NCX_EL_ENUMERATION, FL_YANG },
    { NCX_BT_EMPTY, 5, NCX_EL_EMPTY, FL_YANG },
    { NCX_BT_BOOLEAN, 7, NCX_EL_BOOLEAN, FL_YANG },
    { NCX_BT_INT8, 4, NCX_EL_INT8, FL_YANG },
    { NCX_BT_INT16, 5, NCX_EL_INT16, FL_YANG },
    { NCX_BT_INT32, 5, NCX_EL_INT32, FL_YANG },
    { NCX_BT_INT64, 5, NCX_EL_INT64, FL_YANG },
    { NCX_BT_UINT8, 5, NCX_EL_UINT8, FL_YANG },
    { NCX_BT_UINT16, 6, NCX_EL_UINT16, FL_YANG },
    { NCX_BT_UINT32, 6, NCX_EL_UINT32, FL_YANG },
    { NCX_BT_UINT64, 6, NCX_EL_UINT64, FL_YANG },
    { NCX_BT_DECIMAL64, 9, NCX_EL_DECIMAL64, FL_YANG },
    { NCX_BT_FLOAT64, 7, NCX_EL_FLOAT64, 0 },
    { NCX_BT_STRING, 6, NCX_EL_STRING, FL_YANG },
    { NCX_BT_BINARY, 6, NCX_EL_BINARY, FL_YANG },
    { NCX_BT_INSTANCE_ID, 19, NCX_EL_INSTANCE_IDENTIFIER, FL_YANG },
    { NCX_BT_UNION, 5, NCX_EL_UNION, FL_YANG },
    { NCX_BT_LEAFREF, 7, NCX_EL_LEAFREF, FL_YANG },
    { NCX_BT_IDREF, 11, NCX_EL_IDENTITYREF, FL_YANG },
    { NCX_BT_SLIST, 5, NCX_EL_SLIST, 0 },
    { NCX_BT_CONTAINER, 9, NCX_EL_CONTAINER, 0 },
    { NCX_BT_CHOICE, 6, NCX_EL_CHOICE, 0 },
    { NCX_BT_CASE, 4, NCX_EL_CASE, 0 },
    { NCX_BT_LIST, 4, NCX_EL_LIST, 0 },
    { NCX_BT_NONE, 4, (const xmlChar *)"NONE", 0 }
};



/********************************************************************
* FUNCTION new_origstr
* 
* Allocatate a new tk_origstr_t
*
* INPUTS:
*  ttyp == token type
*  newline == TRUE if newline added; FALSE if not
*  strval == malloced string handed over to this data structure
*
* RETURNS:
*   new token or NULL if some error
*********************************************************************/
static tk_origstr_t *
    new_origstr (tk_type_t ttyp, 
                boolean newline,
                xmlChar *strval)
{
    tk_origstr_t  *origstr;

    origstr = m__getObj(tk_origstr_t);
    if (origstr == NULL) {
        return NULL;
    }
    memset(origstr, 0x0, sizeof(tk_origstr_t));
    if (ttyp == TK_TT_QSTRING) {
        origstr->origtyp = ((newline) 
                            ? TK_ORIGSTR_DQUOTE_NL : TK_ORIGSTR_DQUOTE);
    } else if (ttyp == TK_TT_SQSTRING) {
        origstr->origtyp = ((newline) 
                            ? TK_ORIGSTR_SQUOTE_NL : TK_ORIGSTR_SQUOTE);
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        m__free(origstr);
        return NULL;
    }
    origstr->str = strval;  /* hand off memory here */
    return origstr;

}  /* new_origstr */


/********************************************************************
* FUNCTION free_origstr
* 
* Deallocatate a tk_origstr_t
*
* INPUTS:
*  origstr == original token string struct to delete
*********************************************************************/
static void
    free_origstr (tk_origstr_t *origstr)
{
    if (origstr->str != NULL) {
        m__free(origstr->str);
    }
    m__free(origstr);

}  /* free_origstr */


/********************************************************************
* FUNCTION new_token_ptr
* 
* Allocatate a new token pointer 
*
* INPUTS:
*  tk == token to copy
*  field == field key to save
*
* RETURNS:
*   new token pointer struct or NULL if some error
*********************************************************************/
static tk_token_ptr_t *
    new_token_ptr (tk_token_t *tk, 
                   const void *field)
{
    tk_token_ptr_t  *tkptr;

    tkptr = m__getObj(tk_token_ptr_t);
    if (!tkptr) {
        return NULL;
    }
    memset(tkptr, 0x0, sizeof(tk_token_ptr_t));
    tkptr->tk = tk;
    tkptr->field = field;
    return tkptr;
    
} /* new_token_ptr */


/********************************************************************
* FUNCTION free_token_ptr
* 
* Free a token pointer struct
*
* INPUTS:
*  tkptr == token pointer to free
*********************************************************************/
static void
    free_token_ptr (tk_token_ptr_t *tkptr)
{
    m__free(tkptr);
} /* free_token_ptr */


/********************************************************************
* FUNCTION new_token
* 
* Allocatate a new token with a value string that will be copied
*
* INPUTS:
*  ttyp == token type
*  tval == token value or NULL if not used
*  tlen == token value length if used; Ignored if tval == NULL
*
* RETURNS:
*   new token or NULL if some error
*********************************************************************/
static tk_token_t *
    new_token (tk_type_t ttyp, 
               const xmlChar *tval,
               uint32  tlen)
{
    tk_token_t  *tk;

    tk = m__getObj(tk_token_t);
    if (!tk) {
        return NULL;
    }
    memset(tk, 0x0, sizeof(tk_token_t));
    tk->typ = ttyp;
    if (tval) {
        tk->len = tlen;
        tk->val = xml_strndup(tval, tlen);
        if (!tk->val) {
            m__free(tk);
            return NULL;
        }
    }
    dlq_createSQue(&tk->origstrQ);

#ifdef TK_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\ntk: new token (%s) ", tk_get_token_name(ttyp));
        if (tval) {
            while (tlen--) {
                log_debug4("%c", *tval++);
            }
        } else {
            log_debug4("%s", tk_get_token_sym(ttyp));
        }
    }
#endif
            
    return tk;
    
} /* new_token */


/********************************************************************
* FUNCTION new_mtoken
* 
* Allocatate a new token with a value string that will be
* consumed and freed later, and not copied 
*
* INPUTS:
*  ttyp == token type
*  tval == token value or NULL if not used
*
* RETURNS:
*   new token or NULL if some error
*********************************************************************/
static tk_token_t *
    new_mtoken (tk_type_t ttyp, 
               xmlChar *tval)
{
    tk_token_t  *tk;

    tk = m__getObj(tk_token_t);
    if (!tk) {
        return NULL;
    }
    memset(tk, 0x0, sizeof(tk_token_t));
    tk->typ = ttyp;
    if (tval) {
        tk->len = xml_strlen(tval);
        tk->val = tval;
    }
    dlq_createSQue(&tk->origstrQ);

#ifdef TK_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\ntk: new mtoken (%s) ", tk_get_token_name(ttyp));
        if (tval) {
            while (tlen--) {
                log_debug4("%c", *tval++);
            }
        } else {
            log_debug4("%s", tk_get_token_sym(ttyp));
        }
    }
#endif
            
    return tk;
    
} /* new_mtoken */


/********************************************************************
* FUNCTION free_token
* 
* Cleanup and deallocate a tk_token_t 
*
* RETURNS:
*  none
*********************************************************************/
static void 
    free_token (tk_token_t *tk)
{
    tk_origstr_t  *origstr;

#ifdef TK_DEBUG
    if (LOGDEBUG4) {
        log_debug4("\ntk: free_token: (%s)", tk_get_token_name(tk->typ));
        if (tk->val) {
            log_debug4(" val=(%s) ", tk->val);
        }
    }
#endif

    if (tk->mod) {
        m__free(tk->mod);
    }
    if (tk->val) {
        m__free(tk->val);
    }
    if (tk->origval) {
        m__free(tk->origval);
    }

    while (!dlq_empty(&tk->origstrQ)) {
        origstr = (tk_origstr_t *)dlq_deque(&tk->origstrQ);
        free_origstr(origstr);
    }

    m__free(tk);

} /* free_token */


/********************************************************************
* FUNCTION new_token_wmod
* 
* Allocatate a new identifier token with a <name> qualifier
* <name)> is a module name in NCX
*         is a prefix name in YANG
*
* INPUTS:
*  ttyp == token type
*  mod == module name string, not z-terminated
*  modlen == 'mod' string length
*  tval == token value
*  tlen == token value length
*
* RETURNS:
*   new token or NULL if some error
*********************************************************************/
static tk_token_t *
    new_token_wmod (tk_type_t ttyp, 
                       const xmlChar *mod,
                       uint32 modlen,
                       const xmlChar *tval, 
                       uint32 tlen)
{
    tk_token_t  *ret;

    ret = new_token(ttyp, tval, tlen);
    if (ret) {
        ret->modlen = modlen;
        ret->mod = xml_strndup(mod, modlen);
        if (!ret->mod) {
            free_token(ret);
            return NULL;
        }
    }

#ifdef TK_DEBUG
    if (LOGDEBUG3) {
        log_debug3("  mod: ");
        while (modlen--) {
            log_debug3("%c", *mod++);
        }
    }
#endif

    return ret;
}  /* new_token_wmod */


/********************************************************************
* FUNCTION add_new_token
* 
* Allocatate a new token with the specified type
* Use the token chain directly
*
*    tkc->bptr == first char in the value string to save
* 
*    Add the token to the chain if no error
*    Update the buffer pointer to 'str' after token is added OK
*
* INPUTS:
*    tkc == token chain
*    ttyp == token type to create
*    str == pointer to the first char after the last char in the value
*           value includes chars from bptr to str-1
*        == NULL if no value for this token
*    startpos == line position where this token started
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    add_new_token (tk_chain_t *tkc,
                   tk_type_t ttyp,
                   const xmlChar *str,
                   uint32 startpos)
{
    tk_token_t  *tk;
    uint32       total;

    tk = NULL;
    total = (str) ? (uint32)(str - tkc->bptr) : 0;

    if (total > NCX_MAX_STRLEN) {
        return ERR_NCX_LEN_EXCEEDED;
    } else if (total == 0) {
        /* zero length value strings are allowed */
        tk = new_token(ttyp, NULL, 0);
    } else {
        /* normal case string -- non-zero length */
        tk = new_token(ttyp, tkc->bptr, total);
    }
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }
    tk->linenum = tkc->linenum;
    tk->linepos = startpos;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;

}  /* add_new_token */

/********************************************************************
* FUNCTION consume_escaped_char
* 
* Consume and format an escaped character encountered by add_new_qtoken.
* INPUTS:
*    dest == the destination buffer
*    src  == the source buffer
*    endstr == the end of the source buffer
*
* RETURNS:
*    TRUE if source type is TK_SOURCE_XPATH
*********************************************************************/
static void consume_escaped_char( xmlChar **dest, 
        const xmlChar **src, const xmlChar *endstr )
{
    const xmlChar* instr = *src;
    xmlChar* outstr = *dest;

    if (instr+1 != endstr )
    {
        switch (instr[1]) {
        case 'n':
            *outstr++ = '\n';
            break;
        case 't':
            *outstr++ = '\t';
            break;
        case '"':
            *outstr++ = '"';
            break;
        case '\\':
            *outstr++ = '\\';
            break;
        case '\0':
            /* let the next loop exit on EO-buffer */
            break;
        default:
            /* pass through the escape sequence */
            *outstr++ = '\\';
            *outstr++ = instr[1];
        }
        
        /* adjust the in pointer if not EO-buffer */
        if (instr[1]) {
            instr += 2;
        } else {
            ++instr;
        }
    }

    *src = instr;
    *dest = outstr;
}

/********************************************************************
* FUNCTION is_xpath_string
* 
* Helper function that helps improve code clarity. It simply returns 
* true if sourceType is TK_SOURCE_XPATH. 
*
* Note: This function is likely to be inlined by the compiler.
*
* INPUTS:
*    sourceType == the source type to test.
*
* RETURNS:
*    TRUE if source type is TK_SOURCE_XPATH
*********************************************************************/
static boolean is_xpath_string( const tk_source_t sourceType ) 
{
    return TK_SOURCE_XPATH == sourceType; 
}

/********************************************************************
* FUNCTION is_newline_char
* 
* Helper function that helps improve code clarity. It simply returns 
* true if ch is a newline character. 
*
* Note: This function is likely to be inlined by the compiler.
*
* INPUTS:
*    ch == the character to test.
*
* RETURNS:
*    TRUE if ch is '\n'
*********************************************************************/
static boolean is_newline_char( const xmlChar ch ) 
{
    return '\n' == ch; 
}

/********************************************************************
* FUNCTION is_space_or_tab
* 
* Helper function that helps improve code clarity. It simply returns 
* true if ch is a space or tab character. 
*
* Note: This function is likely to be inlined by the compiler.
*
* INPUTS:
*    ch == the character to test.
*
* RETURNS:
*    TRUE if ch is '\n'
*********************************************************************/
static boolean is_space_or_tab( const xmlChar ch ) 
{
    return '\t' == ch || ' ' == ch;
}

/********************************************************************
* FUNCTION trim_trailing_whitespace
* 
* Helper function that removes any whitespace from the end of a buffer.
*
* INPUTS:
*    buffer == the destination buffer to trim
*    endbuffer  == the end of the destination buffer, this should be one 
*                  past the newline character at the end of the buffer
*
* RETURNS:
*    The new end of the destination buffer
*********************************************************************/
static xmlChar* 
    trim_trailing_whitespace( xmlChar *buffer, xmlChar *endbuffer )
{
    --endbuffer;
    if ( *endbuffer != '\n' ) {
        return ++endbuffer; // nothing to trim
    }
    --endbuffer;           // skip back to first character before the newline 

    while ( endbuffer >= buffer && is_space_or_tab( *endbuffer) ) {  
        --endbuffer;
    }
    ++endbuffer;           // skip non whitespace character
    *endbuffer++ = '\n';   // add a newline
    return endbuffer;
}

/********************************************************************
* FUNCTION skip_leading_whitespace_src
* 
* Skip leading whitespace in the supplied buffer
*
* INPUTS:
*    buffer == the buffer to skip space from
*    endstr  == the end of the buffer, this should be one 
*                  past the newline character at the end of the buffer
*
* RETURNS:
*    The number of characters skipped.
*********************************************************************/
static uint32
    skip_leading_whitespace_src( const xmlChar **src, 
                                 const xmlChar *endstr ) 
{
    const xmlChar* instr = *src;

    uint32 count = 0;

    while ( instr < endstr ) {
        if ( ' ' == *instr ) {
            ++count;
        } else if ( '\t' == *instr ) {
            count += NCX_TABSIZE;
        }
        else {
            break;
        }
        ++instr;
    }

    *src = instr;
    return count;
}

/********************************************************************
* FUNCTION format_leading_whitespace
* 
* Add leading whitespace to a destination buffer after a newline was
* encountered during a copy operation.
*
* INPUTS:
*    destptr == the destination buffer 
*    srcptr == the source buffer 
*    endstr  == the end of the buffer, this should be one 
*                  past the newline character at the end of the buffer
*    indent == line position where this token started
*********************************************************************/
static void
    format_leading_whitespace( xmlChar **destptr,
                               const xmlChar **srcptr,
                               const xmlChar  *endstr,
                               const uint32    indent )
{
    uint32 numLeadingSpaceChars = 0;
    const xmlChar* instr = *srcptr;
    xmlChar* outstr = *destptr;

    // get the number of leading whitespaces on the next line
    numLeadingSpaceChars = skip_leading_whitespace_src( &instr, 
            endstr );

    // linepos is the indent total for the next line subtract 
    // the start position and indent the rest
    if ( numLeadingSpaceChars > indent ) {
        uint32 totReqSpaces = numLeadingSpaceChars - indent;
        uint32 numTabs = totReqSpaces / NCX_TABSIZE;
        uint32 numSpaces = totReqSpaces % NCX_TABSIZE;
        uint32 numConsumedChars = (uint32)(instr - *srcptr);

        /* make sure not to write more chars than were read E.g: do not replace
         * 2 tabs with 7 spaces and over flow the buffer */
        if ( numTabs + numSpaces > numConsumedChars ) {
            if ( numTabs > numConsumedChars ) {
                numTabs = numConsumedChars;
                numSpaces = 0;
            }
            else {
                numSpaces = numConsumedChars - numTabs;
            }
        }

        while ( numTabs ) {
            *outstr++ = '\t';
            --numTabs;
        }

        while ( numSpaces ) {
            *outstr++ = ' ';
            --numSpaces;
        }
    }
    *destptr = outstr;
    *srcptr = instr;
}


/********************************************************************
* FUNCTION copy_and_format_token_str
* 
* Copy the supplied TK_TT_QSTRING token, converting any escaped chars 
* in the source buffer. 
*
* Adjust the leading and trailing whitespace around newline characters
*
* INPUTS:
*    dest == A buffer large enough to store the copy characters. 
*            This must be at least as long as the source string.
*    src  == The source string to copy
*    endstr == pointer to the first char after the last char in the value
*              src string.
*    is_xpath == flag indicating of this is an xpath
*    indent == line position where this token started
*********************************************************************/
static void
    copy_and_format_token_str( xmlChar          *dest, 
                               const xmlChar    *src, 
                               const xmlChar    *endstr,
                               const boolean     is_xpath,
                               const uint32      indent )
{
    xmlChar *outstr = dest;
    const xmlChar *instr = src;

    while (instr < endstr) {
    
        /* translate escape char or copy regular char */
        if (*instr == '\\') {
            consume_escaped_char( &outstr, &instr, endstr );
        } else {
            *outstr++ = *instr++;
        }
    
        /* check if last char written was a newline,
         * DO NOT ADJUST XPATH STRINGS */
        if ( !is_xpath ) {
            if ( is_newline_char( *(outstr-1) ) ) {
                // skip back to the first character before the newline
                outstr = trim_trailing_whitespace( dest, outstr );

                format_leading_whitespace( &outstr, &instr, endstr, indent );
            }
        }
    }

    /* finish the string */
    *outstr = 0;
}

/********************************************************************
* FUNCTION add_new_qtoken
* 
* Allocatate a new TK_TT_QSTRING token
* Use the token chain directly
* Convert any escaped chars in the buffer first
* Adjust the leading and trailing whitespace around
* newline characters
*
*    tkc->bptr == first char in the value string to save
* 
*    Add the token to the chain if no error
*    Update the buffer pointer to 'str' after token is added OK
*
* INPUTS:
*    tkc == token chain to add new token to
*    isdouble == TRUE for TK_TT_QSTRING
*             == FALSE for TK_TT_SQSTRING
*    tkbuff == token input buffer to use
*    endstr == pointer to the first char after the last char in the value
*              value includes chars from bptr to str-1
*           == NULL if no value for this token
*    startline == line number where this token started
*    startpos == line position where this token started
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    add_new_qtoken ( tk_chain_t *tkc,
                     boolean isdouble,
                     xmlChar *tkbuff,
                     const xmlChar *endstr,
                     uint32 startline,
                     uint32 startpos )
{
    tk_token_t    *tk = NULL;
    xmlChar *origbuff = NULL; 
    uint32         total = (endstr) ? (uint32)(endstr - tkbuff) : 0;
    
    if (total > NCX_MAX_STRLEN) {
        return ERR_NCX_LEN_EXCEEDED;
    } 

    if (total == 0) {
        /* zero length value strings are allowed */
        tk = (isdouble) ? new_token( TK_TT_QSTRING,  NULL, 0) 
                        : new_token( TK_TT_SQSTRING, NULL, 0);
    } else if (!isdouble) {
        /* single quote string */
        tk = new_token( TK_TT_SQSTRING, tkbuff, total );
    } else {
        /* double quote normal case -- non-zero length QSTRING fill the buffer, 
         * while converting escaped chars */
        xmlChar *buff = (xmlChar *)m__getMem(total+1);
        if (!buff) {
            return ERR_INTERNAL_MEM;
        }

        copy_and_format_token_str( buff, tkbuff, endstr, 
                                   is_xpath_string( tkc->source), startpos );

        /* if --format=html or --format=yang then a copy of the original double 
         * quoted string needs to be saved unaltered according to the 
         * YANG spec */
        if (TK_DOCMODE(tkc)) {
            origbuff = xml_strndup(tkbuff, total);
            if ( !origbuff ) {
                return ERR_INTERNAL_MEM;
            }
        }

        tk = new_mtoken(TK_TT_QSTRING, buff);
        if ( !tk ) {
            m__free(buff);
            m__free(origbuff);
        }
    }

    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = startline;
    tk->linepos = startpos;
    tk->origval = origbuff;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
}  /* add_new_qtoken */


/********************************************************************
* FUNCTION get_token_id
* 
* Check if the spceified string is a NCX non-string token
* Checks for 1-char and 2-char tokens only!!
*
* INPUTS:
*   buff == token string to check -- NOT ZERO-TERMINATED
*   len == length of string to check
*   srctyp == parsing source
*    (TK_SOURCE_CONF, TK_SOURCE_YANG, TK_SOURCE_CONF, TK_SOURCE_REDO)
*
* RETURNS:
*   token type found or TK_TT_NONE if no match
*********************************************************************/
static tk_type_t 
    get_token_id (const xmlChar *buff, 
                  uint32 len,
                  tk_source_t srctyp)
{
    tk_type_t t;
    uint32    flags;

    /* determine which subset of the token list will be checked */
    switch (srctyp) {
    case TK_SOURCE_CONF:
        flags = FL_CONF;
        break;
    case TK_SOURCE_YANG:
        flags = FL_YANG;
        break;
    case TK_SOURCE_XPATH:
        flags = FL_XPATH;
        break;
    case TK_SOURCE_REDO:
        flags = FL_REDO;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return TK_TT_NONE;
    }

    /* look in the tlist for the specified token 
     * This is an optimized hack to save time, instead
     * of a complete search through all data types
     *
     * !!! MAKE SURE THE FOR LOOPS BELOW ARE UPDATED
     * !!! IF THE tk_token_t ENUMERATION IS CHANGED
     * 
     * Only tokens for the specified source language
     * and the specified length will be returned
     */
    if (len==1) {
        for (t=TK_TT_LBRACE; t <= TK_TT_GT; t++) {
            if (*buff == *tlist[t].tid && (tlist[t].flags & flags)) {
                return tlist[t].ttyp;
            }
        }
    } else if (len==2) {
        for (t=TK_TT_RANGESEP; t <= TK_TT_GEQUAL; t++) {
            if (!xml_strncmp(buff, (const xmlChar *)tlist[t].tid, 2) &&
                (tlist[t].flags & flags)) {
                return tlist[t].ttyp;
            }
        }
    }
    return TK_TT_NONE;

} /* get_token_id */


/********************************************************************
* FUNCTION tokenize_qstring
* 
* Handle a double-quoted string ("string")
*
* Converts escaped chars during this first pass processing
*
* Does not realign whitespace in this function
* That is done when strings are concatenated
* with finish_qstrings
*
* INPUTS:
*   tkc == token chain 
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_qstring (tk_chain_t *tkc)
{
    xmlChar      *str, *tempbuff, *outstr;
    uint32        total, startline, startpos, linelen;
    boolean       done, done2;
    status_t      res;

    startline = tkc->linenum;
    startpos = tkc->linepos;

    /* the bptr is pointing at the quote char which
     * indicates the start of a quoted string
     * find the end of the string of end of the line/buffer
     * start bptr after the quote since that is not saved
     * as part of the content, and doesn't count againt maxlen
     *
     * This first loop does not process chars yet
     * because most quoted strings do not contain the
     * escaped chars, and an extra copy is not needed if this is
     * a simple quoted string on a single line
     */
    str = ++tkc->bptr;      /* skip over escaped double quotes */
    done = FALSE;    
    while (!done) {
        if (!*str) {
            /* End of buffer */
            done = TRUE;
        } else if (*str == NCX_QSTRING_CH && (*(str-1) != '\\')) {
            /* ending double quote */
            done = TRUE;
        } else {
            if (*str == '\\') {
                if (str[1]) {
                    str++;
                }
                if (*str == 'n') {
                    tkc->linepos = 1;
                } else if (*str =='t') {
                    tkc->linepos += NCX_TABSIZE;
                } else {
                    tkc->linepos++;
                }
            } else {
                if (*str == '\n') {
                    tkc->linepos = 1;
                } else if (*str =='\t') {
                    tkc->linepos += NCX_TABSIZE;
                } else {
                    tkc->linepos++;
                }
            }
            str++;
        }
    }

    total = (uint32)(str - tkc->bptr);

    if (*str == NCX_QSTRING_CH) {
        /* easy case, a quoted string on 1 line */
        res = add_new_qtoken(tkc, 
                             TRUE, 
                             tkc->bptr, 
                             str, 
                             startline, 
                             startpos);
        tkc->bptr = str+1;
        return res;
    }

    /* else we reached the end of the buffer without
     * finding the QSTRING_CH. If input from buffer this is 
     * an error
     */
    if (!(tkc->flags & TK_FL_MALLOC)) {
        return ERR_NCX_UNENDED_QSTRING;
    }

    /* FILE input, keep reading lines looking for QUOTE_CH 
     * need to get a temp buffer to store all the lines 
     */
    tempbuff = (xmlChar *)m__getMem(NCX_MAX_Q_STRLEN+1);
    if (!tempbuff) {
        return ERR_INTERNAL_MEM;
    }

    outstr = tempbuff;

    /* check total bytes parsed, and copy into temp buff if any */
    if (total) {
        outstr += xml_strcpy(outstr, tkc->bptr);
    } else {
        outstr[0] = 0;
    }

    /* ELSE real special case, start of QSTRING last char in the line 
     * This should not happen because a newline should at 
     * least follow the QSTRING char 
     */

    /* keep saving lines in tempbuff until the QSTRING_CH is found */
    done = FALSE;
    while (!done) {
        if (!fgets((char *)tkc->buff, TK_BUFF_SIZE, tkc->fp)) {
            /* read line failed -- assume EOF */
            m__free(tempbuff);
            return ERR_NCX_UNENDED_QSTRING;
        } else {
            tkc->linenum++;
            tkc->linepos = 1;
            tkc->bptr = tkc->buff;
        }

#ifdef TK_RDLN_DEBUG
        if (LOGDEBUG3) {
            if (xml_strlen(tkc->buff) < 128) {
                log_debug3("\nNCX Parse: read line (%s)", tkc->buff);
            } else {
                log_debug3("\nNCX Parse: read line len  (%u)", 
                           xml_strlen(tkc->buff));
            }
        }
#endif

        /* look for ending quote on this line */
        str = tkc->bptr;
        done2 = FALSE;
        while (!done2) {
            if (!*str) {
                done2 = TRUE;
            } else if (*str == NCX_QSTRING_CH && (*(str-1) != '\\')) {
                done2 = TRUE;
            } else {
                str++;
            }
        }

        linelen = (uint32)(str-tkc->bptr);

        if (*str) {
            tkc->linepos = linelen+1;
            done = TRUE;   /* stopped on a double quote */
        }

        if (linelen + total < NCX_MAX_Q_STRLEN) {
            /* copy this line to tempbuff */
            outstr += xml_strncpy(outstr, tkc->bptr, linelen);
            total += linelen;
            tkc->bptr = str+1;
        } else {
            /* would be a buffer overflow */
            m__free(tempbuff);
            return ERR_NCX_LEN_EXCEEDED;
        }
    }

    res = add_new_qtoken(tkc, 
                         TRUE, 
                         tempbuff, 
                         outstr, 
                         startline, 
                         startpos);
    m__free(tempbuff);
    return res;

}  /* tokenize_qstring */


/********************************************************************
* FUNCTION tokenize_sqstring
* 
* Handle a single-quoted string ('string')
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_sqstring (tk_chain_t *tkc)
{
    xmlChar      *str, *tempbuff, *outstr;
    uint32        total, startline, startpos, linelen;
    status_t      res;
    boolean       done;

    startline = tkc->linenum;
    startpos = tkc->linepos;
    
    /* the bptr is pointing at the quote char which
     * indicates the start of a quoted string
     * find the end of the string of end of the line/buffer
     * start bptr after the quote since that is not saved
     * as part of the content, and doesn't count againt maxlen
     */
    str = ++tkc->bptr;
    while (*str && (*str != NCX_SQSTRING_CH)) {
        str++;
    }

    total = (uint32)(str - tkc->bptr);

    if (*str == NCX_SQSTRING_CH) {
        /* easy case, a quoted string on 1 line; */
        res = add_new_token(tkc, TK_TT_SQSTRING, str, startpos);
        tkc->bptr = str+1;
        tkc->linepos += total+2;
        return NO_ERR;
    }

    /* else we reached the end of the buffer without
     * finding the SQSTRING_CH. If input from buffer this is 
     * an error
     */
    if (!(tkc->flags & TK_FL_MALLOC)) {
        return ERR_NCX_UNENDED_QSTRING;
    }

    /* FILE input, keep reading lines looking for QUOTE_CH 
     * need to get a temp buffer to store all the lines 
     */
    tempbuff = (xmlChar *)m__getMem(NCX_MAX_Q_STRLEN+1);
    if (!tempbuff) {
        return ERR_INTERNAL_MEM;
    }

    outstr = tempbuff;

    /* check total bytes parsed, and copy into temp buff if any */
    total = (uint32)(str - tkc->bptr);
    if (total) {
        outstr += xml_strcpy(outstr, tkc->bptr);
    } else {
        outstr[0] = 0;
    }

    /* keep saving lines in tempbuff until the QSTRING_CH is found */
    done = FALSE;
    while (!done) {
        if (!fgets((char *)tkc->buff, TK_BUFF_SIZE, tkc->fp)) {
            /* read line failed -- assume EOF */
            m__free(tempbuff);
            return ERR_NCX_UNENDED_QSTRING;
        } else {
            tkc->linenum++;
            tkc->linepos = 1;
            tkc->bptr = tkc->buff;
        }

#ifdef TK_RDLN_DEBUG
        if (LOGDEBUG3) {
            if (xml_strlen(tkc->buff) < 128) {
                log_debug3("\nNCX Parse: read line (%s)", tkc->buff);
            } else {
                log_debug3("\nNCX Parse: read line len  (%u)", 
                           xml_strlen(tkc->buff));
            }
        }
#endif
        str = tkc->bptr;
        while (*str && (*str != NCX_SQSTRING_CH)) {
            str++;
        }

        linelen = (uint32)(str-tkc->bptr);
        
        if (*str) {
            tkc->bptr = str+1;
            tkc->linepos = linelen+1;
            done = TRUE;   /* stopped on a single quote */
        }

        if (linelen + total < NCX_MAX_Q_STRLEN) {
            outstr += xml_strncpy(outstr, tkc->bptr, linelen);
            total += linelen;
        } else {
            m__free(tempbuff);
            return ERR_NCX_LEN_EXCEEDED;
        }
    }

    /* get a token and save the SQSTRING */
    res = add_new_qtoken(tkc, 
                         FALSE, 
                         tempbuff,
                         outstr, 
                         startline, 
                         startpos);
    m__free(tempbuff);
    return res;

}  /* tokenize_sqstring */


/********************************************************************
* FUNCTION skip_yang_cstring
* 
* Handle a YANG multi-line comment string   TK_TT_COMMENT
* Advance current buffer pointer past the comment
*
* INPUTS:
*   tkc == token chain 
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    skip_yang_cstring (tk_chain_t *tkc)
{
    xmlChar   *str;
    boolean    done;

    /* the bptr is pointing at the comment char which
     * indicates the start of a C style comment
     */
    tkc->linepos += 2;
    tkc->bptr += 2;

    /* look for star-slash '*' '/' EO comment sequence */
    str = tkc->bptr;
    while (*str && !(*str=='*' && str[1]=='/')) {
        if (*str == '\t') {
            tkc->linepos += NCX_TABSIZE;
        } else {
            tkc->linepos++;
        }
        str++;
    }

    if (*str) {
        /* simple case, stopped at end of 1 line comment */
        tkc->bptr = str+2;
        tkc->linepos += 2;
        return NO_ERR;
    }

    /* else stopped at end of buffer, get rest of multiline comment */
    if (!(tkc->flags & TK_FL_MALLOC)) {
        return ERR_NCX_UNENDED_COMMENT;
    }

    /* FILE input, keep reading lines looking for the
     * end of comment
     */
    done = FALSE;
    while (!done) {
        if (!fgets((char *)tkc->buff, TK_BUFF_SIZE, tkc->fp)) {
            /* read line failed -- assume EOF */
            return ERR_NCX_UNENDED_COMMENT;
        } else {
            tkc->linenum++;
            tkc->linepos = 1;
            tkc->bptr = tkc->buff;
        }

#ifdef TK_RDLN_DEBUG
        if (LOGDEBUG3) {
            if (xml_strlen(tkc->buff) < 128) {
                log_debug3("\nNCX Parse: read line (%s)", tkc->buff);
            } else {
                log_debug3("\nNCX Parse: read line len  (%u)", 
                           xml_strlen(tkc->buff));
            }
        }
#endif

        str = tkc->bptr;
        while (*str && !(*str=='*' && str[1]=='/')) {
            if (*str == '\t') {
                tkc->linepos += NCX_TABSIZE;
            } else {
                tkc->linepos++;
            }
            str++;
        }

        if (*str) {
            tkc->linepos += 2;
            tkc->bptr = str+2;
            done = TRUE;   /* stopped on an end of comment */
        }
    }

    return NO_ERR;

}  /* skip_yang_cstring */


/********************************************************************
* FUNCTION tokenize_number
* 
* Handle some sort of number string
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_number (tk_chain_t *tkc)
{
    xmlChar      *str;
    uint32        startpos, total;
    status_t      res;

    startpos = tkc->linepos;
    str = tkc->bptr;

    /* the bptr is pointing at the first number char which
     * is a valid (0 - 9) digit, or a + or - char
     * get the sign first
     */
    if (*str == '+' || *str == '-') {
        str++;
    }

    /* check if this is a hex number */
    if (*str == '0' && NCX_IS_HEX_CH(str[1])) {
        /* move the start of number portion */
        str += 2;
        while (isxdigit(*str)) {
            str++;
        }

        total = (uint32)(str - tkc->bptr);

        /* make sure we ended on a proper char and have a proper len */
        if (isalpha(*str) || str==tkc->bptr || total > NCX_MAX_HEXCHAR) {
            return ERR_NCX_INVALID_HEXNUM;
        }

        res = add_new_token(tkc, TK_TT_HNUM, str, startpos);
        tkc->bptr = str;
        tkc->linepos += total;
        return res;
    }

    /* else not a hex number so just find the end or a '.' */
    while (isdigit(*str)) {
        str++;
    }

    /* check if we stopped on a dot, indicating a real number 
     * make sure it's not 2 dots, which is the range separator
     */
    if (*str == '.' && str[1] != '.') {
        /* this is a real number, get the rest of the value */
        str++;     /* skip dot char */
        while (isdigit(*str)) {
            str++;
        }

        total = (uint32)(str - tkc->bptr);

        /* make sure we ended on a proper char and have a proper len
         * This function does not support number entry in
         * scientific notation, just numbers with decimal points
         */
        if (isalpha(*str) || *str=='.' || total > NCX_MAX_RCHAR) {
            return ERR_NCX_INVALID_REALNUM;
        }
        
        res = add_new_token(tkc, TK_TT_RNUM, str, startpos);
        tkc->bptr = str;
        tkc->linepos += total;
        return res;
    }

    /* else this is a decimal number */
    total = (uint32)(str - tkc->bptr);
                          
    /* make sure we ended on a proper char and have a proper len */
    if (isalpha(*str) || total > NCX_MAX_DCHAR) {
        return ERR_NCX_INVALID_NUM;
    }

    res = add_new_token(tkc, TK_TT_DNUM, str, startpos);
    tkc->bptr = str;
    tkc->linepos += total;
    return res;

}  /* tokenize_number */


/********************************************************************
* FUNCTION get_name_comp
* 
* Get a name component
* This will stop the identifier name on the first non-name char
* whatever it is.
*
* INPUTS:
*   str == start of name component
* OUTPUTS
*   *len == length of valid name component, if NO_ERR
* RETURNS:
*   status
*********************************************************************/
static status_t
    get_name_comp (const xmlChar *str, 
                   uint32 *len)
{
    const xmlChar *start;

    start = str;
    *len = 0;

    if (!ncx_valid_fname_ch(*str)) {
        return ERR_NCX_INVALID_NAME;
    }
        
    str++;
    while (ncx_valid_name_ch(*str)) {
        str++;
    }
    if ((str - start) < NCX_MAX_NLEN) {
        /* got a valid identifier fragment */
        *len = (uint32)(str - start);
        return NO_ERR;
    } else {
        return ERR_NCX_LEN_EXCEEDED;
    }
    /*NOTREACHED*/

}  /* get_name_comp */


/********************************************************************
* FUNCTION finish_string
* 
* Finish off a suspected identifier as a plain string
*
* INPUTS:
*   tkc == token chain
*   str == rest of string to process
* RETURNS:
*   status
*********************************************************************/
static status_t
    finish_string (tk_chain_t  *tkc,
                   xmlChar *str)
{
    boolean      done;
    tk_type_t    ttyp;
    uint32       startpos, total;
    status_t     res;

    startpos = tkc->linepos;

    done = FALSE;
    while (!done) {
        if (!*str) {
            done = TRUE;
        } else if (xml_isspace(*str) || *str=='\n') {
            done = TRUE;
        } else if (tkc->source == TK_SOURCE_YANG) {
            ttyp = get_token_id(str, 1, tkc->source);
            switch (ttyp) {
            case TK_TT_SEMICOL:
            case TK_TT_LBRACE:
            case TK_TT_RBRACE:
                done = TRUE;
                break;
            default:
                ;
            }
        } else if (get_token_id(str, 1, tkc->source) != TK_TT_NONE) {
            done = TRUE;
        }

        if (!done) {
            if (*str == '\t') {
                tkc->linepos += NCX_TABSIZE;
            } else {
                tkc->linepos++;
            }
            str++;
        }
    }

    total = (uint32)(str - tkc->bptr);
    if (total > NCX_MAX_Q_STRLEN) {
        return ERR_NCX_LEN_EXCEEDED;
    }

    res = add_new_token(tkc, TK_TT_STRING, str, startpos);
    tkc->bptr = str;    /* advance the buffer pointer */
    return res;

}  /* finish_string */


/********************************************************************
* FUNCTION tokenize_varbind_string
* 
* Handle some sort of $string, which could be a varbind string
* in the form $QName 
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_varbind_string (tk_chain_t *tkc)
{
    xmlChar        *str;
    const xmlChar  *prefix, *name, *item;
    tk_token_t     *tk;
    uint32          len, prelen;
    status_t        res;
    
    prefix = NULL;
    name = NULL;
    item = NULL;
    prelen = 0;

    /* the bptr is pointing at the dollar sign char */
    str = tkc->bptr+1;
    name = str;
    while (ncx_valid_name_ch(*str)) {
        str++;
    }

    /* check reasonable length for a QName */
    len = (uint32)(str - name);
    if (!len || len > NCX_MAX_NLEN) {
        return finish_string(tkc, str);
    }

    /* else got a string fragment that could be a valid ID format */
    if (*str == NCX_MODSCOPE_CH && str[1] != NCX_MODSCOPE_CH) {
        /* stopped on the module-scope-identifier token
         * the first identifier component must be a prefix
         * or possibly a module name 
         */
        prefix = tkc->bptr+1;
        prelen = len;
        item = ++str;

        /* str now points at the start of the imported item 
         * There needs to be at least one valid name component
         * after the module qualifier
         */
        res = get_name_comp(str, &len);
        if (res != NO_ERR) {
            return finish_string(tkc, str);
        }
        str += len;
        /* drop through -- either we stopped on a scope char or
         * the end of the module-scoped identifier string
         */
    } 

    if (prefix) {
        /* XPath $prefix:identifier */
        tk = new_token_wmod(TK_TT_QVARBIND,
                            prefix, 
                            prelen, 
                            item, 
                            (uint32)(str - item));
    } else {
        /* XPath $identifier */
        tk = new_token(TK_TT_VARBIND,  tkc->bptr+1, len);
    }

    if (!tk) {
        return ERR_INTERNAL_MEM;
    }
    tk->linenum = tkc->linenum;
    tk->linepos = tkc->linepos;
    dlq_enque(tk, &tkc->tkQ);

    len = (uint32)(str - tkc->bptr);
    tkc->bptr = str;    /* advance the buffer pointer */
    tkc->linepos += len;
    return NO_ERR;

}  /* tokenize_varbind_string */


/********************************************************************
* FUNCTION tokenize_id_string
* 
* Handle some sort of non-quoted string, which could be an ID string
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_id_string (tk_chain_t *tkc)
{
    xmlChar        *str;
    const xmlChar  *prefix, *item;
    boolean         scoped, namestar;
    tk_token_t     *tk;
    uint32          len, prelen;
    status_t        res;

    prefix = NULL;
    item = NULL;
    scoped = FALSE;
    namestar = FALSE;
    tk = NULL;
    prelen = 0;

    /* the bptr is pointing at the first string char which
     * is a valid identifier first char; start at the next char
      */
    str = tkc->bptr+1;
    if (tkc->source == TK_SOURCE_REDO) {
        /* partial ID syntax; redo so range clause 
         * components are not included 
         */
        while ((*str != '.') && (*str != '-') && 
               ncx_valid_name_ch(*str)) {
            str++;
        }
    } else {
        /* allow full YANG identifier syntax */
        while (ncx_valid_name_ch(*str)) {
            str++;
        }
    }

    /* check max identifier length */
    if ((str - tkc->bptr) > NCX_MAX_NLEN) {
        /* string is too long to be an identifier so just
         * look for any valid 1-char token or whitespace to end it.  
         *
         * This will ignore any multi-char tokens that may be embedded, 
         * but these unquoted strings are a special case
         * and this code depends on the fact that there are
         * no valid NCX token sequences in which a string is
         * followed by a milti-char token without any 
         * whitespace in between.
         */
        return finish_string(tkc, str);
    }

    /* check YANG parser stopped on proper end of ID */
    if (tkc->source == TK_SOURCE_YANG && 
        !(*str=='{' || *str==';' || *str == '/' || *str==':'
          || xml_isspace(*str))) {
        return finish_string(tkc, str);
    }

    /* else got a string fragment that could be a valid ID format */
    if (*str == NCX_MODSCOPE_CH && str[1] != NCX_MODSCOPE_CH) {
        /* stopped on the prefix-scope-identifier token
         * the first identifier component must be a prefix name 
         */
        prefix = tkc->bptr;
        prelen = (uint32)(str - tkc->bptr);
        item = ++str;

        if (tkc->source == TK_SOURCE_XPATH && *item == '*') {
            namestar = TRUE;
            str++;                   /* consume the '*' char */
        } else {
            /* str now points at the start of the imported item 
             * There needs to be at least one valid name component
             * after the prefix qualifier
             */
            res = get_name_comp(str, &len);
            if (res != NO_ERR) {
                return finish_string(tkc, str);
            }

            /* if we stopped on a colon char then treat this as a URI */
            if (str[len] == ':') {
                return finish_string(tkc, str);
            }

            /* drop through -- either we stopped on a scope char or
             * the end of the prefix-scoped identifier string
             */
            str += len;
        }
    } 

    if (tkc->source != TK_SOURCE_XPATH) {
        /* got some sort of identifier string 
         * keep going until we don't stop on the scope char
         */
        while (*str == NCX_SCOPE_CH) {
            res = get_name_comp(++str, &len);
            if (res != NO_ERR) {
                return finish_string(tkc, str);
            }
            scoped = TRUE;
            str += len;
        }

        /* for Xpath purposes in YANG, treat scoped ID as a string now */
        if (scoped && tkc->source==TK_SOURCE_YANG) {
            return finish_string(tkc, str);
        }

        /* done with the string; create a token and save it */
        if (prefix) {
            if ((str - item) > NCX_MAX_Q_STRLEN) {
                return ERR_NCX_LEN_EXCEEDED;
            }
            tk = new_token_wmod(scoped ? TK_TT_MSSTRING : TK_TT_MSTRING,
                                prefix, 
                                prelen, 
                                item, 
                                (uint32)(str - item));
        } else {
            if ((str - tkc->bptr) > NCX_MAX_Q_STRLEN) {
                return ERR_NCX_LEN_EXCEEDED;
            }
            tk = new_token(scoped ? TK_TT_SSTRING : TK_TT_TSTRING,
                           tkc->bptr, 
                           (uint32)(str - tkc->bptr));
        }
    } else if (prefix) {
        if (namestar) {
            /* XPath 'prefix:*'  */
            tk = new_token(TK_TT_NCNAME_STAR,  prefix, prelen);
        } else {
            /* XPath prefix:identifier */
            tk = new_token_wmod(TK_TT_MSTRING,
                                prefix, 
                                prelen, 
                                item, 
                                (uint32)(str - item));
        }
    } else {
        /* XPath identifier */
        tk = new_token(TK_TT_TSTRING,  
                       tkc->bptr, 
                       (uint32)(str - tkc->bptr));
    }

    if (!tk) {
        return ERR_INTERNAL_MEM;
    }
    tk->linenum = tkc->linenum;
    tk->linepos = tkc->linepos;
    dlq_enque(tk, &tkc->tkQ);

    len = (uint32)(str - tkc->bptr);
    tkc->bptr = str;    /* advance the buffer pointer */
    tkc->linepos += len;
    return NO_ERR;

}  /* tokenize_id_string */


/********************************************************************
* FUNCTION tokenize_string
* 
* Handle some sort of non-quoted string, which is not an ID string
*
* INPUTS:
*   tkc == token chain 
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    tokenize_string (tk_chain_t *tkc)
{
    xmlChar  *str;

    /* the bptr is pointing at the first string char 
     * Don't need to check it.
     * Move past it, because if we got here, this is not
     * a valid token of any other kind.
     *
     * The 2nd pass of the parser may very well barf on this 
     * string token, or it could be something like a description
     * clause, where the content is irrelevant to the parser.
     */
    str = tkc->bptr+1;
    return finish_string(tkc, str);

}  /* tokenize_string */


/********************************************************************
* FUNCTION concat_qstrings
* 
* Go through the entire token chain and handle all the
* quoted string concatenation
*
* INPUTS:
*   tkc == token chain to adjust;
*          start token is the first token
*
* OUTPUTS:
*   tkc->tkQ will be adjusted as needed:
*   First string token in a concat chain will be modified
*   to include all the text from the chain.  All the other
*   tokens in the concat chain will be deleted
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t
    concat_qstrings (tk_chain_t *tkc)
{
    tk_token_t  *first, *plus, *prev, *next, *last;
    xmlChar     *buff, *str;
    uint32       bufflen;
    boolean      done;

    /* find the last consecutive quoted string
     * concatenate the strings together if possible
     * and redo the first quoted string to contain
     * the concatenation of the other strings
     *
     *  "string1" + "string2" + 'string3' ...
     */
    first = (tk_token_t *)dlq_firstEntry(&tkc->tkQ);
    while (first) {
        /* check if 'first' is a quoted string */
        if (!(first->typ==TK_TT_QSTRING ||
              first->typ==TK_TT_SQSTRING)) {
            first = (tk_token_t *)dlq_nextEntry(first);
            continue;
        }

        /* check if any string concat is requested
         * loop through the tokens following the string 'first'
         * and find the buffer length needed and the last token
         * in the "str1" + "str2" + "strn" sequence
         */
        last = NULL;
        next = (tk_token_t *)dlq_nextEntry(first);
        bufflen = first->len;
        done = FALSE;
        while (!done) {
            if (!next || next->typ != TK_TT_PLUS) {
                /* no token or not a '+', back to outer loop */
                done = TRUE;
            } else {
                /* found '+', should find another string next */
                plus = next;
                next = (tk_token_t *)dlq_nextEntry(next);
                if (!next || !(next->typ==TK_TT_QSTRING ||
                               next->typ==TK_TT_SQSTRING)) {
                    /* error, missing or wrong token */
                    tkc->cur = (next) ? next : plus;
                    return ERR_NCX_INVALID_CONCAT;
                } else {
                    /* OK, get rid of '+' and setup next loop */
                    last = next;
                    bufflen += last->len;
                    dlq_remove(plus);
                    free_token(plus);
                    next = (tk_token_t *)dlq_nextEntry(next);
                }
            }
        }

        if (last) {
            /* need to concat some strings */
            if (bufflen > NCX_MAX_STRLEN) {
                /* user intended one really big string (2 gig!) 
                 * but this cannot done so error exit
                 */
                tkc->cur = first;
                return ERR_NCX_LEN_EXCEEDED;
            }

            /* else fixup the consecutive strings
             * get a buffer to store the result
             */
            buff = (xmlChar *)m__getMem(bufflen+1);
            if (!buff) {
                tkc->cur = first;
                return ERR_INTERNAL_MEM;
            }

            /* copy the strings */
            str = buff;
            next = first;
            done = FALSE;
            while (!done) {
                str += xml_strcpy(str, next->val);
                if (next == last) {
                    done = TRUE;
                } else {
                    next = (tk_token_t *)dlq_nextEntry(next);
                }
            }

            if (TK_DOCMODE(tkc)) {
                /* make the Q of tk_origstr_t structs
                 * and add it to the &first->origstrQ
                 */
                tk_origstr_t  *origstr;
                boolean newline;

                prev = first;
                next = first;
                done = FALSE;
                while (!done) {
                    origstr = NULL;
                    if (next != first) {
                        xmlChar  *usestr;

                        if (next->origval != NULL) {
                            usestr = next->origval;
                        } else {
                            usestr = next->val;
                        }

                        newline = (prev->linenum != next->linenum);
                        origstr = new_origstr(next->typ,
                                              newline,
                                              usestr);
                        if (origstr == NULL) {
                            tkc->cur = first;
                            m__free(buff);
                            return ERR_INTERNAL_MEM;
                        }

                        /* transferred 'usestr' memory OK
                         * clear pointer to prevent double free
                         */
                        if (next->origval != NULL) {
                            next->origval = NULL;
                        } else {
                            next->val = NULL;
                        }
                        dlq_enque(origstr, &first->origstrQ);
                    }
                    if (next == last) {
                        done = TRUE;
                    } else {
                        prev = next;
                        next = (tk_token_t *)dlq_nextEntry(next);
                    }
                }
            }

            /* fixup the first token */
            first->len = bufflen;
            if (first->origval == NULL) {
                /* save the first SQSTRING since its value
                 * is about to get changed to the entire concat string
                 */
                first->origval = first->val;
            } else {
                /* the first part of a QSTRING has already been 
                 * converted so it cannot be used as 'origval'
                 * like an SQSTRING; just toss it as origval copy is
                 * already set before the conversion was done
                 */
                m__free(first->val);
            }
            first->val = buff;
        
            /* remove the 2nd through the 'last' token */
            done = FALSE;
            while (!done) {
                next = (tk_token_t *)dlq_nextEntry(first);
                dlq_remove(next);
                if (next == last) {
                    done = TRUE;
                }
                free_token(next);
            }

            /* setup the next search for a 'first' string */
            first = (tk_token_t *)dlq_nextEntry(first);
        } else {
            /* did not find a string concat, continue search */
            first = next;
        }
    }

    return NO_ERR;

}  /* concat_qstrings */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION tk_new_chain
* 
* Allocatate a new token parse chain
*
* RETURNS:
*  new parse chain or NULL if memory error
*********************************************************************/
tk_chain_t * 
    tk_new_chain (void)
{
    tk_chain_t  *tkc;

    tkc = m__getObj(tk_chain_t);
    if (!tkc) {
        return NULL;
    }
    memset(tkc, 0x0, sizeof(tk_chain_t));
    dlq_createSQue(&tkc->tkQ);
    tkc->cur = (tk_token_t *)&tkc->tkQ;
    dlq_createSQue(&tkc->tkptrQ);
    return tkc;
           
} /* tk_new_chain */


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
void
    tk_setup_chain_conf (tk_chain_t *tkc,
                         FILE *fp,
                         const xmlChar *filename)
{
#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tkc->fp = fp;
    tkc->source = TK_SOURCE_CONF;
    tkc->filename = filename;
    tkc->flags |= TK_FL_MALLOC;
           
} /* tk_setup_chain_conf */


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
void
    tk_setup_chain_yang (tk_chain_t *tkc,
                         FILE *fp,
                         const xmlChar *filename)
{
#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tkc->fp = fp;
    tkc->source = TK_SOURCE_YANG;
    tkc->filename = filename;
    tkc->flags |= TK_FL_MALLOC;
           
} /* tk_setup_chain_yang */


/********************************************************************
* FUNCTION tk_setup_chain_yin
* 
* Setup a previously allocated chain for a YIN file
*
* INPUTS
*    tkc == token chain to setup
*    filename == source filespec
*********************************************************************/
void
    tk_setup_chain_yin (tk_chain_t *tkc,
                        const xmlChar *filename)
{
#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tkc->source = TK_SOURCE_YANG;
    tkc->filename = filename;
    tkc->flags |= TK_FL_MALLOC;
           
} /* tk_setup_chain_yin */


/********************************************************************
* FUNCTION tk_setup_chain_docmode
* 
* Setup a previously allocated chain for a yangdump doc output mode
*
* INPUTS
*    tkc == token chain to setup
*********************************************************************/
void
    tk_setup_chain_docmode (tk_chain_t *tkc)
{
#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    tkc->flags |= TK_FL_DOCMODE;
           
} /* tk_setup_chain_docmode */


/********************************************************************
* FUNCTION tk_free_chain
* 
* Cleanup and deallocate a tk_chain_t 
* INPUTS:
*   tkc == TK chain to delete
* RETURNS:
*  none
*********************************************************************/
void 
    tk_free_chain (tk_chain_t *tkc)
{
    tk_token_t      *tk;
    tk_token_ptr_t  *tkptr;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    while (!dlq_empty(&tkc->tkQ)) {
        tk = (tk_token_t *)dlq_deque(&tkc->tkQ);
        free_token(tk);
    }
    while (!dlq_empty(&tkc->tkptrQ)) {
        tkptr = (tk_token_ptr_t *)dlq_deque(&tkc->tkptrQ);
        free_token_ptr(tkptr);
    }
    if ((tkc->flags & TK_FL_MALLOC) && tkc->buff) {
        m__free(tkc->buff);
    }
    m__free(tkc);
    
} /* tk_free_chain */


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
ncx_btype_t 
    tk_get_yang_btype_id (const xmlChar *buff, 
                          uint32 len)
{
    uint32 i;

#ifdef DEBUG
    if (!buff) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_BT_NONE;
    }
    if (!len) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NCX_BT_NONE;
    }
#endif

    /* hack first because of NCX_BT_ENUM */
    if (len==11 && !xml_strncmp(buff, NCX_EL_ENUMERATION, 11)) {
        return NCX_BT_ENUM;
    }

    /* look in the blist for the specified type name */
    for (i=1; blist[i].btyp != NCX_BT_NONE; i++) {
        if ((blist[i].blen == len) &&
            !xml_strncmp(blist[i].bid, buff, len)) {
            if (blist[i].flags & FL_YANG) {
                return blist[i].btyp;
            } else {
                return NCX_BT_NONE;
            }
        }
    }
    return NCX_BT_NONE;

} /* tk_get_yang_btype_id */


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
const char *
    tk_get_token_name (tk_type_t ttyp) 
{
    if (ttyp <= TK_TT_RNUM) {
        return tlist[ttyp].tname;
    } else {
        return "--none--";
    }

} /* tk_get_token_name */


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
const char *
    tk_get_token_sym (tk_type_t ttyp) 
{
    if (ttyp <= TK_TT_GEQUAL) {
        return tlist[ttyp].tid;
    } else {
        return "--none--";
    }

} /* tk_get_token_sym */


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
const char *
    tk_get_btype_sym (ncx_btype_t btyp) 
{
    if (btyp <= NCX_LAST_DATATYPE) {
        return (const char *)blist[btyp].bid;
    } else if (btyp == NCX_BT_EXTERN) {
        return "extern";
    } else if (btyp == NCX_BT_INTERN) {
        return "intern";
    } else {
        return "none";
    }
} /* tk_get_btype_sym */


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
tk_type_t
    tk_next_typ (tk_chain_t *tkc)
{
    tk_token_t *tk;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TK_TT_NONE;
    }
#endif

    if (!tkc->cur) {
        /* hit EOF in token chain already */
        return TK_TT_NONE;
    }
    tk = (tk_token_t *)dlq_nextEntry(tkc->cur);
    return  (tk) ? tk->typ : TK_TT_NONE;

} /* tk_next_typ */


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
tk_type_t
    tk_next_typ2 (tk_chain_t *tkc)
{
    tk_token_t *tk;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return TK_TT_NONE;
    }
#endif

    if (!tkc->cur) {
        return TK_TT_NONE;
    }

    tk = (tk_token_t *)dlq_nextEntry(tkc->cur);
    if (tk) {
        tk = (tk_token_t *)dlq_nextEntry(tk);
        return  (tk) ? tk->typ : TK_TT_NONE;
    } else {
        return TK_TT_NONE;
    }

} /* tk_next_typ2 */


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
const xmlChar *
    tk_next_val (tk_chain_t *tkc)
{
    tk_token_t *tk;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    tk = (tk_token_t *)dlq_nextEntry(tkc->cur);
    return  (tk) ? (const xmlChar *)tk->val : NULL;

} /* tk_next_val */


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
void
    tk_dump_token (const tk_token_t *tk)
{
#ifdef DEBUG
    if (!tk) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!LOGDEBUG2) {
        return;
    }

    log_debug2("\nline(%u.%u), typ(%s)",
               tk->linenum, 
               tk->linepos, 
               tk_get_token_name(tk->typ));
    if (tk->val) {
        if (xml_strlen(tk->val) > 40) {
            log_debug2("\n");
        }
        log_debug2("  val(%s)", (const char *)tk->val);
    }

} /* tk_dump_token */


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
void
    tk_dump_chain (const tk_chain_t *tkc)
{
    const tk_token_t *tk;
    int               i;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (!LOGDEBUG3) {
        return;
    }

    i = 0;
    for (tk = (tk_token_t *)dlq_firstEntry(&tkc->tkQ);
         tk != NULL;
         tk = (tk_token_t *)dlq_nextEntry(tk)) {
        log_debug3("\n%s line(%u.%u), tk(%d), typ(%s)",
                   (tk==tkc->cur) ? "*cur*" : "",
                   tk->linenum, 
                   tk->linepos, 
                   ++i, 
                   tk_get_token_name(tk->typ));
        if (tk->val) {
            if (xml_strlen(tk->val) > 40) {
                log_debug3("\n");
            }
            log_debug3("  val(%s)", (const char *)tk->val);
        }
    }

} /* tk_dump_chain */


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
boolean
    tk_is_wsp_string (const tk_token_t *tk)
{
    const xmlChar *str;

#ifdef DEBUG
    if (!tk) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    switch (tk->typ) {
    case TK_TT_QSTRING:
    case TK_TT_SQSTRING:
        str = tk->val;
        while (*str && (*str != '\n') && !xml_isspace(*str)) {
            str++;
        }
        return (*str) ? TRUE : FALSE;
    default:
        return FALSE;
    }

} /* tk_is_wsp_string */


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
status_t 
    tk_tokenize_input (tk_chain_t *tkc,
                       ncx_module_t *mod)
{
    status_t      res;
    boolean       done;
    tk_token_t   *tk;
    tk_type_t     ttyp;

#ifdef DEBUG
    if (!tkc) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* check if a temp buffer is needed */
    if (tkc->flags & TK_FL_MALLOC) {
        tkc->buff = m__getMem(TK_BUFF_SIZE);
        if (!tkc->buff) {
            res = ERR_INTERNAL_MEM;
            ncx_print_errormsg(tkc, mod, res);
            return res;
        } else {
            memset(tkc->buff, 0x0, TK_BUFF_SIZE);
        }
    } else if (tkc->buff == NULL) {
        /* tkc->buff expected to be setup already */
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* setup buffer for parsing */
    res = NO_ERR;
    tkc->bptr = tkc->buff;

    /* outside loop iterates per buffer full only
     * if reading input from file
     */
    done = FALSE;
    while (!done) {
        /* get one line of input if parsing from FILE in buffer,
         * or already have the buffer if parsing from memory
         */
        if (tkc->filename) {
            if (!fgets((char *)tkc->buff, TK_BUFF_SIZE, tkc->fp)) {
                /* read line failed, treating as not an error */
                res = NO_ERR;
                done = TRUE;
                continue;
            } else {
                /* save newline token for conf file only */
                if (tkc->source == TK_SOURCE_CONF) {
                    tk = new_token(TK_TT_NEWLINE, NULL, 0);
                    if (!tk) {
                        res = ERR_INTERNAL_MEM;
                        done = TRUE;
                        continue;
                    } 
                    tk->linenum = tkc->linenum;
                    tk->linepos = tkc->linepos;
                    dlq_enque(tk, &tkc->tkQ);
                }
                tkc->linenum++;
                tkc->linepos = 1;
            }

            /* set buffer pointer to start of buffer */
            tkc->bptr = tkc->buff;

#ifdef TK_RDLN_DEBUG
            if (LOGDEBUG3) {
                if (xml_strlen(tkc->buff) < 80) {
                    log_debug3("\ntk_tokenize: read line (%s)", tkc->buff);
                } else {
                    log_debug3("\ntk_tokenize: read line len  (%d)", 
                               xml_strlen(tkc->buff));
                }
            }
#endif
            ncx_check_warn_linelen(tkc, mod, tkc->buff);
        }

        /* Have some sort of input in the buffer (tkc->buff) */
        while (*tkc->bptr && res==NO_ERR) { 

            /* skip whitespace */
            while (*tkc->bptr && (*tkc->bptr != '\n') &&
                   xml_isspace(*tkc->bptr)) {
                if (*tkc->bptr == '\t') {
                    tkc->linepos += NCX_TABSIZE;
                } else {
                    tkc->linepos++;
                }
                tkc->bptr++;
            }

            /* check the first non-whitespace char found or exit */
            if (!*tkc->bptr) {
                continue;                 /* EOS, exit loop */
            } else if (*tkc->bptr == '\n') {
                /* save newline token for conf file only */
                if (tkc->source == TK_SOURCE_CONF) {
                    tk = new_token(TK_TT_NEWLINE, NULL, 0);
                    if (!tk) {
                        res = ERR_INTERNAL_MEM;
                        done = TRUE;
                        continue;
                    } 
                    tk->linenum = tkc->linenum;
                    tk->linepos = ++tkc->linepos;
                    dlq_enque(tk, &tkc->tkQ);
                }
                tkc->bptr++;
            } else if ((tkc->source == TK_SOURCE_CONF &&
                        *tkc->bptr == NCX_COMMENT_CH) ||
                       (tkc->source == TK_SOURCE_YANG &&
                        *tkc->bptr == '/' && tkc->bptr[1] == '/')) {
                /* CONF files use the '# to eoln' comment format
                 * YANG files use the '// to eoln' comment format
                 * skip past the comment, make next char EOLN
                 *
                 * TBD: SAVE COMMENTS IN XMLDOC SESSION MODE
                 */
                while (*tkc->bptr && *tkc->bptr != '\n') {
                    tkc->bptr++;
                }
            } else if (tkc->source == TK_SOURCE_YANG &&
                       *tkc->bptr == '/' && tkc->bptr[1] == '*') {
                /* found start of a C-style YANG comment */
                res = skip_yang_cstring(tkc);
            } else if (*tkc->bptr == NCX_QSTRING_CH) {
                /* get a dbl-quoted string which may span multiple lines */
                res = tokenize_qstring(tkc);
            } else if (*tkc->bptr == NCX_SQSTRING_CH) {
                /* get a single-quoted string which may span multiple lines */
                res = tokenize_sqstring(tkc);
            } else if (tkc->source == TK_SOURCE_XPATH &&
                       *tkc->bptr == NCX_VARBIND_CH) {
                res = tokenize_varbind_string(tkc);
            } else if (ncx_valid_fname_ch(*tkc->bptr)) {
                /* get some some of unquoted ID string or regular string */
                res = tokenize_id_string(tkc);
            } else if ((*tkc->bptr=='+' || *tkc->bptr=='-') &&
                       isdigit(*(tkc->bptr+1)) &&
                       (tkc->source != TK_SOURCE_YANG) &&
                       (tkc->source != TK_SOURCE_XPATH)) {
                /* get some sort of number 
                 * YANG does not have +/- number sequences
                 * so they are parsed (first pass) as a string
                 * There are corner cases such as range 1..max
                 * that will be parsed wrong (2nd dot).  These
                 * strings use the tk_retokenize_cur_string fn
                 * to break up the string into more tokens
                 */
                res = tokenize_number(tkc);
            } else if (isdigit(*tkc->bptr) &&
                       (tkc->source != TK_SOURCE_YANG)) {
                res = tokenize_number(tkc);
            } else {
                /* check for a 2 char token before 1 char token */
                ttyp = get_token_id(tkc->bptr, 2, tkc->source);
                if (ttyp != TK_TT_NONE) {
                    res = add_new_token(tkc, 
                                        ttyp, 
                                        tkc->bptr+2, 
                                        tkc->linepos);
                    tkc->bptr += 2;
                    tkc->linepos += 2;
                } else {
                    /* not a 2-char, check for a 1-char token */
                    ttyp = get_token_id(tkc->bptr, 1, tkc->source);
                    if (ttyp != TK_TT_NONE) {
                        /* got a 1 char token */
                        res = add_new_token(tkc, 
                                            ttyp, 
                                            tkc->bptr+1, 
                                            tkc->linepos);
                        tkc->bptr++;
                        tkc->linepos++;
                    } else {
                        /* ran out of token type choices 
                         * call it a string 
                         */
                        res = tokenize_string(tkc);
                    }
                }
            }
        }  /* end while non-zero chars left in buff and NO_ERR */

        /* finish outer loop, once through for buffer mode */
        if (!(tkc->flags & TK_FL_MALLOC)) {
            done = TRUE;
        }
    }

    if (res == NO_ERR && tkc->source != TK_SOURCE_XPATH) {
        res = concat_qstrings(tkc);
    }

    if (res == NO_ERR) {
        /* setup the token queue current pointer */
        tkc->cur = (tk_token_t *)&tkc->tkQ;
    } else {
        ncx_print_errormsg(tkc, mod, res);
    }

    return res;

}  /* tk_tokenize_input */


/********************************************************************
* FUNCTION tk_retokenize_cur_string
* 
* The current token is some sort of a string
* Reparse it according to the full NCX token list, as needed
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
status_t 
    tk_retokenize_cur_string (tk_chain_t *tkc,
                              ncx_module_t *mod)
{
    tk_chain_t *tkctest;
    tk_token_t *p;
    status_t    res;

#ifdef DEBUG
    if (!tkc || !tkc->cur) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }   
#endif

    if (!TK_CUR_STR(tkc)) {
        return NO_ERR;   /* not a string, leave it alone */
    }

    /* create a test chain and parse the string */
    tkctest = tk_new_chain();
    if (!tkctest) {
        return ERR_INTERNAL_MEM;
    }
    tkctest->source = TK_SOURCE_REDO;
    tkctest->bptr = tkctest->buff = TK_CUR_VAL(tkc);
    res = tk_tokenize_input(tkctest, mod);

    /* check if the token parse was different
     * if more than 1 token, then it was changed
     * from a single string to something else
     */
    if (res == NO_ERR) {
        /* redo token line info for these expanded tokens */
        for (p = (tk_token_t *)dlq_firstEntry(&tkctest->tkQ); 
             p != NULL;
             p = (tk_token_t *)dlq_nextEntry(p)) {
            p->linenum = tkc->cur->linenum;
            p->linepos = tkc->cur->linepos;
        }

        dlq_block_insertAfter(&tkctest->tkQ, tkc->cur);

        /* get rid of the original string and reset the cur token */
        p = (tk_token_t *)dlq_nextEntry(tkc->cur);
        dlq_remove(tkc->cur);
        free_token(tkc->cur);
        tkc->cur = p;
    }

    tk_free_chain(tkctest);

    return res;

} /* tk_retokenize_cur_string */


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
tk_chain_t * tk_tokenize_metadata_string ( ncx_module_t *mod,
                                           xmlChar *str,
                                           status_t *res )
{
    tk_chain_t *tkc;

    assert( str && " str is NULL" );
    assert( res && " res is NULL" );

    /* create a new chain and parse the string */
    tkc = tk_new_chain();
    if (!tkc) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    tkc->source = TK_SOURCE_YANG;
    tkc->bptr = tkc->buff = str;
    *res = tk_tokenize_input(tkc, mod);
    return tkc;
} /* tk_tokenize_metadata_string */


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
tk_chain_t *
    tk_tokenize_xpath_string (ncx_module_t *mod,
                              xmlChar *str,
                              uint32 curlinenum,
                              uint32 curlinepos,
                              status_t *res)
{
    tk_chain_t *tkc;

#ifdef DEBUG
    if (!str || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }   
#endif

    /* create a new chain and parse the string */
    tkc = tk_new_chain();
    if (!tkc) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    tkc->source = TK_SOURCE_XPATH;
    tkc->bptr = tkc->buff = str;
    tkc->linenum = curlinenum;
    tkc->linepos = curlinepos;
    *res = tk_tokenize_input(tkc, mod);
    return tkc;

} /* tk_tokenize_xpath_string */


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
uint32
    tk_token_count (const tk_chain_t *tkc) 
{
    const tk_token_t *tk;
    uint32  cnt;

#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }   
#endif

    cnt = 0;
    for (tk = (const tk_token_t *)dlq_firstEntry(&tkc->tkQ);
         tk != NULL;
         tk = (const tk_token_t *)dlq_nextEntry(tk)) {
        cnt++;
    }
    return cnt;

} /* tk_token_count */


/********************************************************************
* FUNCTION tk_reset_chain
* 
* Reset the token chain current pointer to the start
*
* INPUTS:
*   tkc == token chain to reset
*
*********************************************************************/
void
    tk_reset_chain (tk_chain_t *tkc) 
{
#ifdef DEBUG
    if (!tkc) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }   
#endif
    tkc->cur = (tk_token_t *)&tkc->tkQ;

}  /* tk_reset_chain */


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
tk_chain_t * 
    tk_clone_chain (tk_chain_t *oldtkc)
{
    tk_chain_t  *tkc;
    tk_token_t  *token, *oldtoken;
    status_t     res;

#ifdef DEBUG
    if (oldtkc == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    tkc = tk_new_chain();
    if (!tkc) {
        return NULL;
    }

    tkc->filename = oldtkc->filename;
    tkc->linenum = oldtkc->linenum;
    tkc->flags = oldtkc->flags;
    tkc->source = oldtkc->source;

    res = NO_ERR;
    for (oldtoken = (tk_token_t *)dlq_firstEntry(&oldtkc->tkQ);
         oldtoken != NULL && res == NO_ERR;
         oldtoken = (tk_token_t *)dlq_nextEntry(oldtoken)) {

        token = new_token(oldtoken->typ,
                          oldtoken->val,
                          oldtoken->len);
        if (!token) {
            tk_free_chain(tkc);
            return NULL;
        }

        if (oldtoken->mod) {
            token->mod = xml_strndup(oldtoken->mod,
                                     oldtoken->modlen);
            if (!token->mod) {
                free_token(token);
                tk_free_chain(tkc);
                return NULL;
            }
            token->modlen = oldtoken->modlen;
        }
                
        token->linenum = oldtoken->linenum;
        token->linepos = oldtoken->linepos;
        token->nsid = oldtoken->nsid;
        dlq_enque(token, &tkc->tkQ);
    }

    return tkc;
           
} /* tk_clone_chain */


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
status_t 
    tk_add_id_token (tk_chain_t *tkc,
                     const xmlChar *valstr)
{
    tk_token_t   *tk;

    if ( !tkc ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    if ( !valstr ) {
        tk = new_token(TK_TT_TSTRING, NULL, 0);
    } else {
        /* normal case string -- non-zero length */
        tk = new_token(TK_TT_TSTRING,
                       valstr, 
                       xml_strlen(valstr));
    }
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
}  /* tk_add_id_token */


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
status_t 
    tk_add_pid_token (tk_chain_t *tkc,
                      const xmlChar *prefix,
                      uint32 prefixlen,
                      const xmlChar *valstr)
{
    tk_token_t   *tk;

#ifdef DEBUG
    if (tkc == NULL || prefix == NULL || valstr == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    tk = new_token_wmod(TK_TT_MSTRING,
                        prefix, 
                        prefixlen, 
                        valstr,
                        xml_strlen(valstr));

    if (tk == NULL) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
    
}  /* tk_add_pid_token */


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
status_t 
    tk_add_string_token (tk_chain_t *tkc,
                         const xmlChar *valstr)
{
    tk_token_t   *tk;
    tk_type_t     tktyp;

    if ( !tkc ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    if ( !valstr ) {
        tk = new_token(TK_TT_QSTRING, NULL, 0);
    } else {
        uint32 tklen = xml_strlen(valstr);

        if (val_need_quotes(valstr)) {
            tktyp = TK_TT_QSTRING;
        } else if (ncx_valid_name(valstr, tklen)) {
            tktyp = TK_TT_TSTRING;
        } else {
            tktyp = TK_TT_STRING;
        }

        /* normal case string -- non-zero length */
        tk = new_token(tktyp, valstr, tklen);
    }
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
    
}  /* tk_add_string_token */


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
status_t 
    tk_add_lbrace_token (tk_chain_t *tkc)
{
    tk_token_t   *tk;

#ifdef DEBUG
    if (tkc == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    tk = new_token(TK_TT_LBRACE,
                   (const xmlChar *)"{",
                   1);
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
    
}  /* tk_add_lbrace_token */


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
status_t 
    tk_add_rbrace_token (tk_chain_t *tkc)
{
    tk_token_t   *tk;

#ifdef DEBUG
    if (tkc == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    tk = new_token(TK_TT_RBRACE,
                   (const xmlChar *)"}",
                   1);
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
    
}  /* tk_add_rbrace_token */


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
status_t 
    tk_add_semicol_token (tk_chain_t *tkc)
{
    tk_token_t   *tk;

#ifdef DEBUG
    if (tkc == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* hack for YIN input, no XML line numbers */
    tkc->linenum++;

    tk = new_token(TK_TT_SEMICOL,
                   (const xmlChar *)";",
                   1);
    if (!tk) {
        return ERR_INTERNAL_MEM;
    }

    tk->linenum = tkc->linenum;
    tk->linepos = 1;
    dlq_enque(tk, &tkc->tkQ);

    return NO_ERR;
    
}  /* tk_add_semicol_token */


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
status_t
    tk_check_save_origstr (tk_chain_t  *tkc,
                           tk_token_t *tk,
                           const void *field)
{
    tk_token_ptr_t  *tkptr;

#ifdef DEBUG
    if (tkc == NULL || tk == NULL || field == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (!TK_DOCMODE(tkc)) {
        return NO_ERR;
    }

    if (!TK_TYP_STR(tk->typ)) {
        return NO_ERR;
    }

    /* save all string tokens, not just the ones that
     * have multi-part format, needed for original quotes
     */
    tkptr = new_token_ptr(tk, field);
    if (tkptr == NULL) {
        return ERR_INTERNAL_MEM;
    }
    dlq_enque(tkptr, &tkc->tkptrQ);

    return NO_ERR;

} /* tk_check_save_origstr */


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
const xmlChar *
    tk_get_first_origstr (const tk_token_ptr_t  *tkptr,
                          boolean *dquote,
                          boolean *morestr)

{
    const tk_token_t  *tk;

#ifdef DEBUG
    if (tkptr == NULL || 
        tkptr->tk == NULL || 
        dquote == NULL || 
        morestr == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    tk = tkptr->tk;

    *morestr = !dlq_empty(&tk->origstrQ);
    *dquote = (tk->typ == TK_TT_QSTRING) ? TRUE : FALSE;
    if (tk->origval) {
        return tk->origval;
    }
    return tk->val;
}


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
const tk_origstr_t *
    tk_first_origstr_rec (const tk_token_ptr_t  *tkptr)
{
    const tk_token_t  *tk;

#ifdef DEBUG
    if (tkptr == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    tk = tkptr->tk;
    return (const tk_origstr_t *)dlq_firstEntry(&tk->origstrQ);

}  /* tk_first_origstr_rec */


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
const tk_origstr_t *
    tk_next_origstr_rec (const tk_origstr_t  *origstr)
{
#ifdef DEBUG
    if (origstr == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    return (const tk_origstr_t *)dlq_nextEntry(origstr);

}  /* tk_next_origstr_rec */


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
const xmlChar *
    tk_get_origstr_parts (const tk_origstr_t  *origstr,
                          boolean *dquote,
                          boolean *newline)

{
#ifdef DEBUG
    if (origstr == NULL || dquote == NULL || newline == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *dquote = (origstr->origtyp == TK_ORIGSTR_DQUOTE ||
               origstr->origtyp == TK_ORIGSTR_DQUOTE_NL);
    *newline = (origstr->origtyp == TK_ORIGSTR_DQUOTE_NL ||
               origstr->origtyp == TK_ORIGSTR_SQUOTE_NL);
    return origstr->str;

}  /* tk_get_origstr_parts */


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
const tk_token_ptr_t *
    tk_find_tkptr (const tk_chain_t  *tkc,
                   const void *field)
{
    const tk_token_ptr_t *tkptr;

#ifdef DEBUG
    if (tkc == NULL || field == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (tkptr = (const tk_token_ptr_t *)
             dlq_firstEntry(&tkc->tkptrQ);
         tkptr != NULL;
         tkptr = (const tk_token_ptr_t *)
             dlq_nextEntry(tkptr)) {

        if (field == tkptr->field) {
            return tkptr;
        }
    }

    return NULL;

}  /* tk_find_tkptr */


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
uint32
    tk_tkptr_quotes (const tk_token_ptr_t  *tkptr)
{

#ifdef DEBUG
    if (tkptr == NULL || tkptr->tk == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    switch (tkptr->tk->typ) {
    case TK_TT_SQSTRING:
        return 1;
    case TK_TT_QSTRING:
        return 2;
    default:
        return 0;
    }

}  /* tk_tkptr_quotes */


/* END file tk.c */
