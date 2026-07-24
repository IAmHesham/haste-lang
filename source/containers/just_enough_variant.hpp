#ifndef JUST_ENOUGH_VARIANT_HPP_
#define JUST_ENOUGH_VARIANT_HPP_

#include "containers/allocator.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>

// -- Type-at indexer -----------------------------------------------------

template <std::size_t I, typename... Ts>
struct JevTypeAt;

template <typename T, typename... Ts>
struct JevTypeAt<0, T, Ts...> { using type = T; };

template <std::size_t I, typename T, typename... Ts>
struct JevTypeAt<I, T, Ts...> { using type = typename JevTypeAt<I - 1, Ts...>::type; };

// -- Compile-time type index ---------------------------------------------

namespace jev_detail {

template <typename T, typename First, typename... Rest>
constexpr std::size_t type_index()
{
	if constexpr (std::is_same_v<T, First>) {
		return 0;
	} else {
		return 1 + type_index<T, Rest...>();
	}
}

} // namespace jev_detail

// -- JustEnoughVariant ---------------------------------------------------

template <typename... Ts>
struct JustEnoughVariant {
	std::uint8_t kind;

	// -- Type queries ----------------------------------------------------

	template <typename T>
	static constexpr std::size_t index_of() noexcept
	{
		return jev_detail::type_index<T, Ts...>();
	}

	template <std::size_t I>
	using type_at = typename JevTypeAt<I, Ts...>::type;

	// -- Layout helpers --------------------------------------------------

	template <typename T>
	static constexpr std::size_t data_offset() noexcept
	{
		constexpr auto a = alignof(T) > alignof(JustEnoughVariant)
		                 ? alignof(T) : alignof(JustEnoughVariant);
		return (sizeof(JustEnoughVariant) + a - 1) & ~(a - 1);
	}

	template <typename T>
	static constexpr std::size_t storage_size() noexcept
	{
		return data_offset<T>() + sizeof(T);
	}

	// -- Accessors -------------------------------------------------------

	template <typename T>
	T &as() noexcept
	{
		return *reinterpret_cast<T*>(reinterpret_cast<char*>(this) + data_offset<T>());
	}

	template <typename T>
	const T &as() const noexcept
	{
		return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(this) + data_offset<T>());
	}

	// -- Factory ---------------------------------------------------------

	template <typename T>
	static JustEnoughVariant *make(Allocator &alloc, const T &value)
	{
		constexpr auto sz = storage_size<T>();
		void *mem = alloc.allocate(std::max(alignof(JustEnoughVariant), alignof(T)), sz);
		std::memset(mem, 0, sz);

		auto *v = static_cast<JustEnoughVariant*>(mem);
		v->kind = static_cast<std::uint8_t>(index_of<T>());
		std::memcpy(reinterpret_cast<char*>(mem) + data_offset<T>(), &value, sizeof(T));
		return v;
	}

	// -- Visit -----------------------------------------------------------

	template <typename F>
	decltype(auto) visit(F &&f)
	{
		return visit_impl<F, 0>(static_cast<F&&>(f));
	}

	template <typename F>
	decltype(auto) visit(F &&f) const
	{
		return visit_impl<F, 0>(static_cast<F&&>(f));
	}

private:
	template <typename F, std::size_t I>
	decltype(auto) visit_impl(F &&f)
	{
		if constexpr (I < sizeof...(Ts)) {
			if (kind == I) {
				return f(as<type_at<I>>());
			}
			return visit_impl<F, I + 1>(static_cast<F&&>(f));
		} else {
			__builtin_unreachable();
		}
	}

	template <typename F, std::size_t I>
	decltype(auto) visit_impl(F &&f) const
	{
		if constexpr (I < sizeof...(Ts)) {
			if (kind == I) {
				return f(as<type_at<I>>());
			}
			return visit_impl<F, I + 1>(static_cast<F&&>(f));
		} else {
			__builtin_unreachable();
		}
	}
};

#endif
