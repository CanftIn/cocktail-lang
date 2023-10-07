#ifndef COCKTAIL_PARSER_PARSE_TREE_H
#define COCKTAIL_PARSER_PARSE_TREE_H

#include <iterator>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
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

  static constexpr int StackDepthLimit = 200;

  static auto Parse(TokenizedBuffer& tokens, DiagnosticConsumer& consumer)
      -> ParseTree;

  [[nodiscard]] auto has_errors() const -> bool { return has_errors_; }

  [[nodiscard]] auto size() const -> int { return node_impls_.size(); }

  [[nodiscard]] auto postorder() const
      -> llvm::iterator_range<PostorderIterator>;

  [[nodiscard]] auto postorder(Node n) const
      -> llvm::iterator_range<PostorderIterator>;

  [[nodiscard]] auto children(Node n) const
      -> llvm::iterator_range<SiblingIterator>;

  [[nodiscard]] auto roots() const -> llvm::iterator_range<SiblingIterator>;

  [[nodiscard]] auto node_has_error(Node n) const -> bool;

  [[nodiscard]] auto node_kind(Node n) const -> ParseNodeKind;

  [[nodiscard]] auto node_token(Node n) const -> TokenizedBuffer::Token;

  [[nodiscard]] auto GetNodeText(Node n) const -> llvm::StringRef;

  auto Print(llvm::raw_ostream& output) const -> void;

  [[nodiscard]] auto Verify() const -> bool;

 private:
  class Parser;
  friend Parser;

  struct NodeImpl {
    explicit NodeImpl(ParseNodeKind k, TokenizedBuffer::Token t,
                      int subtree_size_arg)
        : kind(k), token(t), subtree_size(subtree_size_arg) {}

    ParseNodeKind kind;

    bool has_error = false;

    TokenizedBuffer::Token token;

    int32_t subtree_size;
  };

  static_assert(sizeof(NodeImpl) == 12,
                "Unexpected size of node implementation!");

  explicit ParseTree(TokenizedBuffer& tokens_arg) : tokens_(&tokens_arg) {}

  llvm::SmallVector<NodeImpl, 0> node_impls_;

  TokenizedBuffer* tokens_;

  bool has_errors_ = false;
};

class ParseTree::Node {
 public:
  Node() = default;

  friend auto operator==(Node lhs, Node rhs) -> bool {
    return lhs.index_ == rhs.index_;
  }
  friend auto operator!=(Node lhs, Node rhs) -> bool {
    return lhs.index_ != rhs.index_;
  }
  friend auto operator<(Node lhs, Node rhs) -> bool {
    return lhs.index_ < rhs.index_;
  }
  friend auto operator<=(Node lhs, Node rhs) -> bool {
    return lhs.index_ <= rhs.index_;
  }
  friend auto operator>(Node lhs, Node rhs) -> bool {
    return lhs.index_ > rhs.index_;
  }
  friend auto operator>=(Node lhs, Node rhs) -> bool {
    return lhs.index_ >= rhs.index_;
  }

  [[nodiscard]] auto index() const -> int { return index_; }

  auto Print(llvm::raw_ostream& output) const -> void;

  auto is_valid() -> bool { return index_ != InvalidValue; }

 private:
  friend ParseTree;
  friend Parser;
  friend PostorderIterator;
  friend SiblingIterator;

  static constexpr int InvalidValue = -1;

  explicit Node(int index) : index_(index) {}

  int32_t index_ = InvalidValue;
};

class ParseTree::PostorderIterator
    : public llvm::iterator_facade_base<PostorderIterator,
                                        std::random_access_iterator_tag, Node,
                                        int, Node*, Node> {
 public:
  auto operator==(const PostorderIterator& rhs) const -> bool {
    return node_ == rhs.node_;
  }
  auto operator<(const PostorderIterator& rhs) const -> bool {
    return node_ < rhs.node_;
  }

  auto operator*() const -> Node { return node_; }

  auto operator-(const PostorderIterator& rhs) const -> int {
    return node_.index_ - rhs.node_.index_;
  }

  auto operator+=(int offset) -> PostorderIterator& {
    node_.index_ += offset;
    return *this;
  }
  auto operator-=(int offset) -> PostorderIterator& {
    node_.index_ -= offset;
    return *this;
  }

  auto Print(llvm::raw_ostream& output) const -> void;

 private:
  friend class ParseTree;

  explicit PostorderIterator(Node n) : node_(n) {}

  Node node_;
};

class ParseTree::SiblingIterator
    : public llvm::iterator_facade_base<
          SiblingIterator, std::forward_iterator_tag, Node, int, Node*, Node> {
 public:
  SiblingIterator() = default;

  auto operator==(const SiblingIterator& rhs) const -> bool {
    return node_ == rhs.node_;
  }
  auto operator<(const SiblingIterator& rhs) const -> bool {
    return node_ > rhs.node_;
  }

  auto operator*() const -> Node { return node_; }

  using iterator_facade_base::operator++;
  auto operator++() -> SiblingIterator& {
    node_.index_ -= std::abs(tree_->node_impls_[node_.index_].subtree_size);
    return *this;
  }

  auto Print(llvm::raw_ostream& output) const -> void;

 private:
  friend class ParseTree;

  explicit SiblingIterator(const ParseTree& tree_arg, Node n)
      : tree_(&tree_arg), node_(n) {}

  const ParseTree* tree_;

  Node node_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSE_TREE_H