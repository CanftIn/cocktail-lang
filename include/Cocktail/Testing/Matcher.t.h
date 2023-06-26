#ifndef COCKTAIL_TESTING_MATCHER_T_H
#define COCKTAIL_TESTING_MATCHER_T_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/CppRefactor/MatcherManager.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Tooling/Tooling.h"

namespace Cocktail::Testing {

// Matcher test framework.
template <typename MatcherFactoryType>
class MatcherTestBase : public ::testing::Test {
 protected:
  MatcherTestBase() : matchers(&replacements) {
    matchers.Register(std::make_unique<MatcherFactoryType>());
  }

  // Expects that that the replacements produced by running the finder result in
  // the specified code transformation.
  void ExpectReplacement(llvm::StringRef before, llvm::StringRef after) {
    auto factory =
        clang::tooling::newFrontendActionFactory(matchers.GetFinder());
    constexpr char Filename[] = "test.cc";
    replacements.clear();
    replacements.insert({Filename, {}});
    ASSERT_TRUE(clang::tooling::runToolOnCodeWithArgs(
        factory->create(), before, {}, Filename, "clang-tool",
        std::make_shared<clang::PCHContainerOperations>(),
        clang::tooling::FileContentMappings()));
    EXPECT_THAT(replacements, testing::ElementsAre(testing::Key(Filename)));
    llvm::Expected<std::string> actual =
        clang::tooling::applyAllReplacements(before, replacements[Filename]);

    // Make a specific note if the matcher didn't make any changes.
    std::string unchanged;
    if (before == *actual) {
      unchanged = "NOTE: Actual matches original text, no changes made.";
    }

    if (after.find('\n') == std::string::npos) {
      EXPECT_THAT(*actual, testing::Eq(after.str())) << unchanged;
    } else {
      // Split lines to get gmock to get an easier-to-read error.
      llvm::SmallVector<llvm::StringRef, 0> actual_lines;
      llvm::SplitString(*actual, actual_lines, "\n");
      llvm::SmallVector<llvm::StringRef, 0> after_lines;
      llvm::SplitString(after, after_lines, "\n");
      EXPECT_THAT(actual_lines, testing::ContainerEq(after_lines)) << unchanged;
    }
  }

  Matcher::ReplacementMap replacements;
  MatcherManager matchers;
};

}  // namespace Cocktail::Testing

#endif  // COCKTAIL_TESTING_MATCHER_T_H