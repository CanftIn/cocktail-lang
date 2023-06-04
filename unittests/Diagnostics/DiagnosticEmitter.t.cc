#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Testing/Mocks.t.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"

namespace {

using namespace Cocktail;
using Testing::DiagnosticAt;
using Testing::DiagnosticLevel;
using Testing::DiagnosticMessage;
using Testing::DiagnosticShortName;

struct FakeDiagnostic {
  static constexpr llvm::StringLiteral ShortName = "fake-diagnostic";
  static constexpr llvm::StringLiteral Message = "{0}";

  std::string message;

  auto Format() -> std::string {
    static_cast<void>(ShortName);

    return llvm::formatv(Message.data(), message).str();
  }
};

struct FakeDiagnosticLocationTranslator : DiagnosticLocationTranslator<int> {
  auto GetLocation(int n) -> Diagnostic::Location override {
    return {.file_name = "test", .line_number = 1, .column_number = n};
  }
};

TEST(DiagTest, EmitErrors) {
  FakeDiagnosticLocationTranslator translator;
  Testing::MockDiagnosticConsumer consumer;
  DiagnosticEmitter<int> emitter(translator, consumer);

  EXPECT_CALL(consumer, HandleDiagnostic(
                            AllOf(DiagnosticLevel(Diagnostic::Error),
                                  DiagnosticAt(1, 1), DiagnosticMessage("M1"),
                                  DiagnosticShortName("fake-diagnostic"))));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            AllOf(DiagnosticLevel(Diagnostic::Error),
                                  DiagnosticAt(1, 2), DiagnosticMessage("M2"),
                                  DiagnosticShortName("fake-diagnostic"))));

  emitter.EmitError<FakeDiagnostic>(1, {.message = "M1"});
  emitter.EmitError<FakeDiagnostic>(2, {.message = "M2"});
}

TEST(DiagTest, EmitWarnings) {
  std::vector<std::string> reported;

  FakeDiagnosticLocationTranslator translator;
  Testing::MockDiagnosticConsumer consumer;
  DiagnosticEmitter<int> emitter(translator, consumer);

  EXPECT_CALL(consumer, HandleDiagnostic(
                            AllOf(DiagnosticLevel(Diagnostic::Warning),
                                  DiagnosticAt(1, 3), DiagnosticMessage("M1"),
                                  DiagnosticShortName("fake-diagnostic"))));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            AllOf(DiagnosticLevel(Diagnostic::Warning),
                                  DiagnosticAt(1, 5), DiagnosticMessage("M3"),
                                  DiagnosticShortName("fake-diagnostic"))));

  emitter.EmitWarningIf<FakeDiagnostic>(3, [](FakeDiagnostic& diagnostic) {
    diagnostic.message = "M1";
    return true;
  });
  emitter.EmitWarningIf<FakeDiagnostic>(4, [](FakeDiagnostic& diagnostic) {
    diagnostic.message = "M2";
    return false;
  });
  emitter.EmitWarningIf<FakeDiagnostic>(5, [](FakeDiagnostic& diagnostic) {
    diagnostic.message = "M3";
    return true;
  });
}

}  // namespace