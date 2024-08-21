#include <cstddef>
#include <macros.hpp>
#include "tokens.hpp"
#include <Scanner.hpp>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

Scanner::Scanner(std::string& content): m_content(content) {}
void Scanner::setup_keywords() {
	keywords["var"] = TokenType::Var;
	keywords["func"] = TokenType::Func;
	keywords["if"] = TokenType::If;
	keywords["else"] = TokenType::Else;
	keywords["is"] = TokenType::Is;
}

TokenList Scanner::scan() {
	while (!at_end()) {
		m_start = m_current;
		char c = advance();
		switch (c) {
			case '(': add_token(TokenType::OpenParen); break;
			case ')': add_token(TokenType::CloseParen); break;
			case '+': add_token(TokenType::Plus); break;
			case '-': add_token(TokenType::Minus); break;
			case '*': add_token(TokenType::Star); break;
			case '/': add_token(TokenType::FSlash); break;
			case '\\': add_token(TokenType::BSlash); break;
			case '!':
				if (match('=')) {
					add_token(TokenType::BangEq);
				} else {
					Scanner::Ident ident = get_next_ident();
					if (ident.value == "var") {
						ident.skip();
						add_token(TokenType::NotVar);
					} else {
						add_token(TokenType::Not);
					}
				}
				break;
			case '=': add_token(match('=') ? TokenType::EqEq : TokenType::Eq); break;

			case '\n':
				m_line++;
				break;

			case ' ':
			case '\t':
			case '\r':
				break;

			default:
				if (isalpha(peek())) {
					scan_ident();
				} else if (isdigit(peek())) {
					scan_numbers();
				} else if (peek() == '"') {
					scan_string();
				}
				break;
		}
	}
	return m_tokens;
}

void Scanner::scan_ident() {
	while (isalnum(peek())) advance();

	std::string buffer = m_content.substr(m_start, m_current - m_start);
	if (keywords.contains(buffer)) {
		add_token(keywords[buffer]);
		return;
	}
	add_token(TokenType::Identifier, buffer);
}

void Scanner::scan_numbers() {
	TokenType type = TokenType::IntLit;
	while (isdigit(peek())) advance();

	if (peek() == '.') {
		type = TokenType::FloatLit;
		advance();
		while (isdigit(peek()) || peek() == 'f') advance();
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

char Scanner::peek() {
	if (at_end()) return '\0';
	return m_content[m_current];
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
	return is_alpha(c) && is_digit(c);
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

