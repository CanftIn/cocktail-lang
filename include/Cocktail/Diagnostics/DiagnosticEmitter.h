#ifndef COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H
#define COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H

#include <functional>
#include <string>

#include "llvm/ADT/Any.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

struct Diagnostic {
  llvm::StringRef short_name;
  std::string message;
};

class DiagnosticEmitter {
 public:
  using Callback = std::function<void(const Diagnostic&)>;

  explicit DiagnosticEmitter(Callback callback)
      : callback_(std::move(callback)) {}
  ~DiagnosticEmitter() = default;

  template <typename DiagnosticT>
  void EmitError(
      llvm::function_ref<void(typename DiagnosticT::Substitutions&)> f) {
    typename DiagnosticT::Substitutions substitutions;
    f(substitutions);
    callback_({.short_name = DiagnosticT::ShortName,
               .message = DiagnosticT::Format(substitutions)});
  }

  template <typename DiagnosticT>
  void EmitWarningIf(
      llvm::function_ref<bool(typename DiagnosticT::Substitutions&)> f) {
    typename DiagnosticT::Substitutions substitutions;
    if (f(substitutions)) {
      callback_({.short_name = DiagnosticT::ShortName,
                 .message = DiagnosticT::Format(substitutions)});
    }
  }

 private:
  Callback callback_;
};

inline auto ConsoleDiagnosticEmitter() -> DiagnosticEmitter& {
  static auto* emitter =
      new DiagnosticEmitter([](const Diagnostic& diagnostic) {
        llvm::errs() << diagnostic.short_name << ": " << diagnostic.message
                     << "\n";
      });
  return *emitter;
}

inline auto NullDiagnosticEmitter() -> DiagnosticEmitter& {
  static auto* emitter =
      new DiagnosticEmitter([](const Diagnostic& diagnostic) {
        // Do nothing.
      });
  return *emitter;
}

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H