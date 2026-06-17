#ifndef SOURCE_H_
#define SOURCE_H_

#include "common.h"
#include "my_allocator.h"

struct haste_ast_node;

enum source_file_type {
	SRC_HASTE,
	SRC_UNKNOWN,
};

struct source_file {
	char *path;
	char *content;
	size_t len;
	enum source_file_type type;
	struct haste_ast_node *root;
};

struct source_file_list {
	struct Allocator allocator;
	size_t cap, len;
	struct source_file *items;
};

extern struct source_file_list sources;

char* get_current_working_directory(void);

char *get_absolute_path(struct Allocator allocator, const char* relative_path);

char *read_entire_file(struct Allocator allocator, const char *path);

enum source_file_type get_file_type(const char *path);

source_file_id obtain_source_file_id(const char *base, const char *path);

struct source_file get_source_file(const source_file_id id);

const char *get_source_file_path(const source_file_id id);

size_t get_source_file_len(const source_file_id id);

const char *get_source_file_content(const source_file_id id);

const char *get_source_file_end(const source_file_id id);

enum source_file_type get_source_file_type(const source_file_id id);

struct haste_ast_node *get_source_file_ast(const source_file_id id);

#endif
