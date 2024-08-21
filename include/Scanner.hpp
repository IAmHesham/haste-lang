#pragma once

#include "tokens.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class Scanner {
	public:
		Scanner(std::string &content);
		TokenList scan();

	private:
		inline static std::unordered_map<std::string, TokenType> keywords = std::unordered_map<std::string, TokenType>();
		std::string &m_content;
		std::size_t m_current = 0;
		std::size_t m_start = 0;
		std::size_t m_line = 1;
		std::size_t m_column = 1;
		std::vector<Token> m_tokens;

		void scan_ident();
		void scan_numbers();
		void scan_string();

		void add_token(TokenType type);
		void add_token(TokenType type, std::string value);

		bool is_ident(std::string ident);
		bool is_alpha(char c);
		bool is_digit(char c);
		bool is_alphanum(char c);

		bool match(char a);
		bool at_end();
		char peek();
		char advance();
};
