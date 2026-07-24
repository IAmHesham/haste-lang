#ifndef HASH_MAP_HPP
#define HASH_MAP_HPP

#include "containers/allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <utility>
#include <algorithm>
#include <functional>

#ifndef HASHMAP_INITIAL_CAP
#  define HASHMAP_INITIAL_CAP 16
#endif

template<typename K, typename V>
struct HashMap {
	struct Entry {
		K key;
		V value;
		std::int32_t dib = -1; // Distance From Ideal Bucket. -1 means empty slot.
	};
	
	Entry* entries = nullptr;
	std::size_t size = 0;
	std::size_t cap = 0;
};

template<typename K, typename V>
void hash_map_grow(HashMap<K, V> &self, std::size_t new_cap, Allocator &allocator);

template <typename K, typename V>
void init(HashMap<K, V> &self) 
{
	self.entries = nullptr;
	self.size = 0;
	self.cap = 0;
}

template <typename K, typename V>
void deinit(HashMap<K, V> &self, Allocator &allocator) 
{
	if (self.entries) {
		for (std::size_t i = 0; i < self.cap; ++i) {
			if (self.entries[i].dib >= 0) {
				self.entries[i].key.~K();
				self.entries[i].value.~V();
			}
		}
		allocator.free(self.entries, self.cap * sizeof(typename HashMap<K, V>::Entry));
		self.entries = nullptr;
	}
	self.size = 0;
	self.cap = 0;
}

template <typename K, typename V>
void put(HashMap<K, V> &self, Allocator &allocator, const K &key, const V &value) 
{
	if (self.cap == 0 || (self.size + 1) * 10 > self.cap * 8) {
		std::size_t new_cap = self.cap == 0 ? HASHMAP_INITIAL_CAP : self.cap * 2;
		hash_map_grow(self, new_cap, allocator);
	}
	
	std::size_t hash = std::hash<K>{}(key);
	std::size_t index = hash % self.cap;
	
	K current_key = key;
	V current_value = value;
	std::int32_t current_dib = 0;
	
	while (true) {
		auto &entry = self.entries[index];
		
		if (entry.dib == -1) {
			new (&entry.key) K(std::move(current_key));
			new (&entry.value) V(std::move(current_value));
			entry.dib = current_dib;
			self.size++;
			return;
		}
		
		if (entry.key == current_key) {
			entry.value = std::move(current_value);
			return;
		}
		
		if (current_dib > entry.dib) {
			std::swap(current_key, entry.key);
			std::swap(current_value, entry.value);
			std::swap(current_dib, entry.dib);
		}
		
		index = (index + 1) % self.cap;
		current_dib++;
	}
}

template <typename K, typename V>
bool contains(const HashMap<K, V> &self, const K &key) 
{
	if (self.size == 0) return false;
	
	std::size_t hash = std::hash<K>{}(key);
	std::size_t index = hash % self.cap;
	std::int32_t current_dib = 0;
	
	while (true) {
		const auto &entry = self.entries[index];
		
		if (entry.dib == -1 || current_dib > entry.dib) {
			return false; 
		}
		
		if (entry.key == key) {
			return true;
		}
		
		index = (index + 1) % self.cap;
		current_dib++;
	}
}

template <typename K, typename V>
V* get(const HashMap<K, V> &self, const K &key) 
{
	if (self.size == 0) return nullptr;
	
	std::size_t hash = std::hash<K>{}(key);
	std::size_t index = hash % self.cap;
	std::int32_t current_dib = 0;
	
	while (true) {
		auto &entry = self.entries[index];
		
		if (entry.dib == -1 || current_dib > entry.dib) {
			return nullptr; 
		}
		
		if (entry.key == key) {
			return &entry.value;
		}
		
		index = (index + 1) % self.cap;
		current_dib++;
	}
}

template <typename K, typename V>
bool remove(HashMap<K, V> &self, const K &key) 
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
			entry.value.~V();
			entry.dib = -1;
			self.size--;
			
			std::size_t current_idx = index;
			std::size_t next_idx = (current_idx + 1) % self.cap;
			
			while (self.entries[next_idx].dib > 0) {
				auto &curr = self.entries[current_idx];
				auto &next = self.entries[next_idx];
				
				new (&curr.key) K(std::move(next.key));
				new (&curr.value) V(std::move(next.value));
				curr.dib = next.dib - 1;
				
				next.key.~K();
				next.value.~V();
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

template<typename K, typename V>
void hash_map_grow(HashMap<K, V> &self, std::size_t new_cap, Allocator &allocator) 
{
	using EntryType = typename HashMap<K, V>::Entry;
	
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
			put(self, allocator, old_entries[i].key, old_entries[i].value);
			old_entries[i].key.~K();
			old_entries[i].value.~V();
		}
	}
	
	if (old_entries) {
		allocator.free(old_entries, old_cap * sizeof(EntryType));
	}
}

template <typename K, typename V>
struct HashMapIterator {
	using EntryType = typename HashMap<K, V>::Entry;
	
	EntryType* current;
	EntryType* end;
	
	// Advance to the next occupied slot
	void advance() 
	{
		while (current < end && current->dib == -1) {
			current++;
		}
	}
	
	// Pre-increment operator (++it)
	HashMapIterator& operator++() 
	{
		if (current < end) {
			current++; // Move past current item
			advance();  // Skip subsequent empty slots
		}
		return *this;
	}
	
	// Dereference operators to expose the entry
	EntryType& operator*() const 
	{ 
		return *current; 
	}
	
	EntryType* operator->() const 
	{ 
		return current; 
	}
	
	bool operator==(const HashMapIterator& other) const 
	{ 
		return current == other.current; 
	}
	
	bool operator!=(const HashMapIterator& other) const 
	{ 
		return current != other.current; 
	}
};

template <typename K, typename V>
HashMapIterator<K, V> begin(HashMap<K, V> &self) 
{
	HashMapIterator<K, V> it{ self.entries, self.entries + self.cap };
	it.advance();
	return it;
}

template <typename K, typename V>
HashMapIterator<K, V> end(HashMap<K, V> &self) 
{
	return HashMapIterator<K, V>{ self.entries + self.cap, self.entries + self.cap };
}

template <typename K, typename V>
struct ConstHashMapIterator {
	using EntryType = const typename HashMap<K, V>::Entry;
	
	EntryType* current;
	EntryType* end;
	
	void advance() 
	{
		while (current < end && current->dib == -1) {
			current++;
		}
	}
	
	ConstHashMapIterator& operator++() 
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
	
	bool operator==(const ConstHashMapIterator& other) const 
	{ 
		return current == other.current; 
	}
	
	bool operator!=(const ConstHashMapIterator& other) const 
	{ 
		return current != other.current; 
	}
};

template <typename K, typename V>
ConstHashMapIterator<K, V> begin(const HashMap<K, V> &self)
{
	ConstHashMapIterator<K, V> it{ self.entries, self.entries + self.cap };
	it.advance();
	return it;
}

template <typename K, typename V>
ConstHashMapIterator<K, V> end(const HashMap<K, V> &self)
{
	return ConstHashMapIterator<K, V>{ self.entries + self.cap, self.entries + self.cap };
}

template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, HashMap<K, V> &self)
{
	os << "{";
	auto it = begin(self);
	auto it_end = end(self);
	
	while (it != it_end) {
		os << it->key << ": " << it->value;
		++it;
		if (it != it_end) {
			os << ", ";
		}
	}
	os << "}";
	return os;
}

#endif
