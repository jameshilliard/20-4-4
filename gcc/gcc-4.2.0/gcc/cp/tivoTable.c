#include <assert.h>
#include <stdlib.h>  

#include "config.h"
#include "system.h"
#include "coretypes.h"
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

#define HASH_SIZE 37

struct tivoBucket
{
  tree node;
  struct tivoBucket *next;
};

/* returns: a new, empty tivoTable */
tivoTable_t tivoTableCreate()
{
   const int numBytes = HASH_SIZE * sizeof (struct tivoBucket *);
   tivoTable_t table = (tivoTable_t) xmalloc ( numBytes );
   memset( (char*) table, 0, numBytes );

   return table;
}

void tivoTableMakeEmpty( tivoTable_t table )
{
    int i = 0;

    for( i=0; i<HASH_SIZE; i++ ) {
        /* free the tivoBuckets */
        struct tivoBucket* scan = table[i];
        table[i] = NULL;

        while( scan != NULL ) {
            struct tivoBucket* prev = scan;
            scan = scan->next;

            free( (char*) prev );
        }
    }
}

/* effects: empties the table and deletes it*/
void tivoTableFree( tivoTable_t table )
{
    tivoTableMakeEmpty( table );
    free( (char*) table );
}

tivoBool tivoTableContains( tivoTable_t table, tree node )
{
  struct tivoBucket *b = NULL;
  int hash = ((unsigned long) node) % HASH_SIZE;

  for (b = table[hash]; b; b = b->next) {
      if (b->node == node) {
          return tivoTrue;
      }
  }

  return tivoFalse;
}

void tivoTableAdd( tivoTable_t table, tree node )
{
  int hash = ((unsigned long) node) % HASH_SIZE;
  struct tivoBucket *b = (struct tivoBucket *) xmalloc (sizeof(struct tivoBucket));
  assert( !tivoTableContains(table, node) );
  b->node = node;
  b->next = table[hash];
  table[hash] = b;
}
