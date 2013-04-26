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
#ifndef _H_obj_help
#define _H_obj_help

/*  FILE: obj_help.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Help command support for obj_template_t

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
17-aug-08    abb      Begun; split from obj.h to prevent H file loop

*/


#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_help
#include "help.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION obj_dump_template
*
* Dump the contents of an obj_template_t struct for help text
*
* INPUTS:
*   obj == obj_template to dump help for
*   mode == requested help mode
*   nestlevel == number of levels from the top-level
*                that should be printed; 0 == all levels
*   indent == start indent count
*********************************************************************/
extern void
    obj_dump_template (obj_template_t *obj,
		       help_mode_t mode,
		       uint32 nestlevel,
		       uint32 indent);


/********************************************************************
* FUNCTION obj_dump_datadefQ
*
* Dump the contents of a datadefQ for debugging
*
* INPUTS:
*   datadefQ == Q of obj_template to dump
*   mode    ==  desired help mode (brief, normal, full)
*   nestlevel == number of levels from the top-level
*                that should be printed; 0 == all levels
*   indent == start indent count
*********************************************************************/
extern void
    obj_dump_datadefQ (dlq_hdr_t *datadefQ,
		       help_mode_t mode,
		       uint32 nestlevel,
		       uint32 indent);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_obj_help */
