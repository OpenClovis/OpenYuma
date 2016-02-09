#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <xmlstring.h>

#include "ncx.h"
#include "ncx_list.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncxconst.h"
#include "obj.h"
#include "val.h"
#include "val_parse.h"
#include "val_util.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "xml_rd.h"
#include "xpath.h"
#include "xpath1.h"
#include "xpath_yang.h"
#include "yangconst.h"
#include "val_get_leafref_targval.h"

val_value_t* val_get_leafref_targval(val_value_t *leafref_val, val_value_t *root_val)
{
    status_t res;
    val_value_t* target_val =NULL;

    xpath_resnode_t *resnode;
    xpath_result_t *result =
            xpath1_eval_expr(leafref_val->xpathpcb, leafref_val, root_val, FALSE /* logerrors */, FALSE /* non-configonly */, &res);
    assert(result);
    for (resnode = (xpath_resnode_t *)dlq_firstEntry(&result->r.nodeQ);
         resnode != NULL;
         resnode = (xpath_resnode_t *)dlq_nextEntry(resnode)) {
        char* target_str;
        val_value_t* val;
        val = resnode->node.valptr;
        target_str = val_make_sprintf_string(val);
        if(0==strcmp(target_str,VAL_STRING(leafref_val))) {
            free(target_str);
            target_val = val;
            break;
        }
        free(target_str);
    }
    free(result);
    return target_val;
}
