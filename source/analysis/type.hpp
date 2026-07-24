/** Created in: 22/51/2026 13:06
  *
  */
#ifndef TYPE_H_
#define TYPE_H_

#include "containers/linked_list.hpp"
#include "containers/string_view.hpp"
#include "containers/hash_map.hpp"
#include "utils/intern.hpp"
#include "value.hpp"

#include <variant>

namespace haste {

struct Type {
	/** @breif mainly used to simplify type conversions and coercion */
	enum class Category : std::uint8_t {
		Auto,
		Type,
		UntypedNumber,
		UntypedBoolean,
		Int,
		Float,
		Boolean,
		Function,
	};

	struct Auto {};
	struct AType {};

	struct UntypedNumber {
		bool is_floating : 1;
	};
	struct UntypedBoolean {};

	struct Int {
		bool is_signed : 1;
		std::uint16_t bits;
	};

	struct Float {
		std::uint16_t bits;
	};

	struct Boolean {
		std::uint16_t bits;
	};

	struct Function {
		LinkedList<Type> arguments;
		Type *returns;
	};

	StringView full_name {}; // This is what makes types actually distinct
	StringView short_name {};
	HashMap<StringView, Overloads> methods {};
	std::variant<
		Auto,
		AType,
		UntypedNumber,
		UntypedBoolean,
		Int,
		Float,
		Boolean,
		Function> info;

	std::size_t alignment = 0;
	std::size_t size = 0;

	static Value type();
	static Value untyped_boolean(InternPool &pool);
	static Value untyped_number(InternPool &pool, bool is_floating = false);
	static Value integer(InternPool &pool, std::uint16_t bits = 32);
	static Value uinteger(InternPool &pool, std::uint16_t bits = 32);
	static Value floating(InternPool &pool, std::uint16_t bits = 32);
	static Value boolean(InternPool &pool, std::uint16_t bit = 1);

	bool is_number() const;
	std::size_t has_method(StringView name) const;
	Overloads *get_methods(StringView name);

	Value *get_method(StringView name, std::initializer_list<Type*> args);
	Value *get_method(StringView name, LinkedList<Type*> args);
	Value *get_method(StringView name, Type *return_type);

	template <typename T>
	bool is() const
	{
		return std::holds_alternative<T>(info);
	}

	template <typename T>
	T *as()
	{
		return std::get_if<T>(&info);
	}
};

struct ConversionRule {
	enum class Kind {
		None, /** Casting is not possible */
		Coercion, /** Implicit coercion */
		Cast, /** Explicit conversion */
	};

	Kind kind;
	Value (*convert)(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind); /** nullptr = identity */
};

// --- Conversion function declarations ---
Value coerce_number_to_boolean(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind);
Value coerce_boolean_to_number(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind);
Value coerce_number_to_int(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind);
Value coerce_number_to_float(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind);

static constexpr ConversionRule CONVERSION_TABLE[8][8] = {
	// Source: Auto (0) — not a value type
	{
		{ConversionRule::Kind::Coercion, nullptr}, // → Auto
		{ConversionRule::Kind::None, nullptr},     // → Type
		{ConversionRule::Kind::None, nullptr},     // → UntypedNumber
		{ConversionRule::Kind::None, nullptr},     // → UntypedBoolean
		{ConversionRule::Kind::None, nullptr},     // → Int
		{ConversionRule::Kind::None, nullptr},     // → Float
		{ConversionRule::Kind::None, nullptr},     // → Boolean
		{ConversionRule::Kind::None, nullptr},     // -> function
	},
	// Source: Type (1) — not a value type
	{
		{ConversionRule::Kind::None, nullptr},     // → Auto
		{ConversionRule::Kind::Coercion, nullptr}, // → Type
		{ConversionRule::Kind::None, nullptr},     // → UntypedNumber
		{ConversionRule::Kind::None, nullptr},     // → UntypedBoolean
		{ConversionRule::Kind::None, nullptr},     // → Int
		{ConversionRule::Kind::None, nullptr},     // → Float
		{ConversionRule::Kind::None, nullptr},     // → Boolean
		{ConversionRule::Kind::None, nullptr},     // -> function
	},
	// Source: UntypedNumber (2)
	{
		{ConversionRule::Kind::Coercion, nullptr},                  // → Auto
		{ConversionRule::Kind::None,     nullptr},                  // → Type
		{ConversionRule::Kind::Coercion, nullptr},                  // → UntypedNumber
		{ConversionRule::Kind::Cast,     coerce_number_to_boolean}, // → UntypedBoolean
		{ConversionRule::Kind::Coercion, coerce_number_to_int  },   // → Int
		{ConversionRule::Kind::Coercion, coerce_number_to_float},   // → Float
		{ConversionRule::Kind::Coercion, coerce_number_to_boolean}, // → Boolean
		{ConversionRule::Kind::None, nullptr},                      // -> function
	},
	// Source: UntypedBoolean (3)
	{
		{ConversionRule::Kind::Coercion, nullptr},                  // → Auto
		{ConversionRule::Kind::None,     nullptr},                  // → Type
		{ConversionRule::Kind::Cast,     coerce_boolean_to_number}, // → UntypedNumber
		{ConversionRule::Kind::Coercion, nullptr},                  // → UntypedBoolean
		{ConversionRule::Kind::Cast,     coerce_boolean_to_number}, // → Int
		{ConversionRule::Kind::Cast,     coerce_boolean_to_number}, // → Float
		{ConversionRule::Kind::Coercion, nullptr},                  // → Boolean
		{ConversionRule::Kind::None, nullptr},                      // -> function
	},
	// Source: Int (4)
	{
		{ConversionRule::Kind::Coercion, nullptr},                  // → Auto
		{ConversionRule::Kind::None,     nullptr},                  // → Type
		{ConversionRule::Kind::None,     nullptr},                  // → UntypedNumber
		{ConversionRule::Kind::None,     nullptr},                  // → UntypedBoolean
		{ConversionRule::Kind::Cast,     nullptr},                  // → Int
		{ConversionRule::Kind::Cast,     nullptr},                  // → Float
		{ConversionRule::Kind::Cast,     coerce_number_to_boolean}, // → Boolean
		{ConversionRule::Kind::None, nullptr},                      // -> function
	},
	// Source: Float (5)
	{
		{ConversionRule::Kind::Coercion, nullptr},                  // → Auto
		{ConversionRule::Kind::None,     nullptr},                  // → Type
		{ConversionRule::Kind::None,     nullptr},                  // → UntypedNumber
		{ConversionRule::Kind::None,     nullptr},                  // → UntypedBoolean
		{ConversionRule::Kind::Cast,     nullptr},                  // → Int
		{ConversionRule::Kind::Cast,     nullptr},                  // → Float
		{ConversionRule::Kind::Cast,     coerce_number_to_boolean}, // → Boolean
		{ConversionRule::Kind::None, nullptr},                      // -> function
	},
	// Source: Boolean (6)
	{
		{ConversionRule::Kind::Coercion, nullptr},                  // → Auto
		{ConversionRule::Kind::None, nullptr},                      // → Type
		{ConversionRule::Kind::None, nullptr},                      // → UntypedNumber
		{ConversionRule::Kind::None, nullptr},                      // → UntypedBoolean
		{ConversionRule::Kind::Cast, coerce_boolean_to_number},     // → Int
		{ConversionRule::Kind::Cast, coerce_boolean_to_number},     // → Float
		{ConversionRule::Kind::Cast, nullptr},                      // → Boolean
		{ConversionRule::Kind::None, nullptr},                      // -> function
	},
	// Source: Function (7)
	{
		{ConversionRule::Kind::None, nullptr}, // → Auto
		{ConversionRule::Kind::None, nullptr}, // → Type
		{ConversionRule::Kind::None, nullptr}, // → UntypedNumber
		{ConversionRule::Kind::None, nullptr}, // → UntypedBoolean
		{ConversionRule::Kind::None, nullptr}, // → Int
		{ConversionRule::Kind::None, nullptr}, // → Float
		{ConversionRule::Kind::None, nullptr}, // → Boolean
		{ConversionRule::Kind::None, nullptr}, // -> function
	},
};

std::size_t gategory(const Type *tp);
bool coercible(const Type *from, const Type *to);
bool castable(const Type *from, const Type *to);

/** @brief checks if `a` matches `b`. a type match is done by checking if
  * @brief `a` == `b` is `true`. or if `coercible(a, b)` is `true`. or
  * @brief `coercible(b, a)` is `true`. otherwise the type doesn't match.
  * @returns `true` if the conditions above are met. `false` is not
  * @note In this compiler this is going to be used to check if `assignment` is possible
  * @note or when we can pass a value to an argument of a function call.
  */
bool matches(const Type *const a, const Type *const b);

/** @brief coerce a value `val` to type `target` using the coercion rules
  * @brief of the language.
  * @returns a Value of type `target`. upon failing, it will return
  *          a bad value of `Bad::ImplicitCoercion` when you can only use explicit
  *          cast. or `Bad::Imposible` when no casting is possible.
  */
Value coerce(InternPool &pool, Value val, const Type *target);

/** @brief casts a value `val` to type `target` using the casting rules
  * @brief of the language
  * @returns a Value of type `target`. upon failing it will return
  *          a bad value of `Bad::Impossible`.
  */
Value cast(InternPool &pool, Value val, const Type *target);

};

template <>
struct std::hash<haste::Type> {
	std::size_t operator()(const haste::Type &str) const;
};

#endif /* !TYPE_H_ */
