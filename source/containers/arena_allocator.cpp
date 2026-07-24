#include "arena_allocator.hpp"
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <iostream>

ArenaAllocator::ArenaAllocator(Allocator &child_allocator, std::size_t default_chunk_size)
: child_allocator(child_allocator),
default_chunk_size(default_chunk_size),
head(nullptr)
{}

ArenaAllocator::ArenaAllocator(ArenaAllocator&& other) noexcept 
: child_allocator(other.child_allocator),
default_chunk_size(other.default_chunk_size),
head(other.head) 
{
	other.head = nullptr;
	other.default_chunk_size = 0; 
}

// 2. Move Assignment Operator
ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
	if (this != &other) {
		assert(&child_allocator == &other.child_allocator && 
		"Cannot move-assign ArenaAllocators with different child allocators!");
		
		release_all();
		
		default_chunk_size = other.default_chunk_size;
		head = other.head;
		
		other.head = nullptr;
		other.default_chunk_size = 0;
	}
	return *this;
}

void* ArenaAllocator::allocate(const std::size_t alignment, const std::size_t size)
{
	// 1. Try to allocate out of the current head chunk
	if (head) {
		void* ptr = allocate_from_chunk(head, alignment, size);
		if (ptr) return ptr;
	}
	
	// 2. Current chunk is full or missing. Allocate a new chunk from upstream.
	std::size_t allocation_overhead = alignment + sizeof(ChunkHeader);
	std::size_t chunk_size = std::max(default_chunk_size, size + allocation_overhead);
	
	void* chunk_mem = child_allocator.allocate(alignof(std::max_align_t), chunk_size);
	if (!chunk_mem) return nullptr;
	
	// Initialize chunk header at the start of raw memory
	ChunkHeader* new_chunk = static_cast<ChunkHeader*>(chunk_mem);
	new_chunk->capacity = chunk_size;
	new_chunk->offset = sizeof(ChunkHeader);
	new_chunk->next = head;
	head = new_chunk;
	
	// 3. Allocate out of the fresh chunk
	return allocate_from_chunk(head, alignment, size);
}

void* ArenaAllocator::reallocate(void* ptr, const std::size_t old_size, const std::size_t alignment, const std::size_t new_size)
{
	if (!ptr) {
		return allocate(alignment, new_size);
	}
	
	if (new_size <= old_size) {
		return ptr; // Shrinking optimization
	}
	
	// Optimization: If this was the last allocation in the active chunk, grow in-place
	if (head) {
		uint8_t* byte_ptr = static_cast<uint8_t*>(ptr);
		uint8_t* chunk_start = reinterpret_cast<uint8_t*>(head);
		
		std::size_t ptr_relative_end = byte_ptr - chunk_start + old_size;
		
		if (ptr_relative_end == head->offset) {
			std::size_t extra_needed = new_size - old_size;
			if (head->offset + extra_needed <= head->capacity) {
				head->offset += extra_needed;
				return ptr;
			}
		}
	}
	
	void* new_ptr = allocate(alignment, new_size);
	if (!new_ptr) return nullptr;
	
	std::copy(static_cast<uint8_t*>(ptr), static_cast<uint8_t*>(ptr) + old_size, static_cast<uint8_t*>(new_ptr));
	return new_ptr;
}

void ArenaAllocator::free(void* ptr, const std::size_t size)
{
	(void)ptr;
	(void)size;
}

void ArenaAllocator::release_all()
{
	ChunkHeader* current = head;
	while (current != nullptr) {
		ChunkHeader* next = current->next;
		std::size_t size = current->capacity;
		child_allocator.free(current, size);
		current = next;
	}
	head = nullptr;
}

void* ArenaAllocator::allocate_from_chunk(ChunkHeader* chunk, std::size_t alignment, std::size_t size)
{
	uint8_t* chunk_start = reinterpret_cast<uint8_t*>(chunk);
	uintptr_t current_ptr = reinterpret_cast<uintptr_t>(chunk_start + chunk->offset);
	
	uintptr_t mask = alignment - 1;
	uintptr_t aligned_ptr = (current_ptr + mask) & ~mask;
	std::size_t new_offset = aligned_ptr - reinterpret_cast<uintptr_t>(chunk_start) + size;
	
	if (new_offset > chunk->capacity) {
		return nullptr;
	}
	
	chunk->offset = new_offset;
	return reinterpret_cast<void*>(aligned_ptr);
}

void deinit(ArenaAllocator &self)
{
	self.release_all();
}

std::ostream& operator<<(std::ostream& os, const ArenaAllocator& arena) 
{
	std::size_t total_allocated = 0;
	std::size_t total_used = 0;
	std::size_t chunk_count = 0;
	
	os << "=== ArenaAllocator Debug Info ===\n";
	os << "Default Chunk Size: " << arena.default_chunk_size << " bytes\n";
	
	ArenaAllocator::ChunkHeader* current = arena.head;
	if (!current) {
		os << "Status: Empty (No chunks allocated)\n";
		os << "=================================\n";
		return os;
	}
	
	os << "Active Chunks (Head -> Tail):\n";
	while (current != nullptr) {
		chunk_count++;
		total_allocated += current->capacity;
		total_used += current->offset;
		
		double percentage = (static_cast<double>(current->offset) / current->capacity) * 100.0;
		
		os << "  [Chunk " << chunk_count << "] Address: " << current
		<< " | Used: " << current->offset 
		<< " / " << current->capacity << " bytes "
		<< "(" << std::fixed << std::setprecision(1) << percentage << "%)\n";
		
		current = current->next;
	}
	
	std::size_t net_payload = total_used - (chunk_count * sizeof(ArenaAllocator::ChunkHeader));
	
	os << "Summary:\n";
	os << "  Total Chunks:      " << chunk_count << "\n";
	os << "  Total Reserved:    " << total_allocated << " bytes\n";
	os << "  Total Committed:   " << total_used << " bytes\n";
	os << "  Tracking Overhead: " << (chunk_count * sizeof(ArenaAllocator::ChunkHeader)) << " bytes\n";
	os << "  Estimated Payload: " << net_payload << " bytes\n";
	os << "=================================";
	
	return os;
}
