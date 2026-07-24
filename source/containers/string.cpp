#include "string.hpp"
#include <cassert>
#include <iostream>

String::String() 
{ }

String::String(std::span<char> span)
: data(span.data()), len(span.size())
{ }

String::String(char* data, std::size_t len) : data(data), len(len) 
{ }

char& String::operator[](std::size_t i) 
{
	assert(i < len && "String index out of bounds");
	return data[i];
}

const char& String::operator[](std::size_t i) const 
{
	assert(i < len && "String index out of bounds");
	return data[i];
}

// --- Core API Functions ---
String slice(String self, std::size_t start, std::size_t end) 
{
	assert(start <= end && "Slice start cannot exceed end");
	assert(end <= self.len && "Slice end out of bounds");
	return String{ self.data + start, end - start };
}

std::size_t find(String self, char c) 
{
	for (std::size_t i = 0; i < self.len; ++i) {
		if (self.data[i] == c) return i;
	}
	return self.len; 
}

String trim(String self) 
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
	
	return String{ self.data + start, end - start };
}

void fill(String self, char c) 
{
	for (std::size_t i = 0; i < self.len; ++i) {
		self.data[i] = c;
	}
}

// --- Overloaded Operators ---
bool operator==(const String &lhs, const String &rhs) 
{
	if (lhs.len != rhs.len) return false;
	if (lhs.data == rhs.data) return true;
	for (std::size_t i = 0; i < lhs.len; ++i) {
		if (lhs.data[i] != rhs.data[i]) return false;
	}
	return true;
}

std::ostream &operator<<(std::ostream &os, const String &self)
{
	for (std::size_t i = 0; i < self.len; ++i) {
		os << self.data[i];
	}
	return os;
}

// --- Hash Specialization ---
std::size_t std::hash<String>::operator()(const String &str) const 
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

// --- Iterator Support ---
char* begin(String &self) 
{
	return self.data;
}

char* end(String &self) 
{
	return self.data + self.len;
}

const char* begin(const String &self) 
{
	return self.data;
}

const char* end(const String &self) 
{
	return self.data + self.len;
}
