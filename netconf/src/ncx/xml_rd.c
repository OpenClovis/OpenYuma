#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "procdefs.h"
#include "dlq.h"
#include "ncx.h"
#include "ncx_num.h"
#include "ncxconst.h"
#include "obj.h"
#include "ses.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "val_parse.h"
#include "xmlns.h"
#include "xml_msg.h"
#include "xml_util.h"
#include "xml_wr.h"
#include "xpath.h"
#include "xpath_wr.h"
#include "xpath_yang.h"

static int my_ses_read_cb (void* context, char* buffer, int len)
{
    ses_cb_t *scb = (ses_cb_t *)context; 
    return fread(buffer, 1, len, scb->fp);
}

/********************************************************************
* FUNCTION xml_rd_open_file
* 
* Read the value for the specified obj from an open FILE in XML format
*
* INPUTS:
*    fp == open FILE control block
*    obj == object template for the output value
*    val == address of value for output
*
* RETURNS:
*    status
*********************************************************************/
status_t
    xml_rd_open_file (FILE *fp,
                      obj_template_t *obj, 
                      val_value_t **val)
{
    xml_node_t     top;
    status_t       res;
    /* get a dummy session control block */
    ses_cb_t *scb = ses_new_dummy_scb();
    if (!scb) {
        return ERR_INTERNAL_MEM;
    }
    scb->fp = fp;
    res = xml_get_reader_for_session(my_ses_read_cb,
                                     NULL, scb/*context*/, &scb->reader);
    if(res != NO_ERR) {
        return res;
    }
    /* parse */
    *val = val_new_value();
    if(*val == NULL) {
        return ERR_INTERNAL_MEM;
    }
    xml_init_node(&top);
    
    res = xml_consume_node(scb->reader, &top, TRUE, TRUE);
    if (res != NO_ERR) {
        return res;
    }

    res = val_parse(scb, obj, &top, *val);

    scb->fp = NULL; /* skip fclose inside ses_free_scb */
    ses_free_scb(scb);

    xml_clean_node(&top);

    return res;

} /* xml_rd_open_file */

