#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

enum class TokenType {
	// tokens
	OpenParen, // '('
	CloseParen, // ')'
	Plus, // '+'
	Minus, // '-'
	Star, // '*'
	FSlash, // '/'
	BSlash, // '\'
	Eq, // '='
	EqEq, // '=='
	BangEq, // '!='
	Not, // '!' 'not'
	// Literals
	IntLit,
	FloatLit,
	ComplexLit,
	StringLit,
	Identifier,
	// Keywords
	Var,
	NotVar,
	Func,
	If,
	Else,
	Is,
};

class Token {
public:
	TokenType type = (TokenType) 0;
	std::string value;
	std::size_t line = 1;
	std::size_t column = 1;
	std::size_t length = 1;

	Token() = default;
	Token(TokenType type):
		type(type) {}
	Token(TokenType type, std::string value):
		type(type), value(value) {}
	Token(TokenType type, std::string value, std::size_t line):
		type(type), value(value), line(line) {}
	Token(TokenType type, std::string value, std::size_t line, std::size_t column):
		type(type), value(value), line(line), column(column) {}
	Token(TokenType type, std::string value, std::size_t line, std::size_t column, std::size_t length):
		type(type), value(value), line(line), column(column), length(length) {}
	
	std::string to_string() {
		std::stringstream stream;
		stream << "Token(";
		stream << std::to_string((int) type);
		stream << ", ";
		stream << value;
		stream << ", ";
		stream << std::to_string(line);
		stream << ", ";
		stream << std::to_string(column);
		stream << ", ";
		stream << std::to_string(length);
		stream << ")";
		return stream.str();
	}
};

typedef std::vector<Token> TokenList;
