#ifndef _H_val_parse
#define _H_val_parse

#include "obj.h"
#include "ses.h"
#include "status.h"
#include "val.h"
#include "xml_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


status_t 
    val_parse_split (ses_cb_t  *scb,
                       obj_template_t *obj,
                       obj_template_t *output,
                       const xml_node_t *startnode,
                       val_value_t  *retval);

status_t 
    val_parse (ses_cb_t  *scb,
               obj_template_t *obj,
               const xml_node_t *startnode,
               val_value_t  *retval);


#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_val_parse */
