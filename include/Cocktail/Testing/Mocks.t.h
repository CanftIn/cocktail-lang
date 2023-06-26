#ifndef COCKTAIL_TESTING_MOCKS_T_H
#define COCKTAIL_TESTING_MOCKS_T_H

#include <gmock/gmock.h>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail::Testing {

class MockDiagnosticConsumer : public DiagnosticConsumer {
 public:
  MOCK_METHOD(void, HandleDiagnostic, (const Diagnostic& diagnostic),
              (override));
};

MATCHER_P(IsDiagnosticMessage, matcher, "") {
  const Diagnostic& diag = arg;
  return testing::ExplainMatchResult(matcher, diag.format_fn(diag),
                                     result_listener);
}

inline auto IsDiagnostic(testing::Matcher<DiagnosticKind> kind,
                         testing::Matcher<DiagnosticLevel> level,
                         testing::Matcher<int> line_number,
                         testing::Matcher<int> column_number,
                         testing::Matcher<std::string> message) {
  return testing::AllOf(
      testing::Field("kind", &Diagnostic::kind, kind),
      testing::Field("level", &Diagnostic::level, level),
      testing::Field(
          &Diagnostic::location,
          testing::AllOf(
              testing::Field("line_number", &DiagnosticLocation::line_number,
                             line_number),
              testing::Field("column_number",
                             &DiagnosticLocation::column_number,
                             column_number))),
      IsDiagnosticMessage(message));
}

}  // namespace Cocktail::Testing

namespace Cocktail {

// Printing helpers for tests.
void PrintTo(const Diagnostic& diagnostic, std::ostream* os);
void PrintTo(DiagnosticLevel level, std::ostream* os);

}  // namespace Cocktail

#endif  // COCKTAIL_TESTING_MOCKS_T_H