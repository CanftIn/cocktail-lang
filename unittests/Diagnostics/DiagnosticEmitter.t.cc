#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Testing/Mocks.t.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;

struct FakeDiagnosticLocationTranslator : DiagnosticLocationTranslator<int> {
  auto GetLocation(int n) -> DiagnosticLocation override {
    return {.line_number = 1, .column_number = n};
  }
};

class DiagnosticEmitterTest : public ::testing::Test {
 protected:
  DiagnosticEmitterTest() : emitter_(translator_, consumer_) {}

  FakeDiagnosticLocationTranslator translator_;
  Testing::MockDiagnosticConsumer consumer_;
  DiagnosticEmitter<int> emitter_;
};

TEST_F(DiagnosticEmitterTest, EmitSimpleError) {
  COCKTAIL_DIAGNOSTIC(TestDiagnostic, Error, "simple error");
  EXPECT_CALL(consumer_, HandleDiagnostic(IsDiagnostic(
                             DiagnosticKind::TestDiagnostic,
                             DiagnosticLevel::Error, 1, 1, "simple error")));
  EXPECT_CALL(consumer_, HandleDiagnostic(IsDiagnostic(
                             DiagnosticKind::TestDiagnostic,
                             DiagnosticLevel::Error, 1, 2, "simple error")));
  emitter_.Emit(1, TestDiagnostic);
  emitter_.Emit(2, TestDiagnostic);
}

TEST_F(DiagnosticEmitterTest, EmitSimpleWarning) {
  COCKTAIL_DIAGNOSTIC(TestDiagnostic, Warning, "simple warning");
  EXPECT_CALL(consumer_,
              HandleDiagnostic(IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                            DiagnosticLevel::Warning, 1, 1,
                                            "simple warning")));
  emitter_.Emit(1, TestDiagnostic);
}

TEST_F(DiagnosticEmitterTest, EmitOneArgDiagnostic) {
  COCKTAIL_DIAGNOSTIC(TestDiagnostic, Error, "arg: `{0}`", llvm::StringRef);
  EXPECT_CALL(consumer_, HandleDiagnostic(IsDiagnostic(
                             DiagnosticKind::TestDiagnostic,
                             DiagnosticLevel::Error, 1, 1, "arg: `str`")));
  emitter_.Emit(1, TestDiagnostic, "str");
}

}  // namespace