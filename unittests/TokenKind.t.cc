#include "Cocktail/Lexer/TokenKind.h"

#include <cstring>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm/ADT/StringRef.h"

namespace {

using namespace Cocktail;
using ::testing::MatchesRegex;

// We restrict symbols to punctuation characters that are expected to be widely
// available on modern keyboards used for programming.
constexpr llvm::StringLiteral SymbolRegex = "[][{}!@#%^&*()/?\\|;:.,<>=+~-]+";

// We restrict keywords to be lowercase ASCII letters and underscores.
constexpr llvm::StringLiteral KeywordRegex = "[a-z_]+";

#define COCKTAIL_TOKEN(TokenName)                             \
  TEST(TokenKindTest, TokenName) {                            \
    EXPECT_EQ(#TokenName, TokenKind::TokenName().Name());     \
    EXPECT_FALSE(TokenKind::TokenName().IsSymbol());          \
    EXPECT_FALSE(TokenKind::TokenName().IsKeyword());         \
    EXPECT_EQ("", TokenKind::TokenName().GetFixedSpelling()); \
  }
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling)                  \
  TEST(TokenKindTest, TokenName) {                                  \
    EXPECT_EQ(#TokenName, TokenKind::TokenName().Name());           \
    EXPECT_TRUE(TokenKind::TokenName().IsSymbol());                 \
    EXPECT_FALSE(TokenKind::TokenName().IsGroupingSymbol());        \
    EXPECT_FALSE(TokenKind::TokenName().IsOpeningSymbol());         \
    EXPECT_FALSE(TokenKind::TokenName().IsClosingSymbol());         \
    EXPECT_FALSE(TokenKind::TokenName().IsKeyword());               \
    EXPECT_EQ(Spelling, TokenKind::TokenName().GetFixedSpelling()); \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));         \
  }
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  TEST(TokenKindTest, TokenName) {                                            \
    EXPECT_EQ(#TokenName, TokenKind::TokenName().Name());                     \
    EXPECT_TRUE(TokenKind::TokenName().IsSymbol());                           \
    EXPECT_TRUE(TokenKind::TokenName().IsGroupingSymbol());                   \
    EXPECT_TRUE(TokenKind::TokenName().IsOpeningSymbol());                    \
    EXPECT_EQ(TokenKind::ClosingName(),                                       \
              TokenKind::TokenName().GetClosingSymbol());                     \
    EXPECT_FALSE(TokenKind::TokenName().IsClosingSymbol());                   \
    EXPECT_FALSE(TokenKind::TokenName().IsKeyword());                         \
    EXPECT_EQ(Spelling, TokenKind::TokenName().GetFixedSpelling());           \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));                   \
  }
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  TEST(TokenKindTest, TokenName) {                                            \
    EXPECT_EQ(#TokenName, TokenKind::TokenName().Name());                     \
    EXPECT_TRUE(TokenKind::TokenName().IsSymbol());                           \
    EXPECT_TRUE(TokenKind::TokenName().IsGroupingSymbol());                   \
    EXPECT_FALSE(TokenKind::TokenName().IsOpeningSymbol());                   \
    EXPECT_TRUE(TokenKind::TokenName().IsClosingSymbol());                    \
    EXPECT_EQ(TokenKind::OpeningName(),                                       \
              TokenKind::TokenName().GetOpeningSymbol());                     \
    EXPECT_FALSE(TokenKind::TokenName().IsKeyword());                         \
    EXPECT_EQ(Spelling, TokenKind::TokenName().GetFixedSpelling());           \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));                   \
  }
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling)                 \
  TEST(TokenKindTest, TokenName) {                                  \
    EXPECT_EQ(#TokenName, TokenKind::TokenName().Name());           \
    EXPECT_FALSE(TokenKind::TokenName().IsSymbol());                \
    EXPECT_TRUE(TokenKind::TokenName().IsKeyword());                \
    EXPECT_EQ(Spelling, TokenKind::TokenName().GetFixedSpelling()); \
    EXPECT_THAT(Spelling, MatchesRegex(KeywordRegex.str()));        \
  }
#include "Cocktail/Lexer/TokenRegistry.def"

// Verify that the symbol tokens are sorted from longest to shortest. This is
// important to ensure that simply in-order testing will identify tokens
// following the max-munch rule.
TEST(TokenKindTest, SymbolsInDescendingLength) {
  int previous_length = INT_MAX;
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling)                      \
  EXPECT_LE(llvm::StringRef(Spelling).size(), previous_length)          \
      << "Symbol token not in descending length order: " << #TokenName; \
  previous_length = llvm::StringRef(Spelling).size();
#include "Cocktail/Lexer/TokenRegistry.def"
  EXPECT_GT(previous_length, 0);
}

}  // namespace
