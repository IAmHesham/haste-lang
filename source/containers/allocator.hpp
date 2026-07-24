#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include "containers/string.hpp"
#include "containers/string_view.hpp"
#include <span>
#include <cstring>

struct Allocator {
	virtual void *allocate(
		const std::size_t alignment,
		const std::size_t size) = 0;
	virtual void *reallocate(
		void *ptr,
		const std::size_t old_size,
		const std::size_t alignment,
		const std::size_t new_size) = 0;
	virtual void free(
		void *ptr,
		const std::size_t size) = 0;

	virtual ~Allocator() = default;
};

#define default_allocator default_allocator_
extern Allocator *default_allocator_;
extern std::size_t allocated;

void set_default_allocator(Allocator *allocator);
Allocator *get_default_allocator(void);

template <class T>
T *create(Allocator *allocator)
{
	T *result = (T*)allocator->allocate(alignof(T), sizeof(T));
	memset(result, 0, sizeof(T));
	return result;
}

template <class T>
T *create(Allocator &allocator)
{
	T *result = (T*)allocator.allocate(alignof(T), sizeof(T));
	memset(result, 0, sizeof(T));
	return result;
}

template <class T>
T *create(Allocator *allocator, T init)
{
	T *result = (T*)allocator->allocate(alignof(T), sizeof(T));
	*result = init;
	return result;
}

template <class T>
T *create(Allocator &allocator, T init)
{
	T *result = (T*)allocator.allocate(alignof(T), sizeof(T));
	*result = init;
	return result;
}

template <class T>
std::span<T> alloc(
	Allocator *allocator,
	std::size_t count)
{
	const std::size_t size = sizeof(T) * count;
	T *ptr = (T*)allocator->allocate(alignof(T), size);
	return std::span(ptr, count);
}

template <class T>
std::span<T> alloc(
	Allocator &allocator,
	std::size_t count)
{
	const std::size_t size = sizeof(T) * count;
	T *ptr = (T*)allocator.allocate(alignof(T), size);
	return std::span(ptr, count);
}

template <class T>
std::span<T> realloc(
	Allocator *allocator,
	std::span<T> former,
	std::size_t new_count)
{
	const std::size_t new_size = sizeof(T) * new_count;
	T *ptr = (T*)allocator->reallocate(former.data(), former.size_bytes(), alignof(T), new_size);
	return std::span(ptr, new_size);
}

template <class T>
std::span<T> realloc(
	Allocator &allocator,
	std::span<T> former,
	std::size_t new_count)
{
	const std::size_t new_size = sizeof(T) * new_count;
	T *ptr = (T*)allocator.reallocate(former.data(), former.size_bytes(), alignof(T), new_size);
	return std::span(ptr, new_size);
}

template <class T>
void destroy(
	Allocator *allocator,
	T *data)
{
	allocator->free(data, sizeof(T));
}

template <class T>
void destroy(
	Allocator &allocator,
	T *data)
{
	allocator.free(data, sizeof(T));
}

template <class T>
void destroy(
	Allocator *allocator,
	std::span<T> data)
{
	allocator->free(data.data(), data.size_bytes());
}

template <class T>
void destroy(
	Allocator &allocator,
	std::span<T> data)
{
	allocator.free(data.data(), data.size_bytes());
}

template <class T>
T *clone(Allocator &allocator, T *data)
{
	return create(allocator, *data);
}

String clone(Allocator &allocator, String data);
StringView clone(Allocator &allocator, StringView data);

#endif // !ALLOCATOR_H_
