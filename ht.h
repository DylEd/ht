#ifndef __HT
#define __HT

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct ht_t ht_t;
typedef int ht_status_t;

typedef struct ht_prefix ht_prefix;

typedef enum
{
	HT_HASH_SIZE_32 = 32,
	HT_HASH_SIZE_64 = 64,
	HT_HASH_SIZE_64_DIFFUSE_32 = 65,
	HT_HASH_SIZE_128 = 128,
	HT_HASH_SIZE_128_DIFFUSE_64 = 129,
	HT_HASH_SIZE_128_DIFFUSE_32 = 130,
} ht_hash_size_t;

enum
{
	HT_SUCCESS = 0,
	HT_NULL_TABLE,
	HT_NULL_KEY,
	HT_NULL_VALUE,
	HT_NULL_ITERATOR,
	HT_NULL_PREFIX,
	HT_KEY_ALREADY_IN_USE,
	HT_KEY_NOT_IN_USE,
	HT_CANNOT_SCALE_FIXED_LENGTH_TABLE,
	HT_NONPOSITIVE_LENGTH,
};

typedef union
{
	uint32_t	s32;
	uint64_t	s64;
	uint64_t	s128[2];
} ht_seed_t;


////////////////////////////////////////////////////////////////////////////////
//	CREATION AND DESTRUCTION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//	create a hash table
//	- will free values with free
////////////////////////////////////////////////////////////
ht_t *
ht_create
(
	size_t		table_length,
	ht_hash_size_t	hash_size,
	ht_seed_t	seed
);

////////////////////////////////////////////////////////////
//	create a hash table
//	- 0 may be specified for destroy functions,
//		in which case the function will not
//		be called
////////////////////////////////////////////////////////////
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
);

////////////////////////////////////////////////////////////
//	free a hash table
//	- key will be copied, value will not be
//	- value and extra will be destroyed using
//		the functions provided by the user
////////////////////////////////////////////////////////////
void
ht_destroy
(
	ht_t	*table
);


////////////////////////////////////////////////////////////////////////////////
//	MODIFICATION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//	add a value,key pair to the hash table
//	- key must not be in use
////////////////////////////////////////////////////////////
ht_status_t
ht_add_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
);
#define ht_add(ht,value,value_length,key,key_length) \
	ht_add_with_prefix(ht,value,value_length,key,key_length,0)

////////////////////////////////////////////////////////////
//	change value referenced by key
//	- if key is not in use, then add it
////////////////////////////////////////////////////////////
ht_status_t
ht_update_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
);
#define ht_update(ht,value,value_length,key,key_length) \
	ht_update_with_prefix(ht,value,value_length,key,key_length,0)

////////////////////////////////////////////////////////////
//	same as ht_update but does not add if the key
//		is not in use
////////////////////////////////////////////////////////////
ht_status_t
ht_update_strict_with_prefix
(
	ht_t		*ht,
	void		*value,
	size_t		 value_length,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*prefix
);
#define ht_update_strict(ht,value,value_length,key,key_length) \
	ht_update_strict_with_prefix(ht,value,value_length,key,key_length,0)

////////////////////////////////////////////////////////////
//	get value from key
//	- stored in destination
//	- destination can be NULL
//		- return value indicates key is in use
////////////////////////////////////////////////////////////
ht_status_t
ht_get_with_prefix
(
	ht_t		 *ht,
	const void	 *key,
	size_t		  key_length,
	void		**destination,
	size_t		 *value_length,
	ht_prefix	 *prefix
);
#define ht_get(ht,key,key_length,destination,value_length) \
	ht_get_with_prefix(ht,key,key_length,destination,value_length,0)

////////////////////////////////////////////////////////////
//	get copy of value from key
//	- stored in destination
//	- destination can be NULL
//		- return value indicates key is in use
//	- must be freed by the user
////////////////////////////////////////////////////////////
ht_status_t
ht_get_copy_with_prefix
(
	ht_t		 *ht,
	const void	 *key,
	size_t		  key_length,
	void		**destination,
	size_t		 *value_length,
	ht_prefix	 *state
);
#define ht_get_copy(ht,key,key_length,destination,value_length) \
	ht_get_copy_with_prefix(ht,key,key_length,destination,value_length,0)

////////////////////////////////////////////////////////////
//	remove an element from the table using a key
////////////////////////////////////////////////////////////
ht_status_t
ht_remove_with_prefix
(
	ht_t		*ht,
	const void	*key,
	size_t		 key_length,
	ht_prefix	*state
);
#define ht_remove(ht,key,key_length) \
	ht_remove_with_prefix(ht,key,key_length,0)

////////////////////////////////////////////////////////////
//	remove every element from the table
////////////////////////////////////////////////////////////
ht_status_t
ht_clear_table
(
	ht_t	*ht
);

////////////////////////////////////////////////////////////
//	resize the table
////////////////////////////////////////////////////////////
ht_status_t
ht_resize_table
(
	ht_t	*ht,
	size_t	 num_of_entries
);

////////////////////////////////////////////////////////////
//	for each element in the table, pass it through a
//		function
////////////////////////////////////////////////////////////
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
);

////////////////////////////////////////////////////////////////////////////////
//	PREFIX
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//	create a prefix
//	- key_prefix is the initial portion of a key value
////////////////////////////////////////////////////////////
ht_prefix *
ht_create_prefix
(
	void	*key_prefix,
	size_t	 key_prefix_length
);

////////////////////////////////////////////////////////////
//	clone a prefix
////////////////////////////////////////////////////////////
ht_prefix *
ht_clone_prefix
(
	ht_prefix	*prefix
);

////////////////////////////////////////////////////////////
//	append to a prefix
//	- generates new prefix
//	- key prefix value will be equal to the key prefix
//		value of the original prefix appended with
//		the key prefix appendage
////////////////////////////////////////////////////////////
ht_prefix *
ht_append_prefix
(
	ht_prefix	*prefix,
	void		*key_prefix_appendage,
	size_t		 key_prefix_appendage_length
);

////////////////////////////////////////////////////////////
//	destroy a prefix
////////////////////////////////////////////////////////////
ht_status_t
ht_destroy_prefix
(
	ht_prefix	*prefix
);

////////////////////////////////////////////////////////////
//	retrieve a copy of the key prefix and its length
////////////////////////////////////////////////////////////
ht_status_t
ht_prefix_key
(
	ht_prefix	 *prefix,
	void		**key_prefix_dest,
	size_t		 *key_prefix_length
);

#endif /* __HT */
