#include "Cocktail/LexNew/StringLiteral.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Testing/Lexer.t.h"

namespace Cocktail::Lex {
namespace {

class StringLiteralTest : public ::testing::Test {
 protected:
  StringLiteralTest() : error_tracker(ConsoleDiagnosticConsumer()) {}

  auto Lex(llvm::StringRef text) -> StringLiteral {
    std::optional<StringLiteral> result = StringLiteral::Lex(text);
    COCKTAIL_CHECK(result);
    EXPECT_EQ(result->text(), text);
    return *result;
  }

  auto Parse(llvm::StringRef text) -> std::string {
    StringLiteral token = Lex(text);
    Testing::SingleTokenDiagnosticTranslator translator(text);
    DiagnosticEmitter<const char*> emitter(translator, error_tracker);
    return token.ComputeValue(emitter);
  }

  ErrorTrackingDiagnosticConsumer error_tracker;
};

TEST_F(StringLiteralTest, StringLiteralBounds) {
  llvm::StringLiteral valid[] = {
      R"(##"""##)",
  };

  for (llvm::StringLiteral test : valid) {
    SCOPED_TRACE(test);
    std::optional<StringLiteral> result = StringLiteral::Lex(test);
    EXPECT_TRUE(result.has_value());
    if (result) {
      EXPECT_EQ(result->text(), test);
    }
  }
}

}  // namespace
}  // namespace Cocktail::Lex