#ifndef AST_H_
#define AST_H_

#include "utils/type.h"
#include "lexer/token.h"
#include "my_stream.h"

enum haste_ast_node_kind {
	ND_VALUE,

	ND_INTEGER_LIT,
	ND_FLOAT_LIT,
	ND_STRING_LIT,
	ND_IDENT,

	ND_BINARY,
	ND_UNARY,
	ND_ACCESS,
	ND_INT_BITS,
	ND_UINT_BITS,
	ND_GROUPING,
	ND_DISTINCT,
	ND_CAST,

	ND_STRING,
	ND_CSTR,
	ND_INT,
	ND_UINT,
	ND_FLOAT,
	ND_USIZE,
	ND_VOID,
	ND_AUTO,
	ND_TYPE,

	ND_STRUCT_TYPE,
	ND_STRUCT_FIELD,
	ND_STRUCT_LITERAL,
	ND_STRUCT_LIT_FIELD,

	ND_VAR_DECL,
	ND_BLOCK,
};

struct haste_ast_node {
	struct haste_ast_node *next;
	struct haste_type type;
	struct location location;
	enum haste_ast_node_kind kind : 8;
	bool analyzed : 1;
};

struct haste_ast_value {
	struct haste_ast_node base;
	struct haste_value value;
};

struct haste_ast_integer_lit {
	struct haste_ast_node base;
	int64_t value;
};

struct haste_ast_float_lit {
	struct haste_ast_node base;
	double value;
};

struct haste_ast_string_lit {
	struct haste_ast_node base;
	struct string value;
};

struct haste_ast_ident {
	struct haste_ast_node base;
	struct string value;
};

struct haste_ast_int_bits {
	struct haste_ast_node base;
	uint32_t bits;
};

struct haste_ast_uint_bits {
	struct haste_ast_node base;
	uint32_t bits;
};

struct haste_ast_grouping {
	struct haste_ast_node base;
	struct haste_ast_node *child;
};

struct haste_ast_distinct {
	struct haste_ast_node base;
	struct haste_ast_node *child;
	char padding[8];
};

struct haste_ast_binary {
	struct haste_ast_node base;
	struct haste_ast_node *lhs;
	struct haste_ast_node *rhs;
	enum token_kind op;
	struct location op_loc;
};

struct haste_ast_unary {
	struct haste_ast_node base;
	struct haste_ast_node *rhs;
	enum token_kind op;
	struct location op_loc;
};

struct haste_ast_access {
	struct haste_ast_node base;
	struct haste_ast_node *lhs;
	struct string field;
	struct location field_loc;
	size_t field_index;
};

struct haste_ast_cast {
	struct haste_ast_node base;
	struct haste_ast_node *to;
	struct haste_ast_node *expr;
};

struct haste_ast_struct_type {
	struct haste_ast_node base;
		struct haste_ast_struct_field {
			struct haste_ast_node base;
			size_t name_count;
			struct string *names;
			struct location *name_locs;
			struct haste_ast_node *type;
			struct haste_ast_node *default_value;

			struct haste_ast_struct_field *next;
		} *fields;
	char padding[8];
};

struct haste_ast_struct_literal {
	struct haste_ast_node base;
	struct haste_ast_node *type_expr;
	struct haste_ast_struct_lit_field {
		struct haste_ast_node base;
		struct string name;
		struct location name_loc;
		struct haste_ast_node *value;
		struct haste_ast_struct_lit_field *next;
	} *fields;
};

struct haste_ast_var_decl {
	struct haste_ast_node base;
	bool is_constant : 1;
	bool is_explicitly_comptime : 1;
	bool is_global : 1;
	struct string name;
	struct location name_loc;
	struct haste_ast_node *type;
	struct haste_ast_node *value;
};

struct haste_ast_block {
	struct haste_ast_node base;
	struct haste_ast_node *stmts;
	bool returning : 1;
};

void *node_into_value(
	struct intern_pool *pool,
	void *nd,
	struct haste_value value);
int print_haste_ast(stream_t file, const struct haste_ast_node *root);
bool node_is_declaration(const struct haste_ast_node *node);

#endif
