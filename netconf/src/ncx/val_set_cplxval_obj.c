#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <xmlstring.h>

#include "procdefs.h"
#include "b64.h"
#include "cfg.h"
#include "dlq.h"
#include "getcb.h"
#include "json_wr.h"
#include "log.h"
#include "ncx.h"
#include "ncx_list.h"
#include "ncx_num.h"
#include "ncx_str.h"
#include "ncxconst.h"
#include "obj.h"
#include "ses.h"
#include "tk.h"
#include "typ.h"
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


status_t val_set_cplxval_obj(val_value_t *val, obj_template_t *obj, char* xmlstr)
{
    FILE* fp;
    status_t res;
    val_value_t* tmp_val;

    fp = fmemopen((void*)xmlstr, strlen(xmlstr), "r");
    res = xml_rd_open_file (fp, obj, &tmp_val);
    if(res!=NO_ERR) {
        return NO_ERR;
    }
    val_move_children(tmp_val,val);

    val_free_value(tmp_val);

    fclose(fp);
    return NO_ERR;
}
