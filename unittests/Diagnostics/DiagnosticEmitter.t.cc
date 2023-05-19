#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"

namespace {

using namespace Cocktail;
using ::testing::ElementsAre;
using ::testing::Eq;

struct FakeDiagnostic {
  static constexpr llvm::StringLiteral ShortName = "fake-diagnostic";
  static constexpr llvm::StringLiteral Message = "{0}";

  struct Substitutions {
    std::string message;
  };

  static auto Format(const Substitutions& substitutions) -> std::string {
    static_cast<void>(ShortName);

    return llvm::formatv(Message.data(), substitutions.message).str();
  }
};

TEST(DiagTest, EmitErrors) {
  std::vector<std::string> reported;

  DiagnosticEmitter emitter([&](const Diagnostic& diagnostic) {
    EXPECT_THAT(diagnostic.short_name, Eq("fake-diagnostic"));
    reported.push_back(diagnostic.message);
  });

  emitter.EmitError<FakeDiagnostic>(
      [](FakeDiagnostic::Substitutions& diagnostic) {
        diagnostic.message = "M1";
      });

  emitter.EmitError<FakeDiagnostic>(
      [](FakeDiagnostic::Substitutions& diagnostic) {
        diagnostic.message = "M2";
      });

  EXPECT_THAT(reported, ElementsAre("M1", "M2"));
}

TEST(DiagTest, EmitWarnings) {
  std::vector<std::string> reported;

  DiagnosticEmitter emitter([&](const Diagnostic& diagnostic) {
    EXPECT_THAT(diagnostic.short_name, Eq("fake-diagnostic"));
    reported.push_back(diagnostic.message);
  });

  emitter.EmitWarningIf<FakeDiagnostic>(
      [](FakeDiagnostic::Substitutions& diagnostic) {
        diagnostic.message = "M1";
        return true;
      });
  emitter.EmitWarningIf<FakeDiagnostic>(
      [](FakeDiagnostic::Substitutions& diagnostic) {
        diagnostic.message = "M2";
        return false;
      });
  emitter.EmitWarningIf<FakeDiagnostic>(
      [](FakeDiagnostic::Substitutions& diagnostic) {
        diagnostic.message = "M3";
        return true;
      });

  EXPECT_THAT(reported, ElementsAre("M1", "M3"));
}

}  // namespace