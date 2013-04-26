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
#ifndef _H_agt_proc
#define _H_agt_proc
/*  FILE: agt_proc.h
*********************************************************************
*                                                                   *
*                         P U R P O S E                             *
*                                                                   *
*********************************************************************

   NETCONF /proc file system monitoring

*********************************************************************
*                                                                   *
*                   C H A N G E         H I S T O R Y               *
*                                                                   *
*********************************************************************

date             init     comment
----------------------------------------------------------------------
18-jul-09    abb      Begun.
*/

#ifndef _H_status
#include "status.h"
#endif

/********************************************************************
*                                                                   *
*                         C O N S T A N T S                         *
*                                                                   *
*********************************************************************/

/********************************************************************
*                                                                   *
*                             T Y P E S                             *
*                                                                   *
*********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*                                                                   *
*                        F U N C T I O N S                          *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION agt_proc_init
*
* INIT 1:
*   Initialize the proc monitor module data structures
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_proc_init (void);


/********************************************************************
* FUNCTION agt_proc_init2
*
* INIT 2:
*   Initialize the monitoring data structures
*   This must be done after the <running> config is loaded
*
* INPUTS:
*   none
* RETURNS:
*   status
*********************************************************************/
extern status_t
    agt_proc_init2 (void);


/********************************************************************
* FUNCTION agt_proc_cleanup
*
* Cleanup the module data structures
*
* INPUTS:
*   none
* RETURNS:
*   none
*********************************************************************/
extern void 
    agt_proc_cleanup (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif            /* _H_agt_proc */
