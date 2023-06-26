#ifndef COCKTAIL_CPP_REFACTOR_MATCHER_MANAGER_H
#define COCKTAIL_CPP_REFACTOR_MATCHER_MANAGER_H

#include "Cocktail/CppRefactor/Matcher.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

namespace Cocktail {

// Manages registration of AST matchers.
class MatcherManager {
 public:
  explicit MatcherManager(Matcher::ReplacementMap* in_replacements)
      : replacements(in_replacements) {}

  // Registers Matcher implementations.
  void Register(std::unique_ptr<MatcherFactory> factory) {
    matchers.push_back(std::make_unique<MatchCallbackWrapper>(
        &finder, std::move(factory), replacements));
  }

  auto GetFinder() -> clang::ast_matchers::MatchFinder* { return &finder; }

 private:
  // Adapts Matcher for use with MatchCallback.
  class MatchCallbackWrapper
      : public clang::ast_matchers::MatchFinder::MatchCallback {
   public:
    explicit MatchCallbackWrapper(clang::ast_matchers::MatchFinder* finder,
                                  std::unique_ptr<MatcherFactory> in_factory,
                                  Matcher::ReplacementMap* in_replacements)
        : factory(std::move(in_factory)), replacements(in_replacements) {
      factory->AddMatcher(finder, this);
    }

    void run(const clang::ast_matchers::MatchFinder::MatchResult& match_result)
        override {
      factory->CreateMatcher(&match_result, replacements)->Run();
    }

   private:
    std::unique_ptr<MatcherFactory> factory;
    Matcher::ReplacementMap* const replacements;
  };

  Matcher::ReplacementMap* const replacements;
  clang::ast_matchers::MatchFinder finder;
  std::vector<std::unique_ptr<MatcherFactory>> factories;
  std::vector<std::unique_ptr<MatchCallbackWrapper>> matchers;
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_MATCHER_MANAGER_H