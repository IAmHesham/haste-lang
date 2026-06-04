#include "haste.h"
#include <ctype.h>
#include <stdint.h>
#include <string.h>

struct string string_to_trimed(struct string span)
{
	struct string result = span;

	if (result.len == 0) {
		return result;
	}

	while (result.len > 0 and isspace(result.chars[result.len - 1])) {
		result.len -= 1;
	}

	while (isspace(*result.chars) and *result.chars != '\0' and result.len != 0) {
		result.chars += 1;
		result.len -= 1;
	}

	return result;
}

struct string cstr_as_string(const char *cstr)
{
	uint32_t len = strlen(cstr);
	return string(
		.len = len,
		.chars = cstr);
}

struct string location_as_string(struct location location)
{
	const char *source = get_source_file_content(location.len);
	return string(
		.chars = source + location.start,
		.len = location.len);
}

struct string token_as_string(struct token token)
{
	const char *source = get_source_file_content(token.src);
	return string(
		.chars = source + token.start,
		.len = token.len);
}

struct string string_as_string(struct string s)
{
	return s;
}

struct string token_kind_as_string(enum token_kind kind)
{
	return cstr_as_string(token_kind_name(kind));
}

struct location as_location(struct token token)
{
	return location(
		.start = token.start,
		.len = token.len,
		.src = token.src);
}

static uint32_t get_line_number(const char *content, const char *pos)
{
	uint32_t line = 1;
	for (const char *cur = pos; cur > content; cur -= 1) {
		if (*cur == '\n') {
			line += 1;
		}
	}

	return line;
}

static uint32_t get_column_number(source_file_id src, const char *content, const char *pos)
{
	const char *line_start = pos;
	while (line_start > content and line_start[-1] != '\n') {
		line_start -= 1;
	}
	return display_width(line_start, (int)(pos - line_start), src) + 1;
}

struct file_position as_position(struct location location)
{
	const char *content = get_source_file_content(location.src);
	return file_position(
		.line = get_line_number(content, content + location.start),
		.column = get_column_number(location.src, content, content + location.start),
		.src = location.src);
}

struct location location_conjoin(
    struct location a,
    struct location b)
{
    uint32_t a_end = a.start + a.len;
    uint32_t b_end = b.start + b.len;
    uint32_t start = a.start < b.start ? a.start : b.start;
    uint32_t end   = a_end  > b_end   ? a_end   : b_end;
    return location(
        .start = start,
        .len   = end - start,
        .src   = a.src);
}
