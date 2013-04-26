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
#ifndef _H_yangstats
#define _H_yangstats

/*  FILE: yangstats.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

  YANG metrics and statistics module
 
*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
29-mar-10    abb      Begun

*/

#ifndef _H_ses
#include "ses.h"
#endif

#ifndef _H_yang
#include "yang.h"
#endif

#ifndef _H_yangdump
#include "yangdump.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
 * FUNCTION yangstats_collect_module_stats
 * 
 * Generate a statistics report for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *********************************************************************/
extern void
    yangstats_collect_module_stats (yang_pcb_t *pcb,
                                    yangdump_cvtparms_t *cp);


/********************************************************************
 * FUNCTION yangstats_output_module_stats
 * 
 * Generate a statistics report for a module
 *
 * INPUTS:
 *    pcb == parser control block with module to use
 *    cp == conversion parameters to use
 *    scb == session control block to use for output
 *********************************************************************/
extern void
    yangstats_output_module_stats (yang_pcb_t *pcb,
                                   yangdump_cvtparms_t *cp,
                                   ses_cb_t *scb);


/********************************************************************
 * FUNCTION yangstats_output_final_stats
 * 
 * Generate the final stats report as requested
 * by the --stats and --stat-totals CLI parameters
 *
 * INPUTS:
 *    cp == conversion parameters to use
 *    scb == session control block to use for output
 *********************************************************************/
extern void
    yangstats_output_final_stats (yangdump_cvtparms_t *cp,
                                  ses_cb_t *scb);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_yangstats */
