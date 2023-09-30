#ifndef COCKTAIL_COMMON_PRETTY_STACK_TRACE_FUNCTION_H
#define COCKTAIL_COMMON_PRETTY_STACK_TRACE_FUNCTION_H

#include <functional>

#include "llvm/Support/PrettyStackTrace.h"

namespace Cocktail {

class PrettyStackTraceFunction : public llvm::PrettyStackTraceEntry {
 public:
  explicit PrettyStackTraceFunction(std::function<void(llvm::raw_ostream&)> fn)
      : fn_(std::move(fn)) {}
  ~PrettyStackTraceFunction() override = default;

  auto print(llvm::raw_ostream& output) const -> void override { fn_(output); }

 private:
  const std::function<void(llvm::raw_ostream&)> fn_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_PRETTY_STACK_TRACE_FUNCTION_H