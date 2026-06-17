#ifndef TOKEN_STREAM_H_
#define TOKEN_STREAM_H_

#include "token.h"

#define STREAM_DATA_COUNT 512

struct token_stream {
	struct token items[STREAM_DATA_COUNT];
	size_t read_cursor, write_cursor;

	source_file_id src;
	const char *content, *end;
	uint32_t start, current;
	bool has_error : 1;
	bool ended : 1;
};

struct token_stream token_stream(source_file_id src);

bool token_stream_ended(const struct token_stream *stream);
struct token token_stream_peek(struct token_stream *stream);
struct token token_stream_peek_next(struct token_stream *stream);
struct token token_stream_advance(struct token_stream *stream);

#endif
