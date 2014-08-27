/**
 *  @file           hash.h
 *  @brief          A simple hash table implementation, header only
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 **/

struct hashentry;
struct hashtable;

typedef struct hashtable hashtable_t;

hashtable_t *hash_create(size_t len);
void hash_destroy(hashtable_t * ht);
void hash_insert(hashtable_t * ht, const char *key, const char *val);
char *hash_lookup(hashtable_t * table, const char *key);
void hash_print(hashtable_t * table);

unsigned int hash_get_collisions(hashtable_t * table);
unsigned int hash_get_uniquekeys(hashtable_t * table);
unsigned int hash_get_replaced(hashtable_t * table);

