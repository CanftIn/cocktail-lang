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
  auto GetSourceBuffer(llvm::Twine text) -> SourceBuffer& {
    auto buffer_expected = SourceBuffer::CreateFromText(text.str());
    if (auto err = buffer_expected.takeError()) {
      llvm::errs() << "Error: " << err << "\n";
    }
    return *buffer_expected;
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

}  // namespace