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

TEST(NumericLiteralTest, LexDecimalInteger) {
  auto token = NumericLiteralToken::Lex("123");
  EXPECT_EQ("123", std::string(token->Text()));
}

}  // namespace
