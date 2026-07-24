#include "symbol.hpp"
#include <variant>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace haste {

void deinit(Scope &self, InternPool &pool)
{
	for (auto entry : self.symbols) {
		auto symbol = entry.value;
		std::visit(overloaded {
			[&](auto) {},
			[&](Symbol::Function fn) {
				deinit(fn, *pool.allocator);
			}
		}, symbol.val);
	}
	deinit(self.symbols, *pool.allocator);
}

void deinit(SymbolTable &self)
{
	for (auto scope : self.scopes) {
		deinit(scope, self.pool);
	}
	deinit(self.scopes, *self.pool.allocator);
}

Scope *enter_scope(SymbolTable &self)
{
	Scope scope {};
	append(self.scopes, *self.pool.allocator, scope);
	return &self.scopes[self.scopes.len - 1];
}

void exit_scope(SymbolTable &self)
{
	pop(self.scopes);
}

Scope *global_scope(SymbolTable &self)
{
	return self.scopes.items;
}

Scope *local_scope(SymbolTable &self)
{
	return &self.scopes[self.scopes.len - 1];
}

Symbol *name_symbol(SymbolTable &self, StringView name)
{
	auto &local = *local_scope(self);
	if (contains(local.symbols, name)) {
		throw;
	}

	put(local.symbols, *self.pool.allocator, name, {});
	return get(local.symbols, name);
}

void define_variable(SymbolTable &self, StringView name, ast::Node *node)
{
	auto &local = *local_scope(self);
	auto *symbol = get(local.symbols, name);
	if (symbol == nullptr) {
		symbol = name_symbol(self, name);
	}

	if (auto _ = std::get_if<None>(&symbol->val)) {
		symbol->val = Symbol::Variable {
			.is_declared = false,
			.is_constant = false,
			.node = node,
		};
	} else {
		throw;
	}
}

void declare_variable(SymbolTable &self, StringView name, Value value, ast::Node *node)
{
	auto &local = *local_scope(self);
	auto *symbol = get(local.symbols, name);
	if (symbol == nullptr) {
		throw;
	}

	if (auto var = std::get_if<Symbol::Variable>(&symbol->val)) {
		var->is_declared = true;
		var->value = value;
		var->node = node;
	} else {
		throw;
	}
}

};
