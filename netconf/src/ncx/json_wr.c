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
/*  FILE: json_wr.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
27aug11      abb      begun; start from clone of xml_wr.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>
#include  <string.h>

#include  "procdefs.h"
#include  "dlq.h"
#include  "ncx.h"
#include  "ncx_num.h"
#include  "ncxconst.h"
#include  "obj.h"
#include  "ses.h"
#include  "status.h"
#include  "val.h"
#include  "val_util.h"
#include  "xmlns.h"
#include  "xml_msg.h"
#include  "json_wr.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define JSON_WR_DEBUG  1
#endif


/********************************************************************
* FUNCTION write_json_string_value
* 
* Write a simple value as a JSON string
*
* INPUTS:
*   scb == session control block
*   val == value to write
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    write_json_string_value (ses_cb_t *scb,
                             val_value_t *val)
{
    xmlChar *valstr = val_make_sprintf_string(val);
    if (valstr) {
        ses_putchar(scb, '"');
        ses_putjstr(scb, valstr, -1);
        ses_putchar(scb, '"');
        m__free(valstr);
        return NO_ERR;
    }
    return ERR_INTERNAL_MEM;

} /* write_json_string_value */


/********************************************************************
* FUNCTION write_full_check_val
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
*   justone == TRUE if just one of these; FALSE if more than one
*   isfirst == TRUE if this is the first (top) val printed
*   isfirstchild == TRUE if this is the first value of an array
*                == FALSE if this is the 2nd - Nth value of an array
* RETURNS:
*   status
*********************************************************************/
static status_t
    write_full_check_val (ses_cb_t *scb,
                          xml_msg_hdr_t *msg,
                          val_value_t *val,
                          int32  startindent,
                          val_nodetest_fn_t testfn,
                          boolean justone,
                          boolean isfirst,
                          boolean isfirstchild)
{
    val_value_t       *out;
    boolean            malloced = FALSE;
    status_t           res = NO_ERR;

    out = val_get_value(scb, msg, val, testfn, TRUE, &malloced, &res);

    if (!out || res != NO_ERR) {
        if (res == ERR_NCX_SKIPPED) {
            res = NO_ERR;
        }
        if (out && malloced) {
            val_free_value(out);
        }
        return res;
    }

    /* check if this is an external file to send */
    if (out->btyp == NCX_BT_EXTERN ||
        out->btyp == NCX_BT_INTERN ||
        typ_is_simple(out->btyp)) {

        if (isfirst) {
            write_json_string_value(scb, out);
        } else {
            /* write the name of the node */
            ses_putchar(scb, '"');
            ses_putjstr(scb, out->name, -1);
            ses_putchar(scb, '"');
            ses_putchar(scb, ':');

            switch (out->btyp) {
            case NCX_BT_EXTERN:
                ses_putchar(scb, '"');
                val_write_extern(scb, out);
                ses_putchar(scb, '"');
                break;
            case NCX_BT_INTERN:
                ses_putchar(scb, '"');
                val_write_intern(scb, out);
                ses_putchar(scb, '"');
                break;
            case NCX_BT_ENUM:
                ses_putchar(scb, '"');
                if (VAL_ENUM_NAME(out)) {
                    ses_putjstr(scb, VAL_ENUM_NAME(out), -1);
                }
                ses_putchar(scb, '"');
                break;
            case NCX_BT_EMPTY:
                if (out->v.boo) {
                    ses_putjstr(scb, NCX_EL_NULL, -1);
                } // else skip this value! should already be checked
                break;
            case NCX_BT_BOOLEAN:
                if (out->v.boo) {
                    ses_putjstr(scb, NCX_EL_TRUE, -1);
                } else {
                    ses_putjstr(scb, NCX_EL_FALSE, -1);
                }
                break;
            default:
                write_json_string_value(scb, out);
            }
        }
    } else {
        val_value_t *chval;
        val_value_t *lastch = NULL;
        val_value_t *nextch = NULL;

        int32 indent = (startindent < 0) ? -1 : 
            startindent + ses_indent_count(scb);

        if (isfirst) {
            ses_putchar(scb, '{');
        }

        /* render a complex type; either an object or an array */
        if (isfirstchild) {
            ses_putchar(scb, '"');
            ses_putjstr(scb, out->name, -1);
            ses_putchar(scb, '"');
            ses_putchar(scb, ':');

            if (!justone) {
                ses_putchar(scb, '[');
            }
        }


        ses_indent(scb, indent);
        ses_putchar(scb, '{');

        for (chval = val_get_first_child(out); 
             chval != NULL; 
             chval = nextch) {

            /* JSON ignores XML namespaces, so foo:a and bar:a
             * are both encoded in the same array
             */
            uint32 childcnt = val_instance_count(out, NULL, chval->name);
            boolean firstchild = 
                (lastch && !xml_strcmp(lastch->name, chval->name)) ?
                FALSE : TRUE;

            lastch = chval;
            nextch = val_get_next_child(chval);

            res = write_full_check_val(scb, 
                                       msg,
                                       chval,
                                       indent,
                                       testfn,
                                       (childcnt > 1) ? FALSE : TRUE,
                                       FALSE,
                                       firstchild);
                                       
            if (res == ERR_NCX_SKIPPED) {
                ;
            } else if (res != NO_ERR) {
                /* FIXME: do something about error;
                 * may not always be OK to continue to next child node 
                 */ ;
            } else if (nextch) {
                ses_putchar(scb, ',');
                if (indent >= 0 && 
                    (typ_is_simple(nextch->btyp) || 
                     xml_strcmp(nextch->name, chval->name))) {
                    ses_indent(scb, indent);
                    ses_putchar(scb, ' ');
                }
            }
        }

        ses_indent(scb, indent);
        ses_putchar(scb, '}');

        if (!justone) {
            val_value_t *peeknext = val_get_next_child(val);            
            if (!peeknext || xml_strcmp(val->name, peeknext->name)) {
                ses_putchar(scb, ']');
            }
        }

        if (isfirst) {
            ses_indent(scb, startindent);
            ses_putchar(scb, '}');
        }            
    }

    if (malloced && out) {
        val_free_value(out);
    }

    return res;

}  /* write_full_check_val */


/************  E X T E R N A L    F U N C T I O N S    **************/


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
status_t
    json_wr_full_check_val (ses_cb_t *scb,
                            xml_msg_hdr_t *msg,
                            val_value_t *val,
                            int32  startindent,
                            val_nodetest_fn_t testfn)
{
#ifdef DEBUG
    if (!scb || !msg || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    return write_full_check_val(scb, msg, val, startindent, 
                                testfn, TRUE, TRUE, TRUE);

}  /* json_wr_full_check_val */


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
status_t
    json_wr_check_open_file (FILE *fp, 
                             val_value_t *val,
                             int32 startindent,
                             int32  indent,
                             val_nodetest_fn_t testfn)
{
    ses_cb_t   *scb = NULL;
    rpc_msg_t  *msg = NULL;
    status_t    res = NO_ERR;
    xml_attrs_t myattrs;

#ifdef DEBUG
    if (!fp || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    indent = min(indent, 9);
    xml_init_attrs(&myattrs);

    /* get a dummy session control block */
    scb = ses_new_dummy_scb();
    if (!scb) {
        res = ERR_INTERNAL_MEM;
    } else {
        scb->fp = fp;
        scb->indent = indent;
    }

    /* get a dummy output message */
    if (res == NO_ERR) {
        msg = rpc_new_out_msg();
        if (!msg) {
            res = ERR_INTERNAL_MEM;
        } else {
            /* hack -- need a queue because there is no top
             * element which this usually shadows
             */
            msg->rpc_in_attrs = &myattrs;
        }
    }

    if (res == NO_ERR) {
        /* write the tree in JSON format */
        res = json_wr_full_check_val(scb, &msg->mhdr, val, 
                                     startindent, testfn);
        if (res != ERR_NCX_SKIPPED) {
            ses_finish_msg(scb);
        } else {
            res = NO_ERR;
        }
    }

    /* clean up and exit */
    if (msg) {
        rpc_free_msg(msg);
    }
    if (scb) {
        scb->fp = NULL;   /* do not close the file */
        ses_free_scb(scb);
    }

    xml_clean_attrs(&myattrs);
    return res;

} /* json_wr_check_open_file */


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
status_t
    json_wr_check_file (const xmlChar *filespec, 
                        val_value_t *val,
                        int32 startindent,
                        int32  indent,
                        val_nodetest_fn_t testfn)
{
    FILE       *fp;
    status_t    res;

#ifdef DEBUG
    if (!filespec || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    fp = fopen((const char *)filespec, "w");
    if (!fp) {
        log_error("\nError: Cannot open XML file '%s'", filespec);
        return ERR_FIL_OPEN;
    }
    res = json_wr_check_open_file(fp, val, startindent, indent, testfn);
    fclose(fp);

    return res;

} /* json_wr_check_file */


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
status_t
    json_wr_file (const xmlChar *filespec,
                  val_value_t *val,
                  int32 startindent,
                  int32 indent)
{
    return json_wr_check_file(filespec, val, startindent, indent, NULL);

} /* json_wr_file */


/* END file json_wr.c */
