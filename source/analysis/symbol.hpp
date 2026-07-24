/** Created in: 22/38/2026 14:06
  *
  */
#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "analysis/value.hpp"
#include "ast/ast.hpp"
#include "containers/dynamic_array.hpp"
#include "containers/hash_map.hpp"
#include "containers/string_view.hpp"
#include "utils/intern.hpp"

namespace haste {

struct Symbol {
	struct Entry {
		Value value {};
		const ast::Node *node = nullptr;
	};

	using Function = DynamicArray<Entry>;
	struct Variable {
		Value value = Value::none();
		bool is_declared : 1 = false;
		bool is_constant : 1; // TODO: maybe this should be part of the type system
		const ast::Node *node = nullptr;
	};

	StringView name {};
	std::variant<None, Function, Variable> val = None{};
};

struct Scope {
	HashMap<StringView, Symbol> symbols {};
};

struct SymbolTable {
	InternPool &pool;
	DynamicArray<Scope> scopes {};
	std::size_t errors = 0;

	SymbolTable(InternPool &pool);
};

void deinit(Scope &self, InternPool &pool);
void deinit(SymbolTable &self);

Scope *enter_scope(SymbolTable &self);
void exit_scope(SymbolTable &self);
Scope *global_scope(SymbolTable &self);
Scope *local_scope(SymbolTable &self);
Symbol *name_symbol(SymbolTable &self, StringView name);
void define_variable(SymbolTable &self, StringView name, ast::Node *node = nullptr);
void declare_variable(SymbolTable &self, StringView name, Value value, ast::Node *node = nullptr);

};

#endif /* !SYMBOL_H_ */
