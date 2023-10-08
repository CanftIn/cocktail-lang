#include "Cocktail/Check/NodeStack.h"

#include "Cocktail/SemIR/Node.h"
#include "llvm/ADT/STLExtras.h"

namespace Cocktail::Check {

auto NodeStack::PrintForStackDump(llvm::raw_ostream& output) const -> void {
  output << "NodeStack:\n";
  for (const auto& item : llvm::enumerate(stack_)) {
    auto i = item.index();
    auto entry = item.value();
    auto parse_node_kind = parse_tree_->node_kind(entry.parse_node);
    output << "\t" << i << ".\t" << parse_node_kind;
    if (parse_node_kind == Parse::NodeKind::PatternBinding) {
      output << " -> " << entry.name_id;
    } else {
      if (entry.node_id.is_valid()) {
        output << " -> " << entry.node_id;
      }
    }
    output << "\n";
  }
}

}  // namespace Cocktail::Check