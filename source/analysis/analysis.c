#include "haste.h"
#include "my_allocator.h"
#include "my_common.h"
#include "my_stream.h"
#include "my_termcolor.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

// ── Symbol levels ──────────────────────────────────────────────────
// -1: used before declaration (local)
//  0: undefined (out-of-order global)
//  1: defined (value not ready yet, but address is)
//  2: declared (value is ready)

enum symbol_level {
    SYM_AHH       = -1,
    SYM_UNDEFINED = 0,
    SYM_DEFINED   = 1,
    SYM_DECLARED  = 2,
};

struct symbol {
    const char *key;
    bool is_constant : 1;
    bool is_explicitly_comptime : 1;
    enum symbol_level level;
    struct haste_type type;
    struct haste_value value;
    struct haste_ast_node *node;
};

struct scope {
    struct scope *next;
    size_t len;
    struct symbol *items;
};

struct analyzer {
    struct intern_pool *pool;
    struct scope *global;
    struct scope *local;
    source_file_id src;
    bool had_error;
};

// ── Helpers ─────────────────────────────────────────────────────────

#define try(name, ...) \
    for (int _ok_ = 1, _run_ = 1; _ok_; _ok_ = 0) \
    for (struct haste_value name = (__VA_ARGS__); _run_; _run_ = 0) \
    if (IS_BAD(name)) return name; \
    else

#define bail(self, node, ...) \
    (report_error(self, node, __VA_ARGS__), VAL_BAD)

#define inject(pool, node, result) \
    (node) = node_into_value((pool), (node), (result))

#define with_scope(self) \
    for (struct scope *_s_ = begin_scope(self); _s_; end_scope(self), _s_ = NULL)

#define IS_AUTO(type) type_equal(type, ty_auto)

// ── Error reporting ────────────────────────────────────────────────

static void vreport_at(
    struct analyzer *self,
    struct location loc,
    const char *kind,
    bool set_error,
    const char *fmt,
    va_list args)
{
    f_vreport_at_location(kind, loc, fmt, args);
    if (set_error) self->had_error = true;
}

#define DEFINE_REPORT(name, color, label, set_error) \
    static void _report_##name##_node( \
        struct analyzer *self, struct haste_ast_node *node, \
        const char *fmt, ...) \
    { va_list a; va_start(a, fmt); vreport_at(self, node->location, color label, set_error, fmt, a); va_end(a); } \
    static void _report_##name##_token( \
        struct analyzer *self, struct token tok, \
        const char *fmt, ...) \
    { va_list a; va_start(a, fmt); vreport_at(self, as_location(tok), color label, set_error, fmt, a); va_end(a); } \
    static void _report_##name##_loc( \
        struct analyzer *self, struct location loc, \
        const char *fmt, ...) \
    { va_list a; va_start(a, fmt); vreport_at(self, loc, color label, set_error, fmt, a); va_end(a); }

DEFINE_REPORT(error,   ANSI_CODE_RED,    "Error",   true)
DEFINE_REPORT(note,    ANSI_CODE_GREEN,  "Note",   false)
DEFINE_REPORT(warning, ANSI_CODE_YELLOW, "Warning", false)

#define report_error(self, node, ...) \
    _Generic((node), \
        struct haste_ast_node *: _report_error_node, \
        struct token: _report_error_token, \
        struct location: _report_error_loc)(self, node, __VA_ARGS__)
#define report_note(self, node, ...) \
    _Generic((node), \
        struct haste_ast_node *: _report_note_node, \
        struct token: _report_note_token, \
        struct location: _report_note_loc)(self, node, __VA_ARGS__)
#define report_warning(self, node, ...) \
    _Generic((node), \
        struct haste_ast_node *: _report_warning_node, \
        struct token: _report_warning_token, \
        struct location: _report_warning_loc)(self, node, __VA_ARGS__)

// ── Forward declarations ───────────────────────────────────────────

static struct haste_value analyze_node(
    struct analyzer *self,
    struct haste_ast_node *node,
    struct haste_type expected_type);

struct haste_value analyze_node_type(
    struct analyzer *self,
    struct haste_ast_node *node);

// ── Scope management ───────────────────────────────────────────────

static Error prepare_scope(struct analyzer *self, struct haste_ast_node *nodes, bool top_level);

static struct scope *begin_scope(struct analyzer *self)
{
    struct scope *s = create(self->pool->allocator, struct scope, .next = self->local);
    self->local = s;
    if (self->global == NULL) self->global = s;
    return s;
}

static void end_scope(struct analyzer *self)
{
    if (self->global == NULL) return;
    struct scope *scope = self->local;
    self->local = scope->next;
    hmfree(self->pool->allocator, *scope);
    xdestroy(self->pool->allocator, sizeof(*scope), scope);
}

// ── Symbol table ───────────────────────────────────────────────────

#define local_put(self, name, ...) \
    symbol_put(self, self->local, name, (struct symbol){ __VA_ARGS__ })
#define global_put(self, name, ...) \
    symbol_put(self, self->global, name, (struct symbol){ __VA_ARGS__ })

static bool symbol_put(struct analyzer *self, struct scope *scope, const char *name, struct symbol s)
{
    s.key = name;
    if (hmget(*scope, name)) return true;
    hmput(self->pool->allocator, *scope, s);
    return false;
}

static struct symbol recursion_sentinel = { .value = VAL_BAD };

static struct symbol *symbol_find(struct analyzer *self, const char *name)
{
    leach (struct scope, scope, self->local) {
        struct symbol *s = hmget(*scope, name);
        if (s == NULL) continue;
        if (s->level == SYM_DEFINED) {
            report_error(self, s->node,
                "Recursive declaration is not allowed");
            return &recursion_sentinel;
        }
        if (s->level == SYM_UNDEFINED) {
            analyze_node(self, s->node, (struct haste_type){0});
            return s;
        }
        return s;
    }
    return NULL;
}

#define fail_symbol(sym, node) \
    do { \
        (sym)->value = VAL_BAD; \
        (sym)->type = into_type(VAL_BAD); \
        (sym)->node = (node); \
        (sym)->level = SYM_DECLARED; \
        return VAL_BAD; \
    } while (0)

// ── Binary operations ──────────────────────────────────────────────

static struct haste_value resolve_binary(
    struct analyzer *self,
    struct haste_value lhs,
    struct haste_value rhs,
    enum token_kind op,
    struct location op_loc)
{
#define BIN_CASE(kind, fn, msg) \
    case kind: { \
        struct haste_value r = fn(lhs, rhs); \
        if (IS_BAD(r)) { \
            switch (r.error_code) { \
            case ERR_INCOMPATIBLE_ARITH_TYPES: \
                report_error(self, op_loc, \
                    msg " not possible between {value} and {value}", \
                    typeof_value(lhs), typeof_value(rhs)); \
                break; \
            case ERR_ARITH_OVERFLOW: \
                report_error(self, op_loc, \
                    msg " caused arithmetic overflow"); \
                break; \
            case ERR_DIVISION_BY_ZERO: \
                report_error(self, op_loc, \
                    msg " by zero is not allowed"); \
                break; \
            default: unreachable(); \
            } \
            return VAL_BAD; \
        } \
        return r; \
    }

    switch (op) {
    BIN_CASE(TK_PLUS,   value_add, "Addition")
    BIN_CASE(TK_MINUS,  value_sub, "Subtraction")
    BIN_CASE(TK_STAR,   value_mul, "Multiplication")
    BIN_CASE(TK_FSLASH, value_div, "Division")
    default: unreachable();
    }
#undef BIN_CASE
}

static struct haste_value analyze_binary(
    struct analyzer *self,
    struct haste_ast_binary *node,
    struct haste_type expected_type)
{
    try (lhs, analyze_node(self, node->lhs, expected_type))
    try (rhs, analyze_node(self, node->rhs, expected_type))
    {
        if (not is_comptime_known(lhs) or not is_comptime_known(rhs)) {
            struct haste_type lt = typeof_value(lhs);
            struct haste_type rt = typeof_value(rhs);
            if (not (type_is_number(lt) or type_is_untyped_number(lt))
                or not (type_is_number(rt) or type_is_untyped_number(rt)))
            {
                return bail(self, node->op_loc,
                    "Cannot apply binary op to {value} and {value}", lt, rt);
            }
            struct haste_type result_type = type_is_untyped(lt) ? rt : lt;
            node->base.type = result_type;
            struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)node);
            result.type_info = AS_TYPE_INFO(result_type);
            return result;
        }

        try (result, resolve_binary(self, lhs, rhs, node->op, node->op_loc))
        {
            inject(self->pool, node, result);
            return result;
        }
    }
    return VAL_NONE;
}

// ── Unary operations ───────────────────────────────────────────────

static struct haste_value analyze_unary(
    struct analyzer *self,
    struct haste_ast_unary *node,
    struct haste_type expected_type)
{
    try (value, analyze_node(self, node->rhs, expected_type))
    {
        if (not is_comptime_known(value)) {
            if (node->op == TK_MINUS or node->op == TK_PLUS) {
                node->base.type = typeof_value(value);
                struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)node);
                result.type_info = node->base.type.value.type;
                return result;
            }
            return bail(self, node->op_loc,
                "Unary op not supported on runtime value");
        }

        switch (node->op) {
        case TK_MINUS: {
            if (IS_ZERO(value)) {
                value = VAL_SCALAR(AS_TYPE_INFO(typeof_value(value)), .integer = 0);
            } else if (IS_SCALAR(value)) {
                if (type_is_integer(typeof_value(value))) {
                    value.integer = -value.integer;
                } else if (type_is_float(typeof_value(value))) {
                    value.floating = -value.floating;
                } else {
                    return bail(self, node->op_loc,
                        "Cannot negate {value}", typeof_value(value));
                }
            } else {
                return bail(self, node->op_loc,
                    "Cannot negate {value}", typeof_value(value));
            }
        } break;
        case TK_PLUS:
            break;
        default:
            unreachable();
        }

        inject(self->pool, node, value);
        return value;
    }
    return VAL_NONE;
}

// ── Field access ───────────────────────────────────────────────────

static struct haste_value analyze_access(
    struct analyzer *self,
    struct haste_ast_access *node,
    struct haste_type expected_type)
{
    try (lhs, analyze_node(self, node->lhs, expected_type))
    {
        struct haste_value result = value_access(self->pool, lhs, node->field);
        if (IS_BAD(result)) {
            switch (result.error_code) {
            case ERR_FIELD_DOESNT_EXIST:
            case ERR_NOT_A_STRUCT:
                report_error(self, node->lhs,
                    "Field '{string}' not found in {value}",
                    node->field, typeof_value(lhs));
                break;
            default: unreachable();
            }
            return VAL_BAD;
        }

        inject(self->pool, node, result);
        return result;
    }
    return VAL_NONE;
}

// ── Literals ───────────────────────────────────────────────────────

static struct haste_value analyze_integer_lit(
    struct analyzer *self,
    struct haste_ast_integer_lit *node,
    struct haste_type expected_type)
{
    if (node->value == 0) {
        node->base.type = ty_zero;
        inject(self->pool, node, VAL_ZERO);
        return VAL_ZERO;
    }
    node->base.type = ty_untyped_int;
    struct haste_value result = VAL_SCALAR(ty_untyped_int.value.type, .integer = node->value);
    inject(self->pool, node, result);
    return result;
}

static struct haste_value analyze_float_lit(
    struct analyzer *self,
    struct haste_ast_float_lit *node,
    struct haste_type expected_type)
{
    node->base.type = ty_untyped_float;
    struct haste_value result = VAL_SCALAR(ty_untyped_float.value.type, .floating = node->value);
    inject(self->pool, node, result);
    return result;
}

static struct haste_value analyze_string_lit(
    struct analyzer *self,
    struct haste_ast_string_lit *node,
    struct haste_type expected_type)
{
    node->base.type = ty_untyped_string;
    struct haste_object *obj = create_string(self->pool->arena, node->value.chars, node->value.len);
    struct haste_value result = VAL_OBJ(ty_untyped_string.value.type, obj);
    inject(self->pool, node, result);
    return result;
}

// ── Identifier ─────────────────────────────────────────────────────

static struct haste_value analyze_ident(
    struct analyzer *self,
    struct haste_ast_ident *node,
    struct haste_type expected_type)
{
    const char *name = node->value.chars;
    struct symbol *sym = symbol_find(self, name);
    if (sym == NULL) {
        return bail(self, &node->base, "Undefined symbol '{s}'", name);
    }
    if (sym->level == SYM_AHH) {
        report_error(self, &node->base,
            "Symbol '{s}' used before declaration", name);
        report_note(self, sym->node, "Declared here");
        return VAL_BAD;
    }

    struct haste_value value = sym->value;
    if (not sym->is_constant) {
        value.is_lvalue = true;
    }

    if (not is_comptime_known(value)
        and not IS_TYPE(value)
        and not IS_NONE(value)
        and not IS_BAD(value))
    {
        node->base.type = sym->type;
        struct haste_value result = VAL_RUNTIME((struct haste_ast_node*)node);
        result.is_lvalue = not sym->is_constant;
        result.type_info = sym->type.value.type;
        return result;
    }

    inject(self->pool, node, value);
    return value;
}

// ── Type bit-width expressions ─────────────────────────────────────

static struct haste_value analyze_int_bits(
    struct analyzer *self,
    struct haste_ast_int_bits *node,
    struct haste_type expected_type)
{
    if (node->bits == 0 or node->bits > 128) {
        return bail(self, &node->base, "Invalid int bit width: {u32}", node->bits);
    }
    struct haste_value result = type_get_int(self->pool, node->bits, true);
    inject(self->pool, node, result);
    return result;
}

static struct haste_value analyze_uint_bits(
    struct analyzer *self,
    struct haste_ast_uint_bits *node,
    struct haste_type expected_type)
{
    if (node->bits == 0 or node->bits > 128) {
        return bail(self, &node->base, "Invalid uint bit width: {u32}", node->bits);
    }
    struct haste_value result = type_get_int(self->pool, node->bits, false);
    inject(self->pool, node, result);
    return result;
}

// ── Primitive type expressions ─────────────────────────────────────

#define DEFINE_TYPE_ANALYZER(name, type_val) \
    static struct haste_value analyze_##name( \
        struct analyzer *self, \
        struct haste_ast_node *node, \
        struct haste_type expected_type) \
    { \
        struct haste_value result = VAL_TYPE(type_val.value.type); \
        inject(self->pool, node, result); \
        return result; \
    }

DEFINE_TYPE_ANALYZER(string, ty_string)
DEFINE_TYPE_ANALYZER(cstr,   ty_cstr)
DEFINE_TYPE_ANALYZER(int,    ty_int)
DEFINE_TYPE_ANALYZER(uint,   ty_uint)
DEFINE_TYPE_ANALYZER(float,  ty_float)
DEFINE_TYPE_ANALYZER(usize,  ty_usize)
DEFINE_TYPE_ANALYZER(void,   ty_void)
DEFINE_TYPE_ANALYZER(auto,   ty_auto)
DEFINE_TYPE_ANALYZER(type,   ty_type)

// ── Grouping ───────────────────────────────────────────────────────

static struct haste_value analyze_grouping(
    struct analyzer *self,
    struct haste_ast_grouping *node,
    struct haste_type expected_type)
{
    struct haste_value value = analyze_node(self, node->child, expected_type);
    if (IS_BAD(value)) return VAL_BAD;
    node->base.type = typeof_value(value);
    return value;
}

// ── Distinct ───────────────────────────────────────────────────────

static struct haste_value analyze_distinct(
    struct analyzer *self,
    struct haste_ast_distinct *node,
    struct haste_type expected_type)
{
    try (type_val, analyze_node_type(self, node->child))
    {
        struct haste_type tp = into_type(type_val);
        struct haste_type_info *ti = intern_type_info_unique(self->pool, AS_TYPE_INFO(tp));
        ti->name = NULL;
        struct haste_value result = VAL_TYPE(ti);
        inject(self->pool, node, result);
        return result;
    }
    return VAL_NONE;
}

// ── Cast ───────────────────────────────────────────────────────────

static struct haste_value analyze_cast(
    struct analyzer *self,
    struct haste_ast_cast *node,
    struct haste_type expected_type)
{
    struct haste_type target = ty_auto;
    if (node->to != NULL) {
        struct haste_value tp = analyze_node_type(self, node->to);
        if (IS_BAD(tp)) return VAL_BAD;
        target = into_type(tp);
    } else {
        target = expected_type;
    }

    try (value, analyze_node(self, node->expr, (struct haste_type){0}))
    {
        struct haste_value result = value_cast(self->pool, target, value);
        if (IS_BAD(result)) {
            switch (result.error_code) {
            case ERR_INVALID_CAST:
                report_error(self, &node->base,
                    "Cannot cast {value} to {value}",
                    typeof_value(value), target);
                break;
            default: unreachable();
            }
            return VAL_BAD;
        }
        inject(self->pool, node, result);
        return result;
    }
    return VAL_NONE;
}

// ── Variable declarations ──────────────────────────────────────────

static struct haste_value analyze_var_decl(
    struct analyzer *self,
    struct haste_ast_var_decl *node,
    struct haste_type expected_type)
{
    const char *name = node->name.chars;
    struct symbol *sym = symbol_find(self, name);
    assert(sym != NULL);

    sym->is_constant = node->is_constant;
    sym->level = SYM_DEFINED;

    if (node->type == NULL and node->value == NULL) {
        return bail(self, &node->base,
            "Must specify either a type, a value, or both");
    }

    struct haste_type declared_type = ty_auto;
    if (node->type != NULL) {
        struct haste_value tp = analyze_node(self, node->type, (struct haste_type){0});
        if (IS_BAD(tp)) fail_symbol(sym, &node->base);
        if (not IS_TYPE(tp)) {
            report_error(self, node->type,
                "Expected a type, got {value}", typeof_value(tp));
            fail_symbol(sym, &node->base);
        }
        declared_type = into_type(tp);
    }

    struct haste_value init_value = VAL_UNINIT;
    if (node->value != NULL) {
        init_value = analyze_node(self, node->value, declared_type);
        if (IS_BAD(init_value)) {
            fail_symbol(sym, &node->base);
        }
    }

    if (IS_AUTO(declared_type)) {
        declared_type = typeof_value(init_value);
    } else if (IS_UNINIT(init_value)) {
        init_value = default_for_type(self->pool, declared_type);
    }

    if (not type_equal(declared_type, typeof_value(init_value))) {
        struct haste_type orig = typeof_value(init_value);
        init_value = value_coerce(self->pool, declared_type, init_value);
        if (IS_BAD(init_value)) {
            report_error(self, node->name_loc,
                "Cannot assign {value} to {value}", orig, declared_type);
            fail_symbol(sym, &node->base);
        }
    }

    // If binding a type value, name the type
    if (IS_TYPE(init_value) and node->name.chars != NULL) {
        struct haste_type_info *ti = AS_TYPE_INFO(into_type(init_value));
        if (not type_is_builtin(into_type(init_value)) or ti->name == NULL) {
            ti->name = node->name.chars;
        }
    }

    bool is_explicitly_comptime = node->is_explicitly_comptime
        or (sym->is_constant and type_equal(declared_type, ty_type));

    sym->type = declared_type;
    sym->value = init_value;
    sym->level = SYM_DECLARED;

    node->base.type = declared_type;
    node->is_explicitly_comptime = is_explicitly_comptime;

    if (not IS_RUNTIME(init_value)) {
        inject(self->pool, node->value, init_value);
    }

    return init_value;
}

// ── Struct types ───────────────────────────────────────────────────

static struct haste_value analyze_struct_type(
    struct analyzer *self,
    struct haste_ast_struct_type *node,
    struct haste_type expected_type)
{
    struct haste_type_builder builder = type_builder(self->pool, HASTE_TYB_STRUCT);
    bool had_err = false;

    leach (struct haste_ast_struct_field, field, node->fields) {
        if (field->default_value == NULL and field->type == NULL) {
            report_error(self, &field->base,
                "Field must have a type or a default value");
            had_err = true;
            continue;
        }

        struct haste_value default_val = VAL_NONE;
        if (field->default_value != NULL) {
            default_val = analyze_node(self, field->default_value, (struct haste_type){0});
        }

        struct haste_type field_type = {0};
        if (field->type != NULL) {
            struct haste_value tp = analyze_node_type(self, field->type);
            if (IS_BAD(tp)) { had_err = true; continue; }
            field_type = into_type(tp);
        }

        if (IS_BAD(default_val) or IS_BAD(field_type.value)) {
            had_err = true;
            continue;
        }

        for (size_t i = 0; i < field->name_count; i++) {
            struct haste_value r = add_field(&builder, field->names[i], field_type, default_val);
            if (IS_BAD(r)) {
                switch (r.error_code) {
                case ERR_FIELD_DUPLICATION:
                    report_error(self, &field->base,
                        "Duplicate field '{string}'", field->names[i]);
                    break;
                default: unreachable();
                }
                had_err = true;
                goto next_field;
            }
        }
        next_field:;
    }

    if (had_err) return VAL_BAD;

    struct haste_type result_type = build_type(&builder);
    struct haste_value result = into_value(result_type);
    inject(self->pool, node, result);
    return result;
}

// ── Struct literals ────────────────────────────────────────────────

static struct haste_value analyze_auto_struct_literal(
    struct analyzer *self,
    struct haste_ast_struct_literal *node,
    struct haste_type expected_type)
{
    struct haste_type_builder tb = type_builder(self->pool, HASTE_TYB_STRUCT);
    tb.is_auto = true;
    struct haste_value_builder vb = value_builder(self->pool);

    leach (struct haste_ast_struct_lit_field, lit, node->fields) {
        if (lit->name.chars == NULL) {
            return bail(self, lit->value,
                "Auto struct literals must use named fields");
        }
        struct haste_value fv = analyze_node(self, lit->value, expected_type);
        if (IS_BAD(fv)) return VAL_BAD;

        arrpush(tb.pool->allocator, tb, (struct haste_struct_field){
            .name = lit->name.chars,
            .type = typeof_value(fv),
            .default_value = VAL_NONE,
        });
        value_builder_push(&vb, fv);
    }

    struct haste_type t = build_type(&tb);
    struct haste_value result = build_value(&vb, t);
    inject(self->pool, node, result);
    return result;
}

static struct haste_value analyze_struct_literal(
    struct analyzer *self,
    struct haste_ast_struct_literal *node,
    struct haste_type expected_type)
{
    if (node->type_expr == NULL) {
        return analyze_auto_struct_literal(self, node, expected_type);
    }

    try_type(tp, node->type_expr);
    struct haste_type struct_type = _ty_tp;

    if (IS_AUTO(struct_type)) {
        return analyze_auto_struct_literal(self, node, expected_type);
    }

    if (not IS_STRUCT_TYPE(struct_type)) {
        return bail(self, node->type_expr,
            "Expected a struct type, got {value}", struct_type);
    }

    struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(struct_type);
    struct haste_value_builder builder = value_builder(self->pool);

    leach (struct haste_ast_struct_lit_field, lit, node->fields) {
        if (lit->name.chars == NULL) {
            return bail(self, lit->value,
                "Positional fields not allowed for typed struct literals");
        }

        ptrdiff_t idx = find_named_field(struct_type, lit->name.chars);
        if (idx < 0) {
            report_error(self, lit->name_loc,
                "Field '{s}' does not exist in struct", lit->name.chars);
            return VAL_BAD;
        }

        struct haste_value fv = analyze_node(self, lit->value, st->items[idx].type);
        if (IS_BAD(fv)) return VAL_BAD;

        struct haste_value cv = value_coerce(self->pool, st->items[idx].type, fv);
        if (IS_BAD(cv)) {
            report_error(self, lit->value,
                "Cannot assign {value} to field '{s}' of type {value}",
                typeof_value(fv), lit->name.chars, st->items[idx].type);
            return VAL_BAD;
        }

        value_builder_set(&builder, (size_t)idx, cv);
    }

    struct haste_value result = build_value(&builder, struct_type);
    node->base.type = typeof_value(result);
    inject(self->pool, node, result);
    return result;
}

// ── Block (do/end) ─────────────────────────────────────────────────

static struct haste_value analyze_block(
    struct analyzer *self,
    struct haste_ast_block *node,
    struct haste_type expected_type)
{
    struct haste_value last_val = VAL_NONE;
    with_scope(self) {
        Error err = prepare_scope(self, (void*)node->stmts, false);
        if (err) return VAL_BAD;

        leach (struct haste_ast_node, stmt, node->stmts) {
            last_val = analyze_node(self, stmt, expected_type);
            if (IS_BAD(last_val)) {
                self->had_error = true;
                last_val = VAL_NONE;
            }
        }
    }

    if (not node->returning) {
        node->base.type = ty_void;
        return VAL_UNINIT;
    }
    if (not is_comptime_known(last_val)) {
        node->base.type = typeof_value(last_val);
        return last_val;
    }

    node->base.type = typeof_value(last_val);
    inject(self->pool, node, last_val);
    return last_val;
}

// ── Main dispatch ──────────────────────────────────────────────────

struct haste_value analyze_node(
    struct analyzer *self,
    struct haste_ast_node *node,
    struct haste_type expected_type)
{
    if (node->kind == ND_VALUE or node->analyzed) {
        return node->kind == ND_VALUE
            ? ((struct haste_ast_value*)node)->value
            : VAL_BAD;
    }
    node->analyzed = true;

    switch (node->kind) {
    case ND_STRUCT_FIELD:     unreachable();
    case ND_STRUCT_LIT_FIELD: unreachable();
    case ND_VALUE:            unreachable();
    case ND_BINARY:         return analyze_binary         (self, (void*)node, expected_type);
    case ND_UNARY:          return analyze_unary          (self, (void*)node, expected_type);
    case ND_ACCESS:         return analyze_access         (self, (void*)node, expected_type);
    case ND_INTEGER_LIT:    return analyze_integer_lit    (self, (void*)node, expected_type);
    case ND_FLOAT_LIT:      return analyze_float_lit      (self, (void*)node, expected_type);
    case ND_STRING_LIT:     return analyze_string_lit     (self, (void*)node, expected_type);
    case ND_IDENT:          return analyze_ident          (self, (void*)node, expected_type);
    case ND_GROUPING:       return analyze_grouping       (self, (void*)node, expected_type);
    case ND_DISTINCT:       return analyze_distinct       (self, (void*)node, expected_type);
    case ND_CAST:           return analyze_cast           (self, (void*)node, expected_type);
    case ND_STRUCT_TYPE:    return analyze_struct_type    (self, (void*)node, expected_type);
    case ND_STRUCT_LITERAL: return analyze_struct_literal (self, (void*)node, expected_type);
    case ND_VAR_DECL:       return analyze_var_decl       (self, (void*)node, expected_type);
    case ND_BLOCK:          return analyze_block          (self, (void*)node, expected_type);
    case ND_INT_BITS:       return analyze_int_bits       (self, (void*)node, expected_type);
    case ND_UINT_BITS:      return analyze_uint_bits      (self, (void*)node, expected_type);
    case ND_STRING:         return analyze_string         (self, (void*)node, expected_type);
    case ND_CSTR:           return analyze_cstr           (self, (void*)node, expected_type);
    case ND_INT:            return analyze_int            (self, (void*)node, expected_type);
    case ND_UINT:           return analyze_uint           (self, (void*)node, expected_type);
    case ND_FLOAT:          return analyze_float          (self, (void*)node, expected_type);
    case ND_USIZE:          return analyze_usize          (self, (void*)node, expected_type);
    case ND_VOID:           return analyze_void           (self, (void*)node, expected_type);
    case ND_AUTO:           return analyze_auto           (self, (void*)node, expected_type);
    case ND_TYPE:           return analyze_type           (self, (void*)node, expected_type);
    }
    unreachable();
}

struct haste_value analyze_node_type(
    struct analyzer *self,
    struct haste_ast_node *node)
{
    struct haste_value result = analyze_node(self, node, (struct haste_type){0});
    if (not IS_TYPE(result)) {
        report_error(self, node,
            "Expected a type, got {value}", typeof_value(result));
        return VAL_BAD_ERROR(ERR_NOT_TYPE);
    }
    inject(self->pool, node, result);
    return result;
}

// ── Scope preparation ──────────────────────────────────────────────

static const char *declaration_name(struct haste_ast_node *node)
{
    assert(node_is_declaration(node));
    switch (node->kind) {
    case ND_VAR_DECL: return ((struct haste_ast_var_decl*)node)->name.chars;
    default: unreachable();
    }
}

static Error prepare_scope(struct analyzer *self, struct haste_ast_node *nodes, bool top_level)
{
    Error result = OK;
    leach (struct haste_ast_node, node, nodes) {
        if (not node_is_declaration(node)) continue;
        const char *name = declaration_name(node);
        struct symbol *existing = hmget(*self->local, name);
        if (existing != NULL) {
            report_error(self, node, "Redefinition of '{s}'", name);
            report_note(self, existing->node, "Previously defined here");
            result = ERROR;
            continue;
        }
        local_put(self, name,
            .level = top_level ? SYM_UNDEFINED : SYM_AHH,
            .node = node);
    }
    return result;
}

// ── Entry point ────────────────────────────────────────────────────

Error analyze(struct intern_pool *pool, source_file_id src)
{
    struct analyzer a = {
        .pool = pool,
        .src = src,
    };
    with_scope(&a) {
        struct haste_ast_node *nodes = get_source_file_ast(src);
        Error err = prepare_scope(&a, nodes, true);
        if (err) return ERROR;

        leach (struct haste_ast_node, node, nodes) {
            analyze_node(&a, node, (struct haste_type){0});
            reset_temporary_allocator();
        }
    }
    return a.had_error ? ERROR : OK;
}

Error analyze_one_node(
    struct intern_pool *pool,
    struct haste_ast_node *node,
    struct haste_value *out)
{
    struct analyzer a = {
        .pool = pool,
        .src = -1,
    };
    with_scope(&a) {
        *out = analyze_node(&a, node, into_type(VAL_NONE));
    }
    return a.had_error ? ERROR : OK;
}
