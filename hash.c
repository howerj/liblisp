/** @file       hash.c
 *  @brief      A small hash library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  @todo This hash library should be tested against the linear probing
 *        version to determine if there is any performance difference.
 *  @todo Resizing, deletion, ...
 *  @todo The lisp interpreter symbols a strings that are "interned",
 *        or there is a unique pointer representing each symbol, this
 *        means we could use a pointer instead of a string comparison.
 **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/************************ small hash library **********************************/

static uint32_t hash_alg(hashtable *table, const char *s) {
        return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

static hashentry *hash_new_pair(const char *key, void *val) 
{ /**@brief internal function to create a chained hash node**/
        hashentry *np;
        if (!(np = calloc(1, sizeof(*np)))) return NULL;
        np->key = lstrdup(key);
        np->val = val;
        if (!np->key || !np->val) return NULL;
        return np;
}

hashtable *hash_create(size_t len) {
        hashtable *nt;
        if (!len || !(nt = calloc(1, sizeof(*nt)))) return NULL;
        if (!(nt->table  = calloc(1, sizeof(*nt->table)*len)))
                return free(nt), NULL;
        nt->len = len;
        return nt;
}

void hash_destroy(hashtable *h) {
        size_t i;
        hashentry *cur, *prev;
        if (!h) return;
        for(i = 0; i < h->len; i++)
                if(h->table[i]){
                        prev = NULL;
                        for(cur = h->table[i]; cur; cur = cur->next){
                                free(prev);
                                free(cur->key);
                                prev = cur;
                        }
                        free(prev);
                }
        free(h->table);
        free(h);
}

int hash_insert(hashtable *ht, const char *key, void *val) { assert(ht && key && val);
        uint32_t hash = hash_alg(ht, key);
        hashentry *cur = ht->table[hash], *newt, *last = NULL;

        for(;cur && cur->key && strcmp(key, cur->key); cur = cur->next)
                last = cur; 

        if (cur && cur->key && !strcmp(key, cur->key)) {
                cur->val = val; /*replace*/
        } else {
                if(!(newt = hash_new_pair(key, val))) 
                        return -1;
                ht->used++;
                if (cur == ht->table[hash]) { /*collisions, insert head*/
                        ht->collisions++;
                        newt->next = cur;
                        ht->table[hash] = newt;
                } else if (!cur) { /*free bin*/
                        last->next = newt;
                } else { /*collision, insertions*/
                        ht->collisions++;
                        newt->next = cur;
                        last->next = newt;
                }
        }
        return 0;
}

void *hash_foreach(hashtable *h, hash_func func) { assert(h && func);
        size_t i;
        hashentry *cur;
        void *ret;
        for(i = 0; i < h->len; i++)
                if(h->table[i])
                        for(cur = h->table[i]; cur; cur = cur->next)
                                if((ret = (*func)(cur->key, cur->val)))
                                        return ret;
        return NULL;
}

static void *hprint(const char *key, void *val) { assert(key);
        return printf("(\"%s\" %p)\n", key, val), NULL;
}

void hash_print(hashtable *h)                 { assert(h); hash_foreach(h, hprint); }
double hash_get_load_factor(hashtable *h)     { assert(h && h->used); return (double)h->used / h->len; }
size_t hash_get_collision_count(hashtable *h) { assert(h); return h->collisions; }
size_t hash_get_replacements(hashtable *h)    { assert(h); return h->replacements; }
size_t hash_get_number_of_bins(hashtable *h)  { assert(h); return h->len; }

void* hash_lookup(hashtable *h, const char *key) {
        uint32_t hash;
        hashentry *cur;
        assert(h && key);

        hash = hash_alg(h, key);
        cur = h->table[hash];
        while (cur && cur->next && strcmp(cur->key, key))
                cur = cur->next;
        if(!cur || !cur->key || strcmp(cur->key, key)) return NULL;
        else return cur->val;
}

