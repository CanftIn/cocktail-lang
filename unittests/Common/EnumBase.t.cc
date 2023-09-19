#include "Cocktail/Common/EnumBase.h"

#include <gtest/gtest.h>

namespace Cocktail {
namespace {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(TestKind, uint8_t) {
#define COCKTAIL_ENUM_BASE_TEST_KIND(Name) COCKTAIL_RAW_ENUM_ENUMERATOR(Name)
#include "EnumBase.t.def"
};

class TestKind : public COCKTAIL_ENUM_BASE(TestKind) {
 public:
#define COCKTAIL_ENUM_BASE_TEST_KIND(Name) \
  COCKTAIL_ENUM_CONSTANT_DECLARATION(Name)
#include "EnumBase.t.def"

  using EnumBase::AsInt;
  using EnumBase::FromInt;
};

#define COCKTAIL_ENUM_BASE_TEST_KIND(Name) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(TestKind, Name)
#include "EnumBase.t.def"

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(TestKind) = {
#define COCKTAIL_ENUM_BASE_TEST_KIND(Name) COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "EnumBase.t.def"
};

static_assert(sizeof(TestKind) == sizeof(uint8_t),
              "Class size doesn't match enum size!");

TEST(EnumBaseTest, NamesAndConstants) {
  EXPECT_EQ("Beep", TestKind::Beep.name());
  EXPECT_EQ("Boop", TestKind::Boop.name());
  EXPECT_EQ("Burr", TestKind::Burr.name());
}

TEST(EnumBaseTest, Printing) {
  std::string s;
  llvm::raw_string_ostream stream(s);

  TestKind kind = TestKind::Beep;
  stream << kind << " " << TestKind::Beep;
  kind = TestKind::Boop;
  stream << " " << kind;

  EXPECT_EQ("Beep Beep Boop", s) << "Final kind: " << kind;
}

TEST(EnumBaseTest, Switch) {
  TestKind kind = TestKind::Boop;

  switch (kind) {
    case TestKind::Beep: {
      FAIL() << "Beep case selected!";
      break;
    }
    case TestKind::Boop: {
      EXPECT_EQ("Boop", kind.name());
      break;
    }
    case TestKind::Burr: {
      FAIL() << "Burr case selected!";
      break;
    }
  }
}

TEST(EnumBaseTest, Comparison) {
  TestKind kind = TestKind::Beep;

  EXPECT_EQ(TestKind::Beep, kind);
  EXPECT_NE(TestKind::Boop, kind);
  EXPECT_LT(kind, TestKind::Boop);
  EXPECT_GT(TestKind::Burr, kind);
  EXPECT_LE(kind, TestKind::Beep);
  EXPECT_GE(TestKind::Beep, kind);

  constexpr TestKind Kind2 = TestKind::Beep;
  static_assert(Kind2 == TestKind::Beep);
  static_assert(Kind2 != TestKind::Boop);
  static_assert(Kind2 < TestKind::Boop);
  static_assert(!(Kind2 > TestKind::Burr));
  static_assert(Kind2 <= TestKind::Beep);
  static_assert(!(Kind2 >= TestKind::Burr));
}

TEST(EnumBaseTest, IntConversion) {
  EXPECT_EQ(0, TestKind::Beep.AsInt());
  EXPECT_EQ(1, TestKind::Boop.AsInt());
  EXPECT_EQ(2, TestKind::Burr.AsInt());

  EXPECT_EQ(TestKind::Beep, TestKind::FromInt(0));
  EXPECT_EQ(TestKind::Boop, TestKind::FromInt(1));
  EXPECT_EQ(TestKind::Burr, TestKind::FromInt(2));
}

}  // namespace

}  // namespace Cocktail