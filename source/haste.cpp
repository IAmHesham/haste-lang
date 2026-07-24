#include "haste.hpp"
#include "ast/parser.hpp"
#include "token/lexer.hpp"
#include "common.hpp"
#include "termcolor.hpp"
#include "report.hpp"

namespace haste {

Compiler::Compiler(Allocator &gpa_allocator)
	: arena(ArenaAllocator(gpa_allocator))
{
	init(pool, gpa_allocator, arena);
}

Allocator &Compiler::gpa_allocator() const
{
	return *pool.allocator;
}

Allocator &Compiler::arena_allocator() const
{
	return *pool.arena;
}

void deinit(Compiler &self)
{
	deinit(self.pool);
	deinit(self.source_manager, self.pool);
	deinit(self.arena);
}

void add_source_file(Compiler &self, StringView path)
{
	load_file(self.source_manager, self.pool, path);
}

void compile(Compiler &self)
{
	for (auto source : self.source_manager.sources) {
		if (source.type != SourceFileType::Haste) continue;

		Lexer lexer = Lexer::init(self.pool, source);

		DynamicArray<Token> tokens;
		defer { deinit(tokens, self.gpa_allocator()); };

		try {
			tokens = tokenize(lexer);
		} catch (LexingException) {
			std::cerr << "\nCouldn't compile " << ANSI_CODE_UNDERLINE ANSI_CODE_BOLD "'" << source.filepath << "'" ANSI_CODE_RESET
				" due to " ANSI_CODE_BOLD ANSI_CODE_YELLOW
				<< lexer.errors << ANSI_CODE_RESET
				<< (lexer.errors == 1 ? " previous error" : " previous errors") << "\n";
			throw CompilationException();
		}

		// for (auto token : tokens) {
		// 	std::cout << token << "\n";
		// }

		auto parser = Parser(self.pool, tokens);
		const ast::Node *program = nullptr;
		try {
			program = parse(parser);
		} catch (ParsingException) {
			std::cerr << "\nCouldn't compile " << ANSI_CODE_UNDERLINE ANSI_CODE_BOLD "'" << source.filepath << "'" ANSI_CODE_RESET
				" due to " ANSI_CODE_BOLD ANSI_CODE_YELLOW
				<< parser.errors << ANSI_CODE_RESET
				<< (parser.errors == 1 ? " previous error" : " previous errors") << "\n";
			throw CompilationException();
		}

		std::cout << *program << "\n";
		std::cout << self.pool << "\n";
	}

	// auto one  = intern(self.pool, ast::IntLit{{}, 1});
	// auto two  = intern(self.pool, ast::IntLit{{}, 2});
	// auto add  = intern(self.pool, ast::AdditionExpr{{}, {}, one, two});
	// auto name = intern(self.pool, StringView("x"));
	// auto decl = intern(self.pool, ast::VarDecl{{}, name, add, false});

	// ast::print(*decl, std::cout);
	// std::cout << std::endl;
}

};
