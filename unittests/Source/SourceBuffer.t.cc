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

TEST(SourceBufferTest, SimpleFile) {
  llvm::vfs::InMemoryFileSystem fs;
  COCKTAIL_CHECK(fs.addFile(TestFileName, /*ModificationTime=*/0,
                            llvm::MemoryBuffer::getMemBuffer("Hello World")));

  auto buffer = SourceBuffer::CreateFromFile(fs, TestFileName,
                                             ConsoleDiagnosticConsumer());
  ASSERT_TRUE(buffer);

  EXPECT_EQ(TestFileName, buffer->filename());
  EXPECT_EQ("Hello World", buffer->text());
}

TEST(SourceBufferTest, NoNull) {
  llvm::vfs::InMemoryFileSystem fs;
  static constexpr char NoNull[3] = {'a', 'b', 'c'};
  COCKTAIL_CHECK(fs.addFile(
      TestFileName, /*ModificationTime=*/0,
      llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(NoNull, sizeof(NoNull)),
                                       /*BufferName=*/"",
                                       /*RequiresNullTerminator=*/false)));

  auto buffer = SourceBuffer::CreateFromFile(fs, TestFileName,
                                             ConsoleDiagnosticConsumer());
  ASSERT_TRUE(buffer);

  EXPECT_EQ(TestFileName, buffer->filename());
  EXPECT_EQ("abc", buffer->text());
}

TEST(SourceBufferTest, EmptyFile) {
  llvm::vfs::InMemoryFileSystem fs;
  COCKTAIL_CHECK(fs.addFile(TestFileName, /*ModificationTime=*/0,
                            llvm::MemoryBuffer::getMemBuffer("")));

  auto buffer = SourceBuffer::CreateFromFile(fs, TestFileName,
                                             ConsoleDiagnosticConsumer());
  ASSERT_TRUE(buffer);

  EXPECT_EQ(TestFileName, buffer->filename());
  EXPECT_EQ("", buffer->text());
}

}  // namespace
}  // namespace Cocktail