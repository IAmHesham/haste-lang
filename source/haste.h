#ifndef HASTE_H_
#define HASTE_H_

#include "cwalk.h"
#include "my_allocator.h"
#include "my_common.h"
#include "my_c_allocator.h"
#include "my_arena_allocator.h"
#include "my_temporary_allocator.h"
#include "my_hashtable.h"
#include "my_array.h"
#include "my_managed_array.h"
#include "my_stream.h"

#include "common.h"
#include "driver/options.h"
#include "utils/source.h"
#include "utils/location.h"
#include "utils/unicode.h"
#include "lexer/token.h"
#include "lexer/token_stream.h"
#include "utils/intern.h"
#include "utils/value.h"
#include "utils/type.h"
#include "ast/ast.h"
#include "diagnostics/error.h"
#include "parser/parse.h"
#include "analysis/analysis.h"
#include "codegen/codegen.h"
#include "compiler/compiler.h"

#define as_string(...) \
	_Generic((__VA_ARGS__), \
		const char *: cstr_as_string, \
		char *: cstr_as_string, \
		struct location: location_as_string, \
		struct token: token_as_string, \
		struct string: string_as_string, \
		enum token_kind: token_kind_as_string \
	) (__VA_ARGS__)

#endif
