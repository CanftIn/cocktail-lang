#include "Cocktail/Common/Check.h"

#include <gtest/gtest.h>

namespace {

using namespace Cocktail;

TEST(CheckTest, CheckTrue) { CHECK(true); }

TEST(CheckTest, CheckFalse) {
  ASSERT_DEATH({ CHECK(false); }, "CHECK failure: false\n");
}

TEST(CheckTest, CheckTrueCallbackNotUsed) {
  bool called = false;
  auto callback = [&]() {
    called = true;
    return "called";
  };
  CHECK(true) << callback();
  EXPECT_FALSE(called);
}

TEST(CheckTest, CheckFalseMessage) {
  ASSERT_DEATH({ CHECK(false) << "msg"; }, "CHECK failure: false: msg\n");
}

TEST(CheckTest, CheckOutputForms) {
  const char msg[] = "msg";
  std::string str = "str";
  int i = 1;
  CHECK(true) << msg << str << i << 0;
}

TEST(CheckTest, Fatal) {
  ASSERT_DEATH({ FATAL() << "msg"; }, "FATAL: msg\n");
}

auto FatalNoReturnRequired() -> int { FATAL() << "msg"; }

TEST(ErrorTest, FatalNoReturnRequired) {
  ASSERT_DEATH({ FatalNoReturnRequired(); }, "FATAL: msg\n");
}

}  // namespace