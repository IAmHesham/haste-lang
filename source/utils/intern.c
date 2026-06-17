#include "haste.h"
#include "my_allocator.h"
#include "my_common.h"
#include "my_stream.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define INTERN_DEFAULT_CAP 256

#define setup(pool_, name_, cap_) \
	do { \
		(pool_).name_.cap = (cap_); \
		(pool_).name_.items = alloc((pool_).allocator, sizeof(*(pool_).name_.items) * (cap_)); \
		memset((pool_).name_.items, 0, sizeof(*(pool_).name_.items) * (cap_)); \
	} while (0)

#define deinit(pool_, name_) \
	do { \
		xdestroy((pool_)->allocator, sizeof(*(pool_)->name_.items) * (pool_)->name_.cap, (pool_)->name_.items); \
	} while (0)

struct intern_pool init_intern_pool(struct Allocator allocator, struct Allocator arena)
{
	struct intern_pool table = {
		.arena = arena,
		.allocator = allocator,
	};
	setup(table, strings, INTERN_DEFAULT_CAP);
	setup(table, types, INTERN_DEFAULT_CAP);
	setup(table, nodes, INTERN_DEFAULT_CAP);
	setup(table, objects, INTERN_DEFAULT_CAP);
	return table;
}

void deinit_intern_pool(struct intern_pool *pool)
{
	deinit(pool, strings);
	deinit(pool, types);
	deinit(pool, nodes);
	deinit(pool, objects);
	*pool = (struct intern_pool){0};
}

static uint64_t hash_bytes(const char *s, size_t len)
{
	uint64_t h = 1469598103934665603ULL;
	for (size_t i = 0; i < len; i++) {
		h ^= (unsigned char)s[i];
		h *= 1099511628211ULL;
	}
	return h;
}

static void grow_and_rehash(struct intern_pool *self)
{
	struct intern_pool_strings *strings = &self->strings;
	const size_t new_cap = strings->cap * 2;
	auto old_entries = strings->items;

	strings->items = alloc(self->allocator, sizeof(struct intern_pool_strings) * new_cap);
	memset(strings->items, 0, sizeof(struct intern_pool_strings) * new_cap);

	for (size_t i = 0; i < strings->cap; i++) {
		struct intern_pool_string_entries * old = &old_entries[i];
		if (old->str == NULL) continue;

		size_t idx = old->hash & (new_cap - 1);
		while (strings->items[idx].str != NULL) {
			idx = (idx + 1) & (new_cap - 1);
		}
		strings->items[idx] = *old;
	}

	xdestroy(self->allocator, sizeof(*strings->items) * strings->cap, old_entries);
	strings->cap = new_cap;
}

const char *intern_str(struct intern_pool *pool, const char *start, size_t len)
{
	if (start == NULL) return NULL;
	uint64_t h = hash_bytes(start, len);
	struct intern_pool_strings *strings = &pool->strings;

	if (strings->len >= (strings->cap * 7) / 10) {
		grow_and_rehash(pool);
	}

	size_t idx = h & (strings->cap - 1);

	while (true) {
		struct intern_pool_string_entries * e = &strings->items[idx];
		if (e->str == NULL) {
			char *copy = alloc(pool->arena, len + 1);
			memcpy(copy, start, len);
			copy[len] = '\0';
			e->hash = h;
			e->str  = copy;
			e->len  = len;
			strings->len++;
			return copy;
		}
		if (e->str == start || (e->hash == h && e->len == len &&
		    memcmp(e->str, start, len) == 0)) {
			return e->str;
		}

		idx = (idx + 1) & (strings->cap - 1);
	}
}

const char *intern_cstr(struct intern_pool *pool, const char *str)
{
	return intern_str(pool, str, strlen(str));
}

static size_t node_size(struct haste_ast_node *node);
static void intern_value(struct intern_pool *pool, struct haste_value *v);

static bool object_equal(struct haste_object *a, struct haste_object *b, struct haste_type_info *ti)
{
	if (a == b) return true;
	if (a->kind != b->kind) return false;
	if (a->kind == HASTE_OBJ_STRING) {
		struct haste_string_object *sa = (struct haste_string_object *)a;
		struct haste_string_object *sb = (struct haste_string_object *)b;
		return sa->len == sb->len && memcmp(sa->data, sb->data, sa->len) == 0;
	}
	for (size_t i = 0; i < ti->structure.len; i++) {
		if (!value_equal(((struct haste_struct_object *)a)->fields[i],
		                 ((struct haste_struct_object *)b)->fields[i])) return false;
	}
	return true;
}

static uint64_t hash_value(struct haste_value v)
{
	uint64_t h = (uint64_t)v.kind ^ ((uint64_t)(uintptr_t)v.type_info << 8);
	switch (v.kind) {
	case HASTE_VL_SCALAR:
		h ^= (uint64_t)v.integer * 1099511628211ULL;
		break;
	case HASTE_VL_TYPE:
		h ^= (uint64_t)(uintptr_t)v.type;
		break;
	case HASTE_VL_RUNTIME:
		h ^= (uint64_t)(uintptr_t)v.runtime;
		break;
	default:
		break;
	}
	return h ^ (h >> 32);
}

static uint64_t hash_object(struct haste_object *obj, struct haste_type_info *ti)
{
	if (obj == NULL) return 0;
	if (obj->kind == HASTE_OBJ_STRING) {
		struct haste_string_object *s = (struct haste_string_object *)obj;
		return hash_bytes(s->data, s->len);
	}
	uint64_t h = (uint64_t)(uintptr_t)ti;
	for (size_t i = 0; i < ti->structure.len; i++) {
		h ^= hash_value(((struct haste_struct_object *)obj)->fields[i]);
	}
	return h ^ (h >> 32);
}

static void grow_objects(struct intern_pool *self)
{
	struct intern_pool_objects *objects = &self->objects;
	const size_t new_cap = objects->cap * 2;
	auto old_entries = objects->items;

	objects->items = alloc(self->allocator, sizeof(struct intern_pool_objects_entries) * new_cap);
	memset(objects->items, 0, sizeof(struct intern_pool_objects_entries) * new_cap);

	for (size_t i = 0; i < objects->cap; i++) {
		struct intern_pool_objects_entries *old = &old_entries[i];
		if (old->obj == NULL) continue;

		size_t idx = old->hash & (new_cap - 1);
		while (objects->items[idx].obj != NULL) {
			idx = (idx + 1) & (new_cap - 1);
		}
		objects->items[idx] = *old;
	}

	xdestroy(self->allocator, sizeof(*objects->items) * objects->cap, old_entries);
	objects->cap = new_cap;
}

static struct haste_object *intern_object(struct intern_pool *pool, struct haste_object *obj, struct haste_type_info *ti)
{
	if (obj == NULL) return NULL;

	uint64_t h = hash_object(obj, ti);
	struct intern_pool_objects *objects = &pool->objects;

	if (objects->len >= (objects->cap * 7) / 10) {
		grow_objects(pool);
	}

	size_t idx = h & (objects->cap - 1);
	while (true) {
		struct intern_pool_objects_entries *e = &objects->items[idx];
		if (e->obj == NULL) {
			struct haste_object *copy;
			if (obj->kind == HASTE_OBJ_STRING) {
				struct haste_string_object *s = (struct haste_string_object *)obj;
				size_t sz = sizeof(struct haste_string_object) + s->len;
				copy = alloc(pool->arena, sz);
				memcpy(copy, obj, sz);
			} else {
				size_t n = ti->structure.len;
				size_t sz = sizeof(struct haste_struct_object) + sizeof(struct haste_value) * n;
				struct haste_struct_object *so = alloc(pool->arena, sz);
				memcpy(so, obj, sz);
				for (size_t i = 0; i < n; i++) {
					struct haste_value *field = &so->fields[i];
					intern_value(pool, field);
				}
				copy = &so->base;
			}

			e->hash = h;
			e->type_info = ti;
			e->obj  = copy;
			objects->len++;
			return copy;
		}
		if (object_equal(e->obj, obj, ti)) {
			return e->obj;
		}
		idx = (idx + 1) & (objects->cap - 1);
	}
}

static void intern_value(struct intern_pool *pool, struct haste_value *v)
{
	if (v->kind != HASTE_VL_OBJ) return;
	if (v->obj == NULL) return;

	v->obj = intern_object(pool, v->obj, v->type_info);
}

static void intern_strings(struct intern_pool *pool, size_t count, struct string *strings)
{
	if (strings == NULL) return;
	for (size_t i = 0; i < count; i++) {
		if (strings[i].chars != NULL) {
			strings[i].chars = intern_str(pool, strings[i].chars, strings[i].len);
		}
	}
}

struct haste_ast_node *intern_node(struct intern_pool *pool, struct haste_ast_node *node)
{
	if (node == NULL) return NULL;

	const size_t size = node_size(node);
	void *result = alloc(pool->arena, size >= sizeof(struct haste_ast_value) then size otherwise sizeof(struct haste_ast_value));
	memcpy(result, node, size);

	/* intern_value(pool, &((struct haste_ast_node *)result)->type.value); */

	/* switch (node->kind) { */
	/* case ND_VALUE: { */
	/* 	struct haste_ast_value *n = result; */
	/* 	intern_value(pool, &n->value); */
	/* 	break; */
	/* } */
	/* case ND_STRING_LIT: { */
	/* 	struct haste_ast_string_lit *n = result; */
	/* 	n->value.chars = intern_str(pool, n->value.chars, n->value.len); */
	/* 	break; */
	/* } */
	/* case ND_IDENT: { */
	/* 	struct haste_ast_ident *n = result; */
	/* 	n->value.chars = intern_str(pool, n->value.chars, n->value.len); */
	/* 	break; */
	/* } */
	/* case ND_ACCESS: { */
	/* 	struct haste_ast_access *n = result; */
	/* 	n->field.chars = intern_str(pool, n->field.chars, n->field.len); */
	/* 	break; */
	/* } */
	/* case ND_STRUCT_LIT_FIELD: { */
	/* 	struct haste_ast_struct_lit_field *n = result; */
	/* 	n->name.chars = intern_str(pool, n->name.chars, n->name.len); */
	/* 	break; */
	/* } */
	/* case ND_STRUCT_FIELD: { */
	/* 	struct haste_ast_struct_field *n = result; */
	/* 	if (n->names != NULL) { */
	/* 		struct string *new_names = alloc(pool->arena, sizeof(struct string) * n->name_count); */
	/* 		memcpy(new_names, n->names, sizeof(struct string) * n->name_count); */
	/* 		n->names = new_names; */
	/* 		intern_strings(pool, n->name_count, n->names); */
	/* 	} */
	/* 	if (n->name_locs != NULL) { */
	/* 		struct location *new_locs = alloc(pool->arena, sizeof(struct location) * n->name_count); */
	/* 		memcpy(new_locs, n->name_locs, sizeof(struct location) * n->name_count); */
	/* 		n->name_locs = new_locs; */
	/* 	} */
	/* 	break; */
	/* } */
	/* case ND_FUNC_PARAM: { */
	/* 	struct haste_ast_func_param *n = result; */
	/* 	if (n->names != NULL) { */
	/* 		struct string *new_names = alloc(pool->arena, sizeof(struct string) * n->name_count); */
	/* 		memcpy(new_names, n->names, sizeof(struct string) * n->name_count); */
	/* 		n->names = new_names; */
	/* 		intern_strings(pool, n->name_count, n->names); */
	/* 	} */
	/* 	if (n->name_locs != NULL) { */
	/* 		struct location *new_locs = alloc(pool->arena, sizeof(struct location) * n->name_count); */
	/* 		memcpy(new_locs, n->name_locs, sizeof(struct location) * n->name_count); */
	/* 		n->name_locs = new_locs; */
	/* 	} */
	/* 	break; */
	/* } */
	/* case ND_VAR_DECL: { */
	/* 	struct haste_ast_var_decl *n = result; */
	/* 	n->name.chars = intern_str(pool, n->name.chars, n->name.len); */
	/* 	break; */
	/* } */
	/* case ND_FUNC_DECL: { */
	/* 	struct haste_ast_func_decl *n = result; */
	/* 	n->name.chars = intern_str(pool, n->name.chars, n->name.len); */
	/* 	break; */
	/* } */
	/* default: */
	/* 	break; */
	/* } */

	return result;
}

static size_t node_size(struct haste_ast_node *node)
{
	switch (node->kind) {
	case ND_VALUE:            return sizeof(struct haste_ast_value);
	case ND_INTEGER_LIT:      return sizeof(struct haste_ast_integer_lit);
	case ND_FLOAT_LIT:        return sizeof(struct haste_ast_float_lit);
	case ND_STRING_LIT:       return sizeof(struct haste_ast_string_lit);
	case ND_IDENT:            return sizeof(struct haste_ast_ident);
	case ND_BINARY:           return sizeof(struct haste_ast_binary);
	case ND_UNARY:            return sizeof(struct haste_ast_unary);
	case ND_ACCESS:           return sizeof(struct haste_ast_access);
	case ND_INT_BITS:         return sizeof(struct haste_ast_int_bits);
	case ND_UINT_BITS:        return sizeof(struct haste_ast_uint_bits);
	case ND_GROUPING:         return sizeof(struct haste_ast_grouping);
	case ND_DISTINCT:         return sizeof(struct haste_ast_distinct);
	case ND_CAST:             return sizeof(struct haste_ast_cast);
	case ND_STRUCT_TYPE:      return sizeof(struct haste_ast_struct_type);
	case ND_STRUCT_FIELD:     return sizeof(struct haste_ast_struct_field);
	case ND_STRUCT_LITERAL:   return sizeof(struct haste_ast_struct_literal);
	case ND_STRUCT_LIT_FIELD: return sizeof(struct haste_ast_struct_lit_field);
	case ND_VAR_DECL:         return sizeof(struct haste_ast_var_decl);
	case ND_BLOCK:            return sizeof(struct haste_ast_block);
	/* default:                  return sizeof(struct haste_ast_node); */
	case ND_STRING:
	case ND_CSTR:
	case ND_INT:
	case ND_UINT:
	case ND_FLOAT:
	case ND_USIZE:
	case ND_VOID:
	case ND_AUTO:
	case ND_TYPE:
		return sizeof(struct haste_ast_node);
	}
}

// ── Type info interning ─────────────────────────────────────────────

static bool type_info_equal(const struct haste_type_info *a, const struct haste_type_info *b)
{
	if (a == b) return true;
	if (a->kind != b->kind) return false;
	if (a->bit_size != b->bit_size) return false;
	if (a->name != b->name && (a->name == NULL || b->name == NULL || strcmp(a->name, b->name) != 0)) return false;

	if (a->kind == HASTE_TY_STRUCT || a->kind == HASTE_TY_AUTO_STRUCT) {
		if (a->structure.len != b->structure.len) return false;
		for (size_t i = 0; i < a->structure.len; i++) {
			struct haste_struct_field *fa = &a->structure.items[i];
			assert(fa->name != NULL);
			struct haste_struct_field *fb = &b->structure.items[i];
			assert(fb->name != NULL);
			if (strcmp(fa->name, fb->name) != 0) return false;
			if (fa->type.value.type != fb->type.value.type) return false;
			if (IS_NONE(fa->default_value) != IS_NONE(fb->default_value)) return false;
			if (not IS_NONE(fa->default_value) && !value_equal(fa->default_value, fb->default_value)) return false;
		}
	}
	return true;
}

static uint64_t type_info_hash(const struct haste_type_info *ti)
{
	uint64_t h = (uint64_t)ti->kind;
	h = h * 31 + ti->bit_size;

	if (ti->name != NULL) {
		for (const char *p = ti->name; *p; p++)
			h = h * 31 + (unsigned char)*p;
	}

	if (ti->kind == HASTE_TY_STRUCT || ti->kind == HASTE_TY_AUTO_STRUCT) {
		h = h * 31 + ti->structure.len;
		for (size_t i = 0; i < ti->structure.len; i++) {
			struct haste_struct_field *f = &ti->structure.items[i];
			for (const char *p = f->name; *p; p++)
				h = h * 31 + (unsigned char)*p;
			h = h * 31 + (uint64_t)(uintptr_t)f->type.value.type;
		}
	}
	return h ^ (h >> 32);
}

static void grow_types(struct intern_pool *self)
{
	struct intern_pool_types *types = &self->types;
	const size_t new_cap = types->cap * 2;
	auto old_entries = types->items;

	types->items = alloc(self->allocator, sizeof(struct intern_pool_types_entries) * new_cap);
	memset(types->items, 0, sizeof(struct intern_pool_types_entries) * new_cap);

	for (size_t i = 0; i < types->cap; i++) {
		struct intern_pool_types_entries *old = &old_entries[i];
		if (old->info == NULL) continue;

		size_t idx = old->hash & (new_cap - 1);
		while (types->items[idx].info != NULL) {
			idx = (idx + 1) & (new_cap - 1);
		}
		types->items[idx] = *old;
	}

	xdestroy(self->allocator, sizeof(*types->items) * types->cap, old_entries);
	types->cap = new_cap;
}

static struct haste_type_info *intern_type_info_impl(struct intern_pool *pool, const struct haste_type_info *info, bool dedup)
{
	if (info == NULL) return NULL;

	uint64_t h = type_info_hash(info);
	struct intern_pool_types *types = &pool->types;

	if (types->len >= (types->cap * 7) / 10) {
		grow_types(pool);
	}

	size_t idx = h & (types->cap - 1);

	while (true) {
		struct intern_pool_types_entries *e = &types->items[idx];
		if (e->info == NULL) {
			struct haste_type_info *copy = alloc(pool->arena, sizeof(struct haste_type_info));
			*copy = *info;

			if (copy->kind == HASTE_TY_STRUCT || copy->kind == HASTE_TY_AUTO_STRUCT) {
				size_t n = copy->structure.len;
				struct haste_struct_field *items = alloc(pool->arena, sizeof(struct haste_struct_field) * n);
				memcpy(items, copy->structure.items, sizeof(struct haste_struct_field) * n);
				for (size_t i = 0; i < n; i++) {
					if (items[i].name != NULL)
						items[i].name = intern_str(pool, items[i].name, strlen(items[i].name));
				}
				copy->structure.items = items;
			}

			e->hash = h;
			e->info = copy;
			types->len++;
			return copy;
		}
		if (dedup && type_info_equal(e->info, info)) {
			return e->info;
		}
		idx = (idx + 1) & (types->cap - 1);
	}
}

struct haste_type_info *intern_type_info(struct intern_pool *pool, const struct haste_type_info *info)
{
	return intern_type_info_impl(pool, info, true);
}

struct haste_type_info *intern_type_info_unique(struct intern_pool *pool, const struct haste_type_info *info)
{
	return intern_type_info_impl(pool, info, false);
}
