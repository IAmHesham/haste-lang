#include "string_view.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

StringView::StringView() 
{ }

StringView::StringView(String str)
: data(str.data), len(str.len)
{ }

StringView::StringView(std::span<char> span)
: data(span.data()), len(span.size())
{}

StringView::StringView(std::string_view sv)
: data(sv.data()), len(sv.size())
{}

StringView::StringView(const char* cstr)
: data(cstr), len(std::strlen(cstr))
{}

StringView::StringView(const char* data, std::size_t len)
: data(data), len(len) 
{ }

const char& StringView::operator[](std::size_t i) const 
{
	assert(i < len && "StringView index out of bounds");
	return data[i];
}

// --- Core API Functions ---
StringView slice(StringView self, std::size_t start, std::size_t end) 
{
	assert(start <= end && "Slice start cannot exceed end");
	assert(end <= self.len && "Slice end out of bounds");
	return StringView{ self.data + start, end - start };
}

StringView trim(StringView self) 
{
	std::size_t start = 0;
	while (start < self.len && (self.data[start] == ' '  || 
	self.data[start] == '\t' || 
	self.data[start] == '\n' || 
	self.data[start] == '\r')) {
		start++;
	}
	
	std::size_t end = self.len;
	while (end > start && (self.data[end - 1] == ' '  || 
	self.data[end - 1] == '\t' || 
	self.data[end - 1] == '\n' || 
	self.data[end - 1] == '\r')) {
		end--;
	}
	
	return StringView{ self.data + start, end - start };
}

std::size_t find(StringView self, char c) 
{
	for (std::size_t i = 0; i < self.len; ++i) {
		if (self.data[i] == c) return i;
	}
	return self.len; 
}

std::string_view as_std_string_view(StringView self)
{
	return std::string_view(self.data, self.len);
}

std::string as_std_string(StringView self)
{
	std::string str;
	str.append(self.data, self.len);
	return str;
}

// --- Overloaded Operators ---
bool operator==(const StringView &lhs, const StringView &rhs) 
{
	if (lhs.len != rhs.len) return false;
	if (lhs.data == rhs.data) return true;
	for (std::size_t i = 0; i < lhs.len; ++i) {
		if (lhs.data[i] != rhs.data[i]) return false;
	}

	return true;
}

std::ostream &operator<<(std::ostream &os, const StringView &self)
{
	for (std::size_t i = 0; i < self.len; ++i) {
		os << self.data[i];
	}
	return os;
}

// --- Iterator Support ---
const char* begin(const StringView &self) 
{
	return self.data;
}

const char* end(const StringView &self) 
{
	return self.data + self.len;
}

// --- Hash Specialization ---
std::size_t std::hash<StringView>::operator()(const StringView &str) const 
{
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFFULL
	std::size_t hash = 14695981039346656037ULL;
	std::size_t prime = 1099511628211ULL;
#else
	std::size_t hash = 2166136261U;
	std::size_t prime = 16777619U;
#endif

	for (std::size_t i = 0; i < str.len; ++i) {
		hash ^= static_cast<std::size_t>(str.data[i]);
		hash *= prime;
	}

	return hash;
}
