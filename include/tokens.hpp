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
	Coma, // ','
	Colon, // ':'
	
	// Literals
	IntLit,
	FloatLit,
	ComplexLit,
	StringLit,
	CharLit,
	Identifier,
	SpicialIdentifier,
	
	// Keywords
	NotVar, // !var
	Var,
	Func,
	InlineFunc, // !func
	Return,

	//// Control flows
	If,
	Unless, // !if
	Else,
	Is,
	For,
	Loop,

	//// Class related
	Class,
	FinalClass, // !class

	Try,
	Catch,
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

		bool operator==(const TokenType& other) {
			return type == other;
		}

		bool operator==(std::string& other) {
			return value == other;
		}

		bool operator==(Token& other) {
			return type == other.type && value == other.value;
		}

		bool operator==(const TokenType& other) const {
			return type == other;

		}
		bool operator==(std::string& other) const {
			return value == other;
		}

		bool operator==(Token& other) const {
			return type == other.type && value == other.value;
		}

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
#define TM(token) case TokenType::token: return #token;
				TM(OpenParen)
				TM(CloseParen)
				TM(OpenCurlyBrase)
				TM(CloseCurlyBrase)
				TM(OpenSquareBracket)
				TM(CloseSquareBracket)

				TM(Plus)
				TM(Minus)
				TM(Star)
				TM(FSlash)
				TM(BSlash)

				TM(Almost)
				TM(Eq)
				TM(EqEq)
				TM(BangEq)
				TM(LessThan)
				TM(GreaterThan)
				TM(LessThanOrEq)
				TM(GreaterThanOrEq)
				TM(Not)
				TM(And)
				TM(Or)

				TM(BitwiseAnd)
				TM(BitwiseOr)
				TM(BitwiseNot)
				TM(BitWiseLeftShift)
				TM(BitWiseRightShift)

				TM(Dot)
				TM(SemiColon)
				TM(Colon)

				TM(IntLit)
				TM(FloatLit)
				TM(ComplexLit)
				TM(StringLit)
				TM(CharLit)
				TM(Identifier)
				TM(SpicialIdentifier)

				TM(NotVar)
				TM(Var)

				TM(Func)
				TM(Return)
				TM(InlineFunc)

				TM(If)
				TM(Unless)
				TM(Else)
				TM(Is)
				TM(For)

				TM(Class)
				TM(FinalClass)

				TM(Try)
				TM(Catch)
				

				default: return "Unknown";
			}
		}

};

typedef std::vector<Token> TokenList;
