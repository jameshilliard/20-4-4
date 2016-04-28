#include <assert.h>
#include <stdlib.h>  

#include "config.h"
#include "coretypes.h"
#include "system.h"
#include "tm.h"
#include "tree.h"
#include <stdio.h>
#include "cp/cp-tree.h"
#include "tivoDeclIter.h"
#include "tivoCallGraph.h"
#include "tivoTable.h"
#include "tivoUtil.h"
#include "toplev.h"

#include <string.h>

#include <assert.h>

int tivoCountChain( tree node )
{
    int i = 0;
    tree scan = NULL;

    for( scan = node; scan != NULL; scan = TREE_CHAIN(scan) ) {
        i++;
    }

    return i;
}

tivoBool tivoIsClassType( tree type )
{
    return (TREE_CODE(type) == RECORD_TYPE) ? tivoTrue : tivoFalse;
}

static tivoBool tivoIsRefOrPtrOrArray( tree type )
{
    enum tree_code code = ERROR_MARK;

    if ( type == NULL ) {
        assert( tivoFalse && "null!" );
        return tivoFalse;
    }

    code = TREE_CODE(type);
    return ( code == POINTER_TYPE || 
             code == REFERENCE_TYPE ||
             code == ARRAY_TYPE );
}

tree tivoGetNuggetType( tree type )
{
    /* drill down into the nested type as long as the
     * current type is a ptr or reference
     */
    tree nuggetType = NULL;
    for( nuggetType = type; 
         tivoIsRefOrPtrOrArray(nuggetType); 
         nuggetType = TREE_TYPE(nuggetType) ) {
        /* the real work is in the "increment" above */
    }
    
    return nuggetType;
}
