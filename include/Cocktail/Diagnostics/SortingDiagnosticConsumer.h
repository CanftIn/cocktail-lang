#ifndef COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H
#define COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H

#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/STLExtras.h"

namespace Cocktail {

class SortingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit SortingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer_(&next_consumer) {}

  ~SortingDiagnosticConsumer() override { Flush(); }

  auto HandleDiagnostic(const Diagnostic& diagnostic) -> void override {
    diagnostics_.push_back(diagnostic);
  }

  auto Flush() -> void override {
    llvm::sort(diagnostics_, [](const Diagnostic& lhs, const Diagnostic& rhs) {
      return std::tie(lhs.location.line_number, lhs.location.column_number) <
             std::tie(rhs.location.line_number, rhs.location.column_number);
    });
    for (const auto& diagnostic : diagnostics_) {
      next_consumer_->HandleDiagnostic(diagnostic);
    }
    diagnostics_.clear();
  }

 private:
  llvm::SmallVector<Diagnostic, 0> diagnostics_;
  DiagnosticConsumer* next_consumer_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H