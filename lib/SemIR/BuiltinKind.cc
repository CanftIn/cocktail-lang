#include "Cocktail/SemIR/BuiltinKind.h"

namespace Cocktail::SemIR {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(BuiltinKind) = {
#define COCKTAIL_SEMANTICS_BUILTIN_KIND_NAME(Name) \
  COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "Cocktail/SemIR/BuiltinKind.def"
};

auto BuiltinKind::label() -> llvm::StringRef {
  static constexpr llvm::StringLiteral Labels[] = {
#define COCKTAIL_SEMANTICS_BUILTIN_KIND(Name, Label) Label,
#include "Cocktail/SemIR/BuiltinKind.def"
  };
  return Labels[AsInt()];
}

}  // namespace Cocktail::SemIR