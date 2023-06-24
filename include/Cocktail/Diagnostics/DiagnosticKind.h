#ifndef COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H
#define COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H

#include "Cocktail/Common/Ostream.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

enum class DiagnosticKind : uint32_t {
#define DIAGNOSTIC_KIND(Name) Name,
#include "Cocktail/Diagnostics/DiagnosticRegistry.def"
};

inline auto operator<<(llvm::raw_ostream& out, DiagnosticKind kind)
    -> llvm::raw_ostream& {
  static constexpr llvm::StringLiteral Names[] = {
#define DIAGNOSTIC_KIND(Name) #Name,
#include "Cocktail/Diagnostics/DiagnosticRegistry.def"
  };
  out << Names[static_cast<uint32_t>(kind)];
  return out;
}

inline auto operator<<(std::ostream& out, DiagnosticKind kind)
    -> std::ostream& {
  llvm::raw_os_ostream(out) << kind;
  return out;
}

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H