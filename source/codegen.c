#include "haste.h"
#include "dynamic_memory_stream.h"
#include "my_allocator.h"
#include "my_common.h"
#include "my_stream.h"
#include <assert.h>
#include <signal.h>

struct type_map_entry {
	struct haste_type_info *haste_type;
	const char *c_name;
};

struct codegen_context {
	struct Allocator allocator;
	stream_t structs_stream;
	stream_t globals_stream;
	stream_t decls_stream;
	stream_t impls_stream;
	stream_t current_block_stream;
    
	struct { size_t cap, len; struct type_map_entry *items; } struct_types;
	uint64_t str_counter;
	uint64_t tmp_counter;
};

static const char *codegen_expr(struct codegen_context *ctx, const struct haste_ast_node *node);
static const char *codegen_stmt(struct codegen_context *ctx, const struct haste_ast_node *node);
static void codegen_location(stream_t stream, const struct location location);

static void context_deinit(struct codegen_context *ctx)
{
	arrfree(ctx->allocator, ctx->struct_types);
	sclose(ctx->structs_stream);
	sclose(ctx->globals_stream);
	sclose(ctx->decls_stream);
	sclose(ctx->impls_stream);
	if (ctx->current_block_stream.data != NULL) {
		sclose(ctx->current_block_stream);
	}
	*ctx = (struct codegen_context){0};
}

// ── Haste type → C type ────────────────────────────────────────

static const char *c_type(struct codegen_context *ctx, struct haste_type type)
{
	struct haste_type_info *tp = AS_TYPE_INFO(type);

	if (tp->kind == HASTE_TY_UNTYPED_INT or tp->kind == HASTE_TY_ZERO) return "int32_t";
	if (tp->kind == HASTE_TY_USIZE) return "uint64_t";
	if (tp->kind == HASTE_TY_FLOAT or tp->kind == HASTE_TY_UNTYPED_FLOAT) return "float";
	if (tp->kind == HASTE_TY_VOID) return "void";
	if (tp->kind == HASTE_TY_UNTYPED_STRING or tp->kind == HASTE_TY_CSTR or tp->kind == HASTE_TY_STRING) return "char*";

	if (tp->kind == HASTE_TY_INT or tp->kind == HASTE_TY_UINT) {
		return tsprint("{s}int{z}_t", tp->kind == HASTE_TY_UINT ? "u" : "", tp->bit_size);
	}

	if (tp->kind == HASTE_TY_STRUCT or tp->kind == HASTE_TY_AUTO_STRUCT) {
		struct haste_type_info *type_info = AS_TYPE_INFO(type);
		struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(type);

		for (size_t i = 0; i < ctx->struct_types.len; i += 1) {
			if (ctx->struct_types.items[i].haste_type == type.value.type) {
				return ctx->struct_types.items[i].c_name;
			}
		}

		char *struct_name = tsprint("struct_type_{s}_{z}", type_info->name then type_info->name otherwise "auto", ctx->struct_types.len);
		char *c_name = tsprint("struct {s}", struct_name);

		arrpush(ctx->allocator, ctx->struct_types, ((struct type_map_entry){
			.haste_type = type.value.type,
			.c_name = c_name,
		}));

		sprint(ctx->structs_stream, "struct {s} {\n", struct_name);
		iarreach (i, *st) {
			sprint(ctx->structs_stream, "\t{s} f_{z};\n", c_type(ctx, st->items[i].type), i);
		}
		sprintln(ctx->structs_stream, "};");
		return c_name;
	}

	raise(SIGSEGV);
	unreachable();
}

// ── String globals ────────────────────────────────────────────────

static const char *emit_string_global(struct codegen_context *ctx,
                                       const char *data, uint64_t len)
{
	char *name = tsprint("str_{lu}", ctx->str_counter++);
	sprint(ctx->globals_stream, "static const char {s}[] = ", name);
	sprint(ctx->globals_stream, "\"");
	for (uint64_t i = 0; i < len; i++) {
		switch (data[i]) {
			case '\n': sprint(ctx->globals_stream, "\\n"); break;
			case '"': sprint(ctx->globals_stream, "\\\""); break;
			case '\\': sprint(ctx->globals_stream, "\\\\"); break;
			case '\0': sprint(ctx->globals_stream, "\\0"); break;
			default: sputc(ctx->globals_stream, data[i]); break;
		}
	}
	sprintln(ctx->globals_stream, "\";");
	return name;
}

// ── Haste value → C value ──────────────────────────────────────

static const char *c_value(struct codegen_context *ctx, struct haste_value value)
{
	assert(not IS_TYPE(value));

	switch (value.kind) {
	case HASTE_VL_ZERO: {
		return "0";
	}
	case HASTE_VL_SCALAR: {
		int k = value.type_info->kind;
		if (k == HASTE_TY_USIZE)
			return tsprint("{ld}ULL", value.integer);
		if (k == HASTE_TY_INT or k == HASTE_TY_UNTYPED_INT or k == HASTE_TY_UINT) {
			return tsprint("{ld}", value.integer);
		}
		if (k == HASTE_TY_FLOAT or k == HASTE_TY_UNTYPED_FLOAT)
			return tsprint("{f}f", value.floating);
		unreachable();
	}
	case HASTE_VL_OBJ: {
		if (value.obj->kind == HASTE_OBJ_STRING) {
			struct haste_string_object *s = (struct haste_string_object*)value.obj;
			return emit_string_global(ctx, s->data, s->len);
		}

		if (value.obj->kind == HASTE_OBJ_STRUCT) {
			struct haste_struct_object *so = (struct haste_struct_object*)value.obj;
			struct haste_struct_type_info *st = AS_STRUCT_TYPE_INFO(typeof_value(value));
			char *buf = calloc(4096, 1);
			stream_t str = smemopen(buf, 4096);
			sprint(str, "({s}){{", c_type(ctx, typeof_value(value)));
			iarreach (i, *st) {
				if (i > 0) sprint(str, ", ");
				sprint(str, "{s}", c_value(ctx, so->fields[i]));
			}
			sprint(str, "}");
            const char *res = tsprint("{s}", buf);
            sclose(str);
            free(buf);
			return res;
		}

		unreachable();
	}
	case HASTE_VL_RUNTIME:
		return codegen_expr(ctx, value.runtime);
	case HASTE_VL_NONE:
	case HASTE_VL_BAD:
	case HASTE_VL_UNINIT:
	case HASTE_VL_TYPE:
		unreachable();
	}
}

// ── Expression codegen ────────────────────────────────────────────

static const char *codegen_cast(struct codegen_context *ctx, const struct haste_ast_cast *node)
{
	const char *val = codegen_expr(ctx, node->expr);
	const char *target_type = c_type(ctx, node->base.type);
	return tsprint("({s}){s}", target_type, val);
}

static const char *codegen_value(struct codegen_context *ctx, const struct haste_ast_value *node)
{
	if (is_comptime_known(node->value)) {
		return c_value(ctx, node->value);
	}

	if (IS_RUNTIME(node->value)) {
		return codegen_expr(ctx, node->value.runtime);
	}
	unreachable();
}

static const char *codegen_lvalue(struct codegen_context *ctx, const struct haste_ast_node *node);

static const char *codegen_ident(struct codegen_context *ctx, const struct haste_ast_ident *node)
{
	return codegen_lvalue(ctx, &node->base);
}

static const char *codegen_binary(struct codegen_context *ctx, const struct haste_ast_binary *node)
{
	const char *lhs = codegen_expr(ctx, node->lhs);
	const char *rhs = codegen_expr(ctx, node->rhs);

	switch (node->op) {
	case TK_PLUS:   return tsprint("({s} + {s})", lhs, rhs);
	case TK_MINUS:  return tsprint("({s} - {s})", lhs, rhs);
	case TK_STAR:   return tsprint("({s} * {s})", lhs, rhs);
	case TK_FSLASH: return tsprint("({s} / {s})", lhs, rhs);
	default: unreachable();
	}
}

static const char *codegen_unary(struct codegen_context *ctx, const struct haste_ast_unary *node)
{
	const char *rhs = codegen_expr(ctx, node->rhs);
	switch (node->op) {
	case TK_MINUS: return tsprint("(-{s})", rhs);
	case TK_PLUS:  return tsprint("(+{s})", rhs);
	default: unreachable();
	}
}

static const char *codegen_access(struct codegen_context *ctx, const struct haste_ast_access *node)
{
	const char *ptr = codegen_lvalue(ctx, &node->base);
	return ptr; // lvalue already produced the dot accessor string!
}

static const char *codegen_lvalue(struct codegen_context *ctx, const struct haste_ast_node *node)
{
	switch (node->kind) {
	case ND_IDENT: {
		const struct haste_ast_ident *ident = (const void*)node;
		return ident->value.chars;
	}
	case ND_ACCESS: {
		const struct haste_ast_access *access = (const void*)node;
		const char *ptr = codegen_lvalue(ctx, access->lhs);
		return tsprint("({s}).f_{lu}", ptr, (unsigned long)access->field_index);
	}
	default:
		unreachable();
	}
}

static const char *codegen_func_call(struct codegen_context *ctx, const struct haste_ast_func_call *node)
{
	const char *fn_name = "";
	if (node->callee->kind == ND_IDENT) {
		fn_name = ((const struct haste_ast_ident*)node->callee)->value.chars;
	}

	char *args_buf = calloc(4096, 1);
	stream_t str = smemopen(args_buf, 4096);
	
	size_t i = 0;
	for (const struct haste_ast_func_call_arg *a = node->args; a; a = a->next) {
		if (i > 0) sprint(str, ", ");
		sprint(str, "{s}", codegen_expr(ctx, a->value));
		i++;
	}

    const char *res = tsprint("{s}({s})", fn_name, args_buf);
    sclose(str);
    free(args_buf);
	return res;
}

static const char *codegen_block(struct codegen_context *ctx, const struct haste_ast_block *node)
{
	const char *ret_type = c_type(ctx, node->base.type);
	const char *tmp = NULL;

	if (node->returning) {
		tmp = tsprint("tmp_{lu}", ctx->tmp_counter++);
		sprintln(ctx->current_block_stream, "{s} {s};", ret_type, tmp);
	}
	
	sprintln(ctx->current_block_stream, "{");
	const char *last_val = NULL;
	if (node->stmts != NULL) {
		leach (struct haste_ast_node, stmt, node->stmts) {
			last_val = codegen_stmt(ctx, stmt);
		}
	}
	
	if (tmp != NULL and last_val != NULL) {
		sprintln(ctx->current_block_stream, "{s} = {s};", tmp, last_val);
	}
	sprintln(ctx->current_block_stream, "}");
	
	return tmp;
}

static const char *codegen_return(struct codegen_context *ctx, const struct haste_ast_return *node)
{
	if (node->value != NULL) {
		const char *val = codegen_expr(ctx, node->value);
		sprintln(ctx->current_block_stream, "return {s};", val);
	} else {
		sprintln(ctx->current_block_stream, "return;");
	}
	return "";
}

static const char *codegen_struct_lit(struct codegen_context *ctx, const struct haste_ast_struct_literal *node)
{
	char *buf = NULL;
	stream_t str = sdynmemopen(ctx->allocator, &buf);
	const struct haste_type type = node->base.type;
	const char *tmp = tsprint("tmp_{lu}", ctx->tmp_counter++);
	sprintln(str, "{s} {s};", c_type(ctx, type), tmp);
	leach (struct haste_ast_struct_lit_field, field, node->fields) {
		const char *value = codegen_expr(ctx, field->value);
		sprintln(str, "{s}.{string} = {s};", tmp, field->name, value);
	}
	sprintln(ctx->current_block_stream, "{s}", buf);
	sclose(str);
	return tmp;
}

static const char *codegen_expr(struct codegen_context *ctx, const struct haste_ast_node *node)
{
	switch (node->kind) {
	case ND_VALUE:          return codegen_value    (ctx, (void*)node);
	case ND_CAST:           return codegen_cast     (ctx, (void*)node);
	case ND_GROUPING:       return codegen_expr     (ctx, ((const struct haste_ast_grouping*)node)->child);
	case ND_IDENT:          return codegen_ident    (ctx, (void*)node);
	case ND_BINARY:         return codegen_binary   (ctx, (void*)node);
	case ND_UNARY:          return codegen_unary    (ctx, (void*)node);
	case ND_ACCESS:         return codegen_access   (ctx, (void*)node);
	case ND_FUNC_CALL:      return codegen_func_call(ctx, (void*)node);
	case ND_BLOCK:          return codegen_block    (ctx, (void*)node);
	case ND_RETURN:         return codegen_return   (ctx, (void*)node);
	case ND_STRUCT_LITERAL: return codegen_struct_lit(ctx, (void*)node);
	case ND_INTEGER_LIT: {
		const struct haste_ast_integer_lit *lit = (const void*)node;
		return tsprint("{ld}", lit->value);
	}
	case ND_FLOAT_LIT: {
		const struct haste_ast_float_lit *lit = (const void*)node;
		return tsprint("{f}", lit->value);
	}
	case ND_STRING_LIT: {
		const struct haste_ast_string_lit *lit = (const void*)node;
		return emit_string_global(ctx, lit->value.chars, lit->value.len);
	}
	default:
		unimplemented();
	}
	return NULL;
}

static const char *codegen_var(
	struct codegen_context *ctx,
	const struct haste_ast_var_decl *node,
	bool is_global);

static const char *codegen_stmt(struct codegen_context *ctx, const struct haste_ast_node *node)
{
	codegen_location(ctx->current_block_stream, node->location);

	switch (node->kind) {
	case ND_FUNC_DECL: unimplemented();
	case ND_VAR_DECL:  return codegen_var(ctx, (void*)node, false);
	default: {
		const char *val = codegen_expr(ctx, node);
		if (node->kind != ND_RETURN && node->kind != ND_BLOCK) {
			sprintln(ctx->current_block_stream, "{s};", val);
		}
		return val;
	}
	}
}

static void codegen_location(stream_t stream, const struct location location)
{
	struct file_position pos = as_position(location);
	sprintln(stream, "#line {u32} {s:#}", pos.line, get_source_file_path(location.src));
}

// ── Global declaration codegen ────────────────────────────────────

static const char *codegen_var(struct codegen_context *ctx, const struct haste_ast_var_decl *node, bool is_global)
{
	if (node->is_explicitly_comptime) return "";

	const char *name = node->name.chars;
	const char *type = c_type(ctx, node->base.type);
	const char *init = node->value != NULL ? codegen_expr(ctx, node->value) : "0";

	if (is_global) {
		codegen_location(ctx->globals_stream, node->base.location);
		sprintln(ctx->globals_stream, "{s} {s} {s} = {s};", node->is_constant ? "const" : "", type, name, init);
	} else {
		sprintln(ctx->current_block_stream, "{s} {s} {s} = {s};", node->is_constant ? "const" : "", type, name, init);
	}

	return name;
}

static const char *codegen_func_decl(struct codegen_context *ctx, const struct haste_ast_func_decl *node)
{
	const char *return_type = c_type(ctx, node->base.type);
	const char *name = node->name.chars;

	char *params_buf = NULL;
	stream_t p_str = sdynmemopen(ctx->allocator, &params_buf);

	size_t param_count = 0;
	leach (struct haste_ast_func_param, p, node->params) {
		struct haste_type param_type = {0};
		if (p->type != NULL and p->type->kind == ND_VALUE) {
			struct haste_ast_value *val_node = (struct haste_ast_value*)p->type;
			param_type = into_type(VAL_TYPE(val_node->value.type));
		} else if (p->type != NULL) {
			param_type = p->type->type;
		}
		const char *c_param_type = c_type(ctx, param_type);
		for (size_t i = 0; i < p->name_count; i++) {
			if (param_count > 0) sprint(p_str, ", ");
			sprint(p_str, "{s} {s}", c_param_type, p->names[i].chars);
			param_count++;
		}
	}

	char *sig = tsprint("{s} {s}({s})", return_type, name, param_count == 0 ? "void" : params_buf);
    sclose(p_str);
    // free(params_buf);

	codegen_location(ctx->decls_stream, node->base.location);
	sprintln(ctx->decls_stream, "{s};", sig);

	// Generate body
	if (node->body != NULL) {
		codegen_location(ctx->impls_stream, node->base.location);
		sprintln(ctx->impls_stream, "{s}\n{", sig);

		char *body_buf = NULL;
		stream_t prev = ctx->current_block_stream;
		ctx->current_block_stream = sdynmemopen(ctx->allocator, &body_buf);

		const char *body_val = codegen_expr(ctx, node->body);
		
		if (body_val != NULL) {
			sprintln(ctx->current_block_stream, "return {s};", body_val);
		}

		sprintln(ctx->impls_stream, "{s}}", body_buf);

		sclose(ctx->current_block_stream);
		ctx->current_block_stream = prev;
	}

	return name;
}

static Error codegen_global_node(struct codegen_context *ctx, const struct haste_ast_node *node)
{
	switch (node->kind) {
	case ND_VAR_DECL:
		codegen_var(ctx, (void*)node, true);
		break;
	case ND_FUNC_DECL:
		codegen_func_decl(ctx, (void*)node);
		break;
	default: unreachable();
	}

	reset_temporary_allocator();
	return OK;
}

// ── Entry point ──────────────────────────────────────────────────

Error codegen(
	struct Allocator allocator,
	const source_file_id src,
	const char *output_path,
	bool dump_to_stderr)
{
	struct codegen_context ctx = {0};
	ctx.allocator = allocator;

	char *structs_buf = NULL;
	char *globals_buf = NULL;
	char *decls_buf   = NULL;
	char *impls_buf   = NULL;

	ctx.structs_stream = sdynmemopen(allocator, &structs_buf);
	ctx.globals_stream = sdynmemopen(allocator, &globals_buf);
	ctx.decls_stream   = sdynmemopen(allocator, &decls_buf);
	ctx.impls_stream   = sdynmemopen(allocator, &impls_buf);

	leach (struct haste_ast_node, node, get_source_file_ast(src)) {
		codegen_global_node(&ctx, node);
	}

	stream_t out = {0};
	if (output_path) {
		out = sopen(output_path, "w");
	} else if (dump_to_stderr) {
		out = serr;
	} else {
		out = sout;
	}

	sprintln(out, "#include <stdint.h>\n");
	sprintln(out, "/* Struct Defs */\n{s}", structs_buf);
	sprintln(out, "/* Globals */\n{s}", globals_buf);
	sprintln(out, "/* Function Decls */\n{s}", decls_buf);
	sprintln(out, "/* Function Impls */\n{s}", impls_buf);

	if (output_path) {
		sclose(out);
	}

	/* free(structs_buf); */
	/* free(globals_buf); */
	/* free(decls_buf); */
	/* free(impls_buf); */
	context_deinit(&ctx);

	return OK;
}
