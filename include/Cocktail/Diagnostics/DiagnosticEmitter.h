#ifndef COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H
#define COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H

#include <functional>
#include <string>
#include <utility>

#include "Cocktail/Diagnostics/DiagnosticKind.h"
#include "llvm/ADT/Any.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

enum class DiagnosticLevel : int8_t {
  Warning,
  Error,
};

#define COCKTAIL_DIAGNOSTIC(DiagnosticName, Level, Format, ...) \
  static constexpr auto DiagnosticName =                        \
      Internal::DiagnosticBase<__VA_ARGS__>(                    \
          ::Cocktail::DiagnosticKind::DiagnosticName,           \
          ::Cocktail::DiagnosticLevel::Level, Format)

struct DiagnosticLocation {
  std::string file_name;
  // 1-based line number.
  int32_t line_number;
  // 1-based column number.
  int32_t column_number;
};

struct Diagnostic {
  DiagnosticKind kind;

  DiagnosticLevel level;

  DiagnosticLocation location;

  llvm::StringLiteral format;

  llvm::SmallVector<llvm::Any, 0> format_args;

  std::function<std::string(const Diagnostic&)> format_fn;
};

class DiagnosticConsumer {
 public:
  virtual ~DiagnosticConsumer() = default;

  // Handle a diagnostic.
  virtual auto HandleDiagnostic(const Diagnostic& diagnostic) -> void = 0;

  // Flushes any buffered input.
  virtual auto Flush() -> void {}
};

template <typename LocationT>
class DiagnosticLocationTranslator {
 public:
  virtual ~DiagnosticLocationTranslator() = default;

  [[nodiscard]] virtual auto GetLocation(LocationT loc)
      -> DiagnosticLocation = 0;
};

namespace Internal {

template <typename... Args>
struct DiagnosticBase {
  constexpr DiagnosticBase(DiagnosticKind kind, DiagnosticLevel level,
                           llvm::StringLiteral format)
      : Kind(kind), Level(level), Format(format) {}

  // Calls formatv with the diagnostic's arguments.
  auto FormatFn(const Diagnostic& diagnostic) const -> std::string {
    return FormatFnImpl(diagnostic,
                        std::make_index_sequence<sizeof...(Args)>());
  };

  // The diagnostic's kind.
  DiagnosticKind Kind;
  // The diagnostic's level.
  DiagnosticLevel Level;
  // The diagnostic's format for llvm::formatv.
  llvm::StringLiteral Format;

 private:
  // Handles the cast of llvm::Any to Args types for formatv.
  template <std::size_t... N>
  inline auto FormatFnImpl(const Diagnostic& diagnostic,
                           std::index_sequence<N...> /*indices*/) const
      -> std::string {
    assert(diagnostic.format_args.size() == sizeof...(Args));
    return llvm::formatv(diagnostic.format.data(),
                         llvm::any_cast<Args>(diagnostic.format_args[N])...);
  }
};

}  // namespace Internal

template <typename LocationT>
class DiagnosticEmitter {
 public:
  explicit DiagnosticEmitter(
      DiagnosticLocationTranslator<LocationT>& translator,
      DiagnosticConsumer& consumer)
      : translator_(&translator), consumer_(&consumer) {}
  ~DiagnosticEmitter() = default;

  template <typename... Args>
  void Emit(LocationT location,
            const Internal::DiagnosticBase<Args...>& diagnostic_base,
            typename std::common_type_t<Args>... args) {
    consumer_->HandleDiagnostic({
        .kind = diagnostic_base.Kind,
        .level = diagnostic_base.Level,
        .location = translator_->GetLocation(location),
        .format = diagnostic_base.Format,
        .format_args = {std::move(args)...},
        .format_fn = [&diagnostic_base](const Diagnostic& diagnostic)
            -> std::string { return diagnostic_base.FormatFn(diagnostic); },
    });
  }

 private:
  DiagnosticLocationTranslator<LocationT>* translator_;
  DiagnosticConsumer* consumer_;
};

inline auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer& {
  struct Consumer : DiagnosticConsumer {
    auto HandleDiagnostic(const Diagnostic& diagnostic) -> void override {
      llvm::errs() << diagnostic.location.file_name << ":"
                   << diagnostic.location.line_number << ":"
                   << diagnostic.location.column_number << ": "
                   << diagnostic.format_fn(diagnostic) << "\n";
    }
  };
  static auto* consumer = new Consumer;
  return *consumer;
}

class ErrorTrackingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit ErrorTrackingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer_(&next_consumer) {}

  auto HandleDiagnostic(const Diagnostic& diagnostic) -> void override {
    seen_error_ |= diagnostic.level == DiagnosticLevel::Error;
    next_consumer_->HandleDiagnostic(diagnostic);
  }

  // Reset whether we've seen an error.
  auto Reset() -> void { seen_error_ = false; }

  // Returns whether we've seen an error since the last reset.
  auto seen_error() const -> bool { return seen_error_; }

 private:
  DiagnosticConsumer* next_consumer_;
  bool seen_error_ = false;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H