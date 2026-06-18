#include "haste.h"
#include "my_array.h"
#include "my_common.h"
#include "my_stream.h"
#include <__stddef_unreachable.h>
#include <assert.h>
#include <stdio.h>


#define IS_UNKNOWN(type) \
	type_equal(type, ty_unknown)

#define IS_ZERO_TYPE(type) \
	type_equal(type, ty_zero)

static struct haste_string_object _default_empty_string = {
	.base = { .kind = HASTE_OBJ_STRING },
	.len = 0,
};

struct haste_type into_type(struct haste_value value)
{
	if (IS_BAD(value) or IS_NONE(value)) {
		return (struct haste_type) { value };
	}
	ASSERT_IS_TYPE(value);
	return (struct haste_type) { value };
}

struct haste_value into_value(struct haste_type type)
{
	return type.value;
}

static struct haste_value make_struct_default(struct intern_pool *pool, struct haste_type type, bool force_all)
{
	struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);
	struct haste_struct_object *so = (void*)create_struct(pool->arena, st);
	for (size_t i = 0; i < st->len; i += 1) {
		if (force_all or IS_NONE(so->fields[i])) {
			so->fields[i] = default_for_type(pool, st->items[i].type);
		}
	}
	return VAL_OBJ(AS_TYPE_INFO(type), so);
}

struct haste_value zero_for_type(struct intern_pool *pool, struct haste_type to)
{
	if (IS_STRUCT_TYPE(to))
		return make_struct_default(pool, to, true);
	return default_for_type(pool, to);
}

struct haste_value default_for_type(struct intern_pool *pool, struct haste_type type)
{
	if (type_is_integer(type)) return VAL_SCALAR(AS_TYPE_INFO(type), .integer = 0);
	if (type_is_float(type))   return VAL_SCALAR(AS_TYPE_INFO(type), .floating = 0.0f);
	if (type_equal(type, ty_cstr))
		return VAL_OBJ(AS_TYPE_INFO(type), &_default_empty_string);
	if (IS_STRUCT_TYPE(type))
		return make_struct_default(pool, type, false);
	unreachable();
}

struct haste_type_builder type_builder(
	struct intern_pool *pool,
	enum haste_compound_kind kind)
{
	return (struct haste_type_builder) {
		.pool = pool,
		.kind = kind,
		.is_auto = false,
	};
}

void free_type_builder(struct haste_type_builder *builder)
{
	arrfree(builder->pool->allocator, *builder);
}

struct haste_value add_field(
	struct haste_type_builder *builder,
	struct string name,
	struct haste_type type,
	struct haste_value default_value)
{
	if (builder->kind != HASTE_TYB_STRUCT) {
		return VAL_BAD_ERROR(ERR_NOT_A_STRUCT_OR_TUPLE_OR_UNION);
	}

	// Check if the field already exists by a simple linear search
	// NOTE: the amount of fields are usally small so a linear search
	//       will not be slow
	arreach (struct haste_struct_field, field, *builder) {
		if (strcmp(field.name, name.chars) == 0) {
			return VAL_BAD_ERROR(ERR_FIELD_DUPLICATION);
		}
	}

	if (type_equal(type, ty_auto)) {
		if (IS_NONE(default_value)) {
			return VAL_BAD_ERROR(ERR_NOT_TYPE);
		}
		type = typeof_value(default_value);
	}

	type = untyped_to_typed(type);
	if (not IS_NONE(default_value)) {
		default_value = value_coerce(builder->pool, type, default_value);
		if (IS_BAD(default_value)) {
			return VAL_BAD_ERROR(ERR_INVALID_ASSIGNMET);
		}
	}

	arrpush(builder->pool->allocator, *builder, (struct haste_struct_field){
			.name = name.chars,
			.type = type,
			.default_value = default_value,
		});

	return VAL_NONE;
}

struct haste_type build_type(struct haste_type_builder *builder)
{
	struct haste_type_info type_info = {0};

	// First we turn haste_type_builder into haste_type
	switch (builder->kind) {
	case HASTE_TYB_STRUCT:
		type_info.kind = builder->is_auto ? HASTE_TY_AUTO_STRUCT : HASTE_TY_STRUCT;
		type_info.name = NULL;
		type_info.structure.items = builder->items;
		type_info.structure.len = builder->len;
		break;
	}

	struct haste_type_info *result = intern_type_info(builder->pool, &type_info);
	free_type_builder(builder);
	return into_type(VAL_TYPE(result));
}

ptrdiff_t find_named_field(const struct haste_type tp, const char *name)
{
	if (not IS_STRUCT_TYPE(tp) and not IS_AUTO_STRUCT_TYPE(tp)) {
		return -1;
	}

	struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(tp);
	for (size_t i = 0; i < st->len; i += 1) {
		if (strcmp(st->items[i].name, name) == 0) {
			return i;
		}
	}

	return -1;
}

struct haste_type typeof_value(const struct haste_value value)
{
	if (IS_NONE(value)) {
		unreachable();
	}
	if (IS_BAD(value)) {
		return into_type(VAL_BAD);
	}
	return into_type(VAL_TYPE(value.type_info));
	/* switch (value.kind) { */
	/* case HASTE_VL_NONE: */
	/* 	unreachable(); */
	/* case HASTE_VL_BAD:     return into_type(VAL_BAD); */
	/* case HASTE_VL_ZERO:    return ty_zero; */
	/* case HASTE_VL_UNINIT:  return ty_unknown; */
	/* case HASTE_VL_RUNTIME: return value.runtime->type; */
	/* case HASTE_VL_TYPE: */
	/* case HASTE_VL_SCALAR: */
	/* case HASTE_VL_OBJ: */
	/* 	return into_type(VAL_TYPE(value.type_info)); */
	/* } */
	/* unreachable(); */
}

struct haste_value make_value(struct intern_pool *pool, const struct haste_type type)
{
	struct haste_type_info *type_info = AS_TYPE_INFO(type);

	if (type_info->kind == HASTE_TY_STRUCT
		or type_info->kind == HASTE_TY_AUTO_STRUCT) {
		struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);
		struct haste_struct_object *so = (void*)create_struct(pool->arena, st);
		return VAL_OBJ(AS_TYPE_INFO(type), so);
	}

	return default_for_type(pool, type);
}

bool type_equal(const struct haste_type v1,
                const struct haste_type v2)
{
	return v1.value.type == v2.value.type;
}

uint64_t type_hash(const struct haste_type t)
{
	const struct haste_type_info *ot = AS_TYPE_INFO(t);
	uint64_t h = ot->kind;

	if (ot->name != NULL)
		for (const char *p = ot->name; *p; p += 1)
			h = h * 31 + (unsigned char)*p;

	if (ot->kind == HASTE_TY_STRUCT || ot->kind == HASTE_TY_AUTO_STRUCT) {
		const struct haste_struct_type_info *st = &ot->structure;
		for (size_t i = 0; i < st->len; i += 1) {
			for (const char *p = st->items[i].name; *p; p += 1)
				h = h * 31 + (unsigned char)*p;
			h = h * 31 + (uint64_t)(uintptr_t)st->items[i].type.value.type;
		}
	}

	return h;
}

bool type_is_any_string(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_string;
}

struct haste_type untyped_to_typed(struct haste_type type)
{
	if (type_equal(type, ty_untyped_int))       return ty_int;
	if (type_equal(type, ty_untyped_float))     return ty_float;
	if (type_equal(type, ty_untyped_string))    return ty_string;
	if (IS_ZERO_TYPE(type))              return ty_int;
	return type;
}

bool type_is_integer(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_integer;
}

bool type_is_float(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_float;
}

bool type_is_untyped_float(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_float and AS_TYPE_INFO(t)->is_untyped;
}

bool type_is_number(const struct haste_type t)
{
	return type_is_integer(t) or type_is_float(t);
}

bool type_is_untyped(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_untyped;
}

bool type_is_untyped_integer(const struct haste_type t)
{
	return AS_TYPE_INFO(t)->is_untyped and AS_TYPE_INFO(t)->is_integer;
}

bool type_is_untyped_number(const struct haste_type t)
{
	return type_is_number(t) and type_is_untyped(t);
}

bool haste_is_default_empty_string(const struct haste_object *obj)
{
	return obj == &_default_empty_string.base;
}
