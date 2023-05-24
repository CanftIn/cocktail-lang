#include "Cocktail/Lexer/NumericLiteral.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iterator>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace {

using namespace Cocktail;

//struct NumericLiteralTest : ::testing::Test {
//  auto Lex(llvm::StringRef text) -> NumericLiteralToken {
//    auto result = NumericLiteralToken::Lex(text);
//    assert(result && "failed to lex numeric literal");
//    EXPECT_EQ(result->Text(), text);
//    return *result;
//  }
//};

TEST(NumericLiteralTest, LexBasic) {
  auto token = NumericLiteralToken::Lex("1");
  EXPECT_EQ("1", std::string(token->Text()));
  token = NumericLiteralToken::Lex("123_456.78e-9");
  EXPECT_EQ("123_456.78e-9", std::string(token->Text()));
}

}  // namespace
