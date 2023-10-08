#include "Cocktail/Check/Context.h"
#include "Cocktail/SemIR/Node.h"

namespace Cocktail::Check {

auto HandleNamespaceStart(Context& context, Parse::Node /*parse_node*/)
    -> bool {
  context.declaration_name_stack().Push();
  return true;
}

auto HandleNamespace(Context& context, Parse::Node parse_node) -> bool {
  auto name_context = context.declaration_name_stack().Pop();
  auto namespace_id = context.AddNode(SemIR::Node::Namespace::Make(
      parse_node, context.semantics_ir().AddNameScope()));
  context.declaration_name_stack().AddNameToLookup(name_context, namespace_id);
  return true;
}

}  // namespace Cocktail::Check