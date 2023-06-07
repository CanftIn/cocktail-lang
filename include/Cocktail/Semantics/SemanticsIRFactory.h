#ifndef COCKTAIL_SEMANTICS_SEMANTICS_IR_FACTORY_H
#define COCKTAIL_SEMANTICS_SEMANTICS_IR_FACTORY_H

#include <optional>

#include "Cocktail/Parser/ParseTree.h"
#include "Cocktail/Semantics/SemanticsIR.h"
#include "llvm/ADT/StringMap.h"

namespace Cocktail {

class SemanticsIRFactory {
 public:
  static auto Build(const ParseTree& parse_tree) -> SemanticsIR;

 private:
  explicit SemanticsIRFactory(const ParseTree& parse_tree)
      : semantics_(parse_tree) {}

  void ProcessRoots();

  void ProcessFuntionNode(SemanticsIR::Block& block, ParseTree::Node decl_node);

  SemanticsIR semantics_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_SEMANTICS_SEMANTICS_IR_FACTORY_H