#include "Cocktail/Common/MetaProgramming.h"

#include <gtest/gtest.h>

#include "llvm/Support/raw_ostream.h"

namespace Cocktail {
namespace {

TEST(MetaProgrammingTest, RequiresBasic) {
  bool result = Requires<int, int>([](int a, int b) { return a + b; });
  EXPECT_TRUE(result);
}

struct TypeWithPrint {
  void Print(llvm::raw_ostream& os) const { os << "Test"; }
};

TEST(MetaProgrammingTest, RequiresPrintMethod) {
  bool result = Requires<const TypeWithPrint, llvm::raw_ostream>(
      [](auto&& t, auto&& out) -> decltype(t.Print(out)) {});
  EXPECT_TRUE(result);
}

}  // namespace
}  // namespace Cocktail