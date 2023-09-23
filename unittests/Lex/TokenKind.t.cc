#include "Cocktail/Lex/TokenKind.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {
namespace {

using ::testing::MatchesRegex;

constexpr llvm::StringLiteral SymbolRegex = "[][{}!@#%^&*()/?\\|;:.,<>=+~-]+";

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
#include "Cocktail/Lex/TokenKind.def"

TEST(TokenKindTest, SymbolsInDescendingLength) {
  int previous_length = INT_MAX;
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling)                      \
  EXPECT_LE(llvm::StringRef(Spelling).size(), previous_length)          \
      << "Symbol token not in descending length order: " << #TokenName; \
  previous_length = llvm::StringRef(Spelling).size();
#include "Cocktail/Lex/TokenKind.def"
  EXPECT_GT(previous_length, 0);
}

}  // namespace
}  // namespace Cocktail