#include "allocator.hpp"
#include <cstddef>

std::size_t allocated = 0;
Allocator *default_allocator_ {};

void set_default_allocator(Allocator *allocator)
{
	default_allocator_ = allocator;
}

Allocator *get_default_allocator(void)
{
	return default_allocator_;
}

String clone(Allocator &allocator, String data)
{
	std::span<char> result = alloc<char>(allocator, data.len);
	for (std::size_t i = 0; i < data.len; ++i) {
		result[i] = data[i];
	}
	return result;
}

StringView clone(Allocator &allocator, StringView data)
{
	std::span<char> result = alloc<char>(allocator, data.len);
	for (std::size_t i = 0; i < data.len; ++i) {
		result[i] = data[i];
	}
	return result;
}
