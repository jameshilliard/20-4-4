/*
 * tivoDeclIter.h
 *
 * This contains declarations for iterating over all
 * declarations.
 *
 */

#ifndef __TIVO_DECL_ITER_H__
#define __TIVO_DECL_ITER_H__

#include "tivoTypes.h"
#include "tree.h"

/**
 * function for iterating over a node tree.
 * for RECORD_TYPE, TYPE_DECL, and TREE_VECs,
 *   the iter method is called once with fStart true
 *   before visiting the subtree.  if this returns
 *   true, the subtree will be visited.  in either
 *   case, it will be called again with fStart false.
 * for other types, the iter is called only once
 *   and the return type should be true.
 */
typedef int (*tivoDeclIter_t)( tree node, int fStart );

/* effects: iterates over everything reachable from node */
void tivoIterateOver( tivoDeclIter_t iter, tree node );

/* effects: iterates over everything in the global scope */
void tivoIterateOverGlobal( tivoDeclIter_t iter );

#endif /* __TIVO_DECL_ITER_H__ */
