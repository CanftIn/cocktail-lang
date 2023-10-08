#include "Cocktail/Check/Context.h"
#include "Cocktail/Check/Convert.h"
#include "Cocktail/SemIR/Node.h"

namespace Cocktail::Check {

auto HandleAddress(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleAddress");
}

auto HandleGenericPatternBinding(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "GenericPatternBinding");
}

auto HandlePatternBinding(Context& context, Parse::Node parse_node) -> bool {
  auto [type_node, parsed_type_id] =
      context.node_stack().PopExpressionWithParseNode();
  auto cast_type_id = ExpressionAsType(context, type_node, parsed_type_id);

  // Get the name.
  auto [name_node, name_id] =
      context.node_stack().PopWithParseNode<Parse::NodeKind::Name>();

  // Allocate a node of the appropriate kind, linked to the name for error
  // locations.
  // TODO: Each of these cases should create a `BindName` node.
  switch (auto context_parse_node_kind = context.parse_tree().node_kind(
              context.node_stack().PeekParseNode())) {
    case Parse::NodeKind::VariableIntroducer:
      context.AddNodeAndPush(parse_node, SemIR::Node::VarStorage::Make(
                                             name_node, cast_type_id, name_id));
      break;
    case Parse::NodeKind::ParameterListStart:
      context.AddNodeAndPush(parse_node, SemIR::Node::Parameter::Make(
                                             name_node, cast_type_id, name_id));
      break;
    case Parse::NodeKind::LetIntroducer:
      // Create the node, but don't add it to a block until after we've formed
      // its initializer.
      // TODO: For general pattern parsing, we'll need to create a block to hold
      // the `let` pattern before we see the initializer.
      context.node_stack().Push(
          parse_node,
          context.semantics_ir().AddNodeInNoBlock(SemIR::Node::BindName::Make(
              name_node, cast_type_id, name_id, SemIR::NodeId::Invalid)));
      break;
    default:
      COCKTAIL_FATAL() << "Found a pattern binding in unexpected context "
                       << context_parse_node_kind;
  }
  return true;
}

auto HandleTemplate(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleTemplate");
}

}  // namespace Cocktail::Check