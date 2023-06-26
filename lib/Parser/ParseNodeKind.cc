#include "Cocktail/Parser/ParseNodeKind.h"

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

auto ParseNodeKind::name() const -> llvm::StringRef {
  static constexpr llvm::StringLiteral Names[] = {
#define COCKTAIL_PARSE_NODE_KIND(Name) #Name,
#include "Cocktail/Parser/ParseNodeKind.def"
  };
  return Names[static_cast<int>(kind_)];
}

}  // namespace Cocktail