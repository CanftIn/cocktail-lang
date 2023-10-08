#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandleNamedConstraintDeclaration(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleNamedConstraintDeclaration");
}

auto HandleNamedConstraintDefinition(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleNamedConstraintDefinition");
}

auto HandleNamedConstraintDefinitionStart(Context& context,
                                          Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandleNamedConstraintDefinitionStart");
}

auto HandleNamedConstraintIntroducer(Context& context, Parse::Node parse_node)
    -> bool {
  return context.TODO(parse_node, "HandleNamedConstraintIntroducer");
}

}  // namespace Cocktail::Check