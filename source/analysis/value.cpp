#include "value.hpp"
#include "type.hpp"
#include "utils/helper.hpp"

namespace haste {

Value Value::bad(const Bad whats_bad)
{
	return {.variants = whats_bad};
}

Value Value::none()
{
	return {.variants = None{}};
}

Value Value::vtrue(InternPool &pool)
{
	auto type = Type::untyped_boolean(pool);
	return { *type.as<Type*>(), true };
}

Value Value::vfalse(InternPool &pool)
{
	auto type = Type::untyped_boolean(pool);
	return { *type.as<Type*>(), false };
}

Value Value::boolean(InternPool &pool, bool value, std::uint16_t bits)
{
	auto boolean_type = Type::boolean(pool, bits);
	return { *boolean_type.as<Type*>(), value };
}

Value Value::number(InternPool &pool, double value)
{
	auto type = Type::untyped_number(pool, has_decimal(value));
	return { *type.as<Type*>(), value };
}

Value Value::type(Type *type_info)
{
	auto type_type = Type::type();
	return { *type_type.as<Type*>(), type_info };
}

Value Value::typed_number(Type *type_info, const double value)
{
	return {
		.type_info = type_info,
		.variants = value,
	};
}

Value Value::typed_number(Value type, const double value)
{
	assert(type.is<Type*>());
	return {
		.type_info = *type.as<Type*>(),
		.variants = value,
	};
}

Value typeof(Value other)
{
	return Value::type(other.type_info);
}

Value add(Value a, Value b)
{
}

Value sub(Value a, Value b)
{
}

Value mul(Value a, Value b)
{
}

Value div(Value a, Value b)
{
}

};
