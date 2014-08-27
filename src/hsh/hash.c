/**
 *  @file           hash.c
 *  @brief          A simple hash table implementation.
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  @todo Delete entry function
 *  @todo Clean up
 *  @todo Support associative arrays
 *  @todo Replace char -> uint8_t
 *  @todo Integrate with lisp interpreter
 **/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "hash.h"

/* private types */

typedef struct hashentry {
        char *key;
        char *val;
        struct hashentry *next;    /*linked list of entries in a bin */
        struct hashentry *allnext; /*linked list of all entries */
} hashentry_t;

typedef struct hashtable {
        size_t len;
        struct hashentry **table;
        struct hashentry *head;
        unsigned int collisions;
        unsigned int uniquekeys;
        unsigned int replaced;
} hashtable;

/* internal function prototypes */

static char *_strdup(const char *s);
static uint32_t djb2(const char *s, size_t len);
static uint32_t hash_alg(hashtable * table, const char *s);
static hashentry_t *hash_newpair(const char *key, const char *val);

/*** Function with external linkage ******************************************/

/** 
 *  @brief    Create a new hash table of size len
 *  @param    len       size of hash table
 *  @return   NULL or a new hash table
 */
hashtable *hash_create(size_t len)
{
        hashtable *newt = NULL;

        if (!len)
                return NULL;

        if (NULL == (newt = calloc(sizeof(*newt), 1)))
                return NULL;

        if (NULL == (newt->table = calloc(sizeof(*newt->table), len)))
                return NULL;

        newt->len = len;

        return newt;
}

/** 
 *  @brief    Destroy a hash table, freeing all the elements
 *  @param    ht The hash table to destroy
 *  @return   void
 */
void hash_destroy(hashtable * ht)
{
        hashentry_t *current, *prev;

        if (!ht)
                return;

        current = ht->head;
        while (current) {
                prev = current;
                current = current->allnext;
                free(prev->key);
                free(prev->val);
                free(prev);
        }

        free(ht->table);
        free(ht);

        return;
}

/** 
 *  @brief    Insert a key-value pair into a hash table
 *  @param    ht        The hash table
 *  @param    key       The key
 *  @param    val       The value
 *  @return   void      
 */
void hash_insert(hashtable * ht, const char *key, const char *val)
{
        uint32_t hash;
        hashentry_t *current, *newt, *last;

        assert(NULL != ht);

        hash = hash_alg(ht, key);
        current = ht->table[hash];

        while (current && current->key && strcmp(key, current->key)) {
                last = current;
                current = current->next;
        }

        if (current && current->key && !strcmp(key, current->key)) {
                free(current->val);
                current->val = _strdup(val);
                ht->replaced++;
        } else {
                newt = hash_newpair(key, val);

                /*add to list of all hashes */
                if (!ht->head) {
                        ht->head = newt;
                } else {
                        newt->allnext = ht->head;
                        ht->head = newt;
                }

                ht->uniquekeys++;
                if (current == ht->table[hash]) {
                        newt->next = current;
                        ht->table[hash] = newt;
                } else if (!current) {
                        last->next = newt;
                } else {
                        newt->next = current;
                        last->next = newt;
                }
        }

        return;
}


/** 
 *  @brief    Print out a hash table
 *  @param    ht        The hash table to print out
 *  @return   void      
 */
void hash_print(hashtable * table)
{
        hashentry_t *current;
        if (!table)
                return;

        current = table->head;
        while (current) {
                printf("key '%s' val '%s'\n", current->key, current->val);
                current = current->allnext;
        }
}

/** 
 *  @brief    Lookup a key in the hash table
 *  @param    ht        The hash table to search in
 *  @param    key       The key to search for
 *  @return   The key, if found, NULL otherwise
 */
char *hash_lookup(hashtable * table, const char *key)
{
        uint32_t hash;
        hashentry_t *current;

        assert(NULL != table);

        hash = hash_alg(table, key);
        current = table->table[hash];

        while (current && current->next && strcmp(current->key, key))
                current = current->next;

        return current ? current->val : NULL;
}

/** 
 *  @brief    Get information; number of collisions
 *  @param    ht        Hash table containing the information
 *  @return   unsigned int Number of collisions
 */
unsigned int hash_get_collisions(hashtable_t * table){
        assert(NULL != table);
        return table->collisions;
}

/** 
 *  @brief    Get information; Number of unique keys
 *  @param    ht        Hash table containing the information
 *  @return   unsigned int Number of unique keys
 */
unsigned int hash_get_uniquekeys(hashtable_t * table){
        assert(NULL != table);
        return table->uniquekeys;
}

/** 
 *  @brief    Get information; number of keys that have had their value replaced
 *  @param    ht        Hash table containing the information
 *  @return   unsigned int Number of replacements
 */
unsigned int hash_get_replaced(hashtable_t * table){
        assert(NULL != table);
        return table->replaced;
}

/*** Functions with internal linkage *****************************************/

/** 
 *  @brief    This is a copy of the strdup function provided by some libraries
 *  @param    s         The string to duplicated
 *  @return   char*     The dupplicated string
 */
static char *_strdup(const char *s)
{
        char *str = calloc(sizeof(*s), strlen(s) + 1);
        strcpy(str, s);
        return str;
}

/** 
 *  @brief    A hashing algorithm, see <http://www.cse.yorku.ca/~oz/hash.html>
 *            and any references to "djb2" hash in the literature.
 *  @param    s         The string to hash
 *  @param    len       The strings length
 *  @return   uint32_t  The hashed value
 */
static uint32_t djb2(const char *s, size_t len)
{
        uint32_t hash = 5381;   /*0x1505 */
        size_t i = 0;

        for (i = 0; i < len; s++, i++)
                hash = ((hash << 5) + hash) + (*s);

        return hash;
}

/** 
 *  @brief    A wrapper around a hashing algorithm to account for
 *            the number of bins used.
 *  @param    table     hash table, which contains some information needed
 *  @param    s         string to hash
 *  @return   uint32_t  The hashed value
 */
static uint32_t hash_alg(hashtable * table, const char *s)
{
        return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

/** 
 *  @brief    Create a new hash table key-value pair
 *  @param    key       Key to copy
 *  @param    val       Value to copy
 *  @return   hashentry_t* A new hash table entry that is initialized or NULL
 */
static hashentry_t *hash_newpair(const char *key, const char *val)
{
        hashentry_t *nent = NULL;

        if (NULL == (nent = calloc(sizeof(*nent), 1)))
                return NULL;

        nent->key = _strdup(key);
        nent->val = _strdup(val);

        if ((NULL == nent->key) || (NULL == nent->val))
                return NULL;

        return nent;
}

