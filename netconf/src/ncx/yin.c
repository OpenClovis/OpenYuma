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
/*  FILE: yin.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
23feb08      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif

#ifndef _H_yin
#include "yin.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


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
static yin_mapping_t yinmap[] = {
    { YANG_K_ANYXML, YANG_K_NAME, FALSE },
    { YANG_K_ARGUMENT, YANG_K_NAME, FALSE }, 
    { YANG_K_AUGMENT, YANG_K_TARGET_NODE, FALSE },
    { YANG_K_BASE, YANG_K_NAME, FALSE },
    { YANG_K_BELONGS_TO, YANG_K_MODULE, FALSE},
    { YANG_K_BIT, YANG_K_NAME, FALSE},
    { YANG_K_CASE, YANG_K_NAME, FALSE },
    { YANG_K_CHOICE, YANG_K_NAME, FALSE },
    { YANG_K_CONFIG, YANG_K_VALUE, FALSE },
    { YANG_K_CONTACT, YANG_K_TEXT, TRUE },
    { YANG_K_CONTAINER, YANG_K_NAME, FALSE },
    { YANG_K_DEFAULT, YANG_K_VALUE, FALSE },
    { YANG_K_DESCRIPTION, YANG_K_TEXT, TRUE },
    { YANG_K_DEVIATE, YANG_K_VALUE, FALSE },
    { YANG_K_DEVIATION, YANG_K_TARGET_NODE, FALSE },
    { YANG_K_ENUM, YANG_K_NAME, FALSE },
    { YANG_K_ERROR_APP_TAG, YANG_K_VALUE, FALSE },
    { YANG_K_ERROR_MESSAGE, YANG_K_VALUE, TRUE },
    { YANG_K_EXTENSION, YANG_K_NAME, FALSE },
    { YANG_K_FEATURE, YANG_K_NAME, FALSE },
    { YANG_K_FRACTION_DIGITS, YANG_K_VALUE, FALSE },
    { YANG_K_GROUPING, YANG_K_NAME, FALSE },
    { YANG_K_IDENTITY, YANG_K_NAME, FALSE },
    { YANG_K_IF_FEATURE, YANG_K_NAME, FALSE },
    { YANG_K_IMPORT, YANG_K_MODULE, FALSE },
    { YANG_K_INCLUDE, YANG_K_MODULE, FALSE },
    { YANG_K_INPUT, NULL, FALSE },
    { YANG_K_KEY, YANG_K_VALUE, FALSE },
    { YANG_K_LEAF, YANG_K_NAME, FALSE },
    { YANG_K_LEAF_LIST, YANG_K_NAME, FALSE },
    { YANG_K_LENGTH, YANG_K_VALUE, FALSE },
    { YANG_K_LIST, YANG_K_NAME, FALSE },
    { YANG_K_MANDATORY, YANG_K_VALUE, FALSE },
    { YANG_K_MAX_ELEMENTS, YANG_K_VALUE, FALSE },
    { YANG_K_MIN_ELEMENTS, YANG_K_VALUE, FALSE },
    { YANG_K_MODULE, YANG_K_NAME, FALSE },
    { YANG_K_MUST, YANG_K_CONDITION, FALSE },
    { YANG_K_NAMESPACE, YANG_K_URI, FALSE },
    { YANG_K_NOTIFICATION, YANG_K_NAME, FALSE },
    { YANG_K_ORDERED_BY, YANG_K_VALUE, FALSE },
    { YANG_K_ORGANIZATION, YANG_K_TEXT, TRUE },
    { YANG_K_OUTPUT, NULL, FALSE },
    { YANG_K_PATH, YANG_K_VALUE, FALSE },
    { YANG_K_PATTERN, YANG_K_VALUE, FALSE },
    { YANG_K_POSITION, YANG_K_VALUE, FALSE },
    { YANG_K_PREFIX, YANG_K_VALUE, FALSE },
    { YANG_K_PRESENCE, YANG_K_VALUE, FALSE },
    { YANG_K_RANGE, YANG_K_VALUE, FALSE },
    { YANG_K_REFERENCE, YANG_K_TEXT, TRUE },
    { YANG_K_REFINE, YANG_K_TARGET_NODE, FALSE },
    { YANG_K_REQUIRE_INSTANCE, YANG_K_VALUE, FALSE },
    { YANG_K_REVISION, YANG_K_DATE, FALSE },
    { YANG_K_REVISION_DATE, YANG_K_DATE, FALSE },
    { YANG_K_RPC, YANG_K_NAME, FALSE },
    { YANG_K_STATUS, YANG_K_VALUE, FALSE },
    { YANG_K_SUBMODULE, YANG_K_NAME, FALSE },
    { YANG_K_TYPE, YANG_K_NAME, FALSE },
    { YANG_K_TYPEDEF, YANG_K_NAME, FALSE },
    { YANG_K_UNIQUE, YANG_K_TAG, FALSE },
    { YANG_K_UNITS, YANG_K_NAME, FALSE },
    { YANG_K_USES, YANG_K_NAME, FALSE },
    { YANG_K_VALUE, YANG_K_VALUE, FALSE },
    { YANG_K_WHEN, YANG_K_CONDITION, FALSE },
    { YANG_K_YANG_VERSION, YANG_K_VALUE, FALSE },
    { YANG_K_YIN_ELEMENT, YANG_K_VALUE, FALSE },
    { NULL, NULL, FALSE }
};


/********************************************************************
* FUNCTION find_yin_mapping
* 
* Find a static yin mapping entry
*
* INPUTS:
*   name == keyword name to find
*
* RETURNS:
*   pointer to found entry, NULL if none found
*********************************************************************/
const yin_mapping_t *
    yin_find_mapping (const xmlChar *name)
{
    const yin_mapping_t  *mapping;
    int                   i;

    i = 0;
    for (mapping = &yinmap[i];
         mapping != NULL && mapping->keyword != NULL;
         mapping = &yinmap[++i]) {
        if (!xml_strcmp(name, mapping->keyword)) {
            return mapping;
        }
    }
    return NULL;
         
}  /* yin_find_mapping */

/* END file yin.c */
