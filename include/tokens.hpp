#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>


enum class TokenType {
	// tokens
	OpenParen = 0, // '('
	CloseParen, // ')'
	OpenCurlyBrase, // '{'
	CloseCurlyBrase, // '}'
	OpenSquareBracket, // '['
	CloseSquareBracket, // ']'

	Plus, // '+'
	Minus, // '-'
	Star, // '*'
	FSlash, // '/'
	BSlash, // '\'
	
	Almost, // '~='
	Eq, // '='
	EqEq, // '=='
	BangEq, // '!='
	LessThan, // '<'
	GreaterThan, // '>'
	LessThanOrEq, // '<='
	GreaterThanOrEq, // '>='
	Not, // '!' 'not'
	And, // '&&' 'and'
	Or, // '||' 'or'
	
	BitwiseAnd, // '&'
	BitwiseOr, // '|'
	BitwiseNot, // '~'
	BitWiseLeftShift, // '<<'
	BitWiseRightShift, // '<<'
	
	Dot, // '.'
	SemiColon, // ';'
	Colon, // ':'
	// Literals
	IntLit,
	FloatLit,
	ComplexLit,
	StringLit,
	CharLit,
	Identifier,
	// Keywords
	NotVar,
	Var,
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
			stream << token_tostring(type);
			stream << ", \"";
			stream << value;
			stream << "\", ";
			stream << std::to_string(line);
			stream << ", ";
			stream << std::to_string(column);
			stream << ", ";
			stream << std::to_string(length);
			stream << ")";
			return stream.str();
		}
		static std::string token_tostring(TokenType token) {
			switch (token) {
				case TokenType::OpenParen: return "OpenParen";
				case TokenType::CloseParen: return "CloseParen";
				case TokenType::OpenCurlyBrase: return "OpenCurlyBrase";
				case TokenType::CloseCurlyBrase: return "CloseCurlyBrase";
				case TokenType::OpenSquareBracket: return "OpenSquareBracket";
				case TokenType::CloseSquareBracket: return "CloseSquareBracket";

				case TokenType::Plus: return "Plus";
				case TokenType::Minus: return "Minus";
				case TokenType::Star: return "Star";
				case TokenType::FSlash: return "FSlash";
				case TokenType::BSlash: return "BSlash";

				case TokenType::Almost: return "Almost";
				case TokenType::Eq: return "Eq";
				case TokenType::EqEq: return "EqEq";
				case TokenType::BangEq: return "BangEq";
				case TokenType::LessThan: return "LessThan";
				case TokenType::GreaterThan: return "GreaterThan";
				case TokenType::LessThanOrEq: return "LessThanOrEq";
				case TokenType::GreaterThanOrEq: return "GreaterThanOrEq";
				case TokenType::Not: return "Not";
				case TokenType::And: return "And";
				case TokenType::Or: return "Or";

				case TokenType::BitwiseAnd: return "BitwiseAnd";
				case TokenType::BitwiseOr: return "BitwiseOr";
				case TokenType::BitwiseNot: return "BitwiseNot";
				case TokenType::BitWiseLeftShift: return "BitWiseLeftShift";
				case TokenType::BitWiseRightShift: return "BitWiseRightShift";

				case TokenType::Dot: return "Dot";
				case TokenType::SemiColon: return "SemiColon";
				case TokenType::Colon: return "Colon";

				case TokenType::IntLit: return "IntLit";
				case TokenType::FloatLit: return "FloatLit";
				case TokenType::ComplexLit: return "ComplexLit";
				case TokenType::StringLit: return "StringLit";
				case TokenType::CharLit: return "CharLit";
				case TokenType::Identifier: return "Identifier";

				case TokenType::NotVar: return "NotVar";
				case TokenType::Var: return "Var";
				case TokenType::Func: return "Func";
				case TokenType::If: return "If";
				case TokenType::Else: return "Else";
				case TokenType::Is: return "Is";

				default: return "Unknown";
			}
		}

};

typedef std::vector<Token> TokenList;
