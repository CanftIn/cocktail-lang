#include "Cocktail/Lexer/TokenizedBuffer.h"

#include <gtest/gtest.h>

#include <iterator>

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
}

}  // namespace