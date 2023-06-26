#include "Cocktail/Diagnostics/DiagnosticKind.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/Support/raw_ostream.h"

namespace {

using namespace Cocktail;

TEST(DiagnosticKindTest, Name) {
  std::string buffer;
  llvm::raw_string_ostream(buffer) << DiagnosticKind::TestDiagnostic;
  EXPECT_EQ(buffer, "TestDiagnostic");
}

}  // namespace