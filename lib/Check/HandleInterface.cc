#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandleInterfaceDeclaration(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleInterfaceDeclaration");
}

auto HandleInterfaceDefinition(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleInterfaceDefinition");
}

auto HandleInterfaceDefinitionStart(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleInterfaceDefinitionStart");
}

auto HandleInterfaceIntroducer(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleInterfaceIntroducer");
}

}  // namespace Cocktail::Check