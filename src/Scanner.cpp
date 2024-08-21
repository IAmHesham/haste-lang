#include "tokens.hpp"
#include <Scanner.hpp>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

Scanner::Scanner(std::string& content): m_content(content) {}

TokenList Scanner::scan() {
	while (!at_end()) {
		m_start = m_current;
		switch (peek()) {
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
					break;
				} else if () {}
			case '=':
				if (match('=')) {
					add_token(TokenType::EqEq);
					break;
				}
				add_token(TokenType::Eq);
				break;

			case '\n':
				m_line++;
				m_column = 1;
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
		advance();
	}
	return m_tokens;
}

void Scanner::scan_ident() {
	while (isalnum(peek())) advance();

	std::string buffer = m_content.substr(m_start, m_current - m_start);
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
	add_token(type, m_content.substr(m_start, m_current - m_start + 1));
}
void Scanner::add_token(TokenType type, std::string value) {
	m_tokens.push_back(Token(type, value, m_line, m_start + 1, m_current - m_start));
}
bool Scanner::is_ident(std::string ident) {
	std::size_t current = current;
	std::string buffer = "";
	while (!at_end() && buffer.length() <= ident.length() && isalnum()) {
	}
}

char Scanner::peek() {
	if (at_end()) return '\0';
	return m_content[m_current];
}

bool Scanner::is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_';
}

bool Scanner::is_digit(char c) {}

bool Scanner::is_alphanum(char c) {}

bool Scanner::match(char a) {
	if (peek() == a) {
		advance();
		return true;
	}
	return false;
}

bool Scanner::at_end() {
	return m_current >= m_content.length();
}

char Scanner::advance() {
	m_column++;
	return m_content[m_current++];
}
