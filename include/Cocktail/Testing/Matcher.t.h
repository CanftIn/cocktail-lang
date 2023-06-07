#ifndef COCKTAIL_TESTING_MATCHER_T_H
#define COCKTAIL_TESTING_MATCHER_T_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

namespace Cocktail {

class MatcherTestBase : public ::testing::Test {
 protected:
  void ExpectReplacement(llvm::StringRef before, llvm::StringRef after);

  std::map<std::string, clang::tooling::Replacements> replacements;
  clang::ast_matchers::MatchFinder finder;
};

}  // namespace Cocktail

#endif  // COCKTAIL_TESTING_MATCHER_T_H