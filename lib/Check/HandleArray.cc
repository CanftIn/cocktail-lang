#include "Cocktail/Check/Context.h"
#include "Cocktail/Check/Convert.h"
#include "Cocktail/Parse/NodeKind.h"
#include "Cocktail/SemIR/Node.h"
#include "Cocktail/SemIR/NodeKind.h"

namespace Cocktail::Check {

auto HandleArrayExpressionStart(Context& /*context*/,
                                Parse::Node /*parse_node*/) -> bool {
  return true;
}

auto HandleArrayExpressionSemi(Context& context, Parse::Node parse_node)
    -> bool {
  context.node_stack().Push(parse_node);
  return true;
}

auto HandleArrayExpression(Context& context, Parse::Node parse_node) -> bool {
  // TODO: Handle array type with undefined bound.
  if (context.parse_tree().node_kind(context.node_stack().PeekParseNode()) ==
      Parse::NodeKind::ArrayExpressionSemi) {
    context.node_stack().PopAndIgnore();
    context.node_stack().PopAndIgnore();
    return context.TODO(parse_node, "HandleArrayExpressionWithoutBounds");
  }

  auto bound_node_id = context.node_stack().PopExpression();
  context.node_stack()
      .PopAndDiscardSoloParseNode<Parse::NodeKind::ArrayExpressionSemi>();
  auto element_type_node_id = context.node_stack().PopExpression();
  auto bound_node = context.semantics_ir().GetNode(bound_node_id);
  if (bound_node.kind() == SemIR::NodeKind::IntegerLiteral) {
    auto bound_value = context.semantics_ir().GetIntegerLiteral(
        bound_node.GetAsIntegerLiteral());
    // TODO: Produce an error if the array type is too large.
    if (bound_value.getActiveBits() <= 64) {
      context.AddNodeAndPush(
          parse_node,
          SemIR::Node::ArrayType::Make(
              parse_node, SemIR::TypeId::TypeType, bound_node_id,
              ExpressionAsType(context, parse_node, element_type_node_id)));
      return true;
    }
  }
  COCKTAIL_DIAGNOSTIC(InvalidArrayExpression, Error,
                      "Invalid array expression.");
  context.emitter().Emit(parse_node, InvalidArrayExpression);
  context.node_stack().Push(parse_node, SemIR::NodeId::BuiltinError);
  return true;
}

}  // namespace Cocktail::Check