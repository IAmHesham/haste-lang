/** Created in: 20/52/2026 21:06
  *
  */
#ifndef PARSER_H_
#define PARSER_H_

#include "ast/ast.hpp"
#include "token/token.hpp"
#include "utils/source_manager.hpp"

namespace haste {

struct ParsingException : std::exception {};

struct Parser {
	InternPool &pool;
	DynamicArray<Token> tokens;
	size_t current = 0;
	size_t errors = 0;

	Parser(InternPool &pool, DynamicArray<Token> tokens);
};

const ast::Node *parse(Parser &self);
const ast::Node *parse(InternPool &pool, DynamicArray<Token> tokens);

}; // namespace haste

#endif /* !PARSER_H_ */
