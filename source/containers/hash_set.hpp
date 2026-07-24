#ifndef HASH_SET_HPP_
#define HASH_SET_HPP_

#include "containers/allocator.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
#include <algorithm>
#include <functional>
#include <ostream>

#ifndef HASHSET_INITIAL_CAP
#  define HASHSET_INITIAL_CAP 16
#endif

template<typename K>
struct HashSet {
	struct Entry {
		K key;
		std::int32_t dib = -1; // Distance From Ideal Bucket. -1 means empty slot.
	};
	
	Entry* entries = nullptr;
	std::size_t size = 0;
	std::size_t cap = 0;
};

template<typename K>
void hash_set_grow(HashSet<K> &self, std::size_t new_cap, Allocator &allocator);

template <typename K>
void init(HashSet<K> &self) 
{
	self.entries = nullptr;
	self.size = 0;
	self.cap = 0;
}

template <typename K>
void deinit(HashSet<K> &self, Allocator &allocator) 
{
	if (self.entries) {
		for (std::size_t i = 0; i < self.cap; ++i) {
			if (self.entries[i].dib >= 0) {
				self.entries[i].key.~K();
			}
		}
		destroy(allocator, std::span(self.entries, self.cap));
		self.entries = nullptr;
	}
	self.size = 0;
	self.cap = 0;
}

template <typename K>
bool put(HashSet<K> &self, Allocator &allocator, const K &key) 
{
    if (self.cap == 0 || (self.size + 1) * 10 > self.cap * 8) {
        std::size_t new_cap = self.cap == 0 ? HASHSET_INITIAL_CAP : self.cap * 2;
        hash_set_grow(self, new_cap, allocator);
    }

    std::size_t hash = std::hash<K>{}(key);
    std::size_t index = hash % self.cap;

    K current_key = key;
    std::int32_t current_dib = 0;

    while (true) {
        auto &entry = self.entries[index];

        // 1. ALWAYS check for an exact match first before swapping state!
        if (entry.dib != -1 && entry.key == current_key) {
            return false; 
        }

        // 2. Empty slot found
        if (entry.dib == -1) {
            new (&entry.key) K(std::move(current_key));
            entry.dib = current_dib;
            self.size++;
            return true; 
        }

        // 3. Robin Hood eviction
        if (current_dib > entry.dib) {
            std::swap(current_key, entry.key);
            std::swap(current_dib, entry.dib);
        }

        index = (index + 1) % self.cap;
        current_dib++;
    }
}

template <typename K>
bool contains(HashSet<K> &self, const K &key) 
{
	return get(self, key) != nullptr;
}

template <typename K>
K* get(HashSet<K> &self, const K &key) 
{
	if (self.size == 0) return nullptr;
	
	std::size_t hash = std::hash<K>{}(key);
	std::size_t index = hash % self.cap;
	std::int32_t current_dib = 0;
	
	while (true) {
		auto &entry = self.entries[index];
		
		// Early termination: if we hit an empty slot or a slot with a lower DIB
		// than our current search depth, the key mathematically cannot exist.
		if (entry.dib == -1 || current_dib > entry.dib) {
			return nullptr; 
		}
		
		if (entry.key == key) {
			return &entry.key;
		}
		
		index = (index + 1) % self.cap;
		current_dib++;
	}
}

template <typename K>
bool remove(HashSet<K> &self, const K &key) 
{
	if (self.size == 0) return false;
	
	std::size_t hash = std::hash<K>{}(key);
	std::size_t index = hash % self.cap;
	std::int32_t current_dib = 0;
	
	while (true) {
		auto &entry = self.entries[index];
		
		if (entry.dib == -1 || current_dib > entry.dib) {
			return false;
		}
		
		if (entry.key == key) {
			entry.key.~K();
			entry.dib = -1;
			self.size--;
			
			std::size_t current_idx = index;
			std::size_t next_idx = (current_idx + 1) % self.cap;
			
			while (self.entries[next_idx].dib > 0) {
				auto &curr = self.entries[current_idx];
				auto &next = self.entries[next_idx];
				
				new (&curr.key) K(std::move(next.key));
				curr.dib = next.dib - 1;
				
				next.key.~K();
				next.dib = -1;
				
				current_idx = next_idx;
				next_idx = (next_idx + 1) % self.cap;
			}
			return true;
		}
		
		index = (index + 1) % self.cap;
		current_dib++;
	}
}

template<typename K>
void hash_set_grow(HashSet<K> &self, std::size_t new_cap, Allocator &allocator) 
{
	using EntryType = typename HashSet<K>::Entry;
	
	std::size_t bytes_needed = new_cap * sizeof(EntryType);
	void* raw_mem = allocator.allocate(alignof(EntryType), bytes_needed);
	
	EntryType* new_entries = static_cast<EntryType*>(raw_mem);
	for (std::size_t i = 0; i < new_cap; ++i) {
		new_entries[i].dib = -1;
	}
	
	EntryType* old_entries = self.entries;
	std::size_t old_cap = self.cap;
	
	self.entries = new_entries;
	self.cap = new_cap;
	self.size = 0; 
	
	for (std::size_t i = 0; i < old_cap; ++i) {
		if (old_entries[i].dib >= 0) {
			put(self, allocator, old_entries[i].key);
			old_entries[i].key.~K();
		}
	}
	
	if (old_entries) {
		allocator.free(old_entries, old_cap * sizeof(EntryType));
	}
}

// --- Iterator Support ---

template <typename K>
struct HashSetIterator {
	using EntryType = typename HashSet<K>::Entry;
	
	EntryType* current;
	EntryType* end;
	
	void advance() 
	{
		while (current < end && current->dib == -1) {
			current++;
		}
	}
	
	HashSetIterator& operator++() 
	{
		if (current < end) {
			current++;
			advance();
		}
		return *this;
	}
	
	EntryType& operator*() const 
	{ 
		return *current; 
	}
	
	EntryType* operator->() const 
	{ 
		return current; 
	}
	
	bool operator==(const HashSetIterator& other) const 
	{ 
		return current == other.current; 
	}
	
	bool operator!=(const HashSetIterator& other) const 
	{ 
		return current != other.current; 
	}
};

template <typename K>
HashSetIterator<K> begin(HashSet<K> &self) 
{
	HashSetIterator<K> it{ self.entries, self.entries + self.cap };
	it.advance();
	return it;
}

template <typename K>
HashSetIterator<K> end(HashSet<K> &self) 
{
	return HashSetIterator<K>{ self.entries + self.cap, self.entries + self.cap };
}

template <typename K>
struct ConstHashSetIterator {
	using EntryType = const typename HashSet<K>::Entry;
	
	EntryType* current;
	EntryType* end;
	
	void advance() 
	{
		while (current < end && current->dib == -1) {
			current++;
		}
	}
	
	ConstHashSetIterator& operator++() 
	{
		if (current < end) {
			current++;
			advance();
		}
		return *this;
	}
	
	EntryType& operator*() const 
	{ 
		return *current; 
	}
	
	EntryType* operator->() const 
	{ 
		return current; 
	}
	
	bool operator==(const ConstHashSetIterator& other) const 
	{ 
		return current == other.current; 
	}
	
	bool operator!=(const ConstHashSetIterator& other) const 
	{ 
		return current != other.current; 
	}
};

template <typename K>
ConstHashSetIterator<K> begin(const HashSet<K> &self) 
{
	ConstHashSetIterator<K> it{ self.entries, self.entries + self.cap };
	it.advance();
	return it;
}

template <typename K>
ConstHashSetIterator<K> end(const HashSet<K> &self) 
{
	return ConstHashSetIterator<K>{ self.entries + self.cap, self.entries + self.cap };
}

// --- I/O Formatting Overload ---

template <typename K>
std::ostream &operator<<(std::ostream &os, HashSet<K> &self)
{
	os << "{";
	auto it = begin(self);
	auto it_end = end(self);
	
	while (it != it_end) {
		os << it->key;
		++it;
		if (it != it_end) {
			os << ", ";
		}
	}
	os << "}";
	return os;
}

#endif // !HASH_SET_HPP_
