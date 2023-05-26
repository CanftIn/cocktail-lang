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
      : callback(std::move(callback)) {}
  ~DiagnosticEmitter() = default;

  template <typename DiagnosticT>
  void EmitError(typename DiagnosticT::Substitutions substitutions) {
    callback({.short_name = DiagnosticT::ShortName,
              .message = DiagnosticT::Format(substitutions)});
  }

  template <typename DiagnosticT>
  auto EmitError() -> std::enable_if_t<
      std::is_empty_v<typename DiagnosticT::Substitutions>> {
    EmitError<DiagnosticT>({});
  }

  template <typename DiagnosticT>
  void EmitWarningIf(
      llvm::function_ref<bool(typename DiagnosticT::Substitutions&)> f) {
    typename DiagnosticT::Substitutions substitutions;
    if (f(substitutions)) {
      callback({.short_name = DiagnosticT::ShortName,
                .message = DiagnosticT::Format(substitutions)});
    }
  }

 private:
  Callback callback;
};

inline auto ConsoleDiagnosticEmitter() -> DiagnosticEmitter& {
  static auto* emitter = new DiagnosticEmitter(
      [](const Diagnostic& d) { llvm::errs() << d.message << "\n"; });
  return *emitter;
}

inline auto NullDiagnosticEmitter() -> DiagnosticEmitter& {
  static auto* emitter = new DiagnosticEmitter([](const Diagnostic&) {});
  return *emitter;
}

// CRTP base class for diagnostics with no substitutions.
template <typename Derived>
struct SimpleDiagnostic {
  struct Substitutions {};
  static auto Format(const Substitutions&) -> std::string {
    return Derived::Message.str();
  }
};

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H