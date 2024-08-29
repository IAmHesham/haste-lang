#pragma once

#include <memory>
#include <string>
#include <variant>

namespace AST {

struct IntExpression;
struct FloatExpression;
struct StringExpression;
struct IdentifierExpression;
struct UnaryExpression;
struct BinaryExpression;

using Expression = std::variant<IntExpression, FloatExpression, UnaryExpression, BinaryExpression, StringExpression, IdentifierExpression>;
using ExpressionHandle = std::unique_ptr<Expression>;

enum class Operator {
	Addition,
	Subtraction,
	Multiplication,
	Division,
	Power
};

struct IntExpression {
	long long int value;
};

struct FloatExpression {
	long double value;
};

struct StringExpression {
	std::string value;
};

struct IdentifierExpression {
	std::string value;
};

struct UnaryExpression {
	Operator op;
	ExpressionHandle rhs;
};

struct BinaryExpression {
	ExpressionHandle lhs;
	Operator op;
	ExpressionHandle rhs;
};

}
