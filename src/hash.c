/** @file       hash.c
 *  @brief      A small hash library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  @todo Change to linear probing, making this into a generic table for use in the
 *        lisp interpreter, make a custom callback for hashing lisp code,
 *        simplifying all of the hash stuff instead of cons the key and value
 *        and storing that. **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/************************ small hash library **********************************/

static void null_free(void *p)
{
	UNUSED(p);
}

static int string_compare(const void *a, const void *b)
{
	return strcmp((const char*)a, (const char*)b);
}

static uint32_t string_hash(const void *s)
{
	return djb2(s, strlen(s));
}

static uint32_t hash_alg(const hash_table_t * table, const void *s)
{
	assert(table->len);
	return table->hash(s) % table->len;
}

static hash_entry_t *hash_new_pair(char *key, void *val)
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
	return hash_create_custom(len, null_free, null_free, string_compare, string_hash);
}

hash_table_t *hash_create_custom(size_t len, hash_free_key_f k, hash_free_val_f v, hash_compare_key_f c, hash_f h)
{
	hash_table_t *nt;
	if (!len)
		len++;
	if (!(nt = calloc(1, sizeof(*nt))))
		return NULL;
	if (!(nt->table = calloc(len, sizeof(*nt->table))))
		return free(nt), NULL;
	nt->len = len;
	nt->free_key = k;
	nt->free_val = v;
	nt->compare  = c;
	nt->hash     = h;
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
				h->free_key(cur->key);
				h->free_val(cur->val);
				free(prev);
			}
			free(prev);
		}
	free(h->table);
	free(h);
}

static hash_table_t *hash_copy_and_resize(hash_table_t *old, size_t len)
{
	hash_entry_t *cur;
	hash_table_t *new = hash_create(len);
	if(!new)
		return NULL;
	for (size_t i = 0; i < old->len; i++)
		if (old->table[i])
			for (cur = old->table[i]; cur; cur = cur->next)
				if (hash_insert(new, cur->key, cur->val) < 0)
					goto fail;
	new->free_key = old->free_key;
	new->free_val = old->free_val;
	new->compare = old->compare;
	new->hash = old->hash;
	return new;
fail:
	hash_destroy(new);
	return NULL;
}

hash_table_t *hash_copy(hash_table_t *src)
{
	assert(src);
	return hash_copy_and_resize(src, src->len);
}

static int hash_grow(hash_table_t * ht)
{
	size_t i;
	if((ht->len*2) < ht->len)
		return -1;
	hash_table_t *new = hash_copy_and_resize(ht, ht->len*2);
	hash_entry_t *cur, *prev;
	if (!new)
		return -1;
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
}

int hash_insert(hash_table_t * ht, char *key, void *val)
{
	assert(ht && key && val);
	uint32_t hash;
	hash_entry_t *cur, *newt, *last = NULL;

	if (hash_get_load_factor(ht) >= 0.75f)
		hash_grow(ht);

	hash = hash_alg(ht, key);

	for (cur = ht->table[hash]; cur && cur->key && ht->compare(key, cur->key); cur = cur->next)
		last = cur;

	if (cur && cur->key && !(ht->compare(key, cur->key))) {
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

void *hash_lookup(const hash_table_t * h, const char *key)
{
	uint32_t hash;
	hash_entry_t *cur;
	assert(h && key);

	hash = hash_alg(h, key);
	cur = h->table[hash];
	while (cur && cur->next && h->compare(cur->key, key))
		cur = cur->next;
	if (!cur || !cur->key || h->compare(cur->key, key))
		return NULL;
	else
		return cur->val;
}
