#ifndef COCKTAIL_SEMANTICS_FUNCTION_H
#define COCKTAIL_SEMANTICS_FUNCTION_H

#include "Cocktail/Parser/ParseTree.h"

namespace Cocktail::Semantics {

class Function {
 public:
  Function(ParseTree::Node decl_node, ParseTree::Node name_node)
      : decl_node_(decl_node), name_node_(name_node) {}

  auto decl_node() const -> ParseTree::Node { return decl_node_; }
  auto name_node() const -> ParseTree::Node { return name_node_; }

 private:
  ParseTree::Node decl_node_;
  ParseTree::Node name_node_;
};

}  // namespace Cocktail::Semantics

#endif  // COCKTAIL_SEMANTICS_FUNCTION_H