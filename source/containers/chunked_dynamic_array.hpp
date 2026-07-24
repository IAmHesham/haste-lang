/** Created in: 19/51/2026 12:06
*
*/
#ifndef CHUNKED_DYNAMIC_ARRAY_H_
#define CHUNKED_DYNAMIC_ARRAY_H_
#include "containers/allocator.hpp"
#include <cstddef>
#include <utility>
#include <ostream>
#include <cassert>

#ifndef CHUNKED_ARRAY_CHUNK_SIZE
#  define CHUNKED_ARRAY_CHUNK_SIZE 16
#endif

#ifndef CHUNKED_ARRAY_INITIAL_CHUNKS
#  define CHUNKED_ARRAY_INITIAL_CHUNKS 4
#endif

template<typename T>
struct ChunkedDynamicArray {
	T** chunks = nullptr;
	std::size_t len = 0;
	std::size_t chunk_cap = 0;
	
	T& operator[](std::size_t i) 
	{ 
		assert(i < len && "Index out of bounds");
		std::size_t chunk_idx = i / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = i % CHUNKED_ARRAY_CHUNK_SIZE;
		return chunks[chunk_idx][elem_idx]; 
	}
	
	const T& operator[](std::size_t i) const 
	{ 
		assert(i < len && "Index out of bounds");
		std::size_t chunk_idx = i / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = i % CHUNKED_ARRAY_CHUNK_SIZE;
		return chunks[chunk_idx][elem_idx]; 
	}
};

template <typename T>
void init(ChunkedDynamicArray<T> &self) 
{
	self.chunks = nullptr;
	self.len = 0;
	self.chunk_cap = 0;
}

template <typename T>
void grow_chunk_spine(ChunkedDynamicArray<T> &self, Allocator &allocator) 
{
	std::size_t new_chunk_cap = self.chunk_cap == 0 ? CHUNKED_ARRAY_INITIAL_CHUNKS : self.chunk_cap * 2;
	std::size_t old_size = self.chunk_cap * sizeof(T*);
	std::size_t new_size = new_chunk_cap * sizeof(T*);
	
	self.chunks = static_cast<T**>(allocator.reallocate(self.chunks, old_size, alignof(T*), new_size));
	
	// Initialize newly added chunk pointer slots to nullptr
	for (std::size_t i = self.chunk_cap; i < new_chunk_cap; ++i) {
		self.chunks[i] = nullptr;
	}
	self.chunk_cap = new_chunk_cap;
}

template <typename T>
void append(ChunkedDynamicArray<T> &self, Allocator &allocator, const T& value) 
{
	std::size_t chunk_idx = self.len / CHUNKED_ARRAY_CHUNK_SIZE;
	std::size_t elem_idx = self.len % CHUNKED_ARRAY_CHUNK_SIZE;
	
	if (chunk_idx >= self.chunk_cap) {
		grow_chunk_spine(self, allocator);
	}
	
	if (self.chunks[chunk_idx] == nullptr) {
		void* raw_mem = allocator.allocate(alignof(T), CHUNKED_ARRAY_CHUNK_SIZE * sizeof(T));
		self.chunks[chunk_idx] = static_cast<T*>(raw_mem);
	}
	
	new (&self.chunks[chunk_idx][elem_idx]) T(value);
	self.len++;
}

template <typename T>
void append(ChunkedDynamicArray<T> &self, Allocator &allocator, T&& value) 
{
	std::size_t chunk_idx = self.len / CHUNKED_ARRAY_CHUNK_SIZE;
	std::size_t elem_idx = self.len % CHUNKED_ARRAY_CHUNK_SIZE;
	
	if (chunk_idx >= self.chunk_cap) {
		grow_chunk_spine(self, allocator);
	}
	
	if (self.chunks[chunk_idx] == nullptr) {
		void* raw_mem = allocator.allocate(alignof(T), CHUNKED_ARRAY_CHUNK_SIZE * sizeof(T));
		self.chunks[chunk_idx] = static_cast<T*>(raw_mem);
	}
	
	new (&self.chunks[chunk_idx][elem_idx]) T(std::move(value));
	self.len++;
}

template <typename T>
void pop(ChunkedDynamicArray<T> &self) 
{
	if (self.len > 0) {
		self.len--;
		std::size_t chunk_idx = self.len / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = self.len % CHUNKED_ARRAY_CHUNK_SIZE;
		self.chunks[chunk_idx][elem_idx].~T();
	}
}

template <typename T>
void clear(ChunkedDynamicArray<T> &self) 
{
	for (std::size_t i = 0; i < self.len; ++i) {
		std::size_t chunk_idx = i / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = i % CHUNKED_ARRAY_CHUNK_SIZE;
		self.chunks[chunk_idx][elem_idx].~T();
	}
	self.len = 0;
}

template <typename T>
void deinit(ChunkedDynamicArray<T> &self, Allocator &allocator) 
{
	if (self.chunks) {
		clear(self);
		
		for (std::size_t i = 0; i < self.chunk_cap; ++i) {
			if (self.chunks[i] != nullptr) {
				allocator.free(self.chunks[i], CHUNKED_ARRAY_CHUNK_SIZE * sizeof(T));
			}
		}
		
		allocator.free(self.chunks, self.chunk_cap * sizeof(T*));
		self.chunks = nullptr;
		self.chunk_cap = 0;
		self.len = 0;
	}
}

// --- Iterator Support (Stateful Segmented Iterator) ---
template <typename T>
struct ChunkedArrayIterator {
	T** chunks;
	std::size_t index;
	
	ChunkedArrayIterator& operator++() 
	{
		index++;
		return *this;
	}
	
	T& operator*() const 
	{
		std::size_t chunk_idx = index / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = index % CHUNKED_ARRAY_CHUNK_SIZE;
		return chunks[chunk_idx][elem_idx];
	}
	
	bool operator==(const ChunkedArrayIterator& other) const { return index == other.index; }
	bool operator!=(const ChunkedArrayIterator& other) const { return index != other.index; }
};

template <typename T>
ChunkedArrayIterator<T> begin(ChunkedDynamicArray<T> &self) 
{
	return ChunkedArrayIterator<T>{ self.chunks, 0 };
}

template <typename T>
ChunkedArrayIterator<T> end(ChunkedDynamicArray<T> &self) 
{
	return ChunkedArrayIterator<T>{ self.chunks, self.len };
}

template <typename T>
struct ConstChunkedArrayIterator {
	T* const* chunks;
	std::size_t index;
	
	ConstChunkedArrayIterator& operator++() 
	{
		index++;
		return *this;
	}
	
	const T& operator*() const 
	{
		std::size_t chunk_idx = index / CHUNKED_ARRAY_CHUNK_SIZE;
		std::size_t elem_idx = index % CHUNKED_ARRAY_CHUNK_SIZE;
		return chunks[chunk_idx][elem_idx];
	}
	
	bool operator==(const ConstChunkedArrayIterator& other) const { return index == other.index; }
	bool operator!=(const ConstChunkedArrayIterator& other) const { return index != other.index; }
};

template <typename T>
ConstChunkedArrayIterator<T> begin(const ChunkedDynamicArray<T> &self) 
{
	return ConstChunkedArrayIterator<T>{ self.chunks, 0 };
}

template <typename T>
ConstChunkedArrayIterator<T> end(const ChunkedDynamicArray<T> &self) 
{
	return ConstChunkedArrayIterator<T>{ self.chunks, self.len };
}

// --- I/O Formatting Overload ---
template <typename T>
std::ostream &operator<<(std::ostream &os, ChunkedDynamicArray<T> &self)
{
	os << "[";
	auto it = begin(self);
	auto it_end = end(self);
	
	while (it != it_end) {
		os << *it;
		++it;
		if (it != it_end) {
			os << ", ";
		}
	}
	os << "]";
	return os;
}

#endif /* !CHUNKED_DYNAMIC_ARRAY_H_ */
