#ifndef COCKTAIL_CPP_REFACTOR_MATCHER_H
#define COCKTAIL_CPP_REFACTOR_MATCHER_H

#include <map>
#include <string>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

namespace Cocktail {

class Matcher : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  using MatchFinder = clang::ast_matchers::MatchFinder;
  using Replacements = clang::tooling::Replacements;

  explicit Matcher(std::map<std::string, Replacements>& in_replacements)
      : replacements(&in_replacements) {}

 protected:
  void AddReplacement(const clang::SourceManager& sm,
                      clang::CharSourceRange range,
                      llvm::StringRef replacement_text);
 private:
  std::map<std::string, Replacements>* const replacements;
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_MATCHER_H
