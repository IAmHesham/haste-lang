#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include "Expressions.hpp"

namespace AST {
struct VarStatment;
struct FuncStatment;

using Statment = std::variant<VarStatment, FuncStatment>;
using StatmentHandle = std::unique_ptr<Statment>;

struct ASTMain {
	std::vector<VarStatment> globalVariables;
	std::vector<FuncStatment> globalFunctions;
};

struct VarStatment {
	std::string identifier;
	std::optional<ExpressionHandle> value;
	bool constant = false;
};


struct FuncStatment {};

}
