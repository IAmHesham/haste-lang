#ifndef LOCATION_H_
#define LOCATION_H_

#include "common.h"

struct string {
	const char *chars;
	uint32_t len;
};

struct location {
	uint32_t start;
	uint32_t len;
	source_file_id src;
};

struct file_position {
	uint32_t line, column;
	source_file_id src;
};

#define location(...) ((struct location) { __VA_ARGS__ })
#define file_position(...) ((struct file_position) { __VA_ARGS__ })
#define string(...) ((struct string) { __VA_ARGS__ })

struct string string_to_trimed(struct string span);

struct string cstr_as_string(const char *cstr);
struct string location_as_string(struct location location);
struct string string_as_string(struct string s);

struct location location_conjoin(
    struct location a,
    struct location b);

#endif
