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
/*  FILE: conf.c

    This module parses an NCX Text config file

    The text input format matches the output
    format of vaL-dump_value and val_stdout_value.

    In addition to that syntax, comments may also be present.
    The '#' char starts a comments and all chars up until
    the next newline char (but not including the newline)
    end a comment.

    A comment cannot be inside a string.
    
   Commands in a config file are not whitespace sensitive,
   except within string values.

   Unless inside a string, a newline will end a command line,
   so <name> value pairs must appear on the same line.

     foo 42
     bar fred
     baz fred flintstone
     goo "multi line
          value entered for goo
          must start on the same line as goo"

   Containers are represented by curly braces

   myvars {
     foo 42
     bar fred
     baz fred flintstone
   }

   The left brace must appear on the same line as foo
   The right brace must be after the newline for the
   preceding (baz) command.

   Objects with index values are listed sequentially
   after the list name.  Whitespace is significant,
   so string index values with whitespace or newlines
   must be in double quotes.  

   The nodes within the array which represent the
   index will not be repreated inside the curly braces.
  
   For example, the array 'myarray' is indexed by 'foo',
   so it does not appear again within the entry.

   myarray 42 {
     bar fred
     baz fred flintstone
   }
 
   point 22 478 {
     red 6
     green 17
     blue 9
   }

   
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
20oct07      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_conf
#include "conf.h"
#endif

#ifndef _H_dlq
#include  "dlq.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_ncx
#include "ncx.h"
#endif

#ifndef _H_ncxconst
#include "ncxconst.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tk
#include  "tk.h"
#endif

#ifndef _H_typ
#include  "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define CONF_DEBUG 1
#endif


/********************************************************************
*                                                                   *
*                              T Y P E S                            *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION adv_tk
* 
* Advance to the next token
* Print error message if EOF found instead
*
* INPUTS:
*   tkc == token chain
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    adv_tk (tk_chain_t  *tkc)
{
    status_t  res;

    res = TK_ADV(tkc);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, NULL, res);
    }
    return res;
            
}   /* adv_tk */


/********************************************************************
* FUNCTION get_tk
* 
* Get the next token
* Skip over TK_TT_NEWLINE until a different type is found
*
* INPUTS:
*   tkc == token chain
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    get_tk (tk_chain_t  *tkc)
{
    boolean done;
    status_t  res;

    res = NO_ERR;
    done = FALSE;
    while (!done) {
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            done = TRUE;
        } else if (TK_CUR_TYP(tkc) != TK_TT_NEWLINE) {
            done = TRUE;
        }
    }
    return res;
            
} /* get_tk */


/********************************************************************
* FUNCTION consume_tk
* 
* Consume a specified token type
* Skip over TK_TT_NEWLINE until a different type is found
*
* INPUTS:
*   tkc == token chain
*   ttyp == expected token type
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    consume_tk (tk_chain_t  *tkc,
                tk_type_t  ttyp)
{
    status_t  res;

    res = get_tk(tkc);
    if (res != NO_ERR) {
        return res;
    } else {
        return (TK_CUR_TYP(tkc) == ttyp) ? 
            NO_ERR : ERR_NCX_WRONG_TKTYPE;
    }
    /*NOTREACHED*/
            
} /* consume_tk */


/********************************************************************
* FUNCTION match_name
* 
* Get the next token which must be a string which matches name
*
* INPUTS:
*   tkc == token chain
*   name  == name to match
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    match_name (tk_chain_t  *tkc,
                const xmlChar *name)
{
    status_t res;

    /* get the next token */
    res = consume_tk(tkc, TK_TT_TSTRING);
    if (res == NO_ERR) {
        if (xml_strcmp(TK_CUR_VAL(tkc), name)) {
            res = ERR_NCX_WRONG_VAL;
        }
    }
    return res;

} /* match_name */


/********************************************************************
* FUNCTION skip_object
* 
* Skip until the end of the object
* current token is the parmset name
*
* INPUTS:
*   tkc == token chain
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    skip_object (tk_chain_t  *tkc)
{
    status_t res;
    uint32   brace_count;
    boolean  done;

    brace_count = 0;
    done = FALSE;

    /* get the next token */
    while (!done) {
        res = TK_ADV(tkc);
        if (res != NO_ERR) {
            return res;
        }
        switch (TK_CUR_TYP(tkc)) {
        case TK_TT_LBRACE:
            brace_count++;
            break;
        case TK_TT_RBRACE:
            if (brace_count <= 1) {
                done = TRUE;
            } else {
                brace_count--;
            }
        default:
            ;
        }
    }
    return NO_ERR;

} /* skip_object */


/********************************************************************
* FUNCTION parse_index
* 
* Parse, and fill the indexQ for one val_value_t struct during
* processing of a text config file
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* The value name is the current token.
* Based on the value typdef, the res of the tokens
* comprising the value statement will be processed
*
* INPUTS:
*   tkc == token chain
*   obj == the object template to use for filling in 'val'
*   val == initialized value struct, without any value,
*          which will be filled in by this function
*   nsid == namespace ID to use for this value
*
* OUTPUTS:
*   indexQ filled in as tokens representing the index components
*   are parsed.  NEWLINE tokens are skipped as needed
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    parse_index (tk_chain_t  *tkc,
                 obj_template_t *obj,
                 val_value_t *val,
                 xmlns_id_t  nsid)
{
    obj_key_t     *indef, *infirst;
    val_value_t   *inval;
    status_t       res;

    infirst = obj_first_key(obj);

    /* first make value nodes for all the index values */
    for (indef = infirst; 
         indef != NULL; 
         indef = obj_next_key(indef)) {

        /* advance to the next non-NEWLINE token */
        res = get_tk(tkc);
        if (res != NO_ERR) {
            ncx_conf_exp_err(tkc, res, "index value");
            return res;
        }

        /* check if a valid token is given for the index value */
        if (TK_CUR_TEXT(tkc)) {
            inval = val_make_simval_obj(indef->keyobj,
                                        TK_CUR_VAL(tkc), 
                                        &res);
            if (!inval) {
                ncx_conf_exp_err(tkc, res, "index value");
                return res;
            } else {
                val_change_nsid(inval, nsid);
                val_add_child(inval, val);
            }
        } else {
            res = ERR_NCX_WRONG_TKTYPE;
            ncx_conf_exp_err(tkc, res, "index value");
            return res;
        }
    }

    /* generate the index chain in the indexQ */
    res = val_gen_index_chain(obj, val);
    if (res != NO_ERR) {
        ncx_print_errormsg(tkc, NULL, res);
    }

    return res;
        
} /* parse_index */


/********************************************************************
* FUNCTION parse_val
* 
* Parse, and fill one val_value_t struct during
* processing of a text config file
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
* The value name is the current token.
* Based on the value typdef, the res of the tokens
* comprising the value statement will be processed
*
* INPUTS:
*   tkc == token chain
*   obj == the object template struct to use for filling in 'val'
*   val == initialized value struct, without any value,
*          which will be filled in by this function
*   nsid == namespace ID to use for this value
*   valname == name of the value struct
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    parse_val (tk_chain_t  *tkc,
               obj_template_t *obj,
               val_value_t *val)
{
    obj_template_t  *chobj;
    val_value_t     *chval;
    const xmlChar   *valname, *useval;
    typ_def_t       *typdef;
    status_t         res;
    ncx_btype_t      btyp;
    boolean          done;
    xmlns_id_t       nsid;

    btyp = obj_get_basetype(obj);
    nsid = obj_get_nsid(obj);
    valname = obj_get_name(obj);
    typdef = obj_get_typdef(obj);

    /* check if there is an index clause expected */
    if (typ_has_index(btyp)) {
        res = parse_index(tkc, obj, val, nsid);
        if (res != NO_ERR) {
            return res;
        }
    }

    /* get next token, NEWLINE is significant at this point */
    res = adv_tk(tkc);
    if (res != NO_ERR) {
        return res;
    }

    /* the current token should be the value for a leaf
     * or a left brace for the start of a complex type
     * A NEWLINE is treated as if the user entered a
     * zero-length string for the value.  (Unless the
     * base type is NCX_BT_EMPTY, in which case the NEWLINE
     * is the expected token
     */
    if (typ_is_simple(btyp)) {
        /* form for a leaf is: foo [value] NEWLINE  */
        if (TK_CUR_TYP(tkc)==TK_TT_NEWLINE) {
            useval = NULL;
        } else {
            useval = TK_CUR_VAL(tkc);
        }
        res = val_set_simval(val, 
                             typdef, 
                             nsid, 
                             valname,
                             useval);

        if (res != NO_ERR) {
            log_error("\nError: '%s' cannot be set to '%s'",
                      valname,
                      (TK_CUR_VAL(tkc)) ? TK_CUR_VAL(tkc) : EMPTY_STRING);
            if (btyp == NCX_BT_EMPTY) {
                ncx_conf_exp_err(tkc, res, "empty");
            } else {
                ncx_conf_exp_err(tkc, res, "simple value string");
            }
            return res;
        }

        /* get a NEWLINE unless current token is already a NEWLINE */
        if (TK_CUR_TYP(tkc) != TK_TT_NEWLINE) {
            res = adv_tk(tkc);
            if (res != NO_ERR) {
                return res;
            }
            if (TK_CUR_TYP(tkc) != TK_TT_NEWLINE) {
                res = ERR_NCX_WRONG_TKTYPE;
                ncx_conf_exp_err(tkc, res, "\\n");
            }
        }
    } else {
        /* complex type is foo {  ... } or
         * foo index1 index2 { ... }
         * If there is an index, it was already parsed
         */
        res = consume_tk(tkc, TK_TT_LBRACE);
        if (res != NO_ERR) {
            ncx_conf_exp_err(tkc, res, "left brace");
            return res;
        }

        /* get all the child nodes specified for this complex type */
        res = NO_ERR;
        done = FALSE;
        while (!done && res==NO_ERR) {
            /* start out looking for a child node name or a
             * right brace to end the sub-section
             */
            if (tk_next_typ(tkc)==TK_TT_NEWLINE) {
                /* skip the NEWLINE token */
                (void)adv_tk(tkc);
            } else if (tk_next_typ(tkc)==TK_TT_RBRACE) {
                /* found end of sub-section */
                done = TRUE;
            } else {
                /* get the next token */
                res = adv_tk(tkc);
                if (res != NO_ERR) {
                    continue;
                }

                /* make sure cur token is an identifier string
                 * if so, find the child node and call this function
                 * recursively to fill it in and add it to
                 * the parent 'val'
                 */
                if (TK_CUR_ID(tkc)) {
                    /* parent 'typdef' must have a child with a name
                     * that matches the current token vale
                     */
                    chobj = obj_find_child(obj, 
                                           TK_CUR_MOD(tkc),
                                           TK_CUR_VAL(tkc));
                    if (chobj) {
                        chval = val_new_value();
                        if (!chval) {
                            res = ERR_INTERNAL_MEM;
                            ncx_print_errormsg(tkc, NULL, res);
                        } else {
                            val_init_from_template(chval, chobj);
                            res = parse_val(tkc, chobj, chval);
                            if (res == NO_ERR) {
                                val_add_child(chval, val);
                            } else {
                                val_free_value(chval);
                            }
                        }
                    } else {
                        /* string is not a child name in this typdef */
                        res = ERR_NCX_DEF_NOT_FOUND;
                        ncx_conf_exp_err(tkc, res, "identifier string");
                    }
                } else {
                    /* token is not an identifier string */
                    res = ERR_NCX_WRONG_TKTYPE;
                    ncx_conf_exp_err(tkc, res, "identifier string");
                }
            }
        }  /* end loop through all the child nodes */

        /* expecting a right brace to finish the complex value */
        if (res == NO_ERR) {
            res = consume_tk(tkc, TK_TT_RBRACE);
            if (res != NO_ERR) {
                ncx_conf_exp_err(tkc, res, "right brace");
                return res;
            }
        }
    }

    return res;

}  /* parse_val */


/********************************************************************
* FUNCTION parse_parm
* 
* Parse, and fill one val_value_t struct during
* processing of a parmset
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*   tkc == token chain
*   val == container val to fill in
*   keepvals == TRUE to save existing parms in 'ps', as needed
*               FALSE to overwrite old parms in 'ps', as needed
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    parse_parm (tk_chain_t  *tkc,
                val_value_t *val,
                boolean keepvals)
{
    obj_template_t         *obj;
    const xmlChar          *modname;
    val_value_t            *curparm, *newparm;
    status_t                res;
    ncx_iqual_t             iqual;
    boolean                 match, usewarning, isdefault;

    
    /* get the next token, which must be a TSTRING
     * representing the parameter name 
     */
    if (TK_CUR_TYP(tkc) != TK_TT_TSTRING) {
        res = ERR_NCX_WRONG_TKTYPE;
        ncx_conf_exp_err(tkc, res, "parameter name");
        return res;
    }

    curparm = NULL;
    usewarning = ncx_warning_enabled(ERR_NCX_CONF_PARM_EXISTS);

    /* check if this TSTRING is a parameter in this parmset
     * make sure to always check for prefix:identifier
     * This is automatically processed in tk.c
     */
    if (TK_CUR_MOD(tkc)) {
        modname = xmlns_get_module
            (xmlns_find_ns_by_prefix(TK_CUR_MOD(tkc)));
        if (modname) {
            curparm = val_find_child(val, 
                                     modname,
                                     TK_CUR_VAL(tkc));
        }
    }  else {
        curparm = val_find_child(val, 
                                 val_get_mod_name(val),
                                 TK_CUR_VAL(tkc));
    }
        
    if (curparm) {
        obj = curparm->obj;
    } else {
        obj = obj_find_child(val->obj, 
                             TK_CUR_MOD(tkc),
                             TK_CUR_VAL(tkc));
    }
    if (!obj) {
        res = ERR_NCX_UNKNOWN_PARM;
        if (TK_CUR_MOD(tkc)) {
            log_error("\nError: parameter '%s:%s' not found",
                      TK_CUR_MOD(tkc),
                      TK_CUR_VAL(tkc));
        } else {
            log_error("\nError: parameter '%s' not found",
                      TK_CUR_VAL(tkc));
        }
        ncx_conf_exp_err(tkc, res, "parameter name");
        return res;
    }

    /* got a valid parameter name, now create a new parm
     * even if it may not be kept.  There are corner-cases
     * that require the new value be parsed before knowing
     * if a parm value is a duplicate or not
     */
    newparm = val_new_value();
    if (!newparm) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(tkc, NULL, res);
        return res;
    }
    val_init_from_template(newparm, obj);

    /* parse the parameter value */
    res = parse_val(tkc, obj, newparm);
    if (res != NO_ERR) {
        val_free_value(newparm);
        return res;
    }

    /* check if a potential current value exists, or just
     * add the newparm to the parmset
     */
    if (curparm) {
        isdefault = val_set_by_default(curparm);
        iqual = obj_get_iqualval(obj);
        if (iqual == NCX_IQUAL_ONE || iqual == NCX_IQUAL_OPT) {
            /* only one allowed, check really a match */
            match = TRUE;
            if (val_has_index(curparm) &&
                !val_index_match(newparm, curparm)) {
                match = FALSE;
            }

            if (!match) {
                val_add_child(newparm, val);
            } else if (isdefault) {
                dlq_remove(curparm);
                val_free_value(curparm);
                val_add_child(newparm, val);
            } else if (keepvals) {
                if (usewarning) {
                    /* keep current value and toss new value */
                    log_warn("\nWarning: Parameter '%s' already exists. "
                             "Not using new value\n", 
                             curparm->name);
                    if (LOGDEBUG2) {
                        val_dump_value(newparm, NCX_DEF_INDENT);
                        log_debug2("\n");
                    }
                }
                val_free_value(newparm);
            } else {
                if (usewarning) {
                    /* replace current value and warn old value tossed */
                    log_warn("\nconf: Parameter '%s' already exists. "
                             "Overwriting with new value\n",
                             curparm->name);
                    if (LOGDEBUG2) {
                        val_dump_value(newparm, NCX_DEF_INDENT);
                        log_debug2("\n");
                    }
                }
                dlq_remove(curparm);
                val_free_value(curparm);
                val_add_child(newparm, val);
            }
        } else {
            /* mutliple instances allowed */
            val_add_child(newparm, val);
        }
    } else {
        val_add_child(newparm, val);
    }

    return NO_ERR;

}  /* parse_parm */


/********************************************************************
* FUNCTION parse_top
* 
* Parse, and fill one val_value_t struct
*
* Error messages are printed by this function!!
* Do not duplicate error messages upon error return
*
*
* INPUTS:
*   tkc == token chain
*   val == value iniitalized and could be already filled in
*   keepvals == TRUE to save existing values in 'val', as needed
*               FALSE to overwrite old values in 'val', as needed
*
* RETURNS:
*   status of the operation
*********************************************************************/
static status_t 
    parse_top (tk_chain_t  *tkc,
               val_value_t *val,
               boolean keepvals)
{
    status_t       res;
    boolean        done;

    res = NO_ERR;

    /* get the container name */
    done = FALSE;
    while (!done) {
        res = match_name(tkc, obj_get_name(val->obj));
        if (res == ERR_NCX_EOF) {
            if (LOGDEBUG) {
                log_debug("\nconf: object '%s' not found in file '%s'",
                          obj_get_name(val->obj), 
                          tkc->filename);
            }
            return NO_ERR;
        } else if (res != NO_ERR) {
            res = skip_object(tkc);
            if (res != NO_ERR) {
                return res;
            }
        } else {
            done = TRUE;
        }
    }

    /* get a left brace */
    res = consume_tk(tkc, TK_TT_LBRACE);
    if (res != NO_ERR) {
        ncx_conf_exp_err(tkc, res, "left brace to start object");
        return res;
    }

    done = FALSE;
    while (!done) {

        res = get_tk(tkc);
        if (res == ERR_NCX_EOF) {
            return NO_ERR;
        } else if (res != NO_ERR) {
            return res;
        }
        
        /* allow an empty parmset */
        if (TK_CUR_TYP(tkc)==TK_TT_RBRACE) {
            done = TRUE;
        } else {
            res = parse_parm(tkc, val, keepvals);
            if (res != NO_ERR) {
                done = TRUE;
            }
        }
    }

    return res;

}  /* parse_top */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION conf_parse_from_filespec
* 
* Parse a file as an NCX text config file against
* a specific parmset definition.  Fill in an
* initialized parmset (and could be partially filled in)
*
* Error messages are printed by this function!!
*
* If a value is already set, and only one value is allowed
* then the 'keepvals' parameter will control whether that
* value will be kept or overwitten
*
* INPUTS:
*   filespec == absolute path or relative path
*               This string is used as-is without adjustment.
*   val       == value struct to fill in, must be initialized
*                already with val_new_value or val_init_value
*   keepvals == TRUE if old values should always be kept
*               FALSE if old vals should be overwritten
*   fileerr == TRUE to generate a missing file error
*              FALSE to return NO_ERR instead, if file not found
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t 
    conf_parse_val_from_filespec (const xmlChar *filespec,
                                  val_value_t *val,
                                  boolean keepvals,
                                  boolean fileerr)
{
    tk_chain_t    *tkc;
    FILE          *fp;
    xmlChar       *sourcespec;
    status_t       res;

#ifdef DEBUG
    if (!filespec || !val) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    } 
#endif

    if (*filespec == 0) {
        log_error("\nError: no config file name specified");
        return ERR_NCX_INVALID_VALUE;
    }
        
    res = NO_ERR;
    sourcespec = ncx_get_source(filespec, &res);
    if (!sourcespec) {
        return res;
    }

    fp = fopen((const char *)sourcespec, "r");

    if (!fp) {
        m__free(sourcespec);
        if (fileerr) {
            log_error("\nError: config file '%s' could not be opened",
                      filespec);
            return ERR_FIL_OPEN;
        } else {
            return NO_ERR;
        }
    }

    if (LOGINFO) {
        log_info("\nLoading CLI parameters from '%s'", sourcespec);
    }

    m__free(sourcespec);
    sourcespec = NULL;

    /* get a new token chain */
    res = NO_ERR;
    tkc = tk_new_chain();
    if (!tkc) {
        res = ERR_INTERNAL_MEM;
        ncx_print_errormsg(NULL, NULL, res);
        fclose(fp);
        return res;
    }

    /* else setup the token chain and parse this config file */
    tk_setup_chain_conf(tkc, fp, filespec);

    res = tk_tokenize_input(tkc, NULL);

#ifdef CONF_TK_DEBUG
    if (LOGDEBUG3) {
        tk_dump_chain(tkc);
    }
#endif

    if (res == NO_ERR) {
        res = parse_top(tkc, val, keepvals);
    }

    fclose(fp);
    tkc->fp = NULL;
    tk_free_chain(tkc);

    if (res != NO_ERR) {
        log_error("\nError: invalid .conf file '%s' (%s)",
                  filespec,
                  get_error_string(res));
    }

    return res;

}  /* conf_parse_val_from_filespec */


/* END file conf.c */
