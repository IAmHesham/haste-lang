#include "type.hpp"
#include <cmath>
#include <ranges>
#include <sstream>

namespace haste {

Value Type::type()
{
	static Type tp = {
		.full_name = "type",
		.short_name = "type",
		.info = Type::AType{},
	};
	return {
		.type_info = &tp,
		.variants = &tp,
	};
}

Value Type::untyped_boolean(InternPool &pool)
{
	const UntypedBoolean info = {};
	const StringView full_name = "untyped_bool";
	const Type type = {
		.full_name = full_name,
		.short_name = full_name,
		.info = info,
	};
	const auto result_type = intern(pool, type);
	return Value::type(result_type);
}

Value Type::untyped_number(InternPool &pool, const bool is_floating)
{
	const UntypedNumber info = {
		.is_floating = is_floating,
	};
	const StringView full_name = is_floating ? "untyped_float" : "untyped_int";
	const Type type = {
		.full_name = full_name,
		.short_name = full_name,
		.info = info,
	};
	const auto result_type = intern(pool, type);
	return Value::type(result_type);
}

Value Type::integer(InternPool &pool, std::uint16_t bits)
{
	if (bits == 0 or bits > 128) {
		return Value::bad(Bad::InvalidBitWidth);
	}

	std::stringstream name_builder;
	name_builder << "int" << bits;

	const auto full_name = intern(pool, name_builder.str());
	const auto type_info = Type {
		.full_name = full_name,
		.short_name = full_name,
		.info = Type::Int {
			.is_signed = true,
			.bits = bits,
		},
	};

	return Value::type(intern(pool, type_info));
}

Value Type::uinteger(InternPool &pool, const std::uint16_t bits)
{
	if (bits == 0 or bits > 128) {
		return Value::bad(Bad::InvalidBitWidth);
	}

	std::stringstream name_builder;
	name_builder << "uint" << bits;

	const auto full_name = intern(pool, name_builder.str());
	const auto type_info = Type {
		.full_name = full_name,
		.short_name = full_name,
		.info = Type::Int {
			.is_signed = false,
			.bits = bits,
		},
	};

	return Value::type(intern(pool, type_info));
}

Value Type::floating(InternPool &pool, std::uint16_t bits)
{
	if (bits != 32 and bits != 64 and bits != 128) {
		return Value::bad(Bad::InvalidBitWidth);
	}

	std::stringstream name_builder;
	name_builder << "float" << bits;

	const auto full_name = intern(pool, name_builder.str());
	const auto type_info = Type {
		.full_name = full_name,
		.short_name = full_name,
		.info = Type::Float {
			.bits = bits,
		},
	};

	return Value::type(intern(pool, type_info));
}

Value Type::boolean(InternPool &pool, std::uint16_t bits)
{
	if (bits == 0 or bits > 128) {
		return Value::bad(Bad::InvalidBitWidth);
	}

	std::stringstream name_builder;
	name_builder << "bool" << bits;

	const auto full_name = intern(pool, name_builder.str());
	const auto type_info = Type {
		.full_name = full_name,
		.short_name = full_name,
		.info = Type::Boolean {
			.bits = bits,
		},
	};

	return Value::type(intern(pool, type_info));
}

bool Type::is_number() const
{
	return is<UntypedNumber>() or is<UntypedBoolean>();
}

std::size_t Type::has_method(StringView name) const
{
	Overloads *overloads = get(methods, name);
	if (overloads == nullptr) {
		return 0;
	}
	return overloads->size;
}

Overloads *Type::get_methods(StringView name)
{
	return get(methods, name);
}

Value *Type::get_method(StringView name, std::initializer_list<Type*> args)
{
	const auto overloads = get_methods(name);
	if (overloads == nullptr) return nullptr;

	for (auto overload : *overloads) {
		const auto fn = overload.as<Type*>();
		assert(fn != nullptr);

		const auto type_info = *fn;
		assert(type_info->is<Type::Function>());

		const auto function = type_info->as<Type::Function>();
		const auto argument_list = function->arguments;

		if (args.size() != argument_list.size) {
			continue;
		}

		for (auto i : std::views::iota(argument_list.size)) {
			const auto expected = args[i];
		}
	}

	return nullptr;
}

Value *Type::get_method(StringView name, LinkedList<Type*> args)
{
}

Value *Type::get_method(StringView name, Type *return_type)
{
}

std::size_t gategory(const Type *tp)
{
	return tp->info.index();
}

Value coerce_number_to_boolean(InternPool &pool, Value value, Type *target)
{
	(void)pool;
	auto *num = value.as<double>();
	if (num == nullptr) return Value::bad(Bad::Impossible);
	if (*num != 0.0 and *num != 1.0) return Value::bad(Impossible);
	return {
		.type_info = const_cast<Type*>(target),
		.variants = *num != 0.0,
	};
}

Value coerce_boolean_to_number(InternPool &pool, Value value, Type *target)
{
	(void)pool;
	auto *boolean = value.as<bool>();
	if (boolean == nullptr) return Value::bad(Bad::Impossible);
	return {
		.type_info = const_cast<Type*>(target),
		.variants = *boolean ? 1.0 : 0.0,
	};
}

Value coerce_number_to_int(InternPool &pool, Value value, Type *target, ConversionRule::Kind kind)
{
	(void)pool;
	const Type::UntypedNumber ti = *(*typeof(value).as<Type*>())->as<Type::UntypedNumber>();
	if (ti.is_floating and kind != ConversionRule::Kind::Cast) {
		return Value::bad(Bad::ImplicitCoercion);
	}

	const double integer = *value.as<double>();
	return Value::typed_number(target, std::round(integer));
}

Value coerce_number_to_float(InternPool &pool, Value value, Type *target)
{
	(void)pool;
	const double val = *value.as<double>();
	return Value::typed_number(target, val);
}

bool coercible(const Type *from, const Type *to)
{
	if (from == to) return true;
	const auto &rule = CONVERSION_TABLE[gategory(from)][gategory(to)];
	return rule.kind == ConversionRule::Kind::Coercion;
}

bool castable(const Type *from, const Type *to)
{
	if (from == to) return true;
	const auto &rule = CONVERSION_TABLE[gategory(from)][gategory(to)];
	return rule.kind != ConversionRule::Kind::None;
}

Value coerce(InternPool &pool, Value val, Type *target)
{
	if (val.type_info == target) return val;
	if (val.is<Bad>()) return val;

	const auto &rule = CONVERSION_TABLE[gategory(val.type_info)][gategory(target)];

	switch (rule.kind) {
	case ConversionRule::Kind::None:
		return Value::bad(Bad::Impossible);
	case ConversionRule::Kind::Cast:
		return Value::bad(Bad::ImplicitCoercion);
	case ConversionRule::Kind::Coercion:
		if (rule.convert == nullptr) {
			return {
				.type_info = const_cast<Type*>(target),
				.variants = val.variants,
			};
		}
		return rule.convert(pool, val, target);
	}
}

Value cast(InternPool &pool, Value val, Type *target)
{
	if (val.type_info == target) return val;
	if (val.is<Bad>()) return val;

	const auto &rule = CONVERSION_TABLE[gategory(val.type_info)][gategory(target)];

	switch (rule.kind) {
	case ConversionRule::Kind::None:
		return Value::bad(Bad::Impossible);
	case ConversionRule::Kind::Coercion:
	case ConversionRule::Kind::Cast:
		if (rule.convert == nullptr) {
			return {.type_info = const_cast<Type*>(target), .variants = val.variants};
		}
		return rule.convert(pool, val, target);
	}
}

};

std::size_t std::hash<haste::Type>::operator()(const haste::Type &tp) const
{
	return std::hash<StringView>()(tp.full_name);
}
