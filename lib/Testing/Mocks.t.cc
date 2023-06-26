#include "Cocktail/Testing/Mocks.t.h"

namespace Cocktail {

void PrintTo(const Diagnostic& diagnostic, std::ostream* os) {
  *os << "Diagnostic{" << diagnostic.kind << ", ";
  PrintTo(diagnostic.level, os);
  *os << ", " << diagnostic.location.file_name << ":"
      << diagnostic.location.line_number << ":"
      << diagnostic.location.column_number << ", \""
      << diagnostic.format_fn(diagnostic) << "\"}";
}

void PrintTo(DiagnosticLevel level, std::ostream* os) {
  *os << (level == DiagnosticLevel::Error ? "Error" : "Warning");
}

}  // namespace Cocktail