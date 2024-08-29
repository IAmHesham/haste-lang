#include "Parser.hpp"
#include "AST/Statments.hpp"
#include "tokens.hpp"

Parser::Parser(TokenList tokens): m_tokens(tokens) {}

AST::ASTMain Parser::parser() {
	AST::ASTMain result;
	return result;
}

Token Parser::peek() {
	return m_tokens[m_current];
}

Token Parser::peek(int ahead) {
	return m_tokens[m_current + ahead];
}


