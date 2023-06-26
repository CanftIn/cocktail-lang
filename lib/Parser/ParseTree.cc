#include "Cocktail/Parser/ParseTree.h"

#include <cstdlib>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Lexer/TokenKind.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParserImpl.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

auto ParseTree::Parse(TokenizedBuffer& tokens, DiagnosticConsumer& consumer)
    -> ParseTree {
  TokenizedBuffer::TokenLocationTranslator translator(tokens, nullptr);
  TokenDiagnosticEmitter emitter(translator, consumer);

  return Parser::Parse(tokens, emitter);
}

auto ParseTree::postorder() const -> llvm::iterator_range<PostorderIterator> {
  return {PostorderIterator(Node(0)),
          PostorderIterator(Node(node_impls_.size()))};
}

auto ParseTree::postorder(Node n) const
    -> llvm::iterator_range<PostorderIterator> {
  int end_index = n.index_ + 1;
  int start_index = end_index - node_impls_[n.index_].subtree_size;
  return {PostorderIterator(Node(start_index)),
          PostorderIterator(Node(end_index))};
}

auto ParseTree::children(Node n) const
    -> llvm::iterator_range<SiblingIterator> {
  int end_index = n.index_ - node_impls_[n.index_].subtree_size;
  return {SiblingIterator(*this, Node(n.index_ - 1)),
          SiblingIterator(*this, Node(end_index))};
}

auto ParseTree::roots() const -> llvm::iterator_range<SiblingIterator> {
  return {
      SiblingIterator(*this, Node(static_cast<int>(node_impls_.size()) - 1)),
      SiblingIterator(*this, Node(-1))};
}

auto ParseTree::node_has_error(Node n) const -> bool {
  COCKTAIL_CHECK(n.is_valid());
  return node_impls_[n.index_].has_error;
}

auto ParseTree::node_kind(Node n) const -> ParseNodeKind {
  COCKTAIL_CHECK(n.is_valid());
  return node_impls_[n.index_].kind;
}

auto ParseTree::node_token(Node n) const -> TokenizedBuffer::Token {
  COCKTAIL_CHECK(n.is_valid());
  return node_impls_[n.index_].token;
}

auto ParseTree::GetNodeText(Node n) const -> llvm::StringRef {
  COCKTAIL_CHECK(n.is_valid());
  return tokens_->GetTokenText(node_impls_[n.index_].token);
}

auto ParseTree::Print(llvm::raw_ostream& output) const -> void {
  output << "[\n";
  llvm::SmallVector<std::pair<Node, int>, 16> node_stack;
  for (Node n : roots()) {
    node_stack.push_back({n, 0});
  }

  while (!node_stack.empty()) {
    Node n;
    int depth;
    std::tie(n, depth) = node_stack.pop_back_val();
    const auto& n_impl = node_impls_[n.index()];

    for (int unused_indent : llvm::seq(0, depth)) {
      (void)unused_indent;
      output << "  ";
    }

    output << "{node_index: " << n.index_ << ", kind: '" << n_impl.kind.name()
           << "', text: '" << tokens_->GetTokenText(n_impl.token) << "'";

    if (n_impl.has_error) {
      output << ", has_error: yes";
    }

    if (n_impl.subtree_size > 1) {
      output << ", subtree_size: " << n_impl.subtree_size;
      output << ", children: [\n";
      for (Node sibling_n : children(n)) {
        node_stack.push_back({sibling_n, depth + 1});
      }
      continue;
    }

    COCKTAIL_CHECK(n_impl.subtree_size == 1)
        << "Subtree size must always be a positive integer!";
    output << "}";

    int next_depth = node_stack.empty() ? 0 : node_stack.back().second;
    COCKTAIL_CHECK(next_depth <= depth)
        << "Cannot have the next depth increase!";
    for (int close_children_count : llvm::seq(0, depth - next_depth)) {
      (void)close_children_count;
      output << "]}";
    }

    output << ",\n";
  }
  output << "]\n";
}

auto ParseTree::Verify() const -> bool {
  llvm::SmallVector<ParseTree::Node, 16> ancestors;
  for (Node n : llvm::reverse(postorder())) {
    const auto& n_impl = node_impls_[n.index()];

    if (n_impl.has_error && !has_errors_) {
      llvm::errs()
          << "Node #" << n.index()
          << " has errors, but the tree is not marked as having any.\n";
      return false;
    }

    if (n_impl.subtree_size > 1) {
      if (!ancestors.empty()) {
        auto parent_n = ancestors.back();
        const auto& parent_n_impl = node_impls_[parent_n.index()];
        int end_index = n.index() - n_impl.subtree_size;
        int parent_end_index = parent_n.index() - parent_n_impl.subtree_size;
        if (parent_end_index > end_index) {
          llvm::errs() << "Node #" << n.index() << " has a subtree size of "
                       << n_impl.subtree_size
                       << " which extends beyond its parent's (node #"
                       << parent_n.index() << ") subtree (size "
                       << parent_n_impl.subtree_size << ")\n";
          return false;
        }
      }
      ancestors.push_back(n);
      continue;
    }

    if (n_impl.subtree_size < 1) {
      llvm::errs() << "Node #" << n.index()
                   << " has an invalid subtree size of " << n_impl.subtree_size
                   << "!\n";
      return false;
    }

    int next_index = n.index() - 1;
    while (!ancestors.empty()) {
      ParseTree::Node parent_n = ancestors.back();
      if ((parent_n.index() - node_impls_[parent_n.index()].subtree_size) !=
          next_index) {
        break;
      }
      ancestors.pop_back();
    }
  }
  if (!ancestors.empty()) {
    llvm::errs()
        << "Finished walking the parse tree and there are still ancestors:\n";
    for (Node ancestor_n : ancestors) {
      llvm::errs() << "  Node #" << ancestor_n.index() << "\n";
    }
    return false;
  }

  return true;
}

auto ParseTree::Node::Print(llvm::raw_ostream& output) const -> void {
  output << index();
}

auto ParseTree::PostorderIterator::Print(llvm::raw_ostream& output) const
    -> void {
  output << node_.index();
}

auto ParseTree::SiblingIterator::Print(llvm::raw_ostream& output) const
    -> void {
  output << node_.index();
}

}  // namespace Cocktail