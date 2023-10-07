#ifndef COCKTAIL_SEMIR_BUILTIN_KIND_H
#define COCKTAIL_SEMIR_BUILTIN_KIND_H

#include <cstdint>

#include "Cocktail/Common/EnumBase.h"

namespace Cocktail::SemIR {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(BuiltinKind, uint8_t){
#define COCKTAIL_SEMANTICS_BUILTIN_KIND_NAME(Name) \
  COCKTAIL_RAW_ENUM_ENUMERATOR(Name)
#include "Cocktail/SemIR/BuiltinKind.def"
};

class BuiltinKind : public COCKTAIL_ENUM_BASE(BuiltinKind) {
 public:
#define COCKTAIL_SEMANTICS_BUILTIN_KIND_NAME(Name) \
  COCKTAIL_ENUM_CONSTANT_DECLARATION(Name)
#include "Cocktail/SemIR/BuiltinKind.def"

  auto label() -> llvm::StringRef;

  // The count of enum values excluding Invalid.
  //
  // Note that we *define* this as `constexpr` making it a true compile-time
  // constant.
  static const uint8_t ValidCount;

  // Support conversion to and from an int32_t for SemanticNode storage.
  using EnumBase::AsInt;
  using EnumBase::FromInt;
};

#define COCKTAIL_SEMANTICS_BUILTIN_KIND_NAME(Name) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(BuiltinKind, Name)
#include "Cocktail/SemIR/BuiltinKind.def"

constexpr uint8_t BuiltinKind::ValidCount = Invalid.AsInt();

static_assert(
    BuiltinKind::ValidCount != 0,
    "The above `constexpr` definition of `ValidCount` makes it available in "
    "a `constexpr` context despite being declared as merely `const`. We use it "
    "in a static assert here to ensure that.");

// We expect the builtin kind to fit compactly into 8 bits.
static_assert(sizeof(BuiltinKind) == 1, "Kind objects include padding!");

}  // namespace Cocktail::SemIR

#endif  // COCKTAIL_SEMIR_BUILTIN_KIND_H