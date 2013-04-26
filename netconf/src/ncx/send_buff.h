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
#ifndef _H_send_buff
#define _H_send_buff
/*  FILE: send_buff.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    Send buffer utility

*********************************************************************
*								    *
*		   C H A N G E	 H I S T O R Y			    *
*								    *
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
20-jan-07    abb      begun
*/

#include <stdlib.h>

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


/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


/********************************************************************
* FUNCTION send_buff
*
* Send the buffer to the ncxserver
*
* This function is used by applications which do not
* select for write_fds, and may not block (if fnctl used)
* 
* INPUTS:
*   fd == the socket to write to
*   buffer == the buffer to write
*   cnt == the number of bytes to write
*
* RETURNS:
*   status
*********************************************************************/
extern status_t
    send_buff (int fd,
	       const char *buffer, 
	       size_t cnt);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_send_buff */
