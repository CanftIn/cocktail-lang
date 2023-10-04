#ifndef COCKTAIL_PARSE_TREE_H
#define COCKTAIL_PARSE_TREE_H

#include <iterator>

#include "Cocktail/Common/Error.h"
#include "Cocktail/Common/Ostream.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lex/TokenizedBuffer.h"
#include "Cocktail/Parse/NodeKind.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"

namespace Cocktail::Parse {

// 一个轻量级的句柄，代表解析树中的一个节点。
// 这种类型的对象小且易于复制和存储。它们不包含节点的任何信息，只是作为一个句柄，
// 可以与底层的树一起使用，以查询详细信息。
struct Node : public ComparableIndexBase {
  static const Node Invalid;

  using ComparableIndexBase::ComparableIndexBase;
};

// 基于语言语法的解析令牌的树。
//
// 这是一个纯粹的语法解析树，尚未附加任何语义。它基于令牌流和语言的语法，
// 甚至没有名称查找。
//
// 这棵树旨在使深度优先遍历特别高效，其中后序和反向后序（RPO，拓扑顺序）甚至不需要额外的状态。
//
// 树的节点遵循轻量级模式，并且节点是树的句柄。必须提供树本身以查询有关这些节点的信息。
//
// 节点还与从解析token流中的token一一对应。每个节点都可以被视为流中特定token的树位置。
//
// 一旦建立，树就是不可变的，它旨在支持构建具有特定转换的树的高效模式。
//
// 为什么要这么写：
// 解析树是编译器和解释器中的核心数据结构。它提供了一种结构化的方式来表示和处理源代码。
// Node 和 Tree 的设计使得它们可以高效地表示和处理这种结构。特别是，Node
// 的设计使其成
// 为一个轻量级的句柄，这意味着它可以快速地复制和传递，而不需要复制整个节点的数据。
// 这对于遍历和处理树非常有用。
//
// Tree
// 类提供了一系列的方法来构建、查询和操作解析树。这些方法使得处理解析树变得更加简单和直接。
// 例如，Parse 方法提供了一种从令牌缓冲区构建解析树的方法，而 has_errors
// 方法则允许查询树是否 包含任何错误。
//
// 此外，Tree 类还提供了一系列的迭代器，如 PostorderIterator 和
// SiblingIterator，这些迭代器使
// 得遍历树变得更加简单和高效。这是因为它们提供了一种直接的方式来访问树中的节点，而不需要显式
// 地管理遍历的状态。
class Tree : public Printable<Tree> {
 public:
  class PostorderIterator;
  class SiblingIterator;

  // 工厂函数，用于将token buffer解析为Tree。
  static auto Parse(Lex::TokenizedBuffer& tokens, DiagnosticConsumer& consumer,
                    llvm::raw_ostream* vlog_stream) -> Tree;

  // 测试解析树中是否存在任何错误。
  [[nodiscard]] auto has_errors() const -> bool { return has_errors_; }

  // 返回此解析树中的节点数。
  [[nodiscard]] auto size() const -> int { return node_impls_.size(); }

  // 返回深度优先后序中解析树节点的可迭代范围。
  [[nodiscard]] auto postorder() const
      -> llvm::iterator_range<PostorderIterator>;
  [[nodiscard]] auto postorder(Node n) const
      -> llvm::iterator_range<PostorderIterator>;

  // 返回解析树中节点的直接子节点的可迭代范围。
  // 子节点的顺序与反向后序遍历中找到的顺序相同。
  [[nodiscard]] auto children(Node n) const
      -> llvm::iterator_range<SiblingIterator>;

  // 返回解析树的根的可迭代范围。
  // 根的顺序与反向后序遍历中找到的顺序相同。
  [[nodiscard]] auto roots() const -> llvm::iterator_range<SiblingIterator>;

  // 测试特定节点是否包含错误，并且可能不符合语法的完整预期结构。
  [[nodiscard]] auto node_has_error(Node n) const -> bool;

  // 返回给定解析树节点的种类。
  [[nodiscard]] auto node_kind(Node n) const -> NodeKind;

  // 返回给定解析树节点模型的token。
  [[nodiscard]] auto node_token(Node n) const -> Lex::Token;

  // 返回给定节点的子树大小。
  [[nodiscard]] auto node_subtree_size(Node n) const -> int32_t;

  // 返回给定节点的token的文本。
  [[nodiscard]] auto GetNodeText(Node n) const -> llvm::StringRef;

  auto Print(llvm::raw_ostream& output) const -> void;

  // 将解析树的描述打印到提供的 `raw_ostream`。
  // 树可以以前序或后序打印。
  auto Print(llvm::raw_ostream& output, bool preorder) const -> void;

  // 验证解析树结构。检查解析树结构的不变式并返回验证错误。
  //
  // 这主要是用作调试辅助。Tree中不直接检查，以便可以在调试器中使用它。
  [[nodiscard]] auto Verify() const -> ErrorOr<Success>;

 private:
  friend class Context;

  // 表示树中特定节点的内存中数据表示。
  struct NodeImpl {
    explicit NodeImpl(NodeKind kind, bool has_error, Lex::Token token,
                      int subtree_size)
        : kind(kind),
          has_error(has_error),
          token(token),
          subtree_size(subtree_size) {}

    // 表示此节点的类型。只占用一个字节。
    NodeKind kind;

    // 这里有3个字节的padding，我们可以将标志或其他压缩数据装入其中。

    // 此节点是否包含或是一个解析错误。
    //
    // 当has_error为真时，此节点及其子节点可能不具有预期的语法结构。
    // 在推理任何特定的子树结构之前，必须检查此标志。
    bool has_error = false;

    // 表示此节点的token。
    Lex::Token token;

    // 此节点的解析树子树的大小。这是此节点（及其后代）在解析树中覆盖的节点的数量。
    //
    // 在解析树的反向后序（reverse
    // postorder即RPO）遍历期间，subtree_size是到下一个非后代节点的偏移。
    // 当此节点不是其父节点的第一个子节点（这是RPO中访问的最后一个子节点）时，这是到下一个兄弟节点的偏移。
    // 当此节点是其父节点的第一个子节点时，这将是到节点的父节点的下一个兄弟节点的偏移，
    // 或者如果父节点也是第一个子节点，则是到祖父节点的下一个兄弟节点的偏移，依此类推。
    int32_t subtree_size;
  };

  static_assert(sizeof(NodeImpl) == 12,
                "Unexpected size of node implementation!");

  explicit Tree(Lex::TokenizedBuffer& tokens_arg) : tokens_(&tokens_arg) {
    // 如果树是有效的，每个token将有一个节点，因此reserve一次。
    node_impls_.reserve(tokens_->expected_parse_tree_size());
  }

  // 为Print()函数打印单个节点。
  auto PrintNode(llvm::raw_ostream& output, Node n, int depth,
                 bool preorder) const -> bool;

  // 表示节点实现数据的深度优先后序序列。
  llvm::SmallVector<NodeImpl> node_impls_;

  // 指向已标记的缓冲区的指针。
  Lex::TokenizedBuffer* tokens_;

  // 表示在解析时是否遇到任何错误。
  bool has_errors_ = false;
};

// 一个随机访问迭代器，用于深度优先后序遍历解析树中的节点。
class Tree::PostorderIterator
    : public llvm::iterator_facade_base<PostorderIterator,
                                        std::random_access_iterator_tag, Node,
                                        int, Node*, Node>,
      public Printable<Tree::PostorderIterator> {
 public:
  PostorderIterator() = delete;

  auto operator==(const PostorderIterator& rhs) const -> bool {
    return node_ == rhs.node_;
  }
  auto operator<(const PostorderIterator& rhs) const -> bool {
    return node_ < rhs.node_;
  }

  auto operator*() const -> Node { return node_; }

  auto operator-(const PostorderIterator& rhs) const -> int {
    return node_.index - rhs.node_.index;
  }

  auto operator+=(int offset) -> PostorderIterator& {
    node_.index += offset;
    return *this;
  }
  auto operator-=(int offset) -> PostorderIterator& {
    node_.index -= offset;
    return *this;
  }

  auto Print(llvm::raw_ostream& output) const -> void;

 private:
  friend class Tree;

  explicit PostorderIterator(Node n) : node_(n) {}

  Node node_;
};

// 一个前向迭代器，用于遍历解析树中特定级别的同级节点。
// 可能在 Tree 数据结构中没有良好的局部性。
class Tree::SiblingIterator
    : public llvm::iterator_facade_base<
          SiblingIterator, std::forward_iterator_tag, Node, int, Node*, Node>,
      public Printable<Tree::SiblingIterator> {
 public:
  explicit SiblingIterator() = delete;

  auto operator==(const SiblingIterator& rhs) const -> bool {
    return node_ == rhs.node_;
  }
  auto operator<(const SiblingIterator& rhs) const -> bool {
    // 子迭代器与后置索（postorder index）的遍历顺序相反。
    return node_ > rhs.node_;
  }

  auto operator*() const -> Node { return node_; }

  using iterator_facade_base::operator++;
  auto operator++() -> SiblingIterator& {
    node_.index -= std::abs(tree_->node_impls_[node_.index].subtree_size);
    return *this;
  }

  auto Print(llvm::raw_ostream& output) const -> void;

 private:
  friend class Tree;

  explicit SiblingIterator(const Tree& tree_arg, Node n)
      : tree_(&tree_arg), node_(n) {}

  const Tree* tree_;

  Node node_;
};

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_TREE_H