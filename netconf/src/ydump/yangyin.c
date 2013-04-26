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
/*  FILE: yangyin.c

                
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

#ifndef _H_yangyin
#include "yangyin.h"
#endif

#ifndef _H_yin
#include "yin.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/
/* #define YANGYIN_DEBUG   1 */

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


/********************************************************************
* FUNCTION start_yin_elem
* 
* Generate a start tag
*
* INPUTS:
*   scb == session control block to use for writing
*   name == element name
*   indent == indent count
*
*********************************************************************/
static void
    start_yin_elem (ses_cb_t *scb,
                const xmlChar *name,
                int32 indent)

{
    ses_putstr_indent(scb, (const xmlChar *)"<", indent);
    ses_putstr(scb, name);

}  /* start_yin_elem */


/********************************************************************
* FUNCTION end_yin_elem
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
    end_yin_elem (ses_cb_t *scb,
                  const xmlChar *name,
                  int32 indent)

{
    ses_putstr_indent(scb, (const xmlChar *)"</", indent);
    ses_putstr(scb, name);
    ses_putchar(scb, '>');

}  /* end_yin_elem */


/********************************************************************
* FUNCTION start_ext_elem
* 
* Generate a start tag for an extension
*
* INPUTS:
*   scb == session control block to use for writing
*   prefix == prefix string to use
*   name == element name
*   indent == indent count
*
*********************************************************************/
static void
    start_ext_elem (ses_cb_t *scb,
                    const xmlChar *prefix,
                    const xmlChar *name,
                    int32 indent)
{
    ses_putstr_indent(scb, (const xmlChar *)"<", indent);
    ses_putstr(scb, prefix);
    ses_putchar(scb, ':');
    ses_putstr(scb, name);

}  /* start_ext_elem */


/********************************************************************
* FUNCTION end_ext_elem
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
    end_ext_elem (ses_cb_t *scb,
                  const xmlChar *prefix,
                  const xmlChar *name,
                  int32 indent)
{
    ses_putstr_indent(scb, (const xmlChar *)"</", indent);
    ses_putstr(scb, prefix);
    ses_putchar(scb, ':');
    ses_putstr(scb, name);
    ses_putchar(scb, '>');

}  /* end_ext_elem */


/********************************************************************
* FUNCTION write_import_xmlns
* 
* Generate an xmlns attribute for the import 
*
* INPUTS:
*   scb == session control block to use for writing
*   imp == ncx_import_t struct to use
*   indent == start indent count
*
*********************************************************************/
static void
    write_import_xmlns (ses_cb_t *scb,
                        ncx_import_t *imp,
                        int32 indent)

{
    if (imp->mod == NULL) {
        return;
    }

    ses_putstr_indent(scb, XMLNS, indent);
    ses_putchar(scb, ':');
    ses_putstr(scb, imp->prefix);
    ses_putchar(scb, '=');
    ses_putchar(scb, '"');
    ses_putstr(scb, ncx_get_modnamespace(imp->mod));
    ses_putchar(scb, '"');

}  /* write_import_xmlns */


/********************************************************************
* FUNCTION write_yin_attr
* 
* Generate an attibute declaration
*
* INPUTS:
*   scb == session control block to use for writing
*   attr == attribute name
*   value == attribute value string
*   indent == start indent count
*
*********************************************************************/
static void
    write_yin_attr (ses_cb_t *scb,
                    const xmlChar *attr,
                    const xmlChar *value,
                    int32 indent)

{
    ses_putstr_indent(scb, attr, indent);
    ses_putchar(scb, '=');
    ses_putchar(scb, '"');
    ses_putstr(scb, value);
    ses_putchar(scb, '"');

}  /* write_yin_attr */


/********************************************************************
* FUNCTION advance_token
* 
* Move to the next token
*
* INPUTS:

*
*********************************************************************/
static status_t
    advance_token (yang_pcb_t *pcb)                   
{
    status_t   res;

    res = TK_ADV(pcb->tkc);

#ifdef YANGYIN_DEBUG
    if (res == NO_ERR && LOGDEBUG3) {
        tk_dump_token(pcb->tkc->cur);
    }
#endif

    return res;

}  /* advance_token */


/********************************************************************
* FUNCTION write_cur_token
* 
* Write the full value of the current token to the session
*
* INPUTS:
*    scb == session control block
*    pcb == YANG parser control block
*    elem == TRUE if this is element content; FALSE if attribute
*********************************************************************/
static void
    write_cur_token (ses_cb_t *scb,
                     yang_pcb_t *pcb,
                     boolean elem)
{
    const xmlChar   *prefix, *value;

    prefix = TK_CUR_MOD(pcb->tkc);
    if (prefix != NULL) {
        if (elem) {
            ses_putcstr(scb, prefix, -1);
        } else {
            ses_putastr(scb, prefix, -1);
        }
        ses_putchar(scb, ':');
    }

    value = TK_CUR_VAL(pcb->tkc);
    if (value != NULL) {
        if (elem) {
            ses_putcstr(scb, value, -1);
        } else {
            ses_putastr(scb, value, -1);
        }
    }

}  /* write_cur_token */


/********************************************************************
* FUNCTION write_yin_stmt
*  
* Go through the token chain and write YIN stmts
* recursively if needed, until 1 YANG stmt is handled
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == conversion parms to use
*   scb == session control block for writing output
*   startindent == start indent count
*   done == address of done return var to use
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    write_yin_stmt (yang_pcb_t *pcb,
                    const yangdump_cvtparms_t *cp,
                    ses_cb_t *scb,
                    int32 startindent,
                    boolean *done)
{
    const yin_mapping_t  *mapping;
    ncx_import_t         *import;
    ext_template_t       *extension;
    const xmlChar        *prefix, *modprefix;
    status_t              res;
    boolean               loopdone;

    res = NO_ERR;
    *done = FALSE;

    /* expecting a keyword [string] stmt-end sequence
     * or the very last closing right brace
     */
    if (TK_CUR_TYP(pcb->tkc) == TK_TT_RBRACE) {
        if (tk_next_typ(pcb->tkc) == TK_TT_NONE) {
            *done = TRUE;
            return NO_ERR;
        } else {
            return ERR_NCX_WRONG_TKTYPE;
        }
    } else if (!TK_CUR_ID(pcb->tkc)) {
        return ERR_NCX_WRONG_TKTYPE;
    }

    /* check the keyword type */
    switch (TK_CUR_TYP(pcb->tkc)) {
    case TK_TT_TSTRING:
        /* YANG keyword */
        mapping = yin_find_mapping(TK_CUR_VAL(pcb->tkc));
        if (mapping == NULL) {
            return ERR_NCX_DEF_NOT_FOUND;
        }

        /* output keyword part */
        start_yin_elem(scb, mapping->keyword, startindent);

        /* output [string] part if expected */
        if (mapping->argname == NULL) {
            if (tk_next_typ(pcb->tkc) == TK_TT_LBRACE) {
                ses_putchar(scb, '>');
            }
        } else {
            /* move token pointer to the argument string */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* write the string part 
             * do not add any extra whiespace to the XML string
             */
            if (mapping->elem) {
                ses_putchar(scb, '>');

                /* encode argname,value as an element */
                start_yin_elem(scb, 
                               mapping->argname,
                               startindent + cp->indent);
                ses_putchar(scb, '>');
                write_cur_token(scb, pcb, TRUE);
                end_yin_elem(scb, mapping->argname, -1);
            } else {
                /* encode argname,value as an attribute */
                ses_putchar(scb, ' ');
                ses_putstr(scb, mapping->argname);
                ses_putchar(scb, '=');
                ses_putchar(scb, '"');
                write_cur_token(scb, pcb, FALSE);
                ses_putchar(scb, '"');
                if (tk_next_typ(pcb->tkc) != TK_TT_SEMICOL) {
                    ses_putchar(scb, '>');
                } /* else end with empty element */
            }
        }

        /* move token pointer to the stmt-end char */
        res = advance_token(pcb);
        if (res != NO_ERR) {
            return res;
        }

        switch (TK_CUR_TYP(pcb->tkc)) {
        case TK_TT_SEMICOL:
            /* advance to next stmt, this one is done */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }
            if (mapping->elem) {
                /* end the complex element */
                end_yin_elem(scb, mapping->keyword, startindent);
            } else {
                /* end the empty element */
                ses_putstr(scb, (const xmlChar *)" />");
            }
            break;
        case TK_TT_LBRACE:
            /* advance to next sub-stmt, this one has child nodes */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* write the nested sub-stmts as child nodes */
            if (TK_CUR_TYP(pcb->tkc) != TK_TT_RBRACE) {
                loopdone = FALSE;
                while (!loopdone) {
                    res = write_yin_stmt(pcb,
                                         cp,
                                         scb,
                                         startindent + cp->indent,
                                         done);
                    if (res != NO_ERR) {
                        return res;
                    }
                    if (TK_CUR_TYP(pcb->tkc) == TK_TT_RBRACE) {
                        loopdone = TRUE;
                    }
                }
            }

            /* move to next stmt, this one is done */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* end the complex element */
            end_yin_elem(scb, mapping->keyword, startindent);
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    case TK_TT_MSTRING:
        /* extension keyword */
        prefix = TK_CUR_MOD(pcb->tkc);
        modprefix = ncx_get_mod_prefix(pcb->top);
        if (modprefix != NULL && !xml_strcmp(prefix, modprefix)) {
            /* local module */
            extension = ext_find_extension(pcb->top, 
                                           TK_CUR_VAL(pcb->tkc));
        } else {
            import = ncx_find_pre_import(pcb->top, prefix);
            if (import == NULL || import->mod == NULL) {
                return ERR_NCX_IMP_NOT_FOUND;
            }
            extension = ext_find_extension(import->mod, 
                                           TK_CUR_VAL(pcb->tkc));
        }
        if (extension == NULL) {
            return ERR_NCX_DEF_NOT_FOUND;
        }

        /* got the extension for this external keyword
         * output keyword part 
         */
        start_ext_elem(scb, 
                       prefix,
                       TK_CUR_VAL(pcb->tkc), 
                       startindent);

        /* output [string] part if expected */
        if (extension->arg == NULL) {
            if (tk_next_typ(pcb->tkc) == TK_TT_LBRACE) {
                ses_putchar(scb, '>');
            }
        } else {
            /* move token pointer to the argument string */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* write the string part
             * do not add any extra whiespace chars to the string
             */
            if (extension->argel) {
                ses_putchar(scb, '>');

                /* encode argname,value as an element */
                start_ext_elem(scb, 
                               prefix,
                               extension->arg,
                               startindent + cp->indent);
                ses_putchar(scb, '>');
                write_cur_token(scb, pcb, TRUE);
                end_ext_elem(scb, prefix, extension->arg, -1);
            } else {
                /* encode argname,value as an attribute */
                ses_putchar(scb, ' ');
                ses_putstr(scb, extension->arg);
                ses_putchar(scb, '=');
                ses_putchar(scb, '"');
                write_cur_token(scb, pcb, FALSE);
                ses_putchar(scb, '"');
                if (tk_next_typ(pcb->tkc) != TK_TT_SEMICOL) {
                    ses_putchar(scb, '>');
                } /* else end with empty element */
            }
        }

        /* move token pointer to the stmt-end char */
        res = advance_token(pcb);
        if (res != NO_ERR) {
            return res;
        }

        switch (TK_CUR_TYP(pcb->tkc)) {
        case TK_TT_SEMICOL:
            /* advance to next stmt, this one is done */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }
            if (extension->arg != NULL && extension->argel) {
                /* end the complex element */
                end_ext_elem(scb, 
                             prefix,
                             extension->name, 
                             startindent);
            } else {
                /* end the empty element */
                ses_putstr(scb, (const xmlChar *)" />");
            }
            break;
        case TK_TT_LBRACE:
            /* advance to next sub-stmt, this one has child nodes */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* write the nested sub-stmts as child nodes */
            if (TK_CUR_TYP(pcb->tkc) != TK_TT_RBRACE) {
                loopdone = FALSE;
                while (!loopdone) {
                    res = write_yin_stmt(pcb,
                                         cp,
                                         scb,
                                         startindent + cp->indent,
                                         done);
                    if (res != NO_ERR) {
                        return res;
                    }
                    if (TK_CUR_TYP(pcb->tkc) == TK_TT_RBRACE) {
                        loopdone = TRUE;
                    }
                }
            }

            /* move to next stmt, this one is done */
            res = advance_token(pcb);
            if (res != NO_ERR) {
                return res;
            }

            /* end the complex element */
            end_ext_elem(scb, 
                         prefix,
                         extension->name,
                         startindent);
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
        break;
    default:
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    

    return res;

}   /* write_yin_stmt */


/********************************************************************
* FUNCTION write_yin_contents
*  
* Go through the token chain and write all the
* YIN elements according to algoritm in sec. 11
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
static status_t
    write_yin_contents (yang_pcb_t *pcb,
                        const yangdump_cvtparms_t *cp,
                        ses_cb_t *scb)
{
    status_t       res;
    boolean        done;
    int            i;

    tk_reset_chain(pcb->tkc);
    res = NO_ERR;

    /* need to skip the first 3 tokens because the
     * [sub]module name { tokens have already been
     * handled as a special case 
     */
    for (i = 0; i < 4; i++) {
        res = advance_token(pcb);
        if (res != NO_ERR) {
            return res;
        }
    }

    done = FALSE;
    while (!done && res == NO_ERR) {
        res = write_yin_stmt(pcb, 
                             cp, 
                             scb, 
                             cp->indent,
                             &done);
    }

    return res;

}   /* write_yin_contents */


/*********     E X P O R T E D   F U N C T I O N S    **************/


/********************************************************************
* FUNCTION yangyin_convert_module
*  
*  The YIN namespace will be the default namespace
*  The imported modules will use the xmlprefix in use
*  which is the YANG prefix unless it is a duplicate
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
    yangyin_convert_module (yang_pcb_t *pcb,
                        const yangdump_cvtparms_t *cp,
                        ses_cb_t *scb)
{
    ncx_import_t  *import;
    status_t       res;
    int32          indent;

    res = NO_ERR;
    indent = cp->indent;

    /* start the XML document */
    ses_putstr(scb, XML_START_MSG);

    /* write the top-level start-tag for [sub]module
     * do this special case so the XMLNS attributes
     * can be generated as well as the 'name' attribute
     */
    start_yin_elem(scb,
                   (pcb->top->ismod) 
                   ? YANG_K_MODULE : YANG_K_SUBMODULE,
                   0);
    ses_putchar(scb, ' ');

    /* write the name attribute */
    write_yin_attr(scb, YANG_K_NAME, pcb->top->name, indent);

    /* write the YIN namespace decl as the default namespace */
    write_yin_attr(scb, XMLNS, YIN_URN, indent);

    /* write the module prefix and module namespace */
    if (pcb->top->ismod) {
        ses_putstr_indent(scb, XMLNS, indent);
        ses_putchar(scb, ':');
        ses_putstr(scb, ncx_get_mod_prefix(pcb->top));
        ses_putchar(scb, '=');
        ses_putchar(scb, '"');
        ses_putstr(scb, ncx_get_modnamespace(pcb->top));
        ses_putchar(scb, '"');
    }

    /* write the xmlns decls for all the imports used
     * by this [sub]module
     */
    for (import = (ncx_import_t *)dlq_firstEntry(&pcb->top->importQ);
         import != NULL;
         import = (ncx_import_t *)dlq_nextEntry(import)) {
        if (import->used) {
            write_import_xmlns(scb, import, indent);
        }
    }

    /* finish the top-level start tag */
    ses_putchar(scb, '>');

    res = write_yin_contents(pcb, cp, scb);

    /* finish the top-level element */

    end_yin_elem(scb,
                 (pcb->top->ismod) 
                 ? YANG_K_MODULE : YANG_K_SUBMODULE,
                 0);

    return res;

}   /* yangyin_convert_module */


/* END file yin.c */
