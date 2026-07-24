#ifndef STRING_HPP_
#define STRING_HPP_

#include <cstddef>
#include <ostream>
#include <cassert>

#include <cstddef>
#include <ostream>
#include <span>

struct String {
	char* data = nullptr;
	std::size_t len = 0;
	
	String();
	String(std::span<char> span);
	String(char* data, std::size_t len);
	
	template <std::size_t N>
	String(const char (&literal)[N]) : data(const_cast<char*>(literal)), len(N > 0 && literal[N - 1] == '\0' ? N - 1 : N)
	{ }
	
	char& operator[](std::size_t i);
	const char& operator[](std::size_t i) const;
};

// --- Core API Functions ---
String slice(String self, std::size_t start, std::size_t end);
std::size_t find(String self, char c);
String trim(String self);
void fill(String self, char c);

// --- Overloaded Operators ---
bool operator==(const String &lhs, const String &rhs);
std::ostream &operator<<(std::ostream &os, const String &self);

// --- Hash Specialization ---
template <>
struct std::hash<String> {
	std::size_t operator()(const String &str) const;
};

// --- Iterator Support ---
char* begin(String &self);
char* end(String &self);
const char* begin(const String &self);
const char* end(const String &self);

#endif // !STRING_HPP_
