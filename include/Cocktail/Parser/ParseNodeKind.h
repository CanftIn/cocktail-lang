#ifndef COCKTAIL_PARSER_PARSER_NODE_KIND_H
#define COCKTAIL_PARSER_PARSER_NODE_KIND_H

#include <cstdint>
#include <iterator>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class ParseNodeKind {
 public:
#define COCKTAIL_PARSE_NODE_KIND(Name) \
  static constexpr auto Name() -> ParseNodeKind { return KindEnum::Name; }
#include "Cocktail/Parser/ParseNodeKind.def"

  ParseNodeKind() = delete;

  auto operator==(const ParseNodeKind& rhs) const -> bool {
    return kind == rhs.kind;
  }

  auto operator!=(const ParseNodeKind& rhs) const -> bool {
    return kind != rhs.kind;
  }

  [[nodiscard]] auto GetName() const -> llvm::StringRef;

 private:
  enum class KindEnum : uint8_t {
#define COCKTAIL_PARSE_NODE_KIND(Name) Name,
#include "Cocktail/Parser/ParseNodeKind.def"
  };

  constexpr ParseNodeKind(KindEnum k) : kind(k) {}

  explicit constexpr operator KindEnum() const { return kind; }

  KindEnum kind;
};

// We expect the parse node kind to fit compactly into 8 bits.
static_assert(sizeof(ParseNodeKind) == 1, "Kind objects include padding!");

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSER_NODE_KIND_H