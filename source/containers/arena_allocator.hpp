#ifndef ARENA_ALLOCATOR_HPP_
#define ARENA_ALLOCATOR_HPP_

#include "containers/allocator.hpp"
#include <ostream>

struct ArenaAllocator final : public Allocator {
	struct ChunkHeader {
		std::size_t capacity;
		std::size_t offset;
		ChunkHeader* next;
	};
	
	Allocator &child_allocator = *default_allocator;
	std::size_t default_chunk_size = 4096;
	ChunkHeader* head = nullptr;
	explicit ArenaAllocator(Allocator &upstream_allocator, std::size_t default_chunk_size = 4096);
	
	// Prevent copying
	ArenaAllocator(const ArenaAllocator&) = delete;
	ArenaAllocator& operator=(const ArenaAllocator&) = delete;
	
	ArenaAllocator(ArenaAllocator&& other) noexcept;
	ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;
	
	void *allocate(const std::size_t alignment, const std::size_t size) override;
	void *reallocate(void *ptr, const std::size_t old_size, const std::size_t alignment, const std::size_t new_size) override;
	void free(void *ptr, const std::size_t size) override;
	
	void release_all();
	
private:
	void* allocate_from_chunk(ChunkHeader* chunk, std::size_t alignment, std::size_t size);
};

void deinit(ArenaAllocator &self);

std::ostream& operator<<(std::ostream& os, const ArenaAllocator& arena);

#endif // !ARENA_ALLOCATOR_HPP_
