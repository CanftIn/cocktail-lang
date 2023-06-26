#ifndef COCKTAIL_TESTING_PARSE_T_H
#define COCKTAIL_TESTING_PARSE_T_H

#include <gmock/gmock.h>

#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Parser/ParseTree.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

inline void PrintTo(const ParseTree& tree, std::ostream* output) {
  std::string text;
  llvm::raw_string_ostream text_stream(text);
  tree.Print(text_stream);
  *output << "\n" << text_stream.str() << "\n";
}

namespace Testing {

struct ExpectedNode {
  ParseNodeKind kind = ParseNodeKind::EmptyDeclaration();
  std::string text;
  bool has_error = false;
  bool skip_subtree = false;
  std::vector<ExpectedNode> children;
};

class ExpectedNodesMatcher
    : public ::testing::MatcherInterface<const ParseTree&> {
 public:
  explicit ExpectedNodesMatcher(
      llvm::SmallVector<ExpectedNode, 0> expected_nodess)
      : expected_nodes_(std::move(expected_nodess)) {}

  auto MatchAndExplain(const ParseTree& tree,
                       ::testing::MatchResultListener* output_ptr) const
      -> bool override;
  auto DescribeTo(std::ostream* output_ptr) const -> void override;

 private:
  auto MatchExpectedNode(const ParseTree& tree, ParseTree::Node n,
                         int postorder_index, const ExpectedNode& expected_node,
                         ::testing::MatchResultListener& output) const -> bool;

  llvm::SmallVector<ExpectedNode, 0> expected_nodes_;
};

inline auto ExpectedNodesMatcher::MatchAndExplain(
    const ParseTree& tree, ::testing::MatchResultListener* output_ptr) const
    -> bool {
  auto& output = *output_ptr;
  bool matches = true;
  const auto rpo = llvm::reverse(tree.postorder());
  const auto nodes_begin = rpo.begin();
  const auto nodes_end = rpo.end();
  auto nodes_it = nodes_begin;
  llvm::SmallVector<const ExpectedNode*, 16> expected_node_stack;
  for (const ExpectedNode& en : expected_nodes_) {
    expected_node_stack.push_back(&en);
  }
  while (!expected_node_stack.empty()) {
    if (nodes_it == nodes_end) {
      // We'll check the size outside the loop.
      break;
    }

    ParseTree::Node n = *nodes_it++;
    int postorder_index = n.index();

    const ExpectedNode& expected_node = *expected_node_stack.pop_back_val();

    if (!MatchExpectedNode(tree, n, postorder_index, expected_node, output)) {
      matches = false;
    }

    if (expected_node.skip_subtree) {
      COCKTAIL_CHECK(expected_node.children.empty())
          << "Must not skip an expected subtree while specifying expected "
             "children!";
      nodes_it = llvm::reverse(tree.postorder(n)).end();
      continue;
    }

    int num_children =
        std::distance(tree.children(n).begin(), tree.children(n).end());
    if (num_children != static_cast<int>(expected_node.children.size())) {
      output
          << "\nParse node (postorder index #" << postorder_index << ") has "
          << num_children << " children, expected "
          << expected_node.children.size()
          << ". Skipping this subtree to avoid any unsynchronized tree walk.";
      matches = false;
      nodes_it = llvm::reverse(tree.postorder(n)).end();
      continue;
    }

    for (const ExpectedNode& child_expected_node : expected_node.children) {
      expected_node_stack.push_back(&child_expected_node);
    }
  }

  if (nodes_it != nodes_end) {
    COCKTAIL_CHECK(expected_node_stack.empty())
        << "If we have unmatched nodes in the input tree, should only finish "
           "having fully processed expected tree.";
    output << "\nFinished processing expected nodes and there are still "
           << (nodes_end - nodes_it) << " unexpected nodes.";
    matches = false;
  } else if (!expected_node_stack.empty()) {
    output << "\nProcessed all " << (nodes_end - nodes_begin)
           << " nodes and still have " << expected_node_stack.size()
           << " expected nodes that were unmatched.";
    matches = false;
  }

  return matches;
}

inline auto ExpectedNodesMatcher::DescribeTo(std::ostream* output_ptr) const
    -> void {
  auto& output = *output_ptr;
  output << "Matches expected node pattern:\n[\n";

  llvm::SmallVector<std::pair<const ExpectedNode*, int>, 16>
      expected_node_stack;
  for (const ExpectedNode& expected_node : llvm::reverse(expected_nodes_)) {
    expected_node_stack.push_back({&expected_node, 0});
  }

  while (!expected_node_stack.empty()) {
    const ExpectedNode& expected_node = *expected_node_stack.back().first;
    int depth = expected_node_stack.back().second;
    expected_node_stack.pop_back();
    for (int indent_count = 0; indent_count < depth; ++indent_count) {
      output << "  ";
    }
    output << "{kind: '" << expected_node.kind.name().str() << "'";
    if (!expected_node.text.empty()) {
      output << ", text: '" << expected_node.text << "'";
    }
    if (expected_node.has_error) {
      output << ", has_error: yes";
    }
    if (expected_node.skip_subtree) {
      output << ", skip_subtree: yes";
    }

    if (!expected_node.children.empty()) {
      COCKTAIL_CHECK(!expected_node.skip_subtree)
          << "Must not have children and skip a subtree!";
      output << ", children: [\n";
      for (const ExpectedNode& child_expected_node :
           llvm::reverse(expected_node.children)) {
        expected_node_stack.push_back({&child_expected_node, depth + 1});
      }
      continue;
    }

    output << "}";
    if (!expected_node_stack.empty()) {
      COCKTAIL_CHECK(depth >= expected_node_stack.back().second)
          << "Cannot have an increase in depth on a leaf node!";
      int pop_depth = depth - expected_node_stack.back().second;
      for (int pop_count = 0; pop_count < pop_depth; ++pop_count) {
        output << "]}";
      }
    }
    output << "\n";
  }
  output << "]\n";
}

inline auto ExpectedNodesMatcher::MatchExpectedNode(
    const ParseTree& tree, ParseTree::Node n, int postorder_index,
    const ExpectedNode& expected_node,
    ::testing::MatchResultListener& output) const -> bool {
  bool matches = true;

  ParseNodeKind kind = tree.node_kind(n);
  if (kind != expected_node.kind) {
    output << "\nParse node (postorder index #" << postorder_index << ") is a "
           << kind.name().str() << ", expected a "
           << expected_node.kind.name().str() << ".";
    matches = false;
  }

  if (tree.node_has_error(n) != expected_node.has_error) {
    output << "\nParse node (postorder index #" << postorder_index << ") "
           << (tree.node_has_error(n) ? "has an error"
                                      : "does not have an error")
           << ", expected that it "
           << (expected_node.has_error ? "has an error"
                                       : "does not have an error")
           << ".";
    matches = false;
  }

  llvm::StringRef node_text = tree.GetNodeText(n);
  if (!expected_node.text.empty() && node_text != expected_node.text) {
    output << "\nParse node (postorder index #" << postorder_index
           << ") is spelled '" << node_text.str() << "', expected '"
           << expected_node.text << "'.";
    matches = false;
  }

  return matches;
}

inline auto MatchParseTreeNodes(
    llvm::SmallVector<ExpectedNode, 0> expected_nodes)
    -> ::testing::Matcher<const ParseTree&> {
  return ::testing::MakeMatcher(
      new ExpectedNodesMatcher(std::move(expected_nodes)));
}

// Matcher argument for a node with errors.
struct HasErrorTag {};
inline constexpr HasErrorTag HasError;

// Matcher argument to skip checking the children of a node.
struct AnyChildrenTag {};
inline constexpr AnyChildrenTag AnyChildren;

// A function to generate ExpectedNodes a little more tersely and readably. The
// meaning of each argument is inferred from its type.
template <typename... Args>
auto MatchNode(Args... args) -> ExpectedNode {
  struct ArgHandler {
    ExpectedNode expected;
    void UpdateExpectationsForArg(ParseNodeKind kind) { expected.kind = kind; }
    void UpdateExpectationsForArg(std::string text) {
      expected.text = std::move(text);
    }
    void UpdateExpectationsForArg(HasErrorTag /*tag*/) {
      expected.has_error = true;
    }
    void UpdateExpectationsForArg(AnyChildrenTag /*tag*/) {
      expected.skip_subtree = true;
    }
    void UpdateExpectationsForArg(ExpectedNode node) {
      expected.children.push_back(std::move(node));
    }
  };
  ArgHandler handler;
  (handler.UpdateExpectationsForArg(args), ...);
  return handler.expected;
}

#define COCKTAIL_PARSE_NODE_KIND(kind)                           \
  template <typename... Args>                                    \
  auto Match##kind(Args... args) -> ExpectedNode {               \
    return MatchNode(ParseNodeKind::kind(), std::move(args)...); \
  }
#include "Cocktail/Parser/ParseNodeKind.def"

// Helper for matching a designator `lhs.rhs`.
inline auto MatchDesignator(ExpectedNode lhs, std::string rhs) -> ExpectedNode {
  return MatchDesignatorExpression(std::move(lhs),
                                   MatchDesignatedName(std::move(rhs)));
}

template <typename... Args>
auto MatchParameters(Args... args) -> ExpectedNode {
  return MatchParameterList("(", std::move(args)..., MatchParameterListEnd());
}

template <typename... Args>
auto MatchFunctionWithBody(Args... args) -> ExpectedNode {
  return MatchFunctionDeclaration(
      MatchDeclaredName(), MatchParameters(),
      MatchCodeBlock(std::move(args)..., MatchCodeBlockEnd()));
}

}  // namespace Testing

}  // namespace Cocktail

#endif  // COCKTAIL_TESTING_PARSE_T_H