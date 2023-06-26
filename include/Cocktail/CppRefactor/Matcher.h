#ifndef COCKTAIL_CPP_REFACTOR_MATCHER_H
#define COCKTAIL_CPP_REFACTOR_MATCHER_H

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/Core/Replacement.h"

namespace Cocktail {

class Matcher {
 public:
  using ReplacementMap = std::map<std::string, clang::tooling::Replacements>;

  Matcher(const clang::ast_matchers::MatchFinder::MatchResult* in_match_result,
          ReplacementMap* in_replacements)
      : match_result(in_match_result), replacements(in_replacements) {}
  virtual ~Matcher() = default;

  virtual void Run() = 0;

 protected:
  void AddReplacement(clang::CharSourceRange range,
                      llvm::StringRef replacement_text);

  template <typename NodeType>
  auto GetNodeAsOrDie(llvm::StringRef id) -> const NodeType& {
    auto* node = match_result->Nodes.getNodeAs<NodeType>(id);
    if (!node) {
      llvm::report_fatal_error(std::string("getNodeAs failed for ") + id);
    }
    return *node;
  }

  auto GetLangOpts() -> const clang::LangOptions& {
    return match_result->Context->getLangOpts();
  }

  auto GetSourceManager() -> const clang::SourceManager& {
    return *match_result->SourceManager;
  }

  auto GetSourceText(clang::CharSourceRange range) -> llvm::StringRef {
    return clang::Lexer::getSourceText(range, GetSourceManager(),
                                       GetLangOpts());
  }

 private:
  const clang::ast_matchers::MatchFinder::MatchResult* const match_result;
  ReplacementMap* const replacements;
};

class MatcherFactory {
 public:
  virtual ~MatcherFactory() = default;

  virtual auto CreateMatcher(
      const clang::ast_matchers::MatchFinder::MatchResult* match_result,
      Matcher::ReplacementMap* replacements) -> std::unique_ptr<Matcher> = 0;

  virtual void AddMatcher(
      clang::ast_matchers::MatchFinder* finder,
      clang::ast_matchers::MatchFinder::MatchCallback* callback) = 0;
};

template <typename MatcherType>
class MatcherFactoryBase : public MatcherFactory {
 public:
  auto CreateMatcher(
      const clang::ast_matchers::MatchFinder::MatchResult* match_result,
      Matcher::ReplacementMap* replacements)
      -> std::unique_ptr<Matcher> override {
    return std::make_unique<MatcherType>(match_result, replacements);
  }
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_MATCHER_H
