#ifndef COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H
#define COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H

#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/STLExtras.h"

namespace Cocktail {

/// 用于缓冲、排序和输出诊断信息。
/// 它在接收诊断信息时将其缓冲起来，然后在调用 Flush 函数时对诊断信息
/// 进行排序并传递给下一个消费者。这样可以确保诊断信息按位置信息排序，
/// 以便更容易理解和定位问题。
class SortingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit SortingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer_(&next_consumer) {}

  ~SortingDiagnosticConsumer() override {
    COCKTAIL_CHECK(diagnostics_.empty())
        << "Must flush diagnostics consumer before destroying it";
  }

  auto HandleDiagnostic(Diagnostic diagnostic) -> void override {
    diagnostics_.push_back(std::move(diagnostic));
  }

  // Flush负责对缓冲的诊断信息进行排序和输出。
  void Flush() override {
    // 依据诊断信息中的位置信息（行号和列号）排序。
    llvm::stable_sort(diagnostics_,
                      [](const Diagnostic& lhs, const Diagnostic& rhs) {
                        return std::tie(lhs.message.location.line_number,
                                        lhs.message.location.column_number) <
                               std::tie(rhs.message.location.line_number,
                                        rhs.message.location.column_number);
                      });
    // 责任链机制，确定了下一个处理者是谁。
    for (auto& diag : diagnostics_) {
      next_consumer_->HandleDiagnostic(std::move(diag));
    }
    diagnostics_.clear();
  }

 private:
  llvm::SmallVector<Diagnostic, 0> diagnostics_;
  DiagnosticConsumer* next_consumer_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_SORTING_DIAGNOSTIC_CONSUMER_H