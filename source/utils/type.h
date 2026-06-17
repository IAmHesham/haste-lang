#ifndef TYPE_H_
#define TYPE_H_

#include "value.h"

enum haste_compound_kind {
    HASTE_TYB_STRUCT,
};

struct haste_type_builder {
    struct intern_pool *pool;
    enum haste_compound_kind kind;
    bool is_auto;

    size_t len, cap;
    struct haste_struct_field *items;
};

struct haste_struct_field {
    const char *name;
    struct haste_type  type;
    struct haste_value default_value;
};

struct haste_type_info {
    enum {
        HASTE_TY_ZERO,
        HASTE_TY_UNKNOWN,
        HASTE_TY_TYPE,
        HASTE_TY_INT,
        HASTE_TY_UNTYPED_INT,
        HASTE_TY_FLOAT,
        HASTE_TY_UNTYPED_FLOAT,
        HASTE_TY_AUTO,
        HASTE_TY_VOID,
        HASTE_TY_UNTYPED_STRING,
        HASTE_TY_STRING,
        HASTE_TY_CSTR,
        HASTE_TY_USIZE,
        HASTE_TY_UINT,
        HASTE_TY_STRUCT,
        HASTE_TY_AUTO_STRUCT,
    } kind;
    bool is_integer  : 1;
    bool is_float    : 1;
    bool is_unsigned : 1;
    bool is_string   : 1;
    bool is_untyped  : 1;

    const char *name;
    size_t size;
    size_t align;
    size_t bit_size;

    union {
        struct haste_struct_type_info {
            size_t len;
            struct haste_struct_field *items;
        } structure;
    };
};

extern struct haste_type ty_zero;
extern struct haste_type ty_unknown;
extern struct haste_type ty_type;
extern struct haste_type ty_uint;
extern struct haste_type ty_int;
extern struct haste_type ty_untyped_int;
extern struct haste_type ty_float;
extern struct haste_type ty_untyped_float;
extern struct haste_type ty_auto;
extern struct haste_type ty_void;
extern struct haste_type ty_untyped_string;
extern struct haste_type ty_string;
extern struct haste_type ty_cstr;
extern struct haste_type ty_usize;

struct haste_type into_type(struct haste_value value);
struct haste_value into_value(struct haste_type type);

struct haste_type_builder type_builder(
    struct intern_pool *pool,
    enum haste_compound_kind kind);
void free_type_builder(struct haste_type_builder *builder);
struct haste_value add_field(
    struct haste_type_builder *builder,
    struct string name,
    struct haste_type type,
    struct haste_value default_value);
struct haste_type build_type(struct haste_type_builder *builder);

bool type_is_builtin(struct haste_type ty);

ptrdiff_t find_named_field(const struct haste_type tp, const char *name);

struct haste_type typeof_value(const struct haste_value value);

struct haste_value   make_value(struct intern_pool *pool, const struct haste_type type);
struct haste_object *create_struct(struct Allocator alloc, struct haste_struct_type_info *st);
struct haste_object *create_string(struct Allocator alloc, const char *str, size_t len);

struct haste_value value_cast(struct intern_pool *pool, const struct haste_type to, const struct haste_value value);
struct haste_value value_implicit_cast(struct intern_pool *pool, const struct haste_type to, const struct haste_value value);
struct haste_value value_coerce(struct intern_pool *pool, const struct haste_type to, const struct haste_value value);
struct haste_value zero_for_type(struct intern_pool *pool, struct haste_type to);
struct haste_value default_for_type(struct intern_pool *pool, struct haste_type to);

bool type_equal(const struct haste_type t1,
                const struct haste_type t2);

struct haste_type untyped_to_typed(struct haste_type type);

bool type_is_integer(const struct haste_type t);
bool type_is_float(const struct haste_type t);
bool type_is_untyped_float(const struct haste_type t);
bool type_is_number(const struct haste_type t);
bool type_is_untyped(const struct haste_type t);
bool type_is_untyped_integer(const struct haste_type t);
bool type_is_untyped_number(const struct haste_type t);
bool type_is_any_string(const struct haste_type t);

uint64_t type_hash(const struct haste_type t);

#endif
