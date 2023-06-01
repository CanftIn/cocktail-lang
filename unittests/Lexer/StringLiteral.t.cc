#include "Cocktail/Lexer/StringLiteral.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace {

using namespace Cocktail;

struct StringLiteralTest : ::testing::Test {
  auto Lex(llvm::StringRef text) -> StringLiteralToken {
    llvm::Optional<StringLiteralToken> result = StringLiteralToken::Lex(text);
    assert(result);
    EXPECT_EQ(result->Text(), text);
    return *result;
  }

  auto Parse(llvm::StringRef text) -> StringLiteralToken::ExpandedValue {
    StringLiteralToken token = Lex(text);
    return token.ComputeValue(ConsoleDiagnosticEmitter());
  }
};

TEST_F(StringLiteralTest, StringLiteralBounds) {
  llvm::StringLiteral valid[] = {
      R"("")",
      R"("""
      """)",
      R"("""
      "foo"
      """)",

      // Escaped terminators don't end the string.
      R"("\"")",
      R"("\\")",
      R"("\\\"")",
      R"("""
      \"""
      """)",
      R"("""
      "\""
      """)",
      R"("""
      ""\"
      """)",
      R"("""
      ""\
      """)",
      R"(#"""
      """\#n
      """#)",

      // Only a matching number of '#'s terminates the string.
      R"(#""#)",
      R"(#"xyz"foo"#)",
      R"(##"xyz"#foo"##)",
      R"(#"\""#)",

      // Escape sequences likewise require a matching number of '#'s.
      R"(#"\#"#"#)",
      R"(#"\"#)",
      R"(#"""
      \#"""#
      """#)",

      // #"""# does not start a multiline string literal.
      R"(#"""#)",
      R"(##"""##)",
  };

  for (llvm::StringLiteral test : valid) {
    llvm::Optional<StringLiteralToken> result = StringLiteralToken::Lex(test);
    EXPECT_TRUE(result.hasValue());
    if (result) {
      EXPECT_EQ(result->Text(), test);
    }
  }

  llvm::StringLiteral invalid[] = {
      R"(")",
      R"("""
      "")",
      R"("\)",  //
      R"("\")",
      R"("\\)",  //
      R"("\\\")",
      R"("""
      )",
      R"(#"""
      """)",
      R"(" \
      ")",
  };

  for (llvm::StringLiteral test : invalid) {
    EXPECT_FALSE(StringLiteralToken::Lex(test).hasValue());
  }
}

}  // namespace