/** @file       hash.c
 *  @brief      A small hash library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/************************ small hash library **********************************/

static uint32_t hash_alg(hash_table_t * table, const char *s)
{
	return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

static hash_entry_t *hash_new_pair(const char *key, void *val)
{ /**@brief internal function to create a chained hash node**/
	hash_entry_t *np;
	if (!(np = calloc(1, sizeof(*np))))
		return NULL;
	np->key = (char *)key;
	np->val = val;
	if (!np->key || !np->val)
		return NULL;
	return np;
}

hash_table_t *hash_create(size_t len)
{
	return hash_create_auto_free(len, 0, 0);
}

hash_table_t *hash_create_auto_free(size_t len, unsigned free_key, unsigned free_value)
{
	hash_table_t *nt;
	if (!len)
		len++;
	if (!(nt = calloc(1, sizeof(*nt))))
		return NULL;
	if (!(nt->table = calloc(len, sizeof(*nt->table))))
		return free(nt), NULL;
	nt->len = len;
	nt->free_key = free_key;
	nt->free_value = free_value;
	return nt;
}

void hash_destroy(hash_table_t * h)
{
	size_t i;
	hash_entry_t *cur, *prev;
	if (!h)
		return;
	for (i = 0; i < h->len; i++)
		if (h->table[i]) {
			prev = NULL;
			for (cur = h->table[i]; cur; prev = cur, cur = cur->next) {
				if(h->free_key)
					free(cur->key);
				if(h->free_value)
					free(cur->val);
				free(prev);
			}
			if(h->free_key)
				free(cur->key);
			if(h->free_value)
				free(cur->val);
			free(prev);
		}
	free(h->table);
	free(h);
}

static int hash_grow(hash_table_t * ht)
{
	size_t i;
	hash_table_t *new;
	hash_entry_t *cur, *prev;
	memset(&new, 0, sizeof(new));
	if (!(new = hash_create(ht->len * 2)))
		goto fail;
	for (i = 0; i < ht->len; i++)
		if (ht->table[i])
			for (cur = ht->table[i]; cur; cur = cur->next)
				if (hash_insert(new, cur->key, cur->val) < 0)
					goto fail;
	for (i = 0; i < ht->len; i++)
		if (ht->table[i]) {
			prev = NULL;
			for (cur = ht->table[i]; cur; prev = cur, cur = cur->next)
				free(prev);
			free(prev);
		}
	free(ht->table);
	ht->table = new->table;
	ht->len = new->len;
	free(new);
	return 0;
 fail:	hash_destroy(new);
	return -1;
}

int hash_insert(hash_table_t * ht, const char *key, void *val)
{
	assert(ht && key && val);
	uint32_t hash = hash_alg(ht, key);
	hash_entry_t *cur, *newt, *last = NULL;

	if (hash_get_load_factor(ht) >= 0.75f)
		hash_grow(ht);

	cur = ht->table[hash];

	for (; cur && cur->key && strcmp(key, cur->key); cur = cur->next)
		last = cur;

	if (cur && cur->key && !strcmp(key, cur->key)) {
		ht->replacements++;
		cur->val = val;	/*replace */
	} else {
		if (!(newt = hash_new_pair(key, val)))
			return -1;
		ht->used++;
		if (cur == ht->table[hash]) {	/*collisions, insert head */
			ht->collisions++;
			newt->next = cur;
			ht->table[hash] = newt;
		} else if (!cur) {	/*free bin */
			last->next = newt;
		} else {	/*collision, insertions */
			ht->collisions++;
			newt->next = cur;
			last->next = newt;
		}
	}
	return 0;
}

void *hash_foreach(hash_table_t * h, hash_func func)
{
	assert(h && func);
	size_t i = 0;
	hash_entry_t *cur = NULL;
	void *ret;
	if (h->foreach) {
		i = h->foreach_index;
		cur = h->foreach_cur;
		goto resume;
	}
	h->foreach = 1;
	for (i = 0; i < h->len; i++)
		if (h->table[i])
			for (cur = h->table[i]; cur;) {
				if ((ret = (*func) (cur->key, cur->val))) {
					h->foreach_index = i;
					h->foreach_cur = cur;
					return ret;
				}
 resume:			cur = cur->next;
			}
	h->foreach = 0;
	return NULL;
}

void hash_reset_foreach(hash_table_t * h)
{
	h->foreach = 0;
}

static void *hprint(const char *key, void *val)
{
	assert(key);
	return printf("(\"%s\" %p)\n", key, val), NULL;
}

void hash_print(hash_table_t * h)
{
	assert(h);
	hash_foreach(h, hprint);
}

double hash_get_load_factor(hash_table_t * h)
{
	assert(h && h->len);
	return (double)h->used / h->len;
}

size_t hash_get_collision_count(hash_table_t * h)
{
	assert(h);
	return h->collisions;
}

size_t hash_get_replacements(hash_table_t * h)
{
	assert(h);
	return h->replacements;
}

size_t hash_get_number_of_bins(hash_table_t * h)
{
	assert(h);
	return h->len;
}

void *hash_lookup(hash_table_t * h, const char *key)
{
	uint32_t hash;
	hash_entry_t *cur;
	assert(h && key);

	hash = hash_alg(h, key);
	cur = h->table[hash];
	while (cur && cur->next && strcmp(cur->key, key))
		cur = cur->next;
	if (!cur || !cur->key || strcmp(cur->key, key))
		return NULL;
	else
		return cur->val;
}

