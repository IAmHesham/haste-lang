#ifndef LEXER_H_
#define LEXER_H_

#include "token/token.hpp"
#include "containers/dynamic_array.hpp"
#include "utils/source_manager.hpp"
#include "utils/intern.hpp"

namespace haste {

struct LexingException : std::exception {};

struct Lexer {
	InternPool &pool;
	SourceFile *source    = nullptr;
	std::size_t errors    = 0;
	std::size_t cursor    = 0;
	std::uint32_t line    = 1;
	std::uint32_t column  = 1;

	static Lexer init(InternPool &pool, SourceFile &source);
};

Token next(Lexer &self);
DynamicArray<Token> tokenize(InternPool &pool, SourceFile &source);
DynamicArray<Token> tokenize(Lexer &lexer);

} // namespace haste

#endif
