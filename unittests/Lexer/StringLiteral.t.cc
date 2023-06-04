#include "Cocktail/Lexer/StringLiteral.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Testing/Lexer.t.h"

namespace {

using namespace Cocktail;

struct StringLiteralTest : ::testing::Test {
  StringLiteralTest() : error_tracker(ConsoleDiagnosticConsumer()) {}

  ErrorTrackingDiagnosticConsumer error_tracker;

  auto Lex(llvm::StringRef text) -> LexedStringLiteral {
    llvm::Optional<LexedStringLiteral> result = LexedStringLiteral::Lex(text);
    assert(result);
    EXPECT_EQ(result->Text(), text);
    return *result;
  }

  auto Parse(llvm::StringRef text) -> std::string {
    LexedStringLiteral token = Lex(text);
    Testing::SingleTokenDiagnosticTranslator translator(text);
    DiagnosticEmitter<const char*> emitter(translator, error_tracker);
    return token.ComputeValue(emitter);
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
    llvm::Optional<LexedStringLiteral> result = LexedStringLiteral::Lex(test);
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
    EXPECT_FALSE(LexedStringLiteral::Lex(test).hasValue());
  }
}

}  // namespace