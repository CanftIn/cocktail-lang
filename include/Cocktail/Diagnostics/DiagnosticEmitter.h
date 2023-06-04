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
  enum Level {
    Warning,
    Error,
  };

  struct Location {
    std::string file_name;
    int32_t line_number;
    int32_t column_number;
  };

  Level level;
  Location location;
  llvm::StringRef short_name;
  std::string message;
};

// Receives diagnostics as they are emitted.
class DiagnosticConsumer {
 public:
  virtual ~DiagnosticConsumer() = default;

  virtual auto HandleDiagnostic(const Diagnostic& diagnostic) -> void = 0;
};

// translate some representation of a location
template <typename LocationT>
class DiagnosticLocationTranslator {
 public:
  virtual ~DiagnosticLocationTranslator() = default;

  [[nodiscard]] virtual auto GetLocation(LocationT loc)
      -> Diagnostic::Location = 0;
};

template <typename LocationT>
class DiagnosticEmitter {
 public:
  explicit DiagnosticEmitter(
      DiagnosticLocationTranslator<LocationT>& translator,
      DiagnosticConsumer& consumer)
      : translator(&translator), consumer(&consumer) {}

  ~DiagnosticEmitter() = default;

  template <typename DiagnosticT>
  auto EmitError(LocationT location, DiagnosticT diag) -> void {
    consumer->HandleDiagnostic({
        .level = Diagnostic::Error,
        .location = translator->GetLocation(location),
        .short_name = DiagnosticT::ShortName,
        .message = diag.Format(),
    });
  }

  template <typename DiagnosticT>
  auto EmitError(LocationT location)
      -> std::enable_if_t<std::is_empty_v<DiagnosticT>> {
    EmitError<DiagnosticT>(location, {});
  }

  template <typename DiagnosticT>
  auto EmitWarningIf(LocationT location,
                     llvm::function_ref<bool(DiagnosticT&)> f) -> void {
    DiagnosticT diag;
    if (f(diag)) {
      consumer->HandleDiagnostic({
          .level = Diagnostic::Warning,
          .location = translator->GetLocation(location),
          .short_name = DiagnosticT::ShortName,
          .message = diag.Format(),
      });
    }
  }

 private:
  DiagnosticLocationTranslator<LocationT>* translator;
  DiagnosticConsumer* consumer;
};

inline auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer& {
  struct Consumer : DiagnosticConsumer {
    auto HandleDiagnostic(const Diagnostic& diagnostic) -> void override {
      if (!diagnostic.location.file_name.empty()) {
        llvm::errs() << diagnostic.location.file_name << ":"
                     << diagnostic.location.line_number << ":"
                     << diagnostic.location.column_number << ": ";
      }

      llvm::errs() << diagnostic.message << "\n";
    }
  };
  static auto* consumer = new Consumer;
  return *consumer;
}

// CRTP base class for diagnostics with no substitutions.
template <typename Derived>
struct SimpleDiagnostic {
  static auto Format() -> std::string { return Derived::Message.str(); }
};

// track whether any errors have been produced. (Chain of Responsibility)
class ErrorTrackingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit ErrorTrackingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer(&next_consumer) {}

  auto HandleDiagnostic(const Diagnostic& diagnostic) -> void override {
    seen_error |= diagnostic.level == Diagnostic::Error;
    next_consumer->HandleDiagnostic(diagnostic);
  }

  auto SeenError() const -> bool { return seen_error; }

  auto Reset() -> void { seen_error = false; }

 private:
  DiagnosticConsumer* next_consumer;
  bool seen_error = false;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H