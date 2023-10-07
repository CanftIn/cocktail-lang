#include "Cocktail/Parse/Context.h"

#include <optional>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Common/Ostream.h"
#include "Cocktail/Lex/TokenKind.h"
#include "Cocktail/Lex/TokenizedBuffer.h"
#include "Cocktail/Parse/NodeKind.h"
#include "Cocktail/Parse/Tree.h"
#include "llvm/ADT/STLExtras.h"

namespace Cocktail::Parse {

enum class RelativeLocation : int8_t {
  Around,
  After,
  Before,
};

static auto operator<<(llvm::raw_ostream& out, RelativeLocation loc)
    -> llvm::raw_ostream& {
  switch (loc) {
    case RelativeLocation::Around:
      out << "around";
      break;
    case RelativeLocation::After:
      out << "after";
      break;
    case RelativeLocation::Before:
      out << "before";
      break;
  }
  return out;
}

Context::Context(Tree& tree, Lex::TokenizedBuffer& tokens,
                 Lex::TokenDiagnosticEmitter& emitter,
                 llvm::raw_ostream* vlog_stream)
    : tree_(&tree),
      tokens_(&tokens),
      emitter_(&emitter),
      vlog_stream_(vlog_stream),
      position_(tokens_->tokens().begin()),
      end_(tokens_->tokens().end()) {
  COCKTAIL_CHECK(position_ != end_) << "Empty TokenizedBuffer";
  --end_;
  COCKTAIL_CHECK(tokens_->GetKind(*end_) == Lex::TokenKind::EndOfFile)
      << "TokenizedBuffer should end with EndOfFile, ended with "
      << tokens_->GetKind(*end_);
}

auto Context::AddLeafNode(NodeKind kind, Lex::Token token, bool has_error)
    -> void {
// 解释为什么在添加叶子节点时，subtree_size 被设置为1：
// 
// 在解析树中，每个节点表示源代码中的一个语法结构或表达式。
// 这些节点形成了一个层次结构，其中某些节点可能包含子节点，
// 而另一些节点可能不包含子节点，这些不包含子节点的节点通常被称为叶子节点。
// 
// 对于叶子节点来说，它们通常代表源代码中的一个单一令牌，
// 例如标识符、关键字、运算符等。由于叶子节点不包含子节点，
// 因此它们的 subtree_size 被设置为1，表示它们只覆盖了自身。
// 这是因为在解析树中，叶子节点是树的末端，没有后代节点。
// 
// 解析树的增长方式通常是从根节点开始，然后根据源代码的结构
// 逐步添加节点，直到构建整个树。当解析器遇到源代码的某个部
// 分时，它会创建一个新的节点来表示该部分，并将其添加到解析
// 树中。如果该部分不包含子节点（例如一个标识符或常量），则
// 创建一个叶子节点，subtree_size 设置为1。如果该部分包含子
// 节点（例如一个表达式），则会创建一个非叶子节点，subtree_size
// 将取决于子节点的数量和它们的大小。
  tree_->node_impls_.push_back(
      Tree::NodeImpl(kind, has_error, token, /*subtree_size=*/1));
  if (has_error) {
    tree_->has_errors_ = true;
  }
}

auto Context::AddNode(NodeKind kind, Lex::Token token, int subtree_start,
                      bool has_error) -> void {
  int subtree_size = tree_->size() - subtree_start + 1;
  tree_->node_impls_.push_back(
      Tree::NodeImpl(kind, has_error, token, subtree_size));
  if (has_error) {
    tree_->has_errors_ = true;
  }
}

}  // namespace Cocktail::Parse