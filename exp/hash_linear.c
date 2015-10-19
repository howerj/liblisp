#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

uint32_t djb2(const char *s, size_t len) { assert(s); 
        uint32_t h = 5381;   /*magic number this hash uses, it just is*/
        size_t i = 0;
        for (i = 0; i < len; s++, i++) h = ((h << 5) + h) + (*s);
        return h;
}

char *lstrdup(const char *s) { assert(s);
        char *str;
        if(!(str = malloc(strlen(s) + 1))) 
                return NULL;
        strcpy(str, s);
        return str;
}

struct hashentry { /**< linked list of entries in a bin*/
        char *key; /**< ASCII nul delimited string*/
        void *val; /**< arbitrary value*/
};

struct hashtable {  /**< a hash table*/
        size_t len, /**< number of 'bins' in the hash table*/
               used;/**< number of used 'bins' in the hash table*/ 
        struct hashentry table[]; /**< table of linked lists, flexible array*/
};

typedef void *(*hash_func)(const char *key, void *val); 
typedef struct hashentry hashentry;
typedef struct hashtable hashtable;

static uint32_t hash_alg(hashtable *table, const char *s) {
        return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

hashtable *hash_create(size_t len) {
        hashtable *nt;
        if(len < 2) 
                len = 2;
        if (!(nt = calloc(1, sizeof(*nt) + sizeof(*nt->table) * len))) 
                return NULL;
        nt->len = len;
        return nt;
}

static void internal_hash_destroy(hashtable *h, int freekey) { 
        size_t i;
        if(freekey)
                for(i = 0; i < h->len; i++)
                        free(h->table[i].key);
        free(h);
}

void hash_destroy(hashtable *h) { assert(h);
        internal_hash_destroy(h, 1);
}

hashtable *internal_hash_insert(hashtable *h, const char *key, void *val, int dup);

hashtable *hash_grow(hashtable *h) { assert(h);
        size_t i;
        hashtable *h2;
        if(h->used < ((h->len * 3) / 4))
                return h;
        if(!(h2 = hash_create(h->len * 2)))
                return NULL;
        for(i = 0; i < h->len; i++) {
                if(h->table[i].key)
                        if(!internal_hash_insert(h2, h->table[i].key, h->table[i].val, 0)) {
                                internal_hash_destroy(h2, 0);
                                return NULL;
                        }
        }
        free(h);
        return h2;
}

hashtable *internal_hash_insert(hashtable *h, const char *key, void *val, int dup) {
        uint32_t alg;
        assert(h && key && val);
        alg = hash_alg(h, key);
        if(!(h = hash_grow(h)))
                return NULL;
        for(alg = hash_alg(h, key); ; alg++, alg %= (h->len - 1)) {
                if(!h->table[alg].key) { /*new*/
                        if(!(h->table[alg].key = dup ? lstrdup(key) : (char*)key))
                                return NULL;
                        h->table[alg].val = val;
                        h->used++;
                        return h;
                } 
                if(!strcmp(key, h->table[alg].key)) { /*replace*/
                        h->table[alg].val = val;
                        h->used++;
                        return h;
                }
        }
        return NULL;
}


hashtable *hash_insert(hashtable *h, const char *key, void *val) {
        return internal_hash_insert(h, key, val, 1);
}

void *hash_foreach(hashtable *h, hash_func func) {
        size_t i;
        void *ret;
        for(i = 0; i < h->len; i++)
                if(h->table[i].key) 
                        if((ret = (*func)(h->table[i].key, h->table[i].val)))
                                return ret;
        return NULL;
}

static void *hprint(const char *key, void *val) { assert(key);
        return printf("(\"%s\" %p)\n", key, val), NULL;
}

void hash_print(hashtable *h) { assert(h); hash_foreach(h, hprint); }

void* hash_lookup(hashtable *h, const char *key) { assert(h && key);
        uint32_t alg;
        alg = hash_alg(h, key);
        for(alg = hash_alg(h, key); h->table[alg].key ; alg++, alg %= h->len)
                if(!strcmp(key, h->table[alg].key))
                        return h->table[alg].val;
        return NULL;
}

#define ASSERT(X, MSG) \
        if(!(X)) {\
                fprintf(stderr, "assert failed: '%s' '%s' %d %s\n",\
                       # X, (MSG),__LINE__, __FILE__);\
                exit(-1);\
        } 

int main(void) {
        hashtable *ht;
        ht = hash_create(32);
        ASSERT(ht = hash_insert(ht, "key1", "val1"), "key insertion");
        ASSERT(ht = hash_insert(ht, "key2", "val2"), "key insertion");
        ASSERT(ht = hash_insert(ht, "key3", "val3"), "key insertion");

        /*djb2 collisions*/
        ASSERT(ht = hash_insert(ht, "hetairas",    "hetairas"), "key insertion");
        ASSERT(ht = hash_insert(ht, "mentioner",    "mentioner"), "key insertion");

        ASSERT(ht = hash_insert(ht, "heliotropes", "heliotropes"), "key insertion");
        ASSERT(ht = hash_insert(ht, "neurospora", "neurospora"), "key insertion");

        ASSERT(ht = hash_insert(ht, "depravement", "depravement"), "key insertion");
        ASSERT(ht = hash_insert(ht, "serafins",    "serafins"), "key insertion");

        ASSERT(ht = hash_insert(ht, "stylist",     "stylist"), "key insertion");
        ASSERT(ht = hash_insert(ht, "subgenera",   "subgenera"), "key insertion");

        ASSERT(ht = hash_insert(ht, "joyful",      "joyful"), "key insertion");
        ASSERT(ht = hash_insert(ht, "synaphea",    "synaphea"), "key insertion");

        ASSERT(ht = hash_insert(ht, "redescribed", "redescribed"), "key insertion");
        ASSERT(ht = hash_insert(ht, "urites",      "urites"), "key insertion");

        ASSERT(ht = hash_insert(ht, "dram",        "dram"), "key insertion");
        ASSERT(ht = hash_insert(ht, "vivency",     "vivency"), "key insertion");

        /*djb2a collisions*/
        ASSERT(ht = hash_insert(ht, "haggadot",        "haggadot"), "key insertion");
        ASSERT(ht = hash_insert(ht, "loathsomenesses", "loathsomenesses"), "key insertion");

        ASSERT(ht = hash_insert(ht, "rentability",    "rentability"), "key insertion");
        ASSERT(ht = hash_insert(ht, "adorablenesses", "adorablenesses"), "key insertion");

        ASSERT(ht = hash_insert(ht, "playwright", "playwright"), "key insertion");
        ASSERT(ht = hash_insert(ht, "snush",      "snush"), "key insertion");

        ASSERT(ht = hash_insert(ht, "playwrighting", "playwrighting"), "key insertion");
        ASSERT(ht = hash_insert(ht, "snushing",      "snushing"), "key insertion");

        ASSERT(ht = hash_insert(ht, "treponematoses", "treponematoses"), "key insertion");
        ASSERT(ht = hash_insert(ht, "waterbeds",      "waterbeds"), "key insertion");

        ASSERT(ht = hash_insert(ht, "", ""), "key insertion");

        hash_print(ht);
        printf("%zu:%zu\n", ht->len, ht->used);
        printf("%s:%s\n", "key1", (char*)hash_lookup(ht, "key1"));
        printf("%s:%s\n", "key4", (char*)hash_lookup(ht, "key4"));
        printf("%s:%s\n", "waterbeds", (char*)hash_lookup(ht, "waterbeds"));
        hash_destroy(ht);
        return 0;
}
