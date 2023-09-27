#include "Cocktail/Diagnostics/DiagnosticKind.h"

namespace Cocktail {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(DiagnosticKind) = {
#define COCKTAIL_DIAGNOSTIC_KIND(Name) COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "Cocktail/Diagnostics/DiagnosticKind.def"
};

}  // namespace Cocktail