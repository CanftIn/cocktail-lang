#include "Cocktail/Common/VLog.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::testing::IsEmpty;
using ::testing::StrEq;

class VLogger {
 public:
  explicit VLogger(bool enable) : buffer_(buffer_str_) {
    if (enable) {
      vlog_stream_ = &buffer_;
    }
  }

  auto VLog() { COCKTAIL_VLOG() << "Test\n"; }

  auto buffer() -> llvm::StringRef { return buffer_str_; }

 private:
  std::string buffer_str_;
  llvm::raw_string_ostream buffer_;
  llvm::raw_ostream* vlog_stream_ = nullptr;
};

TEST(VLogTest, Enabled) {
  VLogger vlog(true);
  vlog.VLog();
  EXPECT_THAT(vlog.buffer(), StrEq("Test\n"));
}

TEST(VLogTest, Disabled) {
  VLogger vlog(false);
  vlog.VLog();
  EXPECT_THAT(vlog.buffer(), IsEmpty());
}

}  // namespace