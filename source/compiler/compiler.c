#include "haste.h"
#include "compiler/compiler.h"
#include "lexer/token_stream.h"
#include "ast/ast.h"
#include "parser/parse.h"
#include "analysis/analysis.h"
#include "codegen/codegen.h"
#include "utils/source.h"

static int cm_file_write(void *data, const unsigned char *in, size_t count)
{
	return (int)fwrite(in, 1, count, (FILE*)data);
}

static int cm_file_flush(void *data)
{
	return fflush((FILE*)data);
}

static int cm_file_close(void *data) { (void)data; return 0; }
static int cm_file_read(void *data, unsigned char *out, size_t amount) { (void)data; (void)out; (void)amount; return 0; }
static int cm_file_seek(void *data, long offset, int whence) { (void)data; (void)offset; (void)whence; return 0; }

static const stream_interface_t cm_file_vtable = {
	.close = cm_file_close,
	.read  = cm_file_read,
	.write = cm_file_write,
	.seek  = cm_file_seek,
	.flush = cm_file_flush,
};

void haste_compiler_set_fun(struct haste_compiler *compiler, bool enable)
{
	compiler->enable_fun = enable;
}

Error haste_compiler_add_source(struct haste_compiler *compiler, const char *path)
{
	compiler->arena = Arena(compiler->allocator);
	compiler->pool = init_intern_pool(compiler->allocator, arena_get_allocator(&compiler->arena));
	setup_builtins(&compiler->pool);

	compiler->timers = (struct timer_list){
		.allocator = compiler->allocator,
	};

	sources.allocator = compiler->allocator;
	source_file_id src = obtain_source_file_id(NULL, path);
	if (src < 0) return ERROR;
	compiler->src = src;
	return OK;
}

Error haste_compiler_dump_tokens(struct haste_compiler *compiler, FILE *f)
{
	timer_start(&compiler->timers, "lexing");

	struct token_stream ts = token_stream(compiler->src);
	ts.arena = arena_get_allocator(&compiler->arena);
	stream_t out = f ? (stream_t){ .data = f, .vtable = &cm_file_vtable } : sout;

	while (!token_stream_ended(&ts)) {
		struct token tok = token_stream_peek(&ts);
		print_token(out, tok);
		sprint(out, "\n");
		token_stream_advance(&ts);
	}

	timer_stop(&compiler->timers, 0);
	return OK;
}

Error haste_compiler_dump_ast(struct haste_compiler *compiler, FILE *f)
{
	timer_start(&compiler->timers, "lexing");
	struct token_stream ts = token_stream(compiler->src);
	ts.arena = arena_get_allocator(&compiler->arena);
	token_stream_ended(&ts);
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "parsing");
	if (parse(&compiler->pool, compiler->src)) return ERROR;
	timer_stop(&compiler->timers, 0);

	stream_t out = f ? (stream_t){ .data = f, .vtable = &cm_file_vtable } : sout;
	print_haste_ast(out, get_source_file_ast(compiler->src));
	sprint(out, "\n");
	return OK;
}

Error haste_compiler_dump_sema(struct haste_compiler *compiler, FILE *f)
{
	timer_start(&compiler->timers, "lexing");
	struct token_stream ts = token_stream(compiler->src);
	ts.arena = arena_get_allocator(&compiler->arena);
	token_stream_ended(&ts);
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "parsing");
	if (parse(&compiler->pool, compiler->src)) return ERROR;
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "analysis");
	if (analyze(&compiler->pool, compiler->src)) return ERROR;
	timer_stop(&compiler->timers, 0);

	stream_t out = f ? (stream_t){ .data = f, .vtable = &cm_file_vtable } : sout;
	print_haste_ast(out, get_source_file_ast(compiler->src));
	sprint(out, "\n");
	return OK;
}

Error haste_compiler_dump_c(struct haste_compiler *compiler, FILE *f)
{
	timer_start(&compiler->timers, "lexing");
	struct token_stream ts = token_stream(compiler->src);
	ts.arena = arena_get_allocator(&compiler->arena);
	token_stream_ended(&ts);
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "parsing");
	if (parse(&compiler->pool, compiler->src)) return ERROR;
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "analysis");
	if (analyze(&compiler->pool, compiler->src)) return ERROR;
	timer_stop(&compiler->timers, 0);

	timer_start(&compiler->timers, "codegen");
	Error err = codegen(compiler->allocator, compiler->src, NULL, false, f);
	timer_stop(&compiler->timers, 0);
	return err;
}

void haste_compiler_dump_measure(struct haste_compiler *compiler, FILE *f)
{
	print_timing_report(f, compiler->timers);
}

void haste_compiler_deinit(struct haste_compiler *compiler)
{
	deinit_intern_pool(&compiler->pool);
	arena_free(&compiler->arena);
	marrfree(compiler->timers);
}
