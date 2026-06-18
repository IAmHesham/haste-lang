#define MY_COMMONS_IMPLEMENTATION
#define MY_ALLOCATOR_IMPL
#define MY_ARENA_ALLOCATOR_IMPL
#define MY_C_ALLOCATOR_IMPL
#define MY_TEMPORARY_ALLOCATOR_IMPL
#define MY_HASHTABLE_IMPL
#define MY_STREAM_IMPL
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <locale.h>
#endif
#include "haste.h"
#include "my_timing.h"
#include "cwalk.h"


static void print_errno(void)
{
	fprintf(stderr, " * [%d] %s\n", errno, strerror(errno));
}

static int custom_format_string(stream_t stream, struct modifier_stream mod, va_list args)
{
	discard mod;
	struct string string = va_arg(args, struct string);
	if (string.chars == NULL) {
		return sprint(stream, "(nil)");
	}

	if (match_modifier(&mod, '#')) {
		return sprint(stream, "{s:#*}", string.chars, (int)string.len);
	}
	return sprint(stream, "{s:*}", string.chars, (int)string.len);
}

static int custom_format_token(stream_t stream, struct modifier_stream mod, va_list args)
{
	struct token token = va_arg(args, struct token);
	if (match_modifier(&mod, '#')) {
		if (match_modifier(&mod, '#')) {
			return sprint(stream, "{string:#}", as_string(token));
		}
		return print_token(stream, token);
	}
	return sprint(stream, "{string}", as_string(token));
}

static int custom_format_value(stream_t stream, struct modifier_stream mod, va_list args)
{
	discard mod;
	struct haste_value value = va_arg(args, struct haste_value);
	return print_value(stream, value);
}

static int custom_format_object(stream_t stream, struct modifier_stream mod, va_list args)
{
	discard mod;
	struct haste_value object = va_arg(args, struct haste_value);
	return print_object(stream, object.obj, typeof_value(object));
}

static int custom_format_ast(stream_t stream, struct modifier_stream mod, va_list args)
{
	discard mod;
	struct haste_ast_node *node = va_arg(args, struct haste_ast_node *);
	return print_haste_ast(stream, node);
}

int main(int argc, char *argv[argc])
{
	setup_io_stream();

	define_format_specifier("string", custom_format_string);
	define_format_specifier("token", custom_format_token);
	define_format_specifier("value", custom_format_value);
	define_format_specifier("obj", custom_format_object);
	define_format_specifier("ast", custom_format_ast);

	if (parse_arguments(argc, (const char **)argv)) return 1;

	srand((unsigned int)time(NULL));
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#else
	setlocale(LC_ALL, "C.UTF-8");
#endif

	struct Allocator c_allocator = get_c_allocator();
	set_default_allocator(c_allocator);

	struct haste_compiler compiler = {0};
	compiler.allocator = c_allocator;

	haste_compiler_set_fun(&compiler, !g_options.disable_fun);

	if (haste_compiler_add_source(&compiler, g_options.source_path)) {
		return 1;
	}

	if (g_options.dump_tokens) {
		const char *ext = ".tokens";
		FILE *f = NULL;
		if (g_options.do_dump) {
			f = stderr;
		} else if (g_options.output_path) {
			f = fopen(g_options.output_path, "w");
		} else {
			char buf[4096];
			cwk_path_change_extension(g_options.source_path, ext, buf, sizeof(buf));
			f = fopen(buf, "w");
		}
		if (f) {
			haste_compiler_dump_tokens(&compiler, f);
			if (!g_options.do_dump) fclose(f);
		}
		if (g_options.do_measure) {
			haste_compiler_dump_measure(&compiler, stdout);
		}
		haste_compiler_deinit(&compiler);
		return 0;
	}

	if (g_options.dump_ast) {
		const char *ext = ".ast.json";
		FILE *f = NULL;
		if (g_options.do_dump) {
			f = stderr;
		} else if (g_options.output_path) {
			f = fopen(g_options.output_path, "w");
		} else {
			char buf[4096];
			cwk_path_change_extension(g_options.source_path, ext, buf, sizeof(buf));
			f = fopen(buf, "w");
		}
		if (f && haste_compiler_dump_ast(&compiler, f)) {
			if (!g_options.do_dump) fclose(f);
			haste_compiler_deinit(&compiler);
			return 1;
		}
		if (!g_options.do_dump && f) fclose(f);
		if (g_options.do_measure) {
			haste_compiler_dump_measure(&compiler, stdout);
		}
		haste_compiler_deinit(&compiler);
		return 0;
	}

	if (g_options.dump_sema) {
		const char *ext = ".sema.json";
		FILE *f = NULL;
		if (g_options.do_dump) {
			f = stderr;
		} else if (g_options.output_path) {
			f = fopen(g_options.output_path, "w");
		} else {
			char buf[4096];
			cwk_path_change_extension(g_options.source_path, ext, buf, sizeof(buf));
			f = fopen(buf, "w");
		}
		if (f && haste_compiler_dump_sema(&compiler, f)) {
			if (!g_options.do_dump) fclose(f);
			haste_compiler_deinit(&compiler);
			return 1;
		}
		if (!g_options.do_dump && f) fclose(f);
		if (g_options.do_measure) {
			haste_compiler_dump_measure(&compiler, stdout);
		}
		haste_compiler_deinit(&compiler);
		return 0;
	}

	if (g_options.dump_c) {
		const char *ext = ".c";
		FILE *f = NULL;
		if (g_options.do_dump) {
			f = stderr;
		} else if (g_options.output_path) {
			f = fopen(g_options.output_path, "w");
		} else {
			char buf[4096];
			cwk_path_change_extension(g_options.source_path, ext, buf, sizeof(buf));
			f = fopen(buf, "w");
		}
		if (f && haste_compiler_dump_c(&compiler, f)) {
			if (!g_options.do_dump) fclose(f);
			haste_compiler_deinit(&compiler);
			return 1;
		}
		if (!g_options.do_dump && f) fclose(f);
		if (g_options.do_measure) {
			haste_compiler_dump_measure(&compiler, stdout);
		}
		haste_compiler_deinit(&compiler);
		return 0;
	}

	// Normal compilation
	{
		Error err;

		timer_start(&compiler.timers, "parsing");
		err = parse(&compiler.pool, compiler.src);
		timer_stop(&compiler.timers, 0);
		if (err) {
			if (g_options.do_measure) haste_compiler_dump_measure(&compiler, stdout);
			haste_compiler_deinit(&compiler);
			return 1;
		}

		timer_start(&compiler.timers, "analysis");
		err = analyze(&compiler.pool, compiler.src);
		timer_stop(&compiler.timers, 0);
		if (err) {
			if (g_options.do_measure) haste_compiler_dump_measure(&compiler, stdout);
			haste_compiler_deinit(&compiler);
			return 1;
		}

		timer_start(&compiler.timers, "codegen");
		err = codegen(compiler.allocator, compiler.src, NULL, false, NULL);
		timer_stop(&compiler.timers, 0);
		if (err) {
			if (g_options.do_measure) haste_compiler_dump_measure(&compiler, stdout);
			haste_compiler_deinit(&compiler);
			return 1;
		}
	}

	if (g_options.do_measure) haste_compiler_dump_measure(&compiler, stdout);
	haste_compiler_deinit(&compiler);
	return 0;
}
