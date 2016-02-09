#include <xmlstring.h>
#include <string.h>
#include <assert.h>
#include "dlq.h"
#include "ncx.h"
#include "ncxmod.h"
#include "ncxtypes.h"
#include "status.h"

val_value_t* val_get_leafref_targval(val_value_t *leafref_val, val_value_t *root_val);
