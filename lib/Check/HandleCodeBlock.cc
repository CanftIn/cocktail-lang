#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandleCodeBlockStart(Context& context, Parse::Node parse_node) -> bool {
  context.node_stack().Push(parse_node);
  context.PushScope();
  return true;
}

auto HandleCodeBlock(Context& context, Parse::Node /*parse_node*/) -> bool {
  context.PopScope();
  context.node_stack().PopForSoloParseNode<Parse::NodeKind::CodeBlockStart>();
  return true;
}

}  // namespace Cocktail::Check
