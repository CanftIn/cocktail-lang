#ifndef COCKTAIL_CPP_REFACTOR_FN_INSERTER_H
#define COCKTAIL_CPP_REFACTOR_FN_INSERTER_H

#include "Cocktail/CppRefactor/Matcher.h"

namespace Cocktail {

class FnInserter : public Matcher {
 public:
  using Matcher::Matcher;
  void Run() override;
};

class FnInserterFactory : public MatcherFactoryBase<FnInserter> {
 public:
  void AddMatcher(
      clang::ast_matchers::MatchFinder* finder,
      clang::ast_matchers::MatchFinder::MatchCallback* callback) override;
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_FN_INSERTER_H