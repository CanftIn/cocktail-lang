#ifndef COCKTAIL_TESTING_RAW_OSTREAM_T_H
#define COCKTAIL_TESTING_RAW_OSTREAM_T_H

#include <gtest/gtest.h>

#include <string>

#include "Cocktail/Common/Ostream.h"

namespace Cocktail::Testing {

class TestRawOstream : public llvm::raw_string_ostream {
 public:
  explicit TestRawOstream() : llvm::raw_string_ostream(buffer_) {}

  ~TestRawOstream() override {
    if (!buffer_.empty()) {
      ADD_FAILURE() << "Unchecked output:\n" << buffer_;
    }
  }

  auto TakeStr() -> std::string {
    std::string result = std::move(buffer_);
    buffer_.clear();
    return result;
  }

 private:
  std::string buffer_;
};

}  // namespace Cocktail::Testing

#endif  // COCKTAIL_TESTING_RAW_OSTREAM_T_H