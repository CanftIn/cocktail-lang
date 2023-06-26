#ifndef COCKTAIL_CPP_REFACTOR_VAR_DECL_H
#define COCKTAIL_CPP_REFACTOR_VAR_DECL_H

#include "Cocktail/CppRefactor/Matcher.h"

namespace Cocktail {

// Updates variable declarations for `var name: Type`.
class VarDecl : public Matcher {
 public:
  using Matcher::Matcher;
  void Run() override;

 private:
  auto GetTypeStr(const clang::VarDecl& decl) -> std::string;
};

class VarDeclFactory : public MatcherFactoryBase<VarDecl> {
 public:
  void AddMatcher(
      clang::ast_matchers::MatchFinder* finder,
      clang::ast_matchers::MatchFinder::MatchCallback* callback) override;
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_VAR_DECL_H