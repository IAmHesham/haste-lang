#include "intern.hpp"
#include "analysis/type.hpp"
#include "utils/helper.hpp"

#define INTERN_VERBOSE
#ifdef INTERN_VERBOSE
#  include <iostream>
#endif // INTERN_VERBOSE

namespace haste {

void init(InternPool &self, Allocator &allocator, Allocator &arena)
{
	self.allocator = &allocator;
	self.arena     = &arena;
	init(self.strings);
}

void deinit(InternPool &self)
{
	deinit(self.strings, *self.allocator);
}

StringView intern(InternPool &self, StringView str)
{
	StringView *result = get(self.strings, str);
	if (result != nullptr) {
		return *result;
	}

#ifdef INTERN_VERBOSE
	std::cout << "Interning: " << quoted(shorten(str)) << "\n";
#endif // INTERN_VERBOSE

	StringView sv = clone(*self.arena, str);
	put(self.strings, *self.allocator, sv);
	return sv;
}

StringView intern(InternPool &self, std::string str)
{
	const StringView s = StringView(str.data(), str.size());
	StringView *result = get(self.strings, s);
	if (result) {
		return *result;
	}

#ifdef INTERN_VERBOSE
	std::cout << "Interning: " << quoted(shorten(str)) << "\n";
#endif // INTERN_VERBOSE

	StringView sv = clone(*self.arena, s);
	put(self.strings, *self.allocator, sv);
	return sv;
}

StringView intern_no_alloc(InternPool &self, StringView str)
{
	StringView *result = get(self.strings, str);
	if (result != nullptr) {
		return *result;
	}

#ifdef INTERN_VERBOSE
	std::cout << "Interning: " << quoted(shorten(str)) << "\n";
#endif // INTERN_VERBOSE

	put(self.strings, *self.allocator, str);
	return str;
}

Type *intern(InternPool &self, Type tp)
{
	Type **result = get(self.types, &tp);
	if (result != nullptr) {
		return *result;
	}

#ifdef INTERN_VERBOSE
	std::cout << "InterningType: " << quoted(tp.full_name) << "\n";
#endif // INTERN_VERBOSE

	Type *type = create(self.arena, tp);
	put(self.types, *self.allocator, type);
	return type;
}

std::ostream &operator<<(std::ostream &os, const InternPool &self)
{
    os << "InternPool {\n";
    os << "\tstrings (" << self.strings.size << "):\n";
    bool first = true;
    for (auto it = begin(self.strings); it != end(self.strings); ++it) {
		os << "\t\t" << (void*)it->key.data << " -> " << (void*)(it->key.data + it->key.len) << " ";
		os << quoted(shorten(it->key)) << "\n";
    }
    os << "\ttypes (" << self.types.size << "):\n";
    first = true;
    for (auto it = begin(self.types); it != end(self.types); ++it) {
        if (!first) os << ", ";
        os << it->key->short_name;
        first = false;

		os << "\t\t" << (void*)it->key << " ";
		os << quoted(it->key->full_name) << "\n";
    }
    os << "}";
    return os;
}

};
