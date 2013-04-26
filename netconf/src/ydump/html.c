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
/*  FILE: html.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
23feb08      abb      begun

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

#include <xmlstring.h>
#include <xmlreader.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_html
#include "html.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xml_val
#include "xml_val.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xsd_util
#include "xsd_util.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif

#ifndef _H_yangdump_util
#include "yangdump_util.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define CSS_CONTENT_FILE  (const xmlChar *)"yangdump-css-contents.txt"

#define SPACE_CH          (const xmlChar *)"&nbsp;"

#define START_SEC         (const xmlChar *)" {"

#define END_SEC           (const xmlChar *)"}"

#define MENU_NEST_LEVEL   4

#define MENU_LABEL_LEN    30

/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/

static void
    write_typedefs (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    dlq_hdr_t *typedefQ,
                    int32 startindent);


static void
    write_groupings (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     dlq_hdr_t *groupingQ,
                     int32 startindent);

static void
    write_type_clause (ses_cb_t *scb,
                       const ncx_module_t *mod,
                       const yangdump_cvtparms_t *cp,
                       typ_def_t *typdef,
                       obj_template_t *obj,
                       int32 startindent);

static void
    write_objects (ses_cb_t *scb,
                   const ncx_module_t *mod,
                   const yangdump_cvtparms_t *cp,
                   dlq_hdr_t *datadefQ,
                   int32 startindent);


/********************************************************************
* FUNCTION has_nl
* 
* Check if string has newlines
*
* INPUTS:
*   str == string to use
*
*********************************************************************/
static boolean
    has_nl (const xmlChar *str)
{
    while (*str && *str != '\n') {
        str++;
    }
    return (*str) ? TRUE : FALSE;

}  /* has_nl */



/********************************************************************
* FUNCTION start_elem
* 
* Generate a start tag
*
* INPUTS:
*   scb == session control block to use for writing
*   name == element name
*   css_class == CSS class to use (may be NULL)
*   indent == indent count
*
*********************************************************************/
static void
    start_elem (ses_cb_t *scb,
                const xmlChar *name,
                const xmlChar *css_class,
                int32 indent)

{
    ses_putstr_indent(scb, (const xmlChar *)"<", indent);
    ses_putstr(scb, name);
    if (css_class) {
        ses_putstr(scb, (const xmlChar *)" class=\"");
        ses_putstr(scb, css_class);
        ses_putstr(scb, (const xmlChar *)"\">");
    } else {
        ses_putchar(scb, '>');
    }

}  /* start_elem */


/********************************************************************
* FUNCTION start_id_elem
* 
* Generate a start tag with an id instead of class attribute
*
* INPUTS:
*   scb == session control block to use for writing
*   name == element name
*   id == CSS id to use (may be NULL)
*   indent == indent count
*
*********************************************************************/
static void
    start_id_elem (ses_cb_t *scb,
                   const xmlChar *name,
                   const xmlChar *id,
                   int32 indent)

{
    ses_putstr_indent(scb, (const xmlChar *)"<", indent);
    ses_putstr(scb, name);
    if (id) {
        ses_putstr(scb, (const xmlChar *)" id=\"");
        ses_putstr(scb, id);
        ses_putstr(scb, (const xmlChar *)"\">");
    } else {
        ses_putchar(scb, '>');
    }

}  /* start_id_elem */


/********************************************************************
* FUNCTION end_elem
* 
* Generate an end tag
*
* INPUTS:
*   scb == session control block to use for writing
*   name == element name
*   indent == indent count
*
*********************************************************************/
static void
    end_elem (ses_cb_t *scb,
              const xmlChar *name,
              int32 indent)
{
    ses_putstr_indent(scb, (const xmlChar *)"</", indent);
    ses_putstr(scb, name);
    ses_putchar(scb, '>');

}  /* end_elem */


/********************************************************************
* FUNCTION write_kw
* 
* Generate a keyword at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   kwname == keyword name
*
*********************************************************************/
static void
    write_kw (ses_cb_t *scb,
              const xmlChar *kwname)
{
    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_kw\">");
    ses_putstr(scb, kwname);
    ses_putstr(scb, (const xmlChar *)"</span>");

}  /* write_kw */


/********************************************************************
* FUNCTION write_extkw
* 
* Generate a language extension keyword at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   kwpfix == keyword prefix to use
*   kwname == keyword name
*
*********************************************************************/
static void
    write_extkw (ses_cb_t *scb,
                 const xmlChar *kwpfix,
                 const xmlChar *kwname)
{
    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_kw\">");
    if (kwpfix != NULL) {
        ses_putstr(scb, kwpfix);
        ses_putchar(scb, ':');
    }
    ses_putstr(scb, kwname);
    ses_putstr(scb, (const xmlChar *)"</span>");

}  /* write_extkw */


/********************************************************************
* FUNCTION write_banner_cmt
* 
* Generate a comment at the current location for a section banner
* Check that only the first one is printed
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   cmval == comment value string
*   indent == current indent count
*********************************************************************/
static void
    write_banner_cmt (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const xmlChar *cmval,
                      int32 indent)
{
    boolean needed;

    needed = FALSE;

    if (cp->unified) {
        if (mod->ismod) {
            needed = TRUE;
        }
    } else {
        needed = TRUE;
    }
    if (needed) {
        ses_putchar(scb, '\n');
        ses_indent(scb, indent);
        ses_putstr(scb, (const xmlChar *)"<span class=\"yang_cmt\">");
        ses_putstr(scb, (const xmlChar *)"// ");
        ses_putstr(scb, cmval);
        ses_putstr(scb, (const xmlChar *)"</span>");
    }

}  /* write_banner_cmt */


/********************************************************************
* FUNCTION write_endsec_cmt
* 
* Generate a comment at the current location for an end of a section
* No comment tokens are given
*
* INPUTS:
*   scb == session control block to use for writing
*   cmtype == comment type string
*   cmname == comment value string
*
*********************************************************************/
static void
    write_endsec_cmt (ses_cb_t *scb,
                   const xmlChar *cmtype,
                   const xmlChar *cmname)
{
    ses_putstr(scb, (const xmlChar *)"  <span class=\"yang_cmt\">");
    ses_putstr(scb, (const xmlChar *)"// ");
    if (cmtype) {
        ses_putstr(scb, cmtype);
        ses_putchar(scb, ' ');
    }
    if (cmname) {
        ses_putstr(scb, cmname);
    }
    ses_putstr(scb, (const xmlChar *)"</span>");

}  /* write_endsec_cmt */


/********************************************************************
* FUNCTION write_id
* 
* Generate an identifier at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   idname == identifier name
*
*********************************************************************/
static void
    write_id (ses_cb_t *scb,
              const xmlChar *idname)
{
    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_id\">");
    ses_putstr(scb, idname);
    ses_putstr(scb, (const xmlChar *)"</span>");

}  /* write_id */


/********************************************************************
* FUNCTION write_id_a
* 
* Generate an anchor for an identifier on the current line
* No visible HTML is generated
*
* INPUTS:
*   scb == session control block to use for writing
*   submod == submodule name (if any)
*   idname == identifier name
*   linenum == identifier line number (used in URL)
*
*********************************************************************/
static void
    write_id_a (ses_cb_t *scb,
                const xmlChar *submod,
                const xmlChar *idname,
                uint32 linenum)
{
    char  buff[NCX_MAX_NUMLEN];

    ses_putstr(scb, (const xmlChar *)"<a name=\"");
    if (submod) {
        ses_putstr(scb, submod);
        ses_putchar(scb, '.');
    }
    ses_putstr(scb, idname);
    if (linenum) {
        ses_putchar(scb, '.');
        sprintf(buff, "%u", linenum);
        ses_putstr(scb, (const xmlChar *)buff);
    }
    ses_putstr(scb, (const xmlChar *)"\"></a>");

}  /* write_id_a */


/********************************************************************
* FUNCTION write_str
* 
* Generate a string token at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   strval == string value
*   quotes == quotes style (0, 1, 2)
*********************************************************************/
static void
    write_str (ses_cb_t *scb,
               const xmlChar *strval,
               uint32 quotes)
{
    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_str\">");

    switch (quotes) {
    case 1:
        ses_putchar(scb, '\'');
        break;
    case 2:
        ses_putchar(scb, '"');
        break;
    default:
        ;
    }

    ses_puthstr(scb, strval);

    switch (quotes) {
    case 1:
        ses_putchar(scb, '\'');
        break;
    case 2:
        ses_putchar(scb, '"');
        break;
    default:
        ;
    }
    ses_putstr(scb, (const xmlChar *)"</span>");

}  /* write_str */


/********************************************************************
* FUNCTION write_comstr
* 
* Generate a complex string token sequence
* at the current line location
*
* INPUTS:
*   scb == session control block to use for writing
*   tkptr == token ptr to token with original strings
*   strval == string val pased to write_complex_str
*   indent == current indent count
*********************************************************************/
static void
    write_comstr (ses_cb_t *scb,
                  const tk_token_ptr_t *tkptr,
                  const xmlChar *strval,
                  int32 indent)
{
    const xmlChar *str;
    const tk_origstr_t *origstr;
    uint32         numquotes;
    boolean        dquotes, moreflag, newline;

    /* put the first string (maybe only fragment) */
    dquotes = FALSE;
    moreflag = FALSE;
    newline = FALSE;

    str = tk_get_first_origstr(tkptr, &dquotes, &moreflag);
    if (str == NULL) {
        str = strval;
        numquotes = tk_tkptr_quotes(tkptr);
        moreflag = FALSE;
    } else {
        numquotes = dquotes ? 2 : 1;
    }

    /* write first string at current indent point */
    write_str(scb, str, numquotes);

    if (!moreflag) {
        return;
    }

    indent += ses_indent_count(scb);
    for (origstr = tk_first_origstr_rec(tkptr);
         origstr != NULL;
         origstr = tk_next_origstr_rec(origstr)) {

        dquotes = FALSE;
        newline = FALSE;
        str = tk_get_origstr_parts(origstr, &dquotes, &newline);
        if (str == NULL) {
            SET_ERROR(ERR_INTERNAL_VAL);
            return;
        } else {
            if (newline) {
                ses_putstr_indent(scb, (const xmlChar *)"+ ", indent);
            } else {
                ses_putstr(scb, (const xmlChar *)" + ");
            }
            write_str(scb, str, numquotes);
        }
    }

}  /* write_comstr */


/********************************************************************
* FUNCTION write_a
* 
* Generate an anchor element
*
*  At least one of [fname, idname] must be non-NULL (not checked!)
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   fname == filename field to use in URL (usually module name)
*   fversion == file version field (usually latest revision date)
*   submod == submodule name (may be NULL)
*   idpfix == prefix for identifier name to use (if idname non-NULL)
*   idname == identifier name to use in URL and 'a' content
*   linenum == line number to use in URL
*********************************************************************/
static void
    write_a (ses_cb_t *scb,
             const yangdump_cvtparms_t *cp,
             const xmlChar *fname,
             const xmlChar *fversion,
             const xmlChar *submod,
             const xmlChar *idpfix,
             const xmlChar *idname,
             uint32 linenum)
{
    char buff[NCX_MAX_NUMLEN];

    ses_putstr(scb, (const xmlChar *)"<a href=\"");
    if (fname) {
        if (cp->urlstart && *cp->urlstart) {
            ses_putstr(scb, cp->urlstart);
            if (cp->urlstart[xml_strlen(cp->urlstart)-1] != NCXMOD_PSCHAR) {
                ses_putchar(scb, NCXMOD_PSCHAR);
            }
        }
        ses_putstr(scb, fname);
        if (fversion && cp->versionnames) {
            if (cp->simurls) {
                ses_putchar(scb, NCXMOD_PSCHAR);
            } else {
                ses_putchar(scb, '.');
            }
            ses_putstr(scb, fversion);
        }
        if (!cp->simurls) {
            ses_putstr(scb, (const xmlChar *)".html");
        }
    }

    if (idname) {
        ses_putchar(scb, '#');
        if (submod) {
            ses_putstr(scb, submod);
            ses_putchar(scb, '.');
        }
        ses_putstr(scb, idname);
        if (linenum) {
            sprintf(buff, ".%u", linenum);
            ses_putstr(scb, (const xmlChar *)buff);
        }
    }
    ses_putstr(scb, (const xmlChar *)"\">");
    if (idpfix && idname) {
        ses_putstr(scb, idpfix);
        ses_putchar(scb, ':');
        ses_putstr(scb, idname);
    } else if (idname) {
        ses_putstr(scb, idname);
    } else if (fname) {
        ses_putstr(scb, fname);
    }
    ses_putstr(scb, (const xmlChar *)"</a>");

}  /* write_a */


/********************************************************************
* FUNCTION write_a2
* 
* Generate an anchor element with a separate display than URL
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   fname == filename field to use in URL (usually module name)
*   fversion == file version field (usually latest revision date)
*   submod == submodule name (may be NULL)
*   idname == identifier name to use in URL and 'a' content
*   linenum == line number to use in URL
*   idshow == ID string to show in the href construct
*             (may be NULL to leave off the content and final </a>
*********************************************************************/
static void
    write_a2 (ses_cb_t *scb,
              const yangdump_cvtparms_t *cp,
              const xmlChar *fname,
              const xmlChar *fversion,
              const xmlChar *submod,
              const xmlChar *idname,
              uint32 linenum,
              const xmlChar *idshow)
{
    char buff[NCX_MAX_NUMLEN];

    ses_putstr(scb, (const xmlChar *)"<a href=\"");
    if (fname) {
        if (cp->urlstart && *cp->urlstart) {
            ses_putstr(scb, cp->urlstart);
            if (cp->urlstart[xml_strlen(cp->urlstart)-1] != NCXMOD_PSCHAR) {
                ses_putchar(scb, NCXMOD_PSCHAR);
            }
        }
        ses_putstr(scb, fname);
        if (fversion && cp->versionnames) {
            if (cp->simurls) {
                ses_putchar(scb, NCXMOD_PSCHAR);
            } else {
                ses_putchar(scb, '.');
            }
            ses_putstr(scb, fversion);
        }
        if (!cp->simurls) {
            ses_putstr(scb, (const xmlChar *)".html");
        }
    }

    if (idname) {
        ses_putchar(scb, '#');
        if (submod) {
            ses_putstr(scb, submod);
            ses_putchar(scb, '.');
        }
        ses_putstr(scb, idname);
        if (linenum) {
            sprintf(buff, ".%u", linenum);
            ses_putstr(scb, (const xmlChar *)buff);
        }
    }
    ses_putstr(scb, (const xmlChar *)"\">");
    if (idshow) {
        ses_putstr(scb, idshow);
        ses_putstr(scb, (const xmlChar *)"</a>");
    }

}  /* write_a2 */


/********************************************************************
* FUNCTION write_a_ref
* 
* Generate an anchor element for a reference
*
* INPUTS:
*   scb == session control block to use for writing
*   ref == reference string to use in URL; counted string
*   reflen == length of 'ref'
*
*********************************************************************/
static void
    write_a_ref (ses_cb_t *scb,
                 const xmlChar *ref,
                 uint32 reflen)
{
    const xmlChar    *num;
    uint32            len;

    if (*ref == 'R') {
        /* assume this is an RFC xxxx reference, find the number */
        ses_putstr(scb, (const xmlChar *)"<a href=\"");
        num = ref;
        while (*num && !isdigit(*num)) {
            num++;
        }
        ses_putstr(scb, XSD_RFC_URL);
        while (*num && isdigit(*num)) {
            ses_putchar(scb, *num++);
        }
        ses_putstr(scb, (const xmlChar *)".txt");
    } else if (*ref == 'd') {
        /* assume this is an draft- reference */
        ses_putstr(scb, (const xmlChar *)"<a href=\"");
        ses_putstr(scb, XSD_DRAFT_URL);
        for (len = 0; len < reflen; len++) {
            ses_putchar(scb, ref[len]);
        }
    } else {
        /* skip this unsupported reference type */
        return;
    }
    ses_putstr(scb, (const xmlChar *)"\">");

    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_str\">");
    for (len = 0; len < reflen; len++) {
        ses_putchar(scb, ref[len]);
    }
    ses_putstr(scb, (const xmlChar *)"</span>");
    ses_putstr(scb, (const xmlChar *)"</a>");

}  /* write_a_ref */


/********************************************************************
* FUNCTION write_idref_base
* 
* Generate a base QName; clause for an identityref
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   idref == identity ref struct to use
*   startindent == start indent count
*********************************************************************/
static void
    write_idref_base (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const typ_idref_t *idref,
                      int32 startindent)
{
    const xmlChar       *fname, *fversion, *submod;
    uint32               linenum;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    fname = NULL;
    fversion = NULL;
    linenum = 0;

    ses_indent(scb, startindent);
    write_kw(scb, YANG_K_BASE);
    ses_putchar(scb, ' ');

    /* get filename if identity-stmt found */
    if (idref->base) {
        linenum = idref->base->tkerr.linenum;
        fname = idref->base->tkerr.mod->name;
        fversion = idref->base->tkerr.mod->version;
    }

    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_id\">");
    write_a(scb, cp, 
            fname, 
            fversion, 
            submod,
            idref->baseprefix,
            idref->basename, 
            linenum);
    ses_putstr(scb, (const xmlChar *)"</span>");
    ses_putchar(scb, ';');

}  /* write_idref_base */


/********************************************************************
* FUNCTION write_identity_base
* 
* Generate a base QName; clause for an identity
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   identity == identity struct to use
*   startindent == start indent count
*********************************************************************/
static void
    write_identity_base (ses_cb_t *scb,
                         const ncx_module_t *mod,
                         const yangdump_cvtparms_t *cp,
                         const ncx_identity_t *identity,
                         int32 startindent)
{
    const xmlChar       *fname, *fversion, *submod;
    uint32               linenum;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    fname = NULL;
    fversion = NULL;
    linenum = 0;

    ses_indent(scb, startindent);
    write_kw(scb, YANG_K_BASE);
    ses_putchar(scb, ' ');

    /* get filename if identity-stmt found */
    if (identity->base) {
        linenum = identity->base->tkerr.linenum;
        fname = identity->base->tkerr.mod->name;
        fversion = identity->base->tkerr.mod->version;
    }

    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_id\">");
    write_a(scb, 
            cp, 
            fname, 
            fversion, 
            submod,
            identity->baseprefix,
            identity->basename, 
            linenum);
    ses_putstr(scb, (const xmlChar *)"</span>");
    ses_putchar(scb, ';');

}  /* write_identity_base */


/********************************************************************
* FUNCTION write_href_id
* 
* Generate a simple clause on 1 line with a name anchor for the ID
*
* INPUTS:
*   scb == session control block to use for writing
*   submod == submodule name (may be NULL)
*   kwname == keyword name
*   idname == identifier name (may be NULL)
*   indent == indent count to use
*   linenum == line number for this identifier to use in URLs
*   finsemi == TRUE if end in ';', FALSE if '{'
*   newln == TRUE if a newline should be output first
*            FALSE if newline should not be output first
*********************************************************************/
static void
    write_href_id (ses_cb_t *scb,
                   const xmlChar *submod,
                   const xmlChar *kwname,
                   const xmlChar *idname,
                   int32 indent,
                   uint32 linenum,
                   boolean finsemi,
                   boolean newln)
{
    if (newln) {
        ses_putchar(scb, '\n');
    }
    ses_indent(scb, indent);
    if (idname) {
        write_id_a(scb, submod, idname, linenum);
    } else {
        write_id_a(scb, submod, kwname, linenum);
    }
    write_kw(scb, kwname);
    if (idname) {
        ses_putchar(scb, ' ');
        write_id(scb, idname);
    }
    if (finsemi) {
        ses_putchar(scb, ';');
    } else {
        ses_putstr(scb, START_SEC);
    }

}  /* write_href_id */


/********************************************************************
* FUNCTION write_complex_str
* 
* Generate a complex string from original tokens or
* just a simple clause on 1 line if complex not needed
*
* INPUTS:
*   scb == session control block to use for writing
*   tkc == token chain to search for token ptr records
*   field == address of string token (key to lookup)
*   kwname == keyword name
*   strval == string value
*   indent == indent count to use
*   quotes == quotes style (0, 1, 2)
*   finsemi == TRUE if end in ';', FALSE if '{'
*********************************************************************/
static void
    write_complex_str (ses_cb_t *scb,
                       tk_chain_t  *tkc,
                       const void *field,
                       const xmlChar *kwname,
                       const xmlChar *strval,
                       int32 indent,
                       uint32 quotes,
                       boolean finsemi)
{
    const tk_token_ptr_t  *tkptr;

    if (tkc != NULL && field != NULL) {
        tkptr = tk_find_tkptr(tkc, field);
    } else {
        tkptr = NULL;
    }

    ses_indent(scb, indent);
    write_kw(scb, kwname);
    if (strval) {
        if (!has_nl(strval) &&
            (xml_strlen(strval) + 1) < ses_line_left(scb)) {
            ses_putchar(scb, ' ');
        } else {
            indent += ses_indent_count(scb);
            ses_indent(scb, indent);
        }

        if (tkptr != NULL) {
            write_comstr(scb, tkptr, strval,  indent);
        } else {
            write_str(scb, strval, quotes);
        }
    }
    if (finsemi) {
        ses_putchar(scb, ';');
    } else {
        ses_putstr(scb, START_SEC);
    }

}  /* write_complex_str */


/********************************************************************
* FUNCTION write_leafref_path
* 
* Generate a leafref path-stmt
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typdef == typdef for the leafref
*   obj == object pointer if this is from a leaf or leaf-list
*          NULL if this is froma typedef
*   startindent == start indent count
*********************************************************************/
static void
    write_leafref_path (ses_cb_t *scb,
                        const ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        typ_def_t *typdef,
                        obj_template_t *obj,
                        int32 startindent)
{
    const xmlChar       *submod, *pathstr;
    obj_template_t      *targobj = NULL;
    xpath_pcb_t         *testpath;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    pathstr = typ_get_leafref_path(typdef);
    if (pathstr == NULL) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    testpath = typ_get_leafref_pcb(typdef);
    if ((testpath == NULL || testpath->targobj == NULL) && (obj != NULL)) {
        targobj = obj_get_leafref_targobj(obj);
    } else {
        targobj = testpath->targobj;
    }

    if (targobj == NULL) {
        write_complex_str(scb, 
                          cp->tkc,
                          typ_get_leafref_path_addr(typdef),
                          YANG_K_PATH, 
                          pathstr,
                          startindent, 
                          2, 
                          TRUE);
    } else {
        ncx_module_t *pathmod = obj_get_mod(targobj);

        ses_indent(scb, startindent);
        write_kw(scb, YANG_K_PATH);
        ses_putchar(scb, ' ');
        write_a2(scb,
                 cp, 
                 (pathmod != mod) ? 
                 ncx_get_modname(pathmod) : NULL,
                 (pathmod != mod) ? pathmod->version : NULL,
                 submod,
                 obj_get_name(targobj),
                 targobj->tkerr.linenum,
                 NULL);
        write_str(scb, pathstr, 2);
        ses_putstr(scb, (const xmlChar *)"</a>");
        ses_putchar(scb, ';');
    }

}  /* write_leafref_path */


/********************************************************************
* FUNCTION write_simple_str
* 
* Generate a simple clause on 1 line
*
* INPUTS:
*   scb == session control block to use for writing
*   kwname == keyword name
*   strval == string value
*   indent == indent count to use
*   quotes == quotes style (0, 1, 2)
*   finsemi == TRUE if end in ';', FALSE if '{'
*********************************************************************/
static void
    write_simple_str (ses_cb_t *scb,
                      const xmlChar *kwname,
                      const xmlChar *strval,
                      int32 indent,
                      uint32 quotes,
                      boolean finsemi)
{
    write_complex_str(scb,
                      NULL,
                      NULL,
                      kwname,
                      strval,
                      indent,
                      quotes,
                      finsemi);

}  /* write_simple_str */


/********************************************************************
* FUNCTION write_status
* 
* Generate the HTML for a status clause only if the value
* is other than current
*   
* INPUTS:
*   scb == session control block to use for writing
*   status == status field
*   indent == start indent count
*
*********************************************************************/
static void
    write_status (ses_cb_t *scb,
                  ncx_status_t status,
                  int32 indent)
{
    if (status != NCX_STATUS_CURRENT && status != NCX_STATUS_NONE) {
        write_simple_str(scb, 
                         YANG_K_STATUS,
                         ncx_get_status_string(status),
                         indent, 
                         0, 
                         TRUE);
    }
}  /* write_status */


/********************************************************************
* FUNCTION write_reference_str
* 
* Generate a simple clause on 1 line
*
* INPUTS:
*   scb == session control block to use for writing
*   ref == reference string to use
*   indent == indent count to use
*********************************************************************/
static void
    write_reference_str (ses_cb_t *scb,
                         const xmlChar *ref,
                         int32 indent)
{
    const xmlChar    *str, *start;
    boolean           done;
    uint32            reflen, len;

    if (ref == NULL) {
        return;
    }


    ses_indent(scb, indent);
    write_kw(scb, YANG_K_REFERENCE);
    indent += ses_indent_count(scb);
    ses_indent(scb, indent);

    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_str\">");
    ses_putchar(scb, '"');

    done = FALSE;
    while (!done) {
        str = NULL;
        start = ref;
        reflen = 0;

        len = find_reference(ref, &str, &reflen);

        if (str != NULL) {
            while (start < str) {
                if (*start == '\n') {
                    ses_indent(scb, indent);
                    ses_putchar(scb, ' ');
                    start++;
                } else {
                    ses_putcchar(scb, *start++);
                }
            }
            write_a_ref(scb, str, reflen);
            ref += len;
        } else {
            while (len > 0) {
                if (*ref == '\n') {
                    ses_indent(scb, indent);
                    ref++;
                } else {
                    ses_putcchar(scb, *ref++);
                }
                len--;
            }
        }

        /* only allow 1 reference per line at this time */
        while (*ref && *ref != '\n') {
            ses_putcchar(scb, *ref++);
        }
        if (*ref != '\n') {
            done = TRUE;
        }
    }

    ses_putchar(scb, '"');

    ses_putstr(scb, (const xmlChar *)"</span>");

    ses_putstr(scb, (const xmlChar *)";\n");

}  /* write_reference_str */


/********************************************************************
* FUNCTION write_type
* 
* Generate the HTML for the specified type name
* Does not generate the entire clause, just the type name
*   
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typdef == typ_def_t to use
*   indent == start indent count
*
*********************************************************************/
static void
    write_type (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    const typ_def_t *typdef,
                    int32 indent)
{
    const xmlChar       *fname, *fversion, *submod;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    ses_indent(scb, indent);
    write_kw(scb, YANG_K_TYPE);
    ses_putchar(scb, ' ');

    /* get filename if external module */
    fname = NULL;
    fversion = NULL;
    if (typdef->prefix && 
        xml_strcmp(typdef->prefix, mod->prefix)) {
        if (typdef->tclass == NCX_CL_NAMED && typdef->def.named.typ
            && typdef->def.named.typ->tkerr.mod) {
            fname = typdef->def.named.typ->tkerr.mod->name;
            fversion = typdef->def.named.typ->tkerr.mod->version;
        } else {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    /* write the identifier with an href to the type
     * unless the type is one of the yang builtin types
     */
    if (typdef->tclass == NCX_CL_NAMED) {
        ses_putstr(scb, (const xmlChar *)"<span class=\"yang_id\">");
        write_a(scb, 
                cp, 
                fname, 
                fversion, 
                submod,
                (fname) ? typdef->prefix : NULL,
                typdef->typenamestr, 
                typ_get_named_type_linenum(typdef));
        ses_putstr(scb, (const xmlChar *)"</span>");
    } else {
        write_id(scb, typdef->typenamestr);
    }

}  /* write_type */


/********************************************************************
* FUNCTION write_errinfo
* 
* Generate the HTML for the specified error info struct
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   errinfo == ncx_errinfo_t struct to use
*   indent == start indent count
*
*********************************************************************/
static void
    write_errinfo (ses_cb_t *scb,
                   const yangdump_cvtparms_t *cp,
                   const ncx_errinfo_t *errinfo,
                   int32 indent)
{
    if (errinfo->error_message) {
        write_complex_str(scb, 
                          cp->tkc,
                          &errinfo->error_message, 
                          YANG_K_ERROR_MESSAGE,
                          errinfo->error_message, 
                          indent, 
                          2, 
                          TRUE);
    }
    if (errinfo->error_app_tag) {
        write_complex_str(scb, 
                          cp->tkc,
                          &errinfo->error_app_tag, 
                          YANG_K_ERROR_APP_TAG,
                          errinfo->error_app_tag, 
                          indent, 
                          2, 
                          TRUE);
    }
    if (errinfo->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &errinfo->descr,
                          YANG_K_DESCRIPTION,
                          errinfo->descr, 
                          indent, 
                          2, 
                          TRUE);
    }
    if (errinfo->ref) {
        write_reference_str(scb, errinfo->ref, indent);
    }
}  /* write_errinfo */


/********************************************************************
* FUNCTION write_musts
* 
* Generate the HTML for a Q of ncx_errinfo_t representing
* must-stmts, not just error info
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   mustQ == Q of xpath_pcb_t to use
*   indent == start indent count
*
*********************************************************************/
static void
    write_musts (ses_cb_t *scb,
                 const yangdump_cvtparms_t *cp,
                 const dlq_hdr_t *mustQ,
                 int32 indent)
{
    const xpath_pcb_t   *must;
    const ncx_errinfo_t *errinfo;

    for (must = (const xpath_pcb_t *)dlq_firstEntry(mustQ);
         must != NULL;
         must = (const xpath_pcb_t *)dlq_nextEntry(must)) {

        errinfo = &must->errinfo;
        if (errinfo->descr || 
            errinfo->ref ||
            errinfo->error_app_tag || 
            errinfo->error_message) {
            write_complex_str(scb, 
                              cp->tkc,
                              &must->exprstr,
                              YANG_K_MUST,
                              must->exprstr, 
                              indent, 
                              2, 
                              FALSE);
            write_errinfo(scb, 
                          cp,
                          errinfo, 
                          indent + ses_indent_count(scb));
            ses_putstr_indent(scb, END_SEC, indent);
        } else {
            write_complex_str(scb, 
                              cp->tkc,
                              &must->exprstr,
                              YANG_K_MUST,
                              must->exprstr, 
                              indent, 
                              2, 
                              TRUE);
        }
    }

}  /* write_musts */


/********************************************************************
* FUNCTION write_appinfoQ
* 
* Generate the HTML for a Q of ncx_appinfo_t representing
* vendor extensions entered with the YANG statements
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   appinfoQ == Q of ncx_appinfo_t to use
*   indent == start indent count
*
*********************************************************************/
static void
    write_appinfoQ (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    const dlq_hdr_t *appinfoQ,
                    int32 indent)
{
    const ncx_appinfo_t *appinfo;
    const xmlChar       *fname, *fversion, *submod;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    for (appinfo = (const ncx_appinfo_t *)dlq_firstEntry(appinfoQ);
         appinfo != NULL;
         appinfo = (const ncx_appinfo_t *)dlq_nextEntry(appinfo)) {

        ses_indent(scb, indent);
        if (appinfo->ext) {
            fname = NULL;
            fversion = NULL;
            if (appinfo->ext->tkerr.mod && 
                (appinfo->ext->tkerr.mod != mod)) {
                fname = appinfo->ext->tkerr.mod->name;
                fversion = appinfo->ext->tkerr.mod->version;
            }
            ses_putstr(scb, (const xmlChar *)"<span class=\"yang_kw\">");
            write_a(scb, 
                    cp, 
                    fname, 
                    fversion, 
                    submod, 
                    appinfo->prefix,
                    appinfo->name, 
                    appinfo->ext->tkerr.linenum);
            ses_putstr(scb, (const xmlChar *)"</span>");
        } else {
            write_extkw(scb, appinfo->prefix, appinfo->name);
        }

        if (appinfo->value) {
            ses_putchar(scb, ' ');
            write_str(scb, appinfo->value, 2);
        }

        if (!dlq_empty(appinfo->appinfoQ)) {
            ses_putstr(scb, START_SEC);
            write_appinfoQ(scb, 
                           mod, 
                           cp, 
                           appinfo->appinfoQ,
                           indent+ses_indent_count(scb));
            ses_putstr_indent(scb, END_SEC, indent);
        } else {
            ses_putchar(scb, ';');
        }
    }

}  /* write_appinfoQ */


/********************************************************************
* FUNCTION write_type_contents
* 
* Generate the HTML for the specified type
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typdef == typ_def_t to use
*   obj == object template if this is from a leaf or leaf-list
*          NULL if this is from a typedef
*   startindent == start indent count
*
*********************************************************************/
static void
    write_type_contents (ses_cb_t *scb,
                         const ncx_module_t *mod,
                         const yangdump_cvtparms_t *cp,
                         typ_def_t *typdef,
                         obj_template_t *obj,
                         int32 startindent)
{
    typ_unionnode_t       *un;
    const typ_enum_t      *bit, *enu;
    const typ_range_t     *range;
    const typ_pattern_t   *pat;
    const typ_idref_t     *idref;
    char                   buff[NCX_MAX_NUMLEN];
    int32                  indent;
    boolean                errinfo_set, constrained_set;

    indent = startindent + ses_indent_count(scb);

    write_appinfoQ(scb, mod, cp, &typdef->appinfoQ, startindent);

    switch (typdef->tclass) {
    case NCX_CL_BASE:
        break;
    case NCX_CL_NAMED:
        if (typdef->def.named.newtyp) {
            typdef = typdef->def.named.newtyp;
        } else {
            break;
        }
        /* fall through if typdef set */
    case NCX_CL_SIMPLE:
        switch (typdef->def.simple.btyp) {
        case NCX_BT_UNION:
            for (un = typ_first_unionnode(typdef);
                 un != NULL;
                 un = (typ_unionnode_t *)dlq_nextEntry(un)) {
                if (un->typdef) {
                    write_type_clause(scb, mod, cp,
                                      un->typdef, NULL, startindent);
                } else if (un->typ) {
                    write_type_clause(scb, mod, cp, 
                                      &un->typ->typdef, NULL, startindent);
                } else {
                    SET_ERROR(ERR_INTERNAL_VAL);
                }
            }
            break;
        case NCX_BT_BITS:
            for (bit = typ_first_con_enumdef(typdef);
                 bit != NULL;
                 bit = (const typ_enum_t *)dlq_nextEntry(bit)) {

                write_complex_str(scb, 
                                  cp->tkc,
                                  &bit->name,
                                  YANG_K_BIT, 
                                  bit->name,
                                  startindent, 
                                  0, 
                                  FALSE);

                write_appinfoQ(scb, mod, cp, &bit->appinfoQ, indent);

                sprintf(buff, "%u", bit->pos);
                write_simple_str(scb, 
                                 YANG_K_POSITION,
                                 (const xmlChar *)buff,
                                 indent, 
                                 0, 
                                 TRUE);

                write_status(scb, bit->status, indent);

                if (bit->descr) {
                    write_complex_str(scb, 
                                      cp->tkc,
                                      &bit->descr,
                                      YANG_K_DESCRIPTION,
                                      bit->descr, 
                                      indent, 
                                      2, 
                                      TRUE);
                }

                if (bit->ref) {
                    write_reference_str(scb, bit->ref, indent);
                }

                ses_putstr_indent(scb, END_SEC, startindent);
            }
            break;
        case NCX_BT_ENUM:
            for (enu = typ_first_con_enumdef(typdef);
                 enu != NULL;
                 enu = (const typ_enum_t *)dlq_nextEntry(enu)) {

                write_complex_str(scb,
                                  cp->tkc,
                                  &enu->name,
                                  YANG_K_ENUM, 
                                  enu->name,
                                  startindent, 
                                  2, 
                                  FALSE);

                write_appinfoQ(scb, mod, cp, &enu->appinfoQ, indent);

                sprintf(buff, "%d", enu->val);
                write_simple_str(scb, 
                                 YANG_K_VALUE,
                                 (const xmlChar *)buff,
                                 indent, 
                                 0, 
                                 TRUE);

                write_status(scb, enu->status, indent);

                if (enu->descr) {
                    write_complex_str(scb, 
                                      cp->tkc,
                                      &enu->descr,
                                      YANG_K_DESCRIPTION,
                                      enu->descr, 
                                      indent, 
                                      2, 
                                      TRUE);
                }

                if (enu->ref) {
                    write_reference_str(scb, enu->ref, indent);
                }

                ses_putstr_indent(scb, END_SEC,  startindent);
            }
            break;
        case NCX_BT_EMPTY:
        case NCX_BT_BOOLEAN:
            /* appinfo only */
            break;
        case NCX_BT_DECIMAL64:
            sprintf(buff, "%d", typ_get_fraction_digits(typdef));
            write_simple_str(scb,
                             YANG_K_FRACTION_DIGITS,
                             (const xmlChar *)buff,
                             startindent,
                             0,
                             TRUE);
            /* fall through to check range */
        case NCX_BT_INT8:
        case NCX_BT_INT16:
        case NCX_BT_INT32:
        case NCX_BT_INT64:
        case NCX_BT_UINT8:
        case NCX_BT_UINT16:
        case NCX_BT_UINT32:
        case NCX_BT_UINT64:
        case NCX_BT_FLOAT64:
            range = typ_get_crange_con(typdef);
            if (range && range->rangestr) {
                errinfo_set = ncx_errinfo_set(&range->range_errinfo);
                write_simple_str(scb,
                                 YANG_K_RANGE,
                                 range->rangestr,
                                 startindent,
                                 2, 
                                 !errinfo_set);
                if (errinfo_set) {
                    write_errinfo(scb, 
                                  cp, 
                                  &range->range_errinfo,
                                  indent);
                    ses_putstr_indent(scb, END_SEC, startindent);
                }
            }
            break;
        case NCX_BT_STRING:
        case NCX_BT_BINARY:
            range = typ_get_crange_con(typdef);
            if (range && range->rangestr) {
                errinfo_set = ncx_errinfo_set(&range->range_errinfo);
                write_simple_str(scb,
                                 YANG_K_LENGTH,
                                 range->rangestr,
                                 startindent,
                                 2,
                                 !errinfo_set);
                if (errinfo_set) {
                    write_errinfo(scb,
                                  cp, 
                                  &range->range_errinfo,
                                  indent);
                    ses_putstr_indent(scb, END_SEC, startindent);
                }
            }

            for (pat = typ_get_first_cpattern(typdef);
                 pat != NULL;
                 pat = typ_get_next_cpattern(pat)) {

                errinfo_set = ncx_errinfo_set(&pat->pat_errinfo);
                write_complex_str(scb, 
                                  cp->tkc,
                                  &pat->pat_str,
                                  YANG_K_PATTERN, 
                                  pat->pat_str,
                                  startindent, 
                                  1, 
                                  !errinfo_set);
                if (errinfo_set) {
                    write_errinfo(scb, 
                                  cp, 
                                  &pat->pat_errinfo,
                                  indent);
                    ses_putstr_indent(scb, END_SEC, startindent);
                }
            }
            break;
        case NCX_BT_SLIST:
            break;
        case NCX_BT_LEAFREF:
            write_leafref_path(scb, mod, cp, typdef, obj, startindent);
            break;
        case NCX_BT_INSTANCE_ID:
            constrained_set = typ_get_constrained(typdef);
            write_simple_str(scb, 
                             YANG_K_REQUIRE_INSTANCE,
                             (constrained_set) 
                             ? NCX_EL_TRUE : NCX_EL_FALSE,
                             startindent, 
                             0, 
                             TRUE);
            break;
        case NCX_BT_IDREF:
            idref = typ_get_cidref(typdef);
            if (idref) {
                write_idref_base(scb, 
                                 mod, 
                                 cp, 
                                 idref, 
                                 startindent);
            }
            break;
        default:
            break;
        }
        break;
    case NCX_CL_COMPLEX:
        SET_ERROR(ERR_INTERNAL_VAL);
        break;
    case NCX_CL_REF:
        SET_ERROR(ERR_INTERNAL_VAL);
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* write_type_contents */


/********************************************************************
* FUNCTION write_type_clause
* 
* Generate the HTML for the specified type clause
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typdef == typ_def_t to use
*   obj == object template if this is from a leaf or leaf-list
*          NULL if this is from a typedef
*   startindent == start indent count
*
*********************************************************************/
static void
    write_type_clause (ses_cb_t *scb,
                       const ncx_module_t *mod,
                       const yangdump_cvtparms_t *cp,
                       typ_def_t *typdef,
                       obj_template_t *obj,
                       int32 startindent)
{
    write_type(scb, mod, cp, typdef, startindent);
    if (typ_has_subclauses(typdef)) {
        ses_putstr(scb, START_SEC);
        write_type_contents(scb, 
                            mod, 
                            cp, 
                            typdef,
                            obj,
                            startindent + ses_indent_count(scb));
        ses_putstr_indent(scb, END_SEC, startindent);
    } else {
        ses_putchar(scb, ';');
    }
}  /* write_type_clause */


/********************************************************************
* FUNCTION write_typedef
* 
* Generate the HTML for 1 typedef
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typ == typ_template_t to use
*   startindent == start indent count
*   first == TRUE if this is the first object at this indent level
*            FALSE if not the first object at this indent level
*********************************************************************/
static void
    write_typedef (ses_cb_t *scb,
                   const ncx_module_t *mod,
                   const yangdump_cvtparms_t *cp,
                   typ_template_t *typ,
                   int32 startindent,
                   boolean first)
{
    const xmlChar           *submod;
    int32                    indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);

    write_href_id(scb, 
                  submod, 
                  YANG_K_TYPEDEF, 
                  typ->name, 
                  startindent, 
                  typ_get_typ_linenum(typ),
                  FALSE, 
                  !first);

    /* appinfoQ */
    write_appinfoQ(scb, mod, cp, &typ->appinfoQ, indent);

    /* type field */
    write_type_clause(scb, mod, cp, &typ->typdef, NULL, indent);

    /* units field */
    if (typ->units) {
        write_simple_str(scb, 
                         YANG_K_UNITS, 
                         typ->units, 
                         indent, 
                         2, 
                         TRUE);
    }

    /* default field */
    if (typ->defval) {
        write_complex_str(scb, 
                          cp->tkc,
                          &typ->defval,
                          YANG_K_DEFAULT, 
                          typ->defval, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* status field */
    write_status(scb, typ->status, indent);

    /* description field */
    if (typ->descr) {
        write_complex_str(scb, 
                          cp->tkc,
                          &typ->descr,
                          YANG_K_DESCRIPTION, 
                          typ->descr, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* reference field */
    if (typ->ref) {
        write_reference_str(scb, typ->ref, indent);
    }

    /* end typedef clause */
    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_typedef */


/********************************************************************
* FUNCTION write_typedefs
* 
* Generate the HTML for the specified typedefQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   typedefQ == que of typ_template_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_typedefs (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    dlq_hdr_t *typedefQ,
                    int32 startindent)
{
    typ_template_t    *typ;
    boolean            first;

    if (dlq_empty(typedefQ)) {
        return;
    }

    if (typedefQ == &mod->typeQ) {
        write_banner_cmt(scb, 
                         mod, 
                         cp,
                         (const xmlChar *)"typedefs", 
                         startindent);
    }

    first = TRUE;
    for (typ = (typ_template_t *)dlq_firstEntry(typedefQ);
         typ != NULL;
         typ = (typ_template_t *)dlq_nextEntry(typ)) {

        write_typedef(scb, mod, cp, typ, startindent, first);
        first = FALSE;
    }

}  /* write_typedefs */


/********************************************************************
* FUNCTION write_grouping
* 
* Generate the HTML for 1 grouping
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   grp == grp_template_t to use
*   startindent == start indent count
*   first == TRUE if this is the first object at this indent level
*            FALSE if not the first object at this indent level
*********************************************************************/
static void
    write_grouping (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    grp_template_t *grp,
                    int32 startindent,
                    boolean first)
{
    const xmlChar           *submod;
    int32                    indent;
    boolean                  cooked;

    cooked = (strcmp(cp->objview, OBJVIEW_COOKED)) ? FALSE : TRUE;
    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
        
    indent = startindent + ses_indent_count(scb);

    if (cooked && !grp_has_typedefs(grp)) {
        return;
    }
        
    write_href_id(scb, 
                  submod, 
                  YANG_K_GROUPING, 
                  grp->name,
                  startindent,
                  grp->tkerr.linenum, 
                  FALSE,
                  !first);

    /* appinfoQ */
    write_appinfoQ(scb, mod, cp, &grp->appinfoQ, indent);

    /* status field */
    write_status(scb, grp->status, indent);

    /* description field */
    if (grp->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &grp->descr,
                          YANG_K_DESCRIPTION, 
                          grp->descr, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* reference field */
    if (grp->ref) {
        write_reference_str(scb, grp->ref, indent);
    }

    write_typedefs(scb, mod, cp, &grp->typedefQ, indent);

    write_groupings(scb, mod, cp, &grp->groupingQ, indent);

    if (!cooked) {
        write_objects(scb, mod, cp, &grp->datadefQ, indent);
    }

    /* end grouping clause */
    ses_putstr_indent(scb, END_SEC, startindent);

    /* end grouping comment */
    write_endsec_cmt(scb, YANG_K_GROUPING, grp->name);

}  /* write_grouping */


/********************************************************************
* FUNCTION write_groupings
* 
* Generate the HTML for the specified groupingQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   groupingQ == que of grp_template_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_groupings (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     dlq_hdr_t *groupingQ,
                     int32 startindent)
{
    grp_template_t          *grp;
    boolean                  needed, cooked, first;

    if (dlq_empty(groupingQ)) {
        return;
    }

    cooked = (strcmp(cp->objview, OBJVIEW_COOKED)) ? FALSE : TRUE;

    /* groupings are only generated in cooked mode if they have
     * typedefs, and then just the typedefs are generated
     */
    if (cooked) {
        needed = FALSE;
        for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
             grp != NULL && needed==FALSE;
             grp = (grp_template_t *)dlq_nextEntry(grp)) {
            needed = grp_has_typedefs(grp);
        }
        if (!needed) {
            return;
        }
    }

    /* put comment for first grouping only */
    if (groupingQ == &mod->groupingQ) {
        write_banner_cmt(scb, 
                         mod, 
                         cp,
                         (const xmlChar *)"groupings", 
                         startindent);
    }

    first = TRUE;
    for (grp = (grp_template_t *)dlq_firstEntry(groupingQ);
         grp != NULL;
         grp = (grp_template_t *)dlq_nextEntry(grp)) {

        write_grouping(scb, mod, cp, grp, startindent, first);
        first = FALSE;
    }

}  /* write_groupings */


/********************************************************************
* FUNCTION write_iffeature
* 
* Generate the HTML for 1 if-feature statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   iffeature == ncx_iffeature_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_iffeature (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     const ncx_iffeature_t *iffeature,
                     int32 startindent)
{
    const xmlChar     *fname, *fversion, *submod;
    uint32             linenum;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    fname = NULL;
    fversion = NULL;
    linenum = 0;

    ses_indent(scb, startindent);
    write_kw(scb, YANG_K_IF_FEATURE);
    ses_putchar(scb, ' ');

    /* get filename if-feature-stmt filled in */
    if (iffeature->feature) {
        linenum = iffeature->feature->tkerr.linenum;
        fname = iffeature->feature->tkerr.mod->name;
        fversion = iffeature->feature->tkerr.mod->version;
    }

    ses_putstr(scb, (const xmlChar *)"<span class=\"yang_id\">");
    write_a(scb, 
            cp, 
            fname, 
            fversion, 
            submod,
            iffeature->prefix,
            iffeature->name, 
            linenum);
    ses_putstr(scb, (const xmlChar *)"</span>");
    ses_putchar(scb, ';');

}  /* write_iffeature */


/********************************************************************
* FUNCTION write_iffeatureQ
* 
* Generate the HTML for a Q of if-feature statements
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   iffeatureQ == Q of ncx_iffeature_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_iffeatureQ (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const dlq_hdr_t *iffeatureQ,
                      int32 startindent)
{
    const ncx_iffeature_t   *iffeature;

    for (iffeature = (const ncx_iffeature_t *)
             dlq_firstEntry(iffeatureQ);
         iffeature != NULL;
         iffeature = (const ncx_iffeature_t *)
             dlq_nextEntry(iffeature)) {

        write_iffeature(scb, mod, cp, iffeature, startindent);
    }

}  /* write_iffeatureQ */


/********************************************************************
* FUNCTION write_when
* 
* Check then when-stmt for an object
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_when (ses_cb_t *scb,
                const yangdump_cvtparms_t *cp,
                const obj_template_t *obj,
                int32 indent)
{
    const ncx_errinfo_t *errinfo;

    /* when-stmt? */
    if (obj->when && obj->when->exprstr) {

        errinfo = &obj->when->errinfo;
        if (errinfo->descr || errinfo->ref) {
            write_complex_str(scb, 
                              cp->tkc,
                              &obj->when->exprstr, 
                              YANG_K_WHEN,
                              obj->when->exprstr, 
                              indent, 
                              2, 
                              FALSE);
            write_errinfo(scb,
                          cp,
                          errinfo,
                          indent + ses_indent_count(scb));
            ses_putstr_indent(scb, END_SEC, indent);
        } else {
            write_complex_str(scb, 
                              cp->tkc,
                              &obj->when->exprstr, 
                              YANG_K_WHEN,
                              obj->when->exprstr, 
                              indent, 
                              2, 
                              TRUE);
        }
    }

}  /* write_when */


/********************************************************************
* FUNCTION write_presence_stmt
* 
* Write the presence-stmt
*
* INPUTS:
*   scb == session control block to use for writing
*   tkc == token chain to use
*   field == address of presence field
*   presence == presence string (may be NULL to skip)
*   indent = indent amount
*********************************************************************/
static void
    write_presence_stmt (ses_cb_t *scb,
                         tk_chain_t *tkc,
                         void *field,
                         const xmlChar *presence,
                         int32 indent)
{
    if (presence) {
        write_complex_str(scb, 
                          tkc,
                          field,
                          YANG_K_PRESENCE, 
                          presence, 
                          indent, 
                          2, 
                          TRUE);
    }

}  /* write_presence_stmt */


/********************************************************************
* FUNCTION write_sdr
* 
* Write the status-stmt, description-stmt, and reference-stmt
*
* INPUTS:
*   scb == session control block to use for writing
*   cp == conversion parameters to use
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_sdr (ses_cb_t *scb,
               const yangdump_cvtparms_t *cp,
               const obj_template_t *obj,
               int32 indent)
{
    const xmlChar *str;

    /* status-stmt? (only written if not 'current' */
    write_status(scb, obj_get_status(obj), indent);

    /* description-stmt? */
    str = obj_get_description(obj);
    if (str) {
        write_complex_str(scb, 
                          cp->tkc,
                          obj_get_description_addr(obj),
                          YANG_K_DESCRIPTION, 
                          str,
                          indent, 
                          2, 
                          TRUE);
    }
            
    /* reference-stmt? */
    str = obj_get_reference(obj);
    if (str) {
        write_reference_str(scb, str, indent);
    }

}  /* write_sdr */


/********************************************************************
* FUNCTION write_config_stmt
* 
* Write the config-stmt
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_config_stmt (ses_cb_t *scb,
                       const obj_template_t *obj,
                       int32 indent)
{
    /* config-stmt, only if actually set to false */
    if (obj->flags & OBJ_FL_CONFSET) {
        if (!(obj->flags & OBJ_FL_CONFIG)) {
            write_simple_str(scb, 
                             YANG_K_CONFIG, 
                             NCX_EL_FALSE,
                             indent, 
                             0, 
                             TRUE);
        }
    }

}  /* write_config_stmt */


/********************************************************************
* FUNCTION write_config_stmt_force
* 
* Write the config-stmt; force output even if default
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_config_stmt_force (ses_cb_t *scb,
                             const obj_template_t *obj,
                             int32 indent)
{
    boolean flag;

    /* config-stmt */
    if (obj->flags & OBJ_FL_CONFSET) {
        flag = (obj->flags & OBJ_FL_CONFIG);
        write_simple_str(scb, 
                         YANG_K_CONFIG, 
                         (flag) ? NCX_EL_TRUE : NCX_EL_FALSE,
                         indent, 
                         0, 
                         TRUE);
    }

}  /* write_config_stmt_force */


/********************************************************************
* FUNCTION write_mandatory_stmt
* 
* Write the mandatory-stmt
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_mandatory_stmt (ses_cb_t *scb,
                          const obj_template_t *obj,
                          int32 indent)
{

    /* mandatory field, only if actually set to true */
    if (obj->flags & OBJ_FL_MANDSET) {
        if (obj->flags & OBJ_FL_MANDATORY) {
            write_simple_str(scb, 
                             YANG_K_MANDATORY,
                             NCX_EL_TRUE,
                             indent,
                             0,
                             TRUE);
        }
    }

}  /* write__mandatory_stmt */


/********************************************************************
* FUNCTION write_mandatory_stmt_force
* 
* Write the mandatory-stmt; force output even if default
*
* INPUTS:
*   scb == session control block to use for writing
*   obj == object to check
*   indent = indent amount
*********************************************************************/
static void
    write_mandatory_stmt_force (ses_cb_t *scb,
                                const obj_template_t *obj,
                                int32 indent)
{
    boolean flag;

    /* mandatory field, only if actually set to true */
    if (obj->flags & OBJ_FL_MANDSET) {
        flag = (obj->flags & OBJ_FL_MANDATORY);
        write_simple_str(scb, 
                         YANG_K_MANDATORY,
                         (flag) ? NCX_EL_TRUE : NCX_EL_FALSE,
                         indent,
                         0,
                         TRUE);
    }

}  /* write__mandatory_stmt_force */


/********************************************************************
* FUNCTION write_minmax
* 
* Write the min-elements and/or max-elements stmts if needed
*
* INPUTS:
*   scb == session control block to use for writing
*   minset == TRUE if min-elements set
*   minval == min-elements value
*   maxset == TRUE if max-elements set
*   maxval == min-elements value
*   indent == indent amount
*********************************************************************/
static void
    write_minmax (ses_cb_t *scb,
                  boolean minset,
                  uint32 minval,
                  boolean maxset,
                  uint32 maxval,
                  int32 indent)
{
    char    buff[NCX_MAX_NUMLEN];

    if (minset) {
        sprintf(buff, "%u", minval);
        write_simple_str(scb, 
                         YANG_K_MIN_ELEMENTS, 
                         (const xmlChar *)buff,
                         indent, 
                         0, 
                         TRUE);
    }

    if (maxset) {
        if (maxval == 0) {
            write_simple_str(scb, 
                             YANG_K_MAX_ELEMENTS, 
                             YANG_K_UNBOUNDED,
                             indent, 
                             0, 
                             TRUE);
        } else {
            sprintf(buff, "%u", maxval);
            write_simple_str(scb, 
                             YANG_K_MAX_ELEMENTS, 
                             (const xmlChar *)buff,
                             indent, 
                             0, 
                             TRUE);
        }
    }

}  /* write_minmax */


/********************************************************************
* FUNCTION write_unique_stmts
* 
* Generate the HTML for the specified uniqueQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   uniqueQ == que of obj_unique_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_unique_stmts (ses_cb_t *scb,
                        const ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        const dlq_hdr_t *uniqueQ,
                        int32 startindent)
{
    const obj_unique_t      *uni;
    const obj_unique_comp_t *unicomp, *nextunicomp;
    const xmlChar           *submod;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    for (uni = (const obj_unique_t *)dlq_firstEntry(uniqueQ);
         uni != NULL;
         uni = (const obj_unique_t *)dlq_nextEntry(uni)) {

        ses_indent(scb, startindent);
        write_kw(scb, YANG_K_UNIQUE);
        ses_putstr(scb, (const xmlChar *)" \"");

        for (unicomp = (obj_unique_comp_t *)
                 dlq_firstEntry(&uni->compQ);
             unicomp != NULL; 
             unicomp = nextunicomp) {
            nextunicomp = (obj_unique_comp_t *)
                dlq_nextEntry(unicomp);
            write_a2(scb,
                     cp, 
                     NULL,
                     NULL,
                     submod,
                     obj_get_name(unicomp->unobj),
                     unicomp->unobj->tkerr.linenum,
                     unicomp->xpath);
            if (nextunicomp) {
                ses_putstr(scb, (const xmlChar *)" ");
            }
        }
        ses_putstr(scb, (const xmlChar *)"\";");
    }

}  /* write_unique_stmts */


/********************************************************************
* FUNCTION write_object
* 
* Generate the HTML for the 1 datadef
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   obj == obj_template_t to use
*   startindent == start indent count
*   first == TRUE if this is the first object at this indent level
*            FALSE if not the first object at this indent level
* RETURNS:
*   status (usually NO_ERR or ERR_NCX_SKIPPED)
*********************************************************************/
static status_t
    write_object (ses_cb_t *scb,
                  const ncx_module_t *mod,
                  const yangdump_cvtparms_t *cp,
                  obj_template_t *obj,
                  int32 startindent,
                  boolean first)
{
    obj_container_t   *con;
    obj_leaf_t        *leaf;
    obj_leaflist_t    *leaflist;
    obj_list_t        *list;
    obj_choice_t      *choic;
    obj_case_t        *cas;
    obj_uses_t        *uses;
    obj_augment_t     *aug;
    obj_rpc_t         *rpc;
    obj_rpcio_t       *rpcio;
    obj_notif_t       *notif;
    obj_refine_t      *refine;
    obj_key_t         *key, *nextkey;
    const xmlChar     *fname, *fversion, *submod;
    int32              indent;
    boolean            isempty, fullcase;

    if (obj_is_cloned(obj) && cp->rawmode) {
        /* skip cloned objects in 'raw' object view mode */
        return ERR_NCX_SKIPPED;
    }

    if (cp->rawmode) {
        isempty = obj_is_empty(obj);
    } else {
        isempty = FALSE;
    }
    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);
    fullcase = !obj_is_short_case(obj);

    /* generate start of statement with the object type
     * except a short-form case-stmt
     */
    if (fullcase && 
        obj_has_name(obj) && 
        obj->objtype != OBJ_TYP_RPCIO) {

        write_href_id(scb, 
                      submod, 
                      obj_get_typestr(obj), 
                      obj_get_name(obj),
                      startindent, 
                      obj->tkerr.linenum, 
                      isempty, 
                      !first);
        if (isempty) {
            return NO_ERR;
        }
    }

    switch (obj->objtype) {
    case OBJ_TYP_ANYXML:
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_musts(scb, cp, obj_get_mustQ(obj), indent);
        write_config_stmt(scb, obj, indent);
        write_mandatory_stmt(scb, obj, indent);
        write_sdr(scb, cp, obj, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        break;
    case OBJ_TYP_CONTAINER:
        con = obj->def.container;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_musts(scb, cp, obj_get_mustQ(obj), indent);
        write_presence_stmt(scb, 
                            cp->tkc, 
                            obj_get_presence_string_field(obj),
                            obj_get_presence_string(obj),
                            indent);
        write_config_stmt(scb, obj, indent);
        write_sdr(scb, cp, obj, indent);
        write_typedefs(scb, mod, cp, con->typedefQ, indent);
        write_groupings(scb, mod, cp, con->groupingQ, indent);
        write_objects(scb, mod, cp, con->datadefQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        write_endsec_cmt(scb, YANG_K_CONTAINER, con->name);
        break;
    case OBJ_TYP_LEAF:
        leaf = obj->def.leaf;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_type_clause(scb, mod, cp, leaf->typdef, obj, indent);

        /* units clause */
        if (leaf->units) {
            write_simple_str(scb, 
                             YANG_K_UNITS, 
                             leaf->units,
                             indent, 
                             2, 
                             TRUE);
        }
        write_musts(scb, cp, obj_get_mustQ(obj), indent);

        if (leaf->defval) {
            write_complex_str(scb, 
                              cp->tkc,
                              &leaf->defval,
                              YANG_K_DEFAULT, 
                              leaf->defval, 
                              indent, 
                              2, 
                              TRUE);
        }

        write_config_stmt(scb, obj, indent);
        write_mandatory_stmt(scb, obj, indent);
        write_sdr(scb, cp, obj, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        break;
    case OBJ_TYP_LEAF_LIST:
        leaflist = obj->def.leaflist;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_type_clause(scb, mod, cp, leaflist->typdef, obj, indent);
        if (leaflist->units) {
            write_simple_str(scb,
                             YANG_K_UNITS,
                             leaflist->units,
                             indent,
                             2,
                             TRUE);
        }
        write_musts(scb, cp, obj_get_mustQ(obj), indent);
        write_config_stmt(scb, obj, indent);
        write_minmax(scb,
                     leaflist->minset,
                     leaflist->minelems,
                     leaflist->maxset,
                     leaflist->maxelems,
                     indent);
        if (!leaflist->ordersys) {
            write_simple_str(scb, 
                             YANG_K_ORDERED_BY, 
                             YANG_K_USER,
                             indent, 
                             0, 
                             TRUE);
        }
        write_sdr(scb, cp, obj, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        break;
    case OBJ_TYP_LIST:
        list = obj->def.list;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_musts(scb, cp, obj_get_mustQ(obj), indent);

        /* key field, manual generation to make links */
        if (!dlq_empty(&list->keyQ)) {
            ses_indent(scb, indent);
            write_kw(scb, YANG_K_KEY);
            ses_putstr(scb, (const xmlChar *)" \"");

            for (key = obj_first_key(obj);
                 key != NULL; key = nextkey) {
                nextkey = obj_next_key(key);
                write_a(scb,
                        cp,
                        NULL,
                        NULL,
                        submod,
                        NULL,
                        obj_get_name(key->keyobj),
                        key->keyobj->tkerr.linenum);
                if (nextkey) {
                    ses_putchar(scb, ' ');
                }
            }
            ses_putstr(scb, (const xmlChar *)"\";");
        }

        write_unique_stmts(scb, mod, cp, &list->uniqueQ, indent);
        write_config_stmt(scb, obj, indent);
        write_minmax(scb,
                     list->minset,
                     list->minelems,
                     list->maxset,
                     list->maxelems,
                     indent);

        /* ordered-by field */
        if (!list->ordersys) {
            write_simple_str(scb, 
                             YANG_K_ORDERED_BY, 
                             YANG_K_USER,
                             indent, 
                             0, 
                             TRUE);
        }

        write_sdr(scb, cp, obj, indent);
        write_typedefs(scb, mod, cp, list->typedefQ, indent);
        write_groupings(scb, mod, cp, list->groupingQ, indent);
        write_objects(scb, mod, cp, list->datadefQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        write_endsec_cmt(scb, YANG_K_LIST, list->name);
        break;
    case OBJ_TYP_CHOICE:
        choic = obj->def.choic;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        if (choic->defval) {
            write_complex_str(scb,
                              cp->tkc,
                              &choic->defval,
                              YANG_K_DEFAULT, 
                              choic->defval,
                              indent,
                              2,
                              TRUE);
        }
        write_mandatory_stmt(scb, obj, indent);
        write_sdr(scb, cp, obj, indent);
        write_objects(scb, mod, cp, choic->caseQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        write_endsec_cmt(scb, YANG_K_CHOICE, choic->name);
        break;
    case OBJ_TYP_CASE:
        cas = obj->def.cas;
        if (fullcase) {
            write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
            write_when(scb, cp, obj, indent);
            write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
            write_sdr(scb, cp, obj, indent);
        }

        write_objects(scb, 
                      mod, 
                      cp, 
                      cas->datadefQ, 
                      (fullcase) ? indent : startindent);

        if (fullcase) {
            ses_putstr_indent(scb, END_SEC, startindent);
            write_endsec_cmt(scb, YANG_K_CASE, cas->name);
        }
        break;
    case OBJ_TYP_USES:
        if (!cp->rawmode) {
            return ERR_NCX_SKIPPED;
        }
        uses = obj->def.uses;


        if (!first) {
            ses_putchar(scb, '\n');
        }

        ses_indent(scb, startindent);
        write_id_a(scb, submod, YANG_K_USES, obj->tkerr.linenum);
        write_kw(scb, YANG_K_USES);
        ses_putchar(scb, ' ');
        fname = NULL;
        fversion = NULL;
        if (uses->prefix && 
            xml_strcmp(uses->prefix, mod->prefix)) {
            if (uses->grp && uses->grp->tkerr.mod) {
                fname = uses->grp->tkerr.mod->name;
                fversion = uses->grp->tkerr.mod->version;
            }
        }
        write_a(scb,
                cp, 
                fname,
                fversion, 
                submod,
                uses->prefix,
                uses->name,
                uses->grp->tkerr.linenum);
        if (uses->descr || 
            uses->ref || 
            (obj->when && obj->when->exprstr) ||
            uses->status != NCX_STATUS_CURRENT ||
            !dlq_empty(uses->datadefQ) ||
            !dlq_empty(&obj->appinfoQ)) {

            ses_putstr(scb, START_SEC);
            write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
            write_when(scb, cp, obj, indent);
            write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
            write_sdr(scb, cp, obj, indent);
            write_objects(scb, mod, cp, uses->datadefQ, indent);
            ses_putstr_indent(scb, END_SEC, startindent);
        } else {
            ses_putchar(scb, ';');
        }
        break;
    case OBJ_TYP_AUGMENT:
        if (!cp->rawmode) {
            return ERR_NCX_SKIPPED;
        }

        aug = obj->def.augment;

        if (!first) {
            ses_putchar(scb, '\n');
        }

        ses_indent(scb, startindent);
        write_id_a(scb, submod, YANG_K_AUGMENT, obj->tkerr.linenum);
        write_kw(scb, YANG_K_AUGMENT);
        ses_putchar(scb, ' ');
        fname = NULL;
        fversion = NULL;
        if (aug->targobj && aug->targobj->tkerr.mod &&
            aug->targobj->tkerr.mod != mod) {
            fname = aug->targobj->tkerr.mod->name;
            fversion = aug->targobj->tkerr.mod->version;
        }
        write_a2(scb,
                 cp, 
                 fname, 
                 fversion, 
                 submod,
                 obj_get_name(aug->targobj), 
                 aug->targobj->tkerr.linenum, 
                 aug->target);

        if (isempty) {
            ses_putchar(scb, ';');
            return NO_ERR;
        }

        ses_putstr(scb, START_SEC);
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_when(scb, cp, obj, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_sdr(scb, cp, obj, indent);
        write_objects(scb, mod, cp, &aug->datadefQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        break;
    case OBJ_TYP_RPC:
        rpc = obj->def.rpc;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_sdr(scb, cp, obj, indent);
        write_typedefs(scb, mod, cp, &rpc->typedefQ, indent);
        write_groupings(scb, mod, cp, &rpc->groupingQ, indent);
        write_objects(scb, mod, cp, &rpc->datadefQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        write_endsec_cmt(scb, YANG_K_RPC, rpc->name);
        break;
    case OBJ_TYP_RPCIO:
        rpcio = obj->def.rpcio;

        if (!dlq_empty(&rpcio->typedefQ) ||
            !dlq_empty(&rpcio->groupingQ) ||
            !dlq_empty(&rpcio->datadefQ) ||
            !dlq_empty(&obj->appinfoQ)) {

            write_href_id(scb, 
                          submod, 
                          obj_get_name(obj), 
                          NULL,
                          startindent, 
                          obj->tkerr.linenum,
                          FALSE, 
                          !first);

            write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
            write_typedefs(scb, mod, cp, &rpcio->typedefQ, indent);
            write_groupings(scb, mod, cp, &rpcio->groupingQ, indent);
            write_objects(scb, mod, cp, &rpcio->datadefQ, indent);
            ses_putstr_indent(scb, END_SEC, startindent);
        }
        break;
    case OBJ_TYP_NOTIF:
        notif = obj->def.notif;
        write_appinfoQ(scb, mod, cp, &obj->appinfoQ, indent);
        write_iffeatureQ(scb, mod, cp, &obj->iffeatureQ, indent);
        write_sdr(scb, cp, obj, indent);
        write_typedefs(scb, mod, cp, &notif->typedefQ, indent);
        write_groupings(scb, mod, cp, &notif->groupingQ, indent);
        write_objects(scb, mod, cp, &notif->datadefQ, indent);
        ses_putstr_indent(scb, END_SEC, startindent);
        write_endsec_cmt(scb, YANG_K_NOTIFICATION, notif->name);
        break;
    case OBJ_TYP_REFINE:
        if (!cp->rawmode) {
            return ERR_NCX_SKIPPED;
        }
        refine = obj->def.refine;
        if (!first) {
            ses_putchar(scb, '\n');
        }
        ses_indent(scb, startindent);
        write_id_a(scb, 
                   submod, 
                   YANG_K_REFINE, 
                   obj->tkerr.linenum);
        write_kw(scb, YANG_K_REFINE);
        ses_putchar(scb, ' ');
        fname = NULL;
        fversion = NULL;
        if (refine->targobj && refine->targobj->tkerr.mod &&
            refine->targobj->tkerr.mod != mod) {
            fname = refine->targobj->tkerr.mod->name;
            fversion = refine->targobj->tkerr.mod->version;
        }
        write_a2(scb,
                 cp, 
                 fname, 
                 fversion, 
                 submod,
                 obj_get_name(refine->targobj), 
                 refine->targobj->tkerr.linenum, 
                 refine->target);

        if (isempty) {
            ses_putchar(scb, ';');
            return NO_ERR;
        }

        ses_putstr(scb, START_SEC);

        switch (refine->targobj->objtype) {
        case OBJ_TYP_ANYXML:
            /* must-stmt refine not in -07*/
            write_musts(scb, cp, obj_get_mustQ(obj), indent); 
            write_config_stmt_force(scb, obj, indent);
            write_mandatory_stmt_force(scb, obj, indent);
            write_sdr(scb, cp, obj, indent);
            break;
        case OBJ_TYP_CONTAINER:
            write_musts(scb, cp, obj_get_mustQ(obj), indent); 
            write_presence_stmt(scb, 
                                cp->tkc,
                                &refine->presence, 
                                refine->presence,
                                indent);
            write_config_stmt_force(scb, obj, indent);
            write_sdr(scb, cp, obj, indent);
            break;
        case OBJ_TYP_LEAF:
            write_musts(scb, cp, obj_get_mustQ(obj), indent); 
            if (refine->def) {
                write_complex_str(scb, 
                                  cp->tkc,
                                  &refine->def,
                                  YANG_K_DEFAULT, 
                                  refine->def, 
                                  indent, 
                                  2, 
                                  TRUE);
            }
            write_config_stmt_force(scb, obj, indent);
            write_mandatory_stmt_force(scb, obj, indent);
            write_sdr(scb, cp, obj, indent);
            break;
        case OBJ_TYP_LEAF_LIST:
        case OBJ_TYP_LIST:
            write_musts(scb, cp, obj_get_mustQ(obj), indent); 
            write_config_stmt_force(scb, obj, indent);
            write_minmax(scb,
                         refine->minelems_tkerr.linenum != 0,
                         refine->minelems,
                         refine->maxelems_tkerr.linenum != 0,
                         refine->maxelems,
                         indent);
            write_sdr(scb, cp, obj, indent);
            break;
        case OBJ_TYP_CHOICE:
            if (refine->def) {
                write_complex_str(scb, 
                                  cp->tkc,
                                  &refine->def,
                                  YANG_K_DEFAULT, 
                                  refine->def, 
                                  indent, 
                                  2, 
                                  TRUE);
            }
            write_config_stmt_force(scb, obj, indent);
            write_mandatory_stmt_force(scb, obj, indent);
            write_sdr(scb, cp, obj, indent);
            break;
        case OBJ_TYP_CASE:
            write_sdr(scb, cp, obj, indent);
            break;
        default:
            ;
        }

        ses_putstr_indent(scb, END_SEC, startindent);
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    return NO_ERR;
    
}  /* write_object */


/********************************************************************
* FUNCTION write_objects
* 
* Generate the HTML for the specified datadefQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   datadefQ == que of obj_template_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_objects (ses_cb_t *scb,
                   const ncx_module_t *mod,
                   const yangdump_cvtparms_t *cp,
                   dlq_hdr_t *datadefQ,
                   int32 startindent)
{
    obj_template_t    *obj;
    status_t           res;
    boolean            first;

    if (dlq_empty(datadefQ)) {
        return;
    }

    if (datadefQ == &mod->datadefQ) {
        write_banner_cmt(scb, 
                         mod, 
                         cp,
                         (const xmlChar *)"objects", 
                         startindent);
    }

    first = TRUE;
    for (obj = (obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {
        
        if (obj_is_hidden(obj)) {
            continue;
        }
        res = write_object(scb, 
                           mod, 
                           cp, 
                           obj, 
                           startindent, 
                           first);
        if (first && res == NO_ERR) {
            first = FALSE;
        }
    }

}  /* write_objects */


/********************************************************************
* FUNCTION write_extension
* 
* Generate the HTML for 1 extension
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   ext == ext_template_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_extension (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     const ext_template_t *ext,
                     int32 startindent)
{
    const xmlChar     *submod;
    int32              indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);

    write_href_id(scb, 
                  submod, 
                  YANG_K_EXTENSION, 
                  ext->name, 
                  startindent, 
                  ext->tkerr.linenum, 
                  FALSE, 
                  TRUE);

    write_appinfoQ(scb, mod, cp, &ext->appinfoQ, indent);

    /* argument sub-clause */
    if (ext->arg) {
        write_simple_str(scb, 
                         YANG_K_ARGUMENT, 
                         ext->arg, 
                         indent, 
                         2, 
                         FALSE);
        write_simple_str(scb, 
                         YANG_K_YIN_ELEMENT, 
                         ext->argel ? NCX_EL_TRUE : NCX_EL_FALSE,
                         indent + ses_indent_count(scb),
                         0,
                         TRUE);
        ses_putstr_indent(scb, END_SEC, indent);
    }

    /* status field */
    write_status(scb, ext->status, indent);

    /* description field */
    if (ext->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &ext->descr,
                          YANG_K_DESCRIPTION, 
                          ext->descr,
                          indent,
                          2, 
                          TRUE);
    }

    /* reference field */
    if (ext->ref) {
        write_reference_str(scb, ext->ref, indent);
    }

    /* end extension clause */
    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_extension */


/********************************************************************
* FUNCTION write_extensions
* 
* Generate the HTML for the specified extensionQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   extensionQ == que of ext_template_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_extensions (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const dlq_hdr_t *extensionQ,
                      int32 startindent)
{
    const ext_template_t *ext;

    if (dlq_empty(extensionQ)) {
        return;
    }

    write_banner_cmt(scb, 
                     mod, 
                     cp,
                     (const xmlChar *)"extensions", 
                     startindent);

    for (ext = (const ext_template_t *)dlq_firstEntry(extensionQ);
         ext != NULL;
         ext = (const ext_template_t *)dlq_nextEntry(ext)) {

        write_extension(scb, mod, cp, ext, startindent);
    }

}  /* write_extensions */


/********************************************************************
* FUNCTION write_identity
* 
* Generate the HTML for 1 identity statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   identity == ncx_identity_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_identity (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    const ncx_identity_t *identity,
                    int32 startindent)
{
    const xmlChar     *submod;
    int32              indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);

    write_href_id(scb, 
                  submod, 
                  YANG_K_IDENTITY, 
                  identity->name, 
                  startindent, 
                  identity->tkerr.linenum, 
                  FALSE, 
                  TRUE);

    write_appinfoQ(scb, 
                   mod, 
                   cp, 
                   &identity->appinfoQ, 
                   indent);

    /* optional base sub-clause */
    if (identity->base) {
        write_identity_base(scb, 
                            mod, 
                            cp,
                            identity, 
                            indent);
    }

    /* status field */
    write_status(scb, identity->status, indent);

    /* description field */
    if (identity->descr) {
        write_complex_str(scb, 
                          cp->tkc,
                          &identity->descr,
                          YANG_K_DESCRIPTION, 
                          identity->descr, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* reference field */
    if (identity->ref) {
        write_reference_str(scb, identity->ref, indent);
    }

    /* end identity clause */
    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_identity */


/********************************************************************
* FUNCTION write_identities
* 
* Generate the HTML for the specified identityQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   identityQ == que of ncx_identity_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_identities (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const dlq_hdr_t *identityQ,
                      int32 startindent)
{
    const ncx_identity_t *identity;

    if (dlq_empty(identityQ)) {
        return;
    }

    write_banner_cmt(scb, mod, cp,
                     (const xmlChar *)"identities", 
                     startindent);

    for (identity = (const ncx_identity_t *)
             dlq_firstEntry(identityQ);
         identity != NULL;
         identity = (const ncx_identity_t *)
             dlq_nextEntry(identity)) {

        write_identity(scb, 
                       mod, 
                       cp, 
                       identity, 
                       startindent);
    }

}  /* write_identities */


/********************************************************************
* FUNCTION write_feature
* 
* Generate the HTML for 1 feature statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   feature == ncx_feature_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_feature (ses_cb_t *scb,
                   const ncx_module_t *mod,
                   const yangdump_cvtparms_t *cp,
                   const ncx_feature_t *feature,
                   int32 startindent)
{
    const xmlChar     *submod;
    int32              indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);

    write_href_id(scb, 
                  submod, 
                  YANG_K_FEATURE, 
                  feature->name, 
                  startindent, 
                  feature->tkerr.linenum, 
                  FALSE, 
                  TRUE);

    write_appinfoQ(scb, 
                   mod, 
                   cp, 
                   &feature->appinfoQ, 
                   indent);

    /* optional Q of if-feature statements */
    write_iffeatureQ(scb, mod, cp, &feature->iffeatureQ, indent);

    /* status field */
    write_status(scb, feature->status, indent);

    /* description field */
    if (feature->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &feature->descr,
                          YANG_K_DESCRIPTION, 
                          feature->descr, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* reference field */
    if (feature->ref) {
        write_reference_str(scb, feature->ref, indent);
    }

    /* end feature clause */
    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_feature */


/********************************************************************
* FUNCTION write_features
* 
* Generate the HTML for the specified featureQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   featureQ == que of ncx_feature_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_features (ses_cb_t *scb,
                    const ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp,
                    const dlq_hdr_t *featureQ,
                    int32 startindent)
{
    const ncx_feature_t *feature;

    if (dlq_empty(featureQ)) {
        return;
    }

    write_banner_cmt(scb, mod, cp,
                     (const xmlChar *)"features", 
                     startindent);

    for (feature = (const ncx_feature_t *)
             dlq_firstEntry(featureQ);
         feature != NULL;
         feature = (const ncx_feature_t *)
             dlq_nextEntry(feature)) {

        write_feature(scb, 
                      mod, 
                      cp, 
                      feature, 
                      startindent);
    }

}  /* write_features */



/********************************************************************
* FUNCTION write_deviate
* 
* Generate the HTML for 1 deviate statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   deviate== obj_deviate_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_deviate (ses_cb_t *scb,
                   const ncx_module_t *mod,
                   const yangdump_cvtparms_t *cp,
                   const obj_deviate_t *deviate,
                   int32 startindent)
{
    const xmlChar        *submod;
    int32                 indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);


    ses_indent(scb, startindent);
    write_id_a(scb, submod, YANG_K_DEVIATE, deviate->tkerr.linenum);
    write_kw(scb, YANG_K_DEVIATE);
    ses_putchar(scb, ' ');
    ses_putstr(scb, obj_get_deviate_arg(deviate->arg));
    if (deviate->empty) {
        ses_putchar(scb, ';');
        return;
    }

    ses_putstr(scb, START_SEC);

    /* extension usage */
    write_appinfoQ(scb, 
                   mod, 
                   cp, 
                   &deviate->appinfoQ, 
                   indent);

    /* type-stmt */
    if (deviate->typdef) {
        write_type_clause(scb, 
                          mod,
                          cp,
                          deviate->typdef,
                          NULL,
                          indent);
    }

    /* units-stmt */
    if (deviate->units) {
        write_simple_str(scb, 
                         YANG_K_UNITS, 
                         deviate->units, 
                         indent, 
                         2, 
                         TRUE);
    }

    /* default-stmt */
    if (deviate->defval) {
        write_complex_str(scb, 
                          cp->tkc,
                          &deviate->defval, 
                          YANG_K_DEFAULT, 
                          deviate->defval, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* config-stmt, only if actually set */
    if (deviate->config_tkerr.linenum != 0) {
        write_simple_str(scb, 
                         YANG_K_CONFIG, 
                         (deviate->config) ?
                         NCX_EL_TRUE : NCX_EL_FALSE,
                         indent, 
                         0, 
                         TRUE);
    }

    /* mandatory-stmt, only if actually set */
    if (deviate->mandatory_tkerr.linenum != 0) {
        write_simple_str(scb, 
                         YANG_K_MANDATORY, 
                         (deviate->mandatory) ?
                         NCX_EL_TRUE : NCX_EL_FALSE,
                         indent, 
                         0, 
                         TRUE);
    }

    /* min-elements, max-elements */
    write_minmax(scb,
                 deviate->minelems_tkerr.linenum != 0,
                 deviate->minelems,
                 deviate->maxelems_tkerr.linenum != 0,
                 deviate->maxelems,
                 indent);

    /* must-stmts */
    write_musts(scb, cp, &deviate->mustQ, indent);

    /* unique-stmts */
    write_unique_stmts(scb, 
                       mod,
                       cp,
                       &deviate->uniqueQ, 
                       indent);

    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_deviate */


/********************************************************************
* FUNCTION write_deviation
* 
* Generate the HTML for 1 deviation statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   deviation == obj_deviation_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_deviation (ses_cb_t *scb,
                     const ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp,
                     const obj_deviation_t *deviation,
                     int32 startindent)
{
    const obj_deviate_t  *deviate;
    const xmlChar        *submod, *fname, *fversion;
    int32                 indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    indent = startindent + ses_indent_count(scb);


    ses_indent(scb, startindent);
    write_id_a(scb, submod, YANG_K_DEVIATION, deviation->tkerr.linenum);
    write_kw(scb, YANG_K_DEVIATION);
    ses_putchar(scb, ' ');
    fname = NULL;
    fversion = NULL;
    if (deviation->targobj && 
        deviation->targobj->tkerr.mod &&
        deviation->targobj->tkerr.mod != mod) {

        fname = deviation->targobj->tkerr.mod->name;
        fversion = deviation->targobj->tkerr.mod->version;
    }
    write_a2(scb,
             cp, 
             fname, 
             fversion, 
             submod,
             obj_get_name(deviation->targobj), 
             deviation->targobj->tkerr.linenum, 
             deviation->target);

    if (deviation->empty) {
        ses_putchar(scb, ';');
        return;
    }

    ses_putstr(scb, START_SEC);

    write_appinfoQ(scb, 
                   mod, 
                   cp, 
                   &deviation->appinfoQ, 
                   indent);

    /* description field */
    if (deviation->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &deviation->descr,
                          YANG_K_DESCRIPTION, 
                          deviation->descr, 
                          indent, 
                          2, 
                          TRUE);
    }

    /* reference field */
    if (deviation->ref) {
        write_reference_str(scb, deviation->ref, indent);
    }

    /* 0 or more deviate-stmts */
    for (deviate = (const obj_deviate_t *)
             dlq_firstEntry(&deviation->deviateQ);
         deviate != NULL;
         deviate = (const obj_deviate_t *)
             dlq_nextEntry(deviate)) {
        write_deviate(scb, mod, cp, deviate, indent);
    }

    /* end deviation statement */
    ses_putstr_indent(scb, END_SEC, startindent);

}  /* write_deviation */


/********************************************************************
* FUNCTION write_deviations
* 
* Generate the HTML for the specified deviationQ
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == ncx_module_t struct in progress
*   cp == conversion parameters in use
*   deviationQ == que of obj_deviation_t to use
*   startindent == start indent count
*
*********************************************************************/
static void
    write_deviations (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      const dlq_hdr_t *deviationQ,
                      int32 startindent)
{
    const obj_deviation_t *deviation;

    if (dlq_empty(deviationQ)) {
        return;
    }

    write_banner_cmt(scb, 
                     mod, 
                     cp,
                     (const xmlChar *)"deviations", 
                     startindent);

    for (deviation = (const obj_deviation_t *)
             dlq_firstEntry(deviationQ);
         deviation != NULL;
         deviation = (const obj_deviation_t *)
             dlq_nextEntry(deviation)) {

        write_deviation(scb, 
                        mod, 
                        cp, 
                        deviation, 
                        startindent);
    }

}  /* write_deviations */


/********************************************************************
* FUNCTION write_import
* 
* Generate the HTML for an import statement
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   modprefix == module prefix value for import
*   modname == module name to use in import clause
*   modrevision == module revision date (may be NULL)
*   submod == submodule name to use in URLs in unified mode only
*   appinfoQ  == import appinfo Q (may be NULL)
*   indent == start indent count
*
*********************************************************************/
static void
    write_import (ses_cb_t *scb,
                  const ncx_module_t *mod,
                  const yangdump_cvtparms_t *cp,
                  const xmlChar *modprefix,
                  const xmlChar *modname,
                  const xmlChar *modrevision,
                  const xmlChar *submod,
                  const dlq_hdr_t *appinfoQ,
                  int32 indent)
{
    ses_indent(scb, indent);
    write_kw(scb, YANG_K_IMPORT);
    ses_putchar(scb, ' ');
    write_a(scb,
            cp, 
            modname, 
            (modrevision) ? modrevision : NULL,
            submod,
            NULL,
            NULL,
            0);
    ses_putstr(scb, START_SEC);
    if (appinfoQ) {
        write_appinfoQ(scb, 
                       mod, 
                       cp, 
                       appinfoQ,
                       indent + ses_indent_count(scb));
    }
    write_simple_str(scb, 
                     YANG_K_PREFIX, 
                     modprefix,
                     indent + ses_indent_count(scb), 
                     0, 
                     TRUE);
    if (modrevision) {
        write_simple_str(scb, 
                         YANG_K_REVISION_DATE,
                         modrevision,
                         indent + ses_indent_count(scb), 
                         2, 
                         TRUE);

    }
    ses_putstr_indent(scb, END_SEC, indent);

}  /* write_import */


/********************************************************************
* FUNCTION write_mod_header
* 
* Generate the HTML for the module header info
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   startindent == start indent count
*
* OUTPUTS:
*  current indent count will be startindent + ses_indent_count(scb)
*  upon exit
*********************************************************************/
static void
    write_mod_header (ses_cb_t *scb,
                      const ncx_module_t *mod,
                      const yangdump_cvtparms_t *cp,
                      int32 startindent)
{
    const ncx_import_t       *imp;
    const yang_import_ptr_t  *impptr;
    const ncx_include_t      *inc;
    const ncx_revhist_t      *rev;
    const xmlChar            *submod;
    char                      buff[NCX_MAX_NUMLEN];
    int                       indent;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    /* [sub]module name { */
    ses_indent(scb, startindent);
    if (mod->ismod) {
        write_kw(scb, YANG_K_MODULE);
    } else {
        write_kw(scb, YANG_K_SUBMODULE);
    }
    ses_putchar(scb, ' ');
    write_id(scb, mod->name);
    ses_putstr(scb, START_SEC);
    ses_putchar(scb, '\n');

    /* bump indent one level */
    indent = startindent + ses_indent_count(scb);

    /* yang-version */
    sprintf(buff, "%u", mod->langver);
    write_simple_str(scb, 
                     YANG_K_YANG_VERSION, 
                     (const xmlChar *)buff, 
                     indent, 
                     0, 
                     TRUE);
    ses_putchar(scb, '\n');

    /* namespace or belongs-to */
    if (mod->ismod) {
        write_complex_str(scb, 
                          cp->tkc,
                          &mod->ns,
                          YANG_K_NAMESPACE,
                          mod->ns,
                          indent,
                          2, 
                          TRUE);
        ses_putchar(scb, '\n');
    } else {
        write_complex_str(scb,
                          cp->tkc,
                          &mod->belongs,
                          YANG_K_BELONGS_TO, 
                          mod->belongs,
                          indent, 
                          0, 
                          FALSE);
    }

    if (mod->ismod) {
        write_simple_str(scb,
                         YANG_K_PREFIX,
                         mod->prefix,
                         indent, 
                         0, 
                         TRUE);
    } else {
        write_simple_str(scb,
                         YANG_K_PREFIX,
                         mod->prefix,
                         indent + indent, 
                         0, 
                         TRUE);
        ses_putstr_indent(scb, (const xmlChar *)"}", indent);
    }

    /* blank line */
    ses_putchar(scb, '\n');

    /* imports section */
    if (cp->unified) {
        for (impptr = (const yang_import_ptr_t *)
                 dlq_firstEntry(&mod->saveimpQ);
             impptr != NULL;
             impptr = (const yang_import_ptr_t *)dlq_nextEntry(impptr)) {

            /* the appinfoQ info is not saved in unified mode ouput!! */
            write_import(scb, 
                         mod, 
                         cp, 
                         impptr->modprefix,
                         impptr->modname, 
                         impptr->revision,
                         submod, 
                         NULL, 
                         indent);
        }
        if (!dlq_empty(&mod->saveimpQ)) {
            ses_putchar(scb, '\n');
        }
    } else {
        for (imp = (const ncx_import_t *)dlq_firstEntry(&mod->importQ);
             imp != NULL;
             imp = (const ncx_import_t *)dlq_nextEntry(imp)) {

            write_import(scb, 
                         mod, 
                         cp, 
                         imp->prefix,
                         imp->module, 
                         imp->revision,
                         submod, 
                         &imp->appinfoQ, 
                         indent);

        }
        if (!dlq_empty(&mod->importQ)) {
            ses_putchar(scb, '\n');
        }
    }

    /* includes section        */
    if (!cp->unified) {
        for (inc = (const ncx_include_t *)dlq_firstEntry(&mod->includeQ);
             inc != NULL;
             inc = (const ncx_include_t *)dlq_nextEntry(inc)) {
            ses_indent(scb, indent);
            write_kw(scb, YANG_K_INCLUDE);
            ses_putchar(scb, ' ');
            write_a(scb, 
                    cp, 
                    inc->submodule, 
                    (inc->submod) ? inc->submod->version : NULL,
                    submod, 
                    NULL, 
                    NULL,
                    0);
            if (inc->revision || !dlq_empty(&inc->appinfoQ)) {
                ses_putstr(scb, START_SEC);
                write_appinfoQ(scb, 
                               mod,
                               cp,
                               &inc->appinfoQ,
                               indent + ses_indent_count(scb));
                if (inc->revision) {
                    write_simple_str(scb, 
                                     YANG_K_REVISION_DATE, 
                                     inc->revision,
                                     indent + ses_indent_count(scb),
                                     2, 
                                     TRUE);
                }
                ses_putstr_indent(scb, END_SEC, indent);
            } else {
                ses_putchar(scb, ';');
            }
        }
        if (!dlq_empty(&mod->includeQ)) {
            ses_putchar(scb, '\n');
        }
    }

    /* organization */
    if (mod->organization) {
        write_complex_str(scb, 
                          cp->tkc,
                          &mod->organization,
                          YANG_K_ORGANIZATION,
                          mod->organization, 
                          indent,
                          2, 
                          TRUE);
        ses_putchar(scb, '\n');        
    }

    /* contact */
    if (mod->contact_info) {
        write_complex_str(scb,
                          cp->tkc,
                          &mod->contact_info,
                          YANG_K_CONTACT,
                          mod->contact_info, 
                          indent,
                          2,
                          TRUE);
        ses_putchar(scb, '\n');
    }

    /* description */
    if (mod->descr) {
        write_complex_str(scb,
                          cp->tkc,
                          &mod->descr,
                          YANG_K_DESCRIPTION,
                          mod->descr,
                          indent, 
                          2, 
                          TRUE);
        ses_putchar(scb, '\n');
    }

    /* reference */
    if (mod->ref) {
        write_reference_str(scb, mod->ref, indent);
        ses_putchar(scb, '\n');

    }


    /* revision history section        */
    for (rev = (const ncx_revhist_t *)
             dlq_firstEntry(&mod->revhistQ);
         rev != NULL;
         rev = (const ncx_revhist_t *)dlq_nextEntry(rev)) {

        write_simple_str(scb, 
                         YANG_K_REVISION, 
                         rev->version,
                         indent, 
                         2, 
                         FALSE);

        if (rev->descr != NULL) {
            write_complex_str(scb, 
                              cp->tkc,
                              &rev->descr,
                              YANG_K_DESCRIPTION,
                              rev->descr,
                              indent + ses_indent_count(scb),
                              2, 
                              TRUE);
        } else {
            write_simple_str(scb, 
                             YANG_K_DESCRIPTION,
                             EMPTY_STRING,
                             indent + ses_indent_count(scb),
                             2, 
                             TRUE);
        }

        if (rev->ref) {
            write_reference_str(scb, 
                                rev->ref, 
                                indent + ses_indent_count(scb));
        }

        ses_putstr_indent(scb, END_SEC, indent);
        ses_putchar(scb, '\n');
    }

} /* write_mod_header */


/********************************************************************
* FUNCTION datadefQ_toc_needed
* 
* Check if a sub-menu is needed for a datadefQ
*
* INPUTS:
*   cp == conversion parameters to use
*   datadefQ == Q of obj_template_t to check
*   curlevel == current object nest level
*
* RETURNS:
*   TRUE if child nodes need to be included in a ToC sub-menu
*   FALSE if no child nodes, and a plain ToC entry is needed instead
*********************************************************************/
static boolean
    datadefQ_toc_needed (const yangdump_cvtparms_t *cp,
                         const dlq_hdr_t  *datadefQ,
                         uint32 curlevel)
{
    const obj_template_t *obj;
    boolean               cooked;

    if (curlevel >= MENU_NEST_LEVEL) {
        return FALSE;
    }

    cooked = strcmp(cp->objview, OBJVIEW_COOKED) ? FALSE : TRUE;

    for (obj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {
        if (obj_is_hidden(obj)) {
            continue;
        }
        if (cooked) {
            if (obj_is_data(obj) && obj_has_name(obj)) {
                return TRUE;
            }
        } else {
            if (obj_is_data(obj) && !obj_is_cloned(obj)) {
                return TRUE;
            }
        }
    }
    return FALSE;

} /* datadefQ_toc_needed */


/********************************************************************
* FUNCTION write_toc_menu_datadefQ
* 
* Generate the table of contents menu for a Q of object nodes
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*   datadefQ == Q of obj_template_t to check
*   indent == starting indent count
*********************************************************************/
static void
    write_toc_menu_datadefQ (ses_cb_t *scb,
                             const ncx_module_t *mod,
                             const yangdump_cvtparms_t *cp,
                             const dlq_hdr_t  *datadefQ,
                             int32 indent)
{
    const xmlChar        *submod;
    const obj_template_t *obj;
    const dlq_hdr_t      *childQ;
    boolean               cooked;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    cooked = strcmp(cp->objview, OBJVIEW_COOKED) ? FALSE : TRUE;

    for (obj = (const obj_template_t *)dlq_firstEntry(datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {

        if (obj_is_hidden(obj)) {
            continue;
        }

        if (!obj_is_data(obj)) {
            continue;
        }

        if (obj->objtype == OBJ_TYP_RPCIO &&
            obj_get_child_count(obj) == 0) {
            /* skip placeholder rpc/input and rpc/output */
            continue;
        }

        if (cooked) {
            /* skip uses and augment objects in this mode */
            if (!obj_has_name(obj)) {
                continue;
            }
        } else {
            /* skip over cloned objects */
            if (obj_is_cloned(obj)) {
                continue;
            }
        }

        childQ = obj_get_cdatadefQ(obj);
        if (!childQ || 
            !datadefQ_toc_needed(cp, childQ, obj_get_level(obj))) {
            start_elem(scb, EL_LI, NULL, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    obj_get_name(obj), 
                    obj->tkerr.linenum);
            end_elem(scb, EL_LI, -1);
        } else {
            start_elem(scb, EL_LI, CL_DADDY, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    obj_get_name(obj), 
                    obj->tkerr.linenum);
            start_elem(scb, EL_UL, NULL, indent + ses_indent_count(scb));
            write_toc_menu_datadefQ(scb,
                                    mod,
                                    cp,
                                    childQ,
                                    indent + 2*ses_indent_count(scb));
            end_elem(scb, EL_UL, indent + ses_indent_count(scb));
            end_elem(scb, EL_LI, indent);
        }
    }
}  /* write_toc_menu_datadefQ */


/********************************************************************
* FUNCTION write_toc_menu_grp
* 
* Generate the table of contents menu for a grouping in the module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
static void
    write_toc_menu_grp (ses_cb_t *scb,
                        const ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        int32 indent)
{
    const xmlChar *submod;
    const grp_template_t *grp;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    for (grp = (const grp_template_t *)dlq_firstEntry(&mod->groupingQ);
         grp != NULL;
         grp = (const grp_template_t *)dlq_nextEntry(grp)) {
        if (!datadefQ_toc_needed(cp, &grp->datadefQ, 1)) {
            start_elem(scb, EL_LI, NULL, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    grp->name, 
                    grp->tkerr.linenum);
            end_elem(scb, EL_LI, -1);
        } else {
            start_elem(scb, EL_LI, CL_DADDY, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    grp->name, 
                    grp->tkerr.linenum);
            start_elem(scb, EL_UL, NULL, indent + ses_indent_count(scb));
            write_toc_menu_datadefQ(scb, 
                                    mod, 
                                    cp, &grp->datadefQ,
                                    indent + 2*ses_indent_count(scb));
            end_elem(scb, EL_UL, indent + ses_indent_count(scb));
            end_elem(scb, EL_LI, indent);
        }
    }

}  /* write_toc_menu_grp */


/********************************************************************
* FUNCTION check_obj_toc_needed
* 
* Check if ToC entries needed for a module or submodule
*
* INPUTS:
*   mod == module or submodule to check
*   cooked == TRUE if cooked mode, FALSE if raw mode
*   anyobj == address of any object return flag
*   anrpc == address of any RPC return flag
*   anynotif == address of any notification return flag
*
* OUTPUTS:
*   *anyobj == object return flag
*   *anrpc == RPC return flag
*   *anynotif == notification return flag
*
*********************************************************************/
static void
    check_obj_toc_needed (ncx_module_t *mod,
                          boolean cooked,
                          boolean *anyobj,
                          boolean *anyrpc,
                          boolean *anynotif)
{
    const obj_template_t *obj;

    *anyobj = FALSE;
    *anyrpc = FALSE;
    *anynotif = FALSE;

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (obj_is_hidden(obj)) {
            continue;
        }

        if (cooked) {
            if (obj_is_data(obj)) {
                *anyobj = TRUE;
            }
        } else {
            if (!obj_is_cloned(obj) && obj_is_data(obj)) {
                *anyobj = TRUE;
            }
        }
        if (obj->objtype == OBJ_TYP_RPC) {
            *anyrpc = TRUE;
        } else if (obj->objtype == OBJ_TYP_NOTIF) {
            *anynotif = TRUE;
        }
    }

}  /* check_obj_toc_needed */


/********************************************************************
* FUNCTION write_toc_menu_rpc
* 
* Write the ToC entries for the RPCs in a module or submodule
*
* INPUTS:
*   scb == session to use for writing
*   mod == module or submodule to check
*   cp == conversion parameters to use
*   indent == start indent count
*********************************************************************/
static void
    write_toc_menu_rpc (ses_cb_t *scb,
                        ncx_module_t *mod,
                        const yangdump_cvtparms_t *cp,
                        int32 indent)
{
    obj_template_t       *obj;
    const xmlChar        *submod;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    for (obj = (obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (obj_template_t *)dlq_nextEntry(obj)) {

        if (obj->objtype != OBJ_TYP_RPC) {
            continue;
        }

        if (obj_is_hidden(obj)) {
            continue;
        }

        if (!(obj_rpc_has_input(obj) || obj_rpc_has_output(obj))) {
            start_elem(scb, EL_LI, NULL, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    obj_get_name(obj), 
                    obj->tkerr.linenum);
            end_elem(scb, EL_LI, -1);
        } else {
            start_elem(scb, EL_LI, CL_DADDY, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    obj_get_name(obj), 
                    obj->tkerr.linenum);
            start_elem(scb, EL_UL, NULL, indent + ses_indent_count(scb));
            write_toc_menu_datadefQ(scb, 
                                    mod, 
                                    cp,
                                    &obj->def.rpc->datadefQ,
                                    indent + 2*ses_indent_count(scb));
            end_elem(scb, EL_UL, indent + ses_indent_count(scb));
            end_elem(scb, EL_LI, indent);
        }
    }

} /* write_toc_menu_rpc */


/********************************************************************
* FUNCTION write_toc_menu_notif
* 
* Write the ToC entries for the notifications in a module or submodule
*
* INPUTS:
*   scb == session to use for writing
*   mod == module or submodule to check
*   cp == conversion parameters to use
*   indent == start indent count
*********************************************************************/
static void
    write_toc_menu_notif (ses_cb_t *scb,
                          const ncx_module_t *mod,
                          const yangdump_cvtparms_t *cp,
                          int32 indent)
{
    const obj_template_t *obj;
    const xmlChar        *submod;

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;

    for (obj = (const obj_template_t *)dlq_firstEntry(&mod->datadefQ);
         obj != NULL;
         obj = (const obj_template_t *)dlq_nextEntry(obj)) {

        if (obj->objtype != OBJ_TYP_NOTIF) {
            continue;
        }

        if (obj_is_hidden(obj)) {
            continue;
        }

        if (!datadefQ_toc_needed(cp, &obj->def.notif->datadefQ, 1)) {
            start_elem(scb, EL_LI, NULL, indent);
            write_a(scb, 
                    cp, 
                    NULL,
                    NULL,
                    submod,
                    NULL, 
                    obj_get_name(obj), 
                    obj->tkerr.linenum);
            end_elem(scb, EL_LI, -1);
        } else {
            start_elem(scb, EL_LI, CL_DADDY, indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL,
                    submod,
                    NULL, 
                    obj_get_name(obj),
                    obj->tkerr.linenum);
            start_elem(scb, EL_UL, NULL, indent + ses_indent_count(scb));
            write_toc_menu_datadefQ(scb, 
                                    mod,
                                    cp,
                                    &obj->def.notif->datadefQ,
                                    indent + 2*ses_indent_count(scb));
            end_elem(scb, EL_UL, indent + ses_indent_count(scb));
            end_elem(scb, EL_LI, indent);
        }
    }

} /* write_toc_menu_notif */


/********************************************************************
* FUNCTION write_toc_menu
* 
* Generate the table of contents menu for the module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use
*
*********************************************************************/
static void
    write_toc_menu (ses_cb_t *scb,
                    ncx_module_t *mod,
                    const yangdump_cvtparms_t *cp)
{
    const typ_template_t *typ;
    const ext_template_t *ext;
    const xmlChar        *submod;
    const yang_node_t    *node;
    boolean               anytyp, anygrp, anyobj, anyrpc, anynotif, anyext;
    boolean               isplain, cooked;
    int32                 indent;

    if (!xml_strcmp(cp->html_toc, (const xmlChar *)"plain")) {
        isplain = TRUE;
    } else if (!xml_strcmp(cp->html_toc, (const xmlChar *)"menu")) {
        isplain = FALSE;
    } else if (!xml_strcmp(cp->html_toc, (const xmlChar *)"none")) {
        return;
    } else {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    submod = (cp->unified && !mod->ismod) ? mod->name : NULL;
    cooked = strcmp(cp->objview, OBJVIEW_COOKED) ? FALSE : TRUE;
    anyobj = FALSE;
    anyrpc = FALSE;
    anynotif = FALSE;
    indent = ses_indent_count(scb);

    /* start menu list */
    start_id_elem(scb, EL_UL, (isplain) ? NULL : ID_NAV, indent);

    /* check if any typedef ToC entries to generate */
    anytyp = dlq_empty(&mod->typeQ) ? FALSE : TRUE;
    if (cp->unified && mod->ismod) {
        node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
        while (!anytyp && node) {
            anytyp = dlq_empty(&node->submod->typeQ) ? FALSE : TRUE;
            node = (const yang_node_t *)dlq_nextEntry(node);
        }
    }

    /* generate typedef ToC entries */
    if (anytyp) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb, 
                 cp, 
                 NULL, 
                 NULL, 
                 NULL, 
                 EMPTY_STRING, 
                 0,
                 (const xmlChar *)"Typedefs");
        start_elem(scb, EL_UL, NULL,  3*indent);
        for (typ = (const typ_template_t *)dlq_firstEntry(&mod->typeQ);
             typ != NULL;
             typ = (const typ_template_t *)dlq_nextEntry(typ)) {
            start_elem(scb, EL_LI, NULL, 4*indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    typ->name, 
                    typ_get_typ_linenum(typ));
            end_elem(scb, EL_LI, -1);
        }
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                for (typ = (const typ_template_t *)
                         dlq_firstEntry(&node->submod->typeQ);
                     typ != NULL;
                     typ = (const typ_template_t *)dlq_nextEntry(typ)) {
                    start_elem(scb, EL_LI, NULL, 4*indent);
                    write_a(scb, 
                            cp, 
                            NULL, 
                            NULL,
                            node->submod->name, 
                            NULL,
                            typ->name,
                            typ_get_typ_linenum(typ));
                    end_elem(scb, EL_LI, -1);
                }
            }
        }
        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* check if any grouping ToC entries to generate */
    if (cooked) {
        anygrp = FALSE;
    } else {
        anygrp = dlq_empty(&mod->groupingQ) ? FALSE : TRUE;
        if (cp->unified && mod->ismod) {
            node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
            while (!anygrp && node) {
                anygrp = dlq_empty(&node->submod->groupingQ) ? FALSE : TRUE;
                node = (const yang_node_t *)dlq_nextEntry(node);
            }
        }
    }

    /* generate grouping ToC entries in raw objview mode only */
    if (anygrp) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb, 
                 cp, 
                 NULL, 
                 NULL, 
                 NULL, 
                 EMPTY_STRING,
                 0,
                 (const xmlChar *)"Groupings");
        start_elem(scb, EL_UL, NULL,  3*indent);
        write_toc_menu_grp(scb, mod, cp, 4*indent);
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                write_toc_menu_grp(scb, node->submod, cp, 4*indent);
            }
        }
        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* check if any object, RPC, or notification ToC entries to generate */
    check_obj_toc_needed(mod, cooked, &anyobj, &anyrpc, &anynotif);
    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            check_obj_toc_needed(node->submod,
                                 cooked,
                                 &anyobj, 
                                 &anyrpc,
                                 &anynotif);
        }
    }
    
    /* generate data object ToC entries */
    if (anyobj) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb, 
                 cp, 
                 NULL, 
                 NULL, 
                 NULL, 
                 EMPTY_STRING,
                 0,
                 (const xmlChar *)"Objects");
        start_elem(scb, EL_UL, NULL,  3*indent);
        write_toc_menu_datadefQ(scb, mod, cp, &mod->datadefQ, 4*indent);
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                write_toc_menu_datadefQ(scb,
                                        node->submod,
                                        cp, 
                                        &node->submod->datadefQ,
                                        4*indent);
            }
        }
        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* generate RPC method ToC entries */
    if (anyrpc) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb,
                 cp, 
                 NULL,
                 NULL,
                 NULL,
                 EMPTY_STRING, 
                 0,
                 (const xmlChar *)"RPC&nbsp;Methods");
        start_elem(scb, EL_UL, NULL,  3*indent);
        write_toc_menu_rpc(scb, mod, cp, 4*indent);
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                write_toc_menu_rpc(scb, node->submod, cp, 4*indent);
            }
        }
        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* generate notofocation ToC entries */
    if (anynotif) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb,
                 cp, 
                 NULL, 
                 NULL, 
                 NULL, 
                 EMPTY_STRING,
                 0,
                 (const xmlChar *)"Notifications");
        start_elem(scb, EL_UL, NULL,  3*indent);
        write_toc_menu_notif(scb, mod, cp, 4*indent);
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                write_toc_menu_notif(scb, node->submod, cp, 4*indent);
            }
        }
        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* check if any extension ToC entries to generate */
    anyext = dlq_empty(&mod->extensionQ) ? FALSE : TRUE;
    if (cp->unified && mod->ismod) {
        node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
        while (!anyext && node) {
            anyext = dlq_empty(&node->submod->extensionQ) ? FALSE : TRUE;
            node = (const yang_node_t *)dlq_nextEntry(node);
        }
    }

    /* generate extension ToC entries */
    if (anyext) {
        start_elem(scb, EL_LI, NULL, 2*indent);
        write_a2(scb, 
                 cp, 
                 NULL, 
                 NULL, 
                 NULL, 
                 EMPTY_STRING, 
                 0,
                 (const xmlChar *)"Extensions");
        start_elem(scb, EL_UL, NULL,  3*indent);

        /* generate actual drop-down list items */
        for (ext = (const ext_template_t *)dlq_firstEntry(&mod->extensionQ);
             ext != NULL;
             ext = (const ext_template_t *)dlq_nextEntry(ext)) {
            start_elem(scb, EL_LI, NULL, 4*indent);
            write_a(scb, 
                    cp, 
                    NULL, 
                    NULL, 
                    submod, 
                    NULL, 
                    ext->name, 
                    ext->tkerr.linenum);
            end_elem(scb, EL_LI, -1);
        }
        if (cp->unified && mod->ismod) {
            for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
                 node != NULL;
                 node = (const yang_node_t *)dlq_nextEntry(node)) {
                for (ext = (const ext_template_t *)
                         dlq_firstEntry(&node->submod->extensionQ);
                     ext != NULL;
                     ext = (const ext_template_t *)dlq_nextEntry(ext)) {
                    start_elem(scb, EL_LI, NULL, 4*indent);
                    write_a(scb, 
                            cp, 
                            NULL, 
                            NULL,
                            submod,
                            NULL, 
                            ext->name,
                            ext->tkerr.linenum);
                    end_elem(scb, EL_LI, -1);
                }
            }
        }

        end_elem(scb, EL_UL, 3*indent);
        end_elem(scb, EL_LI, 2*indent);
    }

    /* end the entire ToC list */
    end_elem(scb, EL_UL, indent);
    ses_putchar(scb, '\n');

}   /* write_toc_menu */


/********************************************************************
* FUNCTION write_html_header
* 
* Generate the HTML header section for the specified module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   indent == the indent amount per line, starting at 0
*   css_file == optional CSS inline file name
*********************************************************************/
static void
    write_html_header (ses_cb_t *scb,
                       const ncx_module_t *mod,
                       int32 indent,
                       const xmlChar *css_file)
{
    xmlChar   *filespec;
    status_t   res;
    xmlChar    buffer[NCX_VERSION_BUFFSIZE];
 
    ses_putstr_indent(scb, (const xmlChar *)
                      "<!DOCTYPE html PUBLIC \"-//W3C//DTD "
                      "XHTML 1.0 Strict//EN\" "
                      "\n    \"http://www.w3.org/TR/xhtml1/DTD"
                      "/xhtml1-strict.dtd\">", 
                      0);

    ses_putstr_indent(scb, (const xmlChar *)"<html lang=\"en\" "
                      "xmlns=\"http://www.w3.org/1999/xhtml\">", 
                      0);
    ses_putstr_indent(scb, (const xmlChar *)"<head>", 0);
    ses_putstr_indent(scb, (const xmlChar *)"<title>", indent);
    ses_putstr(scb, (const xmlChar *)"YANG ");
    ses_putstr(scb, 
               (mod->ismod) ?  (const xmlChar *)"Module " :
               (const xmlChar *)"Submodule ");
    ses_putstr(scb, mod->name);
    ses_putstr(scb, (const xmlChar *)" Version ");
    if (mod->version) {
        ses_putstr(scb, mod->version);
    } else {
        ses_putstr(scb, NCX_EL_NONE);
    }
    ses_putstr(scb, (const xmlChar *)"</title>");
    ses_putstr_indent(scb, (const xmlChar *)
                      "<meta http-equiv=\"Content-Type\" "
                      "content=\"text/html; charset=UTF-8\"/>", 
                      indent);
    ses_putstr_indent(scb, (const xmlChar *)
                      "<meta name=\"description\" "
                      "content=\"YANG data model documentation\"/>",
                      2*indent);
    ses_putstr_indent(scb, (const xmlChar *)
                      "<meta name=\"generator\" "
                      "content=\"yangdump ", 
                      indent);

    res = ncx_get_version(buffer, NCX_VERSION_BUFFSIZE);
    if (res == NO_ERR) {
        ses_putstr(scb, buffer);
    } else {
        SET_ERROR(res);
    }
    ses_putstr(scb, (const xmlChar *)
               " (http://www.netconfcentral.org/)\"/>");

    filespec = NULL;
    if (css_file) {
        filespec = ncxmod_find_data_file(css_file, FALSE, &res);
        if (filespec) {
            ses_putstr_indent(scb, (const xmlChar *)
                              "<style type='text/css'><!--", indent);
            ses_put_extern(scb, filespec);
            ses_putstr_indent(scb, (const xmlChar *)"  -->\n</style>", 
                              indent);
        }
    }

    if (filespec == NULL) {
        ses_putstr_indent(scb, (const xmlChar *)
                          "<link rel=\"stylesheet\" "
                          "href=\"http://netconfcentral.org"
                          "/static/css/style.css\""
                          " type=\"text/css\"/>", 
                          indent);
    }

    ses_putstr_indent(scb, (const xmlChar *)"</head>", 0);

    if (filespec) {
        m__free(filespec);
    }

} /* write_html_header */


/********************************************************************
* FUNCTION write_html_body
* 
* Generate the HTML body section for the specified module
*
* INPUTS:
*   scb == session control block to use for writing
*   mod == module in progress
*   cp == conversion parameters to use

*
*********************************************************************/
static void
    write_html_body (ses_cb_t *scb,
                     ncx_module_t *mod,
                     const yangdump_cvtparms_t *cp)
{
    const yang_node_t     *node;
    const yang_stmt_t     *stmt;
    boolean                stmtmode;

    if (cp->html_div) {
        /* start wrapper div */
        start_elem(scb, EL_DIV, NULL, 0);
    } else {
        /* <body> */
        start_elem(scb, EL_BODY, NULL, 0);
    }
    
    /* title */
    start_elem(scb, EL_H1, CL_YANG, cp->indent);
#ifdef MATCH_HTML_TITLE
    if (mod->ismod) {
        ses_putstr(scb, YANG_K_MODULE);
    } else {
        ses_putstr(scb, YANG_K_SUBMODULE);
    }
    ses_putstr(scb, (const xmlChar *)": ");
    ses_putstr(scb, mod->name);
    ses_putstr(scb, (const xmlChar *)", version: ");
    if (mod->version) {
        ses_putstr(scb, mod->version);    
    } else {
        ses_putstr(scb, NCX_EL_NONE);    
    }
#else
    ses_putstr(scb, mod->sourcefn);
#endif
    end_elem(scb, EL_H1, -1);
    ses_putchar(scb, '\n');

    /* table of contents */
    if (cp->html_toc) {
        write_toc_menu(scb, mod, cp);
        ses_putstr(scb, (const xmlChar *)"\n<br />");
    }

    /* start yellow box div */
    start_elem(scb, EL_DIV, CL_YANG, 0);

    /* rest of module body is pre-formatted */
    start_elem(scb, EL_PRE, NULL, 0);
    ses_putchar(scb, '\n');

    /* module header */
    write_mod_header(scb, mod, cp, cp->indent);

    /* if the top-level statement order was saved, it was only for
     * the YANG_PT_TOP module, and none of the sub-modules
     * generate the canonical order for all the submodule content
     * if 'unified mode'
     *
     * Generate the main module canonical order only
     * if top-level statement order is ever disabled
     */
    stmtmode = dlq_empty(&mod->stmtQ) ? FALSE : TRUE;

    /* TBD: need a better way to generate all the top-level
     * extensions.  This approach is broken because it gathers
     * them and puts them at the start after the header
     *
     * will need to check line numbers as other constructs
     * are generated to find any extensions that 'fit'
     * in particuolar line number ranges
     */
    write_appinfoQ(scb, mod, cp, &mod->appinfoQ, 2*cp->indent);
    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_appinfoQ(scb,
                               node->submod,
                               cp, 
                               &node->submod->appinfoQ, 
                               2*cp->indent);
            }
        }
    }

    /* 1) features */
    if (!stmtmode) {
        write_features(scb, 
                       mod,
                       cp, 
                       &mod->featureQ, 
                       2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_features(scb, 
                               node->submod, 
                               cp, 
                               &node->submod->featureQ, 
                               2*cp->indent);
            }
        }
    }

    /* 2) identities */
    if (!stmtmode) {
        write_identities(scb,
                         mod, 
                         cp, 
                         &mod->identityQ, 
                         2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_identities(scb,
                                 node->submod,
                                 cp, 
                                 &node->submod->identityQ, 
                                 2*cp->indent);
            }
        }
    }

    /* 3) typedefs */
    if (!stmtmode) {
        write_typedefs(scb, mod, cp, &mod->typeQ, 2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_typedefs(scb,
                               node->submod,
                               cp, 
                               &node->submod->typeQ, 
                               2*cp->indent);
            }
        }
    }

    /* 4) groupings */
    if (!stmtmode) {
        write_groupings(scb, mod, cp, &mod->groupingQ, 2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_groupings(scb,
                                node->submod,
                                cp, 
                                &node->submod->groupingQ,
                                2*cp->indent);
            }
        }
    }

    /* 5) extensions */
    if (!stmtmode) {
        write_extensions(scb, mod, cp, &mod->extensionQ, 2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_extensions(scb, 
                                 node->submod,
                                 cp, 
                                 &node->submod->extensionQ,
                                 2*cp->indent);
            }
        }
    }

    /* 6) objects */
    if (!stmtmode) {
        write_objects(scb, mod, cp, &mod->datadefQ, 2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_objects(scb,
                              node->submod, 
                              cp, 
                              &node->submod->datadefQ, 
                              2*cp->indent);
            }
        }
    }

    /* 6) deviations */
    if (!stmtmode) {
        write_deviations(scb, mod, cp, &mod->deviationQ, 2*cp->indent);
    }

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                write_deviations(scb,
                                 node->submod, 
                                 cp, 
                                 &node->submod->deviationQ, 
                                 2*cp->indent);
            }
        }
    }

    /* check statement mode on top-level module only */
    if (stmtmode) {
        for (stmt = (const yang_stmt_t *)dlq_firstEntry(&mod->stmtQ);
             stmt != NULL;
             stmt = (const yang_stmt_t *)dlq_nextEntry(stmt)) {
            switch (stmt->stmttype) {
            case YANG_ST_NONE:
                SET_ERROR(ERR_INTERNAL_VAL);
                break;
            case YANG_ST_TYPEDEF:
                write_typedef(scb,
                              mod,
                              cp, 
                              stmt->s.typ, 
                              2*cp->indent,
                              FALSE);
                break;
            case YANG_ST_GROUPING:
                write_grouping(scb,
                               mod,
                               cp, 
                               stmt->s.grp, 
                               2*cp->indent,
                               FALSE);
                break;
            case YANG_ST_EXTENSION:
                write_extension(scb,
                                mod,
                                cp, 
                                stmt->s.ext, 
                                2*cp->indent);
                break;
            case YANG_ST_OBJECT:
                write_object(scb,
                             mod,
                             cp, 
                             stmt->s.obj, 
                             2*cp->indent, 
                             FALSE);
                break;
            case YANG_ST_IDENTITY:
                write_identity(scb,
                               mod,
                               cp, 
                               stmt->s.identity,
                               2*cp->indent);
                break;
            case YANG_ST_FEATURE:
                write_feature(scb,
                              mod,
                              cp, 
                              stmt->s.feature,
                              2*cp->indent);
                break;
            case YANG_ST_DEVIATION:
                write_deviation(scb,
                                mod,
                                cp, 
                                stmt->s.deviation,
                                2*cp->indent);
                break;
            default:
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        }
    }

    /* end module and page */
    ses_indent(scb, cp->indent);
    ses_putchar(scb, '}');
    write_endsec_cmt(scb, 
                     (mod->ismod) ? 
                     YANG_K_MODULE : YANG_K_SUBMODULE, 
                     mod->name);
    end_elem(scb, EL_PRE, 0);

    /* end yellow box div */
    end_elem(scb, EL_DIV, 0);

    if (cp->html_div) {
        end_elem(scb, EL_DIV, 0);
    } else {
        end_elem(scb, EL_BODY, 0);
    }

} /* write_html_body */


/*********     E X P O R T E D   F U N C T I O N S    **************/


/********************************************************************
* FUNCTION html_convert_module
* 
*  Generate HTML version of the specified module(s)
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*
* RETURNS:
*   status
*********************************************************************/
status_t
    html_convert_module (yang_pcb_t *pcb,
                         const yangdump_cvtparms_t *cp,
                         ses_cb_t *scb)
{
    ncx_module_t  *mod;
    status_t       res;

    res = NO_ERR;

    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        return SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
    }

    if (!cp->html_div) {
        write_html_header(scb, mod, cp->indent, cp->css_file);
    }

    write_html_body(scb, mod, cp);

    if (!cp->html_div) {
        ses_putstr(scb, (const xmlChar *)"\n</html>\n");
    }

    return res;

}   /* html_convert_module */


/* END file html.c */
