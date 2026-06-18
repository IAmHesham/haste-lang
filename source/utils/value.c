#include "utils/value.h"
#include "haste.h"
#include "my_allocator.h"
#include "my_common.h"
#include "my_stream.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define create_node(pool_, T_, ...) \
	(void*)_create_node((pool_), &(T_) { __VA_ARGS__ })

inline static void *_create_node(struct intern_pool *pool, void *value)
{
	return intern_node(pool, value);
}

struct haste_type ty_int            = {0};
struct haste_type ty_uint           = {0};
struct haste_type ty_zero           = {0};
struct haste_type ty_unknown        = {0};
struct haste_type ty_type           = {0};
struct haste_type ty_untyped_int    = {0};
struct haste_type ty_float          = {0};
struct haste_type ty_untyped_float  = {0};
struct haste_type ty_auto           = {0};
struct haste_type ty_void           = {0};
struct haste_type ty_untyped_string = {0};
struct haste_type ty_string         = {0};
struct haste_type ty_cstr           = {0};
struct haste_type ty_usize          = {0};

static struct haste_type_info *intern_int_type(struct intern_pool *pool, uint16_t bits, bool is_signed)
{
	size_t bytes = (bits + 7) / 8;
	struct haste_type_info info = {
		.kind = is_signed ? HASTE_TY_INT : HASTE_TY_UINT,
		.is_integer = true,
		.is_unsigned = !is_signed,
		.bit_size = bits,
		.size = bytes,
		.align = bytes < 8 ? bytes : 8,
	};
	return intern_type_info(pool, &info);
}

struct haste_value type_get_int(struct intern_pool *pool, uint16_t bits, bool is_signed)
{
	struct haste_type_info *ti = intern_int_type(pool, bits, is_signed);
	return VAL_TYPE(ti);
}

void setup_builtins(struct intern_pool *pool)
{
	struct haste_type_info *ti;

	ti = intern_type_info(pool, &(struct haste_type_info){
		.kind = HASTE_TY_TYPE, .size = 8, .align = 8, .name = "type" });
	ty_type = into_type(VAL_TYPE(ti));
	ty_type.value.type_info = ti;

#define REGISTER_BUILTIN(val_, ...) \
	do { \
		struct haste_type_info _info_ = { __VA_ARGS__ }; \
		struct haste_type_info *_interned_ = intern_type_info(pool, &_info_); \
		(val_) = into_type(VAL_TYPE(_interned_)); \
	} while (0)

	REGISTER_BUILTIN(ty_zero,
					 .kind = HASTE_TY_ZERO,
					 .name = "zero");
	REGISTER_BUILTIN(ty_unknown,
					 .kind = HASTE_TY_UNKNOWN,
					 .name = "uninit");
	REGISTER_BUILTIN(ty_untyped_int,
					 .kind = HASTE_TY_UNTYPED_INT,
					 .size = 4, .align = 4, .name = "untyped_int",
					 .is_integer = true, .is_untyped = true);
	REGISTER_BUILTIN(ty_float,
					 .kind = HASTE_TY_FLOAT,
					 .size = 4, .align = 4, .name = "float",
					 .is_float = true);
	REGISTER_BUILTIN(ty_untyped_float,
					 .kind = HASTE_TY_UNTYPED_FLOAT,
					 .size = 4, .align = 4, .name = "untyped_float",
					 .is_float = true, .is_untyped = true);
	REGISTER_BUILTIN(ty_auto,
					 .kind = HASTE_TY_AUTO,
					 .name = "auto");
	REGISTER_BUILTIN(ty_void,
					 .kind = HASTE_TY_VOID,
					 .name = "void");
	REGISTER_BUILTIN(ty_untyped_string,
					 .kind = HASTE_TY_UNTYPED_STRING,
					 .size = 8, .align = 8, .name = "untyped_string",
					 .is_string = true, .is_untyped = true);
	REGISTER_BUILTIN(ty_cstr,
					 .kind = HASTE_TY_CSTR,
					 .size = 8, .align = 4, .name = "cstr",
					 .is_string = true);
	REGISTER_BUILTIN(ty_usize,
					 .kind = HASTE_TY_USIZE,
					 .size = 8, .align = 8, .name = "usize",
					 .is_integer = true, .is_unsigned = true);

	{
		struct haste_struct_field *string_fields = alloc(
			pool->arena,
			sizeof(struct haste_struct_field) * 2);
		string_fields[0] = (struct haste_struct_field){
			.name = "ptr",
			.type = ty_cstr,
		};
		string_fields[1] = (struct haste_struct_field){
			.name = "len",
			.type = ty_usize,
		};
		ti = intern_type_info(pool, &(struct haste_type_info){
			.kind = HASTE_TY_STRUCT,
			.structure = {
				.len = 2,
				.items = string_fields,
			},
			.is_string = true,
			.name = "string",
		});
		ty_string = into_type(VAL_TYPE(ti));
	}

	{
		ty_int = into_type(VAL_TYPE(intern_int_type(pool, 32, true)));
		AS_TYPE_INFO(ty_int)->name = "int";
	}

	{
		ty_uint = into_type(VAL_TYPE(intern_int_type(pool, 32, false)));
		AS_TYPE_INFO(ty_uint)->name = "uint";
	}
}

bool type_is_builtin(struct haste_type ty)
{
	struct haste_type_info *ti = AS_TYPE_INFO(ty);
	return ti != NULL && ti->kind != HASTE_TY_STRUCT
		&& ti->kind != HASTE_TY_AUTO_STRUCT;
}

enum arith_op {
	ARITH_ADD,
	ARITH_SUB,
	ARITH_MUL,
	ARITH_DIV,
};

#define value_is_any_int(v) \
	(IS_SCALAR(v) and type_is_integer(typeof_value(v)))

#define value_is_any_float(v) \
	(IS_SCALAR(v) and type_is_float(typeof_value(v)))

static struct haste_value arith_float(enum arith_op op, struct haste_value lhs, struct haste_value rhs)
{
	double a = value_is_any_float(lhs) then lhs.floating otherwise (double)lhs.integer;
	double b = value_is_any_float(rhs) then rhs.floating otherwise (double)rhs.integer;
	double res = 0.0;

	switch (op) {
	case ARITH_ADD: res = a + b; break;
	case ARITH_SUB: res = a - b; break;
	case ARITH_MUL: res = a * b; break;
	case ARITH_DIV:
		if (b == 0.0) return VAL_BAD_ERROR(ERR_DIVISION_BY_ZERO);
		res = a / b;
		break;
	}

	if (type_is_float(typeof_value(lhs)) or type_is_untyped_float(typeof_value(rhs)))
		return VAL_SCALAR(AS_TYPE_INFO(ty_float), .floating = res);

	return VAL_SCALAR(AS_TYPE_INFO(ty_untyped_float), .floating = res);
}

static struct haste_value arith_int(enum arith_op op, struct haste_value lhs, struct haste_value rhs)
{
	int64_t a = lhs.integer;
	int64_t b = rhs.integer;
	int64_t res = 0;

	switch (op) {
	case ARITH_ADD:
		if ((b > 0 && a > INT64_MAX - b) or (b < 0 && a < INT64_MIN - b)) {
			return VAL_BAD_ERROR(ERR_ARITH_OVERFLOW);
		}
		res = a + b;
		break;
	case ARITH_SUB:
		if ((b > 0 && a < INT64_MIN + b) or (b < 0 && a > INT64_MAX + b)) {
			return VAL_BAD_ERROR(ERR_ARITH_OVERFLOW);
		}
		res = a - b;
		break;
	case ARITH_MUL:
		if (a != 0 and b != 0) {
			if ((a > 0 and b > 0 and a > INT64_MAX / b)
			    or (a > 0 and b < 0 and b < INT64_MIN / a)
			    or (a < 0 and b > 0 and a < INT64_MIN / b)
			    or (a < 0 and b < 0 and a < INT64_MAX / b)) {
				return VAL_BAD_ERROR(ERR_ARITH_OVERFLOW);
			}
		}
		res = a * b;
		break;
	case ARITH_DIV:
		if (b == 0) {
			return VAL_BAD_ERROR(ERR_DIVISION_BY_ZERO);
		}
		if (a == INT64_MIN and b == -1) {
			return VAL_BAD_ERROR(ERR_ARITH_OVERFLOW);
		}
		res = a / b;
		break;
	}

	if (type_equal(typeof_value(lhs), typeof_value(rhs))) {
		return VAL_SCALAR(lhs.type_info, .integer = res);
	}

	return VAL_SCALAR(AS_TYPE_INFO(ty_untyped_int), .integer = res);
}

static struct haste_value value_do_arith(
	struct intern_pool *pool,
	enum token_kind op_kind,
	struct location op_loc,
	const enum arith_op op,
	struct haste_value lhs,
	struct haste_value rhs)
{
	if (IS_ZERO(lhs)) lhs = VAL_SCALAR(AS_TYPE_INFO(ty_untyped_int), .integer = 0);
	if (IS_ZERO(rhs)) rhs = VAL_SCALAR(AS_TYPE_INFO(ty_untyped_int), .integer = 0);

	if (IS_RUNTIME(lhs) or IS_RUNTIME(rhs)) {
		struct haste_ast_node *lhs_node = IS_RUNTIME(lhs) ? lhs.runtime : node_into_value(pool, NULL, lhs);
		struct haste_ast_node *rhs_node = IS_RUNTIME(rhs) ? rhs.runtime : node_into_value(pool, NULL, rhs);

		struct haste_type lt = typeof_value(lhs);
		struct haste_type rt = typeof_value(rhs);
		struct haste_type result_type = type_is_untyped(lt) ? rt : lt;

		struct haste_ast_binary *bin = alloc(pool->arena, sizeof(struct haste_ast_binary));
		*bin = (struct haste_ast_binary){
			.base.kind = ND_BINARY,
			.base.type = result_type,
			.base.analyzed = true,
			.base.location = op_loc,
			.lhs = lhs_node,
			.rhs = rhs_node,
			.op = op_kind,
			.op_loc = op_loc,
		};
		struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)bin);
		result.type_info = AS_TYPE_INFO(result_type);
		return result;
	}

	if (not (value_is_any_int(lhs) or value_is_any_float(lhs))
	    or not (value_is_any_int(rhs) or value_is_any_float(rhs)))
		return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);

	if (not type_is_untyped(typeof_value(lhs))
	    and not type_is_untyped(typeof_value(rhs))
	    and not type_equal(typeof_value(lhs), typeof_value(rhs)))
		return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);

	if (value_is_any_float(lhs) or value_is_any_float(rhs))
		return arith_float(op, lhs, rhs);

	return arith_int(op, lhs, rhs);
}

#define DEFINE_ARITH(name, op) \
	struct haste_value name(struct intern_pool *pool, enum token_kind op_kind, struct location op_loc, const struct haste_value lhs, const struct haste_value rhs) \
	{ return value_do_arith(pool, op_kind, op_loc, op, lhs, rhs); }

DEFINE_ARITH(value_add, ARITH_ADD)
DEFINE_ARITH(value_sub, ARITH_SUB)
DEFINE_ARITH(value_mul, ARITH_MUL)
DEFINE_ARITH(value_div, ARITH_DIV)

struct haste_value value_unary(
    struct intern_pool *pool,
    enum token_kind op,
    struct location op_loc,
    const struct haste_value value)
{
    if (IS_RUNTIME(value)) {
        if (op == TK_MINUS or op == TK_PLUS) {
            struct haste_ast_unary *un = alloc(pool->arena, sizeof(struct haste_ast_unary));
            *un = (struct haste_ast_unary){
                .base.kind = ND_UNARY,
                .base.type = typeof_value(value),
                .base.analyzed = true,
                .base.location = op_loc,
                .rhs = value.runtime,
                .op = op,
                .op_loc = op_loc,
            };
            struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)un);
            result.type_info = AS_TYPE_INFO(typeof_value(value));
            return result;
        }
        return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);
    }

    if (not (IS_ZERO(value) or IS_SCALAR(value)))
        return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);

    switch (op) {
    case TK_PLUS:
        return value;
    case TK_MINUS:
        if (IS_ZERO(value))
            return VAL_ZERO;
        if (type_is_integer(typeof_value(value))) {
            struct haste_value result = VAL_SCALAR(value.type_info, .integer = -value.integer);
            result.is_explicitly_comptime = value.is_explicitly_comptime;
            return result;
        }
        if (type_is_float(typeof_value(value))) {
            struct haste_value result = VAL_SCALAR(value.type_info, .floating = -value.floating);
            result.is_explicitly_comptime = value.is_explicitly_comptime;
            return result;
        }
        return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);
    default:
        return VAL_BAD_ERROR(ERR_INCOMPATIBLE_ARITH_TYPES);
    }
}

struct haste_value value_implicit_cast(struct intern_pool *pool, const struct haste_type to, const struct haste_value value)
{
	struct haste_type from = typeof_value(value);

	if (type_equal(to, from)) {
		return value;
	}

	if (type_is_untyped_integer(from) and type_is_number(to)) {
		if (type_is_float(to)) {
		return VAL_SCALAR(AS_TYPE_INFO(to), .floating = (double)value.integer);
		}
		return VAL_SCALAR(AS_TYPE_INFO(to), .integer = value.integer);
	}
	if (type_is_untyped_float(from) and type_is_float(to)) {
		return VAL_SCALAR(AS_TYPE_INFO(to), .floating = value.floating);
	}

	if (type_equal(from, ty_untyped_string)) {
		if (type_equal(to, ty_string)) {
			struct haste_string_object *s = (struct haste_string_object*)value.obj;
			struct haste_value so = make_value(pool, to);
			struct_set_field(pool, &so, (size_t)0,
				VAL_OBJ(AS_TYPE_INFO(ty_cstr), value.obj));
			struct_set_field(pool, &so, (size_t)1,
				VAL_SCALAR(AS_TYPE_INFO(ty_usize), .integer = (int64_t)s->len));
			return so;
		}
		if (type_equal(to, ty_cstr)) {
			return VAL_OBJ(AS_TYPE_INFO(to), value.obj);
		}
	}

	return VAL_BAD_ERROR(ERR_INVALID_IMPLICIT_CAST);
}

typedef struct {
	int64_t as_int;
	double as_float;
	bool is_float;
} RawNumber;

static RawNumber extract_raw(struct haste_value value)
{
	if (type_is_integer(typeof_value(value)))   return (RawNumber){ .as_int = value.integer,           .as_float = (double)value.integer, };
	if (type_is_float(typeof_value(value))) return (RawNumber){ .as_int = (int64_t)value.floating, .as_float = value.floating, };
	unreachable();
}

static struct haste_value construct_from_raw(struct haste_type to, RawNumber raw)
{
	if (type_is_integer(to))
		return VAL_SCALAR(AS_TYPE_INFO(to), .integer = raw.as_int);

	if (type_is_float(to))
		return VAL_SCALAR(AS_TYPE_INFO(to), .floating = raw.as_float);

	unreachable();
}

static struct haste_value value_cast_string_to_struct(struct intern_pool *pool, const struct haste_type to, const struct haste_value value)
{
	struct haste_string_object *s = (struct haste_string_object*)value.obj;
	struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(to);
	struct haste_value so = make_value(pool, to);
	struct_set_field(pool, &so, (size_t)0, value_cast(pool, st->items[0].type, value));
	struct_set_field(pool, &so, (size_t)1, VAL_SCALAR(AS_TYPE_INFO(ty_usize), .integer = (int64_t)s->len));
	return so;
}

static struct haste_value value_cast_auto_struct(
	struct intern_pool *pool,
	const struct haste_type to,
	const struct haste_value value,
	const struct haste_type value_type)
{
	const struct haste_struct_type_info *to_st = AS_STRUCT_TYPE_INFO(to);
	const struct haste_struct_type_info *val_st = AS_STRUCT_TYPE_INFO(value_type);
	const struct haste_struct_object *val_so = AS_STRUCT(value);

	struct haste_value result = default_for_type(pool, to);
	struct haste_struct_object *so = AS_STRUCT(result);

	for (size_t i = 0; i < to_st->len; i += 1) {
		for (size_t j = 0; j < val_st->len; j += 1) {
			if (strcmp(to_st->items[i].name, val_st->items[j].name) == 0) {
                struct haste_value cv = value_cast(pool, to_st->items[i].type, val_so->fields[j]);
                fprintf(stderr, "CAST_AUTO_STRUCT field=%s idx=%zu typeof(cv)=%d is_bad=%d\n", to_st->items[i].name, i, cv.kind, IS_BAD(cv));
				so->fields[i] = cv;
				break;
			}
		}
	}
	return result;
}

static struct haste_value value_cast_runtime(struct intern_pool *pool, const struct haste_type to, const struct haste_value value)
{
	if (type_equal(to, typeof_value(value))) return value;

	struct haste_ast_cast *cast_node = alloc(pool->arena, sizeof(struct haste_ast_cast));
	*cast_node = (struct haste_ast_cast){
		.base.kind = ND_CAST,
		.base.type = to,
		.to = NULL,
		.expr = value.runtime,
	};
	struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)cast_node);
	result.type_info = AS_TYPE_INFO(to);
	return result;
}

struct haste_value value_cast(
	struct intern_pool *pool,
	const struct haste_type to,
	const struct haste_value value)
{
	const struct haste_type value_type = typeof_value(value);

	if (IS_BAD(value))                     return value;
	if (IS_BAD(into_value(to)))            return into_value(to);

	if (type_is_untyped(to))               unreachable();
	if (IS_RUNTIME(value))                 return value_cast_runtime(pool, to, value);
	if (type_equal(to, ty_auto))           return value;
	if (type_equal(to, value_type))        return value;

	{
		struct haste_value implicit = value_implicit_cast(pool, to, value);
		if (not IS_BAD(implicit)) return implicit;
	}

	if (value_equal(value, VAL_UNINIT))    return default_for_type(pool, to);
	if (IS_ZERO(value))                    return zero_for_type(pool, to);

	if (type_equal(to, ty_string) and IS_OBJ(value) and value.obj->kind == HASTE_OBJ_STRING)
		return value_cast_string_to_struct(pool, to, value);

	if (type_equal(to, ty_cstr) and type_equal(to, ty_string)) {
		ptrdiff_t idx = find_named_field(typeof_value(value), "ptr");
		assert(idx >= 0);
		return AS_STRUCT(value)->fields[(size_t)idx];
	}

	if (type_is_any_string(to) and IS_OBJ(value) and value.obj->kind == HASTE_OBJ_STRING)
		return VAL_OBJ(AS_TYPE_INFO(to), value.obj);

	if (IS_STRUCT_TYPE(to) and IS_AUTO_STRUCT_TYPE(value_type))
		return value_cast_auto_struct(pool, to, value, value_type);

	if (not type_is_number(to) or not type_is_number(value_type)) {
        if (type_is_float(to) && type_is_any_string(value_type)) {
            fprintf(stderr, "BUG DETECTED: Returning INVALID_CAST for string to float! IS_BAD(ret) = %d\n", IS_BAD(VAL_BAD_ERROR(ERR_INVALID_CAST)));
        }
		return VAL_BAD_ERROR(ERR_INVALID_CAST);
    }

	return construct_from_raw(to, extract_raw(value));
}

struct haste_value value_assign(struct intern_pool *pool, struct haste_value *lvalue, struct haste_value rvalue)
{
	if (not lvalue->is_lvalue) {
		return VAL_BAD_ERROR(ERR_INVALID_ASSIGNMET);
	}

	struct haste_type lhs_type = typeof_value(*lvalue);

	struct haste_value result = value_implicit_cast(pool, lhs_type, rvalue);
	if (IS_BAD(result)) {
		if (IS_ZERO(rvalue)) {
			result = zero_for_type(pool, lhs_type);
		} else if (IS_UNINIT(rvalue)) {
			result = default_for_type(pool, lhs_type);
		} else if (IS_STRUCT_TYPE(lhs_type) and IS_AUTO_STRUCT_TYPE(typeof_value(rvalue))) {
			result = value_cast(pool, lhs_type, rvalue);
		}
	}
	if (IS_BAD(result)) return result;

	if (IS_STRUCT(*lvalue) and IS_STRUCT(result)) {
		struct haste_struct_object *lso = AS_STRUCT(*lvalue);
		struct haste_struct_object *rso = AS_STRUCT(result);
		struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(lhs_type);
		iarreach (i, *st) {
			lso->fields[i] = rso->fields[i];
		}
	} else {
		*lvalue = result;
	}

	result.is_lvalue = false;
	return result;
}

struct haste_value value_coerce(struct intern_pool *pool, const struct haste_type to, const struct haste_value value)
{
	struct haste_value slot = make_value(pool, to);
	slot.is_lvalue = true;
	return value_assign(pool, &slot, value);
}

struct haste_object *create_struct(struct Allocator alloc, struct haste_struct_type_info *st)
{
	assert(st != NULL);

	struct haste_struct_object *so = alloc(
		alloc,
		sizeof(struct haste_struct_object) +
		sizeof(struct haste_value) *
		SAFE_COUNT(st->len));
	so->base.kind = HASTE_OBJ_STRUCT;
	memset(so->fields, 0, sizeof(struct haste_value) * st->len);

	iarreach (i, *st) {
		struct haste_struct_field field = st->items[i];
		if (not IS_NONE(field.default_value)) {
			so->fields[i] = field.default_value;
		} else {
			so->fields[i] = VAL_NONE;
		}
	}

	return (void*)so;
}

struct haste_object *create_string(struct Allocator alloc, const char *str, size_t len)
{
	struct haste_string_object *so = alloc(
		alloc,
		sizeof(struct haste_string_object) +
		sizeof(char) * len);
	so->base.kind = HASTE_OBJ_STRING;
	so->len = len;
	memcpy(so->data, str, so->len);
	return (void*)so;
}

//
// haste_value_builder
//
struct haste_value_builder value_builder(struct intern_pool *pool)
{
	return (struct haste_value_builder) {
		.pool = pool,
	};
}

void free_value_builder(struct haste_value_builder *builder)
{
	arrfree(builder->pool->allocator, *builder);
}

void value_builder_push(struct haste_value_builder *builder, struct haste_value value)
{
	arrpush(builder->pool->allocator, *builder, value);
}

void value_builder_set(struct haste_value_builder *builder, size_t index, struct haste_value value)
{
	if (index >= builder->cap) {
		size_t old_cap = builder->cap;
		size_t new_cap = old_cap ? old_cap : 8;
		while (new_cap <= index) new_cap *= 2;
		builder->items = xrecreate(builder->pool->allocator, old_cap * sizeof(struct haste_value), new_cap * sizeof(struct haste_value), builder->items);
		builder->cap = new_cap;
		memset(builder->items + old_cap, 0, (new_cap - old_cap) * sizeof(struct haste_value));
	}
	builder->items[index] = value;
	if (index >= builder->len) builder->len = index + 1;
}

struct haste_value build_value(struct haste_value_builder *builder, struct haste_type type)
{
	struct haste_type_info *ti = AS_TYPE_INFO(type);
	if (ti->kind == HASTE_TY_STRUCT || ti->kind == HASTE_TY_AUTO_STRUCT) {
		struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);
		struct haste_struct_object *so = (void*)create_struct(builder->pool->arena, st);

		for (size_t i = 0; i < builder->len && i < st->len; i++) {
			if (not IS_NONE(builder->items[i])) {
				so->fields[i] = builder->items[i];
			}
		}

		free_value_builder(builder);
		return VAL_OBJ(ti, so);
	}
	unreachable();
}

static bool object_equal(struct haste_object *a, struct haste_object *b)
{
	if (a == b) return true;
	if (a->kind != b->kind) return false;
	switch (a->kind) {
	case HASTE_OBJ_STRING: {
		struct haste_string_object *sa = (struct haste_string_object*)a;
		struct haste_string_object *sb = (struct haste_string_object*)b;
		return sa->len == sb->len and memcmp(sa->data, sb->data, sa->len) == 0;
	}
	case HASTE_OBJ_STRUCT:
		return false;
	}
	return false;
}

bool value_equal(struct haste_value a, struct haste_value b)
{
	switch (a.kind) {
	case HASTE_VL_NONE:
	case HASTE_VL_BAD:           return false;
	case HASTE_VL_UNINIT:        return b.kind == HASTE_VL_UNINIT;
	case HASTE_VL_ZERO:
		return b.kind == HASTE_VL_ZERO or value_equal(b, VAL_SCALAR(AS_TYPE_INFO(ty_untyped_int), .integer = 0));
	case HASTE_VL_SCALAR:
		if (not IS_SCALAR(b)) return false;
		if (value_is_any_float(a) or value_is_any_float(b)) {
			double fa = value_is_any_float(a) then a.floating otherwise (double)a.integer;
			double fb = value_is_any_float(b) then b.floating otherwise (double)b.integer;
			return fa == fb;
		}
		return a.integer == b.integer;
	case HASTE_VL_RUNTIME:
		return false;
	case HASTE_VL_TYPE:
		if (not IS_TYPE(b)) {
			return false;
		}
		return type_equal(into_type(a), into_type(b));
	case HASTE_VL_OBJ:
		if (not IS_OBJ(b)) return false;
		return object_equal(a.obj, b.obj);
	}
}

bool is_comptime_known(const struct haste_value v)
{
	switch (v.kind) {
	case HASTE_VL_SCALAR:
	case HASTE_VL_ZERO:
	case HASTE_VL_OBJ:
		return true;
	default:
		return false;
	}
}

bool struct_has_field_name(const struct haste_value value, const char *name)
{
	return find_named_field(typeof_value(value), name) >= 0;
}

static struct haste_value require_struct_value(
	const struct haste_value *value,
	struct haste_struct_type_info **st,
	struct haste_struct_object **so)
{
	if (not IS_STRUCT(*value))
		return VAL_BAD_ERROR(ERR_NOT_A_STRUCT);
	struct haste_type type = typeof_value(*value);
	*st = AS_STRUCT_TYPE_INFO(type);
	*so = AS_STRUCT(*value);
	return VAL_NONE;
}

struct haste_value value_access(
	struct intern_pool *pool,
	const struct haste_value value,
	const struct string str)
{
	struct haste_type type = typeof_value(value);
	if (not IS_STRUCT_TYPE(type) and not IS_AUTO_STRUCT_TYPE(type)) {
		return VAL_BAD_ERROR(ERR_NOT_A_STRUCT);
	}
	struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);

	if (IS_RUNTIME(value)) {
		ssize_t idx = find_named_field(type, str.chars);
		if (idx < 0) {
			return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);
		}

		struct haste_type field_type = st->items[idx].type;

		struct haste_ast_node *node = create_node(
			pool, struct haste_ast_access,
			.base.kind = ND_ACCESS,
			.base.type = field_type,
			.base.analyzed = true,
			.field_index = (size_t)idx);
		struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)node);
		result.is_lvalue = value.is_lvalue;
		result.type_info = AS_TYPE_INFO(field_type);
		return result;
	}

	if (not IS_STRUCT(value)) {
		return VAL_BAD_ERROR(ERR_NOT_A_STRUCT);
	}

	ssize_t idx = find_named_field(type, str.chars);
	if (idx < 0) {
		return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);
	}
	return AS_STRUCT(value)->fields[(size_t)idx];
}

struct haste_value struct_get_field_by_name(
	const struct haste_value value,
	const char *name)
{
	struct haste_struct_type_info *st;
	struct haste_struct_object *so;
	struct haste_value req = require_struct_value(&value, &st, &so);
	if (IS_BAD(req)) return req;

	const ptrdiff_t idx = find_named_field(typeof_value(value), name);
	if (idx < 0)
		return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);

	return struct_get_field_by_index(value, (size_t)idx);
}

struct haste_value struct_get_field_by_index(const struct haste_value value,
											 const size_t idx)
{
	struct haste_struct_type_info *st;
	struct haste_struct_object *so;
	struct haste_value req = require_struct_value(&value, &st, &so);
	if (IS_BAD(req)) return req;

	if (idx >= st->len)
		return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);

	return so->fields[idx];
}

struct haste_value struct_set_field_by_name(struct intern_pool *pool,
											struct haste_value *value,
											const char *name,
											const struct haste_value new_value)
{
	const ptrdiff_t idx = find_named_field(typeof_value(*value), name);
	if (idx < 0)
		return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);

	return struct_set_field_by_index(pool, value, (size_t)idx, new_value);
}

struct haste_value struct_set_field_by_index(struct intern_pool *pool,
											 struct haste_value *value,
											 const size_t idx,
											 const struct haste_value new_value)
{
	struct haste_struct_type_info *st;
	struct haste_struct_object *so;
	struct haste_value req = require_struct_value(value, &st, &so);
	if (IS_BAD(req)) return req;

	if (idx >= st->len) {
		return VAL_BAD_ERROR(ERR_FIELD_DOESNT_EXIST);
	}

	struct haste_value casted = value_cast(pool, st->items[idx].type, new_value);
	if (IS_BAD(casted)) {
		return VAL_BAD_ERROR(ERR_INVALID_ASSIGNMET);
	}

	so->fields[idx] = casted;
	return *value;
}

int print_object(stream_t stream, const struct haste_object *obj, struct haste_type type)
{
	int printed_amount = 0;
	switch (obj->kind) {
	case HASTE_OBJ_STRING: {
		struct haste_string_object *s = (struct haste_string_object*)obj;
		printed_amount += sprint(stream, "{s:*}", s->data, (int)s->len);
	} break;
	case HASTE_OBJ_STRUCT: {
		struct haste_struct_object *so = OAS_STRUCT(obj);
		struct haste_type_info *type_info = AS_TYPE_INFO(type);
		struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);
		printed_amount += sprint(stream, "{s} {", type_info->name then type_info->name otherwise "auto");
		for (size_t i=0; i<st->len; i += 1) {
			struct haste_struct_field field = st->items[i];
			struct haste_value field_value = so->fields[i];
			printed_amount += sprint(stream, "{s}: {value},", field.name, field_value);
		}
		printed_amount += sprint(stream, "}");
	} break;
	}
	return printed_amount;
}

int print_value(stream_t stream, const struct haste_value value)
{
	int printed_amount = 0;

	switch (value.kind) {
	case HASTE_VL_NONE:
		printed_amount += sprint(stream, "NONE");
		break;
	case HASTE_VL_BAD:
		printed_amount += sprint(stream, "BAD");
		break;
	case HASTE_VL_ZERO:
		printed_amount += sprint(stream, "ZERO");
		break;
	case HASTE_VL_UNINIT:
		printed_amount += sprint(stream, "UNINIT");
		break;
	case HASTE_VL_SCALAR:
		if (value_is_any_float(value))
			printed_amount += sprint(stream, "{lf}", value.floating);
		else
			printed_amount += sprint(stream, "{i64}", value.integer);
		break;
	case HASTE_VL_RUNTIME:
		printed_amount += print_haste_ast(stream, value.runtime);
		break;
	case HASTE_VL_TYPE: {
		struct haste_type_info *type = AS_TYPE_INFO(into_type(value));
		if (type->name) {
			printed_amount += sprint(stream, "{s}", type->name);
		} else if (type->kind == HASTE_TY_INT or type->kind == HASTE_TY_UINT) {
			char buf[32];
			snprintf(buf, sizeof(buf), "%sint%zu", type->kind == HASTE_TY_UINT ? "u" : "", type->bit_size);
			printed_amount += sprint(stream, "{s}", buf);
		} else {
			printed_amount += sprint(stream, "auto_struct");
		}
	} break;
	case HASTE_VL_OBJ:
		printed_amount += sprint(stream, "{obj}", value);
		break;
	}

	return printed_amount;
}
