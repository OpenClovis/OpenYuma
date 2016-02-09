#ifndef _H_xml_rd
#define _H_xml_rd

/*  FILE: xml_rd.h
*********************************************************************
*								    *
*			 P U R P O S E				    *
*								    *
*********************************************************************

    XML Read functions
*/

#include <stdio.h>
#include <xmlstring.h>
#include "cfg.h"
#include "dlq.h"
#include "ncxtypes.h"
#include "ses.h"
#include "status.h"
#include "val.h"
#include "val_util.h"
#include "xml_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
*								    *
*			F U N C T I O N S			    *
*								    *
*********************************************************************/


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
extern status_t
    xml_rd_open_file (FILE *fp,
                      obj_template_t *obj, 
                      val_value_t **val);

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif	    /* _H_xml_rd */


