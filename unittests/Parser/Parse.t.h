#ifndef COCKTAIL_PARSER_PARSE_T_H
#define COCKTAIL_PARSER_PARSE_T_H

#include <gmock/gmock.h>

#include <iterator>
#include <ostream>
#include <string>
#include <vector>

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
      : expected_nodes(std::move(expected_nodess)) {}

  auto MatchAndExplain(const ParseTree& tree,
                       ::testing::MatchResultListener* output_ptr) const
      -> bool override;
  auto DescribeTo(std::ostream* output_ptr) const -> void override;

 private:
  auto MatchExpectedNode(const ParseTree& tree, ParseTree::Node n,
                         int postorder_index, const ExpectedNode& expected_node,
                         ::testing::MatchResultListener& output) const -> bool;

  llvm::SmallVector<ExpectedNode, 0> expected_nodes;
};

inline auto ExpectedNodesMatcher::MatchAndExplain(
    const ParseTree& tree, ::testing::MatchResultListener* output_ptr) const
    -> bool {
  auto& output = *output_ptr;
  bool matches = true;
  const auto rpo = llvm::reverse(tree.Postorder());
  const auto nodes_begin = rpo.begin();
  const auto nodes_end = rpo.end();
  auto nodes_it = nodes_begin;
  llvm::SmallVector<const ExpectedNode*, 16> expected_node_stack;
  for (const ExpectedNode& en : expected_nodes) {
    expected_node_stack.push_back(&en);
  }
  while (!expected_node_stack.empty()) {
    if (nodes_it == nodes_end) {
      break;
    }

    ParseTree::Node n = *nodes_it++;
    int postorder_index = n.GetIndex();

    const ExpectedNode& expected_node = *expected_node_stack.pop_back_val();

    if (!MatchExpectedNode(tree, n, postorder_index, expected_node, output)) {
      matches = false;
    }

    if (expected_node.skip_subtree) {
      assert(expected_node.children.empty() &&
             "Must not skip an expected subtree while specifying expected "
             "children!");
      nodes_it = llvm::reverse(tree.Postorder(n)).end();
      continue;
    }

    int num_children =
        std::distance(tree.Children(n).begin(), tree.Children(n).end());
    if (num_children != static_cast<int>(expected_node.children.size())) {
      output
          << "\nParse node (postorder index #" << postorder_index << ") has "
          << num_children << " children, expected "
          << expected_node.children.size()
          << ". Skipping this subtree to avoid any unsynchronized tree walk.";
      matches = false;
      nodes_it = llvm::reverse(tree.Postorder(n)).end();
      continue;
    }

    for (const ExpectedNode& child_expected_node : expected_node.children) {
      expected_node_stack.push_back(&child_expected_node);
    }
  }

  if (nodes_it != nodes_end) {
    assert(expected_node_stack.empty() &&
           "If we have unmatched nodes in the input tree, should only finish "
           "having fully processed expected tree.");
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
  for (const ExpectedNode& expected_node : llvm::reverse(expected_nodes)) {
    expected_node_stack.push_back({&expected_node, 0});
  }

  while (!expected_node_stack.empty()) {
    const ExpectedNode& expected_node = *expected_node_stack.back().first;
    int depth = expected_node_stack.back().second;
    expected_node_stack.pop_back();
    for (int indent_count = 0; indent_count < depth; ++indent_count) {
      output << "  ";
    }
    output << "{kind: '" << expected_node.kind.GetName().str() << "'";
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
      assert(!expected_node.skip_subtree &&
             "Must not have children and skip a subtree!");
      output << ", children: [\n";
      for (const ExpectedNode& child_expected_node :
           llvm::reverse(expected_node.children)) {
        expected_node_stack.push_back({&child_expected_node, depth + 1});
      }
      continue;
    }

    output << "}";
    if (!expected_node_stack.empty()) {
      assert(depth >= expected_node_stack.back().second &&
             "Cannot have an increase in depth on a leaf node!");
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

  ParseNodeKind kind = tree.GetNodeKind(n);
  if (kind != expected_node.kind) {
    output << "\nParse node (postorder index #" << postorder_index << ") is a "
           << kind.GetName().str() << ", expected a "
           << expected_node.kind.GetName().str() << ".";
    matches = false;
  }

  if (tree.HasErrorInNode(n) != expected_node.has_error) {
    output << "\nParse node (postorder index #" << postorder_index << ") "
           << (tree.HasErrorInNode(n) ? "has an error"
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

}  // namespace Testing

}  // namespace Cocktail

#endif  // COCKTAIL_PARSER_PARSE_T_H