#ifndef INTERN_H_
#define INTERN_H_

#include "common.h"
#include "my_allocator.h"

struct haste_type_info;
struct haste_ast_node;
struct haste_object;

struct intern_pool {
	struct Allocator arena;
	struct Allocator allocator;

	struct intern_pool_strings {
		size_t cap, len;

		struct intern_pool_string_entries {
			uint64_t hash;
			const char *str;
			size_t len;
		} *items;
	} strings;

	struct intern_pool_types {
		size_t cap, len;

		struct intern_pool_types_entries {
			uint64_t hash;
			struct haste_type_info *info;
		} *items;
	} types;

	struct intern_pool_nodes {
		size_t cap, len;

		struct intern_pool_nodes_entries {
			uint32_t hash;
			struct haste_ast_node *node;
		} *items;
	} nodes;

	struct intern_pool_objects {
		size_t cap, len;

		struct intern_pool_objects_entries {
			uint64_t hash;
			struct haste_type_info *type_info;
			struct haste_object *obj;
		} *items;
	} objects;
};

struct intern_pool init_intern_pool(struct Allocator allocator, struct Allocator arena);
void deinit_intern_pool(struct intern_pool *pool);

const char *intern_str(struct intern_pool *pool, const char *start, size_t len);
const char *intern_cstr(struct intern_pool *pool, const char *str);

struct haste_ast_node *intern_node(struct intern_pool *pool, struct haste_ast_node *node);
struct haste_type_info *intern_type_info(struct intern_pool *pool, const struct haste_type_info *info);
struct haste_type_info *intern_type_info_unique(struct intern_pool *pool, const struct haste_type_info *info);

#endif
