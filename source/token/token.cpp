#include "token.hpp"
#include "utils/helper.hpp"
#include <iomanip>

namespace haste {

StringView as_string(Token self)
{
	return as_string(self.location);
}

std::ostream &operator<<(std::ostream &os, const TokenKind &token_kind)
{
	switch (token_kind) {
	case TokenKind::KwConst:    os << "KwConst";    break;
	case TokenKind::KwVar:      os << "KwVar";      break;
	case TokenKind::KwAuto:     os << "KwAuto";     break;
	case TokenKind::KwInt:      os << "KwInt";      break;
	case TokenKind::KwFloat:    os << "KwFloat";    break;
	case TokenKind::Identifier: os << "Identifier"; break;
	case TokenKind::Newline:    os << "Newline";    break;
	case TokenKind::Semicolon:  os << "Semicolon";  break;
	case TokenKind::Colon:      os << "Colon";      break;
	case TokenKind::Assign:     os << "Assign";     break;
	case TokenKind::OpenParen:  os << "OpenParen";  break;
	case TokenKind::CloseParen: os << "CloseParen"; break;
	case TokenKind::OpenBrace:  os << "OpenBrace";  break;
	case TokenKind::CloseBrace: os << "CloseBrace"; break;
	case TokenKind::Plus:       os << "Plus";       break;
	case TokenKind::Minus:      os << "Minus";      break;
	case TokenKind::Star:       os << "Star";       break;
	case TokenKind::FSlash:     os << "FSlash";     break;
	case TokenKind::FloatLit:   os << "FloatLit";   break;
	case TokenKind::IntLit:     os << "IntLit";     break;
	case TokenKind::Eof:        os << "Eof";        break;
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const Token &token)
{
	os << std::setw(3) << token.location.line << ":" << std::setw(2) << token.location.column << ": "
	   << std::left << std::setw(14) << token.kind
	   << std::right
	   << quoted(as_string(token));
	return os;
}

}; // namespace haste
