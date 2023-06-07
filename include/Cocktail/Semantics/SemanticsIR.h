#ifndef COCKTAIL_SEMANTICS_SEMANTICS_IR_H
#define COCKTAIL_SEMANTICS_SEMANTICS_IR_H

#include "Cocktail/Parser/ParseTree.h"
#include "Cocktail/Semantics/Function.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace Cocktail {

class SemanticsIR {
 public:
  class Node {
   public:
    Node() : Node(Kind::Invalid, -1) {}

   private:
    friend class SemanticsIR;

    enum class Kind {
      Invalid,
      Function,
    };

    Node(Kind kind, int32_t index) : kind_(kind), index_(index) {}

    Kind kind_;
    int32_t index_;
  };

  struct Block {
   public:
    void Add(llvm::StringRef name, Node named_entity);

   private:
    llvm::SmallVector<Node> ordering_;
    llvm::StringMap<Node> name_lookup_;
  };

 private:
  friend class SemanticsIRFactory;

  explicit SemanticsIR(const ParseTree& parse_tree)
      : parse_tree_(&parse_tree) {}

  auto AddFunction(Block& block, ParseTree::Node decl_node,
                   ParseTree::Node name_node) -> Semantics::Function&;

  llvm::SmallVector<Semantics::Function, 0> functions_;
  Block root_block_;
  const ParseTree* parse_tree_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_SEMANTICS_SEMANTICS_IR_H