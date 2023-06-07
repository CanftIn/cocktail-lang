#include "Cocktail/Testing/Matcher.t.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "clang/Tooling/Tooling.h"

namespace ct = ::clang::tooling;

namespace Cocktail {

void MatcherTestBase::ExpectReplacement(llvm::StringRef before,
                                        llvm::StringRef after) {
  auto factory = ct::newFrontendActionFactory(&finder);
  constexpr char Filename[] = "test.cc";
  replacements.clear();
  replacements.insert({Filename, {}});
  ASSERT_TRUE(ct::runToolOnCodeWithArgs(
      factory->create(), before, {}, Filename, "clang-tool",
      std::make_shared<clang::PCHContainerOperations>(),
      ct::FileContentMappings()));
  EXPECT_THAT(replacements, ::testing::ElementsAre(testing::Key(Filename)));
  auto actual = ct::applyAllReplacements(before, replacements[Filename]);
  llvm::SmallVector<llvm::StringRef, 0> actual_lines;
  llvm::SplitString(*actual, actual_lines, "\n");
  llvm::SmallVector<llvm::StringRef, 0> after_lines;
  llvm::SplitString(after, after_lines, "\n");
  EXPECT_THAT(actual_lines, ::testing::ContainerEq(after_lines));
}

}  // namespace Cocktail