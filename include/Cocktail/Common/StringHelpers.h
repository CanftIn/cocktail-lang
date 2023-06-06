#ifndef COCKTAIL_COMMON_STRING_HELPERS_H
#define COCKTAIL_COMMON_STRING_HELPERS_H

#include <optional>
#include <string>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

auto UnescapeStringLiteral(llvm::StringRef source)
    -> std::optional<std::string>;

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_STRING_HELPERS_H