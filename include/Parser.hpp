#pragma once

#include "AST/Statments.hpp"
#include "tokens.hpp"
#include <cstddef>

class Parser {
public:
	Parser(TokenList tokens);

	AST::ASTMain parser();

private:
	TokenList m_tokens;
	std::size_t m_current = 0;

	Token peek();
	Token peek(int ahead);
};

