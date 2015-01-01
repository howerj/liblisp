/**
 *  @file           hash.c
 *  @brief          A simple hash table implementation.
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  @todo Delete entry function
 *  @todo Add foreach, delete
 *  @todo Replace char -> uint8_t, or void*
 *  @todo change hash print function to use the IO wrapper I made
 *  @todo This should use the memory allocation wrappers I made
 *  @todo Integrate with lisp interpreter *OR* make more generic via void*
 **/
#include <stdint.h>
#include <stdlib.h> /* free, calloc */
#include <string.h> /* strcmp, strlen, strcpy */
#include <stdio.h>  /* printf */
#include <assert.h> /* assert */
#include "hash.h"

/* private types */

typedef struct hashentry {
        char *key;
        char *val;
        struct hashentry *next;    /*linked list of entries in a bin */
} hashentry_t;

typedef struct hashtable {
        size_t len;
        struct hashentry **table;
        unsigned collisions;
        unsigned uniquekeys;
        unsigned replaced;
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

        if (!len || (NULL == (newt = calloc(sizeof(*newt), 1))))
                return NULL;

        if (NULL == (newt->table = calloc(sizeof(*newt->table), len))){
                free(newt);
                return NULL;
        }

        newt->len = len;

        return newt;
}

/** 
 *  @brief    Destroy a hash table, freeing all the elements
 *  @param    table     The hash table to destroy
 *  @return   void
 */
void hash_destroy(hashtable * table)
{
        size_t i;
        hashentry_t *current, *prev;

        if (NULL == table)
                return;

        for(i = 0; i < table->len; i++){
                if(NULL != table->table[i]){
                        prev = NULL;
                        for(current = table->table[i]; current; current = current->next){
                                free(prev);
                                free(current->key);
                                free(current->val);
                                prev = current;
                        }
                        free(prev);
                }
        }

        free(table->table);
        free(table);

        return;
}

/** 
 *  @brief    Insert a key-value pair into a hash table
 *  @param    ht        The hash table, should not be NULL
 *  @param    key       The key, should not be NULL
 *  @param    val       The value, should not be NULL
 *  @return   void      
 */
void hash_insert(hashtable * ht, const char *key, const char *val)
{
        uint32_t hash;
        hashentry_t *current = NULL, *newt = NULL, *last = NULL;

        assert((NULL != ht) && (NULL != key) && (NULL != val));

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
 *  @param    table     The hash table to print out, should not be NULL
 *  @return   void      
 */
void hash_print(hashtable * table)
{
        size_t i;
        hashentry_t *current;

        if (NULL == table)
                return;

        for(i = 0; i < table->len; i++){
                if(NULL != table->table[i]){
                        for(current = table->table[i]; current; current = current->next)
                                printf("key '%s' val '%s'\n", current->key, current->val);
                }
        }
}

/** 
 *  @brief    Lookup a key in the hash table
 *  @param    table     The hash table to search in, should not be NULL
 *  @param    key       The key to search for, should not be NULL
 *  @return   The key, if found, NULL otherwise
 */
char *hash_lookup(hashtable * table, const char *key)
{
        uint32_t hash;
        hashentry_t *current;

        assert((NULL != table) && (NULL != key));

        hash = hash_alg(table, key);
        current = table->table[hash];

        while (current && current->next && strcmp(current->key, key))
                current = current->next;

        return current ? current->val : NULL;
}

/** 
 *  @brief    Get information; number of collisions
 *  @param    table             Hash table containing the information, should not be NULL
 *  @return   unsigned          Number of collisions
 */
unsigned hash_get_collisions(hashtable_t * table){
        assert(NULL != table);
        return table->collisions;
}

/** 
 *  @brief    Get information; Number of unique keys
 *  @param    table             Hash table containing the information, should not be NULL
 *  @return   unsigned          Number of unique keys
 */
unsigned hash_get_uniquekeys(hashtable_t * table){
        assert(NULL != table);
        return table->uniquekeys;
}

/** 
 *  @brief    Get information; number of keys that have had their value replaced
 *  @param    table             Hash table containing the information, should not be NULL
 *  @return   unsigned          Number of replacements
 */
unsigned hash_get_replaced(hashtable_t * table){
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
        char *str;
        assert(s);
        str = calloc(sizeof(*s), strlen(s) + 1);
        strcpy(str, s);
        return str;
}

/** 
 *  @brief    A hashing algorithm, see <http://www.cse.yorku.ca/~oz/hash.html>
 *            and any references to "djb2" hash in the literature.
 *  @param    s         The string to hash, can be NULL if 0 == len
 *  @param    len       The strings length
 *  @return   uint32_t  The hashed value
 */
static uint32_t djb2(const char *s, size_t len)
{
        uint32_t hash = 5381;   /*magic number this hash uses, it just is*/
        size_t i = 0;
        assert(s);

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

