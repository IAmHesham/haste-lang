#include "options.hpp"
#include <string.h>
#include <stdio.h>
#include <iostream>

Options g_options;

static void print_usage(std::ostream& os, const char *prog)
{
	os << "Usage: " << prog << " [options] [file]\n";
	os << "Options:\n";
	os << "  --tokens      Dump token stream and exit\n";
	os << "  --ast         Dump AST after parsing/hoisting and exit\n";
	os << "  --sema        Dump semantic analysis result and exit\n";
	os << "  --c           Dump C code and exit\n";
	os << "  --dump        Write dump output to stderr instead of a file\n";
	os << "  -o <file>     Write dump output to <file>\n";
	os << "  --measure     Show timing report for each compiler phase\n";
	os << "  --no-fun      Enable it if you hate fun\n";
	os << "  --help        Show this help message and exit\n";
}

void parse_arguments(const int argc, const char **argv)
{
	g_options = {};
	g_options.source_path = NULL;
	g_options.output_path = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--tokens") == 0) {
			g_options.dump_tokens = true;
		} else if (strcmp(argv[i], "--ast") == 0) {
			g_options.dump_ast = true;
		} else if (strcmp(argv[i], "--sema") == 0) {
			g_options.dump_sema = true;
		} else if (strcmp(argv[i], "--c") == 0) {
			g_options.dump_c = true;
		} else if (strcmp(argv[i], "--measure") == 0) {
			g_options.do_measure = true;
		} else if (strcmp(argv[i], "--dump") == 0) {
			g_options.do_dump = true;
		} else if (strcmp(argv[i], "-o") == 0) {
			i += 1;
			if (i >= argc) {
				fprintf(stderr, "error: '-o' requires a file argument.\n");
				throw;
			}
			g_options.output_path = argv[i];
		} else if (strcmp(argv[i], "--no-fun") == 0) {
			g_options.disable_fun = true;
		} else if (strcmp(argv[i], "--help") == 0) {
			print_usage(std::cout, argv[0]);
			exit(0);
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "error: unknown option '%s'\n\n", argv[i]);
			print_usage(std::cerr, argv[0]);
			throw;
		} else {
			g_options.source_path = argv[i];
		}
	}

	if (g_options.source_path == NULL) {
		print_usage(std::cerr, argv[0]);
		fprintf(stderr, "\nerror: expected a file. provided none.\n");
		throw;
	}
}
