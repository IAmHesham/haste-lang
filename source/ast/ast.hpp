#ifndef AST_H_
#define AST_H_

#include "analysis/value.hpp"
#include "containers/linked_list.hpp"
#include "containers/just_enough_variant.hpp"
#include "utils/location.hpp"
#include <ostream>

namespace haste {

namespace ast {

struct Auto;
struct Int;
struct Float;
struct IntLit;
struct FloatLit;
struct IdentExpr;

struct AdditionExpr;
struct SubtractionExpr;
struct MultiplicationExpr;
struct DivisionExpr;
struct NegateExpr;
struct ConstDecl;
struct VarDecl;
struct Program;

// -- Node variant --------------------------------------------------------

using Node = JustEnoughVariant<
	Auto, Int, Float,
	IntLit, FloatLit, IdentExpr,
	AdditionExpr, SubtractionExpr, MultiplicationExpr, DivisionExpr,
	NegateExpr,
	VarDecl,
	Program
>;

struct Auto {
	Location location;
	Type *type = nullptr;
};

struct Int {
	Location location;
	Type *type = nullptr;
};

struct Float {
	Location location;
	Type *type = nullptr;
};

struct IntLit   {
	Location location;
	Type *type = nullptr;
	std::uint64_t value;
};

struct FloatLit {
	Location location;
	Type *type = nullptr;
	double value;
};

struct IdentExpr {
	Location location;
	Type *type = nullptr;
	StringView name;
};

struct AdditionExpr {
	Location location;
	Type *type = nullptr;
	Location op_location;
	Node *left;
	Node *right;
};

struct SubtractionExpr {
	Location location;
	Type *type = nullptr;
	Location op_location;
	Node *left;
	Node *right;
};

struct MultiplicationExpr {
	Location location;
	Type *type = nullptr;
	Location op_location;
	Node *left;
	Node *right;
};

struct DivisionExpr {
	Location location;
	Type *type = nullptr;
	Location op_location;
	Node *left;
	Node *right;
};

struct NegateExpr {
	Location location;
	Type *type = nullptr;
	Location op_location;
	Node *operand;
};

struct VarDecl {
	Location location;
	Type *type = nullptr;
	StringView name;
	Node *type_node;
	Node *init;
	bool is_cons = false;
};

struct Program {
	Location location;
	Type *type = nullptr;
	LinkedList<Node*> declarations;
};

// -- Pretty print -------------------------------------------------------

void print(const Node &node, std::ostream &os, int indent = 0);

void print(const Auto &a,               std::ostream &os, int indent = 0);
void print(const Int &i,                std::ostream &os, int indent = 0);
void print(const Float &f,              std::ostream &os, int indent = 0);
void print(const IntLit &lit,           std::ostream &os, int indent = 0);
void print(const FloatLit &lit,         std::ostream &os, int indent = 0);
void print(const IdentExpr &e,          std::ostream &os, int indent = 0);

void print(const AdditionExpr &e,       std::ostream &os, int indent = 0);
void print(const SubtractionExpr &e,    std::ostream &os, int indent = 0);
void print(const MultiplicationExpr &e, std::ostream &os, int indent = 0);
void print(const DivisionExpr &e,       std::ostream &os, int indent = 0);
void print(const NegateExpr &e,         std::ostream &os, int indent = 0);
void print(const VarDecl &d,            std::ostream &os, int indent = 0);
void print(const Program &p,            std::ostream &os, int indent = 0);

template <typename T>
auto operator<<(std::ostream &os, const T &node)
	-> decltype(print(node, os, 0), os)
{
	print(node, os, 0);
	return os;
}


} // namespace ast

// -- Helpers  ----------------------------------------------------------

Location as_location(const ast::Node *node);
Type *typeof(const ast::Node *node);

} // namespace haste

#endif
