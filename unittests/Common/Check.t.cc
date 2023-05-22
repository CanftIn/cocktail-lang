#include "Cocktail/Common/Check.h"

#include <gtest/gtest.h>

namespace {

using namespace Cocktail;

TEST(CheckTest, CheckTrue) { CHECK(true); }

TEST(CheckTest, CheckFalse) {
  ASSERT_DEATH({ CHECK(false); }, "LLVM ERROR: CHECK failure: false");
}

}  // namespace