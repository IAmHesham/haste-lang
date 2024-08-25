#include <cstddef>
#include "macros.hpp"
#include "tokens.hpp"
#include <Scanner.hpp>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

Scanner::Scanner(): m_content("") {
	setup_keywords();
}
Scanner::Scanner(std::string content): m_content(content) {
	setup_keywords();
}

void Scanner::reconstruct(std::string content) {
	m_content = content;
	m_current = 0;
	m_start = 0;
	m_line = 1;
	m_column = 1;
	m_tokens.clear();
}

void Scanner::setup_keywords() {
#define AK(keyword, tokenname) keywords[keyword] = TokenType::tokenname;
	AK("var", Var)
    AK("!var", NotVar)
    AK("func", Func)
    AK("return", Return)
    AK("!func", InlineFunc)
    AK("if", If)
    AK("!if", Unless)
    AK("else", Else)
    AK("is", Is)
    AK("not", Not)
    AK("or", Or)
    AK("and", And)
    AK("class", Class)
    AK("!class", FinalClass)
	AK("for", For)
	AK("loop", Loop)
	AK("try", Try)
	AK("catch", Catch)
}

TokenList Scanner::scan() {
	while (!at_end()) {
		m_start = m_current;
		char c = advance();
		switch (c) {
			case '(': add_token(TokenType::OpenParen); break;
			case ')': add_token(TokenType::CloseParen); break;
			case '{': add_token(TokenType::OpenCurlyBrase); break;
			case '}': add_token(TokenType::CloseCurlyBrase); break;
			case '[': add_token(TokenType::OpenSquareBracket); break;
			case ']': add_token(TokenType::CloseSquareBracket); break;
			case '+': add_token(TokenType::Plus); break;
			case '-': add_token(TokenType::Minus); break;
			case '*': add_token(TokenType::Star); break;
			case '/': add_token(TokenType::FSlash); break;
			case '\\': add_token(TokenType::BSlash); break;
			case '.': add_token(TokenType::Dot); break;
			case ';': add_token(TokenType::SemiColon); break;
			case ',': add_token(TokenType::Coma); break;
			case ':': add_token(TokenType::Colon); break;
			case '=': add_token(match('=') ? TokenType::EqEq : TokenType::Eq); break;
			case '&': add_token(match('&') ? TokenType::And : TokenType::BitwiseAnd); break;
			case '|': add_token(match('|') ? TokenType::Or : TokenType::BitwiseOr); break;
			case '~': add_token(match('=') ? TokenType::Almost : TokenType::BitwiseNot); break;
			case '<': add_token(match('=') ? TokenType::LessThanOrEq : match('<') ? TokenType::BitWiseLeftShift : TokenType::LessThan); break;
			case '>': add_token(match('=') ? TokenType::GreaterThanOrEq : match('>') ? TokenType::BitWiseRightShift : TokenType::GreaterThan); break;
			case '!':
				if (match('=')) {
					add_token(TokenType::BangEq);
				} else {
					Scanner::Ident ident = get_next_ident();
					std::string id = "!" + ident.value;
					if (keywords.contains(id)) {
						ident.skip();
						add_token(keywords[id]);
					} else {
						add_token(TokenType::Not);
					}
				}
				break;

			case '\n':
				m_line++;
				break;

			case ' ':
			case '\t':
			case '\r':
				break;

			default:
				if (is_alpha(c)) {
					scan_ident();
				} else if (is_digit(c)) {
					scan_numbers();
				} else if (c == '\'') {
				} else if (c == '"') {
					scan_string();
				}
				break;
		}
	}
	return m_tokens;
}

void Scanner::scan_ident() {
	if (peek(-1) == '$' && is('"')) {
		advance();
		scan_special();
		return;
	}
	while (is_alphanum()) advance();

	std::string buffer = m_content.substr(m_start, m_current - m_start);
	if (keywords.contains(buffer)) {
		add_token(keywords[buffer]);
		return;
	}
	add_token(TokenType::Identifier, buffer);
}

void Scanner::scan_special() {
	while (!is('"')) advance();
	advance();

	std::string buffer = m_content.substr(m_start + 2, m_current - m_start - 3);
	add_token(TokenType::SpicialIdentifier, buffer);
}

void Scanner::scan_numbers() {
	TokenType type = TokenType::IntLit;
	while (is_digit()) advance();

	if (peek() == '.') {
		type = TokenType::FloatLit;
		advance();
		while (is_digit() || is('f')) advance();
	}
	
	std::string buffer = m_content.substr(m_start, m_current - m_start);
	add_token(type, buffer);
}

void Scanner::scan_string() {
	advance();
	while (peek() != '"') {
		advance();
	}
	advance();

	std::string buffer = m_content.substr(m_start, m_current - m_start);
	add_token(TokenType::StringLit, buffer);
}

void Scanner::add_token(TokenType type) {
	add_token(type, m_content.substr(m_start, m_current - m_start));
}
void Scanner::add_token(TokenType type, std::string value) {
	m_tokens.push_back(Token(type, value, m_line, m_start + 1, m_current - m_start + 1));
}

bool Scanner::ident_match(std::string ident) {
	UNIMPLEMENTED;
}

bool Scanner::is(char c) {
	return peek() == c;
}

char Scanner::peek() {
	if (at_end()) return '\0';
	return m_content[m_current];
}

char Scanner::peek(int i) {
	if (at_end(m_current + i)) return '\0';
	return m_content[m_current + i];
}

bool Scanner::is_alpha() {
	char c  = peek();
	return  (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') || c == '$' || c == '_';
}

bool Scanner::is_digit() {
	char c  = peek();
	return c >= '0' && c <= '9';
}

bool Scanner::is_alphanum() {
	char c  = peek();
	return is_alpha(c) || is_digit(c);
}

bool Scanner::is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_';
}

bool Scanner::is_digit(char c) {
	return c >= '0' && c <= '9';
}

bool Scanner::is_alphanum(char c) {
	return is_alpha(c) || is_digit(c);
}

bool Scanner::match(char a) {
	if (at_end()) return false;
	if (m_content[m_current] != a) return false;

	m_current++;
	return true;
}

bool Scanner::at_end() {
	return m_current >= m_content.length();
}

bool Scanner::at_end(std::size_t i) {
	return i >= m_content.length();
}

char Scanner::advance() {
	return m_content[m_current++];
}

char Scanner::advance(std::size_t i) {
	m_current += i;
	return m_content[m_current - 1];
}

Scanner::Ident Scanner::get_next_ident() {
	std::size_t current = m_current;
	std::string buffer = "";
#define pk m_content[current]
	while ((!at_end(current)) && is_alphanum(pk)) {
		buffer.push_back(pk);
		current++;
	}
	return Scanner::Ident(this, buffer);
}

