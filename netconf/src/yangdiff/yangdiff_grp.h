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
#ifndef _H_yangdiff_grp
#define _H_yangdiff_grp

/*  FILE: yangdiff_grp.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

  Report differences related to YANG groupings
 
*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
10-jul-08    abb      Split out from yangdiff.c

*/

#ifndef _H_yangdiff
#include "yangdiff.h"
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
 * FUNCTION output_groupingQ_diff
 * 
 *  Output the differences report for a Q of grouping definitions
 *  Not always called for top-level groupings; Can be called
 *  for nested groupings
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old grp_template_t to use
 *    newQ == Q of new grp_template_t to use
 *
 *********************************************************************/
extern void
    output_groupingQ_diff (yangdiff_diffparms_t *cp,
			   dlq_hdr_t *oldQ,
			   dlq_hdr_t *newQ);


/********************************************************************
 * FUNCTION groupingQ_changed
 * 
 *  Check if a (nested) Q of groupings changed
 *
 * INPUTS:
 *    cp == parameter block to use
 *    oldQ == Q of old grp_template_t to use
 *    newQ == Q of new grp_template_t to use
 *
 * RETURNS:
 *    1 if field changed
 *    0 if field not changed
 *********************************************************************/
extern uint32
    groupingQ_changed (yangdiff_diffparms_t *cp,
		       dlq_hdr_t *oldQ,
		       dlq_hdr_t *newQ);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_yangdiff_grp */
