
#ifndef _H_agt_plock
#define _H_agt_plock
/* 

 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * All Rights Reserved.
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *

    module ietf-netconf-partial-lock
    revision 2009-10-19

    namespace urn:ietf:params:xml:ns:netconf:partial-lock:1.0
    organization IETF Network Configuration (netconf) Working Group

 */

#include <xmlstring.h>

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_ncxtypes
#include "ncxtypes.h"
#endif

#ifndef _H_status
#include "status.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define y_ietf_netconf_partial_lock_M_ietf_netconf_partial_lock \
    (const xmlChar *)"ietf-netconf-partial-lock"
#define y_ietf_netconf_partial_lock_R_ietf_netconf_partial_lock \
    (const xmlChar *)"2009-10-19"

#define y_ietf_netconf_partial_lock_N_lock_id (const xmlChar *)"lock-id"
#define y_ietf_netconf_partial_lock_N_locked_node (const xmlChar *)"locked-node"
#define y_ietf_netconf_partial_lock_N_partial_lock \
    (const xmlChar *)"partial-lock"
#define y_ietf_netconf_partial_lock_N_partial_unlock \
    (const xmlChar *)"partial-unlock"
#define y_ietf_netconf_partial_lock_N_select (const xmlChar *)"select"

/* leaf-list /partial-lock/input/select */
typedef struct y_ietf_netconf_partial_lock_T_partial_lock_input_select_ {
    dlq_hdr_t qhdr;
    xmlChar *myselect;
} y_ietf_netconf_partial_lock_T_partial_lock_input_select;

/* container /partial-lock/input */
typedef struct y_ietf_netconf_partial_lock_T_partial_lock_input_ {
    dlq_hdr_t myselect;
} y_ietf_netconf_partial_lock_T_partial_lock_input;

/* leaf-list /partial-lock/output/locked-node */
typedef struct y_ietf_netconf_partial_lock_T_partial_lock_output_locked_node_ {
    dlq_hdr_t qhdr;
    xmlChar *locked_node;
} y_ietf_netconf_partial_lock_T_partial_lock_output_locked_node;

/* container /partial-lock/output */
typedef struct y_ietf_netconf_partial_lock_T_partial_lock_output_ {
    uint32 lock_id;
    dlq_hdr_t locked_node;
} y_ietf_netconf_partial_lock_T_partial_lock_output;

/* container /partial-unlock/input */
typedef struct y_ietf_netconf_partial_lock_T_partial_unlock_input_ {
    uint32 lock_id;
} y_ietf_netconf_partial_lock_T_partial_unlock_input;
/* ietf-netconf-partial-lock module init 1 */
extern status_t
    y_ietf_netconf_partial_lock_init (
        const xmlChar *modname,
        const xmlChar *revision);

/* ietf-netconf-partial-lock module init 2 */
extern status_t
    y_ietf_netconf_partial_lock_init2 (void);

/* ietf-netconf-partial-lock module cleanup */
extern void
    y_ietf_netconf_partial_lock_cleanup (void);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif
