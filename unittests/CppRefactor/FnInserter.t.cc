#include "Cocktail/CppRefactor/FnInserter.h"

#include "Cocktail/Testing/Matcher.t.h"

namespace {

using namespace Cocktail;
using namespace Cocktail::Testing;

class FnInserterTest : public MatcherTestBase<FnInserterFactory> {};

TEST_F(FnInserterTest, TrailingReturn) {
  constexpr char Before[] = "auto A() -> int;";
  constexpr char After[] = "fn A() -> int;";
  ExpectReplacement(Before, After);
}

TEST_F(FnInserterTest, Inline) {
  constexpr char Before[] = "inline auto A() -> int;";
  constexpr char After[] = "fn inline A() -> int;";
  ExpectReplacement(Before, After);
}

TEST_F(FnInserterTest, Void) {
  constexpr char Before[] = "void A();";
  constexpr char After[] = "fn A();";
  ExpectReplacement(Before, After);
}

TEST_F(FnInserterTest, Methods) {
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

    void Shape::Draw() {}
  )cpp";
  constexpr char After[] = R"(
    class Shape {
     public:
      fn virtual Draw() = 0;
      fn virtual NumSides() -> int = 0;
    };

    class Circle : public Shape {
     public:
      fn Draw() override;
      fn NumSides() -> int override;
      fn Radius() -> double { return radius_; }

     private:
      double radius_;
    };

    fn Shape::Draw() {}
  )";
  ExpectReplacement(Before, After);
}

TEST_F(FnInserterTest, ConstructorDestructor) {
  constexpr char Before[] = R"cpp(
    class Shape {
     public:
      Shape() {}
      ~Shape() {}
    };
  )cpp";
  ExpectReplacement(Before, Before);
}

TEST_F(FnInserterTest, LegacyReturn) {
  constexpr char Before[] = "int A();";
  ExpectReplacement(Before, Before);
}


}  // namespace