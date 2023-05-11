#ifndef COCKTAIL_PARSER_PARSE_TREE_H
#define COCKTAIL_PARSER_PARSE_TREE_H

#include <iterator>

#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"

namespace Cocktail {

class ParseTree {
 public:
  class Node;
  class PostorderIterator;
  class SiblingIterator;

  static auto Parse(TokenizedBuffer& tokens) -> ParseTree;

  [[nodiscard]] auto HasErrors() const -> bool { return has_errors; }

  [[nodiscard]] auto Size() const -> int { return node_impls.size(); }

  [[nodiscard]] auto Postorder() const
      -> llvm::iterator_range<PostorderIterator>;

  [[nodiscard]] auto Postorder(Node n) const
      -> llvm::iterator_range<PostorderIterator>;

  [[nodiscard]] auto Children(Node n) const
      -> llvm::iterator_range<SiblingIterator>;

  [[nodiscard]] auto Roots() const -> llvm::iterator_range<SiblingIterator>;

  [[nodiscard]] auto HasErrorInNode(Node n) const -> bool;

  [[nodiscard]] auto GetNodeKind(Node n) const -> ParseNodeKind;

  [[nodiscard]] auto GetNodeToken(Node n) const -> TokenizedBuffer::Token;

  [[nodiscard]] auto GetNodeText(Node n) const -> llvm::StringRef;

  auto Print(llvm::raw_ostream& output) const -> void;

  [[nodiscard]] auto Verify() const -> bool;

 private:
  class Parser;
  friend Parser;

  struct NodeImpl {
    ParseNodeKind kind;

    bool has_error = false;

    TokenizedBuffer::Token token;

    int32_t subtree_size;

    explicit NodeImpl(ParseNodeKind k, TokenizedBuffer::Token t,
                      int subtree_size_arg)
        : kind(k), token(t), subtree_size(subtree_size_arg) {}
  };

  static_assert(sizeof(NodeImpl) == 12,
                "Unexpected size of node implementation!");

  explicit ParseTree(TokenizedBuffer& tokens_arg) : tokens(&tokens_arg) {}

  llvm::SmallVector<NodeImpl, 0> node_impls;

  TokenizedBuffer* tokens;

  bool has_errors = false;
};

class ParseTree::Node {
 public:
  Node() = default;

  friend auto operator==(Node lhs, Node rhs) -> bool {
    return lhs.index == rhs.index;
  }
  friend auto operator!=(Node lhs, Node rhs) -> bool {
    return lhs.index != rhs.index;
  }
  friend auto operator<(Node lhs, Node rhs) -> bool {
    return lhs.index < rhs.index;
  }
  friend auto operator<=(Node lhs, Node rhs) -> bool {
    return lhs.index <= rhs.index;
  }
  friend auto operator>(Node lhs, Node rhs) -> bool {
    return lhs.index > rhs.index;
  }
  friend auto operator>=(Node lhs, Node rhs) -> bool {
    return lhs.index >= rhs.index;
  }

  [[nodiscard]] auto GetIndex() const -> int { return index; }

 private:
  friend ParseTree;
  friend Parser;
  friend PostorderIterator;
  friend SiblingIterator;

  explicit Node(int index_arg) : index(index_arg) {}

  int32_t index;
};

class ParseTree::PostorderIterator
    : public llvm::iterator_facade_base<PostorderIterator,
                                        std::random_access_iterator_tag, Node,
                                        int, Node*, Node> {
 public:
  PostorderIterator() = default;

  auto operator==(const PostorderIterator& rhs) const -> bool {
    return node == rhs.node;
  }

  auto operator<(const PostorderIterator& rhs) const -> bool {
    return node < rhs.node;
  }

  auto operator*() const -> Node { return node; }

  auto operator-(const PostorderIterator& rhs) const -> int {
    return node.index - rhs.node.index;
  }

  auto operator+=(int offset) -> PostorderIterator& {
    node.index += offset;
    return *this;
  }

  auto operator-=(int offset) -> PostorderIterator& {
    node.index -= offset;
    return *this;
  }

 private:
  friend class ParseTree;

  Node node;

  explicit PostorderIterator(Node node_arg) : node(node_arg) {}
};

class ParseTree::SiblingIterator
    : public llvm::iterator_facade_base<SiblingIterator,
                                        std::forward_iterator_tag, Node,
                                        int, Node*, Node> {
 public:
  SiblingIterator() = default;

  auto operator==(const SiblingIterator& rhs) const -> bool {
    return node == rhs.node;
  }

  auto operator<(const SiblingIterator& rhs) const -> bool {
    return node > rhs.node;
  }

  auto operator*() const -> Node { return node; }

  using iterator_facade_base::operator++;
  auto operator++() -> SiblingIterator& {
    node.index -= std::abs(tree->node_impls[node.index].subtree_size);
    return *this;
  }

 private:
  friend class ParseTree;

  const ParseTree* tree;

  Node node;

  explicit SiblingIterator(const ParseTree& tree_arg, Node node_arg)
      : tree(&tree_arg), node(node_arg) {}
};

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSE_TREE_H