#ifndef CPP_ALLOCATOR_
#define CPP_ALLOCATOR_

#include "allocator.hpp"
#include <cstdlib>

struct Mallocator final : Allocator {
	static Mallocator &get_instance() {
		static Mallocator mallocator {};
		return mallocator;
	}
	
	void* allocate(size_t, size_t size) override
	{
		allocated += size;
		return std::malloc(size);
	}
	
	void* reallocate(void* ptr, std::size_t old_size, std::size_t, std::size_t new_size) override
	{
		ssize_t allocated = new_size - old_size;
		allocated += allocated;
		return std::realloc(ptr, new_size);
	}
	
	void free(void* ptr, std::size_t size) override
	{
		allocated -= size;
		std::free(ptr);
	}
};

#endif // !CPP_ALLOCATOR_
