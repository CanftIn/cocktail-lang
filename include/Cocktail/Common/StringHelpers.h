#ifndef COCKTAIL_COMMON_STRING_HELPERS_H
#define COCKTAIL_COMMON_STRING_HELPERS_H

#include <optional>
#include <string>

#include "Cocktail/Common/Error.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

auto UnescapeStringLiteral(llvm::StringRef source, bool is_block_string = false)
    -> std::optional<std::string>;

auto ParseBlockStringLiteral(llvm::StringRef source) -> ErrorOr<std::string>;

auto StringRefContainsPointer(llvm::StringRef ref, const char* ptr) -> bool;

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_STRING_HELPERS_H