#include "ht.h"
#include "../hash/spookyhash/spookyhash.h"

#include <stdio.h>

#include <limits.h>

#define TEST_NULL_TABLE(t_x) \
	if(t_x == 0) \
	{ \
		return HT_NULL_TABLE; \
	}

#define TEST_NULL_KEY(k_x) \
	if(k_x == 0) \
	{ \
		return HT_NULL_KEY; \
	}

#define TEST_NULL_VALUE(v_x) \
	if(v_x == 0) \
	{ \
		return HT_NULL_VALUE; \
	}

#define TEST_NULL_ITERATOR(i_x) \
	if(i_x == 0) \
	{ \
		return HT_NULL_ITERATOR; \
	}

#define TEST_NULL_PREFIX(p_x) \
	if(p_x == 0) \
	{ \
		return HT_NULL_PREFIX; \
	}

////////////////////////////////////////
//	HASH DIFFUSE
////////////////////////////////////////
static
inline
uint32_t
diffuse64_32
(
	uint64_t	h
)
{
	uint32_t *m;
	m = (uint32_t *) &h;

	return m[0] ^ m[1];
}

static
inline
uint64_t
diffuse128_64
(
	uint64_t	h1,
	uint64_t	h2
)
{
	return h1 ^ h2;
}

static
inline
uint32_t
diffuse128_32
(
	uint64_t	h1,
	uint64_t	h2
)
{
	uint32_t m1 = diffuse64_32(h1);
	uint32_t m2 = diffuse64_32(h2);
	return m1 ^ m2;
}

struct ht_prefix
{
	void			*prefix;
	size_t			 prefix_length;
	spookyhash_state_t	*state;
};

////////////////////////////////////////
//	HASH TABLE STRUCTS
////////////////////////////////////////
typedef struct ht_entry_t ht_entry_t;
struct ht_entry_t
{
	uint8_t		 *key;
	size_t		  key_length;
	void		 *value;
	size_t		  value_length;
	ht_entry_t		 *next;
};

struct ht_t
{
	ht_entry_t		**table;
	size_t		  table_length;
	ht_hash_size_t	  hash_size;
	size_t		  num_of_entries;
	ht_seed_t	  seed;
	void		 *extra;
	void		(*destroy_value)
			(
				void	*data,
				void	*extra
			);
	void		(*destroy_extra)
			(
				void	*extra
			);
};

////////////////////////////////////////
//	HASHES
////////////////////////////////////////
typedef union
{
	uint32_t	h32;
	uint64_t	h64;
	uint64_t	h128[2];
} ht_hash_t;

static
inline
ht_hash_t
ht_hash
(
	ht_t		*ht,
	uint8_t		*key,
	size_t		 key_length,
	ht_prefix	*prefix
)
{
	ht_seed_t seed = ht->seed;
	ht_hash_t storage;

	switch(ht->hash_size)
	{
		case HT_HASH_SIZE_32:
			{
				uint32_t h;
				if(prefix == 0)
				{
					h = spookyhash32(key,key_length,seed.s32);
				}
				else
				{
					uint64_t h1;
					uint64_t h2;
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
					h = (uint32_t) h1;
				}
				storage.h32 = h;
			}
			break;
		case HT_HASH_SIZE_64:
			{
				uint64_t h;
				if(prefix == 0)
				{
					h = spookyhash64(key,key_length,seed.s64);
				}
				else
				{
					uint64_t h1;
					uint64_t h2;
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
					h = h1;
				}
				storage.h64 = h;
			}
			break;
		case HT_HASH_SIZE_64_DIFFUSE_32:
			{
				uint64_t h;
				if(prefix == 0)
				{
					h = spookyhash64(key,key_length,seed.s64);
				}
				else
				{
					uint64_t h1;
					uint64_t h2;
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
					h = h1;
				}
				storage.h32 = diffuse64_32(h);
			}
			break;
		case HT_HASH_SIZE_128:
			{
				uint64_t h1 = seed.s128[0];
				uint64_t h2 = seed.s128[1];
				if(prefix == 0)
				{
					spookyhash128(key,key_length,&h1,&h2);
				}
				else
				{
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
				}
				storage.h128[0] = h1;
				storage.h128[1] = h2;
			}
			break;
		case HT_HASH_SIZE_128_DIFFUSE_64:
			{
				uint64_t h1 = seed.s128[0];
				uint64_t h2 = seed.s128[1];
				if(prefix == 0)
				{
					spookyhash128(key,key_length,&h1,&h2);
				}
				else
				{
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
				}
				storage.h64 = diffuse128_64(h1,h2);
			}
			break;
		case HT_HASH_SIZE_128_DIFFUSE_32:
			{
				uint64_t h1 = seed.s128[0];
				uint64_t h2 = seed.s128[1];
				if(prefix == 0)
				{
					spookyhash128(key,key_length,&h1,&h2);
				}
				else
				{
					spookyhash_state_t s;
					spookyhash_clone_state(prefix->state,&s);
					spookyhash_update(&s,key,key_length);
					spookyhash_final(&s,&h1,&h2);
				}
				storage.h32 = diffuse128_32(h1,h2);
			}
			break;
	}

	return storage;
}

static
inline
size_t
ht_index
(
	ht_t		*ht,
	ht_hash_t	 hash
)
{
	size_t i = (size_t) -1;
	size_t l = ht->table_length;

	switch(ht->hash_size)
	{
		case HT_HASH_SIZE_32:
		case HT_HASH_SIZE_64_DIFFUSE_32:
		case HT_HASH_SIZE_128_DIFFUSE_32:
			i = hash.h32 % l;
			break;
		case HT_HASH_SIZE_128_DIFFUSE_64:
		case HT_HASH_SIZE_64:
			i = hash.h64 % l;
			break;
		case HT_HASH_SIZE_128:
			i = hash.h128[1] % l;
			break;
	}

	return i;
}

static
int
ht_v_compare
(
	const void	*aa,
	const void	*bb
)
{
	if(aa == 0 || bb == 0)
	{
		return -1;
	}

	const ht_entry_t *a = aa;
	const ht_entry_t *b = bb;

	if(a->key_length != b->key_length)
	{
		return -1;
	}

	return strncmp((char *)a->key,(char *)b->key,a->key_length);
}

static
void
ht_v_destroy
(
	ht_entry_t	*v,
	ht_t	*ht
)
{
	if(v->key)
	{
		free(v->key);
	}
	if(v->value)
	{
		if(ht->destroy_value)
		{
			ht->destroy_value(v->value,ht->extra);
		}
	}

	free(v);
}

static
inline
ht_entry_t *
ht_v_find
(
	ht_entry_t		 *list,
	ht_entry_t		 *v,
	int		(*compare)
			(
				const void	*aa,
				const void	*bb
			)
)
{
	ht_entry_t *next = list;
	while(next)
	{
		if(0 == compare(next,v))
		{
			return next;
		}
		next = next->next;
	}
	return 0;
}

static
inline
void
ht_v_append
(
	ht_t	*ht,
	int	 index,
	ht_entry_t	*v
)
{
	ht_entry_t *vs = ht->table[index];

	if(vs == 0)
	{
		ht->table[index] = v;
		return;
	}

	while(vs->next)
	{
		vs = vs->next;
	}

	vs->next = v;
}

static
void
ht_vacuous_free
(
	void	*data,
	void	*extra
)
{
	free(data);
}

////////////////////////////////////////////////////////////////////////////////
//	CREATION AND DESTRUCTION
////////////////////////////////////////////////////////////////////////////////
ht_t *
ht_create
(
	size_t		table_length,
	ht_hash_size_t	hash_size,
	ht_seed_t	seed
)
{
	return ht_create_full(table_length,hash_size,seed,0,ht_vacuous_free,free);
}

ht_t *
ht_create_full
(
	size_t		table_length,
	ht_hash_size_t	hash_size,
	ht_seed_t	seed,
	void		*extra,
	void		(*destroy_value)
			(
				void	*data,
				void	*extra
			),
	void		(*destroy_extra)
			(
				void	*extra
			)
)
{
	if(table_length <= 0)
	{
		return 0;
	}

	ht_t *h = malloc(sizeof(ht_t));

	*h = (ht_t) {
		.table_length		= table_length,
		.hash_size		= hash_size,
		.seed			= seed,
		.num_of_entries		= 0,
		.extra			= extra,
		.destroy_value		= destroy_value,
		.destroy_extra		= destroy_extra,
	};

	h->table = malloc(table_length * sizeof(ht_entry_t *));

	for(int i = 0; i < table_length; i++)
	{
		h->table[i] = 0;
	}

	return h;
}

void
ht_destroy
(
	ht_t	*ht
)
{
	if(ht == 0)
	{
		return;
	}

	ht_clear_table(ht);

	if(ht->extra)
	{
		if(ht->destroy_extra)
		{
			ht->destroy_extra(ht->extra);
		}
	}

	free(ht->table);

	free(ht);
}


////////////////////////////////////////////////////////////////////////////////
//	MODIFICATION
////////////////////////////////////////////////////////////////////////////////
ht_status_t
ht_add_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);
	TEST_NULL_VALUE(value);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	void *k;
	size_t kl = 0;
	if(prefix != 0)
	{
		kl = key_length + prefix->prefix_length;
		k = malloc(kl);
		memcpy(k,prefix->prefix,prefix->prefix_length);
		memcpy(k + prefix->prefix_length,key,key_length);
	}
	else
	{
		kl = key_length;
		k = malloc(kl);
		memcpy(k,key,kl);
	}

	ht_entry_t *v = malloc(sizeof(ht_entry_t));
	*v = (ht_entry_t) {
		.key = (uint8_t *) k,
		.key_length = kl,
		.value = (void *) value,
		.value_length = value_length,
		.next = 0,
	};

	ht_entry_t *data = ht_v_find(vs,v,ht_v_compare);

	if(data != 0)
	{
		free(v);
		return HT_KEY_ALREADY_IN_USE;
	}

	ht_v_append(ht,index,v);
	ht->num_of_entries++;

	return HT_SUCCESS;
}

ht_status_t
ht_update_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);
	TEST_NULL_VALUE(value);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	void *k = malloc(key_length);
	memcpy(k,key,key_length);

	ht_entry_t *v = malloc(sizeof(ht_entry_t));
	*v = (ht_entry_t) {
		.key = (uint8_t *) k,
		.key_length = key_length,
		.value = (void *) value,
		.value_length = value_length,
	};

	ht_entry_t *data = ht_v_find(vs,v,ht_v_compare);

	if(data == 0)
	{
		ht_v_append(ht,index,v);
		ht->num_of_entries++;
	}
	else
	{
		data->value = value;
		data->value_length = value_length;
		free(k);
		free(v);
	}

	return HT_SUCCESS;
}

ht_status_t
ht_update_strict_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);
	TEST_NULL_VALUE(value);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	ht_entry_t *v = malloc(sizeof(ht_entry_t));
	*v = (ht_entry_t) {
		.key = (uint8_t *) key,
		.key_length = key_length,
		.value = (void *) value,
		.value_length = value_length,
	};

	ht_entry_t *data = ht_v_find(vs,v,ht_v_compare);

	free(v);

	if(data == 0)
	{
		return HT_KEY_NOT_IN_USE;
	}

	data->value = value;
	data->value_length = value_length;

	return HT_SUCCESS;
}

ht_status_t
ht_get_with_prefix
(
	ht_t		 *ht,
	const void	 *key,
	size_t		  key_length,
	void		**destination,
	size_t		 *value_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	ht_entry_t v = {
		.key = (uint8_t *) key,
		.key_length = key_length,
	};

	ht_entry_t *data = ht_v_find(vs,&v,ht_v_compare);

	if(data == 0)
	{
		return HT_KEY_NOT_IN_USE;
	}

	if(destination != 0)
	{
		*destination = data->value;
		*value_length = data->value_length;
	}

	return HT_SUCCESS;
}

ht_status_t
ht_get_copy_with_prefix
(
	ht_t		 *ht,
	const void	 *key,
	size_t		  key_length,
	void		**destination,
	size_t		 *value_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	ht_entry_t v = {
		.key = (uint8_t *) key,
		.key_length = key_length,
	};

	ht_entry_t *data = ht_v_find(vs,&v,ht_v_compare);

	if(data == 0)
	{
		return HT_KEY_NOT_IN_USE;
	}

	if(destination != 0)
	{
		void *value = malloc(data->value_length);
		strncpy(value,data->value,data->value_length);
		*destination = value;
		*value_length = data->value_length;
	}

	return HT_SUCCESS;
}

ht_status_t
ht_remove_with_prefix
(
	ht_t		*ht,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_KEY(key);

	ht_hash_t hash = ht_hash(ht,(uint8_t *)key,key_length,prefix);

	size_t index = ht_index(ht,hash);

	ht_entry_t *vs = ht->table[index];

	ht_entry_t v = {
		.key = (uint8_t *) key,
		.key_length = key_length,
	};

	ht_entry_t *data = ht_v_find(vs,&v,ht_v_compare);

	if(data == 0)
	{
		return HT_KEY_NOT_IN_USE;
	}

	if(data == vs)
	{
		ht_v_destroy(data,ht);

		ht->table[index] = 0;

		ht->num_of_entries--;


		return HT_SUCCESS;
	}

	ht_entry_t *prev = vs;
	while(prev)
	{
		if(prev->next == data)
		{
			break;
		}
		prev = prev->next;
	}

	prev->next = data->next;

	ht_v_destroy(data,ht);

	ht->num_of_entries--;

	return HT_SUCCESS;
}

ht_status_t
ht_clear_table
(
	ht_t		*ht
)
{
	TEST_NULL_TABLE(ht);

	size_t l = ht->table_length;

	for(int i = 0; i < l; i++)
	{
		ht_entry_t *data = ht->table[i];

		while(data)
		{
			ht_entry_t *next = data->next;

			ht_v_destroy(data,ht);

			data = next;
		}

		ht->table[i] = 0;
	}

	ht->num_of_entries = 0;

	return HT_SUCCESS;
}

ht_status_t
ht_resize_table
(
	ht_t		*ht,
	size_t		 table_length
)
{
	TEST_NULL_TABLE(ht);

	size_t l = ht->table_length;
	ht_entry_t **ot = ht->table;

	ht_entry_t **nt = malloc(table_length * sizeof(ht_entry_t *));
	for(int i = 0; i < table_length; i++)
	{
		nt[i] = 0;
	}

	ht->table = nt;
	ht->table_length = table_length;

	for(int i = 0; i < l; i++)
	{
		ht_entry_t *data = ot[i];

		while(data)
		{
			ht_entry_t *next = data->next;

			ht_add(
				ht,
				data->value,
				data->value_length,
				data->key,
				data->key_length
			);

			ht_v_destroy(data,ht);

			data = next;
		}
	}

	free(ot);

	return HT_SUCCESS;
}

ht_status_t
ht_iterate
(
	ht_t	*ht,
	void	(*function)
		(
			void	*value,
			size_t	 value_length,
			void	*key,
			size_t	 key_length,
			size_t	 index
		)
)
{
	TEST_NULL_TABLE(ht);
	TEST_NULL_ITERATOR(function);

	size_t l = ht->table_length;

	for(int i = 0; i < l; i++)
	{
		ht_entry_t *data = ht->table[i];

		while(data)
		{
			function(
				data->value,
				data->value_length,
				data->key,
				data->key_length,
				i
			);

			data = data->next;
		}
	}

	return HT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//	PREFIX
////////////////////////////////////////////////////////////////////////////////
ht_prefix *
ht_create_prefix
(
	void	*key_prefix,
	size_t	 key_prefix_length
)
{
	ht_prefix *p = malloc(sizeof(ht_prefix));

	void *kp = malloc(key_prefix_length);
	memcpy(kp,key_prefix,key_prefix_length);

	spookyhash_state_t *h = malloc(sizeof(spookyhash_state_t));
	spookyhash_init(h,0,0);
	spookyhash_update(h,key_prefix,key_prefix_length);

	*p = (ht_prefix) {
		.prefix		= kp,
		.prefix_length	= key_prefix_length,
		.state		= h,
	};

	return p;
}

ht_prefix *
ht_clone_prefix
(
	ht_prefix	*prefix
)
{
	ht_prefix *p = malloc(sizeof(ht_prefix));

	void *key_prefix = malloc(prefix->prefix_length);
	memcpy(key_prefix,prefix->prefix,prefix->prefix_length);

	spookyhash_state_t *state = malloc(sizeof(spookyhash_state_t));
	spookyhash_clone_state(prefix->state,state);

	*p = (ht_prefix) {
		.prefix		= key_prefix,
		.prefix_length	= prefix->prefix_length,
		.state		= state,
	};

	return p;
}

ht_prefix *
ht_append_prefix
(
	ht_prefix	*prefix,
	void		*key_prefix_appendage,
	size_t		 key_prefix_appendage_length
)
{
	ht_prefix *p = ht_clone_prefix(prefix);

	spookyhash_update(p->state,key_prefix_appendage,key_prefix_appendage_length);

	size_t pl = p->prefix_length + key_prefix_appendage_length;
	p->prefix = realloc(p->prefix,pl);
	memcpy(p->prefix + p->prefix_length,key_prefix_appendage,key_prefix_appendage_length);

	p->prefix_length += key_prefix_appendage_length;

	return p;
}

ht_status_t
ht_destroy_prefix
(
	ht_prefix	*prefix
)
{
	TEST_NULL_PREFIX(prefix);

	free(prefix->prefix);
	free(prefix->state);

	return HT_SUCCESS;
}

ht_status_t
ht_prefix_key
(
	ht_prefix	 *prefix,
	void		**key_prefix_dest,
	size_t		 *key_prefix_length
)
{
	TEST_NULL_PREFIX(prefix);

	void *k = malloc(prefix->prefix_length);
	memcpy(k,prefix->prefix,prefix->prefix_length);

	*key_prefix_dest = k;
	*key_prefix_length = prefix->prefix_length;

	return HT_SUCCESS;
}
