#ifndef COCKTAIL_TESTING_MOCKS_T_H
#define COCKTAIL_TESTING_MOCKS_T_H

#include <gmock/gmock.h>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail::Testing {

class MockDiagnosticConsumer : public DiagnosticConsumer {
 public:
  MOCK_METHOD1(HandleDiagnostic, void(const Diagnostic& diagnostic));
};

MATCHER_P2(DiagnosticAt, line, column, "") {
  const Diagnostic& diag = arg;
  const Diagnostic::Location& loc = diag.location;
  if (loc.line_number != line) {
    *result_listener << "\nExpected diagnostic on line " << line
                     << " but diagnostic is on line " << loc.line_number << ".";
    return false;
  }
  if (loc.column_number != column) {
    *result_listener << "\nExpected diagnostic on column " << column
                     << " but diagnostic is on column " << loc.column_number
                     << ".";
    return false;
  }
  return true;
}

inline auto DiagnosticLevel(Diagnostic::Level level) -> auto {
  return testing::Field(&Diagnostic::level, level);
}

template <typename Matcher>
auto DiagnosticMessage(Matcher&& inner_matcher) -> auto {
  return testing::Field(&Diagnostic::message,
                        std::forward<Matcher&&>(inner_matcher));
}

template <typename Matcher>
auto DiagnosticShortName(Matcher&& inner_matcher) -> auto {
  return testing::Field(&Diagnostic::short_name,
                        std::forward<Matcher&&>(inner_matcher));
}

}  // namespace Cocktail::Testing

#endif  // COCKTAIL_TESTING_MOCKS_T_H