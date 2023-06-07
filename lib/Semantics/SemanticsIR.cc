#include "Cocktail/Semantics/SemanticsIR.h"

#include "Cocktail/Common/Check.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "llvm/Support/FormatVariadic.h"

namespace Cocktail {

void SemanticsIR::Block::Add(llvm::StringRef name, Node named_entity) {
  ordering_.push_back(named_entity);
  name_lookup_.insert({name, named_entity});
}

auto SemanticsIR::AddFunction(Block& block, ParseTree::Node decl_node,
                              ParseTree::Node name_node)
    -> Semantics::Function& {
  auto index = functions_.size();
  functions_.emplace_back(Semantics::Function(decl_node, name_node));
  block.Add(parse_tree_->GetNodeText(name_node),
            Node(Node::Kind::Function, index));
  return functions_[index];
}

}  // namespace Cocktail