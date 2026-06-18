#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "common.h"

struct options {
	bool dump_tokens : 1;
	bool dump_ast    : 1;
	bool dump_sema   : 1;
	bool dump_c      : 1;
	bool do_measure  : 1;
	bool do_dump     : 1;
	bool disable_fun : 1;
	const char *source_path;
	const char *output_path;
};

extern struct options g_options;

Error parse_arguments(const int argc, const char *argv[argc]);

#endif
