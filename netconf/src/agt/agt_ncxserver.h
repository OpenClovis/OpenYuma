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
#ifndef _H_agt_ncxserver
#define _H_agt_ncxserver

/*  FILE: agt_ncxserver.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    NCX Server Library

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
11-jan-07    abb      Begun

*/

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			 C O N S T A N T S			    *
*								    *
*********************************************************************/
#define NCXSERVER_SOCKNAME   "/tmp/ncxserver.sock"

/********************************************************************
*								    *
*			     T Y P E S				    *
*								    *
*********************************************************************/

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/

/********************************************************************
 * FUNCTION agt_ncxserver_run
 * 
 * IO server loop for the ncxserver socket
 * 
 * RETURNS:
 *   status
 *********************************************************************/
extern status_t 
    agt_ncxserver_run (void);


/********************************************************************
 * FUNCTION agt_ncxserver_clear_fd
 * 
 * Clear a dead session from the select loop
 * 
 * INPUTS:
 *   fd == file descriptor number for the socket to clear
 *********************************************************************/
extern void
    agt_ncxserver_clear_fd (int fd);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_agt_ncxserver */
