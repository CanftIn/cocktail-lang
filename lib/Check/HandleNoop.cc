#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandleEmptyDeclaration(Context& /*context*/, Parse::Node /*parse_node*/)
    -> bool {
  // Empty declarations have no actions associated.
  return true;
}

auto HandleFileStart(Context& /*context*/, Parse::Node /*parse_node*/) -> bool {
  // Do nothing, no need to balance this node.
  return true;
}

auto HandleFileEnd(Context& /*context*/, Parse::Node /*parse_node*/) -> bool {
  // Do nothing, no need to balance this node.
  return true;
}

auto HandleInvalidParse(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleInvalidParse");
}

}  // namespace Cocktail::Check