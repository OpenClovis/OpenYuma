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

/*  FILE: agt_commit_complete.c 

*********************************************************************
* P U R P O S E
*********************************************************************
    NETCONF Server Commit Complete callback handler
    This file contains functions to support registering, 
    unregistering and execution of commit complete callbacks. 

*********************************************************************
* C H A N G E	 H I S T O R Y
*********************************************************************

date	     init     comment
----------------------------------------------------------------------
24-oct-11    mp       First draft.
*/

/********************************************************************
* Y U M A    H E A D E R S
*********************************************************************/
#include "agt_commit_complete.h"
#include "dlq.h"
#include "xml_util.h"

/********************************************************************
* S T A N D A R D   H E A D E R S
*********************************************************************/
#include <string.h>
#include <assert.h>

/********************************************************************
* T Y P E S
*********************************************************************/
typedef struct agt_cb_commit_complete_set_t_ {
    dlq_hdr_t                  qhdr;
    xmlChar                   *modname;
    agt_commit_complete_cb_t   callback;
} agt_cb_commit_complete_set_t;
    
/**************    S T A T I C   D A T A ****************************/
static bool initialised = false;
static dlq_hdr_t callbackQ;

/**************    S T A T I C   F U N C T I O N S ******************/

/********************************************************************/
static void free_callback_set( agt_cb_commit_complete_set_t* cbSet )
{
    if ( cbSet->modname )
    {
        m__free( cbSet->modname );
    }
    m__free( cbSet );
}

/********************************************************************/
static agt_cb_commit_complete_set_t* new_callback_set( const xmlChar *modname )
{
    agt_cb_commit_complete_set_t* cbSet = m__getObj( 
            agt_cb_commit_complete_set_t );

    if ( !cbSet )
    {
        return NULL;
    }

    memset( cbSet, 0, sizeof( agt_cb_commit_complete_set_t ) );
    cbSet->modname = xml_strdup( modname );
    if ( !cbSet->modname )
    {
        m__free( cbSet );
        return NULL;
    }

    return cbSet;
}

/********************************************************************/
static agt_cb_commit_complete_set_t* find_callback_set( const xmlChar *modname )
{
    agt_cb_commit_complete_set_t* cbSet;

    for ( cbSet = ( agt_cb_commit_complete_set_t* )dlq_firstEntry( &callbackQ );
          cbSet != NULL;
          cbSet = ( agt_cb_commit_complete_set_t* )dlq_nextEntry( cbSet ) )
    {
        if ( xml_strcmp( modname, cbSet->modname ) )
        {
            return cbSet;
        }
    }

    return NULL;
}

/**************    E X T E R N A L   F U N C T I O N S **************/

/********************************************************************/
void agt_commit_complete_init( void )
{
    if ( !initialised )
    {
        dlq_createSQue( &callbackQ );
        initialised = true;
    }
} /* agt_commit_complete_init */

/********************************************************************/
void agt_commit_complete_cleanup( void )
{
    if ( initialised )
    {
        agt_cb_commit_complete_set_t* cbSet;

        while ( !dlq_empty( &callbackQ ) )
        {
            cbSet = ( agt_cb_commit_complete_set_t* )dlq_deque( &callbackQ );
            free_callback_set( cbSet );
        }
        initialised = false;
    }
} /* agt_commit_complete_cleanup */

/********************************************************************/
status_t agt_commit_complete_register( const xmlChar *modname,
                                       agt_commit_complete_cb_t cb )
{
    assert( modname );

    agt_cb_commit_complete_set_t* cbSet = find_callback_set( modname );

    if ( !cbSet )
    {
        cbSet = new_callback_set( modname );
        if ( !cbSet )
        {
            return ERR_INTERNAL_MEM;
        }

        dlq_enque( cbSet, &callbackQ );
    }

    cbSet->callback = cb;
    return NO_ERR;
}

/********************************************************************/
void agt_commit_complete_unregister( const xmlChar *modname )
{
    assert( modname );

    agt_cb_commit_complete_set_t* cbSet = find_callback_set( modname );

    if ( cbSet )
    {
        dlq_remove( cbSet );
        free_callback_set( cbSet );
    }
}

/********************************************************************/
status_t agt_commit_complete( void )
{
    agt_cb_commit_complete_set_t* cbSet;

    for ( cbSet = ( agt_cb_commit_complete_set_t* )dlq_firstEntry( &callbackQ );
          cbSet != NULL;
          cbSet = ( agt_cb_commit_complete_set_t* )dlq_nextEntry( cbSet ) )
    {
        if ( cbSet->callback )
        {
            status_t res = cbSet->callback();

            if ( NO_ERR != res )
            {
                return res;
            }
        }
    }

    return NO_ERR;
}


/* END file agt_commit_complete.c */

