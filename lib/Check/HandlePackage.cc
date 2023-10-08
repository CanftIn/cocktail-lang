#include "Cocktail/Check/Context.h"

namespace Cocktail::Check {

auto HandlePackageApi(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePackageApi");
}

auto HandlePackageDirective(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePackageDirective");
}

auto HandlePackageImpl(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePackageImpl");
}

auto HandlePackageIntroducer(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePackageIntroducer");
}

auto HandlePackageLibrary(Context& context, Parse::Node parse_node) -> bool {
  return context.TODO(parse_node, "HandlePackageLibrary");
}

}  // namespace Cocktail::Check