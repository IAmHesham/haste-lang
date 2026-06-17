#ifndef VALUE_H_
#define VALUE_H_

#include "common.h"
#include "location.h"
#include "my_allocator.h"
#include "my_stream.h"

#include <assert.h>

struct intern_pool;
struct haste_type_info;

#define STANDARD_BITWIDTH_LIMIT 128

enum haste_value_error {
	ERR_ANY,

	ERR_NOT_TYPE,

	ERR_INCOMPATIBLE_ARITH_TYPES,
	ERR_ARITH_OVERFLOW,
	ERR_DIVISION_BY_ZERO,

	ERR_INVALID_IMPLICIT_CAST,
	ERR_INVALID_CAST,

	ERR_INVALID_ASSIGNMET,

	ERR_NOT_A_STRUCT,
	ERR_FIELD_DOESNT_EXIST,

	ERR_NOT_A_STRUCT_OR_TUPLE_OR_UNION,
	ERR_FIELD_DUPLICATION,
};

enum haste_value_kind {
	HASTE_VL_NONE,
	HASTE_VL_BAD,
	HASTE_VL_ZERO,
	HASTE_VL_UNINIT,
	HASTE_VL_SCALAR,
	HASTE_VL_RUNTIME,
	HASTE_VL_TYPE,
	HASTE_VL_OBJ,
};

struct haste_value {
	enum haste_value_kind kind : 6;
	bool is_explicitly_comptime : 1;
	bool is_lvalue : 1;
	struct haste_type_info *type_info;

	union {
		enum haste_value_error error_code;
		int64_t integer;
		double floating;
		struct haste_type_info *type;
		struct haste_ast_node *runtime;
		struct haste_object *obj;
	};
};

struct haste_type {
	struct haste_value value;
};

struct haste_object {
	enum {
		HASTE_OBJ_STRING,
		HASTE_OBJ_STRUCT,
	} kind : 8;
};

struct haste_string_object {
	struct haste_object base;
	size_t len;
	char data[];
};

struct haste_struct_object {
	struct haste_object base;
	struct haste_value fields[];
};

#define VAL_NONE                  ((struct haste_value) { .kind = HASTE_VL_NONE })
#define VAL_BAD                   ((struct haste_value) { .kind = HASTE_VL_BAD  })
#define VAL_ZERO                  ((struct haste_value) { .kind = HASTE_VL_ZERO, .type_info = AS_TYPE_INFO(ty_zero) })
#define VAL_UNINIT                ((struct haste_value) { .kind = HASTE_VL_UNINIT, .type_info = AS_TYPE_INFO(ty_unknown) })
#define VAL_BAD_ERROR(err_)       ((struct haste_value) { .kind = HASTE_VL_BAD, .error_code = (err_) })
#define VAL_SCALAR(ti, ...)       ((struct haste_value) { .kind = HASTE_VL_SCALAR, .type_info = (ti), __VA_ARGS__ })
#define VAL_RUNTIME(...)          ((struct haste_value) { .kind = HASTE_VL_RUNTIME, .runtime = (__VA_ARGS__) })
#define VAL_TYPE(ti)              ((struct haste_value) { .kind = HASTE_VL_TYPE, .type_info = (ty_type.value.type_info), .type = (ti) })
#define VAL_OBJ(ti, p)            ((struct haste_value) { .kind = HASTE_VL_OBJ, .type_info = (ti), .obj = (struct haste_object*)(void*)(p) })

#define TYPE_INFO(...)             ((struct haste_type_info) { __VA_ARGS__ })
#define STRUCT_TYPE_INFO(...)      ((struct haste_struct_type_info) { .kind = HASTE_TY_STRUCT, __VA_ARGS__ })
#define AUTO_STRUCT_TYPE_INFO(...) ((struct haste_struct_type_info) { .kind = HASTE_TY_AUTO_STRUCT, __VA_ARGS__ })

#define OBJ_STRUCT(...)           ((struct haste_struct_object) { .base = OBJ_TYPE(HASTE_TY_STRUCT), __VA_ARGS__ })

#define IS_NONE(...)              ((__VA_ARGS__).kind == HASTE_VL_NONE)
#define IS_BAD(...)               ((__VA_ARGS__).kind == HASTE_VL_BAD)
#define IS_ZERO(...)              ((__VA_ARGS__).kind == HASTE_VL_ZERO)
#define IS_UNINIT(...)            ((__VA_ARGS__).kind == HASTE_VL_UNINIT)
#define IS_SCALAR(...)            ((__VA_ARGS__).kind == HASTE_VL_SCALAR)
#define IS_RUNTIME(...)           ((__VA_ARGS__).kind == HASTE_VL_RUNTIME)
#define IS_TYPE(...)              ((__VA_ARGS__).kind == HASTE_VL_TYPE)
#define IS_LVALUE(...)            ((__VA_ARGS__).is_lvalue)

#define IS_STRUCT_TYPE(v)         ((AS_TYPE_INFO(v)->kind == HASTE_TY_STRUCT))
#define IS_AUTO_STRUCT_TYPE(v)    ((AS_TYPE_INFO(v)->kind == HASTE_TY_AUTO_STRUCT))

#define IS_OBJ(...)               ((__VA_ARGS__).kind == HASTE_VL_OBJ)
#define IS_STRUCT(...)            ((IS_OBJ(__VA_ARGS__)) and ((__VA_ARGS__).obj->kind == HASTE_OBJ_STRUCT))

#define AS_OBJ(v)                 ((v).obj)
#define AS_TYPE_INFO(v)           ((v).value.type)
#define AS_STRUCT_TYPE_INFO(v)    ((&AS_TYPE_INFO(v)->structure))

#define AS_STRUCT(v)              ((OAS_STRUCT(AS_OBJ(v))))
#define OAS_STRUCT(v)             ((struct haste_struct_object *)(v))

#define ASSERT_IS_TYPE(...) \
	do { \
		assert(IS_TYPE(__VA_ARGS__) and "It should be a type. maybe you forgot to use `typeof_value()`?"); \
	} while (0)

void setup_builtins(struct intern_pool *pool);

bool value_equal(struct haste_value a, struct haste_value b);

struct haste_value value_assign(
	struct intern_pool *pool,
	struct haste_value *lvalue,
	struct haste_value rvalue);

struct haste_value value_add(const struct haste_value lhs, const struct haste_value rhs);
struct haste_value value_sub(const struct haste_value lhs, const struct haste_value rhs);
struct haste_value value_mul(const struct haste_value lhs, const struct haste_value rhs);
struct haste_value value_div(const struct haste_value lhs, const struct haste_value rhs);

bool is_comptime_known(const struct haste_value v);

#define struct_get_field(v_, ...) _Generic((__VA_ARGS__), \
	const char *: struct_get_field_by_name, \
	char *: struct_get_field_by_name, \
	size_t: struct_get_field_by_index) (v_, (__VA_ARGS__))
#define struct_set_field(allocator_, v_, key_, ...) _Generic((key_), \
	const char *: struct_set_field_by_name, \
	char *: struct_set_field_by_name, \
	size_t: struct_set_field_by_index) (allocator_, v_, key_, (__VA_ARGS__))
#define struct_has_field(v_, ...) _Generic((__VA_ARGS__), \
	const char *: struct_has_field_name \
	char *: struct_has_field_name \
	struct token: struct_has_field_token) (v_, (__VA_ARGS__)))
bool struct_has_field_name(const struct haste_value value, const char *name);

struct haste_value value_access(
	struct intern_pool *pool,
	const struct haste_value value,
	const struct string str);

struct haste_value struct_get_field_by_name(const struct haste_value value,
											const char *name);
struct haste_value struct_get_field_by_index(const struct haste_value value,
											 const size_t idx);
struct haste_value struct_set_field_by_name(struct intern_pool *pool,
											struct haste_value *value,
											const char *name,
											const struct haste_value new_value);
struct haste_value struct_set_field_by_index(struct intern_pool *pool,
											 struct haste_value *value,
											 const size_t idx,
											 const struct haste_value new_value);

bool haste_is_default_empty_string(const struct haste_object *obj);

int print_object(stream_t stream, const struct haste_object *obj, struct haste_type type);
int print_value(stream_t stream, const struct haste_value value);

struct haste_value type_get_int(struct intern_pool *pool, uint16_t bits, bool is_signed);

//
// value_builder.c
//
struct haste_value_builder {
	struct intern_pool *pool;
	size_t len, cap;
	struct haste_value *items;
};

struct haste_value_builder value_builder(struct intern_pool *pool);
void free_value_builder(struct haste_value_builder *builder);
void value_builder_push(struct haste_value_builder *builder, struct haste_value value);
void value_builder_set(struct haste_value_builder *builder, size_t index, struct haste_value value);
struct haste_value build_value(struct haste_value_builder *builder, struct haste_type type);

#endif
