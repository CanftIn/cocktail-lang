#ifndef COCKTAIL_PARSER_PARSER_NODE_KIND_H
#define COCKTAIL_PARSER_PARSER_NODE_KIND_H

#include <cstdint>
#include <iterator>

#include "llvm/ADT/StringRef.h"

namespace Cocktail {

class ParseNodeKind {
  enum class KindEnum : uint8_t {
#define COCKTAIL_PARSE_NODE_KIND(Name) Name,
#include "Cocktail/Parser/ParseNodeKind.def"
  };

 public:
#define COCKTAIL_PARSE_NODE_KIND(Name)            \
  static constexpr auto Name() -> ParseNodeKind { \
    return ParseNodeKind(KindEnum::Name);         \
  }
#include "Cocktail/Parser/ParseNodeKind.def"

  ParseNodeKind() = delete;

  friend auto operator==(const ParseNodeKind& lhs, const ParseNodeKind& rhs)
      -> bool {
    return lhs.kind == rhs.kind;
  }

  friend auto operator!=(const ParseNodeKind& lhs, const ParseNodeKind& rhs)
      -> bool {
    return lhs.kind != rhs.kind;
  }

  [[nodiscard]] auto GetName() const -> llvm::StringRef;

  constexpr operator KindEnum() const { return kind; }

 private:
  constexpr explicit ParseNodeKind(KindEnum k) : kind(k) {}

  KindEnum kind;
};

// We expect the parse node kind to fit compactly into 8 bits.
static_assert(sizeof(ParseNodeKind) == 1, "Kind objects include padding!");

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSER_NODE_KIND_H