#ifndef STRING_VIEW_HPP_
#define STRING_VIEW_HPP_

#include "containers/string.hpp"
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <functional>

struct StringView {
	const char* data = nullptr;
	std::size_t len = 0;
	
	StringView();
	StringView(String str);
	StringView(std::span<char> span);
	StringView(std::string_view sv);
	StringView(const char* cstr);
	StringView(const char* data, std::size_t len);
	
	template <std::size_t N>
	StringView(const char (&literal)[N]) : data(literal), len(N > 0 && literal[N - 1] == '\0' ? N - 1 : N) 
	{ }
	
	const char& operator[](std::size_t i) const;
};

// --- Core API Functions ---
StringView slice(StringView self, std::size_t start, std::size_t end = SIZE_MAX);
StringView trim(StringView self);
std::size_t find(StringView self, char c);
std::string_view as_std_string_view(StringView self);
std::string as_std_string(StringView self);

// --- Overloaded Operators ---
bool operator==(const StringView &lhs, const StringView &rhs);
std::ostream &operator<<(std::ostream &os, const StringView &self);

// --- Iterator Support ---
const char* begin(const StringView &self);
const char* end(const StringView &self);

// --- Hash Specialization ---
template <>
struct std::hash<StringView> {
	std::size_t operator()(const StringView &str) const;
};

#endif // !STRING_VIEW_HPP_
