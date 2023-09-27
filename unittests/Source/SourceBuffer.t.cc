#include "Cocktail/Source/SourceBuffer.h"

#include <gtest/gtest.h>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/Support/VirtualFileSystem.h"

namespace Cocktail {
namespace {

static constexpr llvm::StringLiteral TestFileName = "test.cocktail";

TEST(SourceBufferTest, MissingFile) {
  llvm::vfs::InMemoryFileSystem fs;
  auto buffer = SourceBuffer::CreateFromFile(fs, TestFileName,
                                             ConsoleDiagnosticConsumer());
  EXPECT_FALSE(buffer);
}

}  // namespace
}  // namespace Cocktail