#include "Cocktail/Lexer/TokenizedBuffer.h"

#include <gtest/gtest.h>

#include <iterator>

#include "TokenizedBuffer.t.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/raw_ostream.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;

struct LexerTest : ::testing::Test {
  llvm::SmallVector<SourceBuffer, 16> source_storage;

  auto GetSourceBuffer(llvm::Twine text) -> SourceBuffer& {
    source_storage.push_back(
        std::move(*SourceBuffer::CreateFromText(text.str())));
    return source_storage.back();
  }

  auto Lex(llvm::Twine text) -> TokenizedBuffer {
    return TokenizedBuffer::Lex(GetSourceBuffer(text));
  }
};

TEST_F(LexerTest, HandlesEmptyBuffer) {
  auto buffer = Lex("");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_EQ(buffer.Tokens().begin(), buffer.Tokens().end());
}

TEST_F(LexerTest, TracksLinesAndColumns) {
  auto buffer = Lex("\n  ;;\n   ;;;\n");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Semi(),
                           .line = 2,
                           .column = 3,
                           .indent_column = 3},
                          {.kind = TokenKind::Semi(),
                           .line = 2,
                           .column = 4,
                           .indent_column = 3},
                          {.kind = TokenKind::Semi(),
                           .line = 3,
                           .column = 4,
                           .indent_column = 4},
                          {.kind = TokenKind::Semi(),
                           .line = 3,
                           .column = 5,
                           .indent_column = 4},
                          {.kind = TokenKind::Semi(),
                           .line = 3,
                           .column = 6,
                           .indent_column = 4},
                      }));
}

TEST_F(LexerTest, HandlesIntegerLiteral) {
  auto buffer = Lex("12-578\n  1  2");
  EXPECT_FALSE(buffer.HasErrors());
  ASSERT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::IntegerLiteral(),
                           .line = 1,
                           .column = 1,
                           .indent_column = 1,
                           .text = "12"},
                          {.kind = TokenKind::Minus(),
                           .line = 1,
                           .column = 3,
                           .indent_column = 1},
                          {.kind = TokenKind::IntegerLiteral(),
                           .line = 1,
                           .column = 4,
                           .indent_column = 1,
                           .text = "578"},
                          {.kind = TokenKind::IntegerLiteral(),
                           .line = 2,
                           .column = 3,
                           .indent_column = 3,
                           .text = "1"},
                          {.kind = TokenKind::IntegerLiteral(),
                           .line = 2,
                           .column = 6,
                           .indent_column = 3,
                           .text = "2"},
                      }));
  auto token_12 = buffer.Tokens().begin();
  EXPECT_EQ(buffer.GetIntegerLiteral(*token_12), 12);
  auto token_578 = buffer.Tokens().begin() + 2;
  EXPECT_EQ(buffer.GetIntegerLiteral(*token_578), 578);
  auto token_1 = buffer.Tokens().begin() + 3;
  EXPECT_EQ(buffer.GetIntegerLiteral(*token_1), 1);
  auto token_2 = buffer.Tokens().begin() + 4;
  EXPECT_EQ(buffer.GetIntegerLiteral(*token_2), 2);
}

}  // namespace