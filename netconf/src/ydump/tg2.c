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
/*  FILE: tg2.c

  Generate turbogears 2 files for a specific YANG module

  For module foo and bar:
  The TG2 format is:

     tg2env/myproject/myproject/
        templates/
          foo.html      Genshi
          bar.html
        controllers/
          root.py
          foo.py        TG2::RootController
          bar.py
        model/
          __init__.py
          foo.py        SQLAlchemy::DeclarativeBase
          bar.py


  Mapping from YANG to SQL:

  Everything maps to a table class, even a leaf.
  Nested lists and leaf-lists need to be split out
  into their own table, inherited all the ancestor keys
  as unique columns.  Foreign keys link the tables
  to manage fate-sharing.

  Choice nodes are collapsed so the first visible descendant
  node within each case is raised to the the same level
  as the highest-level choice.  This is OK since node names
  are not allowed to collide within a choice/case meta-containers.

  Container nodes are collapsed to the child nodes, but the container
  name is retained as a prefix because child nodes of the
  container may collide with siblings of the container.
  
  Nested lists and leaf-lists are unfolded as follows:

                     list   top
                        |
             +----------+-----------+
             |          |           |
           key a     list b       leaf c
                       |
             +----------+-----------+-------+
             |          |           |       |
           key d      key e       leaf f   leaf-list g

   table: top:
       id: primary key;
       a: unique column;    
       c: column;
  
   table top_b :
       id: primary key;
       top_id: foreign key;
       top_a: unique column;
       d: unique column;
       e: unique column;
       f: column;

   table top_b_g :
       id: primary key;
       top_id: foreign key;
       top_b_id: foreign key;
       top_a: unique column;
       top_b_d: unique column;
       top_b_e: unique column;
       g: column;



*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
11mar10      abb      begun

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

#ifndef _H_c_util
#include "c_util.h"
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

#ifndef _H_py_util
#include "py_util.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_tg2
#include "tg2.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_tstamp
#include "tstamp.h"
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


/*********     E X P O R T E D   F U N C T I O N S    **************/


/********************************************************************
* FUNCTION tg2_convert_module_model
* 
*  Convert the YANG module to TG2 code for the SQL model
*  for the YANG data nodes; used by the Yuma Tools Workbench
*  application
*
* INPUTS:
*   pcb == parser control block of module to convert
*          This is returned from ncxmod_load_module_ex
*   cp == convert parms struct to use
*   scb == session to use for output
*
* RETURNS:
*   status
*********************************************************************/
status_t
    tg2_convert_module_model (const yang_pcb_t *pcb,
                              const yangdump_cvtparms_t *cp,
                              ses_cb_t *scb)
{
    const ncx_module_t    *mod;
    const yang_node_t     *node;


    /* the module should already be parsed and loaded */
    mod = pcb->top;
    if (!mod) {
        return SET_ERROR(ERR_NCX_MOD_NOT_FOUND);
    }

    /* start the python file */
    write_py_header(scb, mod, cp);

    /* convert_one_module_model(mod, cp, scb); */

    if (cp->unified && mod->ismod) {
        for (node = (const yang_node_t *)dlq_firstEntry(&mod->allincQ);
             node != NULL;
             node = (const yang_node_t *)dlq_nextEntry(node)) {
            if (node->submod) {
                /* convert_one_module_model(node->submod, cp, scb); */
            }
        }
    }

    /* end the python file */
    write_py_footer(scb, mod);

    return NO_ERR;

}   /* tg2_convert_module_model */


/* END file tg2.c */
