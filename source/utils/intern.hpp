#ifndef INTERN_HPP_
#define INTERN_HPP_

#include "ast/ast.hpp"
#include "containers/allocator.hpp"
#include "containers/hash_set.hpp"
#include "containers/string.hpp"
#include "containers/string_view.hpp"

namespace haste {

struct Type;

struct InternPool {
	Allocator *allocator = nullptr;
	Allocator *arena     = nullptr;

	HashSet<StringView> strings;
	HashSet<Type*> types;
};

void init(InternPool &self, Allocator &allocator, Allocator &arena);
void deinit(InternPool &self);

StringView intern(InternPool &self, const StringView str);
StringView intern(InternPool &self, const std::string str);
StringView intern_no_alloc(InternPool &self, StringView str);

Type *intern(InternPool &self, const Type tp);

template <typename T>
ast::Node *intern(InternPool &self, const T &node)
{
	return ast::Node::make(*self.arena, node);
}

std::ostream &operator<<(std::ostream &os, const InternPool &self);

};

#endif // !INTERN_HPP_
