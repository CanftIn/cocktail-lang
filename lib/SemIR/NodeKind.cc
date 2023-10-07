#include "Cocktail/SemIR/NodeKind.h"

namespace Cocktail::SemIR {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(NodeKind) = {
#define COCKTAIL_SEMANTICS_NODE_KIND(Name) COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "Cocktail/SemIR/NodeKind.def"
};

// Returns the name to use for this node kind in Semantics IR.
[[nodiscard]] auto NodeKind::ir_name() const -> llvm::StringRef {
  static constexpr llvm::StringRef Table[] = {
#define COCKTAIL_SEMANTICS_NODE_KIND_WITH_IR_NAME(Name, IR_Name) IR_Name,
#include "Cocktail/SemIR/NodeKind.def"
  };
  return Table[AsInt()];
}

auto NodeKind::value_kind() const -> NodeValueKind {
  static constexpr NodeValueKind Table[] = {
#define COCKTAIL_SEMANTICS_NODE_KIND_WITH_VALUE_KIND(Name, Kind) \
  NodeValueKind::Kind,
#include "Cocktail/SemIR/NodeKind.def"
  };
  return Table[AsInt()];
}

auto NodeKind::terminator_kind() const -> TerminatorKind {
  static constexpr TerminatorKind Table[] = {
#define COCKTAIL_SEMANTICS_NODE_KIND_WITH_TERMINATOR_KIND(Name, Kind) \
  TerminatorKind::Kind,
#include "Cocktail/SemIR/NodeKind.def"
  };
  return Table[AsInt()];
}

}  // namespace Cocktail::SemIR