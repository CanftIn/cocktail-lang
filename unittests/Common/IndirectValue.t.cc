#include "Cocktail/Common/IndirectValue.h"

#include <gtest/gtest.h>

#include <string>

namespace Cocktail {
namespace {

TEST(IndirectValueTest, DefaultConstructor) {
  IndirectValue<std::string> value;
  EXPECT_EQ(*value, "");
}

TEST(IndirectValueTest, ValueConstructor) {
  IndirectValue<std::string> value("Hello, world!");
  EXPECT_EQ(*value, "Hello, world!");
}

TEST(IndirectValueTest, CopyConstructor) {
  IndirectValue<std::string> value("Hello, world!");
  IndirectValue<std::string> copy(value);
  EXPECT_EQ(*copy, "Hello, world!");
}

TEST(IndirectValueTest, MoveConstructor) {
  IndirectValue<std::string> value("Hello, world!");
  IndirectValue<std::string> copy(std::move(value));
  EXPECT_EQ(*copy, "Hello, world!");
}

TEST(IndirectValueTest, CopyAssignment) {
  IndirectValue<std::string> value("Hello, world!");
  IndirectValue<std::string> copy;
  copy = value;
  EXPECT_EQ(*copy, "Hello, world!");
}

TEST(IndirectValueTest, MoveAssignment) {
  IndirectValue<std::string> value("Hello, world!");
  IndirectValue<std::string> copy;
  copy = std::move(value);
  EXPECT_EQ(*copy, "Hello, world!");
}

TEST(IndirectValueTest, PointerAccess) {
  IndirectValue<std::string> value("Hello, world!");
  EXPECT_EQ(value->size(), 13);
}

TEST(IndirectValueTest, ConstPointerAccess) {
  const IndirectValue<std::string> value("Hello, world!");
  EXPECT_EQ(value->size(), 13);
}

TEST(IndirectValueTest, GetPointer) {
  IndirectValue<std::string> value("Hello, world!");
  EXPECT_EQ(value.GetPointer()->size(), 13);
}

TEST(IndirectValueTest, ConstGetPointer) {
  const IndirectValue<std::string> value("Hello, world!");
  EXPECT_EQ(value.GetPointer()->size(), 13);
}

TEST(IndirectValueTest, ConstAccess) {
  const IndirectValue<int> v = 42;
  EXPECT_EQ(*v, 42);
  EXPECT_EQ(v.GetPointer(), &*v);
}

TEST(IndirectValueTest, MutableAccess) {
  IndirectValue<int> v = 42;
  EXPECT_EQ(*v, 42);
  EXPECT_EQ(v.GetPointer(), &*v);
  *v = 0;
  EXPECT_EQ(*v, 0);
}

struct NonMovable {
  NonMovable(int i) : i(i) {}
  NonMovable(NonMovable&&) = delete;
  auto operator=(NonMovable&&) -> NonMovable& = delete;

  int i;
};

TEST(IndirectValueTest, Create) {
  IndirectValue<NonMovable> v =
      CreateIndirectValue([] { return NonMovable(42); });
  EXPECT_EQ(v->i, 42);
}

auto GetIntReference() -> const int& {
  static int i = 42;
  return i;
}

TEST(IndirectValueTest, CreateWithDecay) {
  auto v = CreateIndirectValue(GetIntReference);
  EXPECT_TRUE((std::is_same_v<decltype(v), IndirectValue<int>>));
  EXPECT_EQ(*v, 42);
}

struct TestValue {
  TestValue() : state("default constructed") {}
  TestValue(const TestValue& rhs) : state("copy constructed") {}
  TestValue(TestValue&& other) : state("move constructed") {
    other.state = "move constructed from";
  }
  auto operator=(const TestValue&) -> TestValue& {
    state = "copy assigned";
    return *this;
  }
  auto operator=(TestValue&& other) -> TestValue& {
    state = "move assigned";
    other.state = "move assigned from";
    return *this;
  }

  std::string state;
};

TEST(IndirectValueTest, ConstArrow) {
  const IndirectValue<TestValue> v;
  EXPECT_EQ(v->state, "default constructed");
}

TEST(IndirectValueTest, MutableArrow) {
  IndirectValue<TestValue> v;
  EXPECT_EQ(v->state, "default constructed");
  v->state = "explicitly set";
  EXPECT_EQ(v->state, "explicitly set");
}

TEST(IndirectValueTest, CopyConstruct) {
  IndirectValue<TestValue> v1;
  auto v2 = v1;
  EXPECT_EQ(v1->state, "default constructed");
  EXPECT_EQ(v2->state, "copy constructed");
}

TEST(IndirectValueTest, CopyAssign) {
  IndirectValue<TestValue> v1;
  IndirectValue<TestValue> v2;
  v2 = v1;
  EXPECT_EQ(v1->state, "default constructed");
  EXPECT_EQ(v2->state, "copy assigned");
}

TEST(IndirectValueTest, MoveConstruct) {
  IndirectValue<TestValue> v1;
  auto v2 = std::move(v1);
  EXPECT_EQ(v1->state, "move constructed from");
  EXPECT_EQ(v2->state, "move constructed");
}

TEST(IndirectValueTest, MoveAssign) {
  IndirectValue<TestValue> v1;
  IndirectValue<TestValue> v2;
  v2 = std::move(v1);
  EXPECT_EQ(v1->state, "move assigned from");
  EXPECT_EQ(v2->state, "move assigned");
}

TEST(IndirectValueTest, IncompleteType) {
  struct S {
    std::optional<IndirectValue<S>> v;
  };

  S s = {.v = S{}};
}

}  // namespace
}  // namespace Cocktail