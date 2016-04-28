/**
 * useful functions!
 *
 */

#ifndef __TIVO_UTIL_H__
#define __TIVO_UTIL_H__

#include "tree.h"
#include "tivoTypes.h"

/**
 * returns: the number of nodes on this given chain
 */
int tivoCountChain( tree node );

tivoBool tivoIsClassType( tree type );

/*
// define:   "realType(t)" to be the type of t once you've removed
//           all of the trailing *'s and &'s.
// returns:  realType(t)
// requires: type != NULL
// memory:   return value is just a pointer into type!
*/
tree tivoGetNuggetType( tree type );

#endif /* __TIVO_UTIL_H__*/

