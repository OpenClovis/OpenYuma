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
/*  FILE: ses.c

   NETCONF Session Manager: Common Support

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
06jun06      abb      begun; cloned from ses_mgr.c

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include  <assert.h>

#include  "procdefs.h"
#include  "log.h"
#include  "ncx.h"
#include  "ncx_num.h"
#include  "ses.h"
#include  "ses_msg.h"
#include  "status.h"
#include  "tstamp.h"
#include  "val.h"
#include  "xml_util.h"

/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#ifdef DEBUG
#define SES_DEBUG 1
/* #define SES_SSH_DEBUG 1 */
/* #define SES_DEBUG_TRACE 1 */
#define SES_DEBUG_XML_TRACE 1

#endif

#define LTSTR     (const xmlChar *)"&lt;"
#define GTSTR     (const xmlChar *)"&gt;"
#define AMPSTR    (const xmlChar *)"&amp;"
#define QSTR      (const xmlChar *)"&quot;"

/* used by yangcli to read in between stdin polling */
#define MAX_READ_TRIES   500


/********************************************************************
*                                                                   *
*                           T Y P E S                               *
*                                                                   *
*********************************************************************/
    

/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
static ses_total_stats_t totals;


/********************************************************************
* FUNCTION accept_buffer_ssh_v10
*
* Handle one input buffer within the ses_accept_input function
* transport is SSH or TCP; protocol is NETCONF:base:1.0
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.  For SSH, also need to detect the EOM flag
* and remove it + control input to the reader.
*
* This function breaks the byte stream into ses_msg_t structs
* that get queued on the session's msgQ
*
* INPUTS:
*   scb == session control block to accept input for
*   len  == number of bytes in scb->readbuff just read
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    accept_buffer_ssh_v10 (ses_cb_t *scb,
                           size_t len)
{
    ses_msg_t      *msg;
    ses_msg_buff_t *buff;
    const char     *endmatch;
    status_t        res;
    boolean         done;
    xmlChar         ch;
    uint32          count;

#ifdef SES_DEBUG
    if (LOGDEBUG3 && scb->state != SES_ST_INIT) {
        scb->readbuff[len] = 0;
        log_debug3("\nses: accept buffer (%u):\n%s\n", 
                   len, 
                   scb->readbuff);
    } else if (LOGDEBUG2) {
        log_debug2("\nses: accept buffer (%u)", len);
    }
#endif

    /* make sure there is a current message */
    msg = (ses_msg_t *)dlq_lastEntry(&scb->msgQ);
    if (msg == NULL || msg->ready) {
        /* need a new message */
        res = ses_msg_new_msg(&msg);
        if (res != NO_ERR) {
            return res;
        }
        /* add early */
        dlq_enque(msg, &scb->msgQ);
    }

    /* init local vars */
    endmatch = NC_SSH_END;
    done = FALSE;
    count = 0;
    res = NO_ERR;

    /* make sure there is a current buffer to use */
    buff = (ses_msg_buff_t *)dlq_lastEntry(&msg->buffQ);
    if (buff == NULL || buff->bufflen == SES_MSG_BUFFSIZE) {
        /* need a new buffer */
        res = ses_msg_new_buff(scb,
                               FALSE,  /* outbuff */
                               &buff);
        if (res != NO_ERR) {
            return res;
        }
        dlq_enque(buff, &msg->buffQ);
    }

    /* check the chars in the buffer for the 
     * the NETCONF EOM if this is an SSH session
     *
     * TBD: ses how xmlReader handles non-well-formed XML 
     * and junk text.  May need some sort of error state
     * to dump chars until the EOM if SSH, or force the
     * session closed, or force the transport to reset somehow
     */
    while (!done) {
        /* check if the read buffer length has been reached */
        if (count == (uint32)len) {
            done = TRUE;
            continue;
        }

        /* check if a new message is needed */
        if (msg == NULL) {
            res = ses_msg_new_msg(&msg);
            if (res == NO_ERR) {
                /* put msg in the msg Q early */
                dlq_enque(msg, &scb->msgQ);
            } else {
                return res;
            }
        }

        /* check if a buffer is needed */
        if (buff == NULL) {
            /* need new first buff for a message */
            res = ses_msg_new_buff(scb,
                                   FALSE,  /* outbuff */
                                   &buff);
            if (res != NO_ERR) {
                return res;
            }
            dlq_enque(buff, &msg->buffQ);
        } else if (buff->buffpos == SES_MSG_BUFFSIZE) {
            /* current buffer is full; get a new one */
            buff->buffpos = 0;
            buff->bufflen = SES_MSG_BUFFSIZE;
            res = ses_msg_new_buff(scb,
                                   FALSE,  /* outbuff */
                                   &buff);
            if (res != NO_ERR) {
                return res;
            }
            dlq_enque(buff, &msg->buffQ);
        }

        /* get the next char in the input buffer and advance the pointer */
        ch = scb->readbuff[count++];
        buff->buff[buff->buffpos++] = ch;

        /* handle the char in the buffer based on the input state */
        switch (scb->instate) {
        case SES_INST_IDLE:
            /* check for EOM if SSH or just copy the char */
            if (ch==*endmatch) {
                scb->instate = SES_INST_INEND;
                scb->inendpos = 1;
            } else {
                scb->instate = SES_INST_INMSG;
            }
            break;
        case SES_INST_INMSG:
            /* check for EOM if SSH or just copy the char */
            if (ch==*endmatch) {
                scb->instate = SES_INST_INEND;
                scb->inendpos = 1;
            }
            break;
        case SES_INST_INEND:
            /* already matched at least 1 EOM char
             * try to match the rest of the SSH EOM string 
             */
            if (ch == endmatch[scb->inendpos]) {
                /* check message complete */
                if (++scb->inendpos == NC_SSH_END_LEN) {
                    /* completely matched the SSH EOM marker 
                     * finish the current message and put it in the inreadyQ
                     *
                     * buff->buffpos points to the first char after
                     * the EOM string, check any left over bytes
                     * to start a new message
                     *
                     * handle any bytes left at the end of 'buff'
                     * if this is a base:1.1 session then there
                     * is a possibility the new framing will start
                     * right away, because the peer sent a <hello>
                     * and an <rpc> back-to-back
                     *
                     * save the buffer and make the message ready to parse 
                     * don't let the xmlreader see the EOM string
                     */
                    buff->bufflen = buff->buffpos - NC_SSH_END_LEN;
                    buff->buffpos = 0;
                    buff->islast = TRUE;
                    msg->curbuff = NULL;
                    msg->ready = TRUE;
                    ses_msg_make_inready(scb);

                    /* reset reader state */
                    scb->instate = SES_INST_IDLE;
                    scb->inendpos = 0;

                    /* force a new message and buffer */
                    msg = NULL;
                    buff = NULL;

                    /* check corner-case: do not try to handle
                     * base:1.1 back 2 back messages 
                     * if protocol not set yet by hello
                     * an empty message is the last one right now
                     * error out if a 3rd real message <rpc> is found
                     */
                    if (scb->protocol == NCX_PROTO_NONE &&
                        ncx_protocol_enabled(NCX_PROTO_NETCONF11) &&
                        dlq_count(&scb->msgQ) >= 4) {
                        /* do not continue processing readbuff */
                        log_error("\nses: Message could not "
                                  "be deferred for s:%d because "
                                  "protocol framing not set",
                                  scb->sid);
                        return ERR_NCX_OPERATION_FAILED; 
                    }
                }  /* else still more chars in EOM string to match */
            } else {
                /* char did not match the expected position in the 
                 * EOM string, go back to MSG state
                 */
                scb->instate = SES_INST_INMSG;
                scb->inendpos = 0;
            }
            break;
        default:
            /* should not happen */
            return SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    return NO_ERR;

}  /* accept_buffer_ssh_v10 */


/********************************************************************
* FUNCTION accept_buffer_ssh_v11
*
* Handle one input buffer within the ses_accept_input function
* Transport is SSH.  Protocol is NETCONF:base:1.1
* Use SSH base:1.1 framing, not old EOM sequence
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.  Handle NETCONF over SSH base:1.1 protocol framing
* 
* This function breaks the byte stream into ses_msg_t structs
* that get queued on the session's msgQ.  
*
* The chunks encoded into each incoming buffer will be
* be mapped and stored in 1 or more ses_buffer_t structs.
* There will be wasted buffer space if the chunks
* are very small and more than SES_MAX_BUFF_CHUNKS
* per buffer are received
*
* RFC 6242, sec 4.2.  Chunked Framing Mechanism

   This mechanism encodes all NETCONF messages with a chunked framing.
   Specifically, the message follows the ABNF [RFC5234] rule Chunked-
   Message:


        Chunked-Message = 1*chunk
                          end-of-chunks

        chunk           = LF HASH chunk-size LF
                          chunk-data
        chunk-size      = 1*DIGIT1 0*DIGIT
        chunk-data      = 1*OCTET

        end-of-chunks   = LF HASH HASH LF

        DIGIT1          = %x31-39
        DIGIT           = %x30-39
        HASH            = %x23
        LF              = %x0A
        OCTET           = %x00-FF


   The chunk-size field is a string of decimal digits indicating the
   number of octets in chunk-data.  Leading zeros are prohibited, and
   the maximum allowed chunk-size value is 4294967295.

* INPUTS:
*   scb == session control block to accept input for
*   len  == number of bytes in scb->readbuff just read
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    accept_buffer_ssh_v11 (ses_cb_t *scb,
                           size_t len)
{
    ses_msg_t      *msg;
    ses_msg_buff_t *buff;
    status_t        res;
    uint32          count;
    boolean         done;
    xmlChar         ch;
    size_t          chunkleft, inbuffleft, outbuffleft, copylen;
    ncx_num_t       num;

#ifdef SES_DEBUG
    if (LOGDEBUG3 && scb->state != SES_ST_INIT) {
        scb->readbuff[len] = 0;
        log_debug3("\nses: accept base:1.1 buffer (%u):\n%s\n", 
                   len, 
                   scb->readbuff);
    } else if (LOGDEBUG2) {
        log_debug2("\nses: accept base:1.1 buffer (%u)", len);
    }
#endif

    /* make sure there is a current message */
    msg = (ses_msg_t *)dlq_lastEntry(&scb->msgQ);
    if (msg == NULL || msg->ready) {
        /* need a new message */
        res = ses_msg_new_msg(&msg);
        if (res != NO_ERR) {
            return res;
        }
        /* add early */
        dlq_enque(msg, &scb->msgQ);
    }

    /* init local vars */
    done = FALSE;
    res = NO_ERR;
    ch = 0;
    count = 0;

    /* make sure there is a current buffer to use */
    buff = (ses_msg_buff_t *)dlq_lastEntry(&msg->buffQ);
    if (buff == NULL || buff->bufflen == SES_MSG_BUFFSIZE) {
        /* need a new buffer */
        res = ses_msg_new_buff(scb,
                               FALSE,  /* outbuff */
                               &buff);
        if (res != NO_ERR) {
            return res;
        }
        dlq_enque(buff, &msg->buffQ);
    }

    /* check the chars in the buffer for the 
     * frame markers, based on session instate value
     * the framing breaks will be mixed in with the XML
     */
    while (!done) {
        /* check if the read buffer length has been reached */
        if (count == (uint32)len) {
            done = TRUE;
            continue;
        }

        /* check if a buffer is needed */
        if (buff == NULL) {
            /* need new first buff for a message */
            res = ses_msg_new_buff(scb,
                                   FALSE,  /* outbuff */
                                   &buff);
            if (res != NO_ERR) {
                return res;
            }
            dlq_enque(buff, &msg->buffQ);
        } else if (buff->buffpos == SES_MSG_BUFFSIZE) {
            /* current buffer is full; get a new one */
            buff->buffpos = 0;
            buff->bufflen = SES_MSG_BUFFSIZE;
            res = ses_msg_new_buff(scb,
                                   FALSE,  /* outbuff */
                                   &buff);
            if (res != NO_ERR) {
                return res;
            }
            dlq_enque(buff, &msg->buffQ);
        }

        /* get the next char in the input buffer and advance the pointer */
        ch = scb->readbuff[count++];

        /* handle the char in the buffer based on the input state */
        switch (scb->instate) {
        case SES_INST_IDLE:
            /* expecting start of the first chunk of a message */
            if (ch == '\n') {
                /* matched the \n to start a chunk */
                scb->instate = SES_INST_INSTART;
                scb->inendpos = 1;
            } else {
                /* return framing error */
                if (LOGDEBUG) {
                    log_debug("\nses: invalid base:1;1 framing "
                              "(idle: expect first newline)");
                }
                done = TRUE;
                res = ERR_NCX_INVALID_FRAMING;
            }
            break;
        case SES_INST_INSTART:
            if (scb->inendpos == 1) {
                if (ch == '#') {
                    /* matched first hash mark # */
                    scb->inendpos++;
                } else {
                    /* return framing error; 
                     * save buff for garbage collection 
                     */
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(instart: expect '#')");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else if (scb->inendpos == 2) {
                /* expecting at least 1 starting digit */
                if (ch >= '1' && ch <= '9') {
                    scb->startchunk[0] = ch;   /* save first num char */
                    scb->inendpos++;  /* == 3 now */
                } else {
                    /* return framing error */
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(instart: expect digit)");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else {
                /* looking for end of a number
                 * expecting an ending digit or \n
                 */
                if (scb->inendpos == SES_MAX_STARTCHUNK_SIZE) {
                    /* invalid number -- too long error */
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(instart: number too long)");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                } else if (ch == '\n') {
                    /* have the complete number now
                     * done with chunk start tag
                     * get a binary number from this number
                     */
                    ncx_init_num(&num);
                    scb->startchunk[scb->inendpos - 2] = 0;
                    scb->inendpos = 0;
                    res = ncx_convert_num(scb->startchunk,
                                          NCX_NF_DEC,
                                          NCX_BT_UINT32,
                                          &num);
                    if (res == NO_ERR) {
                        msg->expchunksize = num.u;
                        msg->curchunksize = 0;
                        scb->instate = SES_INST_INMSG;
                    } else {
                        if (LOGDEBUG) {
                            log_debug("\nses: invalid base:1;1 framing "
                                      "(instart: invalid number)");
                        }
                        done = TRUE;
                        res = ERR_NCX_INVALID_FRAMING;
                    }
                    ncx_clean_num(NCX_BT_UINT32, &num);
                } else if (ch >= '0' && ch <= '9') {
                    /* continue collecting digits */
                    scb->startchunk[scb->inendpos - 2] = ch;
                    scb->inendpos++;
                } else {
                    /* return framing error; invalid char */
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(instart: expect digit char)");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            }
            break;
        case SES_INST_INMSG:
            /* expecting first part or Nth part of a chunk */
            count--;   /* back up count */
            chunkleft = msg->expchunksize - msg->curchunksize;
            inbuffleft = len - count;
            outbuffleft = SES_MSG_BUFFSIZE - buff->buffpos;
            copylen = min(inbuffleft, chunkleft);

            /* account for the amount copied above */
            msg->curchunksize += copylen;
            if (msg->curchunksize == msg->expchunksize) {
                /* finished this chunk */
                scb->inendpos = 0;
                scb->instate = SES_INST_INBETWEEN;
            } /* else finished the input buffer */

            /* copy the required input bytes to 1 or more buffers */
            if (outbuffleft >= copylen) {
                /* rest of chunk or input fits in rest of current buffer */
                xml_strncpy(&buff->buff[buff->buffpos],
                            &scb->readbuff[count],
                            copylen);
                buff->buffpos += copylen;
                count += copylen;
            } else {
                /* rest of the current buffer not big enough;
                 * split input across N buffers;
                 * first finish current buffer
                 */
                xml_strncpy(&buff->buff[buff->buffpos],
                            &scb->readbuff[count],
                            outbuffleft);
                buff->buffpos = 0;
                buff->bufflen = SES_MSG_BUFFSIZE;
                copylen -= outbuffleft;
                count += outbuffleft;

                /* fill N-2 full buffers and possibly
                 * 1 final partial buffer
                 */
                while (copylen > 0) {
                    size_t copy2len;

                    /* get new buffer */
                    res = ses_msg_new_buff(scb,
                                           FALSE,  /* outbuff */
                                           &buff);
                    if (res != NO_ERR) {
                        return res;
                    }
                    dlq_enque(buff, &msg->buffQ);

                    copy2len = min(SES_MSG_BUFFSIZE, copylen);
                    xml_strncpy(&buff->buff[buff->buffpos],
                                &scb->readbuff[count],
                                copy2len);
                    buff->buffpos += copy2len;
                    buff->bufflen = copy2len;
                    copylen -= copy2len;
                    count += copy2len;
                }
            }
            break;
        case SES_INST_INBETWEEN:
            if (scb->inendpos == 0) {
                if (ch == '\n') {
                    scb->inendpos++;
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(inbetween: expect newline)");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else if (scb->inendpos == 1) {
                if (ch == '#') {
                    scb->inendpos++;
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(inbetween: expect '#')");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else {
                if (ch == '#') {
                    scb->inendpos++;
                    scb->instate = SES_INST_INEND;
                } else if (ch >= '1' && ch <= '9') {
                    /* back up and process this char in start state
                     * account for first 2 chars \n#
                     */
                    count--;
                    scb->instate = SES_INST_INSTART;
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(inbetween: expect digit or '#')");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            }
            break;
        case SES_INST_INEND:
            /* expect to match 4 char \n##\n sequence */
            if (scb->inendpos == 2) {
                if (ch == '#') {
                    scb->inendpos++;
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(inend: expect '#')");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else if (scb->inendpos == 3) {
                if (ch == '\n') {
                    /* completely matched the SSH End of Chunks marker
                     * finish the current message and put it in the inreadyQ
                     */
                    msg->curbuff = NULL;
                    msg->ready = TRUE;
                    ses_msg_make_inready(scb);
                    
                    /* reset reader state */
                    scb->instate = SES_INST_IDLE;
                    scb->inendpos = 0;
                    buff->bufflen = buff->buffpos;
                    buff->buffpos = 0;

                    if (count < len) {
                        /* need a new message */
                        res = ses_msg_new_msg(&msg);
                        if (res != NO_ERR) {
                            return res;
                        }
                        /* add early */
                        dlq_enque(msg, &scb->msgQ);

                        /* get a new buffer */
                        res = ses_msg_new_buff(scb,
                                               FALSE,  /* outbuff */
                                               &buff);
                        if (res != NO_ERR) {
                            return res;
                        }
                        dlq_enque(buff, &msg->buffQ);
                    } else {
                        buff = NULL;
                    }
                } else {
                    if (LOGDEBUG) {
                        log_debug("\nses: invalid base:1;1 framing "
                                  "(inend: expect newline)");
                    }
                    done = TRUE;
                    res = ERR_NCX_INVALID_FRAMING;
                }
            } else {
                /* should not happen */
                done = TRUE;
                res = SET_ERROR(ERR_INTERNAL_VAL);
            }
            break;
        default:
            /* should not happen */
            done = TRUE;
            res = SET_ERROR(ERR_INTERNAL_VAL);
        }
    }

    return res;

}  /* accept_buffer_ssh_v11 */


/********************************************************************
* FUNCTION put_char_entity
*
* Write a character entity for the specified character
*
* INPUTS:
*   scb == session control block to start msg 
*   ch == character to write as a character entity
*
*********************************************************************/
static void
    put_char_entity (ses_cb_t *scb,
                     xmlChar ch)
{
    xmlChar     numbuff[NCX_MAX_NUMLEN];

    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%u", (uint32)ch);

    ses_putchar(scb, '&');
    ses_putchar(scb, '#');
    ses_putstr(scb, numbuff);
    ses_putchar(scb, ';');

}  /* put_char_entity */


/********************************************************************
* FUNCTION handle_prolog_state
*
* Deal with the first few characters of an incoming message
* hack: xmlTextReaderRead wants to start off
* with a newline for some reason, so always
* start the first buffer with a newline, even if
* none was sent by the NETCONF peer.
* Only the first 0xa char seems to matter
* Trailing newlines do not seem to affect the problem
*
* Also, the first line needs to be the <?xml ... ?>
* prolog directive, or the libxml2 parser refuses
* to use the incoming message.  
* It quits and returns EOF instead.
*
* Only the current buffer is processed within the message
* INPUTS:
*   msg == current message to process
*   buffer == buffer to fill in
*   bufflen == max buffer size
*   buff == current buffer about to be read
*   endpos == max end pos of buff to use
*   retlen == address of running return length
*
* OUTPUTS:
*   buffer is filled in with the prolog if needed
*   msg->prolog_state is updated as needed
*   *retlen may be increased if prolog or newline added
*********************************************************************/
static void
    handle_prolog_state (ses_msg_t *msg,
                         char *buffer,
                         int bufflen,
                         ses_msg_buff_t *buff,
                         size_t endpos,
                         int *retlen)
{
    boolean needprolog = FALSE;
    boolean needfirstnl = FALSE;
    char    tempbuff[4];
    int     i, j, k;

    /* save the first 3 chars in the temp buffer */
    memset(tempbuff, 0x0, 4);

    switch (msg->prolog_state) {
    case SES_PRST_NONE:
        if ((endpos - buff->buffpos) < 3) {
            msg->prolog_state = SES_PRST_WAITING;
            return;
        } else if (!strncmp((const char *)&buff->buff[buff->buffpos], 
                            "\n<?", 
                            3)) {
            /* expected string is present */
            msg->prolog_state = SES_PRST_DONE;
            return;
        } else if (!strncmp((const char *)&buff->buff[buff->buffpos], 
                            "<?", 
                            2)) {
            /* expected string except a newline needs
             * to be inserted first to make libxml2 happy
             */
            needfirstnl = TRUE;
            msg->prolog_state = SES_PRST_DONE;
        } else {
            needprolog = TRUE;
            msg->prolog_state = SES_PRST_DONE;
        }
        break;
    case SES_PRST_WAITING:
        if ((*retlen + (endpos - buff->buffpos)) < 3) {
            /* keep waiting */
            return;
        } else {
            msg->prolog_state = SES_PRST_DONE;

            strncpy(tempbuff, buffer, *retlen);
            for (i = *retlen, j=buff->buffpos; i <= 3; i++, j++) {
                tempbuff[i] = (char)buff->buff[j];
            }

            if (!strncmp(tempbuff, "\n<?", 3)) {
                /* expected string is present */
                ;
            } else if (!strncmp(tempbuff, "<?", 2)) {
                /* expected string except a newline needs
                 * to be inserted first to make libxml2 happy
                 */
                needfirstnl = TRUE;
            } else {
                needprolog = TRUE;
            }
        }
        break;
    case SES_PRST_DONE:
        return;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }

    if (needfirstnl || needprolog) {
        buffer[0] = '\n';
        i = 1;
    } else {
        i = 0;
    }

    if (needprolog) {
        /* expected string sequence is not present */
        if (bufflen < XML_START_MSG_SIZE+5) {
            SET_ERROR(ERR_INTERNAL_VAL);
        } else {
            /* copy the prolog string inline
             * to make libxml2 happy
             */
            strncpy(&buffer[i], (const char *)XML_START_MSG, bufflen-i);

            if (tempbuff[0]) {
                for (j = XML_START_MSG_SIZE+i, k = 0; 
                     k <= *retlen; 
                     j++, k++) {
                    if (k == *retlen) {
                        buffer[j] = 0;
                    } else {
                        buffer[j] = tempbuff[k];
                    }
                }
            }
            *retlen += XML_START_MSG_SIZE;
        }
    }

    *retlen += i;

}  /* handle_prolog_state */


/************   E X T E R N A L   F U N C T I O N S     ***********/


/********************************************************************
* FUNCTION ses_new_scb
*
* Create a new session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized SCB, or NULL if malloc error
*********************************************************************/
ses_cb_t *
    ses_new_scb (void)
{
    ses_cb_t  *scb;
    xmlChar   *now;
    
    now = m__getMem(TSTAMP_MIN_SIZE);
    if (now == NULL) {
        return NULL;
    }
    tstamp_datetime(now);

    scb = m__getObj(ses_cb_t);
    if (scb == NULL) {
        m__free(now);
        return NULL;
    }
    memset(scb, 0x0, sizeof(ses_cb_t));

    /* TBD: make session read buff size configurable */
    scb->readbuffsize = SES_READBUFF_SIZE;

    /* make sure the debug log trace code never writes a zero byte
     * past the end of a full read buffer by adding 2 pad bytes
     */
    scb->readbuff = m__getMem(scb->readbuffsize + 2);
    if (scb->readbuff == NULL) {
        m__free(now);
        m__free(scb);
        return NULL;
    }

    scb->start_time = now;
    dlq_createSQue(&scb->msgQ);
    dlq_createSQue(&scb->freeQ);
    dlq_createSQue(&scb->outQ);
    scb->linesize = SES_DEF_LINESIZE;
    scb->withdef = NCX_DEF_WITHDEF;
    scb->indent = NCX_DEF_INDENT;
    scb->cache_timeout = NCX_DEF_VTIMEOUT;
    return scb;

}  /* ses_new_scb */


/********************************************************************
* FUNCTION ses_new_dummy_scb
*
* Create a new dummy session control block
*
* INPUTS:
*   none
* RETURNS:
*   pointer to initialized SCB, or NULL if malloc error
*********************************************************************/
ses_cb_t *
    ses_new_dummy_scb (void)
{
    ses_cb_t  *scb;

    scb = ses_new_scb();
    if (!scb) {
        return NULL;
    }

    scb->type = SES_TYP_DUMMY;
    scb->mode = SES_MODE_XML;
    scb->state = SES_ST_IDLE;
    scb->sid = 0;
    scb->username = xml_strdup(NCX_DEF_SUPERUSER);

    return scb;

}  /* ses_new_dummy_scb */


/********************************************************************
* FUNCTION ses_free_scb
*
* Free a session control block
*
* INPUTS:
*   scb == session control block to free
* RETURNS:
*   none
*********************************************************************/
void ses_free_scb (ses_cb_t *scb)
{
    ses_msg_t *msg;
    ses_msg_buff_t *buff;

    assert( scb && "scb is NULL" );

    if (scb->start_time) {
        m__free(scb->start_time);
    }

    if (scb->username) {
        m__free(scb->username);
    }

    if (scb->peeraddr) {
        m__free(scb->peeraddr);
    }

    if (scb->reader) {
        xml_free_reader(scb->reader);
    }

    if (scb->fd) {
        close(scb->fd);
    }

    if (scb->fp) {
        fclose(scb->fp);
    }

    while (!dlq_empty(&scb->msgQ)) {
        msg = (ses_msg_t *)dlq_deque(&scb->msgQ);
        ses_msg_free_msg(scb, msg);
    }

    if (scb->outbuff) {
        ses_msg_free_buff(scb, scb->outbuff);
    }

    while (!dlq_empty(&scb->outQ)) {
        buff = (ses_msg_buff_t *)dlq_deque(&scb->outQ);
        ses_msg_free_buff(scb, buff);
    }

    /* the freeQ must be cleared after the outQ because
     * the normal free buffer action is to move it to
     * the scb->freeQ
     */
    while (!dlq_empty(&scb->freeQ)) {
        buff = (ses_msg_buff_t *)dlq_deque(&scb->freeQ);
        ses_msg_free_buff(scb, buff);
    }

    if (scb->readbuff != NULL) {
        m__free(scb->readbuff);
    }

    if (scb->buffcnt) {
        log_error("\nsession %d terminated with %d buffers",
                  scb->sid, scb->buffcnt);
    }

    /* the mgrcb must be cleaned before this function is called */

    m__free(scb);

}  /* ses_free_scb */


/********************************************************************
* FUNCTION ses_putchar
*
* Write one char to the session, without any translation
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* NO CHARS ARE ACTUALLY WRITTEN TO A REAL SESSION!!!
* The 'output ready' indicator will be set and the session
* queued in the outreadyQ.  Non-blocking IO functions
* will send the data when the connection allows.
*
* INPUTS:
*   scb == session control block to start msg 
*   ch = xmlChar to write, cast as uint32 to avoid compiler warnings
*
*********************************************************************/
void
    ses_putchar (ses_cb_t *scb,
                 uint32    ch)
{
    ses_msg_buff_t *buff;
    status_t res;

    if (scb->fd) {
        /* Normal NETCONF session mode: */
        res = NO_ERR;
        if (scb->outbuff == NULL) {
            res = ses_msg_new_buff(scb, TRUE, &scb->outbuff);
        }
        if (scb->outbuff != NULL) {
            buff = scb->outbuff;
            res = ses_msg_write_buff(scb, buff, ch);
            if (res == ERR_BUFF_OVFL) {
                res = ses_msg_new_output_buff(scb);
                if (res == NO_ERR) {
                    buff = scb->outbuff;
                    res = ses_msg_write_buff(scb, buff, ch);
                }
            }
        } else {
            res = ERR_NCX_OPERATION_FAILED;
        }

        if (res == NO_ERR) {
            scb->stats.out_bytes++;
            totals.stats.out_bytes++;
        }
    } else if (scb->fp) {
        /* debug session, sending output to a file */
        fputc((int)ch, scb->fp);
    } else {
        /* debug session, sending output to the screen */
        putchar((int)ch);
    }

    if (ch=='\n') {
        scb->stats.out_line = 0;
    } else {
        scb->stats.out_line++;
    }

}  /* ses_putchar */


/********************************************************************
* FUNCTION ses_putstr
*
* Write a zero-terminated string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*
*********************************************************************/
void
    ses_putstr (ses_cb_t *scb,
                const xmlChar *str)
{
    while (*str) {
        ses_putchar(scb, *str++);
    }

}  /* ses_putstr */


/********************************************************************
* FUNCTION ses_putstr_indent
*
* Write a zero-terminated content string to the session
* with indentation
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
void
    ses_putstr_indent (ses_cb_t *scb,
                       const xmlChar *str,
                       int32 indent)
{
    ses_indent(scb, indent);
    while (*str) {
        if (*str == '\n') {
            if (indent < 0) {
                ses_putchar(scb, *str++);
            } else {
                ses_indent(scb, indent);
                str++;
            }
        } else {
            ses_putchar(scb, *str++);
        }
    }
}  /* ses_putstr_indent */


/********************************************************************
* FUNCTION ses_putcstr
*
* write XML element safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
void
    ses_putcstr (ses_cb_t *scb,
                 const xmlChar *str,
                 int32 indent)
{
    while (*str) {
        if (*str == '<') {
            ses_putstr(scb, LTSTR);
            str++;
        } else if (*str == '>') {
            ses_putstr(scb, GTSTR);
            str++;
        } else if (*str == '&') {
            ses_putstr(scb, AMPSTR);
            str++;
        } else if ((scb->mode == SES_MODE_XMLDOC
                    || scb->mode == SES_MODE_TEXT) && *str == '\n') {
            if (indent < 0) {
                ses_putchar(scb, *str++);
            } else {
                ses_indent(scb, indent);
                str++;
            }
        } else {
            ses_putchar(scb, *str++);
        }
    }
}  /* ses_putcstr */


/********************************************************************
* FUNCTION ses_puhcstr
*
* write HTML element safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*
*********************************************************************/
void
    ses_puthstr (ses_cb_t *scb,
                 const xmlChar *str)
{
    while (*str) {
        if (*str == '<') {
            ses_putstr(scb, LTSTR);
            str++;
        } else if (*str == '>') {
            ses_putstr(scb, GTSTR);
            str++;
        } else if (*str == '&') {
            ses_putstr(scb, AMPSTR);
            str++;
        } else {
            ses_putchar(scb, *str++);
        }
    }
}  /* ses_puthstr */


/********************************************************************
* FUNCTION ses_putcchar
*
* Write one content char to the session, with translation as needed
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* NO CHARS ARE ACTUALLY WRITTEN TO A REAL SESSION!!!
* The 'output ready' indicator will be set and the session
* queued in the outreadyQ.  Non-blocking IO functions
* will send the data when the connection allows.
*
* INPUTS:
*   scb == session control block to write 
*   ch = xmlChar to write, cast as uint32 to avoid compiler warnings
*
*********************************************************************/
void
    ses_putcchar (ses_cb_t *scb,
                  uint32    ch)
{
    if (ch) {
        if (ch == '<') {
            ses_putstr(scb, LTSTR);
        } else if (ch == '>') {
            ses_putstr(scb, GTSTR);
        } else if (ch == '&') {
            ses_putstr(scb, AMPSTR);
        } else if ((scb->mode == SES_MODE_XMLDOC
                    || scb->mode == SES_MODE_TEXT) && 
                   ch == '\n') {
            int32 indent = ses_indent_count(scb);
            if (indent < 0) {
                ses_putchar(scb, ch);
            } else {
                ses_indent(scb, indent);
            }
        } else {
            ses_putchar(scb, ch);
        }
    }
}  /* ses_putcchar */


/********************************************************************
* FUNCTION ses_putastr
*
* write XML attribute safe content string
* Write a zero-terminated attribute content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL XML CHARS ARE CONVERTED TO CHAR ENTITIES
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
void
    ses_putastr (ses_cb_t *scb,
                 const xmlChar *str,
                 int32 indent)
{
    while (*str) {
        if (*str == '<') {
            ses_putstr(scb, LTSTR);
            str++;
        } else if (*str == '>') {
            ses_putstr(scb, GTSTR);
            str++;
        } else if (*str == '&') {
            ses_putstr(scb, AMPSTR);
            str++;
        } else if (*str == '"') {
            ses_putstr(scb, QSTR);
            str++;
        } else if (*str == '\n') {
            if (scb->mode == SES_MODE_XMLDOC || 
                scb->mode == SES_MODE_TEXT) {
                if (indent < 0) {
                    ses_putchar(scb, *str++);
                } else {
                    ses_indent(scb, indent);
                    str++;
                }
            } else {
                put_char_entity(scb, *str++);
            }
        } else if (isspace(*str)) {
            put_char_entity(scb, *str++);
        } else {
            ses_putchar(scb, *str++);
        }
    }
}  /* ses_putastr */


/********************************************************************
* FUNCTION ses_putjstr
*
* write JSON safe content string
* Write a zero-terminated element content string to the session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMTERS TO SAVE TIME
* EXCEPT THAT ILLEGAL JSON CHARS ARE CONVERTED TO ESCAPED CHARS
*
* INPUTS:
*   scb == session control block to start msg 
*   str == string to write
*   indent == current indent amount
*
*********************************************************************/
void
    ses_putjstr (ses_cb_t *scb,
                 const xmlChar *str,
                 int32 indent)
{
    ses_indent(scb, indent);
    while (*str) {
        switch (*str) {
        case '"':
            ses_putchar(scb, '\\');
            ses_putchar(scb, '"');
            break;
        case '\\':
            ses_putchar(scb, '\\');
            ses_putchar(scb, '\\');
            break;
        case '/':
            ses_putchar(scb, '\\');
            ses_putchar(scb, '/');
            break;
        case '\b':
            ses_putchar(scb, '\\');
            ses_putchar(scb, 'b');
            break;
        case '\f':
            ses_putchar(scb, '\\');
            ses_putchar(scb, 'f');
            break;
        case '\n':
            ses_putchar(scb, '\\');
            ses_putchar(scb, 'n');
            break;
        case '\r':
            ses_putchar(scb, '\\');
            ses_putchar(scb, 'r');
            break;
        case '\t':
            ses_putchar(scb, '\\');
            ses_putchar(scb, 't');
            break;
        default:
            ses_putchar(scb, *str);
        }
        ++str;
    }
}  /* ses_putjstr */


/********************************************************************
* FUNCTION ses_indent
*
* Write the proper newline + indentation to the specified session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to start msg 
*   indent == number of chars to indent after a newline
*             will be ignored if indent is turned off
*             in the agent profile
*          == -1 means no newline or indent
*          == 0 means just newline
*
*********************************************************************/
void
    ses_indent (ses_cb_t *scb,
                int32 indent)
{
    int32 i;

    if (indent < 0) {
        return;
    }

    /* set limit on indentation in case of bug */
    indent = min(indent, 255);
    ses_putchar(scb, '\n');
    for (i=0; i<indent; i++) {
        ses_putchar(scb, ' ');
    }

}  /* ses_indent */

/********************************************************************
* FUNCTION ses_indent_count
*
* Get the indent count for this session
*
* THIS FUNCTION DOES NOT CHECK ANY PARAMETERS TO SAVE TIME
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   indent value for the session
*********************************************************************/
int32
    ses_indent_count (const ses_cb_t *scb)
{
    return scb->indent;

} /* ses_indent_count */


/********************************************************************
* FUNCTION ses_set_indent
*
* Set the indent count for this session
*
* INPUTS:
*   scb == session control block to check
*   indent == value to use (may get adjusted)
*
*********************************************************************/
void
    ses_set_indent (ses_cb_t *scb,
                    int32 indent)
{
#ifdef DEBUG
    if (scb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (indent < 0) {
        indent = 0;
    } else if (indent > 9) {
        indent = 9;
    }
    scb->indent = indent;

} /* ses_set_indent */


/********************************************************************
* FUNCTION ses_set_mode
*
* Set the output mode for the specified session
*
* INPUTS:
*   scb == session control block to set
*   mode == new mode value
* RETURNS:
*   none
*********************************************************************/
void ses_set_mode (ses_cb_t *scb, ses_mode_t mode)
{
    assert( scb && "scb is NULL" );

    scb->mode = mode;
} /* ses_set_mode */


/********************************************************************
* FUNCTION ses_get_mode
*
* Get the output mode for the specified session
*
* INPUTS:
*   scb == session control block to get
*
* RETURNS:
*   session mode value
*********************************************************************/
ses_mode_t ses_get_mode (ses_cb_t *scb)
{
    assert( scb && "scb is NULL" );

    return scb->mode;
} /* ses_get_mode */


/********************************************************************
* FUNCTION ses_start_msg
*
* Start a new outbound message on the specified session
*
* INPUTS:
*   scb == session control block to start msg 
*
* RETURNS:
*   status
*********************************************************************/
status_t ses_start_msg (ses_cb_t *scb)
{
    assert( scb && "scb is NULL" );

    /* check if this session will allow a msg to start now */
    if (scb->state >= SES_ST_SHUTDOWN) {
        return ERR_NCX_OPERATION_FAILED;
    }

    /* Generate Start of XML Message Directive */
    ses_putstr(scb, XML_START_MSG);

    return NO_ERR;

}  /* ses_start_msg */


/********************************************************************
* FUNCTION ses_finish_msg
*
* Finish an outbound message on the specified session
*
* INPUTS:
*   scb == session control block to finish msg 
* RETURNS:
*   none
*********************************************************************/
void ses_finish_msg (ses_cb_t *scb)
{
    assert( scb && "scb is NULL" );

    /* add the NETCONF EOM marker */
    if (scb->transport==SES_TRANSPORT_SSH ||
        scb->transport==SES_TRANSPORT_TCP) {
        if (scb->framing11) {
            scb->outbuff->islast = TRUE;
        } else {
            ses_putstr(scb, (const xmlChar *)NC_SSH_END);
        }
    }

    /* add a final newline when writing to a file
     * but never if writing to a socket or STDOUT
     */
    if (scb->fd == 0 && scb->fp != stdout) {
        ses_putchar(scb, '\n');
    }

    /* queue for output if not done so already */
    if (scb->type != SES_TYP_DUMMY) {
        ses_msg_finish_outmsg(scb);
    }

}  /* ses_finish_msg */


/********************************************************************
* FUNCTION ses_read_cb
*
* The IO input front-end for the xmlTextReader parser read fn
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.
*
* The underlying transport (accept_buffer*) has already stripped
* all the framing characters from the incoming stream
*
* Uses a complex state machine which does not assume that the
* input from the network is going to arrive in well-formed chunks.
* It has to be treated as a byte stream (SOCK_STREAM).
*
* Does not remove char entities or any XML, just the SSH framing chars
*
* INPUTS:
*   context == scb pointer for the session to read
*   buffer == char buffer to fill
*   len == length of the buffer
*
* RETURNS:
*   number of bytes read into the buffer
*   -1     indicates error and EOF
*********************************************************************/
int
    ses_read_cb (void *context,
                 char *buffer,
                 int len)
{
    ses_cb_t         *scb;
    ses_msg_t        *msg;
    ses_msg_buff_t   *buff;
    int               retlen;
    boolean           done;

    if (len == 0) {
        return 0;
    }

    scb = (ses_cb_t *)context;
    if (scb->state >= SES_ST_SHUTDOWN_REQ) {
        return -1;
    }

    msg = (ses_msg_t *)dlq_firstEntry(&scb->msgQ);
    if (msg == NULL) {
        return 0;
    }

    /* hack: the xmlTextReader parser will request 4 bytes
     * to test the message and a buffer size of 4096
     * if the request is for real.  The 4096 constant
     * is not hard-wired into the code, though.
     */
    if (len == 4) {
        strncpy(buffer, "\n<?x", len);
        return 4;
    }

    retlen = 0;

    /* check if this is the first read for this message */
    buff = msg->curbuff;
    if (buff == NULL) {
        buff = (ses_msg_buff_t *)dlq_firstEntry(&msg->buffQ);
        if (buff == NULL) {
            return 0;
        } else {
            buff->buffpos = buff->buffstart;
            msg->curbuff = buff;
        }
    }

    /* check current buffer end has been reached */
    if (buff->buffpos == buff->bufflen) {
        buff = (ses_msg_buff_t *)dlq_nextEntry(buff);
        if (buff == NULL) {
            return 0;
        } else {
            buff->buffpos = buff->buffstart;
            msg->curbuff = buff;
        }
    }

    handle_prolog_state(msg, buffer, len, buff, buff->bufflen, &retlen);

    /* start transferring bytes to the return buffer */
    done = FALSE;
    while (!done) {

        buffer[retlen++] = (char)buff->buff[buff->buffpos++];

        /* check xmlreader buffer full */
        if (retlen == len) {
            done = TRUE;
            continue;
        }

        /* check current buffer end has been reached */
        if (buff->buffpos == buff->bufflen) {
            buff = (ses_msg_buff_t *)dlq_nextEntry(buff);
            if (buff == NULL) {
                done = TRUE;
                continue;
            } else {
                buff->buffpos = buff->buffstart;
                msg->curbuff = buff;
                handle_prolog_state(msg, buffer, len, buff, 
                                    buff->bufflen, &retlen);
            }
        }
    }

#ifdef SES_DEBUG_XML_TRACE
    ncx_write_tracefile(buffer, retlen);
#endif

    return retlen;

}  /* ses_read_cb */


/********************************************************************
* FUNCTION ses_accept_input
*
* The IO input handler for the ncxserver loop
*
* Need to separate the input stream into separate XML instance
* documents and reset the xmlTextReader each time a new document
* is encountered.  For SSH, also need to detect the EOM flag
* and remove it + control input to the reader.
*
* This function breaks the byte stream into ses_msg_t structs
* that get queued on the session's msgQ
*
* INPUTS:
*   scb == session control block to accept input for
*
* RETURNS:
*   status
*********************************************************************/
status_t
    ses_accept_input (ses_cb_t *scb)
{
    status_t        res;
    ssize_t         ret;
    boolean         done, readdone, erragain;
    uint32          readtries;

#ifdef DEBUG
    if (scb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

#ifdef SES_DEBUG
    if (LOGDEBUG3) {
        log_debug3("\nses_accept_input on session %d", scb->sid);
    }
#endif

    res = NO_ERR;
    done = FALSE;
    readdone = FALSE;
    ret = 0;

    while (!done) {
        if (scb->state >= SES_ST_SHUTDOWN_REQ) {
            return ERR_NCX_SESSION_CLOSED;
        }

        /* loop until 1 buffer is read OK or retry count hit */
        readtries = 0;
        readdone = FALSE;
        while (!readdone && res == NO_ERR) {
            /* check read retry count */
            if (readtries++ == MAX_READ_TRIES) {
                readdone = TRUE;
                res = ERR_NCX_SKIPPED;
                continue;
            }

            /* read data into the new buffer */
            if (scb->rdfn) {
                erragain = FALSE;
                ret = (*scb->rdfn)(scb, 
                                   (char *)scb->readbuff, 
                                   scb->readbuffsize,
                                   &erragain);
            } else {
                ret = read(scb->fd, 
                           scb->readbuff, 
                           scb->readbuffsize);
            }

            if (ret < 0) {
                /* some error or EAGAIN */
                res = NO_ERR;

                if (scb->rdfn) {
                    if (!erragain) {
                        res = ERR_NCX_READ_FAILED;
                    }
                } else {
                    if (errno != EAGAIN) {
                        res = ERR_NCX_READ_FAILED;
                    }
                }

                if (res == ERR_NCX_READ_FAILED) {
                    log_error("\nError: ses read failed on session %d (%s)", 
                              scb->sid, 
                              strerror(errno));
                }
            } else {
                /* channel closed or returned byte count */
                res = NO_ERR;
                readdone = TRUE;
            }
        }

        if (res != NO_ERR) {
            if (res == ERR_NCX_SKIPPED) {
                res = NO_ERR;
            }
            return res;
        }

        /* read was done if we reach here */
        if (ret == 0) {
            /* session closed by remote peer */
            if (LOGINFO) {
                log_info("\nses: session %d shut by remote peer", 
                         scb->sid);
            }
            return ERR_NCX_SESSION_CLOSED;
        } else {
            if (LOGDEBUG2) {
                log_debug2("\nses read OK (%d) on session %d", 
                           ret, 
                           scb->sid);
            }

            /* increment session byte counters */
            scb->stats.in_bytes += (uint32)ret;
            totals.stats.in_bytes += (uint32)ret;

            /* adjust length if read was z-terminated
             * should not be needed; zero not a valid
             * char to appear in an XML document
             */
            if (scb->readbuff[ret - 1] == '\0') {
                /* don't barf if the client sends a 
                 * zero-terminated string instead of
                 * just the contents of the string
                 */
                if (LOGDEBUG3) {
                    log_debug3("\nses: dropping zero byte at EObuff");
                }
                ret--;
            }

            /* pass the read buffer in 1 of these functions
             * to handle the buffer framing
             */
            if (ses_get_protocol(scb) == NCX_PROTO_NETCONF11) {
                res = accept_buffer_ssh_v11(scb, ret);
            } else {
                res = accept_buffer_ssh_v10(scb, ret);
            }

            if (res != NO_ERR || 
                ((uint32)ret < scb->readbuffsize) || 
                scb->rdfn == NULL) {
#ifdef SES_DEBUG_TRACE
                if (LOGDEBUG3) {
                    log_debug3("\nses: bail exit %u:%u (%s)", 
                               ret, 
                               scb->readbuffsize,
                               get_error_string(res));
                }
#endif
                done = TRUE;
            } /* else the SSH2 channel probably has more bytes to read */
        }
    }
    return res;

}  /* ses_accept_input */


/********************************************************************
* FUNCTION ses_state_name
*
* Get the name of a session state from the enum value
*
* INPUTS:
*   state == session state enum value
*
* RETURNS:
*   staing corresponding to the state name
*********************************************************************/
const xmlChar *
    ses_state_name (ses_state_t state)
{
    switch (state) {
    case SES_ST_NONE:
        return (const xmlChar *)"none";
    case SES_ST_INIT:
        return (const xmlChar *)"init";
    case SES_ST_HELLO_WAIT:
        return (const xmlChar *)"hello-wait";
    case SES_ST_IDLE:
        return (const xmlChar *)"idle";
    case SES_ST_IN_MSG:
        return (const xmlChar *)"in-msg";
    case SES_ST_SHUTDOWN_REQ:
        return (const xmlChar *)"shutdown-requested";
    case SES_ST_SHUTDOWN:
        return (const xmlChar *)"shutdown";
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return (const xmlChar *)"--";   
    }
    /*NOTREACHED*/

} /* ses_state_name */


/********************************************************************
* FUNCTION ses_withdef
*
* Get the with-defaults value for this session
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   with-defaults value for the session
*********************************************************************/
ncx_withdefaults_t
    ses_withdef (const ses_cb_t *scb)
{
#ifdef DEBUG
    if (scb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_WITHDEF_NONE;
    }
#endif

    return scb->withdef;

} /* ses_withdef */


/********************************************************************
* FUNCTION ses_line_left
*
* Get the number of bytes that can be added to the current line
* before the session linesize limit is reached
*
* INPUTS:
*   scb == session control block to check
*
* RETURNS:
*   number of bytes left, or zero if limit already reached
*********************************************************************/
uint32
    ses_line_left (const ses_cb_t *scb)
{
    if (scb->stats.out_line >= scb->linesize) {
        return 0;
    } else {
        return scb->linesize - scb->stats.out_line;
    }

} /* ses_line_left */


/********************************************************************
* FUNCTION ses_put_extern
* 
*  write the contents of a file to the session
*
* INPUTS:
*    scb == session to write
*    fspec == filespec to write
*
*********************************************************************/
void
    ses_put_extern (ses_cb_t *scb,
                    const xmlChar *fname)
{
    FILE               *fil;
    boolean             done;
    int                 ch;

    fil = fopen((const char *)fname, "r");
    if (!fil) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    } 

    done = FALSE;
    while (!done) {
        ch = fgetc(fil);
        if (ch == EOF) {
            fclose(fil);
            done = TRUE;
        } else {
            ses_putchar(scb, (uint32)ch);
        }
    }

} /* ses_put_extern */


/********************************************************************
* FUNCTION ses_get_total_stats
* 
*  Get a r/w pointer to the the session totals stats
*
* RETURNS:
*  pointer to the global session stats struct 
*********************************************************************/
ses_total_stats_t *
    ses_get_total_stats (void)
{
    return &totals;
} /* ses_get_total_stats */


/********************************************************************
* FUNCTION ses_get_transport_name
* 
*  Get the name of the transport for a given enum value
*
* INPUTS:
*   transport == ses_transport_t enum value
*
* RETURNS:
*   pointer to the string value for the specified enum
*********************************************************************/
const xmlChar *
    ses_get_transport_name (ses_transport_t transport)
{
    /* needs to match netconf-state DM values */
    switch (transport) {
    case SES_TRANSPORT_NONE:
        return (const xmlChar *)"none";
    case SES_TRANSPORT_SSH:
        return (const xmlChar *)"netconf-ssh";
    case SES_TRANSPORT_BEEP:
        return (const xmlChar *)"netconf-beep";
    case SES_TRANSPORT_SOAP:
        return (const xmlChar *)"netconf-soap-over-https";
    case SES_TRANSPORT_SOAPBEEP:
        return (const xmlChar *)"netconf-soap-over-beep";
    case SES_TRANSPORT_TLS:
        return (const xmlChar *)"netconf-tls";
    case SES_TRANSPORT_TCP:
        return (const xmlChar *)"netconf-tcp";
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
        return (const xmlChar *)"none";
    }

} /* ses_get_transport_name */


/********************************************************************
* FUNCTION ses_set_xml_nons
* 
*  force xmlns attributes to be skipped in XML mode
*
* INPUTS:
*    scb == session to set
*
*********************************************************************/
void
    ses_set_xml_nons (ses_cb_t *scb)
{
    scb->noxmlns = TRUE;

} /* ses_set_xml_nons */


/********************************************************************
* FUNCTION ses_get_xml_nons
* 
*  force xmlns attributes to be skipped in XML mode
*
* INPUTS:
*    scb == session to get
*
* RETURNS:
*   TRUE if no xmlns attributes set
*   FALSE if OK to use xmlns attributes
*********************************************************************/
boolean
    ses_get_xml_nons (const ses_cb_t *scb)
{
    return scb->noxmlns;

} /* ses_get_xml_nons */


/********************************************************************
* FUNCTION ses_set_protocol
* 
*  set the NETCONF protocol version in use
*
* INPUTS:
*    scb == session to set
*    proto == protocol to set
* RETURNS:
*    status
*********************************************************************/
status_t
    ses_set_protocol (ses_cb_t *scb,
                      ncx_protocol_t proto)
{
#ifdef DEBUG
    if (scb == NULL) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif
    if (proto == NCX_PROTO_NONE) {
        return ERR_NCX_INVALID_VALUE;
    }
    if (scb->protocol != NCX_PROTO_NONE) {
        return ERR_NCX_DUP_ENTRY;
    }
    scb->protocol = proto;
    if (scb->transport == SES_TRANSPORT_SSH &&
        proto == NCX_PROTO_NETCONF11) {
        scb->framing11 = TRUE;
    }

    if (scb->outbuff != NULL && scb->framing11) {
        if (scb->outbuff->bufflen != 0) {
            SET_ERROR(ERR_INTERNAL_VAL);
        }
        ses_msg_init_buff(scb, TRUE, scb->outbuff);
    }

    return NO_ERR;

}  /* ses_set_protocol */


/********************************************************************
* FUNCTION ses_get_protocol
* 
*  Get the NETCONF protocol set (or unset) for this session
*
* INPUTS:
*    scb == session to get
*
* RETURNS:
*   protocol enumeration in use
*********************************************************************/
ncx_protocol_t
    ses_get_protocol (const ses_cb_t *scb)
{
#ifdef DEBUG
    if (scb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NCX_PROTO_NONE;
    }
#endif
    return scb->protocol;

}  /* ses_get_protocol */


/********************************************************************
* FUNCTION ses_set_protocols_requested
* 
*  set the NETCONF protocol versions requested
*
* INPUTS:
*    scb == session to set
*    proto == protocol to set
* RETURNS:
*    status
*********************************************************************/
void
    ses_set_protocols_requested (ses_cb_t *scb,
                                 ncx_protocol_t proto)
{
#ifdef DEBUG
    if (scb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif
    switch (proto) {
    case NCX_PROTO_NETCONF10:
        scb->protocols_requested |= NCX_FL_PROTO_NETCONF10;
        break;
    case NCX_PROTO_NETCONF11:
        scb->protocols_requested |= NCX_FL_PROTO_NETCONF11;
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }

}  /* ses_set_protocols_requested */


/********************************************************************
* FUNCTION ses_protocol_requested
* 
*  check if the NETCONF protocol version was requested
*
* INPUTS:
*    scb == session to check
*    proto == protocol to check
* RETURNS:
*    TRUE is requested; FALSE otherwise
*********************************************************************/
boolean
    ses_protocol_requested (ses_cb_t *scb,
                            ncx_protocol_t proto)
{
    boolean ret = FALSE;

#ifdef DEBUG
    if (scb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif
    switch (proto) {
    case NCX_PROTO_NETCONF10:
        if (scb->protocols_requested & NCX_FL_PROTO_NETCONF10) {
            ret = TRUE;
        }
        break;
    case NCX_PROTO_NETCONF11:
        if (scb->protocols_requested & NCX_FL_PROTO_NETCONF11) {
            ret = TRUE;
        }
        break;
    default:
        SET_ERROR(ERR_INTERNAL_VAL);
    }
    return ret;

}  /* ses_protocol_requested */


/* END file ses.c */

