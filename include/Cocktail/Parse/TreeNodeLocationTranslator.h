#ifndef COCKTAIL_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H
#define COCKTAIL_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H

#include "Cocktail/Parse/Tree.h"

namespace Cocktail::Parse {

class NodeLocationTranslator : public DiagnosticLocationTranslator<Node> {
 public:
  explicit NodeLocationTranslator(const Lex::TokenizedBuffer* tokens,
                                  const Tree* parse_tree)
      : token_translator_(tokens), parse_tree_(parse_tree) {}

  auto GetLocation(Node node) -> DiagnosticLocation override {
    return token_translator_.GetLocation(parse_tree_->node_token(node));
  }

 private:
  Lex::TokenLocationTranslator token_translator_;
  const Tree* parse_tree_;
};

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_TREE_NODE_LOCATION_TRANSLATOR_H