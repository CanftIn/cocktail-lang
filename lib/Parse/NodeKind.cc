#include "Cocktail/Parse/NodeKind.h"

#include "Cocktail/Common/Check.h"

namespace Cocktail::Parse {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(NodeKind) = {
#define COCKTAIL_PARSE_NODE_KIND(Name) COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "Cocktail/Parse/NodeKind.def"
};

auto NodeKind::has_bracket() const -> bool {
  static constexpr bool HasBracket[] = {
#define COCKTAIL_PARSE_NODE_KIND_BRACKET(...) true,
#define COCKTAIL_PARSE_NODE_KIND_CHILD_COUNT(...) false,
#include "Cocktail/Parse/NodeKind.def"
  };
  return HasBracket[AsInt()];
}

auto NodeKind::bracket() const -> NodeKind {
  // 节点永远不会自括起来，所以我们将其用于设置child_count的节点。
  static constexpr NodeKind Bracket[] = {
#define COCKTAIL_PARSE_NODE_KIND_BRACKET(Name, BracketName) \
  NodeKind::BracketName,
#define COCKTAIL_PARSE_NODE_KIND_CHILD_COUNT(Name, ...) NodeKind::Name,
#include "Cocktail/Parse/NodeKind.def"
  };
  auto bracket = Bracket[AsInt()];
  COCKTAIL_CHECK(bracket != *this) << *this;
  return bracket;
}

auto NodeKind::child_count() const -> int32_t {
  static constexpr int32_t ChildCount[] = {
#define COCKTAIL_PARSE_NODE_KIND_BRACKET(...) -1,
#define COCKTAIL_PARSE_NODE_KIND_CHILD_COUNT(Name, Size) Size,
#include "Cocktail/Parse/NodeKind.def"
  };
  auto child_count = ChildCount[AsInt()];
  COCKTAIL_CHECK(child_count >= 0) << *this;
  return child_count;
}

}  // namespace Cocktail::Parse