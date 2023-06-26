#include "Cocktail/CppRefactor/FnInserter.h"

#include "Cocktail/Testing/Matcher.t.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;

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

TEST_F(FnInserterTest, Inline) {
  constexpr char Before[] = "inline auto A() -> int;";
  constexpr char After[] = "fn auto A() -> int;";
  ExpectReplacement(Before, After);
}

TEST_F(FnInserterTest, Methods) {
  // "virtual auto" should be "fn virtual"
  constexpr char Before[] = R"cpp(
    class Shape {
     public:
      virtual void Draw() = 0;
      virtual auto NumSides() -> int = 0;
    };

    class Circle : public Shape {
     public:
      void Draw() override;
      auto NumSides() -> int override;
      auto Radius() -> double { return radius_; }

     private:
      double radius_;
    };
  )cpp";
  constexpr char After[] = R"(
    class Shape {
     public:
      virtual void Draw() = 0;
      fn auto NumSides() -> int = 0;
    };

    class Circle : public Shape {
     public:
      void Draw() override;
      fn NumSides() -> int override;
      fn Radius() -> double { return radius_; }

     private:
      double radius_;
    };
  )";
  ExpectReplacement(Before, After);
}

}  // namespace