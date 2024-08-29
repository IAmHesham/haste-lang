#include "gtest/gtest.h"
#include <cstddef>
#include "Scanner.hpp"
#include "tokens.hpp"

static Scanner scanner;

#define S(content) scanner.reconstruct(content); \
	std::size_t _i = 0; \
	TokenList tokens = scanner.scan()
#define EXPECT_TOKEN(token) EXPECT_EQ(tokens[_i++], TokenType::token)

TEST(ScannerTest, Parenthesis) {
	S("}{ ][ )(");

	EXPECT_EQ(tokens.size(), 6);

	EXPECT_EQ(tokens[0], TokenType::CloseCurlyBrase);
	EXPECT_EQ(tokens[1], TokenType::OpenCurlyBrase);
	EXPECT_EQ(tokens[2], TokenType::CloseSquareBracket);
	EXPECT_EQ(tokens[3], TokenType::OpenSquareBracket);
	EXPECT_EQ(tokens[4], TokenType::CloseParen);
	EXPECT_EQ(tokens[5], TokenType::OpenParen);
}

TEST(ScannerTest, MathematicalOperators) {
	S("+ * ** / - \\");

	EXPECT_EQ(tokens.size(), 6);

	EXPECT_TOKEN(Plus);
	EXPECT_TOKEN(Star);
	EXPECT_TOKEN(DoubleStar);
	EXPECT_TOKEN(FSlash);
	EXPECT_TOKEN(Minus);
	EXPECT_TOKEN(BSlash);
}

TEST(ScannerTest, ComparaisonOperator) {
	S("= == ~= != < > <= >=");

	EXPECT_EQ(tokens.size(), 8);

	EXPECT_TOKEN(Eq);
	EXPECT_TOKEN(EqEq);
	EXPECT_TOKEN(Almost);
	EXPECT_TOKEN(BangEq);
	EXPECT_TOKEN(LessThan);
	EXPECT_TOKEN(GreaterThan);
	EXPECT_TOKEN(LessThanOrEq);
	EXPECT_TOKEN(GreaterThanOrEq);
}

TEST(ScannerTest, LogicOperator) {
	S("! && || not and or");

	EXPECT_EQ(tokens.size(), 6);

	EXPECT_TOKEN(Not);
	EXPECT_TOKEN(And);
	EXPECT_TOKEN(Or);
	EXPECT_TOKEN(Not);
	EXPECT_TOKEN(And);
	EXPECT_TOKEN(Or);
}

TEST(ScannerTest, BitwiseOpertor) {
	S("& | ~ << >>");

	EXPECT_EQ(tokens.size(), 5);

	EXPECT_TOKEN(BitwiseAnd);
	EXPECT_TOKEN(BitwiseOr);
	EXPECT_TOKEN(BitwiseNot);
	EXPECT_TOKEN(BitWiseLeftShift);
	EXPECT_TOKEN(BitWiseRightShift);
}

TEST(ScannerTest, IDK) {
	S(". ; : ,");

	EXPECT_EQ(tokens.size(), 4);

	EXPECT_TOKEN(Dot);
	EXPECT_TOKEN(SemiColon);
	EXPECT_TOKEN(Colon);
	EXPECT_TOKEN(Coma);
}

TEST(ScannerTest, LitteralInt) {
	S("05488 123456789");

	EXPECT_EQ(tokens.size(), 2);

	EXPECT_TOKEN(IntLit);
	EXPECT_TOKEN(IntLit);

	EXPECT_EQ(tokens[0].value, "05488");
	EXPECT_EQ(tokens[1].value, "123456789");
}

TEST(ScannerTest, LitteralFloat) {
	S("0.123456789 123456789.123456789f");

	EXPECT_EQ(tokens.size(), 2);	

	EXPECT_TOKEN(FloatLit);
	EXPECT_TOKEN(FloatLit);

	EXPECT_EQ(tokens[0].value, "0.123456789");
	EXPECT_EQ(tokens[1].value, "123456789.123456789f");
}

TEST(ScannerTest, LitteralComplex) {}

TEST(ScannerTest, LitteralString) {
	S("\"Hello World!!!   \" \"dffd\n\"");

	EXPECT_EQ(tokens.size(), 2);

	EXPECT_TOKEN(StringLit);
	EXPECT_TOKEN(StringLit);

	EXPECT_EQ(tokens[0].value, "\"Hello World!!!   \"");
	EXPECT_EQ(tokens[1].value, "\"dffd\n\"");
}

TEST(ScannerTest, LitteralChar) {}

TEST(ScannerTest, Identifier) {
	S("name m_name $func name2");

	EXPECT_EQ(tokens.size(), 4);

	EXPECT_TOKEN(Identifier);
	EXPECT_TOKEN(Identifier);
	EXPECT_TOKEN(Identifier);
	EXPECT_TOKEN(Identifier);

	EXPECT_EQ(tokens[0].value, "name");
	EXPECT_EQ(tokens[1].value, "m_name");
	EXPECT_EQ(tokens[2].value, "$func");
	EXPECT_EQ(tokens[3].value, "name2");
}

TEST(ScannerTest, SpicialIdentifier) {
	S("  $\"this is a special identifier\"");

	EXPECT_EQ(tokens.size(), 1);

	EXPECT_TOKEN(SpicialIdentifier);

	EXPECT_EQ(tokens[0].value, "this is a special identifier");
}



TEST(ScannerTest, KeyWords) {
    // Initialize the macro with the lowercase keywords
    S("!var var func !func return if !if else is for loop class !class try catch ptr");

    // Create the expected token list based on the enum definitions and comments
    EXPECT_EQ(tokens.size(), 16);

    // Check that each token matches the expected token type
    EXPECT_TOKEN(NotVar);  // !var
    EXPECT_TOKEN(Var);
    EXPECT_TOKEN(Func);
    EXPECT_TOKEN(InlineFunc);
    EXPECT_TOKEN(Return);
    EXPECT_TOKEN(If);
    EXPECT_TOKEN(Unless);  // !if
    EXPECT_TOKEN(Else);
    EXPECT_TOKEN(Is);
    EXPECT_TOKEN(For);
    EXPECT_TOKEN(Loop);
    EXPECT_TOKEN(Class);
    EXPECT_TOKEN(FinalClass);  // !class
    EXPECT_TOKEN(Try);
    EXPECT_TOKEN(Catch);
	EXPECT_TOKEN(Ptr);
}

