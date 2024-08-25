#include "gtest/gtest.h"
#include "Scanner.hpp"
#include "tokens.hpp"

static Scanner scanner;

#define S(content) scanner.reconstruct(content); \
	TokenList tokens = scanner.scan()

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
	S("+ * / - \\");

	EXPECT_EQ(tokens.size(), 5);

	EXPECT_EQ(tokens[0], TokenType::Plus);
	EXPECT_EQ(tokens[1], TokenType::Star);
	EXPECT_EQ(tokens[2], TokenType::FSlash);
	EXPECT_EQ(tokens[3], TokenType::Minus);
	EXPECT_EQ(tokens[4], TokenType::BSlash);
}

TEST(ScannerTest, ComparaisonOperator) {
	S("= == ~= != < > <= >=");

	EXPECT_EQ(tokens.size(), 8);

	EXPECT_EQ(tokens[0], TokenType::Eq);
	EXPECT_EQ(tokens[1], TokenType::EqEq);
	EXPECT_EQ(tokens[2], TokenType::Almost);
	EXPECT_EQ(tokens[3], TokenType::BangEq);
	EXPECT_EQ(tokens[4], TokenType::LessThan);
	EXPECT_EQ(tokens[5], TokenType::GreaterThan);
	EXPECT_EQ(tokens[6], TokenType::LessThanOrEq);
	EXPECT_EQ(tokens[7], TokenType::GreaterThanOrEq);
}

TEST(ScannerTest, LogicOperator) {
	S("! && || not and or");

	EXPECT_EQ(tokens.size(), 6);

	EXPECT_EQ(tokens[0], TokenType::Not);
	EXPECT_EQ(tokens[1], TokenType::And);
	EXPECT_EQ(tokens[2], TokenType::Or);
	EXPECT_EQ(tokens[3], TokenType::Not);
	EXPECT_EQ(tokens[4], TokenType::And);
	EXPECT_EQ(tokens[5], TokenType::Or);
}

TEST(ScannerTest, BitwiseOpertor) {
	S("& | ~ << >>");

	EXPECT_EQ(tokens.size(), 5);

	EXPECT_EQ(tokens[0], TokenType::BitwiseAnd);
	EXPECT_EQ(tokens[1], TokenType::BitwiseOr);
	EXPECT_EQ(tokens[2], TokenType::BitwiseNot);
	EXPECT_EQ(tokens[3], TokenType::BitWiseLeftShift);
	EXPECT_EQ(tokens[4], TokenType::BitWiseRightShift);
}

TEST(ScannerTest, IDK) {
	S(". ; : ,");

	EXPECT_EQ(tokens.size(), 4);

	EXPECT_EQ(tokens[0], TokenType::Dot);
	EXPECT_EQ(tokens[1], TokenType::SemiColon);
	EXPECT_EQ(tokens[2], TokenType::Colon);
	EXPECT_EQ(tokens[3], TokenType::Coma);
}

TEST(ScannerTest, LitteralInt) {
	S("05488 123456789");

	EXPECT_EQ(tokens.size(), 2);

	EXPECT_EQ(tokens[0], TokenType::IntLit);
	EXPECT_EQ(tokens[1], TokenType::IntLit);

	EXPECT_EQ(tokens[0].value, "05488");
	EXPECT_EQ(tokens[1].value, "123456789");
}

TEST(ScannerTest, LitteralFloat) {
	S("0.123456789 123456789.123456789f");

	EXPECT_EQ(tokens.size(), 2);	

	EXPECT_EQ(tokens[0], TokenType::FloatLit);
	EXPECT_EQ(tokens[1], TokenType::FloatLit);

	EXPECT_EQ(tokens[0].value, "0.123456789");
	EXPECT_EQ(tokens[1].value, "123456789.123456789f");
}

TEST(ScannerTest, LitteralComplex) {}

TEST(ScannerTest, LitteralString) {
	S("\"Hello World!!!   \" \"dffd\n\"");

	EXPECT_EQ(tokens.size(), 2);

	EXPECT_EQ(tokens[0], TokenType::StringLit);
	EXPECT_EQ(tokens[1], TokenType::StringLit);

	EXPECT_EQ(tokens[0].value, "\"Hello World!!!   \"");
	EXPECT_EQ(tokens[1].value, "\"dffd\n\"");
}

TEST(ScannerTest, LitteralChar) {}

TEST(ScannerTest, Identifier) {
	S("name m_name $func name2");

	EXPECT_EQ(tokens.size(), 4);

	EXPECT_EQ(tokens[0], TokenType::Identifier);
	EXPECT_EQ(tokens[1], TokenType::Identifier);
	EXPECT_EQ(tokens[2], TokenType::Identifier);
	EXPECT_EQ(tokens[3], TokenType::Identifier);

	EXPECT_EQ(tokens[0].value, "name");
	EXPECT_EQ(tokens[1].value, "m_name");
	EXPECT_EQ(tokens[2].value, "$func");
	EXPECT_EQ(tokens[3].value, "name2");
}

TEST(ScannerTest, SpicialIdentifier) {
	S("  $\"this is a special identifier\"");

	EXPECT_EQ(tokens.size(), 1);

	EXPECT_EQ(tokens[0], TokenType::SpicialIdentifier);

	EXPECT_EQ(tokens[0].value, "this is a special identifier");
}



TEST(ScannerTest, KeyWords) {
    // Initialize the macro with the lowercase keywords
    S("!var var func !func return if !if else is for loop class !class try catch");

    // Create the expected token list based on the enum definitions and comments
    EXPECT_EQ(tokens.size(), 15);

    // Check that each token matches the expected token type
    EXPECT_EQ(tokens[0], TokenType::NotVar);  // !var
    EXPECT_EQ(tokens[1], TokenType::Var);
    EXPECT_EQ(tokens[2], TokenType::Func);
    EXPECT_EQ(tokens[3], TokenType::InlineFunc);
    EXPECT_EQ(tokens[4], TokenType::Return);
    EXPECT_EQ(tokens[5], TokenType::If);
    EXPECT_EQ(tokens[6], TokenType::Unless);  // !if
    EXPECT_EQ(tokens[7], TokenType::Else);
    EXPECT_EQ(tokens[8], TokenType::Is);
    EXPECT_EQ(tokens[9], TokenType::For);
    EXPECT_EQ(tokens[10], TokenType::Loop);
    EXPECT_EQ(tokens[11], TokenType::Class);
    EXPECT_EQ(tokens[12], TokenType::FinalClass);  // !class
    EXPECT_EQ(tokens[13], TokenType::Try);
    EXPECT_EQ(tokens[14], TokenType::Catch);
}

