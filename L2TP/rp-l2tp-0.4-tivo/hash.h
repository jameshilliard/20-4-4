// Fixed-size hash tables for now
#define HASHTAB_SIZE 67

// A hash bucket
struct hash_bucket
{
    hash_bucket *next;
    hash_bucket *prev;
    unsigned int hashval;
};

// A hash table
struct hash_table
{
    hash_bucket *buckets[HASHTAB_SIZE];
    size_t hash_offset;
    unsigned int (*compute_hash)(void *data);
    int (*compare)(void *item1, void *item2);
    size_t num_entries;
};


// Functions

void hash_insert(hash_table *tab, void *item);
void hash_remove(hash_table *tab, void *item);
void *hash_find(hash_table *tab, void *item);
void *hash_find_next(hash_table *tab, void *item);
void *hash_start(hash_table *tab, void **cursor);
void *hash_next(hash_table *tab, void **cursor);
size_t hash_num_entries(hash_table *tab);
unsigned int hash_pjw(char const * str);
void hash_init(hash_table *tab, size_t hash_offset, unsigned int (*compute)(void *data), int (*compare)(void *item1, void *item2));
