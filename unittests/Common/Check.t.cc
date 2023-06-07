#include "Cocktail/Common/Check.h"

#include <gtest/gtest.h>

namespace {

using namespace Cocktail;

TEST(CheckTest, CheckTrue) { COCKTAIL_CHECK(true); }

TEST(CheckTest, CheckFalse) {
  ASSERT_DEATH({ COCKTAIL_CHECK(false); },
               "CHECK failure at " + std::string(__FILE__) + ":12: false");
}

TEST(CheckTest, CheckTrueCallbackNotUsed) {
  bool called = false;
  auto callback = [&]() {
    called = true;
    return "called";
  };
  COCKTAIL_CHECK(true) << callback();
  EXPECT_FALSE(called);
}

TEST(CheckTest, CheckFalseMessage) {
  ASSERT_DEATH({ COCKTAIL_CHECK(false) << "msg"; },
               "CHECK failure at " + std::string(__FILE__) + ":27: false: msg");
}

TEST(CheckTest, CheckOutputForms) {
  const char msg[] = "msg";
  std::string str = "str";
  int i = 1;
  COCKTAIL_CHECK(true) << msg << str << i << 0;
}

TEST(CheckTest, Fatal) {
  ASSERT_DEATH({ COCKTAIL_FATAL() << "msg"; },
               "FATAL failure at " + std::string(__FILE__) + ":39: msg");
}

}  // namespace