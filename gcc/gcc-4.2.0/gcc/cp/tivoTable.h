/*
 * tivoTable.h
 *
 * This file contains declarations for the hash table
 * TiVo uses to implement some of its extensions.
 *
 */

#ifndef __TIVO_TABLE_H__
#define __TIVO_TABLE_H__

#include "tivoTypes.h"

#ifndef TREE_TYPE
#include "tree.h"
#endif

struct tivoBucket;
typedef struct tivoBucket** tivoTable_t;

/* returns: a new, empty tivoTable */
tivoTable_t tivoTableCreate(void);

/* effects: empties the table */
void tivoTableMakeEmpty( tivoTable_t table );

/* effects: empties the table and deletes it */
void tivoTableFree( tivoTable_t table );

/* returns: true iff the table contains the given node */
tivoBool tivoTableContains( tivoTable_t table, tree node );

void tivoTableAdd( tivoTable_t table, tree node );
#endif /* __TIVO_TABLE_H__ */
