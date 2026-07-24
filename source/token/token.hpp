/** Created in: 19/55/2026 16:06
  *
  */
#ifndef TOKEN_H_
#define TOKEN_H_

#include "utils/location.hpp"

namespace haste {

enum class TokenKind : std::uint8_t {
	KwConst,    // "const"
	KwVar,      // "var"
	KwAuto,     // "auto"
	KwInt,      // "int"
	KwFloat,    // "float"

	Identifier,

	Newline,    // \n
	Semicolon,  // ;
	Colon,      // :

	Assign,     // =
	OpenParen,  // (
	CloseParen, // )
	OpenBrace,  // {
	CloseBrace, // }
	Plus,       // +
	Minus,      // -
	Star,       // *
	FSlash,     // /
	
	FloatLit,
	IntLit,
	Eof,
};

struct Token {
	TokenKind kind;
	Location location;
};

StringView as_string(Token self);

std::ostream &operator<<(std::ostream &os, const TokenKind &token_kind);
std::ostream &operator<<(std::ostream &os, const Token &token);

}; // namespace haste

#endif /* !TOKEN_H_ */
