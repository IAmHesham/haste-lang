#ifndef TOKEN_H_
#define TOKEN_H_

#include "common.h"
#include "utils/location.h"
#include "my_stream.h"

enum token_kind {
	TK_IDENT = 1,
	TK_KW_STRING,
	TK_KW_CSTR,
	TK_KW_INT_BITS,
	TK_KW_UINT_BITS,
	TK_KW_INT,
	TK_KW_UINT,
	TK_KW_FLOAT,
	TK_KW_USIZE,
	TK_KW_VOID,
	TK_KW_AUTO,
	TK_KW_TYPE,
	TK_KW_CAST,
	TK_KW_CONST,
	TK_KW_VAR,
	TK_KW_STRUCT,
	TK_KW_DISTINCT,
	TK_KW_DO,
	TK_KW_END,

	TK_SEMI_COLON,

	TK_OPEN_BRAKET,
	TK_CLOSE_BRAKET,
	TK_OPEN_PAREN,
	TK_CLOSE_PAREN,

	TK_OPEN_BRACE,
	TK_CLOSE_BRACE,

	TK_COLON,
	TK_EQ,
	TK_COMMA,
	TK_DOT,

	TK_PLUS,
	TK_MINUS,
	TK_STAR,
	TK_FSLASH,

	TK_INT,
	TK_FLOAT,
	TK_STR,
	TK_EOF,
};

struct token {
	uint32_t start;
	uint32_t len;
	source_file_id src;
	enum token_kind kind : 8;

	union {
		int64_t ival;
		double fval;
		const char *str;
		const char *ident;
	};
};

#define token(kind_, start_, end_, ...) \
	(struct token) { \
		.kind = (kind_), \
		.start = (start_), \
		.len = (end_) - (start_), \
		__VA_ARGS__ \
	}

struct token_list {
	size_t len, cap;
	struct token *items;
};

int print_token(stream_t stream, struct token token);

struct string token_as_string(struct token token);
struct string token_kind_as_string(enum token_kind kind);

const char *token_kind_name(enum token_kind kind);

struct location as_location(struct token token);
struct file_position as_position(struct location location);

#endif
