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
/*  FILE: xpath_wr.c

   Write an XPath expression in normalized format
                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
24apr09      abb      begun;

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xpath_wr
#include "xpath_wr.h"
#endif


/********************************************************************
* FUNCTION wr_expr
* 
* Write the specified XPath expression to the current session
* using the default prefixes
*
* The XPath pcb must be previously parsed and found valid
*
* INPUTS:
*    scb == session control block to use
*    msg == message header to use
*    xpathval == the value containing the XPath expr to write
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    wr_expr (ses_cb_t *scb,
             xpath_pcb_t *pcb)
{
    const xmlChar        *cur_val, *prefix;
    boolean               done, needspace, needprefix;
    xmlns_id_t            cur_nsid;
    tk_type_t             cur_typ;
    status_t              res;
    uint32                quotes;

    if (pcb->tkc == NULL || pcb->parseres != NO_ERR) {
        return  pcb->parseres;
    }

    tk_reset_chain(pcb->tkc);

    done = FALSE;
    while (!done) {

        /* get the next token */
        res = TK_ADV(pcb->tkc);
        if (res != NO_ERR) {
            res = NO_ERR;
            done = TRUE;
            continue;
        }

        needspace = FALSE;
        needprefix = FALSE;
        quotes = 0;

        cur_typ = TK_CUR_TYP(pcb->tkc);
        cur_nsid = TK_CUR_NSID(pcb->tkc);
        cur_val = TK_CUR_VAL(pcb->tkc);

        switch (cur_typ) {
        case TK_TT_LBRACE:
        case TK_TT_RBRACE:
        case TK_TT_LPAREN:
        case TK_TT_RPAREN:
        case TK_TT_LBRACK:
        case TK_TT_RBRACK:
        case TK_TT_STAR:
        case TK_TT_ATSIGN:
        case TK_TT_COLON:
        case TK_TT_PERIOD:
        case TK_TT_FSLASH:
        case TK_TT_DBLCOLON:
        case TK_TT_DBLFSLASH:
            needspace = FALSE;
            break;
        case TK_TT_SEMICOL:
        case TK_TT_COMMA:
        case TK_TT_EQUAL:
        case TK_TT_BAR:
        case TK_TT_PLUS:
        case TK_TT_MINUS:
        case TK_TT_LT:
        case TK_TT_GT:
        case TK_TT_RANGESEP:
        case TK_TT_NOTEQUAL:
        case TK_TT_LEQUAL:
        case TK_TT_GEQUAL:
        case TK_TT_STRING:
        case TK_TT_SSTRING:
        case TK_TT_TSTRING:
        case TK_TT_VARBIND:
        case TK_TT_NCNAME_STAR:
        case TK_TT_DNUM:
        case TK_TT_HNUM:
        case TK_TT_RNUM:
            needspace = TRUE;
            break;
        case TK_TT_MSTRING:
        case TK_TT_MSSTRING:
        case TK_TT_QVARBIND:
            needspace = TRUE;
            needprefix = TRUE;
            break;
        case TK_TT_QSTRING:
            needspace = TRUE;
            quotes = 2;
            break;
        case TK_TT_SQSTRING:
            needspace = TRUE;
            quotes = 1;
            break;
        default:
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        if (cur_val == NULL) {
            return SET_ERROR(ERR_INTERNAL_VAL);
        }

        switch (cur_typ) {
        case TK_TT_VARBIND:
        case TK_TT_QVARBIND:
            ses_putchar(scb, '$');
            break;
        default:
            ;
        }

        if (needprefix && cur_nsid != 0) {
            prefix = xmlns_get_ns_prefix(cur_nsid);
            if (prefix) {
                ses_putstr(scb, prefix);
                ses_putchar(scb, ':');
            } else {
                return SET_ERROR(ERR_INTERNAL_VAL);
            }
        }

        if (quotes == 1) {
            ses_putchar(scb, '\'');
        } else if (quotes == 2) {
            ses_putchar(scb, '"');
        }

        ses_putstr(scb, cur_val);

        if (cur_typ == TK_TT_NCNAME_STAR) {
            ses_putchar(scb, ':');
            ses_putchar(scb, '*');
        }

        if (needspace && quotes == 0) {
            ses_putchar(scb, ' ');
        }

        if (quotes == 1) {
            ses_putchar(scb, '\'');
        } else if (quotes == 2) {
            ses_putchar(scb, '"');
        }
    }

    return NO_ERR;

}  /* wr_expr */


/************    E X P O R T E D   F U N C T I O N S    *************/


/********************************************************************
* FUNCTION xpath_wr_expr
* 
* Write the specified XPath expression to the current session
* using the default prefixes
*
* The XPath pcb must be previously parsed and found valid
*
* INPUTS:
*    scb == session control block to use
*    msg == message header to use
*    xpathval == the value containing the XPath expr to write
*
* RETURNS:
*   status
*********************************************************************/
status_t
    xpath_wr_expr (ses_cb_t *scb,
                   val_value_t *xpathval)
{
    xpath_pcb_t          *pcb;
    status_t              res;

#ifdef DEBUG
    if (!scb || !xpathval) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    pcb = val_get_xpathpcb(xpathval);
    if (pcb == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    res = wr_expr(scb, pcb);

    return res;

}  /* xpath_wr_expr */


/* END xpath_wr.c */
