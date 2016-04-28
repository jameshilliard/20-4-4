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

#include <string.h>

/*
// HACK: ugly reach into toplevel.c so we can temporarily
//       disable warnings_are_errors if we want to.
*/
extern int warnings_are_errors;

extern int warn_tivo;

//#define CG_DEBUG

/****************************************************************
 *
 * 
 *
 ****************************************************************/

static void tivoIterateOnDecl( tree node,
                               tivoTable_t visited,
                               tivoDeclIter_t iter )
{
    int nodeCode = ERROR_MARK;

    if ( node == NULL ) {
        /* not sure when this happens...but it does! */
        return;
    }

#ifdef CG_DEBUG
    fprintf( stderr, "*> tivoIterateOnDecl: %s(%s)\n",
         tree_code_name[TREE_CODE(node)],
         tivoGetId(node) );
#endif

    /* if already visiting or visited this node, don't repeat!*/
    if ( tivoTableContains( visited, node ) ) {
#ifdef CG_DEBUG
        fprintf(stderr, " <<<<<<<< Seen it before.. returning\n");
#endif
        return;
    }

    /* add to table now to prevent loops!*/
    tivoTableAdd( visited, node );

    /* get the node's code to dispatch below!*/
    nodeCode = TREE_CODE( node );

#ifdef CG_DEBUG
    fprintf( stderr, "---> tivoIterateOnDecl: %s(%s)\n",
         tree_code_name[nodeCode], tivoGetId(node) );
#endif

    if ( nodeCode == VAR_DECL || nodeCode == TYPE_DECL )
    {
        if ( iter( node, 1 ) )
        {
            tivoIterateOnDecl( TREE_TYPE(node), visited, iter );
        }
        iter( node, 0 );
    }
    else if ( nodeCode == PARM_DECL  || nodeCode == TEMPLATE_TYPE_PARM || 
              nodeCode == FIELD_DECL || nodeCode == FUNCTION_DECL )
    {
        iter( node, 0 );
    }
    else if ( nodeCode == TEMPLATE_DECL )
    {
        iter( node, 0 );
        tivoIterateOnDecl( TREE_TYPE(node), visited, iter);
    }
    else if ( nodeCode == OVERLOAD )
    {
        tree t;
        for(t = node; t; t = OVL_CHAIN(t))
            tivoIterateOnDecl( OVL_FUNCTION(t), visited, iter );
    }
    else if ( nodeCode == RECORD_TYPE )
    {
        if ( iter( node, 1 ) )
        {
            tree scan = NULL;
            int i;

            if ( TYPE_LANG_SPECIFIC(node) != NULL ) {
                /* check methods!*/
                for( ;
                     VEC_iterate(tree,CLASSTYPE_METHOD_VEC(node),i,scan);
                    ) {
                    tivoIterateOnDecl( scan, visited, iter );
                }
            }

            /* check members!*/
            for( scan = TYPE_FIELDS(node);
                 scan != NULL;
                 scan = TREE_CHAIN(scan) ) {
                tivoIterateOnDecl( scan, visited, iter );
            }
        }
        iter( node, 0 );
    }
    else if ( nodeCode == TREE_VEC )
    {
        if ( iter( node, 1 ) )
        {
            int len = TREE_VEC_LENGTH(node);
            int i   = 0;

            for( i=0; i < len; i++ ) {
                tree elt = TREE_VEC_ELT(node,i);
                tivoIterateOnDecl( elt, visited, iter );
            }
        }
        iter( node, 0 );
    }
    else if ( nodeCode == LANG_TYPE )
    {
#ifdef CG_DEBUG
        fprintf( stderr, "     is a lang type!\n" );
        debug_tree(node);
#endif
    }
    else {
#ifdef CG_DEBUG
        fprintf( stderr, "     is *not* a handled type!\n" );
#endif
    }
}

static void tivoIterateOverChain( tree decls,
                                  tivoTable_t visited,
                                  tivoDeclIter_t iter )
{
    /*
    // first, fill an array with the decls 'cuz we want to visit them
    // in the proper order and they're stored reversed!
    */

    const int numDecls = tivoCountChain( decls );
    const int numBytes = numDecls * sizeof(tree);
    tree* array  = (tree*) xmalloc( numBytes );
    tree  scan   = NULL;
    int i = 0;

    memset( array, 0, numBytes );
    
    for( scan = decls; scan != NULL; scan = TREE_CHAIN(scan) ) {
        array[i] = scan;
        i++;
    }

    /* now visit the decls in the opposite order they're stored*/
    for( i=numDecls-1; i >= 0; i-- ) {
        tree aDecl = array[i];

#ifdef CG_DEBUG
        fprintf(stderr, "Global decl...\n");
        debug_tree(aDecl);
#endif

        tivoIterateOnDecl( aDecl, visited, iter );
    }

    /* and free the array!*/
    free( (char*) array );
}

/**********************************************************
 *
 * High-level checking routines!
 *
 **********************************************************/

extern int expanding_inline_function;

void tivoIterateOverGlobal( tivoDeclIter_t iter )
{
    tree decls = getdecls();
    tivoTable_t table = tivoTableCreate();
    
    tivoIterateOverChain( decls, table, iter );
    
    tivoTableFree( table );
}

void tivoIterateOver( tivoDeclIter_t iter, tree node )
{
    tivoTable_t table = tivoTableCreate();
    
    tivoIterateOnDecl( node, table, iter );
    
    tivoTableFree( table );
}
