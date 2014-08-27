/**
 *  @file           test.c
 *  @brief          A test bench for a simple hash table implementation
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

#define BUF_LEN (512u)
#define HASHSZ  (4096u)

int main(void)
{
        static char key[BUF_LEN], val[BUF_LEN], scratch[BUF_LEN];
        char *s;
        unsigned int uniquekeys, replaced;
        hashtable_t *ht = hash_create(HASHSZ);
        int i = 1;

        while (3 == scanf("%512s%512[ \t]%512s", key, scratch, val)) {
                hash_insert(ht, key, val);
                s = hash_lookup(ht, key);
                printf("%s %4d %20s%20s%20s\n", strcmp(s, val) ? "fail" : "pass", i++, key, s, val);
        }

        uniquekeys = hash_get_uniquekeys(ht);
        replaced = hash_get_replaced(ht);
        printf("load %d/%d %f\n", uniquekeys, HASHSZ, uniquekeys / (float)HASHSZ);
        printf("replaced %d\n", replaced);
        hash_print(ht);
        hash_destroy(ht);

        return EXIT_SUCCESS;
}
