#include "parser.hpp"
#include "report.hpp"
#include "utils/intern.hpp"
#include "token/token.hpp"
#include "common.hpp"

namespace haste {

using Self = Parser &;
using Result = ast::Node *;

struct InternalParsingException : std::exception {};

// -- Token helpers ---------------------------------------------------------

static bool ended(Self self)
{
	return self.current >= self.tokens.len;
}

static Token peek(Self self, ssize_t ahead = 0)
{
	if (ended(self)) {
		throw;
	}
	return self.tokens[self.current + ahead];
}

static Token previous(Self self)
{
	if (self.current == 0) return peek(self);
	return self.tokens[self.current - 1];
}

static Token advance(Self self)
{
	if (ended(self)) return peek(self);
	Token tok = peek(self);
	self.current += 1;
	return tok;
}

template <typename... Kinds>
static bool check(Self self, Kinds... kinds)
{
	if (ended(self)) return false;
	return ((peek(self).kind == kinds) || ...);
}

static bool is_terminator(Self self)
{
	return check(self, TokenKind::Newline, TokenKind::Semicolon);
}

template <typename... Kinds>
static bool match(Self self, Kinds... kinds)
{
	if (check(self, kinds...)) {
		advance(self);
		return true;
	}
	return false;
}

static Token consume(Self self, TokenKind kind, Reporter &reporter)
{
	if (match(self, kind)) {
		return previous(self);
	}
	reporter >> std::cerr;
	throw InternalParsingException{};
}

// -- Pratt parser ----------------------------------------------------------

enum : std::uint8_t {
	PREC_NONE,
	PREC_TERM,      // + -
	PREC_FACTOR,    // * /
	PREC_UNARY,     // prefix -
	PREC_PRIMARY,   // literals, grouping
};

constexpr auto PREC_LOWEST = PREC_TERM;

using PrefixFn  = Result(*)(Parser &);
using PostfixFn = Result(*)(Parser &, Result);
using InfixFn   = Result(*)(Parser &, Result);

struct Rule {
	PrefixFn  prefix;
	InfixFn   infix;
	PostfixFn postfix;
	std::uint8_t precedence;
	bool right_assoc;

	bool is_undefined() const
	{
		return not prefix
			and not infix
			and not postfix;
	}
};

// -- Forward declarations --------------------------------------------------

static Result newline(Self self);
static Result auto_kw(Parser &self);
static Result int_kw(Parser &self);
static Result float_kw(Parser &self);
static Result int_lit(Parser &);
static Result float_lit(Parser &);
static Result ident(Parser &);
static Result grouping(Parser &);
static Result unary(Parser &);

static Result binary(Parser &, Result);

static Rule get_rule(TokenKind);
static Result expr(Parser &self);
static Result parse_precedence(Parser &, std::uint8_t rbp);

// -- Rule table ------------------------------------------------------------

static Rule get_rule(TokenKind kind)
{
	switch (kind) {
	case TokenKind::Newline:     return { newline,   nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::KwAuto:      return { auto_kw,   nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::KwInt:       return { int_kw,    nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::KwFloat:     return { float_kw,  nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::IntLit:      return { int_lit,   nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::FloatLit:    return { float_lit, nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::Identifier:  return { ident,     nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::OpenParen:   return { grouping,  nullptr, nullptr, PREC_PRIMARY, false };
	case TokenKind::Plus:        return { unary,     binary,  nullptr, PREC_TERM,    false };
	case TokenKind::Minus:       return { unary,     binary,  nullptr, PREC_TERM,    false };
	case TokenKind::Star:        return { nullptr,   binary,  nullptr, PREC_FACTOR,  false };
	case TokenKind::FSlash:      return { nullptr,   binary,  nullptr, PREC_FACTOR,  false };
	default:                     return { nullptr,   nullptr, nullptr, PREC_NONE,    false };
	}
}

// -- Prefix parsers --------------------------------------------------------

static Result newline(Self self)
{
	Token op_tok = previous(self);
	Rule rule = get_rule(op_tok.kind);
	if (rule.is_undefined()) {
		return expr(self);
	}
	auto prec = rule.right_assoc ? rule.precedence : rule.precedence + 1;
	Result right = parse_precedence(self, prec);
	return right;
}

static Result auto_kw(Parser &self)
{
	Location location = peek(self).location;
	return intern(
		self.pool,
		ast::Auto {location});
}

static Result int_kw(Parser &self)
{
	Location location = peek(self).location;
	return intern(
		self.pool,
		ast::Int {location});
}

static Result float_kw(Parser &self)
{
	Location location = peek(self).location;
	return intern(
		self.pool,
		ast::Float {location});
}

static Result int_lit(Parser &self)
{
	Token tok = previous(self);
	return intern(self.pool, ast::IntLit{
		.location = tok.location,
		.value = static_cast<std::uint64_t>(std::stoul(as_std_string(as_string(tok))))
	});
}

static Result float_lit(Parser &self)
{
	Token tok = previous(self);
	return intern(self.pool, ast::FloatLit{
		.location = tok.location,
		.value = std::stod(as_std_string(as_string(tok)))
	});
}

static Result ident(Parser &self)
{
	Token tok = previous(self);
	return intern(self.pool, ast::IdentExpr{
		.location = tok.location,
		.name = as_string(tok),
	});
}

static Result grouping(Parser &self)
{
	const auto start = previous(self);
	Result inner = parse_precedence(self, PREC_LOWEST);
	consume(self, TokenKind::CloseParen, begin_report(start)
		<< ReportKind(Error)
		<< ReportBrief() << "Expected ')' as closing for this.");
	return inner;
}

static Result unary(Parser &self)
{
	Token op_tok = previous(self);
	Result operand = parse_precedence(self, PREC_UNARY);

	switch (op_tok.kind) {
	case TokenKind::Minus:
		return intern(self.pool, ast::NegateExpr{
			.location = conjoin(op_tok.location, as_location(operand)),
			.op_location = op_tok.location,
			.operand = operand,
		});
	case TokenKind::Plus:
		return operand;
	default:
		throw std::string("Reached unreachable code: Unexpected token ") + as_std_string(as_string(op_tok.location));
	}
}

// -- Infix parsers ---------------------------------------------------------

static Result binary(Parser &self, Result left)
{
	Token op_tok = previous(self);
	Rule rule = get_rule(op_tok.kind);
	auto prec = rule.right_assoc ? rule.precedence : rule.precedence + 1;
	Result right = parse_precedence(self, prec);
	auto loc = conjoin(as_location(left), as_location(right));

	switch (op_tok.kind) {
	case TokenKind::Plus:
		return intern(self.pool, ast::AdditionExpr{
			.location = loc,
			.op_location = op_tok.location,
			.left = left,
			.right = right,
		});
	case TokenKind::Minus:
		return intern(self.pool, ast::SubtractionExpr{
			.location = loc,
			.op_location = op_tok.location,
			.left = left,
			.right = right,
		});
	case TokenKind::Star:
		return intern(self.pool, ast::MultiplicationExpr{
			.location = loc,
			.op_location = op_tok.location,
			.left = left,
			.right = right,
		});
	case TokenKind::FSlash:
		return intern(self.pool, ast::DivisionExpr{
			.location = loc,
			.op_location = op_tok.location,
			.left = left,
			.right = right,
		});
	default:
		throw std::string("Reached unreachable code: Unexpected token ") + as_std_string(as_string(op_tok.location));
	}
}

// -- Core Pratt loop -------------------------------------------------------

static Result parse_precedence(Parser &self, std::uint8_t rbp)
{
	if (ended(self)) {
		begin_report(previous(self))
			<< ReportKind(Error)
			<< ReportBrief() << "Incomplete expression"
			<< ReportDescription() << "Maybe you needed a valid expression here."
			>> std::cerr;
		throw InternalParsingException{};
	}

	Token token = advance(self);
	Rule rule = get_rule(token.kind);
	if (rule.prefix == nullptr) {
		begin_report(token)
			<< ReportKind(Error)
			<< ReportBrief() << "Expected an expression"
			<< ReportDescription() << "Maybe you needed a valid expression here."
			>> std::cerr;
		throw InternalParsingException{};
	}

	Result left = rule.prefix(self);

	for (rule = get_rule(peek(self).kind);
		 not ended(self) and rbp <= rule.precedence;
		 rule = get_rule(peek(self).kind))
	{
		if (peek(self).kind == TokenKind::Newline) {
			break;
		}
		const Token tok = advance(self);
		const Rule tok_rule = get_rule(tok.kind);

		if (tok_rule.postfix) {
			left = tok_rule.postfix(self, left);
			continue;
		}

		if (tok_rule.infix == nullptr) {
			begin_report(tok)
				<< ReportKind(Error)
				<< ReportBrief() << "This is not a valid operator"
				<< ReportDescription() << "Expected + * / - here. got '" << as_string(tok) << "' instead."
				>> std::cerr;
			throw InternalParsingException{};
		}
		left = tok_rule.infix(self, left);
	}

	return left;
}

static Result expr(Parser &self)
{
	return parse_precedence(self, PREC_TERM);
}

// -- Variable declarations -------------------------------------------------

static Result var_decl(Parser &self)
{
	const auto start = previous(self).location;
	const auto is_constant = previous(self).kind == TokenKind::KwConst;

	const auto name = consume(self, TokenKind::Identifier,
	begin_report(peek(self))
		<< ReportKind(Error)
		<< ReportBrief() << "Ok bruv this is not an identifier 🥀"
		<< ReportDescription() << "a name is needed after `const` and `var`");

	ast::Node *type = nullptr;
	ast::Node *init = nullptr;
	if (match(self, TokenKind::Colon)) {
		if (match(self, TokenKind::Assign)) {
			init = expr(self);
		} else {
			type = expr(self);
		}
	}

	if (not init and match(self, TokenKind::Assign)) {
		init = expr(self);
	}

	const auto end = previous(self).location;
	return intern(
		self.pool,
		ast::VarDecl {
			.location = conjoin(start, end),
			.type = nullptr,
			.name = as_string(name),
			.type_node = type,
			.init = init,
			.is_cons = is_constant,
		});
}

static Result decl(Parser &self)
{
	if (match(self, TokenKind::KwConst, TokenKind::KwVar)) {
		return var_decl(self);
	}

	begin_report(peek(self))
		<< ReportKind(Error)
		<< ReportBrief() << "Expected a declaration here"
		<< ReportDescription() << "Expected `const` or `var` or `func`"
		>> std::cerr;
	throw InternalParsingException{};
}

// -- Error synchronizing ------------------------------------------------------------

static void synchronize(Self self, InternalParsingException)
{
	DynamicArray<Token> pairs {};
	defer { deinit(pairs, *self.pool.allocator); };

	while (not ended(self)) {
		if (is_terminator(self) and pairs.len == 0) {
			advance(self);
			break;
		}
		if (match(self, TokenKind::OpenParen, TokenKind::OpenBrace)) {
			append(pairs, *self.pool.allocator, previous(self));
			continue;
		}

		if (match(self, TokenKind::CloseParen, TokenKind::CloseBrace)) {
			auto tok = previous(self);
			auto token = pairs[pairs.len - 1];
			pop(pairs);

			if (tok.kind != token.kind) {
				begin_report(tok)
					<< ReportKind(Error)
					<< ReportBrief() << "Unmatched closing here"
					<< ReportDescription() << "We got '" << as_string(tok) << "'"
					>> std::cerr;
				begin_report(token)
					<< ReportKind(Info)
					<< ReportBrief() << "It opens here"
					<< ReportDescription() << "Expected a closing for this instead."
					>> std::cerr;
				self.errors += 1;
			}
			continue;
		}
		advance(self);
	}

	if (pairs.len > 0) {
		for (auto token : pairs) {
			begin_report(token)
				<< ReportKind(Error)
				<< ReportBrief() << "Expected a closing for this"
				>> std::cerr;
			self.errors += 1;
		}
		throw ParsingException{};
	}
}

// -- Public API ------------------------------------------------------------

Parser::Parser(InternPool &pool, DynamicArray<Token> tokens)
: pool(pool),
  tokens(tokens)
{  }

const ast::Node *parse(InternPool &pool, DynamicArray<Token> tokens)
{
	Parser parser = Parser(pool, tokens);
	return parse(parser);
}

const ast::Node *parse(Parser &self)
{
	Location start = peek(self).location;

	LinkedList<ast::Node *> nodes;
	init(nodes);

	while (not ended(self)) {
		try {
			auto node = decl(self);
			append(nodes, *self.pool.arena, node);
			match(self, TokenKind::Newline);
			match(self, TokenKind::Semicolon);
		} catch (InternalParsingException e) {
			self.errors += 1;
			synchronize(self, e);
		}
	}

	if (self.errors > 0) {
		throw ParsingException{};
	}

	Location end = previous(self).location;
	Location location = conjoin(start, end);
	return intern(
		self.pool,
		ast::Program {
			.location = location,
			.declarations = nodes,
		});
}

};
