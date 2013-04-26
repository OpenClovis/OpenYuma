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
#ifndef _H_libncx
#define _H_libncx

/*  FILE: libncx.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    This is a wrapper for all NCX Library functions.
    Include individual H files instead to save compile time.



    NCX Module Library Utility Functions


                      Container of Definitions
                        +-----------------+ 
                        |   ncx_module_t  |
                        |   ncxtypes.h    |
                        +-----------------+

            Template/Schema 
          +-----------------+ 
          | obj_template_t  |
          |     obj.h       |
          +-----------------+
             ^          |
             |          |       Value Instances
             |          |     +-----------------+
             |          +---->|   val_value_t   |
             |                |      val.h      |
             |                +-----------------+
             |                               
             |       Data Types
             |   +--------------------+      
             +---|   typ_template_t   |      
                 |       typ.h        |      
                 +--------------------+      

 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
29-oct-05    abb      Begun
20-jul-08    abb      Start YANG rewrite; remove PSD and PS
23-aug-09    abb      Update diagram in header
29-may-10    abb      Make libncx wrapper for all H files
                      in libncx.so
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <xmlstring.h>

#ifndef _H_procdefs
#include "procdefs.h"
#endif

#ifndef _H_b64
#include "b64.h"
#endif

#ifndef _H_blob
#include "blob.h"
#endif

#ifndef _H_bobhash
#include "bobhash.h"
#endif

#ifndef _H_cap
#include "cap.h"
#endif

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_cli
#include "cli.h"
#endif

#ifndef _H_conf
#include "conf.h"
#endif

#ifndef _H_def_reg
#include "def_reg.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ext
#include "ext.h"
#endif

#ifndef _H_getcb
#include "getcb.h"
#endif

#ifndef _H_grp
#include "grp.h"
#endif

#ifndef _H_help
#include "help.h"
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

#ifndef _H_ncx_appinfo
#include "ncx_appinfo.h"
#endif

#ifndef _H_ncx_feature
#include "ncx_feature.h"
#endif

#ifndef _H_ncx_list
#include "ncx_list.h"
#endif

#ifndef _H_ncx_num
#include "ncx_num.h"
#endif

#ifndef _H_ncx_str
#include "ncx_str.h"
#endif

#ifndef _H_ncxmod
#include "ncxmod.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_obj_help
#include "obj_help.h"
#endif

#ifndef _H_op
#include "op.h"
#endif

#ifndef _H_rpc
#include "rpc.h"
#endif

#ifndef _H_rpc_err
#include "rpc_err.h"
#endif

#ifndef _H_runstack
#include "runstack.h"
#endif

#ifndef _H_send_buff
#include "send_buff.h"
#endif

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_ses_msg
#include "ses_msg.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifndef _H_tk
#include "tk.h"
#endif

#ifndef _H_top
#include "top.h"
#endif

#ifndef _H_tstamp
#include "tstamp.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_var
#include "var.h"
#endif

#ifndef _H_version
#include "version.h"
#endif

#ifndef _H_xml_msg
#include "xml_msg.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_xml_val
#include "xml_val.h"
#endif

#ifndef _H_xml_wr
#include "xml_wr.h"
#endif

#ifndef _H_xmlns
#include "xmlns.h"
#endif

#ifndef _H_xpath
#include "xpath.h"
#endif

#ifndef _H_xpath1
#include "xpath1.h"
#endif

#ifndef _H_xpath_yang
#include "xpath_yang.h"
#endif

#ifndef _H_xpath_wr
#include "xpath_wr.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yang_ext
#include "yang_ext.h"
#endif

#ifndef _H_yang_grp
#include "yang_grp.h"
#endif

#ifndef _H_yang_obj
#include "yang_obj.h"
#endif

#ifndef _H_yang_parse
#include "yang_parse.h"
#endif

#ifndef _H_yang_typ
#include "yang_typ.h"
#endif

#ifndef _H_yin
#include "yin.h"
#endif

#ifndef _H_yinyang
#include "yinyang.h"
#endif

#ifdef __cplusplus
}   /* end extern 'C' */
#endif

#endif	    /* _H_libncx */
