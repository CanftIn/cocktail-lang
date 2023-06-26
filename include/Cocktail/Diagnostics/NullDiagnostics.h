#ifndef COCKTAIL_DIAGNOSTICS_NULL_DIAGNOSTICS_H
#define COCKTAIL_DIAGNOSTICS_NULL_DIAGNOSTICS_H

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail {

template <typename LocationT>
inline auto NullDiagnosticLocationTranslator()
    -> DiagnosticLocationTranslator<LocationT>& {
  struct Translator : DiagnosticLocationTranslator<LocationT> {
    [[nodiscard]] auto GetLocation(LocationT /*unused*/)
        -> DiagnosticLocation override {
      return {};
    }
  };
  static auto* translator = new Translator();
  return *translator;
}

inline auto NullDiagnosticConsumer() -> DiagnosticConsumer& {
  struct Consumer : DiagnosticConsumer {
    auto HandleDiagnostic(const Diagnostic& /*unused*/) -> void override {}
  };
  static auto* consumer = new Consumer();
  return *consumer;
}

template <typename LocationT>
inline auto NullDiagnosticEmitter() -> DiagnosticEmitter<LocationT>& {
  static auto* emitter = new DiagnosticEmitter<LocationT>(
      NullDiagnosticLocationTranslator<LocationT>(), NullDiagnosticConsumer());
  return *emitter;
}

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_NULL_DIAGNOSTICS_H