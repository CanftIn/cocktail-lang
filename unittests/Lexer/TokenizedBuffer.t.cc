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

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::StrEq;

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

TEST_F(LexerTest, Symbols) {
  auto buffer = Lex("<<<");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::LessLess()},
                          {TokenKind::Less()},
                      }));

  buffer = Lex("<<=>>");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::LessLessEqual()},
                          {TokenKind::GreaterGreater()},
                      }));

  buffer = Lex("< <=> >");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::Less()},
                          {TokenKind::LessEqualGreater()},
                          {TokenKind::Greater()},
                      }));

  buffer = Lex("\\/?#@&^!");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::Backslash()},
                          {TokenKind::Slash()},
                          {TokenKind::Question()},
                          {TokenKind::Hash()},
                          {TokenKind::At()},
                          {TokenKind::Amp()},
                          {TokenKind::Caret()},
                          {TokenKind::Exclaim()},
                      }));
}

TEST_F(LexerTest, Parens) {
  auto buffer = Lex("()");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::OpenParen()},
                          {TokenKind::CloseParen()},
                      }));

  buffer = Lex("((()()))");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::OpenParen()},
                          {TokenKind::OpenParen()},
                          {TokenKind::OpenParen()},
                          {TokenKind::CloseParen()},
                          {TokenKind::OpenParen()},
                          {TokenKind::CloseParen()},
                          {TokenKind::CloseParen()},
                          {TokenKind::CloseParen()},
                      }));
}

TEST_F(LexerTest, CurlyBraces) {
  auto buffer = Lex("{}");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::OpenCurlyBrace()},
                          {TokenKind::CloseCurlyBrace()},
                      }));

  buffer = Lex("{{{}{}}}");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::OpenCurlyBrace()},
                          {TokenKind::OpenCurlyBrace()},
                          {TokenKind::OpenCurlyBrace()},
                          {TokenKind::CloseCurlyBrace()},
                          {TokenKind::OpenCurlyBrace()},
                          {TokenKind::CloseCurlyBrace()},
                          {TokenKind::CloseCurlyBrace()},
                          {TokenKind::CloseCurlyBrace()},
                      }));
}

TEST_F(LexerTest, MatchingGroups) {
  {
    TokenizedBuffer buffer = Lex("(){}");
    ASSERT_FALSE(buffer.HasErrors());
    auto it = buffer.Tokens().begin();
    auto open_paren_token = *it++;
    auto close_paren_token = *it++;
    EXPECT_EQ(close_paren_token,
              buffer.GetMatchedClosingToken(open_paren_token));
    EXPECT_EQ(open_paren_token,
              buffer.GetMatchedOpeningToken(close_paren_token));
    auto open_curly_token = *it++;
    auto close_curly_token = *it++;
    EXPECT_EQ(close_curly_token,
              buffer.GetMatchedClosingToken(open_curly_token));
    EXPECT_EQ(open_curly_token,
              buffer.GetMatchedOpeningToken(close_curly_token));
    EXPECT_EQ(buffer.Tokens().end(), it);
  }

  {
    TokenizedBuffer buffer = Lex("({x}){(y)} {{((z))}}");
    ASSERT_FALSE(buffer.HasErrors());
    auto it = buffer.Tokens().begin();
    auto open_paren_token = *it++;
    auto open_curly_token = *it++;
    ASSERT_EQ("x", buffer.GetIdentifierText(buffer.GetIdentifier(*it++)));
    auto close_curly_token = *it++;
    auto close_paren_token = *it++;
    EXPECT_EQ(close_paren_token,
              buffer.GetMatchedClosingToken(open_paren_token));
    EXPECT_EQ(open_paren_token,
              buffer.GetMatchedOpeningToken(close_paren_token));
    EXPECT_EQ(close_curly_token,
              buffer.GetMatchedClosingToken(open_curly_token));
    EXPECT_EQ(open_curly_token,
              buffer.GetMatchedOpeningToken(close_curly_token));

    open_curly_token = *it++;
    open_paren_token = *it++;
    ASSERT_EQ("y", buffer.GetIdentifierText(buffer.GetIdentifier(*it++)));
    close_paren_token = *it++;
    close_curly_token = *it++;
    EXPECT_EQ(close_curly_token,
              buffer.GetMatchedClosingToken(open_curly_token));
    EXPECT_EQ(open_curly_token,
              buffer.GetMatchedOpeningToken(close_curly_token));
    EXPECT_EQ(close_paren_token,
              buffer.GetMatchedClosingToken(open_paren_token));
    EXPECT_EQ(open_paren_token,
              buffer.GetMatchedOpeningToken(close_paren_token));

    open_curly_token = *it++;
    auto inner_open_curly_token = *it++;
    open_paren_token = *it++;
    auto inner_open_paren_token = *it++;
    ASSERT_EQ("z", buffer.GetIdentifierText(buffer.GetIdentifier(*it++)));
    auto inner_close_paren_token = *it++;
    close_paren_token = *it++;
    auto inner_close_curly_token = *it++;
    close_curly_token = *it++;
    EXPECT_EQ(close_curly_token,
              buffer.GetMatchedClosingToken(open_curly_token));
    EXPECT_EQ(open_curly_token,
              buffer.GetMatchedOpeningToken(close_curly_token));
    EXPECT_EQ(inner_close_curly_token,
              buffer.GetMatchedClosingToken(inner_open_curly_token));
    EXPECT_EQ(inner_open_curly_token,
              buffer.GetMatchedOpeningToken(inner_close_curly_token));
    EXPECT_EQ(close_paren_token,
              buffer.GetMatchedClosingToken(open_paren_token));
    EXPECT_EQ(open_paren_token,
              buffer.GetMatchedOpeningToken(close_paren_token));
    EXPECT_EQ(inner_close_paren_token,
              buffer.GetMatchedClosingToken(inner_open_paren_token));
    EXPECT_EQ(inner_open_paren_token,
              buffer.GetMatchedOpeningToken(inner_close_paren_token));

    EXPECT_EQ(buffer.Tokens().end(), it);
  }
}

TEST_F(LexerTest, MismatchedGroups) {
  auto buffer = Lex("{");
  EXPECT_TRUE(buffer.HasErrors());
  EXPECT_THAT(buffer,
              HasTokens(llvm::ArrayRef<ExpectedToken>{
                  {TokenKind::OpenCurlyBrace()},
                  {.kind = TokenKind::CloseCurlyBrace(), .recovery = true},
              }));

  buffer = Lex("}");
  EXPECT_TRUE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Error(), .text = "}"},
                      }));

  buffer = Lex("{(}");
  EXPECT_TRUE(buffer.HasErrors());
  EXPECT_THAT(
      buffer,
      HasTokens(llvm::ArrayRef<ExpectedToken>{
          {.kind = TokenKind::OpenCurlyBrace(), .column = 1},
          {.kind = TokenKind::OpenParen(), .column = 2},
          {.kind = TokenKind::CloseParen(), .column = 3, .recovery = true},
          {.kind = TokenKind::CloseCurlyBrace(), .column = 3},
      }));

  buffer = Lex(")({)");
  EXPECT_TRUE(buffer.HasErrors());
  EXPECT_THAT(
      buffer,
      HasTokens(llvm::ArrayRef<ExpectedToken>{
          {.kind = TokenKind::Error(), .column = 1, .text = ")"},
          {.kind = TokenKind::OpenParen(), .column = 2},
          {.kind = TokenKind::OpenCurlyBrace(), .column = 3},
          {.kind = TokenKind::CloseCurlyBrace(), .column = 4, .recovery = true},
          {.kind = TokenKind::CloseParen(), .column = 4},
      }));
}

TEST_F(LexerTest, Keywords) {
  auto buffer = Lex("   fn");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(
      buffer,
      HasTokens(llvm::ArrayRef<ExpectedToken>{
          {.kind = TokenKind::FnKeyword(), .column = 4, .indent_column = 4},
      }));

  buffer = Lex("and or not if else for loop return var break continue _");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {TokenKind::AndKeyword()},
                          {TokenKind::OrKeyword()},
                          {TokenKind::NotKeyword()},
                          {TokenKind::IfKeyword()},
                          {TokenKind::ElseKeyword()},
                          {TokenKind::ForKeyword()},
                          {TokenKind::LoopKeyword()},
                          {TokenKind::ReturnKeyword()},
                          {TokenKind::VarKeyword()},
                          {TokenKind::BreakKeyword()},
                          {TokenKind::ContinueKeyword()},
                          {TokenKind::UnderscoreKeyword()},
                      }));
}

TEST_F(LexerTest, Comments) {
  auto buffer = Lex(" ;\n  // foo\n  ;");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Semi(),
                           .line = 1,
                           .column = 2,
                           .indent_column = 2},
                          {.kind = TokenKind::Semi(),
                           .line = 3,
                           .column = 3,
                           .indent_column = 3},
                      }));

  buffer = Lex("// foo\n//\n// bar");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{}));

  // Make sure weird characters aren't a problem.
  buffer = Lex("  //foo#$!^?@-_💩🍫⃠ [̲̅$̲̅(̲̅ ͡° ͜ʖ ͡°̲̅)̲̅$̲̅]");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{}));
}

TEST_F(LexerTest, DocComments) {
  auto buffer = Lex("  /// foo");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::DocComment(),
                           .line = 1,
                           .column = 3,
                           .indent_column = 3,
                           .text = "/// foo"},
                      }));

  buffer = Lex("/// foo\n//\n/// bar");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::DocComment(),
                           .line = 1,
                           .column = 1,
                           .indent_column = 1,
                           .text = "/// foo"},
                          {.kind = TokenKind::DocComment(),
                           .line = 3,
                           .column = 1,
                           .indent_column = 1,
                           .text = "/// bar"},
                      }));

  buffer = Lex("/// foo\n///\n/// bar");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::DocComment(),
                           .line = 1,
                           .column = 1,
                           .indent_column = 1,
                           .text = "/// foo"},
                          {.kind = TokenKind::DocComment(),
                           .line = 2,
                           .column = 1,
                           .indent_column = 1,
                           .text = "///"},
                          {.kind = TokenKind::DocComment(),
                           .line = 3,
                           .column = 1,
                           .indent_column = 1,
                           .text = "/// bar"},
                      }));

  // Make sure weird characters aren't a problem.
  buffer = Lex("  ///foo#$!^?@-_💩🍫⃠ [̲̅$̲̅(̲̅ ͡° ͜ʖ ͡°̲̅)̲̅$̲̅]");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::DocComment(),
                           .line = 1,
                           .column = 3,
                           .indent_column = 3,
                           .text = "///foo#$!^?@-_💩🍫⃠ [̲̅$̲̅(̲̅ ͡° ͜ʖ ͡°̲̅)̲̅$̲̅]"},
                      }));
}

TEST_F(LexerTest, Identifiers) {
  auto buffer = Lex("   foobar");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Identifier(),
                           .column = 4,
                           .indent_column = 4,
                           .text = "foobar"},
                      }));

  // Check different kinds of identifier character sequences.
  buffer = Lex("_foo_bar");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Identifier(), .text = "_foo_bar"},
                      }));

  buffer = Lex("foo2bar00");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer,
              HasTokens(llvm::ArrayRef<ExpectedToken>{
                  {.kind = TokenKind::Identifier(), .text = "foo2bar00"},
              }));

  // Check that we can parse identifiers that start with a keyword.
  buffer = Lex("fnord");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Identifier(), .text = "fnord"},
                      }));

  // Check multiple identifiers with indent and interning.
  buffer = Lex("   foo;bar\nbar \n  foo\tfoo");
  EXPECT_FALSE(buffer.HasErrors());
  EXPECT_THAT(buffer, HasTokens(llvm::ArrayRef<ExpectedToken>{
                          {.kind = TokenKind::Identifier(),
                           .line = 1,
                           .column = 4,
                           .indent_column = 4,
                           .text = "foo"},
                          {.kind = TokenKind::Semi()},
                          {.kind = TokenKind::Identifier(),
                           .line = 1,
                           .column = 8,
                           .indent_column = 4,
                           .text = "bar"},
                          {.kind = TokenKind::Identifier(),
                           .line = 2,
                           .column = 1,
                           .indent_column = 1,
                           .text = "bar"},
                          {.kind = TokenKind::Identifier(),
                           .line = 3,
                           .column = 3,
                           .indent_column = 3,
                           .text = "foo"},
                          {.kind = TokenKind::Identifier(),
                           .line = 3,
                           .column = 7,
                           .indent_column = 3,
                           .text = "foo"},
                      }));
}

auto GetAndDropLine(llvm::StringRef& text) -> std::string {
  auto newline_offset = text.find_first_of('\n');
  llvm::StringRef line = text.slice(0, newline_offset);

  if (newline_offset != llvm::StringRef::npos) {
    text = text.substr(newline_offset + 1);
  } else {
    text = "";
  }

  return line.str();
}

TEST_F(LexerTest, Printing) {
  auto buffer = Lex(";");
  ASSERT_FALSE(buffer.HasErrors());
  std::string print_storage;
  llvm::raw_string_ostream print_stream(print_storage);
  buffer.Print(print_stream);
  llvm::StringRef print = print_stream.str();
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 0, kind: 'Semi', line: 1, column: 1, "
                    "indent: 1, spelling: ';' }"));
  EXPECT_TRUE(print.empty());

  // Test kind padding.
  buffer = Lex("(;foo;)");
  ASSERT_FALSE(buffer.HasErrors());
  print_storage.clear();
  buffer.Print(print_stream);
  print = print_stream.str();
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 0, kind:  'OpenParen', line: 1, column: "
                    "1, indent: 1, spelling: '(', closing_token: 4 }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 1, kind:       'Semi', line: 1, column: "
                    "2, indent: 1, spelling: ';' }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 2, kind: 'Identifier', line: 1, column: "
                    "3, indent: 1, spelling: 'foo', identifier: 0 }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 3, kind:       'Semi', line: 1, column: "
                    "6, indent: 1, spelling: ';' }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 4, kind: 'CloseParen', line: 1, column: "
                    "7, indent: 1, spelling: ')', opening_token: 0 }"));
  EXPECT_TRUE(print.empty());

  // Test digit padding with max values of 9, 10, and 11.
  buffer = Lex(";\n\n\n\n\n\n\n\n\n\n        ;;");
  ASSERT_FALSE(buffer.HasErrors());
  print_storage.clear();
  buffer.Print(print_stream);
  print = print_stream.str();
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 0, kind: 'Semi', line:  1, column:  1, "
                    "indent: 1, spelling: ';' }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 1, kind: 'Semi', line: 11, column:  9, "
                    "indent: 9, spelling: ';' }"));
  EXPECT_THAT(GetAndDropLine(print),
              StrEq("token: { index: 2, kind: 'Semi', line: 11, column: 10, "
                    "indent: 9, spelling: ';' }"));
  EXPECT_TRUE(print.empty());
}

}  // namespace