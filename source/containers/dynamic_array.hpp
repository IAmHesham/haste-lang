#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include "containers/allocator.hpp"
#include <cstddef>
#include <utility>
#include <span>
#include <ostream>

#ifndef DYNAMIC_ARRAY_INITIAL_CAP
#  define DYNAMIC_ARRAY_INITIAL_CAP 16
#endif

#ifndef DYNAMIC_ARRAY_GROW_FACTOR
#  define DYNAMIC_ARRAY_GROW_FACTOR 2
#endif

template<typename T>
struct DynamicArray {
    T* items = nullptr;
    std::size_t len = 0;
    std::size_t cap = 0;

    T& operator[](std::size_t i) 
    { 
        return items[i]; 
    }
    
    const T& operator[](std::size_t i) const 
    { 
        return items[i]; 
    }
};

template <typename T>
void init(DynamicArray<T> &self) 
{
    self.items = nullptr;
    self.len = 0;
    self.cap = 0;
}

template <typename T>
std::span<T> as_span(DynamicArray<T> &self) 
{
    return std::span<T>(self.items, self.len);
}

template <typename T>
std::span<const T> as_span(const DynamicArray<T> &self) 
{
    return std::span<const T>(self.items, self.len);
}

template <typename T>
std::span<T> as_full_span(DynamicArray<T> &self) 
{
    return std::span<T>(self.items, self.cap);
}

template <typename T>
void reserve(DynamicArray<T> &self, Allocator &allocator, std::size_t min_cap) 
{
    if (self.cap >= min_cap) return;

    std::size_t new_cap = self.cap ? self.cap : DYNAMIC_ARRAY_INITIAL_CAP;
    while (new_cap < min_cap) new_cap *= DYNAMIC_ARRAY_GROW_FACTOR;

    std::size_t old_size = self.cap * sizeof(T);
    std::size_t new_size = new_cap * sizeof(T);

    self.items = static_cast<T*>(allocator.reallocate(self.items, old_size, alignof(T), new_size));
    self.cap = new_cap;
}

template <typename T>
void grow(DynamicArray<T> &self, Allocator &allocator) 
{
    std::size_t new_cap = self.cap == 0 ? DYNAMIC_ARRAY_INITIAL_CAP : self.cap * DYNAMIC_ARRAY_GROW_FACTOR;
    std::size_t old_size = self.cap * sizeof(T);
    std::size_t new_size = new_cap * sizeof(T);

    self.items = static_cast<T*>(allocator.reallocate(self.items, old_size, alignof(T), new_size));
    self.cap = new_cap;
}

template <typename T>
void append(DynamicArray<T> &self, Allocator &allocator, const T value) 
{
    if (self.len >= self.cap) grow(self, allocator);
    new (&self.items[self.len++]) T(value);
}

// template <typename T>
// void append(DynamicArray<T> &self, Allocator &allocator, T&& value) 
// {
//     if (self.len >= self.cap) grow(self, allocator);
//     new (&self.items[self.len++]) T(std::move(value));
// }

template <typename T>
void pop(DynamicArray<T> &self) 
{
    if (self.len > 0) {
        self.len--;
        self.items[self.len].~T(); // Call destructor on popped element
    }
}

template <typename T>
void clear(DynamicArray<T> &self) 
{
    for (std::size_t i = 0; i < self.len; ++i) {
        self.items[i].~T();
    }
    self.len = 0;
}

template <typename T>
void deinit(DynamicArray<T> &self, Allocator &allocator) 
{
    if (self.items) {
        for (std::size_t i = 0; i < self.len; ++i) {
            self.items[i].~T();
        }
        allocator.free(self.items, self.cap * sizeof(T));
        self.items = nullptr;
        self.cap = 0;
        self.len = 0;
    }
}

// --- Iterator Support (Global Functions) ---

template <typename T>
T* begin(DynamicArray<T> &self) 
{ 
    return self.items; 
}

template <typename T>
T* end(DynamicArray<T> &self) 
{ 
    return self.items + self.len; 
}

template <typename T>
const T* begin(const DynamicArray<T> &self) 
{ 
    return self.items; 
}

template <typename T>
const T* end(const DynamicArray<T> &self) 
{ 
    return self.items + self.len; 
}

// --- I/O Formatting Overload ---
template <typename T>
std::ostream &operator<<(std::ostream &os, DynamicArray<T> &self)
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

#endif
