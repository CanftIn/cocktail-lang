#include "Cocktail/CppRefactor/ForRange.h"

#include "Cocktail/Testing/Matcher.t.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;

class ForRangeTest : public MatcherTestBase<ForRangeFactory> {};

TEST_F(ForRangeTest, Basic) {
  constexpr char Before[] = R"cpp(
    void Foo() {
      int items[] = {1};
      for (int i : items) {
      }
    }
  )cpp";
  constexpr char After[] = R"(
    void Foo() {
      int items[] = {1};
      for (int i  in  items) {
      }
    }
  )";
  ExpectReplacement(Before, After);
}

TEST_F(ForRangeTest, NoSpace) {
  // Do not mark `cpp` so that clang-format won't "fix" the `:` spacing.
  constexpr char Before[] = R"(
    void Foo() {
      int items[] = {1};
      for (int i:items) {
      }
    }
  )";
  constexpr char After[] = R"(
    void Foo() {
      int items[] = {1};
      for (int i in items) {
      }
    }
  )";
  ExpectReplacement(Before, After);
}

}  // namespace