#include "ast.hpp"
#include <ostream>

namespace haste {

namespace ast {

// -- Indent helper -------------------------------------------------------

static void do_indent(std::ostream &os, int level)
{
	for (int i = 0; i < level; i++) {
		os << "  ";
	}
}

// -- Node (variant dispatcher) -------------------------------------------

void print(const Node &node, std::ostream &os, int indent)
{
	node.visit([&](const auto &data) {
		print(data, os, indent);
	});
}

// -- Leaf nodes ----------------------------------------------------------

void print(const Auto &, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "Auto";
}

void print(const Int &, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "Int";
}

void print(const Float &, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "Float";
}

void print(const IntLit &lit, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "IntLit(" << lit.value << ')';
}

void print(const FloatLit &lit, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "FloatLit(" << lit.value << ')';
}

void print(const IdentExpr &e, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "IdentExpr(\"" << e.name << "\")";
}

// -- Binary expression nodes ----------------------------------------------

static void print_binary(std::ostream &os, const char *name,
                         const Node *left, const Node *right, int indent)
{
	do_indent(os, indent);
	os << name << '\n';
	print(*left,  os, indent + 1);
	os << '\n';
	print(*right, os, indent + 1);
}

void print(const AdditionExpr &e, std::ostream &os, int indent)
{
	print_binary(os, "AdditionExpr", e.left, e.right, indent);
}

void print(const SubtractionExpr &e, std::ostream &os, int indent)
{
	print_binary(os, "SubtractionExpr", e.left, e.right, indent);
}

void print(const MultiplicationExpr &e, std::ostream &os, int indent)
{
	print_binary(os, "MultiplicationExpr", e.left, e.right, indent);
}

void print(const DivisionExpr &e, std::ostream &os, int indent)
{
	print_binary(os, "DivisionExpr", e.left, e.right, indent);
}

// -- Unary expression nodes -----------------------------------------------

void print(const NegateExpr &e, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "NegateExpr\n";
	print(*e.operand, os, indent + 1);
}

// -- Declaration nodes ----------------------------------------------------

void print(const VarDecl &d, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "VarDecl(\"" << d.name << "\", " << d.is_cons << ")\n";
	if (d.type_node and d.init) {
		print(*d.type_node, os, indent + 1);
		os << "\n";
		print(*d.init, os, indent + 1);
	} else if (d.type_node) {
		print(*d.type_node, os, indent + 1);
	} else if (d.init) {
		print(*d.init, os, indent + 1);
	}
}

void print(const Program &p, std::ostream &os, int indent)
{
	do_indent(os, indent);
	os << "Program";
	for (auto &node : p.declarations) {
		os << '\n';
		print(*node, os, indent + 1);
	}
}

}; // namespace ast

Location as_location(const ast::Node *node)
{
	Location result{};
	node->visit([&](const auto &data) { result = data.location; });
	return result;
}

Type *typeof(const ast::Node *node)
{
	Type *result = nullptr;
	node->visit([&](const auto &data) { result = data.type; });
	return result;
}

}; // namespace haste
