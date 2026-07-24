#include "token/lexer.hpp"
#include "report.hpp"
#include "termcolor.hpp"
#include "common.hpp"

namespace haste {

// -- UTF-8 helpers -------------------------------------------------------

static std::size_t utf8_char_len(unsigned char c)
{
	if (c < 0x80) return 1;
	if (c < 0xC0) return 1; // continuation byte alone
	if (c < 0xE0) return 2;
	if (c < 0xF0) return 3;
	if (c < 0xF8) return 4;
	return 1;
}

static bool is_utf8_start(char)
{
	// return static_cast<unsigned char>(c) >= 0xC2;
	return false;
}

static bool is_utf8_cont(char c)
{
	return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

// -- Character predicates ------------------------------------------------

static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_utf8_start(c); }
static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alnum(char c) { return is_alpha(c) || is_digit(c) || is_utf8_cont(c); }

// -- Cursor helpers ------------------------------------------------------

static void advance(Lexer &self)
{
	auto c = static_cast<unsigned char>(self.source->content.data[self.cursor]);
	self.cursor += utf8_char_len(c);
	self.column++;
}

static Token make_token(Lexer &self, TokenKind kind, std::size_t start, std::uint32_t start_line, std::uint32_t start_col)
{
	return Token{kind, {self.source, start_line, start_col, static_cast<std::uint32_t>(self.cursor - start)}};
}

// -- Whitespace ----------------------------------------------------------

static void skip_whitespace(Lexer &self)
{
	StringView content = self.source->content;
	while (self.cursor < content.len) {
		char c = content.data[self.cursor];
		if (c == ' ' || c == '\t') {
			advance(self);
		} else {
			break;
		}
	}
}

// -- Newline -------------------------------------------------------------

static Token lex_newline(Lexer &self, char c, std::uint32_t start_line, std::uint32_t start_col)
{
	self.cursor++;
	std::uint32_t len = 1;
	if (c == '\r' && self.cursor < self.source->content.len && self.source->content.data[self.cursor] == '\n') {
		self.cursor++;
		len = 2;
	}
	self.line++;
	self.column = 1;
	return Token{TokenKind::Newline, {self.source, start_line, start_col, len}};
}

// -- Identifier or keyword ----------------------------------------------

static Token lex_identifier_or_keyword(Lexer &self, std::uint32_t start_line, std::uint32_t start_col)
{
	std::size_t start = self.cursor;
	advance(self);

	StringView content = self.source->content;
	while (self.cursor < content.len && (is_alnum(content.data[self.cursor]) || content.data[self.cursor] == '_')) {
		advance(self);
	}

	StringView word(content.data + start, self.cursor - start);

	TokenKind kind;
	if      (word == "const")  kind = TokenKind::KwConst;
	else if (word == "var")    kind = TokenKind::KwVar;
	else if (word == "auto")   kind = TokenKind::KwAuto;
	else if (word == "int")    kind = TokenKind::KwAuto;
	else if (word == "float")  kind = TokenKind::KwAuto;
	else                       kind = TokenKind::Identifier;

	intern_no_alloc(self.pool, word);

	return make_token(self, kind, start, start_line, start_col);
}

// -- Number literal ------------------------------------------------------

static Token lex_number(Lexer &self, std::uint32_t start_line, std::uint32_t start_col)
{
	std::size_t start = self.cursor;
	advance(self);

	StringView content = self.source->content;
	while (self.cursor < content.len && is_digit(content.data[self.cursor])) {
		advance(self);
	}

	TokenKind kind = TokenKind::IntLit;
	if (self.cursor < content.len && content.data[self.cursor] == '.') {
		advance(self);
		if (self.cursor < content.len && is_digit(content.data[self.cursor])) {
			kind = TokenKind::FloatLit;
			while (self.cursor < content.len && is_digit(content.data[self.cursor])) {
				advance(self);
			}
		} else {
			throw;
		}
	}

	Token result = make_token(self, kind, start, start_line, start_col);
	intern_no_alloc(self.pool, as_string(result));

	return result;
}

// -- Punctuator ----------------------------------------------------------
static Token lex_punctuator(Lexer &self, char c, std::uint32_t start_line, std::uint32_t start_col)
{
	const auto location = Location{
		.src = self.source,
		.line = start_line,
		.column = start_col,
		.len = 1,
	};
	advance(self);

	switch (c) {
	case '(': return Token{TokenKind::OpenParen,  location};
	case ')': return Token{TokenKind::CloseParen, location};
	case '{': return Token{TokenKind::OpenBrace,  location};
	case '}': return Token{TokenKind::CloseBrace, location};
	case '+': return Token{TokenKind::Plus,       location};
	case '-': return Token{TokenKind::Minus,      location};
	case '*': return Token{TokenKind::Star,       location};
	case '/': return Token{TokenKind::FSlash,     location};
	case '=': return Token{TokenKind::Assign,     location};
	case ';': return Token{TokenKind::Semicolon,  location};
	case ':': return Token{TokenKind::Colon,      location};
	default:
		begin_report(location)
			<< ReportKind(Error)
			<< ReportBrief() << "Unexpected character '" << c << "'"
			<< ReportDescription() << "This is not a valid token."
			>> std::cerr;
		self.errors += 1;
		throw LexingException();
	}
}

// -- Public API ----------------------------------------------------------

Lexer Lexer::init(InternPool &pool, SourceFile &source)
{
	return {
		.pool = pool,
		.source = &source,
	};
}

Token next(Lexer &self)
{
	skip_whitespace(self);

	StringView content = self.source->content;
	if (self.cursor >= content.len) {
		return Token{TokenKind::Eof, {self.source, self.line, self.column, 0}};
	}

	char c = content.data[self.cursor];
	std::uint32_t start_line = self.line;
	std::uint32_t start_col  = self.column;

	if (c == '\n' || c == '\r')    return lex_newline(self, c, start_line, start_col);
	if (is_alpha(c) || c == '_')   return lex_identifier_or_keyword(self, start_line, start_col);
	if (is_digit(c))               return lex_number(self, start_line, start_col);
	return lex_punctuator(self, c, start_line, start_col);
}

DynamicArray<Token> tokenize(InternPool &pool, SourceFile &source)
{
	Lexer lexer = Lexer::init(pool, source);
	return tokenize(lexer);
}

DynamicArray<Token> tokenize(Lexer &lexer)
{
	DynamicArray<Token> tokens {};
	errdefer { deinit(tokens, *lexer.pool.allocator); };

	while (true) {
		Token token {};
		try {
			token = next(lexer);
		} catch (LexingException e) {
			continue;
		}
		if (token.kind == TokenKind::Eof) {
			break;
		}
		// this condition prevents duplication of the 'new line' and 'semicolon' tokens
		// TODO: make it so that when the current token is newline or a semicolon. and the previous
		//       token is a newline or a semicolon we will prevent duplication.
		//       e.g. if the source is ";\n" the resulted token is: "semicolon" and not "semicolon newline"
		//       also if the source is "\n;" the resulted token is: "newline" and not "newline semicolon"
		if (tokens.len != 0 and (token.kind == TokenKind::Newline or token.kind == TokenKind::Semicolon)
			and token.kind == tokens[tokens.len - 1].kind) {
			continue;
		}
		append(tokens, *lexer.pool.allocator, token);
	}

	if (lexer.errors > 0) {
		throw LexingException();
	}

	return tokens;
}

}; // namespace haste
