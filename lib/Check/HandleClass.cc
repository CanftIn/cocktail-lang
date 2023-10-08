#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandleClassDeclaration(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleClassDeclaration");
}

auto HandleClassDefinition(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleClassDefinition");
}

auto HandleClassDefinitionStart(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleClassDefinitionStart");
}

auto HandleClassIntroducer(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleClassIntroducer");
}

}  // namespace Cocktail::Check
