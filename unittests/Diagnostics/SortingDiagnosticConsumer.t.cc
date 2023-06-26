#include "Cocktail/Diagnostics/SortingDiagnosticConsumer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Testing/Mocks.t.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;
using ::testing::InSequence;

COCKTAIL_DIAGNOSTIC(TestDiagnostic, Error, "{0}", llvm::StringRef);

struct FakeDiagnosticLocationTranslator
    : DiagnosticLocationTranslator<DiagnosticLocation> {
  auto GetLocation(DiagnosticLocation loc) -> DiagnosticLocation override {
    return loc;
  }
};

TEST(SortedDiagnosticEmitterTest, SortErrors) {
  FakeDiagnosticLocationTranslator translator;
  Testing::MockDiagnosticConsumer consumer;
  SortingDiagnosticConsumer sorting_consumer(consumer);
  DiagnosticEmitter<DiagnosticLocation> emitter(translator, sorting_consumer);

  emitter.Emit({"f", 2, 1}, TestDiagnostic, "M1");
  emitter.Emit({"f", 1, 1}, TestDiagnostic, "M2");
  emitter.Emit({"f", 1, 3}, TestDiagnostic, "M3");
  emitter.Emit({"f", 3, 4}, TestDiagnostic, "M4");
  emitter.Emit({"f", 3, 2}, TestDiagnostic, "M5");

  InSequence s;
  EXPECT_CALL(consumer, HandleDiagnostic(
                            IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                         DiagnosticLevel::Error, 1, 1, "M2")));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                         DiagnosticLevel::Error, 1, 3, "M3")));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                         DiagnosticLevel::Error, 2, 1, "M1")));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                         DiagnosticLevel::Error, 3, 2, "M5")));
  EXPECT_CALL(consumer, HandleDiagnostic(
                            IsDiagnostic(DiagnosticKind::TestDiagnostic,
                                         DiagnosticLevel::Error, 3, 4, "M4")));
  sorting_consumer.Flush();
}

}  // namespace