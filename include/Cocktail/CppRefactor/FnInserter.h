#ifndef COCKTAIL_CPP_REFACTOR_FN_INSERTER_H
#define COCKTAIL_CPP_REFACTOR_FN_INSERTER_H

#include "Cocktail/CppRefactor/Matcher.h"

namespace Cocktail {

class FnInserter : public Matcher {
 public:
  explicit FnInserter(std::map<std::string, Replacements>& in_replacements,
                      MatchFinder* finder);

  void run(const MatchFinder::MatchResult& result) override;

 private:
  static constexpr char Label[] = "FnInserter";
};

}  // namespace Cocktail

#endif  // COCKTAIL_CPP_REFACTOR_FN_INSERTER_H