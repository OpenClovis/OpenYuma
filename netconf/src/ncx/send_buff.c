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
/*  FILE: send_buff.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
20jan07      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_send_buff
#include  "send_buff.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

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
status_t
    send_buff (int fd,
               const char *buffer, 
               size_t cnt)
{
    size_t sent, left;
    ssize_t  retsiz;
    uint32   retry_cnt;

    retry_cnt = 1000;
    sent = 0;
    left = cnt;
    
    while (sent < cnt) {
        retsiz = write(fd, buffer, left);
        if (retsiz < 0) {
            switch (errno) {
            case EAGAIN:
            case EBUSY:
                if (--retry_cnt) {
                    break;
                } /* else fall through */
            default:
                return errno_to_status();
            }
        } else {
            sent += (size_t)retsiz;
            buffer += retsiz;
            left -= (size_t)retsiz;
        }
    }

    return NO_ERR;

} /* send_buff */

/* END file send_buff.c */
