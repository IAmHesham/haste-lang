#ifndef COMMON_HPP_
#define COMMON_HPP_
#include <exception>
#include <utility>

template <typename F>
class Deferrer {
	F f;
	bool active;
public:
	explicit Deferrer(F&& f) : f(std::forward<F>(f)), active(true) {}
	
	Deferrer(Deferrer&& other) noexcept : f(std::move(other.f)), active(other.active) {
		other.active = false;
	}
	
	~Deferrer() {
		if (active) {
			f();
		}
	}
	
	Deferrer(const Deferrer&) = delete;
	Deferrer& operator=(const Deferrer&) = delete;
	Deferrer& operator=(Deferrer&&) = delete;
};

struct DeferHelper {
	template <typename F>
	Deferrer<F> operator+(F&& f) {
		return Deferrer<F>(std::forward<F>(f));
	}
};

template <typename F>
class ErrDeferrer {
	F f;
	int ex_count;
	bool active;
public:
	explicit ErrDeferrer(F&& f) : f(std::forward<F>(f)), ex_count(std::uncaught_exceptions()), active(true) {}
	
	ErrDeferrer(ErrDeferrer&& other) noexcept : f(std::move(other.f)), ex_count(other.ex_count), active(other.active) {
		other.active = false;
	}
	
	~ErrDeferrer() {
		if (active && std::uncaught_exceptions() > ex_count) {
			f();
		}
	}
	
	ErrDeferrer(const ErrDeferrer&) = delete;
	ErrDeferrer& operator=(const ErrDeferrer&) = delete;
	ErrDeferrer& operator=(ErrDeferrer&&) = delete;
};

struct ErrDeferHelper {
	template <typename F>
	ErrDeferrer<F> operator+(F&& f) {
		return ErrDeferrer<F>(std::forward<F>(f));
	}
};

#define DEFER_CONCAT_IMPL(x, y) x##y
#define DEFER_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)

#ifdef __COUNTER__
#  define defer const auto DEFER_CONCAT(_defer_obj_, __COUNTER__) = DeferHelper() + [&]()
#  define errdefer const auto DEFER_CONCAT(_errdefer_obj_, __COUNTER__) = ErrDeferHelper() + [&]()
#else
#  define defer const auto DEFER_CONCAT(_defer_obj_, __LINE__) = DeferHelper() + [&]()
#  define errdefer const auto DEFER_CONCAT(_errdefer_obj_, __LINE__) = ErrDeferHelper() + [&]()
#endif

#endif // !COMMON_HPP_
