#include "Cocktail/CppRefactor/FnInserter.h"

#include "Cocktail/Testing/Matcher.t.h"

namespace {

using namespace Cocktail;

class FnInserterTest : public MatcherTestBase {
 protected:
  FnInserterTest() : fn_inserter(replacements, &finder) {}

  Cocktail::FnInserter fn_inserter;
};

TEST_F(FnInserterTest, TrailingReturn) {
  constexpr char Before[] = "auto A() -> int;";
  constexpr char After[] = "fn A() -> int;";
  ExpectReplacement(Before, After);
}

}  // namespace