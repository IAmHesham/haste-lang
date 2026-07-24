#ifndef OPTIONS_H_
#define OPTIONS_H_

struct Options {
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

extern struct Options g_options;

void parse_arguments(int argc, const char **argv);

#endif
