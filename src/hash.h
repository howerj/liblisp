/**
 *  @file           hash.h
 *  @brief          A simple hash table implementation, header only
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 **/

#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

struct hashentry;
typedef struct hashtable hashtable_t;

hashtable_t *hash_create(size_t len);
void hash_destroy(hashtable_t * table);
void hash_insert(hashtable_t * ht, const char *key, void*  val);
void*  hash_lookup(hashtable_t * table, const char *key);
void hash_print(hashtable_t * table);

unsigned hash_get_collisions(hashtable_t * table);
unsigned hash_get_uniquekeys(hashtable_t * table);
unsigned hash_get_replaced(hashtable_t * table);

#ifdef __cplusplus
}
#endif
#endif
