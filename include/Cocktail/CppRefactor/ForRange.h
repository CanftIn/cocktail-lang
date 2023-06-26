#ifndef COCKTAIL_CPP_REFACTOR_FOR_RANGE_H
#define COCKTAIL_CPP_REFACTOR_FOR_RANGE_H

#include "Cocktail/CppRefactor/Matcher.h"

namespace Cocktail {

class ForRange : public Matcher {
 public:
  using Matcher::Matcher;
  void Run() override;

 private:
  auto GetTypeStr(const clang::VarDecl& decl) -> std::string;
};

class ForRangeFactory : public MatcherFactoryBase<ForRange> {
 public:
  void AddMatcher(
      clang::ast_matchers::MatchFinder* finder,
      clang::ast_matchers::MatchFinder::MatchCallback* callback) override;
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_FOR_RANGE_H