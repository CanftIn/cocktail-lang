#include "Cocktail/Parser/ParseTree.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <forward_list>

#include "../Lexer/TokenizedBuffer.t.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseNodeKind.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Parse.t.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

namespace {

using namespace Cocktail;
using Cocktail::Testing::IsKeyValueScalars;
using Cocktail::Testing::MatchParseTreeNodes;
using ::testing::Eq;
using ::testing::Ne;
using ::testing::NotNull;
using ::testing::StrEq;

struct ParseTreeTest : ::testing::Test {
  std::forward_list<SourceBuffer> source_storage;
  std::forward_list<TokenizedBuffer> token_storage;
  DiagnosticEmitter emitter = NullDiagnosticEmitter();

  auto GetSourceBuffer(llvm::Twine t) -> SourceBuffer& {
    source_storage.push_front(
        std::move(*SourceBuffer::CreateFromText(t.str())));
    return source_storage.front();
  }

  auto GetTokenizedBuffer(llvm::Twine t) -> TokenizedBuffer& {
    token_storage.push_front(TokenizedBuffer::Lex(GetSourceBuffer(t), emitter));
    return token_storage.front();
  }
};

TEST_F(ParseTreeTest, Empty) {
  TokenizedBuffer tokens = GetTokenizedBuffer("");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_FALSE(tree.HasErrors());
  EXPECT_THAT(tree.Postorder().begin(), Eq(tree.Postorder().end()));
}

TEST_F(ParseTreeTest, EmptyDeclaration) {
  TokenizedBuffer tokens = GetTokenizedBuffer(";");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_FALSE(tree.HasErrors());
  auto it = tree.Postorder().begin();
  auto end = tree.Postorder().end();
  ASSERT_THAT(it, Ne(end));
  ParseTree::Node n = *it++;
  EXPECT_THAT(it, Eq(end));

  EXPECT_FALSE(tree.HasErrorInNode(n));
  EXPECT_THAT(tree.GetNodeKind(n), Eq(ParseNodeKind::EmptyDeclaration()));
  auto t = tree.GetNodeToken(n);
  ASSERT_THAT(tokens.Tokens().begin(), Ne(tokens.Tokens().end()));
  EXPECT_THAT(t, Eq(*tokens.Tokens().begin()));
  EXPECT_THAT(tokens.GetTokenText(t), Eq(";"));

  EXPECT_THAT(tree.Postorder(n).begin(), Eq(tree.Postorder().begin()));
  EXPECT_THAT(tree.Postorder(n).end(), Eq(tree.Postorder().end()));
  EXPECT_THAT(tree.Children(n).begin(), Eq(tree.Children(n).end()));
}

TEST_F(ParseTreeTest, BasicFunctionDeclaration) {
  TokenizedBuffer tokens = GetTokenizedBuffer("fn F();");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_FALSE(tree.HasErrors());
  EXPECT_THAT(
      tree, MatchParseTreeNodes(
                {{.kind = ParseNodeKind::FunctionDeclaration(),
                  .text = "fn",
                  .children = {
                      {ParseNodeKind::Identifier(), "F"},
                      {.kind = ParseNodeKind::ParameterList(),
                       .text = "(",
                       .children = {{ParseNodeKind::ParameterListEnd(), ")"}}},
                      {ParseNodeKind::DeclarationEnd(), ";"}}}}));
}

TEST_F(ParseTreeTest, BasicFunctionDefinition) {
  TokenizedBuffer tokens = GetTokenizedBuffer(
      "fn F() {\n"
      "}");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_FALSE(tree.HasErrors());
  EXPECT_THAT(
      tree, MatchParseTreeNodes(
                {{.kind = ParseNodeKind::FunctionDeclaration(),
                  .children = {
                      {ParseNodeKind::Identifier(), "F"},
                      {.kind = ParseNodeKind::ParameterList(),
                       .children = {{ParseNodeKind::ParameterListEnd()}}},
                      {.kind = ParseNodeKind::CodeBlock(),
                       .text = "{",
                       .children = {{ParseNodeKind::CodeBlockEnd(), "}"}}}}}}));
}

TEST_F(ParseTreeTest, FunctionDefinitionWithNestedBlocks) {
  TokenizedBuffer tokens = GetTokenizedBuffer(
      "fn F() {\n"
      "  {\n"
      "    {{}}\n"
      "  }\n"
      "}");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_FALSE(tree.HasErrors());
  EXPECT_THAT(
      tree,
      MatchParseTreeNodes(
          {{.kind = ParseNodeKind::FunctionDeclaration(),
            .children = {
                {ParseNodeKind::Identifier(), "F"},
                {.kind = ParseNodeKind::ParameterList(),
                 .children = {{ParseNodeKind::ParameterListEnd()}}},
                {.kind = ParseNodeKind::CodeBlock(),
                 .children = {
                     {.kind = ParseNodeKind::CodeBlock(),
                      .children = {{.kind = ParseNodeKind::CodeBlock(),
                                    .children =
                                        {{.kind = ParseNodeKind::CodeBlock(),
                                          .children = {{ParseNodeKind::
                                                            CodeBlockEnd()}}},
                                         {ParseNodeKind::CodeBlockEnd()}}},
                                   {ParseNodeKind::CodeBlockEnd()}}},
                     {ParseNodeKind::CodeBlockEnd()}}}}}}));
}

TEST_F(ParseTreeTest, FunctionDefinitionWithIdenifierInStatements) {
  TokenizedBuffer tokens = GetTokenizedBuffer(
      "fn F() {\n"
      "  bar\n"
      "}");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_TRUE(tree.HasErrors());
  EXPECT_THAT(
      tree,
      MatchParseTreeNodes(
          {{.kind = ParseNodeKind::FunctionDeclaration(),
            .children = {{ParseNodeKind::Identifier(), "F"},
                         {.kind = ParseNodeKind::ParameterList(),
                          .children = {{ParseNodeKind::ParameterListEnd()}}},
                         {.kind = ParseNodeKind::CodeBlock(),
                          .has_error = true,
                          .children = {{ParseNodeKind::CodeBlockEnd()}}}}}}));
}

TEST_F(ParseTreeTest, FunctionDefinitionWithIdenifierInNestedBlock) {
  TokenizedBuffer tokens = GetTokenizedBuffer(
      "fn F() {\n"
      "  {bar}\n"
      "}");
  ParseTree tree = ParseTree::Parse(tokens, emitter);
  EXPECT_TRUE(tree.HasErrors());
  EXPECT_THAT(
      tree,
      MatchParseTreeNodes(
          {{.kind = ParseNodeKind::FunctionDeclaration(),
            .children = {
                {ParseNodeKind::Identifier(), "F"},
                {.kind = ParseNodeKind::ParameterList(),
                 .children = {{ParseNodeKind::ParameterListEnd()}}},
                {.kind = ParseNodeKind::CodeBlock(),
                 .children = {{.kind = ParseNodeKind::CodeBlock(),
                               .has_error = true,
                               .children = {{ParseNodeKind::CodeBlockEnd()}}},
                              {ParseNodeKind::CodeBlockEnd()}}}}}}));
}

}  // namespace