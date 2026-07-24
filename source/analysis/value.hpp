/** Created in: 22/45/2026 13:06
  *
  */
#ifndef VALUE_H_
#define VALUE_H_

#include "containers/just_enough_variant.hpp"
#include "containers/linked_list.hpp"

#include <cstdint>
#include <variant>

namespace haste {

struct InternPool;

struct Type;
struct None {};

using Object = JustEnoughVariant<>;
enum Bad : std::uint8_t {
	Propagated,
	Impossible, /** general purpose error */

	// numbers
	InvalidBitWidth,
	BadArithmatic,

	// conversion
	ImplicitCoercion,
};

struct Value {
	Type *type_info = nullptr;
	std::variant<
		Bad,
		None,
		bool,
		double,
		Type*,
		Object*> variants;

	static Value none();
	static Value bad(const Bad whats_bad);
	static Value vtrue(InternPool &pool);
	static Value vfalse(InternPool &pool);
	static Value boolean(InternPool &pool, const bool value, const std::uint16_t bits = 1);
	static Value number(InternPool &pool, const double value);
	static Value type(Type *type_info);

	static Value typed_number(Type *type_info, const double value);
	static Value typed_number(Value type, const double value);

	template <typename T>
	bool is() const
	{
		return std::holds_alternative<T>(variants);
	}

	bool is(Bad whats_bad) const
	{
		if (auto x = std::get_if<Bad>(&variants)) {
			return *x == whats_bad;
		}

		return false;
	}

	template <typename T>
	T *as()
	{
		return std::get_if<T>(&variants);
	}
};

using Overloads = LinkedList<Value>;

Value typeof(Value other);

Value add(Value a, Value b);
Value sub(Value a, Value b);
Value mul(Value a, Value b);
Value div(Value a, Value b);

bool operator==(Value a, Value b);

};

#endif /* !VALUE_H_ */
