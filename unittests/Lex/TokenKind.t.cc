#include "Cocktail/Lex/TokenKind.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

#include "llvm/ADT/StringRef.h"

namespace Cocktail::Lex {
namespace {

using ::testing::MatchesRegex;

constexpr llvm::StringLiteral SymbolRegex =
    R"([][{}!@#%^&*()/?\\|;:.,<>=+~-]+)";

constexpr llvm::StringLiteral KeywordRegex = "[a-z_]+|Self|String";

#define COCKTAIL_TOKEN(TokenName)                         \
  TEST(TokenKindTest, TokenName) {                        \
    EXPECT_FALSE(TokenKind::TokenName.is_symbol());       \
    EXPECT_FALSE(TokenKind::TokenName.is_keyword());      \
    EXPECT_EQ("", TokenKind::TokenName.fixed_spelling()); \
  }
#define COCKTAIL_SYMBOL_TOKEN(TokenName, Spelling)              \
  TEST(TokenKindTest, TokenName) {                              \
    EXPECT_TRUE(TokenKind::TokenName.is_symbol());              \
    EXPECT_FALSE(TokenKind::TokenName.is_grouping_symbol());    \
    EXPECT_FALSE(TokenKind::TokenName.is_opening_symbol());     \
    EXPECT_FALSE(TokenKind::TokenName.is_closing_symbol());     \
    EXPECT_FALSE(TokenKind::TokenName.is_keyword());            \
    EXPECT_EQ(Spelling, TokenKind::TokenName.fixed_spelling()); \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));     \
  }
#define COCKTAIL_OPENING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, ClosingName) \
  TEST(TokenKindTest, TokenName) {                                            \
    EXPECT_TRUE(TokenKind::TokenName.is_symbol());                            \
    EXPECT_TRUE(TokenKind::TokenName.is_grouping_symbol());                   \
    EXPECT_TRUE(TokenKind::TokenName.is_opening_symbol());                    \
    EXPECT_EQ(TokenKind::ClosingName, TokenKind::TokenName.closing_symbol()); \
    EXPECT_FALSE(TokenKind::TokenName.is_closing_symbol());                   \
    EXPECT_FALSE(TokenKind::TokenName.is_keyword());                          \
    EXPECT_EQ(Spelling, TokenKind::TokenName.fixed_spelling());               \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));                   \
  }
#define COCKTAIL_CLOSING_GROUP_SYMBOL_TOKEN(TokenName, Spelling, OpeningName) \
  TEST(TokenKindTest, TokenName) {                                            \
    EXPECT_TRUE(TokenKind::TokenName.is_symbol());                            \
    EXPECT_TRUE(TokenKind::TokenName.is_grouping_symbol());                   \
    EXPECT_FALSE(TokenKind::TokenName.is_opening_symbol());                   \
    EXPECT_TRUE(TokenKind::TokenName.is_closing_symbol());                    \
    EXPECT_EQ(TokenKind::OpeningName, TokenKind::TokenName.opening_symbol()); \
    EXPECT_FALSE(TokenKind::TokenName.is_keyword());                          \
    EXPECT_EQ(Spelling, TokenKind::TokenName.fixed_spelling());               \
    EXPECT_THAT(Spelling, MatchesRegex(SymbolRegex.str()));                   \
  }
#define COCKTAIL_KEYWORD_TOKEN(TokenName, Spelling)             \
  TEST(TokenKindTest, TokenName) {                              \
    EXPECT_FALSE(TokenKind::TokenName.is_symbol());             \
    EXPECT_TRUE(TokenKind::TokenName.is_keyword());             \
    EXPECT_EQ(Spelling, TokenKind::TokenName.fixed_spelling()); \
    EXPECT_THAT(Spelling, MatchesRegex(KeywordRegex.str()));    \
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
}  // namespace Cocktail::Lex